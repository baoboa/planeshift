/*
 * pawsinventorydollview.cpp
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "net/message.h"
#include "net/clientmsghandler.h"
#include "globals.h"

#include "gui/pawsslot.h"
#include "gui/pawsexchangewindow.h"
#include "gui/pawsinventorydollview.h"

#include "paws/pawsmanager.h"
#include "paws/pawsobjectview.h"
#include "paws/pawswidget.h"

/**********************************************************************
*
*                        class pawsInventoryDollView
*
***********************************************************************/

bool pawsInventoryDollView::OnMouseDown(int /*button*/, int /*modifiers*/, int /*x*/, int /*y*/)
{
    // Make sure we actually have something
    if ( !psengine->GetSlotManager()->IsDragging() )
        return true;
       
    pawsSlot *draggingSlot = (pawsSlot*) PawsManager::GetSingleton().GetDragDropWidget(); 
    //Client* client = clients->Find(me->clientnum);    
    //psCharacter *chardata=client->GetCharacterData();
    
    int container = psengine->GetSlotManager()->HoldingContainerID();
    int slot = psengine->GetSlotManager()->HoldingSlotID();
    int count = draggingSlot->StackCount();

    // Let server know movement of item to doll
    csRef<MsgHandler> msgHandler = psengine->GetMsgHandler();
    psSlotMovementMsg msg(container, slot, CONTAINER_INVENTORY_EQUIPMENT, PSCHARACTER_SLOT_NONE, count);
    PawsManager::GetSingleton().SetDragDropWidget(NULL);
    
    // Let the slot manager know that we are not dragging items anymore.
    psengine->GetSlotManager()->SetDragFlag(false);
    msgHandler->SendMessage( msg.msg );                               
    
    return true;
}

