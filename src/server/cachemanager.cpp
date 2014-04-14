/*
 * CacheManager.cpp
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
#include <zlib.h>
#include <csutil/stringarray.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "util/log.h"
#include "util/stringarray.h"
#include "util/serverconsole.h"
#include "util/eventmanager.h"
#include "util/psdatabase.h"
#include "util/strutil.h"

#include "net/message.h"

#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pstrade.h"
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psquest.h"
#include "bulkobjects/psattack.h"
#include "bulkobjects/psmerchantinfo.h"
#include "bulkobjects/psspell.h"
#include "bulkobjects/psglyph.h"
#include "bulkobjects/pstrait.h"


#include "rpgrules/factions.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "cachemanager.h"
#include "commandmanager.h"
#include "questmanager.h"
#include "combatmanager.h"
#include "client.h"
#include "globals.h"
#include "scripting.h"

CacheManager::CacheManager()
{
    slotMap[PSCHARACTER_SLOT_RIGHTHAND]   = PSITEMSTATS_SLOT_RIGHTHAND;
    slotMap[PSCHARACTER_SLOT_LEFTHAND]    = PSITEMSTATS_SLOT_LEFTHAND;
    slotMap[PSCHARACTER_SLOT_BOTHHANDS]   = PSITEMSTATS_SLOT_BOTHHANDS;
    slotMap[PSCHARACTER_SLOT_HELM]        = PSITEMSTATS_SLOT_HELM;
    slotMap[PSCHARACTER_SLOT_RIGHTFINGER] = PSITEMSTATS_SLOT_RIGHTFINGER;
    slotMap[PSCHARACTER_SLOT_LEFTFINGER]  = PSITEMSTATS_SLOT_LEFTFINGER;
    slotMap[PSCHARACTER_SLOT_NECK]        = PSITEMSTATS_SLOT_NECK;
    slotMap[PSCHARACTER_SLOT_BOOTS]       = PSITEMSTATS_SLOT_BOOTS;
    slotMap[PSCHARACTER_SLOT_BACK]        = PSITEMSTATS_SLOT_BACK;
    slotMap[PSCHARACTER_SLOT_ARMS]        = PSITEMSTATS_SLOT_ARMS;
    slotMap[PSCHARACTER_SLOT_GLOVES]      = PSITEMSTATS_SLOT_GLOVES;
    slotMap[PSCHARACTER_SLOT_LEGS]        = PSITEMSTATS_SLOT_LEGS;
    slotMap[PSCHARACTER_SLOT_BELT]        = PSITEMSTATS_SLOT_BELT;
    slotMap[PSCHARACTER_SLOT_BRACERS]     = PSITEMSTATS_SLOT_BRACERS;
    slotMap[PSCHARACTER_SLOT_TORSO]       = PSITEMSTATS_SLOT_TORSO;
    slotMap[PSCHARACTER_SLOT_MIND]        = PSITEMSTATS_SLOT_MIND;

    psItemStatFlags statflag("MELEEWEAPON",  PSITEMSTATS_FLAG_IS_A_MELEE_WEAPON);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("ARMOR",        PSITEMSTATS_FLAG_IS_ARMOR);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("RANGEWEAPON",  PSITEMSTATS_FLAG_IS_A_RANGED_WEAPON);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("SHIELD",       PSITEMSTATS_FLAG_IS_A_SHIELD);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("AMMO",         PSITEMSTATS_FLAG_IS_AMMO);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("CONTAINER",    PSITEMSTATS_FLAG_IS_A_CONTAINER);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("TRAP",         PSITEMSTATS_FLAG_IS_A_TRAP);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("CONSTRUCTIBLE", PSITEMSTATS_FLAG_IS_CONSTRUCTIBLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("CANTRANSFORM", PSITEMSTATS_FLAG_CAN_TRANSFORM);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("NOPICKUP", PSITEMSTATS_FLAG_NOPICKUP);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("USESAMMO",     PSITEMSTATS_FLAG_USES_AMMO);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("STACKABLE",    PSITEMSTATS_FLAG_IS_STACKABLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("EQUIP_STACKABLE",    PSITEMSTATS_FLAG_IS_EQUIP_STACKABLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("GLYPH",        PSITEMSTATS_FLAG_IS_GLYPH);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("TRIA",         PSITEMSTATS_FLAG_TRIA);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("HEXA",         PSITEMSTATS_FLAG_HEXA);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("OCTA",         PSITEMSTATS_FLAG_OCTA);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("CIRCLE",       PSITEMSTATS_FLAG_CIRCLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("CONSUMABLE",   PSITEMSTATS_FLAG_CONSUMABLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("READABLE",     PSITEMSTATS_FLAG_IS_READABLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("WRITEABLE",    PSITEMSTATS_FLAG_IS_WRITEABLE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("CREATIVE",     PSITEMSTATS_FLAG_CREATIVE);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("BUY_PERSONALISE",PSITEMSTATS_FLAG_BUY_PERSONALISE);
    ItemStatFlagArray.Push(statflag);

    // Flag that allows an item to be stacked if qualities are different
    // and the resulting stack will have the new average.
    statflag.Set("AVERAGEQUALITY", PSITEMSTATS_FLAG_AVERAGEQUALITY);
    ItemStatFlagArray.Push(statflag);
    statflag.Set("RECHARGEABLE", PSITEMSTATS_FLAG_IS_RECHARGEABLE);
    ItemStatFlagArray.Push(statflag);

    statflag.Set("END",0);
    ItemStatFlagArray.Push(statflag);

    effectID = 0;

    commandManager = NULL;

    lootRandomizer = new LootRandomizer(this);

    // Init common string data.
    compressed_msg_strings = 0;
    compressed_msg_strings_size = 0;
    num_compressed_strings = 0;
    AddCommonStringID("");
}

CacheManager::~CacheManager()
{
    UnloadAll();
}


bool CacheManager::PreloadAll(EntityManager* entitymanager)
{
    if(!PreloadSectors())
        return false;
    if(!PreloadSkills())
        return false;
    if(!PreloadLimitations())
        return false;
    if(!PreloadRaceInfo())
        return false;
    if(!PreloadTraits())
        return false; // Need RaceInfo
    if(!PreloadWeaponTypes())
        return false;
    if (!PreloadItemCategories())
        return false;
    if(!PreloadItemAnimList())
        return false;
    if(!PreloadItemStatsDatabase())
        return false;
    if(!PreloadWays())
        return false;
    if(!PreloadFactions())
        return false;
    if(!PreloadScripts(entitymanager))
        return false;
    if(!PreloadMathScripts())
        return false;
    if(!PreloadSpells())
        return false;
    if(!PreloadQuests())
        return false;
    if(!PreloadAttackTypes())
        return false;
    if (!PreloadAttacks())
        return false;
    if (!PreloadTradeCombinations())
        return false;
    if(!PreloadTradeTransformations())
        return false;
    if(!PreloadUniqueTradeTransformations())
        return false;
    if(!PreloadTradeProcesses())
        return false;
    if(!PreloadTradePatterns())
        return false;
    if(!PreloadCraftMessages())
        return false;
    if(!PreloadTips())
        return false;
    if(!PreloadBadNames())
        return false;
    if(!PreloadArmorVsWeapon())
        return false;
    if(!PreloadMovement())
        return false;
    if(!PreloadStances())
        return false;
    if(!PreloadOptions())
        return false;
    if(!PreloadLootModifiers())
        return false;

    PreloadCommandGroups();

    return true;
}

void CacheManager::PreloadCommandGroups()
{
    commandManager = new psCommandManager;
    commandManager->LoadFromDatabase();
}

void CacheManager::UnloadAll()
{

    delete commandManager;

    {
        csHash<psQuest*>::GlobalIterator it(quests_by_id.GetIterator());
        while(it.HasNext())
        {
            psQuest* newQuest = it.Next();
            delete newQuest;
        }
        quests_by_id.Empty();
    }
    {
        csHash<psAttack*>::GlobalIterator it(attacks_by_id.GetIterator ());
        while (it.HasNext ())
        {
            psAttack* newAttack = it.Next ();
            delete newAttack;
        }
        attacks_by_id.Empty();
    }
    {
        csHash<csPDelArray<CombinationConstruction>*,uint32>::GlobalIterator it(tradeCombinations_IDHash.GetIterator ());

        while(it.HasNext())
        {
            csPDelArray<CombinationConstruction>* newArray = it.Next();
            delete newArray;
        }
        tradeCombinations_IDHash.Empty();
    }

    {
        csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *, uint32>::GlobalIterator it(tradeTransformations_IDHash.GetIterator());
        while(it.HasNext())
        {
            csHash<csPDelArray<psTradeTransformations> *,uint32>* transHash = it.Next();
            csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator it2(transHash->GetIterator());
            while(it2.HasNext())
            {
                csPDelArray<psTradeTransformations>* newArray = it2.Next();
                delete newArray;
            }
            transHash->Empty();
            delete transHash;
        }
        tradeTransformations_IDHash.Empty();
    }

    {
        csHash<csArray<uint32>*,uint32>::GlobalIterator it(tradeTransUnique_IDHash.GetIterator());
        while(it.HasNext())
        {
            csArray<uint32>* newArray = it.Next();
            delete newArray;
        }
        tradeTransUnique_IDHash.Empty();
    }

    {
        csHash<csArray<psTradeProcesses*>*,uint32>::GlobalIterator it(tradeProcesses_IDHash.GetIterator());
        while(it.HasNext())
        {
            csArray<psTradeProcesses*>* newArray = it.Next();
            csArray<psTradeProcesses*>::Iterator it2(newArray->GetIterator());
            while(it2.HasNext())
            {
                psTradeProcesses* newProc = it2.Next();
                delete newProc;
            }
            delete newArray;
        }
        tradeProcesses_IDHash.Empty();
    }

    {
        csHash<psTradePatterns*,uint32>::GlobalIterator it(tradePatterns_IDHash.GetIterator());
        while(it.HasNext())
        {
            psTradePatterns* pattern  = it.Next();
            delete pattern;
        }
        tradePatterns_IDHash.Empty();
    }

    {
        csHash<csArray<CraftTransInfo*>*,uint32>::GlobalIterator it(tradeCraftTransInfo_IDHash.GetIterator());
        while(it.HasNext())
        {
            csArray<CraftTransInfo*>* newArray = it.Next();
            csArray<CraftTransInfo*>::Iterator it2(newArray->GetIterator());
            while(it2.HasNext())
            {
                CraftTransInfo* newCraftTransInfo = it2.Next();
                delete newCraftTransInfo;
            }
            delete newArray;
        }
        tradeCraftTransInfo_IDHash.Empty();
    }

    {
        csHash<csArray<CraftComboInfo*>*,uint32>::GlobalIterator it(tradeCraftComboInfo_IDHash.GetIterator());
        while(it.HasNext())
        {
            csArray<CraftComboInfo*>* newComboArray = it.Next();
            csArray<CraftComboInfo*>::Iterator it2(newComboArray->GetIterator());
            while(it2.HasNext())
            {
                CraftComboInfo* newCombo = it2.Next();
                csArray<CraftSkills*>::Iterator it3(newCombo->skillArray->GetIterator());
                while(it3.HasNext())
                {
                    CraftSkills* newCraftSkill = it3.Next();
                    delete newCraftSkill;
                }
                delete newCombo->skillArray;
                delete newCombo;
            }
            delete newComboArray;
        }
        tradeCraftComboInfo_IDHash.Empty();
    }

    {
        csHash<psItemStats*, csString>::GlobalIterator it(itemStats_NameHash.GetIterator());
        while(it.HasNext())
            delete it.Next();
    }

    {
        csHash<psSectorInfo*>::GlobalIterator it(sectorinfo_by_id.GetIterator());
        while(it.HasNext())
            delete it.Next();
    }

    {
        csHash<ProgressionScript*,csString>::GlobalIterator it(scripts.GetIterator());
        while(it.HasNext())
            delete it.Next();
    }

    {
        csHash<CachedObject*, csString>::GlobalIterator it(generic_object_cache.GetIterator());
        while(it.HasNext())
        {
            CachedObject* newCachedObject = it.Next();
            newCachedObject->event->CancelEvent();
            newCachedObject->object->ProcessCacheTimeout();
            newCachedObject->object->DeleteSelf();
            delete newCachedObject;
        }
    }

    {
        csHash<Faction*, int>::GlobalIterator it(factions_by_id.GetIterator());
        while(it.HasNext())
        {
            Faction* newFaction = it.Next();
            newFaction->PositiveFactionEvents.DeleteAll();
            newFaction->NegativeFactionEvents.DeleteAll();
            delete newFaction;
        }
    }

    {
        csHash<psSkillInfo*, int>::GlobalIterator it(skillinfo_IDHash.GetIterator());
        while(it.HasNext())
            delete it.Next();
        skillinfo_IDHash.DeleteAll();
        skillinfo_NameHash.DeleteAll();
        skillinfo_CategoryHash.DeleteAll();
    }

    {
        csHash<psGuildAlliance*>::GlobalIterator it(alliance_by_id.GetIterator());
        while(it.HasNext())
        {
            psGuildAlliance* alliance = it.Next();
            delete alliance;
        }
        alliance_by_id.Empty();
    }

    {
        csHash<psGuildInfo*>::GlobalIterator it(guildinfo_by_id.GetIterator());
        while(it.HasNext())
        {
            psGuildInfo* ginfo = it.Next();
            delete ginfo;
        }
        guildinfo_by_id.Empty();
    }

    delete lootRandomizer;
    delete [] compressed_msg_strings;
    // ToDo: unload everything else
}

void CacheManager::RemoveInstance(psItem* &item)
{
    Notify2(LOG_CACHE, "Removing Instance of item: %u", item->GetUID());
    if(item->GetUID() != 0)
    {
        db->Command("DELETE from item_instances where id='%u'", item->GetUID());
    }
    delete item;
    item = NULL;
}

void CacheManager::RemoveItemStats(psItemStats* &itemStats)
{
    if(itemStats && itemStats->GetUID() != 0)
    {
        itemStats_NameHash.Delete(itemStats->GetDownCaseName(), itemStats);
        itemStats_IDHash.Delete(itemStats->GetUID(), itemStats);

        db->Command("DELETE from item_stats where id='%u'", itemStats->GetUID());

        delete itemStats;
        itemStats = NULL;
    }
}

bool CacheManager::PreloadLootModifiers()
{
    // Order by's are a little slower but it guarentees order
    Result result(db->Select("SELECT * FROM loot_modifiers ORDER BY modifier_type, probability"));
    if(!result.IsValid())
    {
        Error2("Could not load loot modifiers due to database error: %s\n", db->GetLastError());
        return false;
    }

    float previous_probability = 0;
    for(unsigned int i = 0; i < result.Count(); i++)
    {
        LootModifier* entry = new LootModifier;

        entry->id = result[i].GetInt("id");
        entry->modifier_type = result[i][ "modifier_type" ];
        entry->name = result[i][ "name" ];
        entry->effect = result[i][ "effect" ];
        entry->equip_script = result[i]["equip_script"];
        entry->effect = result[i][ "effect" ];
        entry->probability = result[i].GetFloat("probability");
        entry->probabilityRange = result[i].GetFloat("probability")-previous_probability;
        entry->stat_req_modifier = result[i][ "stat_req_modifier" ];
        entry->cost_modifier = result[i].GetFloat("cost_modifier");
        entry->mesh = result[i][ "mesh" ];
        entry->icon = result[i][ "icon" ];
        entry->not_usable_with = result[i][ "not_usable_with" ];

        Result restrainResult(db->Select("SELECT * FROM loot_modifiers_restrains where loot_modifier_id=%i", entry->id));
        if(!restrainResult.IsValid())
        {
            Error2("Could not load loot modifiers restrains due to database error: %s\n", db->GetLastError());
            delete entry;
            return false;
        }

        for(size_t j = 0; j < restrainResult.Count(); j++)
        {
            entry->itemRestrain.PutUnique(restrainResult[j].GetUInt32("item_id"),
                                          *restrainResult[j]["allowed"] == 'Y');
        }

        lootRandomizer->AddLootModifier(entry);
        previous_probability = entry->probability;
    }

    Notify2(LOG_STARTUP, "%lu loot modifiers Loaded", result.Count());
    return true;
}

bool CacheManager::PreloadOptions()
{
    unsigned int currentrow;
    Result result(db->Select("SELECT * from server_options"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >server_options<");
        return false;
    }

    //check all rows and add the options to the tree
    for(currentrow = 0; currentrow < result.Count(); currentrow++)
        rootOptionEntry.setOption(result[currentrow]["option_name"], result[currentrow]["option_value"]);


    Notify2(LOG_STARTUP, "%lu server options Loaded", result.Count());
    return true;
}

bool optionEntry::setOption(const csString path, const csString value)
{
    csStringArray splittedOptName;
    //we split the options at : as we identify a path with it.
    //changing this line changes the path delimiter of our options
    splittedOptName.SplitString(path,":");
    //is the category/option we have at hand already indexed?
    if(!subOptions.Contains(splittedOptName.Get(0))) //if not add it
    {
        optionEntry entry;
        subOptions.Put(splittedOptName.Get(0),entry);
    }

    //get what we have or we have created above for working on it
    optionEntry* optEntry = subOptions.GetElementPointer(splittedOptName.Get(0));
    if(!optEntry)
        return false;

    //if this isn't the last entry in the path create another sub node
    if(splittedOptName.GetSize() > 1)
        return optEntry->setOption(path.Slice(csString(splittedOptName.Get(0)).Length()+1), value);

    //if this is the last entry in the path just assign the value directly and we are done
    optEntry->setValue(value);
    return true;
}

optionEntry* optionEntry::getOption(const csString path)
{
    //iterate the tree to find the option we want
    csStringArray splittedOptName;
    //we split the options at : as we identify a path with it.
    //changing this line changes the path delimiter of our options
    splittedOptName.SplitString(path,":");
    //is the cateogory/option indexed?
    if(!subOptions.Contains(splittedOptName.Get(0)))
        return NULL;

    //get the actual option entry
    optionEntry* optEntry = subOptions.GetElementPointer(splittedOptName.Get(0));
    if(!optEntry)
        return NULL;

    //is this the last entry?
    if(splittedOptName.GetSize() > 1)//if not then call the "subfolder"
        return optEntry->getOption(path.Slice(csString(splittedOptName.Get(0)).Length()+1));

    return optEntry;
}

optionEntry* optionEntry::getOptionSafe(const csString path, csString fallback)
{
    //try to get the option normally
    optionEntry* Entry = getOption(path);
    if(Entry) //we found it all done
        return Entry;

    //try to make a new entry and assign the fallback
    setOption(path, fallback);
    //return the option we have created.
    return getOption(path);
}

csVector3 optionEntry::getValueAsVector()
{
    //split the array each ,
    csStringArray cordArray;
    cordArray.SplitString(getValue(),",");
    //if there are enough string to do 3 cordinates parse them and stuff them in a csVector3
    if(cordArray.GetSize() > 2)
        return csVector3(atof(cordArray.Get(0)), atof(cordArray.Get(1)), atof(cordArray.Get(2)));
    //in case the data is bad just return 0,0,0
    Error2("Server option is not a valid vector (%s should be like x,y,z)", getValue().GetData());
    return csVector3(0,0,0);
}

bool optionEntry::getValueAsVector(csVector3 &vector)
{
    //split the array each ,
    csStringArray cordArray;
    cordArray.SplitString(getValue(),",");
    //if there are enough string to do 3 cordinates parse them and stuff them in a csVector3
    //else return false.
    if(cordArray.GetSize() < 3)
        return false;

    vector = csVector3(atof(cordArray.Get(0)), atof(cordArray.Get(1)), atof(cordArray.Get(2)));
    return true;
}

bool CacheManager::PreloadSkills()
{
    unsigned int currentrow;
    Result result(db->Select("SELECT * from skills"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >skills<");
        return false;
    }

    for(currentrow = 0; currentrow < result.Count(); currentrow++)
    {
        if(result[currentrow]["skill_id"]!=NULL)
        {
            psSkillInfo* newskill = new psSkillInfo;

            newskill->id = (PSSKILL) result[currentrow].GetInt("skill_id");
            newskill->name = result[currentrow]["name"];
            newskill->description = result[currentrow]["description"];
            newskill->practice_factor = result[currentrow].GetInt("practice_factor");
            newskill->mental_factor = result[currentrow].GetInt("mental_factor");
            newskill->price = psMoney(result[currentrow].GetInt("price"));
            newskill->baseCost = result[currentrow].GetInt("base_rank_cost");

            csString type(result[currentrow]["category"]);
            if(type == "STATS")
                newskill->category = PSSKILLS_CATEGORY_STATS;
            else if(type == "COMBAT")
                newskill->category = PSSKILLS_CATEGORY_COMBAT;
            else if(type == "MAGIC")
                newskill->category = PSSKILLS_CATEGORY_MAGIC;
            else if(type == "JOBS")
                newskill->category = PSSKILLS_CATEGORY_JOBS;
            else if(type == "VARIOUS")
                newskill->category = PSSKILLS_CATEGORY_VARIOUS;
            else
            {
                Error3("Unknown category '%s' for skill id %u",type.GetDataSafe(),newskill->id);
                delete newskill;
                return false;
            }

            newskill->costScript = result[currentrow]["cost_script"];

            //check if there is a valid cost_script
            if(!psserver->GetMathScriptEngine()->FindScript(newskill->costScript))
            {
                Error3("Unknown script '%s' for skill id %u", newskill->costScript.GetData(), newskill->id);
                delete newskill;
                return false;
            }

            skillinfo_IDHash.Put((int)newskill->id, newskill);
            skillinfo_NameHash.Put(csString(newskill->name).Upcase(), newskill);
            skillinfo_CategoryHash.Put((int)newskill->category, newskill);

            // Register the skill name in the msg_strings
            msg_strings.Request(newskill->name);
        }
    }
    Notify2(LOG_STARTUP, "%lu Skills Loaded", result.Count());
    return true;
}

bool CacheManager::PreloadLimitations()
{
    psCharacterLimitation* limit;
    unsigned int currentrow;
    Result result(db->Select("SELECT * from character_limitations"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >character_limitations<");
        return false;
    }

    for(currentrow=0; currentrow< result.Count(); currentrow++)
    {
        limit = new psCharacterLimitation;
        limit->id = result[currentrow].GetInt("id");
        limit->limit_type = result[currentrow]["limit_type"];
        limit->min_score  = result[currentrow].GetInt("player_score");
        limit->value      = result[currentrow]["value"];

        limits.Push(limit);
    }
    Notify2(LOG_STARTUP, "%lu Limitations Loaded", result.Count());
    return true;
}

const psCharacterLimitation* CacheManager::GetLimitation(size_t index)
{
    if(index >= limits.GetSize())
    {
        return NULL;
    }
    else
    {
        return limits[index];
    }
}

bool CacheManager::PreloadSectors()
{
    unsigned int currentrow;
    psSectorInfo* newsector;
    Result result(db->Select("SELECT * from sectors"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >sectors<");
        return false;
    }

    for(currentrow=0; currentrow< result.Count(); currentrow++)
    {
        newsector=new psSectorInfo;
        CS_ASSERT(newsector!=NULL);

        newsector->uid  = result[currentrow].GetInt("id");
        newsector->name = result[currentrow]["name"];

        //this thing is awful :|
        {
            psSectorInfo::weatherTypeData data;
            data.enabled = strcmp(result[currentrow]["rain_enabled"],"Y")==0;

            data.min_gap = result[currentrow].GetInt("rain_min_gap");
            data.max_gap = result[currentrow].GetInt("rain_max_gap");
            CS_ASSERT(data.min_gap <= data.max_gap);

            data.min_duration = result[currentrow].GetInt("rain_min_duration");
            data.max_duration = result[currentrow].GetInt("rain_max_duration");
            CS_ASSERT(data.min_duration <= data.max_duration);

            data.min_density = result[currentrow].GetInt("rain_min_drops");
            data.max_density = result[currentrow].GetInt("rain_max_drops");
            CS_ASSERT(data.min_density <= data.max_density);

            data.min_fade_in = result[currentrow].GetInt("rain_min_fade_in");
            data.max_fade_in = result[currentrow].GetInt("rain_max_fade_in");
            CS_ASSERT(data.min_fade_in <= data.max_fade_in);

            data.min_fade_out = result[currentrow].GetInt("rain_min_fade_out");
            data.max_fade_out = result[currentrow].GetInt("rain_max_fade_out");
            CS_ASSERT(data.min_fade_out <= data.max_fade_out);

            newsector->AddWeatherTypeData(data, (unsigned int) psWeatherMessage::RAIN);

        }

        {
            psSectorInfo::weatherTypeData data;
            data.enabled = strcmp(result[currentrow]["snow_enabled"],"Y")==0;

            data.min_gap = result[currentrow].GetInt("snow_min_gap");
            data.max_gap = result[currentrow].GetInt("snow_max_gap");
            CS_ASSERT(data.min_gap <= data.max_gap);

            data.min_duration = result[currentrow].GetInt("snow_min_duration");
            data.max_duration = result[currentrow].GetInt("snow_max_duration");
            CS_ASSERT(data.min_duration <= data.max_duration);

            data.min_density = result[currentrow].GetInt("snow_min_flakes");
            data.max_density = result[currentrow].GetInt("snow_max_flakes");
            CS_ASSERT(data.min_density <= data.max_density);

            data.min_fade_in = result[currentrow].GetInt("snow_min_fade_in");
            data.max_fade_in = result[currentrow].GetInt("snow_max_fade_in");
            CS_ASSERT(data.min_fade_in <= data.max_fade_in);

            data.min_fade_out = result[currentrow].GetInt("snow_min_fade_out");
            data.max_fade_out = result[currentrow].GetInt("snow_max_fade_out");
            CS_ASSERT(data.min_fade_out <= data.max_fade_out);

            newsector->AddWeatherTypeData(data, (unsigned int) psWeatherMessage::SNOW);
        }

        {
            psSectorInfo::weatherTypeData data;
            data.enabled = strcmp(result[currentrow]["fog_enabled"],"Y")==0;

            data.min_gap = result[currentrow].GetInt("fog_min_gap");
            data.max_gap = result[currentrow].GetInt("fog_max_gap");
            CS_ASSERT(data.min_gap <= data.max_gap);

            data.min_duration = result[currentrow].GetInt("fog_min_duration");
            data.max_duration = result[currentrow].GetInt("fog_max_duration");
            CS_ASSERT(data.min_duration <= data.max_duration);

            data.min_density = result[currentrow].GetInt("fog_min_density");
            data.max_density = result[currentrow].GetInt("fog_max_density");
            CS_ASSERT(data.min_density <= data.max_density);

            data.min_fade_in = result[currentrow].GetInt("fog_min_fade_in");
            data.max_fade_in = result[currentrow].GetInt("fog_max_fade_in");
            CS_ASSERT(data.min_fade_in <= data.max_fade_in);

            data.min_fade_out = result[currentrow].GetInt("fog_min_fade_out");
            data.max_fade_out = result[currentrow].GetInt("fog_max_fade_out");
            CS_ASSERT(data.min_fade_out <= data.max_fade_out);

            newsector->AddWeatherTypeData(data, (unsigned int) psWeatherMessage::FOG);
        }

        {
            psSectorInfo::weatherTypeData data;
            data.enabled = true;
            data.min_gap = result[currentrow].GetInt("lightning_min_gap");
            data.max_gap = result[currentrow].GetInt("lightning_max_gap");
            CS_ASSERT(data.min_gap <= data.max_gap);
            newsector->AddWeatherTypeData(data, (unsigned int) psWeatherMessage::LIGHTNING);

        }

        newsector->is_colliding = (result[currentrow].GetInt("collide_objects") != 0);
        newsector->is_non_transient = (result[currentrow].GetInt("non_transient_objects") != 0);

        newsector->is_teleporting = (*result[currentrow]["TeleportingSectorEnable"] != 'N');
        newsector->has_penalty = (*result[currentrow]["TeleportingPenaltyEnable"] != 'N');

        newsector->deathRestoreMana = (*result[currentrow]["DeathRestoreMana"] != 'N');
        newsector->deathRestoreHP = (*result[currentrow]["DeathRestoreHP"] != 'N');

        newsector->teleportingSector = result[currentrow]["TeleportingSector"];
        newsector->deathSector = result[currentrow]["DeathSector"];

        csStringArray teleportingcordarray;
        teleportingcordarray.SplitString(result[currentrow]["TeleportingCords"],",");
        if(teleportingcordarray.GetSize() > 3)
        {
            newsector->teleportingCords = csVector3(atof(teleportingcordarray.Get(0)),
                                                    atof(teleportingcordarray.Get(1)),
                                                    atof(teleportingcordarray.Get(2)));
            newsector->teleportingRot = atof(teleportingcordarray.Get(3));
        }

        csStringArray deathcordarray;
        deathcordarray.SplitString(result[currentrow]["DeathCords"],",");
        if(deathcordarray.GetSize() > 3)
        {
            newsector->deathCords = csVector3(atof(deathcordarray.Get(0)),
                                              atof(deathcordarray.Get(1)),
                                              atof(deathcordarray.Get(2)));
            newsector->deathRot = atof(deathcordarray.Get(3));
        }



        newsector->say_range = result[currentrow].GetFloat("say_range");

        newsector->god_name = result[currentrow]["god_name"];

        sectorinfo_by_id.Put(newsector->uid,newsector);
        sectorinfo_by_name.Put(csHashCompute(newsector->name),newsector);

        // Register the sector name in the msg_strings
        msg_strings.Request(newsector->name);
    }

    Notify2(LOG_STARTUP, "%lu Sectors Loaded", result.Count());
    return true;
}

bool CacheManager::PreloadMovement()
{
    Result modes(db->Select("SELECT * FROM movement_modes"));
    if(!modes.IsValid())
    {
        Error1("Could not cache database table. Check >movement_modes<");
        return false;
    }
    if(!modes.Count())
    {
        Error1("No movement modes to load.");
        return false;
    }

    for(unsigned int i=0; i<modes.Count(); i++)
    {
        psCharMode* newmode = new psCharMode;

        newmode->id = modes[i].GetUInt32("id");
        newmode->name = modes[i]["name"];
        newmode->move_mod.x = modes[i].GetFloat("move_mod_x");
        newmode->move_mod.y = modes[i].GetFloat("move_mod_y");
        newmode->move_mod.z = modes[i].GetFloat("move_mod_z");
        newmode->rotate_mod.x = modes[i].GetFloat("rotate_mod_x");
        newmode->rotate_mod.y = modes[i].GetFloat("rotate_mod_y");
        newmode->rotate_mod.z = modes[i].GetFloat("rotate_mod_z");
        newmode->idle_animation = modes[i]["idle_animation"];

        if(newmode->id > 127)   // Based on DR message variable size
        {
            Error3("ID %u for movement '%s' is to large.\n"
                   "Clients only support up to 128 different movement types, with IDs from 0-127",
                   newmode->id, newmode->name.GetData());
            delete newmode;
            return false;
        }

        char_modes.Put(newmode->id,newmode);
    }
    Notify2(LOG_STARTUP, "%lu Movement Modes Loaded", modes.Count());

    Result types(db->Select("SELECT * FROM movement_types"));
    if(!types.IsValid())
    {
        return false;
    }

    if(!types.Count())
    {
        Error1("No movement types to load.");
        return false;
    }

    for(unsigned int i=0; i<types.Count(); i++)
    {
        psMovement* newmove = new psMovement;

        newmove->id = types[i].GetUInt32("id");
        newmove->name = types[i]["name"];
        newmove->base_move.x = types[i].GetFloat("base_move_x");
        newmove->base_move.y = types[i].GetFloat("base_move_y");
        newmove->base_move.z = types[i].GetFloat("base_move_z");
        newmove->base_rotate.x = types[i].GetFloat("base_rotate_x");
        newmove->base_rotate.y = types[i].GetFloat("base_rotate_y");
        newmove->base_rotate.z = types[i].GetFloat("base_rotate_z");

        if(newmove->id > 31)   // Based on active move bits in client movement manager
        {
            Error3("ID %u for movement '%s' is to large.\n"
                   "Clients only support up to 32 different movement types, with IDs from 0-31",
                   newmove->id, newmove->name.GetData());
            delete newmove;
            return false;
        }

        movements.Put(newmove->id,newmove);
    }

    Notify2(LOG_STARTUP, "%lu Movement Types Loaded", types.Count());

    return true;
}

uint8_t CacheManager::GetCharModeID(const char* name)
{
    for(size_t i=0; i<char_modes.GetSize(); i++)
    {
        if(char_modes[i]->name == name)
        {
            return (uint8_t)i;
        }
    }
    return (uint8_t)-1;
}

uint8_t CacheManager::GetMovementID(const char* name)
{
    for(size_t i=0; i<movements.GetSize(); i++)
    {
        if(movements[i]->name == name)
        {
            return (uint8_t)i;
        }
    }
    return (uint8_t)-1;
}

bool CacheManager::PreloadArmorVsWeapon()
{
    unsigned int currentrow;
    Result result(db->Select("select * from armor_vs_weapon"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >armor_vs_weapon<");
        return false;
    }

    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        ArmorVsWeapon* newvs=new ArmorVsWeapon;

        newvs->id          = result[currentrow].GetUInt32("id");
        newvs->c[0][0]     = result[currentrow].GetFloat("1a");
        newvs->c[0][1]     = result[currentrow].GetFloat("1b");
        newvs->c[0][2]     = result[currentrow].GetFloat("1c");
        newvs->c[0][3]     = result[currentrow].GetFloat("1d");

        newvs->c[1][0]     = result[currentrow].GetFloat("2a");
        newvs->c[1][1]     = result[currentrow].GetFloat("2b");
        newvs->c[1][2]     = result[currentrow].GetFloat("2c");

        newvs->c[2][0]     = result[currentrow].GetFloat("3a");
        newvs->c[2][1]     = result[currentrow].GetFloat("3b");
        newvs->c[2][2]     = result[currentrow].GetFloat("3c");
        newvs->weapontype  = result[currentrow]["weapon_type"];

        armor_vs_weapon.Push(newvs);
    }


    Notify2(LOG_COMBAT,"Testing Armor VS Weapon table ('3c','Dagger'): %f\n",GetArmorVSWeaponResistance("3c","Dagger"));
    Notify2(LOG_COMBAT,"Testing Armor VS Weapon table ('1d','Claymore'): %f\n",GetArmorVSWeaponResistance("1d","Claymore"));
    Notify2(LOG_COMBAT,"Testing Armor VS Weapon table ('2b','Sabre'): %f\n",GetArmorVSWeaponResistance("2b","Sabre"));


    Notify2(LOG_STARTUP, "%lu Armor vs Weapon Loaded", result.Count());
    return true;
}

bool CacheManager::PreloadStances()
{
    Result result(db->Select("select * from stances order by id"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >stances<");
        return false;
    }

    for(unsigned int currentRow = 0; currentRow < result.Count(); currentRow++)
    {
        iResultRow &row = result[currentRow];
        Stance temp;
        temp.stance_id = row.GetInt("id")-1;
        temp.stance_name = row["name"];
        temp.stamina_drain_P = row.GetFloat("stamina_drain_P");
        temp.stamina_drain_M = row.GetFloat("stamina_drain_M");
        temp.attack_speed_mod = row.GetFloat("attack_speed_mod");
        temp.attack_damage_mod = row.GetFloat("attack_damage_mod");
        temp.defense_avoid_mod = row.GetFloat("defense_avoid_mod");
        temp.defense_absorb_mod = row.GetFloat("defense_absorb_mod");
        stances.Push(temp);
        stanceID.Push(temp.stance_name);

        // Register the sector name in the msg_strings
        msg_strings.Request(temp.stance_name);
    }
    Notify2(LOG_STARTUP, "%lu Stances Loaded", result.Count());
    return true;
}

bool CacheManager::PreloadAttacks()
{
    Result result(db->Select("select * from attacks order by id"));

    if (!result.IsValid())
    {
        Error1("Could not cache database table. Check >attacks<");
        return false;
    }

    for (unsigned long currentrow=0; currentrow<result.Count(); currentrow++)
    {
        psAttack* attack = new psAttack();

        if (attack->Load(result[currentrow]))
        {
            attacks_by_id.Put(attack->GetID(), attack);
            // add the icon to the string cache
            msg_strings.Request(result[currentrow]["image_name"]);
        }
        else
            delete attack;
    }

    Notify2( LOG_STARTUP, "%lu Attacks Loaded", result.Count() );
    return true;
}

csHash<psAttack*>::GlobalIterator CacheManager::GetAttackIterator()
{
    return attacks_by_id.GetIterator();
}

bool CacheManager::UnloadAttack(int id)
{
    bool ret = false;

    psAttack* attack = attacks_by_id.Get(id, NULL);
    if(attack)
    {
        delete attack;
        attacks_by_id.DeleteAll(id);
        ret = true;
    }
    else
    {
        CPrintf(CON_ERROR, "Cannot find attacks %d to remove.\n", id);
    }

    return ret;
}

bool CacheManager::LoadAttack(int id)
{
    if(attacks_by_id.Get(id, NULL))
    {
        CPrintf(CON_ERROR, "Attack already exists.\n", id);
        return false;
    }

    psAttack *attack;

    Result result(db->Select("select * from attacks where id=%d", id));

    if (!result.IsValid() || result.Count() == 0)
    {
        CPrintf(CON_ERROR, "Cannot find attack %d in database.\n", id);
        return false;
    }

    attack = new psAttack();
    if(attack->Load(result[id]))
    {
        attacks_by_id.Put(attack->GetID(),attack);
        return true;
    }
    delete attack;
    return false;
}

bool CacheManager::PreloadQuests()
{
    unsigned int currentrow;
    psQuest* quest;

    Result result(db->Select("select * from quests order by id"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >quests<");
        return false;
    }

    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        quest = new psQuest;

        if(quest->Load(result[currentrow]))
        {
            quests_by_id.Put(quest->GetID(),quest);
        }
        else
        {
            delete quest;
        }
    }

    Notify2(LOG_STARTUP, "%lu Quests Loaded", result.Count());
    return true;
}

csHash<psQuest*>::GlobalIterator CacheManager::GetQuestIterator()
{
    return quests_by_id.GetIterator();
}

bool CacheManager::UnloadQuest(int id)
{
    bool ret = false;

    psQuest* quest = quests_by_id.Get(id, NULL);
    if(quest)
    {
        delete quest;
        quests_by_id.DeleteAll(id);
        ret = true;
    }
    else
    {
        CPrintf(CON_ERROR, "Cannot find quest %d to remove.\n", id);
    }

    return ret;
}

bool CacheManager::LoadQuest(int id)
{
    if(quests_by_id.Get(id, NULL))
    {
        CPrintf(CON_ERROR, "Quest already exists.\n", id);
        return false;
    }

    psQuest* quest;

    Result result(db->Select("select * from quests where id=%d", id));

    if(!result.IsValid() || result.Count() == 0)
    {
        CPrintf(CON_ERROR, "Cannot find quest %d in database.\n", id);
        return false;
    }

    quest = new psQuest;

    bool success = false;

    success = quest->Load(result[0]);
    if(success)
    {
        // The quest id must first be put into the hash because PostLoad may references it.
        quests_by_id.Put(quest->GetID(),quest);

        success = quest->PostLoad();
        if(!success)
        {
            quests_by_id.DeleteAll(quest->GetID());
            CPrintf(CON_ERROR, "Could not load quest prerequisites for quest id %d", quest->GetID());
        }
    }

    if(success)
    {
        // Load scripts
        success = psserver->questmanager->LoadQuestScript(id);
        if(!success)
        {
            quests_by_id.DeleteAll(quest->GetID());
            CPrintf(CON_ERROR, "Could not load quest script for quest %d\n", id);
        }
    }

    if(!success)
    {
        delete quest;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////
// Trade Crafts

// Trade Combinations
bool CacheManager::PreloadTradeCombinations()
{
    unsigned int currentrow;
    uint32 lastarrayid = (uint32)-1;
    uint32 lastitemid = (uint32)-1;
    int lastitemqty = -1;
    CombinationConstruction* ctr;
    csPDelArray<CombinationConstruction>* newArray = NULL;

    Result result(db->Select("select * from trade_combinations order by pattern_id, result_id, item_id"));
    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >trade_combinations<");
    }
    else
    {
        // Loop through all the combinations
        for(currentrow=0; currentrow<result.Count(); currentrow++)
        {
            // If it is a new pattern start a new combination set for them
            uint32 id = result[currentrow].GetUInt32("pattern_id");
            if(id != lastarrayid)
            {
                newArray = new csPDelArray<CombinationConstruction>;
                tradeCombinations_IDHash.Put(id,newArray);
                lastarrayid = id;
            }

            // If it is a new item or quantity start a new combination construction for it.
            uint32 resultItem = result[currentrow].GetUInt32("result_id");
            int resultQty = result[currentrow].GetInt("result_qty");
            if((resultItem != lastitemid) || (resultQty != lastitemqty))
            {
                ctr = new CombinationConstruction;
                ctr->resultItem = resultItem;
                ctr->resultQuantity = resultQty;
                newArray->Push(ctr);
                lastitemid = resultItem;
                lastitemqty = resultQty;
            }

            // Load the combination and push it into it's construction set.
            psTradeCombinations* comb = new psTradeCombinations;
            if(comb->Load(result[currentrow]))
                ctr->combinations.Push(comb);
            else
                delete comb;
        }

        Notify2(LOG_STARTUP, "%lu Trade Combinations Loaded", result.Count());
    }
    return true;
}

csPDelArray<CombinationConstruction>* CacheManager::FindCombinationsList(uint32 patternid)
{
    return tradeCombinations_IDHash.Get(patternid,NULL);
}

// Trade Transformations
bool CacheManager::PreloadTradeTransformations()
{
    unsigned int currentrow;
    uint32 lastpid = (uint32)-1;
    uint32 lastiid = (uint32)-1;
    csHash<csPDelArray<psTradeTransformations> *,uint32>* transHash = NULL;
    csPDelArray<psTradeTransformations>* newArray = NULL;

    Result result(db->Select("select * from trade_transformations order by pattern_id, item_id"));
    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >trade_transformations<");
    }
    else
    {
        for(currentrow=0; currentrow<result.Count(); currentrow++)
        {
            // For new patterns create a new transformation hash and push it onto IDHash table
            uint32 pid = result[currentrow].GetUInt32("pattern_id");
            if(pid != lastpid)
            {
                transHash = new csHash<csPDelArray<psTradeTransformations> *,uint32>;
                tradeTransformations_IDHash.Put(pid,transHash);
                lastpid = pid;
            }

            // For new items create new transformation array
            uint32 iid = result[currentrow].GetUInt32("item_id");
            if(iid != lastiid)
            {
                newArray = new csPDelArray<psTradeTransformations>;
                transHash->Put(iid,newArray);
                lastiid = iid;
            }

            // Finally just load the transformation into existing array
            psTradeTransformations* tran = new psTradeTransformations;
            if(tran->Load(result[currentrow]))
                newArray->Push(tran);
            else
                delete tran;
        }

        Notify2(LOG_STARTUP, "%lu Trade Transformations Loaded", result.Count());
    }
    return true;
}

csPDelArray<psTradeTransformations>* CacheManager::FindTransformationsList(uint32 patternid, uint32 targetid)
{
    // First get transformation hash table using the patternid
    csHash<csPDelArray<psTradeTransformations> *,uint32>* transHash;
    transHash = tradeTransformations_IDHash.Get(patternid,NULL);
    if(!transHash)
    {
        Error2("No data in trade_transformations for pattern %d", patternid);
        return NULL;
    }

    // then return transformation array pointer
    csPDelArray<psTradeTransformations>* transArray = transHash->Get(targetid,NULL);
    if(!transArray)
    {
        Error2("No data in trade_transformations array for item %d", targetid);
        return NULL;
    }

    return transArray;
}

// Trade Transformations
bool CacheManager::PreloadUniqueTradeTransformations()
{
    uint32 currentID;
    csString query;
    csArray<uint32>* newArray;

    // Get a list of the trade patterns ids
    Result result(db->Select("select id from trade_patterns order by id"));
    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >trade_patterns<");
    }
    else
    {
        for(size_t currentrow=0; currentrow<result.Count(); currentrow++)
        {
            newArray = new csArray<uint32>;
            currentID = result[currentrow].GetUInt32("id");

            // Now get a list of unique transformation items for each pattern
            query.Format("select distinct item_id from trade_transformations where pattern_id =%d order by item_id", currentID);
            Result result(db->Select(query));
            if(!result.IsValid())
            {
                Error1("No distinct item ids found in trade_transformations");
            }
            else
            {
                // Push each result item ID into array
                for(size_t transrow=0; transrow<result.Count(); transrow++)
                {
                    newArray->Push(result[transrow].GetUInt32("item_id"));
                }

                // Now get a list of unique combination items for each pattern
                query.Format("select distinct item_id from trade_combinations where pattern_id =%d order by item_id", currentID);
                Result result2(db->Select(query));
                if(!result2.IsValid())
                {
                    Error1("No distinct item ids found in trade_combinations");
                }
                else
                {
                    // Push each result item ID into array
                    for(size_t combsrow=0; combsrow<result2.Count(); combsrow++)
                    {
                        newArray->Push(result2[combsrow].GetUInt32("item_id"));
                    }

                    // Add hash
                    tradeTransUnique_IDHash.Put(currentID,newArray);
                }
            }
        }

        Notify2(LOG_STARTUP, "%lu Unique Trade Transformations Loaded", result.Count());
    }
    return true;
}

csArray<uint32>* CacheManager::GetTradeTransUniqueByID(uint32 id)
{
    return tradeTransUnique_IDHash.Get(id,NULL);
}

// Trade Processes
bool CacheManager::PreloadTradeProcesses()
{
    uint32 lastpid = (uint32)-1;
    unsigned int currentrow;
    psTradeProcesses* newProcess;
    csArray<psTradeProcesses*>* newArray = NULL;

    // Get a list of the trade processes
    Result result(db->Select("select * from trade_processes order by process_id, subprocess_number"));
    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >trade_process<");
    }
    else
    {
        for(currentrow=0; currentrow<result.Count(); currentrow++)
        {
            // For a new process_id create a new process array and hash it
            uint32 pid = result[currentrow].GetUInt32("process_id");
            if(pid != lastpid)
            {
                newArray = new csArray<psTradeProcesses*>;
                tradeProcesses_IDHash.Put(pid,newArray);
                lastpid = pid;
            }

            // For each row create a new processes class to store it
            newProcess = new psTradeProcesses;
            if(!newProcess->Load(result[currentrow]))
            {
                Error2("Failure to load process %u",pid);
                delete newProcess;
                continue;
            }

            // Put process into array
            newArray->Push(newProcess);
        }
        Notify2(LOG_STARTUP, "%lu Trade Processes Loaded", result.Count());
    }
    return true;
}

csArray<psTradeProcesses*>* CacheManager::GetTradeProcessesByID(uint32 id)
{
    return tradeProcesses_IDHash.Get(id,NULL);
}

// Trade Patterns
bool CacheManager::PreloadTradePatterns()
{
    unsigned int currentrow;
    psTradePatterns* newPattern;

    // Get a list of the trade patterns ignoring the dummy ones
    Result result(db->Select("select * from trade_patterns order by designitem_id"));
    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >trade_patterns<");
    }
    else
    {
        for(currentrow=0; currentrow<result.Count(); currentrow++)
        {
            newPattern = new psTradePatterns;
            if(!newPattern->Load(result[currentrow]))
            {
                delete newPattern;
                Error1("Unable to load new pattern.");
                continue;
            }
            csString newName = newPattern->GetPatternString();
            tradePatterns_IDHash.Put(newPattern->GetDesignItemId(),newPattern);
            tradePatterns_NameHash.Put(newName, newPattern);
        }

        Notify2(LOG_STARTUP, "%lu Trade Patterns Loaded", result.Count());
    }
    return true;
}

csArray<psTradePatterns*> CacheManager::GetTradePatternByItemID(uint32 id)
{
    return tradePatterns_IDHash.GetAll(id);
}

psTradePatterns* CacheManager::GetTradePatternByName(csString name)
{
    return tradePatterns_NameHash.Get(name,NULL);
}


/** Insert Sorted Unique
 *  Insert the itemID into an array, sorted according the the item's Name, no more than one time.
 */
