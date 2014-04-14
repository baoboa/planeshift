/*
 * networkmgr.h
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
#ifndef __NETWORKMGR_H__
#define __NETWORKMGR_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "net/cmdbase.h"            // Subscriber class
#include "net/npcmessages.h"
#include "util/serverconsole.h"     // iCommandCatcher

class MsgHandler;
class EventManager;
class psLinearMovement;

//=============================================================================
// Local Includes
//=============================================================================

/**
 * \addtogroup npcclient
 * @{ */

class psNPCCommandsMessage;
class psNetConnection;
class psGameEvent;
class NPC;
class gemNPCActor;
class gemNPCObject;
class gemNPCItem;

/**
 * Handle all network messages inn and out of the NPC Client.
 */
class NetworkManager : public psClientNetSubscriber
{
protected:
    MsgHandler* msghandler;
    psNetConnection* connection;
    bool ready;
    bool connected;

    // Command Message Queue
    psNPCCommandsMessage* outbound;
    csHash<NPC*,PID>      cmd_dr_outbound; /// Entities queued for sending of DR.
    int                   cmd_count;       /// Number of command messages queued

    void RequestAllObjects();

    /**
     * Handle the map list received from server.
     *
     * @param msg        The undecoded message to handle.
     */
    bool HandleMapList(MsgEntry* msg);

    /**
     * Handle a list of npcs received from server.
     *
     * @param msg        The undecoded message to handle.
     */
    bool HandleNPCList(MsgEntry* msg);

    /**
     * Handle update update of positions from server.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandlePositionUpdates(MsgEntry* msg);

    /**
     * Handle new entities from server.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandlePersistMessage(MsgEntry* msg);

    /**
     * Handle perceptions from the server.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandlePerceptions(MsgEntry* msg);

    /**
     * Handle command to disconnect from server.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandleDisconnect(MsgEntry* msg);

    /**
    * Handle Hired npc script from server.
    *
    * @param msg        The undecoded message to handle.
    */
    void HandleHiredNPCScript(MsgEntry* me);

    /**
     * Handle time updates from server.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandleTimeUpdate(MsgEntry* msg);

    /**
     * Handle information from server that work is done.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandleNPCWorkDone(MsgEntry* msg);

    /**
     * Handle information from server about NPCs to use.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandleNewNpc(MsgEntry* msg);

    /**
     * Handle information from server about deleted NPC.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandleNpcDeleted(MsgEntry* msg);

    /**
     *
     *
     * @param me         The undecoded message to handle.
     */
    void HandleConsoleCommand(MsgEntry* me);

    /**
     * Handle list of races from server.
     *
     * @param me         The undecoded message to handle.
     */
    void HandleRaceList(MsgEntry* me);

    /**
     * Handle list of all entities from server.
     *
     * @param message        The undecoded message to handle.
     */
    void HandleAllEntities(MsgEntry* message);

    /**
     * Handle an actor from server.
     *
     * @param msg        The undecoded message to handle.
     */
    void HandleActor(MsgEntry* msg);

    /**
     * Handle a new item from server.
     *
     * @param me         The undecoded message to handle.
     */
    void HandleItem(MsgEntry* me);

    /**
     * Handle removal of an object from server.
     *
     * @param me         The undecoded message to handle.
     */
    void HandleObjectRemoval(MsgEntry* me);

    /**
     * Handle changes to path network from server.
     *
     * @param me         The undecoded message to handle.
     */
    void HandlePathNetwork(MsgEntry* me);

    /**
     * Handle location changes from server.
     *
     * @param me         The undecoded message to handle.
     */
    void HandleLocation(MsgEntry* me);

    csString host,password,user;
    int port;

public:

    /**
     * Construct the network manager.
     */
    NetworkManager(MsgHandler* mh,psNetConnection* conn, iEngine* engine);

    /**
     * Destructor.
     */
    virtual ~NetworkManager();

    /**
     * Handle all messages from server and send them to specefic Handle functions for each message type.
     *
     * @param pMsg        The undecoded message to handle.
     */
    virtual void HandleMessage(MsgEntry* pMsg);

