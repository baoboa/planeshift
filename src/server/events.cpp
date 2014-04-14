/*
 * events.cpp
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 * Author: Keith Fulton <keith@planeshift.it>
 */

#include <psconfig.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/strutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "events.h"
#include "psserver.h"
#include "netmanager.h"
#include "economymanager.h"
#include "globals.h"
#include "gem.h"

PSF_IMPLEMENT_MSG_FACTORY(psDamageEvent,MSGTYPE_DAMAGE_EVENT);

psDamageEvent::psDamageEvent(gemActor* attack,gemActor* victim,float dmg)
{
    msg.AttachNew(new MsgEntry(sizeof(gemActor*)*2 + sizeof(float) ,PRIORITY_LOW));

    msg->SetType(MSGTYPE_DAMAGE_EVENT);
    msg->clientnum      = 0;
    msg->AddPointer((uintptr_t) attack);
    msg->AddPointer((uintptr_t) victim);
    msg->Add(dmg);

    valid=!(msg->overrun);
}

psDamageEvent::psDamageEvent(MsgEntry* event)
{
    if(!event)
    {
        attacker = NULL;
        target   = NULL;
        damage   = 0.0;
        return;
    }

    attacker = (gemActor*)event->GetPointer();
    target   = (gemActor*)event->GetPointer();
    damage   = event->GetFloat();
}

csString psDamageEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Attacker: %p Target: %p Damage: %f",(void*)attacker,(void*)target,damage);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psDeathEvent,MSGTYPE_DEATH_EVENT);

psDeathEvent::psDeathEvent(gemActor* dead,gemActor* killer)
{
    msg.AttachNew(new MsgEntry(sizeof(gemActor*)*2 ,PRIORITY_LOW));

    msg->SetType(MSGTYPE_DEATH_EVENT);
    msg->clientnum = 0;
    msg->AddPointer((uintptr_t) dead);
    msg->AddPointer((uintptr_t) killer);


    valid=!(msg->overrun);
}

psDeathEvent::psDeathEvent(MsgEntry* event)
{
    if(!event)
    {
        deadActor = NULL;
        killer    = NULL;
        return;
    }

    deadActor = (gemActor*)event->GetPointer();
    killer    = (gemActor*)event->GetPointer();
}

csString psDeathEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("DeadActor: %p Killer: %p",(void*)deadActor,(void*)killer);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psTargetChangeEvent,MSGTYPE_TARGET_EVENT);

psTargetChangeEvent::psTargetChangeEvent(gemActor* targeter, gemObject* targeted)
{
    msg.AttachNew(new MsgEntry(sizeof(gemActor*) + sizeof(gemObject*), PRIORITY_LOW));

    msg->SetType(MSGTYPE_TARGET_EVENT);
    msg->clientnum = 0;
    msg->AddPointer((uintptr_t) targeter);
    msg->AddPointer((uintptr_t) targeted);

    valid = !(msg->overrun);
}

psTargetChangeEvent::psTargetChangeEvent(MsgEntry* event)
{
    if(!event)
    {
        character = NULL;
        target    = NULL;
        return;
    }

    character = (gemActor*)event->GetPointer();
    target    = (gemObject*)event->GetPointer();
}

csString psTargetChangeEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Character: %p Target: %p",(void*)character,(void*)target);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psZPointsGainedEvent,MSGTYPE_ZPOINT_EVENT);

psZPointsGainedEvent::psZPointsGainedEvent(gemActor* actor, const char* name, int gained, bool rankup)
{
    msg.AttachNew(new MsgEntry(sizeof(gemActor*) +
                               sizeof(int) +
                               sizeof(bool) +
                               strlen(name)+1, PRIORITY_LOW));

    msg->SetType(MSGTYPE_ZPOINT_EVENT);
    msg->clientnum = actor->GetClientID();
    msg->Add(name);
    msg->AddPointer((uintptr_t) actor);
    msg->Add((uint32_t)gained);
    msg->Add(rankup);
}

psZPointsGainedEvent::psZPointsGainedEvent(MsgEntry* event)
{
    if(!event)
    {
        actor = NULL;
        amountGained = 0;
        rankUp = false;
        return;
    }

    skillName = event->GetStr();
    actor = (gemActor*)event->GetPointer();
    amountGained = event->GetUInt32();
    rankUp = event->GetBool();
}

csString psZPointsGainedEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Skill: %s Actor: %p ZGained: %d RankUp: %s",
                      skillName.GetDataSafe(),(void*)actor,amountGained,(rankUp?"true":"false"));

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psBuyEvent,MSGTYPE_BUY_EVENT);

