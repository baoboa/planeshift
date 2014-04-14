/*
 * slotmanager.h
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __SLOTMANAGER_H__
#define __SLOTMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

/** Handles Drag and Drop Messages from the client.
 *  This basically figures out what item is being dragged and places it in a
 *  holding slot.  It then looks for the destination slot and sees if it can be placed
 *  there.  If not it will Replace the holding item back to where it came from.
 */
class SlotManager : public MessageManager<SlotManager>
{
public:
    SlotManager(GEMSupervisor* gemsupervisor, CacheManager* cachemanager);
    virtual ~SlotManager();

    bool Initialize();

    psItem* FindItem(Client* client, int containerID, INVENTORY_SLOT_NUMBER slotID);

private:

    ///drop a stack of the item type requested by searching into the different slots
    void HandleDropCommand(MsgEntry* me, Client* client);
    void HandleSlotMovement(MsgEntry* me, Client* client);

    /** Create a money item.
     *
     * @param slot One of the four money slots.
     * @param stackCount The amount to put in the item.
     * @return A new psItem object.
     */
    psItem* MakeMoneyItem(INVENTORY_SLOT_NUMBER slot, int stackCount);

    /// Handle psSlotMovementMsg from Inventory to somewhere.
    void MoveFromInventory(psSlotMovementMsg &msg, Client* fromClient);

    /// Handle psSlotMovementMsg from Exchange Offering slots to somewhere else.
    void MoveFromOffering(psSlotMovementMsg &msg, Client* fromClient);

    /// Handle moving money back from offering to own inventory.
    void MoveFromOfferedMoney(psSlotMovementMsg &msg, Client* fromClient);

    /// Handle psSlotMovementMsg from Money slots in inv to somewhere else.
    void MoveFromMoney(psSlotMovementMsg &msg, Client* fromClient);

    /// Handle moving items from containers back to inventory.
    void MoveFromWorldContainer(psSlotMovementMsg &msg, Client* fromClient, uint32 containerEntityID);

    /// Consume an item and fire off any progression events it has.
    void Consume(psItem* item, psCharacter* charData, int count);

    GEMSupervisor* gemSupervisor;
    CacheManager* cacheManager;
    MathScript* qualityScript;
};

#endif
