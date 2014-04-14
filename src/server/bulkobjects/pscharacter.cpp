/*
* pscharacter.cpp
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
#include <iutil/virtclk.h>
#include <csutil/databuf.h>
#include <ctype.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "../globals.h"
#include "psserver.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "clients.h"
#include "psserverchar.h"
#include "exchangemanager.h"
#include "spellmanager.h"
#include "workmanager.h"
#include "marriagemanager.h"
#include "npcmanager.h"
#include "playergroup.h"
#include "events.h"
#include "progressionmanager.h"
#include "chatmanager.h"
#include "commandmanager.h"
#include "gmeventmanager.h"
#include "progressionmanager.h"
#include "client.h"


#include "util/psdatabase.h"
#include "util/log.h"
#include "util/psxmlparser.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/log.h"

#include "rpgrules/factions.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharacterloader.h"
#include "psglyph.h"
#include "psquest.h"
#include "dictionary.h"
#include "psraceinfo.h"
#include "psmerchantinfo.h"
#include "pstrainerinfo.h"
#include "servervitals.h"
#include "pssectorinfo.h"
#include "pstrait.h"
#include "adminmanager.h"
#include "scripting.h"
#include "pscharquestmgr.h"

// The sizes and scripts need balancing.  For now, maxSize is disabled.
#define ENABLE_MAX_CAPACITY 0


const char* psCharacter::characterTypeName[] = { "player", "npc", "pet", "mount", "mountpet" };

//-----------------------------------------------------------------------------


// Definition of the itempool for psCharacter(s)
PoolAllocator<psCharacter> psCharacter::characterpool;


void* psCharacter::operator new(size_t allocSize)
{
// Debug3(LOG_CHARACTER,"%i %i", allocSize,sizeof(psCharacter));
//    CS_ASSERT(allocSize<=sizeof(psCharacter));
    return (void*)characterpool.CallFromNew();
}

void psCharacter::operator delete(void* releasePtr)
{
    characterpool.CallFromDelete((psCharacter*)releasePtr);
}

psCharacter::psCharacter() : inventory(this),
    guildinfo(NULL), modifiers(this), skills(this),
    acquaintances(101, 10, 101), npcMasterId(0), deaths(0), kills(0), suicides(0), loaded(false),
    race(NULL)
{
    characterType = PSCHARACTER_TYPE_UNKNOWN;

    helmGroup.Clear();
    BracerGroup.Clear();
    BeltGroup.Clear();
    CloakGroup.Clear();
    helpEventFlags = 0;
    accountid = 0;
    pid = 0;
    ownerId = 0;

    petElapsedTime = 0.0;
    lastSavedPetElapsedTime = 0.0;

    joinNotifications = 0;
    overrideMaxHp = 0.0f;
    overrideMaxMana = 0.0f;

    name = lastname = fullName = " ";
    SetSpouseName("");
    isMarried = false;

    race.SetCharacter(this);
    vitals = new psServerVitals(this);
//    workInfo = new WorkInformation();

    lootCategoryId = 0;
    lootMoney = 0;

    location.loc_sector = NULL;
    location.loc.Set(0.0f);
    location.loc_yrot = 0.0f;
    spawnLoc = location;

    for(int i=0; i<PSTRAIT_LOCATION_COUNT; i++)
        traits[i] = NULL;

    npcSpawnRuleId = -1;

    tradingStopped  = false;
    tradingStatus   = psCharacter::NOT_TRADING;

    merchant        = NULL;
    merchantInfo    = NULL;

    trainerInfo = NULL;
    trainer     = NULL;
    actor = NULL;

    timeconnected = 0;
    startTimeThisSession = csGetTicks();

//    transformation = NULL;
    killExp = 0;
    imperviousToAttack = 0;

    factions = NULL;

    lastResponse = -1;

    songExecutionTime = 0;

    banker = false;
    hired = false;
    isStatue = false;
     
    attackQueue.AttachNew(new psAttackQueue(this));
}

psCharacter::~psCharacter()
{
    if(guildinfo)
        guildinfo->Disconnect(this);

    if(factions)
    {
        delete factions;
        factions = NULL;
    }

    // First force and update of the DB of all QuestAssignments before deleting
    // every assignment.
    questManager.UpdateQuestAssignments(true);

    delete vitals;
    vitals = NULL;

//    delete workInfo;
}

void psCharacter::SetActor(gemActor* newActor)
{
    actor = newActor;
    if(actor)
    {
        inventory.RunEquipScripts();
        inventory.CalculateLimits();
    }
}

bool psCharacter::Load(iResultRow &row)
{

    // TODO:  Link in account ID?
    csTicks start = csGetTicks();
    pid = row.GetInt("id");

    buddyManager.Initialize(pid);
    questManager.Initialize(this);

    accountid = AccountID(row.GetInt("account_id"));
    SetCharType(row.GetUInt32("character_type"));

    SetFullName(row["name"], row["lastname"]);
    SetOldLastName(row["old_lastname"]);

    unsigned int raceid = row.GetUInt32("racegender_id");
    psRaceInfo* raceinfo = psserver->GetCacheManager()->GetRaceInfoByID(raceid);
    if(!raceinfo)
    {
        Error3("Character ID %s has unknown race id %s. Character loading failed.",row["id"],row["racegender_id"]);
        return false;
    }
    SetRaceInfo(raceinfo);

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }


    SetDescription(row["description"]);

    SetOOCDescription(row["description_ooc"]); //loads the ooc description of the player

    SetCreationInfo(row["creation_info"]); //loads the info gathered in the creation in order to allow the player
    //to review them

    SetLifeDescription(row["description_life"]); //Loads the life events added by players

    // NPC fields here
    npcSpawnRuleId = row.GetUInt32("npc_spawn_rule");
    npcMasterId    = row.GetUInt32("npc_master_id");

    // This substitution allows us to make 100 orcs which are all copies of the stats, traits and equipment
    // from a single master instance.
    PID use_id = npcMasterId ? npcMasterId : pid;

    //check if the row is null in this case we will check our master
    if(row["base_hitpoints_max"] == NULL)
    {
        //it was null so we check the master character
        Result hpResult(db->Select("SELECT base_hitpoints_max FROM characters WHERE id=%u", use_id.Unbox()));

        //if we got a valid result we will set it. Note that NULL being in the master
        //too will yield to 0 being set. We take the first result even though more than one
        //is impossible anyway (except if there is a problem with the schema.
        if(hpResult.IsValid() && hpResult.Count() > 0)
        {
            GetMaxHP().SetBase(hpResult[0].GetFloat("base_hitpoints_max"));
        }
        else
        {
            //if the result is invalid we end with the same result of before 0 being set
            //in other words autocalculate
            GetMaxHP().SetBase(0.0f);
        }
    }
    else //in the other cases just go on and take the value from the current character
    {
        GetMaxHP().SetBase(row.GetFloat("base_hitpoints_max"));
    }

    //check if the row is null in this case we will check our master
    if(row["base_mana_max"] == NULL)
    {
        //it was null so we check the master character
        Result manaResult(db->Select("SELECT base_mana_max FROM characters WHERE id=%u", use_id.Unbox()));

        //if we got a valid result we will set it. Note that NULL being in the master
        //too will yield to 0 being set. We take the first result even though more than one
        //is impossible anyway (except if there is a problem with the schema.
        if(manaResult.IsValid() && manaResult.Count() > 0)
        {
            GetMaxMana().SetBase(manaResult[0].GetFloat("base_mana_max"));
        }
        else
        {
            //if the result is invalid we end with the same result of before 0 being set
            //in other words autocalculate
            GetMaxMana().SetBase(0.0f);
        }
    }
    else //in the other cases just go on and take the value from the current character
    {
        GetMaxMana().SetBase(row.GetFloat("base_mana_max"));
    }

    overrideMaxHp = GetMaxHP().Base();
    overrideMaxMana = GetMaxMana().Base();

    if(!LoadSkills(use_id))
    {
        Error2("Cannot load skills for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    RecalculateStats();

    // If mod_hp, mod_mana, stamina physical and mental are set < 0 in the db, then that means
    // to use whatever is calculated as the max, so npc's spawn at 100%.
    float mod = row.GetFloat("mod_hitpoints");
    SetHitPoints(mod < 0 ? GetMaxHP().Base() : mod);
    mod = row.GetFloat("mod_mana");
    SetMana(mod < 0 ? GetMaxMana().Base() : mod);
    mod = row.GetFloat("stamina_physical");
    SetStamina(mod < 0 ? GetMaxPStamina().Base() : mod,true);
    mod = row.GetFloat("stamina_mental");
    SetStamina(mod < 0 ? GetMaxMStamina().Base() : mod,false);

    if(GetHP() <= 0)
        SetHitPoints(GetMaxHP().Base());
    if(GetMana() <= 0)
        SetMana(GetMaxMana().Base());

    vitals->SetOrigVitals(); // This saves them as loaded state for restoring later without hitting db, npc death resurrect.

    lastLoginTime = row["last_login"];
    petElapsedTime = row.GetFloat("pet_elapsed_time");
    lastSavedPetElapsedTime = petElapsedTime;
    progressionScriptText = row["progression_script"];

    // Set on-hand money.
    money.Set(
        row.GetInt("money_circles"),
        row.GetInt("money_octas"),
        row.GetInt("money_hexas"),
        row.GetInt("money_trias"));

    // Set bank money.
    bankMoney.Set(
        row.GetInt("bank_money_circles"),
        row.GetInt("bank_money_octas"),
        row.GetInt("bank_money_hexas"),
        row.GetInt("bank_money_trias"));

    psSectorInfo* sectorinfo=psserver->GetCacheManager()->GetSectorInfoByID(row.GetUInt32("loc_sector_id"));
    if(sectorinfo==NULL)
    {
        Error3("Character %s has unresolvable sector id %lu.", ShowID(pid), row.GetUInt32("loc_sector_id"));
        return false;
    }

    SetLocationInWorld(row.GetUInt32("loc_instance"),
                       sectorinfo,
                       row.GetFloat("loc_x"),
                       row.GetFloat("loc_y"),
                       row.GetFloat("loc_z"),
                       row.GetFloat("loc_yrot"));
    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    spawnLoc = location;

    // Guild fields here
    guildinfo = psserver->GetCacheManager()->FindGuild(row.GetUInt32("guild_member_of"));
    if(guildinfo)
        guildinfo->Connect(this);

    SetNotifications(row.GetInt("join_notifications"));

    // Loot rule here
    lootCategoryId = row.GetInt("npc_addl_loot_category_id");

    imperviousToAttack = (row["npc_impervious_ind"][0]=='Y') ? ALWAYS_IMPERVIOUS : 0;

    // Familiar Fields here
    animalAffinity  = row[ "animal_affinity" ];
    //ownerId         = row.GetUInt32( "ownerId" );
    helpEventFlags = row.GetUInt32("help_event_flags");

    timeconnected        = row.GetUInt32("time_connected_sec");
    startTimeThisSession = csGetTicks();

    if(!LoadTraits(use_id))
    {
        Error2("Cannot load traits for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    // This data is loaded only if it's a player, not an NPC
    if(!IsNPC() && !IsPet())
    {
        if(!questManager.LoadQuestAssignments())
        {
            Error2("Cannot load quest assignments for Character %s. Character loading failed.", ShowID(pid));
            return false;
        }

        if(!LoadGMEvents())
        {
            Error2("Cannot load GM Events for Character %s. Character loading failed.", ShowID(pid));
            return false;
        }

    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if(use_id != pid)
    {
        // This has a master npc template, so load character specific items
        // from the master npc.
        if(!inventory.Load(use_id))
        {
            Error2("Cannot load character specific items for Character %s. Character loading failed.", ShowID(pid));
            return false;
        }
    }
    else
    {
        inventory.Load();
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if(!LoadRelationshipInfo(pid))  // Buddies, Marriage Info, Familiars
    {
        return false;
    }

    factions = new FactionSet(NULL, psserver->GetCacheManager()->GetFactionHash());
    if(!LoadFactions(pid))
    {
        return false;
    }

    if(!LoadVariables(use_id))
    {
        return false;
    }


    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    // Load merchant info
    csRef<psMerchantInfo> merchant;
    merchant.AttachNew(new psMerchantInfo());
    if(merchant->Load(use_id))
    {
        merchantInfo = merchant;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    // Load trainer info
    csRef<psTrainerInfo> trainer;
    trainer.AttachNew(new psTrainerInfo());
    if(trainer->Load(use_id))
    {
        trainerInfo = trainer;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if(!LoadSpells(use_id))
    {
        Error2("Cannot load spells for Character %s. Character loading failed.", ShowID(pid));
        return false;
    }

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time loading character %s %s:%d",
                      csGetTicks() - start, ShowID(pid), __FILE__, __LINE__);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    // Load Experience Points W and Progression Points X
    SetExperiencePoints(row.GetUInt32("experience_points"));
    SetProgressionPoints(row.GetUInt32("progression_points"),false);

    // Load the kill exp
    killExp = row.GetUInt32("kill_exp");

    // Load if the character/npc is a banker
    if(row.GetInt("banker") == 1)
    {
        banker = true;
    }

    // Load if the character is a statue
    if(row.GetInt("statue") == 1)
        isStatue = true;

    loaded = true;
    return true;
}

bool psCharacter::QuickLoad(iResultRow &row, bool noInventory)
{
    pid = row.GetInt("id");
    SetFullName(row["name"], row["lastname"]);

    unsigned int raceid = row.GetUInt32("racegender_id");
    psRaceInfo* raceinfo = psserver->GetCacheManager()->GetRaceInfoByID(raceid);
    if(!raceinfo)
    {
        Error3("Character ID %s has unknown race id %s.",row["id"],row["racegender_id"]);
        return false;
    }

    if(!noInventory)
    {
        SetRaceInfo(raceinfo);

        if(!LoadTraits(pid))
        {
            Error2("Cannot load traits for Character %s.", ShowID(pid));
            return false;
        }

        // Load equipped items
        inventory.QuickLoad(pid);
    }

    return true;
}

bool psCharacter::LoadRelationshipInfo(PID pid)
{
    // To reduce number of queries load all releationships types and sorte them out in the later load functions.
    Result has_a(db->Select("SELECT a.*, b.name AS \"buddy_name\" FROM character_relationships a, characters b WHERE a.character_id = %u AND a.related_id = b.id order by a.character_id", pid.Unbox()));
    Result of_a(db->Select("SELECT a.*, b.name AS \"buddy_name\" FROM character_relationships a, characters b WHERE a.related_id = %u AND a.character_id = b.id order by a.related_id", pid.Unbox()));

    if(!LoadFamiliar(has_a, of_a))
    {
        Error2("Cannot load familiar info for Character %s.", ShowID(pid));
        return false;
    }

    if(!LoadMarriageInfo(has_a))
    {
        Error2("Cannot load Marriage Info for Character %s.", ShowID(pid));
        return false;
    }

    if(!buddyManager.LoadBuddies(has_a, of_a))
    {
        Error2("Cannot load buddies for Character %s.", ShowID(pid));
        return false;
    }

    if(!LoadExploration(has_a))
    {
        Error2("Cannot load exploration points for Character %s.", ShowID(pid));
        return false;
    }

    return true;
}

void psCharacter::UpdateFactions()
{
    //not initialized yet
    if(!GetFactions())
        return;

    //iterate all the standings and update them
    csHash<FactionStanding*, int>::GlobalIterator iter(GetFactions()->GetStandings().GetIterator());
    while(iter.HasNext())
    {
        FactionStanding* standing = iter.Next();
        if(standing->dirty)
        {
            db->CommandPump("INSERT INTO character_factions "
                            "(character_id, faction_id, value) "
                            "values (%d, %d, %d) "
                            "ON DUPLICATE KEY UPDATE value=%d",
                            pid.Unbox(),
                            standing->faction->id,
                            standing->score,
                            standing->score);
        }
    }
}

bool psCharacter::LoadFactions(PID pid)
{
    Result factions(db->Select("SELECT faction_id, value from character_factions where character_id = %u", pid.Unbox()));

    if(factions.IsValid())
    {
        for(unsigned long i = 0; i < factions.Count(); i++)
        {
            //sets the faction, overwrites what could be there and doesn't set the dirty flag
            GetFactions()->UpdateFactionStanding(factions[i].GetUInt32("faction_id"), factions[i].GetUInt32("value"), false, true);
        }
    }

    return true;
}

void psCharacter::UpdateVariables()
{
    //iterate all the variables and update them
    //we don't escape as we use " and the variables are set internally.
    //maybe that could be done anyway as an additional safety measure
    //update only if the variable was changed in the mean time (dirty flag)
    csHash<charVariable, csString>::GlobalIterator iter(charVariables.GetIterator());
    while(iter.HasNext())
    {
        charVariable &variable = iter.Next();
        if(variable.dirty)
        {
            db->CommandPump("INSERT INTO character_variables "
                            "(character_id, name, value) "
                            "values (%d, \"%s\", \"%s\") "
                            "ON DUPLICATE KEY UPDATE value=\"%s\"",
                            pid.Unbox(),
                            variable.name.GetData(),
                            variable.value.GetData(),
                            variable.value.GetData());
        }
    }
}

bool psCharacter::LoadVariables(PID pid)
{
    Result variables(db->Select("SELECT name, value from character_variables where character_id = %u", pid.Unbox()));

    if(variables.IsValid())
    {
        for(unsigned long i = 0; i < variables.Count(); i++)
        {
            charVariables.PutUnique(variables[i]["name"], charVariable(variables[i]["name"], variables[i]["value"]));
        }
    }

    return true;
}

void psCharacter::LoadIntroductions()
{
    Result r = db->Select("SELECT * FROM introductions WHERE charid=%d", pid.Unbox());
    if(r.IsValid())
    {
        for(unsigned long i = 0; i < r.Count(); i++)
        {
            unsigned int charID = r[i].GetUInt32("introcharid");
            // Safe to skip test because the DB disallows duplicate rows
            acquaintances.AddNoTest(charID);
        }
    }
}



bool psCharacter::LoadMarriageInfo(Result &result)
{
    if(!result.IsValid())
    {
        Error3("Could not load marriage info for %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    for(unsigned int x = 0; x < result.Count(); x++)
    {

        if(strcmp(result[x][ "relationship_type" ], "spouse") == 0)
        {
            const char* spouseName = result[x]["spousename"];
            if(spouseName == NULL)
                return true;

            SetSpouseName(spouseName);

            Notify2(LOG_RELATIONSHIPS, "Successfully loaded marriage info for %s", name.GetData());
            break;
        }
    }

    return true;
}

bool psCharacter::LoadFamiliar(Result &pet, Result &owner)
{
    ownerId = 0;

    if(!pet.IsValid())
    {
        Error3("Could not load pet info for %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    if(!owner.IsValid())
    {
        Error3("Could not load owner info for character %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    unsigned int x;
    for(x = 0; x < pet.Count(); x++)
    {

        if(strcmp(pet[x][ "relationship_type" ], "familiar") == 0)
        {
            familiarsId.InsertSorted(pet[x].GetInt("related_id"));
            Notify2(LOG_PETS, "Successfully loaded familiar for %s", name.GetData());
        }
    }

    for(x = 0; x < owner.Count(); x++)
    {

        if(strcmp(owner[x][ "relationship_type" ], "familiar") == 0)
        {
            ownerId = owner[x].GetInt("character_id");
            Notify2(LOG_PETS, "Successfully loaded owner for %s", name.GetData());
            break;
        }
    }

    return true;
}

bool psCharacter::LoadExploration(Result &exploration)
{
    if(!exploration.IsValid())
    {
        Error3("Could not load exploration info for character %s: %s", ShowID(pid), db->GetLastError());
        return false;
    }

    for(uint i=0; i<exploration.Count(); ++i)
    {
        if(strcmp(exploration[i]["relationship_type"], "exploration") == 0)
        {
            exploredAreas.Push(exploration[i].GetInt("related_id"));
        }
    }

    return true;
}

/// Load GM events for this player from GMEventManager
bool psCharacter::LoadGMEvents(void)
{
    assignedEvents.runningEventID =
        psserver->GetGMEventManager()->GetAllGMEventsForPlayer(pid,
                assignedEvents.completedEventIDs,
                assignedEvents.runningEventIDAsGM,
                assignedEvents.completedEventIDsAsGM);
    return true;  // cant see how this can fail, but keep convention for now.
}

void psCharacter::SetLastLoginTime()
{
    csString timeStr;

    time_t curr=time(0);
    tm  result;
#ifdef WIN32
    tm* gmtm = &result;
    errno_t err = gmtime_s(&result, &curr);
#else
    tm* gmtm = gmtime_r(&curr, &result);
#endif

    lastLoginTime.Format("%d-%02d-%02d %02d:%02d:%02d",
                         gmtm->tm_year+1900,
                         gmtm->tm_mon+1,
                         gmtm->tm_mday,
                         gmtm->tm_hour,
                         gmtm->tm_min,
                         gmtm->tm_sec);

    if(guildinfo)
    {
        guildinfo->UpdateLastLogin(this);
    }

    //Store in database
    if(!db->CommandPump("UPDATE characters SET last_login='%s' WHERE id='%d'", lastLoginTime.GetData(), pid.Unbox()))
    {
        Error2("Last login storage: DB Error: %s\n", db->GetLastError());
        return;
    }
}

csString psCharacter::GetLastLoginTime() const
{
    return lastLoginTime;
}

void psCharacter::SetPetElapsedTime(double elapsedTime)
{
    static double saveInterval = 10.0; // Default to 10sec.
    petElapsedTime = elapsedTime;

    // If new and last saved value differ for more than saveInterval store the
    // current value to db.
    if(fabs(petElapsedTime - lastSavedPetElapsedTime) > saveInterval)
    {
        lastSavedPetElapsedTime = petElapsedTime;
        //Store in database
        if(!db->CommandPump("UPDATE characters SET pet_elapsed_time='%.2f' WHERE id='%d'", petElapsedTime, pid.Unbox()))
        {
            Error2("Last login storage: DB Error: %s\n", db->GetLastError());
            return;
        }
    }
}

double psCharacter::GetPetElapsedTime() const
{
    return petElapsedTime;
}

bool psCharacter::LoadSpells(PID use_id)
{
    // Load spells in asc since we use push to create the spell list.
    Result spells(db->Select("SELECT * FROM player_spells WHERE player_id=%u ORDER BY spell_slot ASC", use_id.Unbox()));
    if(spells.IsValid())
    {
        int i,count=spells.Count();

        for(i=0; i<count; i++)
        {
            psSpell* spell = psserver->GetCacheManager()->GetSpellByID(spells[i].GetInt("spell_id"));
            if(spell != NULL)
                AddSpell(spell);
            else
            {
                Error2("Spell id=%i not found in cachemanager", spells[i].GetInt("spell_id"));
            }
        }
        return true;
    }
    else
        return false;
}

bool psCharacter::LoadSkills(PID use_id)
{
    // Load skills
    Result skillResult(db->Select("SELECT * FROM character_skills WHERE character_id=%u", use_id.Unbox()));

    for(size_t z = 0; z < psserver->GetCacheManager()->GetSkillAmount(); z++)
    {
        skills.SetSkillInfo((PSSKILL)z, psserver->GetCacheManager()->GetSkillByID((PSSKILL)z), false);
    }

    if(skillResult.IsValid())
    {
        unsigned int i;
        for(i=0; i<skillResult.Count(); i++)
        {
            if(skillResult[i]["skill_id"]!=NULL)
            {
                PSSKILL skill = (PSSKILL)skillResult[i].GetInt("skill_id");
                skills.SetSkillPractice(skill, skillResult[i].GetInt("skill_Z"));
                skills.SetSkillKnowledge(skill, skillResult[i].GetInt("skill_Y"));
                skills.SetSkillRank(skill, skillResult[i].GetInt("skill_Rank"), false);
            }
        }
        skills.Calculate();
    }
    else
        return false;

    return true;
}

bool psCharacter::LoadTraits(PID use_id)
{
    // Load traits
    Result traits(db->Select("SELECT * FROM character_traits WHERE character_id=%u", use_id.Unbox()));
    if(traits.IsValid())
    {
        unsigned int i;
        for(i=0; i<traits.Count(); i++)
        {
            psTrait* trait=psserver->GetCacheManager()->GetTraitByID(traits[i].GetInt("trait_id"));
            if(!trait)
            {
                Error3("%s has unknown trait id %s.", ShowID(pid), traits[i]["trait_id"]);
            }
            else
                SetTraitForLocation(trait->location,trait);
        }
        return true;
    }
    else
        return false;
}

void psCharacter::AddSpell(psSpell* spell)
{
    spellList.Push(spell);
}

void psCharacter::SetFullName(const char* newFirstName, const char* newLastName)
{
    if(!newFirstName)
    {
        Error1("Null passed as first name...");
        return;
    }

    // Error3( "SetFullName( %s, %s ) called...", newFirstName, newLastName );
    // Update fist, last & full name
    if(strlen(newFirstName))
    {
        name = newFirstName;
        fullName = name;
    }
    if(newLastName)
    {
        lastname = newLastName;

        if(strlen(newLastName))
        {
            fullName += " ";
            fullName += lastname;
        }
    }

    //Error2( "New fullname is now: %s", fullname.GetData() );
}

void psCharacter::SetRaceInfo(psRaceInfo* rinfo)
{
    race.SetBase(rinfo);
}

void OverridableRace::OnChange()
{
    //if the race info is invalid do nothing
    psRaceInfo* raceInfo = Current();
    if(!raceInfo)
        return;

    MathEnvironment env;
    env.Define("Actor", character);
    env.Define("STR", raceInfo->GetBaseAttribute(PSITEMSTATS_STAT_STRENGTH));
    env.Define("AGI", raceInfo->GetBaseAttribute(PSITEMSTATS_STAT_AGILITY));
    env.Define("END", raceInfo->GetBaseAttribute(PSITEMSTATS_STAT_ENDURANCE));
    env.Define("INT", raceInfo->GetBaseAttribute(PSITEMSTATS_STAT_INTELLIGENCE));
    env.Define("WILL", raceInfo->GetBaseAttribute(PSITEMSTATS_STAT_WILL));
    env.Define("CHA", raceInfo->GetBaseAttribute(PSITEMSTATS_STAT_CHARISMA));

    (void) psserver->GetCacheManager()->GetSetBaseSkillsScript()->Evaluate(&env);

    //as we are changing or obtaining for the first time a race set the inventory correctly for this.
    character->Inventory().SetBasicArmor(raceInfo);
    character->Inventory().SetBasicWeapon(raceInfo);

    //set the race groups accordly too.
    character->SetHelmGroup(raceInfo->GetHelmGroup());
    character->SetBracerGroup(raceInfo->GetBracerGroup());
    character->SetBeltGroup(raceInfo->GetBeltGroup());
    character->SetCloakGroup(raceInfo->GetCloakGroup());

    //this is needed to force an update the mesh in gem. but do it only if the actor is already initialized
    if(character->GetActor())
    {
        character->GetActor()->SetMesh(raceInfo->GetMeshName());

        //TODO: this is an hack which should be removed and handled more cleanly as an actor property
        //      when sent to the client, in place of a special message which alters only the player
        //      with the mod client. (letting alone the convoluted approach to reach the client)

        if(character->GetActor()->GetClientID())
        {
            //For now we check if we are near 1. In that case we disable the movement mod, else we send it directly.
            float movMod = Current()->GetSpeedModifier();
            if(movMod < 1+EPSILON && movMod > 1-EPSILON)
            {
                psMoveModMsg modMsg(character->GetActor()->GetClientID(), psMoveModMsg::NONE,
                                    csVector3(0), 0);
                modMsg.SendMessage();
}
            else
            {
                psMoveModMsg modMsg(character->GetActor()->GetClientID(), psMoveModMsg::MULTIPLIER,
                                    csVector3(movMod), movMod);
                modMsg.SendMessage();
            }
        }
    }
}

float psCharacter::GetScale()
{
    // baseScale is based on cal3d file, example kran: 0.024
    float raceScale = GetRaceInfo()->GetScale();

    // scaleValue is a user entered scale, so for 10% more it's 1.1
    float scaleVar = GetScaleValue();

    // use overridden scale if specified
    float scale =  scaleVar*raceScale;
    Debug5(LOG_CELPERSIST, 0, "DEBUG: Race scale %f , scaleVar: %f, persist New Scale %s %f\n",
           raceScale, scaleVar, GetCharName(), scale);

    return scale;
}

float psCharacter::GetScaleValue()
{
    // scaleVar is a user entered scale, so for 10% more it's 1.1
    float scaleVar = 1.0;

    if(GetVariableValue("scale"))
    {
        scaleVar = atof(GetVariableValue("scale").GetData());
    }

    // TODO: add any other "user" modifications to scale (Buffs).

    return scaleVar;
}


psRaceInfo* psCharacter::GetRaceInfo()
{
    return race.Current();
}

OverridableRace &psCharacter::GetOverridableRace()
{
    return race;
}

void psCharacter::SetFamiliarID(PID v)
{
    familiarsId.InsertSorted(v);

    csString sql;
    sql.Format("INSERT INTO character_relationships VALUES (%u, %d, 'familiar', '')", pid.Unbox(), v.Unbox());
    if(!db->Command(sql))
    {
        Error3("Couldn't execute SQL %s!, %s's pet relationship is not saved.", sql.GetData(), ShowID(pid));
    }
};

void psCharacter::LoadActiveSpells()
{
    if(progressionScriptText.IsEmpty())
        return;

    ProgressionScript* script = ProgressionScript::Create(psserver->entitymanager, psserver->GetCacheManager(), fullName, progressionScriptText);
    if(!script)
    {
        Error3("Saved progression script for >%s< is invalid:\n%s", fullName.GetData(), progressionScriptText.GetData());
        return;
    }

    CS_ASSERT(actor);

    MathEnvironment env;
    env.Define("Actor", actor);
    script->Run(&env);

    delete script;
}

void psCharacter::UpdateRespawn(csVector3 pos, float yrot, psSectorInfo* sector, InstanceID instance)
{
    spawnLoc.loc_sector = sector;
    spawnLoc.loc = pos;
    spawnLoc.loc_yrot   = yrot;
    spawnLoc.worldInstance = instance;

    // Save to database
    st_location &l = spawnLoc;
    psString sql;

    sql.AppendFmt("update characters set loc_x=%10.2f, loc_y=%10.2f, loc_z=%10.2f, loc_yrot=%10.2f, loc_sector_id=%u, loc_instance=%u where id=%u",
                  l.loc.x, l.loc.y, l.loc.z, l.loc_yrot, l.loc_sector->uid, l.worldInstance, pid.Unbox());
    if(db->CommandPump(sql) != 1)
    {
        Error3("Couldn't save character's position to database.\nCommand was "
               "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
    }
}

bool psCharacter::HasVariableDefined(const csString &name)
{
    return charVariables.Contains(name);
}

csString psCharacter::GetVariableValue(const csString &name)
{
    return charVariables.Get(name, charVariable()).value;
}

Buffable<int> &psCharacter::GetBuffableVariable(const csString &name, const csString &value)
{
    if(!HasVariableDefined(name))
    {
        //if we didn't find the variable we add it but we don't set it as dirty
        //so it doesn't get written to database
        //TODO: add an additional flag for temporary variables?
        charVariables.PutUnique(name, charVariable(name,value, false));
    }
    return charVariables.GetElementPointer(name)->GetBuffable();
}

void psCharacter::SetVariable(const csString &name, const csString &value)
{
    //we set the variable dirty so this should be used only for variables.
    //not already in the db (aka don't use this to load from the db)
    //right now this must not overwrite temporary variables or there could be issues.
    if(!HasVariableDefined(name))
    {
        charVariables.PutUnique(name, charVariable(name,value, true));
    }
    else //update variables
    {
        charVariable* var = charVariables.GetElementPointer(name);
        var->value = value;
        var->dirty = true;
        var->intBuff.SetBase(strtoul(value.GetDataSafe(),NULL,0));
    }
}

void psCharacter::SetVariable(const csString &name)
{
    csString value("");
    SetVariable(name, value);
}

void psCharacter::UnSetVariable(const csString &name)
{
    charVariables.DeleteAll(name);
    //update the database
    db->CommandPump("DELETE FROM character_variables where "
                    "character_id = %u and name = \"%s\"",
                    pid.Unbox(),
                    name.GetData());

}

csHash<charVariable, csString>::ConstGlobalIterator psCharacter::GetVariables() const
{
    return charVariables.GetIterator();
}



unsigned int psCharacter::GetExperiencePoints() // W
{
    return vitals->GetExp();
}

void psCharacter::SetExperiencePoints(unsigned int W)
{
    vitals->SetExp(W);
}

/*
* Will adde W to the experience points. While the number
* of experience points are greater than needed points
* for progression points the experience points are transformed
* into  progression points.
* @return Return the number of progression points gained.
*/
unsigned int psCharacter::AddExperiencePoints(unsigned int W)
{
    unsigned int pp = 0;
    unsigned int exp = vitals->GetExp();
    unsigned int progP = vitals->GetPP();

    exp += W;
    bool updatedPP = false;

    while(exp >= 200)
    {
        exp -= 200;
        if(progP != UINT_MAX) //don't allow overflow
            progP++;
        pp++;
        updatedPP = true;
    }

    vitals->SetExp(exp);
    if(updatedPP)
    {
        SetProgressionPoints(progP,true);
    }

    return pp;
}

