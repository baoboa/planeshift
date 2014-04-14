/*
 * spawnmanager.h by Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */

#ifndef __SPAWNMANAGER_H__
#define __SPAWNMANAGER_H__

class gemObject;
class psCharacter;
class psDatabase;
class psScheduledItem;
class psSectorInfo;

#include <csgeom/vector3.h>
#include <csutil/hash.h>
#include "util/gameevent.h"
#include "msgmanager.h"
#include "lootrandomizer.h"


/**
 * \addtogroup server
 * @{ */

/**
 * This class is used to store respawn ranges for NPCs. They are intended
 * to be gathered into a SpawnRule.
 */
class SpawnRange
{
protected:
    csRandomGen* randomgen;

    /// Unique ID used for ident
    int   id;

    char  type;  /// A = Area (rect), L = Line Segment, C = Circle

    /// Corresponding spawn rule ID
    int   npcspawnruleid;

    /// Spawn sector name
    csString spawnsector;

private:
    /// Spawn range rectangular volume with corners (x1,y1,z1) and (x2,y2,z2)
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;
    float radius;

    /// Pre-computed area to avoid overhead
    float area;

    csVector3 PickWithRadius(csVector3 pos, float radius);

public:
    /// Ctor clears all to zero
    SpawnRange();

    /// Setup variables and compute area
    void Initialize(int idval,
                    int spawnruleid,
                    const char* type_code,
                    float rx1, float ry1, float rz1,
                    float rx2, float ry2, float rz2,
                    float radius,
                    const char* sectorname);

    void SetID(int idval)
    {
        id = idval;
    };
    int GetID()
    {
        return id;
    }

    /// Get range's XZ area
    float GetArea()
    {
        return area;
    };

    /// Get spawn sector name
    const csString &GetSector()
    {
        return spawnsector;
    }

    /// Randomly pick a position within the range
    const csVector3 PickPos();
};

class LootEntrySet;

/**
 * This class is used to store respawn rules for NPCs. They are loaded
 * from the database at startup and used only in RAM thereafter.
 */
class SpawnRule
{
protected:

    csRandomGen* randomgen;

    /// Unique Id used for ident and tree sorting
    int   id;

    /// Minimum respawn delay in ticks (msec)
    int   minspawntime;

    /// Maximum respawn delay in ticks
    int   maxspawntime;

    /// Odds of substitute spawn triggering instead of respawn of same entity
    float substitutespawnodds;

    /// Player id to respawn if substitution spawn odds are met
    PID   substituteplayer;

    /// If ranges are populated, these are ignored
    float    fixedspawnx;
    float    fixedspawny;
    float    fixedspawnz;
    float    fixedspawnrot;
    csString fixedspawnsector;
    InstanceID fixedinstance;
    float minSpawnSpacingDistance; ///< What should the free space be around before spawn position is accepted.

    /// Spawn ranges for the current rule
    csHash<SpawnRange*> ranges;

    /// Rules for generating loot when this npc is killed.
    LootEntrySet* loot;

    int dead_remain_time;

public:

    /// Ctor clears all to zero
    SpawnRule();
    ~SpawnRule();

    /// Setup variables
    void Initialize(int idval,
                    int minspawn,
                    int maxspawn,
                    float substodds,
                    int substplayer,
                    float x,float y,float z,float angle,
                    const char* sector,
                    LootEntrySet* loot_id,
                    int dead_time,
                    float minSpacing,
                    InstanceID instance);

    int  GetID()
    {
        return id;
    };
    void SetID(int idval)
    {
        id = idval;
    };

    /// Get random value between min and max spawn time, in ticks/msecs.
    int GetRespawnDelay();

    /// Determine if substitute player should be spawned.  Returns either original or substitute.
    PID CheckSubstitution(PID originalplayer);

    /**
     * Pick a spot for the entity to respawn.
     *
     * @return True if this is a random area rule, or false if it is a fixed position.
     */
    bool DetermineSpawnLoc(psCharacter* ch, csVector3 &pos, float &angle, csString &sectorname, InstanceID &instance);

    /// Add a spawn range to current rule
    void AddRange(SpawnRange* range);

    /// Get the Loot Rule set to generate loot
    LootEntrySet* GetLootRules()
    {
        return loot;
    }

    int GetDeadRemainTime()
    {
        return dead_remain_time;
    }

    int GetMinSpawnSpacingDistance()
    {
        return minSpawnSpacingDistance;
    }

};

class psItemStats;
class psCharacter;

/**
 * This class holds one loot possibility for a killed npc.
 * The npc_spawn_rule has a ptr to an array of these.
 */
struct LootEntry
{
    psItemStats* item;
    int   min_item; ///< The minimal amount of these items.
    int   max_item; ///< The maximum amount of these items.
    float probability;
    int   min_money;
    int   max_money;
    bool  randomize;
    float randomizeProbability; ///< The probability that if this item was picked it will be randomized.
};

/**
 * This class stores an array of LootEntry and calculates
 * required loot on a newly dead mob.
 */
class LootEntrySet
{
protected:
    int id;
    csArray<LootEntry*> entries;
    float total_prob;
    LootRandomizer* lootRandomizer;

    void CreateSingleLoot(psCharacter* chr);
    void CreateMultipleLoot(psCharacter* chr, size_t numModifiers = 0);

public:
    LootEntrySet(int idx, LootRandomizer* lr)
    {
        id=idx;
        total_prob=0;
        lootRandomizer=lr;
    }
    ~LootEntrySet();

    /// This adds another item to the entries array
    void AddLootEntry(LootEntry* entry);

