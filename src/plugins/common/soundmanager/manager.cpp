/*
 * manager.cpp
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

#include "manager.h"

#include "soundmanager.h"
#include "handle.h"
#include "system.h"
#include "data.h"

#include <util/log.h>


SoundSystemManager::SoundSystemManager()
{
    // Initialised to false to make sure it is ..
    Initialised = false;

    playerPosition.Set(0.0);
    playerVelocity.Set(0.0);

    soundSystem = new SoundSystem;
    soundDataCache = new SoundDataCache;
    mainSndCtrl = new SoundControl(-1);
}

SoundSystemManager::~SoundSystemManager()
{
    Initialised = false;

    // pause all sounds and call updatesound to remove them
    csHash<SoundHandle*, uint>::GlobalIterator handleIter(soundHandles.GetIterator());
    SoundHandle* sh;

    while(handleIter.HasNext())
    {
        sh = handleIter.Next();
        sh->sndstream->Pause();
        sh->SetAutoRemove(true);
    }

    UpdateSound();

    // Deleting SoundControls
    csHash<SoundControl*, uint>::GlobalIterator controlIter(soundControllers.GetIterator());
    SoundControl* sc;

    while(controlIter.HasNext())
    {
        sc = controlIter.Next();
        delete sc;
    }

    soundHandles.DeleteAll();
    soundControllers.DeleteAll();

    delete mainSndCtrl;
    delete soundSystem;
    delete soundDataCache;

}

/*
 * Initialize the SoundSystem (SndSys) and the Datamanager (SndData)
 * Load our sound library
 *
 * Set Initialized to true if successfull / to false if not
 */
bool SoundSystemManager::Initialize(iObjectRegistry* objectReg)
{
    const char* soundLib;

    // Configuration
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        speedOfSound = configManager->GetInt("Planeshift.Sound.SpeedOfSound", DEFAULT_SPEED_OF_SOUND);
        dopplerFactor = configManager->GetFloat("Planeshift.Sound.DopplerFactor", DEFAULT_DOPPLER_FACTOR);
        updateTime = configManager->GetInt("Planeshift.Sound.SndSysUpdateTime", DEFAULT_SNDSYS_UPDATE_TIME);
        soundLib = configManager->GetStr("Planeshift.Sound.SoundLib", DEFAULT_SOUNDLIB_PATH);
    }
    else
    {
        speedOfSound = DEFAULT_SPEED_OF_SOUND;
        dopplerFactor = DEFAULT_DOPPLER_FACTOR;
        updateTime = DEFAULT_SNDSYS_UPDATE_TIME;
        soundLib = DEFAULT_SOUNDLIB_PATH;
    }

    // Initializing the event timer
    eventTimer = csEventTimer::GetStandardTimer(objectReg);

    if(soundSystem->Initialize(objectReg)
       && soundDataCache->Initialize(objectReg))
    {
        // FIXME what if soundlib.xml doesnt exist?
        soundDataCache->LoadSoundLib(soundLib, objectReg);
        LastUpdateTime = csGetTicks();
        Initialised = true;
    }

    return Initialised;
}

/*
 * Main Update function which is called by the engine
 * it wont do anything if not Initialised
 * slowdown is done here because we dont need that many updates
 *
 * there should be 50ms betweens the updates
 * and MUST be 100ms for FadeSounds
 * csTicks are not THAT accurate..
 * and because of that its now a FIXME 
 * i knew that we would run into this :/
 * but i already have a plan.. :)   
 */

void SoundSystemManager::Update()
{
    SndTime = csGetTicks();

    // call it all 100 Ticks
    if(Initialised && LastUpdateTime + updateTime <= SndTime)
    {
        UpdateSound();
        // make a update on sounddata to check if there are sounds to unload
        soundDataCache->Update();
    }
}

/*
 * play a 2D sound
 * means: no 3D sound, no position etc needed
 * Note: You cant convert a 2D to a 3D sound
 * you need to supply a handle
 */

