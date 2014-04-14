/*
* workmanager.cpp
*
* Copyright (C) 2001-2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <math.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"

#include "util/eventmanager.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/mathscript.h"

#include "bulkobjects/dictionary.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psglyph.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psmerchantinfo.h"
#include "bulkobjects/psnpcdialog.h"
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/pstrade.h"
#include "bulkobjects/pssectorinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "workmanager.h"
#include "gem.h"
#include "netmanager.h"
#include "psserver.h"
#include "psserverchar.h"
#include "playergroup.h"
#include "entitymanager.h"
#include "actionmanager.h"
#include "weathermanager.h"
#include "cachemanager.h"
#include "globals.h"
#include "adminmanager.h"
#include "npcmanager.h"

//#define DEBUG_WORKMANAGER         // debugging only
//#define NO_RANDOM_QUALITY         // do not apply randomness to calculations

/*
 *  There are four types of work that can be done:
 *  1. Manufacturing work harvests raw materials.
 *  2. Production work creates new items from raw materials.
 *  3. Lock picking opens locked containers.
 *  4. Cleanup work removes the ownership of abandoned items.
 *
 *  Go to each section of the code for a full description of each process.
**/


//  To add a new constraint you need to:
//    put a reference to the new constraint function in this struct with constraint string and player message
//    declare a new constraint function in the header like: static bool constraintNew();
//    code the constaint function elsewhere in this module
const constraint constraints[] =
{
    // Time of day.
    // Parameter: hh; where hh is hour of 24 hour clock.
    // Example: TIME(12) is noon.
    {WorkManager::constraintTime, "TIME", "You can not do this work at this time of the day!"},

    // People in area.
    // Parameter: n,r; where n is number of people and r is the range.
    // Example: FRIENDS(6,4) is six people within 4.
    {WorkManager::constraintFriends, "FRIENDS", "You need more people for this work!"},

    // Location of player.
    // Parameter: s,x,y,z,r; where s is sector, x is x-coord, y is y-coord, z is z-coord, and r is rotation.
    // Example: LOCATION(3,-10.53,176.36,,) is at [-10.53,176.36] any hight and any direction in sector 3.
    {WorkManager::constraintLocation, "LOCATION","You can not do this work here!"},

    // Player mode.
    // Parameter: mode; where mode is psCharacter mode string.
    // Example: MODE(sitting) is player needs to be sitting as work is started.
    {WorkManager::constraintMode, "MODE","You are not in the right position to complete this work!"},

    // Player gender.
    // Parameter: gender; where gender is psCharacter's gender.
    // Example: GENDER(F) is player needs to be female as work is completed.
    {WorkManager::constraintGender, "GENDER","You are not in the right gender to complete this work!"},

    // Player race.
    // Parameter: race; where race is psCharacter's race.
    // Example: RACE(ylianm) is player needs to be ylianm as work is completed.
    {WorkManager::constraintRace, "RACE","You do not have right racial background to complete this work!"},

    // Array end.
    {NULL, "", ""}
};

//-----------------------------------------------------------------------------

WorkManager::WorkManager(CacheManager* cachemanager, EntityManager* entitymanager)
{
    cacheManager = cachemanager;
    entityManager = entitymanager;
    currentQuality = 1.00;

    Subscribe(&WorkManager::HandleWorkCommand, MSGTYPE_WORKCMD, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&WorkManager::HandleLockPick, MSGTYPE_LOCKPICK, REQUIRE_READY_CLIENT | REQUIRE_ALIVE | REQUIRE_TARGET);
    Subscribe(&WorkManager::StopUseWork, MSGTYPE_CRAFT_CANCEL, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);

    MathScriptEngine* eng = psserver->GetMathScriptEngine();
    calc_repair_rank = eng->FindScript("Calculate Repair Rank");
    calc_repair_time = eng->FindScript("Calculate Repair Time");
    calc_repair_result = eng->FindScript("Calculate Repair Result");
    calc_repair_quality = eng->FindScript("Calculate Repair Quality");
    calc_repair_exp = eng->FindScript("Calculate Repair Experience");
    calc_lockpicking_exp = eng->FindScript("Calculate Lockpicking Experience");
    calc_mining_chance = eng->FindScript("Calculate Mining Odds");
    calc_mining_exp = eng->FindScript("Calculate Mining Experience");
    calc_transform_exp = eng->FindScript("Calculate Transformation Experience");
    calc_transform_practice = eng->FindScript("Calculate Transformation Practice");
    calc_lockpick_time = eng->FindScript("Lockpicking Time");
    calc_transform_apply_skill = eng->FindScript("Calculate Transformation Apply Skill");
    calc_transform_time = eng->FindScript("Calculate Transformation Time");
    calc_combine_quality = eng->FindScript("CombineQuality");

    CS_ASSERT_MSG("Could not load mathscript 'Calculate Repair Rank'", calc_repair_rank);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Repair Time'", calc_repair_time);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Repair Result'", calc_repair_result);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Repair Quality'", calc_repair_quality);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Repair Experience'", calc_repair_exp);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Lockpicking Experience'", calc_lockpicking_exp);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Mining Odds'", calc_mining_chance);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Mining Experience'", calc_mining_exp);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Transformation Experience'", calc_transform_exp);
    CS_ASSERT_MSG("Could not load mathscript 'Calculate Transformation Practice'", calc_transform_practice);
    CS_ASSERT_MSG("Could not load mathscript 'Lockpicking Time'", calc_lockpick_time);
    CS_ASSERT_MSG("Could not load mathscript 'CombineQuality'", calc_combine_quality);
    //optional for now
    //CS_ASSERT_MSG("Could not load mathscript 'Calculate Transformation Apply Skill'", calc_transform_apply_skill);
    //CS_ASSERT_MSG("Could not load mathscript 'Calculate Transformation Time'", calc_transform_time);


    Initialize();
}


WorkManager::~WorkManager()
{
    //do nothing
}

void WorkManager::Initialize()
{
    Result res(db->Select("select * from natural_resources"));

    if(res.IsValid())
    {
        for(unsigned int i=0; i<res.Count(); i++)
        {
            NaturalResource* nr = new NaturalResource;

            nr->sector = res[i].GetInt("loc_sector_id");
            nr->loc.x  = res[i].GetFloat("loc_x");
            nr->loc.y  = res[i].GetFloat("loc_y");
            nr->loc.z  = res[i].GetFloat("loc_z");
            nr->radius = res[i].GetFloat("radius");
            nr->visible_radius = res[i].GetFloat("visible_radius");
            nr->probability = res[i].GetFloat("probability");
            nr->skill = cacheManager->GetSkillByID(res[i].GetInt("skill"));
            nr->skill_level = res[i].GetInt("skill_level");
            nr->item_cat_id = res[i].GetUInt32("item_cat_id");
            nr->item_quality = res[i].GetFloat("item_quality");
            nr->anim = res[i]["animation"];
            nr->anim_duration_seconds = res[i].GetInt("anim_duration_seconds");
            nr->reward = res[i].GetInt("item_id_reward");
            nr->reward_nickname = res[i]["reward_nickname"];

            size_t actionNum = resourcesActions.FindCaseInsensitive(res[i]["action"]);
            if(actionNum == csArrayItemNotFound)
                actionNum = resourcesActions.Push(res[i]["action"]);

            nr->action = actionNum;

            resources.Push(nr);

        }
    }
    else
    {
        Error2("Database error loading natural_resources: %s\n",
               db->GetLastError());
    }
}

void WorkManager::HandleWorkCommand(MsgEntry* me, Client* client)
{
    psWorkCmdMessage msg(me);

    if(msg.command == "use")
    {
        // Check if it's an action location with a script, then pass the job to ActionManager
        gemObject* target = client->GetTargetObject();
        gemActionLocation* gemAction = target->GetALPtr();
        bool examineScript = false;
        if(gemAction)
            examineScript = psserver->GetActionManager()->HandleUse(gemAction, client);

        // otherwise let it handle to WorkManager
        if(!examineScript)
            HandleUse(client);
    }
    else if(msg.command == "combine")
    {
        HandleCombine(client);
    }

    else if(msg.command == "repair")
    {
        HandleRepair(client->GetActor(), client, msg.repairSlotName);
    }
    else if(msg.command == "construct")
    {
        HandleConstruct(client);
    }
    else
    {
        size_t result = resourcesActions.FindCaseInsensitive(msg.command);
        if(result != csArrayItemNotFound)
        {
            HandleProduction(client->GetActor(),result,msg.filter,client);
        }
        else
        {
            psserver->SendSystemError(me->clientnum,"Invalid work command.");
        }
    }
}

void WorkManager::HandleLockPick(MsgEntry* me,Client* client)
{
    gemObject* target = client->GetTargetObject();

    // Check if target is action item
    gemActionLocation* gemAction = target->GetALPtr();
    if(gemAction)
    {
        target = gemAction->GetAction()->GetRealItem();
    }

    // Check target gem and range ignoring Y co-ordinate
    if(!target || client->GetActor()->RangeTo(client->GetTargetObject(), true, true) > RANGE_TO_USE)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You need to be closer to the lock to try this.");
        return;
    }

    // Get item
    psItem* item = target->GetItem();
    if(!item)
    {
        Error1("Found gemItem but no psItem was attached!\n");
        return;
    }

    StartLockpick(client,item);
}


//-----------------------------------------------------------------------------
// Repair
//-----------------------------------------------------------------------------

void WorkManager::HandleRepair(gemActor* actor, Client* client, const csString &repairSlotName)
{
    // Make sure client isn't already busy digging, etc.
    if(actor->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        if(client)
        {
            psserver->SendSystemError(client->GetClientNum(),"You cannot repair anything because you are %s.", actor->GetModeStr());
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,actor->GetEID().Unbox(),"%s cannot repair anything because you are %s.", actor->GetName(), actor->GetModeStr());
        }
        return;
    }

    // Need to have work stamina and generaly not beeing exhausted.
    if(!CheckStamina(actor->GetCharacterData()))
    {
        if(client)
        {
            psserver->SendSystemError(client->GetClientNum(),"You cannot repair because you are too tired.");
        }
        else
        {
            Debug2(LOG_SUPERCLIENT,actor->GetEID().Unbox(),"%s cannot repair because you are too tired.", actor->GetName());
        }
        return;
    }

    // Check for repairable item in precised or default(right hand) slot
    int slotTarget;
    if(repairSlotName.IsEmpty())
        slotTarget = PSCHARACTER_SLOT_RIGHTHAND;
    else
        slotTarget = cacheManager->slotNameHash.GetID(repairSlotName);

    psItem* repairTarget = client->GetCharacterData()->Inventory().GetInventoryItem((INVENTORY_SLOT_NUMBER)slotTarget);
    if(repairTarget==NULL)
    {
        if(slotTarget == -1)
        {
            psserver->SendSystemError(client->GetClientNum(),"The Slot %s doesn't exists.", repairSlotName.GetData());
        }
        else if(repairSlotName.IsEmpty())
        {
            psserver->SendSystemError(client->GetClientNum(),"The Default Slot (Right Hand) is empty.", repairSlotName.GetData());
        }
        else
        {
            psserver->SendSystemError(client->GetClientNum(),"The Slot %s is empty.", repairSlotName.GetData());
        }
        return;
    }
    if(repairTarget->GetItemQuality() >= repairTarget->GetMaxItemQuality())
    {
        psserver->SendSystemError(client->GetClientNum(),"Your %s is in perfect condition.", repairTarget->GetName());
        return;
    }

    // Check for required repair kit item in any inventory slot
    int toolstatid     = repairTarget->GetRequiredRepairTool();
    psItemStats* tool  = cacheManager->GetBasicItemStatsByID(toolstatid);
    psItem* repairTool = NULL;
    if(tool)
    {
        size_t index = client->GetCharacterData()->Inventory().FindItemStatIndex(tool);
        if(index == SIZET_NOT_FOUND)
        {
            psserver->SendSystemError(client->GetClientNum(),"You must have a %s to repair your %s.", tool->GetName(), repairTarget->GetName());
            return;
        }
        repairTool = client->GetCharacterData()->Inventory().GetInventoryIndexItem(index);
        if(repairTool == repairTarget)
        {
            psserver->SendSystemError(client->GetClientNum(), "You can't use your repair tool on itself.");
            return;
        }
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(),"The %s cannot be repaired!", repairTarget->GetName());
        return;
    }

    // Calculate if current skill is enough to repair the item
    int rankneeded;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        calc_repair_rank->Evaluate(&env);
        rankneeded = env.Lookup("Result")->GetRoundValue();
    }

    int skillid = repairTarget->GetBaseStats()->GetCategory()->repairSkillId;
    int repairskillrank = client->GetCharacterData()->Skills().GetSkillRank(PSSKILL(skillid)).Current();
    if(repairskillrank<rankneeded)
    {
        psserver->SendSystemError(client->GetClientNum(),"This item is too complex for your current repair skill. You cannot repair it.");
        return;
    }

    // If skill=0, check if it has at least theoretical training in that skill
    if(repairskillrank == 0 && client->GetCharacterData()->Skills().Get(PSSKILL(skillid)).CanTrain())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You don't have the skill to repair your %s.",repairTarget->GetName());
        return;
    }

    // Calculate time required for repair based on item and skill level
    csTicks repairDuration;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", client->GetCharacterData());
        //update the script if needed. Here we don't protect against bad scripts as before for now.
        calc_repair_time->Evaluate(&env);
        repairDuration = (csTicks)(env.Lookup("Result")->GetValue() * 1000); // convert secs to msec
    }

    // Calculate result after repair
    float repairResult;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", client->GetCharacterData());
        calc_repair_result->Evaluate(&env);
        repairResult = env.Lookup("Result")->GetValue();
    }

    // Queue time event to trigger when repair is complete, if not canceled.
    csVector3 dummy = csVector3(0,0,0);

    psWorkGameEvent* evt = new psWorkGameEvent(this, client->GetActor(),repairDuration,REPAIR,dummy,NULL,client,repairTarget,repairResult);
    psserver->GetEventManager()->Push(evt);  // wake me up when repair is done

    client->GetActor()->SetTradeWork(evt);

    repairTarget->SetInUse(true);

    psserver->SendSystemInfo(client->GetClientNum(),"You start repairing the %s and continue for %d seconds.",repairTarget->GetName(),repairDuration/1000);
    client->GetActor()->SetMode(PSCHARACTER_MODE_WORK);
}

void WorkManager::HandleRepairEvent(psWorkGameEvent* workEvent)
{
    psItem* repairTarget = workEvent->object;

    // We're done work, clear the mode
    workEvent->client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
    repairTarget->SetInUse(false);
    // Check for presence of required tool
    int toolstatid     = repairTarget->GetRequiredRepairTool();
    psItemStats* tool  = cacheManager->GetBasicItemStatsByID(toolstatid);
    psItem* repairTool = NULL;
    if(tool)
    {
        size_t index = workEvent->client->GetCharacterData()->Inventory().FindItemStatIndex(tool);
        if(index == SIZET_NOT_FOUND)
        {
            psserver->SendSystemError(workEvent->client->GetClientNum(),"You must have a %s to repair your %s.", tool->GetName(), repairTarget->GetName());
            return;
        }
        repairTool = workEvent->client->GetCharacterData()->Inventory().GetInventoryIndexItem(index);
        if(repairTool == repairTarget)
        {
            psserver->SendSystemError(workEvent->client->GetClientNum(), "You can't use your repair tool on itself.");
            return;
        }
    }

    // Now consume the item if we need to
    if(repairTarget->GetRequiredRepairToolConsumed() && repairTool)
    {
        // Always assume 1 item is consumed, not stacked
        psItem* item = workEvent->client->GetCharacterData()->Inventory().RemoveItemID(repairTool->GetUID(),1);
        if(item)
        {
            // This message must come first because item is about to be deleted.
            psserver->SendSystemResult(workEvent->client->GetClientNum(),
                                       "You used a %s in your repair work.",
                                       item->GetName());

            cacheManager->RemoveInstance(item);
            psserver->GetCharManager()->UpdateItemViews(workEvent->client->GetClientNum());
        }
    }
    else
    {
        // TODO: Implement decay of quality of repair tool here if not consumed.
    }

    // Calculate resulting qualities after repair
    float resultQuality;
    float resultMaxQuality;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", workEvent->client->GetCharacterData());
        env.Define("RepairAmount", workEvent->repairAmount);
        calc_repair_quality->Evaluate(&env);
        resultMaxQuality = env.Lookup("ResultMaxQ")->GetValue();
        resultQuality = env.Lookup("ResultQ")->GetValue();
    }

    repairTarget->SetItemQuality(resultQuality);
    repairTarget->SetMaxItemQuality(resultMaxQuality);

    psserver->SendSystemResult(workEvent->client->GetClientNum(),
                               "You have repaired your %s to %.0f out of %.0f",
                               repairTarget->GetName(),
                               repairTarget->GetItemQuality(),
                               repairTarget->GetMaxItemQuality());

    // calculate practice points

    int practicePoints;
    float modifier;
    {
        MathEnvironment env;
        env.Define("Object", repairTarget);
        env.Define("Worker", workEvent->client->GetCharacterData());
        env.Define("RepairAmount", workEvent->repairAmount);
        calc_repair_exp->Evaluate(&env);
        practicePoints   = env.Lookup("ResultPractice")->GetRoundValue();
        modifier = env.Lookup("ResultModifier")->GetValue();
    }

    int skillid = repairTarget->GetBaseStats()->GetCategory()->repairSkillId;
    //assigns points and exp
    workEvent->client->GetCharacterData()->CalculateAddExperience((PSSKILL)skillid, practicePoints, modifier);

    repairTarget->Save(false);
}



