/*
* pscharquestmgr.cpp
*
* Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2 of the License)
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================


//=============================================================================
// Project Includes
//=============================================================================
#include "../globals.h"
#include "../gem.h"
#include "../cachemanager.h"
#include "../client.h"
#include "../commandmanager.h"

#include "util/psdatabase.h"


#include "pscharquestmgr.h"
#include "pscharacter.h"

//////////////////////////////////////////////////////////////////////////
// QuestAssignment accessor functions to accommodate removal of csWeakRef better
//////////////////////////////////////////////////////////////////////////

csWeakRef<psQuest> &QuestAssignment::GetQuest()
{
    if(!quest.IsValid())
    {
        psQuest* q = psserver->GetCacheManager()->GetQuestByID(quest_id);
        SetQuest(q);
    }
    return quest;
}

void QuestAssignment::SetQuest(psQuest* q)
{
    if(q)
    {
        quest = q;
        quest_id = q->GetID();
    }
}

bool QuestAssignment::IsCompleted()
{
    return status == PSQUEST_COMPLETE;
}

////////////////////////////////////////////////////////////////////////

void psCharacterQuestManager::Initialize(psCharacter* cOwner)
{
    owner = cOwner;
}


psCharacterQuestManager::~psCharacterQuestManager()
{
    while(assignedQuests.GetSize())
    {
        delete assignedQuests.Pop();
    }
    assignedQuests.DeleteAll();
}


QuestAssignment* psCharacterQuestManager::IsQuestAssigned(int id)
{
    for(size_t i=0; i<assignedQuests.GetSize(); i++)
    {
        if(assignedQuests[i]->GetQuest().IsValid() && assignedQuests[i]->GetQuest()->GetID() == id &&
                assignedQuests[i]->status != PSQUEST_DELETE)
            return assignedQuests[i];
    }

    return NULL;
}


int psCharacterQuestManager::GetAssignedQuestLastResponse(size_t i)
{
    if(i<assignedQuests.GetSize())
    {
        return assignedQuests[i]->last_response;
    }
    else
    {
        //could use error message
        return -1; //return no response
    }
}


//if (parent of) quest id is an assigned quest, set its last response
bool psCharacterQuestManager::SetAssignedQuestLastResponse(psQuest* quest, int response, gemObject* npc)
{
    int id = 0;

    if(quest)
    {
        while(quest->GetParentQuest())  //get highest parent
        {
            quest= quest->GetParentQuest();
        }
        id = quest->GetID();
    }
    else
        return false;


    for(size_t i=0; i<assignedQuests.GetSize(); i++)
    {
        if(assignedQuests[i]->GetQuest().IsValid() && assignedQuests[i]->GetQuest()->GetID() == id &&
                assignedQuests[i]->status == PSQUEST_ASSIGNED && !assignedQuests[i]->GetQuest()->GetParentQuest())
        {
            assignedQuests[i]->last_response = response;
            assignedQuests[i]->last_response_from_npc_pid = npc->GetPID();
            assignedQuests[i]->dirty = true;
            UpdateQuestAssignments();
            return true;
        }
    }
    return false;
}


size_t  psCharacterQuestManager::GetAssignedQuests(psQuestListMessage &questmsg,int cnum)
{
    if(assignedQuests.GetSize())
    {
        csString quests;
        quests.Append("<quests>");
        csArray<QuestAssignment*>::Iterator iter = assignedQuests.GetIterator();

        while(iter.HasNext())
        {
            QuestAssignment* assigned_quest = iter.Next();
            // exclude deleted
            if(assigned_quest->status == PSQUEST_DELETE || !assigned_quest->GetQuest().IsValid())
                continue;
            // exclude substeps
            if(assigned_quest->GetQuest()->GetParentQuest())
                continue;

            csString item;
            csString escpxml_image = EscpXML(assigned_quest->GetQuest()->GetImage());
            csString escpxml_name = EscpXML(assigned_quest->GetQuest()->GetName());
            item.Format("<q><image icon=\"%s\" /><desc text=\"%s\" /><id text=\"%d\" /><status text=\"%c\" /></q>",
                        escpxml_image.GetData(),
                        escpxml_name.GetData(),
                        assigned_quest->GetQuest()->GetID(),
                        assigned_quest->status);
            quests.Append(item);
        }
        quests.Append("</quests>");
        Debug2(LOG_QUESTS, owner->GetPID().Unbox(), "QuestMsg was %s\n", quests.GetData());
        questmsg.Populate(quests,cnum);
    }
    return assignedQuests.GetSize();
}


QuestAssignment* psCharacterQuestManager::AssignQuest(psQuest* quest, PID assigner_id)
{
    CS_ASSERT(quest);    // Must not be NULL

    QuestAssignment* q = IsQuestAssigned(quest->GetID());
    if(!q)   // make new entry if needed, reuse if old
    {
        q = new QuestAssignment;
        q->SetQuest(quest);
        q->status = PSQUEST_DELETE;

        assignedQuests.Push(q);
    }

    if(q->status != PSQUEST_ASSIGNED)
    {
        q->dirty  = true;
        q->status = PSQUEST_ASSIGNED;
        q->lockout_end = 0;
        q->assigner_id = assigner_id;
        //set last response to current response only if this is the top parent
        q->last_response = quest->GetParentQuest() ? -1 : owner->GetLastResponse();    //This should be the response given when starting this quest
        q->completionOrder = 0;  //this rappresents the default.

        // assign any skipped parent quests
        if(quest->GetParentQuest() && !IsQuestAssigned(quest->GetParentQuest()->GetID()))
            AssignQuest(quest->GetParentQuest(),assigner_id);

        // assign any skipped sub quests
        csHash<psQuest*>::GlobalIterator it = psserver->GetCacheManager()->GetQuestIterator();
        while(it.HasNext())
        {
            psQuest* q = it.Next();
            if(q->GetParentQuest())
            {
                if(q->GetParentQuest()->GetID() == quest->GetID())
                    AssignQuest(q,assigner_id);
            }
        }

        q->GetQuest()->SetQuestLastActivatedTime(csGetTicks() / 1000);

        Debug3(LOG_QUESTS, owner->GetPID().Unbox(), "Assigned quest '%s' to player '%s'\n", quest->GetName(), owner->GetCharName());
        UpdateQuestAssignments();
    }
    else
    {
        Debug3(LOG_QUESTS, owner->GetPID().Unbox(), "Did not assign %s quest to %s because it was already assigned.\n", quest->GetName(), owner->GetCharName());
    }

    return q;
}


bool psCharacterQuestManager::CompleteQuest(psQuest* quest)
{
    CS_ASSERT(quest);    // Must not be NULL

    QuestAssignment* q = IsQuestAssigned(quest->GetID());
    QuestAssignment* parent = NULL;

    // substeps are not assigned, so the above check fails for substeps.
    // in this case we check if the parent quest is assigned
    if(!q && quest->GetParentQuest())
    {
        parent = IsQuestAssigned(quest->GetParentQuest()->GetID());
    }

    // create an assignment for the substep if parent is valid
    if(parent)
    {
        q = AssignQuest(quest,parent->assigner_id);
    }

    if(q)
    {
        if(q->status == PSQUEST_DELETE || q->status == PSQUEST_COMPLETE)
        {
            Debug3(LOG_QUESTS, owner->GetPID().Unbox(), "Player '%s' has already completed quest '%s'.  No credit.\n", owner->GetCharName(), quest->GetName());
            return false;  // already completed, so no credit here
        }

        q->dirty  = true;
        q->status = PSQUEST_COMPLETE; // completed
        q->lockout_end = owner->GetTotalOnlineTime() +
                         q->GetQuest()->GetPlayerLockoutTime();
        q->last_response = -1; //reset last response for this quest in case it is restarted

        // Complete all substeps if this is the parent quest
        if(!q->GetQuest()->GetParentQuest())
        {
            // assign any skipped sub quests
            csHash<psQuest*>::GlobalIterator it = psserver->GetCacheManager()->GetQuestIterator();
            while(it.HasNext())
            {
                psQuest* currQuest = it.Next();
                if(currQuest->GetParentQuest())
                {
                    if(currQuest->GetParentQuest()->GetID() == quest->GetID())
                    {
                        QuestAssignment* currAssignment = IsQuestAssigned(currQuest->GetID());
                        if(currAssignment && currAssignment->status == PSQUEST_ASSIGNED)
                            DiscardQuest(currAssignment, true);
                    }
                }
            }

            q->completionOrder = 0;
        }
        else
        {
            //if this is a substep assign the completion order by checking
            //all the entries for the highest number, even though it's possible to cache
            //this in reality this won't happen frequently due to the fact players
            //must trigger it.
            csArray<int> &steps = q->GetQuest()->GetParentQuest()->GetSubQuests();
            unsigned int maxCompletionOrder = 0;

            for(size_t i = 0; i < steps.GetSize(); ++i)
            {

                //Check if the quest is assigned and completed in order to check its completion numbering
                QuestAssignment* stepAssignment = IsQuestAssigned(steps.Get(i));
                if(stepAssignment && stepAssignment->IsCompleted() &&
                        stepAssignment->completionOrder > maxCompletionOrder)
                {
                    maxCompletionOrder = stepAssignment->completionOrder;
                }
            }
            q->completionOrder = maxCompletionOrder+1;
        }

        Debug3(LOG_QUESTS, owner->GetPID().Unbox(), "Player '%s' just completed quest '%s'.\n", owner->GetCharName(), quest->GetName());
        UpdateQuestAssignments();
        return true;
    }

    return false;
}


bool psCharacterQuestManager::DiscardQuest(psQuest* quest, bool force)
{
    QuestAssignment* questassignment = IsQuestAssigned(quest->GetID());
    if(!questassignment)
        return false;
    DiscardQuest(questassignment, force);
    return true;
}


void psCharacterQuestManager::DiscardQuest(QuestAssignment* q, bool force)
{
    CS_ASSERT(q);    // Must not be NULL

    if(force || (q->status != PSQUEST_DELETE && !q->GetQuest()->HasInfinitePlayerLockout()))
    {
        q->dirty = true;
        q->status = PSQUEST_DELETE;  // discarded
        if(q->GetQuest()->HasInfinitePlayerLockout())
            q->lockout_end = 0;
        else
            q->lockout_end = owner->GetTotalOnlineTime() +
                             q->GetQuest()->GetPlayerLockoutTime();
        // assignment entry will be deleted after expiration

        Debug3(LOG_QUESTS, owner->GetPID().Unbox(), "Player '%s' just discarded quest '%s'.\n",
               owner->GetCharName(),q->GetQuest()->GetName());

        UpdateQuestAssignments();
    }
    else
    {
        Debug3(LOG_QUESTS, owner->GetPID().Unbox(),
               "Did not discard %s quest for player %s because it was already discarded or was a one-time quest.\n",
               q->GetQuest()->GetName(),owner->GetCharName());
        // Notify the player that he can't discard one-time quests
        psserver->SendSystemError(owner->GetActor()->GetClient()->GetClientNum(),
                                  "You can't discard this quest, since it can be done just once!");
    }
}


bool psCharacterQuestManager::CheckQuestAssigned(psQuest* quest)
{
    CS_ASSERT(quest);    // Must not be NULL
    QuestAssignment* questAssignment = IsQuestAssigned(quest->GetID());
    if(questAssignment)
    {
        if(questAssignment->status == PSQUEST_ASSIGNED)
            return true;
    }
    return false;
}


bool psCharacterQuestManager::CheckQuestCompleted(psQuest* quest)
{
    CS_ASSERT(quest);    // Must not be NULL
    QuestAssignment* questAssignment = IsQuestAssigned(quest->GetID());
    if(questAssignment)
    {
        return questAssignment->IsCompleted();
    }
    return false;
}


//This incorrectly named function checks if the npc (assigner_id) is supposed to answer
// in the (parent)quest at this moment.
bool psCharacterQuestManager::CheckQuestAvailable(psQuest* quest, PID assigner_id)
{
    CS_ASSERT(quest);    // Must not be NULL

    unsigned int now = csGetTicks() / 1000;

    if(quest->GetParentQuest())
    {
        quest = quest->GetParentQuest();
    }

    bool notify = false;
    if(owner->GetActor()->GetClient())
    {
        notify = psserver->GetCacheManager()->GetCommandManager()->Validate(owner->GetActor()->GetClient()->GetSecurityLevel(), "quest notify");
    }

    //NPC should always answer, if the quest is assigned, no matter who started the quest.
    QuestAssignment* q = IsQuestAssigned(quest->GetID());
    if(q && q->status == PSQUEST_ASSIGNED)
    {
        return true;
    }

    //Since the quest is not assigned, this conversation will lead to starting the quest.
    //Check all assigned quests, to make sure there is no other quest already started by this NPC
    /*****
    for (size_t i=0; i<assignedQuests.GetSize(); i++)
    {
        if (assignedQuests[i]->GetQuest().IsValid() && assignedQuests[i]->assigner_id == assigner_id &&
            assignedQuests[i]->GetQuest()->GetID() != quest->GetID() &&
            assignedQuests[i]->GetQuest()->GetParentQuest() == NULL &&
            assignedQuests[i]->status == PSQUEST_ASSIGNED)
        {
            if (notify)
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(),
                                         "GM NOTICE: Quest found, but you already have one assigned from same NPC");
            }

            return false; // Cannot have multiple quests from the same guy
        }
    }
    ********/

    if(q)  //then quest in assigned list, but not PSQUEST_ASSIGNED
    {
        // Character has this quest in completed list. Check if still in lockout
        if(q->GetQuest()->HasInfinitePlayerLockout() ||
                q->lockout_end > owner->GetTotalOnlineTime())
        {
            if(notify)
            {
                if(owner->GetActor()->questtester)  // GM flag
                {
                    psserver->SendSystemInfo(owner->GetActor()->GetClientID(),
                                             "GM NOTICE: Quest (%s) found and player lockout time has been overridden.",
                                             quest->GetName());
                    return true; // Quest is available for GM
                }
                else
                {
                    if(q->GetQuest()->HasInfinitePlayerLockout())
                    {
                        psserver->SendSystemInfo(owner->GetActor()->GetClientID(),
                                                 "GM NOTICE: Quest (%s) found but quest has infinite player lockout.",
                                                 quest->GetName());
                    }
                    else
                    {
                        psserver->SendSystemInfo(owner->GetActor()->GetClientID(),
                                                 "GM NOTICE: Quest (%s) found but player lockout time hasn't elapsed yet. %d seconds remaining.",
                                                 quest->GetName(), q->lockout_end - (owner->GetTotalOnlineTime()));
                    }

                }

            }

            return false; // Cannot have the same quest while in player lockout time.
        }
    }

    // If here, quest is not in assignedQuests, or it is completed and not in player lockout time
    // Player is allowed to start this quest, now check if quest has a lockout
    if(quest->GetQuestLastActivatedTime() &&
            (quest->GetQuestLastActivatedTime() + quest->GetQuestLockoutTime() > now))
    {
        if(notify)
        {
            if(owner->GetActor()->questtester)  // GM flag
            {
                psserver->SendSystemInfo(owner->GetActor()->GetClientID(),
                                         "GM NOTICE: Quest(%s) found; quest lockout time has been overrided",
                                         quest->GetName());
                return true; // Quest is available for GM
            }
            else
                psserver->SendSystemInfo(owner->GetActor()->GetClientID(),
                                         "GM NOTICE: Quest(%s) found, but quest lockout time hasn't elapsed yet. %d seconds remaining.",
                                         quest->GetName(),quest->GetQuestLastActivatedTime()+quest->GetQuestLockoutTime() - now);
        }

        return false; // Cannot start this quest while in quest lockout time.
    }

    return true; // Quest is available
}


