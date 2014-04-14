/*
 * psnpcloader.cpp - Author: Ian Donderwinkel
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"

#include "../clients.h"
#include "../psserver.h"
#include "../exchangemanager.h"
#include "../cachemanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psnpcloader.h"
#include "pscharacter.h"
#include "pscharacterloader.h"
#include "psnpcdialog.h"
#include "pstrainerinfo.h"
#include "psraceinfo.h"
#include "pssectorinfo.h"
#include "psmerchantinfo.h"
#include "psguildinfo.h"




#define SUPER_CLIENT_ACCOUNT    9



//-----------------------------------------------------------------------------
bool psNPCLoader::LoadFromFile(csString &filename)
{
    area.Clear();

    CPrintf(CON_DEBUG, "Importing NPC from file: %s\n",filename.GetData());

    // try to read data from the file

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());
    csRef<iDataBuffer> data(vfs->ReadFile(filename.GetData()));
    if(!data || !data->GetSize())
    {
        CPrintf(CON_ERROR, "Error: Couldn't load file '%s'.\n", filename.GetData());
        return false;
    }

    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(data);
    if(error)
    {
        Error2("Error in XML: %s", error);
        return false;
    }
    npcRoot = doc->GetRoot()->GetNode("npc");

    if(!npcRoot)
    {
        CPrintf(CON_ERROR, "Error: no <npc> tag found\n");
        return false;
    }


    dialogManager = new psDialogManager;
    npc = new psCharacter;
    npc->SetCharType(PSCHARACTER_TYPE_NPC);

    // process xml data
    if(!ReadBasicInfo() || !ReadLocation())
    {
        CPrintf(CON_ERROR, "Error: Failed to load NPC data\n");
        return false;
    }
    ReadDescription();
    ReadTraits();
    ReadStats();
    ReadMoney();
    ReadSkills();
    ReadTrainerInfo();
    ReadMerchantInfo();
    ReadFactions();
    ReadKnowledgeAreas();
    ReadSpecificKnowledge();
    ReadSpecialResponses();

    // insert processed data into the database
    if(!WriteToDatabase())
    {
        CPrintf(CON_ERROR, "Error: Failed to insert data into the database\n");
        return false;
    }

    return true;
}


bool psNPCLoader::LoadDialogsFromFile(csString &filename)
{
    // <npcdialogs>
    //   <specificknowledge>
    //     ..
    //   </specificknowledge>
    //   <specialresponses>
    //     ..
    //   </specialresponses>
    // </npcdialogs>
    // <quest name="name" task="task" ...>
    //   <npcdialogs>
    //     <specificknowledge>
    //       ..
    //     </specificknowledge>
    //   </npcdialogs>
    //   <queststep>
    //     <specificknowledge>
    //         ..
    //     </specificknowledge>
    //     <specialresponses>
    //     ..
    //     </specialresponses>
    //   </queststep>
    //   <queststep>
    //   ..
    // </quest>
    area.Clear();
    dialogManager = new psDialogManager;

    CPrintf(CON_DEBUG, "Importing NPC dialogs from file: %s\n",filename.GetData());

    // try to read data from the file
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());
    csRef<iDataBuffer> data = vfs->ReadFile(filename.GetData());
    if(!data || !data->GetSize())
    {
        CPrintf(CON_ERROR, "Error: Couldn't load file '%s'.\n", filename.GetData());
        return false;
    }

    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(data);
    if(error)
    {
        Error2("Error in XML: %s", error);
        return false;
    }

    npcRoot = doc->GetRoot()->GetNode("npcdialogs");
    if(!npcRoot)
    {
        csRef<iDocumentNode> questNode = doc->GetRoot()->GetNode("quest");
        if(!questNode)
        {

            CPrintf(CON_ERROR, "Error: no <npcdialogs> or <quest> tag found\n");
            return false;
        }
        csString name(questNode->GetAttributeValue("name"));
        csString task(questNode->GetAttributeValue("task"));

        // Find a new unused quest id from the DB
        Result nextId(db->Select("SELECT MAX(id) FROM quests"));
        int questID;
        int masterQuestID = questID = nextId[0].GetInt(0) + 1;



        CPrintf(CON_DEBUG, "Importing master quest to id %i\n", masterQuestID);

        npcRoot = questNode->GetNode("npcdialogs");
        if(!npcRoot)
        {
            CPrintf(CON_ERROR, "Error: no <npcdialogs> tag found\n");
        }
        ReadSpecificKnowledge(questID);
        csString escName;
        csString escTask;
        db->Escape(escName, name.GetDataSafe());
        db->Escape(escTask, task.GetDataSafe());

        db->Command("INSERT INTO quests (id,name,task,master_quest_id,player_lockout_time,quest_lockout_time) VALUES('%i','%s','%s','0','%i','%i')", masterQuestID, escName.GetDataSafe(), escTask.GetDataSafe(),
                    questNode->GetAttributeValueAsInt("player_lockout_time"), questNode->GetAttributeValueAsInt("quest_lockout_time"));

        csRef<iDocumentNodeIterator> iter = questNode->GetNodes("queststep");
        while(iter->HasNext())
        {
            csRef<iDocumentNode> questStepNode = iter->Next();
            csString name(questStepNode->GetAttributeValue("name"));
            csString task(questStepNode->GetAttributeValue("task"));
            // int minor_step_number = questStepNode->GetAttributeValueAsInt("minor_step_number");
            npcRoot = questStepNode->GetNode("npcdialogs");
            questID++;
            CPrintf(CON_DEBUG, "Importing queststep to id %i\n", questID);
            ReadSpecificKnowledge(questID);
            ReadSpecialResponses(questID);
            db->Command("INSERT INTO quests (id,name,task,master_quest_id,minor_step_number,player_lockout_time,quest_lockout_time) VALUES('%i','%s','%s','%i','%i','%i','%i')", questID,
                        escName.GetData(), escTask.GetData(), masterQuestID, questStepNode->GetAttributeValueAsInt("minor_step_number"),
                        questStepNode->GetAttributeValueAsInt("player_lockout_time"), questStepNode->GetAttributeValueAsInt("quest_lockout_time"));
        }
    }
    else
    {
        this->area = npcRoot->GetAttributeValue("area");

        CPrintf(CON_DEBUG, "Importing area '%s' dialogs\n", area.GetDataSafe());
        // process xml data
        ReadSpecificKnowledge();
        ReadSpecialResponses();


        /*
        // check if area is a valid knowledge area
        Result result(db->Select("SELECT  *"
        "FROM npc_knowledge_areas "
        "WHERE area=\"%s\";", area.GetData()));

        if (result.Count()==0)
        {
        CPrintf(CON_ERROR, "Error: Area %s isn't a valid knowledge area\n", area.GetData());
        return false;
        }
        */
    }
    // write npc dialog(s) to database
    dialogManager->area = area.GetDataSafe();
    dialogManager->WriteToDatabase();

    return true;
}


