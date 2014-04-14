/*
* networkmgr.cpp
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

#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <iutil/vfs.h>
#include <iengine/engine.h>
#include <csutil/strhashr.h>


//=============================================================================
// Project Space Includes
//=============================================================================
#include <ibgloader.h>
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/command.h"
#include "util/eventmanager.h"

#include "net/connection.h"
#include "net/messages.h"
#include "net/msghandler.h"
#include "net/npcmessages.h"
#include "engine/linmove.h"
#include "util/strutil.h"
#include "util/waypoint.h"
#include "engine/psworld.h"
#include "rpgrules/vitals.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "networkmgr.h"
#include "globals.h"
#include "npcclient.h"
#include "npc.h"
#include "gem.h"
#include "tribe.h"
#include "perceptions.h"
#include "npcbehave.h"

extern bool running;

NetworkManager::NetworkManager(MsgHandler* mh,psNetConnection* conn, iEngine* engine)
    : port(0),reconnect(false)
{
    msghandler = mh;
    this->engine = engine;
    ready = false;
    connected = false;

    msghandler->Subscribe(this,MSGTYPE_NPCLIST);
    msghandler->Subscribe(this,MSGTYPE_MAPLIST);
    msghandler->Subscribe(this,MSGTYPE_CELPERSIST);
    msghandler->Subscribe(this,MSGTYPE_ALLENTITYPOS);
    msghandler->Subscribe(this,MSGTYPE_NPCCOMMANDLIST);
    msghandler->Subscribe(this,MSGTYPE_PERSIST_ALL_ENTITIES);
    msghandler->Subscribe(this,MSGTYPE_PERSIST_ACTOR);
    msghandler->Subscribe(this,MSGTYPE_PERSIST_ITEM);
    msghandler->Subscribe(this,MSGTYPE_REMOVE_OBJECT);
    msghandler->Subscribe(this,MSGTYPE_DISCONNECT);
    msghandler->Subscribe(this,MSGTYPE_WEATHER);
    msghandler->Subscribe(this,MSGTYPE_MSGSTRINGS);
    msghandler->Subscribe(this,MSGTYPE_NEW_NPC);
    msghandler->Subscribe(this,MSGTYPE_NPC_DELETED);
    msghandler->Subscribe(this,MSGTYPE_NPC_COMMAND);
    msghandler->Subscribe(this,MSGTYPE_NPCRACELIST);
    msghandler->Subscribe(this,MSGTYPE_NPC_WORKDONE);
    msghandler->Subscribe(this,MSGTYPE_PATH_NETWORK);
    msghandler->Subscribe(this,MSGTYPE_LOCATION);
    msghandler->Subscribe(this,MSGTYPE_HIRED_NPC_SCRIPT);

    connection= conn;
    connection->SetEngine(engine);

    PrepareCommandMessage();
}

NetworkManager::~NetworkManager()
{
    if(msghandler)
    {
        msghandler->Unsubscribe(this,MSGTYPE_NPCLIST);
        msghandler->Unsubscribe(this,MSGTYPE_MAPLIST);
        msghandler->Unsubscribe(this,MSGTYPE_CELPERSIST);
        msghandler->Unsubscribe(this,MSGTYPE_ALLENTITYPOS);
        msghandler->Unsubscribe(this,MSGTYPE_NPCCOMMANDLIST);
        msghandler->Unsubscribe(this,MSGTYPE_PERSIST_ACTOR);
        msghandler->Unsubscribe(this,MSGTYPE_PERSIST_ITEM);
        msghandler->Unsubscribe(this,MSGTYPE_REMOVE_OBJECT);
        msghandler->Unsubscribe(this,MSGTYPE_DISCONNECT);
        msghandler->Unsubscribe(this,MSGTYPE_WEATHER);
        msghandler->Unsubscribe(this,MSGTYPE_MSGSTRINGS);
        msghandler->Unsubscribe(this,MSGTYPE_NPC_COMMAND);
        msghandler->Unsubscribe(this,MSGTYPE_NPCRACELIST);
        msghandler->Unsubscribe(this,MSGTYPE_NPC_WORKDONE);
        msghandler->Unsubscribe(this,MSGTYPE_PATH_NETWORK);
        msghandler->Unsubscribe(this,MSGTYPE_LOCATION);
        msghandler->Unsubscribe(this,MSGTYPE_HIRED_NPC_SCRIPT);
    }

    delete outbound;
    outbound = NULL;
}

void NetworkManager::Authenticate(csString &host,int port,csString &user,csString &pass)
{
    this->port = port;
    this->host = host;
    this->user = user;
    this->password = pass;

    psNPCAuthenticationMessage login(0,user,pass);
    msghandler->SendMessage(login.msg);
}

void NetworkManager::Disconnect()
{
    psDisconnectMessage discon(0, 0, "");
    msghandler->SendMessage(discon.msg);
    connection->SendOut(); // Flush the network
    connection->DisConnect();
}

csStringHashReversible* NetworkManager::GetMsgStrings()
{
    return connection->GetAccessPointers()->msgstringshash;
}

const char* NetworkManager::GetCommonString(uint32_t cstr_id)
{
    return connection->GetAccessPointers()->Request(cstr_id);
}

uint32_t NetworkManager::GetCommonStringID(const char* string)
{
    return connection->GetAccessPointers()->Request(string).GetHash();
}


void NetworkManager::HandleMessage(MsgEntry* message)
{
    switch(message->GetType())
    {
        case MSGTYPE_MAPLIST:
        {
            connected = true;
            if(HandleMapList(message))
            {
                RequestAllObjects();
            }
            else
            {
                npcclient->Disconnect();
            }

            break;
        }
        case MSGTYPE_NPCRACELIST:
        {
            HandleRaceList(message);
            break;
        }
        case MSGTYPE_NPCLIST:
        {
            HandleNPCList(message);
            ready = true;
            // Activates NPCs on server side
            psNPCReadyMessage mesg;
            msghandler->SendMessage(mesg.msg);
            npcclient->LoadCompleted();
            break;
        }
        case MSGTYPE_PERSIST_ALL_ENTITIES:
        {
            HandleAllEntities(message);
            break;
        }
        case MSGTYPE_PERSIST_ACTOR:
        {
            HandleActor(message);
            break;
        }

        case MSGTYPE_PERSIST_ITEM:
        {
            HandleItem(message);
            break;
        }

        case MSGTYPE_REMOVE_OBJECT:
        {
            HandleObjectRemoval(message);
            break;
        }

        case MSGTYPE_ALLENTITYPOS:
        {
            HandlePositionUpdates(message);
            break;
        }
        case MSGTYPE_NPCCOMMANDLIST:
        {
            HandlePerceptions(message);
            break;
        }
        case MSGTYPE_MSGSTRINGS:
        {
            csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());

            // receive message strings hash table
            psMsgStringsMessage msg(message);
            connection->SetMsgStrings(0, msg.msgstrings);
            break;
        }
        case MSGTYPE_DISCONNECT:
        {
            HandleDisconnect(message);
            break;
        }
        case MSGTYPE_WEATHER:
        {
            HandleTimeUpdate(message);
            break;
        }
        case MSGTYPE_NEW_NPC:
        {
            HandleNewNpc(message);
            break;
        }
        case MSGTYPE_NPC_DELETED:
        {
            HandleNpcDeleted(message);
            break;
        }
        case MSGTYPE_NPC_COMMAND:
        {
            HandleConsoleCommand(message);
            break;
        }
        case MSGTYPE_NPC_WORKDONE:
        {
            HandleNPCWorkDone(message);
            break;
        }
        case MSGTYPE_PATH_NETWORK:
        {
            HandlePathNetwork(message);
            break;
        }
        case MSGTYPE_LOCATION:
        {
            HandleLocation(message);
            break;
        }
        case MSGTYPE_HIRED_NPC_SCRIPT:
        {
            HandleHiredNPCScript(message);
            break;
        }
    }
}

void NetworkManager::HandleConsoleCommand(MsgEntry* me)
{
    csString buffer;

    psServerCommandMessage msg(me);
    printf("Got command: %s\n", msg.command.GetDataSafe());

    size_t i = msg.command.FindFirst(' ');
    csString word;
    msg.command.SubString(word,0,i);
    const COMMAND* cmd = find_command(word.GetDataSafe());

    if(cmd && cmd->allowRemote)
    {
        int ret = execute_line(msg.command, &buffer);
        if(ret == -1)
        {
            buffer = "Error executing command on the server.";
        }
    }
    else
    {
        buffer = cmd ? "That command is not allowed to be executed remotely"
                 : "No command by that name.  Please try again.";
    }
    printf("%s\n", buffer.GetData());

    /*psServerCommandMessage retn(me->clientnum, buffer);
    retn.SendMessage();*/
}

void NetworkManager::HandleRaceList(MsgEntry* me)
{
    psNPCRaceListMessage mesg(me);

    size_t count = mesg.raceInfo.GetSize();
    for(size_t c = 0; c < count; c++)
    {
        npcclient->AddRaceInfo(mesg.raceInfo[c]);
        Debug4(LOG_STARTUP,0, "Loading race speed: %s %f %f\n",mesg.raceInfo[c].name.GetData(),mesg.raceInfo[c].walkSpeed,mesg.raceInfo[c].runSpeed);
    }
}

