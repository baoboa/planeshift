/*
 * cmdgroups.cpp by Anders Reggestad <andersr@pvv.org>
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
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

#include "gui/pawsgroupwindow.h"

#include "paws/pawsmanager.h"
#include "paws/pawsyesnobox.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "cmdgroups.h"
#include "globals.h"

psGroupCommands::psGroupCommands(ClientMsgHandler* mh,
                                 CmdHandler* ch,
                                 iObjectRegistry* obj)
  : psCmdBase(mh,ch,obj)
{
    cmdsource->Subscribe("/invite",this);        // ask player to join group
    cmdsource->Subscribe("/disband",this);       // disband group (leader only)
    cmdsource->Subscribe("/groupremove",this);   // remove player from group (leader only)
    cmdsource->Subscribe("/leavegroup",this);    // name group level something (ranks)
    cmdsource->Subscribe("/groupmembers",this);  // see list of members (optional level #)
    cmdsource->Subscribe("/groupchallenge",this);// challenges another group (leader only)
    cmdsource->Subscribe("/groupyield",this);    //yields to all the groups in challenge (leader only)
}

psGroupCommands::~psGroupCommands()
{
    cmdsource->Unsubscribe("/invite",this);
    cmdsource->Unsubscribe("/disband",this);     
    cmdsource->Unsubscribe("/groupremove",this);  // remove player from group (leader only)
    cmdsource->Unsubscribe("/leavegroup",this);  
    cmdsource->Unsubscribe("/groupmembers",this);
    cmdsource->Unsubscribe("/groupchallenge",this);
    cmdsource->Unsubscribe("/groupyield",this);
}

const char *psGroupCommands::HandleCommand(const char *cmd)
{
    psGroupCmdMessage cmdmsg(cmd);
    cmdmsg.SendMessage();
    return NULL;
}

void psGroupCommands::HandleMessage(MsgEntry* /*msg*/)
{

}
