/*
 * slotmanager.cpp
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
 */
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/psconst.h"
#include "util/eventmanager.h"
#include "util/mathscript.h"

#include "net/messages.h"
#include "net/msghandler.h"

#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psitem.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "slotmanager.h"
#include "cachemanager.h"
#include "exchangemanager.h"

// TODO: These should not be included (code needs refactoring)
#include "progressionmanager.h"
#include "workmanager.h"
#include "psserverchar.h"
#include "client.h"
#include "globals.h"
#include "adminmanager.h"
#include "scripting.h"


SlotManager::SlotManager(GEMSupervisor* gemsupervisor, CacheManager* cachemanager)
{
    gemSupervisor = gemsupervisor;
    cacheManager = cachemanager;
    qualityScript = psserver->GetMathScriptEngine()->FindScript("CalculateConsumeQuality");
}

SlotManager::~SlotManager()
{
    //do nothing
}

bool SlotManager::Initialize()
{
    Subscribe(&SlotManager::HandleSlotMovement, MSGTYPE_SLOT_MOVEMENT, REQUIRE_READY_CLIENT);
    Subscribe(&SlotManager::HandleDropCommand, MSGTYPE_CMDDROP, REQUIRE_READY_CLIENT);
    if(!qualityScript)
    {
        Error1("Failed to find MathScript CalculateConsumeQuality!");
        return false;
    }
    return true;
}

void SlotManager::HandleSlotMovement(MsgEntry* me, Client* fromClient)
{
    Debug1(LOG_ITEM, me->clientnum, "SlotManager::HandleSlotMovement begins.");

    psSlotMovementMsg mesg(me);

    Debug2(LOG_ITEM, me->clientnum, "Got slot movement message for %d stacked items.", mesg.stackCount);
    Debug3(LOG_ITEM, me->clientnum, "--> From container %d, slot %d", mesg.fromContainer, mesg.fromSlot);
    Debug3(LOG_ITEM, me->clientnum, "--> To   container %d, slot %d", mesg.toContainer, mesg.toSlot);

    // If stacks are less than 1 then we should not do anything server wise.
    // This means the player does not own the item.
    if(mesg.stackCount < 1)
    {
        return;
    }

    if(!fromClient->GetCharacterData())
    {
        Error2("Could not find Character Data for client: %s", fromClient->GetName());
        return;
    }

    uint32 worldContainerID = 0;
    int otherContainerID = 0;
    if(mesg.fromContainer > 1000)   // not an inventory slot
    {
        worldContainerID = mesg.fromContainer;
        otherContainerID = mesg.toContainer;
        mesg.fromContainer = CONTAINER_GEM_OBJECT;
    }

    switch(mesg.fromContainer)
    {
        case CONTAINER_GEM_OBJECT:
            MoveFromWorldContainer(mesg, fromClient, worldContainerID);
            break;
        case CONTAINER_INVENTORY_MONEY:
            MoveFromMoney(mesg, fromClient);
            break;
        case CONTAINER_EXCHANGE_OFFERING:
            MoveFromOffering(mesg, fromClient);
            break;
        case CONTAINER_OFFERING_MONEY:
            MoveFromOfferedMoney(mesg, fromClient);
            break;
        case CONTAINER_INVENTORY_BULK:
        case CONTAINER_INVENTORY_EQUIPMENT:
        default:
            MoveFromInventory(mesg, fromClient);
            break;
    }
    // TODO: move inside above functions based on to and from containers
    //  ie. this is not needed for world container to world container moves
    //  also it updates glyphs as well
    if(worldContainerID == 0 || otherContainerID <= 100)
    {
        psserver->GetCharManager()->UpdateItemViews(fromClient->GetClientNum());
    }
    else
    {
        csString status;
        status.Format("Received nop slot movement message. Client %u Container %d Slot %d", fromClient->GetClientNum(), mesg.fromContainer, mesg.fromSlot);

        if(LogCSV::GetSingletonPtr())
            LogCSV::GetSingleton().Write(CSV_STATUS, status);
    }

    Debug1(LOG_ITEM, me->clientnum, "SlotManager::HandleSlotMovement ends.");
    return;
}