psBuyEvent::psBuyEvent(PID from, const char* fromName, PID to, const char* toName, unsigned int item, const char* itemName, int stack, int quality, unsigned int price)
{
    // Merchant => Player

    msg.AttachNew(new MsgEntry((sizeof(int) * 4) + strlen(fromName) + 1 + strlen(toName) + 1 + strlen(itemName) + 1 + sizeof(unsigned int)+
                               sizeof(item), PRIORITY_LOW));

    msg->SetType(MSGTYPE_BUY_EVENT);
    msg->clientnum = 0;
    msg->Add(from.Unbox());
    msg->Add(fromName);
    msg->Add(to.Unbox());
    msg->Add(toName);
    msg->Add(item);
    msg->Add(itemName);
    msg->Add((int32_t) stack);
    msg->Add((int32_t) quality);
    msg->Add((uint32_t) price);
}

psBuyEvent::psBuyEvent(MsgEntry* event)
{
    if(!event)
        return;

    trans.AttachNew(new TransactionEntity()); // needs to be handled by economy manager

    trans->from = PID(event->GetUInt32());
    trans->fromName = event->GetStr();
    trans->to = PID(event->GetUInt32());
    trans->toName = event->GetStr();

    trans->item = event->GetUInt32();
    trans->itemName = event->GetStr();
    trans->count = event->GetInt32();
    trans->quality = event->GetInt32();
    trans->price = event->GetUInt32();
}

csString psBuyEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("From: %s To: %s Item: '%d' Count: %d Quality: %d Price %d",
                      ShowID(trans->from), ShowID(trans->to), trans->item,
                      trans->count,trans->quality,trans->price);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSellEvent,MSGTYPE_SELL_EVENT);

psSellEvent::psSellEvent(PID from, const char* fromName, PID to, const char* toName, unsigned int item, const char* itemName, int stack, int quality, unsigned int price)
{
    // Player => Merchant

    msg.AttachNew(new MsgEntry((sizeof(int) * 4) + strlen(fromName) + 1 + strlen(toName) + 1 + strlen(itemName) + 1 + sizeof(unsigned int)+
                               sizeof(item), PRIORITY_LOW));

    msg->SetType(MSGTYPE_SELL_EVENT);
    msg->clientnum = 0;
    msg->Add(from.Unbox());
    msg->Add(fromName);
    msg->Add(to.Unbox());
    msg->Add(toName);
    msg->Add(item);
    msg->Add(itemName);
    msg->Add((int32_t) stack);
    msg->Add((int32_t) quality);
    msg->Add((uint32_t) price);
}

psSellEvent::psSellEvent(MsgEntry* event)
{
    if(!event)
        return;

    trans.AttachNew(new TransactionEntity()); // needs to be handled by economy manager

    trans->from = PID(event->GetUInt32());
    trans->fromName = event->GetStr();
    trans->to = PID(event->GetUInt32());
    trans->toName = event->GetStr();

    trans->item = event->GetUInt32();
    trans->itemName = event->GetStr();

    trans->count = event->GetInt32();
    trans->quality = event->GetInt32();
    trans->price = event->GetUInt32();
}

csString psSellEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("From: %s To: %s Item: '%d' Count: %d Quality: %d Price %d",
                      ShowID(trans->from), ShowID(trans->to), trans->item,
                      trans->count,trans->quality,trans->price);

    return msgtext;
}
//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPickupEvent,MSGTYPE_PICKUP_EVENT);

psPickupEvent::psPickupEvent(PID to, const char* toName, unsigned int item, const char* itemName, int stack, int quality,unsigned int price)
{
    // Player => Merchant

    msg.AttachNew(new MsgEntry((sizeof(int) * 4) + strlen(toName) + 1 + strlen(itemName) + 1 + sizeof(unsigned int)+
                               sizeof(item), PRIORITY_LOW));

    msg->SetType(MSGTYPE_PICKUP_EVENT);
    msg->clientnum = 0;
    msg->Add(to.Unbox());
    msg->Add(toName);
    msg->Add(item);
    msg->Add(itemName);
    msg->Add((int32_t) stack);
    msg->Add((int32_t) quality);
    msg->Add((uint32_t) price);
}

psPickupEvent::psPickupEvent(MsgEntry* event)
{
    if(!event)
        return;

    trans.AttachNew(new TransactionEntity()); // needs to be handled by economy manager

    trans->to = PID(event->GetUInt32());
    trans->toName = event->GetStr();

    trans->item = event->GetUInt32();
    trans->itemName = event->GetStr();
    trans->count = event->GetInt32();
    trans->quality = event->GetInt32();
    trans->price = event->GetUInt32();
}

csString psPickupEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("From: %s To: %s Item: '%d' Count: %d Quality: %d Price %d",
                      ShowID(trans->from), ShowID(trans->to), trans->item,
                      trans->count,trans->quality,trans->price);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psDropEvent,MSGTYPE_DROP_EVENT);

