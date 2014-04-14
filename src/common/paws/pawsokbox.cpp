/*
 * pawsokbox.cpp - Author: Andrew Craig
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
#include "pawsokbox.h"
#include "pawstextbox.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsOkBox::pawsOkBox():
    text(NULL)
{
    okButton  = 0;
    notify = 0;
    factory = "pawsOkBox";
}
pawsOkBox::pawsOkBox(const pawsOkBox &origin)
    :pawsWidget(origin),
     text(origin.text)
{
    notify = 0;
    okButton = 0;
    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.okButton == origin.children[i])
            okButton = origin.children[i];
        if(okButton != 0)
            break;
    }
}
pawsOkBox::~pawsOkBox()
{
}

void pawsOkBox::SetNotify(pawsWidget* widget)
{
    notify = widget;
}

bool pawsOkBox::PostSetup()
{
    text = (pawsMultiLineTextBox*)FindWidget("Message Box");
    if(!text)
        return false;

    okButton = FindWidget("OkButton");
    return true;
}


void pawsOkBox::SetText(const char* newtext)
{
    text->SetText(newtext);
}


bool pawsOkBox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if(notify)
    {
        bool result = notify->OnButtonPressed(mouseButton, keyModifier, widget);
        SetNotify(NULL);
        PawsManager::GetSingleton().SetModalWidget(NULL);
        Hide();
        return result;
    }
    else
    {
        Hide();
        PawsManager::GetSingleton().SetModalWidget(NULL);
        return true;
    }
}

