/*
 * soundctrl.cpp
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


//====================================================================================
// Local Includes
//====================================================================================
#include "soundctrl.h"
#include "manager.h"


SoundControl::SoundControl(int ID)
{
    id        		= ID;
    isEnabled 		= true;
    isMuted   		= false;
    isDampened		= false;
    volume    		= VOLUME_NORM;
    volumeDamp 		= 1.0f;
}

SoundControl::~SoundControl()
{
    // of course we don't delete SoundControlListeners here
    listeners.DeleteAll();
}

void SoundControl::Subscribe(iSoundControlListener* listener)
{
    listeners.Add(listener);
}

void SoundControl::Unsubscribe(iSoundControlListener* listener)
{
    listeners.Delete(listener);
}

void SoundControl::CallListeners()
{
    csSet<iSoundControlListener*>::GlobalIterator listenerIter(listeners.GetIterator());

    while(listenerIter.HasNext())
    {
        listenerIter.Next()->OnSoundChange(this);
    }
}

int SoundControl::GetID() const
{
    return id;
}

void SoundControl::ActivateToggle()
{
    SetToggle(true);
}

void SoundControl::DeactivateToggle()
{
    SetToggle(false);
}

void SoundControl::SetToggle(bool toggle)
{
    if(isEnabled != toggle)
    {
        isEnabled = toggle;
        CallListeners();
    }
}

bool SoundControl::GetToggle() const
{
    return isEnabled;
}

void SoundControl::VolumeDampening(float dampPercent)
{
    /**
     * If the volume should be dampened we try to lower the volume percent with
     * 5% each time until we are at a level of 'dampPercent' volume. When we want
     * to restore the volume to full we restore it with 1% change each iteration.
     */
    if(dampPercent < 1.0)
    {
        if(volumeDamp > dampPercent)
        {
            volumeDamp -= 0.05f;
        }
        else
        {
            isDampened = true;
        }
    }
    else
    {
        if(volumeDamp < 1.0)
        {
            volumeDamp += 0.01f;
        }
        else
        {
            isDampened = false;
        }
    }
}

bool SoundControl::IsDampened() const
{
    return isDampened;
}

void SoundControl::Mute()
{
    isMuted = true;
    CallListeners();
}

void SoundControl::Unmute()
{
    isMuted = false;
    CallListeners();
}

void SoundControl::SetVolume(float vol)
{
    volume = vol;
    CallListeners();
}

float SoundControl::GetVolume() const
{
    if(isMuted)
    {
        return 0.0;
    }
    else
    {
        return volume * volumeDamp;
    }
}

