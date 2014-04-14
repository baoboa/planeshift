/*
* pscharacterloader.cpp
*
* Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/object.h>
#include <csutil/threading/thread.h>
#include <csutil/stringarray.h>
#include <iengine/sector.h>
#include <iengine/engine.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/gameevent.h"
#include "util/psdatabase.h"

#include "rpgrules/factions.h"

#include "../psserver.h"
#include "../playergroup.h"
#include "../entitymanager.h"
#include "../gem.h"
#include "../clients.h"
#include "../cachemanager.h"
#include "../progressionmanager.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharacterloader.h"
#include "psglyph.h"
#include "psraceinfo.h"
#include "psitem.h"
#include "pssectorinfo.h"
#include "pscharacter.h"
#include "pscharacterlist.h"
#include "gmeventmanager.h"
#include "psguildinfo.h"
#include "pstrait.h"
#include "client.h"
#include "psserverchar.h"
#include "marriagemanager.h"

psCharacterLoader::psCharacterLoader()
{
}


psCharacterLoader::~psCharacterLoader()
{
}

bool psCharacterLoader::Initialize()
{

    // Per-instance initialization should go here

    return true;
}

bool psCharacterLoader::AccountOwner(const char* characterName, AccountID accountID)
{
    csString escape;
    db->Escape(escape,characterName);
    int count = db->SelectSingleNumber("SELECT count(*) FROM characters where name='%s' AND account_id=%d",escape.GetData(), accountID.Unbox());
    return (count > 0);
}

psCharacterList* psCharacterLoader::LoadCharacterList(AccountID accountid)
{
    // Check the generic cache first
    iCachedObject* obj = psserver->GetCacheManager()->RemoveFromCache(psserver->GetCacheManager()->MakeCacheName("list",accountid.Unbox()));
    if(obj)
    {
        Notify2(LOG_CACHE,"Returning char object %p from cache.",obj->RecoverObject());
        return (psCharacterList*)obj->RecoverObject();
    }

    Notify1(LOG_CACHE,"******LOADING CHARACTER LIST*******");
    // Load if not in cache
    Result result(db->Select("SELECT id,name,lastname FROM characters WHERE account_id=%u ORDER BY id", accountid.Unbox()));

    if(!result.IsValid())
        return NULL;

    psCharacterList* charlist = new psCharacterList;

    charlist->SetValidCount(result.Count());
    for(unsigned int i=0; i<result.Count(); i++)
    {
        charlist->SetEntryValid(i,true);
        charlist->SetCharacterID(i,result[i].GetInt("id"));
        charlist->SetCharacterFullName(i,result[i]["name"],result[i]["lastname"]);
    }
    return charlist;
}



/*  This uses a relatively slow method of looking up the list of ids of all npcs (optionally in a given sector)
*  and then loading each character data element with a seperate query.
*/

psCharacter** psCharacterLoader::LoadAllNPCCharacterData(psSectorInfo* sector,int &count)
{
    psCharacter** charlist;
    unsigned int i;
    count=0;

    Result npcs((sector)?
                db->Select("SELECT * from characters where characters.loc_sector_id='%u' and npc_spawn_rule>0",sector->uid)
                : db->Select("SELECT * from characters where npc_spawn_rule>0"));


    if(!npcs.IsValid())
    {
        Error3("Failed to load NPCs in Sector %s.   Error: %s",
               sector==NULL?"ENTIRE WORLD":sector->name.GetData(),db->GetLastError());
        return NULL;
    }

    if(npcs.Count()==0)
    {
        Error2("No NPCs found to load in Sector %s.",
               sector==NULL?"ENTIRE WORLD":sector->name.GetData());
        return NULL;
    }

    charlist=new psCharacter *[npcs.Count()];

    for(i=0; i<npcs.Count(); i++)
    {
        charlist[count]=new psCharacter();
        if(!charlist[count]->Load(npcs[i]))
        {
            delete charlist[count];
            charlist[count]=NULL;
            continue;
        }
        count++;
    }

    return charlist;
}