bool SoundSystemManager::
Play2DSound(const char* name, bool loop, size_t loopstart, size_t loopend,
            float volume_preset, SoundControl* &sndCtrl, SoundHandle* &handle)
{
    if(!InitSoundHandle(name, loop, loopstart, loopend, volume_preset, CS_SND3D_DISABLE, sndCtrl, handle, false))
    {
        return false;
    }

    handle->sndstream->Unpause();

    return true;
}

/*
 * play a 3D sound
 * we need volume, position, direction, radiation, min- and maxrange.
 * 3dtype (absolute / relative
 * and direction if radiation is not 0
 * you need to supply a handle
 */

bool SoundSystemManager::
Play3DSound(const char* name, bool loop, size_t loopstart, size_t loopend,
            float volume_preset, SoundControl* &sndCtrl, csVector3 pos,
            csVector3 dir, float mindist, float maxdist, float rad,
            int type3d, SoundHandle* &handle, bool dopplerEffect)
{
    if(!Initialised)
    {
        return false;
    }

    // checking if the listener is within the given range
    // TODO REMOVE THIS WHEN CS WILL MANAGE CORRECTLY MAXDIST
    csVector3 rangeVec = pos - soundSystem->GetListenerPosition();
    float range = rangeVec.Norm();

    if(range > maxdist)
    {
        return false;
    }

    // Initializing
    if(!InitSoundHandle(name, loop, loopstart, loopend, volume_preset, type3d, sndCtrl, handle, dopplerEffect))
    {
        return false;
    }

    /* make it 3d */
    handle->ConvertTo3D(mindist, maxdist, pos, dir, rad);

    /* TODO RIGHT NOW THE DOPPLER EFFECT DOESN'T WORK PROPERLY
    if(dopplerEffect)
    {
        // computing the delay caused by the speed of sound
        csVector3 diff = pos - soundSystem->GetListenerPosition();
        float distance = diff.Norm();
        unsigned int delay = distance * 1000 / speedOfSound;

        handle->UnpauseAfterDelay(delay);
    }
    else
    {*/
        handle->sndstream->Unpause();
    //}

    return true;
}

bool SoundSystemManager::StopSound(uint handleID)
{
    SoundHandle* handle = soundHandles.Get(handleID, 0);
    if(handle == 0)
    {
        return false;
    }

    // Pause the sound and set autoremove
    // The handle will be removed in the next update
    handle->sndstream->Pause();
    handle->SetAutoRemove(true);

    return true;
}

bool SoundSystemManager::SetSoundSource(uint handleID, csVector3 position)
{
    SoundHandle* handle = soundHandles.Get(handleID, 0);
    if(handle == 0)
    {
        return false;
    }

    handle->sndsource3d->SetPosition(position);

    return true;
}

void SoundSystemManager::SetPlayerPosition(csVector3& pos)
{
    playerPosition = pos;
}

csVector3& SoundSystemManager::GetPlayerPosition()
{
    return playerPosition;
}

void SoundSystemManager::SetPlayerVelocity(csVector3 vel)
{
    playerVelocity = vel;
}

bool SoundSystemManager::IsHandleValid(uint handleID) const
{
    return soundHandles.Contains(handleID);
}

/*
 * reomves all idle sounds to save memory and sndsources
 * adjust volumes and fades music
 * TODO Split into three parts and make it event based
 */

void SoundSystemManager::UpdateSound()
{
    SoundHandle* sh;
    csArray<SoundHandle*> handles = soundHandles.GetAll();

    for(size_t i = 0; i < handles.GetSize(); i++)
    {
        sh = handles[i];

        if(sh->sndstream->GetPauseState() == CS_SNDSYS_STREAM_PAUSED
            && sh->GetAutoRemove() == true)
        {
            RemoveHandle(sh->GetID());
            continue;
        }

        // applying Doppler effect
        if(sh->Is3D())
        {
            if(!sh->IsWithinMaximumDistance(soundSystem->GetListenerPosition()))
            {
                StopSound(sh->GetID());
                continue;
            }

            if(sh->IsDopplerEffectEnabled())
            {
                ChangePlayRate(sh);
            }
        }

        // fading if needed
        sh->FadeStep();

        LastUpdateTime = csGetTicks();
    }
}

