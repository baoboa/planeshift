/*
 * ClientMsgHandler.h by Keith Fulton <keith@paqrat.com>
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
#ifndef __CLNTMSGHANDLER_H__
#define __CLNTMSGHANDLER_H__

#include <csutil/scf.h>

#include "net/msghandler.h"
#include "util/genericevent.h"

class psNetConnection;
class NetBase;
struct iObjectRegistry;

/**
 * \addtogroup common_net
 * @{ */

class ClientMsgHandler : public MsgHandler
{
public:
    ClientMsgHandler();
    virtual ~ClientMsgHandler();

    /** Initializes the Handler */
    bool Initialize(NetBase *nb, iObjectRegistry* object_reg);

    /**
     * This is a conveniance function that basically calls NetBase::Connect.
     */
    bool Connect(const char* server, int port);

    /**
     * Send outgoing messages, setting the clientnum automatically
     * beforehand.
     */
    void SendMessage(MsgEntry* msg) { netbase->SendMessage(msg); }

    /** 
     * This function implementes the iEventHandler interface and is called
     * whenever a subscribed event occurs.  We use this to trap idle events
     * and process the incoming queue during those times instead of running
     * a separate thread.
     */
    bool HandleEvent(iEvent &Event);

    /**
     * This function Publish all messages currently in the inbound message queue
     */
    bool DispatchQueue();

    /// Get the next sequence number to use for an ordered message
    int GetNextSequenceNumber(msgtype mtype);


    /// Declare our event handler
    DeclareGenericEventHandler(EventHandler,ClientMsgHandler,"planeshift.clientmsghandler");
    csRef<EventHandler> scfiEventHandler;

protected:
    iObjectRegistry* object_reg;
    csHash<OrderedMessageChannel*> orderedMessages;
};

/** @} */

#endif
