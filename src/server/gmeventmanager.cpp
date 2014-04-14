/* gmeventmanager.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Manages Game Master events for players.
 */

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/eventmanager.h"
#include "util/strutil.h"

#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "gmeventmanager.h"
#include "clients.h"
#include "gem.h"
#include "cachemanager.h"
#include "entitymanager.h"
#include "adminmanager.h"
#include "globals.h"

GMEventManager::GMEventManager()
{
    nextEventID = 0;

    // initialise gmEvents
    gmEvents.DeleteAll();

    Subscribe(&GMEventManager::HandleGMEventCommand, MSGTYPE_GMEVENT_INFO, REQUIRE_READY_CLIENT);
}

GMEventManager::~GMEventManager()
{
    for(size_t e = 0; e < gmEvents.GetSize(); e++)
    {
        gmEvents[e]->Player.DeleteAll();
        delete gmEvents[e];
    }
    gmEvents.DeleteAll();
}

bool GMEventManager::Initialise(void)
{
    GMEvent* ongoingGMEvent;

    // load any existing gm events from database
    Result events(db->Select("SELECT * from gm_events order by id"));
    if(events.IsValid())
    {
        for(unsigned long e=0; e<events.Count(); e++)
        {
            ongoingGMEvent = new GMEvent;
            ongoingGMEvent->id = events[e].GetInt("id");
            ongoingGMEvent->status = static_cast<GMEventStatus>(events[e].GetInt("status"));
            ongoingGMEvent->gmID = events[e].GetInt("gm_id");
            ongoingGMEvent->EndTime = 0;
            ongoingGMEvent->eventName = csString(events[e]["name"]);
            ongoingGMEvent->eventDescription = csString(events[e]["description"]);
            gmEvents.Push(ongoingGMEvent);

            // setup next available id
            if(ongoingGMEvent->id >= nextEventID)
                nextEventID = ongoingGMEvent->id + 1;
        }

        // load registered players from database
        Result registeredPlayers(db->Select("SELECT * from character_events order by event_id"));
        if(registeredPlayers.IsValid())
        {
            PlayerData eventPlayer;
            int eventID;
            for(unsigned long rp=0; rp<registeredPlayers.Count(); rp++)
            {
                eventID = registeredPlayers[rp].GetInt("event_id");
                eventPlayer.PlayerID = registeredPlayers[rp].GetInt("player_id");
                eventPlayer.CanEvaluate = (registeredPlayers[rp]["vote"] == NULL);
                if((ongoingGMEvent = GetGMEventByID(eventID)) != NULL)
                    ongoingGMEvent->Player.Push(eventPlayer);
                else
                {
                    Error1("GMEventManager: gm_events / character_events table mismatch.");
                    return false;        // ermm.. somethings gone wrong with the DB!!!
                }
            }
        }
        else
        {
            Error1("GMEventManager: character_events table is not valid.");
            return false;
        }

        return true;
    }

    Error1("GMEventManager: gm_events table is not valid.");
    return false;
}