bool CacheManager::UniqueInsertIntoItemArray(csArray<uint32>* finalItems, uint32 itemID)
{
    if(itemID==0) return false;

    psItemStats* NewStats = GetBasicItemStatsByID(itemID);
    if(!NewStats)
    {
        return false;
    }

    for(size_t i=0; i<finalItems->GetSize(); i++)
    {
        psItemStats* CurrStats;
        if(finalItems->Get(i) == itemID)    //already in list
        {
            return false;
        }
        CurrStats = GetBasicItemStatsByID(finalItems->Get(i));
        if(!CurrStats)
        {
            return false;
        }
        if(strcmp(NewStats->GetName(),CurrStats->GetName())<1)
        {
            finalItems->Insert(i, itemID);      //insert before current item
            return true;
        }
    }
    finalItems->Push(itemID);
    return true;
}

/** Check if an int value is already in an array
 * This is used primarily with the "stack" arrays that track progress through a network of data nodes,
 * to determine if a node has already been visited.
 */
int CacheManager::Contains(csArray<uint32>* list, uint32 id)
{
    int matches	= 0;
    for(size_t i=0; i<list->GetSize(); i++)
    {
        if(list->Get(i)==id)
        {
            matches++;
        }
    }
    return matches;
}

/** Build the list of 'final' items.
 * starting at any random node, recursively walk the data network until a 'final' looking node is encountered,
 * then add it to the finalItems list.
 */
