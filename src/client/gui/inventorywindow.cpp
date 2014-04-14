/*
 * inventorywindow.cpp - Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/util.h>
#include <csutil/xmltiny.h>
#include <iutil/evdefs.h>
#include <imesh/spritecal3d.h>

#include "net/message.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"
#include "util/strutil.h"
#include "util/psconst.h"
#include "globals.h"

#include "pscelclient.h"
#include "psinventorycache.h"


#include "paws/pawstextbox.h"
#include "paws/pawsprefmanager.h"
#include "inventorywindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawslistbox.h"
#include "paws/pawsnumberpromptwindow.h"

#include "gui/pawsmoney.h"
#include "gui/pawsexchangewindow.h"
#include "gui/pawsslot.h"
#include "gui/pawscontrolwindow.h"
#include "gui/pawsinventorydollview.h"
#include "gui/pawscontainerdescwindow.h"
#include "../charapp.h"

#define VIEW_BUTTON 1005
#define QUIT_BUTTON 1006

#define DEC_STACK_BUTTON 1010
#define INC_STACK_BUTTON 1011


/**********************************************************************
*
*                        class pawsInventoryWindow
*
***********************************************************************/

pawsInventoryWindow::pawsInventoryWindow()
{
    loader =  csQueryRegistry<iThreadedLoader> ( PawsManager::GetSingleton().GetObjectRegistry() );

    bulkSlots.SetSize( INVENTORY_BULK_COUNT );
    equipmentSlots.SetSize(INVENTORY_EQUIP_COUNT);
    for ( size_t n = 0; n < equipmentSlots.GetSize(); n++ )
        equipmentSlots[n] = NULL;

    charApp = new psCharAppearance(PawsManager::GetSingleton().GetObjectRegistry());
}

pawsInventoryWindow::~pawsInventoryWindow()
{
    delete charApp;
}


void pawsInventoryWindow::Show()
{
    if(!IsVisible())//ask to get the inventory only if the window was previously invisible.
    {
        // Ask the server to send us the inventory
        if ( !inventoryCache->GetInventory())
            inventoryCache->SetCacheStatus(psCache::INVALID);
    }
    pawsControlledWindow::Show();
}

bool pawsInventoryWindow::SetupSlot( const char* slotName )
{
    pawsSlot* slot = dynamic_cast <pawsSlot*> (FindWidget(slotName));
    if (slot == NULL)
    {
        Error2("Could not locate pawsSlot %s.",slotName);
        return false;
    }
    uintptr_t slotID = psengine->slotName.GetID(slotName);
    if ( slotID == csInvalidStringID )
    {
        Error2("Could not located the %s slot", slotName);
        return false;
    }
    slot->SetContainer(  CONTAINER_INVENTORY_EQUIPMENT );
    slot->SetSlotID( slotID );

    equipmentSlots[slotID] = slot;
    return true;
}

bool pawsInventoryWindow::PostSetup()
{
    // If you add something here, DO NOT FORGET TO CHANGE 'INVENTORY_EQUIP_COUNT'!!!
    const char* equipmentSlotNames[] = { "lefthand", "righthand", "leftfinger", "rightfinger", "helm", "neck", "back", "arms", "gloves", "boots", "legs", "belt", "bracers", "torso", "mind" };

    // Setup the Doll
    if ( !SetupDoll() )
        return false;

    money = dynamic_cast <pawsMoney*> (FindWidget("Money"));
    if ( money )
    {
        money->SetContainer( CONTAINER_INVENTORY_MONEY );
    }

    for(size_t x = 0; x < INVENTORY_EQUIP_COUNT-1; x++)
    {
        if(!SetupSlot(equipmentSlotNames[x]))
            return false;
    }

    pawsListBox* bulkList = dynamic_cast <pawsListBox*> (FindWidget("BulkList"));
    if (bulkList)
    {
        int colCount = bulkList->GetTotalColumns();
        int rowCount = (int) ceil(float(INVENTORY_BULK_COUNT)/colCount);

        for(int r = 0; r < rowCount; r ++)
        {
            pawsListBoxRow* listRow = bulkList->NewRow(r);
            for (int j = 0; j < colCount; j++)
            {
                int i = r*colCount + j;
                pawsSlot* slot;
                slot = dynamic_cast <pawsSlot*> (listRow->GetColumn(j));
                slot->SetContainer( CONTAINER_INVENTORY_BULK );
                //csString name;
                slot->SetSlotID( i );
                csString name;
                name.Format("invslot_%d", 16 + i ); // 16 equip slots come first
                slot->SetSlotName(name);

                if(i >= INVENTORY_BULK_COUNT)
                {
                    slot->Hide();
                    continue;
                }

                //printf("Subscribing bulk slot to %s.\n",name.GetData() );
                PawsManager::GetSingleton().Subscribe( name, slot );
                PawsManager::GetSingleton().Subscribe("sigClearInventorySlots", slot);
                bulkSlots[i] = slot;
            }
        }
    }

    // subscribe all equipment slots to sigClearInventorySlots - needed e.g for stacks of ammo or showing the orignal tooltip
    for(size_t z = 0; z < INVENTORY_EQUIP_COUNT-1; z++)
    {
        pawsSlot* slotname = dynamic_cast<pawsSlot*>(FindWidget(equipmentSlotNames[z]));
        if(slotname)
        {
            PawsManager::GetSingleton().Subscribe("sigClearInventorySlots", slotname);
        }
    }

    // Ask the server to send us the inventory
    inventoryCache = psengine->GetInventoryCache();
    if (!inventoryCache)
        return false;

    if ( !inventoryCache->GetInventory() )
    {
        inventoryCache->SetCacheStatus(psCache::INVALID);
        return false;
    }

    return true;
}

