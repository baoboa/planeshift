/*
 * psnetmanager.cpp by Matze Braun <MatzeBraun@gmx.de>
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

#include <iutil/objreg.h>
#include <iutil/eventq.h>

#include "psnetmanager.h"
#include "net/clientmsghandler.h"
#include "authentclient.h"
#include "net/connection.h"
#include "net/cmdhandler.h"
#include "cmdusers.h"
#include "cmdguilds.h"
#include "cmdgroups.h"
#include "cmdutil.h"
#include "cmdadmin.h"
#include "paws/pawsmanager.h"
#include "globals.h"

psNetManager::psNetManager()
{
    connected    = false;
    connection   = NULL;
}

psNetManager::~psNetManager()
{
    if (connected)
        Disconnect();
    if (connection)
        delete connection;
}

bool psNetManager::Initialize( iObjectRegistry* newobjreg )
{
    object_reg = newobjreg;
    
    connection = new psNetConnection;
    if (!connection->Initialize(object_reg))
        return false;

    connection->SetEngine(psengine->GetEngine());

    msghandler.AttachNew(new ClientMsgHandler);
    bool rc = msghandler->Initialize((NetBase*) connection, object_reg);
    if (!rc)
        return false;

    cmdhandler.AttachNew(new CmdHandler(object_reg));

    usercmds.AttachNew(new psUserCommands(msghandler, cmdhandler, object_reg));
    if (!usercmds->LoadEmotes())
    {
        return false;
    }
    
    guildcmds.AttachNew(new psGuildCommands(msghandler, cmdhandler, object_reg));
    groupcmds.AttachNew(new psGroupCommands(msghandler, cmdhandler, object_reg));
    
    utilcmds.AttachNew(new psUtilityCommands(msghandler, cmdhandler, object_reg));
    admincmds.AttachNew(new psAdminCommands(msghandler, cmdhandler, object_reg));

    authclient.AttachNew(new psAuthenticationClient(GetMsgHandler()));

    return true;
}

ClientMsgHandler* psNetManager::GetMsgHandler()
{
    return msghandler;
}

const char* psNetManager::GetAuthMessage()
{
    return authclient->GetRejectMessage();
}

bool psNetManager::Connect(const char* server, int port)
{
    if (connected)
        Disconnect();

    if (!connection->Connect(server,port))
    {
        errormsg = PawsManager::GetSingleton().Translate("Couldn't resolve server hostname.");
        return false;
    }

    connected = true;
    return true;
}


void psNetManager::Disconnect()
{
    if (!connected)
        return;

    connected = false;

    psDisconnectMessage discon(0, 0, "");
    msghandler->SendMessage(discon.msg);
    msghandler->Flush(); // Flush the network
    //msghandler->DispatchQueue(); // Flush inbound message queue
   
    connection->DisConnect();
}

void psNetManager::Authenticate(const csString & name, const csString & pwd, const  csString & pwd256)
{
    authclient->Authenticate(name, pwd, pwd256);
}
