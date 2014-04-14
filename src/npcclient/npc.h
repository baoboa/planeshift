/*
* npc.h
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

/* This file holds definitions for ALL global variables in the planeshift
* server, normally you should move global variables into the psServer class
*/
#ifndef __NPC_H__
#define __NPC_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>

struct iMovable;

#include "util/psutil.h"
#include "util/gameevent.h"
#include "util/remotedebug.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "tribe.h"
#include "gem.h"
#include "stat.h"

class  Behavior;
class  EventManager;
class  HateList;
class  LocationType;
class  NPCType;
class  NetworkManager;
class  Tribe;
class  Waypoint;
class  gemNPCActor;
class  gemNPCObject;
class  iResultRow;
class  psLinearMovement;
class  psNPCClient;
class  psNPCTick;
struct iCollideSystem;
struct HateListEntry;

/**
 * \addtogroup npcclient
 * @{ */

#define NPC_BRAIN_TICK 200

// Define a variadic macro for debugging prints for NPCs.
// In this way only the check IsDebugging is executed for each NPC unless debugging
// is turned on. Using the Printf functions will cause all args to be
// resolved and that will result in a lot off spilled CPU.
#define NPCDebug(npc,debug,...) \
    { if (npc->IsDebugging()) { npc->Printf(debug, __VA_ARGS__); }}

/**
 * This object represents the entities which have attacked or
 * hurt the NPC and prioritizes them.
 */
class HateList
{
protected:
    csHash<HateListEntry*, EID> hatelist;

public:
    HateList(psNPCClient* npcclient, iEngine* engine, psWorld* world)
    {
        this->npcclient = npcclient;
        this->engine = engine;
        this->world = world;
    }
    ~HateList();

    void AddHate(EID entity_id, float delta);

    /**
     * Find the most hated entity within range of given position
     *
     *  Check the hate list and retrive most hated entity within range.
     *
     *  @param  npc                  The NPC
     *  @param  pos                  The position
     *  @param  sector               The sector of the position
     *  @param  range                The range to search for hated entities.
     *  @param  region               The region to search for hated entities within.
     *  @param  includeOutsideRegion Include enties outside region in the search.
     *  @param  includeInvisible     Include invisible entities in the search.
     *  @param  includeInvincible    Include invincible entities in the search.
     *  @param  hate                 If diffrent from NULL, set upon return to the hate of the hated.
     *  @return The hated entity
     */
    gemNPCActor* GetMostHated(NPC* npc, csVector3 &pos, iSector* sector, float range, LocationType* region,
                              bool includeOutsideRegion, bool includeInvisible, bool includeInvincible, float* hate);
    bool Remove(EID entity_id);
    void DumpHateList(csString &output, const csVector3 &myPos, iSector* mySector);
    void DumpHateList(NPC* npc, const csVector3 &myPos, iSector* mySector);
    void Clear();
    float GetHate(EID ent);

private:
    psNPCClient* npcclient;
    iEngine* engine;
    psWorld* world;
};

/**
* This object represents each NPC managed by this superclient.
*/
class NPC : private ScopedTimerCB, public iScriptableVar, public RemoteDebug
{
public:
    /**
     * Structure to hold located positions
     */
    typedef struct
    {
        csVector3               pos;        ///< The position of the located object
        iSector*                sector;     ///< The sector for the located object
        float                   angle;      ///< The angle of the located object
        Waypoint*               wp;         ///< The nearest waypoint to the located object
        float                   radius;     ///< The radius of the located object
        csWeakRef<gemNPCObject> target;     ///< The located target
    } Locate;
    typedef csHash<Locate*,csString> LocateHash;

    enum
    {
        LOCATION_NONE = 0,
        LOCATION_POS = 1,
        LOCATION_SECTOR = 2,
        LOCATION_ANGLE = 4,
        LOCATION_WP = 8,
        LOCATION_RADIUS = 16,
        LOCATION_TARGET = 32,
        LOCATION_ALL = -1
    };

protected:

    typedef csHash<csString,csString> BufferHash;

    NPCType*           oldbrain;        ///< delete at checkpoint
    NPCType*           brain;
    csString           origtype;        ///< Name of NPCType for 'reload'
    csString           type;
    PID                pid;
    csString           name;
    csTicks            last_update;
    gemNPCActor*       npcActor;
    iMovable*          movable;

    uint8_t            DRcounter;
    csVector3          lastDrPosition;
    iSector*           lastDrSector;
    csTicks            lastDrTime;
    bool               lastDrMoving;
    float              lastDrYRot;
    csVector3          lastDrVel;
    float              lastDrAngVel;

