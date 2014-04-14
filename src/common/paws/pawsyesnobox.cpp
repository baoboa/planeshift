/*
 * pawsyesnobox.cpp - Author: Andrew Craig
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

// COMMON INCLUDES

// CLIENT INCLUDES

// PAWS INCLUDES
#include "pawsmanager.h"
#include "pawsyesnobox.h"
#include "pawstextbox.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsYesNoBox::pawsYesNoBox()
{
    notify = 0;
    useCustomIDs = false;

    handler = 0;
    yesButton = 0;
    noButton  = 0;
}

pawsYesNoBox::pawsYesNoBox(const pawsYesNoBox &origin)
    :pawsWidget(origin),
     handler(origin.handler),
     owner(origin.owner),
     useCustomIDs(origin.useCustomIDs)

{
    notify = 0;
    yesButton = 0;
    noButton = 0;
    text = 0;

    for(unsigned int i = 0 ; i< origin.children.GetSize(); i++)
    {
        if(origin.text == origin.children[i])
            text = dynamic_cast<pawsMultiLineTextBox*>(children[i]);
        else if(origin.yesButton == origin.children[i])
            yesButton = origin.children[i];
        else if(origin.noButton == origin.children[i])
            noButton = origin.children[i];

        if(text != 0 && yesButton != 0 && noButton != 0) break;
    }
}

pawsYesNoBox::~pawsYesNoBox()
{
}

bool pawsYesNoBox::PostSetup()
{
    text = dynamic_cast<pawsMultiLineTextBox*>(FindWidget("Message Box"));
    if(!text)
    {
        Error1("Error in pawsYesNoBox PostSetup");
        return false;
    }

    yesButton = FindWidget("YesButton");
    noButton = FindWidget("NoButton");
    return true;
}

void pawsYesNoBox::Hide()
{
    pawsWidget::Hide();
    useCustomIDs = false;

    if(PawsManager::GetSingleton().GetModalWidget() == this)
        PawsManager::GetSingleton().SetModalWidget(0);

    // Reset the ID's back to defaults

    SetID();
}

void pawsYesNoBox::SetID(int yes, int no)
{
    useCustomIDs = true;

    if(yesButton) yesButton->SetID(yes);
    if(noButton)  noButton->SetID(no);
}

void pawsYesNoBox::SetText(const char* newtext)
{
    //text->Clear();
    text->SetText(newtext);
}

void pawsYesNoBox::SetNotify(pawsWidget* widget)
{
    notify = widget;
}

bool pawsYesNoBox::OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget)
{
    // The parent is responsible for handling the button presses.
    if(notify)
    {
        bool result = notify->OnButtonReleased(mouseButton, keyModifier, widget);
        SetNotify(0);
        Hide();
        return result;
    }
    else if(handler)
    {
        if(widget == yesButton)
            handler(true , owner);
        else
            handler(false, owner);
        Hide();
        return true;
    }

    return false;
}

void pawsYesNoBox::SetCallBack(YesNoResponseFunc handler , void* owner, const char* text)
{
    this->handler = handler;
    this->owner = owner;

    if(text == NULL)
        text = "";
    SetText(text);
}

pawsYesNoBox* pawsYesNoBox::Create(pawsWidget* notify, const csString &text,
                                   int yesID, int noID)
{
    pawsYesNoBox* dialog = dynamic_cast <pawsYesNoBox*>(PawsManager::GetSingleton().FindWidget("YesNoWindow"));
    dialog->SetNotify(notify);
    dialog->SetText(PawsManager::GetSingleton().Translate(text));
    dialog->SetID(yesID, noID);
    dialog->Show();
    PawsManager::GetSingleton().SetModalWidget(dialog);
    dialog->MoveTo((PawsManager::GetSingleton().GetGraphics2D()->GetWidth() - dialog->GetActualWidth(512)) / 2,
                   (PawsManager::GetSingleton().GetGraphics2D()->GetHeight() - dialog->GetActualHeight(256))/2);
    return dialog;
}
