/** psinventorycachehdlr.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Server-side of the inventory cache handling.
 */

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharacter.h"
#include "psinventorycachesvr.h"

psInventoryCacheServer::psInventoryCacheServer()
{
}

psInventoryCacheServer::~psInventoryCacheServer()
{
}

bool psInventoryCacheServer::SetSlotModified(INVENTORY_SLOT_NUMBER slot)
{
    if(slot<0 || slot>=PSCHARACTER_SLOT_BULK_END)
    {
        SetCacheStatus(INVALID);    // somethings gone wrong; force entire
        // inventory update next time
        return false;
    }

    SlotModified[slot] = true;

    return true;
}

bool psInventoryCacheServer::HasSlotModified(INVENTORY_SLOT_NUMBER slot)
{
    if(slot<0 || slot>=PSCHARACTER_SLOT_BULK_END)
    {
        return false;
    }

    return SlotModified[slot];
}


bool psInventoryCacheServer::ClearSlot(INVENTORY_SLOT_NUMBER slot)
{
    if(slot<0 || slot>=PSCHARACTER_SLOT_BULK_END)
    {
        SetCacheStatus(INVALID);    // something has gone wrong; force entire
        // inventory update next time
        return false;
    }

    SlotModified[slot] = false;

    return true;
}


bool psInventoryCacheServer::ClearAllSlots(void)
{
    bool slotOK = true;

    for(int slot = 0; slot < PSCHARACTER_SLOT_BULK_END && slotOK; slot++)
    {
        slotOK = ClearSlot((INVENTORY_SLOT_NUMBER)slot);
    }

    return slotOK;
}

