/*
 * clients.cpp - Author: Keith Fulton
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

#include "psserver.h"
#include "playergroup.h"
#include "globals.h"
#include "clients.h"
#include "util/psconst.h"
#include "util/log.h"
#include "netmanager.h"


SockAddress::SockAddress(const SOCKADDR_IN &sock)
{
#ifdef INCLUDE_IPV6_SUPPORT
    addr = sock;
#else
    port = sock.sin_port;
    addr = sock.sin_addr.s_addr;
#endif
}

bool SockAddress::operator< (const SockAddress &other) const
{
#ifdef INCLUDE_IPV6_SUPPORT
    int result = memcmp(&addr.sin6_addr, &other.addr.sin6_addr, 16);
    if(result < 0)
        return true;
    else if(result > 0)
        return false;
    return addr.sin6_port < other.addr.sin6_port;
#else
    if(addr < other.addr)
        return true;
    else if(addr > other.addr)
        return false;
    return port < other.port;
#endif
}


ClientConnectionSet::ClientConnectionSet():addrHash(307),hash(307)
{
}

ClientConnectionSet::~ClientConnectionSet()
{
    AddressHash::GlobalIterator it(addrHash.GetIterator());
    Client* p = NULL;
    while(it.HasNext())
    {
        p = it.Next();
        delete p;
    }
}

bool ClientConnectionSet::Initialize()
{
    return true;
}

Client* ClientConnectionSet::Add(LPSOCKADDR_IN addr)
{
    int newclientnum;

    // Get a random uniq client number
    Client* testclient;
    do
    {
        newclientnum = psserver->rng->Get(0x8fffff); //make clientnum random
        testclient = FindAny(newclientnum);
    }
    while(testclient != NULL);

    // Have uniq client number, create the new client
    Client* client = new Client();
    if(!client->Initialize(addr, newclientnum))
    {
        Bug1("Client Init failed?!?\n");
        delete client;
        return NULL;
    }

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    addrHash.PutUnique(SockAddress(client->GetAddress()), client);
    hash.Put(client->GetClientNum(), client);
    return client;
}

void ClientConnectionSet::MarkDelete(Client* client)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    uint32_t clientid = client->GetClientNum();
    if(!addrHash.DeleteAll(client->GetAddress()))
        Bug2("Couldn't delete client %d, it was never added!", clientid);

    hash.DeleteAll(clientid);
    toDelete.Push(client);
}

void ClientConnectionSet::SweepDelete()
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    toDelete.Empty();
}

size_t ClientConnectionSet::Count() const
{
    return addrHash.GetSize();
}

size_t ClientConnectionSet::CountReadyPlayers() const
{
    size_t count = 0;

    // Need to lock even if we are const
    ClientConnectionSet* ccs = const_cast<ClientConnectionSet*>(this);
    CS::Threading::RecursiveMutexScopedLock lock(ccs->mutex);

    AddressHash::ConstGlobalIterator it(addrHash.GetIterator());
    while(it.HasNext())
    {
        Client* p = it.Next();

        if(p->IsReady() && p->IsPlayerClient())
        {
            count++;
        }

    }

    return count;
}



Client* ClientConnectionSet::FindAny(uint32_t clientnum)
{
    if(clientnum==0)
        return NULL;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    return hash.Get(clientnum, 0);
}

Client* ClientConnectionSet::Find(uint32_t clientnum)
{
    if(clientnum==0)
        return NULL;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    Client* temp = hash.Get(clientnum, 0);

    if(temp && temp->IsReady())
        return temp;
    else
        return NULL;
}

Client* ClientConnectionSet::Find(const char* name)
{
    if(!name)
    {
        Error1("name == 0!");
        return NULL;
    }

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    AddressHash::GlobalIterator it(addrHash.GetIterator());
    Client* p = NULL;
    while(it.HasNext())
    {
        p = it.Next();
        if(!p->GetName())
            continue;

        if(!strcasecmp(p->GetName(), name))
            break;
        p = NULL;
    }

    if(p && p->IsReady())
        return p;
    else
        return NULL;
}

Client* ClientConnectionSet::FindPlayer(PID playerID)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    AddressHash::GlobalIterator it(addrHash.GetIterator());

    while(it.HasNext())
    {
        Client* p = it.Next();
        if(p->GetPID() == playerID)
            return p;
    }

    return NULL;
}

Client* ClientConnectionSet::FindAccount(AccountID accountID, uint32_t excludeClient)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    AddressHash::GlobalIterator it(addrHash.GetIterator());

    while(it.HasNext())
    {
        Client* p = it.Next();
        if(p->GetAccountID() == accountID && p->GetClientNum() != excludeClient)
            return p;
    }

    return NULL;
}

Client* ClientConnectionSet::Find(LPSOCKADDR_IN addr)
{
    CS::Threading::RecursiveMutexScopedLock lock(mutex);

    return addrHash.Get(SockAddress(*addr), NULL);
}

csRef<NetPacketQueueRefCount> ClientConnectionSet::FindQueueAny(uint32_t clientnum)
{
    if(clientnum==0)
        return NULL;

    CS::Threading::RecursiveMutexScopedLock lock(mutex);
    Client* client = hash.Get(clientnum, 0);
    if(client)
        return client->outqueue;
    else
        return NULL;
}

ClientIterator::ClientIterator(ClientConnectionSet &clients)
    : ClientConnectionSet::AddressHash::GlobalIterator(clients.addrHash.GetIterator()), mutex(clients.mutex)
{
    mutex.Lock();
}

ClientIterator::~ClientIterator()
{
    mutex.Unlock();
}
