/** psinventorycachesvr.h
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

#ifndef PS_INVENTORY_CACHE_SERVER
#define PS_INVENTORY_CACHE_SERVER

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "util/pscache.h"

//=============================================================================
// Local Includes
//=============================================================================

/**
 * \addtogroup bulkobjects
 * @{ */

/**
 * The psInventoryCacheServer class implements the inventory cache on the server.
 */
class psInventoryCacheServer : public psCache
{

public:
    psInventoryCacheServer();
    ~psInventoryCacheServer();

    /**
     * Flags that a bulk slot's contents has changed.
     *
     * @param slot: slot number
     * @return bool Slot has been flagged successfully.
     */
    bool SetSlotModified(INVENTORY_SLOT_NUMBER slot);

    /**
     * Checks if a bulk slot's contents has changed.
     *
     * @param slot: slot number
     * @return bool bulk slot status.
     */
    bool HasSlotModified(INVENTORY_SLOT_NUMBER slot);

    /**
     * Clears flag that bulk slot has modified.
     *
     * @param slot slot number
     * @return bool Slot has been flagged successfully.
     */
    bool ClearSlot(INVENTORY_SLOT_NUMBER slot);

    /**
     * Clears all slot modified flags.
     *
     * @return bool Success flag.
     */
    bool ClearAllSlots(void);

private:
    bool SlotModified[PSCHARACTER_SLOT_BULK_END];                   ///< flag if a bulk-slot has modified since last update
};

/** @} */

#endif