unsigned int psCharacter::AddExperiencePointsNotify(unsigned int experiencePoints)
{
    if(experiencePoints > 0)
    {
        unsigned int PP = AddExperiencePoints(experiencePoints);
        if(GetActor() && GetActor()->GetClientID())
        {
            if(PP > 0)
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(), "You gained %d experience point%s and %d progression point%s!",
                                         experiencePoints, experiencePoints>1?"s":"", PP, PP>1?"s":"");
            }
            else
            {
                psserver->SendSystemInfo(GetActor()->GetClientID(), "You gained %d experience point%s",experiencePoints,experiencePoints>1?"s":"");
            }
        }
        return PP;
    }
    return 0;
}

unsigned int psCharacter::CalculateAddExperience(PSSKILL skill, unsigned int practicePoints, float modifier)
{
    if(practicePoints > 0)
    {
        MathEnvironment env;
        env.Define("ZCost", skills.Get(skill).zCost);
        env.Define("YCost", skills.Get(skill).yCost);
        env.Define("ZCostNext", skills.Get(skill).zCostNext);
        env.Define("YCostNext", skills.Get(skill).yCostNext);
        env.Define("Character", this);
        env.Define("PracticePoints", practicePoints);
        env.Define("Modifier", modifier);
        (void) psserver->GetCacheManager()->GetExpSkillCalc()->Evaluate(&env);
        unsigned int experiencePoints = env.Lookup("Exp")->GetRoundValue();

        if(GetActor()->GetClient()->GetSecurityLevel() >= GM_DEVELOPER)
        {
            psserver->SendSystemInfo(GetActor()->GetClientID(),
                                     "Giving %d experience and %d practicepoints to skill %d with modifier %f.\n"
                                     "zcost for the skill is %d for this level and %d for the next level\n"
                                     "ycost for the skill is %d for this level and %d for the next level\n",
                                     experiencePoints, practicePoints, skill, modifier,
                                     skills.Get(skill).zCost, skills.Get(skill).zCostNext,
                                     skills.Get(skill).yCost, skills.Get(skill).yCostNext);
        }

        AddExperiencePointsNotify(experiencePoints);

        if(psserver->GetCacheManager()->GetSkillByID((PSSKILL)skill))  //check if skill is valid
        {
            Skills().AddSkillPractice(skill, practicePoints);
        }
        return experiencePoints;
    }
    return 0;
}