int psCharacterQuestManager::NumberOfQuestsCompleted(csString category)
{
    int count=0;
    for(size_t i=0; i<assignedQuests.GetSize(); i++)
    {
        // Character have this quest
        if(assignedQuests[i]->GetQuest().IsValid() && assignedQuests[i]->GetQuest()->GetParentQuest() == NULL &&
                assignedQuests[i]->status == PSQUEST_COMPLETE &&
                assignedQuests[i]->GetQuest()->GetCategory() == category)
        {
            count++;
        }
    }
    return count;
}


bool psCharacterQuestManager::UpdateQuestAssignments(bool force_update)
{
    for(size_t i=0; i<assignedQuests.GetSize(); i++)
    {
        QuestAssignment* q = assignedQuests[i];
        if(q->GetQuest().IsValid() && (q->dirty || force_update))
        {
            // will delete the quest only after the expiration time, so the player cannot get it again immediately
            // If it's a step, We can delete it even though it has inf lockout
            if(q->status == PSQUEST_DELETE &&
                    ((!q->GetQuest()->HasInfinitePlayerLockout() &&
                      (!q->GetQuest()->GetPlayerLockoutTime() || !q->lockout_end ||
                       (q->lockout_end < owner->GetTotalOnlineTime()))) ||
                     q->GetQuest()->GetParentQuest()))   // delete
            {
                db->CommandPump("DELETE FROM character_quests WHERE player_id=%d AND quest_id=%d",
                                owner->GetPID().Unbox(), q->GetQuest()->GetID());

                delete assignedQuests[i];
                assignedQuests.DeleteIndex(i);
                i--;  // reincremented in loop
                continue;
            }

            // Update or create a new entry in DB

            if(!q->dirty)
                continue;


            db->CommandPump("insert into character_quests "
                            "(player_id, assigner_id, quest_id, "
                            "status, remaininglockout, last_response, last_response_npc_id, completionOrder) "
                            "values (%d, %d, %d, '%c', %d, %d, %d, %d) "
                            "ON DUPLICATE KEY UPDATE "
                            "status='%c',remaininglockout=%ld,last_response=%ld,last_response_npc_id=%ld,completionOrder=%d;",
                            owner->GetPID().Unbox(),
                            q->assigner_id.Unbox(),
                            q->GetQuest()->GetID(),
                            q->status,
                            q->lockout_end,
                            q->last_response,
                            q->last_response_from_npc_pid.Unbox(),
                            q->completionOrder,
                            q->status,
                            q->lockout_end,
                            q->last_response,
                            q->last_response_from_npc_pid.Unbox(),
                            q->completionOrder);
            Debug3(LOG_QUESTS, owner->GetPID().Unbox(), "Updated quest info for player %d, quest %d.\n", owner->GetPID().Unbox(), assignedQuests[i]->GetQuest()->GetID());
            assignedQuests[i]->dirty = false;
        }
    }
    return true;
}


