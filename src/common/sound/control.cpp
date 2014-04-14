/*
 * control.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */


#include "control.h"
#include "manager.h"

SoundControl::SoundControl ()
{
    id        = -1;
    isEnabled = true;
    isMuted   = false;
    volume    = VOLUME_NORM;

    hasCallback = false;
}

SoundControl::~SoundControl ()
{
}

int SoundControl::GetID ()
{
    return id;
}

void SoundControl::ActivateToggle ()
{
    isEnabled = true;
    Callback();
}

void SoundControl::DeactivateToggle ()
{
    isEnabled = false;
    Callback();
}

void SoundControl::SetToggle (bool value)
{
    isEnabled = value;
    Callback();
}

bool SoundControl::GetToggle ()
{
    return isEnabled;
}

void SoundControl::Mute ()
{
    isMuted = true;
    Callback();
}

void SoundControl::Unmute ()
{
    isMuted = false;
}

void SoundControl::SetVolume (float vol)
{
    volume = vol;
    Callback();
}

float SoundControl::GetVolume ()
{
    return volume;
}

void SoundControl::SetCallback(void (*object), void (*function) (void *))
{
    callbackobject = object;
    callbackfunction = function;
    hasCallback = true; 
}

void SoundControl::Callback ()
{
    if (hasCallback == true)
    {
        callbackfunction(callbackobject);
    }
}

void SoundControl::RemoveCallback()
{
    hasCallback = false;
}

