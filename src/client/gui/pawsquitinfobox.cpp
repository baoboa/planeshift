/*
 * pawsquitinfobox.cpp - Author: Christian Svensson
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
//////////////////////////////////////////////////////////////////////


#include <psconfig.h>
#include "globals.h"
// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// COMMON INCLUDES

// CLIENT INCLUDES
#include "psengine.h"
#include "pscharcontrol.h"

// PAWS INCLUDES
#include "pawsquitinfobox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsokbox.h"
#include "paws/pawstextbox.h"

pawsQuitInfoBox::pawsQuitInfoBox()
{
    textBox = NULL;
    okButton = NULL;
}

bool pawsQuitInfoBox::PostSetup()
{
    textBox = (pawsMultiLineTextBox*)FindWidget("Message Box");
    if ( !textBox ) 
        return false;

    okButton = FindWidget("OkButton");    
    if ( !okButton )
        return false;

    text.Clear();

    return true;
}

void pawsQuitInfoBox::SetBox(csString newText)
{
    text = newText;
    textBox->SetText(text.GetData());
    MoveToCenter();
    this->SetModalState(true);
}

void pawsQuitInfoBox::Show()
{
    if (psengine->GetCharControl())
        psengine->GetCharControl()->CancelMouseLook();

    PawsManager::GetSingleton().GetMouse()->Hide(false);

    pawsWidget::Show();

    //Abort connection NOW, this is final
    psengine->Disconnect();
}

void pawsQuitInfoBox::MoveToCenter()
{
    this->MoveTo( (graphics2D->GetWidth()-512)/2 , (graphics2D->GetHeight()-256)/2);
}

bool pawsQuitInfoBox::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if (widget->GetID() == 100)
    {
        psengine->QuitClient();
        return true;
    }

    return false;
}