bool CacheManager::FindFinalItem(csHash<csHash<csPDelArray<psTradeTransformations>*, uint32> *,uint32>* txItemHash,
                                 csHash<csHash<csPDelArray<psTradeTransformations>*, uint32> *,uint32>* rxItemHash,
                                 csArray<uint32>* finalItems, uint32 itemID, csArray<uint32>* itemStack)
{
    csHash<csPDelArray<psTradeTransformations> *,uint32>* iHash	= NULL;

    itemStack->Push(itemID);
    iHash = txItemHash->Get(itemID, NULL);
    if(!iHash)
    {
        //no records found matching this itemID
        UniqueInsertIntoItemArray(finalItems, itemID);
    }
    else
    {
        csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator Iter(iHash->GetIterator());
        Iter.Reset();
        while(Iter.HasNext())
        {
            uint32 key;
            Iter.Next(key); // Move to next key

            if(itemID == key)
            {
                UniqueInsertIntoItemArray(finalItems, itemID);
            }
            else if(Contains(itemStack, key)>0)
            {
                //if there's a loop, it's impossible to determine programmatically which *should* be the final state;

                //DISCARDED OPTION: adopt the heuristic that the one that is the target of fewer transformations is the final one.
                //DISCARDED OPTION: adopt the hueristic that if they have the same number they are both 'final' states. This should
                //heuristic : product of more transforms = final item
                if(rxItemHash->Get(key, NULL)->GetSize() > rxItemHash->Get(itemID, NULL)->GetSize())
                {
                    UniqueInsertIntoItemArray(finalItems, key);
                }
                else
                {
                    UniqueInsertIntoItemArray(finalItems, itemID);
                }
            }
            else  	//move toward the final result...
            {
                if(!Contains(finalItems, key))
                {
                    FindFinalItem(txItemHash, rxItemHash, finalItems, key, itemStack);
                }
            }
        }
    }
    itemStack->Pop();

    return true;
}


