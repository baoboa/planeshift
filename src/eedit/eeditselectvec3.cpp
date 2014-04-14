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
#include "eeditselectvec3.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsspinbox.h"

EEditSelectVec3::EEditSelectVec3() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectVec3::~EEditSelectVec3()
{
    delete selectCallback;
}

void EEditSelectVec3::Select(const csVector3 & startValue, EEditInputboxManager::iSelectVec3 * callback, const csVector2 & pos)
{
    MoveTo((int)pos.x, (int)pos.y);
    valX->SetValue(startValue.x);
    valY->SetValue(startValue.y);
    valZ->SetValue(startValue.z);
    delete selectCallback;
    selectCallback = callback;
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectVec3::PostSetup()
{
    valX = (pawsSpinBox *) FindWidget("select_x");
    valY = (pawsSpinBox *) FindWidget("select_y");
    valZ = (pawsSpinBox *) FindWidget("select_z");
    ok   = (pawsButton *)  FindWidget("ok");
    
    return true;
}

bool EEditSelectVec3::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)
{
    if (widget == ok)
    {
        if (selectCallback) selectCallback->Select(csVector3(valX->GetValue(), valY->GetValue(), valZ->GetValue()));
        delete selectCallback;
        selectCallback = 0;
        Hide();
        return true;
    }
    
    return false;
}