bool pawsInventoryWindow::SetupDoll()
{
    pawsObjectView* widget = dynamic_cast<pawsObjectView*>(FindWidget("InventoryDoll"));
    if (widget)
    {
        GEMClientActor* actor = psengine->GetCelClient()->GetMainPlayer();
        if (!actor)
            return false;

        while(!widget->ContinueLoad())
        {
            csSleep(100);
        }

        // Set the doll view
        while(!widget->View(actor->GetFactName()))
        {
            csSleep(100);
        }

        CS_ASSERT(widget->GetObject()->GetMeshObject());

        // Set the charApp.
        widget->SetCharApp(charApp);

        // Register this doll for updates
        widget->SetID(actor->GetEID().Unbox());

        csRef<iSpriteCal3DState> spstate = scfQueryInterface<iSpriteCal3DState> (widget->GetObject()->GetMeshObject());
        if (spstate)
        {
            // Setup cal3d to select random 0 velocity anims
            spstate->SetVelocity(0.0,&psengine->GetRandomGen());
        }

        charApp->Clone(actor->CharAppearance());
        charApp->SetMesh(widget->GetObject());

        charApp->ApplyTraits(actor->traits);
        charApp->ApplyEquipment(actor->equipment);
    }
    return true;
}

bool pawsInventoryWindow::OnMouseDown( int button, int keyModifier, int x, int y )
{
    // Check to see if we are dropping an item
    if ( psengine->GetSlotManager() && psengine->GetSlotManager()->IsDragging() )
    {
        psengine->GetSlotManager()->CancelDrag();
        return true;
    }
    return pawsControlledWindow::OnMouseDown(  button, keyModifier, x, y );
}


bool pawsInventoryWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifer*/, pawsWidget* widget)
{
    // Check to see if this was the view button.
    if ( widget->GetID() == VIEW_BUTTON )
    {
        if ( psengine->GetSlotManager()->IsDragging() )
        {
            psViewItemDescription out(psengine->GetSlotManager()->HoldingContainerID(),
                                      psengine->GetSlotManager()->HoldingSlotID());
            out.SendMessage();

            psengine->GetSlotManager()->CancelDrag();
        }

        return true;
    }

    return true;
}

void pawsInventoryWindow::Close()
{
    Hide();
}

pawsSlot* pawsInventoryWindow::GetFreeSlot()
{
    for ( size_t n = 0; n < bulkSlots.GetSize(); n++ )
    {
        if ( bulkSlots[n] && bulkSlots[n]->IsEmpty() )
        {
            return bulkSlots[n];
        }
    }

    return NULL;
}