void SlotManager::MoveFromWorldContainer(psSlotMovementMsg &msg, Client* fromClient, uint32 containerEntityID)
{
    // printf("Moving item from world container %d, from slot %d.\n", containerEntityID, msg.fromSlot);

    psCharacter* chr = fromClient->GetCharacterData();

    gemObject* obj = gemSupervisor->FindObject(EID(containerEntityID)); // CEL id assigned
    gemContainer* worldContainer = dynamic_cast<gemContainer*>(obj);
    if(!worldContainer)
    {
        Error2("Couldn't find any CEL entity id %d.", containerEntityID);
        return;
    }
    psItem* parentItem = worldContainer->GetItem();
    if(!parentItem)
    {
        Error2("No parent container item found for worldContainer %d.", containerEntityID);
        return;
    }
    if(!parentItem->GetIsContainer())
    {
        Error2("Cannot put item into another item %s.\n", parentItem->GetName());
        return;
    }

    // Check to see if that item exists in the container with the required amount.
    // This will return the entire stack of items despite the count that we want.
    psItem* itemProposed = worldContainer->FindItemInSlot(msg.fromSlot, msg.stackCount);

    if(!itemProposed)
    {
        Error5("Cannot find any item in slot %d for container %s(%d) range of player is %g.", msg.fromSlot, parentItem->GetName(), parentItem->GetUID(), fromClient->GetActor()->RangeTo(worldContainer, true, true));
        return;
    }

    if(!worldContainer->CanTake(fromClient,itemProposed))
    {
        Error2("Client %u tried to take item it doesn't own.", fromClient->GetClientNum());
        return;
    }

    if(fromClient->GetActor()->RangeTo(worldContainer, true, true) > RANGE_TO_USE)
    {
        psserver->SendSystemError(fromClient->GetClientNum(),
                                  "You are not in range to use %s.",worldContainer->GetItem()->GetName());
        return;
    }

    psserver->GetWorkManager()->StopWork(fromClient, itemProposed);

    uint32 to_containerEntityID;
    if(msg.toContainer > 100)
    {
        to_containerEntityID = msg.toContainer;
        msg.toContainer   = CONTAINER_GEM_OBJECT;
    }

    switch(msg.toContainer)
    {
            //---------------------------------------------------------------------
            // Moving from a world container to another world container.  Possibly
            // the same one but in a different slot.
            //---------------------------------------------------------------------
        case CONTAINER_GEM_OBJECT:
        {
            // TODO: Talad would like to be able to put containers in world containers.
            if(itemProposed->GetIsContainer())
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "You may not put a container in a container.");
                return;
            }

            gemObject* obj = gemSupervisor->FindObject(EID(to_containerEntityID)); // CEL id assigned
            gemContainer* to_worldContainer = dynamic_cast<gemContainer*>(obj);
            if(!to_worldContainer)
            {
                Error2("Couldn't find any CEL entity id %d.", to_containerEntityID);
                return;
            }

            psItem* to_parentItem = to_worldContainer->GetItem();
            if(!to_parentItem)
            {
                Error2("No parent container item found for worldContainer %d.", to_containerEntityID);
                return;
            }
            if(!to_parentItem->GetIsContainer())
            {
                Error2("Cannot put item into another item %s.\n", to_parentItem->GetName());
                return;
            }

            // Now take this out of slot and put in other slot
            // TODO: This should be msg.stackCount once we can split stacks inside a container.

            psItem* newItem = NULL;

            csString reason;
            if(to_worldContainer->CanAdd(msg.stackCount, itemProposed, msg.toSlot, reason))
            {
                newItem = worldContainer->RemoveFromContainer(itemProposed, msg.fromSlot,fromClient, msg.stackCount);
                if(!newItem)
                {
                    Error1("Can not delete item from container");
                    return;
                }

                if(!to_worldContainer->AddToContainer(newItem, fromClient, msg.toSlot))
                {
                    Error2("Bad container slot %i when trying to add items to container", msg.toSlot);
                    return;
                }
            }
            else
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "Item cannot be added to that container"+reason+".");
                parentItem->SendContainerContents(fromClient, to_containerEntityID);
                return;
            }


            // Start work on item again
            // If the entire stack was taken then newItem will be the same as itemProposed.  If that is not the
            // case there was a stack split so setup work on the two stacks.
            if(newItem != itemProposed)
            {
                psserver->GetWorkManager()->StartAutoWork(fromClient, worldContainer, itemProposed, itemProposed->GetStackCount());
            }

            if(newItem)
            {
                psserver->GetWorkManager()->StartAutoWork(fromClient, to_worldContainer, newItem, newItem->GetStackCount());
            }

            to_parentItem->SendContainerContents(fromClient, to_containerEntityID);

            break;
        }
        // Drop one item into the world.
        case CONTAINER_WORLD:
        {
            itemProposed->SetGuardingCharacterID(0);
            worldContainer->RemoveFromContainer(itemProposed,fromClient);
            chr->DropItem(itemProposed, msg.posWorld, msg.rot);
            break;
        }

        case CONTAINER_EXCHANGE_OFFERING:
        case CONTAINER_EXCHANGE_RECEIVING:
        case CONTAINER_INVENTORY_MONEY:
        case CONTAINER_OFFERING_MONEY:
        case CONTAINER_RECEIVING_MONEY:
        {
            psserver->SendSystemError(fromClient->GetClientNum(), "You can't place that item there.");
            break;
        }

        case CONTAINER_INVENTORY_EQUIPMENT:
        case CONTAINER_INVENTORY_BULK:
        default:
        {

            // See if it can be placed in the player's inventory
            // TODO: Can't use destSlot here because Add doesn't check if the destination slot's full.
            // After fixing Add, re-enable this.
            //INVENTORY_SLOT_NUMBER destSlot = (INVENTORY_SLOT_NUMBER) (msg.toSlot + (int) PSCHARACTER_SLOT_BULK1);
            psItem* newItem;

            if(chr->Inventory().Add(itemProposed, true))  // Test = true
            {
                // Now that it was successful, take it out of the world container
                //worldContainer->RemoveFromContainer(itemProposed,fromClient);
                newItem = worldContainer->RemoveFromContainer(itemProposed, msg.fromSlot,fromClient, msg.stackCount);

                // If we did not take the entire stack, restart the work on the items that are left over.
                if(newItem != itemProposed)
                {
                    psserver->GetWorkManager()->StartAutoWork(fromClient, worldContainer, itemProposed, itemProposed->GetStackCount());
                }

                chr->Inventory().Add(newItem, false);
                itemProposed->SetGuardingCharacterID(0);

                parentItem->SendContainerContents(fromClient, containerEntityID);
                return;
            }

            itemProposed->Save(false);
            // Update client(s)
            itemProposed->UpdateView(fromClient, worldContainer->GetEID(), true);

            parentItem->SendContainerContents(fromClient, containerEntityID);
            break;
        }
    }
}