//-----------------------------------------------------------------------------
// Production
//-----------------------------------------------------------------------------

void WorkManager::HandleProduction(gemActor* actor, size_t type, const char* reward, Client* client)
{
    if(client && !LoadLocalVars(client))
    {
        return;
    }

    // Make sure client isn't already busy digging, etc.
    if(actor->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        if(client)
        {
            psserver->SendSystemError(client->GetClientNum(), "You cannot %s because you are %s.", resourcesActions.Get(type), actor->GetModeStr());
        }
        else
        {
            Debug4(LOG_SUPERCLIENT,actor->GetEID().Unbox(), "%s cannot %s because you are %s.", actor->GetName(), resourcesActions.Get(type), actor->GetModeStr());
        }
        return;
    }

    // Need to have work stamina
    if(!CheckStamina(actor->GetCharacterData()))
    {
        if(client)
        {
            psserver->SendSystemError(client->GetClientNum(), "You cannot %s because you are too tired.", resourcesActions.Get(type));
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,actor->GetEID().Unbox(), "%s cannot %s because you are too tired.", actor->GetName(), resourcesActions.Get(type));
        }
        return;
    }

    csVector3 pos;

    // Make sure they are not in the same loc as the last production.
    // skip this check for NPCs.
    actor->GetLastProductionPos(pos);
    if(client && SameProductionPosition(actor, pos))
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You cannot %s in the same place twice in a"
                                  " row.", resourcesActions.Get(type));
        return;
    }

    // Search for a natural resource at the current player location
    iSector* sector;
    actor->GetPosition(pos, sector);
    csArray<NearNaturalResource> resources = FindNearestResource(sector,pos,type,(reward == NULL || !strcmp(reward,""))? NULL : reward);

    // no resource found
    if(resources.IsEmpty())
    {
        if(client)
        {
            psserver->SendSystemInfo(client->GetClientNum(),"You don't see a good place to %s.",resourcesActions.Get(type));
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,actor->GetEID().Unbox(),"%s doesn't see a good place to %s.",actor->GetName(),resourcesActions.Get(type));
        }
        return;
    }

    // Validate the skill
    // If skill == 0 and can still be trained it's not possible to execute this operation
    // check all the resources to check if one validates, else the resource is removed from the array.
    // if the array ends up empty it means we don't have a resource to attempt because of our skills.
    for(size_t i = 0; i < resources.GetSize(); i++)
    {
        NaturalResource* nr = resources.Get(i).resource;
        if(actor->GetCharacterData()->Skills().GetSkillRank((PSSKILL)nr->skill->id).Current() == 0 &&
                actor->GetCharacterData()->Skills().Get((PSSKILL)nr->skill->id).CanTrain())
        {
            //we can't attempt this resource so we remove it from our array of possibilities
            resources.DeleteIndex(i);
            //reduce the iterator position as we are removing an item.
            i--;
        }
    }

    //if we ended up with an empty array it means we don't have the skill to do any of these
    if(resources.IsEmpty())
    {
        if(client)
        {
            psserver->SendSystemInfo(client->GetClientNum(),"You don't have the skill to %s any resource here.",resourcesActions.Get(type));
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,actor->GetEID().Unbox(),"%s doesn't see a good place to %s.",actor->GetName(),resourcesActions.Get(type));
        }
        return;
    }

    // Validate category of equipped item
    // checks all the resources to see if one validates, else the resource is removed from the array.
    // if the array ends up empty it means we don't have a resource to attempt because of our tools.
    for(size_t i = 0; i < resources.GetSize(); i++)
    {
        NaturalResource* nr = resources.Get(i).resource;
        psItem* item = actor->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
        if(!item || nr->item_cat_id != item->GetCategory()->id)
        {
            //try the left hand
            item = actor->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
            if(!item || nr->item_cat_id != item->GetCategory()->id)
            {
                //we can't attempt this resource so we remove it from our array of possibilities
                resources.DeleteIndex(i);
                //reduce the iterator position as we are removing an item.
                i--;
            }
        }
    }

    //if we ended up with an empty array it means we don't have the tool to do any of these
    if(resources.IsEmpty())
    {
        if(client)
        {
            psserver->SendSystemError(client->GetClientNum(),"You don't have a good tool to %s with, equipped in your hands.",resourcesActions.Get(type));
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,actor->GetEID().Unbox(),"%s don't have a good tool to %s with, equipped in your hand.",actor->GetName(),resourcesActions.Get(type));
        }
        return;
    }

    //get the first resource data for use in time calculation (will be the most difficult to get)
    NaturalResource* nr = resources.Get(0).resource;

    // Calculate time required
    int time_req = nr->anim_duration_seconds;

    // Send anim and confirmation message to client and nearby people
    psOverrideActionMessage msg(0,actor->GetEID(),nr->anim,nr->anim_duration_seconds);
    psserver->GetEventManager()->Multicast(msg.msg, actor->GetMulticastClients(),0,PROX_LIST_ANY_RANGE);

    actor->SetMode(PSCHARACTER_MODE_WORK);

    actor->GetCharacterData()->SetStaminaRegenerationWork(nr->skill->id);
    actor->SetProductionStartPos(pos);

    // Queue up game event for success
    psWorkGameEvent* ev = new psWorkGameEvent(this,actor,time_req*1000,PRODUCTION,pos,&resources,client);
    psserver->GetEventManager()->Push(ev);  // wake me up when digging is done

    actor->SetTradeWork(ev);

    if(client)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You start to %s.",resourcesActions.Get(type));
    }
    else
    {
        Debug4(LOG_SUPERCLIENT,actor->GetEID().Unbox(),"%s start to %s for %s",actor->GetName(),resourcesActions.Get(type),reward);
    }
}

// Function used by super client
// TODO generalize the npcclient for now it has no way to specify a specific type it can only dig
// TODO remove this little bogus function somehow
void WorkManager::HandleProduction(gemActor* actor,const char* restype,const char* reward)
{
    size_t type = resourcesActions.FindCaseInsensitive(restype);
    HandleProduction(actor, type, reward);
}


bool WorkManager::SameProductionPosition(gemActor* actor,
        const csVector3 &startPos)
{
    csVector3 pos;
    iSector* sector;

    actor->GetPosition(pos, sector);

    return ((startPos - pos).SquaredNorm() < 1);
}

csArray<NearNaturalResource> WorkManager::FindNearestResource(iSector* sector, csVector3 &pos, const size_t action,const char* reward)
{
    csArray<NearNaturalResource> nearResources;

    psSectorInfo* playersector= cacheManager->GetSectorInfoByName(sector->QueryObject()->GetName());
    int sectorid = playersector->uid;

    Debug2(LOG_TRADE,0, "Finding nearest resource for %s\n", reward ? reward : "any resource");

    for(size_t i = 0; i < resources.GetSize(); i++)
    {
        NaturalResource* curr=resources[i];
        if(curr->sector==sectorid)
        {
            if(curr->action == action && (!reward || curr->reward_nickname.CompareNoCase(reward)))
            {
                csVector3 diff = curr->loc - pos;
                float dist = diff.Norm();
                // Add the resource if dist is less than radius
                if(dist < curr->visible_radius)
                {
                    nearResources.Push(NearNaturalResource(curr,dist));
                }
            }
        }
    }

    if(nearResources.IsEmpty())
        Debug2(LOG_TRADE,0, "No resource found for %s\n", reward ? reward : "any resource");

    nearResources.Sort();

    for(size_t i= 0; i < nearResources.GetSize(); i++)
    {
        NearNaturalResource &res = nearResources.Get(i);
        Debug6(LOG_TRADE,0, "found res %zu(%s): dist %f, probability %f, level %d\n", i, res.resource->reward_nickname.GetData(), res.dist, res.resource->probability, res.resource->skill_level);
    }

    return nearResources;
}