void pawsInventoryWindow::Dequip( const char* itemName )
{
    pawsListBox* bulkList = dynamic_cast <pawsListBox*> (FindWidget("BulkList"));
    if ( (itemName != NULL) && (bulkList) )
    {
        pawsSlot* fromSlot = NULL;
        // See if we can find the item in the equipment slots.
        for ( size_t z = 0; z < equipmentSlots.GetSize(); z++ )
        {
            if ( equipmentSlots[z] && !equipmentSlots[z]->IsEmpty() )
            {
                csString tip(equipmentSlots[z]->GetToolTip());
                if ( tip.CompareNoCase(itemName) )
                {
                    fromSlot = equipmentSlots[z];
                    break;
                }
            }
        }

        if ( fromSlot == NULL ) // if item was not found, look in slotnames
            fromSlot = dynamic_cast <pawsSlot*> (FindWidget(itemName));

        if ( fromSlot )
        {
            int container   = fromSlot->ContainerID();
            int slot        = fromSlot->ID();
            int stackCount  = fromSlot->StackCount();

            pawsSlot* freeSlot = GetFreeSlot();
            if ( freeSlot )
            {

                // Move from the equipped slot to an empty slot
                // this message isn't being processed until after all the 'dequip' messages are generated...
                psSlotMovementMsg msg( container, slot,
                                       freeSlot->ContainerID() ,
                                       freeSlot->ID(),
                                       stackCount );
                freeSlot->Reserve();
                msg.SendMessage();
                fromSlot->Clear();
            }
        }
    }
}

bool pawsInventoryWindow::Equip( const char* itemName, int stackCount, int toSlotID )
{
    csArray<psInventoryCache::CachedItemDescription*>::Iterator iter = psengine->GetInventoryCache()->GetSortedIterator();
    psInventoryCache::CachedItemDescription* from = NULL;

    // Search cache for item to equip
    while (iter.HasNext())
    {
        psInventoryCache::CachedItemDescription* slotDesc = iter.Next();

        if (slotDesc->name.CompareNoCase(itemName))
        {
            if (slotDesc->containerID == CONTAINER_INVENTORY_BULK)
            {
                // We found an item with a matching name.
                // If the stack count matches the desired amount, we are done.
                // Otherwise, keep looking for a better match.
                from = slotDesc;
                if (slotDesc->stackCount == stackCount)
                    break;
            }
        }
    }

    if (from)
    {
        int container   = from->containerID;
        int slot        = from->slot;

        // Since we are about to move the item from inventory to the
        // equipped slots, move it in the cache so we don't find it again.
        // The server will soon update our inventory and equipped slots
        // so this won't be noticed except by shortcut commands that try
        // to search the cached inventory again before the server update.
        psengine->GetInventoryCache()->MoveItem( container, slot,
                CONTAINER_INVENTORY_EQUIPMENT, toSlotID, stackCount );

        // Adjust container and slot ID for message format.
        slot -= PSCHARACTER_SLOT_BULK1;
        container = (INVENTORY_SLOT_NUMBER)(slot / 100);
        slot = slot % 100;

        psSlotMovementMsg msg( container, slot,
                               CONTAINER_INVENTORY_EQUIPMENT, toSlotID,
                               stackCount );
        msg.SendMessage();
        return true;
    }
    return false;
}

//search for items in bulk, then in equipped slots
void pawsInventoryWindow::Write( const char* itemName )
{
    pawsListBox* bulkList = dynamic_cast <pawsListBox*> (FindWidget("BulkList"));
    if ( (itemName != NULL) && (bulkList) )
    {
        pawsSlot* fromSlot = NULL;
        for ( size_t z = 0; z < bulkSlots.GetSize(); z++ )
        {
            if ( !bulkSlots[z]->IsEmpty() )
            {
                csString tip(bulkSlots[z]->GetToolTip());
                if ( tip.CompareNoCase(itemName) )
                {
                    fromSlot = bulkSlots[z];
                    break;
                }
            }
        }

        if( fromSlot == NULL)
        {
            // See if we can find the item in the equipment slots.
            for ( size_t z = 0; z < equipmentSlots.GetSize(); z++ )
            {
                if ( equipmentSlots[z] && !equipmentSlots[z]->IsEmpty() )
                {
                    csString tip(equipmentSlots[z]->GetToolTip());
                    if ( tip.CompareNoCase(itemName) )
                    {
                        fromSlot = equipmentSlots[z];
                        break;
                    }
                }
            }

            if ( fromSlot == NULL ) // if item was not found, look in slotnames
                fromSlot = dynamic_cast <pawsSlot*> (FindWidget(itemName));
        }

        if ( fromSlot )
        {
            printf("Found item %s to write on\n", itemName);
            int container   = fromSlot->ContainerID();
            int slot        = fromSlot->ID();

            //psItem* item = charData->GetItemInSlot( slot );
            psWriteBookMessage msg(slot, container);
            msg.SendMessage();
        }
    }

}
