/*
 * psmusic.cpp
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

#include "pssound.h"


psMusic::psMusic()
{
    active = false;
    handle = NULL;
}

psMusic::~psMusic()
{
    Stop();
}

bool psMusic::CheckTimeOfDay(int time)
{
    if((timeofday <= time)
        && (timeofdayrange >= time))
    {
        return true;        
    }

    return false;
}

bool psMusic::CheckType(const int _type)
{
    if(type == _type)
    {
        return true;
    }
    
    return false;
}

void psMusic::FadeUp()
{
    if(handle != NULL)
    {
        handle->Fade((maxvol - minvol), fadedelay, FADE_UP);
    }
}

void psMusic::FadeDown()
{
    if(handle != NULL)
    {
        handle->Fade(maxvol, fadedelay, FADE_DOWN);
    }
}

void psMusic::FadeDownAndStop()
{
    active = false;

    if(handle != NULL)
    {
        handle->RemoveCallback();
        handle->Fade(maxvol, fadedelay, FADE_STOP);
        handle = NULL;
    }
}

bool psMusic::Play(bool loopToggle, SoundControl* &ctrl)
{
    Stop(); // stop any previous sound
    if(SoundSystemManager::GetSingleton().Play2DSound(resource, loopToggle,
                              loopstart, loopend, minvol, ctrl, handle))
    {
        active = true;
        UpdateHandleCallback();
        return true;
    }
    
    return false;
}

void psMusic::Stop()
{
    active = false;
    
    if(handle != NULL)
    {
        SoundSystemManager::GetSingleton().StopSound(handle->GetID());
    }

    handle = 0;
}

void psMusic::SetManaged()
{
    if(handle != NULL)
    {
        handle->SetAutoRemove(false);
    }
}

void psMusic::SetUnManaged()
{
    if(handle != NULL)
    {
        handle->SetAutoRemove(true);
    }
}

void psMusic::DontLoop()
{
    if(handle != NULL)
    {
        handle->sndstream->SetLoopState(DONT_LOOP);
    }
}

void psMusic::Loop()
{
    if(handle != NULL)
    {
        handle->sndstream->SetLoopState(LOOP);
        handle->sndstream->Unpause();
    }
}

void psMusic::UpdateHandleCallback()
{
    if(handle != NULL)
    {
        handle->SetCallback(this, &StopCallback);
    }
}

void psMusic::StopCallback(void* object)
{
    psMusic* which = (psMusic*) object;
    which->active = false;
    which->handle = NULL;
}

