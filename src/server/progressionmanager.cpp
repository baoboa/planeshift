/*
 * progressionmanager.cpp
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <stdlib.h>
#include "globals.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>
#include <csutil/scfstr.h>
#include <iengine/engine.h>
#include <iutil/cfgmgr.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/skillcache.h"
#include "util/eventmanager.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/psxmlparser.h"

#include "rpgrules/factions.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/pstrainerinfo.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/pstrait.h"

#include "net/messages.h"
#include "net/npcmessages.h"

#include "engine/psworld.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "clients.h"
#include "psserver.h"
#include "events.h"
#include "psserverchar.h"
#include "playergroup.h"
#include "gem.h"
#include "progressionmanager.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "spellmanager.h"
#include "weathermanager.h"
#include "actionmanager.h"
#include "workmanager.h"
#include "npcmanager.h"
#include "usermanager.h"
#include "introductionmanager.h"
#include "adminmanager.h"

ProgressionManager::ProgressionManager(ClientConnectionSet* ccs, CacheManager* cachemanager)
{
    clients      = ccs;
    cacheManager = cachemanager;

    calc_dynamic_experience = psserver->GetMathScriptEngine()->FindScript("Calculate Dynamic Experience");
    progressionRequiresTraining = false;
    progressionMaxStatValue = 0;
}


ProgressionManager::~ProgressionManager()
{
    //do nothing
}

bool ProgressionManager::Initialize(iObjectRegistry* object_reg)
{
    Subscribe(&ProgressionManager::HandleSkill, MSGTYPE_GUISKILL, REQUIRE_READY_CLIENT);
    Subscribe(&ProgressionManager::HandleDeathEvent, MSGTYPE_DEATH_EVENT, NO_VALIDATION);
    Subscribe(&ProgressionManager::HandleZPointEvent, MSGTYPE_ZPOINT_EVENT, REQUIRE_READY_CLIENT);

    Result result_affinitycategories(db->Select("SELECT * from char_create_affinity"));

    if(result_affinitycategories.IsValid())
    {
        for(unsigned int x = 0; x < result_affinitycategories.Count(); x++)
        {
            affinitycategories.Put(csString(result_affinitycategories[(unsigned long)x]["category"]).Downcase() , csString(result_affinitycategories[(unsigned long)x]["attribute"]).Downcase());
        }
    }

    // Read config: allow practice without training
    csRef<iConfigManager> configmanager =  csQueryRegistry<iConfigManager> (object_reg);
    if(!configmanager)
        return false;

    progressionRequiresTraining = psserver->GetCacheManager()->getOption("progression:requiretraining")->getValueAsInt()==1 ? true : false;
    progressionMaxSkillValue = psserver->GetCacheManager()->getOption("progression:maxskillvalue")->getValueAsInt();
    if (!progressionMaxSkillValue)
        progressionMaxSkillValue = 200;
    progressionMaxStatValue = psserver->GetCacheManager()->getOption("progression:maxstatvalue")->getValueAsInt();
    if (!progressionMaxStatValue)
        progressionMaxStatValue = 400;

    if(!calc_dynamic_experience)
    {
        Error1("Could not find mathscript 'Calculate Dynamic Experience'");
    }
    return true;
}


void ProgressionManager::HandleZPointEvent(MsgEntry* me, Client* client)
{
    psZPointsGainedEvent evt(me);

    csString string;
    string.Format("You've gained some practice points in %s.", evt.skillName.GetData());
    psserver->SendSystemInfo(evt.actor->GetClientID(), string);
    if(evt.rankUp)
    {
        string.Format("You've ranked up in %s!", evt.skillName.GetData());
        psserver->SendSystemResult(evt.actor->GetClientID(), string);
    }

    SendSkillList(client, false, PSSKILL_NONE, client->GetCharacterData()->GetTrainer() != NULL);
}


void ProgressionManager::HandleDeathEvent(MsgEntry* me, Client* notused)
{
    Debug1(LOG_COMBAT, me->clientnum,"Progression Manager handling Death Event\n");
    psDeathEvent evt(me);

    // Only award experience if the dead actor is a NPC and not a pet, or a GM with the givekillexp flag.
    if(evt.killer && ((evt.deadActor->GetClientID()==0 && !evt.deadActor->GetCharacterData()->IsPet()) || evt.deadActor->givekillexp))
    {
        AllocateKillDamage(evt.deadActor, evt.deadActor->GetCharacterData()->GetKillExperience());
    }
}

void ProgressionManager::AllocateKillDamage(gemActor* deadActor, int exp)
{
    csArray<gemActor*> attackers;

    // Last timestamp, used for breaking the loop when > 10 secs had gone
    unsigned int lastTimestamp = 0;
    float        totalDamage   = 0; // The denominator for the percentages

    int i;
    // First build list of attackers and determine how far to go back and what total dmg was.
    for(i = (int) deadActor->GetDamageHistoryCount(); i > 0; i--)
    {
        AttackerHistory* history = deadActor->GetDamageHistory(i-1);

        // 15 secs passed
        if(lastTimestamp - history->TimeOfAttack() > 15000 && lastTimestamp != 0)
        {
            Debug1(LOG_COMBAT, 0, "15 secs passed between hits, breaking loop\n");
            break;
        }
        lastTimestamp = history->TimeOfAttack();

        totalDamage += history->Damage();

        bool found = false;

        gemActor* attacker = history->Attacker();
        if(!attacker)
            continue;  // This attacker has disconnected since he did this damage.

        // Have we already added that player?
        for(size_t j = 0; j < attackers.GetSize(); j++)
        {
            if(attackers[j] == attacker)
            {
                found = true;
                break;
            }
        }

        // New player, add to list
        if(!found)
        {
            attackers.Push(attacker);
        }
    }
    int lastHistory = i;

    for(size_t i = 0; i < attackers.GetSize(); i++)
    {
        gemActor* attacker = attackers[i];

        float dmgMade = 0;
        float mod = 0;

        for(int j = (int) deadActor->GetDamageHistoryCount(); j > lastHistory; j--)
        {
            AttackerHistory* history = deadActor->GetDamageHistory(j-1);
            if(history->Attacker() == attacker)
            {
                dmgMade += history->Damage();
            }
        }

        if(!totalDamage)
        {
            Error2("%s was found to have zero totalDamage in damagehistory!", deadActor->GetName());
            continue;
        }
        // Use the latest HP (needs to be redesigned when NPCs can cast heal spells on eachoter)
        mod = dmgMade / totalDamage; // Get a 0.something value or 1 if we did all dmg
        if(mod > 1.0)
            mod = 1.0;

        int final = 0;
        if(exp <= 0) //use automatically generated experience if exp doesn't have a valid value
        {
            MathEnvironment env;
            env.Define("Killer",    attacker);
            env.Define("DeadActor", deadActor);
            calc_dynamic_experience->Evaluate(&env);
            final = env.Lookup("Exp")->GetValue();
        }
        else
        {
            final = exp;
        }

        final *= mod;

        // Reward player with final experience points and notify client.
        attacker->GetCharacterData()->AddExperiencePointsNotify(final);
    }

    deadActor->ClearDamageHistory();
}

void ProgressionManager::HandleSkill(MsgEntry* me, Client* client)
{
    psGUISkillMessage msg(me);
    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psGUISkillMessage from client %u.\n",me->clientnum);
        return;
    }

    //    CPrintf(CON_DEBUG, "ProgressionManager::HandleSkill(%d,%s)\n",msg.command, (const char*)msg.commandData);
    switch(msg.command)
    {
        case psGUISkillMessage::REQUEST:
        {
            // Clear the current skill cache
            psCharacter* character = client->GetCharacterData();
            if(character)
                character->GetSkillCache()->clear();

            SendSkillList(client, true);
            break;
        }
        case psGUISkillMessage::SKILL_SELECTED:
        {
            csRef<iDocumentSystem> xml;
            xml.AttachNew(new csTinyDocumentSystem);

            CS_ASSERT(xml);

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse(msg.commandData);
            if(error)
            {
                Error2("Error in XML: %s", error);
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("S");
            if(!topNode)
            {
                Error1("No <S> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");

            psSkillInfo* info = cacheManager->GetSkillByName(skillName);
            Faction* faction = cacheManager->GetFactionByName(skillName);
            csString buff;
            csString description;
            int cathegory;
            if(info)
            {
                description = EscpXML(info->description).GetData();
                cathegory = info->category;
            }
            else if(faction)
            {
                description = faction->description;
                cathegory = PSSKILLS_CATEGORY_FACTIONS;
            }
            else
            {
                description = "";
                cathegory = PSSKILLS_CATEGORY_VARIOUS;
            }
            buff.Format("<DESCRIPTION NAME=\"%s\" DESC=\"%s\" CAT=\"%d\"/>",
                        EscpXML(skillName).GetData(), description.GetData(),
                        cathegory);

            psCharacter* chr = client->GetCharacterData();

            MathEnvironment skillVal;
            chr->GetSkillValues(&skillVal);


            psGUISkillMessage newmsg(client->GetClientNum(),
                                     psGUISkillMessage::DESCRIPTION,
                                     buff,
                                     NULL,
                                     (unsigned int)skillVal.Lookup("STR")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("END")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("AGI")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("INT")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("WIL")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("CHA")->GetRoundValue(),
                                     (unsigned int)(chr->GetHP()),
                                     (unsigned int)(chr->GetMana()),
                                     (unsigned int)(chr->GetStamina(true)),
                                     (unsigned int)(chr->GetStamina(false)),
                                     (unsigned int)(chr->GetMaxHP().Current()),
                                     (unsigned int)(chr->GetMaxMana().Current()),
                                     (unsigned int)(chr->GetMaxPStamina().Current()),
                                     (unsigned int)(chr->GetMaxMStamina().Current()),
                                     true,
                                     PSSKILL_NONE,
                                     -1,
                                     client->GetCharacterData()->GetTrainer() != NULL);

            if(newmsg.valid)
                SendMessage(newmsg.msg);
            else
            {
                Bug2("Could not create valid psGUISkillMessage for client %u.\n",client->GetClientNum());
            }
            break;
        }
        case psGUISkillMessage::BUY_SKILL:
        {
            Debug1(LOG_SKILLXP, client->GetClientNum(),"---------------Buying Skill-------------\n");
            csRef<iDocumentSystem> xml;
            xml.AttachNew(new csTinyDocumentSystem);

            CS_ASSERT(xml);

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse(msg.commandData);
            if(error)
            {
                Error2("Error in XML: %s", error);
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("B");
            if(!topNode)
            {
                Error1("No <B> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");
            int skillAmount = topNode->GetAttributeValueAsInt("AMOUNT");

            psSkillInfo* info = cacheManager->GetSkillByName(skillName);
            Debug2(LOG_SKILLXP, client->GetClientNum(),"    Looking for: %s\n", (const char*)skillName);

            if(!info)
            {
                Error2("No skill with name %s found!",skillName.GetData());
                Error2("Full Data Sent from Client was: %s\n", msg.commandData.GetData());
                return;
            }

            psCharacter* character = client->GetCharacterData();

            if(character->GetTrainer() == NULL)
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "Can't buy skills when not training!");
                return;
            }

            gemActor* actorTrainer = character->GetTrainer()->GetActor();
            if(actorTrainer)
            {
                if((character->GetActor()->RangeTo(actorTrainer, false) > RANGE_TO_SELECT) ||
                        (actorTrainer->GetVisibility() == false))
                {
                    psserver->SendSystemInfo(client->GetClientNum(),
                                             "Need to get a bit closer to understand the training.");
                    return;
                }
            }

            if(client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
            {
                csString err;
                err.Format("You can't train while %s.", client->GetActor()->GetModeStr());
                psserver->SendSystemError(client->GetClientNum(), err);
                return;
            }

            Debug2(LOG_SKILLXP, client->GetClientNum(),"    PP available: %u\n", character->GetProgressionPoints());

            // Test for progression points
            if((int)character->GetProgressionPoints() < skillAmount)
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You don't have enough progression points!");
                return;
            }

            // Test for money

            if((info->price * skillAmount) > character->Money())
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You don't have the money to buy this skill!");
                return;
            }
            if(!character->CanTrain(info->id))
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You cannot train this skill any higher yet!");
                return;
            }

            unsigned int current = character->Skills().GetSkillRank((PSSKILL) info->id).Base();
            float faction = actorTrainer->GetRelativeFaction(character->GetActor());
            if(!character->GetTrainer()->GetTrainerInfo()->CanTrainSkill((PSSKILL)info->id, current, faction))
            {
                psserver->SendSystemInfo(client->GetClientNum(),
                                         "You cannot train this skill currently.");
                return;
            }

            //crop skillamount to the real amount needed for training
            Skill &SelectedSkill = character->Skills().Get((PSSKILL) info->id);
            if(skillAmount > SelectedSkill.yCost-SelectedSkill.y)
                skillAmount = SelectedSkill.yCost-SelectedSkill.y;

            character->UseProgressionPoints(skillAmount);
            character->SetMoney(character->Money()-(info->price * skillAmount));
            character->Train(info->id,skillAmount);
            SendSkillList(client, true, info->id, client->GetCharacterData()->GetTrainer() != NULL);
            psserver->GetCharManager()->UpdateItemViews(client->GetClientNum());
            psserver->SendSystemInfo(client->GetClientNum(), "You've received some %s training", skillName.GetData());

            break;
        }
        case psGUISkillMessage::QUIT:
        {
            client->GetCharacterData()->SetTrainer(NULL);
            client->GetCharacterData()->GetSkillCache()->clear();
            break;
        }
    }
}

void ProgressionManager::SendSkillList(Client* client, bool forceOpen, PSSKILL focus, bool isTraining)
{
    psCharacter* character = client->GetCharacterData();
    psCharacter* trainer = character->GetTrainer();
    psTrainerInfo* trainerInfo = NULL;
    float faction = 0.0;
    int selectedSkillCat = -1; //This is used for storing the category of the selected skill
    int selectedSkillNameId = -1; // Name ID value of the selected skill

    // Get the current skill cache
    psSkillCache* skills = character->GetSkillCache();

    skills->setProgressionPoints(character->GetProgressionPoints());

    if(trainer && trainer->GetActor())
    {
        trainerInfo = trainer->GetTrainerInfo();
        faction = trainer->GetActor()->GetRelativeFaction(character->GetActor());
    }

    // for each skill
    for(unsigned int skillID = 0; skillID < cacheManager->GetSkillAmount(); skillID++)
    {
        psSkillInfo* info = cacheManager->GetSkillByID(skillID);
        if(!info)
        {
            Error2("Can't find skill %d",skillID);
            return;
        }

        Skill &charSkill = character->Skills().Get((PSSKILL) skillID);

        // If we are not in training, or skill can be trained
        if(
            !trainerInfo
            ||
            trainerInfo->CanTrainSkill((PSSKILL) skillID, character->Skills().GetSkillRank((PSSKILL) skillID).Base(), faction)
        )
        {

            /* Get the ID value for the skill name string and find the skill
               in the cache. If it can't be found, skip this skill.
            */
            unsigned int skillNameId =
                cacheManager->FindCommonStringID(info->name);

            if(skillNameId == 0)
            {
                Error2("Can't find skill name \"%s\" in common strings", info->name.GetData());
                continue;
            }

            psSkillCacheItem* item = skills->getItemBySkillId(skillID);

            if(info->id == focus)
            {
                selectedSkillNameId = (int)skillNameId;
                selectedSkillCat=info->category;
            }

            unsigned int actualStat = character->Skills().GetSkillRank((PSSKILL) skillID).Current();

            // this skill is already present in the cache
            if(item)
            {
                item->update(charSkill.rank.Base(), actualStat,
                             charSkill.y, charSkill.yCost,
                             charSkill.z, charSkill.zCost);
            }
            // this skill is not present yet in the cache
            else
            {
                item = new psSkillCacheItem(skillID, skillNameId,
                                            charSkill.rank.Base(), actualStat,
                                            charSkill.y, charSkill.yCost,
                                            charSkill.z, charSkill.zCost,
                                            info->category, false);
                skills->addItem(skillID, item);
            }
        }
        // We are training, but this skill is not available for training
        // so remove it from the list and send only the skills the trainer is providing education for
        else if(trainerInfo)
        {
            psSkillCacheItem* item = skills->getItemBySkillId(skillID);
            if(item)
            {
                item->setRemoved(true);
            }
        }
    }

    // get stats values (str, end, agi, ...)
    MathEnvironment statsVal;
    character->GetSkillValues(&statsVal);

    // send stats and skills to the client
    psGUISkillMessage newmsg(client->GetClientNum(),
                             psGUISkillMessage::SKILL_LIST,
                             "",
                             skills,
                             (unsigned int)statsVal.Lookup("STR")->GetRoundValue(),
                             (unsigned int)statsVal.Lookup("END")->GetRoundValue(),
                             (unsigned int)statsVal.Lookup("AGI")->GetRoundValue(),
                             (unsigned int)statsVal.Lookup("INT")->GetRoundValue(),
                             (unsigned int)statsVal.Lookup("WIL")->GetRoundValue(),
                             (unsigned int)statsVal.Lookup("CHA")->GetRoundValue(),
                             (unsigned int)character->GetHP(),
                             (unsigned int)character->GetMana(),
                             (unsigned int)character->GetStamina(true),
                             (unsigned int)character->GetStamina(false),
                             (unsigned int)character->GetMaxHP().Current(),
                             (unsigned int)character->GetMaxMana().Current(),
                             (unsigned int)character->GetMaxPStamina().Current(),
                             (unsigned int)character->GetMaxMStamina().Current(),
                             forceOpen,
                             selectedSkillNameId,
                             selectedSkillCat,
                             isTraining //If we are training the client must know it
                            );

    Debug2(LOG_SKILLXP, client->GetClientNum(),"Sending psGUISkillMessage w/ stats to %d, Valid: ",int(client->GetClientNum()));
    if(newmsg.valid)
    {
        Debug1(LOG_SKILLXP, client->GetClientNum(),"Yes\n");
        SendMessage(newmsg.msg);

    }
    else
    {
        Debug1(LOG_SKILLXP, client->GetClientNum(),"No\n");
        Bug2("Could not create valid psGUISkillMessage for client %u.\n",client->GetClientNum());
    }
}

void ProgressionManager::StartTraining(Client* client, psCharacter* trainer)
{
    client->GetCharacterData()->SetTrainer(trainer);
    SendSkillList(client, true, PSSKILL_NONE, client->GetCharacterData()->GetTrainer() != NULL);
}

ProgressionScript* ProgressionManager::FindScript(char const* name)
{
    return cacheManager->GetProgressionScript(name);
}

void ProgressionManager::QueueEvent(psGameEvent* event)
{
    psserver->GetEventManager()->Push(event);
}

void ProgressionManager::SendMessage(MsgEntry* me)
{
    psserver->GetEventManager()->SendMessage(me);
}

void ProgressionManager::Broadcast(MsgEntry* me)
{
    psserver->GetEventManager()->Broadcast(me, NetBase::BC_EVERYONE);
}

