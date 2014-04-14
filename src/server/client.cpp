/*
 * client.cpp - Author: Keith Fulton
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/util.h>


//=============================================================================
// Project Space Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psguildinfo.h"

#include "util/psscf.h"
#include "util/consoleout.h"
#include "util/psdatabase.h"
#include "net/msghandler.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "usermanager.h"
#include "client.h"
#include "psserver.h"
#include "playergroup.h"
#include "globals.h"
#include "netmanager.h"
#include "combatmanager.h"
#include "events.h"
#include "advicemanager.h"
#include "entitymanager.h"
#include "cachemanager.h"
#include "adminmanager.h"

Client::Client()
    : accumulatedLag(0), zombie(false), zombietimeout(0), 
      allowedToDisconnect(true), ready(false),
      accountID(0), playerID(0), securityLevel(0), superclient(false),
      name(""), waypointPathIndex(0), pathPath(NULL), selectedLocationID(0),
      cheatMask(NO_CHEAT)
{
    actor           = 0;
    exchangeID      = 0;
    advisorPoints   = 0;
    lastInviteTime  = 0;
    spamPoints      = 0;
    clientnum       = 0;
    detectedCheatCount = 0;

    nextFloodHistoryIndex = 0;

    lastInventorySend = 0;
    lastGlyphSend = 0;

    isAdvisor           = false;
    lastInviteResult    = true;
    hasBeenWarned       = false;
    hasBeenPenalized    = false;
    valid               = false;
    isBuddyListHiding   = false;

    mute.Initialize(this);
    SetMute(false);

    // pets[0] is a special case for the player's familiar.
    pets.Insert(0, PID(0));
}

Client::~Client()
{
}

bool Client::Initialize(LPSOCKADDR_IN addr, uint32_t clientnum)
{
    Client::addr=*addr;
    Client::clientnum=clientnum;
    Client::valid=true;

    outqueue.AttachNew(new NetPacketQueueRefCount(MAXCLIENTQUEUESIZE));
    if(!outqueue)
        ERRORHALT("No Memory!");

    return true;
}

bool Client::Disconnect()
{
    // Make sure the advisor system knows this client is gone.
    if(isAdvisor)
    {
        psserver->GetAdviceManager()->RemoveAdvisor(this->GetClientNum(), 0);
    }

    if(GetActor() && GetActor()->InGroup())
    {
        GetActor()->RemoveFromGroup();
    }

    // Only save if an account has been found for this client.
    if(accountID.IsValid())
    {
        SaveAccountData();
    }

    /*we have to clear the challenges else the other players will be stuck
     in challenge mode */
    if(GetDuelClientCount())
    {
        ClearAllDuelClients();
    }

    return true;
}

void Client::SetAllowedToDisconnect(bool allowed)
{
    allowedToDisconnect = allowed;
}

bool Client::AllowDisconnect()
{
    if(!GetActor() || !GetCharacterData())
        return true;

    if(GetActor()->GetInvincibility())
        return true;

    if(!zombie)
    {
        zombie = true;
        // max 3 minute timeout period
        zombietimeout = csGetTicks() + 3 * 60 * 1000;
    }
    else if(csGetTicks() > zombietimeout)
    {
        return true;
    }

    return allowedToDisconnect;
}

// Called from network thread, no access to server
// internal data should be made
bool Client::ZombieAllowDisconnect()
{
    if(csGetTicks() > zombietimeout)
    {
        return true;
    }

    return allowedToDisconnect;
}

void Client::SetTargetObject(gemObject* newobject, bool updateClientGUI)
{
    gemActor* myactor = GetActor();
    if(myactor)
    {
        // We don't want to fire a target change event if the target hasn't changed.
        if(newobject == myactor->GetTargetObject())
            return;

        myactor->SetTargetObject(newobject);

        psTargetChangeEvent targetevent(myactor, newobject);
        targetevent.FireEvent();

        if(updateClientGUI)
        {
            psGUITargetUpdateMessage updateMessage(GetClientNum(), newobject? newobject->GetEID() : 0);
            updateMessage.SendMessage();
        }
    }
}

gemObject* Client::GetTargetObject() const
{
    gemActor* myactor = GetActor();
    if(myactor)
    {
        return myactor->GetTargetObject();
    }
    return NULL;
}