bool CacheManager::ReconcileFinalItems(csHash<csHash<csPDelArray<psTradeTransformations>*, uint32> *,uint32>* txItemHash,
                                       csHash<csHash<csPDelArray<psTradeTransformations>*, uint32> *,uint32>* txResultHash,
                                       csArray<uint32>* finalItems,
                                       csArray<uint32>* craftBookItems,
                                       uint32 resultID,
                                       uint32 patternID,
                                       csArray<uint32>* itemStack)
{
    csHash<csPDelArray<psTradeTransformations> *,uint32>*   rHash   = NULL;

    itemStack->Push(resultID);
    rHash = txResultHash->Get(resultID, NULL);
    if(!rHash)
    {
        //no records found matching this resultID...try combinations
        csString        query;

        //query.Format("select * from trade_combinations where result_id=%d and pattern_id=%d", resultID, patternID);
        query.Format("select * from trade_combinations where result_id=%d", resultID);
        Result combinations(db->Select(query));
        if(!combinations.IsValid())
        {
            Error3("invalid return from \"select * from trade_combinations where result_id=%d and pattern_id=%d\"", resultID, patternID);
        }
        else if(combinations.Count()>0)
        {
            for(size_t j=0; j<combinations.Count(); j++)
            {
                if(Contains(finalItems, combinations[j].GetUInt32("item_id")))
                {
                    UniqueInsertIntoItemArray(craftBookItems, combinations[j].GetUInt32("item_id"));
                }
                ReconcileFinalItems(txItemHash, txResultHash, finalItems, craftBookItems, combinations[j].GetUInt32("item_id"), patternID, itemStack);
            }
        }
    }
    else
    {
        csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator Iter(rHash->GetIterator());
        Iter.Reset();
        while(Iter.HasNext())
        {
            uint32  key;
            Iter.Next(key); // Get next key

            if(!Contains(itemStack, key))
            {
                if(Contains(finalItems, key))
                {
                    UniqueInsertIntoItemArray(craftBookItems,  key);
                }
                ReconcileFinalItems(txItemHash, txResultHash, finalItems, craftBookItems, key, patternID, itemStack);
            }
        }
    }

    return true;
}



/** load transformations into hashes for faster access.
 * NOTE that these hashes are for use specifically in creating the Craft book messages and are cleaned up after.
 */
bool CacheManager::loadTradeTransformationsByPatternAndGroup(Result* TransformationList,
        csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>* txResultHash,
        csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>* txItemHash)
{
    csHash<csPDelArray<psTradeTransformations> *,uint32>* rHash	= NULL;
    csPDelArray<psTradeTransformations>* rArray	= NULL;
    csHash<csPDelArray<psTradeTransformations> *,uint32>* iHash	= NULL;
    csPDelArray<psTradeTransformations>* iArray	= NULL;
    psTradeTransformations* tx	= NULL;

    for(size_t currentrow=0; currentrow<TransformationList->Count(); currentrow++)
    {
        uint32 itemID	= (*TransformationList)[currentrow].GetUInt32("item_id");
        uint32 resultID	= (*TransformationList)[currentrow].GetUInt32("result_id");

        //load hash by result - use to traverse from results to item
        if(!(rHash = txResultHash->Get(resultID, NULL)))
        {
            //if there's no hash for this resultID, add one
            rHash = new csHash<csPDelArray<psTradeTransformations> *,uint32>;
            txResultHash->Put(resultID,rHash);
        }

        //only if transformation from item -> result occurs by different processes will there be more than 1 item in the array
        if(!(rArray = rHash->Get(itemID, NULL)))
        {
            //if there's no array for this itemID, add one
            rArray = new csPDelArray<psTradeTransformations>;
            rHash->Put(itemID,rArray);
        }

        //load hash by item - use to traverse from item to results
        if(!(iHash = txItemHash->Get(itemID, NULL)))
        {
            //if there's no hash for this itemID, add one
            iHash = new csHash<csPDelArray<psTradeTransformations> *,uint32>;
            txItemHash->Put(itemID, iHash);
        }

        //only if transformation from item -> result occurs by different processes will there be more than 1 item in the array
        if(!(iArray = iHash->Get(resultID, NULL)))
        {
            //if there's no array for this resultID, add one
            iArray = new csPDelArray<psTradeTransformations>;
            iHash->Put(resultID,iArray);
        }

        tx = new psTradeTransformations;
        if(tx->Load((*TransformationList)[currentrow]))
        {
            rArray->Push(tx);
        }
        else
        {
            delete tx;
        }
    }
    return true;
}

/** dispose of transformation hashes for craft books.
 *
 */
bool CacheManager::freeTradeTransformationsByPatternAndGroup(csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>* txItemHash, csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>* txResultHash)
{

    csHash<csPDelArray<psTradeTransformations> *,uint32>*   iHash   = NULL;
    csPDelArray<psTradeTransformations>*                    iArray  = NULL;

    csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>::GlobalIterator txIter(txItemHash->GetIterator());
    txIter.Reset();
    while(txIter.HasNext())
    {
        iHash = txIter.Next();
        if(iHash)
        {
            csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator Iter(iHash->GetIterator());
            Iter.Reset();
            while(Iter.HasNext())
            {
                iArray =  Iter.Next();
                if(iArray)
                {
                    delete iArray;
                }
            }
            delete iHash;
        }
    }

    csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>::GlobalIterator rxIter(txResultHash->GetIterator());
    rxIter.Reset();
    while(rxIter.HasNext())
    {
        iHash = rxIter.Next();
        if(iHash)
        {
            csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator Iter(iHash->GetIterator());
            Iter.Reset();
            while(Iter.HasNext())
            {
                iArray =  Iter.Next();
                if(iArray)
                {
                    delete iArray;
                }
            }
            delete iHash;
        }
    }

    delete txItemHash;
    delete txResultHash;

    return false;
}

bool CacheManager::DescribeTransformation(psTradeTransformations* t, csArray<CraftTransInfo*>* newArray)
{
    //    uint32	itemID = t->GetItemId();
    uint32	processID = t->GetProcessId();
    //    uint32	resultID = t->GetResultId();

    csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(processID);

    if(procArray)
    {
        for(size_t k=0; k<procArray->GetSize(); k++)
        {
            psTradeProcesses* proc = procArray->Get(k);
            if(proc->GetSubprocessId()==0)
            {
                CraftTransInfo* craftInfo = new CraftTransInfo;

                craftInfo->priSkillId = proc->GetPrimarySkillId();
                craftInfo->minPriSkill = proc->GetMinPrimarySkill();
                craftInfo->secSkillId = proc->GetSecondarySkillId();
                craftInfo->minSecSkill = proc->GetMinSecondarySkill();
                craftInfo->craftStepDescription = CreateTransCraftDescription(t,proc);
                if(!(craftInfo->craftStepDescription.IsEmpty()))
                {
                    craftInfo->craftStepDescription.Append(".\n");
                    craftInfo->craftStepDescription.Insert(0, "   ");
                    newArray->Push(craftInfo);
                }
                else
                    delete craftInfo;
            }
        }
    }

    return true;
}

bool CacheManager::DescribeMultiTransformation(csPDelArray<psTradeTransformations>* rArray, csArray<CraftTransInfo*>* newArray)
{
    //sort transformations by their processes skill requirements
    csPDelArray<psTradeTransformations>* sortedArray = NULL;
    for(size_t i=0; i<rArray->GetSize(); i++)
    {
        psTradeTransformations*    t = rArray->Get(i);
        if(sortedArray==NULL)
        {
            sortedArray = new  csPDelArray<psTradeTransformations>;
            sortedArray->Push(rArray->Get(i));
        }
        else
        {

            size_t j;
            for(j=0; j<sortedArray->GetSize(); j++)
            {
                uint32      uprocessID = t->GetProcessId();
                csArray<psTradeProcesses*>* uProcArray = GetTradeProcessesByID(uprocessID);

                psTradeTransformations*    s = sortedArray->Get(j);
                uint32      sprocessID = s->GetProcessId();
                csArray<psTradeProcesses*>* sProcArray = GetTradeProcessesByID(sprocessID);

                int uPri = uProcArray->Get(0)->GetPrimarySkillId();
                int upLvl = uProcArray->Get(0)->GetMinPrimarySkill();
                int sPri = sProcArray->Get(0)->GetPrimarySkillId();
                int spLvl = sProcArray->Get(0)->GetMinPrimarySkill();

                if(uPri < sPri)
                {
                    sortedArray->Insert(j, t);
                    break;
                }
                else if(uPri==sPri)
                {
                    //if Primary skills are the same, consider primary skill levels
                    if(upLvl < spLvl)
                    {
                        sortedArray->Insert(j, t);
                        break;
                    }
                    else if(uPri==sPri)
                    {
                        //if Primary skills and levels are the same, consider secondary skills

                        int uSec = uProcArray->Get(0)->GetSecondarySkillId();
                        int usLvl = uProcArray->Get(0)->GetMinSecondarySkill();
                        int sSec = sProcArray->Get(0)->GetSecondarySkillId();
                        int ssLvl = sProcArray->Get(0)->GetMinSecondarySkill();

                        if(uSec < sSec)
                        {
                            sortedArray->Insert(j, t);
                            break;
                        }
                        else if(uPri==sPri)
                        {
                            //if Secondary skills are the same, consider Secondary skill levels
                            if(usLvl < ssLvl)
                            {
                                sortedArray->Insert(j, t);
                                break;
                            }
                        }
                    }
                }
            }
            if(j==sortedArray->GetSize())
            {
                sortedArray->Push(rArray->Get(i));
            }
        }
    }

//printf( "----------\n" );
//for( int i=0; i<sortedArray->GetSize(); i++ ) {
//                psTradeTransformations*    s = sortedArray->Get(i);
//                uint32      sprocessID = s->GetProcessId();
//                csArray<psTradeProcesses*>* sProcArray = GetTradeProcessesByID(sprocessID);
//
//                int sPri = sProcArray->Get(0)->GetPrimarySkillId();
//                int sSec = sProcArray->Get(0)->GetSecondarySkillId();
//printf( "DEBUG: sortedArray( %d ) Primary = %d, Secondary = %d\n", i, sPri, sSec);
//}
//printf( "----------\n" );

    if(!sortedArray)
    {
        return false;
    }


    psTradeTransformations*    t = sortedArray->Get(0);
    uint32      processID = t->GetProcessId();
    int         lastPrimarySkillId;
    uint32      lastPrimarySkillLvl;
    int         lastSecondarySkillId;
    uint32      lastSecondarySkillLvl;

    csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(processID);
    if(!procArray)
    {
        return false;
    }
    psTradeProcesses* proc = procArray->Get(0);

    CraftTransInfo* craftInfo;
    craftInfo = new CraftTransInfo;

    craftInfo->priSkillId = proc->GetPrimarySkillId();
    lastPrimarySkillId = proc->GetPrimarySkillId();

    craftInfo->minPriSkill = proc->GetMinPrimarySkill();
    lastPrimarySkillLvl = proc->GetMinPrimarySkill();

    craftInfo->secSkillId = proc->GetSecondarySkillId();
    lastSecondarySkillId = proc->GetSecondarySkillId();

    craftInfo->minSecSkill = proc->GetMinSecondarySkill();
    lastSecondarySkillLvl = proc->GetMinSecondarySkill();

    craftInfo->craftStepDescription = CreateTransCraftDescription(t,proc);

    uint32 itemCounter=0;

    for(size_t j=1; j<sortedArray->GetSize(); j++)
    {
        psTradeTransformations*    u = sortedArray->Get(j);
        csArray<psTradeProcesses*>* procArray = GetTradeProcessesByID(u->GetProcessId());

        for(size_t k=0; k<procArray->GetSize(); k++)
        {
            proc = procArray->Get(k);
            if(proc->GetEquipementId() != 0)
            {
                if(proc->GetPrimarySkillId() != lastPrimarySkillId  ||
                        proc->GetMinPrimarySkill() != lastPrimarySkillLvl ||
                        proc->GetSecondarySkillId() != lastSecondarySkillId ||
                        proc->GetMinSecondarySkill() != lastSecondarySkillLvl)
                {
                    craftInfo->craftStepDescription.Append(".\n");
                    newArray->Push(craftInfo);

                    craftInfo = new CraftTransInfo;

                    craftInfo->priSkillId = proc->GetPrimarySkillId();
                    lastPrimarySkillId = proc->GetPrimarySkillId();
                    craftInfo->minPriSkill = proc->GetMinPrimarySkill();

                    craftInfo->secSkillId = proc->GetSecondarySkillId();
                    lastSecondarySkillId = proc->GetSecondarySkillId();
                    craftInfo->minSecSkill = proc->GetMinSecondarySkill();

                    craftInfo->craftStepDescription = CreateTransCraftDescription(t,proc);
                    continue;
                }
                /*                else if( proc->GetSecondarySkillId() != lastSecondarySkillId )
                                { //Primary is same but secondary is different

                                    craftInfo->craftStepDescription.Append(".\n");
                                    newArray->Push(craftInfo);

                                    craftInfo = new CraftTransInfo;

                                    craftInfo->priSkillId = proc->GetPrimarySkillId();
                                    lastPrimarySkillId = proc->GetPrimarySkillId();
                                    craftInfo->minPriSkill = proc->GetMinPrimarySkill();

                                    craftInfo->secSkillId = proc->GetSecondarySkillId();
                                    lastSecondarySkillId = proc->GetSecondarySkillId();
                                    craftInfo->minSecSkill = proc->GetMinSecondarySkill();

                                    craftInfo->craftStepDescription = CreateTransCraftDescription(t,proc);
                                    //itemCounter=1;
                                    continue;
                                }
                */
                psItemStats* toolStats = GetBasicItemStatsByID(proc->GetEquipementId());
                if(toolStats)
                {
                    //if(itemCounter>0)
                    //{
                    craftInfo->craftStepDescription.Append(", ");
                    //}
                    craftInfo->craftStepDescription.Append(toolStats->GetName());
                    itemCounter++;
                }
                else
                {
                    craftInfo->craftStepDescription.Append(" <tool ");
                    craftInfo->craftStepDescription.Append(toolStats->GetName());
                    craftInfo->craftStepDescription.Append("> ");
                }
            }
        }
    }
    if(!(craftInfo->craftStepDescription.IsEmpty()))
    {
        craftInfo->craftStepDescription.Insert(0, "   ");
        craftInfo->craftStepDescription.Append(".\n");
        newArray->Push(craftInfo);
    }
    else
    {
        delete craftInfo;
    }

    return true;
}