    Locate*            activeLocate;   ///< The current "Active" locate
    LocateHash         storedLocates;  ///< List of stored locate locations

    float              ang_vel,vel;
    float              walkVelocity;
    float              runVelocity;
    float              scale;                ///< Model scale value
    csString           region_name;          ///< Region name as loaded from db
    LocationType*      region;               ///< Cached pointer to the region
    bool               insideRegion;         ///< State variable for inside outside region checks.
    Perception*        last_perception;
    bool               alive;
    EID                owner_id;
    EID                target_id;

    csArray<csString>  autoMemorizeTypes;    ///< Used to store what types of perceptions to memorize without changing behaviors

    Tribe*             tribe;
    csString           tribeMemberType;      ///< What type/class is this NPC in the tribe.
    bool               insideTribeHome;      ///< State variable for inside outside tribe home checks.

    csVector3          spawnPosition;        ///< The stored position that this NPC where spawned
    iSector*           spawnSector;          ///< The stored sector that this NPC where spawned

    psNPCRaceListMessage::NPCRaceInfo_t*        raceInfo;

    // Stats
    Stat               hp;
    Stat               mana;
    Stat               pysStamina;
    Stat               menStamina;

    csArray< csWeakRef<gemNPCActor> > controlledActors; ///< Actors that are dragged/pushed around by this NPC.

    // Initial position checks
    csVector3          checkedPos;
    iSector*           checkedSector;
    bool               checked;
    bool               checkedResult;
    bool               disabled;

    int                fallCounter; // Incremented if the NPC fall off the map

    void Advance(csTicks when);

    /**
     * Handle post tick processing.
     */
    void TickPostProcess(csTicks when);

    HateList           hatelist;

public:

    NPC(psNPCClient* npcclient, NetworkManager* networkmanager, psWorld* world, iEngine* engine, iCollideSystem* cdsys);
    virtual ~NPC();


    void Tick();

    PID                   GetPID()
    {
        return pid;
    }
    /**
     * Return the entity ID if an entity exist else 0.
     */
    EID                   GetEID();
    iMovable*             GetMovable()
    {
        return movable;
    }
    psLinearMovement*     GetLinMove();

    uint8_t               GetDRCounter(csTicks when, const csVector3 &pos, float yRot, iSector* sector,
                                       const csVector3 &vel, float angVel)
    {
        lastDrTime = when;
        lastDrPosition = pos;
        lastDrSector = sector;
        lastDrYRot = yRot;
        lastDrVel = vel;
        lastDrAngVel = angVel;
        return ++DRcounter;
    }
    void                  SetDRCounter(uint8_t counter)
    {
        DRcounter = counter;
    }

    /**
     * Loads an NPC from one row of sc_npc_definitions, it also loads the corresponding brain.
     *
     * Used at boot time
     */
    bool Load(iResultRow &row,csHash<NPCType*, const char*> &npctypes, EventManager* eventmanager, PID usePID);

    /**
     * Loads an NPC base information and his brain
     */
    void Load(const char* name, PID pid, NPCType* type, const char* region_name, int debugging, bool disabled, EventManager* eventmanager);

    /** Insert a new copy of NPC into npc_definitions.
     */
    bool InsertCopy(PID use_char_id, PID ownerPID);

    /** Remove this NPC from npc_definitions.
     */
    bool Delete();

    void SetActor(gemNPCActor* actor);
    gemNPCActor* GetActor()
    {
        return npcActor;
    }
    const char* GetName()
    {
        return name.GetDataSafe();
    }
    void SetAlive(bool a);
    bool IsAlive() const
    {
        return alive;
    }
    void Disable(bool disable = true);
    bool IsDisabled()
    {
        return disabled;
    }

    Behavior* GetCurrentBehavior();
    NPCType*  GetBrain();
    const char* GetOrigBrainType()
    {
        return origtype;
    }

    /**
     * Sets a new brain (npctype)  to this npc.
     * @param type The new type to assign to this npc.
     */
    void SetBrain(NPCType* type);

    /**
     * Callback for debug scope timers
     */
    void ScopedTimerCallback(const ScopedTimer* timer);

    /**
     * Provide info about the NPC.
     *
     * This funcion is both used by the "/info" in the client
     * and the "info \<pid\> npcclient console command.
     */
    csString Info(const csString &infoRequestSubCmd);

    /**
     * Dump all information for one NPC to the console.
     *
     * The main use of this fuction is the "print \<pid\>"
     * npcclient console command.
     */
    void Dump();

