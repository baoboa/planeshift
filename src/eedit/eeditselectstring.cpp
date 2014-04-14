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
#include "eeditselectstring.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"

EEditSelectString::EEditSelectString() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectString::~EEditSelectString()
{
    delete selectCallback;
}

void EEditSelectString::Select(const csString & startValue, EEditInputboxManager::iSelectString * callback, const csVector2 & pos)
{
    MoveTo((int)pos.x, (int)pos.y);
    value->SetText(startValue);
    delete selectCallback;
    selectCallback = callback;
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectString::PostSetup()
{
    value = (pawsEditTextBox *) FindWidget("select_value");
    ok    = (pawsButton *)      FindWidget("ok");
    
    return true;
}

bool EEditSelectString::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget == ok)
    {
        if (selectCallback) selectCallback->Select(value->GetText());
        delete selectCallback;
        selectCallback = 0;
        Hide();
        return true;
    }
    
    return false;
}

