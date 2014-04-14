/*
* tutorialmanager.cpp
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <string.h>
#include <memory.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/psdatabase.h"

#include "bulkobjects/pscharacter.h"
#include "bulkobjects/pscharacterloader.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "tutorialmanager.h"
#include "client.h"
#include "clients.h"
#include "events.h"
#include "gem.h"
#include "netmanager.h"
#include "globals.h"

TutorialManager::TutorialManager(ClientConnectionSet* pCCS)
{
    clients = pCCS;
    Subscribe(&TutorialManager::HandleConnect, MSGTYPE_CONNECT_EVENT, REQUIRE_ANY_CLIENT);
    Subscribe(&TutorialManager::HandleMovement, MSGTYPE_MOVEMENT_EVENT, REQUIRE_ANY_CLIENT);
    Subscribe(&TutorialManager::HandleTarget, MSGTYPE_TARGET_EVENT, NO_VALIDATION);
    Subscribe(&TutorialManager::HandleGeneric, MSGTYPE_GENERIC_EVENT, REQUIRE_ANY_CLIENT);
    Subscribe(&TutorialManager::HandleDamage, MSGTYPE_DAMAGE_EVENT, NO_VALIDATION);
    Subscribe(&TutorialManager::HandleDeath, MSGTYPE_DEATH_EVENT, NO_VALIDATION);
    LoadTutorialStrings();
}

TutorialManager::~TutorialManager()
{
    //do nothing
}

bool TutorialManager::LoadTutorialStrings()
{
    Result result(db->Select("select * from tips where id>1000"));
    if(!result.IsValid())
    {
        Error2("Could not load tutorial strings due to database error: %s\n",
               db->GetLastError());
        return false;
    }

    for(unsigned long i=0; i < result.Count(); i++)
    {
        int id = result[i].GetInt("id") - 1001;
        csString tip = result[i]["tip"];
        tutorialMsg.GetExtend(id) = tip;
    }
    return true;
}

void TutorialManager::SendTutorialMessage(int which, Client* client, const char* instrs)
{
    SendTutorialMessage(which, client->GetClientNum(), instrs);
}

void TutorialManager::SendTutorialMessage(int which, uint32_t clientnum, const char* instrs)
{
    if(!instrs)
    {
        csString error;
        error.Format("TutorialEventMessage %d is missing instructions.  Please file a bug.", which);
        instrs = error;
    }
    psTutorialMessage msg(clientnum, which, instrs);
    msg.SendMessage();
}

void TutorialManager::HandleConnect(MsgEntry* me,Client* client)
{
//    printf("Got psConnectEvent\n");
    psConnectEvent evt(me);
    if(client)
    {
        psCharacter* ch = client->GetCharacterData();
        if(ch->NeedsHelpEvent(CONNECT))
        {
            SendTutorialMessage(CONNECT,client,tutorialMsg[CONNECT]);
            ch->CompleteHelpEvent(CONNECT);
        }
    }
}

void TutorialManager::HandleMovement(MsgEntry* me,Client* client)
{
    // printf("Got psMovementEvent\n");
    psMovementEvent evt(me);
    if(client)
    {
        psCharacter* ch = client->GetCharacterData();
        if(ch->NeedsHelpEvent(MOVEMENT))
        {
            SendTutorialMessage(MOVEMENT,client,tutorialMsg[MOVEMENT]);
            ch->CompleteHelpEvent(MOVEMENT);
        }
    }
}

// psTargetEvent already published so intercepting this takes zero code
void TutorialManager::HandleTarget(MsgEntry* me,Client* client)
{
    // printf("Got psTargetEvent\n");
    psTargetChangeEvent evt(me);
    if(evt.character)
    {
        client = evt.character->GetClient();
        psCharacter* ch = evt.character->GetCharacterData();
        if(ch->NeedsHelpEvent(NPC_SELECT))
        {
            SendTutorialMessage(NPC_SELECT,client,tutorialMsg[NPC_SELECT]);
            ch->CompleteHelpEvent(NPC_SELECT);
        }

        if(ch->NeedsHelpEvent(NPC_TALK))
        {
            SendTutorialMessage(NPC_TALK,client,tutorialMsg[NPC_TALK]);
            ch->CompleteHelpEvent(NPC_TALK);
        }
    }
}

/// Specifically handle the Damage event in the tutorial
void TutorialManager::HandleDamage(MsgEntry* me,Client* client)
{
    //printf("Got psDamageEvent\n");
    psDamageEvent evt(me);
    if(evt.target && evt.attacker)   // someone hurt us
    {
        client = evt.target->GetClient();
        if(client)
        {
            psCharacter* ch = evt.target->GetCharacterData();
            if(ch->NeedsHelpEvent(DAMAGE_SELF))
            {
                SendTutorialMessage(DAMAGE_SELF,client,tutorialMsg[DAMAGE_SELF]);
                ch->CompleteHelpEvent(DAMAGE_SELF);
            }
        }
    }
    else if(evt.target && !evt.attacker)  // no one hurt us, so fall damage
    {
        client = evt.target->GetClient();
        if(client)
        {
            psCharacter* ch = evt.target->GetCharacterData();
            if(ch->NeedsHelpEvent(DAMAGE_FALL))
            {
                SendTutorialMessage(DAMAGE_FALL,client,tutorialMsg[DAMAGE_FALL]);
                ch->CompleteHelpEvent(DAMAGE_FALL);
            }
        }
    }
}

/// Specifically handle the message sent by a script
void TutorialManager::HandleScriptMessage(uint32_t client, unsigned int which)
{
    if(which < tutorialMsg.GetSize())
    {
        SendTutorialMessage(which, client, tutorialMsg[which]);
    }
    else
    {
        Error2("Tip %d doesn't exist in the database.",which+1001);
    }
}

/// Specifically handle the Damage event in the tutorial
void TutorialManager::HandleDeath(MsgEntry* me,Client* client)
{
    //printf("Got psDeathEvent\n");
    psDeathEvent evt(me);
    if(evt.deadActor)   // We're dead
    {
        client = evt.deadActor->GetClient();
        if(client)
        {
            psCharacter* ch = evt.deadActor->GetCharacterData();
            if(ch->NeedsHelpEvent(DEATH_SELF))
            {
                SendTutorialMessage(DEATH_SELF,client,tutorialMsg[DEATH_SELF]);
                ch->CompleteHelpEvent(DEATH_SELF);
            }
        }
    }
}

void TutorialManager::HandleGeneric(MsgEntry* me,Client* client)
{
    // printf("Got psGenericEvent\n");
    psGenericEvent evt(me);
    if(evt.client_id)
    {
        switch(evt.eventType)
        {
                /* Quest assignments are now explained in the tutorial, this is redundant
                case psGenericEvent::QUEST_ASSIGN:
                {
                    psCharacter *ch = client->GetCharacterData();
                    if (ch->NeedsHelpEvent(QUEST_ASSIGN))
                    {
                        SendTutorialMessage(QUEST_ASSIGN,client,tutorialMsg[QUEST_ASSIGN]);
                        ch->CompleteHelpEvent(QUEST_ASSIGN);
                    }
                    break;
                }
                */
            case psGenericEvent::SPAWN_MOVE:
            {
                psCharacter* ch = client->GetCharacterData();
                if(ch->NeedsHelpEvent(SPAWN_MOVE))
                {
                    SendTutorialMessage(SPAWN_MOVE,client,tutorialMsg[SPAWN_MOVE]);
                    ch->CompleteHelpEvent(SPAWN_MOVE);
                }
                break;
            }
            default:
                // printf("Unknown generic event received in Tutorial Manager.\n");
                break;
        }
    }
}