void WorkManager::HandleProductionEvent(psWorkGameEvent* workEvent)
{
    if(!workEvent->worker.IsValid())  // Worker has disconnected
        return;

    // Make sure clients are in the same loc as they started to dig.
    if(workEvent->client && !SameProductionPosition(workEvent->client->GetActor(),
            workEvent->client->GetActor()->GetProductionStartPos()))
    {
        psserver->SendSystemError(workEvent->client->GetClientNum(),
                                  "You were unsuccessful since you moved away "
                                  "from where you started.");

        // Actor isn't working anymore.
        workEvent->worker->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    //we check the first item only as that's the one we had at the start
    psCharacter* workerchar = workEvent->worker->GetCharacterData();
    psItem* tool = workerchar->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if(!tool || workEvent->nrr.Get(0).resource->item_cat_id != tool->GetCategory()->id)
    {
        tool = workerchar->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        if(!tool || workEvent->nrr.Get(0).resource->item_cat_id != tool->GetCategory()->id)
        {
            if(workEvent->client)
            {
                psserver->SendSystemInfo(workEvent->worker->GetClientID(),"You were unsuccessful since you no longer have the tool.");
            }

            workEvent->worker->SetMode(PSCHARACTER_MODE_PEACE); // Actor isn't working anymore
            return;
        }
    }

    bool successfullProduction = false;   //used for giving experience later.
    size_t resNum = 0;                    //used to store the last resource checked
    //iterate all resources from the most difficult to the most easy to find
    //if all fail bad luck. If we find one we break out of the loop.
    for(size_t resNum = 0; resNum < workEvent->nrr.GetSize(); resNum++)
    {
        float roll = psserver->GetRandom();

        // Get player skill value
        float cur_skill = workerchar->Skills().GetSkillRank((PSSKILL)workEvent->nrr.Get(resNum).resource->skill->id).Current();

        // If skill=0, check if it has at least theoretical training in that skill
        if(cur_skill==0)
        {
            bool fullTrainingReceived = !workerchar->Skills().Get((PSSKILL)workEvent->nrr.Get(resNum).resource->skill->id).CanTrain();
            if(fullTrainingReceived)
                cur_skill = 0.7F; // consider the skill somewhat usable
        }

        // Calculate factor for distance from center of resource
        float dist = workEvent->nrr.Get(resNum).dist;
        float distance = 1 - (dist / workEvent->nrr.Get(resNum).resource->radius);
        if(distance < 0.0) distance = 0.0f;  // Clamp value 0..1

        MathEnvironment env;
        env.Define("Distance",           distance);                                          // Distance from mine to the actual mining
        env.Define("Probability",        workEvent->nrr.Get(resNum).resource->probability);  // Probability of successful mining
        env.Define("ToolQuality",        tool->GetItemQuality());                            // Quality of mining equipment
        env.Define("RequiredToolQuality",workEvent->nrr.Get(resNum).resource->item_quality); // Required quality of mining equipment
        env.Define("PlayerSkill",        cur_skill);                                         // Player skill
        env.Define("RequiredSkill",      workEvent->nrr.Get(resNum).resource->skill_level);  // Required skill

        calc_mining_chance->Evaluate(&env);
        MathVar* varTotal = env.Lookup("Total");
        MathVar* varResultQuality = env.Lookup("ResultQuality");
        float total = varTotal->GetValue();
        float resultQuality = varResultQuality->GetValue();

        csString debug;
        debug.AppendFmt("Distance:    %1.3f\n",distance);
        debug.AppendFmt("Total Factor:    %1.3f\n",total);
        debug.AppendFmt("Result Quality:    %1.3f\n",resultQuality);
        debug.AppendFmt("Roll:            %1.3f\n",roll);
        if(workEvent->client)
        {
            Debug2(LOG_TRADE, workEvent->client->GetClientNum(), "%s", debug.GetData());
        }
        else
        {
            Debug2(LOG_TRADE, 0, "%s", debug.GetData());
        }


        if(roll < total)   // successful!
        {
            psItemStats* newitem = cacheManager->GetBasicItemStatsByID(workEvent->nrr.Get(resNum).resource->reward);
            // Save resourceNick for reporting to the npcclient
            csString resourceNick = workEvent->nrr.Get(resNum).resource->reward_nickname;
            if(!newitem)
            {
                Bug2("Natural Resource reward item #%d not found!\n",workEvent->nrr.Get(resNum).resource->reward);
            }
            else
            {
                psItem* item = newitem->InstantiateBasicItem();
                if(!item)
                {
                    Bug2("Failed instantizing reward item #%d!\n",workEvent->nrr.Get(resNum).resource->reward);
                    return;
                }

                // Set quality based on script result
                item->SetItemQuality(resultQuality);
                item->SetMaxItemQuality(resultQuality);

                if(!workerchar->Inventory().AddOrDrop(item))
                {
                    Debug5(LOG_ANY, workerchar->GetPID().Unbox(), "HandleProductionEvent() could not give item of stat %u (%s) to character %s[%s])",
                           newitem->GetUID(), newitem->GetName(), workerchar->GetCharName(), ShowID(workerchar->GetPID()));

                    if(workEvent->client)
                    {
                        // Assume it's full
                        psserver->SendSystemInfo(workEvent->client->GetClientNum(),"You found %s, but you can't carry anymore of it so you dropped it", newitem->GetName());
                    }
                    else
                    {
                        Debug5(LOG_SUPERCLIENT,workEvent->worker->GetEID().Unbox(),"%s(%s) found %s, but dropped it: %s",workEvent->worker->GetName(),
                               ShowID(workEvent->worker->GetEID()), newitem->GetName(), workerchar->Inventory().lastError.GetDataSafe());
                    }

                }
                else
                {
                    if(workEvent->client)
                    {
                        psserver->GetCharManager()->UpdateItemViews(workEvent->client->GetClientNum());
                        psserver->SendSystemResult(workEvent->client->GetClientNum(),"You've got some %s!", newitem->GetName());
                    }
                    else
                    {
                        Debug4(LOG_SUPERCLIENT,workEvent->worker->GetEID().Unbox(),"%s(%s) got some %s.",workEvent->worker->GetName(),
                               ShowID(workEvent->worker->GetEID()), newitem->GetName());
                    }
                }

                item->SetLoaded();  // Item is fully created
                item->Save(false);    // First save

                if(!workEvent->client)
                {
                    // Inform the npcclient about the item it's npc just got
                    psserver->GetNPCManager()->WorkDoneNotify(workEvent->worker->GetEID(), newitem->GetName(), resourceNick);
                }

                // No matter if the item could be moved to inventory reset the last position and
                // give out skills.
                csVector3 pos;
                iSector* sector;

                workEvent->worker->GetPosition(pos, sector);

                // Store the mining position for the next check.
                if(workEvent->client)
                {
                    workEvent->client->GetActor()->SetLastProductionPos(pos);
                }

                //as we found an ore we set we were successfull and exit from the loop (so we don't give the player
                //more than one ore at a time
                successfullProduction = true;
                break;
            }
        }
    }


    //if we didn't succed in the previous part just tell the player
    if(!successfullProduction)
    {
        if(workEvent->client)
        {
            psserver->SendSystemInfo(workEvent->worker->GetClientID(),"You were not successful.");
        }
        else
        {
            Debug2(LOG_SUPERCLIENT,workEvent->worker->GetEID().Unbox(),"%s where not successful.",workEvent->worker->GetName());
        }
    }

    //Assign experience and practice points
    if(workEvent->client)
    {
        int practicePoints;
        int experiencePoints;
        float modifier;

        // Get player skill value
        float cur_skill = workerchar->Skills().GetSkillRank((PSSKILL)workEvent->nrr.Get(resNum).resource->skill->id).Current();

        // If skill=0, check if it has at least theoretical training in that skill
        if(cur_skill==0)
        {
            bool fullTrainingReceived = !workerchar->Skills().Get((PSSKILL)workEvent->nrr.Get(resNum).resource->skill->id).CanTrain();
            if(fullTrainingReceived)
                cur_skill = 0.7F; // consider the skill somewhat usable
        }

        MathEnvironment env;
        env.Define("Success", successfullProduction);
        env.Define("Worker", workEvent->client->GetCharacterData());
        env.Define("Probability", workEvent->nrr.Get(resNum).resource->probability); // Probability of successful mining
        env.Define("ActionTime", workEvent->nrr.Get(resNum).resource->anim_duration_seconds); // Duration of the mining work.
        env.Define("PlayerSkill", cur_skill);                                         // Player skill
        env.Define("RequiredSkill", workEvent->nrr.Get(resNum).resource->skill_level);  // Required skill

        calc_mining_exp->Evaluate(&env);
        practicePoints   = env.Lookup("ResultPractice")->GetRoundValue();
        modifier = env.Lookup("ResultModifier")->GetValue();
        MathVar* varResult = env.Lookup("Exp"); //optional variable
        if(varResult)
            experiencePoints = varResult->GetRoundValue();
        else
            experiencePoints = 0;


        // assign practice and experience
        if(practicePoints != 0)
        {
            workEvent->client->GetCharacterData()->CalculateAddExperience((PSSKILL)workEvent->nrr.Get(resNum).resource->skill->id, practicePoints, modifier);
        }
        else
        {
            workEvent->client->GetCharacterData()->AddExperiencePointsNotify(experiencePoints);
        }

    }
    else
    {
        //TODO: Fix the code abow to work without using the client all the time.
        Debug2(LOG_SUPERCLIENT,workEvent->worker->GetEID().Unbox(),"%s where not assigned experience for his work. Not implemented yet.",workEvent->worker->GetName());
    }

    workEvent->worker->SetMode(PSCHARACTER_MODE_PEACE); // Actor isn't working anymore
}


//-----------------------------------------------------------------------------
// Manufacture/cleanup
//-----------------------------------------------------------------------------

/*
*    There are four trade DB tables; trade_patterns, trade_processes, trade_transofrmations, and trade_combinations.
* The pattern data represents the intent of the work.  The patterns are divided into group patterns and the specific
* patterns that go with the design items that are placed in the mind slot.  The group patterns allow work to be associated
* with a group of patterns; This saves a lot of effort in creating individual transformations and combinations
* for similar patterns.  The process table holds all the common transform data, such as equipment, animations and skills.
* The transform table holds a list of transforms.  These are the starting item, quantity, and the result item and
* quantity. It also hold the duration of the trasform.  The combination table contains all the one to many items
* that make up a single combination.
*
*    As items are moved into and out of containers the entry functions are called to check
* if work can be done on the items.  Other entry functions are called when /combine or /use
* commands are issued.  Here are the different types of work that can be done:
*
* 1.    Command /use (targeted item) Specific types of items can be targeted and used to change items held
*       in the hand into other items.  Example:  Steel stock can be transformed into shield pieces by holding
*       hot stock in one hand and a hammer in the other while targeting an anvil.
* 2.    Command /use (targeted container) Containers can be targeted and used to change items contained into
*       other items.  Example:  Mechanical mixers can be targeted and used to mix large amounts of batter into dough.
* 3.    Command /combine (targeted container) Containers can be targeted and used to combine items into other items.
*       Example: Coal and iron ore can be combined in a foundry to make super sharp steel.
* 4.    Drag and drop Single items that are put into special auto-transformation containers can be changed into other
*       items.  Example: Heating iron ore in a forge creates iron stock.
* 5.    Issue a progression script to initiate craft tranformation.  Example:  Issue creat reagent spell.
*
*
* A set of member variables is used to guide the work process.  These are usually loaded at the start of the
*    entry functions, before validations functions are called to make sure the player meets the work requirements.
* There are four calls to IsTransformable().  The first is checking for a single transform that matches all the player
* item, tool, and skill criteria.  The second checks the same criteria looking for any possible group transform.  The
* next check is checking equipment and items for patternless transforms.  These transforms are possible without
* any item in the mind slot.  The transforms that allow any item to be transformed come next.  These are used in the
* oven for example where any item can be changed into dust.  Next the any item group transforms are checked.
* transform.  Finally the patternless any item transforms are checked.
*
* The IsTransformable() function will then loop through all the transforms associated with the current design item
*    and the current work item looking for a match.  It will check for required equipment, skills, quantities,
*    training, as well as any specific constraints made on doing the work.
* If a valid transformation is found StartTransformationEvent() is called that creates a work event and
*    returns control back.  A combination transformation causes the set of combining items into a temporary item
*    and then creates starts the transformation for the temporary item.
* Depending on the type of work being done more calls IsTransformable() might be made.  For example, when
*    issuing a /use command on an item both the right and left hand are checked for transformations.
* If no transformations are found an appropriate message is displayed to the user.
* Once the work event is fired skill points are collected and some calculations are done to determine any random
*    changes to the planned result.
* At this point the item is transformed into a different item and the user is informed of the change.
* If the transformation is caused by an auto-transform container then IsTransformable() is called again to
*    see if a second transformation needs to be started.
*
* Practice points are awarded only if skills are indicated in the transformation processes.  Experience
*    is awarded based on the difference of the original quality of the item and the resulting item's quality.
*
* Entry Points:
*    StartAutoWork - Item moved into container: SlotManager::MoveFromInventory()
*                  - Item stack split in container: SlotManager::MoveFromWorldContainer()
*    HandleUse - Client issues /use command from buttons: gemActiveObject::SendBehaviorMessage()
*              - Workmanager gets /use message: WorkManager::HandleWorkCommand()
*    HandleCombine - Client issues /combine command from buttons: gemActiveObject::SendBehaviorMessage()
*                  - Workmanager gets /combine message: WorkManager::HandleWorkCommand()
*    HandleWorkEvent - Work event triggers: psWorkGameEvent::Trigger()
*    StopWork - Items removed from containers: SlotManager::MoveFromWorldContainer();
*
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing work
void WorkManager::StopWork(Client* client, psItem* item)
{
    // Assign the member vars
    if(!LoadLocalVars(client))
    {
        return;
    }

    // Check for bad item pointer
    if(!item)
    {
        Error1("Bad item pointer in StopWork");
        return;
    }

    // if no event for item then return
    psWorkGameEvent* curEvent = item->GetTransformationEvent();
    if(!curEvent)
    {
        return;
    }

    // Check for ending clean up event
    if(curEvent->category == CLEANUP)
    {
        Debug2(LOG_TRADE,clientNum, "Stopping cleaning up %s item.\n", item->GetName());
        StopCleanupWork(client, item);
    }
    else
    {
        Debug3(LOG_TRADE, clientNum, "Player %s stopped working on %s item.\n", ShowID(worker->GetPID()), item->GetName());

        // Handle all the different transformation types
        if(curEvent->GetTransformationType() == TRANSFORMTYPE_AUTO_CONTAINER)
        {
            StopAutoWork(client, item);
        }
        else
        {
            StopUseWork(0, client);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle /use command
void WorkManager::HandleUse(Client* client)
{
    // Assign the member vars
    if(!LoadLocalVars(client))
    {
        return;
    }

    //Check if we are starting or stopping use
    if(owner->GetMode() != PSCHARACTER_MODE_WORK)
    {
        StartUseWork(client);
    }
    else
    {
        StopUseWork(0, client);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some use work
void WorkManager::StartUseWork(Client* client)
{
    // Check to see if we have everything we need to do any trade work
    if(!ValidateWork())
    {
        return;
    }

    // Check to see if we have pattern
    if(!ValidateMind())
    {
        return;
    }

    // Check to see if player has stamina for work
    if(!ValidateStamina(client))
    {
        return;
    }

    // Check for any targeted item or container in hand
    if(!ValidateTarget(client))
    {
        return;
    }

    if(workItem && workItem->GetIsContainer())    // Check if the target is a container
    {
        // cast a gem container to iterate through
        gemContainer* container = dynamic_cast<gemContainer*>(workItem->GetGemObject());
        if(!container)
        {
            Error1("Could not instantiate gemContainer");
            return;
        }

        // Load item array from the container
        csArray<psItem*> itemArray;
        gemContainer::psContainerIterator it(container);
        while(it.HasNext())
        {
            // Only check the stuff that player owns
            psItem* item = it.Next();
            if((item->GetGuardingCharacterID() == owner->GetPID())
                    || (item->GetGuardingCharacterID() == 0))
            {
                itemArray.Push(item);
            }
        }

        // Check for too many items
        if(itemArray.GetSize() > 1)
        {
            // Tell the player they are trying to work on too many items
            SendTransformError(clientNum, TRANSFORM_TOO_MANY_ITEMS);
            return;
        }

        // Only one item from container can be transformed at once
        if(itemArray.GetSize() == 1)
        {
            // Check to see if item can be transformed
            uint32 itemID = itemArray[0]->GetBaseStats()->GetUID();
            int count = itemArray[0]->GetStackCount();

            // Verify there is a valid transformation for the item that was dropped
            unsigned int transMatch = AnyTransform(patterns, patternKFactor, itemID, count);
            if((transMatch == TRANSFORM_MATCH) || (transMatch == TRANSFORM_GARBAGE))
            {
                // Set up event for transformation
                StartTransformationEvent(
                    TRANSFORMTYPE_CONTAINER, PSCHARACTER_SLOT_NONE, count, itemArray[0]->GetItemQuality(), itemArray[0]);
                psserver->SendSystemOK(clientNum,"You start work on %d %s.", itemArray[0]->GetStackCount(), itemArray[0]->GetName());
                psCraftCancelMessage msg;
                msg.SetCraftTime(CalculateEventDuration(trans, process, itemArray[0], worker), clientNum);
                msg.SendMessage();
                return;
            }
            else
            {
                // The transform could not be created so send the reason back to the user.
                SendTransformError(clientNum, transMatch, itemID, count);
                return;
            }
        }
        // nothing (owned by the player) in the container to use
        else
        {
            SendTransformError(clientNum, TRANSFORM_BAD_USE);
        }
    }
    else // check for in hand use
    {
        // Check if player has any transformation items in right hand
        psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
        if(rhand != NULL)
        {
            // Find out if we can do a transformation on items in hand
            uint32 handId = rhand->GetBaseStats()->GetUID();
            int handCount = rhand->GetStackCount();

            // Verify there is a valid transformation for the item that was dropped
            unsigned int rhandMatch = AnyTransform(patterns, patternKFactor, handId, handCount);
            if((rhandMatch == TRANSFORM_MATCH) || (rhandMatch == TRANSFORM_GARBAGE))
            {
                // Set up event for transformation
                StartTransformationEvent(
                    TRANSFORMTYPE_SLOT, PSCHARACTER_SLOT_RIGHTHAND, handCount, rhand->GetItemQuality(), rhand);
                psserver->SendSystemOK(clientNum,"You start work on %d %s.", rhand->GetStackCount(), rhand->GetName());
                psCraftCancelMessage msg;
                msg.SetCraftTime(CalculateEventDuration(trans, process, rhand, worker), clientNum);
                msg.SendMessage();
                return;
            }
            else if(rhandMatch != TRANSFORM_UNKNOWN_ITEM)
            {
                // The transform could not be created so send the reason back to the user.
                SendTransformError(clientNum, rhandMatch, handId, handCount);
                return;
            }
        }

        // Check if player has any transformation items in left hand
        psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        if(lhand != NULL)
        {
            // Find out if we can do a transformation on items in hand
            uint32 handId = lhand->GetBaseStats()->GetUID();
            int handCount = lhand->GetStackCount();

            // Verify there is a valid transformation for the item that was dropped
            unsigned int lhandMatch = AnyTransform(patterns, patternKFactor, handId, handCount);
            if((lhandMatch == TRANSFORM_MATCH) || (lhandMatch == TRANSFORM_GARBAGE))
            {
                // Set up event for transformation
                StartTransformationEvent(
                    TRANSFORMTYPE_SLOT, PSCHARACTER_SLOT_LEFTHAND, handCount, lhand->GetItemQuality(), lhand);
                psserver->SendSystemOK(clientNum,"You start work on %d %s.", lhand->GetStackCount(), lhand->GetName());
                psCraftCancelMessage msg;
                msg.SetCraftTime(CalculateEventDuration(trans, process, lhand, worker), clientNum);
                msg.SendMessage();
                return;
            }
            else if(lhandMatch != TRANSFORM_UNKNOWN_ITEM)
            {
                // The transform could not be created so send the reason back to the user.
                SendTransformError(clientNum, lhandMatch, handId, handCount);
            }
        }

        // Since either hand can fail to transform normally send general message if we get to here
        SendTransformError(clientNum, TRANSFORM_MISSING_ITEM);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing use work
void WorkManager::StopUseWork(MsgEntry* me,Client* client)
{
    // Check for any targeted item or container in hand
    if(!ValidateTarget(client))
    {
        return;
    }

    // Kill the work event if it exists
    if(client->GetActor()->GetMode() == PSCHARACTER_MODE_WORK)
    {
        client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
        psserver->SendSystemOK(client->GetClientNum(),"You stop working.");
    }
}

////////////////////////////////////////////////////////////////////////
// Handle /combine command
void WorkManager::HandleCombine(Client* client)
{
    // Assign the member vars
    if(!LoadLocalVars(client))
    {
        return;
    }

    // Check if we are starting or stopping use
    if(owner->GetMode() != PSCHARACTER_MODE_WORK)
    {
        StartCombineWork(client);
    }
    else
    {
        StopCombineWork(client);
    }
}

////////////////////////////////////////////////////////////////////////
// Check if possible to do some use work
void WorkManager::StartCombineWork(Client* client)
{
    // Check to see if we have everything we need to do any trade work
    if(!ValidateWork())
    {
        return;
    }

    // Check to see if we have pattern
    if(!ValidateMind())
    {
        return;
    }

    // Check for any targeted item or container in hand
    if(!ValidateTarget(client))
    {
        return;
    }

    // Check if targeted item is container
    if(workItem && workItem->GetIsContainer())
    {
        // Combine anything can be combined in container
        CombineWork();
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do combine work if possible
bool WorkManager::CombineWork()
{
    // Find out if anything can be combined in container
    uint32 combinationId = 0;
    int combinationQty = 0;
    int resultQuality = 0;
    if(IsContainerCombinable(combinationId, combinationQty))
    {
        //now we know the result quality as this function is going to destroy it we take a copy
        //TODO: loadlocalvariables is a big nuinseance which should it does unexpected effect to the environment.
        resultQuality = currentQuality;
        // Transform all items in container into the combination item
        psItem* newItem = CombineContainedItem(combinationId, combinationQty, currentQuality, workItem);
        if(newItem)
        {
            //restore the valid current quality
            currentQuality = resultQuality;
            if (!ValidateMind()) //unfortunately the bad loadlocalvariables is damaging this data so we restore it
            {
                return false;
            }
            // Find out if we can do a combination transformation
            unsigned int transMatch = AnyTransform(patterns, patternKFactor, combinationId, combinationQty);
            if((transMatch == TRANSFORM_MATCH) || (transMatch == TRANSFORM_GARBAGE))
            {
                // Set up event for transformation
                if(workItem->GetCanTransform())
                {
                    StartTransformationEvent(
                        TRANSFORMTYPE_AUTO_CONTAINER, PSCHARACTER_SLOT_NONE,
                        combinationQty, currentQuality, newItem);
                }
                else
                {
                    StartTransformationEvent(
                        TRANSFORMTYPE_CONTAINER, PSCHARACTER_SLOT_NONE,
                        combinationQty, currentQuality, newItem);
                }
                psserver->SendSystemOK(clientNum,"You start to work on combining items.");
                return true;
            }
            else
            {
                // The transform could not be created so send "wrong" message and the reason back to the user.
                psserver->SendSystemError(clientNum, "You start to combine items, but could not go any further.");
                SendTransformError(clientNum, transMatch, combinationId, combinationQty);
                return true;
            }
        }
    }
    else
    {
        SendTransformError(clientNum, TRANSFORM_BAD_COMBINATION);
        return false;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing combine work
void WorkManager::StopCombineWork(Client* client)
{
    // Check for any targeted item or container in hand
    if(!ValidateTarget(client))
    {
        return;
    }

    // Kill the work event if it exists
    if(worker->GetMode() == PSCHARACTER_MODE_WORK)
    {
        worker->SetMode(PSCHARACTER_MODE_PEACE);
    }

    // Tell the user
    psserver->SendSystemOK(clientNum,"You stop working.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Handle /construct command
void WorkManager::HandleConstruct(Client* client)
{
    // Assign the member vars
    if(!LoadLocalVars(client))
    {
        return;
    }

    //Check if we are starting or stopping use
    if(owner->GetMode() != PSCHARACTER_MODE_WORK)
    {
        StartConstructWork(client);
    }
    else
    {
        StopConstructWork(client);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some use work
void WorkManager::StartConstructWork(Client* client)
{
    // Check to see if we have everything we need to do any trade work
    if(!ValidateWork())
    {
        return;
    }

    // Check to see if we have pattern
    if(!ValidateMind())
    {
        return;
    }

    // Check for any targeted item or container in hand
    if(!ValidateTarget(client))
    {
        return;
    }

    // Check if targeted item is constructible
    if(workItem && workItem->GetIsConstructible())
    {
        if(workItem->GetIsTrap())
        {
            StartTransformationEvent(TRANSFORMTYPE_TARGET_TO_NPC, PSCHARACTER_SLOT_NONE, 1, workItem->GetItemQuality(), workItem);
            psserver->SendSystemOK(clientNum, "You begin constructing %s.", workItem->GetName());
        }
        else if(workItem->GetIsContainer())
        {
            uint32 combinationId = 0;
            int combinationQty = 0;
            if(IsContainerCombinable(combinationId, combinationQty))
            {
                // Check to see if item can be transformed
                uint32 itemID = workItem->GetBaseStats()->GetUID();
                unsigned int transMatch = AnyTransform(patterns, patternKFactor, itemID, 1);
                if(transMatch == TRANSFORM_MATCH)
                {
                    // Set up event for transformation
                    StartTransformationEvent(
                        TRANSFORMTYPE_SELF_CONTAINER, PSCHARACTER_SLOT_NONE, 1, workItem->GetItemQuality(), workItem);
                    psserver->SendSystemOK(clientNum,"You start work on %s.", workItem->GetName());
                    return;
                }
                else
                {
                    // The transform could not be created so send the reason back to the user.
                    SendTransformError(clientNum, transMatch, itemID, 1);
                    return;
                }
            }
        }
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing combine work
void WorkManager::StopConstructWork(Client* client)
{
    // Check for any targeted item or container in hand
    if(!ValidateTarget(client))
    {
        return;
    }

    // Kill the work event if it exists
    if(worker->GetMode() == PSCHARACTER_MODE_WORK)
    {
        worker->SetMode(PSCHARACTER_MODE_PEACE);
    }

    // Tell the user
    psserver->SendSystemOK(clientNum, "You stop constructing.");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if possible to do some automatic work
void WorkManager::StartAutoWork(Client* client, gemContainer* container, psItem* droppedItem, int count)
{
    // Assign the member vars
    if(!LoadLocalVars(client))
    {
        return;
    }

    if(!droppedItem)
    {
        Error1("Bad item passed to start AutoWork.");
        return;
    }

    // Interrupt the first event of the dropped item if any to handle stacking
    psWorkGameEvent* stackEvent = droppedItem->GetTransformationEvent();
    if(stackEvent)
    {
        droppedItem->SetTransformationEvent(NULL);
        stackEvent->Interrupt();
    }

    // We know it is a container so check if it's a transform container
    if(container->GetCanTransform())
    {
        // Check to see if we have everything we need to do any trade work
        if(!ValidateWork())
        {
            return;
        }

        // Work item is container into which items were dropped
        workItem = container->GetItem();
        autoItem = droppedItem;
        uint32 autoID = autoItem->GetBaseStats()->GetUID();

        // Check to see if we have pattern
        if(ValidateMind())
        {
            // Verify there is a valid transformation for the item that was dropped
            unsigned int transMatch = AnyTransform(patterns, patternKFactor, autoID, count);
            switch(transMatch)
            {
                case TRANSFORM_MATCH:
                {
                    // Set up event for auto transformation
                    StartTransformationEvent(
                        TRANSFORMTYPE_AUTO_CONTAINER, PSCHARACTER_SLOT_NONE, count, autoItem->GetItemQuality(), autoItem);
                    psserver->SendSystemOK(clientNum,"You start work on %d %s.", autoItem->GetStackCount(), autoItem->GetName());
                    return;
                }
                case TRANSFORM_GARBAGE:
                {
                    // Set up event for auto transformation
                    StartTransformationEvent(
                        TRANSFORMTYPE_AUTO_CONTAINER, PSCHARACTER_SLOT_NONE, count, autoItem->GetItemQuality(), autoItem);
                    psserver->SendSystemError(clientNum,"You are not sure what is going to happen to %d %s.", autoItem->GetStackCount(), autoItem->GetName());
                    return;
                }
                default:
                {
                    // The transform could not be created so send the reason back to the user.
                    SendTransformError(clientNum, transMatch, autoID, count);
                    break;
                }
            }
        }
        // If no transformations started go ahead and start a cleanup event
        StartCleanupEvent(TRANSFORMTYPE_AUTO_CONTAINER, client, droppedItem, client->GetActor());
        return;
    }
    // If this is a non-autotransform container go ahead and start a cleanup event
    StartCleanupEvent(TRANSFORMTYPE_CONTAINER, client, droppedItem, client->GetActor());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing automatic work
void WorkManager::StopAutoWork(Client* client, psItem* autoItem)
{
    // Check for proper autoItem
    if(!autoItem)
    {
        Error1("StopAutoWork does not have a good autoItem pointer.");
        return;
    }

    // Stop the auto item's work event
    psWorkGameEvent* workEvent = autoItem->GetTransformationEvent();
    if(workEvent)
    {
        if(!workEvent->GetTransformation())
        {
            Error1("StopAutoWork does not have a good transformation pointer.");
        }

        // Unless it's some garbage item let players know
        if(workEvent->GetTransformation()->GetResultId() > 0)
        {
            psserver->SendSystemOK(clientNum,"You stop working on %s.", autoItem->GetName());
        }
        autoItem->SetTransformationEvent(NULL);
        workEvent->Interrupt();
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns with the result ID and quantity of the combination
//  if work item container has the correct items in the correct amounts
// Note:  This assumes that the combination items array is sorted by
//  resultId and then itemId
bool WorkManager::IsContainerCombinable(uint32 &resultId, int &resultQty)
{
    // cast a gem container to iterate thru
    gemContainer* container = dynamic_cast<gemContainer*>(workItem->GetGemObject());
    if(!container)
    {
        Error1("Could not instantiate gemContainer");
        return false;
    }

    // Load item array from the conatiner
    csArray<psItem*> itemArray;
    gemContainer::psContainerIterator it(container);
    currentQuality = 1.00;
    float minQuality = 300.0;
    float maxQuality = 0.0;
    float totalQuality = 0.0;
    while(it.HasNext())
    {
        // Only check the stuff that player owns or is public
        psItem* item = it.Next();
        if((item->GetGuardingCharacterID() == owner->GetPID())
                || (item->GetGuardingCharacterID() == 0))
        {
            itemArray.Push(item);

            // While we are here find the item with the least quality
            float quality = item->GetItemQuality();
            if(minQuality > quality)
            {
                minQuality = quality;
            }
            if(maxQuality < quality)
            {
                maxQuality = quality;
            }
            totalQuality += quality;
        }
    }

    {
        MathEnvironment env;
        env.Define("MinQuality", minQuality);
        env.Define("MaxQuality", maxQuality);
        env.Define("TotalQuality", totalQuality);
        env.Define("ItemNumber",
                (double)(itemArray.GetSize() > 0 ? itemArray.GetSize() : 1.0));

        calc_combine_quality->Evaluate(&env);
        currentQuality = env.Lookup("Result")->GetRoundValue();
    }

    // Check if specific combination is possible
    if(ValidateCombination(itemArray, resultId, resultQty))
        return true;

    // Check if any combination is possible
    return AnyCombination(itemArray, resultId, resultQty);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks to see if every item in list matches every item in a valid combination
bool WorkManager::ValidateCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty)
{
    // check if player owns anything in conatiner
    size_t itemCount = itemArray.GetSize();
    if(itemCount != 0)
    {
        // Get all possible combinations for those patterns and group
        csArray<csPDelArray<CombinationConstruction>*> combArray;
        csArray<csPDelArray<CombinationConstruction>*> combGroupArray;


        for(size_t i = 0; i < patterns.GetSize(); i++)
        {
            //get the combinations for the pattern
            csPDelArray<CombinationConstruction>* result = cacheManager->FindCombinationsList(patterns.Get(i)->GetId());
            if(result) //if it's not null we add it to the valid results
            {
                combArray.Push(result);
            }
            //get the combinations for the group id
            result = cacheManager->FindCombinationsList(patterns.Get(i)->GetGroupPatternId());
            if(result) //if it's not null we add it to the valid results
            {
                combGroupArray.Push(result);
            }
        }

        if(combArray.IsEmpty())
        {
            // Check for group pattern combinations
            if(combGroupArray.IsEmpty())
            {
                if(secure) psserver->SendSystemInfo(clientNum,"Failed to find any combinations in patterns and groups.");
                return false;
            }
        }

        // Go through all of the combinations looking for exact match
        resultId = 0;
        resultQty = 0;

        // Check all the possible combination in this data set
        for(size_t u = 0; u < combArray.GetSize(); u++)
        {
            if(secure) psserver->SendSystemInfo(clientNum,"Checking combinations for patterns.");
            for(size_t i=0; i<combArray.Get(u)->GetSize(); i++)
            {
                // Check for matching lists
                CombinationConstruction* current = combArray.Get(u)->Get(i);
                if(secure) psserver->SendSystemInfo(clientNum,"Checking combinations for result id %u quantity %d.", current->resultItem, current->resultQuantity);
                if(MatchCombinations(itemArray,current))
                {
                    resultId = current->resultItem;
                    resultQty = current->resultQuantity;
                    if(secure) psserver->SendSystemInfo(clientNum,"Found matching combination.");
                    return true;
                }
            }
        }

        // Check all the possible combination in this data set
        for(size_t u = 0; u < combGroupArray.GetSize(); u++)
        {
            if(secure) psserver->SendSystemInfo(clientNum,"Checking combinations for group patterns.");
            for(size_t i=0; i<combGroupArray.Get(u)->GetSize(); i++)
            {
                // Check for matching lists
                CombinationConstruction* current = combGroupArray.Get(u)->Get(i);
                if(secure) psserver->SendSystemInfo(clientNum,"Checking combinations for result id %u quantity %d.", current->resultItem, current->resultQuantity);
                if(MatchCombinations(itemArray,current))
                {
                    resultId = current->resultItem;
                    resultQty = current->resultQuantity;
                    if(secure) psserver->SendSystemInfo(clientNum,"Found matching group combination.");
                    return true;
                }
            }
        }
    }
    else
    {
        if(secure) psserver->SendSystemInfo(clientNum,"Failed to find any items you own in container.");
        return false;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks to see if every item in list is in the set of ingredients for this pattern
bool WorkManager::AnyCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty)
{
    // check if player owns anything in conatiner
    size_t itemCount = itemArray.GetSize();
    if(itemCount != 0)
    {
        //This is a bit weird but looked more elegant. Essentially we want to first
        //do one run where we only check the patterns then we do a second run
        //where we only check for the groups.
        for(bool groupCheck = false; groupCheck == false; groupCheck = true)
        {
            for(size_t i = 0; i < patterns.GetSize(); i++)
            {
                uint32 activePattern = groupCheck? patterns.Get(i)->GetGroupPatternId() : patterns.Get(i)->GetId();
                // Get list of unique ingredients for this pattern
                csArray<uint32>* uniqueArray = cacheManager->GetTradeTransUniqueByID(activePattern);
                if(uniqueArray == NULL)
                {
                    continue;
                }

                // Go through all of the items making sure they are ingredients
                for(size_t i=0; i<itemArray.GetSize(); i++)
                {
                    psItem* item = itemArray.Get(i);
                    if(uniqueArray->Find(item->GetBaseStats()->GetUID()) == csArrayItemNotFound)
                    {
                        return false;
                    }
                }

                // Now find the patternless transform to get results
                resultId = 0;
                resultQty = 0;

                // Get all unknow item transforms for this pattern
                csPDelArray<psTradeTransformations>* transArray =
                    cacheManager->FindTransformationsList(activePattern, 0);
                if(transArray == NULL)
                {
                    return false;
                }

                // Go thru list of transforms
                for(size_t j=0; j<transArray->GetSize(); j++)
                {
                    // Get first transform with a 0 process ID this indicates processless any ingredient transform
                    psTradeTransformations* trans = transArray->Get(j);
                    if(trans->GetProcessId() == 0)
                    {
                        resultId = trans->GetResultId();
                        resultQty = trans->GetResultQty();
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks for matching combination list to item array
bool WorkManager::MatchCombinations(csArray<psItem*> itemArray, CombinationConstruction* current)
{
    // If the items count match then this is a possible valid combination.
    if(itemArray.GetSize() == current->combinations.GetSize())
    {
        // Setup two arrays for comparison
        csArray<psItem*> itemsMatched;
        csArray<psItem*> itemsLeft = itemArray;

        // Iterate over the items in the construction set looking for matches.
        for(size_t j = 0; j < current->combinations.GetSize(); j++)
        {
            uint32 combId  = current->combinations[j]->GetItemId();
            int combMinQty = current->combinations[j]->GetMinQty();
            int combMaxQty = current->combinations[j]->GetMaxQty();

            // Iterate again over all items left in match set.
            for(size_t z = 0; z < itemsLeft.GetSize(); z++)
            {
                uint32 id =  itemsLeft[z]->GetCurrentStats()->GetUID();
                int stackQty = itemsLeft[z]->GetStackCount();
                if(id ==  combId && stackQty >= combMinQty && stackQty <= combMaxQty)
                {
                    // We have matched the ids and the stack counts are in range.
                    itemsMatched.Push(itemsLeft[z]);
                    itemsLeft.DeleteIndexFast(z);
                    break;
                }
            }
        }

        // If all items matched up then we have a valid combination
        if((itemsLeft.GetSize() == 0) && (itemsMatched.GetSize() == current->combinations.GetSize()))
        {
            return true;
        }
        else
        {
            if(secure)
            {
                psserver->SendSystemInfo(clientNum,"Combination match fail.");
                psserver->SendSystemInfo(clientNum,"Items left unmatched:");
                for(size_t z = 0; z < itemsLeft.GetSize(); z++)
                {
                    psserver->SendSystemInfo(clientNum,"Item id %u quantity %d",itemsLeft[z]->GetCurrentStats()->GetUID(),itemsLeft[z]->GetStackCount());
                }
            }
            return false;
        }
    }
    if(secure) psserver->SendSystemInfo(clientNum,"Bad count of items %u compared to combination count %u.", itemArray.GetSize(), current->combinations.GetSize());
    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if any transformation is possible
//   We check the more specific transforms for matches first
unsigned int WorkManager::AnyTransform(csArray<psTradePatterns*> &patterns, float &KFactor, uint32 targetId, int targetQty)
{
    unsigned int singleMatch = TRANSFORM_GARBAGE;
    unsigned int groupMatch  = TRANSFORM_GARBAGE;
    KFactor = 0;

    // First check for specific single transform match
    for(size_t i = 0; i < patterns.GetSize(); i++)
    {
        uint32 singlePatternId = patterns.Get(i)->GetId();
        if(secure) psserver->SendSystemInfo(clientNum,"Checking single transforms for pattern id %u, target id %u, and target qty %u.", singlePatternId,targetId,targetQty);
        singleMatch =  IsTransformable(singlePatternId, targetId, targetQty);
        if(singleMatch == TRANSFORM_MATCH)
        {
            KFactor = patterns.Get(i)->GetKFactor();
            return TRANSFORM_MATCH;
        }
    }

    // Then check for a group transform match
    for(size_t i = 0; i < patterns.GetSize(); i++)
    {
        uint32 groupPatternId = patterns.Get(i)->GetGroupPatternId();
        if(secure) psserver->SendSystemInfo(clientNum,"Checking group transforms for pattern id %u, target id %u, and target qty %u.",groupPatternId, targetId, targetQty);
        groupMatch = IsTransformable(groupPatternId, targetId, targetQty);
        if(groupMatch == TRANSFORM_MATCH)
        {
            KFactor = patterns.Get(i)->GetKFactor();
            return TRANSFORM_MATCH;
        }
    }

    // Check for patternless transforms
    if(secure) psserver->SendSystemInfo(clientNum,"Checking patternless transforms for pattern id %u, target id %u, and target qty %u.", 0, targetId, targetQty);
    unsigned int lessMatch = IsTransformable(0, targetId, targetQty);
    if(lessMatch == TRANSFORM_MATCH)
        return TRANSFORM_MATCH;

    //This is a bit weird but looked more elegant. Essentially we want to first
    //do one run where we only check the patterns then we do a second run
    //where we only check for the groups.
    for(bool groupCheck = false; groupCheck == false; groupCheck = true)
    {
        for(size_t i = 0; i < patterns.GetSize(); i++)
        {
            uint32 singlePatternId = groupCheck? patterns.Get(i)->GetGroupPatternId() : patterns.Get(i)->GetId();
            // Check for transforms of any ingredients
            if(secure) psserver->SendSystemInfo(clientNum,"Checking for any ingredient transforms for single pattern id %u, and target id %u.", singlePatternId, targetId);
            if(IsIngredient(singlePatternId, targetId))
            {
                KFactor = patterns.Get(i)->GetKFactor();
                return TRANSFORM_MATCH;
            }
        }
    }

    // Check for specific single any item transform match
    unsigned int singleGarbMatch = TRANSFORM_GARBAGE;
    for(size_t i = 0; i < patterns.GetSize(); i++)
    {
        uint32 singlePatternId = patterns.Get(i)->GetId();
        if(secure) psserver->SendSystemInfo(clientNum,"Checking single any item transforms for pattern id %u, target id %u, and target qty %u.",singlePatternId, 0, targetQty);
        singleGarbMatch = IsTransformable(singlePatternId, 0, targetQty);
        if(singleGarbMatch == TRANSFORM_MATCH)
        {
            KFactor = patterns.Get(i)->GetKFactor();
            return TRANSFORM_GARBAGE;
        }
    }

    // Then check for a group any item transform match
    for(size_t i = 0; i < patterns.GetSize(); i++)
    {
        uint32 groupPatternId = patterns.Get(i)->GetGroupPatternId();
        if(secure) psserver->SendSystemInfo(clientNum,"Checking group any item transforms for pattern id %u, target id %u, and target qty %u.", groupPatternId, 0, targetQty);
        unsigned int groupGarbMatch = IsTransformable(groupPatternId, 0, targetQty);
        if(groupGarbMatch == TRANSFORM_MATCH)
        {
            KFactor = patterns.Get(i)->GetKFactor();
            return TRANSFORM_GARBAGE;
        }
    }

    // Check patternless for transforms of any ingredients
    if(secure) psserver->SendSystemInfo(clientNum,"Checking for pattern-less any ingredient transforms for target id %u.", targetId);
    if(IsIngredient(0, targetId))
        return TRANSFORM_MATCH;

    // Check for patternless specific single any item transform match
    if(secure) psserver->SendSystemInfo(clientNum,"Checking pattern-less single any item transforms for target qty %u.", targetQty);
    singleGarbMatch = IsTransformable(0, 0, targetQty);
    if(singleGarbMatch == TRANSFORM_MATCH)
        return TRANSFORM_GARBAGE;

    // If no other non-garbage/unknown failures then check for pattern-less any item transforms
    if((singleMatch == TRANSFORM_GARBAGE || singleMatch == TRANSFORM_UNKNOWN_ITEM)
            && (groupMatch == TRANSFORM_GARBAGE || groupMatch == TRANSFORM_UNKNOWN_ITEM)
            && (lessMatch == TRANSFORM_GARBAGE || lessMatch == TRANSFORM_UNKNOWN_ITEM))
    {
        if(secure) psserver->SendSystemInfo(clientNum,"Checking patternless any item tranforms for pattern id %u, target id %u, and target qty %u.", 0, 0, 0);
        unsigned int lessGarbMatch = IsTransformable(0, 0, 0);
        if(lessGarbMatch == TRANSFORM_MATCH)
            return TRANSFORM_GARBAGE;
    }

    // Otherwise return a combination of all of them except garbage
    return (singleMatch | groupMatch | lessMatch);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if a transformation is possible
unsigned int WorkManager::IsTransformable(uint32 patternId, uint32 targetId, int targetQty)
{
    unsigned int match = TRANSFORM_GARBAGE;
    uint32 workID = 0;

    // Only get those with correct target item and pattern
    csPDelArray<psTradeTransformations>* transArray =
        cacheManager->FindTransformationsList(patternId, targetId);
    if(transArray == NULL)
    {
        if(secure) psserver->SendSystemInfo(clientNum,"Found no transformations for item %d and pattern %d.", targetId, patternId);
        return TRANSFORM_UNKNOWN_ITEM;
    }

    // Go thru all the trasnformations and check if one is possible
    if(secure) psserver->SendSystemInfo(clientNum,"Found %u transformations.", transArray->GetSize());
    if(workItem)
    {
        workID = workItem->GetCurrentStats()->GetUID();
    }
    psTradeTransformations* transCandidate;
    psTradeProcesses* procCandidate;
    for(size_t i=0; i<transArray->GetSize(); i++)
    {
        transCandidate = transArray->Get(i);
        if(secure) psserver->SendSystemInfo(clientNum,"Testing transformation id %u.", transCandidate->GetId());

        // Go thru all the possable processes and check if one is possible
        csArray<psTradeProcesses*>* procArray = cacheManager->GetTradeProcessesByID(
                transCandidate->GetProcessId());

        // If no process for this transform then just continue on
        if(!procArray)
        {
            if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation has no process array.");
            continue;
        }

        for(size_t j=0; j<procArray->GetSize(); j++)
        {
            // Check the non-zero work item does not match
            procCandidate = procArray->Get(j);
            if(procCandidate->GetWorkItemId() != 0 && procCandidate->GetWorkItemId() != workID)
            {
                if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation for wrong item candidate id=%u workitem Id=%u.", procCandidate->GetWorkItemId(), workID);
                match |= TRANSFORM_MISSING_ITEM;
                continue;
            }

            // Check if we do not have the correct quantity
            int itemQty = transCandidate->GetItemQty();
            if(itemQty != 0 && itemQty != targetQty)
            {
                if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation for wrong quantity candidateQty=%d itemQty=%d.", transCandidate->GetItemQty(), itemQty);
                match |= TRANSFORM_BAD_QUANTITY;
                continue;
            }

            // Check if any equipement is required
            uint32 equipmentId = procCandidate->GetEquipementId();
            if(equipmentId > 0)
            {
                // Check if required equipment is on hand
                if(!IsOnHand(equipmentId))
                {
                    if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation for wrong equipment transform id=%u equipmentId=%u.", transCandidate->GetId(), equipmentId);
                    match |= TRANSFORM_MISSING_EQUIPMENT;
                    continue;
                }
            }

            // Check if the player has the sufficient training
            if(!ValidateTraining(transCandidate, procCandidate))
            {
                if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation fails training check.");
                match |= TRANSFORM_BAD_TRAINING;
                continue;
            }

            // Check if the player has the correct skills
            if(!ValidateSkills(transCandidate, procCandidate))
            {
                if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation fails skill check.");
                match |= TRANSFORM_BAD_SKILLS;
                continue;
            }

            // Check if the transformation constraint is meet
            if(!ValidateConstraints(transCandidate, procCandidate))
            {
                // Player message comes from database
                if(secure) psserver->SendSystemInfo(clientNum,"No match, transformation fails constraint check.\n");
                match |= TRANSFORM_FAILED_CONSTRAINTS;
                continue;
            }

            // Good match
            if(secure) psserver->SendSystemInfo(clientNum,"Good match for transformation id=%u.\n", transCandidate->GetId());
            trans = transCandidate;
            process = procCandidate;
            return TRANSFORM_MATCH;
        }
    }

    // Return all the errors
    return match;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if any ingredient transformation is possible
bool WorkManager::IsIngredient(uint32 patternId, uint32 targetId)
{
    // Check if ingredient is on list of unique ingredients for this pattern
    csArray<uint32>* itemArray = cacheManager->GetTradeTransUniqueByID(patternId);
    if(itemArray == NULL)
    {
        if(secure) psserver->SendSystemInfo(clientNum,"No items for this pattern or group pattern.");
        return false;
    }

    // Go thru list
    for(size_t i=0; i<itemArray->GetSize(); i++)
    {
        // Check item on ingredient list
        if(itemArray->Get(i) == targetId)
        {
            // Get all unknow item transforms for this pattern
            csPDelArray<psTradeTransformations>* transArray =
                cacheManager->FindTransformationsList(patternId, 0);
            if(transArray == NULL)
            {
                if(secure) psserver->SendSystemInfo(clientNum,"No known transformations for this item.");
                return false;
            }

            // Go thru list of transforms
            for(size_t j=0; j<transArray->GetSize(); j++)
            {
                // Get first transform with a 0 process ID this indicates processless any ingredient transform
                trans = transArray->Get(j);
                process = NULL;
                if(trans->GetProcessId() == 0)
                {
                    return true;
                }
            }
        }
    }
    if(secure) psserver->SendSystemInfo(clientNum,"Not an ingredient for this pattern.");
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Do transformation
// This is called once all the transformation requirements have been verified.
void WorkManager::StartTransformationEvent(int transType, INVENTORY_SLOT_NUMBER transSlot, int resultQty,
        float resultQuality, psItem* item)
{
    // Get transformation delay
    int delay;

    if(trans)
    {
        delay = CalculateEventDuration(trans, process, item,  worker);
    }
    else
    {
        // Used in the case of a transformation like item->npc.
        delay = int(resultQuality/10);
    }

    // Let the server admin know
    if(secure) psserver->SendSystemInfo(clientNum,"Scheduled to finish work on %s in %1.1f seconds.\n", item->GetName(), (float)delay);

    // Make sure we have an item
    if(!item)
    {
        Error1("Bad item in StartTransformationEvent().");
        return;
    }

    // kill old event - just in case
    psWorkGameEvent* oldEvent = item->GetTransformationEvent();
    if(oldEvent)
    {
        Error1("Had to kill old event in StartTransformationEvent.");
        item->SetTransformationEvent(NULL);
        oldEvent->Interrupt();
    }

    // Set event
    csVector3 pos(0,0,0);
    psWorkGameEvent* workEvent = new psWorkGameEvent(
        this, owner, delay*1000, MANUFACTURE, pos, NULL, worker->GetClient());
    workEvent->SetTransformationType(transType);
    workEvent->SetTransformationSlot(transSlot);
    workEvent->SetResultQuality(resultQuality);
    workEvent->SetTransformationItem(item);
    workEvent->SetWorkItem(workItem);
    workEvent->SetTargetGem(gemTarget);
    workEvent->SetResultQuantity(resultQty);
    workEvent->SetTransformation(trans);
    workEvent->SetProcess(process);
    workEvent->SetKFactor(patternKFactor);
    item->SetTransformationEvent(workEvent);

    // Send render effect to client and nearby people
    if(process && process->GetRenderEffect().Length() > 0)
    {
        csVector3 offset(0,0,0);
        workEvent->effectID =  cacheManager->NextEffectUID();
        psEffectMessage newmsg(0, process->GetRenderEffect(), offset, owner->GetEID(),0 ,workEvent->effectID);
        newmsg.Multicast(workEvent->multi,0,PROX_LIST_ANY_RANGE);
    }

    // If it's not an autotransformation event run the animation and setup the owner with the event
    if(transType != TRANSFORMTYPE_AUTO_CONTAINER)
    {
        // Send anim and confirmation message to client and nearby people
        if(process && !process->GetAnimation().IsEmpty())
        {
            psOverrideActionMessage msg(clientNum, worker->GetEID(), process->GetAnimation(), delay);
            psserver->GetEventManager()->Multicast(msg.msg, worker->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
        }

        // Setup work owner
        worker->SetMode(PSCHARACTER_MODE_WORK);
        owner->SetTradeWork(workEvent);
        if(process)
        {
            owner->GetCharacterData()->SetStaminaRegenerationWork(process->GetPrimarySkillId());
        }
    }

    // Start event
    psserver->GetEventManager()->Push(workEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Loads up local member variables
bool WorkManager::LoadLocalVars(Client* client, gemObject* target)
{
    if(client == NULL)
    {
        Error1("Bad client pointer.");
        return false;
    }

    clientNum = client->GetClientNum();
    if(clientNum == 0)
    {
        Error1("Bad client number.");
        return false;
    }

    worker = client->GetActor();
    if(!worker.IsValid())
    {
        Error1("Bad client GetActor pointer.");
        return false;
    }

    owner = worker;
    if(!owner || !owner->GetCharacterData())
    {
        Error1("Bad client actor character data pointer.");
        return false;
    }

    // Setup security check
    secure = (client->GetSecurityLevel() > GM_LEVEL_0) ? true : false;

    // Setup mode string pointer
    preworkModeString = owner->GetModeStr();

    // Null out everything else
    gemTarget = target;
    workItem = NULL;
    patterns.Empty();
    currentQuality = 0.0;
    trans = NULL;
    process = NULL;

    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check to see if client can do any trade work
bool WorkManager::ValidateWork()
{
    if(worker->GetMode() == PSCHARACTER_MODE_WORK)
    {
        psserver->SendSystemInfo(clientNum,"You can not practice your trade when working.");
        return false;
    }
    // Check if not in normal mode
    else if(worker->GetMode() == PSCHARACTER_MODE_COMBAT)
    {
        psserver->SendSystemInfo(clientNum,"You can not practice your trade during combat.");
        return false;
    }
    return true;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check to see if client can do any trade work and
// that we have everything we need to do any trade work
bool WorkManager::ValidateMind()
{
    // Check for the existance of a design item
    psItem* designitem = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_MIND);
    if(!designitem)
    {
        patterns.Empty();
        return true;
    }

    // Check if there is a pattern for that design item
    psItemStats* itemStats = designitem->GetCurrentStats();
    patterns = cacheManager->GetTradePatternByItemID(itemStats->GetUID());
    if(patterns.GetSize() == 0)
    {
        Error3("ValidateWork() could not find pattern ID for item %u (%s) in mind slot",
               itemStats->GetUID(),itemStats->GetName());
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////
// Check for any targeted item or container in hand
bool WorkManager::ValidateTarget(Client* client)
{
    // Check if player has something targeted
    gemObject* target = client->GetTargetObject();

    gemActionLocation* gemAction = target->GetALPtr();
    if(gemAction) target = gemAction->GetAction()->GetRealItem();

    if(target)
    {
        // Make sure it's not character
        if(target->GetActorPtr())
        {
            psserver->SendSystemInfo(clientNum,"Only items can be targeted for use.");
            return false;
        }

        // Check range ignoring Y co-ordinate and ignoring instance
        if(worker->RangeTo(target, true, true) > RANGE_TO_USE)
        {
            psserver->SendSystemError(clientNum,"You are not in range to use %s.",target->GetItem()->GetName());
            return false;
        }

        // Only legit items
        if(!target->GetItem())
        {
            psserver->SendSystemInfo(clientNum,"That item can not be used in this way.");
            return false;
        }

        // Otherwise assign item
        workItem = target->GetItem();
        return true;
    }

    // if nothing targeted check for containers in hand
    else
    {
        // Check if player has any containers in right hand
        psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
        if(rhand)
        {
            // If it's a container it's our target
            if(rhand->GetIsContainer())
            {
                workItem = rhand;
                return true;
            }
        }

        // Check if player has any containers in left hand
        psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
        if(lhand)
        {
            // If it's a container it's our target
            if(lhand->GetIsContainer())
            {
                workItem = lhand;
                return true;
            }
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if client is too tired
bool WorkManager::ValidateStamina(Client* client)
{
    //TODO: use factors based on the work to determine required stamina
    // check stamina
    if(client->GetActor()->GetCharacterData()->GetStamina(true)
            <= client->GetActor()->GetCharacterData()->GetMaxPStamina().Current()*.1
            || client->GetActor()->GetCharacterData()->GetStamina(false)
            <= client->GetActor()->GetCharacterData()->GetMaxMStamina().Current()*.1)
    {
        SendTransformError(clientNum, TRANSFORM_NO_STAMINA);
        return false;
    }
    return true;
}

bool WorkManager::CheckStamina(psCharacter* owner) const
{
//todo- use factors based on the work to determine required stamina
    return (owner->GetStamina(true) >= owner->GetMaxPStamina().Current()*.1  // physical
            && owner->GetStamina(false) >= owner->GetMaxMStamina().Current()*.1 // mental
           );
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if equipment is in hand
bool WorkManager::IsOnHand(uint32 equipId)
{
    // Check right hand
    psItem* rhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
    if(rhand)
    {
        if(equipId == rhand->GetCurrentStats()->GetUID())
        {
            return true;
        }
    }

    // Check left hand
    psItem* lhand = owner->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
    if(lhand)
    {
        if(equipId == lhand->GetCurrentStats()->GetUID())
        {
            return true;
        }
    }
    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates player has enough training
bool WorkManager::ValidateTraining(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Check primary skill training if any skill required
    int priSkill = processCandidate->GetPrimarySkillId();
    if(priSkill > 0)
    {
        // If primary skill is zero, check if this skill should be trained first
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        if(basePriSkill == 0)
        {
            if(owner->GetCharacterData()->Skills().Get((PSSKILL)priSkill).CanTrain())
                return false;
        }
    }

    // Check secondary skill training if any skill required
    int secSkill = processCandidate->GetSecondarySkillId();
    if(secSkill > 0)
    {
        // If secondary skill is zero, check if this skill should be trained first
        unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
        if(baseSecSkill == 0)
        {
            if(owner->GetCharacterData()->Skills().Get((PSSKILL)secSkill).CanTrain())
                return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates player has the correct skills
bool WorkManager::ValidateSkills(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Check if players primary skill levels are less then minimum for that skill
    int priSkill = processCandidate->GetPrimarySkillId();
    if(priSkill > 0)
    {
        unsigned int minPriSkill = processCandidate->GetMinPrimarySkill();
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        if(minPriSkill > basePriSkill)
        {
            return false;
        }
    }

    // Check if players secondary skill levels are less then minimum for that skill
    int secSkill = processCandidate->GetSecondarySkillId();
    if(secSkill > 0)
    {
        unsigned int minSecSkill = processCandidate->GetMinSecondarySkill();
        unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
        if(minSecSkill > baseSecSkill)
        {
            return false;
        }
    }
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validates player is not over skilled
bool WorkManager::ValidateNotOverSkilled(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Check if players primary skill levels are less then maximum for that skill
    int priSkill = processCandidate->GetPrimarySkillId();
    if(priSkill >= 0)
    {
        unsigned int maxPriSkill = processCandidate->GetMaxPrimarySkill();
        unsigned int basePriSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)priSkill).Current();
        if(maxPriSkill < basePriSkill)
        {
            return false;
        }
    }

    // Check if players secondary skill levels are less then maximum for that skill
    int secSkill = processCandidate->GetSecondarySkillId();
    if(secSkill >= 0)
    {
        unsigned int maxSecSkill = processCandidate->GetMaxSecondarySkill();
        unsigned int baseSecSkill = owner->GetCharacterData()->Skills().GetSkillRank((PSSKILL)secSkill).Current();
        if(maxSecSkill < baseSecSkill)
        {
            return false;
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Validate if all the transformation constraints are meet
// Note: This assumes that each of the constraints have paramaters
bool WorkManager::ValidateConstraints(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate)
{
    // Set up to go through the constraint string picking out the functions and parameters
    const char constraintSeperators[] = "\t(),";
    char constraintPtr[256];
    char* param;

    // Get a work copy of the constraint string to token up
    const char* constraintStr = processCandidate->GetConstraintString();
    if(strlen(constraintStr) < 256)
    {
        strcpy(constraintPtr,constraintStr);
    }
    else
    {
        return false;
    }
    

    // Loop through all the constraint strings
    char* funct = strtok(constraintPtr, constraintSeperators);
    while(funct != NULL)
    {
        // Save the pointer to the constraint and get the parameter string
        param = strtok(NULL, constraintSeperators);

        // Check all the constraints
        int constraintId = -1;
        while(constraints[++constraintId].constraintFunction != NULL)
        {
            // Check if the transformation constraint matches
            if(strcmp(constraints[constraintId].name,funct) == 0)
            {

                // Call the associated function
                if(!constraints[constraintId].constraintFunction(this, param))
                {
                    // Send constraint specific message to client
                    psserver->SendSystemError(clientNum, constraints[constraintId].message);
                    return false;
                }
                break;
            }
        }

        // Get the next constraint
        funct = strtok(NULL, constraintSeperators);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Calculate how long it will take to complete event
//  based on the transformation's point quanity
int WorkManager::CalculateEventDuration(psTradeTransformations* trans, psTradeProcesses* process, psItem* transItem, gemActor* worker)
{
    // Calculate the seconds needed in order to complete this event
    int time = 0;

    if(calc_transform_time)
    {
        MathEnvironment env;
        env.Define("Object", transItem);
        env.Define("Worker", worker);
        env.Define("Transform", trans);
        env.Define("Process", process);

        calc_transform_time->Evaluate(&env);
        int time = env.Lookup("Time")->GetValue();
        return time;
    }
    else
    {
        //if we lack a script we fallback to this solution.
        if(trans->GetItemQty() == 0 && trans->GetItemId() != 0 && trans->GetResultId() != 0)
            time = int(trans->GetTransPoints() +  trans->GetTransPoints() * (transItem->GetStackCount() - 1) * 0.1);
        else
            time = trans->GetTransPoints();
    }

    return time;
}

void WorkManager::ApplyProcessScript(psItem* oldItem, psItem* newItem, gemActor* worker, psTradeProcesses* process, psTradeTransformations* trans)
{
    //Check if the process is valid (not null) and that the scriptname contains something and
    //it's not an empty string. Default in the db is "Apply Post Trade Process"
    if(process && process->GetScript())
    {
        MathEnvironment env;
        env.Define("OldItem", oldItem);
        env.Define("NewItem", newItem);
        env.Define("Worker",  worker);
        env.Define("Process", process);
        env.Define("Transform", trans);
        process->GetScript()->Evaluate(&env);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Combine transform all items in container into a new item
psItem* WorkManager::CombineContainedItem(uint32 newId, int newQty, float itemQuality, psItem* containerItem)
{
#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "deleting items from container...\n");
#endif

    // cast a gem container to iterate thru
    gemContainer* container = dynamic_cast<gemContainer*>(containerItem->GetGemObject());
    if(!container)
    {
        Error1("Could not instantiate gemContainer");
        return NULL;
    }

    // Remove all items in container and destroy them
    gemContainer::psContainerIterator it(container);
    while(it.HasNext())
    {
        // Remove all items from container that player owns
        psItem* item = it.Next();
        if((item->GetGuardingCharacterID() == owner->GetPID())
                || (item->GetGuardingCharacterID() == 0))
        {
            // Kill any trade work started
            psWorkGameEvent* workEvent = item->GetTransformationEvent();
            if(workEvent)
            {
                item->SetTransformationEvent(NULL);
                workEvent->Interrupt();
            }

            // Remove items and delete
            it.RemoveCurrent(owner->GetClient());
            if(!item->Destroy())
            {
                Error2("CombineContainedItem() could not remove old item ID #%u from database.", item->GetUID());
                return NULL;
            }
            delete item;
        }
    }

    // Create combined item
    psItem* newItem = CreateTradeItem(newId, newQty, itemQuality);
    if(!newItem)
    {
        Error2("CreateTradeItem() could not create new item ID #%u", newId);
        return NULL;
    }

    newItem->SetOwningCharacter(owner->GetCharacterData());
    // this is done by AddToContainer, as long as SetOwningCharacter is set _before_
    // newItem->SetGuardingCharacterID(owner->GetPID());

    // Locate item in container and save container
    if(!container->AddToContainer(newItem, owner->GetClient()))
    {
        Error3("Bad container slot %i when trying to add item instance #%u.", PSCHARACTER_SLOT_NONE, newItem->GetUID());
        return NULL;
    }
    containerItem->Save(true);

    // Zero out x,y,z location because it is in container
    float xpos,ypos,zpos,yrot;
    psSectorInfo* sectorinfo;
    InstanceID instance;
    containerItem->GetLocationInWorld(instance, &sectorinfo, xpos, ypos, zpos, yrot);
    newItem->SetLocationInWorld(instance,sectorinfo, 0.00, 0.00, 0.00, 0.00);

    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if(newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
               newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
               worker->GetGuildID(), owner->GetFirstName(), owner);
        if(workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);
    //load local variables which gets triggered when the call it.RemoveCurrent(owner->GetClient());
    //is done zeroes out workitem we restore it instead.
    workItem = containerItem;
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform the container into a new item
psItem* WorkManager::TransformSelfContainerItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans)
{
    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform the one items in container into a new item
psItem* WorkManager::TransformContainedItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans)
{
    if(!oldItem)
    {
        Error1("TransformAutoContainedItem() had no item to remove.");
        return NULL;
    }

#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "deleting item from container...\n");
#endif

    gemContainer* container = dynamic_cast<gemContainer*>(workItem->GetGemObject());
    if(!container)
    {
        Error1("Could not instantiate gemContainer");
        return NULL;
    }

    INVENTORY_SLOT_NUMBER oldslot = oldItem->GetLocInParent(false);

    // Remove items from container
    container->RemoveFromContainer(oldItem, owner->GetClient());

    //Destroy the old item
    if(!oldItem->Destroy())
    {
        Error2("TransformContainedItem() could not remove old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }

    psItem* newItem = NULL;
    if(newId > 0)
    {
        //Create item and save it to item instances
        newItem = CreateTradeItem(newId, newQty, itemQuality);
        //As the item wasn't consumed apply the process script on it
        if(newItem) //just a slight integrity check.
        {
            ApplyProcessScript(oldItem, newItem, owner, process, trans);
        }
    }

    //As the old item is no more needed delete it
    delete oldItem;

    if(!newItem)
    {
        // Check for consumed item
        if(newId > 0)
        {
            Error2("CreateTradeItem() could not create new item ID #%u", newId);
        }
        return NULL;
    }

    newItem->SetOwningCharacter(owner->GetCharacterData());
    // this is done by AddToContainer, as long as SetOwningCharacter is set _before_
    // newItem->SetGuardingCharacterID(owner->GetPID());

    // Locate item in container and save container
    if(!container->AddToContainer(newItem, owner->GetClient(), oldslot))
    {
        Error3("Bad container slot %i when trying to add item instance #%u.", PSCHARACTER_SLOT_NONE, newItem->GetUID());
        return NULL;
    }
    workItem->Save(true);

    // Zero out x,y,z location because it is in container
    float xpos,ypos,zpos,yrot;
    psSectorInfo* sectorinfo;
    InstanceID instance;
    workItem->GetLocationInWorld(instance, &sectorinfo, xpos, ypos, zpos, yrot);
    newItem->SetLocationInWorld(instance,sectorinfo, 0.00, 0.00, 0.00, 0.00);

    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if(newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
               newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
               worker->GetGuildID(), owner->GetFirstName(), owner);
        if(workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform all items in equipment into a new item
psItem* WorkManager::TransformSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans)
{
    // Remove items from slot and destroy it
    psItem* oldItem = owner->GetCharacterData()->Inventory().RemoveItem(NULL,slot);
    if(!oldItem)
    {
        Error2("TransformSlotItem() could not remove item in slot #%i", slot);
        return NULL;
    }

    //Destroy the old item
    if(!oldItem->Destroy())
    {
        Error2("TransformSlotItem() could not remove old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }

    psItem* newItem = NULL;
    if(newId > 0)
    {
        //Create item and save it to item instances
        newItem = CreateTradeItem(newId, newQty, itemQuality);
        //As the item wasn't consumed apply the process script on it
        if(newItem) //just a slight integrity check.
        {
            ApplyProcessScript(oldItem, newItem, owner, process, trans);
        }
    }

    //As the old item is no more needed delete it
    delete oldItem;

    if(!newItem)
    {
        // Check for consumed item
        if(newId > 0)
        {
            Error2("CreateTradeItem() could not create new item ID #%u", newId);
        }
        return NULL;
    }

    // Check for valid slot
    if(slot < 0)
    {
        Error1("No good slot to locate new item");
        return NULL;
    }

    // Locate item in owner's slot
    newItem->SetOwningCharacter(owner->GetCharacterData());
    owner->GetCharacterData()->Inventory().Add(newItem,false,false,slot);
    if(!owner->GetCharacterData()->Inventory().EquipItem(newItem,slot))
    {
        // If can't equip then drop
        owner->GetCharacterData()->DropItem(newItem);
        psserver->SendSystemError(clientNum,
                                  "Item %s was dropped because it could not be equiped.",newItem->GetName());
    }
    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if(newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
               newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
               worker->GetGuildID(), owner->GetFirstName(), owner);
        if(workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    // Send equip message to client
    psserver->GetCharManager()->SendInventory(clientNum);
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform all items in equipment into a new item
psItem* WorkManager::TransformTargetSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans)
{
    // Remove items from targeted slot and destroy it
    psItem* oldItem = gemTarget->GetCharacterData()->Inventory().RemoveItem(NULL,slot);
    if(!oldItem)
    {
        Error2("TransformSlotItem() could not remove item in slot #%i", slot);
        return NULL;
    }

    //Destroy the old item
    if(!oldItem->Destroy())
    {
        Error2("TransformSlotItem() could not remove old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }

    psItem* newItem = NULL;
    if(newId > 0)
    {
        //Create item and save it to item instances
        newItem = CreateTradeItem(newId, newQty, itemQuality);
        //As the item wasn't consumed apply the process script on it
        if(newItem) //just a slight integrity check.
        {
            ApplyProcessScript(oldItem, newItem, owner, process, trans);
        }
    }

    //As the old item is no more needed delete it
    delete oldItem;

    if(!newItem)
    {
        // Check for consumed item
        if(newId > 0)
        {
            Error2("CreateTradeItem() could not create new item ID #%u", newId);
        }
        return NULL;
    }

    // Check for valid slot
    if(slot < 0)
    {
        Error1("No good slot to locate new item");
        return NULL;
    }

    // Check for valid target
    if(!gemTarget->GetCharacterData())
    {
        Error1("No good target to locate new item");
        return NULL;
    }

    // Locate in target slot
    newItem->SetOwningCharacter(gemTarget->GetCharacterData());
    gemTarget->GetCharacterData()->Inventory().Add(newItem);
    if(!gemTarget->GetCharacterData()->Inventory().EquipItem(newItem,slot))
    {
        // If can't equip then drop
        gemTarget->GetCharacterData()->DropItem(newItem);
        psserver->SendSystemError(gemTarget->GetClientID(),
                                  "Item %s was dropped because it could not be equiped.", newItem->GetName());
    }
    newItem->SetLoaded();  // Item is fully created

    // Check for conditions that would cause assert on newItem->Save(true)
    if(newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
               newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
               worker->GetGuildID(), gemTarget->GetCharacterData()->GetCharName(), gemTarget->GetCharacterData());
        if(workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        return NULL;
    }
    newItem->Save(true);

    // Send equip message to target
    psserver->GetCharManager()->SendInventory(gemTarget->GetClientID());
    if(gemTarget->GetClientID())  // No superclient
    {
        psserver->GetCharManager()->UpdateItemViews(gemTarget->GetClientID());
    }
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transforms the targeted item into a new item
psItem* WorkManager::TransformTargetItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans)
{
    if(!oldItem)
    {
        Error1("TransformTargetItem() had no item to transform.");
        return NULL;
    }

#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "deleting item from world...\n");
#endif

    // Get the location of what will be replaced
    InstanceID instance;
    psSectorInfo* sectorinfo;
    float xpos,ypos,zpos,yrot;
    oldItem->GetLocationInWorld(instance, &sectorinfo, xpos, ypos, zpos, yrot);

    //Destroy gem and item
    entityManager->RemoveActor(oldItem->GetGemObject());
    if(!oldItem->Destroy())
    {
        Error2("TransformTargetItem() could not destroy old item ID #%u from database", oldItem->GetUID());
        return NULL;
    }

    psItem* newItem = NULL;
    if(newId > 0)
    {
        //Create item and save it to item instances
        newItem = CreateTradeItem(newId, newQty, itemQuality, true);
        //As the item wasn't consumed apply the process script on it
        if(newItem) //just a slight integrity check.
        {
            ApplyProcessScript(oldItem, newItem, owner, process, trans);
        }
    }

    //As the old item is no more needed delete it
    delete oldItem;

    if(!newItem)
    {
        // Check for consumed item
        if(newId > 0)
        {
            Error2("CreateTradeItem() could not create new item ID #%u", newId);
        }
        return NULL;
    }

    // Locate the new item where the old one was
    newItem->SetLocationInWorld(instance,sectorinfo,xpos,ypos,zpos,yrot);
    if(!entityManager->CreateItem(newItem, true))
    {
        delete newItem;
        return NULL;
    }


    // Check for conditions that would cause assert on newItem->Save(true)
    if(newItem->GetLocInParent(false) == -1 && newItem->GetOwningCharacter() && newItem->GetContainerID()==0)
    {
        Error6("Problem on item: item UID=%i crafterID=%s guildID=%i name=%s owner=%p",
               newItem->GetBaseStats()->GetUID(), ShowID(worker->GetPID()),
               worker->GetGuildID(), owner->GetFirstName(), owner);
        if(workItem && workItem->GetBaseStats())
        {
            Error2("workitem=%i",workItem->GetBaseStats()->GetUID());
        }
        delete newItem;
        return NULL;
    }

    // Target it and set it loaded
    worker->GetClient()->SetTargetObject(newItem->GetGemObject());
    newItem->SetLoaded();
    newItem->Save(true);
    return newItem;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transforms the targeted item into an NPC.
void WorkManager::TransformTargetItemToNpc(psItem* workItem, Client* client)
{
    InstanceID instance;
    psSectorInfo* sectorinfo;
    float loc_x;
    float loc_y;
    float loc_z;
    float loc_yrot;
    workItem->GetLocationInWorld(instance, &sectorinfo, loc_x, loc_y, loc_z, loc_yrot);

    psCharacter* charData = new psCharacter();
    charData->SetCharType(PSCHARACTER_TYPE_NPC);
    psRaceInfo* raceinfo = cacheManager->GetRaceInfoByNameGender("Special", PSCHARACTER_GENDER_NONE);
    charData->SetPID(psserver->GetUnusedPID());
    charData->SetName(charData->GetPID().Show());
    charData->SetRaceInfo(raceinfo);
    charData->SetOwnerID(client->GetPID());
    charData->SetLocationInWorld(instance, sectorinfo, loc_x, loc_y, loc_z, loc_yrot);

    csString script;
    script.Format("<response><run script='%s'/></response>", workItem->GetBaseStats()->GetConsumeScriptName().GetData());
    NpcResponse* response = dict->AddResponse(script);
    dict->AddTrigger(charData->GetPID().Show(), "!anyrange", 0, response->id);

    entityManager->RemoveActor(workItem->GetGemObject());
    workItem->Destroy();

    entityManager->CreateNPC(charData, true, true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create the new item and return newly created item pointer
//  The location and saving of item is up to calling routine
psItem* WorkManager::CreateTradeItem(uint32 newId, int newQty, float itemQuality, bool transient)
{
    Debug3(LOG_TRADE, 0,"Creating new item id(%u) quantity(%d)\n", newId, newQty);

#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "creating item id=%u qty=%i quality=%f\n",
            newId, newQty, itemQuality);
#endif

    // Check to make sure it's not a trasnformation that just destroys items
    if(newId > 0)
    {
        // Create a new item including stack count and the calculated quality
        psItemStats* baseStats = cacheManager->GetBasicItemStatsByID(newId);
        if(!baseStats)
        {
            Error2("CreateTradeItem() could not get base states for item #%iu", newId);
            return NULL;
        }

        // Make a perminent new item
        psItem* newItem = baseStats->InstantiateBasicItem(transient);
        if(!newItem)
        {
            Error3("CreateTradeItem() could not create item (%s) id #%u",
                   baseStats->GetName(), newId);
            return NULL;
        }

        // Set stuff
        newItem->SetItemQuality(itemQuality);
        newItem->SetMaxItemQuality(itemQuality);            // Set the max quality the same as the inital quality.
        newItem->SetStackCount(newQty);
        newItem->SetDecayResistance(0.5);

        // Set current player ID creator mark
        newItem->SetCrafterID(worker->GetPID());

        // Set current player guild to creator mark
        if(owner->GetGuild())
            newItem->SetGuildID(owner->GetGuild()->GetID());
        else
            newItem->SetGuildID(0);

#ifdef DEBUG_WORKMANAGER
        CPrintf(CON_DEBUG, "done creating item crafterID=%s guildID=%i name=%s owner=%p\n",
                ShowID(worker->GetPID()), worker->GetGuildID(), owner->GetFirstName(), owner);
#endif

        return newItem;
    }
    return NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint functions
#define PSABS(x)    ((x) < 0 ? -(x) : (x))

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check hour of day
bool WorkManager::constraintTime(WorkManager* that,char* param)
{
    // Check paramater pointer
    if(!param)
        return false;

    // Get game hour in 24 hours cycle
    int curTime = psserver->GetWeatherManager()->GetGameTODHour();
    int targetTime = atoi(param);

    if(curTime == targetTime)
    {
        return true;
    }
    return false;
}

#define FRIEND_RANGE 5
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check if players are near worker (including worker)
//  Note: Constraint distance is limited to proximiy list.
bool WorkManager::constraintFriends(WorkManager* that, char* param)
{
    // Check paramater pointer
    if(!param)
        return false;

    // Count proximity player objects
    csArray< gemObject*>* targetsInRange  = that->worker->GetObjectsInRange(FRIEND_RANGE);

    size_t count = (size_t)atoi(param);
    if(targetsInRange->GetSize() == count)
    {
        return true;
    }
    return false;
}


#define MAXDISTANCE 1
#define MAXANGLE 0.2
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check location
bool WorkManager::constraintLocation(WorkManager* that, char* param)
{
    int ch = ',';
    char* pdest;

    // Get the current position of client
    csVector3 pos;
    float yrot;
    iSector* sect;
    that->worker->GetPosition(pos, yrot, sect);

    // Parse through the constraint parameters
    pdest = strchr(param, ch);
    *pdest = '\0';
    char* sector = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr(param, ch);
    *pdest = '\0';
    char* xloc = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr(param, ch);
    *pdest = '\0';
    char* yloc = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr(param, ch);
    *pdest = '\0';
    char* zloc = param;

    // go to the next parameter
    param = pdest + 1;
    pdest = strchr(param, ch);
    *pdest = '\0';
    char* yrotation = param;

    // Skip if no sector constraint name specified
    if(strlen(sector) != 0)
    {
        // Check sector
        if(strcmp(sector, sect->QueryObject()->GetName()) != 0)
            return false;
    }

    // Skip if no X constraint co-ord specified
    if(strlen(xloc) != 0)
    {
        // Check X location
        float x = atof(xloc);
        if(PSABS(pos.x - x) > MAXDISTANCE)
            return false;
    }

    // Skip if no Y constraint co-ord specified
    if(strlen(yloc) != 0)
    {
        // Check Y location
        float y = atof(yloc);
        if(PSABS(pos.y - y) > MAXDISTANCE)
            return false;
    }

    // Skip if no Z constraint co-ord specified
    if(strlen(zloc) != 0)
    {
        // Check Z location
        float z = atof(zloc);
        if(PSABS(pos.z - z) > MAXDISTANCE)
            return false;
    }

    // Skip if no Yrot constraint co-ord specified
    if(strlen(yrotation) != 0)
    {
        // Check Y rotation
        float r = atof(yrotation);
        if(PSABS(yrot - r) > MAXANGLE)
            return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check client mode
bool WorkManager::constraintMode(WorkManager* that, char* param)
{
    // Check mode string pointer
    if(!that->preworkModeString)
        return false;

    // Check constraint mode to mode before work started
    if(strcmp(that->preworkModeString, param) != 0)
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check client gender
bool WorkManager::constraintGender(WorkManager* that, char* param)
{
    // Get race info
    psRaceInfo* race = that->owner->GetCharacterData()->GetRaceInfo();

    // Check constraint gender to player gender
    if(strcmp(race->GetGender(), param) != 0)
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constraint function to check client race
bool WorkManager::constraintRace(WorkManager* that, char* param)
{
    // Get race info
    psRaceInfo* race = that->owner->GetCharacterData()->GetRaceInfo();

    // Check constraint race to player race
    if(strcmp(race->GetRace(), param) != 0)
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This handles trade work transformation events
void WorkManager::HandleWorkEvent(psWorkGameEvent* workEvent)
{
#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "handling work event...\n");
#endif
    // Load vars
    if(!LoadLocalVars(workEvent->client,workEvent->gemTarget))
    {
        Error1("Could not load local variables in HandleWorkEvent().\n");
        if(secure) psserver->SendSystemInfo(clientNum,"Could not load local variables in HandleWorkEvent().");
        owner->SetTradeWork(NULL);
        worker->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    int transType = workEvent->GetTransformationType();
    if(transType != TRANSFORMTYPE_TARGET_TO_NPC)
    {
        // Get active transformation and then do it
        trans = workEvent->GetTransformation();
        if(!trans)
        {
            Error1("No valid transformation in HandleWorkEvent().\n");
            if(secure) psserver->SendSystemInfo(clientNum,"No valid transformation in HandleWorkEvent().");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }

        // Get active process
        process = workEvent->GetProcess();
        if(!process)
        {
            Error1("No valid process pointer for transaction.\n");
            if(secure) psserver->SendSystemInfo(clientNum,"No valid process pointer for transaction.\n");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // Check for exploits before starting
    workItem = workEvent->GetWorkItem();
    if(transType == TRANSFORMTYPE_CONTAINER)
    {
        // Get event data
        if(!workItem)
        {
            Error1("No valid transformation item in HandleWorkEvent().\n");
            if(secure) psserver->SendSystemInfo(clientNum,"No valid transformation item in HandleWorkEvent().");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }

        // Check to see if player walked away from non-auto container
        gemObject* target = workItem->GetGemObject();
        if(worker->RangeTo(target, true) > RANGE_TO_USE)
        {
            psserver->SendSystemOK(clientNum,"You interrupted your work when you moved away.");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // Clear out work item event pointer
    if(workItem)
    {
        workItem->SetTransformationEvent(NULL);
    }

    // Set and load
    currentQuality = workEvent->GetResultQuality();
    const int originalQty = workEvent->GetResultQuantity();
    psItem* sourceItem = workEvent->GetTranformationItem();

    uint32 resultItem = 0;
    int resultQty = 0;
    int itemQty = 0;
    int QtyMultiplier = 1;

    if(trans)
    {
        resultItem = trans->GetResultId();
        resultQty = trans->GetResultQty();
        itemQty = trans->GetItemQty();
    }

    if(!sourceItem)
    {
        Error1("Bad transformation item in HandleWorkEvent().\n");
        if(secure) psserver->SendSystemInfo(clientNum,"Bad transformation item in HandleWorkEvent().");
        owner->SetTradeWork(NULL);
        worker->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    // Check for item being moved from hand slots - container movement is handled by slot manager
    if(transType == TRANSFORMTYPE_SLOT)
    {
        INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
        psItem* oldItem = owner->GetCharacterData()->Inventory().GetInventoryItem(slot);
        if(oldItem != workEvent->GetTranformationItem() || (oldItem && oldItem->GetStackCount() != originalQty))
        {
            psserver->SendSystemOK(clientNum,"You interrupted your work when you moved item.");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // Check for item being moved from targetted hand slots
    if(transType == TRANSFORMTYPE_TARGETSLOT)
    {
        INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
        psItem* oldItem = gemTarget->GetCharacterData()->Inventory().GetInventoryItem(slot);
        if(oldItem != workEvent->GetTranformationItem() || (oldItem && oldItem->GetStackCount() != originalQty))
        {
            psserver->SendSystemOK(clientNum,"You interrupted your transform when your target moved item.");
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }
    // Check and apply the skills to quality
    float startQuality = sourceItem->GetItemQuality();
    if(process)
    {
        if(resultItem > 0 && !CalculateQuality(workEvent->GetKFactor(), workEvent->GetTranformationItem(), owner, itemQty == 0, currentQuality, process, trans, workEvent->delayticks) && process->GetGarbageId() != 0)
        {
            resultItem = process->GetGarbageId();
            resultQty = process->GetGarbageQty();
            psItemStats* Stats = cacheManager->GetBasicItemStatsByID(resultItem);
            if(Stats)
                startQuality = currentQuality = Stats->GetQuality();
        }
    }

    //  Check for 0 quantity transformations
    if(itemQty == 0)
    {
        resultQty = workEvent->GetResultQuantity();
        itemQty = resultQty;
        QtyMultiplier = resultQty;
    }

    // store the source item for later comparison with result
    psItem* sourceItemCopy = new psItem();
    sourceItem->Copy(sourceItemCopy);

    // Handle all the different transformation types
    psItem* newItem = NULL;
    switch(transType)
    {
        case TRANSFORMTYPE_SLOT:
        {
            INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
            newItem = TransformSlotItem(slot, resultItem, resultQty, currentQuality, workEvent->GetProcess(), workEvent->GetTransformation());
            if(!newItem && (resultItem != 0))
            {
                Error1("TransformSlotItem did not create new item in HandleWorkEvent().\n");
                if(secure) psserver->SendSystemInfo(clientNum,"TransformSlotItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_TARGETSLOT:
        {
            INVENTORY_SLOT_NUMBER slot = workEvent->GetTransformationSlot();
            newItem = TransformTargetSlotItem(slot, resultItem, resultQty, currentQuality, workEvent->GetProcess(), workEvent->GetTransformation());
            if(!newItem && (resultItem != 0))
            {
                Error1("TransformTargetSlotItem did not create new item in HandleWorkEvent().\n");
                if(secure) psserver->SendSystemInfo(clientNum,"TransformSlotItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_TARGET:
        {
            newItem = TransformTargetItem(workEvent->GetTranformationItem(), resultItem, resultQty, currentQuality, workEvent->GetProcess(), workEvent->GetTransformation());
            if(!newItem && (resultItem != 0))
            {
                Error1("TransformTargetItem did not create new item in HandleWorkEvent().\n");
                if(secure) psserver->SendSystemInfo(clientNum,"TransformSlotItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_TARGET_TO_NPC:
        {
            bool is_trap = workEvent->GetTranformationItem()->GetIsTrap();
            TransformTargetItemToNpc(workEvent->GetTranformationItem(), workEvent->client);

            if(is_trap)
            {
                psserver->SendSystemInfo(clientNum, "Trap constructed!");
            }

            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }

        case TRANSFORMTYPE_SELF_CONTAINER:
        {
            newItem = TransformSelfContainerItem(workEvent->GetTranformationItem(), resultItem, resultQty, currentQuality, workEvent->GetProcess(), workEvent->GetTransformation());
            if(!newItem  && (resultItem != 0))
            {
                Error1("TransformSelfContainerItem did not create new item in HandleWorkEvent().\n");
                if(secure) psserver->SendSystemInfo(clientNum,"TransformSelfContainerItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_AUTO_CONTAINER:
        case TRANSFORMTYPE_SLOT_CONTAINER:
        case TRANSFORMTYPE_CONTAINER:
        {
            newItem = TransformContainedItem(workEvent->GetTranformationItem(), resultItem, resultQty, currentQuality, workEvent->GetProcess(), workEvent->GetTransformation());
            if(!newItem  && (resultItem != 0))
            {
                Error1("TransformContainedItem did not create new item in HandleWorkEvent().\n");
                if(secure) psserver->SendSystemInfo(clientNum,"TransformContainedItem did not create new item in HandleWorkEvent().");
                owner->SetTradeWork(NULL);
                worker->SetMode(PSCHARACTER_MODE_PEACE);
                return;
            }
            workEvent->SetTransformationItem(newItem);
            break;
        }

        case TRANSFORMTYPE_UNKNOWN:
        default:
        {
            break;
        }
    }

    // check if result item is identical to source and in this case do not give experience or practice
    bool somethingHappened = true;
    if(newItem)
    {
        if(sourceItemCopy->GetBaseStats()->GetUID()==newItem->GetBaseStats()->GetUID() &&
                sourceItemCopy->GetItemQuality()==newItem->GetItemQuality() &&
                sourceItemCopy->GetMaxItemQuality()==newItem->GetMaxItemQuality() &&
                sourceItemCopy->GetModifier(0)==newItem->GetModifier(0) &&
                sourceItemCopy->GetModifier(1)==newItem->GetModifier(1) &&
                sourceItemCopy->GetModifier(2)==newItem->GetModifier(2))
        {
            somethingHappened = false;
        }
    }
    delete sourceItemCopy;


    // Calculate and apply experience points
    if(resultItem > 0 && somethingHappened)
    {
        unsigned int experiencePoints;
        {
            MathEnvironment env;
            env.Define("StartQuality", startQuality);
            env.Define("CurrentQuality", currentQuality);
            env.Define("QtyMultiplier", QtyMultiplier);
            env.Define("Character", owner);

            calc_transform_exp->Evaluate(&env);
            experiencePoints = env.Lookup("Exp")->GetRoundValue();
        }

        owner->GetCharacterData()->AddExperiencePointsNotify(experiencePoints);

        Debug2(LOG_TRADE, clientNum, "Giving experience points %i.\n",experiencePoints);
        if(secure) psserver->SendSystemInfo(clientNum,"Giving experience points %i.",experiencePoints);
    }

    // Calculate and apply practice points
    if(resultItem > 0 && somethingHappened)
    {
        unsigned int primaryPracticePoints;
        unsigned int secondaryPracticePoints;
        double primaryModifier = 0.0f;
        double secondaryModifier = 0.0f;

        MathEnvironment env;
        env.Define("StartQuality", startQuality);
        env.Define("CurrentQuality", currentQuality);
        env.Define("QtyMultiplier", QtyMultiplier);
        env.Define("Character", owner);
        env.Define("Process", workEvent->GetProcess());
        env.Define("CalculatedTime", workEvent->delayticks/1000);

        calc_transform_practice->Evaluate(&env);
        primaryPracticePoints = env.Lookup("PriPoints")->GetRoundValue();

        // Check for a primary Modifier for the exp assignment, defaults to 0 in order to reproduce previous behaviour.
        if(env.Lookup("PrimaryModifier") != NULL)
        {
            primaryModifier = env.Lookup("PrimaryModifier")->GetValue();
        }

        int priSkill = workEvent->GetProcess()->GetPrimarySkillId();
        owner->GetCharacterData()->CalculateAddExperience((PSSKILL)priSkill, primaryPracticePoints, primaryModifier);

        Debug3(LOG_TRADE, clientNum, "Giving %i practice points to skill %d .",primaryPracticePoints, priSkill);
        if(secure) psserver->SendSystemInfo(clientNum,"Giving %i practice points to skill %d .",primaryPracticePoints, priSkill);

        int secSkill = workEvent->GetProcess()->GetSecondarySkillId();
        if(secSkill)
        {
            secondaryPracticePoints = env.Lookup("SecPoints")->GetRoundValue();

            // Check for a primary Modifier for the exp assignment, defaults to 0 in order to reproduce previous behaviour.
            if(env.Lookup("SecondaryModifier") != NULL)
            {
                primaryModifier = env.Lookup("SecondaryModifier")->GetValue();
            }

            owner->GetCharacterData()->CalculateAddExperience((PSSKILL)secSkill, secondaryPracticePoints, secondaryModifier);
            Debug3(LOG_TRADE, clientNum, "Giving %i practice points to skill %d .",secondaryPracticePoints, secSkill);
            if(secure) psserver->SendSystemInfo(clientNum,"Giving %i practice points to skill %d .",secondaryPracticePoints, secSkill);
        }

    }

    // Let the user know the we have done something
    if(resultItem <= 0)
    {
        psItemStats* itemStats = cacheManager->GetBasicItemStatsByID(trans->GetItemId());
        if(itemStats)
        {
            psserver->SendSystemOK(clientNum," %i %s is gone.", itemQty, itemStats->GetName());
        }
        else
        {
            Error2("HandleWorkEvent() could not get item stats for item ID #%u.", trans->GetItemId());
            if(secure) psserver->SendSystemInfo(clientNum,"HandleWorkEvent() could not get item stats for item ID #%u.", trans->GetItemId());
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }
    else
    {
        psItemStats* resultStats = cacheManager->GetBasicItemStatsByID(resultItem);
        psItem* newItem = workEvent->GetTranformationItem();
        if(resultStats && newItem)
        {
            psserver->SendSystemOK(clientNum, "You made %i %s with quality %.0f.", resultQty, resultStats->GetName(), newItem->GetItemQuality());
        }
        else
        {
            Error2("HandleWorkEvent() could not get result item or item stats for item ID #%u.", resultItem);
            if(secure) psserver->SendSystemInfo(clientNum,"HandleWorkEvent() could not get result item or stats for item ID #%u.", resultItem);
            owner->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
            return;
        }
    }

    // If there is something left lets see what other damage we can do
    if(resultItem > 0 && somethingHappened)
    {
        // If it's an auto transformation keep going
        if(transType == TRANSFORMTYPE_AUTO_CONTAINER)
        {
            // Check to see if we have pattern
            if(ValidateMind())
            {
                //TODO: Maybe all those function like this which seems copy & paste should be put in an unique one?
                // Check if there is another transformation possible for the item just created
                unsigned int transMatch = AnyTransform(patterns, patternKFactor, resultItem, resultQty);
                if((transMatch == TRANSFORM_MATCH) || (transMatch == TRANSFORM_GARBAGE))
                {
                    // Set up event for new transformation with the same setting as the old
                    StartTransformationEvent(
                        workEvent->GetTransformationType(), workEvent->GetTransformationSlot(),
                        workEvent->GetResultQuantity(), workEvent->GetResultQuality(),
                        workEvent->GetTranformationItem());
                    if(transMatch == TRANSFORM_GARBAGE)
                        psserver->SendSystemError(clientNum,"You are not sure what is going to happen to %d %s.", workEvent->GetTranformationItem()->GetStackCount(), workEvent->GetTranformationItem()->GetName());
                    return;
                }
            }
        }

        // If item in any container clean it up
        if((transType == TRANSFORMTYPE_AUTO_CONTAINER) ||
                (transType == TRANSFORMTYPE_CONTAINER))
        {
            // If no transformations started go ahead and start a cleanup event
            psItem* newItem = workEvent->GetTranformationItem();
            if(newItem)
            {
                StartCleanupEvent(transType,workEvent->client, newItem, workEvent->client->GetActor());
            }
        }
    }

    // Make sure we clear out the old event and go to peace
    owner->SetTradeWork(NULL);
    worker->SetMode(PSCHARACTER_MODE_PEACE);
}

// Apply skills if any to determine quality
bool WorkManager::CalculateQuality(float factor, psItem* transItem, gemActor* worker, bool amountModifier, float &currentQuality, psTradeProcesses* process, psTradeTransformations* trans, csTicks time)
{
    if(calc_transform_apply_skill)
    {
        MathEnvironment env;
        env.Define("Quality", currentQuality);
        env.Define("Factor", factor);
        env.Define("Object", transItem);
        env.Define("Worker", worker);
        env.Define("Process", process);
        env.Define("Transform", trans);
        env.Define("Secure", secure);
        env.Define("AmountModifier", amountModifier);
        env.Define("CalculatedTime", time/1000);

        calc_transform_apply_skill->Evaluate(&env);
        currentQuality = env.Lookup("Quality")->GetValue();
        return (currentQuality > 0);
    }
    return false;
}

void WorkManager::SendTransformError(uint32_t clientNum, unsigned int result, uint32 curItemId, int curItemQty)
{
    csString error("");

    // There is some hierachy to the error codes
    //  for example, bad quantity is only important if no other error is found
    if(result & TRANSFORM_UNKNOWN_WORKITEM)
    {
        psserver->SendSystemError(clientNum, "Nothing has been chosen to use.");
        return;
    }
    if(result & TRANSFORM_FAILED_CONSTRAINTS)
    {
        // Message is sent by constraint logic.
        // error = "The conditions are not right for this to work.";
        return;
    }
    if(result & TRANSFORM_UNKNOWN_PATTERN)
    {
        psserver->SendSystemError(clientNum, "You are unable to think about what to do with this.");
        return;
    }
    if(result & TRANSFORM_BAD_TRAINING)
    {
        psserver->SendSystemError(clientNum, "You need some more training before you can do this sort of work.");
        return;
    }
    if(result & TRANSFORM_BAD_SKILLS)
    {
        psserver->SendSystemError(clientNum, "This is a bit beyond your skills at this moment.");
        return;
    }
    if(result & TRANSFORM_OVER_SKILLED)
    {
        psserver->SendSystemError(clientNum, "You loose interest in these simple tasks and cannot complete the work.");
        return;
    }
    if(result & TRANSFORM_NO_STAMINA)
    {
        psserver->SendSystemError(clientNum, "You are either too tired or not focused enough to do this now.");
        return;
    }
    if(result & TRANSFORM_MISSING_EQUIPMENT)
    {
        psserver->SendSystemError(clientNum, "You do not have the correct tools for this sort of work.");
        return;
    }
    if(result & TRANSFORM_BAD_QUANTITY)
    {
        psserver->SendSystemError(clientNum, "This doesn't seem to be the right amount to do anything useful.");
        return;
    }
    if(result & TRANSFORM_BAD_USE)
    {
        psserver->SendSystemError(clientNum, "There is nothing in this container that you own.");
        return;
    }
    if(result & TRANSFORM_TOO_MANY_ITEMS)
    {
        psserver->SendSystemError(clientNum, "You can only work on one item at a time this way.");
        return;
    }
    if(result & TRANSFORM_BAD_COMBINATION)
    {
        psserver->SendSystemError(clientNum, "You do not have the right amount of the right items for what you have in mind.");
        return;
    }
    if(result & TRANSFORM_MISSING_ITEM)
    {
        psserver->SendSystemError(clientNum, "You are not sure what to do with this.");
        return;
    }
    if(result & TRANSFORM_UNKNOWN_ITEM)
    {
        psItemStats* curStats = cacheManager->GetBasicItemStatsByID(curItemId);
        if(curStats)
        {
            psserver->SendSystemError(clientNum, "You are not sure what to do with %d %s.", curItemQty, curStats->GetName());
        }
        return;
    }
    if(result & TRANSFORM_GONE_WRONG)
    {
        psserver->SendSystemError(clientNum, "Something has obviously gone wrong.");
        return;
    }
    if(result & TRANSFORM_GARBAGE)
    {
        psItemStats* curStats = cacheManager->GetBasicItemStatsByID(curItemId);
        if(curStats)
        {
            psserver->SendSystemError(clientNum, "You are not sure what to do with this.");
        }
        return;
    }
    Error4("SendTransformError() got unknown result type #%d from item ID #%u (%d).", result, curItemId, curItemQty);
    return;
}

//-----------------------------------------------------------------------------
// Cleaning up discarded items from public containers
//-----------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop doing cleanup work
void WorkManager::StopCleanupWork(Client* client, psItem* cleanItem)
{
    // Check for proper autoItem
    if(!cleanItem)
    {
        Error1("StopCleanupWork does not have a good cleanItem pointer.");
        return;
    }

    // Stop the cleanup item's work event
    psWorkGameEvent* workEvent = cleanItem->GetTransformationEvent();
    if(workEvent)
    {
        cleanItem->SetTransformationEvent(NULL);
        workEvent->Interrupt();
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Start doing cleanup
void WorkManager::StartCleanupEvent(int transType, Client* client, psItem* item, gemActor* worker)
{
    if(!item)
    {
        Error1("No valid transformation item in StartCleanupEvent().\n");
        return;
    }

#ifdef DEBUG_WORKMANAGER
    // Let the server admin know
    CPrintf(CON_DEBUG, "Scheduled item id #%d to be cleaned up in %d seconds.\n", item->GetUID(), CLEANUP_DELAY);
#endif

    // Set event
    csVector3 pos(0,0,0);
    psWorkGameEvent* workEvent = new psWorkGameEvent(
        this, worker, CLEANUP_DELAY*1000, CLEANUP, pos, NULL, client);
    workEvent->SetTransformationType(transType);
    workEvent->SetTransformationItem(item);
    item->SetTransformationEvent(workEvent);
    psserver->GetEventManager()->Push(workEvent);

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Remove ownership of item making item free game.
void WorkManager::HandleCleanupEvent(psWorkGameEvent* workEvent)
{
#ifdef DEBUG_WORKMANAGER
    CPrintf(CON_DEBUG, "handling cleanup workEvent...\n");
#endif

    // Get event item
    psItem* cleanItem = workEvent->GetTranformationItem();
    if(cleanItem)
    {
        cleanItem->SetGuardingCharacterID(0);
        cleanItem->Save(true);
        cleanItem->SetTransformationEvent(NULL);
        psserver->SendSystemOK(workEvent->client->GetClientNum(),"You probably should not leave %d %s here.", cleanItem->GetStackCount(), cleanItem->GetName());
    }
}

//-----------------------------------------------------------------------------
// Lock picking
//-----------------------------------------------------------------------------

void WorkManager::StartLockpick(Client* client,psItem* item)
{
    // Check if its a lock
    if(!item->GetIsLockable())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"This object isn't lockable");
        return;
    }

    // Check if player has a key
    if(client->GetCharacterData()->Inventory().HaveKeyForLock(item->GetUID()))
    {
        bool locked = item->GetIsLocked();
        // Make sure the player knows he unlocked or locked his lock. Imagine guildhouse left unlocked on accident.
        if(locked)
        {
            psserver->SendSystemError(client->GetClientNum(), "You unlocked %s.", item->GetName());
        }
        else
        {
            psserver->SendSystemOK(client->GetClientNum(), "You locked %s.", item->GetName());
        }
        item->SetIsLocked(!locked);
        item->Save(false);
        return;
    }

    if(item->GetIsUnpickable())
    {
        psserver->SendSystemInfo(client->GetClientNum(),"This lock is impossible to %s by lockpicking.", item->GetIsLocked() ? "open" : "close");
        return;
    }

    // Make sure client isn't already busy digging, etc.
    gemActor* actor = client->GetActor();
    if(actor->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You cannot lockpick anything because you are %s.", actor->GetModeStr());
        return;
    }

    if(!CheckStamina(client->GetCharacterData()))
    {
        psserver->SendSystemInfo(client->GetClientNum(),"You are too tired to lockpick");
        return;
    }

    client->GetCharacterData()->SetStaminaRegenerationWork(item->GetLockpickSkill());

    psserver->SendSystemInfo(client->GetClientNum(),"You start lockpicking %s",item->GetName());
    actor->SetMode(PSCHARACTER_MODE_WORK);

    // Execute mathscript to get lockpicking time
    MathEnvironment env;
    env.Define("LockQuality", item->GetItemQuality());
    env.Define("Object", item);

    calc_lockpick_time->Evaluate(&env);
    MathVar* time = env.Lookup("Time");

    // Add new event
    csVector3 emptyV = csVector3(0,0,0);
    psWorkGameEvent* ev = new psWorkGameEvent(
        this,
        actor,
        time->GetRoundValue(),
        LOCKPICKING,
        emptyV,
        0,
        client,
        item);
    psserver->GetEventManager()->Push(ev);
}

void WorkManager::LockpickComplete(psWorkGameEvent* workEvent)
{

    if( workEvent->object==NULL  )
    {
        psserver->SendSystemInfo(workEvent->client->GetClientNum(),"Lockpick target is no longer valid." );
        workEvent->client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    if( workEvent->object->GetGemObject()==NULL )
    {
        psserver->SendSystemInfo(workEvent->client->GetClientNum(),"Lockpick target is no longer valid." );
        workEvent->client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
        return;
    }

    psCharacter* character = workEvent->client->GetCharacterData();
    PSSKILL skill = workEvent->object->GetLockpickSkill();

        // Check if not moved too far away from lock
        // First check if lock (object such as box) is in not inventory
        // For now, that is never the case, but I can imagine that changing
    if((PSCHARACTER_SLOT_NONE == workEvent->object->GetLocInParent()) &&
        (workEvent->client->GetActor()->RangeTo(workEvent->object->GetGemObject()) > RANGE_TO_USE))
    {
        // Send denied message
        psserver->SendSystemInfo(workEvent->client->GetClientNum(),"You failed your lockpicking attempt, because you moved away.");
    }
    else
    {
        // Check if the user has the right skills
        int rank = 0;
        if(skill >= 0 && skill < (PSSKILL)cacheManager->GetSkillAmount())
        {
            rank = character->Skills().GetSkillRank(skill).Current();
        }

        if(rank >= (int) workEvent->object->GetLockStrength())
        {
            bool locked = workEvent->object->GetIsLocked();
            psserver->SendSystemOK(workEvent->client->GetClientNum(), locked ? "You unlocked %s." : "You locked %s.", workEvent->object->GetName());
            workEvent->object->SetIsLocked(!locked);
            workEvent->object->Save(false);

            // Calculate practice points.

            int practicePoints;
            float modifier;
            {
                MathEnvironment env;
                env.Define("Object", workEvent->object);
                env.Define("Worker", workEvent->client->GetCharacterData());
                env.Define("RequiredSkill", skill);
                env.Define("PlayerSkill", rank);
                env.Define("LockStrength", workEvent->object->GetLockStrength());

                calc_lockpicking_exp->Evaluate(&env);
                practicePoints = env.Lookup("ResultPractice")->GetRoundValue();
                modifier = env.Lookup("ResultModifier")->GetValue();
            }

            // Assign points and exp.
            workEvent->client->GetCharacterData()->CalculateAddExperience(skill, practicePoints, modifier);
        }
        else
        {
            // Send denied message
            psserver->SendSystemInfo(workEvent->client->GetClientNum(),"You failed your lockpicking attempt.");
        }
    }
    workEvent->client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
}


//-----------------------------------------------------------------------------
// Events
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////
//  Primary work object
psWorkGameEvent::psWorkGameEvent(WorkManager* mgr,
                                 gemActor* worker,
                                 int delayticks,
                                 int cat,
                                 csVector3 &pos,
                                 csArray<NearNaturalResource>* natres,
                                 Client* c,
                                 psItem* object,
                                 float repairAmt)
    : psGameEvent(0,delayticks,"psWorkGameEvent"), gemTarget(NULL),
      transformation(NULL),process(NULL),effectID(0),resultQuantity(0),
      resultQuality(0.0f),KFactor(0.0f),transSlot(PSCHARACTER_SLOT_NONE),item(NULL),workItem(NULL),
      transType(0)
      
{
    workmanager = mgr;
    if(natres)
        nrr.Merge(*natres);
    client      = c;
    position    = pos;
    category    = cat;
    this->object = object;
    this->worker = worker;
    worker->RegisterCallback(this);
    repairAmount = repairAmt;
}


void psWorkGameEvent::Interrupt()
{
    // Setting the character mode ends up calling this function again, so we have to check for reentrancy here
    if(workmanager)
    {
        workmanager = NULL;

        // TODO: get tick count so far before killing event
        //  for partial trade work - store it in psItem
        //  leave work state alone so it can be restarted

        // Stop event from being executed when triggered.
        if(category == REPAIR && client->GetActor() && (client->GetActor()->GetMode() == PSCHARACTER_MODE_WORK))
        {
            client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
            if(object)
                object->SetInUse(false);
        }
    }
}


// Event trigger
void psWorkGameEvent::Trigger()
{
    if(workmanager && worker.IsValid())
    {
        if(effectID != 0)
        {
            psStopEffectMessage newmsg(effectID);
            newmsg.Multicast(multi,0,PROX_LIST_ANY_RANGE);
        }

        // Work done for now
        switch(category)
        {
            case(REPAIR):
            {
                workmanager->HandleRepairEvent(this);
                break;
            }
            case(MANUFACTURE):
            {
                workmanager->HandleWorkEvent(this);
                break;
            }
            case(PRODUCTION):
            {
                workmanager->HandleProductionEvent(this);
                break;
            }
            case(LOCKPICKING):
            {
                workmanager->LockpickComplete(this);
                break;
            }
            case(CLEANUP):
            {
                workmanager->HandleCleanupEvent(this);
                break;
            }
            default:
                Error1("Unknown transformation type!");
                break;
        }
    }
}

void psWorkGameEvent::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    if(workmanager && worker.IsValid())
    {
        worker->UnregisterCallback(this);

        // Cleanup any autoitem associated with event
        if(transType == TRANSFORMTYPE_AUTO_CONTAINER)
        {
            if(item)
            {

#ifdef DEBUG_WORKMANAGER
                CPrintf(CON_DEBUG, "Cleaning up item from auto-transform container...\n");
#endif
                // clear out event
                item->SetTransformationEvent(NULL);

            }
            Interrupt();
        }
        else
        {
            // Setting character to peace will interrupt work event
            worker->SetTradeWork(NULL);
            worker->SetMode(PSCHARACTER_MODE_PEACE);
        }
    }
}

psWorkGameEvent::~psWorkGameEvent()
{
    if(worker.IsValid())
    {
        worker->UnregisterCallback(this);
        if(worker->GetTradeWork() == this)
            worker->SetTradeWork(NULL);
    }

    // For manufactuing events only
    if(category == MANUFACTURE)
    {
        // Remove any item reference to this  event
        psItem* transItem = GetTranformationItem();
        if(transItem && transItem->GetTransformationEvent() == this)
            transItem->SetTransformationEvent(NULL);

        // The garbage transformation is not cached and needs to be cleaned up
        if(transformation && !transformation->GetTransformationCacheFlag())
        {
            delete transformation;
        }
    }
}
