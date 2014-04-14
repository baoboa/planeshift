/*
 * subscriber.h
 *
 * Copyright (C) 2001 PlaneShift Team (info@planeshift.it, 
 * http://www.planeshift.it)
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
#ifndef __SUBSCRIBER_H__
#define __SUBSCRIBER_H__

#include <csutil/refcount.h>
#include <csutil/ref.h>

class MsgEntry;
class Client;

/**
 * \addtogroup common_net
 * @{ */

/**
 * This interface must be implemented by objects that want to receive network
 * messages.  Objects must call a provider's Subscribe function to sign up
 * for the msg types they want.
 */
struct iNetSubscriber : public virtual csRefCount
{
    virtual bool Verify(MsgEntry *msg,unsigned int flags,Client*& client) = 0;

    /**
     *  Interprets a received message and executes the command.
     */
    virtual void HandleMessage(MsgEntry* msg,Client *client) = 0;
};

/**
 * This interface must be implemented by objects that want to receive command line strings
 * messages.  Objects must call a provider's Subscribe function to sign up
 * for the "/commands" they want.
 */
struct iCmdSubscriber : public virtual csRefCount
{
    /**
     *  Interprets a received message and executes the command.
     *  char * returned can be an error message to display.
     */
    virtual const char *HandleCommand(const char *cmd) = 0;
};

/** @} */

#endif