bool psNPCLoader::ReadBasicInfo()
{
    csString name, race, sex, invulnerable;
    float kill_exp;

    name = npcRoot->GetAttributeValue("name");
    race = npcRoot->GetAttributeValue("race");
    sex = npcRoot->GetAttributeValue("sex");
    invulnerable = npcRoot->GetAttributeValue("invulnerable");
    kill_exp = npcRoot->GetAttributeValueAsFloat("kill_exp");

    if(name.IsEmpty())
    {
        CPrintf(CON_ERROR, "Error: No name specified\n");
        return false;
    }

    if(race.IsEmpty())
    {
        CPrintf(CON_ERROR, "Error: No race specified\n");
        return false;
    }

    if(sex.IsEmpty())
    {
        CPrintf(CON_ERROR, "Error: No gender specified\n");
        return false;
    }

    PSCHARACTER_GENDER gender;
    if(sex=='f'||sex=='F')
        gender = PSCHARACTER_GENDER_FEMALE;
    else if(sex=='m' || sex=='M')
        gender = PSCHARACTER_GENDER_MALE;
    else if(sex=='n' || sex=='N')
        gender = PSCHARACTER_GENDER_NONE;
    else
    {
        CPrintf(CON_ERROR, "Error: Unknown gender: %s\n", sex.GetData());
        return false;
    }

    psRaceInfo* raceInfo = psserver->GetCacheManager()->GetRaceInfoByNameGender(race, gender);
    if(!raceInfo)
    {
        CPrintf(CON_ERROR, "Error: Unknown race: %s gender: %s\n",race.GetData(), sex.GetData());
        return false;
    }

    npc->SetName(name);
    npc->SetRaceInfo(raceInfo);
    npc->SetImperviousToAttack((invulnerable=="Y") ? ALWAYS_IMPERVIOUS : 0);
    npc->SetKillExperience((int) kill_exp);

    return true;
}

bool psNPCLoader::ReadDescription()
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("description");
    if(!xmlnode)
    {
        CPrintf(CON_ERROR, "Error: no <description> tag found\n");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes();
    while(iter->HasNext())
    {
        csRef<iDocumentNode> desc = iter->Next();
        if(desc->GetType() == CS_NODE_TEXT)
        {
            npc->SetDescription(desc->GetValue());
            return true;
        }
    }
    CPrintf(CON_ERROR, "Error: description in <description> tag found\n");
    return false;
}

bool psNPCLoader::ReadLocation()
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("pos");
    if(!xmlnode)
    {
        CPrintf(CON_ERROR, "Error: no <pos> tag found\n");
        return false;
    }

    csVector3 pos, rot;
    csString  sector;
    InstanceID instance;

    instance = DEFAULT_INSTANCE;
    const char* inst_str = xmlnode->GetAttributeValue("instance");
    if(inst_str)
    {
        instance = strtoul(inst_str,NULL,10);
    }
    pos.x = xmlnode->GetAttributeValueAsFloat("x");
    pos.y = xmlnode->GetAttributeValueAsFloat("y");
    pos.z = xmlnode->GetAttributeValueAsFloat("z");
    rot.x = xmlnode->GetAttributeValueAsFloat("xr");
    rot.y = xmlnode->GetAttributeValueAsFloat("yr");
    rot.z = xmlnode->GetAttributeValueAsFloat("zr");
    sector = xmlnode->GetAttributeValue("sct");

    psSectorInfo*   sectorInfo = psserver->GetCacheManager()->GetSectorInfoByName(sector);
    if(sectorInfo==NULL)
    {
        CPrintf(CON_ERROR, "Error: Couldn't find a sector with name: %s\n",sector.GetData());
        return false;
    }
    npc->SetLocationInWorld(instance,sectorInfo, pos.x, pos.y, pos.z, rot.y);

    return true;
}


void psNPCLoader::ReadTraits()
{
    csString beard, face, hair, skin;

    beard = npcRoot->GetAttributeValue("beard");
    face = npcRoot->GetAttributeValue("face");
    hair = npcRoot->GetAttributeValue("hair");
    skin = npcRoot->GetAttributeValue("skin");

    psTrait* faceTrait   = psserver->GetCacheManager()->GetTraitByName(face);
    psTrait* hairTrait  = psserver->GetCacheManager()->GetTraitByName(hair);
    psTrait* beardTrait = psserver->GetCacheManager()->GetTraitByName(beard);
    psTrait* skinTrait  = psserver->GetCacheManager()->GetTraitByName(skin);

    if(faceTrait)
        npc->SetTraitForLocation(PSTRAIT_LOCATION_FACE, faceTrait);

    if(hairTrait)
        npc->SetTraitForLocation(PSTRAIT_LOCATION_HAIR_STYLE, hairTrait);

    if(beardTrait)
        npc->SetTraitForLocation(PSTRAIT_LOCATION_BEARD_STYLE, beardTrait);

    if(skinTrait)
        npc->SetTraitForLocation(PSTRAIT_LOCATION_SKIN_TONE, skinTrait);
}


