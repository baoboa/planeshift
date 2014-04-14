/*
 * cmdbase.h - Author: Keith Fulton
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

/**
 * This implements the base functionality for classes which handle command lines.
 * It is assumed that most, if not all of these classes will need to send messages
 * to the server and display text on the in-game console.
 */
#ifndef __CMDBASE_H__
#define __CMDBASE_H__

#include "net/subscriber.h"
#include "net/messages.h"
#include <iutil/objreg.h>

#include <ivaria/reporter.h>

class  CmdHandler;  // needed for circular includes
class  ClientMsgHandler;
class  Client;  // dummy blind definition.  Only used on server.

/**
 * \addtogroup common_net
 * @{ */

class psCmdBase : public iNetSubscriber, public iCmdSubscriber
{
protected:
    ClientMsgHandler   *msgqueue;
    CmdHandler         *cmdsource;
    iObjectRegistry    *objreg;

public:
    psCmdBase(ClientMsgHandler *mh, CmdHandler *ch, iObjectRegistry* obj);
    
    virtual ~psCmdBase() {}
    virtual bool Setup(ClientMsgHandler* mh, CmdHandler* ch);

    // iCmdSubscriber interface
    virtual const char *HandleCommand(const char *cmd) = 0;

    // iNetSubscriber interface
    virtual void HandleMessage(MsgEntry *msg,Client*) { HandleMessage(msg); }
    virtual void HandleMessage(MsgEntry *msg) = 0;
    virtual bool Verify(MsgEntry *,unsigned int,Client *& ) { return true; }

    // Wrapper for csReport
    void Report(int severity, const char* msgtype, const char* description, ... );
};

class psClientNetSubscriber : public iNetSubscriber
{
public:
    virtual ~psClientNetSubscriber() {}

    // iNetSubscriber interface
    virtual void HandleMessage(MsgEntry *msg,Client*) { HandleMessage(msg); }
    virtual void HandleMessage(MsgEntry *msg) = 0;
    virtual bool Verify(MsgEntry *,unsigned int,Client *& ) { return true; }

};

/** @} */

#endif