void psCharacter::SetSpouseName(const char* name)
{
    if(!name)
        return;

    spouseName = name;

    if(!strcmp(name,""))
        isMarried = false;
    else
        isMarried = true;

}

unsigned int psCharacter::GetProgressionPoints() // X
{
    return vitals->GetPP();
}

void psCharacter::SetProgressionPoints(unsigned int X,bool save)
{
    unsigned int exp = vitals->GetExp();
    if(save)
    {
        Debug3(LOG_SKILLXP, pid.Unbox(), "Updating PP points and Exp to %u and %u\n", X, exp);
        // Update the DB
        csString sql;
        sql.Format("UPDATE characters SET progression_points = '%u', experience_points = '%u' WHERE id ='%u'", X, exp, pid.Unbox());
        if(!db->CommandPump(sql))
        {
            Error3("Couldn't execute SQL %s!, %s's PP points are NOT saved", sql.GetData(), ShowID(pid));
        }
    }

    vitals->SetPP(X);
}

void psCharacter::UseProgressionPoints(unsigned int X)
{

    SetProgressionPoints(vitals->GetPP()-X,true);
}

int psCharacter::GetMaxAllowedRealm(PSSKILL skill)
{
    unsigned int waySkillRank = skills.GetSkillRank(skill).Current();

    // Special case for rank 0 people just starting.
    if(waySkillRank == 0 && skills.GetSkillRank(skill).Base() == 0 && !skills.Get(skill).CanTrain())
        return 1;

    MathEnvironment env;
    env.Define("WaySkill", waySkillRank);

    (void) psserver->GetCacheManager()->GetMaxRealmScript()->Evaluate(&env);

    MathVar* maxRealm = env.Lookup("MaxRealm");
    if(!maxRealm)
    {
        Error1("Failed to evaluate MathScript >MaxRealm<.");
        return 0;
    }

    return maxRealm->GetRoundValue();
}