void SoundSystemManager::UpdateListener(csVector3 v, csVector3 f, csVector3 t)
{
    soundSystem->UpdateListener(v, f, t);
}

csVector3 SoundSystemManager::GetListenerPos() const
{
    return soundSystem->GetListenerPosition();
}

SoundControl* SoundSystemManager::AddSoundControl(uint sndCtrlID)
{
    SoundControl* newControl;

    if(soundControllers.Get(sndCtrlID, 0) != 0)
    {
        return 0;
    }

    newControl = new SoundControl(sndCtrlID);
    soundControllers.Put(sndCtrlID, newControl);

    return newControl;
}

void SoundSystemManager::RemoveSoundControl(uint sndCtrlID)
{
    SoundControl* sndCtrl = soundControllers.Get(sndCtrlID, 0);

    soundControllers.Delete(sndCtrlID, sndCtrl);
    delete sndCtrl;
}

SoundControl* SoundSystemManager::GetSoundControl(uint sndCtrlID) const
{
    return soundControllers.Get(sndCtrlID, 0);
}

uint SoundSystemManager::FindHandleID()
{
    uint handleID;

    do
    {
        handleID = SoundManager::randomGen.Get(10000);
    } while(soundHandles.Get(handleID, 0) != 0
        || handleID == 0);

    return handleID;
}

void SoundSystemManager::RemoveHandle(uint handleID)
{
    SoundHandle* handle = soundHandles.Get(handleID, 0);
    if(handle == 0)
    {
        return;
    }

    soundHandles.Delete(handleID, handle);
    delete handle;
}

void SoundSystemManager::ChangePlayRate(SoundHandle* handle)
{
    int percentRate;
    float distance;
    float distanceAfterTimeUnit;
    float relativeSpeed;
    csVector3 sourcePosition;

    sourcePosition = handle->GetSourcePosition();
    distance = (playerPosition - sourcePosition).Norm();
    distanceAfterTimeUnit = (playerPosition + playerVelocity - sourcePosition).Norm();
    relativeSpeed = distanceAfterTimeUnit - distance;
    percentRate = (1 - dopplerFactor * relativeSpeed / speedOfSound) * 100;

    handle->sndstream->SetPlayRatePercent(percentRate);
}

bool SoundSystemManager::
InitSoundHandle(const char* name, bool loop, size_t loopstart, size_t loopend,
            float volume_preset, int type3d, SoundControl* &sndCtrl, SoundHandle* &handle, bool dopplerEffect)
{
    uint handleID;

    if(Initialised == false)
    {
        Debug1(LOG_SOUND,0,"Sound not Initialised\n");
        return false;
    }

    if(name == NULL)
    {
        Error1("Error: PlaySound got NULL as soundname\n");
        return false;
    }

    if(sndCtrl == 0)
    {
        return false;
    }

    if(sndCtrl->GetToggle() == false) /* FIXME */
    {
        return false;
    }

    handleID = FindHandleID();
    if(handle == 0)
    {
        handle = new SoundHandle();
    }
    handle->SetID(handleID);

    if(!handle->Init(name, loop, volume_preset, type3d, sndCtrl, dopplerEffect))
    {
      delete handle;
      handle = 0;
      return false;
    }

    handle->sndstream->SetLoopBoundaries(loopstart, loopend);
    handle->sndsource->SetVolume((volume_preset * mainSndCtrl->GetVolume() * sndCtrl->GetVolume()));

    soundHandles.Put(handleID, handle);

    return true;
}
