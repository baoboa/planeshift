/*
 * msgmanager.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __MSGMANAGER_H__
#define __MSGMANAGER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/subscriber.h"         // Subscriber class
#include "net/message.h"            // For msgtype typedef
#include "util/eventmanager.h"
#include "globals.h"

//=============================================================================
// Local Includes
//=============================================================================

class Client;
class gemObject;
class gemActor;
class MsgEntry;
class GEMSupervisor;

/**
 * \addtogroup server
 * @{ */

/// These flags define the tests that are centrally done
/// before subclasses get the message.
#define NO_VALIDATION                0x00
#define REQUIRE_ANY_CLIENT           0x01
#define REQUIRE_READY_CLIENT         0x02
#define REQUIRE_ALREADY_READY_CLIENT 0x04
#define REQUIRE_ACTOR                0x08
#define REQUIRE_ALIVE                0x10
#define REQUIRE_TARGET               0x20
#define REQUIRE_TARGETACTOR          0x40
#define REQUIRE_TARGETNPC            0x80


/**
 * Base server-side class for subscriptions.
 *
 * Contains non-templated functions for classes that need to be able to subscribe
 * to messages.
 */
class MessageManagerBase : public iNetSubscriber
{
public:

    virtual bool Verify(MsgEntry* pMsg, unsigned int flags, Client* &client);

    /** Finds Client* of character with given name. */
    Client* FindPlayerClient(const char* name);

    /**
     * Decodes an area: expression.
     *
     *  @param client The client of the caller
     *  @param target The area: expression
     */
    csArray<csString> DecodeCommandArea(Client* client, csString target);

    /**
     * Find the object we are referring to in str.
     *
     * This str can have different formats, depending on the object
     * we are trying to get.
     *
     * @param str the field containing the reference to the object
     * @param me the client's actor who is sending the command
     */
    gemObject* FindObjectByString(const csString &str, gemActor* me) const;
};

/**
 * Provides a manager to facilitate subscriptions.
 *
 * Any server-side class that needs to be informed of incoming messages should
 * derive from this class. To use, simply inherit from this class with the
 * template name being your class name.
 */
template <class SubClass>
class MessageManager : public MessageManagerBase
{
public:
    typedef void(SubClass::*FunctionPointer)(MsgEntry*, Client*);

    /** @brief Unsubscribes all messages then destroys this object */
    virtual ~MessageManager()
    {
        UnsubscribeAll();
    }

    /**
     * Subscribes this manager to a specific message type with a custom callback.
     *
     * Any time a message with the specified type (and flags are met) is
     * received the specified function is called
     * @param fpt The function to call
     * @param type The type of message to be notified of
     * @param flags to check <i>Default: 0x01</i>
     */
    inline void Subscribe(FunctionPointer fpt, msgtype type, uint32_t flags = 0x01)
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        if(!handlers.Contains(type))
        {
            GetEventManager()->Subscribe(this, type, flags);
        }
        handlers.Put(type, fpt);
    }

    /**
     * Unsubscribes this manager from a specific message type.
     *
     * @param type The type of message to unsubscribe from
     * @return True if a subscription was removed, false if this was not subscribed
     */
    inline bool Unsubscribe(msgtype type)
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        if(handlers.Contains(type))
        {
            handlers.DeleteAll(type);
            return GetEventManager()->Unsubscribe(this, type);
        }
        return false;
    }

    /**
     * Unsubscribes a specific handler from a specific message type.
     *
     * @param handler The handler to unsubscribe
     * @param type The type of message to unsubscribe from
     * @return True if a subscription was removed, false if this was not subscribed
     */
    inline bool Unsubscribe(FunctionPointer handler, msgtype type)
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        if(handlers.Contains(type))
        {
            handlers.Delete(type, handler);
            if(!handlers.Contains(type))
            {
                return GetEventManager()->Unsubscribe(this, type);
            }
            else
            {
                return true;
            }
        }
        else
        {
            return false;
        }
    }

    /**
     * Unsubscribes this manager from all message types.
     *
     * @return True if a subscription was removed, false if this was not subscribed
     */
    inline bool UnsubscribeAll()
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        handlers.Empty();
        if(psserver->GetEventManager())
            return GetEventManager()->UnsubscribeAll(this);
        return false;
    }

    /**
     * Transfers the message to the manager specific function.
     *
     * @note DO NOT OVERRIDE
     * @param msg Message that is forwarded to the manager's function
     * @param client Client that is forwarded to the manager's function
     */
    void HandleMessage(MsgEntry* msg, Client* client)
    {
        csArray<FunctionPointer> msgHandlers;
        {
            CS::Threading::RecursiveMutexScopedLock lock(mutex);
            msgHandlers = handlers.GetAll(msg->GetType());
        }
        SubClass* self = dynamic_cast<SubClass*>(this);
        if(!self)
        {
            // fatal error, we aren't a base of our subclass!
            return;
        }

        for(size_t i = 0; i < msgHandlers.GetSize(); ++i)
        {
            (self->*msgHandlers[i])(msg, client);
        }
    }

private:
    CS::Threading::RecursiveMutex mutex;
    csHash<FunctionPointer, msgtype> handlers;

    /**
     * Gets the event manager from the server.
     *
     * Asserts if the retrieved manager is valid
     *
     * @return The event manager
     */
    inline EventManager* GetEventManager() const
    {
        EventManager* eventManager = psserver->GetEventManager();
        CS_ASSERT(eventManager);
        return eventManager;
    }
};

/** @} */

#endif