void psCharacter::DropItem(psItem* &item, csVector3 suggestedPos, const csVector3 &rot, bool guarded, bool transient, bool inplace)
{
    if(!item)
        return;

    if(item->IsInUse())
    {
        psserver->SendSystemError(actor->GetClientID(),"You cannot drop an item while using it.");
        return;
    }

    // Handle position
    if(inplace)
    {
        // drop at the character's position.
        suggestedPos.x = location.loc.x;
        suggestedPos.y = location.loc.y;
        suggestedPos.z = location.loc.z;
    }
    else if(suggestedPos != 0)
    {
        // User-specified position: check if it's close enough to the character.
        csVector3 delta;
        delta = suggestedPos - location.loc;

        if(delta.Norm() > MAX_DROP_DISTANCE && actor->GetClient()->GetSecurityLevel() < GM_DEVELOPER)
        {
            // Not close enough, cap it to the maximum range in the specified direction.
            suggestedPos = location.loc + delta.Unit() * MAX_DROP_DISTANCE;
        }
    }
    else
    {
        // No position specified.
        suggestedPos.x = location.loc.x - (DROP_DISTANCE * sinf(location.loc_yrot));
        suggestedPos.y = location.loc.y;
        suggestedPos.z = location.loc.z - (DROP_DISTANCE * cosf(location.loc_yrot));
    }

    // Play the drop item sound for this item
    psserver->GetCharManager()->SendOutPlaySoundMessage(actor->GetClientID(), item->GetSound(), "drop");

    // Announce drop (in the future, there should be a drop animation)
    psSystemMessage newmsg(actor->GetClientID(), MSG_INFO_BASE, "%s dropped %s.", fullName.GetData(), item->GetQuantityName().GetData());
    newmsg.Multicast(actor->GetMulticastClients(), 0, RANGE_TO_SELECT);

    // If we're dropping from inventory, we should properly remove it.
    // No need to check the return value: we're removing the whole item and
    // already have a pointer.  Plus, well get NULL and crash if it isn't...
    inventory.RemoveItemID(item->GetUID());

    if(guarded) //if we want to guard the item assign the guarding pid
        item->SetGuardingCharacterID(pid);

    gemObject* obj = EntityManager::GetSingleton().MoveItemToWorld(item,
                     location.worldInstance, location.loc_sector,
                     suggestedPos.x, suggestedPos.y, suggestedPos.z,
                     rot.x, rot.y, rot.z, this, transient);

    if(obj)
    {
        // Assign new object to replace the original object
        item = obj->GetItem();
    }

    psMoney money;

    psDropEvent evt(pid,
                    GetCharName(),
                    item->GetUID(),
                    item->GetName(),
                    item->GetStackCount(),
                    (int)item->GetCurrentStats()->GetQuality(),
                    0);
    evt.FireEvent();

    // If a container, move its contents as well...
    gemContainer* cont = dynamic_cast<gemContainer*>(obj);
    if(cont)
    {
        for(size_t i=0; i < Inventory().GetInventoryIndexCount(); i++)
        {
            psItem* item = Inventory().GetInventoryIndexItem(i);
            if(item->GetContainerID() == cont->GetItem()->GetUID())
            {
                // This item is in the dropped container
                size_t slot = item->GetLocInParent() - PSCHARACTER_SLOT_BULK1;
                Inventory().RemoveItemIndex(i);
                if(!cont->AddToContainer(item, actor->GetClient(), (int)slot))
                {
                    Error2("Cannot add item into container slot %zu.\n", slot);
                    return;
                }
                if(guarded) //if we want to guard the item assign the guarding pid
                    item->SetGuardingCharacterID(pid);
                i--; // indexes shift when we remove one.
                item->Save(false);
            }
        }
    }
}

// This is lame, but we need a key for these special "modifier" things
#define MODIFIER_FAKE_ACTIVESPELL ((ActiveSpell*) 0xf00)
void psCharacter::CalculateEquipmentModifiers()
{
    // In this method we potentially call Equip and Unequip scripts that modify stats.
    // The stat effects that these scripts have are indistinguishable from magic effects on stats,
    // and magical changes to stats also need to call this method (weapon may need to be set
    // inactive when strength spell expires). This could result in an
    // endless loop, hence this method locks itself against being called recursively.
    static bool lock_me = false;
    if(lock_me)
    {
        return;
    }
    lock_me = true;

    csList<psItem*> itemlist;

    for(int i = 0; i < PSITEMSTATS_STAT_COUNT; i++)
    {
        modifiers[(PSITEMSTATS_STAT) i].Cancel(MODIFIER_FAKE_ACTIVESPELL);
    }

    psItem* currentitem = NULL;

    // Loop through every holding item adding it to list of items to check
    for(int i = 0; i < PSCHARACTER_SLOT_BULK1; i++)
    {
        currentitem=inventory.GetInventoryItem((INVENTORY_SLOT_NUMBER)i);

        // checking the equipment array is necessary since the item could be just unequipped
        // (this method is also called by Unequip)
        if(!currentitem || inventory.GetEquipmentObject(currentitem->GetLocInParent()).itemIndexEquipped == 0)
            continue;

        itemlist.PushBack(currentitem);
    }
    // go through list and make items active whose requirements are fulfilled and remove item from list.
    // stop when a complete loop has been made without making a change.
    bool hasChanged;
    do
    {
        hasChanged = false;
        csList<psItem*>::Iterator it(itemlist);
        while(it.HasNext())
        {
            currentitem = it.Next();

            csString response;
            if(!currentitem->CheckRequirements(this, response))
            {
                continue;
            }
            if(!currentitem->IsActive())
            {
                currentitem->RunEquipScript(actor);
            }
            // Check for attr bonuses
            for(int i = 0; i < PSITEMSTATS_STAT_BONUS_COUNT; i++)
            {
                PSITEMSTATS_STAT attributeNum = currentitem->GetWeaponAttributeBonusType(i);
                if(attributeNum != PSITEMSTATS_STAT_NONE)
                {
                    modifiers[attributeNum].Buff(MODIFIER_FAKE_ACTIVESPELL, (int) currentitem->GetWeaponAttributeBonusMax(i));
                }
            }
            hasChanged = true;
            itemlist.Delete(it);
            break;
        }
    }
    while(hasChanged);

    // go through list of items whose requirements are not fulfilled and deactivate them
    csList<psItem*>::Iterator i(itemlist);
    while(i.HasNext())
    {
        currentitem = i.Next();
        if(currentitem->IsActive())
        {
            currentitem->CancelEquipScript();
        }
    }
    itemlist.DeleteAll();
    lock_me = false;
}

void psCharacter::AddLootItem(psItem* item)
{
    if(!item)
    {
        Error2("Attempted to add 'null' loot item to character %s, ignored.",fullName.GetDataSafe());
        return;
    }
    lootPending.Push(item);
}

size_t psCharacter::GetLootItems(psLootMessage &msg, EID entity, int cnum)
{
    if(lootPending.GetSize())
    {
        csString loot;
        loot.Append("<loot>");

        for(size_t i=0; i<lootPending.GetSize(); i++)
        {
            if(!lootPending[i])
            {
                printf("Potential ERROR: why this happens?");
                continue;
            }
            csString item;
            csString escpxml_imagename = EscpXML(lootPending[i]->GetImageName());
            csString escpxml_name = EscpXML(lootPending[i]->GetName());
            item.Format("<li><image icon=\"%s\" count=\"1\" /><desc text=\"%s\" /><id text=\"%u\" /></li>",
                        escpxml_imagename.GetData(),
                        escpxml_name.GetData(),
                        lootPending[i]->GetBaseStats()->GetUID()); //use the basic item id to reference this.
            loot.Append(item);
        }
        loot.Append("</loot>");
        Debug3(LOG_COMBAT, pid.Unbox(), "Loot was %s for %s\n", loot.GetData(), name.GetData());
        msg.Populate(entity,loot,cnum);
    }
    return lootPending.GetSize();
}

csArray<psItem*> psCharacter::RemoveLootItems(csArray<csString> categories)
{
    csArray<psItem*> items;

    for(int x = lootPending.GetSize()-1; x >= 0; x--)
    {
        if(!lootPending[x])
        {
            printf("Potential ERROR: why this happens?");
            lootPending.DeleteIndex(x);
            continue;
        }

        if(categories.IsEmpty())
        {
            items.Push(lootPending[x]);
            lootPending.DeleteIndex(x);
        }
        else
        {
            for(size_t i = 0; i < categories.GetSize(); i++)
            {
                if(lootPending[x]->GetCategory()->name.Find(categories[i], 0, true) != csArrayItemNotFound)
                {
                    items.Push(lootPending[x]);
                    lootPending.DeleteIndex(x);
                    break; // inner loop. Item in pos x was removed
                }
            }
        }
    }
    return items;
}

psItem* psCharacter::RemoveLootItem(int id)
{
    size_t x;
    for(x = 0; x < lootPending.GetSize(); x++)
    {
        if(!lootPending[x])
        {
            printf("Potential ERROR: why this happens?");
            lootPending.DeleteIndex(x);
            continue;
        }

        if(lootPending[x]->GetBaseStats()->GetUID() == (uint32) id)
        {
            psItem* item = lootPending[x];
            lootPending.DeleteIndex(x);
            return item;
        }
    }
    return NULL;
}
int psCharacter::GetLootMoney()
{
    int val = lootMoney;
    lootMoney = 0;
    return val;
}

void psCharacter::ClearLoot()
{
    //delete the instanced items
    for(size_t i = 0; i < lootPending.GetSize(); i++)
        delete lootPending.Get(i);

    //delete the pointers to the instanced items we have just deleted
    lootPending.DeleteAll();
    lootMoney = 0;
}

void psCharacter::SetMoney(psMoney m)
{
    money          = m;
    SaveMoney(false);
}

void psCharacter::SetMoney(psItem* &itemdata)
{
    /// Check to see if the item is a money item and treat as a special case.
    if(itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_TRIA)
        money.AdjustTrias(itemdata->GetStackCount());

    if(itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_HEXA)
        money.AdjustHexas(itemdata->GetStackCount());

    if(itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_OCTA)
        money.AdjustOctas(itemdata->GetStackCount());

    if(itemdata->GetBaseStats()->GetFlags() & PSITEMSTATS_FLAG_CIRCLE)
        money.AdjustCircles(itemdata->GetStackCount());

    psserver->GetCacheManager()->RemoveInstance(itemdata);
    SaveMoney(false);
}

void psCharacter::SetMoney(psItemStats* MoneyObject,  int amount)
{
    /// Check to see if the item is a money item and treat as a special case.
    if(MoneyObject->GetFlags() & PSITEMSTATS_FLAG_TRIA)
        money.AdjustTrias(amount);

    if(MoneyObject->GetFlags() & PSITEMSTATS_FLAG_HEXA)
        money.AdjustHexas(amount);

    if(MoneyObject->GetFlags() & PSITEMSTATS_FLAG_OCTA)
        money.AdjustOctas(amount);

    if(MoneyObject->GetFlags() & PSITEMSTATS_FLAG_CIRCLE)
        money.AdjustCircles(amount);

    SaveMoney(false);
}

void psCharacter::AdjustMoney(psMoney m, bool bank)
{
    psMoney* mon;
    if(bank)
        mon = &bankMoney;
    else
        mon = &money;
    mon->Adjust(MONEY_TRIAS, m.GetTrias());
    mon->Adjust(MONEY_HEXAS, m.GetHexas());
    mon->Adjust(MONEY_OCTAS, m.GetOctas());
    mon->Adjust(MONEY_CIRCLES, m.GetCircles());
    SaveMoney(bank);
}

void psCharacter::SaveMoney(bool bank)
{
    if(!loaded)
        return;

    psString sql;

    if(bank)
    {
        sql.AppendFmt("update characters set bank_money_circles=%d, bank_money_trias=%d, bank_money_hexas=%d, bank_money_octas=%d where id=%u",
                      bankMoney.GetCircles(), bankMoney.GetTrias(), bankMoney.GetHexas(), bankMoney.GetOctas(), pid.Unbox());
    }
    else
    {
        sql.AppendFmt("update characters set money_circles=%d, money_trias=%d, money_hexas=%d, money_octas=%d where id=%u",
                      money.GetCircles(), money.GetTrias(), money.GetHexas(), money.GetOctas(), pid.Unbox());
    }

    if(db->CommandPump(sql) != 1)
    {
        Error3("Couldn't save character's money to database.\nCommand was "
               "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
    }
}

void psCharacter::ResetStats()
{
    vitals->ResetVitals();
    inventory.CalculateLimits();
}

void psCharacter::CombatDrain(int slot)
{
    psItem* weapon = inventory.GetEffectiveWeaponInSlot((INVENTORY_SLOT_NUMBER) slot);
    if(!weapon)//shouldn't happen
        return;

    MathEnvironment env;
    env.Define("Actor", GetActor());
    env.Define("Weapon", weapon);

    (void) psserver->GetCacheManager()->GetStaminaCombat()->Evaluate(&env);

    MathVar* phyDrain = env.Lookup("PhyDrain");
    MathVar* mntDrain = env.Lookup("MntDrain");
    if(!phyDrain || !mntDrain)
    {
        Error1("Failed to evaluate MathScript >StaminaCombat<.");
        return;
    }

    AdjustStamina(-phyDrain->GetValue(), true);
    AdjustStamina(-mntDrain->GetValue(), false);
}

void psCharacter::AdjustHitPoints(float delta)
{
    vitals->AdjustVital(VITAL_HITPOINTS, DIRTY_VITAL_HP, delta);
}

void psCharacter::SetHitPoints(float v)
{
    vitals->SetVital(VITAL_HITPOINTS, DIRTY_VITAL_HP,v);
}

float psCharacter::GetHP()
{
    return vitals->GetHP();
}

float psCharacter::GetMana()
{
    return vitals->GetMana();
}
float psCharacter::GetStamina(bool pys)
{
    return pys ? vitals->GetPStamina() : vitals->GetMStamina();
}

bool psCharacter::UpdateStatDRData(csTicks now)
{
    bool res = vitals->Update(now);

    // if HP dropped to zero, provoke the killing process
    if(GetHP() == 0   &&   actor != NULL   &&   actor->IsAlive())
    {
        actor->Kill(NULL);
    }
    return res;
}

bool psCharacter::SendStatDRMessage(uint32_t clientnum, EID eid, int flags, csRef<PlayerGroup> group)
{
    return vitals->SendStatDRMessage(clientnum, eid, flags, group);
}

VitalBuffable &psCharacter::GetMaxHP()
{
    return vitals->GetVital(VITAL_HITPOINTS).max;
}
VitalBuffable &psCharacter::GetMaxMana()
{
    return vitals->GetVital(VITAL_MANA).max;
}
VitalBuffable &psCharacter::GetMaxPStamina()
{
    return vitals->GetVital(VITAL_PYSSTAMINA).max;
}
VitalBuffable &psCharacter::GetMaxMStamina()
{
    return vitals->GetVital(VITAL_MENSTAMINA).max;
}

VitalBuffable &psCharacter::GetHPRate()
{
    return vitals->GetVital(VITAL_HITPOINTS).drRate;
}
VitalBuffable &psCharacter::GetManaRate()
{
    return vitals->GetVital(VITAL_MANA).drRate;
}
VitalBuffable &psCharacter::GetPStaminaRate()
{
    return vitals->GetVital(VITAL_PYSSTAMINA).drRate;
}
VitalBuffable &psCharacter::GetMStaminaRate()
{
    return vitals->GetVital(VITAL_MENSTAMINA).drRate;
}

void psCharacter::AdjustMana(float adjust)
{
    vitals->AdjustVital(VITAL_MANA, DIRTY_VITAL_MANA,adjust);
}

void psCharacter::SetMana(float v)
{
    vitals->SetVital(VITAL_MANA, DIRTY_VITAL_MANA, v);
}

void psCharacter::AdjustStamina(float delta, bool pys)
{
    if(pys)
        vitals->AdjustVital(VITAL_PYSSTAMINA, DIRTY_VITAL_PYSSTAMINA, delta);
    else
        vitals->AdjustVital(VITAL_MENSTAMINA, DIRTY_VITAL_MENSTAMINA, delta);
}

void psCharacter::SetStamina(float v,bool pys)
{
    if(pys)
        vitals->SetVital(VITAL_PYSSTAMINA, DIRTY_VITAL_PYSSTAMINA,v);
    else
        vitals->SetVital(VITAL_MENSTAMINA, DIRTY_VITAL_MENSTAMINA,v);
}

void psCharacter::SetStaminaRegenerationNone(bool physical,bool mental)
{
    if(physical) GetPStaminaRate().SetBase(0.0);
    if(mental)   GetMStaminaRate().SetBase(0.0);
}

void psCharacter::SetStaminaRegenerationWalk(bool physical,bool mental)
{
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_WALK]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_WALK]);

    (void) psserver->GetCacheManager()->GetStaminaRatioWalk()->Evaluate(&env);

    MathVar* ratePhy = env.Lookup("PStaminaRate");
    MathVar* rateMen = env.Lookup("MStaminaRate");

    if(physical && ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(mental   && rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());
}

