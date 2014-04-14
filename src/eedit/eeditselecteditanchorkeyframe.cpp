/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2003 PlaneShift Team (info@planeshift.it,
 * http://www.planeshift.it)
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
#include "eeditselecteditanchorkeyframe.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsspinbox.h"
#include "paws/pawscheckbox.h"

EEditSelectEditAnchorKeyFrame::EEditSelectEditAnchorKeyFrame() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectEditAnchorKeyFrame::~EEditSelectEditAnchorKeyFrame()
{
    delete selectCallback;
}

void EEditSelectEditAnchorKeyFrame::Select(float time, EEditInputboxManager::iSelectEditAnchorKeyFrame * callback, 
                                           const csVector2 & pos)
{
    // move to given coordinate (probable mouse position)
    MoveTo((int)pos.x, (int)pos.y);

    // set default value
    newTime->SetValue(time);
    
    // set the new callback
    delete selectCallback;
    selectCallback = callback;

    // show the widget
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectEditAnchorKeyFrame::PostSetup()
{
    newTime     = (pawsSpinBox *)  FindWidget("select_time");
    hasPosX     = (pawsCheckBox *) FindWidget("select_pos_x");
    hasPosY     = (pawsCheckBox *) FindWidget("select_pos_y");
    hasPosZ     = (pawsCheckBox *) FindWidget("select_pos_z");
    hasToTarget = (pawsCheckBox *) FindWidget("select_to_target");
    ok          = (pawsButton *)   FindWidget("ok");
    
    return true;
}

bool EEditSelectEditAnchorKeyFrame::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)
{
    if (widget == ok)
    {
        /*
        csString anchorType;
        if (anchorTypeBasic->GetState())  anchorType = "basic";
        if (anchorTypeSpline->GetState()) anchorType = "spline";
        if (anchorTypeSocket->GetState()) anchorType = "socket";
        
        if (selectCallback) selectCallback->Select(newName->GetText(), anchorType);
        delete selectCallback;
        selectCallback = 0;
        
        Hide();
        return true;
        */
    }
    
    return false;
}

