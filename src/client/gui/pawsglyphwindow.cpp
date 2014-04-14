/*
 * glyphwindow.cpp - Author: Anders Reggestad
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

// CS INCLUDES
#include <csutil/util.h>
#include <iutil/evdefs.h>

// COMMON INCLUDES
#include "net/message.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"
#include "util/strutil.h"
#include "util/log.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"
#include "gui/psmainwidget.h"

// PAWS INCLUDES
#include "paws/pawstextbox.h"
#include "paws/pawsprefmanager.h"
#include "pawsglyphwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsobjectview.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawslistbox.h"


#define PURIFY_BUTTON 1000
#define SAVE_BUTTON   1001
#define CAST_BUTTON   1002
#define RESEARCH_BUTTON   1003



//////////////////////////////////////////////////////////////////////
//
//                      pawsGlyphWindow
//
//////////////////////////////////////////////////////////////////////

pawsGlyphWindow::pawsGlyphWindow()

{
    for (int i = 0; i < GLYPH_ASSEMBLER_SLOTS; i++)
        assembler[i] = NULL;
}


pawsGlyphWindow::~pawsGlyphWindow()
{
}

void pawsGlyphWindow::Show()
{
    if(!IsVisible())
    {
        // Ask the server to send us the glyphs
        psRequestGlyphsMessage msg;
        msg.SendMessage();
    }
    pawsWidget::Show();
}

void pawsGlyphWindow::Hide()
{
    pawsGlyphSlot * floatingSlot = dynamic_cast <pawsGlyphSlot*> (PawsManager::GetSingleton().GetDragDropWidget());
    if ( floatingSlot!=NULL )
        PawsManager::GetSingleton().SetDragDropWidget(NULL);

    pawsWidget::Hide();
}

bool pawsGlyphWindow::PostSetup()
{
    // Subscribe our message types that we are interested in.
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GLYPH_REQUEST);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GLYPH_ASSEMBLE);

    description = dynamic_cast <pawsMessageTextBox*> (FindWidget("SpellDescription"));
    if ( !description ) return false;

    spellName = dynamic_cast <pawsTextBox*> (FindWidget("SpellName"));
    if ( !spellName ) return false;
    
    spellImage = FindWidget("SpellImage");
    if ( !spellImage ) return false;

    ways.SetSize(GLYPH_WAYS);

    ways[0] = dynamic_cast <pawsListBox*> (FindWidget("crystal"));
    if ( !ways[0] ) return false;

    ways[1] = dynamic_cast <pawsListBox*> (FindWidget("blue"));
    if ( !ways[1] ) return false;

    ways[2] = dynamic_cast <pawsListBox*> (FindWidget("azure"));
    if ( !ways[2] ) return false;

    ways[3] = dynamic_cast <pawsListBox*> (FindWidget("brown"));
    if ( !ways[3] ) return false;

    ways[4] = dynamic_cast <pawsListBox*> (FindWidget("red"));
    if ( !ways[4] ) return false;

    ways[5] = dynamic_cast <pawsListBox*> (FindWidget("dark"));
    if ( !ways[5] ) return false;

    for (int i = 0; i < GLYPH_WAYS; i++)
    {
        NewGlyphSlotRow(i); // start with three rows of slots
        NewGlyphSlotRow(i);
        NewGlyphSlotRow(i);
    }

    for (int i = 0; i < GLYPH_ASSEMBLER_SLOTS; i++)
    {
        csString slotName;
        slotName =  "assembler";
        slotName += i;
        assembler[i] = dynamic_cast <pawsGlyphSlot*> (FindWidget(slotName));
        if (assembler[i] == NULL)
            return false;
        assembler[i]->SetStatID( 0 );            
        assembler[i]->SetDrag(false);        
        assembler[i]->DrawStackCount(false);
        PawsManager::GetSingleton().Subscribe("sigClearGlyphSlots", assembler[i]);
    }

    return true;
}

void pawsGlyphWindow::HandleMessage ( MsgEntry* me )
{
    switch( me->GetType() )
    {
        case MSGTYPE_GLYPH_REQUEST:
        {
            HandleGlyphList(me);
            break;
        }
        case MSGTYPE_GLYPH_ASSEMBLE:
        {
            HandleAssemble( me );
            break;
        }
    }
}

void pawsGlyphWindow::HandleGlyphList( MsgEntry* me )
{
    psRequestGlyphsMessage msg( me );
    
    // Clear out the old...
    pawsGlyphSlot * floatingSlot = dynamic_cast <pawsGlyphSlot*> (PawsManager::GetSingleton().GetDragDropWidget());
    if (floatingSlot)
        PawsManager::GetSingleton().SetDragDropWidget(NULL);  

    PawsManager::GetSingleton().Publish("sigClearGlyphSlots");
    ClearSpell();

    // Refresh with the new.
    for (size_t x = 0; x < msg.glyphs.GetSize(); x++)
    {
        if (msg.glyphs[x].way>=GLYPH_WAYS)
            continue;

        pawsGlyphSlot * itemSlot = FindFreeSlot(msg.glyphs[x].way);
        CS_ASSERT(itemSlot);
        itemSlot->PlaceItem(msg.glyphs[x].image, "", "", 1);
        itemSlot->SetStatID(msg.glyphs[x].statID);
        itemSlot->SetPurifyStatus(msg.glyphs[x].purifiedStatus);
        itemSlot->SetToolTip(msg.glyphs[x].name);
    }
}

void pawsGlyphWindow::NewGlyphSlotRow(int wayNum)
{
    if (wayNum<0  ||  wayNum>=GLYPH_WAYS)
        return;

    int row = ways[wayNum]->GetRowCount();
    pawsListBoxRow *listRow = ways[wayNum]->NewRow(row);
    for (int j = 0; j < 3; j++)
    {
        pawsGlyphSlot *slot = dynamic_cast <pawsGlyphSlot*> (listRow->GetColumn(j));
        //slot->SetSlotID(3*row+j);
        slot->SetWay(wayNum);
        slot->SetDrag(false);
        slot->DrawStackCount(false);
        PawsManager::GetSingleton().Subscribe("sigClearGlyphSlots", slot);
    }
}

void pawsGlyphWindow::HandleAssemble( MsgEntry* me )
{
    psGlyphAssembleMessage mesg;
    mesg.FromServer( me );
    
    description->Clear();
    
    spellName->SetText(mesg.name);
    description->AddMessage( mesg.description );
    description->ResetScroll();
    spellImage->SetBackground(mesg.image);
}

void pawsGlyphWindow::ClearSpell( void )
{
    description->Clear();
    
    spellName->SetText( "" );
    spellImage->SetBackground("");
    description->ResetScroll();
}

pawsGlyphSlot * pawsGlyphWindow::FindFreeSlot(int wayNum)
{
    size_t rowNum;
	int colNum;
    pawsListBox * way;
    pawsListBoxRow * row;
    pawsGlyphSlot * slot;

    way = ways[wayNum];
    for (rowNum=0; rowNum < way->GetRowCount(); rowNum++)
    {
        row = way->GetRow(rowNum);
        for (colNum=0; colNum < way->GetTotalColumns(); colNum++)
        {
            slot = dynamic_cast <pawsGlyphSlot*> (row->GetColumn(colNum));
            if (slot!=NULL  &&  slot->IsEmpty())
                return slot;
        }
    }

    // no free slot available ? then create new row
    NewGlyphSlotRow(wayNum);
    return dynamic_cast <pawsGlyphSlot*> (way->GetRow(way->GetRowCount()-1)->GetColumn(0));
}

bool pawsGlyphWindow::OnMouseDown(int button, int modifiers, int x, int y)
{
    pawsGlyphSlot* widget = dynamic_cast<pawsGlyphSlot*>(WidgetAt(x, y));
    if(widget)
    {
        return OnButtonPressed(button, modifiers, widget);
    }

    return pawsWidget::OnMouseDown(button, modifiers, x, y);
}

bool pawsGlyphWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    pawsWidget *dndWidget = PawsManager::GetSingleton().GetDragDropWidget();
    pawsGlyphSlot *floatingSlot = dynamic_cast <pawsGlyphSlot*> (dndWidget);
    pawsGlyphSlot *clickedSlot = dynamic_cast <pawsGlyphSlot*> (widget);
    csString slotName = widget->GetName();

    // Check to see if this was the purify button.
    if ( widget->GetID() == PURIFY_BUTTON )
    {
        if (floatingSlot!=NULL  && floatingSlot->GetPurifyStatus()==0)
        {
            psPurifyGlyphMessage mesg( floatingSlot->GetStatID() );
            mesg.SendMessage();

            PawsManager::GetSingleton().SetDragDropWidget(NULL);
        }
        return true;
    }

    // Check to see if this was the cast button.
    if ( widget->GetID() == CAST_BUTTON )
    {
        if (!spellName->GetText())
            return true;

        if (strcmp(spellName->GetText(),"")!=0)
        {
            csString name(spellName->GetText());
            psSpellCastMessage mesg( name, psengine->GetKFactor() );
            mesg.SendMessage();
        }
        return true;
    }

    // Check to see if this was the research button.
    if ( widget->GetID() == RESEARCH_BUTTON )
    {
        SendAssembler();
        return true;
    }

    if (clickedSlot == NULL)
        return true;

    // user clicked on one of the assembler slots
    if (slotName.Slice(0, 9) == "assembler")
    {
        if (!dndWidget)
        {
            StartDrag(clickedSlot);
        }
        else if (floatingSlot && floatingSlot->GetPurifyStatus() == 2)
        {
            StopDrag(clickedSlot);
			SendAssembler(true);
        }
        else if (floatingSlot)
        {
            psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Not a purified glyph"));
            error.FireEvent();
        }
    }
    // user clicked on one of the glyph slots
    else
    {
        if (!dndWidget)
        {
            StartDrag(clickedSlot);
        }
        else if (floatingSlot && clickedSlot->Way() == floatingSlot->Way())
        {
            StopDrag(clickedSlot);
        }
    }

    return true;
}

void pawsGlyphWindow::StartDrag(pawsGlyphSlot *sourceSlot)
{
    if (sourceSlot->IsEmpty())
        return;
    pawsGlyphSlot * floatingSlot = new pawsGlyphSlot();

    floatingSlot->PlaceItem(sourceSlot->ImageName(), "", "", 1);
    floatingSlot->SetStatID(sourceSlot->GetStatID());
    floatingSlot->SetPurifyStatus(sourceSlot->GetPurifyStatus());
    floatingSlot->SetWay(sourceSlot->Way());
    floatingSlot->SetToolTip(sourceSlot->GetToolTip());
    floatingSlot->SetBackgroundAlpha(0);
    floatingSlot->DrawStackCount(false);

    PawsManager::GetSingleton().SetDragDropWidget(floatingSlot);

    sourceSlot->Clear();
}

void pawsGlyphWindow::StopDrag(pawsGlyphSlot *destSlot)
{
    if (!destSlot->IsEmpty())
        return;

    pawsGlyphSlot *floatingSlot = dynamic_cast <pawsGlyphSlot*> (PawsManager::GetSingleton().GetDragDropWidget());

    destSlot->PlaceItem(floatingSlot->ImageName(), "", "", 1);
    destSlot->SetStatID(floatingSlot->GetStatID());
    destSlot->SetPurifyStatus(floatingSlot->GetPurifyStatus());
    destSlot->SetWay(floatingSlot->Way());
    destSlot->SetToolTip(floatingSlot->GetToolTip());
    PawsManager::GetSingleton().SetDragDropWidget(NULL);
    ClearSpell();
}

void pawsGlyphWindow::SendAssembler(bool infoRequest)
{
    psGlyphAssembleMessage mesg( assembler[0]->GetStatID(),
                                 assembler[1]->GetStatID(), 
                                 assembler[2]->GetStatID(), 
                                 assembler[3]->GetStatID(),
								 infoRequest);
                                 
    mesg.SendMessage();
}