psDropEvent::psDropEvent(PID from, const char* fromName, unsigned int item, const char* itemName, int stack, int quality,unsigned int price)
{
    // Player => Merchant

    msg.AttachNew(new MsgEntry((sizeof(int) * 4) + strlen(fromName) + 1 + strlen(itemName) + 1 + sizeof(unsigned int)+
                               sizeof(item), PRIORITY_LOW));

    msg->SetType(MSGTYPE_DROP_EVENT);
    msg->clientnum = 0;
    msg->Add(from.Unbox());
    msg->Add(fromName);
    msg->Add(item);
    msg->Add(itemName);
    msg->Add((int32_t) stack);
    msg->Add((int32_t) quality);
    msg->Add((uint32_t) price);
}

psDropEvent::psDropEvent(MsgEntry* event)
{
    if(!event)
        return;

    trans.AttachNew(new TransactionEntity()); // needs to be handled by economy manager

    trans->from = PID(event->GetUInt32());
    trans->fromName = event->GetStr();

    trans->item = event->GetUInt32();
    trans->itemName = event->GetStr();
    trans->count = event->GetInt32();
    trans->quality = event->GetInt32();
    trans->price = event->GetUInt32();
}

csString psDropEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("From: %u To: %u Item: '%d' Count: %d Quality: %d Price %d",
                      trans->from.Unbox(), trans->to.Unbox(), trans->item,
                      trans->count,trans->quality,trans->price);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psLootEvent,MSGTYPE_LOOT_EVENT);

psLootEvent::psLootEvent(PID from, const char* fromName, PID to, const char* toName, unsigned int item, const char* itemName, int stack, int quality, unsigned int price)
{
    // Player => Merchant

    msg.AttachNew(new MsgEntry((sizeof(int) * 4) + strlen(fromName) + 1 + strlen(toName) + 1 + strlen(itemName) + 1 + sizeof(unsigned int)+
                               sizeof(item), PRIORITY_LOW));

    msg->SetType(MSGTYPE_LOOT_EVENT);
    msg->clientnum = 0;
    msg->Add(from.Unbox());
    msg->Add(fromName);
    msg->Add(to.Unbox());
    msg->Add(toName);
    msg->Add(item);
    msg->Add(itemName);
    msg->Add((int32_t) stack);
    msg->Add((int32_t) quality);
    msg->Add((uint32_t) price);
}

psLootEvent::psLootEvent(MsgEntry* event)
{
    if(!event)
        return;

    trans.AttachNew(new TransactionEntity()); // needs to be handled by economy manager

    trans->from = PID(event->GetUInt32());
    trans->fromName = event->GetStr();
    trans->to = PID(event->GetUInt32());
    trans->toName = event->GetStr();

    trans->item = event->GetUInt32();
    trans->itemName = event->GetStr();
    trans->count = event->GetInt32();
    trans->quality = event->GetInt32();
    trans->price = event->GetUInt32();
}

csString psLootEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("From: %s To: %s Item: '%d' Count: %d Quality: %d Price %d",
                      ShowID(trans->from), ShowID(trans->to), trans->item,
                      trans->count,trans->quality,trans->price);

    return msgtext;
}


//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psConnectEvent,MSGTYPE_CONNECT_EVENT);

psConnectEvent::psConnectEvent(int clientID)
{
    // Player => Merchant

    msg.AttachNew(new MsgEntry(sizeof(int), PRIORITY_LOW));

    msg->SetType(MSGTYPE_CONNECT_EVENT);
    msg->clientnum = clientID;
    msg->Add((int32_t) clientID);
}

psConnectEvent::psConnectEvent(MsgEntry* event)
{
    if(!event)
    {
        client_id = 0;
        return;
    }

    client_id = event->GetInt32();
}

csString psConnectEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ClientID: %d", client_id);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMovementEvent,MSGTYPE_MOVEMENT_EVENT);

psMovementEvent::psMovementEvent(int clientID)
{
    msg.AttachNew(new MsgEntry(sizeof(int), PRIORITY_LOW));

    msg->SetType(MSGTYPE_MOVEMENT_EVENT);
    msg->clientnum = clientID;
    msg->Add((int32_t) clientID);
}

psMovementEvent::psMovementEvent(MsgEntry* event)
{
    if(!event)
    {
        client_id = 0;
        return;
    }

    client_id = event->GetInt32();
}

csString psMovementEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ClientID: %d", client_id);

    return msgtext;
}



//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGenericEvent,MSGTYPE_GENERIC_EVENT);

psGenericEvent::psGenericEvent(int clientID, psGenericEvent::Type type)
{
    msg.AttachNew(new MsgEntry(sizeof(int)*2, PRIORITY_LOW));

    msg->SetType(MSGTYPE_GENERIC_EVENT);
    msg->clientnum = clientID;
    msg->Add((int32_t) type);
    msg->Add((int32_t) clientID);
}

psGenericEvent::psGenericEvent(MsgEntry* event)
{
    if(!event)
    {
        eventType = UNKNOWN;
        client_id = 0;
        return;
    }

    eventType = (psGenericEvent::Type) event->GetInt32();
    client_id = event->GetInt32();
}

csString psGenericEvent::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Type: %d, ClientID: %d", eventType, client_id);

    return msgtext;
}
