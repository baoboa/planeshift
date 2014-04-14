/*
 * pawsspellbook.cpp - Anders Reggestad <andersr@pvv.org>
 *                   - PAWS conversion Andrew Craig
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

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/strutil.h"
#include "net/connection.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsspellbookwindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "gui/pawsslot.h"
#include "gui/pawscontrolwindow.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsSpellBookWindow::pawsSpellBookWindow():
    spellList(NULL),spellDescription(NULL),spellImage(NULL)
{
    selectedSpell.Clear();
}

pawsSpellBookWindow::~pawsSpellBookWindow()
{

}

bool pawsSpellBookWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_SPELL_BOOK);

    spellList        = (pawsListBox*)FindWidget("SpellList");
    spellDescription = (pawsMessageTextBox*)FindWidget("Description");
    spellImage       = (pawsSlot*)FindWidget("SpellImage");

    spellList->SetSortingFunc(0, textBoxSortFunc);
    spellList->SetSortingFunc(5, textBoxSortFunc);
    spellList->SetSortingFunc(6, textBoxSortFunc);

    return true;
}


void pawsSpellBookWindow::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_SPELL_BOOK:
        {
            HandleSpells( me );
            spellList->SortRows();   ///after getting spells sort the column specified in xml
            break;
        }
    }              
}

bool pawsSpellBookWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if (!strcmp(widget->GetName(),"Combine"))
    {
        CreateNewSpell();
        return true;
    }
    else if (!strcmp(widget->GetName(),"Cast"))
    {
        Cast();
        return true;
    }
    else if (!strcmp(widget->GetName(),"ActiveMagic"))
    {
        ShowActiveMagic();
        return true;
    }
    return true;
}


void pawsSpellBookWindow::HandleSpells( MsgEntry* me )
{
    spellList->Clear();
    descriptions_Hash.Empty();
    images_Hash.Empty();

    psSpellBookMessage mesg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());
    for ( size_t x = 0; x < mesg.spells.GetSize(); x++ )
    {       
        pawsListBoxRow* row = spellList->NewRow();
        pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
        name->SetText( mesg.spells[x].name );
                       
        pawsTextBox* way = (pawsTextBox*)row->GetColumn(5);
        way->SetText( mesg.spells[x].way );
 
        pawsTextBox* realm = (pawsTextBox*)row->GetColumn(6);
        realm->FormatText( "%d", mesg.spells[x].realm );
        
        for (size_t i = 0; i < 4; i++)
        {
            if(!mesg.spells[x].glyphs[i])
                break;

            pawsWidget* glyph = (pawsWidget*)row->GetColumn(1+i);
            glyph->SetBackground(mesg.spells[x].glyphs[i]);
        }
        descriptions_Hash.Put(mesg.spells[x].name, mesg.spells[x].description);
        images_Hash.Put(mesg.spells[x].name, mesg.spells[x].image);
        if (selectedSpell == mesg.spells[x].name)
        {
            spellList->Select(row);
        }
    }            
}


void pawsSpellBookWindow::CreateNewSpell()
{
    pawsWidget* widget = PawsManager::GetSingleton().FindWidget( "GlyphWindow" );
    if ( widget->IsVisible() )
        widget->Hide();
    else
        widget->Show();
}

void pawsSpellBookWindow::Cast()
{
    if ( selectedSpell.Length() > 0 )
    {
        psSpellCastMessage msg(selectedSpell, psengine->GetKFactor());
        msg.SendMessage();
    }
}


void pawsSpellBookWindow::ShowActiveMagic()
{
    pawsWidget* widget = PawsManager::GetSingleton().FindWidget( "ActiveMagicWindow" );
    if ( widget->IsVisible() )
        widget->Hide();
    else
        widget->Show();
}

void pawsSpellBookWindow::Show()
{
    pawsControlledWindow::Show();
    psSpellBookMessage msg;
    msg.SendMessage();
}


void pawsSpellBookWindow::Close()
{
    Hide();
    spellList->Clear();
}

void pawsSpellBookWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        spellDescription->Clear();
        pawsListBoxRow* row = widget->GetSelectedRow();

        pawsTextBox* spellName = (pawsTextBox*)(row->GetColumn(0));

        selectedSpell.Replace( spellName->GetText() );
        spellDescription->AddMessage(descriptions_Hash.Get(spellName->GetText(), "Unknown"));
        spellDescription->ResetScroll();
        //spellImage->SetBackground(images_Hash.Get(spellName->GetText(), ""));
        spellImage->PlaceItem(images_Hash.Get(spellName->GetText(),""), "", "", 1);
        csString action = "/cast " + csString(spellName->GetText());
        spellImage->SetBartenderAction(action);
        spellImage->SetToolTip(spellName->GetText());
    }
}
