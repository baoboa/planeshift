/*
 * pawssmallinventory.cpp 
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "pawssmallinventory.h"
#include "paws/pawsborder.h"
#include "paws/pawslistbox.h"
#include "util/log.h"
#include "util/psconst.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "gui/pawsslot.h"
#include "gui/pawsmoney.h"

#include "globals.h"


pawsSmallInventoryWindow::pawsSmallInventoryWindow()
{
    bulkSlots.SetSize( INVENTORY_BULK_COUNT );
   
}                         

void pawsSmallInventoryWindow::Show()
{
    if(!IsVisible())//ask to get the inventory only if the window was previously invisible.
    {
        // Ask the server to send us the inventory
        psGUIInventoryMessage request;
        request.SendMessage();
    }
        pawsWidget::Show();
}

bool pawsSmallInventoryWindow::PostSetup()
{

    if ( !border )
    {
        Error1( "Small Inventory Window expects border=\"yes\" in xml" );
        return false;
    }
    else
    {       
        border->JustTitle();
        
        // Setup our inventory slots in the list box.
        pawsListBox * bulkList = dynamic_cast <pawsListBox*> (FindWidget("BulkList"));
        if(bulkList)
        {
            int colCount = bulkList->GetTotalColumns();
            int rowCount = (int) ceil(float(INVENTORY_BULK_COUNT)/colCount);

            for(int r = 0; r < rowCount; r ++)
            {
                pawsListBoxRow * listRow = bulkList->NewRow(r);
                for (int j = 0; j < colCount; j++)
                {
                    int i = r*colCount + j;
                    pawsSlot * slot;
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
        money = dynamic_cast <pawsMoney*> (FindWidget("Money")); 
        if (money)
        {
            money->SetContainer(CONTAINER_INVENTORY_MONEY);
        }
    }
    return true;    
}

bool pawsSmallInventoryWindow::OnMouseDown( int button, int keyModifier, int x, int y )
{
    // Check to see if we are dropping an item
    if ( psengine->GetSlotManager() && psengine->GetSlotManager()->IsDragging() )
    {
        psengine->GetSlotManager()->CancelDrag();
        return true;
    }
    return pawsWidget::OnMouseDown(  button, keyModifier, x, y );
}

bool pawsSmallInventoryWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifer*/, pawsWidget* /*widget*/)
{
    return true;
}