    void Authenticate(csString &host,int port,csString &user,csString &pass);
    bool IsReady()
    {
        return ready;
    }
    void Disconnect();

    /**
     * Checks if the npc command message could overrun if the neededSize is tried to be added.
     *
     * Automatically sends the message in case it could overrun.
     * @param neededSize The size of data we are going to attempt to add to the npc commands message.
     */
    void CheckCommandsOverrun(size_t neededSize);

private:

    /**
     * Send a DR Data command to server.
     *
     * This command is private to the NetworkManager since it should
     * only be called as a result of first calling QueueDRData that will
     * put the npc in a queue for DR updates.
     *
     * @param npc             The npc to queue DR Data for.
     */
    void QueueDRDataCommand(NPC* npc);

public:
    /**
     * Prepare a new command message allow for queuing of new commands.
     */
    void PrepareCommandMessage();

    /**
     * Send the queued commands.
     *
     * @param final        True if this is the final. And a forced sending should be done.
     */
    void SendAllCommands(bool final = false);

    /**
     * Queue the NPC for an DR Update to the server.
     *
     * Can be called multiple times. If in queue already nothing will
     * be added.
     *
     * @param npc      Queue DR data for this NPC.
     */
    void QueueDRData(NPC* npc);

    /**
     * Queue the NPC for an DR Update to the server.
     *
     * Can be called multiple times. If in queue already nothing will
     * be added.
     *
     * @param npc      Queue DR data for this NPC.
     */
    void QueueDRData2(NPC* npc);

    /**
     * Call to remove queued dr updates when entities are removed/deleted.
     */
    void DequeueDRData(NPC* npc);

    /**
     * Queue an Attack Command for an NPC to the server.
     *
     * @param attacker The NPC that will attack
     * @param target   The target for the NPC or NULL to stop attack
     * @param stance   The stance to be used
     */
    void QueueAttackCommand(gemNPCActor* attacker, gemNPCActor* target, const char* stance);

    /**
     * Send a script command to server
     *
     * @param npc             The npc that sit/stand
     * @param target          The current target of then npc
     * @param scriptName      The name of the progression script to run at server
     */
    void QueueScriptCommand(gemNPCActor* npc, gemNPCObject* target, const csString &scriptName);

    /**
     * Send a sit command to server
     *
     * @param npc             The npc that sit/stand
     * @param target          The current target of then npc
     * @param sit             True if sitting, false if standing
     */
    void QueueSitCommand(gemNPCActor* npc, gemNPCObject* target, bool sit);

    /**
     * Send a spawn command to server
     *
     * @param mother          The mother of the spawn
     * @param father          If the father is known point to the father, use mother if unknow.
     * @param tribeMemberType What type of tribe member is this.
     */
    void QueueSpawnCommand(gemNPCActor* mother, gemNPCActor* father, const csString &tribeMemberType);

    /**
     * Send a spawn building command to server
     *
     * @param spawner      The entity that spawned the building
     * @param where        Containing the desired location
     * @param sector       The sector in which we want a spawn.
     * @param buildingName The name of the building. Or more exactly... of the mesh for it.
     * @param tribeID      The owner of this building
     * @param pickupable   Should the building be possible to pick up
     */
    void QueueSpawnBuildingCommand(gemNPCActor* spawner, csVector3 where, iSector* sector, const char* buildingName, int tribeID, bool pickupable);

    /**
     * Send a unbuild command to server
     *
     * @param unbuilder    The entity that unbuild the building
     * @param building     The building.
     */
    void QueueUnbuildCommand(gemNPCActor* unbuilder, gemNPCItem* building);

    /**
     * Queue a talk command to the server
     *
     * The speaker will send a talk event say/action to target or nearby targets.
     *
     * @param speaker         The npc that speaks.
     * @param target          The current target.
     * @param talkType        What kind of talk is this.
     * @param publicTalk      True if this should be published. False will be a wisper.
     * @param text            The text to talk.
     */
    void QueueTalkCommand(gemNPCActor* speaker, gemNPCActor* target, psNPCCommandsMessage::PerceptionTalkType talkType, bool publicTalk, const char* text);

