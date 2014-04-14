/*
 * smallinventory.h - Author: Andrew Craig
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_SMALL_INVENTORY_WINDOW
#define PAWS_SMALL_INVENTORY_WINDOW

#include "net/cmdbase.h"
#include "paws/pawsmanager.h"
#include "paws/pawswidget.h"
#include "rpgrules/psmoney.h"
#include "gui/pawscontrolwindow.h"

class pawsSlot;
class pawsMoney;

/** A small version of the inventory window. 
    That just shows the items that are in the bulk slot. Useful for 
    places like adding stuff to containers where you don't need the full 
    inventory screen.
*/
class pawsSmallInventoryWindow  : public pawsWidget
{
public:
    pawsSmallInventoryWindow();
    
    
    virtual bool PostSetup();
    virtual void Show();
private:    
    csArray<pawsSlot*>  bulkSlots; 
    void UpdateMoney( const char* moneyName, const char* imageName, int value );
        
    pawsMoney * money;

    bool OnMouseDown( int button, int keyModifier, int x, int y );
    bool OnButtonPressed( int mouseButton, int keyModifer, pawsWidget* widget );

};

CREATE_PAWS_FACTORY( pawsSmallInventoryWindow );

#endif

