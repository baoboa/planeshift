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
#include "eeditselectyesno.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsradio.h"

EEditSelectYesNo::EEditSelectYesNo() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectYesNo::~EEditSelectYesNo()
{
    delete selectCallback;
}

void EEditSelectYesNo::Select(bool startValue, EEditInputboxManager::iSelectYesNo * callback, const csVector2 & pos)
{
    MoveTo((int)pos.x, (int)pos.y);
    yes->SetState(startValue);
    no->SetState(!startValue);
    delete selectCallback;
    selectCallback = callback;
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectYesNo::PostSetup()
{
    yes = (pawsRadioButton *) FindWidget("select_yes"); CS_ASSERT(yes);
    no  = (pawsRadioButton *) FindWidget("select_no");  CS_ASSERT(no);
    
    return true;
}

bool EEditSelectYesNo::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget * widget )
{
    if (widget == yes)
    {
        if (selectCallback) selectCallback->Select(true);
        delete selectCallback;
        selectCallback = 0;
        Hide();
        return true;
    }
    else if (widget == no)
    {
        if (selectCallback) selectCallback->Select(false);
        delete selectCallback;
        selectCallback = 0;
        Hide();
        return true;
    }
    
    return false;
}