void psNPCLoader::ReadMoney()
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("money");
    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <money> tag found\n");
        return;
    }

    float circles, hexas, octas, trias;

    circles = xmlnode->GetAttributeValueAsFloat("circles");
    hexas = xmlnode->GetAttributeValueAsFloat("hexas");
    octas = xmlnode->GetAttributeValueAsFloat("octas");
    trias = xmlnode->GetAttributeValueAsFloat("trias");

    psMoney money((int)circles, (int)octas, (int)hexas, (int)trias);
    npc->SetMoney(money);
}


void psNPCLoader::ReadStats()
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("stats");

    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <stats> tag found\n");
        return;
    }

    MathEnvironment env;
    env.Define("Actor", npc);
    env.Define("STR", xmlnode->GetAttributeValueAsFloat("agi"));
    env.Define("AGI", xmlnode->GetAttributeValueAsFloat("cha"));
    env.Define("END", xmlnode->GetAttributeValueAsFloat("end"));
    env.Define("INT", xmlnode->GetAttributeValueAsFloat("int"));
    env.Define("WILL", xmlnode->GetAttributeValueAsFloat("str"));
    env.Define("CHA", xmlnode->GetAttributeValueAsFloat("wil"));

    psserver->GetCacheManager()->GetSetBaseSkillsScript()->Evaluate(&env);
}


void psNPCLoader::ReadKnowledgeAreas()
{
    knowledgeAreas.Empty();
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("knowledgeareas");
    if(!xmlnode)
    {
        CPrintf(CON_ERROR, "Error: no <knowledgeareas> tag found\n");
        return;
    }
    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("knowledgearea");
    while(iter->HasNext())
    {
        csRef<iDocumentNode> areanode = iter->Next();
        csString area;
        int priority;
        area = areanode->GetAttributeValue("name");
        priority = areanode->GetAttributeValueAsInt("priority");

        if(area.IsEmpty() || priority==0)
        {
            CPrintf(CON_ERROR, "Error: invalid name or priority in <knowledgearea> \n");
            return;
        }
        knowledgeAreas.Push(area);
        knowledgeAreasPriority.Push(priority);
    }
}


void psNPCLoader::ReadTrainerInfo()
{
    trainerSkills.Empty();
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("trains");
    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <trains> tag found\n");
        return;
    }
    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("train");
    while(iter->HasNext())
    {
        csString skill;
        int      min_rank;
        int         max_rank;
        int      min_faction;

        csRef<iDocumentNode> trainnode = iter->Next();

        skill = trainnode->GetAttributeValue("name");
        min_rank = trainnode->GetAttributeValueAsInt("min_rank");
        max_rank = trainnode->GetAttributeValueAsInt("max_rank");
        min_faction = trainnode->GetAttributeValueAsInt("min_faction");

        psSkillInfo*    skillInfo = psserver->GetCacheManager()->GetSkillByName(skill);
        if(skillInfo!=NULL)
        {
            psTrainerSkill trainerSkill;

            trainerSkill.skill       = skillInfo;
            trainerSkill.min_rank    = min_rank;
            trainerSkill.max_rank    = max_rank;
            trainerSkill.min_faction = min_faction;

            trainerSkills.Push(trainerSkill);
        }
        else
        {
            CPrintf(CON_WARNING, "Unknown skill for training '%s'... skipping\n",skill.GetData());
        }
    }
}


void psNPCLoader::ReadSkills()
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("skills");
    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <skills> tag found\n");
        return;
    }
    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("skill");
    while(iter->HasNext())
    {
        csString skill;
        int      value;

        csRef<iDocumentNode> skillnode = iter->Next();

        skill = skillnode->GetAttributeValue("name");
        value = skillnode->GetAttributeValueAsInt("value");

        psSkillInfo*    skillInfo = psserver->GetCacheManager()->GetSkillByName(skill);
        if(skillInfo!=NULL)
        {
            npc->Skills().SetSkillRank(skillInfo->id, value);
        }
        else
        {
            CPrintf(CON_WARNING, "Unknown skill '%s'... skipping\n",skill.GetData());
        }
    }
}


void psNPCLoader::ReadFactions()
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("factions");
    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <factions> tag found\n");
        return;
    }
    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("faction");
    while(iter->HasNext())
    {
        csString name;
        int      value;

        csRef<iDocumentNode> factionnode = iter->Next();

        name = factionnode->GetAttributeValue("name");
        value = factionnode->GetAttributeValueAsInt("value");

        // get faction id
        int factionID = db->SelectSingleNumber("SELECT id FROM factions WHERE faction_name='%s'",name.GetData());
        csString temp;

        if(factionID!=-1)
        {
            if(factionStandings=="")
                factionStandings+= temp.Format("%i,%i",factionID,value);
            else
                factionStandings+= temp.Format(",%i,%i",factionID,value);
        }
        else
        {
            CPrintf(CON_WARNING, "Unknown faction: '%s'... skipping\n",name.GetData());
        }
    }
}


