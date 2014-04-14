/*
 * Author: Andrew Robberts
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
#include "eeditselectnewanchor.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsradio.h"
#include "paws/pawstextbox.h"

EEditSelectNewAnchor::EEditSelectNewAnchor() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectNewAnchor::~EEditSelectNewAnchor()
{
    delete selectCallback;
}

void EEditSelectNewAnchor::Select(EEditInputboxManager::iSelectNewAnchor * callback, const csVector2 & pos)
{
    // move to given coordinate (probable mouse position)
    MoveTo((int)pos.x, (int)pos.y);

    // set default value
    newName->SetText("");
    anchorTypeBasic->SetState(true);
    anchorTypeSpline->SetState(false);
    anchorTypeSocket->SetState(false);
    
    // set the new callback
    delete selectCallback;
    selectCallback = callback;

    // show the widget
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectNewAnchor::PostSetup()
{
    newName          = (pawsEditTextBox *) FindWidget("select_name");
    anchorTypeBasic  = (pawsRadioButton *) FindWidget("anchor_type_basic");
    anchorTypeSpline = (pawsRadioButton *) FindWidget("anchor_type_spline");
    anchorTypeSocket = (pawsRadioButton *) FindWidget("anchor_type_socket");
    add              = (pawsButton *)      FindWidget("add");
    
    return true;
}

bool EEditSelectNewAnchor::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)
{
    if (widget == add)
    {
        csString anchorType;
        if (anchorTypeBasic->GetState())  anchorType = "basic";
        if (anchorTypeSpline->GetState()) anchorType = "spline";
        if (anchorTypeSocket->GetState()) anchorType = "socket";
        
        if (selectCallback) selectCallback->Select(newName->GetText(), anchorType);
        delete selectCallback;
        selectCallback = 0;
        
        Hide();
        return true;
    }
    
    return false;
}

