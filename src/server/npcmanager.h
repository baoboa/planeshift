/*
* npcmanager.h by Keith Fulton <keith@paqrat.com>
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

#ifndef __NPCMANAGER_H_
#define __NPCMANAGER_H_
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/netbase.h"                // PublishVector class
#include "net/npcmessages.h"

#include "bulkobjects/psskills.h"
#include "cachemanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class

#define OWNER_ALL 0xFFFFFFFF


class Client;
class psDatabase;
class psNPCCommandsMessage;
class ClientConnectionSet;
class EntityManager;
class EventManager;
class gemObject;
class gemActor;
class gemNPC;
class PetOwnerSession;
class psPathPoint;
class Waypoint;
class psPath;
class Location;
class LocationType;

class NPCManager : public MessageManager<NPCManager>
{
public:
    NPCManager(ClientConnectionSet* pCCS,
               psDatabase* db,
               EventManager* evtmgr,
               GEMSupervisor* gemsupervisor,
               CacheManager* cachemanager,
               EntityManager* entitymanager);

    virtual ~NPCManager();

    /// Initialize the npc manager.
    bool Initialize();

    /// Send a list of managed NPCs to a newly connecting superclient.
    void SendNPCList(Client* client);

    /// Send stats for all managed NPCs to a newly connecting superclient.
    void SendAllNPCStats(Client* client);

    /// Remove a disconnecting superclient from the active list.
    void Disconnect(Client* client);

    /// Communicate a entity added to connected superclients.
    void AddEntity(gemObject* obj);

    /// Communicate a entity going away to connected superclients.
    void RemoveEntity(MsgEntry* me);

    /// Build a message with all changed world positions for superclients to get.
    void UpdateWorldPositions();

    /// Let the superclient know the result of an assessment
    void QueueAssessPerception(EID entityEID, EID targetEID, const csString &physicalAssessmentPerception,
                               const csString &physicalAssessmentDifferencePerception,
                               const csString &magicalAssessmentPerception,
                               const csString &magicalAssessmentDifferencePerception,
                               const csString &overallAssessmentPerception,
                               const csString &overallAssessmentDifferencePerception);


    /// Let the superclient know that a player has said something to one of its npcs.
    void QueueTalkPerception(gemActor* speaker, gemNPC* target);

    /// Let the superclient know that a player has attacked one of its npcs.
    void QueueAttackPerception(gemActor* attacker,gemNPC* target);

    /// Let the superclient know that a player has taken HP from one of its npcs.
    void QueueDamagePerception(gemActor* attacker,gemNPC* target,float dmg);

    /// Let the superclient know that one of its npcs has died.
    void QueueDeathPerception(gemObject* who);

    /// Let the superclient know that a spell has been cast.
    void QueueSpellPerception(gemActor* caster, gemObject* target,const char* spell_cat, uint32_t spell_category, float severity);

    //// Let the superclient know the stats of the actor
    void QueueStatDR(gemActor* actor, unsigned int statsDirtyFlags);

    /// Let the superclient know that one enemy is close.
    void QueueEnemyPerception(psNPCCommandsMessage::PerceptionType type, gemActor* npc, gemActor* player, float relative_faction);

    /// Let the superclient know that one of its npcs has been commanded to stay.
    void QueueOwnerCmdPerception(gemActor* owner, gemNPC* pet, psPETCommandMessage::PetCommand_t command);

    /// Let the superclient know that one of its npcs has a change in inventory.
    void QueueInventoryPerception(gemActor* owner, psItem* itemdata, bool inserted);

    /// Let the superclient know that one of the actors flags has changed.
    void QueueFlagPerception(gemActor* owner);

    /// Let the superclient know that a response has commanded a npc.
    void QueueNPCCmdPerception(gemActor* owner, const csString &cmd);

    /// Let the superclient know that a transfer has happend.
    void QueueTransferPerception(gemActor* owner, psItem* itemdata, csString target);

    /// Let the superclient know the npc was spawned successfully.
    void QueueSpawnedPerception(gemNPC* spawned, gemNPC* spawner, const csString &tribeMemberType);

    /// Let the superclient know that the npc was teleported
    void QueueTeleportPerception(gemNPC* npc, csVector3 &pos, float yrot, iSector* sector, InstanceID instance);

    /// Let the superclient know that info is requested for a npc
    void QueueInfoRequestPerception(gemNPC* npc, Client* client, const char* infoRequestSubCmd);

    /// Let the superclient know that an attempt to attack failed
    void QueueFailedToAttackPerception(gemNPC* attacker, gemObject* target);

    /// Send a perception to the super client for the npc.
    void QueuePerceptPerception(gemNPC* npc, csString perception, csString type);

    /// Send a spoken to perception to the super client for the npc
    void QueueSpokenToPerception(gemNPC* npc, bool spokenTo);

    /// Send a change owner perception.
    void QueueChangeOwnerPerception(gemNPC* npc, EID owner);

    /**
     * Requests the npcclient to change the brain of this npc (the type) to another one.
     * @param npc The npc which will be changed.
     * @param client The client requesting the action.
     * @param brainName The name of the new brain (npctype) to associate to the npc.
     */
    void ChangeNPCBrain(gemNPC* npc, Client* client, const char* brainName);

    /**
     * Requests the npcclient to change the debug level of this npc.
     * @param npc The npc which will be changed.
     * @param client The client requesting the action.
     * @param debugLevel The new debug level for the npc.
     */
    void DebugNPC(gemNPC* npc, Client* client, uint8_t debugLevel);

    /**
     * Requests the npcclient to change the debug level of this npc.
     * @param npc The npc which will be changed.
     * @param client The client requesting the action.
     * @param debugLevel The new debug level for the npc.
     */
    void DebugTribe(gemNPC* npc, Client* client, uint8_t debugLevel);

    /// Send all queued commands and perceptions to active superclients and reset the queues.
    void SendAllCommands(bool createNewTick);

    /// Get the vector of active superclients, used in Multicast().
    csArray<PublishDestination> &GetSuperClients()
    {
        return superclients;
    }

    /// Inform the npc about the reward granted after a work done
    void WorkDoneNotify(EID npcEID, csString reward, csString nick);

    /// Send a newly spawned npc to a superclient to manage it.
    void NewNPCNotify(PID player_id, PID master_id, PID owner_id);

    /// Send a deleted npc to a superclient to un-manage it.
    void DeletedNPCNotify(PID player_id);

    /// Tell a superclient to control an existing npc.
    void ControlNPC(gemNPC* npc);

    /** Check work location.
     *  @param npc Use the superclient of this npc to verify the location.
     *  @param location The location to verify.
     */
    void CheckWorkLocation(gemNPC* npc, Location* location);

    /// Add Session for pets
    PetOwnerSession* CreatePetOwnerSession(gemActor*, psCharacter*);

    /// Dismiss active (summoned) pet.
    void DismissPet(gemNPC* pet, Client* owner);

    /// Remove Session for pets
    void RemovePetOwnerSession(PetOwnerSession* session);

    /// Updates time in game for a pet
    void UpdatePetTime();

    /**
     * Returns the skill used to handle pet operations.
     *
     * @return A PSSKILL which is the skill choosen to handle pet operations.
     */
    PSSKILL GetPetSkill()
    {
        return (PSSKILL)petSkill->getValueAsInt();
    }

    /**
     * Notify superclients that a location where adjusted.
     *
     * @param location  The location that has been adjusted.
     */
    void LocationAdjusted(Location* location);

    /**
     * Notify superclients that a location where created.
     *
     * @param location The location that has been created.
     */
    void LocationCreated(Location* location);

    /**
     * Notify superclients that a location where inserted into a region.
     *
     * @param location The location that has been inserted.
     */
    void LocationInserted(Location* location);

    /** Notify superclients that a location radius where chaned.
     * @param location The location that has been chaned.
     */
    void LocationRadius(Location* location);

    /** Notify superclients that a location type has been added.
     * @param locationType The location type that has been created.
     */
    void LocationTypeAdd(LocationType* locationType);

    /** Notify superclients that a location type has been deleted.
     * @param locationTypeName The name of the location type that has been deleted.
     */
    void LocationTypeRemove(const csString &locationTypeName);

    /** Notify superclients that a waypoint where adjusted.
     * @param wp The waypoint that has been adjusted.
     */
    void WaypointAdjusted(Waypoint* wp);

    /** Notify superclients that a pathpoint where adjusted.
     * @param point The pathpoint that has been adjusted.
     */
    void PathPointAdjusted(psPathPoint* point);

    /** Notify superclients that a alias has been added to a waypoint.
     * @param wp The waypoint that got a new alias.
     * @param alias The alias that was added.
     */
    void WaypointAddAlias(const Waypoint* wp, const WaypointAlias* alias);

    /** Notify superclients that a alias has been added to a waypoint.
     * @param wp The waypoint that got a new alias.
     * @param alias The alias that was added.
     */
    void WaypointAliasRotation(const Waypoint* wp, const WaypointAlias* alias);

    /** Notify superclients that a alias has been removed from a waypoint.
     * @param wp The waypoint that a alias where removed.
     * @param alias The alias that was removed.
     */
    void WaypointRemoveAlias(const Waypoint* wp, const csString &alias);

    /** Notify superclients that a flag has been changed for waypoint.
     * @param wp The waypoint that where changed.
     * @param flag The flag that where changed.
     * @param enable True if the flag where enabled.
     */
    void WaypointSetFlag(const Waypoint* wp, const csString &flag, bool enable);

    /** Notify superclients that a flag has been changed for a path.
     * @param path The path that where changed.
     * @param flag The flag that where changed.
     * @param enable True if the flag where enabled.
     */
    void PathSetFlag(const psPath* path, const csString &flag, bool enable);

    /** Notify superclients that a waypoint has changed radius.
     * @param waypoint The waypoint that where changed.
     */
    void WaypointRadius(const Waypoint* waypoint);

    /** Notify superclients that a waypoint has been renamed.
     * @param waypoint The waypoint that where changed.
     */
    void WaypointRename(const Waypoint* waypoint);

    /** Notify superclients that a path has been renamed.
     * @param path The path that where changed.
     */
    void PathRename(const psPath* path);

    /** Notify superclients that a waypoint has been created.
     * @param waypoint The waypoint that where created.
     */
    void WaypointCreate(const Waypoint* waypoint);

    /** Notify superclients that a path has been created.
     * @param path The path that where created.
     */
    void PathCreate(const psPath* path);

    /** Notify superclients that a point has been added to a path.
     * @param path The path where the point where added.
     * @param point The point that where added.
     */
    void AddPoint(const psPath* path, const psPathPoint* point);

    /** Notify superclients that a point has been removed from a path.
     * @param path The path where the point where removed.
     * @param pointId The point that where removed.
     */
    void RemovePoint(const psPath* path, int pointId);

    MathScript* GetPetDepletedLockoutTime() { return petDepletedLockoutTime; }
    MathScript* GetPetDismissLockoutTime() { return petDismissLockoutTime; }
    MathScript* GetMaxPetTime() { return maxPetTime; }
    MathScript* GetPetDeathLockoutTime() { return petDeathLockoutTime; }
    MathScript* GetPetTrainingLockoutTime() { return petTrainingLockoutTime; }