void SlotManager::MoveFromMoney(psSlotMovementMsg &msg, Client* fromClient)
{
    psCharacter* chr = fromClient->GetCharacterData();
    // Remove the money from the character

    // This can be caused by packet misordering
    if(chr->Money().Get(msg.fromSlot) - msg.stackCount < 0)
    {
        psserver->SendSystemError(fromClient->GetClientNum(), "You do not have that much money.");
        return;
    }

    psMoney money;
    money.Adjust(msg.fromSlot, msg.stackCount);

    switch(msg.toContainer)
    {
        case CONTAINER_WORLD:
        {
            // printf("Dropping money from slot %d, count %d, in world.\n", msg.fromSlot, msg.stackCount);
            if(msg.stackCount > MAX_STACK_COUNT)
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "You can't place a stack of more than %d items.", MAX_STACK_COUNT);
                return;
            }
            psItem* item = MakeMoneyItem((INVENTORY_SLOT_NUMBER)msg.fromSlot, msg.stackCount);
            if(item)
            {
                // Remove the money from inventory, and put in world.
                money.Set(-money.Get(3), -money.Get(2), -money.Get(1), -money.Get(0));
                chr->AdjustMoney(money, false);
                chr->DropItem(item, msg.posWorld, msg.rot, msg.guarded, true, msg.inplace);
            }
            else
                Error3("Could not create money item from slot %d, count %d!", msg.fromSlot, msg.stackCount);

            break;
        }
        case CONTAINER_OFFERING_MONEY:
        case CONTAINER_EXCHANGE_OFFERING:
        {
            Exchange* exchange = psserver->exchangemanager->GetExchange(fromClient->GetExchangeID());
            if(!exchange)
            {
                Error2("Client %s requested exchange slot movement but is not in an exchange.", fromClient->GetName());
                return;
            }
            exchange->AdjustMoney(fromClient, money);
            money.Set(-money.Get(3), -money.Get(2), -money.Get(1), -money.Get(0));
            break;
        }
        case CONTAINER_INVENTORY_MONEY:
        {
            break;
        }
        default:
        {
            psserver->SendSystemError(fromClient->GetClientNum(), "You can't place money there.");
            break;
        }
    }
}

