/*
 * cmdgroups.h by Anders Reggestad <andersr@pvv.org>
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

#ifndef __CMDGROUPS_H__
#define __CMDGROUPS_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================


/** Manager class to help with handling of 'group' commands.
 */
class psGroupCommands : public psCmdBase
{
public:
    psGroupCommands(ClientMsgHandler* mh,
                    CmdHandler* ch,
                    iObjectRegistry* obj);
    virtual ~psGroupCommands();

    /** Handles a command to send to the server.  
      * The following commands are supported as the command parameter:
      <UL>
        <LI> /invite
        <LI> /disband   disband group (leader only)
        <LI> /groupremove  remove player from group (leader only)
        <LI> /leavegroup  name group level something (ranks)
        <LI> /groupmembers see list of members (optional level #)
        <LI> /confirmgroupjoin
       </UL>        
       
       @return NULL
    */        
    virtual const char *HandleCommand(const char *cmd);

    virtual void HandleMessage(MsgEntry *msg);
};

#endif