void Client::SetFamiliar(gemActor* familiar)
{
    if(familiar)
    {
        pets[0] = familiar->GetPID();
    }
    else
    {
        pets[0] = PID(0);
    }
}

gemActor* Client::GetFamiliar()
{
    if(pets[0].IsValid())
    {
        return psserver->entitymanager->GetGEM()->FindNPCEntity(pets[0]);
    }
    return NULL;
}

void Client::AddPet(gemActor* pet)
{
    pets.Push(pet->GetPID());
}

void Client::RemovePet(size_t i)
{
    // Be careful to not cause pet numbers to be reordered.
    if(i > 0)
    {
        if(i == pets.GetSize() - 1)
            pets.Truncate(i);
        else
            pets[i] = PID(0);
    }
}

gemActor* Client::GetPet(size_t i)
{
    if(i >= pets.GetSize() || !pets[i].IsValid())
        return NULL;

    return psserver->entitymanager->GetGEM()->FindNPCEntity(pets[i]);
}

size_t Client::GetNumPets()
{
    return pets.GetSize();
}

bool Client::IsMyPet(gemActor* other) const
{
    for(size_t i = 0; i < pets.GetSize(); i++)
    {
        if(psserver->entitymanager->GetGEM()->FindNPCEntity(pets[i]) == other)
        {
            return true;
        }
    }
    return false;
}

void Client::SetName(const char* n)
{
    name = n;
}

const char* Client::GetName()
{
    // If there is a character than get the first name of the character.
    if(GetCharacterData())
    {
        return GetCharacterData()->GetCharName();
    }

    // Until a character is loaded use the name set when the client is created. This will than
    // be the account name.
    return name;
}

void Client::SetActor(gemActor* myactor)
{
    actor = myactor;
    if(actor == NULL)
    {
        allowedToDisconnect = true;
    }
}

psCharacter* Client::GetCharacterData()
{
    return (actor?actor->GetCharacterData():NULL);
}


bool Client::ValidateDistanceToTarget(float range)
{
    gemObject* targetObject = GetTargetObject();

    // Check if target is set
    if(!targetObject) return false;

    return actor->IsNear(targetObject, range);
}


int Client::GetTargetClientID()
{
    gemObject* targetObject = GetTargetObject();
    // Check if target is set
    if(!targetObject) return -1;

    return targetObject->GetClientID();
}

int Client::GetGuildID()
{
    psCharacter* mychar = GetCharacterData();
    if(mychar == NULL)
        return 0;

    psGuildInfo* guild = mychar->GetGuild();
    if(guild == NULL)
        return 0;

    return guild->GetID();
}

int Client::GetAllianceID()
{
    psCharacter* mychar = GetCharacterData();
    if(mychar == NULL)
        return 0;

    psGuildInfo* guild = mychar->GetGuild();
    if(guild == NULL)
        return 0;

    return guild->GetAllianceID();
}

void Client::GetIPAddress(char* addrStr, socklen_t size)
{
#ifdef INCLUDE_IPV6_SUPPORT
    inet_ntop(AF_INET6, &addr.sin6_addr, addrStr, size);
#else
    unsigned int a1,a2,a3,a4;
#ifdef WIN32
    unsigned long a = addr.sin_addr.S_un.S_addr;
#else
    unsigned long a = addr.sin_addr.s_addr;
#endif

    if(addrStr)
    {
        a1 = a&0x000000FF;
        a2 = (a&0x0000FF00)>>8;
        a3 = (a&0x00FF0000)>>16;
        a4 = (a&0xFF000000)>>24;
        sprintf(addrStr,"%d.%d.%d.%d",a1,a2,a3,a4);
    }
#endif
}

csString Client::GetIPAddress()
{
#ifdef INCLUDE_IPV6_SUPPORT
    char ipaddr[INET6_ADDRSTRLEN] = {0};
    GetIPAddress(ipaddr, INET6_ADDRSTRLEN);
#else
    char ipaddr[INET_ADDRSTRLEN] = {0};
    GetIPAddress(ipaddr, INET_ADDRSTRLEN);
#endif
    return csString(ipaddr);
}