void SlotManager::MoveFromInventory(psSlotMovementMsg &msg, Client* fromClient)
{
    INVENTORY_SLOT_NUMBER srcSlot = PSCHARACTER_SLOT_NONE;
    INVENTORY_SLOT_NUMBER destSlot = PSCHARACTER_SLOT_NONE;

    if(msg.fromContainer == CONTAINER_INVENTORY_BULK)
        srcSlot = (INVENTORY_SLOT_NUMBER)(msg.fromSlot + (int)PSCHARACTER_SLOT_BULK1);
    else if(msg.fromContainer == CONTAINER_INVENTORY_EQUIPMENT)
        srcSlot = (INVENTORY_SLOT_NUMBER) msg.fromSlot;
    else
        srcSlot = (INVENTORY_SLOT_NUMBER)(msg.fromContainer * 100 + msg.fromSlot + (int)PSCHARACTER_SLOT_BULK1);

    psCharacter* chr = fromClient->GetCharacterData();

    psItem* itemProposed = chr->Inventory().GetItem(NULL, srcSlot);
    if(!itemProposed)
    {
        //Error2("Couldn't find item in proposed srcSlot %d.\b", srcSlot);
        return;
    }
    Debug3(LOG_ITEM, 0, "Proposing to move item %s from slot %d", itemProposed->GetName(), srcSlot);

    if(itemProposed->IsInUse())
    {
        psserver->SendSystemError(fromClient->GetClientNum(), "You cannot move an item being used.");
        return;
    }

    if(chr->Inventory().GetOfferedStackCount(itemProposed) > 0)
    {
        // printf("%s has %d in offered slot.\n", itemProposed->GetName(), chr->Inventory().GetOfferedStackCount(itemProposed));
        psserver->SendSystemError(fromClient->GetClientNum(), "You cannot move an item being exchanged.");
        return;
    }

    int containerEntityID = 0;
    if(msg.toContainer > 100)
    {
        containerEntityID = msg.toContainer;
        msg.toContainer   = CONTAINER_GEM_OBJECT;
    }

    csString explanation;

    switch(msg.toContainer)
    {
        case CONTAINER_GEM_OBJECT:
        {
            // TODO: Talad would like to be able to put containers in world containers.
            if(itemProposed->GetIsContainer())
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "You may not put a container in a container.");
                return;
            }
            gemObject* obj = gemSupervisor->FindObject(EID(containerEntityID)); // CEL id assigned
            gemContainer* worldContainer = dynamic_cast<gemContainer*>(obj);
            if(!worldContainer)
            {
                Error2("Couldn't find any CEL entity id %d.", containerEntityID);
                return;
            }
            psItem* parentItem = worldContainer->GetItem();
            if(!parentItem)
            {
                Error2("No parent container item found for worldContainer %d.", containerEntityID);
                return;
            }
            if(!parentItem->GetIsContainer())
            {
                Error2("Cannot put item into another item %s.\n", parentItem->GetName());
                return;
            }
            if(parentItem->GetIsNpcOwned() && fromClient->GetSecurityLevel() < GM_DEVELOPER)
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "You may not use that container.");
                return;
            }
            if(fromClient->GetActor()->RangeTo(worldContainer, true, true) > RANGE_TO_USE)
            {
                psserver->SendSystemError(fromClient->GetClientNum(),
                                          "You are not in range to use %s.",worldContainer->GetItem()->GetName());
                return;
            }
            // don't allow players to put an item from in locked container
            if(parentItem->GetIsLocked() && fromClient->GetSecurityLevel() < GM_LEVEL_2)
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "You cannot put an item in a locked container!");
                return;
            }

            // Now take this out of inventory and put in container
            csString reason;
            if(worldContainer->CanAdd(msg.stackCount, itemProposed, msg.toSlot, reason))
            {
                psItem* newItem = chr->Inventory().RemoveItem(NULL, (INVENTORY_SLOT_NUMBER) srcSlot, msg.stackCount);
                if(!newItem)
                {
                    psserver->SendSystemError(fromClient->GetClientNum(), "Couldn't find %d items in that slot.", msg.stackCount);
                    return;
                }
                worldContainer->AddToContainer(newItem, fromClient, msg.toSlot);
                psserver->GetWorkManager()->StartAutoWork(fromClient, worldContainer, newItem, msg.stackCount);
            }
            else
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "Item cannot be added to that container"+reason+".");
                parentItem->SendContainerContents(fromClient, containerEntityID);
                return;
            }

            break;
        }
        case CONTAINER_EXCHANGE_OFFERING:
        {
            // TODO: Trading containers is broken (bugs.hydlaa.com #2659).
            if(itemProposed->GetIsContainer())
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "Trading containers is not yet implemented.");
                return;
            }

            Exchange* exchange = psserver->exchangemanager->GetExchange(fromClient->GetExchangeID());
            if(!exchange)
            {
                Error2("Client %s requested exchange slot movement but is not in an exchange.", fromClient->GetName());
                return;
            }

            // We should probably swap items...for now, require an empty slot.
            if(chr->Inventory().FindExchangeSlotOffered(msg.toSlot))
                return;

            // printf("Adding item %s to exchange offering.\n", itemProposed->GetName());
            chr->Inventory().SetExchangeOfferSlot(NULL, srcSlot, msg.toSlot, msg.stackCount);
            exchange->AddItem(fromClient, srcSlot, msg.stackCount, msg.toSlot);
            break;
        }

        case CONTAINER_WORLD:
        {
            psItem* newItem = chr->Inventory().RemoveItemID(itemProposed->GetUID(), msg.stackCount);
            chr->DropItem(newItem, msg.posWorld, msg.rot);
            break;
        }

        case CONTAINER_EXCHANGE_RECEIVING:
        case CONTAINER_INVENTORY_MONEY:
        case CONTAINER_OFFERING_MONEY:
        case CONTAINER_RECEIVING_MONEY:
        {
            psserver->SendSystemError(fromClient->GetClientNum(), "You can't place that item there.");
            break;
        }

        case CONTAINER_INVENTORY_BULK:
        case CONTAINER_INVENTORY_EQUIPMENT:
        default:
        {
            // Get the real destination slot from the message...
            if(msg.toContainer == CONTAINER_INVENTORY_EQUIPMENT)
                destSlot = (INVENTORY_SLOT_NUMBER) msg.toSlot;
            else if(msg.toContainer == CONTAINER_INVENTORY_BULK)
                destSlot = (INVENTORY_SLOT_NUMBER)(msg.toSlot + (int) PSCHARACTER_SLOT_BULK1);
            else
                destSlot = (INVENTORY_SLOT_NUMBER)(msg.toContainer * 100 + msg.toSlot + (int) PSCHARACTER_SLOT_BULK1);

            // Handle anything dragged to the character doll
            if(destSlot == PSCHARACTER_SLOT_NONE)
            {
                if(itemProposed->GetBaseStats()->GetIsConsumable())
                {
                    int stackCount = csMin(msg.stackCount, (int)itemProposed->GetStackCount());
                    Consume(itemProposed, chr, stackCount);
                    return;
                }
                else
                {
                    destSlot = chr->Inventory().FindFreeEquipSlot(itemProposed);
                    if(destSlot == PSCHARACTER_SLOT_NONE)
                    {
                        psserver->SendSystemError(fromClient->GetClientNum(), "This item cannot be equipped.");
                        return;
                    }
                }
            }

            if(destSlot == srcSlot)  // if you drop back on itself, don't do anything
                return;

            // Disallow containers in containers
            if(msg.toContainer > 0)  // container item
            {
                if(itemProposed->GetIsContainer())
                {
                    psserver->SendSystemError(fromClient->GetClientNum(), "You may not put a container in a container.");
                    return;
                }
            }

            // Check for slot compatibility
            if(!chr->Inventory().CheckSlotRequirements(itemProposed, destSlot, msg.stackCount))
            {
                psserver->SendSystemError(fromClient->GetClientNum(), "%s does not fit in that slot.", itemProposed->GetName());
                return;
            }
            else
            {
                // printf("Item %s will fit in slot %d\n", itemProposed->GetName(), destSlot);
            }


            // Check for dest slot already occupied
            psItem* item = chr->Inventory().GetItem(NULL, destSlot);
            uint32_t srcparent = 0;
            if(item)
            {
                // TODO: This is probably buggy when you hit max stack count...
                if(item->CheckStackableWith(itemProposed, false))
                {
                    // easy case is to stack compatible items
                    psItem* stack = chr->Inventory().RemoveItem(NULL, srcSlot, msg.stackCount);
                    if(!stack)
                        return;

                    item->CombineStack(stack);
                    item->Save(false);
                    // Updating Containers view if needed
                    if(destSlot > 99)
                    {
                        chr->Inventory().GetItem(NULL, (INVENTORY_SLOT_NUMBER)(destSlot / 100))->SendContainerContents(fromClient, msg.toContainer);
                    }
                    else if(srcSlot > 99)
                    {
                        chr->Inventory().GetItem(NULL, (INVENTORY_SLOT_NUMBER)(srcSlot / 100))->SendContainerContents(fromClient, msg.fromContainer);
                    }
                    return;
                }
                // printf("Proposed move will displace item %s\n", item->GetName());
                if(srcSlot > 99)
                {
                    psItem* parentItem = chr->Inventory().GetItem(NULL, (INVENTORY_SLOT_NUMBER)(srcSlot/100));
                    if(!parentItem)
                        return;
                    if(!parentItem->GetIsContainer())
                    {
                        psserver->SendSystemError(fromClient->GetClientNum(), "Cannot put item into another item %s.", parentItem->GetName());
                        return;
                    }
                    srcparent = parentItem->GetUID();
                }
                if(msg.stackCount != itemProposed->GetStackCount())
                {
                    psserver->SendSystemError(fromClient->GetClientNum(), "Cannot drop a partial stack on an existing item.");
                    return;
                }
                if(!chr->Inventory().CheckSlotRequirements(item, srcSlot))
                {
                    psserver->SendSystemError(fromClient->GetClientNum(), "You cannot swap with %s; it won't fit in that slot.", item->GetName());
                    return;
                }
            }


            // Get the parent item UID, if necessary.
            uint32_t parent = 0;
            if(destSlot > 99)
            {
                psItem* parentItem = chr->Inventory().GetItem(NULL, (INVENTORY_SLOT_NUMBER)(destSlot / 100));
                CS_ASSERT(parentItem); // CheckSlotRequirements ensures this.
                parent = parentItem->GetUID();
            }
            // printf("Ok!\n");

            // At this point we're sure the items can all move where they need to, so just do it.

            psItem* itemMoving = NULL;
            // Check for splitting stack
            if(msg.stackCount < itemProposed->GetStackCount())
            {
                itemMoving = itemProposed->SplitStack(msg.stackCount);
                if(chr->Inventory().Add(itemMoving, false, false, destSlot))
                {
                    itemProposed->Save(false);
                }
                else // this really should never happen, but just in case
                {
                    itemProposed->CombineStack(itemMoving);
                }

            }
            else // entire stack
            {
                // The following is an ugly hack: the destination slot must
                // be cleared -before- we move the item there, since the
                // DEEQUIP message must go to the client before the EQUIP
                // message.  If not, the client gets a ghost-weapon mesh stuck
                // in their hand - shouldn't have other ill effects though.
                if(item)
                    item->UpdateInventoryStatus(chr, 0, PSCHARACTER_SLOT_NONE);
                itemMoving = itemProposed;
                itemMoving->UpdateInventoryStatus(chr, parent, destSlot);
                itemMoving->Save(false);
                if(item)
                {
                    // If we pushed an item out of the way in that slot, put it where the other one came from
                    item->UpdateInventoryStatus(chr, srcparent, srcSlot);
                    item->Save(false);
                }

            }
            // Updating Containers view if needed
            if(destSlot > 99)
            {
                chr->Inventory().GetItem(NULL, (INVENTORY_SLOT_NUMBER)(destSlot / 100))->SendContainerContents(fromClient, msg.toContainer);
            }
            else if(srcSlot > 99)
            {
                chr->Inventory().GetItem(NULL, (INVENTORY_SLOT_NUMBER)(srcSlot / 100))->SendContainerContents(fromClient, msg.fromContainer);
            }
            break;
        }
    }
}