void psNPCLoader::ReadSpecificKnowledge(int questID)
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("specificknowledge");
    if(!xmlnode)
    {
        CPrintf(CON_ERROR, "Error: no <specificknowledge> tag found\n");
        return;
    }

    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("trigger");

    while(iter->HasNext())
    {
        csRef<iDocumentNode> triggernode = iter->Next();

        // Create a new trigger
        psTriggerBlock* trigger = new psTriggerBlock(dialogManager);
        trigger->Create(triggernode, questID);
        dialogManager->AddTrigger(trigger);
    }
}


void psNPCLoader::ReadSpecialResponses(int questID)
{
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("specialresponses");
    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <specialresponses> tag found\n");
        return;
    }

    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("specialresponse");
    while(iter->HasNext())
    {
        csRef<iDocumentNode> responseNode = iter->Next();
        psSpecialResponse* special = new psSpecialResponse(responseNode, questID);
        dialogManager->AddSpecialResponse(special);
    }
}


void psNPCLoader::ReadMerchantInfo()
{
    buys.Empty();
    sells.Empty();
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("buys");
    if(!xmlnode)
        CPrintf(CON_WARNING, "Warning: no <buys> tag found\n");
    else
    {
        csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("buy");
        while(iter->HasNext())
        {
            csString name;

            csRef<iDocumentNode> buynode = iter->Next();
            name = buynode->GetAttributeValue("name");

            psItemCategory* itemCategory = psserver->GetCacheManager()->GetItemCategoryByName(name);

            if(itemCategory!=NULL)
            {
                buys.Push(itemCategory->id);
            }
            else
            {
                CPrintf(CON_WARNING, "Unknown Item Category (Buy): '%s'... skipping\n",name.GetData());
            }
        }
    }

    xmlnode = npcRoot->GetNode("sells");
    if(!xmlnode)
        CPrintf(CON_WARNING, "Warning: no <sells> tag found\n");
    else
    {
        csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("sell");
        while(iter->HasNext())
        {
            // read data from <sells> section
            csString name;
            csRef<iDocumentNode> sellnode = iter->Next();
            name = sellnode->GetAttributeValue("name");

            psItemCategory* itemCategory = psserver->GetCacheManager()->GetItemCategoryByName(name);

            if(itemCategory!=NULL)
            {
                sells.Push(itemCategory->id);
            }
            else
            {
                CPrintf(CON_WARNING, "Unknown Item Category (Sell): '%s'... skipping\n",name.GetData());
            }
        }
    }
}


void psNPCLoader::SetupEquipment()
{
    // Read data from <equipment> section
    csRef<iDocumentNode> xmlnode = npcRoot->GetNode("equipment");
    if(!xmlnode)
    {
        CPrintf(CON_WARNING, "Warning: no <equipment> tag found\n");
        return;
    }
    csRef<iDocumentNodeIterator> iter = xmlnode->GetNodes("obj");
    while(iter->HasNext())
    {
        csString name, crafter, guild, location;
        int stack_count;
        float item_quality;
        int slot;
        csRef<iDocumentNode> objNode = iter->Next();

        name = objNode->GetAttributeValue("name");
        stack_count = objNode->GetAttributeValueAsInt("stack_count");
        item_quality = objNode->GetAttributeValueAsFloat("item_quality");
        crafter = objNode->GetAttributeValue("crafter");
        guild = objNode->GetAttributeValue("guild");
        location = objNode->GetAttributeValue("location");
        slot = objNode->GetAttributeValueAsInt("slot");

        psItem* newItem = new psItem();  // Not calling Loaded(), and will not ever save

        psItemStats* stat = psserver->GetCacheManager()->GetBasicItemStatsByName(name);
        if(!stat)
        {
            Error2("Could not load basic item stat for %s", name.GetData());
            continue;
        }

        newItem->SetBaseStats(stat);
        if(stack_count != -1)
            newItem->SetStackCount(stack_count);
        if(item_quality)
            newItem->SetItemQuality(item_quality);
        if(!crafter.IsEmpty())
        {
            PID craftid = psServer::CharacterLoader.FindCharacterID(crafter.GetData());
            newItem->SetCrafterID(craftid);
        }
        if(!guild.IsEmpty())
        {
            psGuildInfo* newguild=psserver->GetCacheManager()->FindGuild(guild.GetData());
            if(newguild)
            {
                newItem->SetGuildID(newguild->GetID());
            }
        }
        if(location == "equipped")
        {
            if(!npc->Inventory().AddLoadedItem(0,(INVENTORY_SLOT_NUMBER)slot, newItem))
            {
                Error2("Could not equip item %s on npc", name.GetData());
                delete newItem;
                continue;
            }
        }
        else
        {
            if(!npc->Inventory().Add(newItem))
            {
                Error2("Could not put item %s on npc", name.GetData());
                delete newItem;
                continue;
            }
        }
        newItem->Save(true);
    }
}