    /**
     * Dump all state information for npc.
     */
    void DumpState(csString &output);

    /**
     * Dump all behaviors for npc.
     */
    void DumpBehaviorList(csString &output);

    /**
     * Dump all reactions for npc.
     */
    void DumpReactionList(csString &output);

    /**
     * Dump all hated entities for npc.
     */
    void DumpHateList(csString &output);

    /**
     * Dump all hated entities for npc. Using debug prints at level 5.
     */
    void DumpHateList(NPC* npc);

    /**
     * Dump the debug log for npc
     */
    void DumpDebugLog(csString &output);

    /**
     * Dump the memory for npc
     */
    void DumpMemory(csString &output);

    /**
     * Dump the controlled npcs for npc
     */
    void DumpControlled(csString &output);

    /**
     * Clear the NPCs state.
     *
     * Brain, hatelist, etc. is cleared.
     */
    void ClearState();

    /**
     * Send a perception to this NPC
     *
     * This will send the perception to this npc.
     * If the maxRange has been set to something greater than 0.0
     * a range check will be applied. Only if within the range
     * from the base position and sector will this perception
     * be triggered.
     *
     * @param pcpt       Perception to be sent.
     * @param maxRange   If greater then 0.0 then max range apply
     * @param basePos    The base position for range checks.
     * @param baseSector The base sector for range checks.
     * @param sameSector Only trigger if in same sector
     */
    void TriggerEvent(Perception* pcpt, float maxRange=-1.0,
                      csVector3* basePos=NULL, iSector* baseSector=NULL,
                      bool sameSector=false);

    /**
     * Send a perception to this NPC
     *
     * Uses the above method, but acts as a wrapper.
     * Receives only the name of the perception and uses default values
     * for the rest of the arguments
     */
    void TriggerEvent(const char* pcpt);

    void SetLastPerception(Perception* pcpt);
    Perception* GetLastPerception()
    {
        return last_perception;
    }

    /**
     * Find the most hated entity within range of the NPC
     *
     * Check the hate list and retrive most hated entity within range.
     *
     * @param  range             The range to search for hated entities.
     * @param  includeOutsideRegion Include enties outside region in the search.
     * @param  includeInvisible  Include invisible entities in the search.
     * @param  includeInvincible Include invincible entities in the search.
     * @param  hate              If diffrent from NULL, set upon return to the hate of the hated.
     * @return The hated entity
     */
    gemNPCActor* GetMostHated(float range, bool includeOutsideRegion, bool includeInvisible,
                              bool includeInvincible, float* hate=NULL);

    /**
     * Find the most hated entity within range of a given position
     *
     * Check the hate list and retrive most hated entity within range.
     *
     * @param  pos                  The position
     * @param  sector               The sector of the position
     * @param  range                The range to search for hated entities.
     * @param  region               The region to search for hated entities within.
     * @param  includeOutsideRegion Include enties outside region in the search.
     * @param  includeInvisible     Include invisible entities in the search.
     * @param  includeInvincible    Include invincible entities in the search.
     * @param  hate                 If diffrent from NULL, set upon return to the hate of the hated.
     * @return The hated entity
     */
    gemNPCActor* GetMostHated(csVector3 &pos, iSector* sector, float range, LocationType* region, bool includeOutsideRegion,
                              bool includeInvisible, bool includeInvincible, float* hate);


    /**
     * Retrive the hate this NPC has to a given entity.
     *
     * @param entity The entity to check for hate.
     * @return the hate value
     */
    float GetEntityHate(gemNPCActor* entity);

    /**
     * Change the hate list value for an attacker.
     *
     * @param attacker The entity that should be adjusted.
     * @param delta    The change in hate to apply to the attacker.
     */
    void AddToHateList(gemNPCActor* attacker,float delta);

    /**
     * Remove a entity from the hate list.
     */
    void RemoveFromHateList(EID who);

    /**
     * Set the NPCs locate.
     */
    void SetLocate(const csString &destination, const NPC::Locate &locate);

    /**
     * Get the NPCs current active locate.
     */
    void GetActiveLocate(csVector3 &pos, iSector* &sector, float &rot);

    /**
     * Return the wp of the current active locate.
     *
     * @param wp will be set with the wp currently in the active locate.
     */
    void GetActiveLocate(Waypoint* &wp);

    /**
     * Get the radius of the last locate operatoins
     *
     * @return The radius of the last locate
     */
    float GetActiveLocateRadius() const;