void SlotManager::MoveFromOfferedMoney(psSlotMovementMsg &msg, Client* fromClient)
{
    psMoney money;
    money.Adjust(msg.fromSlot, msg.stackCount);

    Exchange* exchange = psserver->exchangemanager->GetExchange(fromClient->GetExchangeID());
    if(!exchange)
    {
        Error2("Client %s requested exchange slot movement but is not in an exchange.", fromClient->GetName());
        return;
    }

    // Cannot use Adjust to negate money, so must set positive, then reverse
    money.Set(-money.Get(3), -money.Get(2), -money.Get(1), -money.Get(0));
    exchange->AdjustMoney(fromClient, money);
}

void SlotManager::MoveFromOffering(psSlotMovementMsg &msg, Client* fromClient)
{
    INVENTORY_SLOT_NUMBER srcSlot = PSCHARACTER_SLOT_NONE;

    srcSlot = (INVENTORY_SLOT_NUMBER) msg.fromSlot;

    psCharacter* chr = fromClient->GetCharacterData();

    psCharacterInventory::psCharacterInventoryItem* itemInSlot = chr->Inventory().FindExchangeSlotOffered(srcSlot);
    psItem* itemProposed = itemInSlot ? itemInSlot->GetItem() : NULL;

    if(!itemProposed)
    {
        Error2("Couldn't find item in offered srcSlot %d.\b", srcSlot);
        return;
    }
    // printf("Proposing to move item %s\n", itemProposed->GetName());

    Exchange* exchange = psserver->exchangemanager->GetExchange(fromClient->GetExchangeID());
    if(!exchange)
    {
        Error2("Client %s requested exchange slot movement but is not in an exchange.", fromClient->GetName());
        return;
    }

    switch(msg.toContainer)
    {
        case CONTAINER_EXCHANGE_OFFERING:
        {
            // printf("Moving item %s within exchange offering.\n", itemProposed->GetName());
            exchange->MoveItem(fromClient, srcSlot, msg.stackCount, msg.toSlot);
            break;
        }
        case CONTAINER_INVENTORY_BULK:
        case CONTAINER_INVENTORY_EQUIPMENT:
        default:
        {
            // Update exchange (and clients) that this item isn't in the offering anymore
            exchange->RemoveItem(fromClient, msg.fromSlot, msg.stackCount);
            break;
        }
    }
}



