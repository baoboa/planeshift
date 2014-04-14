/*
* actionmanager.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits :
*                  Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : server manager for clickable map object actions
*
*/
#ifndef __ACTIONMANAGER_H__
#define __ACTIONMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class
#include "scripting.h"


class psDatabase;
class SpawnManager;
class psServer;
class psDatabase;
class ClientConnectionSet;
class psActionLocation;
class ActionManager;
class psSectorInfo;
class Client;
class gemActionLocation;
class gemActor;

/**
 * Time out event on interacting with an action item.
 *
 * This timer is fired when an entity is detected to trigger an action location. It
 * will than regullary check if the entity has left and release the entity
 * from the triggered action location.
 */
class psActionTimeoutGameEvent : public psGameEvent
{
public:
    /**
     * Constructor.
     *
     * @param mgr            Pointer to ActionManager, used for callbacks from Trigger.
     * @param actionLocation Action location to guard.
     * @pram  actorEID       The EID of the actor that is inside the action location.
     */
    psActionTimeoutGameEvent(ActionManager* mgr, const psActionLocation* actionLocation, EID actorEID);

    /**
     * Destructor.
     */
    ~psActionTimeoutGameEvent();

    /**
     * Abstract event processing function.
     */
    virtual void Trigger();
    virtual bool IsValid()
    {
        return valid;
    }

protected:
    ActionManager*           actionManager;  ///< Reference to the action manager
    bool                     valid;          ///< Is this trigger still valid

    EID                      actorEID;       ///< Reference to the actor
    const  psActionLocation* actionLocation; ///< The action location
};


//----------------------------------------------------------------------------

/**
 * Handles the map interaction system.
 *
 * Used to populate/update/change current action locations. Action locations
 * can either be triggered by user action or proximity to the action location.
 */
class ActionManager : public MessageManager<ActionManager>
{
public:

    /**
     * Constructor.
     */
    ActionManager(psDatabase* db);

    /**
     * Destructor.
     */
    virtual ~ActionManager();

    /**
     * Loads cache from action_location table in db.
     *
     * @param sectorinfo The sector to repopulate. Null means all sectors.
     */
    bool RepopulateActionLocations(psSectorInfo* sectorinfo = 0);

    /**
     * Loads cache with given action location.
     *
     * @param action The action location to which you want to load
     */
    bool CacheActionLocation(psActionLocation* action);

    /**
     * Processes psMapActionMessages.
     *
     * @param msg         The message to process
     * @param client      The client that sent the message.
     */
    void HandleMapAction(MsgEntry* msg, Client* client);

    /**
     * Handles the /use command on an AL or the click on use button.
     * @param client            The client that issued the use command
     */
    bool HandleUse(gemActionLocation* actionlocation, Client* client);

    /**
     * Remove all active trigger flag for this client and action location.
     *
     * @param actorEID       The EID of the actor to remove from the given action location.
     * @param actionLocation The location that is triggered for the given actor.
     */
    void RemoveActiveTrigger(EID actorEID, const psActionLocation* actionLocation);

    /**
     * Check if there is an active action location for this client/actionLocation pair.
     *
     * @param actorEID       The EID of the actor to remove from the given action location.
     * @param actionLocation The location that is triggered for the given actor.
     */
    bool HasActiveTrigger(EID actorEID, const psActionLocation* actionLocation);

    /**
     * Add a new active actionLocation for this client.
     *
     * @param actorEID       The EID of the actor to remove from the given action location.
     * @param actionLocation The location that is triggered for the given actor.
     */
    void AddActiveTrigger(EID actorEID, const psActionLocation* actionLocation);

    /**
     * Finds an ActionLocation from it's CEL Entity ID.
     *
     * @param id The id of the cel entity to find.
     */
    psActionLocation* FindAction(EID id);

    /**
     * Finds an ActionLocation from the action ID.
     *
     * @param id The id of the action location.
     */
    psActionLocation* FindActionByID(uint32 id);

    /**
     * Finds an inactive entrance action location in the specified target sector map.
     *
     * @param entranceSector The entrance teleport target sector string to qualify the search.
     */
    psActionLocation* FindAvailableEntrances(csString entranceSector);

    /**
     * Handle notification from the proxy system about players nearby action locations.
     */
    void NotifyProximity(gemActor* actor, gemActionLocation* actionLocationObject, float range);

protected:

    // Message Handlers

    /**
     * Handles Query messages from client.
     *
     * @param xml xml containing query parameters.
     * @param client      The client that sent the message.
     */
    void HandleQueryMessage(csString xml, Client* client);

    void LoadXML(iDocumentNode* topNode);
    bool HandleSelectQuery(iDocumentNode* topNode, Client* client);
    bool ProcessMatches(csArray<psActionLocation*> matches, Client* client);

    /**
     * Handles Save messages from client.
     *
     * @param xml         xml containing query parameters.
     * @param client      The client that sent the message.
     */
    void HandleSaveMessage(csString xml, Client* client);

    /**
     * Handles List messages from client.
     *
     * @param xml         xml containing query parameters.
     * @param client      The client that sent the message.
     */
    void HandleListMessage(csString xml, Client* client);

    /**
     * Handles Delete messages from client.
     *
     * @param xml         xml containing query parameters.
     * @param client      The client that sent the message.
     */
    void HandleDeleteMessage(csString xml, Client* client);

    /**
     * Handles Reload messages from client.
     *
     * @param client      The client that sent the message.
     */
    void HandleReloadMessage(Client* client);

    // Operation Handlers

    /**
     * Handles Examine Operation for a action location.
     *
     * @param action      The action that is to be performed.
     * @param client      The client that sent the message.
     */
    void HandleExamineOperation(psActionLocation* action, Client* client);

    /**
     * Handles Script Operation for a action location.
     *
     * @param action      The action that is to be performed.
     * @param actor       The actor triggered the script.
     */
    void HandleScriptOperation(psActionLocation* action, gemActor* actor);

    // Current action location data
    csString triggerType;
    csString sectorName;
    csString meshName;
    csVector3 position;


    psDatabase*              database;
    csHash<psActionLocation*> actionLocationList;
    csHash<psActionLocation*> actionLocation_by_name;
    csHash<psActionLocation*> actionLocation_by_sector;
    csHash<psActionLocation*, uint32> actionLocation_by_id;
    csHash<const psActionLocation*> activeTriggers;

};

#endif
