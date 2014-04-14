/*
* actionmanager.cpp
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits :
*            Michael Cummings <cummings.michael@gmail.com>
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

#include <psconfig.h>
//=============================================================================
// CrystalSpace Includes
//=============================================================================
#include <csutil/xmltiny.h>
#include <csgeom/math3d.h>
#include <iutil/object.h>
#include <iengine/campos.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/eventmanager.h"

#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"

#include "net/msghandler.h"
#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "actionmanager.h"
#include "gem.h"
#include "clients.h"
#include "events.h"
#include "cachemanager.h"
#include "netmanager.h"
#include "npcmanager.h"
#include "psserverchar.h"
#include "entitymanager.h"
#include "progressionmanager.h"
#include "adminmanager.h"
#include "minigamemanager.h"


psActionTimeoutGameEvent::psActionTimeoutGameEvent(ActionManager* mgr, const psActionLocation* actionLocation, EID actorEID)
    : psGameEvent(0, 1000, "psActionTimeoutGameEvent"), actionManager(mgr), actorEID(actorEID), actionLocation(actionLocation)
{
    valid = false;
    if(actionManager)
    {
        valid = true;
    }
}

psActionTimeoutGameEvent::~psActionTimeoutGameEvent()
{
    valid = false;
    actionManager = NULL;
    actionLocation = NULL;
    actorEID = 0;
}

void psActionTimeoutGameEvent::Trigger()
{
    psSectorInfo* sector;
    float pos_x, pos_y, pos_z, yrot;
    InstanceID instance;
    gemActor* actor = (gemActor*)psserver->entitymanager->GetGEM()->FindObject(actorEID);

    if(valid && actor && actionLocation)
    {
        actor->GetCharacterData()->GetLocationInWorld(instance, sector, pos_x, pos_y, pos_z, yrot);
        csVector3 clientPos(pos_x, pos_y, pos_z);

        if(csSquaredDist::PointPoint(clientPos, actionLocation->position) > (actionLocation->radius * actionLocation->radius))
        {
            actionManager->RemoveActiveTrigger(actorEID, actionLocation);
            valid = false;
        }
        else
        {
            // still in range create new event
            psActionTimeoutGameEvent* newevent = new psActionTimeoutGameEvent(actionManager, actionLocation, actorEID);
            psserver->GetEventManager()->Push(newevent);
        }
    }
}

//----------------------------------------------------------------------------

ActionManager::ActionManager(psDatabase* db)
{
    database = db;

    // Action Messages from client that need handling
    Subscribe(&ActionManager::HandleMapAction, MSGTYPE_MAPACTION, REQUIRE_READY_CLIENT);
}


ActionManager::~ActionManager()
{
    // Unsubscribe from Messages

    csHash<psActionLocation*>::GlobalIterator it(actionLocationList.GetIterator());
    while(it.HasNext())
    {
        psActionLocation* actionLocation = it.Next();
        delete actionLocation;
    }

    database = NULL;
}

void ActionManager::HandleMapAction(MsgEntry* me, Client* client)
{
    psMapActionMessage msg(me);

    if(!msg.valid)
        return;

    switch(msg.command)
    {
        case psMapActionMessage::QUERY :
            HandleQueryMessage(msg.actionXML, client);
            break;
        case psMapActionMessage::SAVE :
            HandleSaveMessage(msg.actionXML, client);
            break;
        case psMapActionMessage::LIST_QUERY :
            HandleListMessage(msg.actionXML, client);
            break;
        case psMapActionMessage::DELETE_ACTION:
            HandleDeleteMessage(msg.actionXML, client);
            break;
        case psMapActionMessage::RELOAD_CACHE:
            HandleReloadMessage(client);
            break;
    }
}

void ActionManager::HandleQueryMessage(csString xml, Client* client)
{
    csString  triggerType;
    bool handled = false;

    // Search for matching locations
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, topNode, node;

    if((doc = ParseString(xml)) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location")))
            && (node = topNode->GetNode("triggertype")))
    {
        triggerType = node->GetContentsValue();

        LoadXML(topNode);

        if(triggerType.CompareNoCase("SELECT"))
        {
            handled = HandleSelectQuery(topNode, client);
        }

        if(!handled)
        {
            // evk: Sending to clients with security level 30+ only.
            if(client->GetSecurityLevel() >= GM_DEVELOPER)
            {
                // Set target for later use
                client->SetMesh(meshName);
                psserver->SendSystemError(client->GetClientNum(),"You have targeted %s.",meshName.GetData());

                // Send unhandled message to client
                psMapActionMessage msg(client->GetClientNum(), psMapActionMessage::NOT_HANDLED, xml);
                if(msg.valid)
                {
                    msg.SendMessage();
                }
            }
            //send actions possible based on items on hand (probably not the best place)
            else if(client->GetCharacterData())
            {
                //should be done with a for checking the entire equipped inventory for now for the use we need we just
                //check the two hands as a start.
                psItem* item = client->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_RIGHTHAND);
                if(!item || !item->GetItemCommand().Length())
                    item = client->GetCharacterData()->Inventory().GetInventoryItem(PSCHARACTER_SLOT_LEFTHAND);
                if(item && item->GetItemCommand().Length())
                {
                    psGUIInteractMessage interactMsg(client->GetClientNum(), psGUIInteractMessage::GENERIC, item->GetItemCommand());
                    interactMsg.SendMessage();
                }
            }
        }
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s",
               client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }
}

void ActionManager::LoadXML(iDocumentNode* topNode)
{
    csRef<iDocumentNode> node;

    // trigger type
    node = topNode->GetNode("triggertype");
    if(node)
    {
        triggerType = node->GetContentsValue();
    }

    // sector
    node = topNode->GetNode("sector");
    if(node)
    {
        sectorName = node->GetContentsValue();
    }

    // mesh
    node = topNode->GetNode("mesh");
    if(node)
    {
        meshName = node->GetContentsValue();
    }

    csRef<iDocumentNode> posNode = topNode->GetNode("position");
    if(posNode)
    {
        float posx=0.0f, posy=0.0f, posz=0.0f;

        node = posNode->GetNode("x");
        if(node) posx = node->GetContentsValueAsFloat();

        node = posNode->GetNode("y");
        if(node) posy = node->GetContentsValueAsFloat();

        node = posNode->GetNode("z");
        if(node) posz = node->GetContentsValueAsFloat();

        position = csVector3(posx, posy, posz);
    }
}


bool ActionManager::HandleSelectQuery(iDocumentNode* topNode, Client* client)
{
    bool handled = false;

    psActionLocation* search = new psActionLocation();
    search->Load(topNode);
    if(client && client->GetActor())
        search->SetInstance(client->GetActor()->GetInstance());

    // Search for matches
    csArray<psActionLocation*> matchMesh;   // matches on sector + mesh
    csArray<psActionLocation*> matchPoly;   // matches on sector + mesh + poly or point w/in radius
    csArray<psActionLocation*> matchPoint;  // matches on sector + mesh + poly + point w/in radius
    csArray<psActionLocation*> matchInstance;  // matches on sector + mesh + poly + point w/in radius + instance
    csArray<psActionLocation*> matches;
    psActionLocation* actionLocation;


    csString hashKey = triggerType + sectorName + meshName;
    csHash<psActionLocation*>::Iterator iter(actionLocationList.GetIterator(csHashCompute(hashKey)));
    while(iter.HasNext())
    {
        actionLocation = iter.Next();

        switch(actionLocation->IsMatch(search))
        {
            case 0: // No Match
                break;
            case 1: // Match on Sector + Mesh
                matchMesh.Push(actionLocation);
                break;
            case 2: // Match on Poly or Point
                matchPoly.Push(actionLocation);
                break;
            case 3: // Match on Poly and Point
                matchPoint.Push(actionLocation);
                break;
            case 4: //match on instance
                matchInstance.Push(actionLocation);
                break;
        }
    }

    // Use correct Set of Matches
    if(matchInstance.GetSize() != 0)
        matchInstance.TransferTo(matches);
    if(matchPoint.GetSize() != 0)
        matchPoint.TransferTo(matches);
    else if(matchPoly.GetSize() != 0)
        matchPoly.TransferTo(matches);
    else if(matchMesh.GetSize() != 0)
        matchMesh.TransferTo(matches);

    // ProcessMatches
    handled = ProcessMatches(matches, client);

    // cleanup
    delete search;

    return handled;
}

void ActionManager::NotifyProximity(gemActor* actor, gemActionLocation* actionLocationObject, float range)
{
    // Actor and action Location has been percepted as close by the proximity list.
    // Now lets check if this should trigger en effect....

    psActionLocation* actionLocation = actionLocationObject->GetAction();
    if(actionLocation->triggertype != psActionLocation::TRIGGERTYPE_PROXIMITY)
    {
        return;  // ActionLocation not of type PROXIMITY.
    }

    if(range > actionLocation->radius)
    {
        return; // Outside range of action location.
    }

    if(HasActiveTrigger(actor->GetEID(),actionLocation))
    {
        return; // Trigger is active for this client.
    }

    if(actionLocation->responsetype == "SCRIPT")
    {
        HandleScriptOperation(actionLocation, actor);

        AddActiveTrigger(actor->GetEID(),actionLocation);
    }
}



psActionLocation* ActionManager::FindAction(EID id)
{
    // FIXME : at the moment, the ALID is used as the EID
    // it's an ugly hack, that should be changed in the future.
    // this function should be changed too when this is done
    return actionLocation_by_id.Get(id.Unbox(), NULL);
}

psActionLocation* ActionManager::FindActionByID(uint32 id)
{
    return actionLocation_by_id.Get(id, NULL);
}

psActionLocation* ActionManager::FindAvailableEntrances(csString entranceSector)
{
    psActionLocation* actionLocation;
    csHash<psActionLocation*>::GlobalIterator iter(actionLocationList.GetIterator());

    while(iter.HasNext())
    {
        actionLocation = iter.Next();
        if(actionLocation->IsEntrance() && !actionLocation->IsActive())
        {
            if(actionLocation->GetEntranceSector() == entranceSector)
            {
                return actionLocation;
            }
        }
    }
    return NULL;
}

bool ActionManager::ProcessMatches(csArray<psActionLocation*> matches, Client* client)
{
    bool handled = false;
    psActionLocation* actionLocation;

    // Call correct OperationHandler
    csArray<psActionLocation*>::Iterator results(matches.GetIterator());
    while(results.HasNext())
    {
        actionLocation = results.Next();

        if(actionLocation->IsActive())
        {
            if(actionLocation->responsetype == "EXAMINE")
            {
                HandleExamineOperation(actionLocation, client);
                handled = true;
            }

            if(actionLocation->responsetype == "SCRIPT")
            {
                HandleScriptOperation(actionLocation, client->GetActor());
                handled = true;
            }
        }
    }

    return handled;
}

void ActionManager::HandleListMessage(csString xml, Client* client)
{
    if(client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    csString  sectorName;

    // Search for matching locations
    csRef<iDocument> doc ;
    csRef<iDocumentNode> root, topNode, node;

    if((doc = ParseString(xml)) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location")))
            && (node = topNode->GetNode("sector"))) // sector
    {
        sectorName = node->GetContentsValue();

        // Call proper operation
        csString responseXML("");
        responseXML.Append("<locations>");

        csHash<psActionLocation*>::Iterator iter(actionLocation_by_sector.GetIterator(csHashCompute(sectorName)));
        while(iter.HasNext())
        {
            psActionLocation* actionLocation = iter.Next();

            responseXML.Append(actionLocation->ToXML());
        }
        responseXML.Append("</locations>");

        psMapActionMessage msg(client->GetClientNum(), psMapActionMessage::LIST, responseXML);
        if(msg.valid)
            msg.SendMessage();
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s",
               client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }

}

void ActionManager::HandleSaveMessage(csString xml, Client* client)
{
    if(client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    // Search for matching locations
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, topNode;
    if((doc = ParseString(xml)) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location"))))   // sector
    {
        csRef<iDocumentNode> node;
        psActionLocation* action = NULL;

        node = topNode->GetNode("id");
        if(!node) return;
        csString id(node->GetContentsValue());

        node = topNode->GetNode("name");
        if(!node) return;
        csString name(node->GetContentsValue());

        node = topNode->GetNode("sector");
        if(!node) return;
        csString sectorName(node->GetContentsValue());

        node = topNode->GetNode("mesh");
        if(!node) return;
        csString meshName(node->GetContentsValue());

        if(id.Length() == 0)
        {
            // Add New Location
            action = new psActionLocation();
            CS_ASSERT(action != NULL);
        }
        else
        {
            // Update existing location
            psActionLocation* current;

            // Find Elememt
            csHash<psActionLocation*>::Iterator iter(actionLocation_by_sector.GetIterator(csHashCompute(sectorName)));
            while(iter.HasNext())
            {
                current = iter.Next();
                CS_ASSERT(current != NULL);

                if(current->id == (size_t)atoi(id.GetData()))
                {
                    action = current;
                }
            }

            // Remove from Cache

            csString hashKey = action->GetTriggerTypeAsString() + action->sectorname + action->meshname;
            actionLocationList.Delete(csHashCompute(hashKey), action);
            actionLocation_by_name.Delete(csHashCompute(action->name), action);
            actionLocation_by_sector.Delete(csHashCompute(action->sectorname), action);
            actionLocation_by_id.Delete(action->id, action);
        }

        if(action != NULL)
        {
            // Update DB
            if(action->Load(topNode))
            {
                action->Save();

                //Update Cache
                if(!CacheActionLocation(action))
                {
                    Error2("Failed to populate action : \"%s\"", action->name.GetData());
                    delete action;
                }
            }
            else
            {
                Error2("Failed to load action : \"%s\"", action->name.GetData());
                delete action;
            }
        }

        csString xmlMsg;
        csString escpxml = EscpXML(sectorName);
        xmlMsg.Format("<location><sector>%s</sector></location>", escpxml.GetData());
        HandleListMessage(xmlMsg, client);
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s",
               client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }
}

void ActionManager::HandleDeleteMessage(csString xml, Client* client)
{
    if(client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    // Search for matching locations
    csRef<iDocument>     doc;
    csRef<iDocumentNode> root, topNode, node;

    if((doc = ParseString(xml)) && ((root = doc->GetRoot())) && ((topNode = root->GetNode("location")))
            && (node = topNode->GetNode("id")))
    {
        csString id(node->GetContentsValue());

        // Find Elememt
        psActionLocation* current, *actionLocation = NULL;
        csHash<psActionLocation*>::GlobalIterator iter(actionLocationList.GetIterator());
        while(iter.HasNext())
        {
            current = iter.Next();
            CS_ASSERT(current != NULL);

            if(current->id == (size_t)atoi(id.GetData()))
            {
                actionLocation = current;
            }
        }

        // No Match
        if(!actionLocation) return;

        csString sectorName = actionLocation->sectorname;

        // Update DB
        if(actionLocation->Delete())
        {
            // Remove from Cache
            actionLocationList.Delete(csHashCompute(actionLocation->GetTriggerTypeAsString() + actionLocation->sectorname + actionLocation->meshname), actionLocation);
            actionLocation_by_name.Delete(csHashCompute(actionLocation->name), actionLocation);
            actionLocation_by_sector.Delete(csHashCompute(actionLocation->sectorname), actionLocation);
            actionLocation_by_id.Delete(actionLocation->id, actionLocation);

            if(actionLocation->gemAction != NULL)
                delete actionLocation->gemAction;
            delete actionLocation;
        }

        csString xmlMsg;
        csString escpxml = EscpXML(sectorName);
        xmlMsg.Format("<location><sector>%s</sector></location>", escpxml.GetData());
        HandleListMessage(xmlMsg, client);
    }
    else
    {
        Error4("Player (%s)(%s) tried to send a bogus XML string as an action message\nString was %s",
               client->GetActor()->GetCharacterData()->GetCharName(), ShowID(client->GetActor()->GetPID()), xml.GetData());
    }


}

void ActionManager::HandleReloadMessage(Client* client)
{
    if(client->GetSecurityLevel() <= GM_LEVEL_9)
    {
        psserver->SendSystemError(client->GetClientNum(), "Access is denied. Only Admin level 9 can manage Actions.");
        return;
    }

    csHash<psActionLocation*>::GlobalIterator it(actionLocationList.GetIterator());
    while(it.HasNext())
    {
        psActionLocation* actionLocation = it.Next();
        if(actionLocation->gemAction != NULL)
            delete actionLocation->gemAction;
        delete actionLocation;
    }

    // reset game sessions
    psserver->GetMiniGameManager()->ResetAllGameSessions();

    actionLocationList.DeleteAll();
    actionLocation_by_sector.DeleteAll();
    actionLocation_by_name.DeleteAll();
    actionLocation_by_id.DeleteAll();

    //send message to the user reload is complete
    if(!RepopulateActionLocations())
    {
        psserver->SendSystemError(client->GetClientNum(),"One or more Action Locations failed to reload.");
    }
    else
    {
        psserver->SendSystemResult(client->GetClientNum(),"Action Location cache is now reloaded.");
    }
}

void ActionManager::HandleExamineOperation(psActionLocation* action, Client* client)
{
    // Assume you are allowed in unless there is a script that returns false
    bool allowedEntry = true;

    // Create Entity on Client
    action->Send(client->GetClientNum());

    // Set as the target
    client->SetTargetObject(action->GetGemObject(), true);

    // Find the real item if it exists
    gemItem* realItem = action->GetRealItem();

    // If no enter check is specified, do nothing
    if(action->GetEnterScript())
    {
        MathEnvironment env;
        env.Define("Actor", client->GetActor());

        allowedEntry = action->GetEnterScript()->Evaluate(&env) != 0.0;
    }

    // Invoke Interaction menu
    uint32_t options = psGUIInteractMessage::EXAMINE;

    // Is action location an entrance
    if(action->IsEntrance())
    {
        // If there is a real item associated with the entrance must be a lock
        if(realItem)
        {
            // Is there a security lock
            if(realItem->IsSecurityLocked())
            {
                options |= psGUIInteractMessage::ENTERLOCKED;
            }

            // Is there a lock
            if(realItem->IsLockable())
            {
                // Is it locked
                if(realItem && realItem->IsLocked())
                    options |= psGUIInteractMessage::UNLOCK;
                else
                    options |= psGUIInteractMessage::LOCK;
            }
        }
        // Otherwise walk right in if script allowed
        else if(allowedEntry)
        {
            options |= psGUIInteractMessage::ENTER;
        }
    }

    // Or a game board
    else if(action->IsGameBoard())
    {
        options |= psGUIInteractMessage::PLAYGAME;
    }
    // Or an examine + script action
    else if(action->IsExamineScript())
    {
        options |= psGUIInteractMessage::USE;
    }
    // Everything else
    else if(realItem)
    {
        // Container items can combine
        if(realItem->IsContainer())
            options |= psGUIInteractMessage::COMBINE;
        if(realItem->IsConstructible())
            options |= psGUIInteractMessage::CONSTRUCT;
        // All other action location items can be used
        options |= psGUIInteractMessage::USE;
    }
    options |= psGUIInteractMessage::CLOSE;

    psGUIInteractMessage interactMsg(client->GetClientNum(), options);
    interactMsg.SendMessage();
}

// Handle /use command
bool ActionManager::HandleUse(gemActionLocation* actionlocation, Client* client)
{
    if(client->GetActor()->RangeTo(actionlocation, true, true) > RANGE_TO_USE)
    {
        psserver->SendSystemError(client->GetClientNum(),"You are not in range to use %s.",actionlocation->GetName());
        return false;
    }
    // find the script
    ProgressionScript* progScript = psserver->GetProgressionManager()->FindScript(actionlocation->GetAction()->GetScriptToRun().GetData());
    if(progScript)
    {
        csString parameters = actionlocation->GetAction()->GetScriptParameters();
        // passes the parameters to the script to run
        MathScript* bindings = MathScript::Create("RunScript bindings", parameters);

        MathEnvironment env;
        env.Define("Target", client->GetActor()); // needed if called by an action location
        bindings->Evaluate(&env);
        MathScript::Destroy(bindings);
        progScript->Run(&env);
        return true;
    }
    return false;
}

void ActionManager::HandleScriptOperation(psActionLocation* action, gemActor* actor)
{
    // if no event is specified, do nothing
    if(action->response.Length() > 0)
    {
        ProgressionScript* script = psserver->GetProgressionManager()->FindScript(action->response);
        if(!script)
        {
            Error2("Failed to find progression script \"%s\"", action->response.GetData());
            return;
        }

        // Find the real item if there exist
        gemItem* realItem = action->GetRealItem();

        MathEnvironment env;
        env.Define("Actor", actor);
        if(realItem)
        {
            env.Define("Item", realItem->GetItem());
        }

        script->Run(&env);
    }
}

void ActionManager::RemoveActiveTrigger(EID actorEID, const psActionLocation* actionLocation)
{
    csString key("");
    size_t   id = actionLocation->id;
    key.AppendFmt("%zu", id);
    key.AppendFmt("%u", actorEID.Unbox());

    csHash<const psActionLocation*>::Iterator active(activeTriggers.GetIterator(csHashCompute(key)));

    while(active.HasNext())
    {
        activeTriggers.Delete(csHashCompute(key), active.Next());
    }
}

bool ActionManager::HasActiveTrigger(EID actorEID, const psActionLocation* actionLocation)
{
    csString key("");
    size_t   id = actionLocation->id;
    key.AppendFmt("%zu", id);
    key.AppendFmt("%u", actorEID.Unbox());
    csHash<const psActionLocation*>::Iterator active(activeTriggers.GetIterator(csHashCompute(key)));

    // There is a active trigger if this iterator has a next element.
    return (active.HasNext());
}

void ActionManager::AddActiveTrigger(EID actorEID, const psActionLocation* actionLocation)
{
    csString key("");
    size_t   id = actionLocation->id;
    key.AppendFmt("%zu", id);
    key.AppendFmt("%u", actorEID.Unbox());
    activeTriggers.Put(csHashCompute(key), actionLocation);

    // Create game event to check if the actor move outside trigger area again. While
    // inside the game event will be recreated until client can be removed from active triggers lists.
    psActionTimeoutGameEvent* event = new psActionTimeoutGameEvent(this, actionLocation, actorEID);
    psserver->GetEventManager()->Push(event);
}

bool ActionManager::RepopulateActionLocations(psSectorInfo* sectorinfo)
{
    unsigned int currentrow;
    psActionLocation* newaction;
    csString query;

    if(sectorinfo)
    {
        query.Format("SELECT al.*, master.triggertype master_triggertype, master.responsetype master_responsetype, master.response master_response FROM action_locations al LEFT OUTER JOIN action_locations master ON al.master_id = master.id WHERE al.sectorname='%s'", sectorinfo->name.GetData());
    }
    else
    {
        query = "SELECT al.*, master.triggertype master_triggertype, master.responsetype master_responsetype, master.response master_response FROM action_locations al LEFT OUTER JOIN action_locations master ON al.master_id = master.id";
    }

    Result result(db->Select(query));

    if(!result.IsValid())
        return false;

    bool errorDetected = false;
    for(currentrow = 0; currentrow < result.Count(); currentrow++)
    {
        newaction = new psActionLocation();
        CS_ASSERT(newaction != NULL);

        if(newaction->Load(result[currentrow]))
        {
            if(!CacheActionLocation(newaction))
            {
                Error2("Failed to populate action : \"%s\"", newaction->name.GetData());
                delete newaction;
                errorDetected = true;
            }
        }
        else
        {
            Error2("Failed to find load action : \"%s\"", newaction->name.GetData());
            delete newaction;
            errorDetected = true;
        }
    }
    return !errorDetected;
}


bool ActionManager::CacheActionLocation(psActionLocation* action)
{

    // Parse the action response string
    if(!action->ParseResponse())
    {
        Error2("Failed to parse response string for action : \"%s\"", action->name.GetData());
        return false;
    }

    // Create location and add to hash lists
    if(EntityManager::GetSingleton().CreateActionLocation(action, false))
    {
        csString hashKey = action->GetTriggerTypeAsString() + action->sectorname + action->meshname;
        actionLocationList.Put(csHashCompute(hashKey), action);
        actionLocation_by_name.Put(csHashCompute(action->name), action);
        actionLocation_by_sector.Put(csHashCompute(action->sectorname), action);
        actionLocation_by_id.Put(action->id, action);
    }
    else
    {
        Error2("Failed to create action : \"%s\"", action->name.GetData());
        return false;
    }
    return true;
}

