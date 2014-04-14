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
#include "handle.h"
#include "control.h"
#include "system.h"
#include "data.h"
#include "util/log.h"

/*
 * Initialize the SoundSystem (SndSys) and the Datamanager (SndData)
 * Load our soundlib.xml - FIXME HARDCODED
 *
 * Set Initialized to true if successfull / to false if not
 */

SoundSystemManager::SoundSystemManager (iObjectRegistry* objectReg)
{
    // Initialised to false to make sure it is ..
    Initialised = false;

    // Create a new SoundSystem and SoundData Instance
    soundSystem = new SoundSystem;
    soundData = new SoundData;

    // SoundControls used by GUI and EFFECTS - FIXME (to be removed)
    mainSndCtrl = GetSoundControl();
    guiSndCtrl = GetSoundControl();
    effectSndCtrl = GetSoundControl();

    if (soundSystem->Initialize (objectReg)
        && soundData->Initialize (objectReg))
    {
        //  soundLib = cfg->GetStr("PlaneShift.Sound.SoundLib", "/planeshift/art/soundlib.xml"); /* FIXME HARDCODED*/
        // also FIXME what if soundlib.xml doesnt exist?
        soundData->LoadSoundLib ("/planeshift/art/soundlib.xml", objectReg);
        LastUpdateTime = csGetTicks();
        Initialised = true;
    }
}

SoundSystemManager::~SoundSystemManager ()
{
    Initialised = false;
    // pause all sounds and call updatesound to remove them
    for (size_t i = 0; i < soundHandles.GetSize (); i++)
    {
        soundHandles[i]->sndstream->Pause();
        soundHandles[i]->SetAutoRemove(true);
    }

    UpdateSound();

    soundController.DeleteAll();

    delete soundSystem;
    delete soundData;

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

void SoundSystemManager::Update ()
{
    SndTime = csGetTicks();

    // call it all 100 Ticks
    if (Initialised && LastUpdateTime + 100 <= SndTime)
    {
        UpdateSound();
        // make a update on sounddata to check if there are sounds to unload
        soundData->Update();
    }
}

/*
 * play a 2D sound
 * means: no 3D sound, no position etc needed
 * Note: You cant convert a 2D to a 3D sound
 * you need to supply a handle
 */

bool SoundSystemManager::
Play2DSound (const char *name, bool loop, size_t loopstart, size_t loopend,
             float volume_preset, SoundControl* &sndCtrl, SoundHandle* &handle)
{
    /* FIXME redundant code Play3DSound */

    if (Initialised == false)
    {
        Debug1(LOG_SOUND,0,"Sound not Initialised\n");
        return false;
    }

    if (name == NULL)
    {
        Error1("Error: Play2DSound got NULL as soundname\n");
        return false;
    }

    if (sndCtrl->GetToggle() == false) /* FIXME */
    {
        return false;
    }

    handle = new SoundHandle(this);

    if (!handle->Init(name, loop, volume_preset, CS_SND3D_DISABLE, sndCtrl))
    {
      delete handle;
      handle = NULL;
      return false;
    }

    handle->sndstream->SetLoopBoundaries(loopstart, loopend);
    handle->sndsource->SetVolume(( volume_preset * sndCtrl->GetVolume()));

    handle->sndstream->Unpause();
    soundHandles.Push(handle);

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
Play3DSound (const char *name, bool loop, size_t loopstart, size_t loopend,
             float volume_preset, SoundControl* &sndCtrl, csVector3 pos,
             csVector3 dir, float mindist, float maxdist, float rad,
             int type3d, SoundHandle* &handle)
{
    // FIXME redundant code Play2DSound
    if (Initialised == false)
    {
        Debug1(LOG_SOUND,0,"Sound not Initialised\n");
        return false;
    }

    if (name == NULL)
    {
        Error1("Error: Play2DSound got NULL as soundname\n");
        return false;
    }

    if (sndCtrl->GetToggle() == false) /* FIXME */
    {
        return false;
    }

    handle = new SoundHandle(this);

    if (!handle->Init(name, loop, volume_preset, type3d, sndCtrl))
    {
      delete handle;
      handle = NULL;
      return false;
    }

    handle->sndstream->SetLoopBoundaries(loopstart, loopend);
    handle->sndsource->SetVolume((volume_preset * sndCtrl->GetVolume()));

    /* make it 3d */
    handle->ConvertTo3D (mindist, maxdist, pos, dir, rad);
    handle->sndstream->Unpause();

    soundHandles.Push(handle);
    return true;
}

void SoundSystemManager::StopSound (SoundHandle *handle)
{
    soundHandles.Delete(handle);
}

/*
 * reomves all idle sounds to save memory and sndsources
 * adjust volumes and fades music
 * TODO Split into three parts and make it event based
 */

void SoundSystemManager::UpdateSound ()
{
    float vol;
    SoundHandle* tmp = NULL;

    for (size_t i = 0; i < soundHandles.GetSize (); i++)
    {
        tmp = soundHandles.Get(i);

        if (tmp->sndstream->GetPauseState () == CS_SNDSYS_STREAM_PAUSED
            && tmp->GetAutoRemove() == true)
        {
            StopSound(tmp);
            i--;
            continue;
        }

        // fade in or out
        // fade >0 is number of steps up <0 is number of steps down, 0 is nothing
        if (tmp->fade > 0)
        {
            tmp->sndsource->SetVolume(tmp->sndsource->GetVolume()
                                      + ((tmp->fade_volume
                                          * tmp->sndCtrl->GetVolume())
                                         * mainSndCtrl->GetVolume()));
            tmp->fade--;
        }
        else if (tmp->fade < 0)
        {
            /*
             *  fading down means we might want to stop the sound
             * if fade_stop is set do that (instead of the last step)
             * dont delete it here it would ruin the Array
             * our "garbage collector (UpdateSounds)" will pick it up
             *
             * also check the toggle just pause if its false
             */

            if ((tmp->fade == -1
                && tmp->fade_stop == true)
                || tmp->sndCtrl->GetToggle() == false)
            {
                StopSound(tmp);
                i--;
                continue;
            }
            else
            {
                tmp->sndsource->SetVolume(tmp->sndsource->GetVolume()
                                          - ((tmp->fade_volume
                                              * tmp->sndCtrl->GetVolume())
                                             * mainSndCtrl->GetVolume()));
                tmp->fade++;
            }
        }
        else if (tmp->sndCtrl->GetToggle() == true)
        {
            if (mainSndCtrl->GetToggle() == false)
            {
                vol = VOLUME_ZERO;
            }
            else
            {
                vol = ((tmp->preset_volume * tmp->sndCtrl->GetVolume())
                       * mainSndCtrl->GetVolume());
            }

            // limit volume to 2.0f (VOLUME_MAX defined in manager.h)
            if (vol >= VOLUME_MAX)
            {
                tmp->sndsource->SetVolume(VOLUME_MAX);
            }
            else
            {
                tmp->sndsource->SetVolume(vol);
            }
        }
      LastUpdateTime = csGetTicks();
    }
}


/*
 * Update Listener position
 */

void SoundSystemManager::UpdateListener (csVector3 v, csVector3 f, csVector3 t)
{
    soundSystem->UpdateListener (v, f, t);
}

SoundControl* SoundSystemManager::GetSoundControl ()
{
    SoundControl *newControl;

    newControl = new SoundControl;
    soundController.Push(newControl);
    newControl->id = soundController.GetSize();

    return newControl;
}