bool CacheManager::DescribeCombination(Result* combinations, csArray<CraftTransInfo*>* newArray)
{
    CraftTransInfo* craftInfo;
    craftInfo = new CraftTransInfo;

    craftInfo->craftStepDescription = CreateComboCraftDescription(combinations);

    //note that the skill mins are not used for combinations, so set them all to 0
    craftInfo->priSkillId = craftInfo->minPriSkill = craftInfo->secSkillId = craftInfo->minSecSkill = 0;

    craftInfo->craftStepDescription.Insert(0, "   ");
    newArray->Push(craftInfo);

    return true;
}

/** List the steps to produce an item.
 * given a result item ID, recursively list all the transformations and combinations required to produce it
 */
bool CacheManager::ListProductionSteps(csArray<CraftTransInfo*>* newArray,
                                       csHash<csHash<csPDelArray<psTradeTransformations> *, uint32> *,uint32>* txResultHash,
                                       csArray<uint32>* finalItems, uint32 resultID, uint32 patternID, uint groupID, csArray<uint32>* itemStack)
{
    csHash<csPDelArray<psTradeTransformations> *,uint32>*   rHash   = NULL;
    csPDelArray<psTradeTransformations>*                    rArray  = NULL;

    itemStack->Push(resultID);
    rHash = txResultHash->Get(resultID, NULL);
    if(!rHash)
    {
        //no records found matching this resultID...try combinations
        csString        query;

        query.Format("select * from trade_combinations where result_id=%d", resultID);
        Result combinations(db->Select(query));
        if(!combinations.IsValid())
        {
            Error3("invalid return from \"select * from trade_combinations where result_id=%d and pattern_id=%d\"", resultID, patternID);
        }
        else if(combinations.Count()>0)
        {
            for(size_t j=0; j<combinations.Count(); j++)
            {
                if(!Contains(finalItems, combinations[j].GetUInt32("item_id")))
                {
                    ListProductionSteps(newArray, txResultHash, finalItems, combinations[j].GetUInt32("item_id"), patternID, groupID, itemStack);
                }
            }
            DescribeCombination(&combinations, newArray);
        }
    }
    else
    {
        csHash<csPDelArray<psTradeTransformations> *,uint32>::GlobalIterator Iter(rHash->GetIterator());
        Iter.Reset();
        while(Iter.HasNext())
        {
            uint32  Key;
            rArray =  Iter.Next(Key);

            if(resultID == Key || Contains(itemStack, Key))
            {
                if(rArray->GetSize()==1)
                {
                    DescribeTransformation(rArray->Get(0), newArray);
                }
                else
                {
                    DescribeMultiTransformation(rArray, newArray);
                }
            }
            else if(!Contains(itemStack, Key))
            {
                if(!Contains(finalItems, Key))
                {
                    ListProductionSteps(newArray, txResultHash, finalItems, Key, patternID, groupID, itemStack);
                }
                for(size_t j=0; j<rArray->GetSize(); j++)
                {
                    DescribeTransformation(rArray->Get(j), newArray);
                }

            }
        }
    }

    return true;
}





// Trade Info Message
bool CacheManager::PreloadCraftMessages()
{
    csArray<CraftTransInfo*>* newArray = NULL;


    // Get a list of all the trade patterns in the database ordered by design item ID
    Result result(db->Select("SELECT * from trade_patterns where designitem_id<>0 order by designitem_id"));
    for(size_t currentPattern=0; currentPattern<result.Count(); currentPattern++)
    {
        CraftTransInfo* craftInfo;
        csString query;
        csArray<uint32>* finalItems = new csArray<uint32>;
        csArray<uint32>* finalPatternItems = new csArray<uint32>;
        csArray<uint32>* craftBookItems = new csArray<uint32>;

        csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>*	txResultHash	= new csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>;
        csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>*	txItemHash	= new csHash<csHash<csPDelArray<psTradeTransformations> *,uint32> *,uint32>;

        // Get the design item that goes with the pattern
        uint32 patternID = result[currentPattern].GetUInt32("id");
        uint32 designItemID = result[currentPattern].GetUInt32("designitem_id");
        uint32 currentGroupID = result[currentPattern].GetUInt32("group_id");

        newArray = new csArray<CraftTransInfo*>;
        tradeCraftTransInfo_IDHash.Put(designItemID,newArray);

        //Get the list of transformations for this pattern and group
        query.Format("select * from trade_transformations where pattern_id=%i or pattern_id=%i", patternID, currentGroupID);
        Result TransformationList(db->Select(query));
        if(!result.IsValid())
        {
            Error1("Could not cache database table. Check >trade_transformations<");
        }
        else
        {
            loadTradeTransformationsByPatternAndGroup(&TransformationList, txResultHash, txItemHash);
        }

        //identify the final products of transformations
        for(size_t i=0; i<TransformationList.Count(); i++)
        {
            csArray<uint32>* itemStack = new csArray<uint32>;

            FindFinalItem(txItemHash, txResultHash, finalItems, TransformationList[i].GetInt("item_id"), itemStack);
            itemStack->DeleteAll();
        }

        //identify final items directly related to items with the pattern_id
        for(size_t i=0; i<TransformationList.Count(); i++)
        {
            if(TransformationList[i].GetUInt32("pattern_id")==patternID)
            {
                csArray<uint32>* itemStack = new csArray<uint32>;

                FindFinalItem(txItemHash, txResultHash, finalPatternItems, TransformationList[i].GetInt("item_id"), itemStack);
                itemStack->DeleteAll();
            }
        }

        //merge the final items that appear in 'finalItems' and are used by items appearing in 'finalPatternItems' with those
        //appearing in 'finalPatternItems' to produce 'craftBookItems'
        for(size_t i=0; i<finalPatternItems->GetSize(); i++)
        {
            csArray<uint32>* itemStack = new csArray<uint32>;

            ReconcileFinalItems(txItemHash, txResultHash, finalItems, craftBookItems, finalPatternItems->Get(i), patternID, itemStack);
            itemStack->DeleteAll();
        }
        for(size_t i=0; i<finalPatternItems->GetSize(); i++)
        {
            UniqueInsertIntoItemArray(craftBookItems, finalPatternItems->Get(i));
        }

        //for each of the items in craftBookItems
        for(size_t i=0; i<craftBookItems->GetSize(); i++)
        {
            //list the item to be created
            craftInfo = new CraftTransInfo;
            craftInfo->priSkillId = craftInfo->minPriSkill = craftInfo->secSkillId = craftInfo->minSecSkill = 0;
            craftInfo->craftStepDescription.Append("-- ");
            if(GetBasicItemStatsByID(craftBookItems->Get(i)))
            {
                craftInfo->craftStepDescription.Append(GetBasicItemStatsByID(craftBookItems->Get(i))->GetName());
            }
            else
            {
                craftInfo->craftStepDescription.Append(" item ");
                craftInfo->craftStepDescription.Append(craftBookItems->Get(i));
            }
            craftInfo->craftStepDescription.Append(" ----- \n");
            newArray->Push(craftInfo);

            //list the steps to create it
            csArray<uint32>* itemStack = new csArray<uint32>;

            ListProductionSteps(newArray, txResultHash, finalItems, craftBookItems->Get(i), patternID, currentGroupID, itemStack);
            itemStack->DeleteAll();

            //insert a blank row between items
            craftInfo = new CraftTransInfo;
            craftInfo->priSkillId = craftInfo->minPriSkill = craftInfo->secSkillId = craftInfo->minSecSkill = 0;
            craftInfo->craftStepDescription = "\n";
            newArray->Push(craftInfo);
        }

        freeTradeTransformationsByPatternAndGroup(txItemHash, txResultHash);
        finalItems->DeleteAll();
        finalPatternItems->DeleteAll();

    }
    Notify2(LOG_STARTUP, "%lu Craft Books Loaded", result.Count());
    return true;
}

csString CacheManager::CreateTransCraftDescription(psTradeTransformations* tran, psTradeProcesses* proc)
{
    csString desc("");

    // Get base item or skip for 0 id
    psItemStats* itemStats = GetBasicItemStatsByID(tran->GetItemId());
    if(!itemStats)
    {
        return desc;
    }

    // Get result name or skip for 0 id
    psItemStats* resultStats = GetBasicItemStatsByID(tran->GetResultId());
    if(!resultStats)
    {
        return desc;
    }

    // Get work name or skip for 0 id
    psItemStats* workStats = GetBasicItemStatsByID(proc->GetWorkItemId());
    if(!workStats)
    {
        return desc;
    }
    // Create craft message
    //  Example: "Bake with skill 2 waybread dough into a waybread using oven"
    if(tran->GetItemQty() > 1)
        desc.Format("%s %d %ss into ", proc->GetName().GetData(), tran->GetItemQty(), itemStats->GetName());
    else
        desc.Format("%s %s into ", proc->GetName().GetData(), itemStats->GetName());

    csString secondHalf;
    if(tran->GetResultQty() > 1)
        secondHalf.Format("%d %ss using %s", tran->GetResultQty(), resultStats->GetName(), workStats->GetName());
    else
        secondHalf.Format("%s using %s", resultStats->GetName(), workStats->GetName());

    desc.Append(secondHalf);

    // Get tool name if one exists
    if(proc->GetEquipementId() != 0)
    {
        csString temp;
        psItemStats* toolStats = GetBasicItemStatsByID(proc->GetEquipementId());
        if(!toolStats)
        {
            Error2("No tool id %u", proc->GetEquipementId());
            temp.Format(" <tool %i> ", proc->GetEquipementId());
            desc.Append(temp);
        }
        else
        {
            temp.Format(" with a %s", toolStats->GetName());
            desc.Append(temp);
        }
    }
    return desc;
}

csString CacheManager::CreateComboCraftDescription(Result* currentComb)
{
    csString temp;
    csString desc("");

    // Check for matching lists and create combination info string
    //  Example: "Combine between 2 and 4 Eggs, Nuts, 5 Milk into Waybread Batter."

    desc.Append("Combine ");

    // Get each of the items
    for(size_t j=0; j<(*currentComb).Count(); j++)
    {
        uint32 combId  = (*currentComb)[j].GetInt("item_id");
        int combMinQty = (*currentComb)[j].GetInt("min_qty");
        int combMaxQty = (*currentComb)[j].GetInt("max_qty");
        psItemStats* itemStats = GetBasicItemStatsByID(combId);
        if(!itemStats)
        {
            Error2("No item stats for id %u", combId);
            desc.Append("<item ");
            desc.Append(combId);
            desc.Append("> ");
        }
        else if(combMinQty == combMaxQty)
            // Check if min and max is same
        {
            if(combMinQty == 1)
            {
                temp.Format("%s, ", itemStats->GetName());
                desc.Append(temp);
            }
            else
            {
                temp.Format("%d %ss, ", combMinQty, itemStats->GetName());
                desc.Append(temp);
            }
        }
        else
        {
            temp.Format("between %d and %d %ss, ", combMinQty, combMaxQty, itemStats->GetName());
            desc.Append(temp);
        }
    }

    // Get result item names
    psItemStats* resultItemStats = GetBasicItemStatsByID((*currentComb)[0].GetInt("result_id"));
    if(!resultItemStats)
    {
        Error2("No item stats for id %u", (*currentComb)[0].GetInt("result_id"));
        desc.Append(" <item ");
        desc.Append((*currentComb)[0].GetInt("result_id"));
        desc.Append("> ");
    }

    // Add result part of description
    if((*currentComb)[0].GetInt("result_id") == 1)
    {
        temp.Format("into %s.\n", resultItemStats->GetName());
        desc.Append(temp);
    }
    else
    {
        temp.Format("into %d %ss.\n", (*currentComb)[0].GetInt("result_qty"), resultItemStats->GetName());
        desc.Append(temp);
    }
    return desc;
}

csArray<CraftTransInfo*>* CacheManager::GetTradeTransInfoByItemID(uint32 id)
{
    return tradeCraftTransInfo_IDHash.Get(id,NULL);
}

csArray<CraftComboInfo*>* CacheManager::GetTradeComboInfoByItemID(uint32 id)
{
    return tradeCraftComboInfo_IDHash.Get(id,NULL);
}

////////////////////////////////////////////////////////////////////
bool CacheManager::PreloadTips()
{
    unsigned int currentrow;

    // Id<1000 means we are excluding Tutorial tips
    Result result(db->Select("select tip from tips where id<1000"));
    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >tips<");
        return false;
    }

    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        csString dummy(result[currentrow][0]);
        tips_list.Push(dummy);
    }

    Notify2(LOG_STARTUP, "%lu Tips Loaded", result.Count());
    return true;
}

/////////////////////////////////////////////////////////////////////
const char* CacheManager::FindCommonString(unsigned int id)
{
    if(id==0)
    {
        return "";
    }
    return msg_strings.Request(id);
}
unsigned int CacheManager::FindCommonStringID(const char* name)
{
    if(name == NULL)
    {
        return 0;
    }

    csStringID id = (csStringID) msg_strings.Request(name);
    return (id == csInvalidStringID) ? 0 : id.GetHash();
}

#define COMPRESSION_BUFFSIZE MAX_MESSAGE_SIZE/2
#define PACKING_BUFFSIZE COMPRESSION_BUFFSIZE*3
void CacheManager::GetCompressedMessageStrings(char* &data, unsigned long &size,
        uint32_t &num_strings, csMD5::Digest &digest)
{
    if(compressed_msg_strings == NULL || num_compressed_strings != msg_strings.GetSize())
    {
        num_compressed_strings = msg_strings.GetSize();
        delete[] compressed_msg_strings;
        compressed_msg_strings = new char[COMPRESSION_BUFFSIZE]; // Holds compressed data.
        compressed_msg_strings_size = 0;

        char* temp = new char[PACKING_BUFFSIZE]; // Holds packed strings.

        csStringHashReversible::GlobalIterator it = msg_strings.GetIterator();
        while(it.HasNext())
        {
            const char* string;
            csStringID id = it.Next(string);

            // Pack ID
            uint32* p = (uint32*)(temp+compressed_msg_strings_size);
            *p = csLittleEndian::UInt32(id);
            compressed_msg_strings_size += sizeof(uint32);

            // Pack string
            strcpy(temp+compressed_msg_strings_size, string);
            compressed_msg_strings_size += strlen(string)+1;
        }

        // Ready
        z_stream z;
        z.zalloc = NULL;
        z.zfree = NULL;
        z.opaque = NULL;
        z.next_in = (Bytef*)temp;
        z.avail_in = (uInt)compressed_msg_strings_size;
        z.next_out = (Bytef*)compressed_msg_strings;
        z.avail_out = COMPRESSION_BUFFSIZE;

        // Set
        int err = deflateInit(&z,Z_BEST_COMPRESSION);
        CS_ASSERT(err == Z_OK);

        // Go
        err = deflate(&z,Z_FINISH);
        CS_ASSERT(err == Z_STREAM_END);

        // Now se how much data we did are left with after deflating
        compressed_msg_strings_size = COMPRESSION_BUFFSIZE - z.avail_out;

        deflateEnd(&z);

        delete[] temp;

        compressed_msg_strings_digest = csMD5::Encode(compressed_msg_strings, compressed_msg_strings_size);
    }

    data = compressed_msg_strings;
    size = compressed_msg_strings_size;
    num_strings = num_compressed_strings;
    digest = compressed_msg_strings_digest;
}

psAttack* CacheManager::GetAttackByID(unsigned int id)
{
    psAttack *attack = attacks_by_id.Get(id, NULL);
    return attack;
}