bool psCharacterQuestManager::LoadQuestAssignments()
{
    PID pid = owner->GetPID();

    Result result(db->Select("SELECT * FROM character_quests WHERE player_id=%u", pid.Unbox()));
    if(!result.IsValid())
    {
        Error3("Could not load quest assignments for character %u. Error was: %s", pid.Unbox(), db->GetLastError());
        return false;
    }

    unsigned int age = owner->GetTotalOnlineTime();

    for(unsigned int i=0; i<result.Count(); i++)
    {
        QuestAssignment* q = new QuestAssignment;
        q->dirty = false;
        q->SetQuest(psserver->GetCacheManager()->GetQuestByID(result[i].GetInt("quest_id")));
        q->status        = result[i]["status"][0];
        q->lockout_end   = result[i].GetInt("remaininglockout");
        q->assigner_id   = PID(result[i].GetInt("assigner_id"));
        q->last_response = result[i].GetInt("last_response");
        q->last_response_from_npc_pid = PID(result[i].GetInt("last_response_npc_id"));
        q->completionOrder = result[i].GetInt("completionOrder");

        if(!q->GetQuest())
        {
            Error3("Quest %d for player %d not found!", result[i].GetInt("quest_id"), pid.Unbox());
            delete q;
            return false;
        }

        // Sanity check to see if time for completion is withing
        // lockout time.
        if(q->lockout_end > age + q->GetQuest()->GetPlayerLockoutTime())
            q->lockout_end = age + q->GetQuest()->GetPlayerLockoutTime();

        Debug6(LOG_QUESTS, owner->GetPID().Unbox(), "Loaded quest %-40.40s, status %c, lockout %lu, last_response %d, for player %s.\n",
               q->GetQuest()->GetName(),q->status,
               (q->lockout_end > age ? q->lockout_end-age:0),q->last_response, owner->GetCharFullName());
        assignedQuests.Push(q);
    }
    return true;
}


