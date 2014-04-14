/*
 * pawsitemdescriptionwidow.cpp - Author: Andrew Craig
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
#include "globals.h"

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// CLIENT INCLUDES
#include "pscelclient.h"

// PAWS INCLUDES
#include "pawsitemdescriptionwindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/log.h"
#include "net/connection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsItemDescriptionWindow::pawsItemDescriptionWindow()
{
}

pawsItemDescriptionWindow::~pawsItemDescriptionWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_VIEW_ITEM);
}

bool pawsItemDescriptionWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_VIEW_ITEM);

    // Store some of our children for easy access later on.
    
    name = dynamic_cast<pawsTextBox*> (FindWidget("ItemName"));
    if ( !name ) return false;

    description = dynamic_cast<pawsMultiLineTextBox*> (FindWidget("ItemDescription"));
    if ( !description ) return false;

    pic = FindWidget("ItemImage");
    if ( !pic ) return false;

    return true;
}

void pawsItemDescriptionWindow::HandleMessage( MsgEntry* me )
{   
    Show();
    psViewItemDescription mesg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());
    description->SetText( mesg.itemDescription );
    csString nameStr;
    nameStr = mesg.itemName;
    nameStr.AppendFmt( " (%u)",mesg.stackCount);
    name->SetText( nameStr );
    if ( mesg.itemIcon != NULL )
        pic->SetBackground( mesg.itemIcon );

}

bool pawsItemDescriptionWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    csString widgetName(widget->GetName());
    if ( widgetName == "StudyButton" )
    {
        pawsWidget* widget = PawsManager::GetSingleton().FindWidget("CraftWindow");
        if ( widget )
            widget->Show();
        return true;        
    }

    // Since it is not study button we can easily just close the window.
    Hide();
    PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
    return true;
}


void pawsItemDescriptionWindow::ShowItemDescription(csString &desc, csString &itemName, csString &icon)
{
    Show();
    description->SetText( desc );     
    name->SetText( itemName );       
    pic->SetBackground( icon );    
}

void pawsItemDescriptionWindow::ShowItemDescription(csString &desc, csString &itemName, csString &icon, int count)
{
    csString displayName;
    displayName.Format("%s (%d)",itemName.GetData(),count);
    ShowItemDescription(desc,displayName,icon);
}