psItem* SlotManager::FindItem(Client* client, int containerID, INVENTORY_SLOT_NUMBER slotID)
{
    // Container inside something
    if(containerID >= 0 && containerID < 100)
    {
        slotID = (INVENTORY_SLOT_NUMBER)(containerID * 100 + slotID + PSCHARACTER_SLOT_BULK1); // adjust slot number to contained slot
        return FindItem(client, CONTAINER_INVENTORY_BULK, slotID);
    }

    switch(containerID)
    {
        case CONTAINER_INVENTORY_BULK:
        {
            return client->GetCharacterData()->Inventory().GetItem(NULL, slotID);
        }
        case CONTAINER_INVENTORY_EQUIPMENT:
        {
            return client->GetCharacterData()->Inventory().GetInventoryItem(slotID);
        }

        case CONTAINER_EXCHANGE_OFFERING:
        {
            Exchange* exchange = psserver->GetExchangeManager()->GetExchange(client->GetExchangeID());
            if(!exchange) return NULL;

            // Container is different from each perspective
            if(exchange->GetStarterClient() == client)
                return exchange->GetStarterOffer(slotID);
            else
                return exchange->GetTargetOffer(slotID);
        }
        case CONTAINER_EXCHANGE_RECEIVING:
        {
            Exchange* exchange = psserver->GetExchangeManager()->GetExchange(client->GetExchangeID());
            if(!exchange) return NULL;

            // Container is different from each perspective
            if(exchange->GetStarterClient() == client)
                return exchange->GetTargetOffer(slotID);
            else
                return exchange->GetStarterOffer(slotID);
        }

        default:  // Find container in world
        {
            gemObject* object = gemSupervisor->FindObject(EID(containerID));
            if(object && slotID == -1)
                return object->GetItem();

            gemContainer* container = dynamic_cast<gemContainer*>(object);
            if(container)
            {
                return container->FindItemInSlot(slotID);
            }
            else
            {
                //Error2("No Object %d found", containerID);
                return NULL;
            }
        }
    }
}

