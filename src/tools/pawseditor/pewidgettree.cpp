/*
* pewidgettree.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : control tree for properties editing
*
*/

#define NODE_WIDGET2_COLOUR                        0xff00ff
#define NODE_WIDGET_COLOUR                      0xff33cc
#define NODE_ANCHOR_ATTR_COLOUR                 0xff6699
#define NODE_ANCHOR_KEYFRAME_ROOT_COLOUR        0xff9966 
#define NODE_ANCHOR_KEYFRAME_COLOUR             0xffcc33
#define NODE_ANCHOR_KEYFRAME_ATTR_COLOUR        0xffff00

#include <psconfig.h>
#include "pawseditorglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "paws/pawstree.h"
#include "pewidgettree.h"

bool peWidgetTree::PostSetup()
{
    widgetItemsTree = (pawsTree *)FindWidget("WidgetItemsTree");
    if (!widgetItemsTree)
    {
        printf("Warning: Widget Tree toolbox does not have an WidgetItemsTree defined!\n");
        return false;
    }
    widgetItemsTree->SetNotify(this);
    widgetItemsTree->SetScrollBars(false, true);

    return true;
}

bool peWidgetTree::OnSelected( pawsWidget* widget)
{
    return true;
}

void peWidgetTree::LoadWidget(iDocumentNode* widgetNode)
{
    widgetItemsTree->Clear();
    
    widgetItemsRoot = new pawsSimpleTreeNode();
    widgetItemsTree->InsertChild("", widgetItemsRoot, "");

    csRef<iDocumentNodeIterator> iter = widgetNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        // This is a widget so read it's factory to create it.
        if ( strcmp( node->GetValue(), "widget" ) == 0 )
        {
            csString name;
            if(node->GetAttribute( "name" ))
                name = node->GetAttribute( "name" )->GetValue();
            
            widgetItemsRoot->Set(showLabel,false,"",name );
            widgetItemsRoot->SetColour(NODE_WIDGET_COLOUR);
            widgetItemsRoot->SetName( name );

            BuildWidgetTree( widgetItemsRoot, node );
        }
    }
}

void peWidgetTree::BuildWidgetTree(pawsSimpleTreeNode *parent, iDocumentNode* widgetNode)
{
    csString name;
    csRef<iDocumentNodeIterator> iter = widgetNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        pawsSimpleTreeNode *widgetItem = new pawsSimpleTreeNode();
        parent->InsertChild(widgetItem);
        if ( strcmp( node->GetValue(), "widget" ) == 0 )
        {
            if(node->GetAttribute( "name" ))
                name = node->GetAttribute( "name" )->GetValue();
            else
                name = "<missing name>";
            widgetItem->SetColour(NODE_WIDGET_COLOUR);
        }
        else
        {
            name = node->GetValue();
        }
        widgetItem->Set(showLabel,false,"",name );
        widgetItem->SetName( name );

        BuildWidgetTree( widgetItem, node );
    }
}