csArray<psAttack*> CacheManager::GetAllAttacks()
{
    csHash<psAttack*>::GlobalIterator it (attacks_by_id.GetIterator ());
    csArray<psAttack*> attacks;
    while(it.HasNext())
    {
        attacks.Push(it.Next());
    }
    return attacks;
}

psAttack* CacheManager::GetAttackByName(const char *name)
{
    if (name==NULL)
        return NULL;

    csHash<psAttack*>::GlobalIterator it (attacks_by_id.GetIterator ());
    while (it.HasNext())
    {
        psAttack* attack = it.Next();
        if(!strcasecmp(attack->GetName(), name))
            return attack;
    }
    return NULL;
}

psQuest* CacheManager::GetQuestByID(unsigned int id)
{
    psQuest* quest = quests_by_id.Get(id, NULL);
    return quest;
}

psQuest* CacheManager::GetQuestByName(const char* name)
{
    if(name==NULL)
        return NULL;

    csHash<psQuest*>::GlobalIterator it(quests_by_id.GetIterator());
    while(it.HasNext())
    {
        psQuest* quest = it.Next();
        if(!strcasecmp(quest->GetName(), name))
            return quest;
    }
    return NULL;
}

psQuest* CacheManager::AddDynamicQuest(const char* name, psQuest* parentQuest, int step)
{
    if(!name)
    {
        return NULL;
    }

    psQuest* ptr;
    //  subquests need a fixed id to be loaded at next restart
    // quest_id*10000+sub_id
    int id = 10000+(parentQuest->GetID()*100)+step;
    ptr = new psQuest;
    ptr->Init(id,name);
    ptr->SetParentQuest(parentQuest);
    parentQuest->AddSubQuest(id);
    quests_by_id.Put(id, ptr);

    return ptr;
}

psSkillInfo* CacheManager::GetSkillByID(unsigned int id)
{
    return skillinfo_IDHash.Get((int)id, NULL);
}

size_t CacheManager::GetSkillAmount()
{
    return skillinfo_IDHash.GetSize();
}

psSkillInfo* CacheManager::GetSkillByName(const char* name)
{
    return skillinfo_NameHash.Get(csString(name).Upcase(), NULL);
}

void CacheManager::GetSkillsListbyCategory(csArray <psSkillInfo*> &listskill, int category)
{
    listskill = skillinfo_CategoryHash.GetAll(category);
    return;
}

csHash<psSectorInfo*>::GlobalIterator CacheManager::GetSectorIterator()
{
    return sectorinfo_by_name.GetIterator();
}

psSectorInfo* CacheManager::GetSectorInfoByID(unsigned int id)
{
    return sectorinfo_by_id.Get(id, NULL);
}

psSectorInfo* CacheManager::GetSectorInfoByName(const char* name)
{
    if(name == NULL || strlen(name) == 0)
    {
        return NULL;
    }

    return sectorinfo_by_name.Get(csHashCompute(name), NULL);
}

PSTRAIT_LOCATION CacheManager::ConvertTraitLocationString(const char* locationstring)
{

    if(locationstring==NULL)
    {
        return PSTRAIT_LOCATION_NONE;
    }

    for(size_t i = 0; i < PSTRAIT_LOCATION_COUNT; i++)
    {
        if(!strcasecmp(locationstring,psTrait::locationString[i]))
            return (PSTRAIT_LOCATION)i;
    }

    return PSTRAIT_LOCATION_NONE;
}

bool CacheManager::PreloadTraits()
{
    unsigned int currentrow;
    psTrait* newtrait;
    Result result(db->Select("SELECT * from traits order by id"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >traits<");
        return false;
    }

    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        newtrait=new psTrait;

        PSTRAIT_LOCATION loc;

        newtrait->uid               = result[currentrow].GetUInt32("id");
        newtrait->next_trait_uid    = result[currentrow].GetUInt32("next_trait");
        newtrait->raceID            = result[currentrow].GetUInt32("race_id");
        newtrait->name              = result[currentrow]["name"];
        newtrait->cstr_id_mesh      = FindCommonStringID(result[currentrow]["cstr_mesh"]);
        newtrait->cstr_id_material  = FindCommonStringID(result[currentrow]["cstr_material"]);
        newtrait->cstr_id_texture   = FindCommonStringID(result[currentrow]["cstr_texture"]);
        newtrait->onlyNPC           = result[currentrow].GetInt("only_npc") != 0;
        newtrait->shaderVar         = result[currentrow]["shader"];
        psRaceInfo* raceInfo = GetRaceInfoByID(newtrait->raceID);
        if(raceInfo == NULL)
        {
            Error3("Trait (%u) references unresolvable race  %s.",
                   newtrait->uid,result[currentrow]["race_id"]);
            delete newtrait;
            continue;
        }

        newtrait->race = raceInfo->race;
        newtrait->gender = raceInfo->gender;

        loc=ConvertTraitLocationString(result[currentrow]["location"]);
        if(loc==PSTRAIT_LOCATION_NONE)
        {
            Error3("Trait (%u) references unresolvable location  %s.",
                   newtrait->uid,result[currentrow]["location"]);
            delete newtrait;
            continue;
        }
        newtrait->location = loc;

        traitlist.Push(newtrait);
    }
    // Update cross ref to next_trait
    for(size_t i = 0; i < traitlist.GetSize(); i++)
    {
        unsigned int next_uid = traitlist[i]->next_trait_uid;
        traitlist[i]->next_trait = GetTraitByID(next_uid);
    }


    Notify2(LOG_STARTUP, "%zu Traits Loaded", traitlist.GetSize());
    return true;
}

// TODO: This should be done faster, probably not with an array
psTrait* CacheManager::GetTraitByID(unsigned int id)
{
    size_t i;
    psTrait* currenttrait;

    for(i=0; i<traitlist.GetSize(); i++)
    {
        currenttrait=traitlist.Get(i);
        if(currenttrait!=NULL && currenttrait->uid==id)
        {
            return currenttrait;
        }
    }
    return NULL;
}

// TODO: This should be done faster, probably not with an array
psTrait* CacheManager::GetTraitByName(const char* name)
{
    /*
        int i;
        psTrait *currenttrait;

        for (i=0;i<traitlist.GetSize();i++)
        {
            currenttrait=traitlist.Get(i);
            if (currenttrait!=NULL && currenttrait->name == name)
                return currenttrait;
        }
        */
    return NULL;
}

CacheManager::TraitIterator CacheManager::GetTraitIterator()
{
    return traitlist.GetIterator();
}

PSCHARACTER_GENDER CacheManager::ConvertGenderString(const char* genderstring)
{
    if(genderstring==NULL)
        return PSCHARACTER_GENDER_NONE;

    switch(genderstring[0])
    {
        case 'M':
        case 'm':
            return PSCHARACTER_GENDER_MALE;
        case 'F':
        case 'f':
            return PSCHARACTER_GENDER_FEMALE;
        case 'N':
        case 'n':
            return PSCHARACTER_GENDER_NONE;
    };

    return PSCHARACTER_GENDER_NONE;
}

bool CacheManager::PreloadRaceInfo()
{
    unsigned int currentrow;
    Result result(db->Select("SELECT * from race_info"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >race_info<");
        return false;
    }


    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        psRaceInfo* newraceinfo = new psRaceInfo;

        if(newraceinfo->Load(result[currentrow]))
        {
            // Do not load the BaseSpeeds for the Special raceinfo
            // that allow imaginary NPCs without race to be created.
            if(strcmp(newraceinfo->GetName(),"Special")!=0)
            {
                if(!newraceinfo->LoadBaseSpeeds(psserver->GetObjectReg()))
                {
                    Error2("Could not load base speeds for raceinfo %s.",newraceinfo->GetName());
                    delete newraceinfo;
                    return false;
                }
            }

            raceinfolist.Push(newraceinfo);
        }
        else
        {
            Error1("Could not load raceinfo. Check >race_info<");
            delete newraceinfo;
            return false;
        }
    }

    Notify2(LOG_STARTUP, "%lu Races Loaded", result.Count());
    return true;
}

size_t CacheManager::GetRaceInfoCount()
{
    return raceinfolist.GetSize();
}

psRaceInfo* CacheManager::GetRaceInfoByIndex(int idx)
{
    if(idx<0 || (size_t)idx>=raceinfolist.GetSize())
    {
        return NULL;
    }

    return raceinfolist.Get(idx);
}

// TODO: This should be done faster, probably not with an array
psRaceInfo* CacheManager::GetRaceInfoByID(unsigned int id)
{
    size_t i;
    psRaceInfo* currentri;

    for(i=0; i<raceinfolist.GetSize(); i++)
    {
        currentri=raceinfolist.Get(i);
        if(currentri && currentri->uid==id)
        {
            return currentri;
        }
    }
    return NULL;
}

// TODO: This should be done faster, probably not with an array
psRaceInfo* CacheManager::GetRaceInfoByNameGender(const char* name,PSCHARACTER_GENDER gender)
{
    size_t i;
    psRaceInfo* currentri;
    csString raceName(name);

    for(i=0; i<raceinfolist.GetSize(); i++)
    {
        currentri=raceinfolist.Get(i);
        if(currentri!=NULL && currentri->gender==gender && raceName.CompareNoCase(currentri->GetName()))
        {
            return currentri;
        }
    }
    return NULL;
}

// TODO: This should be done faster, probably not with an array
psRaceInfo* CacheManager::GetRaceInfoByNameGender(unsigned int id, PSCHARACTER_GENDER gender)
{
    size_t i;
    psRaceInfo* currentri;

    for(i=0; i<raceinfolist.GetSize(); i++)
    {
        currentri=raceinfolist.Get(i);
        // If the current race matches the race we're looking for, and only has the 'none' gender,
        // it's the race we need.
        if(currentri!=NULL &&
                currentri->race==id &&
                (currentri->gender == PSCHARACTER_GENDER_NONE || currentri->gender==gender))
        {
            return currentri;
        }
    }
    return NULL;
}

psRaceInfo* CacheManager::GetRaceInfoByMeshName(const csString &meshname)
{
    size_t i;
    psRaceInfo* currentri;

    for(i=0; i<raceinfolist.GetSize(); i++)
    {
        currentri=raceinfolist.Get(i);
        if(currentri!=NULL && currentri->mesh_name==meshname)
        {
            return currentri;
        }
    }
    return NULL;
}

psItemCategory* CacheManager::GetItemCategoryByID(unsigned int id)
{
    size_t i;
    for(i=0; i<itemCategoryList.GetSize(); i++)
    {
        psItemCategory* currentCategory;
        currentCategory=itemCategoryList.Get(i);
        if(currentCategory && id==currentCategory->id)
        {
            return currentCategory;
        }
    }
    return NULL;

}

psItemCategory* CacheManager::GetItemCategoryByName(const csString &name)
{
    size_t i;
    for(i=0; i<itemCategoryList.GetSize(); i++)
    {
        psItemCategory* currentCategory;
        currentCategory=itemCategoryList.Get(i);
        if(currentCategory && name==currentCategory->name)
        {
            return currentCategory;
        }
    }
    return NULL;

}

psAttackType *CacheManager::GetAttackTypeByID(unsigned int id)
{
    size_t i;
    for (i=0;i<wayList.GetSize();i++)
    {
        psAttackType *currentType;
        currentType=attackTypeList.Get(i);
        if (currentType && id==currentType->id)
        {
            return currentType;
        }
    }
    return NULL;
}

psAttackType *CacheManager::GetAttackTypeByName(csString name)
{
    size_t i;
    for (i=0;i<attackTypeList.GetSize();i++)
    {
        psAttackType *currentType;
        currentType=attackTypeList.Get(i);
        if (currentType && name.Upcase()==currentType->name.Upcase())
        {
            return currentType;
        }
    }
    return NULL;
}

psWeaponType *CacheManager::GetWeaponTypeByID(unsigned int id)
{
    size_t i;
    for (i=0;i<weaponTypeList.GetSize();i++)
    {
        psWeaponType *currentType;
        currentType=weaponTypeList.Get(i);
        if (currentType && id==currentType->id)
        {
            return currentType;
        }
    }
    return NULL;
}

psWeaponType *CacheManager::GetWeaponTypeByName(csString name)
{
    size_t i;
    for (i=0;i<weaponTypeList.GetSize();i++)
    {
        psWeaponType *currentType;
        currentType=weaponTypeList.Get(i);
        if (currentType && name.CompareNoCase(currentType->name))
        {
            return currentType;
        }
    }
    return NULL;
}

// TODO:  This function needs to be implemented in a fast fashion
psWay* CacheManager::GetWayByID(unsigned int id)
{
    size_t i;
    for(i=0; i<wayList.GetSize(); i++)
    {
        psWay* currentWay;
        currentWay=wayList.Get(i);
        if(currentWay && id==currentWay->id)
        {
            return currentWay;
        }
    }
    return NULL;
}

// TODO:  This function needs to be implemented in a fast fashion
psWay* CacheManager::GetWayByName(const csString &name)
{
    size_t i;
    for(i=0; i<wayList.GetSize(); i++)
    {
        psWay* currentWay;
        currentWay=wayList.Get(i);
        if(currentWay && name==currentWay->name)
        {
            return currentWay;
        }
    }
    return NULL;
}

Faction* CacheManager::GetFaction(const char* name)
{
    return factions.Get(name, NULL);
}

Faction* CacheManager::GetFactionByName(const char* name)
{
    if(name==NULL)
        return NULL;

    csHash<Faction*, int>::GlobalIterator it(factions_by_id.GetIterator());
    while(it.HasNext())
    {
        Faction* faction = it.Next();
        if(!strcasecmp(faction->name, name))
            return faction;
    }
    return NULL;
}

Faction* CacheManager::GetFaction(int id)
{
    return factions_by_id.Get(id,0);
}

ProgressionScript* CacheManager::GetProgressionScript(const char* name)
{
    if(!name)
        return NULL;
    return scripts.Get(name, NULL);
}

// TODO:  This function needs to be implemented in a fast fashion
psSpell* CacheManager::GetSpellByID(unsigned int id)
{
    size_t i;
    for(i=0; i<spellList.GetSize(); i++)
    {
        psSpell* currentSpell;
        currentSpell=spellList.Get(i);
        if(currentSpell && id==(unsigned int)currentSpell->GetID())
            return currentSpell;
    }
    return NULL;
}

// TODO:  This function needs to be implemented in a fast fashion
psSpell* CacheManager::GetSpellByName(const csString &name)
{
    size_t i;
    for(i=0; i<spellList.GetSize(); i++)
    {
        psSpell* currentSpell;
        currentSpell=spellList.Get(i);
        if(currentSpell && name.CompareNoCase(currentSpell->GetName()))
            return currentSpell;
    }
    return NULL;
}

CacheManager::SpellIterator CacheManager::GetSpellIterator()
{
    return spellList.GetIterator();
}


psItemStats* CacheManager::GetBasicItemStatsByName(csString name)
{
    psItemStats* itemstats = itemStats_NameHash.Get(name.Downcase(),NULL);
    if(itemstats)
    {
        return itemstats;
    }

    // Not found in cache so look it up in DB.
    bool loaded;
    csString escape;
    db->Escape(escape, name);

    Result result(db->Select("SELECT * from item_stats where stat_type in ('B','U','R') and name='%s'", (const char*) name));

    if(!result.IsValid() || result.Count() == 0)
    {
        return NULL;
    }

    itemstats = new psItemStats;
    loaded = itemstats->ReadItemStats(result[0]);

    // Prevent id conflicts
    if(!loaded|| itemStats_IDHash.Get(itemstats->GetUID(),NULL))
    {
        if(loaded)
        {
            CPrintf(CON_ERROR, "Duplicate item_stats ID where id='%u' found.\n", itemstats->GetUID());
        }

        delete itemstats;
        return NULL;
    }
    else
    {
        CS_ASSERT(itemstats->GetUID() != 0);

        itemStats_IDHash.Put(itemstats->GetUID(),itemstats);
        itemStats_NameHash.Put(itemstats->GetDownCaseName(),itemstats);
    }
    return itemstats;
}

uint32 CacheManager::BasicItemStatsByNameExist(csString name)
{
    psItemStats* itemstats = itemStats_NameHash.Get(name.Downcase(), NULL);
    if(itemstats)
    {
        return itemstats->GetUID();
    }

    return 0;
}