void NetworkManager::HandleAllEntities(MsgEntry* message)
{
    psPersistAllEntities allEntities(message);

    printf("Got All Entities message.\n");
    bool done=false;
    int count=0;
    while(!done)
    {
        count++;
        MsgEntry* entity = allEntities.GetEntityMessage();
        if(entity)
        {
            if(entity->GetType() == MSGTYPE_PERSIST_ACTOR)
                HandleActor(entity);
            else if(entity->GetType() == MSGTYPE_PERSIST_ITEM)
                HandleItem(entity);
            else
                Error2("Unhandled type of entity (%d) in AllEntities message.",entity->GetType());

            delete entity;
        }
        else
            done = true;
    }
    printf("End of All Entities message, with %d entities done.\n", count);
}

void NetworkManager::HandleActor(MsgEntry* msg)
{
    psPersistActor mesg(msg, connection->GetAccessPointers(), true);

    Debug4(LOG_NET, 0, "Got persistActor message, size %zu, id=%d, name=%s", msg->GetSize(),mesg.playerID.Unbox(),mesg.name.GetDataSafe());

    gemNPCObject* obj = npcclient->FindEntityID(mesg.entityid);

    if(obj && obj->GetPID() == mesg.playerID)
    {
        // We already know this entity so just update the entity.
        CPrintf(CON_ERROR, "Already know about gemNPCActor: %s (%s), %s %s.\n",
                mesg.name.GetData(), obj->GetName(), ShowID(mesg.playerID), ShowID(mesg.entityid));

        obj->Move(mesg.pos, mesg.yrot, mesg.sectorName, mesg.instance);
        obj->SetInvisible((mesg.flags & psPersistActor::INVISIBLE) ? true : false);
        obj->SetInvincible((mesg.flags & psPersistActor::INVINCIBLE) ? true : false);
        obj->SetAlive((mesg.flags & psPersistActor::IS_ALIVE) ? true : false);
        NPC* npc = obj->GetNPC();
        if(npc)
        {
            npc->SetAlive(obj->IsAlive());
        }

        return;
    }

    if(!obj && mesg.playerID != 0)
    {
        // Check if we find obj in characters
        obj = npcclient->FindCharacterID(mesg.playerID);
    }

    if(obj)
    {
        // We have a player id, entity id mismatch and we already know this entity
        // so we can only assume a RemoveObject message misorder and we will delete the existing one and recreate.
        CPrintf(CON_ERROR, "Deleting because we already know gemNPCActor: "
                "<%s, %s, %s> as <%s, %s, %s>.\n",
                mesg.name.GetData(), ShowID(mesg.entityid), ShowID(mesg.playerID),
                obj->GetName(), ShowID(obj->GetEID()), ShowID(obj->GetPID()));

        npcclient->Remove(obj);
        obj = NULL; // Obj isn't valid after remove
    }

    gemNPCActor* actor = new gemNPCActor(npcclient, mesg);

    if(mesg.flags & psPersistActor::NPC)
    {
        npcclient->AttachNPC(actor, mesg.counter, mesg.ownerEID, mesg.masterID);
    }

    npcclient->Add(actor);

}

void NetworkManager::HandleItem(MsgEntry* me)
{
    psPersistItem mesg(me, connection->GetAccessPointers());

    gemNPCObject* obj = npcclient->FindEntityID(mesg.eid);

    Debug4(LOG_NET, 0, "Got persistItem message, size %zu, eid=%d, name=%s\n", me->GetSize(),mesg.eid.Unbox(),mesg.name.GetDataSafe());

    if(obj && obj->GetPID().IsValid())
    {
        // We have a player/NPC item mismatch.
        CPrintf(CON_ERROR, "Deleting because we already know gemNPCActor: "
                "<%s, %s> as <%s, %s>.\n",
                mesg.name.GetData(), ShowID(mesg.eid), obj->GetName(), ShowID(obj->GetEID()));

        npcclient->Remove(obj);
        obj = NULL; // Obj isn't valid after remove
    }


    if(obj)
    {
        // We already know this item so just update the position.
        CPrintf(CON_ERROR, "Deleting because we already know "
                "gemNPCItem: %s (%s), %s.\n", mesg.name.GetData(),
                obj->GetName(), ShowID(mesg.eid));

        npcclient->Remove(obj);
        obj = NULL; // Obj isn't valid after remove
    }

    // Add the new item to the world
    gemNPCItem* item = new gemNPCItem(npcclient, mesg);
    npcclient->Add(item);

}

void NetworkManager::HandleObjectRemoval(MsgEntry* me)
{
    psRemoveObject mesg(me);

    gemNPCObject* object = npcclient->FindEntityID(mesg.objectEID);
    if(object == NULL)
    {
        CPrintf(CON_ERROR, "NPCObject %s cannot be removed - not found\n", ShowID(mesg.objectEID));
        return;
    }

    // If this is a NPC remove any queued dr updates before removing the entity.
    NPC* npc = object->GetNPC();
    if(npc)
    {
        DequeueDRData(npc);
    }

    npcclient->Remove(object);   // Object isn't valid after remove
}

