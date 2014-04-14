/*
 * cmdbase.cpp - Author: Keith Fulton
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
 *
 * This implements the base functionality for classes which handle command lines.
 * It is assumed that most, if not all of these classes will need to send messages
 * to the server and display text on the in-game console.
 */
#include <psconfig.h>

#include "net/cmdbase.h"
#include "net/clientmsghandler.h"

psCmdBase::psCmdBase(ClientMsgHandler *mh,CmdHandler *ch,iObjectRegistry* obj)
{
    msgqueue  = mh;
    cmdsource = ch;
    objreg    = obj;
}

bool psCmdBase::Setup(ClientMsgHandler *mh,CmdHandler *ch)
{
    msgqueue  = mh;
    cmdsource = ch;

    return true;
}

void psCmdBase::Report(int severity, const char* msgtype, const char* description, ... )
{
    char str[1024];

    va_list args;
    va_start (args, description);

    vsnprintf(str,1024,description,args);
    str[1023]=0x00;
    csReport(objreg,severity,msgtype,"%s",str);

    va_end (args);
}