protected:

    /// Handle a login message from a superclient.
    void HandleAuthentRequest(MsgEntry* me,Client* client);

    /// Handle a network msg with a list of npc directives.
    void HandleCommandList(MsgEntry* me,Client* client);

    /// Catch an internal server event for damage so a perception can be sent about it.
    void HandleDamageEvent(MsgEntry* me,Client* client);

    /// Catch an internal server event for death so a perception can be sent about it.
    void HandleDeathEvent(MsgEntry* me,Client* client);

    void HandleNPCReady(MsgEntry* me,Client* client);

    /// Handle debug meshes from superclient
    void HandleSimpleRenderMesh(MsgEntry* me, Client* client);

    /// Send the list of maps for the superclient to load on startup.
    void SendMapList(Client* client);

    /// Send the list of races for the superclient to load on startup.
    void SendRaces(Client* client);

    /// Check if a pet is within range to react to commands
    bool CanPetHearYou(int clientnum, Client* owner, gemNPC* pet, const char* type);

    /// Check if your pet will reacto to your command based on skills
    bool WillPetReact(int clientnum, Client* owner, gemNPC* pet, const char* type, int level);

    /// Handle network message with pet directives
    void HandlePetCommand(MsgEntry* me, Client* client);

    /// Handle network message with pet skills
    void HandlePetSkill(MsgEntry* me, Client* client);
    void SendPetSkillList(Client* client, bool forceOpen = true, PSSKILL focus = PSSKILL_NONE);