void NetworkManager::HandlePathNetwork(MsgEntry* me)
{
    psPathNetworkMessage msg(me);

    psPathNetwork* pathNetwork = npcclient->GetPathNetwork();

    switch(msg.command)
    {
        case psPathNetworkMessage::PATH_ADD_POINT:
        {
            psPath* path = pathNetwork->FindPath(msg.id);
            if(path)
            {
                psPathPoint* point = path->AddPoint(msg.position, 0.0, msg.sector->QueryObject()->GetName());
                point->SetID(msg.secondId);

                Debug4(LOG_NET, 0, "Added point %d to path %d at %s.\n",
                       msg.secondId, msg.id, toString(msg.position,msg.sector).GetDataSafe());
            }
            else
            {
                Error3("Failed to add point %d to path %d.\n", msg.secondId, msg.id);
            }
        }
        break;
        case psPathNetworkMessage::PATH_CREATE:
        {
            psPath* path = new psLinearPath(msg.id,msg.string,msg.flags);
            if(path)
            {
                Waypoint* startWaypoint = pathNetwork->FindWaypoint(msg.startId);
                if(!startWaypoint)
                {
                    Error2("Failed to find start waypoint %d\n",msg.startId);
                    delete path;
                    return;
                }

                Waypoint* stopWaypoint = pathNetwork->FindWaypoint(msg.stopId);
                if(!stopWaypoint)
                {
                    Error2("Failed to find stop waypoint %d\n",msg.stopId);
                    delete path;
                    return;
                }

                path->SetStart(startWaypoint);
                path->SetEnd(stopWaypoint);

                if(pathNetwork->CreatePath(path) != NULL)
                {
                    Debug2(LOG_NET, 0, "Created path %d.\n", msg.id);
                }
            }
            else
            {
                Error2("Failed to create path %d.\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::PATH_REMOVE_POINT:
        {
            psPath* path = pathNetwork->FindPath(msg.id);
            if(path)
            {
                int index = path->FindPointIndex(msg.secondId);
                if(index >= 0)
                {
                    path->RemovePoint(index);
                    Debug3(LOG_NET, 0, "Removed point %d from path %d.\n", msg.secondId, msg.id);
                }
                else
                {
                    Error3("Removed point %d from path %d failed.\n", msg.secondId, msg.id);
                }

            }
            else
            {
                Error3("Failed to remove point %d from path %d.\n", msg.secondId, msg.id);
            }

        }
        break;
        case psPathNetworkMessage::PATH_RENAME:
        {
            psPath* path = pathNetwork->FindPath(msg.id);
            if(path)
            {
                path->Rename(msg.string);

                Debug3(LOG_NET, 0, "Set name %s for path %d.\n",
                       msg.string.GetDataSafe(), msg.id);
            }
            else
            {
                Error2("Failed to find path %d for rename\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::PATH_SET_FLAG:
        {
            psPath* path = pathNetwork->FindPath(msg.id);
            if(path)
            {
                path->SetFlag(msg.string, msg.enable);

                Debug4(LOG_NET, 0, "Set flag %s for path %d to %s.\n",
                       msg.string.GetDataSafe(), msg.id, msg.enable?"TRUE":"FALSE");
            }
            else
            {
                Error2("Failed to find path %d for set flag\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::POINT_ADJUSTED:
        {
            psPathPoint* point = pathNetwork->FindPathPoint(msg.id);
            if(point)
            {
                point->Adjust(msg.position, msg.sector);

                Debug3(LOG_NET, 0, "Adjusted pathpoint %d to %s.\n",
                       msg.id, toString(msg.position, msg.sector).GetDataSafe());
            }
            else
            {
                Error2("Failed to find pathpoint %d for adjust\n", msg.id);
            }
        }
        break;
        case psPathNetworkMessage::WAYPOINT_ALIAS_ADD:
        {
            Waypoint* wp = pathNetwork->FindWaypoint(msg.id);
            if(wp)
            {
                wp->AddAlias(msg.aliasID,msg.string,msg.rotationAngle);

                Debug3(LOG_NET, 0, "Added alias %s to waypoint %d.\n",
                       msg.string.GetDataSafe(), msg.id);
            }
            else
            {
                Error2("Failed to find waypoint %d for add alias\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_ALIAS_ROTATION_ANGLE:
        {
            Waypoint* wp = pathNetwork->FindWaypoint(msg.id);
            if(wp)
            {
                wp->SetRotationAngle(msg.string,msg.rotationAngle);

                Debug4(LOG_NET, 0, "Sett rotation angle %.3f for alias %s to waypoint %d.\n",
                       msg.rotationAngle,msg.string.GetDataSafe(), msg.id);
            }
            else
            {
                Error2("Failed to find waypoint %d for alias set rotation angle\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_ADJUSTED:
        {
            Waypoint* wp = pathNetwork->FindWaypoint(msg.id);
            if(wp)
            {
                wp->Adjust(msg.position, msg.sector);

                Debug3(LOG_NET, 0, "Adjusted waypoint %d to %s.\n",
                       msg.id, toString(msg.position, msg.sector).GetDataSafe());
            }
            else
            {
                Error2("Failed to find waypoint %d for adjust\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_CREATE:
        {
            csString sectorName = msg.sector->QueryObject()->GetName();
            Waypoint* wp = pathNetwork->CreateWaypoint(msg.string, msg.position, sectorName, msg.radius, msg.flags);
            if(wp)
            {
                wp->SetID(msg.id);

                Debug3(LOG_NET, 0, "Created waypoint %d at %s.\n",
                       msg.id, toString(msg.position, msg.sector).GetDataSafe());
            }
            else
            {
                Error2("Failed to find waypoint %d for adjust\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_RADIUS:
        {
            Waypoint* wp = pathNetwork->FindWaypoint(msg.id);
            if(wp)
            {
                wp->SetRadius(msg.radius);
                wp->RecalculateEdges(npcclient->GetWorld(), npcclient->GetEngine());

                Debug3(LOG_NET, 0, "Set radius %.2f for waypoint %d.\n",
                       msg.radius, msg.id);
            }
            else
            {
                Error2("Failed to find waypoint %d for radius\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_RENAME:
        {
            Waypoint* waypoint = pathNetwork->FindWaypoint(msg.id);
            if(waypoint)
            {
                waypoint->Rename(msg.string);

                Debug3(LOG_NET, 0, "Set name %s for waypoint %d.\n",
                       msg.string.GetDataSafe(), msg.id);
            }
            else
            {
                Error2("Failed to find waypoint %d for rename\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_ALIAS_REMOVE:
        {
            Waypoint* wp = pathNetwork->FindWaypoint(msg.id);
            if(wp)
            {
                wp->RemoveAlias(msg.string);

                Debug3(LOG_NET, 0, "Removed alias %s from waypoint %d.\n",
                       msg.string.GetDataSafe(), msg.id);
            }
            else
            {
                Error2("Failed to find waypoint %d for remove alias\n", msg.id);
            }

        }
        break;
        case psPathNetworkMessage::WAYPOINT_SET_FLAG:
        {
            Waypoint* wp = pathNetwork->FindWaypoint(msg.id);
            if(wp)
            {
                wp->SetFlag(msg.string, msg.enable);

                Debug4(LOG_NET, 0, "Set flag %s for waypoint %d to %s.\n",
                       msg.string.GetDataSafe(), msg.id, msg.enable?"TRUE":"FALSE");
            }
            else
            {
                Error2("Failed to find waypoint %d for set flag.\n", msg.id);
            }

        }
        break;
    }
}

void NetworkManager::HandleLocation(MsgEntry* me)
{
    psLocationMessage msg(me);

    LocationManager* locations = npcclient->GetLocationManager();

    switch(msg.command)
    {
        case psLocationMessage::LOCATION_ADJUSTED:
        {
            Location* location = locations->FindLocation(msg.id);
            if(location)
            {
                location->Adjust(msg.position, msg.sector);

                Debug4(LOG_NET, 0, "Adjusted location %s(%d) to %s.\n",
                       location->GetName(), msg.id, toString(msg.position, msg.sector).GetDataSafe());
            }
            else
            {
                Error2("Failed to find location %d for adjust\n", msg.id);
            }

        }
        break;
        case psLocationMessage::LOCATION_CREATED:
        {
            Location* location = locations->CreateLocation(msg.typeName, msg.name, msg.position, msg.sector, msg.radius, msg.rotationAngle, msg.flags);
            if(location)
            {
                location->SetID(msg.id);

                Debug3(LOG_NET, 0, "Created location %d at %s.\n",
                       msg.id, toString(msg.position, msg.sector).GetDataSafe());
            }
            else
            {
                Error2("Failed to find location %d for adjust\n", msg.id);
            }

        }
        break;
        case psLocationMessage::LOCATION_INSERTED:
        {
            Location* location = locations->FindLocation(msg.prevID);
            if(!location)
            {
                Error2("Failed to find location %d",msg.prevID);
                return;
            }

            Location* newLocation = location->Insert(msg.id, msg.position, msg.sector);
            if(newLocation)
            {
                Debug3(LOG_NET, 0, "Insert new location %d after location %d\n", msg.id, msg.prevID);
            }
            else
            {
                Error3("Failed to insert new location %d after location %d\n", msg.id, msg.prevID);
            }

        }
        break;
        case psLocationMessage::LOCATION_RADIUS:
        {
            Location* location = locations->FindLocation(msg.id);
            if(location)
            {
                location->SetRadius(msg.radius);

                Debug3(LOG_NET, 0, "Set radius %.2f for location %d.\n",
                       msg.radius, msg.id);
            }
            else
            {
                Error2("Failed to find location %d for radius\n", msg.id);
            }

        }
        break;
        case psLocationMessage::LOCATION_RENAME:
        {
            Location* location = locations->FindLocation(msg.id);
            if(location)
            {
                location->SetName(msg.name);

                Debug3(LOG_NET, 0, "Set name %s for location %d.\n",
                       location->GetName(), msg.id);
            }
            else
            {
                Error2("Failed to find location %d for rename\n", msg.id);
            }

        }
        break;
        case psLocationMessage::LOCATION_SET_FLAG:
        {
            Location* location = locations->FindLocation(msg.id);
            if(location)
            {
                if(!location->SetFlag(msg.flags, msg.enable))
                {
                    Error3("Failed to set flag %s for location %d\n",msg.flags.GetDataSafe(),msg.id);
                    return;
                }

                Debug4(LOG_NET, 0, "Set flag %s for location %d to %s.\n",
                       msg.flags.GetDataSafe(), msg.id, msg.enable?"TRUE":"FALSE");
            }
            else
            {
                Error2("Failed to find location %d for set flag.\n", msg.id);
            }

        }
        break;
        case psLocationMessage::LOCATION_TYPE_ADD:
        {
            LocationType* locationType = locations->CreateLocationType(msg.id,msg.typeName);
            if(!locationType)
            {
                Error1("Failed to create location type");
                return;
            }
            Debug3(LOG_NET, 0, "Created location type %s(%d).\n",
                   locationType->GetName(),locationType->GetID());

        }
        break;
        case psLocationMessage::LOCATION_TYPE_REMOVE:
        {
            if(!locations->RemoveLocationType(msg.typeName))
            {
                return;
            }
            Debug2(LOG_NET, 0, "Removed location type %s.\n",
                   msg.typeName.GetDataSafe());

        }
        break;
        default:
        {
            Error2("Command %d for Location Handling not implemented\n",msg.command);
        }
    }
}

void NetworkManager::HandleHiredNPCScript(MsgEntry* me)
{
    psHiredNPCScriptMessage msg(me);

    LocationManager* locations = npcclient->GetLocationManager();

    if(msg.command == psHiredNPCScriptMessage::CHECK_WORK_LOCATION)
    {
        Location* location = locations->FindLocation(msg.locationType,msg.locationName);
        if(!location)
        {
            psHiredNPCScriptMessage reply(0,
                                          psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT,
                                          msg.hiredEID, false, "Error: NPC Client failed to find work location.");
            reply.SendMessage();
            return;
        }

        csVector3 myPos = location->GetPosition();
        iSector* mySector = location->GetSector(npcclient->GetEngine());

        Waypoint* waypoint = npcclient->FindNearestWaypoint(myPos, mySector, 25.0f);
        if(!waypoint)
        {
            psHiredNPCScriptMessage reply(0,
                                          psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT,
                                          msg.hiredEID, false, "Found no Waypoint near location.");
            reply.SendMessage();
            return;
        }

        gemNPCActor* npcActor =  dynamic_cast<gemNPCActor*>(npcclient->FindEntityID(msg.hiredEID));
        if(!npcActor)
        {
            psHiredNPCScriptMessage reply(0,
                                          psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT,
                                          msg.hiredEID, false, "Error: NPC Client failed to find hired NPC actor.");
            reply.SendMessage();
            return;
        }

        NPC* npc = npcActor->GetNPC();
        if(!npc)
        {
            psHiredNPCScriptMessage reply(0,
                                          psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT,
                                          msg.hiredEID, false, "Error: NPC Client failed to find hired NPC.");
            reply.SendMessage();
            return;
        }

        csVector3 endPos = waypoint->GetPosition();
        iSector* endSector = waypoint->GetSector(npcclient->GetEngine());


        float distance = npcclient->GetWorld()->Distance2(myPos, mySector, endPos, endSector);
        if(distance > 0.5)
        {
            csRef<iCelHPath> path = npcclient->ShortestPath(npc, myPos,mySector,endPos,endSector);
            if(!path || !path->HasNext())
            {
                psHiredNPCScriptMessage reply(0,
                                              psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT,
                                              msg.hiredEID, false, "No path to any nearby waypoints.");
                reply.SendMessage();
                return;
            }
        }
        // All checks passed. There is a path from location to the waypoint network.
        psHiredNPCScriptMessage reply(0,
                                      psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT,
                                      msg.hiredEID, true, "Success.");
        reply.SendMessage();
        return;
    }
}




void NetworkManager::HandleTimeUpdate(MsgEntry* me)
{
    psWeatherMessage msg(me);

    if(msg.type == psWeatherMessage::DAYNIGHT)   // time update msg
    {
        npcclient->UpdateTime(msg.minute, msg.hour, msg.day, msg.month, msg.year);
    }
}


void NetworkManager::RequestAllObjects()
{
    Notify1(LOG_STARTUP, "Requesting all game objects");

    psRequestAllObjects mesg;
    msghandler->SendMessage(mesg.msg);
}


bool NetworkManager::HandleMapList(MsgEntry* msg)
{
    psMapListMessage list(msg);
    CPrintf(CON_CMDOUTPUT,"\n");

    if(list.map.GetSize() == 0)
    {
        CPrintf(CON_ERROR, "NO maps to load\n");
        exit(1);
    }

    for(size_t i=0; i<list.map.GetSize(); i++)
    {
        CPrintf(CON_CMDOUTPUT,"Loading world '%s'\n",list.map[i].GetDataSafe());

        if(!npcclient->LoadMap(list.map[i]))
        {
            CPrintf(CON_ERROR,"Failed to load world '%s'\n",list.map[i].GetDataSafe());
            return false;
        }
    }

    CPrintf(CON_CMDOUTPUT,"Finishing all map loads\n");

    csRef<iBgLoader> loader = csQueryRegistry<iBgLoader>(npcclient->GetObjectReg());
    while(loader->GetLoadingCount() != 0)
    {
        loader->ContinueLoading(true);
    }

    // @@@ RlyDontKnow: Path Networks have to be loaded *after*
    // all required maps are loaded. Else sector Transformations
    // aren't known which will result in broken paths including
    // warp portals.
    if(!npcclient->LoadPathNetwork())
    {
        CPrintf(CON_ERROR, "Couldn't load the path network\n");
        exit(1);
    }

    return true;
}

bool NetworkManager::HandleNPCList(MsgEntry* msg)
{
    uint32_t length;
    PID pid;
    EID eid;

    length = msg->GetUInt32();
    CPrintf(CON_WARNING, "Received list of %i NPCs.\n", length);
    for(unsigned int x=0; x<length; x++)
    {
        pid = PID(msg->GetUInt32());
        eid = EID(msg->GetUInt32());

        // enable the NPC on NPCCLient if it has an associated npcclient ID
        NPC* npc = npcclient->FindNPCByPID(pid);
        if(npc)
            npc->Disable(false);

        // print the received NPC
        CPrintf(CON_WARNING, "Enabling NPC %s %s\n", ShowID(pid),ShowID(eid));
    }

    return true;
}



void NetworkManager::HandlePositionUpdates(MsgEntry* msg)
{
    psAllEntityPosMessage updates(msg);

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());

    for(int x=0; x<updates.count; x++)
    {
        csVector3 pos;
        iSector* sector;
        InstanceID instance;
        bool forced;

        EID id = updates.Get(pos, sector, instance, forced, 0, GetMsgStrings(), engine);
        npcclient->SetEntityPos(id, pos, sector, instance, forced);
    }
}

void NetworkManager::HandlePerceptions(MsgEntry* msg)
{
    psNPCCommandsMessage list(msg);

    char cmd = list.msg->GetInt8();
    while(cmd != psNPCCommandsMessage::CMD_TERMINATOR)
    {
        switch(cmd)
        {
            case psNPCCommandsMessage::PCPT_ASSESS:
            {
                // Extract the data
                EID actorEID = EID(msg->GetUInt32());
                EID targetEID = EID(msg->GetUInt32());
                csString physical = msg->GetStr();
                csString physicalDiff = "assess ";
                physicalDiff += msg->GetStr();
                csString magical = msg->GetStr();
                csString magicalDiff = "assess ";
                magicalDiff += msg->GetStr();
                csString overall = msg->GetStr();
                csString overallDiff = "assess ";
                overallDiff += msg->GetStr();

                NPC* npc = npcclient->FindNPC(actorEID);
                if(!npc)
                {
                    Debug3(LOG_NPC, actorEID.Unbox(), "Got assess perception for unknown NPC(%s) for %s!\n", ShowID(targetEID), ShowID(targetEID));
                    break;
                }

                gemNPCObject* target = npcclient->FindEntityID(targetEID);
                if(!target)
                {
                    NPCDebug(npc, 5, "Got access perception from unknown target(%s)!\n", ShowID(targetEID));
                    break;
                }

                NPCDebug(npc, 5, "Got Assess perception for %s(%s) to %s(%s) with "
                         "Physical assessment: '%s' '%s' "
                         "Magical assessment: '%s' '%s' "
                         "Overall assessment: '%s' '%s'",
                         npc->GetName(), ShowID(actorEID),
                         target->GetName(), ShowID(targetEID),
                         physical.GetDataSafe(),physicalDiff.GetDataSafe(),
                         magical.GetDataSafe(),magicalDiff.GetDataSafe(),
                         overall.GetDataSafe(),overallDiff.GetDataSafe());

                Perception physicalPerception(physicalDiff, physical);
                npc->TriggerEvent(&physicalPerception);

                Perception magicalPerception(magicalDiff, magical);
                npc->TriggerEvent(&magicalPerception);

                Perception overallPerception(overallDiff, overall);
                npc->TriggerEvent(&overallPerception);
                break;
            }
            case psNPCCommandsMessage::PCPT_SPOKEN_TO:
            {
                EID npcEID    = EID(list.msg->GetUInt32());
                bool spokenTo = list.msg->GetBool();

                NPC* npc = npcclient->FindNPC(npcEID);
                if(!npc)
                {
                    Debug2(LOG_NPC, npcEID.Unbox(), "Got spoken_to perception for unknown NPC(%s)!\n", ShowID(npcEID));
                    break;
                }


                Perception perception("spoken_to",spokenTo?"true":"false");
                NPCDebug(npc, 5, "Got spoken_to perception for actor %s(%s) with spoken_to=%s.\n",
                         npc->GetName(), ShowID(npcEID), spokenTo?"true":"false");

                npc->TriggerEvent(&perception);
                break;
            }
            case psNPCCommandsMessage::PCPT_TALK:
            {
                EID speakerEID = EID(list.msg->GetUInt32());
                EID targetEID  = EID(list.msg->GetUInt32());
                int faction    = list.msg->GetInt16();

                NPC* npc = npcclient->FindNPC(targetEID);
                if(!npc)
                {
                    Debug3(LOG_NPC, targetEID.Unbox(), "Got talk perception for unknown NPC(%s) from %s!\n", ShowID(targetEID), ShowID(speakerEID));
                    break;
                }

                gemNPCObject* speaker_ent = npcclient->FindEntityID(speakerEID);
                if(!speaker_ent)
                {
                    NPCDebug(npc, 5, "Got talk perception from unknown speaker(%s)!\n", ShowID(speakerEID));
                    break;
                }

                FactionPerception talk("talk",faction,speaker_ent);
                NPCDebug(npc, 5, "Got Talk perception for from actor %s(%s), faction diff=%d.\n",
                         speaker_ent->GetName(), ShowID(speakerEID), faction);

                npc->TriggerEvent(&talk);
                break;
            }
            case psNPCCommandsMessage::PCPT_ATTACK:
            {
                EID targetEID   = EID(list.msg->GetUInt32());
                EID attackerEID = EID(list.msg->GetUInt32());

                NPC* npc = npcclient->FindNPC(targetEID);
                gemNPCActor* attacker_ent = (gemNPCActor*)npcclient->FindEntityID(attackerEID);

                if(!npc)
                {
                    Debug2(LOG_NPC, targetEID.Unbox(), "Got attack perception for unknown NPC(%s)!\n", ShowID(targetEID));
                    break;
                }
                if(!attacker_ent)
                {
                    NPCDebug(npc, 5, "Got attack perception for unknown attacker (%s)!", ShowID(attackerEID));
                    break;
                }

                AttackPerception attack("attack",attacker_ent);
                NPCDebug(npc, 5, "Got Attack perception for from actor %s(%s).",
                         attacker_ent->GetName(), ShowID(attackerEID));

                npc->TriggerEvent(&attack);
                break;
            }
            case psNPCCommandsMessage::PCPT_DEBUG_NPC:
            {
                EID npc_eid = EID(list.msg->GetUInt32());
                uint32_t clientNum = list.msg->GetUInt32();
                uint8_t debugLevel = list.msg->GetUInt8();

                NPC* npc = npcclient->FindNPC(npc_eid);

                if(!npc)
                {
                    QueueSystemInfoCommand(clientNum,"NPCClient: No NPC found!");
                    break;
                }

                npc->AddDebugClient(clientNum,debugLevel);

                QueueSystemInfoCommand(clientNum,"NPCClient: NPC Debug level set to %d for %s(%s)",debugLevel,npc->GetName(),ShowID(npc->GetEID()));
                break;
            }
            case psNPCCommandsMessage::PCPT_DEBUG_TRIBE:
            {
                EID npc_eid = EID(list.msg->GetUInt32());
                uint32_t clientNum = list.msg->GetUInt32();
                uint8_t debugLevel = list.msg->GetUInt8();

                NPC* npc = npcclient->FindNPC(npc_eid);

                if(!npc)
                {
                    QueueSystemInfoCommand(clientNum,"NPCClient: No NPC found!");
                    break;
                }

                if(!npc->GetTribe())
                {
                    QueueSystemInfoCommand(clientNum,"NPCClient: NPC not part of any tribe!");
                    break;
                }

                npc->GetTribe()->AddDebugClient(clientNum,debugLevel);

                QueueSystemInfoCommand(clientNum,"NPCClient: Tribe Debug level set to %d for %s(%s)",debugLevel,npc->GetName(),ShowID(npc->GetEID()));
                break;
            }
            case psNPCCommandsMessage::PCPT_GROUPATTACK:
            {
                EID targetEID = EID(list.msg->GetUInt32());
                NPC* npc = npcclient->FindNPC(targetEID);

                int groupCount = list.msg->GetUInt8();
                csArray<gemNPCObject*> attacker_ents(groupCount);
                csArray<int> bestSkillSlots(groupCount);
                for(int i=0; i<groupCount; i++)
                {
                    attacker_ents.Push(npcclient->FindEntityID(EID(list.msg->GetUInt32())));
                    bestSkillSlots.Push(list.msg->GetInt8());
                    if(!attacker_ents.Top())
                    {
                        if(npc)
                        {
                            NPCDebug(npc, 5, "Got group attack perception for unknown group member!",
                                     npc->GetActor()->GetName());
                        }

                        attacker_ents.Pop();
                        bestSkillSlots.Pop();
                    }
                }


                if(!npc)
                {
                    Debug2(LOG_NPC, targetEID.Unbox(), "Got group attack perception for unknown NPC(%s)!", ShowID(targetEID));
                    break;
                }


                if(attacker_ents.GetSize() == 0)
                {
                    NPCDebug(npc, 5, "Got group attack perception and all group members are unknown!");
                    break;
                }

                GroupAttackPerception attack("groupattack",attacker_ents,bestSkillSlots);
                NPCDebug(npc, 5, "Got Group Attack perception for recognising %i actors in the group.",
                         attacker_ents.GetSize());

                npc->TriggerEvent(&attack);
                break;
            }

            case psNPCCommandsMessage::PCPT_DMG:
            {
                EID attackerEID = EID(list.msg->GetUInt32());
                EID targetEID   = EID(list.msg->GetUInt32());
                float dmg       = list.msg->GetFloat();
                float hp        = list.msg->GetFloat();
                float maxHP     = list.msg->GetFloat();

                NPC* npc = npcclient->FindNPC(targetEID);
                if(!npc)
                {
                    Debug2(LOG_NPC, targetEID.Unbox(), "Attack on unknown NPC(%s).", ShowID(targetEID));
                    break;
                }

                if(npc->GetActor())
                {
                    npc->GetActor()->SetHP(hp);
                    npc->GetActor()->SetMaxHP(maxHP);
                }


                gemNPCObject* attacker_ent = npcclient->FindEntityID(attackerEID);
                if(!attacker_ent)
                {
                    CPrintf(CON_ERROR, "%s got attack perception for unknown attacker! (%s)\n",
                            npc->GetName(), ShowID(attackerEID));
                    break;
                }

                DamagePerception damage("damage",attacker_ent,dmg);
                NPCDebug(npc, 5, "Got Damage perception for from actor %s(%s) for %1.1f HP to %.1f HP of %.1f HP.",
                         attacker_ent->GetName(), ShowID(attackerEID), dmg, hp, maxHP);

                npc->TriggerEvent(&damage);
                break;
            }
            case psNPCCommandsMessage::PCPT_DEATH:
            {
                EID who = EID(list.msg->GetUInt32());
                NPC* npc = npcclient->FindNPC(who);

                DeathPerception pcpt(who);
                npcclient->TriggerEvent(&pcpt); // Broadcast

                if(npc)
                {
                    NPCDebug(npc, 5, "Got Death message");
                    npcclient->HandleDeath(npc);
                }
                break;
            }
            case psNPCCommandsMessage::PCPT_STAT_DR:
            {
                EID          entityEID       = EID(list.msg->GetUInt32());
                unsigned int statsDirtyFlags = msg->GetUInt16();

                float hp = 0.0, maxHP = 0.0, hpRate = 0.0;
                float mana = 0.0, maxMana = 0.0, manaRate = 0.0;
                float pysStamina = 0.0, maxPysStamina = 0.0, pysStaminaRate = 0.0;
                float menStamina = 0.0, maxMenStamina = 0.0, menStaminaRate = 0.0;

                if(statsDirtyFlags & DIRTY_VITAL_HP)
                {
                    hp = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_HP_MAX)
                {
                    maxHP = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_HP_RATE)
                {
                    hpRate = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_MANA)
                {
                    mana = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_MANA_MAX)
                {
                    maxMana = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_MANA_RATE)
                {
                    manaRate = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA)
                {
                    pysStamina = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA_MAX)
                {
                    maxPysStamina = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA_RATE)
                {
                    pysStaminaRate = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA)
                {
                    menStamina = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA_MAX)
                {
                    maxMenStamina = list.msg->GetFloat();
                }
                if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA_RATE)
                {
                    menStaminaRate = list.msg->GetFloat();
                }

                gemNPCActor* entity = dynamic_cast<gemNPCActor*>(npcclient->FindEntityID(entityEID));
                if(!entity)
                {
                    break;
                }

                NPC* npc = npcclient->FindNPC(entityEID);
                if(npc)
                {
                    NPCDebug(npc, 5, "Got StatDR Perception: HP: %.2f/.2f %.2f Mana: %.2f/%.2f %.2f PysStamina: %.2f/%.2f %.2f MenStamina: %.2f/%.2f %.2f",
                             hp,maxHP,hpRate,mana,maxMana,manaRate,pysStamina,maxPysStamina,pysStaminaRate,menStamina,maxMenStamina,menStaminaRate);
                }

                if(statsDirtyFlags & DIRTY_VITAL_HP)
                {
                    entity->SetHP(hp);
                }
                if(statsDirtyFlags & DIRTY_VITAL_HP_MAX)
                {
                    entity->SetMaxHP(maxHP);
                }
                if(statsDirtyFlags & DIRTY_VITAL_HP_RATE)
                {
                    entity->SetHPRate(hpRate);
                }
                if(statsDirtyFlags & DIRTY_VITAL_MANA)
                {
                    entity->SetMana(mana);
                }
                if(statsDirtyFlags & DIRTY_VITAL_MANA_MAX)
                {
                    entity->SetMaxMana(maxMana);
                }
                if(statsDirtyFlags & DIRTY_VITAL_MANA_RATE)
                {
                    entity->SetManaRate(manaRate);
                }
                if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA)
                {
                    entity->SetPysStamina(pysStamina);
                }
                if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA_MAX)
                {
                    entity->SetMaxPysStamina(maxPysStamina);
                }
                if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA_RATE)
                {
                    entity->SetPysStaminaRate(pysStaminaRate);
                }
                if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA)
                {
                    entity->SetMenStamina(menStamina);
                }
                if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA_MAX)
                {
                    entity->SetMaxMenStamina(maxMenStamina);
                }
                if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA_RATE)
                {
                    entity->SetMenStaminaRate(menStaminaRate);
                }
                break;
            }
            case psNPCCommandsMessage::PCPT_SPELL:
            {
                EID caster = EID(list.msg->GetUInt32());
                EID target = EID(list.msg->GetUInt32());
                uint32_t strhash = list.msg->GetUInt32();
                float    severity = list.msg->GetInt8() / 10;
                csString type = GetCommonString(strhash);

                gemNPCObject* caster_ent = npcclient->FindEntityID(caster);
                NPC* npc = npcclient->FindNPC(target);
                gemNPCObject* target_ent = (npc) ? npc->GetActor() : npcclient->FindEntityID(target);

                if(npc)
                {
                    NPCDebug(npc, 5, "Got Spell Perception %s is casting %s on me.",
                             (caster_ent)?caster_ent->GetName():"(unknown entity)",type.GetDataSafe());
                }

                if(!caster_ent || !target_ent)
                    break;

                iSector* sector;
                csVector3 pos;
                psGameObject::GetPosition(target_ent, pos, sector);

                if(npc)
                {
                    SpellPerception pcpt_self("spell:self",caster_ent,target_ent,type,severity);
                    npc->TriggerEvent(&pcpt_self, -1, NULL, NULL, true);
                }

                SpellPerception pcpt_target("spell:target",caster_ent,target_ent,type,severity);
                npcclient->TriggerEvent(&pcpt_target, 30, &pos, sector, true); // Broadcast to same sector

                SpellPerception pcpt_unknown("spell:unknown",caster_ent,target_ent,type,severity);
                npcclient->TriggerEvent(&pcpt_unknown, 30, &pos, sector, true); // Broadcast to same sector

                break;
            }
            case psNPCCommandsMessage::PCPT_ANYRANGEPLAYER:
            case psNPCCommandsMessage::PCPT_LONGRANGEPLAYER:
            case psNPCCommandsMessage::PCPT_SHORTRANGEPLAYER:
            case psNPCCommandsMessage::PCPT_VERYSHORTRANGEPLAYER:
            {
                EID npcEID    = EID(list.msg->GetUInt32());
                EID playerEID = EID(list.msg->GetUInt32());
                float faction = list.msg->GetFloat();

                NPC* npc = npcclient->FindNPC(npcEID);
                if(!npc)
                    break;  // This perception is not our problem

                NPCDebug(npc, 5, "Range perception: NPC: %s, player: %s, faction: %.0f",
                         ShowID(npcEID), ShowID(playerEID), faction);

                gemNPCObject* npc_ent = npc->GetActor();
                gemNPCObject* player = npcclient->FindEntityID(playerEID);

                if(!player || !npc_ent)
                    break;


                csString pcpt_name;
                if(npc->GetOwner() == player)
                {
                    pcpt_name.Append("owner ");
                }
                else
                {
                    pcpt_name.Append("player ");
                }
                if(cmd == psNPCCommandsMessage::PCPT_ANYRANGEPLAYER)
                    pcpt_name.Append("anyrange");
                if(cmd == psNPCCommandsMessage::PCPT_LONGRANGEPLAYER)
                    pcpt_name.Append("sensed");
                if(cmd == psNPCCommandsMessage::PCPT_SHORTRANGEPLAYER)
                    pcpt_name.Append("nearby");
                if(cmd == psNPCCommandsMessage::PCPT_VERYSHORTRANGEPLAYER)  // PERSONAL_RANGE
                    pcpt_name.Append("adjacent");

                NPCDebug(npc, 5, "Got Player %s in Range of %s with the %s Perception, with faction %d",
                         player->GetName(), npc_ent->GetName(), pcpt_name.GetData(), int(faction));

                FactionPerception pcpt(pcpt_name, int (faction), player);

                npc->TriggerEvent(&pcpt);
                break;
            }
            case psNPCCommandsMessage::PCPT_OWNER_CMD:
            {
                psPETCommandMessage::PetCommand_t command = (psPETCommandMessage::PetCommand_t)list.msg->GetUInt32();
                EID owner_id  = EID(list.msg->GetUInt32());
                EID pet_id    = EID(list.msg->GetUInt32());
                EID target_id = EID(list.msg->GetUInt32());

                gemNPCObject* owner = npcclient->FindEntityID(owner_id);
                NPC* npc = npcclient->FindNPC(pet_id);

                gemNPCObject* pet = (npc) ? npc->GetActor() : npcclient->FindEntityID(pet_id);

                gemNPCObject* target = npcclient->FindEntityID(target_id);

                if(npc)
                {
                    NPCDebug(npc, 5, "Got OwnerCmd %d Perception from %s for %s with target %s",
                             command,(owner)?owner->GetName():"(unknown entity)",
                             (pet)?pet->GetName():"(unknown entity)",
                             (target)?target->GetName():"(none)");
                }

                if(!npc | !owner || !pet)
                    break;

                OwnerCmdPerception pcpt("OwnerCmdPerception", command, owner, pet, target);

                npc->TriggerEvent(&pcpt);
                break;
            }
            case psNPCCommandsMessage::PCPT_OWNER_ACTION:
            {
                int action = list.msg->GetInt32();
                EID owner_id = EID(list.msg->GetUInt32());
                EID pet_id = EID(list.msg->GetUInt32());

                gemNPCObject* owner = npcclient->FindEntityID(owner_id);
                NPC* npc = npcclient->FindNPC(pet_id);
                gemNPCObject* pet = (npc) ? npc->GetActor() : npcclient->FindEntityID(pet_id);

                if(npc)
                {
                    NPCDebug(npc, 5, "Got OwnerAction %d Perception from %s for %s",
                             action,(owner)?owner->GetName():"(unknown entity)",
                             (pet)?pet->GetName():"(unknown entity)");
                }

                if(!npc || !owner || !pet)
                    break;

                OwnerActionPerception pcpt("OwnerActionPerception", action, owner, pet);

                npc->TriggerEvent(&pcpt);
                break;
            }
            case psNPCCommandsMessage::PCPT_PERCEPT:
            {
                EID npcEID          = EID(list.msg->GetUInt32());
                csString perception = list.msg->GetStr();
                csString type       = list.msg->GetStr();

                NPC* npc = npcclient->FindNPC(npcEID);
                if(npc)
                {
                    NPCDebug(npc, 5, "%s got Perception: %s Type: %s ", ShowID(npcEID), perception.GetDataSafe(), type.GetDataSafe());


                    Perception pcpt(perception, type);
                    npc->TriggerEvent(&pcpt);
                }
                break;
            }
            case psNPCCommandsMessage::PCPT_INVENTORY:
            {
                EID owner_id = EID(msg->GetUInt32());
                csString item_name = msg->GetStr();
                bool inserted = msg->GetBool();
                int count = msg->GetInt16();

                gemNPCObject* owner = npcclient->FindEntityID(owner_id);
                NPC* npc = npcclient->FindNPC(owner_id);

                if(!owner || !npc)
                    break;

                NPCDebug(npc, 5, "Got Inventory %s Perception from %s for %d %s\n",
                         (inserted?"Add":"Remove"),owner->GetName(),
                         count,item_name.GetData());

                iSector* sector;
                csVector3 pos;
                psGameObject::GetPosition(owner, pos, sector);

                csString str;
                str.Format("inventory:%s",(inserted?"added":"removed"));

                InventoryPerception pcpt(str, item_name, count, pos, sector, 5.0);
                npc->TriggerEvent(&pcpt);

                break;
            }

            case psNPCCommandsMessage::PCPT_FAILED_TO_ATTACK:
            {
                EID npcEID = EID(msg->GetUInt32());
                EID targetEID = EID(msg->GetUInt32());

                NPC* npc = npcclient->FindNPC(npcEID);
                gemNPCObject* target = npcclient->FindEntityID(targetEID);

                if(!npc || !target)
                    break;

                NPCDebug(npc, 5, "Got Failed to Attack perception");

                Perception failedToAttack("failed to attack");
                npc->TriggerEvent(&failedToAttack);

                break;
            }

            case psNPCCommandsMessage::PCPT_FLAG:
            {
                EID owner_id = EID(msg->GetUInt32());
                uint32_t flags = msg->GetUInt32();

                gemNPCObject* obj = npcclient->FindEntityID(owner_id);

                if(!obj)
                    break;

                obj->SetInvisible((flags & psNPCCommandsMessage::INVISIBLE) ? true : false);
                obj->SetInvincible((flags & psNPCCommandsMessage::INVINCIBLE) ? true : false);
                obj->SetAlive((flags & psNPCCommandsMessage::IS_ALIVE));
                NPC* npc = obj->GetNPC();
                if(npc)
                {
                    npc->SetAlive(obj->IsAlive());
                }

                break;
            }

            case psNPCCommandsMessage::PCPT_NPCCMD:
            {
                EID owner_id = EID(msg->GetUInt32());
                csString cmd   = msg->GetStr();

                NPC* npc = npcclient->FindNPC(owner_id);

                if(!npc)
                    break;

                NPCCmdPerception pcpt(cmd, npc);

                npcclient->TriggerEvent(&pcpt); // Broadcast

                break;
            }

            case psNPCCommandsMessage::PCPT_TRANSFER:
            {
                EID entity_id = EID(msg->GetUInt32());
                csString item = msg->GetStr();
                int count = msg->GetInt8();
                csString target = msg->GetStr();

                NPC* npc = npcclient->FindNPC(entity_id);

                if(!npc)
                    break;

                NPCDebug(npc, 5, "Got Transfer Perception from %s for %d %s to %s\n",
                         npc->GetName(),count,
                         item.GetDataSafe(),target.GetDataSafe());

                iSector* sector;
                csVector3 pos;
                psGameObject::GetPosition(npc->GetActor(), pos, sector);

                InventoryPerception pcpt("transfer", item, count, pos, sector, 5.0);

                if(target == "tribe" && npc->GetTribe())
                {
                    npc->GetTribe()->HandlePerception(npc,&pcpt);
                }
                break;
            }
            case psNPCCommandsMessage::PCPT_SPAWNED:
            {
                PID spawned_pid = PID(msg->GetUInt32());
                EID spawned_eid = EID(msg->GetUInt32());
                EID spawner_eid = EID(msg->GetUInt32());
                csString tribeMemberType = msg->GetStr();


                NPC* npc = npcclient->FindNPC(spawner_eid);

                if(!npc)
                    break;

                NPCDebug(npc, 5, "Got spawn Perception to %u from %u\n",
                         spawned_eid.Unbox(), spawner_eid.Unbox());
                npc->GetTribe()->AddMember(spawned_pid, tribeMemberType);
                NPC* spawned_npc = npcclient->FindNPC(spawned_eid);
                if(spawned_npc)
                {
                    npcclient->CheckAttachTribes(spawned_npc);
                }
                break;
            }
            case psNPCCommandsMessage::PCPT_TELEPORT:
            {
                EID npc_eid = EID(msg->GetUInt32());
                csVector3 pos = msg->GetVector3();
                float yrot = msg->GetFloat();
                iSector* sector = msg->GetSector(0, GetMsgStrings(), engine);
                InstanceID instance = msg->GetUInt32();

                NPC* npc = npcclient->FindNPC(npc_eid);

                if(!npc)
                    break;

                NPCDebug(npc, 5, "Got teleport perception to %s\n",toString(pos,sector).GetDataSafe());

                PositionPerception pcpt("teleported",NULL,instance,sector,pos,yrot,0.0);
                npc->TriggerEvent(&pcpt);
                break;
            }

            case psNPCCommandsMessage::PCPT_INFO_REQUEST:
            {
                EID npc_eid = EID(msg->GetUInt32());
                uint32_t clientNum = msg->GetUInt32();
                csString infoRequestSubCmd = msg->GetStr();

                NPC* npc = npcclient->FindNPC(npc_eid);

                if(!npc)
                    break;

                NPCDebug(npc, 5, "Got info request.");

                // Send reply back in 3 parts. Since they only can hold a limited chars per message.

                csString reply("NPCClient: ");
                reply.Append(npc->Info(infoRequestSubCmd));
                QueueSystemInfoCommand(clientNum,reply);

                QueueSystemInfoCommand(clientNum," Behaviors: %s",npc->GetBrain()->InfoBehaviors(npc).GetDataSafe());

                QueueSystemInfoCommand(clientNum," Reactions: %s",npc->GetBrain()->InfoReactions(npc).GetDataSafe());

                break;

            }

            case psNPCCommandsMessage::PCPT_CHANGE_OWNER:
            {
                EID npc_eid = EID(msg->GetUInt32());
                EID owner_eid = EID(msg->GetUInt32());

                NPC* npc = npcclient->FindNPC(npc_eid);
                if(!npc)
                    break;

                NPCDebug(npc, 5, "Got change owner request (%u).", owner_eid.Unbox());
                npc->SetOwner(owner_eid);
                break;
            }

            case psNPCCommandsMessage::PCPT_CHANGE_BRAIN:
            {
                EID npc_eid = EID(msg->GetUInt32());
                uint32_t clientNum = msg->GetUInt32();
                csString brainType = msg->GetStr();

                NPC* npc = npcclient->FindNPC(npc_eid);

                if(!npc)
                {
                    QueueSystemInfoCommand(clientNum,"NPC not found");
                    break;
                }

                NPCDebug(npc, 5, "Got change brain request to %s from client %u.", brainType.GetDataSafe(), clientNum);

                bool reload = (brainType == "reload");
                if(reload)
                {
                    brainType = npc->GetOrigBrainType();
                }

                //search for the requested npc type
                NPCType* type = npcclient->FindNPCType(brainType.GetDataSafe());
                if(!type)
                {
                    Error2("NPC type '%s' is not found",(const char*)brainType);
                    QueueSystemInfoCommand(clientNum,"Brain not found.");
                    break;
                }

                //if found set it
                npc->SetBrain(type);
                QueueSystemInfoCommand(clientNum,"Brain %s %s.",brainType.GetDataSafe(),reload?"reloaded":"changed");
                break;
            }

            default:
            {
                CPrintf(CON_ERROR,"************************\nUnknown npc cmd: %d\n*************************\n",cmd);
                abort();
            }
        }
        cmd = list.msg->GetInt8();
    }
}

void NetworkManager::HandleDisconnect(MsgEntry* msg)
{
    psDisconnectMessage disconnect(msg);

    // Special case to drop login failure reply from server immediately after we have already logged in.
    if(connected && disconnect.msgReason.CompareNoCase("Already Logged In."))
        return;

    CPrintf(CON_WARNING, "Disconnected: %s\n",disconnect.msgReason.GetData());

    // Reconnect is disabled right now because the CEL entity registry is not flushed upon
    // reconnect which will cause BIG problems.
    Disconnect();
    CPrintf(CON_WARNING, "Reconnect disabled...\n");
#if 1
    npcclient->GetEventManager()->Stop();
#else
    if(!reconnect && false)
    {
        connected = ready = false;

        // reconnect
        connection->DisConnect();
        npcclient->RemoveAll();

        reconnect = true;
        // 60 secs to allow any connections to go linkdead.
        psNPCReconnect* recon = new psNPCReconnect(60000, this, false);
        npcclient->GetEventManager()->Push(recon);
    }
#endif
}

void NetworkManager::HandleNPCWorkDone(MsgEntry* me)
{
    psNPCWorkDoneMessage msg(me);

    NPC* npc = npcclient->FindNPC(msg.npcEID);

    if(!npc)
    {
        Error2("Could not find npc(%s) for Got work done.", ShowID(msg.npcEID));
        return;
    }

    // If he just prospected a mine and its a tribe member
    if(npc->GetTribe())
    {
        npc->GetTribe()->ProspectMine(npc,msg.resource,msg.nick);
    }
}

void NetworkManager::HandleNewNpc(MsgEntry* me)
{
    psNewNPCCreatedMessage msg(me);

    NPC* npc = npcclient->FindNPCByPID(msg.master_id);
    if(npc)
    {
        NPCDebug(npc, 5, "Got new NPC notification for %s", ShowID(msg.new_npc_id));

        // Insert a row in the db for this guy next.
        // We will get an entity msg in a second to make him come alive.
        npc->InsertCopy(msg.new_npc_id,msg.owner_id);

        // Now requery so we have the new guy on our list when we get the entity msg.
        if(!npcclient->ReadSingleNPC(msg.new_npc_id))
        {
            Error3("Error creating copy of master %s as id %s.", ShowID(msg.master_id), ShowID(msg.new_npc_id));
            return;
        }
    }
    else
    {
        // Ignore it here.  We don't manage the master.
    }
}

void NetworkManager::HandleNpcDeleted(MsgEntry* me)
{
    psNPCDeletedMessage msg(me);

    NPC* npc = npcclient->FindNPCByPID(msg.npc_id);
    if(npc)
    {
        // We are managing this NPC. Delete the NPC.

        // Delete from NPC Client DB.
        npc->Delete();

        // Remove the NPC from the client.
        npcclient->Remove(npc);
        delete npc;
    }
}


void NetworkManager::PrepareCommandMessage()
{
    outbound = new psNPCCommandsMessage(0,30000);
    cmd_count = 0;
}

void NetworkManager::QueueDRData(NPC* npc)
{
    // When a NPC is dead, this may still be called by Behavior::Interrupt
    /*    if(!npc->IsAlive())
        return;
    cmd_dr_outbound.PutUnique(npc->GetPID(), npc);
    */
}

void NetworkManager::QueueDRData2(NPC* npc)
{
    // When a NPC is dead, this may still be called by Behavior::Interrupt
    if(!npc->IsAlive())
        return;
    cmd_dr_outbound.PutUnique(npc->GetPID(), npc);
}

void NetworkManager::DequeueDRData(NPC* npc)
{
    NPCDebug(npc, 15, "Dequeuing DR Data...");
    cmd_dr_outbound.DeleteAll(npc->GetPID());
}

void NetworkManager::CheckCommandsOverrun(size_t neededSize)
{
    if(outbound->msg->current + neededSize > outbound->msg->bytes->GetSize())
        SendAllCommands();
}

void NetworkManager::QueueDRDataCommand(NPC* npc)
{
    gemNPCActor* entity = npc->GetActor();
    if(!entity) return; //TODO: This shouldn't happen but it happens so this is just a quick patch and
    //      the real problem should be fixed (entities being removed from the game
    //      world while moving will end up here with entity being null)
    //      DequeueDRData should be supposed to remove all queued data but the iterator
    //      in SendAllCommands still "catches them"

    CheckCommandsOverrun(100);

    psLinearMovement* linmove = npc->GetLinMove();
    bool onGround;
    csVector3 myPos,vel,worldVel;
    float yrot,angVel;
    iSector* mySector;
    uint8_t mode = 0;
    linmove->GetDRData(onGround,myPos,yrot,mySector,vel,worldVel,angVel);

    uint8_t counter = npc->GetDRCounter(csGetTicks(),myPos,yrot,mySector,vel,angVel);

    psDRMessage drmsg(0,entity->GetEID(),onGround,mode,counter,myPos,yrot,mySector,csString(),
                      vel,worldVel,angVel,connection->GetAccessPointers());

    if(DoLogDebug(LOG_DRDATA))
    {
        Debug(LOG_DRDATA, entity->GetEID().Unbox(), "%s, %s, %s, %s, %f, %f, %f, %f, %s, %f, %f, %f, %f, %f, %f, %f",
              "NPCCLIENT", "SET", ShowID(entity->GetEID()),onGround?"TRUE":"FALSE", myPos.x, myPos.y, myPos.z, yrot,
              mySector->QueryObject()->GetName(), vel.x, vel.y, vel.z, worldVel.x, worldVel.y, worldVel.z, angVel);
    }

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_DRDATA);
    outbound->msg->Add(drmsg.msg->bytes->payload,(uint32_t)drmsg.msg->bytes->GetTotalSize());

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueDRData put message in overrun state!\n");
    }
    cmd_count++;
}

void NetworkManager::QueueAttackCommand(gemNPCActor* attacker, gemNPCActor* target, const char* stance)
{

    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_ATTACK);
    outbound->msg->Add(attacker->GetEID().Unbox());

    if(target)
    {
        outbound->msg->Add(target->GetEID().Unbox());
    }
    else
    {
        outbound->msg->Add((uint32_t) 0);    // 0 target means stop attack
    }

    outbound->msg->Add(GetCommonStringID(stance));

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueAttackCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueScriptCommand(gemNPCActor* npc, gemNPCObject* target, const csString &scriptName)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_SCRIPT);
    outbound->msg->Add(npc->GetEID().Unbox());
    if(target)
    {
        outbound->msg->Add(target->GetEID().Unbox());
    }
    else
    {
        outbound->msg->Add((uint32_t)0);
    }
    outbound->msg->Add(scriptName);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueScriptCommand put message in overrun state!\n");
    }
    cmd_count++;
}

void NetworkManager::QueueSitCommand(gemNPCActor* npc, gemNPCObject* target, bool sit)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_SIT);
    outbound->msg->Add(npc->GetEID().Unbox());
    if(target)
    {
        outbound->msg->Add(target->GetEID().Unbox());
    }
    else
    {
        outbound->msg->Add((uint32_t)0);
    }
    outbound->msg->Add(sit);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueSitCommand put message in overrun state!\n");
    }
    cmd_count++;
}

void NetworkManager::QueueSpawnCommand(gemNPCActor* mother, gemNPCActor* father, const csString &tribeMemberType)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_SPAWN);
    outbound->msg->Add(mother->GetEID().Unbox());
    outbound->msg->Add(father->GetEID().Unbox());
    outbound->msg->Add(tribeMemberType);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueSpawnCommand put message in overrun state!\n");
    }
    cmd_count++;
}

void NetworkManager::QueueSpawnBuildingCommand(gemNPCActor* spawner, csVector3 where, iSector* sector, const char* buildingName, int tribeID, bool pickupable)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_SPAWN_BUILDING);
    outbound->msg->Add(spawner->GetEID().Unbox());
    outbound->msg->Add(where);
    outbound->msg->Add(sector->QueryObject()->GetName());
    outbound->msg->Add(buildingName);
    outbound->msg->Add(pickupable);

    outbound->msg->Add(tribeID);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueSpawnBuildingCommand put message in overrun state!\n");
    }
    cmd_count++;
}

void NetworkManager::QueueUnbuildCommand(gemNPCActor* unbuilder, gemNPCItem* building)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_UNBUILD);
    outbound->msg->Add(unbuilder->GetEID().Unbox());
    outbound->msg->Add(building->GetEID().Unbox());

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueUnbuildingCommand put message in overrun state!\n");
    }
    cmd_count++;
}

void NetworkManager::QueueTalkCommand(gemNPCActor* speaker, gemNPCActor* target, psNPCCommandsMessage::PerceptionTalkType talkType, bool publicTalk, const char* text)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_TALK);
    outbound->msg->Add(speaker->GetEID().Unbox());
    if(target)
    {
        outbound->msg->Add(target->GetEID().Unbox());
    }
    else
    {
        outbound->msg->Add((uint32_t)0);
    }
    outbound->msg->Add((uint32_t) talkType);
    outbound->msg->Add(publicTalk);
    outbound->msg->Add(text);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueTalkCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueVisibilityCommand(gemNPCActor* entity, bool status)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_VISIBILITY);
    outbound->msg->Add(entity->GetEID().Unbox());

    outbound->msg->Add(status);
    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueVisibleCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueuePickupCommand(gemNPCActor* entity, gemNPCObject* item, int count)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_PICKUP);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(item->GetEID().Unbox());
    outbound->msg->Add((int16_t) count);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueuePickupCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueEmoteCommand(gemNPCActor* npc, gemNPCObject* target,  const csString &cmd)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_EMOTE);
    outbound->msg->Add(npc->GetEID().Unbox());
    if(target)
    {
        outbound->msg->Add(target->GetEID().Unbox());
    }
    else
    {
        outbound->msg->Add((uint32_t)0);
    }
    outbound->msg->Add(cmd);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueEmoteCommand put message in overrun state!\n");
    }

    cmd_count++;
}


