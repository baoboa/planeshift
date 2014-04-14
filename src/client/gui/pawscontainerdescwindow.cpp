/*
 * pawscontainerdescriptionwidow.cpp - Author: Thomas Towey
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

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// COMMON INCLUDES
#include "net/connection.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "psclientchar.h"

// PAWS INCLUDES
#include "pawscontainerdescwindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "inventorywindow.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

#include "util/log.h"
#include "gui/pawsslot.h"

#include "globals.h"

// BUTTONS AND SLOTS
#define VIEW_BUTTON 11
#define INVENTORY_BUTTON 12
#define COMBINE_BUTTON 13
#define UNCOMBINE_BUTTON 14

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsContainerDescWindow::pawsContainerDescWindow()
{
    containerID = 0;
    containerSlots = 0;
}

pawsContainerDescWindow::~pawsContainerDescWindow()
{
}

bool pawsContainerDescWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_VIEW_CONTAINER);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_UPDATE_ITEM);

    // Store some of our children for easy access later on.
    name = (pawsTextBox*)FindWidget("ItemName");
    if (!name)
        return false;

    description = dynamic_cast<pawsMultiLineTextBox*> (FindWidget("ItemDescription"));
    if (!description)
        return false;

    pic = (pawsWidget*)FindWidget("ItemImage");
    if (!pic)
        return false;

    // Create bulk slots.
    contents = dynamic_cast <pawsListBox*> (FindWidget("BulkList"));
    if (!contents)
        return false;

    return true;
}

void pawsContainerDescWindow::HandleUpdateItem( MsgEntry* me )
{
    psViewItemUpdate mesg( me, psengine->GetNetManager()->GetConnection()->GetAccessPointers() );
    csString sigData, data;

    // We send ownerID to multiple clients, so each client must decide if the item is owned by
    // them or not.  This is double checked on the server if someone tries to move an item,
    // so hacking this to override just breaks the display, but does not enable a cheat.
    if (mesg.ownerID.IsValid() && mesg.ownerID != psengine->GetCelClient()->GetMainPlayer()->GetEID())
    {
        mesg.stackCount = -1; // hardcoded signal that item is not owned by this player
    }

    sigData.Format("invslot_%d", mesg.containerID.Unbox() * 100 + mesg.slotID);
    if (!mesg.clearSlot)
    {
        data.Format("%s %d %d %s %s %s", mesg.icon.GetData(), mesg.stackCount, 0, mesg.meshName.GetData(), mesg.materialName.GetData(), mesg.name.GetData());
    }

    Debug3(LOG_CHARACTER, 0, "Got item update for %s: %s", sigData.GetDataSafe(), data.GetDataSafe() );

    // FIXME: psViewItemMessages should probably send out purification status

    PawsManager::GetSingleton().Publish(sigData, data);
}

void pawsContainerDescWindow::HandleViewContainer( MsgEntry* me )
{
    Show();
    psViewContainerDescription mesg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());

    description->SetText( mesg.itemDescription );
    name->SetText( mesg.itemName );
    pic->Show();
    pic->SetBackground(mesg.itemIcon);
    if (pic->GetBackground() != mesg.itemIcon) // if setting the background failed...hide it
        pic->Hide();

    bool newContainer = false;
    if(containerID != mesg.containerID)
    {
        newContainer = true;
    }

    containerID = mesg.containerID;

    if ( mesg.hasContents )
    {
        if (newContainer)
        {
            Debug2(LOG_CHARACTER, 0, "Setting up container %d.", mesg.containerID);

            contents->Clear();
            containerSlots = mesg.maxContainerSlots;

            const int cols = contents->GetTotalColumns(); //6;
            const int rows = (int) ceil(float(containerSlots)/cols);
            for (int i = 0; i < rows; i++)
            {
                pawsListBoxRow* listRow = contents->NewRow(i);
                for (int j = 0; j < cols; j++)
                {
                    pawsSlot* slot = dynamic_cast <pawsSlot*> (listRow->GetColumn(j));
                    CS_ASSERT( slot );

                    if (i * cols + j >= containerSlots)
                    {
                        slot->Hide();
                        continue;
                    }

                    slot->SetContainer(mesg.containerID);
                    slot->SetSlotID(i*cols+j);
                    //slot->SetDefaultToolTip("Empty");

                    // Note that the server adds +16 to the slotID that the
                    // client sends so we subscribe to the slotID+16
                    // but publish without the +16 to compensate.
                    csString slotName;
                    slotName.Format("invslot_%d", mesg.containerID * 100 + i*cols+j + (mesg.containerID < 100 ? 16 : 0));
                    slot->SetSlotName(slotName);
                    Debug3(LOG_CHARACTER, 0, "Container slot %d subscribing to %s.", i*cols+j, slotName.GetData());
                    // New slots must subscribe to sigClear* -before-
                    // invslot_n, or else the cached clear signal will override
                    // the signal with the cached slot data, resulting in an
                    // empty window.
                    PawsManager::GetSingleton().Subscribe(mesg.containerID < 100? "sigClearInventoryContainerSlots" : "sigClearContainerSlots", slot);
                    PawsManager::GetSingleton().Subscribe(slotName, slot);
                }
            }
        }
        PawsManager::GetSingleton().Publish(mesg.containerID < 100? "sigClearInventoryContainerSlots" : "sigClearContainerSlots");
        for (size_t i=0; i < mesg.contents.GetSize(); i++)
        {
            csString sigData, data;
            sigData.Format("invslot_%u", mesg.containerID * 100 + mesg.contents[i].slotID);

            data.Format( "%s %d %d %s %s %s", mesg.contents[i].icon.GetData(),
                         mesg.contents[i].stackCount,
                         mesg.contents[i].purifyStatus,
                         mesg.contents[i].meshName.GetData(),
                         mesg.contents[i].materialName.GetData(),
                         mesg.contents[i].name.GetData() );

            Debug3(LOG_PAWS, 0, "Publishing slot data %s -> %s", sigData.GetData(), data.GetData() );
            PawsManager::GetSingleton().Publish(sigData, data );
        }
        contents->Show();
    }
    else
    {
        contents->Hide();
    }
}

void pawsContainerDescWindow::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_VIEW_CONTAINER:
        {
            HandleViewContainer( me );
            break;
        }
        case MSGTYPE_UPDATE_ITEM:
        {
            HandleUpdateItem( me );
            break;
        }
    }

}

bool pawsContainerDescWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    csString widgetName(widget->GetName());

    if ( widgetName == "SmallInvButton" )
    {
        pawsWidget* widget = PawsManager::GetSingleton().FindWidget("SmallInventoryWindow");
        if ( widget )
            widget->Show();
        return true;
    }
    // Check to see if player attempts to take and stack everything
    else if(widgetName == "TakeStackAll")
    {
        GEMClientObject* oldtarget = psengine->GetCharManager()->GetTarget();
        EID oldID;

        if(oldtarget)
        {
            oldID = oldtarget->GetEID();
        }

        Debug3(LOG_PAWS, 0, "selecting containerID %d, oldID %u", containerID, oldID.Unbox());
        psUserActionMessage setnewtarget(0, containerID, "select");
        setnewtarget.SendMessage();

        // Attempt to grab all items in the container.
        psengine->GetCmdHandler()->Execute("/takestackall");

        psUserActionMessage setoldtarget(0, oldID, "select");
        setoldtarget.SendMessage();
    }
    // Check to see if player attempts to take everything
    else if (widgetName == "TakeAll")
    {
        GEMClientObject* oldtarget = psengine->GetCharManager()->GetTarget();
        EID oldID;

        if(oldtarget)
        {
            oldID = oldtarget->GetEID();
        }

        Debug3(LOG_PAWS, 0, "selecting containerID %d, oldID %u", containerID, oldID.Unbox());
        psUserActionMessage setnewtarget(0, containerID, "select");
        setnewtarget.SendMessage();

        // Attempt to grab all items in the container.
        psengine->GetCmdHandler()->Execute("/takeall");

        psUserActionMessage setoldtarget(0, oldID, "select");
        setoldtarget.SendMessage();
    }
    else if(widgetName == "UseContainer")
    {
        psengine->GetCmdHandler()->Execute("/use");
    }

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
    /* Ahhem! - internal memoír for tzaeru to not forget uncommenting this & completing the bottom most this. */
    else if ( widget->GetID() == INVENTORY_BUTTON )
    {
        if ( psengine->GetSlotManager()->IsDragging() )
        {
            pawsInventoryWindow* inv = (pawsInventoryWindow*)PawsManager::GetSingleton().FindWidget("InventoryWindow");
            pawsSlot* slot = inv->GetFreeSlot();

            if(!slot)
            {
                PawsManager::GetSingleton().CreateWarningBox("Your inventory is full!");
                return true;
            }

            psengine->GetSlotManager()->Handle(slot);
        }

        return true;
    }
    else if ( widget->GetID() == COMBINE_BUTTON || widget->GetID() == UNCOMBINE_BUTTON )
    {
        GEMClientObject* oldtarget = psengine->GetCharManager()->GetTarget();
        EID oldID;
        if(oldtarget)
        {
            oldID = oldtarget->GetEID();
        }
        Debug3(LOG_PAWS, 0, "selecting containerID %d, oldID %u", containerID, oldID.Unbox());
        psUserActionMessage setnewtarget(0, containerID, "select");
        setnewtarget.SendMessage();

        if(widget->GetID() == COMBINE_BUTTON)
            psengine->GetCmdHandler()->Execute("/combine");
        else
            psengine->GetCmdHandler()->Execute("/uncombine");

        psUserActionMessage setoldtarget(0, oldID, "select");
        setoldtarget.SendMessage();
    }

    return true;
}
