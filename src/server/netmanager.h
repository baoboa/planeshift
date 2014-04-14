/*
 * netmanager.h by Matze Braun <matze@braunis.de> and
 *                 Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2001-3 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __NETMANAGER_H__
#define __NETMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/threading/thread.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/netbase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "clients.h"

class CacheManager;

/**
 * \addtogroup server
 * @{ */

/**
 * This is the network thread handling packets sending/receiving to/from network
 * other threads can register a message queue and the network thread will sort
 * messages for them and put them in their queue.
 */
class NetManager : public NetBase
{
public:
    NetManager();
    ~NetManager();

    /**
     * Initialize the network thread.
     *
     * Initializes firstmsg and timeout is the timout in ms for a
     * connection. If no packets are sent or received in this time then the
     * connection is made linkdead.
     *
     * @param cachemanager Pointer to the cachemanager
     * @param client_firstmsg: Is the message type that must be first received from a client
     *     to determine if it is a valid client.
     * @param npcclient_firstmsg: Is the message type that must be first received from a npc client
     *     to determine if it is a valid npc client.
     * @param timeout: Is the timeout value for a connection given in ms. The
     *     default value given is 15000ms.
     * @return Returns success or faliure for initializing.
     */
    bool Initialize(CacheManager* cachemanager, int client_firstmsg, int npcclient_firstmsg, int timeout=15000);

    static NetManager* Create(CacheManager* cacheManager, int client_firstmsg, int npcclient_firstmsg, int timeout=15000);

    static void Destroy();

    /**
     * This broadcasts the same msg out to a bunch of Clients.
     *
     * Which clients recieve this message, depend on the scope.DON'T use this
     * in the client app, it's only here for the MsgHandler class!
     *
     * @param me Is the message to be broadcast to other clients.
     * @param scope Determines which clients will recieve the broadcasted
     *     message. Scope can be one of:
     *     - NetBase::BC_EVERYONEBUTSELF
     *     - NetBase::BC_GROUP
     *     - NetBase::BC_GUILD
     *     - NetBase::BC_SECTOR
     *     - NetBase::BC_EVERYONE
     *     - NetBase::BC_FINALPACKET
     *     .
     * @param guild Is the ID of the guild of the owner of the message. This is
     *     only used when scope is set to BC_GUILD. By default is set to -1 and
     *     is unused.
     * @see MsgEntry
     */
    virtual void Broadcast(MsgEntry* me, int scope,int guild=-1);

    /**
     * Sends the given message me to all the clients in the list.
     *
     * Sends the given message me to all the clients in the list (clientlist)
     * which is of size count. This will send the message to all the clients
     * except the client which has a client number given in the variable
     * except.
     * @note (Brendon): Why is multi not const & ?
     *
     * @param me     Is the message to be sent to other clients.
     * @param multi  Is a vector of all the clients to send this message to.
     * @param except Is a client number for a client NOT to send this message
     *     to. This would usually be the client trying to send the message.
     * @param range  Is the maximum distance the client must be away to be out
     *     of "message reception range".
     */
    virtual void Multicast(MsgEntry* me, const csArray<PublishDestination> &multi, uint32_t except, float range);

    /**
     * Checks for and deletes link dead clients.
     *
     * This is called periodicly to detect linkdead clients. If it finds a
     * link dead client then it will delete the client's connection.
     */
    void CheckLinkDead(void);

    /**
     * Gets a list of all connected clients.
     *
     * @return Returns a pointer to the list of all the clients.
     */
    ClientConnectionSet* GetConnections()
    {
        return &clients;
    }

    /**
     * Gets a client with the specified id.
     *
     * @param cnum The id of the client to retrive.
     * @return Returns a pointer to the specified client or NULL if not found or client not ready.
     */
    Client* GetClient(int cnum);

    /**
     * Gets a client with the specified id.
     *
     * @param cnum The id of the client to retrive.
     * @return Returns a pointer to the specified client or NULL if not found.
     */
    Client* GetAnyClient(int cnum);

    /**
     * Sends the given message to the client listed in the message.
     *
     * MsgEntry has info on who the message is going to.
     *
     * @param me Is a message MsgEntry which contains the message and the
     *     client number to send the message to.
     * @return Returns success or faliure.
     */
    virtual bool SendMessage(MsgEntry* me);

    /**
     * Queues the message for sending later, so the calling classes don't have
     * to all manage this themselves.
     */
    bool SendMessageDelayed(MsgEntry* me, csTicks delay);

    /**
     * This is the main thread function.
     *
     * If it hasn't been initialized it waits until the server is ready to go
     * (i.e. is initialized) if so then it sits in a loop where it calls
     * NetBase::ProcessNetwork() and also checks all the dead links. This
     * method also calculates the data transfer rate.
     */
    void Run();

protected:

    /**
     * Returns a NetManager::Connection from the given IP address.
     *
     * @param addr Is the IP address of client we want to find the connection
     *     for.
     * @return Returns a Connection object that contains the connection for
     *     the given IP address.
     */
    virtual Connection* GetConnByIP(LPSOCKADDR_IN addr);

    /**
     * Returns a NetManager::Connection from the given client number.
     *
     * @param clientnum Is the client number we wish to get the connection
     *     for.
     * @return Returns a Connection object that contains the connection for
     *     the given client number.
     */
    virtual Connection* GetConnByNum(uint32_t clientnum);

    /**
     * Handles/connects a new client from an unknown IP address.
     *
     * This is called when we have recieved a message from a client that we
     * don't know yet. This happens when a new client connects to us. First
     * thing it does is check to make sure the msgtype is equal to firstmsg.
     * If not then we have recieved some random packet. If firstmsg is accepted
     * then it adds this new client to our client list and then changed the
     * client number on the message so an acknowledge can be sent back later.
     *
     * @param addr Is the IP address of the client we have not seen before.
     * @param msg Is the message recieved from the unknown client.
     * @return Returns true if the client has been verified and connected or
     *     false if it is a bad client or failed to connect properly.
     */
    virtual bool HandleUnknownClient(LPSOCKADDR_IN addr, MsgEntry* msg);

private:
    /**
     * This cycles through set of pkts awaiting ack and resends old ones.
     */
    void CheckResendPkts(void);

    /// list of connected clients
    ClientConnectionSet clients;

    /// UDP port the server binds to
    int port;

    /// Max time (ms) between packets before connection is linkdead.
    int timeout;

    /// Type of first msg from client must be this. (e.g. MSGTYPE_AUTHENTICATE)
    uint32_t client_firstmsg;
    /// Type of first msg from npc client must be this. (e.g. MSGTYPE_NPCAUTHENT)
    uint32_t npcclient_firstmsg;

    /// Network will stop when set to true
    bool stop_network;
};

/** @} */

#endif
