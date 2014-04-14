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
#include "eeditselectfloat.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsspinbox.h"

EEditSelectFloat::EEditSelectFloat() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectFloat::~EEditSelectFloat()
{
    delete selectCallback;
}

void EEditSelectFloat::Select(float startValue, EEditInputboxManager::iSelectFloat * callback, const csVector2 & pos)
{
    MoveTo((int)pos.x, (int)pos.y);
    value->SetValue(startValue);
    delete selectCallback;
    selectCallback = callback;
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectFloat::PostSetup()
{
    value = (pawsSpinBox *) FindWidget("select_value"); CS_ASSERT(value);
    ok    = (pawsButton *)  FindWidget("ok");           CS_ASSERT(ok);
    
    return true;
}

bool EEditSelectFloat::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget == ok)
    {
        if (selectCallback) selectCallback->Select(value->GetValue());
        delete selectCallback;
        selectCallback = 0;
        Hide();
        return true;
    }
    
    return false;
}