psItemStats* CacheManager::GetBasicItemStatsByID(uint32 id)
{
    if(id == 0)
    {
        return NULL;
    }

    psItemStats* itemstats = itemStats_IDHash.Get(id,NULL);
    if(itemstats)
    {
        return itemstats;
    }

    Result result(db->Select("SELECT * from item_stats where stat_type in ('B','U','R')  and id='%u'", id));

    if(!result.IsValid() || result.Count() == 0)
    {
        return NULL;
    }

    itemstats = new psItemStats;
    bool loaded = itemstats->ReadItemStats(result[0]);
    // Prevent name conflicts
    if(!loaded || itemStats_NameHash.Get(itemstats->GetDownCaseName(),NULL))
    {
        if(loaded)
        {
            CPrintf(CON_ERROR, "Duplicate item_stats name where name='%s' found.\n", itemstats->GetName());
        }
        delete itemstats;
        return NULL;
    }
    else
    {
        CS_ASSERT(itemstats->GetUID() != 0);

        itemStats_IDHash.Put(itemstats->GetUID(),itemstats);
        itemStats_NameHash.Put(itemstats->GetDownCaseName(),itemstats);
    }
    return itemstats;
}

void CacheManager::CacheNameChange(csString oldName, csString newName)
{
    if(oldName.Length() > 0 && newName.Length() > 0)
    {
        // first remove old name
        psItemStats* itemstats = itemStats_NameHash.Get(oldName.Downcase(), NULL);
        if(itemstats)
        {
            itemStats_NameHash.Delete(oldName.Downcase(), itemstats);

            // add new one
            itemStats_NameHash.Put(newName.Downcase(), itemstats);
        }
        else
        {
            Error2("CacheNameChange error: no item stats for \"%s\".", oldName.GetDataSafe());
        }
    }
    else
    {
        Error1("CacheNameChange error: old or new name zero length.");
    }
}

void CacheManager::GetTipByID(int id, csString &tip)
{
    if((size_t)id>=tips_list.GetSize())
    {
        tip.Clear();
        return;
    }

    tip = tips_list.Get(id);
}

unsigned int CacheManager::GetTipLength()
{
    return (unsigned int)tips_list.GetSize();
}

//this is used only by the char creation!
const char* CacheManager::Attribute2String(PSITEMSTATS_STAT s)
{
    switch(s)
    {
        case PSITEMSTATS_STAT_STRENGTH:
            return "Strength";
        case PSITEMSTATS_STAT_AGILITY:
            return "Agility";
        case PSITEMSTATS_STAT_ENDURANCE:
            return "Endurance";
        case PSITEMSTATS_STAT_INTELLIGENCE:
            return "Intelligence";
        case PSITEMSTATS_STAT_WILL:
            return "Will";
        case PSITEMSTATS_STAT_CHARISMA:
            return "Charisma";
        default:
            return "None";
    }
}

PSITEMSTATS_STAT CacheManager::ConvertAttributeString(const char* attributestring)
{
    if(attributestring==NULL)
        return PSITEMSTATS_STAT_NONE;

    if(!strcasecmp(attributestring,"STRENGTH") || !strcasecmp(attributestring,"STR"))
        return PSITEMSTATS_STAT_STRENGTH;
    if(!strcasecmp(attributestring,"AGILITY") || !strcasecmp(attributestring,"AGI"))
        return PSITEMSTATS_STAT_AGILITY;
    if(!strcasecmp(attributestring,"ENDURANCE") || !strcasecmp(attributestring,"END"))
        return PSITEMSTATS_STAT_ENDURANCE;
    if(!strcasecmp(attributestring,"INTELLIGENCE") || !strcasecmp(attributestring,"INT"))
        return PSITEMSTATS_STAT_INTELLIGENCE;
    if(!strcasecmp(attributestring,"WILL") || !strcasecmp(attributestring,"WIL"))
        return PSITEMSTATS_STAT_WILL;
    if(!strcasecmp(attributestring,"CHARISMA") || !strcasecmp(attributestring,"CHA"))
        return PSITEMSTATS_STAT_CHARISMA;
    return PSITEMSTATS_STAT_NONE;
}

PSSKILL CacheManager::ConvertSkillString(const char* skillstring)
{
    if(skillstring==NULL)
        return PSSKILL_NONE;

    psSkillInfo* skillinfo=GetSkillByName(skillstring);

    if(skillinfo==NULL)
        return PSSKILL_NONE;
    return skillinfo->id;
}

PSSKILL CacheManager::ConvertSkill(int skill_id)
{
    if(skill_id >= (int)PSSKILL_NONE && skill_id < (int)GetSkillAmount())
    {
        return (PSSKILL)skill_id;
    }

    return PSSKILL_NONE;
}

bool CacheManager::PreloadItemCategories()
{
    Result categories(db->Select("SELECT * from item_categories"));

    if(categories.IsValid())
    {
        int i,count=categories.Count();

        for(i=0; i<count; i++)
        {
            psItemCategory* category = new psItemCategory;

            category->id                   = categories[i].GetInt("category_id");
            category->name                 = categories[i]["name"];
            category->repairToolStatId     = categories[i].GetInt("item_stat_id_repair_tool");
            const char* flag               = categories[i]["is_repair_tool_consumed"];
            category->repairToolConsumed   = (flag && flag[0]=='Y');
            category->repairSkillId        = categories[i].GetInt("skill_id_repair");
            category->repairDifficultyPct  = categories[i].GetInt("repair_difficulty_pct");
            category->identifySkillId      = categories[i].GetInt("identify_skill_id");
            category->identifyMinSkill     = categories[i].GetInt("identify_min_skill");

            itemCategoryList.Push(category);
        }
    }
    Notify2(LOG_STARTUP, "%lu Item Categories Loaded", categories.Count());
    return true;
}
bool CacheManager::PreloadWeaponTypes()
{
    Result types(db->Select("SELECT * from weapon_types"));

    if(types.IsValid())
    {
        int i, count = types.Count();
        for(i=0; i< count; i++)
        {
            psWeaponType * type = new psWeaponType;
            type->id            = atoi(types[i]["id"]);
            type->name          = types[i]["name"];
            type->skill         = (PSSKILL)atoi(types[i]["skill"]);
            weaponTypeList.Push(type);
        } 
    }

    Notify2( LOG_STARTUP, "%lu Weapon Types Loaded", types.Count());
    return true;
}
bool CacheManager::PreloadAttackTypes()
{
    Result types(db->Select("SELECT * from attack_types"));

    if( types.IsValid())
    {
        int i, count=types.Count();

        for(i=0; i < count; i++)
        {
            psAttackType * type = new psAttackType;
            type->id            = types[i].GetInt("id");
            type->name          = types[i]["name"];
            type->OneHand    = (bool)types[i]["onehand"];
            type->related_stat  = (PSSKILL)atoi(types[i]["stat"]);
            type->weapon        = types[i]["weaponName"];
            csString tempWTypes = types[i]["weaponType"]; 

            WordArray ids(tempWTypes);
            for(unsigned int i = 0; i < ids.GetCount(); i++)
            {
                psWeaponType* weapontype = GetWeaponTypeByID(atoi(ids[i]));
                if(!weapontype)
                {
                    weapontype = GetWeaponTypeByName(ids[i]);
                }
                if(weapontype)
                    type->weaponTypes.Push(weapontype);
            }

            attackTypeList.Push(type);
        }
    }

    Notify2( LOG_STARTUP, "%lu Attack Types Loaded", types.Count());
    return true;
}

bool CacheManager::PreloadWays()
{
    Result ways(db->Select("SELECT * from ways"));
    if(ways.IsValid())
    {
        int i,count=ways.Count();

        for(i=0; i<count; i++)
        {
            psWay* way = new psWay;
            way->id =   atoi(ways[i]["id"]);
            way->name = ways[i]["name"];
            way->skill = (PSSKILL)atoi(ways[i]["skill"]);
            way->related_stat_skill = (PSSKILL)atoi(ways[i]["related_stat"]);
            wayList.Push(way); 
        }
    }

    Notify2(LOG_STARTUP, "%lu Ways Loaded", ways.Count());
    return true;
}

void CacheManager::PreloadFactionCharacterEvents(const char* script, Faction* faction)
{
    csString factionCharacterEvents = script;
    while(factionCharacterEvents.Length())
    {
        //get the value from the string
        size_t cutpos = factionCharacterEvents.FindFirst(' ', 0);
        if(cutpos == (size_t) -1) //script error or finished parsing bail out
            break;
        size_t cutpos2 = factionCharacterEvents.FindFirst('\n', 0);
        if(cutpos2 == (size_t) -1) //script error or finished parsing bail out
            break;

        //get the value of the faction which will trigger the text
        int value = atoi(factionCharacterEvents.Slice(0,cutpos).GetDataSafe());

        //take out the string we need
        csString entry_text = factionCharacterEvents.Slice(cutpos+1, cutpos2-cutpos-1);

        //prepare the struct containing the parsed data
        FactionLifeEvent factionevt;
        factionevt.event_description = entry_text;
        factionevt.value = abs(value); //we want a positive value for checking later
        if(value >= 0) //check what cshash should get the parsed data
            faction->PositiveFactionEvents.PushSmart(factionevt); //this was a positive value
        if(value <= 0)
            faction->NegativeFactionEvents.PushSmart(factionevt); //this was a negative value

        //prepare for the next line
        factionCharacterEvents = factionCharacterEvents.Slice(cutpos2+1);
    }
    //sort the entries. It's needed to have a faster runtime access.
    faction->PositiveFactionEvents.Sort();
    faction->NegativeFactionEvents.Sort();
}

bool CacheManager::PreloadFactions()
{
    Result result_factions(db->Select("SELECT * from factions"));

    unsigned int x = 0;

    if(result_factions.IsValid())
    {
        for(x = 0; x < result_factions.Count(); x++)
        {
            Faction* f = new Faction;
            f->id = atoi(result_factions[x]["id"]);
            f->name = result_factions[x]["faction_name"];
            f->description = result_factions[x]["faction_description"];
            f->weight = atof(result_factions[x]["faction_weight"]);

            // Parses the script for factions dynamic life events and populates the arrays for it
            // used to store the script to be parsed to generate dynamic life events
            PreloadFactionCharacterEvents(result_factions[x]["faction_character"], f);

            // Stored two different ways
            factions.Put(f->name, f);
            factions_by_id.Put(f->id,f);
        }
    }

    Notify2(LOG_STARTUP, "%lu Factions Loaded", result_factions.Count());
    return true;
}

bool CacheManager::PreloadScripts(EntityManager* entitymanager)
{
    Result result(db->Select("SELECT * from progression_events"));

    if(result.IsValid())
    {
        for(size_t i = 0; i < result.Count(); i++)
        {
            ProgressionScript* s = ProgressionScript::Create(entitymanager, this, result[i]["name"], result[i]["event_script"]);
            if(!s)
            {
                Error2("Couldn't load script %s\n", result[i]["name"]);
                Error2("Script Data was:\n%s\n", result[i]["event_script"]);
                return false;
            }
            scripts.Put(s->Name(), s);
        }
    }
    return true;
}

#define LOOKUP(var, name) \
    var = eng->FindScript(name); \
    if(!var) \
    { \
        Error1("Failed to find MathScript >" name "<!"); \
        return false; \
    }

bool CacheManager::PreloadMathScripts()
{
    MathScriptEngine* eng = psserver->GetMathScriptEngine();
    LOOKUP(maxCarryWeight, "CalculateMaxCarryWeight");
    LOOKUP(maxCarryAmount, "CalculateMaxCarryAmount");
    LOOKUP(maxRealmScript, "MaxRealm");
    LOOKUP(maxManaScript, "CalculateMaxMana");
    LOOKUP(maxHPScript, "CalculateMaxHP");
    LOOKUP(staminaCalc, "StaminaBase");
    LOOKUP(expSkillCalc, "Calculate Skill Experience");
    LOOKUP(staminaRatioWalk, "StaminaRatioWalk");
    LOOKUP(staminaRatioStill, "StaminaRatioStill");
    LOOKUP(staminaRatioSit, "StaminaRatioSit");
    LOOKUP(staminaRatioWork, "StaminaRatioWork");
    LOOKUP(staminaCombat, "StaminaCombat");
    LOOKUP(dodgeValueCalc, "CalculateDodgeValue");
    LOOKUP(armorSkillsPractice, "PracticeArmorSkills");
    LOOKUP(charLevelGet, "GetCharLevel");
    LOOKUP(skillValuesGet, "GetSkillValues");
    LOOKUP(baseSkillValuesGet, "GetSkillBaseValues");
    LOOKUP(setBaseSkillsScript, "SetBaseSkills");
    LOOKUP(calc_decay, "Calculate Decay");
    LOOKUP(calcItemPrice, "Calc Item Price");
    LOOKUP(calcItemSellPrice, "Calc Item Sell Price");
    LOOKUP(playerSketchLimits, "Calc Player Sketch Limits");
    LOOKUP(spellPowerLevel, "CalculatePowerLevel");
    LOOKUP(manaCost, "CalculateManaCost");
    LOOKUP(castSuccess, "CalculateChanceOfCastSuccess");
    LOOKUP(researchSuccess, "CalculateChanceOfResearchSuccess");
    LOOKUP(spellPractice, "SpellPractice");
    LOOKUP(doDamage, "DoDamageScript");
    LOOKUP(staminaMove, "StaminaMove");
    LOOKUP(msAffinity, "CalculateFamiliarAffinity");
    return true;
}

bool CacheManager::PreloadSpells()
{
    Result spells(db->Select("SELECT * from spells"));
    if(spells.IsValid())
    {
        int i,count=spells.Count();

        for(i=0; i<count; i++)
        {
            psSpell* spell = new psSpell;
            if(spell->Load(spells[i]))
            {
                spellList.Push(spell);
            }
            else
            {
                delete spell;
            }
        }
    }
    Notify2(LOG_STARTUP, "%lu Spells Loaded", spells.Count());
    return true;
}

csPDelArray<psItemAnimation>* CacheManager::FindAnimationList(int id)
{
    for(size_t x=0; x<item_anim_list.GetSize(); x++)
    {
        if(item_anim_list[x]->Get(0)->id == id)
        {
            return item_anim_list[x];
        }
    }
    return NULL;
}

bool CacheManager::PreloadItemAnimList()
{
    unsigned int currentrow,lastarrayid=0;
    psItemAnimation* newitem;
    csPDelArray<psItemAnimation>* newarray;

    Result result(db->Select("SELECT * from item_animations order by id, min_use_level"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >item_animations<");
        return false;
    }

    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        unsigned int id = result[currentrow].GetInt("id");
        if(id != lastarrayid)  // new set
        {
            newarray = new csPDelArray<psItemAnimation>;
            item_anim_list.Push(newarray);
            lastarrayid = id;
        }
        newitem            = new psItemAnimation;
        newitem->id        = id;
        newitem->anim_name = result[currentrow]["cstr_animation"];
        if(!newitem->anim_name)
        {
            Error2("Invalid 'cstr_animation' for item anim '%u'\n", id);
        }
        newitem->anim_id   = FindCommonStringID(newitem->anim_name);
        newitem->flags     = result[currentrow].GetInt("type_flags");
        newitem->min_level_required = result[currentrow].GetInt("min_use_level");

        newarray->Push(newitem);
    }

    Notify2(LOG_STARTUP, "%lu Item Animations Loaded", result.Count());
    return true;
}

bool CacheManager::LoadWorldItems(psSectorInfo* sector, csArray<psItem*> &items)
{
    /* We have 6 sets of items in game:
     * - items on the ground (loc_sector_id!=0)
     * - containers on the ground (loc_sector_id!=0)
     * - items inside containers on the ground (parent_item_id object has loc_sector_id!=0)
     *
     * - items inside player inventory (location='I' or location='E')
     * - containers in player inventory (location='I' or location='E')
     * - items inside containers in player inventory (parent_item_id object has location='I' or location='E')
     *
     * Here we want to load only the first 3 sets
     */

    Result result(sector ?
                  db->Select("SELECT * from item_instances where loc_sector_id='%u'",sector->uid) :
                  // it's an object on the ground, not held by players
                  // or the parent container is an object on the ground, not held by players
                  db->Select("SELECT * from item_instances"
                             " where ifnull(loc_sector_id,0)!=0"
                             " or parent_item_id IN (select id "
                             " from item_instances "
                             "where ifnull(loc_sector_id,0)!=0) order by parent_item_id"));

    // Load items
    if(!result.IsValid())
    {
        Error3("Error loading world items.\nQuery: %s\nError: %s",db->GetLastQuery(), db->GetLastError());
        return false;
    }
    for(unsigned long i = 0; i < result.Count(); i++)
    {
        psItem* item;
        unsigned int stats_id=result[i].GetUInt32("item_stats_id_standard");
        psItemStats* stats= GetBasicItemStatsByID(stats_id);

        if(!stats)
        {
            Error3("Error in LoadWorldItems! Item instance %lu could not load base stats %lu. Check the item_stats_id_standard value for this item. Skipping.\n",
                   result[i].GetUInt32("id"), result[i].GetUInt32("item_stats_id_standard"));
            continue;
        }

        item = psCharacterInventory::GetItemFactory(stats);

        if(!item->Load(result[i]))
        {
            Error2("Error in LoadWorldItems! Item instance %lu could not be loaded. Skipping.\n",result[i].GetUInt32("id"));
            delete item;
            continue;
        }

        items.Push(item);
    }

    return true;
}

