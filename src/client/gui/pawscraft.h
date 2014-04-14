/*
 * pawscraft.h - Author: Andrew Craig <acraig@planeshift.it> 
 *
 * Copyright (C) 2003-2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_CRAFT_WINDOW_HEADER
#define PAWS_CRAFT_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "net/cmdbase.h"

class pawsSimpleTree;
class pawsMultiLineTextBox;

/** Window Widget that displays information about the mind item to 
 *  be used in crafting.
 *
 * This handles the incoming network traffic and displays the information 
 * for the client in a tree about all the items available in the current
 * mind item.
 */
class pawsCraftWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    bool PostSetup(); 
    virtual void Show();
    void HandleMessage( MsgEntry* me );
    bool OnSelected(pawsWidget* widget);
/*    
    struct TreeNode
    {
        csString name;
        csString equipment;
        csString workItem;
        int count;
    };
*/    
    
protected:
//    csPDelArray< TreeNode > nodes;
    pawsMultiLineTextBox* textBox;
        
//    pawsSimpleTree* itemTree;
    
//    void Add( const char* parent, const char* realParent, psMsgCraftingInfo::CraftingItem* item );
    
};

CREATE_PAWS_FACTORY( pawsCraftWindow );


#endif
