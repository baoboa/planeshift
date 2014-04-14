/*
 * cmdhandler.h - Author: Keith Fulton
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
#ifndef __CmdHandler_H__
#define __CmdHandler_H__

#include <csutil/ref.h>
#include <csutil/refcount.h>
#include <csutil/list.h>
#include <csutil/csstring.h>
#include <csutil/parray.h>
#include <csutil/redblacktree.h>
#include <iutil/objreg.h>
#include <csutil/redblacktree.h>

#include "net/subscriber.h"
#include "net/cmdbase.h"
#include "util/psstring.h"

class psChatWindow;

/**
 * \addtogroup common_net
 * @{ */

// This little struct tracks who is interested in what.
struct CmdSubscription
{
    /// name of command this listener listens to, like "/say"
    csString cmd;

    /// is this command visible to user in /help command?
    bool visible;

    /// pointer to the subscriber
    iCmdSubscriber *subscriber;
};

class CmdHandler : public csRefCount
{
protected:
    iObjectRegistry*    objreg;
    csPDelArray<CmdSubscription>  subscribers;

public:
    CmdHandler(iObjectRegistry *obj);
    virtual ~CmdHandler();

    enum
    {
    INVISIBLE_TO_USER = 0,
    VISIBLE_TO_USER = 1
    };
    /**
     * Any subclass of iCmdSubscriber can subscribe to typed commands
     * with this function
     */
    bool Subscribe(const char *cmd, iCmdSubscriber *subscriber);

    /** Remove subscriber from list */
    bool Unsubscribe(const char *cmd, iCmdSubscriber *subscriber);

    /** remove subscriber from list for all commands previously subscribed */
    bool UnsubscribeAll(iCmdSubscriber *subscriber);
    
    /// Call all HandleCommand funcs for all subscribed commands
    const char *Publish(const csString & cmd);

    /** Eventually will execute script commands, but now just wrapper to Publish.
      * @param script The full script to run.
      * @param breakSemiColon If true assume that the script is a collection 
                              seperated by ';'.  If false then assume it is 
                              one string.
      */
    void Execute(const char *script, bool breakSemiColon=true);

    /// Return a list (alpha-sorted with unique entries) of the subscribed commands
    void GetSubscribedCommands(csRedBlackTree<psString>& tree);
};

/** @} */

#endif

