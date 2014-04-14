/*
 * cmdguilds.h - Author: Keith Fulton
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

#ifndef __CMDGUILDS_H__
#define __CMDGUILDS_H__
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


/** Manager class for handling guild client commands. */
class psGuildCommands : public psCmdBase
{
public:
    psGuildCommands(ClientMsgHandler* mh,
                    CmdHandler* ch,
                    iObjectRegistry* obj );
    virtual ~psGuildCommands();

    virtual const char *HandleCommand(const char *cmd);

    virtual void HandleMessage(MsgEntry *msg);
};

#endif

