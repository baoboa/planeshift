/*
 * clients.h - Author: Keith Fulton
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
#ifndef __CLIENTS_H__
#define __CLIENTS_H__

#include <csutil/hash.h>
#include <csutil/threading/thread.h>

#include "client.h"

class ClientIterator;
class iResultRow;

class SockAddress
{
public:
    SockAddress(const SOCKADDR_IN &sock);
    bool operator< (const SockAddress &other) const;
private:
    // Define a few operators as privat so that we know
    // that they are not used.
    bool operator> (const SockAddress &other) const;
    bool operator= (const SockAddress &other) const;
    bool operator== (const SockAddress &other) const;

#ifdef INCLUDE_IPV6_SUPPORT
    SOCKADDR_IN addr;
#else
    uint32_t port;
    uint32_t addr;
#endif
};

template<> class csHashComputer<SockAddress> :
    public csHashComputerStruct<SockAddress> {};

/**
 * This class is a list of several CLient objects, it's designed for finding
 * clients very fast based on their clientnum or their IP address.
 * This class is also threadsafe now
 */
class ClientConnectionSet
{
public:
    typedef csHash<Client*, SockAddress> AddressHash;
protected:
    friend class ClientIterator;

    AddressHash addrHash;
    csHash<Client*> hash;
    csPDelArray<Client> toDelete;
    CS::Threading::RecursiveMutex mutex;

public:
    ClientConnectionSet();
    ~ClientConnectionSet();

    bool Initialize();

    Client* Add(LPSOCKADDR_IN addr);

    // Delete all clients marked to be deleted
    void SweepDelete();

    // Mark this client as ready to be deleted
    void MarkDelete(Client* client);

    /// Count the number of clients connected, including superclients
    size_t Count(void) const;

    /// Count the number of ready players connected
    size_t CountReadyPlayers() const;

    ///  Find by 32bit id value, used in UDP messages, returns ready or not.
    Client* FindAny(uint32_t id);
    ///  Find by 32bit id value, used in UDP messages.  Returns NULL if found but not ready.
    Client* Find(uint32_t id);
    /// Find by player name, used for Chat and other purposes. Returns NULL if found but not ready.
    Client* Find(const char* name);
    /// Find by player id
    Client* FindPlayer(PID playerID);
    /// Find by account id
    Client* FindAccount(AccountID accountID, uint32_t excludeClient = 0);
    /// Find by IP addr and Port
    Client* Find(LPSOCKADDR_IN addr);

    csRef<NetPacketQueueRefCount> FindQueueAny(uint32_t id);
};

class ClientIterator : public ClientConnectionSet::AddressHash::GlobalIterator
{
public:
    ClientIterator(ClientConnectionSet &clients);
    ~ClientIterator();

private:

    /// This is a pointer to the mutex in the ClientConnectionSet class
    CS::Threading::RecursiveMutex &mutex;
};


#endif
