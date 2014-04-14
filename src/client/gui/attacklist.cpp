/*
 * attacklist.cpp              author: hartra344 [at] gmail [dot] com
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "net/cmdhandler.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "gui/attacklist.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "gui/pawsslot.h"
#include "gui/pawscontrolwindow.h"


pawsAttackBookWindow::pawsAttackBookWindow() :
    attackList(NULL),
    attackDescription(NULL),
    attackImage(NULL)
{
}

pawsAttackBookWindow::~pawsAttackBookWindow()
{
}

bool pawsAttackBookWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_ATTACK_BOOK);

    attackList        = (pawsListBox*)FindWidget("AttackList");
    attackDescription = (pawsMessageTextBox*)FindWidget("Description");
    attackImage       = (pawsSlot*)FindWidget("AttackImage");

    attackList->SetSortingFunc(0, textBoxSortFunc);
    attackList->SetSortingFunc(5, textBoxSortFunc);
    attackList->SetSortingFunc(6, textBoxSortFunc);

    return true;
}

void pawsAttackBookWindow::HandleMessage(MsgEntry* me)
{
    switch(me->GetType())
    {
        case MSGTYPE_ATTACK_BOOK:
        {
            HandleAttacks(me);
            attackList->SortRows();   ///after getting attacks sort the column specified in xml
            break;
        }
    }
}

bool pawsAttackBookWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(!strcmp(widget->GetName(),"Queue"))
    {
        Queue(); //queue the selected attack
        return true;
    }
    return false;
}

void pawsAttackBookWindow::HandleAttacks(MsgEntry* me)
{
    //lists all available attacks
    attackList->Clear();
    descriptions_Hash.Empty();
    images_Hash.Empty();


    psAttackBookMessage mesg(me, ((psNetManager*)psengine->GetNetManager())->GetConnection()->GetAccessPointers());

    for(size_t x = 0; x < mesg.attacks.GetSize(); x++)
    {
        pawsListBoxRow* row = attackList->NewRow();
        pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
        name->SetText(mesg.attacks[x].name);

        pawsTextBox* type = (pawsTextBox*)row->GetColumn(2);
        type->SetText(mesg.attacks[x].type);


        descriptions_Hash.Put(mesg.attacks[x].name, mesg.attacks[x].description);
        images_Hash.Put(mesg.attacks[x].name, mesg.attacks[x].image);
        if(selectedAttack == mesg.attacks[x].name)
        {
            attackList->Select(row);
        }
    }
}



void pawsAttackBookWindow::Queue()
{
    if(selectedAttack.Length() > 0)
    {
        csString Command;
        Command.Append("/queue ");
        Command.Append(selectedAttack);
        psengine->GetCmdHandler()->Execute(Command);
    }
}


void pawsAttackBookWindow::Show()
{
    pawsControlledWindow::Show();
    psAttackBookMessage msg;
    msg.SendMessage();
}


void pawsAttackBookWindow::Close()
{
    Hide();
    attackList->Clear();
}

void pawsAttackBookWindow::OnListAction(pawsListBox* widget, int status)
{
    if(status==LISTBOX_HIGHLIGHTED)
    {
        attackDescription->Clear();
        pawsListBoxRow* row = widget->GetSelectedRow();

        pawsTextBox* attackName = (pawsTextBox*)(row->GetColumn(0));

        selectedAttack.Replace(attackName->GetText());
        attackDescription->AddMessage(descriptions_Hash.Get(attackName->GetText(), "Unknown"));
        attackDescription->ResetScroll();
        //printf("DEBUG OnListAction. %s - %s - \n",attackName->GetText(),images_Hash.Get(attackName->GetText(),"").GetData());
        attackImage->PlaceItem(images_Hash.Get(attackName->GetText(),"").GetData(), "", "", 1);
        csString action = "/queue " + csString(attackName->GetText());
        attackImage->SetBartenderAction(action);
        attackImage->SetToolTip(attackName->GetText());
    }
}