bool psNPCLoader::WriteToDatabase()
{
    // write npc character data to database
    psCharacterLoader charloader;
    charloader.NewCharacterData(SUPER_CLIENT_ACCOUNT, npc);

    csString name = npc->GetCharName();
    PID      pid  = npc->GetPID();
    csString invulnerable = (npc->GetImperviousToAttack() & ALWAYS_IMPERVIOUS) ? "Y" : "N";
    int kill_exp  = npc->GetKillExperience();

    // add a spawn rule for the npc
    db->Command("UPDATE characters c SET c.npc_spawn_rule=1 WHERE c.id=%u;", pid.Unbox());

    // update factions
    db->Command("UPDATE characters c SET c.faction_standings='%s' WHERE c.id=%i;",
                factionStandings.GetDataSafe(), pid.Unbox());

    // update npc_master_id
    db->Command("UPDATE characters c SET c.npc_master_id=%u WHERE c.id=%u;", pid.Unbox(), pid.Unbox());

    // update invulnerable and kill_exp
    db->Command("UPDATE characters c SET c.npc_impervious_ind='%s', kill_exp=%i WHERE c.id=%u;", invulnerable.GetData(), kill_exp, pid.Unbox());

    // This can only be called after the character has been initialised.
    SetupEquipment();

    // write knowledge areas to database
    for(size_t i=0; i<knowledgeAreas.GetSize(); i++)
    {
        int priority = knowledgeAreasPriority[i];
        db->Command("INSERT INTO npc_knowledge_areas(player_id, area, priority) "
                    "VALUES (%d, '%s', '%d')",
                    pid.Unbox(), knowledgeAreas[i].GetData(), priority);
    }


    // write npcdialog (triggers and (special)responses) to database
    dialogManager->area = name;
    dialogManager->WriteToDatabase();

    // write trainer info to database
    for(size_t i=0; i<trainerSkills.GetSize(); i++)
    {
        db->Command("INSERT INTO trainer_skills(player_id, skill_id, min_rank, max_rank, min_faction) "
                    "VALUES (%i, %i, %i, %i, %f)",
                    pid.Unbox(),
                    trainerSkills[i].skill->id,
                    trainerSkills[i].min_rank,
                    trainerSkills[i].max_rank,
                    trainerSkills[i].min_faction);
    }

    // write merchant info to database
    for(size_t i=0; i<buys.GetSize(); i++)
        db->Command("INSERT INTO merchant_item_categories VALUES(%u,%d)", pid.Unbox(), buys[i]);

    for(size_t i=0; i<sells.GetSize(); i++)
        db->Command("INSERT INTO merchant_item_categories VALUES(%u,%d)", pid.Unbox(), sells[i]);

    CPrintf(CON_DEBUG, "The NPC has been assigned the following id: %u\n", npc->GetPID().Unbox());

    return true;
}

//-----------------------------------------------------------------------------
bool psNPCLoader::SaveToFile(int id, csString &filename)
{
    filename.Insert(0,"/this/");

    psCharacterLoader loader;
    npc = loader.LoadCharacterData(id,false);

    if(!npc)
    {
        CPrintf(CON_ERROR, "Error: Couldn't load NPC with id: %i\n",id);
        return false;
    }

    //*npc = *character;  // removed



    npcID = id;
    area  = npc->GetCharName();

    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot();
    npcRoot = root->CreateNodeBefore(CS_NODE_ELEMENT);
    npcRoot->SetValue("npc");


    WriteBasicInfo();
    WriteDescription();
    WriteLocation();
    WriteStats();
    WriteMoney();
    WriteEquipment();
    WriteFactions();
    WriteSkills();
    WriteMerchantInfo();
    WriteTrainerInfo();
    WriteKnowledgeAreas();
    WriteSpecificKnowledge();

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());

    if(!vfs)
        return false;

    csString error = doc->Write(vfs, filename);
    if(!error.IsEmpty())
    {
        CPrintf(CON_ERROR, "Error writing to file %s: %s\n",filename.GetData(), error.GetData());
        return false;
    }

    return true;
}


bool psNPCLoader::SaveDialogsToFile(csString &area, csString &filename, int questid, bool quest)
{
    filename.Insert(0,"/this/");

    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot();

    if(quest)
    {
        this->area.Clear();

        csRef<iDocumentNode> questNode = root->CreateNodeBefore(CS_NODE_ELEMENT);
        questNode->SetValue("quest");

        Result questInfo(db->Select("SELECT name, task, player_lockout_time, quest_lockout_time FROM quests WHERE id = '%i'", questid));
        questNode->SetAttribute("name", questInfo[0][0]);
        questNode->SetAttribute("task", questInfo[0][1]);
        questNode->SetAttributeAsInt("player_lockout_time", questInfo[0].GetInt(2));
        questNode->SetAttributeAsInt("quest_lockout_time", questInfo[0].GetInt(3));

        npcRoot = questNode->CreateNodeBefore(CS_NODE_ELEMENT);
        npcRoot->SetValue("npcdialogs");
        if(!WriteSpecificKnowledge(questid))
        {
            CPrintf(CON_ERROR,"Error writing specific knowledge.\n");
            return false;
        }

        Result questSteps(db->Select("SELECT id, name, task, minor_step_number, player_lockout_time, quest_lockout_time FROM quests WHERE master_quest_id = '%i' ORDER BY minor_step_number", questid));
        for(unsigned int i = 0; i<questSteps.Count(); i++)
        {
            csRef<iDocumentNode> questStepNode = questNode->CreateNodeBefore(CS_NODE_ELEMENT);
            questStepNode->SetValue("queststep");
            questStepNode->SetAttribute("name", questInfo[i][1]);
            questStepNode->SetAttribute("task", questInfo[i][2]);
            if(questSteps[i].GetInt(3))
                questStepNode->SetAttributeAsInt("minor_step_number", questSteps[i].GetInt(3));
            if(questSteps[i].GetInt(4))
                questStepNode->SetAttributeAsInt("player_lockout_time", questInfo[i].GetInt(4));
            if(questSteps[i].GetInt(5))
                questStepNode->SetAttributeAsInt("quest_lockout_time", questInfo[i].GetInt(5));

            npcRoot = questStepNode->CreateNodeBefore(CS_NODE_ELEMENT);
            npcRoot->SetValue("npcdialogs");
            if(!WriteSpecificKnowledge(questSteps[i].GetInt(0)))
            {
                CPrintf(CON_ERROR,"Error writing specific knowledge.\n");
                return false;
            }
        }
    }
    else            // This is when exporting areas or queststeps
    {
        npcRoot = root->CreateNodeBefore(CS_NODE_ELEMENT);
        npcRoot->SetValue("npcdialogs");

        csString temp;
        if(questid == -1)
        {
            npcRoot->SetAttribute("area", area);
            this->area = area;
        }
        else
            this->area.Clear();

        if(!WriteSpecificKnowledge(questid))
        {
            CPrintf(CON_ERROR,"Error writing specific knowledge.\n");
            return false;
        }
    }

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());

    if(!vfs)
        return false;

    csString error = doc->Write(vfs, filename);
    if(!error.IsEmpty())
    {
        CPrintf(CON_ERROR, "Error writing to file %s: %s\n",filename.GetData(), error.GetData());
        return false;
    }

    return true;
}