void psCharacter::SetStaminaRegenerationSitting()
{
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);


    (void) psserver->GetCacheManager()->GetStaminaRatioSit()->Evaluate(&env);

    MathVar* ratePhy = env.Lookup("PStaminaRate");
    MathVar* rateMen = env.Lookup("MStaminaRate");

    if(ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());
}

void psCharacter::SetStaminaRegenerationStill(bool physical,bool mental)
{

    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);

    (void) psserver->GetCacheManager()->GetStaminaRatioStill()->Evaluate(&env);

    MathVar* ratePhy = env.Lookup("PStaminaRate");
    MathVar* rateMen = env.Lookup("MStaminaRate");

    if(physical && ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(mental   && rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());
}

void psCharacter::SetStaminaRegenerationWork(int skill)
{
    //Gms don't want to lose stamina when testing
    if(actor->nevertired)
        return;

    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("BaseRegenPhysical", GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_PHYSICAL_STILL]);
    env.Define("BaseRegenMental",   GetRaceInfo()->baseRegen[PSRACEINFO_STAMINA_MENTAL_STILL]);

    // Need real formula for this. Shouldn't be hard coded anyway.
    // Stamina drain needs to be set depending on the complexity of the task.
    psSkillInfo* skillInfo = psserver->GetCacheManager()->GetSkillByID(skill);
    //if skill is none (-1) we set zero here
    int factor = skillInfo? skillInfo->mental_factor : 100;

    env.Define("SkillMentalFactor",  factor);

    (void) psserver->GetCacheManager()->GetStaminaRatioWork()->Evaluate(&env);

    MathVar* ratePhy = env.Lookup("PStaminaRate");
    MathVar* rateMen = env.Lookup("MStaminaRate");

    if(ratePhy) GetPStaminaRate().SetBase(ratePhy->GetValue());
    if(rateMen) GetMStaminaRate().SetBase(rateMen->GetValue());
}

void psCharacter::CalculateMaxStamina()
{
    MathEnvironment env;
    // Set the actor to retreive the skill values from
    env.Define("Actor", this);

    (void) psserver->GetCacheManager()->GetStaminaCalc()->Evaluate(&env);

    MathVar* basePhy = env.Lookup("BasePhy");
    MathVar* baseMen = env.Lookup("BaseMen");
    if(!basePhy || !baseMen)
    {
        Error1("Failed to evaluate MathScript >StaminaBase<.");
        return;
    }
    // Set the max values
    GetMaxPStamina().SetBase(basePhy->GetValue());
    GetMaxMStamina().SetBase(baseMen->GetValue());
}

unsigned int psCharacter::GetStatsDirtyFlags() const
{
    return vitals->GetStatsDirtyFlags();
}

void psCharacter::SetAllStatsDirty()
{
    vitals->SetAllStatsDirty();
}

void psCharacter::ClearStatsDirtyFlags(unsigned int dirtyFlags)
{
    vitals->ClearStatsDirtyFlags(dirtyFlags);
}

void psCharacter::ResetSwings(csTicks timeofattack)
{
    psItem* Weapon;

    for(int slot = 0; slot < PSCHARACTER_SLOT_BULK1; slot++)
    {
        Weapon = Inventory().GetEffectiveWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);
        if(Weapon !=NULL)
        {
            inventory.GetEquipmentObject((INVENTORY_SLOT_NUMBER)slot).eventId=0;
        }
    }
}

void psCharacter::TagEquipmentObject(INVENTORY_SLOT_NUMBER slot,int eventId)
{
    psItem* Weapon;

    // Slot out of range
    if(slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return;

    // TODO: Reduce ammo if this is an ammunition using weapon

    // Reset next attack time
    Weapon=Inventory().GetEffectiveWeaponInSlot(slot);

    if(!Weapon)  //no need to continue
        return;

    inventory.GetEquipmentObject(slot).eventId = eventId;

    //drain stamina on player attacks
    if(actor->GetClientID() && !actor->nevertired)
        CombatDrain(slot);
}

int psCharacter::GetSlotEventId(INVENTORY_SLOT_NUMBER slot)
{
    // Slot out of range
    if(slot<0 || slot>=PSCHARACTER_SLOT_BULK1)
        return 0;

    return inventory.GetEquipmentObject(slot).eventId;
}

// AVPRO= attack Value progression (this is to change the progression of all the calculation in AV for now it is equal to 1)
#define AVPRO 1

float psCharacter::GetTargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    psItem* weapon=Inventory().GetEffectiveWeaponInSlot(slot);
    if(weapon==NULL)
        return 0.0f;

    return weapon->GetTargetedBlockValue();
}

float psCharacter::GetUntargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    psItem* weapon=Inventory().GetEffectiveWeaponInSlot(slot);
    if(weapon==NULL)
        return 0.0f;

    return weapon->GetUntargetedBlockValue();
}


float psCharacter::GetTotalTargetedBlockValue()
{
    float blockval=0.0f;
    int slot;

    for(slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
        blockval+=GetTargetedBlockValueForWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);

    return blockval;
}

float psCharacter::GetTotalUntargetedBlockValue()
{
    float blockval=0.0f;
    int slot;

    for(slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
        blockval+=GetUntargetedBlockValueForWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);

    return blockval;
}


float psCharacter::GetCounterBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot)
{
    psItem* weapon=inventory.GetEffectiveWeaponInSlot(slot);
    if(weapon==NULL)
        return 0.0f;

    return weapon->GetCounterBlockValue();
}

bool psCharacter::ArmorUsesSkill(INVENTORY_SLOT_NUMBER slot, PSITEMSTATS_ARMORTYPE skill)
{
    if(inventory.GetInventoryItem(slot)==NULL)
        return inventory.GetEquipmentObject(slot).default_if_empty->GetArmorType()==skill;
    else
        return inventory.GetInventoryItem(slot)->GetArmorType()==skill;
}

void psCharacter::CalculateArmorForSlot(INVENTORY_SLOT_NUMBER slot, float &heavy_p, float &med_p, float &light_p)
{
    if(ArmorUsesSkill(slot,PSITEMSTATS_ARMORTYPE_LIGHT)) light_p+=1.0f/6.0f;
    if(ArmorUsesSkill(slot,PSITEMSTATS_ARMORTYPE_MEDIUM)) med_p+=1.0f/6.0f;
    if(ArmorUsesSkill(slot,PSITEMSTATS_ARMORTYPE_HEAVY)) heavy_p+=1.0f/6.0f;
}


float psCharacter::GetDodgeValue()
{
    float heavy_p,med_p,light_p;

    // hold the % of each type of armor worn
    heavy_p=med_p=light_p=0.0f;

    CalculateArmorForSlot(PSCHARACTER_SLOT_HELM, heavy_p, med_p, light_p);
    CalculateArmorForSlot(PSCHARACTER_SLOT_TORSO, heavy_p, med_p, light_p);
    CalculateArmorForSlot(PSCHARACTER_SLOT_ARMS, heavy_p, med_p, light_p);
    CalculateArmorForSlot(PSCHARACTER_SLOT_GLOVES, heavy_p, med_p, light_p);
    CalculateArmorForSlot(PSCHARACTER_SLOT_LEGS, heavy_p, med_p, light_p);
    CalculateArmorForSlot(PSCHARACTER_SLOT_BOOTS, heavy_p, med_p, light_p);

    MathEnvironment env;

    // Add actor to manipulate and points to calculate.
    env.Define("Actor", this);
    env.Define("HeavyPoints", heavy_p);
    env.Define("MediumPoints", med_p);
    env.Define("LightPoints", light_p);

    (void) psserver->GetCacheManager()->GetDodgeValueCalc()->Evaluate(&env);

    return env.Lookup("Result")->GetValue();
}

/**
 * PracticeArmorSkills is never used.
 */
void psCharacter::PracticeArmorSkills(unsigned int practice, INVENTORY_SLOT_NUMBER attackLocation)
{
    unsigned int heavy_p = 0;
    unsigned int med_p = 0;
    unsigned int light_p = 0;

    psItem* armor = inventory.GetEffectiveArmorInSlot(attackLocation);

    switch(armor->GetArmorType())
    {
        case PSITEMSTATS_ARMORTYPE_LIGHT:
            light_p = practice;
            break;
        case PSITEMSTATS_ARMORTYPE_MEDIUM:
            med_p = practice;
            break;
        case PSITEMSTATS_ARMORTYPE_HEAVY:
            heavy_p = practice;
            break;
        default:
            break;
    }

    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("HeavyPoints", heavy_p);
    env.Define("MediumPoints", med_p);
    env.Define("LightPoints", light_p);

    (void) psserver->GetCacheManager()->GetArmorSkillsPractice()->Evaluate(&env);
}

/**
 * PracticeWeaponSkills is never used.
 */