    /**
     * Copy locates
     *
     * Typically used to take backups of "Active" locate.
     */
    bool CopyLocate(csString source, csString destination, unsigned int flags);

    /**
     * Replace a location.
     *
     * This replace $LOCATION[\<location\>.\<attribute\>].
     */
    void ReplaceLocations(csString &result);

    /**
     * Return the angular velocity of this NPC.
     */
    float GetAngularVelocity();

    /**
     * Return the ?override? velocity for this NPC.
     */
    float GetVelocity();

    /**
     * Return the walk velocity for this NPC.
     */
    float GetWalkVelocity();

    /**
     * Return the run velocity for this NPC.
     */
    float GetRunVelocity();

    csString &GetRegionName()
    {
        return region_name;
    }
    LocationType* GetRegion();

    /**
     * Check the inside region state of the npc.
     *
     */
    bool IsInsideRegion()
    {
        return insideRegion;
    }

    /**
     * Set the inside region state
     *
     * Keep track of last perception for inbound our, outbound
     */
    void SetInsideRegion(bool inside)
    {
        insideRegion = inside;
    }


    /**
     * Return the nearest actor within the given range.
     *
     * @param range The search range to look for players
     * @param destPosition Return the position of the target
     * @param destSector Return the sector of the target
     * @param destRange Return the range to the target
     * @return The actor of the nearest player or NPC or NULL if none within range
     */
    gemNPCActor* GetNearestActor(float range, csVector3 &destPosition, iSector* &destSector, float &destRange);

    /**
     * Return the nearest NPC within the given range.
     *
     * @param range The search range to look for players
     * @param destPosition Return the position of the target
     * @param destSector Return the sector of the target
     * @param destRange Return the range to the target
     * @return The actor of the nearest NPC or NULL if none within range
     */
    gemNPCActor* GetNearestNPC(float range, csVector3 &destPosition, iSector* &destSector, float &destRange);

    /**
     * Return the nearest player within the given range.
     *
     * @param range The search range to look for players
     * @param destPosition Return the position of the target
     * @param destSector Return the sector of the target
     * @param destRange Return the range to the target
     * @return The actor of the nearest player or NULL if none within range
     */
    gemNPCActor* GetNearestPlayer(float range, csVector3 &destPosition, iSector* &destSector, float &destRange);

    gemNPCActor* GetNearestVisibleFriend(float range);

    gemNPCActor* GetNearestDeadActor(float range);

    /**
     * Add new types to the autoMemorize store. They should be "," seperated.
     */
    void AddAutoMemorize(csString types);

    /**
     * Remove types from the autoMemorize store. They should be "," seperated.
     */
    void RemoveAutoMemorize(csString types);

    /**
     * Check if the NPC has anything to automemorize.
     */
    bool HasAutoMemorizeTypes() const
    {
        return !autoMemorizeTypes.IsEmpty();
    }

    /**
     * Check if the type is contained in the NPC autoMemorize store
     */
    bool ContainAutoMemorizeType(const csString &type);

private:
    /**
     * Callback function to report local debug.
     */
    virtual void LocalDebugReport(const csString &debugString);

    /**
     * Callback function to report remote debug.
     */
    virtual void RemoteDebugReport(uint32_t clientNum, const csString &debugString);

public:
    gemNPCObject* GetTarget();
    void SetTarget(gemNPCObject* t);

    gemNPCObject* GetOwner();
    const char* GetOwnerName();

    /**
     * Sets the owner of this npc.
     *
     * The server will send us the owner of
     * the entity connected to it so we can follow it's directions.
     *
     * @param owner_EID the eid of the entity who owns this npc.
     */
    void SetOwner(EID owner_EID);

    /**
     * Set a new tribe for this npc
     */
    void SetTribe(Tribe* new_tribe);

    /**
     * Get the tribe this npc belongs to.
     *
     * @return Null if not part of a tribe
     */
    Tribe* GetTribe();

    /**
     * Set the type/class for this npc in a tribe.
     */
    void SetTribeMemberType(const char* tribeMemberType);

    /** Return the type/class for this NPC's tribe membership if any.
     */
    const csString &GetTribeMemberType() const;

    /**
     * Check the inside tribe home state of the npc.
     *
     */
    bool IsInsideTribeHome()
    {
        return insideTribeHome;
    }

    /**
     * Set the inside tribe home state
     *
     * Keep track of last perception for inbound our, outbound
     */
    void SetInsideTribeHome(bool inside)
    {
        insideTribeHome = inside;
    }