    /**
     * Queue a change in visibility.
     *
     * @param entity          The entity to change visibility.
     * @param status          The new visibility status. True or False.
     */
    void QueueVisibilityCommand(gemNPCActor* entity, bool status);

    /**
     * Queue a command to pickup a item.
     *
     * @param entity          The entity that should pick up a item.
     * @param item            The item to pick up.
     * @param count           The number of items to pick up of a stack.
     */
    void QueuePickupCommand(gemNPCActor* entity, gemNPCObject* item, int count);

    /**
     * Send an emote command to server
     *
     * @param npc             The npc that has an emotion
     * @param target          The current target of then npc
     * @param cmd             The emotion
     */
    void QueueEmoteCommand(gemNPCActor* npc, gemNPCObject* target, const csString &cmd);

    /**
     * Send a command to equip an equipment.
     */
    void QueueEquipCommand(gemNPCActor* entity, csString item, csString slot, int count);

    /**
     * Send a command to dequip an equipment.
     */
    void QueueDequipCommand(gemNPCActor* entity, csString slot);

    /**
     * Send a command to start working.
     */
    void QueueWorkCommand(gemNPCActor* entity, const csString &type, const csString &resource);

    /**
     * Send a command to transfere an item between two entities.
     */
    void QueueTransferCommand(gemNPCActor* entity, csString item, int count, csString target);

    /**
     * Send a command to delete a npc.
     */
    void QueueDeleteNPCCommand(NPC* npcx);

    /**
     * Send a command to drop the content of a slot.
     */
    void QueueDropCommand(gemNPCActor* entity, csString slot);

    /**
     * Send a command to resurrect.
     */
    void QueueResurrectCommand(csVector3 where, float rot, iSector* sector, PID character_id);

    /**
     * Send a command to start a sequence.
     */
    void QueueSequenceCommand(csString name, int cmd, int count);

    /**
     * Send a command to change the temporarily impervious status.'
     *
     * @param entity           The entity to change.
     * @param impervious       Set to true if temporarily impervious should be turned on.
     */
    void QueueTemporarilyImperviousCommand(gemNPCActor* entity, bool impervious);

    /**
     * Send an system info command to the client.
     *
     * @param clientNum        The client that should receive the system info.
     * @param reply            The reply to send back.
     */
    void QueueSystemInfoCommand(uint32_t clientNum, const char* reply, ...);

    /**
     * Send a command to do an assessment.
     *
     */
    void QueueAssessCommand(gemNPCActor* entity, gemNPCObject* target, const csString &physicalAssessmentPerception,
                            const csString &magicalAssessmentPerception,  const csString &overallAssessmentPerception);

    /**
     * Send a command to start cast a spell.
     */
    void QueueCastCommand(gemNPCActor* entity, gemNPCObject* target, const csString &spell, float kFactor);

    /**
     * Send a command to change the busy state.
     */
    void QueueBusyCommand(gemNPCActor* entity, bool busy);

    /**
     * Send a command start controll an entity.
     */
    void QueueControlCommand(gemNPCActor* controllingEntity, gemNPCActor* controlledEntity);

    /**
     * Send a command to loot selected target.
     */
    void QueueLootCommand(gemNPCActor* entity, EID targetEID, const csString &type);

    /**
     * Send a console command.
     *
     * @param cmd          The command to send.
     */
    void SendConsoleCommand(const char* cmd);

    /**
     * Get the message string table.
     */
    csStringHashReversible* GetMsgStrings();

    /**
     * Convert a common string id into the corresponding string
     */
    const char* GetCommonString(uint32_t cstr_id);

    /**
     * Convert a string into a common string id
     */
    uint32_t GetCommonStringID(const char* string);

    /**
     * Reauthenicate with the server.
     */
    void ReAuthenticate();

    /**
     * Reconnect to the server server.
     */
    void ReConnect();

    bool reconnect; ///< Set to true if reconnect should be done.

private:
    iEngine* engine;
};

class psNPCReconnect : public psGameEvent
{
protected:
    NetworkManager* networkMgr;
    bool authent;

public:
    psNPCReconnect(int offsetticks, NetworkManager* mgr, bool authent);
    virtual void Trigger();  // Abstract event processing function
};

/** @} */

#endif

