/*
* dummysndctrl.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
*
* Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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


//====================================================================================
// Local Includes
//====================================================================================
#include "dummysndctrl.h"


DummySoundControl::DummySoundControl(int ID):
    id(ID),volume(0.0f),isEnabled(false),dampening(false)
{
}

DummySoundControl::~DummySoundControl()
{
}

int DummySoundControl::GetID() const
{
    return id;
}

void DummySoundControl::ActivateToggle()
{
    isEnabled = true;
}

void DummySoundControl::DeactivateToggle()
{
    isEnabled = false;
}

void DummySoundControl::SetToggle(bool toogle)
{
    isEnabled = toogle;
}

bool DummySoundControl::GetToggle() const
{
    return isEnabled;
}

void DummySoundControl::Mute()
{
    return;
}

void DummySoundControl::Unmute()
{
    return;
}

void DummySoundControl::SetVolume(float vol)
{
    volume = vol;
}

float DummySoundControl::GetVolume() const
{
    return volume;
}

void DummySoundControl::VolumeDampening(float damp)
{
    dampening = (damp < 1.0f);
    return;
}

bool DummySoundControl::IsDampened() const
{
    return dampening;
}