void psCharacter::PracticeWeaponSkills(unsigned int practice)
{
    int slot;

    for(slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
    {
        psItem* weapon=inventory.GetEffectiveWeaponInSlot((INVENTORY_SLOT_NUMBER)slot);
        if(weapon!=NULL)
            PracticeWeaponSkills(weapon,practice);
    }

}

void psCharacter::PracticeWeaponSkills(psItem* weapon, unsigned int practice)
{
    for(int index = 0; index < PSITEMSTATS_WEAPONSKILL_INDEX_COUNT; index++)
    {
        PSSKILL skill = weapon->GetWeaponSkill((PSITEMSTATS_WEAPONSKILL_INDEX)index);
        if(skill != PSSKILL_NONE)
            skills.AddSkillPractice(skill,practice);
    }
}

void psCharacter::SetTraitForLocation(PSTRAIT_LOCATION location,psTrait* trait)
{
    if(location<0 || location>=PSTRAIT_LOCATION_COUNT)
        return;

    traits[location]=trait;
}

psTrait* psCharacter::GetTraitForLocation(PSTRAIT_LOCATION location)
{
    if(location<0 || location>=PSTRAIT_LOCATION_COUNT)
        return NULL;

    return traits[location];
}


void psCharacter::GetLocationInWorld(InstanceID &instance,psSectorInfo* &sectorinfo,float &loc_x,float &loc_y,float &loc_z,float &loc_yrot)
{
    sectorinfo=location.loc_sector;
    loc_x=location.loc.x;
    loc_y=location.loc.y;
    loc_z=location.loc.z;
    loc_yrot=location.loc_yrot;
    instance = location.worldInstance;
}

void psCharacter::SetLocationInWorld(InstanceID instance, psSectorInfo* sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot)
{
    psSectorInfo* oldsector = location.loc_sector;
    InstanceID oldInstance = location.worldInstance;

    location.loc_sector=sectorinfo;
    location.loc.x=loc_x;
    location.loc.y=loc_y;
    location.loc.z=loc_z;
    location.loc_yrot=loc_yrot;
    location.worldInstance = instance;

    if(oldInstance != instance || (oldsector && oldsector != sectorinfo))
    {
        if(GetCharType() == PSCHARACTER_TYPE_PLAYER)    // NOT an NPC so it's ok to save location info
            SaveLocationInWorld();
    }
}

void psCharacter::SaveLocationInWorld()
{
    if(!loaded)
        return;

    st_location &l = location;
    psString sql;

    sql.AppendFmt("update characters set loc_x=%10.2f, loc_y=%10.2f, loc_z=%10.2f, loc_yrot=%10.2f, loc_sector_id=%u, loc_instance=%u where id=%u",
                  l.loc.x, l.loc.y, l.loc.z, l.loc_yrot, l.loc_sector->uid, l.worldInstance, pid.Unbox());
    if(db->CommandPump(sql) != 1)
    {
        Error3("Couldn't save character's position to database.\nCommand was "
               "<%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
    }
}




psSpell* psCharacter::GetSpellByName(const csString &spellName)
{
    for(size_t i=0; i < spellList.GetSize(); i++)
    {
        if(spellList[i]->GetName().CompareNoCase(spellName)) return spellList[i];
    }
    return NULL;
}

psSpell* psCharacter::GetSpellByIdx(int index)
{
    if(index < 0 || (size_t)index >= spellList.GetSize())
        return NULL;
    return spellList[index];
}

bool psCharacter::SetTradingStopped(bool stopped)
{
    bool old = tradingStopped;
    tradingStopped=stopped;
    return old;
}

// Check if player and target is ready to do a exchange
//       - Not fighting
//       - Not casting spell
//       - Not exchanging with a third player
//       - Not player stopped trading
//       - Not trading with a merchant
bool psCharacter::ReadyToExchange()
{
    return (//TODO: Test for fighting &&
               //TODO: Test for casting spell
               //  !exchangeMgr.IsValid() &&
               !tradingStopped &&
               tradingStatus == NOT_TRADING);

}

void psCharacter::MakeTextureString(csString &traits)
{
    // initialize string
    traits = "<traits>";

    // cycle through and add entries for each part
    for(unsigned int i=0; i<PSTRAIT_LOCATION_COUNT; i++)
    {
        psTrait* trait;
        trait = GetTraitForLocation((PSTRAIT_LOCATION)i);
        while(trait != NULL)
        {
            csString buff = trait->ToXML(true);
            traits.Append(buff);
            trait = trait->next_trait;
        }
    }

    // terminate string
    traits.Append("</traits>");

    Notify2(LOG_CHARACTER, "Traits string: %s", (const char*)traits);
}

void psCharacter::MakeEquipmentString(csString &equipment)
{
    equipment = "<equiplist>";
    equipment.AppendFmt("<helm>%s</helm><bracer>%s</bracer><belt>%s</belt><cloak>%s</cloak>", EscpXML(helmGroup).GetData(), EscpXML(BracerGroup).GetData(), EscpXML(BeltGroup).GetData(), EscpXML(CloakGroup).GetData());

    for(int i=0; i<PSCHARACTER_SLOT_BULK1; i++)
    {
        psItem* item = inventory.GetInventoryItem((INVENTORY_SLOT_NUMBER)i);
        if(item == NULL)
            continue;

        csString slot = EscpXML(psserver->GetCacheManager()->slotNameHash.GetName(i));
        csString mesh = EscpXML(item->GetMeshName());
        csString part = EscpXML(item->GetPartName());
        csString texture = EscpXML(item->GetTextureName());
        csString partMesh = EscpXML(item->GetPartMeshName());
        csString removedMesh = EscpXML(item->GetSlotRemovedMesh(i, GetActor() ? GetActor()->GetMesh() : (csString)GetRaceInfo()->GetMeshName()));

        equipment.AppendFmt("<equip slot=\"%s\" mesh=\"%s\" part=\"%s\" texture=\"%s\" partMesh=\"%s\"  removedMesh=\"%s\" />",
                            slot.GetData(), mesh.GetData(), part.GetData(), texture.GetData(), partMesh.GetData(), removedMesh.GetData());
    }

    equipment.Append("</equiplist>");

    Notify2(LOG_CHARACTER, "Equipment string: %s", equipment.GetData());
}


bool psCharacter::AppendCharacterSelectData(psAuthApprovedMessage &auth)
{
    csString traits;
    csString equipment;

    MakeTextureString(traits);
    MakeEquipmentString(equipment);

    auth.AddCharacter(fullName, GetRaceInfo()->GetName(), GetRaceInfo()->GetMeshName(), traits, equipment);
    return true;
}


bool psCharacter::CheckResponsePrerequisite(NpcResponse* resp)
{
    CS_ASSERT(resp);    // Must not be NULL

    return resp->CheckPrerequisite(this);
}




psGuildLevel* psCharacter::GetGuildLevel()
{
    if(guildinfo == NULL)
        return 0;

    psGuildMember* membership = guildinfo->FindMember(pid);
    if(membership == NULL)
        return 0;

    return membership->guildlevel;
}

psGuildMember* psCharacter::GetGuildMembership()
{
    if(guildinfo == NULL)
        return 0;

    return guildinfo->FindMember(pid);
}

bool psCharacter::Knows(PID charID)
{
    // Introduction system is currently disabled - it's trivially worked
    // around, and simply alienating players.
    //return acquaintances.Contains(charID);
    return true;
}

bool psCharacter::Introduce(psCharacter* c)
{
    if(!c) return false;
    PID theirID = c->GetPID();

    if(!acquaintances.Contains(theirID))
    {
        acquaintances.AddNoTest(theirID);
        db->CommandPump("insert into introductions values(%d, %d)", this->pid.Unbox(), theirID.Unbox());
        return true;
    }
    return false;
}

bool psCharacter::Unintroduce(psCharacter* c)
{
    if(!c) return false;
    PID theirID = c->GetPID();

    if(acquaintances.Contains(theirID))
    {
        acquaintances.Delete(theirID);
        db->CommandPump("delete from introductions where charid=%d and introcharid=%d", this->pid.Unbox(), theirID.Unbox());
        return true;
    }
    return false;
}


bool psCharacter::AddExploredArea(PID explored)
{
    int rows = db->Command("INSERT INTO character_relationships (character_id, related_id, relationship_type) VALUES (%u, %u, 'exploration')",
                           pid.Unbox(), explored.Unbox());

    if(rows != 1)
    {
        psserver->GetDatabase()->SetLastError(psserver->GetDatabase()->GetLastSQLError());
        return false;
    }

    exploredAreas.Push(explored);

    return true;
}

bool psCharacter::HasExploredArea(PID explored)
{
    for(size_t i=0; i<exploredAreas.GetSize(); ++i)
    {
        if(explored == exploredAreas[i])
            return true;
    }

    return false;
}

double psCharacter::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "AttackerTargeted")
    {
        return true;
        // return (attacker_targeted) ? 1 : 0;
    }
    else if(property == "TotalTargetedBlockValue")
    {
        return GetTotalTargetedBlockValue();
    }
    else if(property == "TotalUntargetedBlockValue")
    {
        return GetTotalUntargetedBlockValue();
    }
    else if(property == "DodgeValue")
    {
        return GetDodgeValue();
    }
    else if(property == "KillExp")
    {
        return killExp;
    }
    else if(property == "GetAttackValueModifier")
    {
        return attackModifier.Value();
    }
    else if(property == "GetDefenseValueModifier")
    {
        return defenseModifier.Value();
    }
    else if(property == "HP")
    {
        return GetHP();
    }
    else if(property == "MaxHP")
    {
        return GetMaxHP().Current();
    }
    else if(property == "BaseHP")
    {
        return GetMaxHP().Base();
    }
    else if(property == "Mana")
    {
        return GetMana();
    }
    else if(property == "MaxMana")
    {
        return GetMaxMana().Current();
    }
    else if(property == "BaseMana")
    {
        return GetMaxMana().Base();
    }
    else if(property == "PStamina")
    {
        return GetStamina(true);
    }
    else if(property == "MStamina")
    {
        return GetStamina(false);
    }
    else if(property == "MaxPStamina")
    {
        return GetMaxPStamina().Current();
    }
    else if(property == "MaxMStamina")
    {
        return GetMaxMStamina().Current();
    }
    else if(property == "BasePStamina")
    {
        return GetMaxPStamina().Base();
    }
    else if(property == "BaseMStamina")
    {
        return GetMaxMStamina().Base();
    }
    else if(property == "AllArmorStrMalus")
    {
        return modifiers[PSITEMSTATS_STAT_STRENGTH].Current();
    }
    else if(property == "AllArmorAgiMalus")
    {
        return modifiers[PSITEMSTATS_STAT_AGILITY].Current();
    }
    else if(property == "PID")
    {
        return (double) pid.Unbox();
    }
    else if(property == "loc_x")
    {
        return location.loc.x;
    }
    else if(property == "loc_y")
    {
        return location.loc.y;
    }
    else if(property == "loc_z")
    {
        return location.loc.z;
    }
    else if(property == "loc_yrot")
    {
        return location.loc_yrot;
    }
    else if(property == "sector")
    {
        return env->GetValue(location.loc_sector);
    }
    else if(property == "owner")
    {
        return (double) ownerId.Unbox();
    }
    else if(property == "IsNPC")
    {
        return (double)IsNPC();
    }
    else if(property == "IsPet")
    {
        return (double)IsPet();
    }
    else if(property == "Race")
    {
        if(!GetRaceInfo())
            return 0;
        return (double)GetRaceInfo()->GetRaceID();
    }
    else if(property == "RaceUID")
    {
        if(!GetRaceInfo())
            return 0;
        return (double)GetRaceInfo()->GetUID();
    }

    Error2("Requested psCharacter property not found '%s'", ptr);
    return 0;
}

double psCharacter::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    csString function(functionName);
    if(function == "HasCompletedQuest")
    {
        const char* questName = env->GetString(params[0]);
        psQuest* quest = psserver->GetCacheManager()->GetQuestByName(questName);
        return (double) questManager.CheckQuestCompleted(quest);
    }
    else if(function == "Faction")
    {
        const char* factionName = env->GetString(params[0]);
        Faction* faction = psserver->GetCacheManager()->GetFactionByName(factionName);
        return (double)(faction ? factions->GetFaction(faction) : 0);
    }
    else if(function == "GetStatValue")
    {
        PSITEMSTATS_STAT stat = (PSITEMSTATS_STAT)(int)params[0];

        PSSKILL skill = statToSkill(stat);
        if (skill != PSSKILL_NONE)
        {
            return (double) GetSkillRank(statToSkill(stat)).Current();
        }
    }
    else if(function == "GetAverageSkillValue")
    {
        PSSKILL skill1 = (PSSKILL)(int)params[0];
        PSSKILL skill2 = (PSSKILL)(int)params[1];
        PSSKILL skill3 = (PSSKILL)(int)params[2];

        double v1 = skills.GetSkillRank(skill1).Current();

        double count = (double)(1+(skill2!=PSSKILL_NONE)+(skill3!=PSSKILL_NONE));
        v1 /= count;

        if(skill2!=PSSKILL_NONE)
        {
            double v2 = skills.GetSkillRank(skill2).Current();
            v1 += v2/count;
        }

        if(skill3!=PSSKILL_NONE)
        {
            double v3 = skills.GetSkillRank(skill3).Current();
            v1 += v3/count;
        }

        return v1;
    }
    else if(function == "SkillRank")
    {
        const char* skillName = env->GetString(params[0]);
        PSSKILL skill = psserver->GetCacheManager()->ConvertSkillString(skillName);
        if (skill != PSSKILL_NONE)
        {
            double value = skills.GetSkillRank(skill).Current();
            return value;
        }
    }
    else if(function == "GetSkillValue")
    {
        PSSKILL skill = (PSSKILL)(int)params[0];
        double value = skills.GetSkillRank(skill).Current();
        return value;
    }
    else if(function == "GetSkillBaseValue")
    {
        PSSKILL skill = (PSSKILL)(int)params[0];
        double value = skills.GetSkillRank(skill).Base();
        return value;
    }
    else if(function == "SetSkillValue")
    {
        PSSKILL skill = (PSSKILL)(int)params[0];
        skills.SetSkillRank(skill, (int)params[1]);
        return 0;
    }
    else if(function == "PracticeSkillID")
    {
        PSSKILL skill = (PSSKILL)(int)params[0];

        return skills.AddSkillPractice(skill, params[1]);
    }
    else if(function == "GetVariableValueInt")
    {
        const char* variableName = env->GetString(params[0]);
        double value = charVariables.Get(variableName, charVariable()).intBuff.Current();
        return value;
    }
    else if(function == "PracticeSkill")
    {
        const char* skillName = env->GetString(params[0]);
        PSSKILL skill = psserver->GetCacheManager()->ConvertSkillString(skillName);

        return skills.AddSkillPractice(skill, params[1]);
    }
    else if(function == "HasExploredArea")
    {
        if(!HasExploredArea(params[0]))
        {
            AddExploredArea(params[0]);
            return 0;
        }

        return 1;
    }
    else if(function == "IsWithin")
    {
        if(location.loc_sector->uid != params[4])
            return 0.0;

        csVector3 other(params[1], params[2], params[3]);
        return (csVector3(other - location.loc).Norm() <= params[0]) ? 1.0 : 0.0;
    }
    else if(function == "IsEnemy")
    {
        // Check for self.
        if(ownerId == params[0])
            return 0.0;

        Client* owner = EntityManager::GetSingleton().GetClients()->FindPlayer((PID)params[0]);
        if(owner && (owner->GetActor()->GetTargetType(GetActor()) & TARGET_FOE))
        {
            return 1.0;
        }

        return 0.0;
    }
    else if(function == "GetItem")
    {
        INVENTORY_SLOT_NUMBER slot = (INVENTORY_SLOT_NUMBER)(int)params[0];
        psItem* item = NULL;
        if(inventory.GetInventoryItem(slot))
        {
            item = inventory.GetInventoryItem(slot);
        }
        else
        {
            item = inventory.GetEquipmentObject(slot).default_if_empty;
        }

        return env->GetValue(item);
    }

    // deletes an item by its index in the inventory. DeleteItem(position, stackamount (-1 for whole stack))
    else if(function == "DeleteItem")
    {
        psItem* item = inventory.RemoveInventoryItem((INVENTORY_SLOT_NUMBER)int(params[0]), int(params[1]));

        //Item not found
        if(!item)
        {
            return 0.0f;
        }

        //Delete the item
        if(!item->Destroy())
        {
            Error2("Could not remove old item ID #%u from database", item->GetUID());
            return 0.0f;
        }

        if(GetActor() && GetActor()->GetClientID())
        {
            psserver->GetCharManager()->SendInventory(GetActor()->GetClientID());
        }

        return 1.0f;
    }
    /**
     * Seems not to be used and could be replaced by a correct GetSkillValue in scripts instead.
     */
    else if(function == "GetArmorSkill")
    {
        PSSKILL skill;
        switch((PSITEMSTATS_ARMORTYPE)(int)params[0])
        {
            case PSITEMSTATS_ARMORTYPE_LIGHT:
                skill = PSSKILL_LIGHTARMOR;
                break;
            case PSITEMSTATS_ARMORTYPE_MEDIUM:
                skill = PSSKILL_MEDIUMARMOR;
                break;
            case PSITEMSTATS_ARMORTYPE_HEAVY:
                skill = PSSKILL_HEAVYARMOR;
                break;
            default:
                return 0;
        }
        return (double)skills.GetSkillRank(skill).Current();
    }
    else if(function == "CalculateAddExperience")
    {
        return (double)CalculateAddExperience((PSSKILL)(int) params[0], (unsigned int) params[1], (float) params[2]);
    }

    CPrintf(CON_ERROR, "psCharacter::CalcFunction(%s) failed\n", functionName);
    return 0;
}

