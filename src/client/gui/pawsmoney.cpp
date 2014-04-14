/*
 * pawsmoney.cpp - Author: Ondrej Hurt
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


// CS INCLUDES
#include <psconfig.h>

// COMMON INCLUDES
#include "util/localization.h"
#include "util/log.h"
#include "util/psxmlparser.h"
#include "rpgrules/psmoney.h"

// PAWS INCLUDES
#include "pawsmoney.h"
#include "paws/pawsmanager.h"
#include "inventorywindow.h"


#define MONEY_FILE_NAME  "money.xml"
#define SLOT_SIZE       48

csRef<iDocumentNode> FindFirstWidget(iDocument * doc);


/**************************************************************************
*
*                        class pawsMoney
*
***************************************************************************/

pawsMoney::pawsMoney()
{
    circles   =   NULL;
    octas     =   NULL;
    hexas     =   NULL;
    trias     =   NULL;
    amount    =   0;
}

bool pawsMoney::Setup(iDocumentNode * node)
{
    csString borderStr;
    
    border = node->GetAttributeValueAsBool("border");
    
    spacing = node->GetAttributeValueAsInt("spacing");
    return CreateGUI();
}

bool pawsMoney::CreateGUI()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> widgetNode;

    doc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), PawsManager::GetSingleton().GetLocalization()->FindLocalizedFile(MONEY_FILE_NAME));
    if (doc == NULL)
    {
        Error2("File %s not found", MONEY_FILE_NAME);
        return false;
    }
    widgetNode = FindFirstWidget(doc);
    if (widgetNode == NULL)
    {
        Error2("Failed to load children from %s", MONEY_FILE_NAME);
        return false;
    }
    
    // This is a weird case because we need to remember the left,top from the original widget spec
    // but the width and height from this one
    csRect rect=this->GetDefaultFrame(); // save them here
    if (!LoadAttributes(widgetNode))
        return false;
    this->SetRelativeFramePos(rect.xmin,rect.ymin); // restore them here

    if ( ! LoadChildren(widgetNode) )
    {
        Error2("Failed to load children from %s", MONEY_FILE_NAME);
        return false;
    }
    
    // SetRelativeFrameSize(GetActualWidth(100), GetActualHeight(100));
    return true;
}

void pawsMoney::SetContainer(int container)
{
    if (!trias) return;

    trias->SetContainer( container );
    hexas->SetContainer( container );
    octas->SetContainer( container );
    circles->SetContainer( container );
    
    circles->SetSlotID( MONEY_CIRCLES );
    octas->SetSlotID( MONEY_OCTAS ); 
    hexas->SetSlotID( MONEY_HEXAS ); 
    trias->SetSlotID( MONEY_TRIAS ); 
}


bool pawsMoney::PostSetup()
{
    circles = dynamic_cast <pawsSlot*>(FindWidget("Circles"));
    if (circles == NULL)
        return false;
    circles->SetEmptyOnZeroCount(false);
    circles->PlaceItem("MoneyCircles", "money#circle01a_pile_01");
    circles->SetSlotID(MONEY_CIRCLES);
    //if (border) 
    //    circles->SetBackground("Bulk Item Slot");
            
    octas = dynamic_cast <pawsSlot*>(FindWidget("Octas"));
    if (octas == NULL)
        return false;
    octas->SetEmptyOnZeroCount(false);                
    octas->PlaceItem("MoneyOctas", "money#octa_01a_pile_01_01");
    octas->SetSlotID(MONEY_OCTAS);
    //if (border) 
    //    octas->SetBackground("Bulk Item Slot");
    
    hexas = dynamic_cast <pawsSlot*>(FindWidget("Hexas"));
    if (hexas == NULL)
        return false;
    hexas->SetEmptyOnZeroCount(false);        
    hexas->PlaceItem("MoneyHexas", "money#hexa_01a_pile_01_01");
    hexas->SetSlotID(MONEY_HEXAS);   
    //if (border) hexas->SetBackground("Bulk Item Slot");
    
    trias = dynamic_cast <pawsSlot*>(FindWidget("Trias"));
    if (trias == NULL)
        return false;
    trias->SetEmptyOnZeroCount(false);        
    trias->PlaceItem("MoneyTrias", "money#tria_01a_pile_01_01");
    trias->SetSlotID(MONEY_TRIAS);     
    //if (border) trias->SetBackground("Bulk Item Slot");
    
    return true;
}

void pawsMoney::Draw()
{
    pawsWidget::Draw();
}

void pawsMoney::Drag( bool dragOn )
{
    trias->SetDrag( dragOn );
    hexas->SetDrag( dragOn );
    octas->SetDrag( dragOn );
    circles->SetDrag( dragOn );
}

void pawsMoney::Set(int circles, int octas, int hexas, int trias)
{
    if (!this->trias) return;

    this->  circles  ->StackCount(circles);
    this->  octas    ->StackCount(octas);
    this->  hexas    ->StackCount(hexas);
    this->  trias    ->StackCount(trias);

    RecalculateAmount();
}

void pawsMoney::Set(const psMoney & money)
{
    Set( money.GetCircles(), money.GetOctas(), money.GetHexas(), 
        money.GetTrias() );
}

void pawsMoney::Set(int coinType, int count)
{
    switch (coinType)
    {
        case MONEY_CIRCLES:  circles  -> StackCount(count); break;
        case MONEY_OCTAS:    octas    -> StackCount(count); break;
        case MONEY_HEXAS:    hexas    -> StackCount(count); break;
        case MONEY_TRIAS:    trias    -> StackCount(count); break;
        default: CS_ASSERT(0);
    }

    this->RecalculateAmount();
}

void pawsMoney::Get(int & circles, int & octas, int & hexas, int & trias)
{
    circles  =  this->  circles  ->StackCount();
    octas    =  this->  octas    ->StackCount();
    hexas    =  this->  hexas    ->StackCount();
    trias    =  this->  trias    ->StackCount();
}

int pawsMoney::Get(int coinType)
{
    switch (coinType)
    {
        case MONEY_CIRCLES:  return  circles  -> StackCount(); break;
        case MONEY_OCTAS:    return  octas    -> StackCount(); break;
        case MONEY_HEXAS:    return  hexas    -> StackCount(); break;
        case MONEY_TRIAS:    return  trias    -> StackCount(); break;
        default: CS_ASSERT(0);
    }
    return 0;
}

bool pawsMoney::IsNoAmount()
{
    if (amount < 1)
        return true;
    
    return false;
}

void pawsMoney::RecalculateAmount()
{
    amount  = this->trias->StackCount();
    amount += this->hexas->StackCount()   * 10;
    amount += this->octas->StackCount()   * 50;
    amount += this->circles->StackCount() * 250;
}

psMoney pawsMoney::Get()
{
    psMoney money(Get(MONEY_CIRCLES), Get(MONEY_OCTAS), Get(MONEY_HEXAS), Get(MONEY_TRIAS));
    return money;
}


void pawsMoney::OnUpdateData(const char* /*dataname*/, PAWSData& value)
{
    psMoney money( value.GetStr() );
    Set( money );            
}
