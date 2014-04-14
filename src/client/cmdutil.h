/*
 * cmdutil.h - Author: Keith Fulton
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

#ifndef __CMDUTIL_H__
#define __CMDUTIL_H__
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

class WordArray;

/** Class to handle general player commands. */
class psUtilityCommands : public psCmdBase
{
public:
    psUtilityCommands(ClientMsgHandler *mh,
                      CmdHandler *ch,
                      iObjectRegistry* obj);

    virtual ~psUtilityCommands();

    virtual const char *HandleCommand(const char *cmd);

    virtual void HandleMessage(MsgEntry *msg);

    static void HandleConfirmButton(bool answeredYes, void *thisptr);

    /// Handles the quit dialog box confirmation on /quit
    static void HandleQuit( bool answeredYes, void* thisPtr );
    
    csString SaveCamera();

protected:
    csString text;
    csString yescommands;
    csString nocommands;
    
private:
    /// Handles the /quit command.
    void HandleQuit();    
    
    /// Handles the /dump_movements command.
    //void HandleDumpMovements();
    
    /// Handles the /screenshot command
    void HandleScreenShot(bool compress, bool bigScreenShot, int width, int height);
    
    /// Handles the /confirm command
    void HandleConfirm( WordArray& array, csString& error );

    /// Handles the /echo command.
    bool HandleEcho( WordArray& words );

    /// Handles the /testanim command.
    void HandleTestAnim( WordArray& words, csString& output );
};

#endif