void NetworkManager::QueueEquipCommand(gemNPCActor* entity, csString item, csString slot, int count)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_EQUIP);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(item);
    outbound->msg->Add(slot);
    outbound->msg->Add((int16_t) count);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueEquipCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueDequipCommand(gemNPCActor* entity, csString slot)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_DEQUIP);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(slot);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueDequipCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueWorkCommand(gemNPCActor* entity, const csString &type, const csString &resource)
{
    CheckCommandsOverrun(sizeof(uint8_t) + sizeof(uint32_t) + (type.Length()+1) + (resource.Length()+1));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_WORK);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(type);
    outbound->msg->Add(resource);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueWorkCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueTransferCommand(gemNPCActor* entity, csString item, int count, csString target)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_TRANSFER);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(item);
    outbound->msg->Add((int8_t)count);
    outbound->msg->Add(target);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueTransferCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueDeleteNPCCommand(NPC* npc)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_DELETE_NPC);
    outbound->msg->Add(npc->GetPID().Unbox());

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueDeleteNPCCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueDropCommand(gemNPCActor* entity, csString slot)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_DROP);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(slot);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueDropCommand put message in overrun state!\n");
    }

    cmd_count++;
}


void NetworkManager::QueueResurrectCommand(csVector3 where, float rot, iSector* sector, PID character_id)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_RESURRECT);
    outbound->msg->Add(character_id.Unbox());
    outbound->msg->Add((float)rot);
    outbound->msg->Add(where);
    outbound->msg->Add(sector, 0, GetMsgStrings());

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueResurrectCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueSequenceCommand(csString name, int cmd, int count)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_SEQUENCE);
    outbound->msg->Add(name);
    outbound->msg->Add((int8_t) cmd);
    outbound->msg->Add((int32_t) count);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueSequenceCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueTemporarilyImperviousCommand(gemNPCActor* entity, bool impervious)
{
    CheckCommandsOverrun(100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_TEMPORARILY_IMPERVIOUS);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add((bool) impervious);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueTemporarilyImperviousCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueSystemInfoCommand(uint32_t clientNum, const char* reply, ...)
{
    // Format the reply
    va_list args;
    va_start(args, reply);
    csString str;
    str.FormatV(reply, args);
    va_end(args);

    // Queue the System Info

    CheckCommandsOverrun(sizeof(int8_t)+sizeof(uint32_t)+(str.Length()+1));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_INFO_REPLY);
    outbound->msg->Add(clientNum);
    outbound->msg->Add(str.GetDataSafe());

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueSystemInfoCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueAssessCommand(gemNPCActor* entity, gemNPCObject* target, const csString &physicalAssessmentPerception,
                                        const csString &magicalAssessmentPerception,  const csString &overallAssessmentPerception)
{
    CheckCommandsOverrun(sizeof(int8_t)+2*sizeof(uint32_t)+(physicalAssessmentPerception.Length()+1)+
                         (magicalAssessmentPerception.Length()+1)+(overallAssessmentPerception.Length()+1));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_ASSESS);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add(physicalAssessmentPerception);
    outbound->msg->Add(magicalAssessmentPerception);
    outbound->msg->Add(overallAssessmentPerception);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueAssessCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueCastCommand(gemNPCActor* entity, gemNPCObject* target, const csString &spell, float kFactor)
{
    CheckCommandsOverrun(sizeof(int8_t)+2*sizeof(uint32_t)+(spell.Length()+1)+sizeof(float));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_CAST);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add(spell);
    outbound->msg->Add(kFactor);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueCastCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueBusyCommand(gemNPCActor* entity, bool busy)
{
    CheckCommandsOverrun(sizeof(int8_t)+sizeof(uint32_t)+sizeof(bool));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_BUSY);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(busy);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueBusyCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueControlCommand(gemNPCActor* controllingEntity, gemNPCActor* controlledEntity)
{
    CheckCommandsOverrun(sizeof(int8_t)+sizeof(uint32_t)+100);

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_CONTROL);
    outbound->msg->Add(controllingEntity->GetEID().Unbox());

    psDRMessage drmsg(0,controlledEntity->GetEID(),0,connection->GetAccessPointers(),controlledEntity->pcmove);
    outbound->msg->Add(drmsg.msg->bytes->payload,(uint32_t)drmsg.msg->bytes->GetTotalSize());

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueControlCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::QueueLootCommand(gemNPCActor* entity, EID targetEID, const csString &type)
{
    CheckCommandsOverrun(sizeof(uint8_t) + sizeof(uint32_t) + (type.Length()+1));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_LOOT);
    outbound->msg->Add(entity->GetEID().Unbox());
    outbound->msg->Add(targetEID.Unbox());
    outbound->msg->Add(type);

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NetworkManager::QueueLootCommand put message in overrun state!\n");
    }

    cmd_count++;
}

