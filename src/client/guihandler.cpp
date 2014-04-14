/*
* guihandler.cpp
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*           Keith Fulton <keith@planeshift.it>
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
* Creation Date: 4/20/2005
* Description : Handles network messages to and from the GUI, using PAWS pub/sub system.
*
*/
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/objreg.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/connection.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "paws/pawsmanager.h"
#include "guihandler.h"
#include "globals.h"
#include "psinventorycache.h"


GUIHandler::GUIHandler() 
           : psCmdBase(NULL,NULL,psengine->GetObjectRegistry())
{
    // Subscribe our message types that we are interested in. 
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_GUIINVENTORY );

    inventoryCache = psengine->GetInventoryCache();
}


void GUIHandler::HandleMessage(MsgEntry* me)
{
    switch( me->GetType() )
    {
        case MSGTYPE_GUIINVENTORY:
            HandleInventory(me);
            break;
    }                    
}

void GUIHandler::HandleInventory(MsgEntry* me)
{
    psGUIInventoryMessage incoming(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());

    // drop inventory list, if its version is older than the current.
    // this may happen due to UDP latency.
    if (inventoryCache->GetInventoryVersion() >= incoming.version)
        return;
    
    // Set new version in any case (LIST or UPDATE_LIST)
    inventoryCache->SetInventoryVersion(incoming.version);

    // merge the received inventory list into the client's inventory cache
    if (incoming.command == psGUIInventoryMessage::LIST)   // for the whole lot
    {
        inventoryCache->EmptyInventory();   // empty the inventory first
        inventoryCache->SetCacheStatus(psCache::VALID);
    }

    float itemWeight = 0;
    float totalSize = 0;

    for ( size_t z = 0; z < incoming.totalItems; z++ )  // cache inventory items
    {
        inventoryCache->SetInventoryItem(incoming.items[z].slot,
                                         incoming.items[z].container,
                                         incoming.items[z].name,
                                         incoming.items[z].meshName,
                                         incoming.items[z].materialName,
                                         incoming.items[z].weight,
                                         incoming.items[z].size,
                                         incoming.items[z].stackcount,
                                         incoming.items[z].iconImage,
                                         incoming.items[z].purifyStatus);

        itemWeight += incoming.items[z].weight;
        totalSize  += incoming.items[z].size;
    }
    if (incoming.command == psGUIInventoryMessage::UPDATE_LIST)
    {
        for ( size_t z = 0; z < incoming.totalEmptiedSlots; z++)
        {
            inventoryCache->EmptyInventoryItem( incoming.items[incoming.totalItems+z].slot,
                                                incoming.items[incoming.totalItems+z].container);
        }
    }


    // TODO: Calculate total weight

    PawsManager::GetSingleton().Publish( "fcurrcap", totalSize );         
    PawsManager::GetSingleton().Publish( "sigInvMoney", incoming.money.ToString() );         
    PawsManager::GetSingleton().Publish( "sigInvMoneyTotal", incoming.money.GetTotal() );
    csString weight = csString().Format("%.2f/%.2f",itemWeight, incoming.maxWeight);
    PawsManager::GetSingleton().Publish( "sigInvWeightTotal", weight );                
}