bool GMEventManager::AddNewGMEvent(Client* client, csString eventName, csString eventDescription)
{
    int newEventID, zero=0;
    PID gmID = client->GetPID();
    int clientnum = client->GetClientNum();
    GMEvent* gmEvent;

    // if this GM already has an active event, he/she cant start another
    if((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) != NULL)
    {
        psserver->SendSystemInfo(clientnum,
                                 "You are already running the \'%s\' event.",
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    if(eventName.Length() > MAX_EVENT_NAME_LENGTH)
    {
        eventName.Truncate(MAX_EVENT_NAME_LENGTH);
        psserver->SendSystemInfo(client->GetClientNum(),
                                 "Event name truncated to \'%s\'.",
                                 eventName.GetDataSafe());
    }


    // GM can register his/her event
    newEventID = GetNextEventID();

    // remove undesirable characters from the name & description
    csString escEventName, escEventDescription;
    db->Escape(escEventName, eventName.GetDataSafe());
    db->Escape(escEventDescription, eventDescription.GetDataSafe());

    // first in the database
    db->Command("INSERT INTO gm_events(id, gm_id, name, description, status) VALUES (%i, %i, '%s', '%s', %i)",
                newEventID,
                gmID.Unbox(),
                escEventName.GetDataSafe(),
                escEventDescription.GetDataSafe(),
                RUNNING);

    // and in the local cache.
    gmEvent = new GMEvent;
    gmEvent->id = newEventID;
    gmEvent->gmID = gmID;
    gmEvent->EndTime = 0;
    gmEvent->eventName = eventName;
    gmEvent->eventDescription = eventDescription;
    gmEvent->status = RUNNING;
    gmEvents.Push(gmEvent);

    // keep psCharacter upto date
    client->GetActor()->GetCharacterData()->AssignGMEvent(gmEvent->id,true);

    psserver->SendSystemInfo(client->GetClientNum(),
                             "You have initiated a new event, \'%s\'.",
                             eventName.GetDataSafe());

    return true;
}

bool GMEventManager::RegisterPlayerInGMEvent(Client* client, Client* target)
{
    PID playerID;
    int clientnum = client->GetClientNum();
    int zero=0;
    GMEvent* gmEvent;
    PID gmID = client->GetPID();

    // make sure GM is running (or assisting) an event, the player is valid and available.
    if((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        zero = 0;
        if((gmEvent = GetGMEventByPlayer(gmID, RUNNING, zero)) == NULL)
        {
            psserver->SendSystemInfo(clientnum, "You are not running/assisting an event.");
            return false;
        }
    }
    if(!target)
    {
        psserver->SendSystemInfo(clientnum, "Invalid target for registering in event.");
        return false;
    }
    if((playerID = target->GetPID()) == gmID)
    {
        psserver->SendSystemInfo(clientnum, "You cannot register yourself in your own event.");
        return false;
    }
    zero = 0;
    if(GetGMEventByPlayer(playerID, RUNNING, zero) != NULL)
    {
        psserver->SendSystemInfo(clientnum, "%s is already registered in an event.", target->GetName());
        return false;
    }
    if(gmEvent->Player.GetSize() == MAX_PLAYERS_PER_EVENT)
    {
        psserver->SendSystemInfo(clientnum,
                                 "There are already %d players in the \'%s\' event.",
                                 MAX_PLAYERS_PER_EVENT,
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    // store the registration in db
    db->Command("INSERT INTO character_events(event_id, player_id) VALUES (%i, %i)", gmEvent->id, playerID.Unbox());
    PlayerData eventPlayer;
    eventPlayer.PlayerID = playerID;
    eventPlayer.CanEvaluate = true;
    gmEvent->Player.Push(eventPlayer);

    // keep psCharacter up to date
    target->GetActor()->GetCharacterData()->AssignGMEvent(gmEvent->id,false);

    // player is available to join the valid GM event
    psserver->SendSystemInfo(clientnum, "%s is registered in the \'%s\' event.",
                             target->GetName(),
                             gmEvent->eventName.GetDataSafe());
    psserver->SendSystemInfo(target->GetClientNum(), "You are registered in the \'%s\' event.",
                             gmEvent->eventName.GetDataSafe());

    // if the player has sufficient security level, they can help administer the event
    if(target->GetSecurityLevel() >= SUPPORT_GM_LEVEL)
    {
        psserver->SendSystemInfo(clientnum, "He/she can assist with running this event.");
        psserver->SendSystemInfo(target->GetClientNum(), "You may assist running this event.");
    }

    return true;
}

bool GMEventManager::RegisterPlayersInRangeInGMEvent(Client* client, float range)
{
    int clientnum = client->GetClientNum(), zero = 0;
    PID gmID = client->GetPID();

    // make sure GM is running (or assisting) an event
    if(GetGMEventByGM(gmID, RUNNING, zero) == NULL)
    {
        zero = 0;
        if(GetGMEventByPlayer(gmID, RUNNING, zero) == NULL)
        {
            psserver->SendSystemInfo(clientnum, "You are not running/assisting an event.");
            return false;
        }
    }
    // check range is within max permissable
    if(range <= 0.0 || range > MAX_REGISTER_RANGE)
    {
        psserver->SendSystemInfo(clientnum,
                                 "Range should be greater than 0m upto %.2fm to register participants.",
                                 MAX_REGISTER_RANGE);
        return false;
    }

    // look for other players in range (ignoring NPCs & pets).
    gemActor* registeringGM = client->GetActor();
    csArray<gemObject*>* targetsInRange = registeringGM->GetObjectsInRange(range);
    for(size_t i=0; i<targetsInRange->GetSize(); i++)
    {
        gemObject* nearbyTarget;
        Client* target;
        if((nearbyTarget=targetsInRange->Get(i)) && (target=nearbyTarget->GetClient()) &&
                target != client && target->IsReady())
        {
            RegisterPlayerInGMEvent(client, target);
        }
    }

    return true;
}

bool GMEventManager::ListGMEvents(Client* client)
{
    psserver->SendSystemInfo(client->GetClientNum(), "Event list");
    psserver->SendSystemInfo(client->GetClientNum(), "--------------------");
    for(unsigned int i = 0 ; i < gmEvents.GetSize() ; i++)
    {
        if(gmEvents[i]->status == RUNNING)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "%s", gmEvents[i]->eventName.GetData());
        }
    }
    return true;
}

bool GMEventManager::CompleteGMEvent(Client* client, csString eventName)
{
    int zero = 0;
    GMEvent* theEvent = GetGMEventByName(eventName, RUNNING, zero);
    if(!theEvent)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Event %s wasn't found.", eventName.GetData());
        return false;
    }
    return CompleteGMEvent(client, theEvent->gmID, (theEvent->gmID == client->GetPID()));
}

bool GMEventManager::CompleteGMEvent(Client* client, PID gmID, bool byTheControllerGM)
{
    int zero = 0;

    // if this GM does not have an active event, he/she can't end it.
    GMEvent* theEvent;
    int clientnum = client->GetClientNum();

    if((theEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        psserver->SendSystemInfo(clientnum, "You are not running an event.");
        return false;
    }

    // inform players
    ClientConnectionSet* clientConnections = psserver->GetConnections();
    Client* target;
    for(size_t p = 0; p < theEvent->Player.GetSize(); p++)
    {
        if((target = clientConnections->FindPlayer(theEvent->Player[p].PlayerID)))
        {
            // psCharacter
            target->GetActor()->GetCharacterData()->CompleteGMEvent(false);

            psserver->SendSystemInfo(target->GetClientNum(),
                                     "Event '%s' complete.",
                                     theEvent->eventName.GetDataSafe());
        }
    }

    // GMs psCharacter
    if(byTheControllerGM)
        client->GetActor()->GetCharacterData()->CompleteGMEvent(true);

    // Update description & flag the event complete
    if(theEvent->gmID == UNDEFINED_GMID)
        theEvent->eventDescription += " (No GM)";
    else
        theEvent->eventDescription += " (" + csString(client->GetName()) + ")";
    csString EscEventDescription;
    db->Escape(EscEventDescription, theEvent->eventDescription);
    db->Command("UPDATE gm_events SET status = %d, description = '%s' WHERE id = %d",
                COMPLETED, EscEventDescription.GetDataSafe(), theEvent->id);
    theEvent->status = COMPLETED;
    theEvent->EndTime = csGetTicks();
    psserver->SendSystemInfo(clientnum, "Event '%s' complete.", theEvent->eventName.GetDataSafe());

    return true;
}

bool GMEventManager::RemovePlayerFromGMEvent(Client* client, Client* target)
{
    PID playerID;
    int clientnum = client->GetClientNum();
    int zero=0;
    GMEvent* gmEvent;
    GMEvent* playerEvent;
    PID gmID = client->GetPID();

    // make sure GM is running (or assisting) an event, the player is valid and registered.
    if((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        zero = 0;
        if((gmEvent = GetGMEventByPlayer(gmID, RUNNING, zero)) == NULL)
        {
            psserver->SendSystemInfo(clientnum, "You are not running/assisting an event.");
            return false;
        }
    }
    if(!target)
    {
        psserver->SendSystemInfo(clientnum, "Invalid target - cannot remove.");
        return false;
    }
    playerID = target->GetPID();
    zero = 0;
    playerEvent = GetGMEventByPlayer(playerID, RUNNING, zero);
    if(!playerEvent || gmEvent->id != playerEvent->id)
    {
        psserver->SendSystemInfo(clientnum,
                                 "%s is not registered in your \'%s\' event.",
                                 target->GetName(),
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    // all tests pass, drop the player
    RemovePlayerRefFromGMEvent(gmEvent, target, playerID);

    psserver->SendSystemInfo(clientnum, "%s has been removed from the \'%s\' event.",
                             target->GetName(),
                             gmEvent->eventName.GetDataSafe());
    psserver->SendSystemInfo(target->GetClientNum(), "You have been removed from the \'%s\' event.",
                             gmEvent->eventName.GetDataSafe());

    return true;
}

bool GMEventManager::RewardPlayersInGMEvent(Client* client,
        RangeSpecifier rewardRecipient,
        float range,
        Client* target,
        psRewardData* rewardData)
{
    GMEvent* gmEvent;
    int clientnum = client->GetClientNum(), zero = 0;
    PID gmID = client->GetPID();

    // make sure GM is running (or assisting) an event
    if((gmEvent = GetGMEventByGM(gmID, RUNNING, zero)) == NULL)
    {
        zero = 0;
        if((gmEvent = GetGMEventByPlayer(gmID, RUNNING, zero)) == NULL)
        {
            psserver->SendSystemInfo(clientnum, "You are not running/assisting an event.");
            return false;
        }
    }
    // check range is within max permissable
    if(rewardRecipient == IN_RANGE && (range <= 0.0 || range > MAX_REGISTER_RANGE))
    {
        psserver->SendSystemInfo(clientnum,
                                 "Range should be greater than 0m upto %.2fm to reward participants.",
                                 MAX_REGISTER_RANGE);
        return false;
    }

    if(rewardRecipient == INDIVIDUAL)
    {
        if(!target)
            psserver->SendSystemInfo(clientnum, "Invalid target for reward.");
        else
        {
            zero = 0;
            GMEvent* playersEvent = GetGMEventByPlayer(target->GetPID(), RUNNING, zero);
            if(playersEvent && playersEvent->id == gmEvent->id)
            {
                psserver->GetAdminManager()->AwardToTarget(clientnum, target, rewardData);
            }
            else
            {
                psserver->SendSystemInfo(clientnum,
                                         "%s is not registered in your \'%s\' event.",
                                         target->GetName(),
                                         gmEvent->eventName.GetDataSafe());
            }
        }
    }
    else
    {
        // reward ALL players (incl. within range)
        ClientConnectionSet* clientConnections = psserver->GetConnections();
        gemActor* clientActor = client->GetActor();
        for(size_t p = 0; p < gmEvent->Player.GetSize(); p++)
        {
            if((target = clientConnections->FindPlayer(gmEvent->Player[p].PlayerID)))
            {
                if(rewardRecipient == ALL || clientActor->RangeTo(target->GetActor()) <= range)
                {
                    psserver->GetAdminManager()->AwardToTarget(clientnum, target, rewardData);
                }
            }
        }
    }

    return true;
}

int GMEventManager::GetAllGMEventsForPlayer(PID playerID,
        csArray<int> &completedEvents,
        int &runningEventAsGM,
        csArray<int> &completedEventsAsGM)
{
    int runningEvent = -1, id, index=0;
    GMEvent* gmEvent;

    completedEvents.DeleteAll();
    completedEventsAsGM.DeleteAll();

    if((gmEvent = GetGMEventByGM(playerID, RUNNING, index)))
    {
        runningEventAsGM = gmEvent->id;
    }
    else
    {
        runningEventAsGM = -1;
    }

    index = 0;
    do
    {
        gmEvent = GetGMEventByGM(playerID, COMPLETED, index);
        if(gmEvent)
        {
            id = gmEvent->id;
            completedEventsAsGM.Push(id);
        }
    }
    while(gmEvent);

    index = 0;
    gmEvent = GetGMEventByPlayer(playerID, RUNNING, index);
    if(gmEvent)
        runningEvent = gmEvent->id;

    index = 0;
    do
    {
        gmEvent = GetGMEventByPlayer(playerID, COMPLETED, index);
        if(gmEvent)
        {
            id = gmEvent->id;
            completedEvents.Push(id);
        }
    }
    while(gmEvent);

    return (runningEvent);
}

bool GMEventManager::CheckEvalStatus(PID PlayerID, GMEvent* Event)
{
    if(Event->status == COMPLETED && PlayerID != Event->gmID &&
            Event->EndTime != 0 && csGetTicks()-Event->EndTime < EVAL_LOCKOUT_TIME)
    {
        size_t PlayerIndex = GetPlayerFromEvent(PlayerID, Event);
        if(PlayerIndex != SIZET_NOT_FOUND)
        {
            if(Event->Player.Get(PlayerIndex).CanEvaluate)
                return true;
        }
    }
    return false;
}

void GMEventManager::SetEvalStatus(PID PlayerID, GMEvent* Event, bool NewStatus)
{
    size_t PlayerIndex = GetPlayerFromEvent(PlayerID, Event);
    if(PlayerIndex != SIZET_NOT_FOUND)
    {
        Event->Player.Get(PlayerIndex).CanEvaluate = false;
    }
}

void GMEventManager::HandleGMEventCommand(MsgEntry* me, Client* client)
{
    psGMEventInfoMessage msg(me);
    PID RequestingPlayerID = client->GetPID();

    if(msg.command == psGMEventInfoMessage::CMD_QUERY)
    {
        GMEvent* theEvent;
        if((theEvent = GetGMEventByID(msg.id)) == NULL)
        {
            Error3("Client %s requested unavailable GM Event %d", client->GetName(), msg.id);
            return;
        }

        csString eventDesc(theEvent->eventDescription);

        if(theEvent->status != EMPTY)
        {
            ClientConnectionSet* clientConnections = psserver->GetConnections();
            Client* target;

            // if this client is the GM, list the participants too
            if(RequestingPlayerID == theEvent->gmID)
            {
                eventDesc.AppendFmt(". Participants: %zu. Online: ", theEvent->Player.GetSize());
                csArray<PlayerData>::Iterator iter = theEvent->Player.GetIterator();
                while(iter.HasNext())
                {
                    if((target = clientConnections->FindPlayer(iter.Next().PlayerID)))
                    {
                        eventDesc.AppendFmt("%s, ", target->GetName());
                    }
                }
            }
            else if(theEvent->status == RUNNING)  // and name the running GM
            {
                if(theEvent->gmID == UNDEFINED_GMID)
                {
                    eventDesc.AppendFmt(" (No GM)");
                }
                if((target = clientConnections->FindPlayer(theEvent->gmID)))
                {
                    eventDesc.AppendFmt(" (%s)", target->GetName());
                }
            }

            psGMEventInfoMessage response(me->clientnum,
                                          psGMEventInfoMessage::CMD_INFO,
                                          msg.id,
                                          theEvent->eventName.GetDataSafe(),
                                          eventDesc.GetDataSafe(),
                                          CheckEvalStatus(RequestingPlayerID, theEvent));
            response.SendMessage();
        }
        else
        {
            Error3("Client %s requested unavailable GM Event %d", client->GetName(), msg.id);
        }
    }
    else if(msg.command == psGMEventInfoMessage::CMD_EVAL)
    {
        GMEvent* Event;
        if((Event = GetGMEventByID(msg.id)) != NULL)
        {
            // check if we should allow to comment this event. Running Gm can't evaluate it
            if(CheckEvalStatus(RequestingPlayerID, Event))
            {
                WriteGMEventEvaluation(client, Event, msg.xml);
                psserver->SendSystemOK(client->GetClientNum(), "Thanks. Your evaluation was stored in the database.");
                SetEvalStatus(RequestingPlayerID, Event, false);
            }
            else
            {
                psserver->SendSystemError(client->GetClientNum(), "You can't evaluate this event.");
            }
        }
        else
        {
            Error3("Client %s evaluated an unavailable GM Event %d", client->GetName(), msg.id);
        }
    }
    else if(msg.command == psGMEventInfoMessage::CMD_DISCARD)
    {
        DiscardGMEvent(client, msg.id);
    }
}

bool GMEventManager::RemovePlayerFromGMEvents(PID playerID)
{
    int runningEventIDAsGM;
    int runningEventID, gmEventID;
    csArray<int> completedEventIDsAsGM;
    csArray<int> completedEventIDs;
    GMEvent* gmEvent;
    bool eventsFound = false;

    runningEventID = GetAllGMEventsForPlayer(playerID,
                     completedEventIDs,
                     runningEventIDAsGM,
                     completedEventIDsAsGM);

    // remove if partaking in an ongoing event
    if(runningEventID >= 0)
    {
        gmEvent = GetGMEventByID(runningEventID);
        if(gmEvent)
        {
            size_t PlayerIndex = GetPlayerFromEvent(playerID, gmEvent);
            if(PlayerIndex != SIZET_NOT_FOUND)
            {
                gmEvent->Player.DeleteIndex(PlayerIndex);
                eventsFound = true;
            }
        }
        else
            Error3("Cannot remove player %s from GM Event %d.", ShowID(playerID), runningEventID);
    }
    // remove ref's to old completed events
    csArray<int>::Iterator evIter = completedEventIDs.GetIterator();
    while(evIter.HasNext())
    {
        gmEventID = evIter.Next();
        gmEvent = GetGMEventByID(gmEventID);
        if(gmEvent)
        {
            size_t PlayerIndex = GetPlayerFromEvent(playerID, gmEvent);
            if(PlayerIndex != SIZET_NOT_FOUND)
            {
                gmEvent->Player.DeleteIndex(PlayerIndex);
                eventsFound = true;
            }
        }
        else
            Error3("Cannot remove player %s from GM Event %d.", ShowID(playerID), runningEventID);
    }
    // ...and from the DB too
    if(eventsFound)
        db->Command("DELETE FROM character_events WHERE player_id = %u", playerID.Unbox());

    // if this is a GM thats being deleted, set its GMID to UNDEFINED_GMID.
    if(runningEventIDAsGM >= 0)
    {
        gmEvent = GetGMEventByID(runningEventIDAsGM);
        if(gmEvent)
        {
            gmEvent->gmID = UNDEFINED_GMID;
            db->Command("UPDATE gm_events SET gm_id = %d WHERE id = %d", UNDEFINED_GMID, gmEvent->id);
        }
        else
            Error3("Cannot remove GM %s from Event %d.", ShowID(playerID), runningEventID);
    }
    evIter = completedEventIDsAsGM.GetIterator();
    while(evIter.HasNext())
    {
        gmEventID = evIter.Next();
        gmEvent = GetGMEventByID(gmEventID);
        if(gmEvent)
        {
            gmEvent->gmID = UNDEFINED_GMID;
            db->Command("UPDATE gm_events SET gm_id = %d WHERE id = %d", UNDEFINED_GMID, gmEvent->id);
        }
        else
            Error3("Cannot remove GM %s from Event %d.", ShowID(playerID), gmEventID);
    }

    return true;
}

bool GMEventManager::AssumeControlOfGMEvent(Client* client, csString eventName)
{
    int zero=0;
    PID newGMID = client->GetPID();
    int clientnum = client->GetClientNum();
    GMEvent* gmEvent;

    // if this GM already has an active event, he/she cant control another
    if((gmEvent = GetGMEventByGM(newGMID, RUNNING, zero)))
    {
        psserver->SendSystemInfo(clientnum,
                                 "You are already running the \'%s\' event.",
                                 gmEvent->eventName.GetDataSafe());
        return false;
    }

    // find the requested event
    zero = 0;
    if(!(gmEvent = GetGMEventByName(eventName, RUNNING, zero)))
    {
        psserver->SendSystemInfo(clientnum,
                                 "The \'%s\' event is not recognised or not running.",
                                 eventName.GetDataSafe());
        return false;
    }

    // look for the current GM if there is one
    ClientConnectionSet* clientConnections = psserver->GetConnections();
    Client* target;
    if(gmEvent->gmID != UNDEFINED_GMID)
    {
        if((target = clientConnections->FindPlayer(gmEvent->gmID)))
        {
            psserver->SendSystemInfo(clientnum,
                                     "The \'%s\' event's GM, %s, is online: you cannot assume control.",
                                     gmEvent->eventName.GetDataSafe(),
                                     target->GetName());
            return false;
        }
    }

    // if the GM is a participant, then remove the reference
    RemovePlayerRefFromGMEvent(gmEvent, client, newGMID);

    // OK, assume control
    gmEvent->gmID = newGMID;
    db->Command("UPDATE gm_events SET gm_id = %d WHERE id = %d", newGMID.Unbox(), gmEvent->id);
    client->GetActor()->GetCharacterData()->AssignGMEvent(gmEvent->id,true);
    psserver->SendSystemInfo(clientnum,
                             "You now control the \'%s\' event.",
                             gmEvent->eventName.GetDataSafe());

    csArray<PlayerData>::Iterator iter = gmEvent->Player.GetIterator();
    while(iter.HasNext())
    {
        if((target = clientConnections->FindPlayer(iter.Next().PlayerID)))
        {
            psserver->SendSystemInfo(target->GetClientNum(),
                                     "The GM %s is now controlling the \'%s\' event.",
                                     client->GetName(),
                                     gmEvent->eventName.GetDataSafe());
        }
    }

    return true;
}

bool GMEventManager::EraseGMEvent(Client* client, csString eventName)
{
    GMEvent* gmEvent;
    int zero=0;

    // cannot discard your running event
    if(GetGMEventByName(eventName, RUNNING, zero))
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot discard an ongoing event");
        return false;
    }

    // look for completed event to discard
    zero = 0;
    if((gmEvent = GetGMEventByName(eventName, COMPLETED, zero)))
        return EraseGMEvent(client, gmEvent);

    psserver->SendSystemError(client->GetClientNum(), "Cannot find this event...");
    return false;
}

void GMEventManager::WriteGMEventEvaluation(Client* client, GMEvent* Event, csString XmlStr)
{
    //NOTE: The xml format is <GMEventEvaluation vote=# comments="*" />
    csRef<iDocumentNode> Node;
    if((Node = ParseStringGetNode(XmlStr, "GMEventEvaluation")))
    {
        csString EscpComment;
        db->Escape(EscpComment, Node->GetAttributeValue("comments"));
        /* We have to cap the vote in the 1-10 area.
         * In order to avoid hacks which alters results, although they
         * can be identified also in another place (upon inspecting the data)
         */
        int vote = Node->GetAttributeValueAsInt("vote");
        if(vote > 10) vote = 10;
        if(vote <  1) vote =  1;
        db->Command("UPDATE character_events SET vote = %d, comments = \"%s\" WHERE player_id = %d AND event_id = %d", vote, EscpComment.GetDataSafe(), client->GetPID().Unbox(), Event->id);
    }
}

GMEventStatus GMEventManager::GetGMEventDetailsByID(int id,
        csString &name,
        csString &description)
{
    GMEvent* gmEvent = GetGMEventByID(id);

    if(gmEvent)
    {
        name = gmEvent->eventName;
        description = gmEvent->eventDescription;
        return gmEvent->status;
    }

    // ooops - some kinda cockup
    name = "?";
    description = "?";
    return EMPTY;
}

GMEventManager::GMEvent* GMEventManager::GetGMEventByID(int id)
{
    for(size_t e = 0; e < gmEvents.GetSize(); e++)
    {
        if(gmEvents[e]->id == id)
            return gmEvents[e];
    }

    return NULL;
}

GMEventManager::GMEvent* GMEventManager::GetGMEventByGM(PID gmID, GMEventStatus status, int &startIndex)
{
    for(size_t e = startIndex; e < gmEvents.GetSize(); e++)
    {
        startIndex++;
        if(gmEvents[e]->gmID == gmID && gmEvents[e]->status == status)
            return gmEvents[e];
    }

    return NULL;
}

GMEventManager::GMEvent* GMEventManager::GetGMEventByName(csString eventName, GMEventStatus status, int &startIndex)
{
    for(size_t e = startIndex; e < gmEvents.GetSize(); e++)
    {
        startIndex++;
        if(gmEvents[e]->eventName == eventName && gmEvents[e]->status == status)
            return gmEvents[e];
    }

    return NULL;
}

GMEventManager::GMEvent* GMEventManager::GetGMEventByPlayer(PID playerID, GMEventStatus status, int &startIndex)
{
    for(size_t e = startIndex; e < gmEvents.GetSize(); e++)
    {
        startIndex++;
        if(gmEvents[e]->status == status)
        {
            csArray<PlayerData>::Iterator iter = gmEvents[e]->Player.GetIterator();
            while(iter.HasNext())
            {
                if(iter.Next().PlayerID == playerID)
                {
                    return gmEvents[e];
                }
            }
        }
    }

    return NULL;
}

size_t GMEventManager::GetPlayerFromEvent(PID &PlayerID, GMEvent* Event)
{
    for(size_t Position = 0; Position < Event->Player.GetSize(); Position++)
    {
        if(Event->Player.Get(Position).PlayerID == PlayerID)
            return Position;
    }
    return SIZET_NOT_FOUND;
}

int GMEventManager::GetNextEventID(void)
{
    // TODO this is just too simple
    return nextEventID++;
}

void GMEventManager::DiscardGMEvent(Client* client, int eventID)
{
    int runningEventIDAsGM;
    int runningEventID;
    PID playerID;
    csArray<int> completedEventIDsAsGM;
    csArray<int> completedEventIDs;
    GMEvent* gmEvent;

    // look for this event in the player's events
    runningEventID = GetAllGMEventsForPlayer((playerID = client->GetPID()),
                     completedEventIDs,
                     runningEventIDAsGM,
                     completedEventIDsAsGM);

    // cannot discard an ongoing event if the player is the GM of it
    if(runningEventIDAsGM == eventID)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot discard an ongoing event for which you are the GM");
        return;
    }

    gmEvent = GetGMEventByID(eventID);

    // GM discarding an old event - all traces shall be removed.
    if(completedEventIDsAsGM.Find(eventID) != csArrayItemNotFound && gmEvent)
    {
        EraseGMEvent(client, gmEvent);
        return;
    }

    // attempt to remove player from event...
    if((runningEventID == eventID || completedEventIDs.Find(eventID) != csArrayItemNotFound) && gmEvent)
    {
        if(RemovePlayerRefFromGMEvent(gmEvent, client, playerID))
            psserver->SendSystemInfo(client->GetClientNum(),
                                     "You have discarded the \'%s\' event.",
                                     gmEvent->eventName.GetDataSafe());

        ClientConnectionSet* clientConnections = psserver->GetConnections();
        Client* target = clientConnections->FindPlayer(gmEvent->gmID);
        if(target)
        {
            psserver->SendSystemInfo(target->GetClientNum(),
                                     "%s has discarded the \'%s\' event.",
                                     client->GetName(),
                                     gmEvent->eventName.GetDataSafe());
        }
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "Cannot find this event...");
        Error2("Cannot find event ID %d.", eventID);
    }
}

bool GMEventManager::RemovePlayerRefFromGMEvent(GMEvent* gmEvent, Client* client, PID playerID)
{
    size_t PlayerIndex = GetPlayerFromEvent(playerID, gmEvent);

    if(PlayerIndex != SIZET_NOT_FOUND && gmEvent->Player.DeleteIndex(PlayerIndex))
    {
        // ...from the database
        db->Command("DELETE FROM character_events WHERE event_id = %d AND player_id = %d", gmEvent->id, playerID.Unbox());
        // ...from psCharacter
        client->GetActor()->GetCharacterData()->RemoveGMEvent(gmEvent->id);

        return true;
    }

    return false;
}

bool GMEventManager::EraseGMEvent(Client* client, GMEvent* gmEvent)
{
    PID discarderGMID = client->GetPID();
    int clientnum = client->GetClientNum();

    // If event belongs to another GM, only proceed if that GM not online.
    ClientConnectionSet* clientConnections = psserver->GetConnections();
    Client* target;
    if(discarderGMID != gmEvent->gmID && gmEvent->gmID != UNDEFINED_GMID)
    {
        if((target = clientConnections->FindPlayer(gmEvent->gmID)))
        {
            psserver->SendSystemInfo(clientnum,
                                     "The \'%s\' event's GM, %s, is online: you cannot discard their event.",
                                     gmEvent->eventName.GetDataSafe(),
                                     target->GetName());
            return false;
        }
    }

    // Remove players.
    size_t noPlayers = gmEvent->Player.GetSize();
    for(size_t p = 0; p < noPlayers; p++)
    {
        PID playerID = gmEvent->Player[p].PlayerID;
        if((target = clientConnections->FindPlayer(playerID)))
        {
            // psCharacter
            target->GetActor()->GetCharacterData()->RemoveGMEvent(gmEvent->id);

            psserver->SendSystemInfo(target->GetClientNum(),
                                     "Event '%s' has been discarded.",
                                     gmEvent->eventName.GetDataSafe());
        }
    }
    gmEvent->Player.DeleteAll();

    // remove GM
    if(discarderGMID == gmEvent->gmID  && gmEvent->gmID != UNDEFINED_GMID)
    {
        client->GetActor()->GetCharacterData()->RemoveGMEvent(gmEvent->id, true);
    }

    // remove from DB
    db->Command("DELETE FROM character_events WHERE event_id = %d", gmEvent->id);
    db->Command("DELETE FROM gm_events WHERE id = %d", gmEvent->id);

    psserver->SendSystemInfo(client->GetClientNum(),
                             "Event '%s' has been discarded.",
                             gmEvent->eventName.GetDataSafe());

    // remove gmEvent ref
    gmEvents.Delete(gmEvent);

    return true;
}