/** A skill can only be trained if the player requires points for it.
  */
bool psCharacter::CanTrain(PSSKILL skill)
{
    return skills.CanTrain(skill);
}

void psCharacter::GetSkillValues(MathEnvironment* env)
{
    env->Define("Actor", this);
    (void) psserver->GetCacheManager()->GetSkillValuesGet()->Evaluate(env);
}

void psCharacter::GetSkillBaseValues(MathEnvironment* env)
{
    env->Define("Actor", this);
    (void) psserver->GetCacheManager()->GetBaseSkillValuesGet()->Evaluate(env);
}

void psCharacter::Train(PSSKILL skill, int yIncrease)
{
    skills.Train(skill, yIncrease);   // Normal training

    skills.CheckDoRank(skill);
    if(!psServer::CharacterLoader.UpdateCharacterSkill(
                pid,
                skill,
                skills.GetSkillPractice((PSSKILL)skill),
                skills.GetSkillKnowledge((PSSKILL)skill),
                skills.GetSkillRank((PSSKILL)skill).Base()
            ))
    {
        Error2("Couldn't save skills for character %u!\n", pid.Unbox());
    }
}

void psCharacter::StartSong()
{
    // at this point songExecutionTime is bonusTime
    songExecutionTime = csGetTicks() - songExecutionTime;
}

void psCharacter::EndSong(csTicks bonusTime)
{
    // saving bonusTime for the next execution
    songExecutionTime = bonusTime;
}

/*-----------------------------------------------------------------*/


void Skill::CalculateCosts(psCharacter* user)
{
    if(!info || !user)
        return;

    // Calc the new Y/Z cost
    MathScript* script = psserver->GetMathScriptEngine()->FindScript(info->costScript);
    if(!script)
    {
        Error4("Couldn't find script %s to calculate skill cost of %d (%s)!",
               info->costScript.GetData(), info->id, info->name.GetData());
        return;
    }

    MathEnvironment env;
    env.Define("BaseCost",       info->baseCost);
    env.Define("SkillRank",      rank.Base());
    env.Define("SkillID",        info->id);
    env.Define("PracticeFactor", info->practice_factor);
    env.Define("MentalFactor",   info->mental_factor);
    env.Define("Actor",          user);

    (void) script->Evaluate(&env);

    MathVar* yCostVar = env.Lookup("YCost");
    MathVar* zCostVar = env.Lookup("ZCost");
    if(!yCostVar || !zCostVar)
    {
        Error4("Failed to evaluate MathScript >%s< to calculate skill cost of %d (%s).",
               info->costScript.GetData(), info->id, info->name.GetData());
        return;
    }

    // Get the output
    yCost = yCostVar->GetRoundValue();
    zCost = zCostVar->GetRoundValue();

    //calculate the next level costs. Used by the CalculateAddExperience.
    env.Define("SkillRank",      rank.Base()+1);
    script->Evaluate(&env);

    yCostNext = yCostVar->GetRoundValue();
    zCostNext = zCostVar->GetRoundValue();

    /*
        // Make sure the y values is clamped to the cost.  Otherwise Practice may always
        // fail.
        if  (y > yCost)
        {
            dirtyFlag = true;
            y = yCost;
        }
        if ( z > zCost )
        {
            dirtyFlag = true;
            z = zCost;
        }
    */
}

void Skill::Train(int yIncrease)
{
    if(y < yCost)
    {
        y+=yIncrease;
        dirtyFlag = true; // Mark for DB update
    }
}


bool Skill::CheckDoRank(psCharacter* user)
{
    if(y >= yCost && z >= zCost)
    {
        rank.SetBase(rank.Base()+1);
        z = 0;
        y = 0;
        // Reset the costs for Y/Z
        CalculateCosts(user);
        return true;
    }
    return false;
}

bool Skill::Practice(unsigned int amount, unsigned int &actuallyAdded, psCharacter* user)
{
    bool rankup = false;

    // never allow practice above max skill level
    if ((info->id <= 45 || info->id >= 52) && rank.Base() >= psserver->GetProgressionManager()->progressionMaxSkillValue)
        return false;

    // never allow practice above max stat level
    if ((info->id > 45 || info->id < 52) && rank.Base() >= psserver->GetProgressionManager()->progressionMaxStatValue)
        return false;

    // if the server is setup to have Progression without training OR
    // If the current knowledge level (Y) is at max level
    // then practice can take place
    if(y >= yCost || !psserver->GetProgressionManager()->progressionRequiresTraining)
    {
        z+=amount;
        rankup = CheckDoRank(user);
        if(rankup)
        {
            actuallyAdded = -zCost;
        }
        else
        {
            actuallyAdded = amount;
        }
    }
    else
    {
        actuallyAdded = 0;
    }
    dirtyFlag = true;  // Mark for DB update
    return rankup;
}

void psCharacter::SetSkillRank(PSSKILL which, unsigned int rank)
{
    if(rank < 0)
        rank = 0;

    skills.SetSkillRank(which, rank);
    skills.SetSkillKnowledge(which,0);
    skills.SetSkillPractice(which,0);
}

unsigned int psCharacter::GetCharLevel(bool physical)
{
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("Physical", (physical ? 1 : 0));

    (void) psserver->GetCacheManager()->GetCharLevelGet()->Evaluate(&env);

    return env.Lookup("Result")->GetRoundValue();
}

//This function recalculates Hp, Mana, and Stamina when needed (char creation, combats, training sessions)
void psCharacter::RecalculateStats()
{
    MathEnvironment env; // safe enough to reuse...and faster...
    env.Define("Actor", this);

    if(overrideMaxMana)
    {
        GetMaxMana().SetBase(overrideMaxMana);
    }
    else
    {
        (void) psserver->GetCacheManager()->GetMaxManaScript()->Evaluate(&env);
        MathVar* maxMana = env.Lookup("MaxMana");
        if(maxMana)
        {
            GetMaxMana().SetBase(maxMana->GetValue());
        }
        else
        {
            Error1("Failed to evaluate MathScript >CalculateMaxMana<.");
        }
    }

    if(overrideMaxHp)
    {
        GetMaxHP().SetBase(overrideMaxHp);
    }
    else
    {
        (void) psserver->GetCacheManager()->GetMaxHPScript()->Evaluate(&env);
        MathVar* maxHP = env.Lookup("MaxHP");
        GetMaxHP().SetBase(maxHP->GetValue());
    }

    // The max weight that a player can carry
    inventory.CalculateLimits();

    // Stamina
    CalculateMaxStamina();
}

size_t psCharacter::GetAssignedGMEvents(psGMEventListMessage &gmeventsMsg, int clientnum)
{
    // GM events consist of events ran by the GM (running & completed) and
    // participated in (running & completed).
    size_t numberOfEvents = 0;
    csString gmeventsStr, event, name, desc;
    gmeventsStr.Append("<gmevents>");

    // XML: <event><name text><role text><status text><id text></event>
    if(assignedEvents.runningEventIDAsGM >= 0)
    {
        psserver->GetGMEventManager()->GetGMEventDetailsByID(assignedEvents.runningEventIDAsGM,
                name,
                desc);
        event.Format("<event><name text=\"%s\" /><role text=\"*\" /><status text=\"R\" /><id text=\"%d\" /></event>",
                     name.GetData(), assignedEvents.runningEventIDAsGM);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }
    if(assignedEvents.runningEventID >= 0)
    {
        psserver->GetGMEventManager()->GetGMEventDetailsByID(assignedEvents.runningEventID,
                name,
                desc);
        event.Format("<event><name text=\"%s\" /><role text=\" \" /><status text=\"R\" /><id text=\"%d\" /></event>",
                     name.GetData(), assignedEvents.runningEventID);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }

    csArray<int>::Iterator iter = assignedEvents.completedEventIDsAsGM.GetIterator();
    while(iter.HasNext())
    {
        int gmEventIDAsGM = iter.Next();
        psserver->GetGMEventManager()->GetGMEventDetailsByID(gmEventIDAsGM,
                name,
                desc);
        event.Format("<event><name text=\"%s\" /><role text=\"*\" /><status text=\"C\" /><id text=\"%d\" /></event>",
                     name.GetData(), gmEventIDAsGM);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }

    csArray<int>::Iterator iter2 = assignedEvents.completedEventIDs.GetIterator();
    while(iter2.HasNext())
    {
        int gmEventID = iter2.Next();
        psserver->GetGMEventManager()->GetGMEventDetailsByID(gmEventID,
                name,
                desc);
        event.Format("<event><name text=\"%s\" /><role text=\" \" /><status text=\"C\" /><id text=\"%d\" /></event>",
                     name.GetData(), gmEventID);
        gmeventsStr.Append(event);
        numberOfEvents++;
    }

    gmeventsStr.Append("</gmevents>");

    if(numberOfEvents)
        gmeventsMsg.Populate(gmeventsStr, clientnum);

    return numberOfEvents;
}

void psCharacter::AssignGMEvent(int id, bool playerIsGM)
{
    if(playerIsGM)
        assignedEvents.runningEventIDAsGM = id;
    else
        assignedEvents.runningEventID = id;
}

void psCharacter::CompleteGMEvent(bool playerIsGM)
{
    if(playerIsGM)
    {
        assignedEvents.completedEventIDsAsGM.Push(assignedEvents.runningEventIDAsGM);
        assignedEvents.runningEventIDAsGM = -1;
    }
    else
    {
        assignedEvents.completedEventIDs.Push(assignedEvents.runningEventID);
        assignedEvents.runningEventID = -1;
    }
}

void psCharacter::RemoveGMEvent(int id, bool playerIsGM)
{
    if(playerIsGM)
    {
        if(assignedEvents.runningEventIDAsGM == id)
            assignedEvents.runningEventIDAsGM = -1;
        else
            assignedEvents.completedEventIDsAsGM.Delete(id);
    }
    else
    {
        if(assignedEvents.runningEventID == id)
            assignedEvents.runningEventID = -1;
        else
            assignedEvents.completedEventIDs.Delete(id);
    }
}


bool psCharacter::UpdateFaction(Faction* faction, int delta)
{
    if(!GetActor())
    {
        return false;
    }

    GetFactions()->UpdateFactionStanding(faction->id,delta);
    if(delta > 0)
    {
        psserver->SendSystemInfo(GetActor()->GetClientID(),"Your faction with %s has improved.",faction->name.GetData());
    }
    else
    {
        psserver->SendSystemInfo(GetActor()->GetClientID(),"Your faction with %s has worsened.",faction->name.GetData());
    }


    psFactionMessage factUpdate(GetActor()->GetClientID(), psFactionMessage::MSG_UPDATE);
    int standing;
    float weight;
    GetFactions()->GetFactionStanding(faction->id, standing ,weight);

    factUpdate.AddFaction(faction->name, standing);
    factUpdate.BuildMsg();
    factUpdate.SendMessage();

    return true;
}