void CacheManager::AddItemStatsToHashTable(psItemStats* newitem)
{
    itemStats_IDHash.Put(newitem->GetUID(),newitem);
    itemStats_NameHash.Put(newitem->GetDownCaseName(),newitem);
}

bool CacheManager::PreloadItemStatsDatabase()
{
    uint32 currentrow;
    psItemStats* newitem;
    Result result(db->Select("SELECT * from item_stats where stat_type in ('B','U','R') "));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >item_stats<");
        return false;
    }

    for(currentrow=0; currentrow<result.Count(); currentrow++)
    {
        newitem = new psItemStats;
        bool loaded = newitem->ReadItemStats(result[currentrow]);
        // Prevent name conflicts
        if(!loaded || itemStats_NameHash.Get(newitem->GetDownCaseName(), NULL))
        {
            if(loaded)
            {
                CPrintf(CON_ERROR, "Duplicate item_stats name where name='%s' found.\n", newitem->GetName());
            }
            else
            {
                CPrintf(CON_ERROR, "Failed to load item_stats with id='%s' and name='%s'\n",
                        result[currentrow]["id"],result[currentrow]["name"]);
            }

            delete newitem;
            return false;
        }
        else
        {
            CS_ASSERT(newitem->GetUID() != 0);

            AddItemStatsToHashTable(newitem);
        }
    }
    Notify2(LOG_STARTUP, "%lu Item Stats Loaded", result.Count());
    return true;
}

psItemStats* CacheManager::CopyItemStats(uint32 id, csString newName)
{
    // NOTE: This -must- match the schema of item_stats, except for UNIQUE KEY 'name' which is handled below.
    const char* fields = "stat_type, weight, visible_distance, size, container_max_size, valid_slots, flags, decay_rate, item_skill_id_1, item_skill_id_2, item_skill_id_3, item_bonus_1_attr, item_bonus_2_attr, item_bonus_3_attr, item_bonus_1_max, item_bonus_2_max, item_bonus_3_max, dmg_slash, dmg_blunt, dmg_pierce, weapon_speed, weapon_penetration, weapon_block_targeted, weapon_block_untargeted, weapon_counterblock, armor_hardness, cstr_gfx_mesh, cstr_gfx_icon, cstr_gfx_texture, cstr_part, cstr_part_mesh, armorvsweapon_type, category_id, base_sale_price, item_type, requirement_1_name, requirement_1_value, requirement_2_name, requirement_2_value, requirement_3_name, requirement_3_value, item_type_id_ammo, spell_id_on_hit, spell_on_hit_prob, spell_id_feature, spell_feature_charges, spell_feature_timing, item_anim_id, description, sound, item_max_quality, equip_script, consume_script, creative_definition, max_charges, weapon_range";

    if(db->Command("INSERT INTO item_stats (%s) SELECT %s FROM item_stats WHERE id = %u", fields, fields, id) == QUERY_FAILED)
    {
        Error4("Error while copying item stats for id %d.\nSQL:%s\nError:%s\n",id,db->GetLastQuery(),db->GetLastError());
        return NULL;
    }

    id = db->GetLastInsertID();
    CS_ASSERT(id != 0);
    csString escape;
    db->Escape(escape, newName);

    // change to the new name for the new item
    if(db->Command("UPDATE item_stats SET name = \'%s\' WHERE id = \'%u\'", escape.GetDataSafe(), id) == QUERY_FAILED)
    {
        Error4("Error while renaming copied item_stats for id %d.\nSQL:%s\nError:%s\n", id, db->GetLastQuery(), db->GetLastError());
        db->Command("DELETE FROM item_stats WHERE id = \'%u\'", id);
        return NULL;
    }

    Result result(db->Select("SELECT * from item_stats where id='%u'", id));

    if(!result.IsValid())
    {
        return NULL;
    }

    if(result.Count() != 1)
    {
        return NULL;
    }

    psItemStats* newItem = new psItemStats;
    if(!newItem->ReadItemStats(result[0]))
    {
        delete newItem;
        return NULL;
    }
//    basicitemstatslist.Push( newItem );
    itemStats_IDHash.Put(newItem->GetUID(),newItem);
    itemStats_NameHash.Put(newName.Downcase(), newItem);
    return newItem;
}

bool CacheManager::PreloadBadNames()
{
    Result result(db->Select("SELECT * from bad_names"));

    if(!result.IsValid())
    {
        Error1("Could not cache database table. Check >bad_names<");
        return false;
    }

    for(unsigned int i = 0; i < result.Count(); i++)
    {
        bad_names.Push(NormalizeCharacterName(result[i]["name"]));
    }

    Notify2(LOG_STARTUP, "%lu Bad Names Loaded", result.Count());
    return true;
}

void CacheManager::AddBadName(const char* name)
{
    if(!name)
        return;

    csString newname = NormalizeCharacterName(name);

    db->Command("INSERT INTO bad_names (`name`) VALUES ('%s')", newname.GetDataSafe());

    bad_names.Push(newname);
}

void CacheManager::DelBadName(const char* name)
{
    csString cname = NormalizeCharacterName(name);

    for(size_t i = 0; i < bad_names.GetSize(); i++)
    {
        if(cname.CompareNoCase(bad_names[i]))
        {
            bad_names.DeleteIndex(i);
            db->Command("DELETE FROM bad_names WHERE name='%s'",cname.GetData());
            return;
        }
    }
}

size_t CacheManager::GetBadNamesCount()
{
    return bad_names.GetSize();
}

const char* CacheManager::GetBadName(int pos)
{
    return bad_names[pos];
}

psAccountInfo* CacheManager::GetAccountInfoByID(AccountID accountid)
{
    Result result(db->Select("SELECT * from accounts where id=%u", accountid.Unbox()));
    if(!result.IsValid() || result.Count()<1)
    {
        Warning3(LOG_CONNECTIONS, "Could not find %s: %s", ShowID(accountid), db->GetLastError());
        return NULL;
    }
    psAccountInfo* accountinfo=new psAccountInfo;
    if(accountinfo->Load(result[0]))
    {
        return accountinfo;
    }
    else
    {
        delete accountinfo;
        return NULL;
    }
}

psAccountInfo* CacheManager::GetAccountInfoByCharID(PID charid)
{
    unsigned int accountid = db->SelectSingleNumber("SELECT account_id from characters where id=%u", charid.Unbox());
    return GetAccountInfoByID(accountid);
}

psAccountInfo* CacheManager::GetAccountInfoByUsername(const char* username)
{
    iCachedObject* obj = RemoveFromCache(username);
    if(obj)
    {
        Notify2(LOG_CACHE, "Found account for %s in cache!", username);
        return (psAccountInfo*)obj->RecoverObject();
    }

    csString escape;
    db->Escape(escape, username);
    Result result(db->Select("SELECT * from accounts where username='%s'",escape.GetData()));
    if(!result.IsValid() || result.Count()<1)
    {
        Warning3(LOG_CONNECTIONS,"Could not find account for login %s.  Error: %s",username,db->GetLastError());
        return NULL;
    }
    psAccountInfo* accountinfo=new psAccountInfo;
    if(accountinfo->Load(result[0]))
    {
        return accountinfo;
    }
    else
    {
        delete accountinfo;
        return NULL;
    }
}

bool CacheManager::UpdateAccountInfo(psAccountInfo* ainfo)
{
    const char* fieldnames[]=
    {
        "last_login_ip",
        "security_level",
        "last_login",
        "operating_system",
        "graphics_card",
        "graphics_version"
    };
    char accountidstring[11];
    psStringArray fields;

    fields.Push(ainfo->lastloginip);
    fields.FormatPush("%d",ainfo->securitylevel);
    fields.Push(ainfo->lastlogintime);
    fields.Push(ainfo->os);
    fields.Push(ainfo->gfxcard);
    fields.Push(ainfo->gfxversion);
    snprintf(accountidstring,11,"%u",ainfo->accountid);
    accountidstring[10]=0x00;

    if(!db->GenericUpdateWithID("accounts","id",accountidstring,fieldnames,fields))
    {
        Error3("Failed to update account %u. Error %s",ainfo->accountid,db->GetLastError());
        return false;
    }
    return true;
}

unsigned int CacheManager::NewAccountInfo(psAccountInfo* ainfo)
{
    const char* fieldnames[]=
    {
        "username",
        "password",
        "last_login_ip",
        "security_level",
        "status"
    };
    psStringArray fields;

    fields.Push(ainfo->username);
    fields.Push(ainfo->password);
    fields.Push(ainfo->lastloginip);
    fields.FormatPush("%d",ainfo->securitylevel);
    fields.Push("A"); //created from server console so auto enabled

    unsigned int id=db->GenericInsertWithID("accounts",fieldnames,fields);

    if(id == 0)
    {
        Error3("Failed to create new account for user %s. Error %s",ainfo->username.GetData(),db->GetLastError());
        return false;
    }

    ainfo->accountid = id;
    return id;
}

psGuildInfo* CacheManager::FindGuild(unsigned int id)
{
    psGuildInfo* g = guildinfo_by_id.Get(id, 0);
    if(g)
    {
        return g;
    }

    // Load on demand if not found
    g = new psGuildInfo;
    if(g->Load(id))
    {
        guildinfo_by_id.Put(g->GetID(), g);
        return g;
    }
    delete g;
    return NULL;
}

psGuildInfo* CacheManager::FindGuild(const csString &name)
{
    csHash<psGuildInfo*>::GlobalIterator gIter = guildinfo_by_id.GetIterator();
    while(gIter.HasNext())
    {
        psGuildInfo* guild = gIter.Next();
        if(guild->GetName() == name)
        {
            return guild;
        }
    }

    psGuildInfo* g = new psGuildInfo;
    if(g->Load(name))
    {
        guildinfo_by_id.Put(g->GetID(), g);
        return g;
    }
    delete g;
    return NULL;
}

bool CacheManager::CreateGuild(const char* guildname, Client* client)
{
    PID leaderID = client->GetPID();

    psGuildInfo* gi = new psGuildInfo(guildname,leaderID);

    if(!gi->InsertNew())
    {
        delete gi;
        return false;
    }

    gi->AddNewMember(client->GetCharacterData(), MAX_GUILD_LEVEL);

    guildinfo_by_id.Put(gi->GetID(),gi);
    return true;
}

void CacheManager::RemoveGuild(psGuildInfo* which)
{
    guildinfo_by_id.Delete(which->GetID(),which);
    delete which;
}

psGuildAlliance* CacheManager::FindAlliance(unsigned int id)
{
    psGuildAlliance* a = alliance_by_id.Get(id, 0);
    if(a)
    {
        return a;
    }

    // Load on demand if not found
    a = new psGuildAlliance();
    if(a->Load(id))
    {
        alliance_by_id.Put(id,a);
        return a;
    }
    delete a;
    return NULL;
}

bool CacheManager::CreateAlliance(const csString &name, psGuildInfo* founder, Client* client)
{
    psGuildAlliance* a = new psGuildAlliance(name);

    if(!a->InsertNew())
    {
        delete a;
        return false;
    }

    a->AddNewMember(founder);
    a->SetLeader(founder);

    alliance_by_id.Put(a->GetID(),a);
    return true;
}

bool CacheManager::RemoveAlliance(psGuildAlliance* which)
{
    alliance_by_id.Delete(which->GetID(),which);
    which->RemoveAlliance();
    delete which;
    return true;
}

float CacheManager::GetArmorVSWeaponResistance(const char* armor_type, const char* weapon_type)
{
    if(!weapon_type || !armor_type || strlen(armor_type) < 2)
        return 1.0f;

    // Loopide loop
    for(size_t i = 0; i < armor_vs_weapon.GetSize(); i++)
    {
        ArmorVsWeapon* atbl = armor_vs_weapon.Get(i);
        if(!atbl)
            continue;

        if(atbl->weapontype == weapon_type)
        {
            // Get the float in our 2d array
            csString armor = armor_type;
            char lvl = armor.GetAt(1);
            int arrayLvl = 0;

            int cat = atoi(csString(armor.GetAt(0)));
            if(lvl == 'a')
                arrayLvl = 0;
            else if(lvl =='b')
                arrayLvl = 1;
            else if(lvl == 'c')
                arrayLvl = 2;
            else
                arrayLvl = 3;

            return atbl->c[cat-1][arrayLvl];
        }
    }

    // Not found
    Debug2(LOG_COMBAT,0,"Didn't find weapon %s in the armor vs weapon table, assuming value is 1.0\n",weapon_type);
    return 1.0f;
}

float CacheManager::GetArmorVSWeaponResistance(psItemStats* armor, psItemStats* weapon)
{
    if(!armor || !weapon)
        return 1.0f;

    // Get the values to use
    csString armorstr,weaponstr;
    armor->GetArmorVsWeaponType(armorstr);
    weapon->GetArmorVsWeaponType(weaponstr);

    return GetArmorVSWeaponResistance(armorstr,weaponstr);
}

const char* CacheManager::MakeCacheName(const char* prefix, uint32 id)
{
    sprintf(CacheNameBuffer,"%.5s%u",prefix,id);
    return CacheNameBuffer;
}

void CacheManager::AddToCache(iCachedObject* obj, const char* name, int max_cache_time_seconds)
{
    Debug4(LOG_CACHE,0,"Now adding object <%s:%p> to cache for %d seconds.",name,obj,max_cache_time_seconds);

    // Set up the object which the event trigger will use to recover pointers or cancel itself.
    CachedObject* newRecord = new CachedObject;

    newRecord->name = name;
    newRecord->object = obj;

    // Now create the timed event which will make the cache clean it up later
    psCacheExpireEvent* evt = new psCacheExpireEvent(max_cache_time_seconds*1000, newRecord);

    // Setting this enables removal from cache to clear this event
    newRecord->event = evt;

    // Now queue it
    psserver->GetEventManager()->Push(evt);

    // Ensure no duplicate keys in generic cache, and clean up after self if found
    iCachedObject* oldObject = RemoveFromCache(newRecord->name);

    if(oldObject)
    {
        oldObject->ProcessCacheTimeout(); // Make sure object knows it is gone
        oldObject->DeleteSelf();
    }

    // Now add the new one back in
    generic_object_cache.Put(newRecord->name, newRecord);
}

iCachedObject* CacheManager::RemoveFromCache(const char* name)
{
    CachedObject* oldRecord = generic_object_cache.Get(name, NULL);
    if(oldRecord)
    {
        oldRecord->event->CancelEvent();
        generic_object_cache.DeleteAll(oldRecord->name);
        iCachedObject* save = oldRecord->object;
        delete oldRecord;
        Notify2(LOG_CACHE,"Found object in cache and returning ptr %p.",save);
        return save;
    }
    Debug2(LOG_CACHE,0,"Object <%s> not found in cache.",name);
    return NULL;
}

CacheManager::psCacheExpireEvent::psCacheExpireEvent(int delayticks,CachedObject* object)
    : psGameEvent(0,delayticks,"psCacheExpireEvent")
{
    valid = true;
    myObject = object;
}

void CacheManager::psCacheExpireEvent::Trigger()
{
    if(valid)
    {
        Debug2(LOG_CACHE,0,"Deleting object <%s> from cache due to timeout.",myObject->name.GetDataSafe());
        // Notify object it is going away
        myObject->object->ProcessCacheTimeout();
        // Delete the underlying object
        myObject->object->DeleteSelf();
        // Now remove the record from the cache last
        psserver->GetCacheManager()->RemoveFromCache(myObject->name);
    }
}
