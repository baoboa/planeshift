/*
 * handle.cpp
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

#include "handle.h"
#include "manager.h"
#include "system.h"
#include "data.h"

#define FADE_TIMESTEP 100

/*
 * A Soundhandle contains all informations we have about a sound
 * source stream 2D/3D and its completly public
 * and provides a interface to the source stream
 * volumes, fading parameters
 */

SoundHandle::SoundHandle()
    : scfImplementationType(this)
{
    id = 0; // this parameter makes sense only for SoundSystemManager
    fadeSteps = 0;
    fadeStop = false;
    maxDistance = -1;

    hasCallback = false;

    autoremove = true;
    delayActive = false;

    // initializing pointers to null
    sndCtrl = 0;
    snddata = 0;
    sndstream = 0;
    sndsource = 0;
    sndsource3d = 0;
    sndsourcedir = 0;
}

SoundHandle::~SoundHandle()
{
    if(hasCallback == true)
    {
        Callback();
    }
    /*
     * i was running into a bug because i didnt do the isvalid / NULL
     * checks ..
     * sources get removed even if you dont do RemoveSource
     * but it will not dereference them! ..
     */
    if(sndstream != NULL)
    {
        SoundSystemManager::GetSingleton().GetSoundSystem()->RemoveStream(sndstream);
    }

    if(sndsource != NULL)
    {
        SoundSystemManager::GetSingleton().GetSoundSystem()->RemoveSource(sndsource);
    }

    if(delayActive)
    {
        SoundSystemManager::GetSingleton().eventTimer->RemoveTimerEvent(this);
    }

    // unsubscribing to SoundControls' events
    if(sndCtrl != 0)
    {
        sndCtrl->Unsubscribe(this);
    }
    SoundSystemManager::GetSingleton().mainSndCtrl->Unsubscribe(this);
}

/*
 * utilize all the SndSys and SndData function to create a real sound
 * error msgs are handled by the SndSys / SndData functions
 * all we do is handle results
 *
 * the soundhandles which contain all our precious informations
 * are created here. Note that those are NOT pushed into the array!
 * Thats done within the Play3D functions before the sounds are unpaused
 */

bool SoundHandle::Init(const char* resname, bool loop, float volumePreset,
                       int type, SoundControl* &ctrl, bool doppler)
{
    csRef<iSndSysData> snddata;

    if(!SoundSystemManager::GetSingleton().GetSoundDataCache()->GetSoundData(resname, snddata))
    {
        return false;
    }

    if(!SoundSystemManager::GetSingleton().GetSoundSystem()->CreateStream(snddata, loop, type, sndstream))
    {
        SoundSystemManager::GetSingleton().GetSoundDataCache()->UnloadSoundFile(resname);
        return false;
    }

    if(!SoundSystemManager::GetSingleton().GetSoundSystem()->CreateSource(sndstream, sndsource))
    {
        return false;
    }

    currentVolume = volumePreset;
    sndCtrl = ctrl;
    name = csString(resname);
    dopplerEffect = doppler;

    // subscribe to SoundControls' events
    sndCtrl->Subscribe(this);
    SoundSystemManager::GetSingleton().mainSndCtrl->Subscribe(this);

    return true;
}

bool SoundHandle::Perform(iTimerEvent* /*ev*/)
{
    SoundSystemManager::GetSingleton().eventTimer->RemoveTimerEvent(this);
    delayActive = false;

    if(sndstream.IsValid())
    {
        return sndstream->Unpause();
    }

    return false;
}

/*
 * This is a utility function to calculate fading
 *
 * fading is done in ten steps per second
 * we calculate the number of steps and the volume for each step
 * FIXME csticks
 *
 * fading parameters of the given handle are updated.
 * Fading is done by sndSysMgr::UpdateSound
 *
 */


void SoundHandle::Fade(float volume, int time, int direction)
{
    // setting new steady state volume
    if(direction == FADE_UP)
    {
        currentVolume += volume;
        fadeSteps = time / FADE_TIMESTEP;
    }
    else /* FADE_DOWN + FADE_STOP */
    {
        currentVolume -= volume;
        fadeSteps = -time / FADE_TIMESTEP;
    }

    // checking boundaries
    if(currentVolume < VOLUME_ZERO)
    {
        currentVolume = VOLUME_ZERO;
    }
    else if(currentVolume > VOLUME_MAX)
    {
        currentVolume = VOLUME_MAX;
    }

    // there must be at least one step or the volume don't change
    if(fadeSteps == 0)
    {
        fadeSteps = 1;
    }

    if(direction == FADE_STOP)
    {
        fadeStop = true;
    }
    else /* sanity */
    {
        fadeStop = false;
    }
}


