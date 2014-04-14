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
#include "eeditselectnewanchorkeyframe.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsspinbox.h"

EEditSelectNewAnchorKeyFrame::EEditSelectNewAnchorKeyFrame() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectNewAnchorKeyFrame::~EEditSelectNewAnchorKeyFrame()
{
    delete selectCallback;
}

void EEditSelectNewAnchorKeyFrame::Select(EEditInputboxManager::iSelectNewAnchorKeyFrame * callback, 
                                          const csVector2 & pos)
{
    // move to given coordinate (probable mouse position)
    MoveTo((int)pos.x, (int)pos.y);

    // set default value
    newTime->SetValue(0);
    
    // set the new callback
    delete selectCallback;
    selectCallback = callback;

    // show the widget
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectNewAnchorKeyFrame::PostSetup()
{
    newTime = (pawsSpinBox *) FindWidget("select_time");
    add     = (pawsButton *)  FindWidget("add");
    
    return true;
}

bool EEditSelectNewAnchorKeyFrame::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)
{
    if (widget == add)
    {
        if (selectCallback) selectCallback->Select(newTime->GetValue());
        delete selectCallback;
        selectCallback = 0;
        
        Hide();
        return true;
    }
    
    return false;
}

