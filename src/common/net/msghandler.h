/*
 * msghandler.h by Matze Braun <MatzeBraun@gmx.de>
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
#ifndef __MSGHANDLER_H__
#define __MSGHANDLER_H__

#include <csutil/parray.h>
#include <csutil/refcount.h>
#include <csutil/threading/thread.h>
#include <csutil/threading/rwmutex.h>

#include "net/message.h"
#include "net/netbase.h"

// forward decls
struct iNetSubscriber;
class NetBase;
class Client;

/**
 * \addtogroup common_net
 * @{ */

//-----------------------------------------------------------------------------


/** @brief Manages a \ref iNetSubscriber watching a certain message type
 *
 * Simple class containing data for subscribes that want to be informed of
 * messages.
 */
struct Subscription
{
    uint32_t flags; /**< Additional flags for detecting if the subscriber should be notified */
    iNetSubscriber* subscriber; /**< The actual subscriber that wants to be notified */

    /**
     * Constructor without a callback
     *
     * @param nSubscriber Sets \ref subscriber
     * @param nFlags      Sets \ref flags
     */
    Subscription(iNetSubscriber *nSubscriber, uint32_t nFlags = 0x01/*REQUIRE_READY_CLIENT*/)
        : flags(nFlags), subscriber(nSubscriber)
    {
    }

    // comparison operator required for usage in csHash
    bool operator<(const Subscription& other) const
    {
        return subscriber < other.subscriber;
    }
};


//-----------------------------------------------------------------------------


/**
 * This class holds the structure for guaranteed inbound ordering of certain message
 * types.  We need to track the next sequence number we're expecting (in a range
 * from 1-127, where 0 means unordered) and to queue up the messages we've received
 * early.  This is used by Client and ClientMsgHandler classes.
 */
struct OrderedMessageChannel
{
    int nextSequenceNumber;
    csRefArray<MsgEntry> pendingMessages;

    OrderedMessageChannel() : nextSequenceNumber(0) { }

    int GetCurrentSequenceNumber() { return nextSequenceNumber; }

    int IncrementSequenceNumber()
    {
        nextSequenceNumber++;
        if (nextSequenceNumber > 63)  // must be clamped to 6 bit value
            nextSequenceNumber = 1;    // can't use zero because that means unsequenced

        return nextSequenceNumber;
    }
};

//-----------------------------------------------------------------------------

/**
 * This class is the client's and server's main interface for either sending
 * network messages out or getting notified about inbound ones which have been
 * received.
 */
class MsgHandler : public csRefCount
{
public:
    MsgHandler();
    virtual ~MsgHandler();

    /** Initializes the Handler */
    bool Initialize(NetBase *nb, int queuelen = 500);

    /** @brief Subscribes an \ref iNetSubscriber to a specific message type
     *
     * Subclasses of \ref iNetSubscriber subscribe to incoming network messages
     * using this function. Adds the resulting \ref Subscription to
     * \ref subscribers.
     * 
     * @param subscriber The subscriber that wants to be informed of messages
     * @param type The type of message to monitor
     * @param flags Additional flags to determine if the message should be forwarded
     */
    virtual void Subscribe(iNetSubscriber *subscriber, msgtype type, uint32_t flags = 0x01/*REQUIRE_READY_CLIENT*/);

    /** @brief Unsubscribes a subscriber from a specific message type
     *
     * If \ref subscribers contains a \ref Subscription that has the specified
     * subscriber and the message type key it is removed and true is returned
     * 
     * @param subscriber The subscriber to look for in the hash
     * @param type The type of message to search
     * @return True if the subscription is found and deleted, false otherwise
     */
    virtual bool Unsubscribe(iNetSubscriber *subscriber , msgtype type);

    /**
     * Searches all message types and deletes any \ref Subscription that has
     * the specified subscriber.
     * @param subscriber The subscriber to search for and remove
     * @return True if at least one Subscription was deleted, false otherwise
     */
    virtual bool UnsubscribeAll(iNetSubscriber *subscriber);

    /// Distribute message to all subscribers
    void Publish(MsgEntry *msg);

    /// import the broadcasttype
    typedef NetBase::broadcasttype broadcasttype;

    /// Allows subscribers to respond with new messages
    virtual void SendMessage(MsgEntry *msg)
    { netbase->SendMessage(msg); }

    /// Send messages to many clients with one func call
    virtual void Broadcast(MsgEntry* msg, broadcasttype scope = NetBase::BC_EVERYONEBUTSELF, int guildID=-1)
    { netbase->Broadcast(msg, scope, guildID); }

    /**
     * Sends the given message me to all the clients in the list.
     *
     * Sends the given message me to all the clients in the list (clientlist)
     * which is of size count. This will send the message to all the clients
     * except the client which has a client number given in the variable
     * except.
     * @note (Brendon) Why is multi not const & ?
     *
     * @param msg     Is the message to be sent to other clients.
     * @param multi  Is a vector of all the clients to send this message to.
     * @param except Is a client number for a client NOT to send this message
     *     to. This would usually be the client trying to send the message.
     * @param range  Is the maximum distance the client must be away to be out
     *     of "message reception range".
     */
    virtual void Multicast(MsgEntry* msg, csArray<PublishDestination>& multi, uint32_t except, float range)
    { netbase->Multicast(msg, multi, except, range); }

    void AddToLocalQueue(MsgEntry *me) { netbase->QueueMessage(me); }

    csTicks GetPing() { return netbase->GetPing(); }

    /// Flush the connected output queue.
    bool Flush() { return netbase->Flush(queue); }

protected:
    NetBase                       *netbase;
    MsgQueue                      *queue;

    /** 
     * @brief Stores the hash of all subscribers and the message type they are subscribed to
     */
    csHash<Subscription, msgtype> subscribers;
    CS::Threading::ReadWriteMutex mutex; /**< @brief Protects \ref subscribers */
};

/** @} */

#endif

