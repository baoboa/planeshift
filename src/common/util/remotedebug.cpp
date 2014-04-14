/*
 * remotedebug.cpp by Anders Reggestad <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

//=============================================================================
// OS Includes
//=============================================================================
#include <stdarg.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "util/log.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "remotedebug.h"


RemoteDebug::RemoteDebug()
    :localDebugLevel(0),highestActiveDebugLevel(0)
{
    for(int i = 0; i < 10; i++)
    {
        debugLog.Push(csString(""));
    }
    nextDebugLogEntry = 0;
}

RemoteDebug::~RemoteDebug()
{
    
}

bool RemoteDebug::SwitchDebugging()
{
    localDebugLevel = !localDebugLevel;
    return localDebugLevel > 0;
}

void RemoteDebug::SetDebugging(int debugLevel)
{
    localDebugLevel = debugLevel;

    CalculateHighestActiveDebugLevel();
}

int RemoteDebug::GetDebugging() const
{
    return localDebugLevel;
}

void RemoteDebug::AddDebugClient(uint clientNum, int debugLevel)
{
    if (debugLevel == 0)
    {
        RemoveDebugClient(clientNum);
    }
    else
    {
        debugClients.PushSmart(DebugClient(clientNum, debugLevel));
    }

    CalculateHighestActiveDebugLevel();
}

void RemoteDebug::RemoveDebugClient(uint clientnum)
{
    debugClients.Delete(DebugClient(clientnum, 0));

    CalculateHighestActiveDebugLevel();
}

csString RemoteDebug::GetRemoteDebugClientsString() const
{
    csString clients;
    
    for(size_t i = 0; i < debugClients.GetSize(); i++)
    {
        clients.AppendFmt("%s%u(%d)",i?" ":"",debugClients[i].clientNum, debugClients[i].debugLevel);
    }
    return clients;
}

void RemoteDebug::CalculateHighestActiveDebugLevel()
{
    highestActiveDebugLevel = localDebugLevel;
    for(size_t i = 0; i < debugClients.GetSize(); i++)
    {
        if (debugClients[i].debugLevel > highestActiveDebugLevel)
        {
            highestActiveDebugLevel = debugClients[i].debugLevel;
        }
    }
}


void RemoteDebug::Printf(int debugLevel, const char* msg,...)
{
    va_list args;

    if(!IsDebugging())
        return;

    va_start(args, msg);
    VPrintf(debugLevel,msg,args);
    va_end(args);
}

void RemoteDebug::VPrintf(int debugLevel, const char* msg, va_list args)
{
    CS_ASSERT(msg != NULL);

    const int REMOTE_DEBUG_BUFFER = 1024;
    
    char str[REMOTE_DEBUG_BUFFER];

    if(!IsDebugging())
        return;

    int ret = vsnprintf(str, REMOTE_DEBUG_BUFFER, msg, args);
    if (ret >= REMOTE_DEBUG_BUFFER)
    {
        // Buffer truncated. Terminate buffer and insert ... at end.
        str[REMOTE_DEBUG_BUFFER-1] = '\0';
        str[REMOTE_DEBUG_BUFFER-2] = '.';
        str[REMOTE_DEBUG_BUFFER-3] = '.';
        str[REMOTE_DEBUG_BUFFER-4] = '.';
    }
    

    // Add string to the internal log buffer
    debugLog[nextDebugLogEntry] = str;
    nextDebugLogEntry = (nextDebugLogEntry+1)%debugLog.GetSize();

    if(!IsDebugging(debugLevel))
        return;

    if (debugLevel <= localDebugLevel)
    {
        LocalDebugReport(str);
    }

    for(size_t i = 0; i < debugClients.GetSize(); i++)
    {
        if (debugLevel <= debugClients[i].debugLevel)
        {
            RemoteDebugReport(debugClients[i].clientNum, str);
        }
    }    
}