void NetworkManager::SendAllCommands(bool final)
{
    // If this is the final send all for a tick we need to check if any NPCs has been queued for sending of DR data.
    if(final)
    {
        csHash<NPC*,PID>::GlobalIterator it(cmd_dr_outbound.GetIterator());
        while(it.HasNext())
        {
            NPC* npc = it.Next();
            if(npc->GetActor())
            {
                if(npc->GetLinMove()->GetSector())
                {
                    QueueDRDataCommand(npc);
                }
                else
                {
                    CPrintf(CON_WARNING,"NPC %s is trying to send DR data with no sector\n",npc->GetName());
                    npc->Disable(); // Disable the NPC.
                }

            }
            else
            {
                //TODO: probably here the npc should be destroyed?
                CPrintf(CON_WARNING,"NPC %s is trying to send DR data with no actor\n",npc->GetName());
                npc->Disable(); // Disable the NPC.
            }
        }
        cmd_dr_outbound.DeleteAll();
    }

    if(cmd_count)
    {
        outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_TERMINATOR);
        outbound->msg->ClipToCurrentSize();

        msghandler->SendMessage(outbound->msg);

        delete outbound;
        PrepareCommandMessage();
    }
}


void NetworkManager::ReAuthenticate()
{
    Authenticate(host,port,user,password);
}

void NetworkManager::ReConnect()
{
    if(!connection->Connect(host,port))
    {
        CPrintf(CON_ERROR, "Couldn't resolve hostname %s on port %d.\n",(const char*)host,port);
        return;
    }
    // 2 seconds to allow linkdead messages to be processed
    psNPCReconnect* recon = new psNPCReconnect(2000, this, true);
    npcclient->GetEventManager()->Push(recon);
}

void NetworkManager::SendConsoleCommand(const char* cmd)
{
    psServerCommandMessage msg(0,cmd);
    msg.SendMessage();
}

/*------------------------------------------------------------------*/

psNPCReconnect::psNPCReconnect(int offsetticks, NetworkManager* mgr, bool authent)
    : psGameEvent(0,offsetticks,"psNPCReconnect"), networkMgr(mgr), authent(authent)
{

}

void psNPCReconnect::Trigger()
{
    if(!running)
        return;
    if(!authent)
    {
        networkMgr->ReConnect();
        return;
    }
    if(!networkMgr->IsReady())
    {
        CPrintf(CON_WARNING, "Reconnecting...\n");
        networkMgr->ReAuthenticate();
    }
    networkMgr->reconnect = false;
}
