/*
 * inventory.h - Author: Andrew Craig
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

#ifndef PAWS_INVENTORY_WINDOW
#define PAWS_INVENTORY_WINDOW

// CS INCLUDES
#include <csutil/array.h>
#include <csutil/leakguard.h>
#include <imap/loader.h>

#include "psslotmgr.h"
#include "rpgrules/psmoney.h"

#include "paws/pawsmanager.h"
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawsnumberpromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "net/cmdbase.h"
#include "../psinventorycache.h"


struct iEngine;
class pawsMoney;
class pawsSlot;
class pawsInventoryDollView;
class psCharAppearance;

//---------------------------------------------------------------------------

/** The is the main Inventory window for PlaneShift. It handles all the player's
 * inventory and has a nice 3d model of the player to see what they looked like in
 * their equipment.
 */
class pawsInventoryWindow  : public pawsControlledWindow
{
public:
    pawsInventoryWindow();
    ~pawsInventoryWindow();
        
    /** From pawsWidget
     */
    bool PostSetup();
    void Show();
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    virtual bool OnMouseDown( int button, int keyModifier, int x, int y );    
    virtual void Close();

    /// Get a free slot
    pawsSlot* GetFreeSlot();

    /** Equips an item into it's closest available slot.
     *  Will pick the first item of the given name in the bulk slots to try
     *  to equip. In this sense drinking a potion is the same as 'equipping' it.
     *
     *  @param itemName The name of the item in the inventory.
     *  @param stackCount The amount to try to equip.  
     *  @param toSlotID The slot you want to equip it to.
     */
    bool Equip( const char* itemName, int stackCount, int toSlotID = -1 );
    
    
    /** Dequips an item into closest available bulk slot. 
     *  Will pick the first item of the given name in an equipment slot  to try
     *  to move to bulk. Will remove all items from the equipped slot.
     *
     *  @param itemName The name of the item in the inventory.      
     */    
    void Dequip(const char* itemName);
    
    /** Finds an item with the given name and attempts to open it as a book
     *  to write on.
     *  @param itemName The name of the item in the inventory.  
     */
    
    void Write( const char* itemName);

    
protected:

    bool SetupDoll();

    /// Helper function to setup the pawsSlot and container definitions.
    bool SetupSlot( const char* slot );
    
    /// A quick pointer to the object view that is the mesh doll.
    pawsInventoryDollView* view;
    
    void UpdateMoney( const char* moneyName, const char* imageName, int value );
    
    csRef<iThreadedLoader> loader;

    /// Total items to drop
    int maxDropCount;

    pawsTextBox* trias;
    pawsTextBox* weight;

    pawsMoney * money;
   
    csArray<pawsSlot*> bulkSlots;
    csArray<pawsSlot*> equipmentSlots;

private:
    psInventoryCache* inventoryCache;
    psCharAppearance* charApp;
      
};

CREATE_PAWS_FACTORY( pawsInventoryWindow );

//--------------------------------------------------------------------------



#endif 

