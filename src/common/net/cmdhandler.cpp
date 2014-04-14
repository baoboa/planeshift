/*
 * cmdhandler.cpp - Author: Keith Fulton
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
#include <psconfig.h>

#include <csutil/util.h>

#include "util/pserror.h"
#include "util/psstring.h"
#include "net/clientmsghandler.h"
#include "cmdhandler.h"


CmdHandler::CmdHandler(iObjectRegistry *newobjreg)
{
    objreg = newobjreg;
}

CmdHandler::~CmdHandler()
{
}

bool CmdHandler::Subscribe(const char *cmd, iCmdSubscriber *subscriber)
{    
    for ( size_t x = 0; x < subscribers.GetSize(); x++ )
    {
        if (subscribers[x]->cmd == cmd && subscribers[x]->subscriber == subscriber)
        {
            // Already subscribed => Do nothing
            return true; // No error so return true.
        }
    }

    // Create a new subscribtion
    CmdSubscription* p = new CmdSubscription;

    p->subscriber = subscriber;
    p->cmd        = cmd;

    subscribers.Push(p);

    return true;
}

bool CmdHandler::Unsubscribe(const char *cmd, iCmdSubscriber *subscriber)
{
    for ( size_t x = 0; x < subscribers.GetSize(); x++ )
    {
        if (subscribers[x]->cmd == cmd && subscribers[x]->subscriber == subscriber)
        {
            subscribers.DeleteIndex(x);
            return true;            
        }
    }
    
    return false;
}

bool CmdHandler::UnsubscribeAll(iCmdSubscriber *subscriber)
{
    bool unsubscribed_commands(false);
    for (size_t x = subscribers.GetSize() - 1; x != (size_t)-1; x--)
    {
        if (subscribers[x]->subscriber == subscriber)
        {
            subscribers.DeleteIndex(x);
            unsubscribed_commands = true;
        }
    }
    return unsubscribed_commands;
}

const char *CmdHandler::Publish(const csString & cmd)
{
    if (subscribers.GetSize() == 0)
        return "No command handlers installed!";

    csString fullLine = cmd;
    csString command;
    // Get first word of line if there is a space, or get entire line if one word
    {
        size_t spacePosition = cmd.FindFirst(' ');
        if(spacePosition != SIZET_NOT_FOUND)
        {
            command = fullLine.Slice(0, spacePosition);
        }
        else
        {
            command = fullLine;
        }
    }
    // Ensure the command is lower case
    {
        csString commandLower = command;
        if(commandLower.Downcase() != command)
        {
            // Splice the rest of the command in after the downcase command
            // so that we don't replace the command in the text
            // e.g.  /Say /Say is preserved as /say /Say
            fullLine = commandLower + fullLine.Slice(commandLower.Length());
            command = commandLower;
        }
    }
    
    bool found = false;
    const char *err = NULL;
    for( size_t x = 0; x < subscribers.GetSize(); x++ )
    {
        if(subscribers[x]->cmd == command)
        {
            found = true;
            const char* ret = subscribers[x]->subscriber->HandleCommand(fullLine);
            if(ret)
                err = ret;
        }        
    }

    // Only last error returned will be shown.
    // This should be ok since most commands will only have one subscriber.
    return found ? err : "Sorry.  Unknown command (use /show help).";
}

void CmdHandler::Execute(const char *script, bool breakSemiColon)
{
    if (!script || !strlen(script) )
        return;

    /**
     * script is assumed to be null terminated
     * with each command on a new line OR separated by semicolon.
     */
    psString copy(script),linefeed("\n"),semicolon(";"),line;

    size_t start=0;
    size_t end=0;
    while ( end < copy.Length() )
    {
        end = copy.FindSubString(linefeed,(int) start);
        if (end == SIZET_NOT_FOUND)
        {
            if ( breakSemiColon )
            {
                end = copy.FindSubString(semicolon,(int)start);
                if (end == SIZET_NOT_FOUND)
                    end = copy.Length();
            }
            else
            {
                end = copy.Length();
            }                
        }
        copy.GetSubString(line,start,end);
        line.LTrim();
        Publish(line); // This executes each line
        start = end+1;
    }
}

void CmdHandler::GetSubscribedCommands(csRedBlackTree<psString>& tree)
{
    //we empty the list first
    tree.DeleteAll();

    for ( size_t x = 0; x < subscribers.GetSize(); x++ )
    {
        tree.Insert(subscribers[x]->cmd);        
    }
}