void psNPCLoader::WriteBasicInfo()
{
    psRaceInfo* raceInfo = npc->GetOverridableRace().Base();
    csString gender;
    if(raceInfo->gender==PSCHARACTER_GENDER_FEMALE)
        gender = "f" ;
    else if(raceInfo->gender==PSCHARACTER_GENDER_MALE)
        gender = "m";
    else
        gender = "n";

    npcRoot->SetAttribute("name", npc->GetCharName());
    npcRoot->SetAttribute("race", raceInfo->GetName());
    npcRoot->SetAttribute("sex", gender);
    npcRoot->SetAttribute("invulnerable", (npc->GetImperviousToAttack() & ALWAYS_IMPERVIOUS) ? "Y":"N");
    npcRoot->SetAttributeAsInt("kill_exp", npc->GetKillExperience());
}


void psNPCLoader::WriteDescription()
{
    csRef<iDocumentNode> descNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    descNode->SetValue("description");
    csRef<iDocumentNode> descText = descNode->CreateNodeBefore(CS_NODE_TEXT);
    descText->SetValue(npc->GetDescription());
}

void psNPCLoader::WriteLocation()
{
    csRef<iDocumentNode> posNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    posNode->SetValue("pos");
    posNode->SetAttribute("sct", npc->GetLocation().loc_sector->name.GetData());
    posNode->SetAttributeAsFloat("x", npc->GetLocation().loc.x);
    posNode->SetAttributeAsFloat("y", npc->GetLocation().loc.y);
    posNode->SetAttributeAsFloat("z", npc->GetLocation().loc.z);
    posNode->SetAttributeAsFloat("xr", 0.0f);
    posNode->SetAttributeAsFloat("yr", npc->GetLocation().loc_yrot);
    posNode->SetAttributeAsFloat("zr", 0.0f);
}


void psNPCLoader::WriteStats()
{
    csRef<iDocumentNode> statsNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    statsNode->SetValue("stats");

    MathEnvironment baseSkillVal;
    npc->GetSkillBaseValues(&baseSkillVal);

    statsNode->SetAttributeAsInt("agi", baseSkillVal.Lookup("AGI")->GetRoundValue());
    statsNode->SetAttributeAsInt("cha", baseSkillVal.Lookup("CHA")->GetRoundValue());
    statsNode->SetAttributeAsInt("end", baseSkillVal.Lookup("END")->GetRoundValue());
    statsNode->SetAttributeAsInt("int", baseSkillVal.Lookup("INT")->GetRoundValue());
    statsNode->SetAttributeAsInt("str", baseSkillVal.Lookup("STR")->GetRoundValue());
    statsNode->SetAttributeAsInt("wil", baseSkillVal.Lookup("WIL")->GetRoundValue());
}


void psNPCLoader::WriteMoney()
{
    csRef<iDocumentNode> moneyNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    moneyNode->SetValue("money");
    moneyNode->SetAttributeAsInt("circles", npc->Money().GetCircles());
    moneyNode->SetAttributeAsInt("hexas", npc->Money().GetHexas());
    moneyNode->SetAttributeAsInt("octas", npc->Money().GetOctas());
    moneyNode->SetAttributeAsInt("trias", npc->Money().GetTrias());
}


void psNPCLoader::WriteKnowledgeAreas()
{
    Result result(db->Select("SELECT area, priority "
                             "FROM npc_knowledge_areas "
                             "WHERE player_id=%i "
                             "ORDER BY priority;",
                             npcID));

    csRef<iDocumentNode> areasNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    areasNode->SetValue("knowledgeareas");

    for(int i=0; i < (int)result.Count(); i++)
    {
        csRef<iDocumentNode> areaNode = areasNode->CreateNodeBefore(CS_NODE_ELEMENT);

        areaNode->SetValue("knowledgearea");
        areaNode->SetAttribute("name", result[i][0]);
        areaNode->SetAttribute("priority", result[i][1]);
    }
}


bool psNPCLoader::WriteSpecificKnowledge(int questid)
{
    csRef<iDocumentNode> specificsNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    specificsNode->SetValue("specificknowledge");

    Result result;

    if(questid == -1)
        result = db->Select("SELECT DISTINCT trigger "
                            "FROM npc_triggers "
                            "WHERE prior_response_required=0 and area=\"%s\" and quest_id<1 "
                            ,area.GetData());
    else
        result = db->Select("SELECT DISTINCT trigger "
                            "FROM npc_triggers "
                            "WHERE prior_response_required=0 and quest_id=%i "
                            ,questid);

    if(result.IsValid())
    {
        for(int i=0; i < (int)result.Count(); i++)
        {
            csString res(result[i][0]);
            if(!WriteTrigger(specificsNode, res, 0, questid))
                return false;
        }
    }
    return true;
}