    /**
     * Get the npc race info
     */
    psNPCRaceListMessage::NPCRaceInfo_t* GetRaceInfo();


    /**
     * Get the npc HP
     */
    float GetHP();

    /**
     * Get the npc MaxHP
     */
    float GetMaxHP() const;

    /**
     * Get the npc HPRate
     */
    float GetHPRate() const;

    /**
     * Get the npc Mana
     */
    float GetMana();

    /**
     * Get the npc MaxMana
     */
    float GetMaxMana() const;

    /**
     * Get the npc ManaRate
     */
    float GetManaRate() const;

    /**
     * Get the npc PysStamina
     */
    float GetPysStamina();

    /**
     * Get the npc MaxPysStamina
     */
    float GetMaxPysStamina() const;

    /**
     * Get the npc PysStaminaRate
     */
    float GetPysStaminaRate() const;

    /**
     * Get the npc MenStamina
     */
    float GetMenStamina();

    /**
     * Get the npc MaxMenStamina
     */
    float GetMaxMenStamina() const;

    /**
     * Get the npc MenStaminaRate
     */
    float GetMenStaminaRate() const;

    /**
     * Take control of another entity.
     */
    void TakeControl(gemNPCActor* actor);

    /**
     * Release control of another controlled entity.
     */
    void ReleaseControl(gemNPCActor* actor);

    /**
     * Update entities controlled by this NPC.
     */
    void UpdateControlled();

    /**
     *
     */
    void CheckPosition();

    /**
     * Store the start position.
     *
     * Store the current position as spawn position so that NPCs can locate
     * spawn position.
     */
    void StoreSpawnPosition();

    /**
     * Return the position part of the spawn position
     */
    const csVector3 &GetSpawnPosition() const;

    /**
     * Return the sector part of the spawn position
     */
    iSector* GetSpawnSector() const;


    /**
     * Increment the fall counter
     *
     * Fall counter is used for debugging.
     */
    void IncrementFallCounter()
    {
        ++fallCounter;
    }

    /**
     * Return the fall counter
     *
     * Fall counter is used for debugging.
     */
    int GetFallCounter()
    {
        return fallCounter;
    }

    /**
     * Return a named buffer from the NPC.
     *
     * Function used to get data from buffers containing information.
     *
     * @param  bufferName The buffer name.
     * @return The content of the named buffer.
     */
    csString GetBuffer(const csString &bufferName);

    /**
     * Set/Update the value of a named buffer.
     *
     * @param bufferName The buffer name.
     * @param value The value to put in the buffer.
     */
    void SetBuffer(const csString &bufferName, const csString &value);

    /**
     * Replace $NBUFFER[x] with values from the NPC buffer.
     *
     * @param result String to replace buffers in.
     */
    void ReplaceBuffers(csString &result);

    /**
     * Retrive the current buffer memory of the npc
     */
    Tribe::Memory* GetBufferMemory()
    {
        return bufferMemory;
    }

    /**
     * Set a new buffer memory for the NPC.
     *
     *  @param memory The new memory to store in the NPC.
     */
    void SetBufferMemory(Tribe::Memory* memory);

    /**
     * Set a building spot for this NPC.
     *
     * @param buildingSpot The building spot to set.
     */
    void SetBuildingSpot(Tribe::Asset* buildingSpot);

    /**
     * Get the stored building spot for this NPC
     */
    Tribe::Asset* GetBuildingSpot();

private:
    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    virtual const char* ToString();
    ///@}

private:
    psNPCTick*        tick;
    psNPCClient*      npcclient;
    NetworkManager*   networkmanager;
    psWorld*          world;
    iCollideSystem*   cdsys;

    BufferHash        npcBuffer;        ///< Used to store dynamic data
    Tribe::Memory*    bufferMemory;     ///< Used to store location data
    Tribe::Asset*     buildingSpot;     ///< Used to store current building spot.

    friend class psNPCTick;

};

/** The event that makes the NPC brain go TICK.
 */
class psNPCTick : public psGameEvent
{
protected:
    NPC* npc;

public:
    psNPCTick(int offsetticks, NPC* npc): psGameEvent(0,offsetticks,"psNPCTick"), npc(npc) {};

    virtual void Trigger()
    {
        if(npc)
        {
            npc->tick = NULL;
            npc->Tick();
        }
    }

    void Remove()
    {
        npc = NULL;
    }


    virtual csString ToString() const
    {
        return "psNPCTick";
    }
};

struct HateListEntry
{
    EID   entity_id;
    float hate_amount;
};

/** @} */

#endif

