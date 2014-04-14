 /*
 * pawscraft.cpp - Author: Andrew Craig <acraig@planeshift.it> 
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
 
 
 #include <psconfig.h>

#include "globals.h"


#include "paws/pawstree.h"
#include "paws/pawstextbox.h"
#include "net/clientmsghandler.h"
#include "pawscraft.h"

  
bool pawsCraftWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CRAFT_INFO);
    return true;
}

void pawsCraftWindow::Show()
{
    pawsWidget::Show();        

    // Get craft info for mind item
    psMsgCraftingInfo msg;
    msg.SendMessage();
}   


void pawsCraftWindow::HandleMessage( MsgEntry* me )
{
    textBox = dynamic_cast<pawsMultiLineTextBox*>(FindWidget("HelpText"));
    psMsgCraftingInfo mesg(me);
    csString text(mesg.craftInfo);
    text.ReplaceAll( "[[", "   With higher ");
    text.ReplaceAll( "]]", " skill you could: " );
    if (text)
        textBox->SetText(text.GetData());
}

/*
void pawsCraftWindow::Add( const char* parent, const char* realParent, psMsgCraftingInfo::CraftingItem* item )
{
    csString name(parent);
    name.Append("$");
    name.Append(item->name);
    
    itemTree->InsertChildL( parent, name, item->name, "" );
    TreeNode* node = new TreeNode;
    node->name = name;
    node->count = item->count;
    node->equipment = item->requiredEquipment;
    node->workItem = item->requiredWorkItem;
    nodes.Push( node );
            
    for ( size_t z = 0; z <  item->subItems.GetSize(); z++ )
    {
        Add( name, item->name, item->subItems[z] );
    }        
}
*/

bool pawsCraftWindow::OnSelected(pawsWidget* /*widget*/)
{
/*
    pawsTreeNode* node = static_cast<pawsTreeNode*> (widget);
    
    for ( size_t z = 0; z < nodes.GetSize(); z++ )
    {
        if ( nodes[z]->name == node->GetName() )
        {
            csString text("");
            csString dummy;
            if ( nodes[z]->count > 0 )
            {
                dummy.Format("Count: %d\n", nodes[z]->count );
                text.Append(dummy);            
            }
            
            if ( nodes[z]->equipment.Length() > 0 )
            {
                dummy.Format("Required Equipment: %s\n", nodes[z]->equipment.GetData() );
                text.Append(dummy);
            }
                            
            if ( nodes[z]->workItem.Length() > 0 )
            {            
                dummy.Format("Required Work Item: %s\n", nodes[z]->workItem.GetData() );                
                text.Append(dummy);
            }                
            textBox->SetText( text );
        }
    }
    return false;        
*/       
    return true;
}