psItem* SlotManager::MakeMoneyItem(INVENTORY_SLOT_NUMBER slot, int stackCount)
{
    psItem* holdingItem =  new psItem();
    csString type("Tria");

    switch(slot)
    {
        case MONEY_TRIAS:
            type="Tria";
            break;
        case MONEY_HEXAS:
            type="Hexa";
            break;
        case MONEY_OCTAS:
            type="Octa";
            break;
        case MONEY_CIRCLES:
            type="Circle";
            break;
        default:
            Error2("Unknown fromSlot: %d", slot);
            break;
    }

    psItemStats* stat = cacheManager->GetBasicItemStatsByName(type);
    if(!stat)
    {
        Error2("Could not load basic item stat for %s", type.GetData());
    }
    else
    {
        holdingItem->SetBaseStats(stat);
        holdingItem->SetStackCount(stackCount);
    }

    holdingItem->SetLoaded(); // Loaded and ready

    return holdingItem;
}

void SlotManager::Consume(psItem* item, psCharacter* charData, int count)
{

    if(!item)
        return;

    csString scriptName = item->GetBaseStats()->GetConsumeScriptName();
    if(scriptName.IsEmpty())
    {
        Error2("No consume script for consumable item %s",item->GetName());
        return;
    }

    ProgressionScript* script = psserver->GetProgressionManager()->FindScript(scriptName);
    if(!script)
    {
        Error2("No event found for consume script >%s<.", scriptName.GetData());
        return;
    }

    // Use math function to determine effect of quality
    MathEnvironment env;
    env.Define("Quality", item->GetMaxItemQuality());
    (void) qualityScript->Evaluate(&env);

    gemActor* actor = charData->GetActor();
    env.Define("Actor", actor);

    // Remove the item from inventory and destroy it
    psItem* consumedItem = charData->Inventory().RemoveItemID(item->GetUID(), count);
    if(consumedItem)
    {
        env.Define("Item", consumedItem);
        for(unsigned short i = 0; i < consumedItem->GetStackCount(); i++)
        {
            script->Run(&env);
        }
        consumedItem->Destroy();
        delete consumedItem;
    }
}