psCharacter* psCharacterLoader::LoadCharacterData(PID pid, bool forceReload)
{
    //if (!forceReload)
    //{
    // Check the generic cache first
    iCachedObject* obj = psserver->GetCacheManager()->RemoveFromCache(psserver->GetCacheManager()->MakeCacheName("char", pid.Unbox()));
    if(obj)
    {
        if(!forceReload)
        {
            Debug2(LOG_CACHE, pid.Unbox(), "Returning char object %p from cache.\n", obj->RecoverObject());

            psCharacter* charData = (psCharacter*)obj->RecoverObject();

            // Clear loot items
            if(charData)
                charData->ClearLoot();

            return charData;
        }
        else //this is needed because if the item is cached it needs to be destroyed (and so saved) before we load it again
        {
            delete(psCharacter*)obj->RecoverObject();
        }
    }
    //}


    // Now load from the database if not found in cache
    csTicks start = csGetTicks();

    Result result(db->Select("SELECT * FROM characters WHERE id=%u", pid.Unbox()));

    if(!result.IsValid())
    {
        Error2("Character data invalid for %s.", ShowID(pid));
        return NULL;
    }

    if(result.Count()>1)
    {
        Error2("Character %s has multiple entries.  Check table constraints.", ShowID(pid));
        return NULL;
    }
    if(result.Count()<1)
    {
        Error2("Character %s has no character entry!", ShowID(pid));
        return NULL;
    }

    psCharacter* chardata = new psCharacter();

    Debug2(LOG_CACHE, pid.Unbox(), "New character data ptr is %p.", chardata);
    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    // Read basic stats
    if(!chardata->Load(result[0]))
    {
        if(csGetTicks() - start > 500)
        {
            csString status;
            status.Format("Warning: Spent %u time loading FAILED character %s",
                          csGetTicks() - start, ShowID(pid));
            psserver->GetLogCSV()->Write(CSV_STATUS, status);
        }
        Error2("Load failed for character %s.", ShowID(pid));
        delete chardata;
        return NULL;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    chardata->LoadIntroductions();

    return chardata;
}


psCharacter* psCharacterLoader::QuickLoadCharacterData(PID pid, bool noInventory)
{
    Result result(db->Select("SELECT id, name, lastname, racegender_id FROM characters WHERE id=%u LIMIT 1", pid.Unbox()));

    if(!result.IsValid() || result.Count() < 1)
    {
        Error2("Character data invalid for %s.", ShowID(pid));
        return NULL;
    }

    psCharacter* chardata = new psCharacter();

    if(!chardata->QuickLoad(result[0], noInventory))
    {
        Error2("Quick load failed for character %s.", ShowID(pid));
        delete chardata;
        return NULL;
    }

    return chardata;
}


bool psCharacterLoader::NewNPCCharacterData(AccountID accountid, psCharacter* chardata)
{
    const char* fieldnames[]=
    {
        "account_id",
        "name",
        "lastname",
        "racegender_id",
        "character_type",
        "description",
        "description_ooc",
        "creation_info",
        "description_life",
        "mod_hitpoints",
        "base_hitpoints_max",
        "mod_mana",
        "base_mana_max",
        "stamina_physical",
        "stamina_mental",
        "money_circles",
        "money_trias",
        "money_hexas",
        "money_octas",
        "bank_money_circles",
        "bank_money_trias",
        "bank_money_hexas",
        "bank_money_octas",
        "loc_x",
        "loc_y",
        "loc_z",
        "loc_yrot",
        "loc_sector_id",
        "loc_instance",
        "npc_spawn_rule",
        "npc_master_id",
        "npc_addl_loot_category_id",
        "npc_impervious_ind",
        "kill_exp",
        "animal_affinity",
    };
    psStringArray values;

    values.FormatPush("%d",accountid.Unbox() ? accountid.Unbox() : chardata->GetAccount().Unbox());
    values.FormatPush("%s",chardata->GetCharName());
    values.FormatPush("%s",chardata->GetCharLastName());
    values.FormatPush("%u",chardata->GetOverridableRace().Base()->uid);
    values.FormatPush("%u",chardata->GetCharType());
    values.FormatPush("%s",chardata->GetDescription());
    values.FormatPush("%s",chardata->GetOOCDescription());
    values.FormatPush("%s",chardata->GetCreationInfo());
    values.FormatPush("%s",chardata->GetLifeDescription());
    values.FormatPush("%10.2f",chardata->GetHP());
    values.FormatPush("%10.2f",chardata->GetMaxHP().Base());
    values.FormatPush("%10.2f",chardata->GetMana());
    values.FormatPush("%10.2f",chardata->GetMaxMana().Base());
    values.FormatPush("%10.2f",chardata->GetStamina(true));
    values.FormatPush("%10.2f",chardata->GetStamina(false));
    values.FormatPush("%u",chardata->Money().GetCircles());
    values.FormatPush("%u",chardata->Money().GetTrias());
    values.FormatPush("%u",chardata->Money().GetHexas());
    values.FormatPush("%u",chardata->Money().GetOctas());
    values.FormatPush("%u",chardata->BankMoney().GetCircles());
    values.FormatPush("%u",chardata->BankMoney().GetTrias());
    values.FormatPush("%u",chardata->BankMoney().GetHexas());
    values.FormatPush("%u",chardata->BankMoney().GetOctas());
    float x,y,z,yrot;
    psSectorInfo* sectorinfo;
    InstanceID instance;
    chardata->GetLocationInWorld(instance,sectorinfo,x,y,z,yrot);
    values.FormatPush("%10.2f",x);
    values.FormatPush("%10.2f",y);
    values.FormatPush("%10.2f",z);
    values.FormatPush("%10.2f",yrot);
    values.FormatPush("%u",sectorinfo->uid);
    values.FormatPush("%u",instance);

    values.FormatPush("%u",chardata->npcSpawnRuleId);
    values.FormatPush("%u",chardata->npcMasterId);
    values.FormatPush("%u",chardata->lootCategoryId);
    values.FormatPush("%c",chardata->GetImperviousToAttack() & ALWAYS_IMPERVIOUS ? 'Y' : 'N');
    values.FormatPush("%u",chardata->GetKillExperience());
    values.FormatPush("%s",chardata->GetAnimalAffinity());


    unsigned int id=db->GenericInsertWithID("characters",fieldnames,values);

    if(id==0)
    {
        Error3("Failed to create new character for %s: %s", ShowID(accountid), db->GetLastError());
        return false;
    }

    chardata->SetPID(id);

    return true;
}

bool psCharacterLoader::NewCharacterData(AccountID accountid, psCharacter* chardata)
{
    chardata->npcSpawnRuleId = 0;
    chardata->npcMasterId    = 0;

    if(!NewNPCCharacterData(accountid, chardata))
    {
        return false;
    }

    size_t i;

    // traits
    if(!ClearCharacterTraits(chardata->GetPID()))
    {
        Error3("Failed to clear traits for character %s: %s.",
               ShowID(chardata->GetPID()), db->GetLastError());
    }

    for(i=0; i<PSTRAIT_LOCATION_COUNT; i++)
    {
        psTrait* trait=chardata->GetTraitForLocation((PSTRAIT_LOCATION)i);
        if(trait!=NULL)
        {
            SaveCharacterTrait(chardata->GetPID(),trait->uid);
        }
    }


    // skills
    if(!ClearCharacterSkills(chardata->GetPID()))
    {
        Error3("Failed to clear skills for character %s: %s.",
               ShowID(chardata->GetPID()), db->GetLastError());
    }

    for(i=0; i<psserver->GetCacheManager()->GetSkillAmount(); i++)
    {
        unsigned int skillRank = chardata->Skills().GetSkillRank((PSSKILL) i).Base();
        unsigned int skillY = chardata->Skills().GetSkillKnowledge((PSSKILL) i);
        unsigned int skillZ = chardata->Skills().GetSkillPractice((PSSKILL) i);
        SaveCharacterSkill(chardata->GetPID(),i,skillZ,skillY,skillRank);
    }

    return true;
}

bool psCharacterLoader::UpdateQuestAssignments(psCharacter* chr)
{
    return chr->GetQuestMgr().UpdateQuestAssignments();
}

bool psCharacterLoader::ClearCharacterSpell(psCharacter* character)
{
    unsigned long result=db->CommandPump("DELETE FROM player_spells WHERE player_id='%u'", character->GetPID().Unbox());
    if(result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::SaveCharacterSpell(psCharacter* character)
{
    int index = 0;
    while(psSpell* spell = character->GetSpellByIdx(index))
    {
        unsigned long result=db->CommandPump("INSERT INTO player_spells (player_id,spell_id,spell_slot) VALUES('%u','%u','%u')",
                                             character->GetPID().Unbox(), spell->GetID(), index);
        if(result==QUERY_FAILED)
            return false;
        index++;
    }
    return true;
}



bool psCharacterLoader::ClearCharacterTraits(PID pid)
{
    unsigned long result=db->CommandPump("DELETE FROM character_traits WHERE character_id='%u'", pid.Unbox());
    if(result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::SaveCharacterTrait(PID pid, unsigned int trait_id)
{
    unsigned long result=db->CommandPump("INSERT INTO character_traits (character_id,trait_id) VALUES('%u','%u')", pid.Unbox(), trait_id);
    if(result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::ClearCharacterSkills(PID pid)
{
    unsigned long result=db->CommandPump("DELETE FROM character_skills WHERE character_id='%u'", pid.Unbox());
    if(result==QUERY_FAILED)
        return false;

    return true;
}


bool psCharacterLoader::UpdateCharacterSkill(PID pid, unsigned int skill_id, unsigned int skill_z, unsigned int skill_y, unsigned int skill_rank)
{
    csString sql;

    sql.Format("UPDATE character_skills SET skill_z=%u, skill_y=%u, skill_rank=%u WHERE character_id=%u AND skill_id=%u",
               skill_z, skill_y, skill_rank, pid.Unbox(), (unsigned int) skill_id);

    unsigned long result=db->Command(sql);

    // If there was nothing to update we can add the skill to the database
    if(result == 0)
    {
        return SaveCharacterSkill(pid, skill_id, skill_z, skill_y, skill_rank);
    }
    else if(result == 1)
    {
        return true;
    }
    else
    {
        // If it updated more than one row than that is a problem.
        return false;
    }
}


bool psCharacterLoader::SaveCharacterSkill(PID pid, unsigned int skill_id,
        unsigned int skill_z, unsigned int skill_y, unsigned int skill_rank)
{
    // If a player has no knowledge of the skill then no need to add to database yet.
    if(skill_z == 0 && skill_y == 0 && skill_rank == 0)
        return true;

    unsigned long result=db->CommandPump("INSERT INTO character_skills (character_id,skill_id,skill_y,skill_z,skill_rank) VALUES('%u','%u','%u','%u','%u')",
                                         pid.Unbox(), skill_id, skill_y, skill_z, skill_rank);
    if(result==QUERY_FAILED)
        return false;

    return true;
}

bool psCharacterLoader::DeleteCharacterData(PID pid, csString &error)
{
    csString query;
    query.Format("SELECT name, lastname, guild_member_of, guild_level FROM characters where id='%u'\n", pid.Unbox());
    Result result(db->Select(query));
    if(!result.IsValid() || !result.Count())
    {
        error = "Invalid DB entry!";
        return false;
    }

    iResultRow &row = result[0];

    const char* charName = row["name"];
    const char* charLastName = row["lastname"];
    csString charFullName;
    charFullName.Format("%s %s", charName, charLastName);

    unsigned int guild = row.GetUInt32("guild_member_of");
    int guildLevel = row.GetInt("guild_level");

    // If Guild leader? then can't delete
    if(guildLevel == 9)
    {
        error = "Character is a guild leader";
        return false;
    }

    // Online? Kick
    Client* zombieClient = psserver->GetConnections()->FindPlayer(pid);
    if(zombieClient)
        psserver->RemovePlayer(zombieClient->GetClientNum(),"This character is being deleted");

    // Remove the character from guild if he has joined any
    psGuildInfo* guildinfo = psserver->GetCacheManager()->FindGuild(guild);
    if(guildinfo)
        guildinfo->RemoveMember(guildinfo->FindMember(pid));

    // Now delete this character and all refrences to him from DB

    // Note: Need to delete the pets using this function as well.

    query.Format("SELECT character_id FROM character_relationships WHERE related_id=%u and relationship_type='spouse'",pid.Unbox());
    result = db->Select(query);
    if(!result.IsValid())
    {
        error = "Invalid DB entry!";
        return false;
    }
    if(result.Count())
    {
        Client* divorcedClient = psserver->GetConnections()->FindPlayer(result[0].GetUInt32("character_id"));
        if(divorcedClient != NULL)           //in other case it's not necessary to remove this data because it removes from DB
            psserver->marriageManager->DeleteMarriageInfo(divorcedClient->GetCharacterData());
    }

    query.Format("DELETE FROM character_relationships WHERE character_id=%u OR related_id=%u", pid.Unbox(), pid.Unbox());
    db->CommandPump(query);

    query.Format("DELETE FROM character_quests WHERE player_id=%u", pid.Unbox());
    db->CommandPump(query);

    query.Format("DELETE FROM character_skills WHERE character_id=%u", pid.Unbox());
    db->CommandPump(query);

    query.Format("DELETE FROM character_traits WHERE character_id=%u", pid.Unbox());
    db->CommandPump(query);

    query.Format("DELETE FROM player_spells WHERE player_id=%u", pid.Unbox());
    db->CommandPump(query);

    query.Format("DELETE FROM characters WHERE id=%u", pid.Unbox());
    db->CommandPump(query);

    /// Let GMEventManager sort the DB out, as it is a bit complex, and its cached too
    if(!psserver->GetGMEventManager()->RemovePlayerFromGMEvents(pid))
    {
        Error2("Failed to remove %s from GM events database/cache", ShowID(pid));
    }

    csArray<gemObject*> list;
    psserver->entitymanager->GetGEM()->GetPlayerObjects(pid, list);
    for(size_t x = 0; x < list.GetSize(); x++)
    {
        psserver->entitymanager->GetGEM()->RemoveEntity(list[x]);
    }

    query.Format("DELETE from item_instances WHERE char_id_owner=%u", pid.Unbox());
    db->CommandPump(query);

    query.Format("DELETE from introductions WHERE charid=%u OR introcharid=%u", pid.Unbox(), pid.Unbox());
    db->CommandPump(query);

    CPrintf(CON_DEBUG, "\nSuccessfully deleted character %s\n", ShowID(pid));

    return true;

}

// 08-02-2005 Borrillis:
// NPC's should not save their locations back into the characters table
// This causes mucho problems so don't "fix" it.
bool psCharacterLoader::SaveCharacterData(psCharacter* chardata, gemActor* actor, bool charRecordOnly)
{
    bool playerORpet = chardata->GetCharType() == PSCHARACTER_TYPE_PLAYER ||
                       chardata->GetCharType() == PSCHARACTER_TYPE_PET;

    size_t i;
    static iRecord* updatePlayer;
    static iRecord* updateNpc;
    if(playerORpet && updatePlayer == NULL)
    {
        updatePlayer = db->NewUpdatePreparedStatement("characters", "id", 35, __FILE__, __LINE__); // 35 fields + 1 id field
    }

    if(!playerORpet && updateNpc == NULL)
    {
        updateNpc = db->NewUpdatePreparedStatement("characters", "id", 28, __FILE__, __LINE__); // 28 fields + 1 id field
    }

    // Give 100% hp if the char is dead
    if(!actor->IsAlive())
    {
        chardata->SetHitPoints(chardata->GetMaxHP().Base());
    }

    iRecord* targetUpdate = (playerORpet) ? updatePlayer : updateNpc;

    targetUpdate->Reset();

    targetUpdate->AddField("name", chardata->GetCharName());
    targetUpdate->AddField("lastname", chardata->GetCharLastName());
    targetUpdate->AddField("old_lastname", chardata->GetOldLastName());
    targetUpdate->AddField("racegender_id", chardata->GetOverridableRace().Base()->uid);
    targetUpdate->AddField("character_type", chardata->GetCharType());
    targetUpdate->AddField("mod_hitpoints", (playerORpet)? chardata->GetHP():chardata->GetMaxHP().Base());
    targetUpdate->AddField("mod_mana", (playerORpet)?chardata->GetMana():chardata->GetMaxMana().Base());
    targetUpdate->AddField("stamina_physical", chardata->GetStamina(true));
    targetUpdate->AddField("stamina_mental", chardata->GetStamina(false));
    targetUpdate->AddField("money_circles", chardata->Money().GetCircles());
    targetUpdate->AddField("money_trias", chardata->Money().GetTrias());
    targetUpdate->AddField("money_hexas", chardata->Money().GetHexas());
    targetUpdate->AddField("money_octas", chardata->Money().GetOctas());
    targetUpdate->AddField("bank_money_circles", chardata->BankMoney().GetCircles());
    targetUpdate->AddField("bank_money_trias", chardata->BankMoney().GetTrias());
    targetUpdate->AddField("bank_money_hexas", chardata->BankMoney().GetHexas());
    targetUpdate->AddField("bank_money_octas", chardata->BankMoney().GetOctas());

    if(playerORpet)    // Only Pets and Players save location info
    {
        float yrot;
        csVector3 pos(0,0,0);
        psSectorInfo* sectorinfo;
        csString sector;
        InstanceID instance;
        if(!actor->IsAlive())  //resurrect the player where it should be before saving him.
        {
            actor->Resurrect();
        }

        iSector* sec;
        // We want to save the last reported location
        actor->GetLastLocation(pos, yrot, sec, instance);
        sector = sec->QueryObject()->GetName();

        sectorinfo = psserver->GetCacheManager()->GetSectorInfoByName(sector);

        if(!sectorinfo)
        {
            Error3("ERROR: Sector %s could not be found in the database! Character %s could not be saved!",sector.GetData(),chardata->GetCharName());
            return false;
        }

        targetUpdate->AddField("loc_x", pos.x);
        targetUpdate->AddField("loc_y", pos.y);
        targetUpdate->AddField("loc_z", pos.z);
        targetUpdate->AddField("loc_yrot", yrot);
        targetUpdate->AddField("loc_sector_id", sectorinfo->uid);
        targetUpdate->AddField("loc_instance", instance);
        //Saves the guild/alliance notification setting: this is done only when the client correctly quits.
        //This is to avoid flodding with setting changes as much as possible
        targetUpdate->AddField("join_notifications", chardata->GetNotifications());
    }

    if(!chardata->GetLastLoginTime().GetData())
    {
        chardata->SetLastLoginTime();
    }
    targetUpdate->AddField("last_login", chardata->GetLastLoginTime().GetData());

    // Create XML for a new progression script that'll restore ActiveSpells.
    csString script;
    csArray<ActiveSpell*> asps = actor->GetActiveSpells();
    if(!asps.IsEmpty())
    {
        script.Append("<script>");
        for(size_t i = 0; i < asps.GetSize(); i++)
            script.Append(asps[i]->Persist());
        script.Append("</script>");
    }
    targetUpdate->AddField("progression_script", script);

    targetUpdate->AddField("time_connected_sec", chardata->GetTotalOnlineTime());
    targetUpdate->AddField("experience_points", chardata->GetExperiencePoints()); // Save W
    // X is saved when changed
    targetUpdate->AddField("animal_affinity", chardata->animalAffinity.GetDataSafe());
    //fields.FormatPush("%u", chardata->owner_id );
    targetUpdate->AddField("help_event_flags", chardata->helpEventFlags);
    targetUpdate->AddField("description",chardata->GetDescription());
    targetUpdate->AddField("description_ooc",chardata->GetOOCDescription());
    targetUpdate->AddField("creation_info",chardata->GetCreationInfo());
    targetUpdate->AddField("description_life",chardata->GetLifeDescription());

    // Done building the fields struct, now
    // SAVE it to the DB.
    if(!targetUpdate->Execute(chardata->GetPID().Unbox()))
    {
        Error3("Failed to save character %s: %s", ShowID(chardata->GetPID()), db->GetLastError());
    }

    if(charRecordOnly)
        return true;   // some updates don't need to save off every table.

    // traits
    if(!ClearCharacterTraits(chardata->GetPID()))
    {
        Error3("Failed to clear traits for character %s: %s.", ShowID(chardata->GetPID()), db->GetLastError());
    }
    for(i=0; i<PSTRAIT_LOCATION_COUNT; i++)
    {
        psTrait* trait=chardata->GetTraitForLocation((PSTRAIT_LOCATION)i);
        if(trait!=NULL)
        {
            SaveCharacterTrait(chardata->GetPID(),trait->uid);
        }
    }

    // For all the skills we have update them. If the update fails it will automatically save a new
    // one to the database.
    for(i=0; i<psserver->GetCacheManager()->GetSkillAmount(); i++)
    {
        if(chardata->Skills().Get((PSSKILL) i).dirtyFlag)
        {
            unsigned int skillY = chardata->Skills().GetSkillKnowledge((PSSKILL) i);
            unsigned int skillZ = chardata->Skills().GetSkillPractice((PSSKILL) i);
            unsigned int skillRank = chardata->Skills().GetSkillRank((PSSKILL) i).Base();
            UpdateCharacterSkill(chardata->GetPID(),i,skillZ,skillY,skillRank);
        }
    }

    if(!ClearCharacterSpell(chardata))
        Error3("Failed to clear spells for character %s: %s.", ShowID(chardata->GetPID()), db->GetLastError());
    SaveCharacterSpell(chardata);

    UpdateQuestAssignments(chardata);

    //saves the faction to db
    chardata->UpdateFactions();

    //saves the character varaibles to db
    chardata->UpdateVariables();

    return true;
}

unsigned int psCharacterLoader::InsertNewCharacterData(const char** fieldnames, psStringArray &fieldvalues)
{
    return db->GenericInsertWithID("characters",fieldnames,fieldvalues);
}

PID psCharacterLoader::FindCharacterID(const char* character_name, bool excludeNPCs)
{
    csString escape;

    // Don't crash
    if(character_name==NULL)
        return 0;
    // Insufficient Escape Buffer space, and this is too long anyway
    if(strlen(character_name)>32)
        return 0;
    db->Escape(escape,character_name);

    int result;

    if(!excludeNPCs)
    {
        result = db->SelectSingleNumber("SELECT id from characters where name='%s'",escape.GetData());
    }
    else
    {
        result = db->SelectSingleNumber("SELECT id from characters where name='%s' AND npc_master_id=0",escape.GetData());
    }

    return PID(result <= 0 ? 0 : result);
}

PID psCharacterLoader::FindCharacterID(AccountID accountID, const char* character_name)
{
    csString escape;

    // Don't crash
    if(character_name==NULL)
        return 0;
    // Insufficient Escape Buffer space, and this is too long anyway
    if(strlen(character_name)>32)
        return 0;
    db->Escape(escape,character_name);

    int result = db->SelectSingleNumber("SELECT id FROM characters WHERE name='%s' AND account_id=%u", escape.GetData(), accountID.Unbox());

    return PID(result <= 0 ? 0 : result);
}


//-----------------------------------------------------------------------------



psSaveCharEvent::psSaveCharEvent(gemActor* object)
    : psGameEvent(0,psSaveCharEvent::SAVE_PERIOD,"psCharSaveEvent")
{
    this->actor = NULL;

    object->RegisterCallback(this);
    this->actor = object;
}

psSaveCharEvent::~psSaveCharEvent()
{
    if(this->actor)
    {
        this->actor->UnregisterCallback(this);
    }
}

void psSaveCharEvent::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    if(this->actor)
        this->actor->UnregisterCallback(this);

    this->actor = NULL;
    SetValid(false);
}


void psSaveCharEvent::Trigger()
{
    psServer::CharacterLoader.SaveCharacterData(actor->GetCharacterData(), actor);
    psSaveCharEvent* saver = new psSaveCharEvent(actor);
    saver->QueueEvent();
}
