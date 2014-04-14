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
#include "eeditcameratoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"

EEditCameraToolbox::EEditCameraToolbox() : scfImplementationType(this)
{
}

EEditCameraToolbox::~EEditCameraToolbox()
{
}

void EEditCameraToolbox::Update(unsigned int elapsed)
{
}

size_t EEditCameraToolbox::GetType() const
{
    return T_CAMERA;
}

const char * EEditCameraToolbox::GetName() const
{
    return "Camera";
}

bool EEditCameraToolbox::PostSetup()
{
    camLeft    = (pawsButton *) FindWidget("cam_left");     CS_ASSERT(camLeft);
    camRight   = (pawsButton *) FindWidget("cam_right");    CS_ASSERT(camRight);
    camUp      = (pawsButton *) FindWidget("cam_up");       CS_ASSERT(camUp);
    camDown    = (pawsButton *) FindWidget("cam_down");     CS_ASSERT(camDown);
    camForward = (pawsButton *) FindWidget("cam_forward");  CS_ASSERT(camForward);
    camBack    = (pawsButton *) FindWidget("cam_back");     CS_ASSERT(camBack);
    return true;
}
bool EEditCameraToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == camUp)
        editApp->SetCamFlag(CAM_UP, true);
    else if (widget == camDown)
        editApp->SetCamFlag(CAM_DOWN, true);
    else if (widget == camRight)
        editApp->SetCamFlag(CAM_RIGHT, true);
    else if (widget == camLeft)
        editApp->SetCamFlag(CAM_LEFT, true);
    else if (widget == camForward)
        editApp->SetCamFlag(CAM_FORWARD, true);
    else if (widget == camBack)
        editApp->SetCamFlag(CAM_BACK, true);

    return false;
}

bool EEditCameraToolbox::OnButtonReleased(int mouseButton, pawsWidget* widget)
{
    if (widget == camUp)
        editApp->SetCamFlag(CAM_UP, false);
    else if (widget == camDown)
        editApp->SetCamFlag(CAM_DOWN, false);
    else if (widget == camRight)
        editApp->SetCamFlag(CAM_RIGHT, false);
    else if (widget == camLeft)
        editApp->SetCamFlag(CAM_LEFT, false);
    else if (widget == camForward)
        editApp->SetCamFlag(CAM_FORWARD, false);
    else if (widget == camBack)
        editApp->SetCamFlag(CAM_BACK, false);
    
    return false;
}