csString Client::GetIPRange(int octets)
{
#ifdef INCLUDE_IPV6_SUPPORT
    char ipaddr[INET6_ADDRSTRLEN] = {0};
    GetIPAddress(ipaddr, INET6_ADDRSTRLEN);
#else
    char ipaddr[INET_ADDRSTRLEN] = {0};
    GetIPAddress(ipaddr, INET_ADDRSTRLEN);
#endif
    return GetIPRange(ipaddr,octets);
}

csString Client::GetIPRange(const char* ipaddr, int octets)
{
    csString range(ipaddr);
    for(size_t i=0; i<range.Length(); i++)
    {
        if(range[i] == '.')
            --octets;

        if(!octets)
        {
            range[i+1] = '\0';
            break;
        }
    }
    return range;
}

unsigned int Client::GetAccountTotalOnlineTime()
{
    unsigned int totalTimeConnected = GetCharacterData()->GetOnlineTimeThisSession();
    Result result(db->Select("SELECT SUM(time_connected_sec) FROM characters WHERE account_id = %d", accountID.Unbox()));
    if(result.IsValid())
        totalTimeConnected += result[0].GetUInt32("SUM(time_connected_sec)");

    return totalTimeConnected;
}

void Client::AddDuelClient(uint32_t clientnum)
{
    if(!IsDuelClient(clientnum))
        duel_clients.Push(clientnum);
}

void Client::RemoveDuelClient(Client* client)
{
    if(actor)
        actor->RemoveAttackerHistory(client->GetActor());
    duel_clients.Delete(client->GetClientNum());
}

void Client::ClearAllDuelClients()
{
    for(int i = 0; i < GetDuelClientCount(); i++)
    {
        Client* duelClient = psserver->GetConnections()->Find(duel_clients[i]);
        if(duelClient)
        {
            // Also remove us from their list.
            duelClient->RemoveDuelClient(this);

            if(actor)
                actor->RemoveAttackerHistory(duelClient->GetActor());
        }
    }
    duel_clients.Empty();
}

int Client::GetDuelClientCount()
{
    return (int)duel_clients.GetSize();
}

int Client::GetDuelClient(int id)
{
    return duel_clients[id];
}

bool Client::IsDuelClient(uint32_t clientnum)
{
    return (duel_clients.Find(clientnum) != csArrayItemNotFound);
}

void Client::AnnounceToDuelClients(gemActor* attacker, const char* event)
{
    for(size_t i = 0; i < duel_clients.GetSize(); i++)
    {
        uint32 duelClientID = duel_clients[i];
        Client* duelClient = psserver->GetConnections()->Find(duelClientID);
        if(duelClient)
        {
            if(!attacker)
                psserver->SendSystemOK(duelClientID, "%s has %s %s!", GetName(), event, duelClient->GetName());
            else if(duelClientID == attacker->GetClientID())
                psserver->SendSystemOK(duelClientID, "You've %s %s!", event, GetName());
            else
                psserver->SendSystemOK(duelClientID, "%s has %s %s!", attacker->GetName(), event, GetName());
        }
    }
}

void Client::FloodControl(uint8_t chatType, const csString &newMessage, const csString &recipient)
{
    int matches = 0;

    floodHistory[nextFloodHistoryIndex] = FloodBuffRow(chatType, newMessage, recipient, csGetTicks());
    nextFloodHistoryIndex = (nextFloodHistoryIndex + 1) % floodMax;

    // Count occurances of this new message in the flood history.
    for(int i = 0; i < floodMax; i++)
    {
        if(csGetTicks() - floodHistory[i].ticks < floodForgiveTime && floodHistory[i].chatType == chatType && floodHistory[i].text == newMessage && floodHistory[i].recipient == recipient)
            matches++;
    }

    if(matches >= floodMax)
    {
        SetMute(true);
        psserver->SendSystemError(clientnum, "BAM! Muted.");
    }
    else if(matches >= floodWarn)
    {
        psserver->SendSystemError(clientnum, "Flood warning. Stop or you will be muted.");
    }
}

FloodBuffRow::FloodBuffRow(uint8_t chtType, csString txt, csString rcpt, unsigned int newticks)
{
    chatType = chtType;
    recipient = rcpt;
    text = txt;
    ticks = newticks;
}