bool psCharacter::CheckFaction(Faction* faction, int value)
{
    if(!GetActor()) return false;

    return GetFactions()->CheckFaction(faction,value);
}

const char* psCharacter::GetDescription()
{
    return description.GetDataSafe();
}

void psCharacter::SetDescription(const char* newValue)
{
    description = newValue;
    bool bChanged = false;
    while(description.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        description.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if(bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! Description trimmed.");
}

const char* psCharacter::GetOOCDescription()
{
    return oocdescription.GetDataSafe();
}

void psCharacter::SetOOCDescription(const char* newValue)
{
    oocdescription = newValue;
    bool bChanged = false;
    while(description.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        description.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if(bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! Description trimmed.");
}

//returns the stored char creation info of the player
const char* psCharacter::GetCreationInfo()
{
    return creationinfo.GetDataSafe();
}

void psCharacter::SetCreationInfo(const char* newValue)
{
    creationinfo = newValue;
    bool bChanged = false;
    while(creationinfo.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        creationinfo.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if(bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! creation info trimmed.");
}

//Generates and returns the dynamic life events from factions
bool psCharacter::GetFactionEventsDescription(csString &factionDescription)
{
    //iterates all the various factions in this character
    csHash<FactionStanding*, int>::GlobalIterator iter(GetFactions()->GetStandings().GetIterator());
    while(iter.HasNext())
    {
        FactionStanding* standing = iter.Next();
        int score = 0; //used to store the current score
        if(standing->score >= 0) //positive factions
        {
            csArray<FactionLifeEvent>::ReverseIterator scoreIter = standing->faction->PositiveFactionEvents.GetReverseIterator();
            score = standing->score;
            while(scoreIter.HasNext())
            {
                FactionLifeEvent &lifevt = scoreIter.Next();
                if(score > lifevt.value) //check if the score is enough to attribuite this life event
                {
                    factionDescription += lifevt.event_description + "\n"; //add the life event to the description
                    break; //nothing else to do as we found what we needed so bail out
                }
            }
        }
        else //negative factions
        {
            csArray<FactionLifeEvent>::ReverseIterator scoreIter = standing->faction->NegativeFactionEvents.GetReverseIterator();
            score = abs(standing->score); //we store values as positive to make things easier and faster so take the
            //absolute value
            while(scoreIter.HasNext())
            {
                FactionLifeEvent &lifevt = scoreIter.Next();
                if(score > lifevt.value)
                {
                    factionDescription += lifevt.event_description + "\n";
                    break;
                }
            }
        }
    }
    return (factionDescription.Length() > 0); //if the string contains something it means some events were attribuited
}

//returns the stored custom life event info of the player
const char* psCharacter::GetLifeDescription()
{
    return lifedescription.GetDataSafe();
}



void psCharacter::SetLifeDescription(const char* newValue)
{
    lifedescription = newValue;
    bool bChanged = false;
    while(lifedescription.Find("\n\n\n\n") != (size_t)-1)
    {
        bChanged = true;
        lifedescription.ReplaceAll("\n\n\n\n", "\n\n\n");
    }

    if(bChanged && GetActor() && GetActor()->GetClient())
        psserver->SendSystemError(GetActor()->GetClient()->GetClientNum(), "Warning! custom life events trimmed.");
}

//TODO: Make this not return a temp csString, but fix in place
csString NormalizeCharacterName(const csString &name)
{
    csString normName = name;
    normName.Downcase();
    normName.Trim();
    if(normName.Length() > 0)
        normName.SetAt(0,toupper(normName.GetAt(0)));
    return normName;
}

void SkillStatBuffable::OnChange()
{
    chr->RecalculateStats();
}

void CharStat::SetBase(int x)
{
    SkillStatBuffable::SetBase(x);
    chr->Skills().Calculate();
}

StatSet::StatSet(psCharacter* self) : CharacterAttribute(self)
{
    for(int i = 0; i < PSITEMSTATS_STAT_COUNT; i++)
    {
        stats[i].Initialize(self);
    }
}

CharStat &StatSet::Get(PSITEMSTATS_STAT which)
{
    CS_ASSERT(which >= 0 && which < PSITEMSTATS_STAT_COUNT);
    return stats[which];
}

SkillSet::SkillSet(psCharacter* self) : CharacterAttribute(self)
{
    for(size_t i = 0; i < psserver->GetCacheManager()->GetSkillAmount(); i++)
    {
        //generate a new skill
        Skill mySkill;
        //initialize the new skill
        mySkill.Clear();
        mySkill.rank.Initialize(self);
        //push the new skill in the skillset
        skills.Push(mySkill);
    }
}

int SkillSet::AddSkillPractice(psSkillInfo* skillInfo, unsigned int val)
{
    unsigned int added;
    bool rankUp;
    csString name;

    if(self->GetActor()->GetClientID() == 0)
        return 0;

    PSSKILL skill = skillInfo->id;

    rankUp = AddToSkillPractice(skill, val, added);

    // Save skill and practice only when the level reached a new rank
    // this is done to avoid saving to db each time a player hits
    // an opponent
    if(rankUp)
    {
        psServer::CharacterLoader.UpdateCharacterSkill(self->GetPID(),
                skill,
                GetSkillPractice((PSSKILL)skill),
                GetSkillKnowledge((PSSKILL)skill),
                GetSkillRank((PSSKILL)skill).Base()
                                                      );
    }

    name = skillInfo->name;
    Debug5(LOG_SKILLXP,self->GetActor()->GetClientID(),"Adding %d points to skill %s to character %s (%d)\n",val,skillInfo->name.GetData(),
           self->GetCharFullName(),
           self->GetActor()->GetClientID());
    if(added > 0)
    {
        psZPointsGainedEvent event(self->GetActor(), skillInfo->name, added, rankUp);
        event.FireEvent();
    }

    return added;
}

int SkillSet::AddSkillPractice(PSSKILL skill, unsigned int val)
{
    psSkillInfo* skillInfo = psserver->GetCacheManager()->GetSkillByID(skill);

    if(skillInfo)
    {
        return AddSkillPractice(skillInfo, val);
    }
    else
    {
        Debug4(LOG_SKILLXP,self->GetActor()->GetClientID(),"WARNING! Skill practise to unknown skill(%d) for character %s (%d)\n",
               (int)skill,
               self->GetCharFullName(),
               self->GetActor()->GetClientID());
    }

    return 0;
}

unsigned int SkillSet::GetBestSkillSlot(bool withBuffer)
{
    unsigned int max = 0;
    unsigned int i = 0;
    for(; i<psserver->GetCacheManager()->GetSkillAmount(); i++)
    {
        unsigned int rank = withBuffer ? skills[i].rank.Current() : skills[i].rank.Base();
        if(rank > max)
            max = rank;
    }

    if(i == psserver->GetCacheManager()->GetSkillAmount())
        return (unsigned int)~0;
    else
        return i;
}

void SkillSet::Calculate()
{
    for(size_t z = 0; z < psserver->GetCacheManager()->GetSkillAmount(); z++)
    {
        skills[z].CalculateCosts(self);
    }
}

bool SkillSet::CanTrain(PSSKILL skill)
{
    if(skill<0 || skill>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return false;
    else
    {
        return skills[skill].CanTrain();
    }
}

void SkillSet::CheckDoRank(PSSKILL skill)
{

    if(skill<0 ||skill>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return;
    else
    {
        skills[skill].CheckDoRank(self);
    }
}

void SkillSet::Train(PSSKILL skill, int yIncrease)
{

    if(skill<0 ||skill>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return;
    else
    {
        skills[skill].Train(yIncrease);
    }
}


void SkillSet::SetSkillInfo(PSSKILL which, psSkillInfo* info, bool recalculatestats)
{
    if(which<0 || which>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return;
    else
    {
        skills[which].info = info;
        skills[which].CalculateCosts(self);
    }

    if(recalculatestats)
        self->RecalculateStats();
}

void SkillSet::SetSkillRank(PSSKILL which, unsigned int rank, bool recalculatestats)
{
    if(which < 0 || which >= (PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return;

    // Clamp rank to stay within sane values, even if given something totally outrageous.
    if(rank < 0)
    {
        rank = 0;
    }
    else if(rank > SKILL_MAX_RANK)
    {
        rank = SKILL_MAX_RANK;
    }

    skills[which].rank.SetBase(rank);
    skills[which].CalculateCosts(self);
    skills[which].dirtyFlag = true;  // Mark for DB update

    if(recalculatestats)
    {
        self->RecalculateStats();
    }
}

void SkillSet::SetSkillKnowledge(PSSKILL which, int y_value)
{
    if(which<0 || which>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return;
    if(y_value < 0)
        y_value = 0;
    skills[which].y = y_value;
    skills[which].dirtyFlag = true;   // Mark for DB update
}


void SkillSet::SetSkillPractice(PSSKILL which,int z_value)
{
    if(which<0 || which>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return;
    if(z_value < 0)
        z_value = 0;

    skills[which].z = z_value;
    skills[which].dirtyFlag = true;   // Mark for DB update
}


bool SkillSet::AddToSkillPractice(PSSKILL skill, unsigned int val, unsigned int &added)
{
    if(skill<0 || skill>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
    {
        added = 0;
        return 0;
    }

    bool rankup = false;
    rankup = skills[skill].Practice(val, added, self);
    return rankup;
}


unsigned int SkillSet::GetSkillPractice(PSSKILL skill)
{

    if(skill<0 || skill>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return 0;
    return skills[skill].z;
}


unsigned int SkillSet::GetSkillKnowledge(PSSKILL skill)
{

    if(skill<0 || skill>=(PSSKILL)psserver->GetCacheManager()->GetSkillAmount())
        return 0;

    // if Allow Practice Without Training is enabled, always return the actual Knowledge to 
    // be equal as the cost
    if(!psserver->GetProgressionManager()->progressionRequiresTraining && (skill <= 45 || skill >= 52))
        return skills[skill].yCost;
    else
        return skills[skill].y;
}

SkillRank &SkillSet::GetSkillRank(PSSKILL skill)
{
    CS_ASSERT(skill >= 0 && skill < (PSSKILL)psserver->GetCacheManager()->GetSkillAmount());
    return skills[skill].rank;
}

Skill &SkillSet::Get(PSSKILL skill)
{
    CS_ASSERT(skill >= 0 && skill < (PSSKILL)psserver->GetCacheManager()->GetSkillAmount());

    // if Allow Practice Without Training is enabled, always return the actual Knowledge to 
    // be equal as the cost (excluding stats: int, end, agi, ...)
    if(!psserver->GetProgressionManager()->progressionRequiresTraining && (skill <= 45 || skill >= 52))
        skills[skill].y = skills[skill].yCost;

    return skills[skill];
}

//////////////////////////////////////////////////////////////////////////
// BuddyManager
//////////////////////////////////////////////////////////////////////////
bool psBuddyManager::AddBuddy(PID buddyID, csString &buddyName)
{
    // Cannot addself to buddy list
    if(buddyID == characterId)
        return false;

    for(size_t x = 0; x < buddyList.GetSize(); x++)
    {
        if(buddyList[x].playerId == buddyID)
        {
            return true;
        }
    }

    Buddy b;
    b.name      = buddyName;
    b.playerId  = buddyID;

    buddyList.Push(b);
    return true;
}


void psBuddyManager::RemoveBuddy(PID buddyID)
{
    for(size_t x = 0; x < buddyList.GetSize(); x++)
    {
        if(buddyList[x].playerId == buddyID)
        {
            buddyList.DeleteIndex(x);
            return;
        }
    }
}


void psBuddyManager::AddBuddyOf(PID buddyID)
{
    if(buddyOfList.Find(buddyID)  == csArrayItemNotFound)
    {
        buddyOfList.Push(buddyID);
    }
}


void psBuddyManager::RemoveBuddyOf(PID buddyID)
{
    buddyOfList.Delete(buddyID);
}


bool psBuddyManager::LoadBuddies(Result &myBuddies, Result &buddyOf)
{
    unsigned int x;

    if(!myBuddies.IsValid())
        return true;

    for(x = 0; x < myBuddies.Count(); x++)
    {

        if(strcmp(myBuddies[x][ "relationship_type" ], "buddy") == 0)
        {
            Buddy newBud;
            newBud.name = myBuddies[x][ "buddy_name" ];
            newBud.playerId = PID(myBuddies[x].GetUInt32("related_id"));

            buddyList.Insert(0, newBud);
        }
    }

    // Load all the people that I am a buddy of. This is used to inform these people
    // of when I log in/out.
    for(x = 0; x < buddyOf.Count(); x++)
    {
        if(strcmp(buddyOf[x][ "relationship_type" ], "buddy") == 0)
        {
            buddyOfList.Insert(0, buddyOf[x].GetUInt32("character_id"));
        }
    }

    return true;
}


bool psBuddyManager::IsBuddy(PID buddyID)
{
    for(size_t x = 0; x < buddyList.GetSize(); x++)
    {
        if(buddyList[x].playerId == buddyID)
        {
            return true;
        }
    }
    return false;
}


