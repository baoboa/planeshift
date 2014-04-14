/*
 * msghandler.cpp
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
 */
#include <psconfig.h>

#include "net/message.h"
#include "net/messages.h"
#include "net/netbase.h"
#include "net/msghandler.h"
#include "csutil/scopedlock.h"

#include "net/subscriber.h"
#include "util/psconst.h"

class Client;

MsgHandler::MsgHandler()
{
    netbase = NULL;
    queue   = NULL;
}

MsgHandler::~MsgHandler()
{   
    if (queue)
        delete queue;
}

bool MsgHandler::Initialize(NetBase* nb, int queuelen)
{
    if (!nb) return false;
    netbase = nb;

    queue = new MsgQueue(queuelen);
    if (!netbase->AddMsgQueue(queue))
        return false;

    return true;
}

void MsgHandler::Publish(MsgEntry* me)
{
    const int mtype = me->GetType();
    bool handled = false;
    netbase->LogMessages('R',me);

    csArray<Subscription> handlers;
    {
        CS::Threading::ScopedReadLock lock(mutex);
        handlers = subscribers.GetAll(mtype);
    }

    for(size_t i = 0; i < handlers.GetSize(); ++i)
    {
        Subscription& sub = handlers[i];
        Client *client;
        me->Reset();
        // Copy the reference so we can modify it in the loop
        MsgEntry *message = me;
        if(sub.subscriber->Verify(message, sub.flags, client))
        {
            sub.subscriber->HandleMessage(message, client);
        }
        handled = true;
    }

    if (!handled)
    {
        Debug4(LOG_ANY,me->clientnum,"Unhandled message received 0x%04X(%d) from %d",
               me->GetType(), me->GetType(), me->clientnum);
    }
}

void MsgHandler::Subscribe(iNetSubscriber* subscriber, msgtype type, uint32_t flags)
{
    CS_ASSERT(subscriber);

    CS::Threading::ScopedWriteLock lock(mutex);
    subscribers.Delete(type, subscriber);
    subscribers.Put(type, Subscription(subscriber, flags));
}

bool MsgHandler::Unsubscribe(iNetSubscriber* subscriber, msgtype type)
{
    CS::Threading::ScopedWriteLock lock(mutex);
    return subscribers.Delete(type, subscriber);
}

bool MsgHandler::UnsubscribeAll(iNetSubscriber *subscriber)
{
    CS::Threading::ScopedWriteLock lock(mutex);
    bool found = false;

    csHash<Subscription, msgtype>::GlobalIterator iter = subscribers.GetIterator();
    while(iter.HasNext())
    {
        if(iter.Next().subscriber == subscriber)
        {
            subscribers.DeleteElement(iter);
            found = true;
        }
    }
    return found;
}