bool Client::IsGM() const
{
    return GetSecurityLevel() >= GM_LEVEL_0;
}

static inline void TestTarget(csString &targetDesc, int32_t targetType,
                              enum TARGET_TYPES type, const char* desc)
{
    if(targetType & type)
    {
        if(targetDesc.Length() > 0)
        {
            targetDesc.Append((targetType > (type * 2)) ? ", " : ", or ");
        }
        targetDesc.Append(desc);
    }
}

void Client::GetTargetTypeName(int32_t targetType, csString &targetDesc)
{
    targetDesc.Clear();
    TestTarget(targetDesc, targetType, TARGET_NONE, "the surrounding area");
    TestTarget(targetDesc, targetType, TARGET_ITEM, "items");
    TestTarget(targetDesc, targetType, TARGET_SELF, "yourself");
    TestTarget(targetDesc, targetType, TARGET_FRIEND, "living friends");
    TestTarget(targetDesc, targetType, TARGET_FOE, "living enemies");
    TestTarget(targetDesc, targetType, TARGET_DEAD, "the dead");
}

bool Client::IsAlive(void) const
{
    return actor ? actor->IsAlive() : true;
}

void Client::SaveAccountData()
{
    // First penalty after relogging should not be death
    if(spamPoints >= 2)
        spamPoints = 1;

    // Save to the db
    db->CommandPump("UPDATE accounts SET spam_points = '%d', advisor_points = '%d' WHERE id = '%d'",
                    spamPoints, advisorPoints, accountID.Unbox());
}

void Client::PathSetIsDisplaying(iSector* sector)
{
    pathDisplaySectors.PushBack(sector);
}

void Client::PathClearDisplaying()
{
    pathDisplaySectors.DeleteAll();
}

csList<iSector*>::Iterator Client::GetPathDisplaying()
{
    return csList<iSector*>::Iterator(pathDisplaySectors);
}

bool Client::PathIsDisplaying()
{
    return !pathDisplaySectors.IsEmpty();
}


void Client::WaypointSetIsDisplaying(iSector* sector)
{
    waypointDisplaySectors.PushBack(sector);
}

void Client::WaypointClearDisplaying()
{
    waypointDisplaySectors.DeleteAll();
}

csList<iSector*>::Iterator Client::GetWaypointDisplaying()
{
    return csList<iSector*>::Iterator(waypointDisplaySectors);
}

bool Client::WaypointIsDisplaying()
{
    return !waypointDisplaySectors.IsEmpty();
}

void Client::LocationSetIsDisplaying(iSector* sector)
{
    locationDisplaySectors.PushBack(sector);
}

void Client::LocationClearDisplaying()
{
    locationDisplaySectors.DeleteAll();
}

csList<iSector*>::Iterator Client::GetLocationDisplaying()
{
    return csList<iSector*>::Iterator(locationDisplaySectors);
}

bool Client::LocationIsDisplaying()
{
    return !locationDisplaySectors.IsEmpty();
}

void Client::SetAdvisorBan(bool ban)
{
    db->Command("UPDATE accounts SET advisor_ban = %d WHERE id = %d", (int) ban, accountID.Unbox());

    if(isAdvisor)
        psserver->GetAdviceManager()->RemoveAdvisor(clientnum, clientnum);

    psserver->SendSystemError(clientnum, "You have been %s from advising by a GM.", ban ? "banned" : "unbanned");
}

bool Client::IsAdvisorBanned()
{
    bool advisorBan = false;
    Result result(db->Select("SELECT advisor_ban FROM accounts WHERE id = %d", accountID.Unbox()));
    if(result.IsValid())
        advisorBan = result[0].GetUInt32("advisor_ban") != 0;

    return advisorBan;
}

void Client::SetCheatMask(CheatFlags mask,bool flag)
{
    if(flag)
        cheatMask |= mask;
    else
        cheatMask &= (~mask);
}

OrderedMessageChannel* Client::GetOrderedMessageChannel(msgtype mtype)
{
    OrderedMessageChannel* channel = orderedMessages.Get(mtype,NULL);

    if(!channel)
    {
        channel = new OrderedMessageChannel;
        orderedMessages.Put(mtype, channel);
    }
    return channel;
}

void MuteBuffable::OnChange() {}