bool psNPCLoader::WriteResponse(csRef<iDocumentNode> attitudeNode, int id, int questID)
{
    // <attitude max=".." min="..">
    //   <pronoun him=".." her=".." it=".." them=".."/>
    //   <response say="..."/>
    //   <response say="..."/>
    //   <script>
    //      ...
    //   </script>
    // </attitude>

    Result result(db->Select("SELECT  response1, response2, response3, response4, response5, "
                             "pronoun_him, pronoun_her, pronoun_it, pronoun_them, script, quest_id "
                             "FROM npc_responses "
                             "WHERE id=\"%i\";", id));

    // Sanity check for quest_id
    if(questID != -1)
    {
        for(unsigned int i = 0; i < result.Count(); i++)
        {
            if(result[i].GetInt(10) != questID)
            {
                CPrintf(CON_ERROR, "Sanity check failed: Response id %i does not have expected quest id %i\n", id, questID);
                return false;
            }
        }
    }

    csString him  = result[0][5];
    csString her  = result[0][6];
    csString it   = result[0][7];
    csString them = result[0][8];

    if(!him.IsEmpty() || !her.IsEmpty() || !it.IsEmpty() || !them.IsEmpty())
    {
        csRef<iDocumentNode> pronounNode = attitudeNode->CreateNodeBefore(CS_NODE_ELEMENT);

        pronounNode->SetValue("pronoun");

        if(!him.IsEmpty())
            pronounNode ->SetAttribute("him", him.GetData());
        if(!her.IsEmpty())
            pronounNode ->SetAttribute("her", her.GetData());
        if(!it.IsEmpty())
            pronounNode ->SetAttribute("it", it.GetData());
        if(!them.IsEmpty())
            pronounNode ->SetAttribute("them", them.GetData());
    }


    for(int i=0; i<5; i++)
    {
        if(!csString(result[0][i]).IsEmpty())
        {
            csRef<iDocumentNode> responseNode = attitudeNode->CreateNodeBefore(CS_NODE_ELEMENT);

            responseNode->SetValue("response");
            responseNode->SetAttribute("say", result[0][i]);
        }
    }

    csString script = result[0][9];
    if(!script.IsEmpty())
    {
        csRef<iDocumentNode> scriptNode = attitudeNode->CreateNodeBefore(CS_NODE_ELEMENT);

        scriptNode->SetValue("script");
        csRef<iDocumentNode> textNode = scriptNode->CreateNodeBefore(CS_NODE_TEXT);
        textNode->SetValue(script.GetData());
    }
    return true;
}


bool psNPCLoader::WriteTrigger(csRef<iDocumentNode> specificsNode, csString &trigger,int priorID, int questID)
{
    // <trigger>
    //      <phrase value="..">
    //      <phrase value="..">
    //      <attitude min="0" max="100">
    //          <response say=".. "/>
    //          <trigger>
    //              ..
    //          </trigger>
    //      </attitude>
    // </trigger>

    Result result;
    Result phrases;

    csString escArea;
    csString escTrigger;
    db->Escape(escArea, area);
    db->Escape(escTrigger, trigger);

    if(questID == -1)
    {
        result = db->Select("SELECT response_id, "
                            "       min_attitude_required, "
                            "       max_attitude_required "
                            "FROM npc_triggers "
                            "WHERE prior_response_required=\"%i\" and trigger=\"%s\" and area=\"%s\" and quest_id<1;",
                            priorID,
                            escTrigger.GetData(),
                            escArea.GetData());

        // all triggers that have the same response id will be grouped together
        // and written to the xml file as <phrase value="..">
        phrases = db->Select("SELECT id, trigger "
                             "FROM npc_triggers "
                             "WHERE response_id=\"%i\" and area=\"%s\" and quest_id<1;",
                             result[0].GetInt(0),
                             escArea.GetData());
    }
    else
    {
        result = db->Select("SELECT response_id, "
                            "       min_attitude_required, "
                            "       max_attitude_required "
                            "FROM npc_triggers "
                            "WHERE prior_response_required=\"%i\" and trigger=\"%s\" and quest_id=\"%i\";",
                            priorID,
                            escTrigger.GetData(),
                            questID);


        // all triggers that have the same response id will be grouped together
        // and written to the xml file as <phrase value="..">
        phrases = db->Select("SELECT id, trigger, area "
                             "FROM npc_triggers "
                             "WHERE response_id=\"%i\" and quest_id=\"%i\";",
                             result[0].GetInt(0),
                             questID);
    }

    csString temp;
    csRef<iDocumentNode> triggerNode;

    for(int i=0; i< (int)phrases.Count(); i++)
    {
        int triggerID = phrases[i].GetInt(0);
        bool duplicate=false;

        for(size_t j=0; j<triggers.GetSize(); j++)
        {
            if(triggerID==triggers[j])
                duplicate=true;
        }

        // if the trigger hasn't already been added
        if(!duplicate)
        {
            triggers.Push(triggerID);
            if(!triggerNode)
            {
                triggerNode = specificsNode->CreateNodeBefore(CS_NODE_ELEMENT);
                triggerNode->SetValue("trigger");
                csString area(phrases[0][2]);
                if(this->area.IsEmpty() && !area.IsEmpty())
                    triggerNode->SetAttribute("area", area);
            }
            csRef<iDocumentNode> phraseNode = triggerNode->CreateNodeBefore(CS_NODE_ELEMENT);

            phraseNode->SetValue("phrase");
            phraseNode->SetAttribute("value", phrases[i][1]);
        }
    }

    if(triggerNode)
    {
        for(int i=0; i<(int)result.Count(); i++)
        {
            int responseID  = result[i].GetInt(0);
            int minAttitude = result[i].GetInt(1);
            int maxAttitude = result[i].GetInt(2);

            csRef<iDocumentNode> attitudeNode = triggerNode->CreateNodeBefore(CS_NODE_ELEMENT);

            attitudeNode->SetValue("attitude");
            attitudeNode->SetAttributeAsInt("min", minAttitude);
            attitudeNode->SetAttributeAsInt("max", maxAttitude);

            if(!WriteResponse(attitudeNode, responseID, questID))
                return false;

            // check if this trigger contains other triggers
            Result childResult(db->Select("SELECT DISTINCT trigger "
                                          "FROM npc_triggers "
                                          "WHERE prior_response_required=%i;" ,responseID));

            if(childResult.IsValid())
            {
                for(int j=0; j<(int)childResult.Count(); j++)
                {
                    csString res(childResult[j][0]);
                    if(!WriteTrigger(attitudeNode, res, responseID, questID))
                        return false;
                }
            }
        }
    }
    return true;
}


