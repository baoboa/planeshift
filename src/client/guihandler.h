/*
* guihandler.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*           Keith Fulton <keith@planeshift.it>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
*/
#ifndef GUIHANDLER_H
#define GUIHANDLER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================



class psInventoryCache;

/**
 * This class receives all network messages from the server
 * which affect the GUI or are displayable by the GUI.  It
 * unpacks these messages and publishes them to the correct
 * widget subscriber names.
 */
class GUIHandler : public psCmdBase
{
public:
    GUIHandler();
    void HandleMessage( MsgEntry* me );    
        
    const char* HandleCommand( const char* /*cmd*/ ) { return NULL; }
    
    psInventoryCache* GetInventoryCache(void) { return inventoryCache; }

protected:
    void HandleInventory( MsgEntry* me );

private:
    psInventoryCache* inventoryCache;
};

#endif