void SlotManager::HandleDropCommand(MsgEntry* me, Client* fromClient)
{
    psCmdDropMessage mesg(me);
    if(mesg.quantity < 1)
    {
        mesg.quantity = 1;
        psserver->SendSystemInfo(fromClient->GetClientNum(), "You cannot drop less than one item.");
    }
    if(mesg.quantity > 65)
    {
        mesg.quantity = 65;
        psserver->SendSystemInfo(fromClient->GetClientNum(), "You cannot drop more than a stack.");
    }

    if(!fromClient->GetCharacterData())
    {
        Error2("Could not find Character Data for client: %s", fromClient->GetName());
        return;
    }

    psCharacter* chr = fromClient->GetCharacterData();
    psItem* stackItem = chr->Inventory().StackNumberItems(mesg.itemName, mesg.quantity, mesg.container);

    if(!stackItem)
    {
        //Error2("Could not find Item  for this item: %s", mesg.itemName.GetData());
        psserver->SendSystemInfo(fromClient->GetClientNum(), "Item %s not found in inventory.", mesg.itemName.GetData());
        return;
    }

    int removeCount;
    if(mesg.quantity < stackItem->GetStackCount())
    {
        removeCount=mesg.quantity;
    }
    else
    {
        removeCount=-1;
    }

    psItem* toDropItem = chr->Inventory().RemoveItemID(stackItem->GetUID(), removeCount);
    chr->DropItem(toDropItem, 0, 0, mesg.guarded, true, mesg.inplace);


    psserver->GetCharManager()->UpdateItemViews(fromClient->GetClientNum());

    return;
}