    /**
     * This calculates the loot for the killed character, given the
     * current set of loot entries, and adds them to the
     * character as lootable inventory.
     *
     * @param character the character who has been killed
     * @param numModifiers max number of modifiers to use
     */
    void CreateLoot(psCharacter* character, size_t numModifiers = 0);
};

/** A structure to hold the clients that are pending a group loot question.
 */
class PendingLootPrompt;

/**
 *  This class is periodically called by the engine to ensure that
 *  monsters (and other NPCs) are respawned appropriately.
 */
class SpawnManager : public MessageManager<SpawnManager>
{
protected:
    psDatabase*             database;
    csHash<SpawnRule*>      rules;
    csHash<LootEntrySet*>   looting;
    LootRandomizer*         lootRandomizer;
    CacheManager*           cacheManager;
    EntityManager*          entityManager;
    GEMSupervisor*          gem;

    void HandleLootItem(MsgEntry* me,Client* client);
    void HandleDeathEvent(MsgEntry* me,Client* notused);

public:

    SpawnManager(psDatabase* db, CacheManager* cachemanager, EntityManager* entitymanager, GEMSupervisor* gemsupervisor);
    virtual ~SpawnManager();

    /**
     * Returns the loot randomizer.
     *
     * @return Returns a reference to the current loot randomizer.
     */
    LootRandomizer* GetLootRandomizer()
    {
        return lootRandomizer;
    }

    /**
     * Load all rules into memory for future use.
     */
    void PreloadDatabase();

    /**
     * Load all loot categories into the manager for use in spawn rules.
     */
    void PreloadLootRules();

#if 0
    /**
     * Load NPC Waypoint paths as named linear spawn ranges.
     */
    LoadWaypointsAsSpawnRanges(iDocumentNode* topNode);
#endif

    /**
     * Load all ranges for a rule.
     */
    void LoadSpawnRanges(SpawnRule* rule);

    /**
     * Load hunt location.
     *
     * @param sectorinfo The sector to load in. NULL means all sectors.
     */
    void LoadHuntLocations(psSectorInfo* sectorinfo = 0);

    /**
     * Used by LoadHuntLocations() to spawn the hunt locations in game.
     *
     * @param result the result set to use for the spawm
     * @param sectorinfo The sector to load in. NULL means all sectors.
     */
    void SpawnHuntLocations(Result &result, psSectorInfo* sectorinfo);

    /**
     * Called at server startup to create all creatures currently marked as
     * "living" in the database.
     *
     * This will restore the server to its last known NPC population if it crashes.
     *
     * @param sectorinfo The sector we want to repopulate.
     *                   If NULL then respawn all sectors.
     */
    void RepopulateLive(psSectorInfo* sectorinfo = 0);

    /**
     * Respawn a NPC given by playerID and find the position
     * from the spawnRule.
     */
    void Respawn(PID playerID, SpawnRule* spawnRule);

    /**
     * Respawn a NPC in the given position.
     */
    void Respawn(psCharacter* chardata, InstanceID instance, csVector3 &where, float rot, const char* sector);

    /**
     * Adds all items to the world.
     *
     * Called at the server startup to add all the items to the game.
     *
     * @param sectorinfo The sector to respawn the items into. If NULL it will
     *                   respawn all items in all sectors.
     */
    void RepopulateItems(psSectorInfo* sectorinfo = 0);

    /**
     * Sets the NPC as dead, plays the death animation and triggers the
     * loot generator.  Also queues the removal of the corpse.
     */
    void KillNPC(gemActor* npc, gemActor* killer);

    /**
     * Kills the specified NPC, updates the database that he is "dead"
     * and queues him for respawn according to the spawn rules.
     */
    void RemoveNPC(gemObject* obj);
};


/**
 * When an NPC or mob is killed in the spawn manager, its respawn event
 * is immediately created and added to the schedule to be triggered at the
 * appropriate time.
 */
class psRespawnGameEvent : public psGameEvent
{
protected:
    SpawnManager* spawnmanager;
    PID           playerID;    ///< The PID of the entity to respawn
    SpawnRule*    spawnRule;   ///< The rule to use for determine where

public:
    /**
     * Event to respawn given PID in a position to be found by the spawnRule.
     */
    psRespawnGameEvent(SpawnManager* mgr,
                       int delayticks,
                       PID playerID,
                       SpawnRule* spawnRule);

    virtual void Trigger();  // Abstract event processing function
};

/**
 * When an NPC or mob is killed in the spawn manager, its respawn event
 * is immediately created and added to the schedule to be triggered at the
 * appropriate time.
 */
class psDespawnGameEvent : public psGameEvent
{
protected:
    SpawnManager* spawnmanager;
    GEMSupervisor* gem;
    EID entity;

public:
    psDespawnGameEvent(SpawnManager* mgr,
                       GEMSupervisor* gemSupervisor,
                       int delayticks,
                       gemObject* obj);

    virtual void Trigger();  // Abstract event processing function
};

class psItemSpawnEvent : public psGameEvent
{
public:
    psItemSpawnEvent(psScheduledItem* item);
    virtual ~psItemSpawnEvent();

    void Trigger();  // Abstract event processing function

protected:
    psScheduledItem* schedule;
};

/**
 * Handles looting of an item, whether there is a group or not.
 *
 * @param item The item to loot (at best returned from RemoveLootItem)
 * @param obj The actor to loot the item from
 * @param client The client which is looting
 * @param cacheManager The CacheManager to use for the PendingLootPrompt
 * @param gem The GEMSupervisor to use for the PendingLootPrompt
 * @param lootAction Either 0 for LOOT_SELF or 1 for LOOT_ROLL
 */
void handleGroupLootItem(psItem* item, gemActor* obj, Client* client, CacheManager* cacheManager, GEMSupervisor* gem, uint8_t lootAction = 0);

/** @} */

#endif