/*
 * This implementation is robust to changes of currentVolume and
 * SoundControls' volume while still fading.
 */
void SoundHandle::FadeStep()
{
    float volumeDiff;
    float steadyStateVol;
    float currentSourceVol;

    // do nothing if there is no fading ongoing
    if(fadeSteps == 0)
    {
        return;
    }

    steadyStateVol = SoundSystemManager::GetSingleton().mainSndCtrl->GetVolume()
                     * sndCtrl->GetVolume() * currentVolume;
    currentSourceVol = sndsource->GetVolume();

    // if the sound control's volume has changed so much that the steady state
    // volume has been already passed, volumeDiff is < 0
    volumeDiff = (steadyStateVol - currentSourceVol) / fadeSteps;

    // if volumeDiff is 0 there is no need for fading anymore
    if(volumeDiff <= 0 || fadeSteps == 1)
    {
        sndsource->SetVolume(steadyStateVol);
        fadeSteps = 0;
    }
    else
    {
        if(fadeSteps > 0)
        {
            sndsource->SetVolume(currentSourceVol + volumeDiff);
            fadeSteps--;
        }
        else // fadeSteps < 0
        {
            sndsource->SetVolume(currentSourceVol - volumeDiff);
            fadeSteps++;
        }
    }

    // stop if needed
    if(fadeSteps == 0 && fadeStop)
    {
        sndstream->Pause();
    }
}

/*
 * Convert To 3D ;)
 * adds a 3D Source and a Directional 3D Source if theres a rad bigger 0
 */

void SoundHandle::ConvertTo3D(float mindist, float maxdist, csVector3 pos,
                              csVector3 dir, float rad)
{
    maxDistance = maxdist;

    SoundSystemManager::GetSingleton().GetSoundSystem()->Create3DSource(sndsource, sndsource3d, mindist, maxdist,
            pos);

    /* create a directional source if rad > 0 */
    if(rad > 0)
    {
        SoundSystemManager::GetSingleton().GetSoundSystem()->CreateDirectional3DSource(sndsource3d, sndsourcedir,
                dir, rad);
    }
}

bool SoundHandle::Is3D()
{
    if(sndsource3d != 0)
    {
        return true;
    }

    return false;
}

bool SoundHandle::IsDopplerEffectEnabled()
{
    return dopplerEffect;
}

bool SoundHandle::IsWithinMaximumDistance(csVector3 listenerPos) const
{
    csVector3 rangeVec;
    float range;

    rangeVec = sndsource3d->GetPosition() - listenerPos;
    range = rangeVec.Norm();

    if(range <= maxDistance)
    {
        return true;
    }

    return false;
}

csVector3 SoundHandle::GetSourcePosition()
{
    return sndsource3d->GetPosition();
}

void SoundHandle::SetAutoRemove(bool toggle)
{
    autoremove = toggle;
}

bool SoundHandle::GetAutoRemove()
{
    return (!delayActive && autoremove);
}

void SoundHandle::UnpauseAfterDelay(unsigned int delay)
{
    if(delay == 0)
    {
        sndstream->Unpause();
    }
    else
    {
        delayActive = true;
        SoundSystemManager::GetSingleton().eventTimer->AddTimerEvent(this, delay);
    }
}

void SoundHandle::SetCallback(void (*object), void (*function)(void*))
{
    callbackobject = object;
    callbackfunction = function;
    hasCallback = true;
}

void SoundHandle::Callback()
{
    if(hasCallback == true)
    {
        callbackfunction(callbackobject);
    }
}

void SoundHandle::RemoveCallback()
{
    hasCallback = false;
}

void SoundHandle::OnSoundChange(SoundControl* /*ctrl*/)
{
    SoundControl* mainSndCtrl = SoundSystemManager::GetSingleton().mainSndCtrl;

    // if sound are disabled we remove the sound
    if(!sndCtrl->GetToggle() || !mainSndCtrl->GetToggle())
    {
        sndstream->Pause();
        SetAutoRemove(true);
        return;
    }

    // if the handle is fading it will fade to the new volume
    if(fadeSteps == 0)
    {
        float newVolume = currentVolume * sndCtrl->GetVolume() * mainSndCtrl->GetVolume();

        if(newVolume >= VOLUME_MAX)
        {
            sndsource->SetVolume(VOLUME_MAX);
        }
        else
        {
            sndsource->SetVolume(newVolume);
        }
    }
}