public:
    /// Notification that an pet has been killed.
    /// Upon notification of a killed pet this function will start a timer that prevent
    /// resummon of dead PETs.
    void PetHasBeenKilled(gemNPC*  pet);

    /**
     * Provide response for the /info commands for GMs for pets.
     */
    void PetInfo(Client* client, psCharacter* pet);

protected:
    /// Handle network message with console commands from npcclient
    void HandleConsoleCommand(MsgEntry* me,Client* client);

    /// Create an empty command list message, waiting for items to be queued in it.
    void PrepareMessage();

    /** Check if the perception queue is going to overflow with the next perception.
     *  If the queue is going to overflow it will automatically send the commands and clean up to allow
     *  new messages to be queued. Pet time updates and world position updates are still left to the npc
     *  ticks.
     *  @param expectedAddSize: The size this percention is expecting to add, remember to update this in any
     *                          perception being expanded
     */
    void CheckSendPerceptionQueue(size_t expectedAddSize);

    /// List of active superclients.
    csArray<PublishDestination> superclients;

    psDatabase*  database;
    EventManager* eventmanager;
    GEMSupervisor* gemSupervisor;
    CacheManager* cacheManager;
    EntityManager* entityManager;
    ClientConnectionSet* clients;
    psNPCCommandsMessage* outbound;
    int cmd_count;
    optionEntry* petSkill;

    csHash<PetOwnerSession*, PID> OwnerPetList;

    /// Math script setup for pet range check
    MathScript* petRangeScript;

    /// Math script setup for pet should react check
    MathScript* petReactScript;

    MathScript* petDepletedLockoutTime;
    MathScript* petDismissLockoutTime;
    MathScript* maxPetTime;
    MathScript* petDeathLockoutTime;
    MathScript* petTrainingLockoutTime;
};

#endif
