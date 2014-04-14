/*
 * connection.h by Matze Braun <matze@braunis.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <csutil/threading/thread.h>

#include "net/netbase.h"

struct iObjectRegistry;

/**
 * \addtogroup common_net
 * @{ */

/**
 * Client-side UDP handler.
 *
 * Implementation of client side network connection on top of the common network
 * code in psnet. For details about the functions look at connection.h and
 * netbase.h.  This class differs from NetManager in that it only has a single 
 * connection (to the server) as opposed to NetManager which is managing many 
 * client connections at once.  This means several functions are overridden
 * as empty functions because they aren't relevant with a single connection.
 */
class psNetConnection : public NetBase, public CS::Threading::Runnable
{
public:
    /// Creates a queue of messages queueLength wide.
    psNetConnection(int queueLength = 100);
    /// Disconnects and terminates the thread.
    virtual ~psNetConnection();

    /// Just some basic initialization for the class.
    bool Initialize(iObjectRegistry* object_reg);

    /** Connects to the server and starts the thread to poll it.
     *
     * Opens a non-blocking UDP socket to the specified server on the
     * specified port, and kicks off the thread which will handle sending
     * and receiving on this socket.
     *
     * @param server: The text hostname or IP address of the destination
     * @param port: The port number to try on the server.
     * @return True if successful connection is made, false if error.
     */
    bool Connect(const char *server, int port);

    /** Kills the thread and closes the socket.
     */
    void DisConnect();

    /// Broadcasting makes no sense in a single connection class.  Just wraps SendMessage.
    virtual void Broadcast (MsgEntry* me, int scope, int guildID);

    /// Multicasting makes no sense in a single connection class.  Just wraps SendMessage.
    virtual void Multicast (MsgEntry* me, const csArray<PublishDestination>& multi, uint32_t except, float range);

    /// Necessary CS definition and stub function
    virtual void IncRef ()
    {}
    /// Necessary CS definition and stub function
    virtual void DecRef ()
    {}

    virtual int GetRefCount() { return 1; }

protected:
    /// Returns the same address back if the address is correct.  NULL if not.
    virtual Connection *GetConnByIP (LPSOCKADDR_IN addr);
    /// Returns the server connection if the clientnum is 0.
    virtual Connection *GetConnByNum (uint32_t clientnum);
    /// Another overridden function only used by multi-connection class NetManager.
    virtual bool HandleUnknownClient (LPSOCKADDR_IN addr, MsgEntry *data);

    /** This checks the time since the last packet received from the server.
     *
     * This function tracks how long it has been since the last packet was
     * received from the server.  If it has been longer than LINKDEAD_TIMEOUT
     * msec, then it attempts to send a packet to the server just to get a
     * response.  If this goes longer than 5 retries, it kills the client.
     */
    void CheckLinkDead (csTicks time);

    /**
     * This is the inbound queue to the client.  This class adds messages to
     * this queue, and ClientMsgHandler pulls them out of this queue.
     * @see ClientMsgHandler.
     */
    MsgQueue *inQueue;
    
    /// Connection class holds the IP address and port we are using here.
    Connection *server;
    iObjectRegistry* object_reg;
    
    /// When this is false, thread will finish it's last loop and exit
    bool shouldRun; 
    
    /** this is the main thread function... */
    void Run ();
   
private:
    /// This csThread handles the details of spawning and running the thread.
    csRef<CS::Threading::Thread> thread;
};
    
/** @} */

#endif