void psNPCLoader::WriteFactions()
{
    // <factions>
    //      <faction name=".." value="..">
    //      <faction name=".." value="..">
    //      ....
    // </factions>

    csString factions;
    npc->GetFactions()->GetFactionListCSV(factions);

    // consider (null) and NULL as empty entries
    if(factions.StartsWith("(null)") || factions.StartsWith("NULL"))
        return;

    // factions are stored in the DB like this: '3,40,2,50,..'
    // so this needs to be converted
    int pos = 0;
    int start = 0;
    csString sub;
    csArray<int>    intList;

    while(pos< (int)factions.Length())
    {
        if(factions.GetAt(pos)==',')
        {
            factions.SubString(sub,start,pos-start);
            intList.Push(atoi(sub));
            start = ++pos;
        }
        pos++;
    }

    csRef<iDocumentNode> factionsNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    factionsNode->SetValue("factions");


    if(factions.Length()>0)
    {

        factions.SubString(sub, start, factions.Length()-start);
        intList.Push(atoi(sub));


        for(size_t i=0; i<intList.GetSize(); i+=2)
        {
            csString temp;
            int factionID = intList[i  ];
            int value     = intList[i+1];

            Result faction(db->Select("SELECT faction_name "
                                      "FROM factions "
                                      "WHERE id=%i;",
                                      factionID));
            csRef<iDocumentNode> factionNode = factionsNode->CreateNodeBefore(CS_NODE_ELEMENT);

            factionNode->SetValue("faction");
            factionNode->SetAttribute("name", faction[0][0]);
            factionNode->SetAttributeAsInt("value", value);
        }
    }
}


void psNPCLoader::WriteMerchantInfo()
{
    psMerchantInfo* merchantInfo = npc->GetMerchantInfo();

    csRef<iDocumentNode> buysNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    buysNode->SetValue("buys");
    csRef<iDocumentNode> sellsNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    sellsNode->SetValue("sells");
    if(merchantInfo)
    {
        // buys

        for(size_t i=0; i<merchantInfo->categories.GetSize(); i++)
        {
            psItemCategory* itemCategory = merchantInfo->categories[i];

            csRef<iDocumentNode> buyNode = buysNode->CreateNodeBefore(CS_NODE_ELEMENT);
            buyNode->SetValue("buy");
            buyNode->SetAttribute("name", itemCategory->name.GetData());
        }

        // sells
        for(size_t i=0; i<merchantInfo->categories.GetSize(); i++)
        {
            psItemCategory* itemCategory = merchantInfo->categories[i];

            csRef<iDocumentNode> sellNode = sellsNode->CreateNodeBefore(CS_NODE_ELEMENT);
            sellNode->SetValue("sell");
            sellNode->SetAttribute("name", itemCategory->name.GetData());
        }
    }
}


void psNPCLoader::WriteTrainerInfo()
{
    Result trains(db->Select("SELECT s.name, t.min_rank, t.max_rank, t.min_faction "
                             "FROM trainer_skills t, skills s "
                             "WHERE player_id=%i and t.skill_id=s.skill_id;",
                             npcID));
    csRef<iDocumentNode> trainsNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    trainsNode->SetValue("trains");

    if(trains.Count()>0)
    {
        for(int i=0; i< (int)trains.Count(); i++)
        {
            csRef<iDocumentNode> trainNode = trainsNode->CreateNodeBefore(CS_NODE_ELEMENT);

            trainNode->SetValue("train");
            trainNode->SetAttribute("name", trains[i][0]);
            trainNode->SetAttribute("min_rank", trains[i][1]);
            trainNode->SetAttribute("max_rank", trains[i][2]);
            trainNode->SetAttribute("min_faction", trains[i][3]);
        }
    }
}


void psNPCLoader::WriteSkills()
{
    csRef<iDocumentNode> skillsNode = npcRoot->CreateNodeBefore(CS_NODE_ELEMENT);

    skillsNode->SetValue("skills");
    for(size_t i=0; i<psserver->GetCacheManager()->GetSkillAmount(); i++)
    {
        int rank = npc->Skills().GetSkillRank((PSSKILL)i).Current();
        if(rank)
        {
            csRef<iDocumentNode> skillNode = skillsNode->CreateNodeBefore(CS_NODE_ELEMENT);

            skillNode->SetValue("skill");

            psSkillInfo* skillInfo = psserver->GetCacheManager()->GetSkillByID(i);
            skillNode->SetAttribute("name", skillInfo->name);
            skillNode->SetAttributeAsInt("value", rank);
        }
    }
}


void psNPCLoader::WriteEquipment()
{
    npc->Inventory().WriteAllInventory(npcRoot);
}


//-----------------------------------------------------------------------------

bool psNPCLoader::RemoveFromDatabase(int id)
{
    // first check a character with this id exists
    Result result(db->Select("SELECT name "
                             "FROM characters "
                             "WHERE id=\"%i\" ",
                             id));

    if(result.Count()==0)
    {
        CPrintf(CON_ERROR, "Error: No character with id %i found\n",id);
        return false;
    }

    csString name = result[0][0];

    // todo: delete the data from the database

    return false;
}
