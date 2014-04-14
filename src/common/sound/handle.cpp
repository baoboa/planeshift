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
#include "control.h"

/*
 * A Soundhandle contains all informations we have about a sound
 * source stream 2D/3D and its completly public
 * and provides a interface to the source stream
 * volumes, fading parameters
 */

SoundHandle::SoundHandle (SoundSystemManager* manager):
    sndCtrl(NULL), preset_volume(false), fade_volume(0.0f), callbackobject(NULL)
{
    fade = 0;
    fade_stop = false;
    autoremove = true;
    sndsource = NULL;
    sndstream = NULL;
    hasCallback = false;
    this->manager = manager;
    callbackfunction = NULL;
}

SoundHandle::~SoundHandle ()
{
    if (hasCallback == true)
    {
        Callback();
    }
    /*
     * i was running into a bug because i didnt do the isvalid / NULL
     * checks ..
     * sources get removed even if you dont do RemoveSource
     * but it will not dereference them! ..
     */
    if (sndstream != NULL)
    {
        manager->GetSoundSystem()->RemoveStream(sndstream);
    }

    if (sndsource != NULL)
    {
        manager->GetSoundSystem()->RemoveSource(sndsource);
    }

}

/*
 * utilize all the SndSys and SndData function to create a real sound
 * error msgs are handled by the SndSys / SndData functions
 * all we do is handle results
 *
 * the soundhandles which contain all our precious informations
 * are created here. Not that those are NOT pushed into the array!
 * Thats done within the Play*D functions before the sounds are unpaused
 */

bool SoundHandle::Init (const char *resname, bool loop, float volume_preset,
                        int type, SoundControl* &ctrl)
{
    csRef<iSndSysData>  snddata; 

    if (!manager->GetSoundData()->LoadSoundFile(resname, snddata))
    {
        return false;
    }

    if (!manager->GetSoundSystem()->CreateStream(snddata, loop, type, sndstream))
    {
        manager->GetSoundData()->UnloadSoundFile(resname);
        return false;
    }

    manager->GetSoundSystem()->CreateSource(sndstream, sndsource);
    preset_volume = volume_preset;
    sndCtrl = ctrl;
    name = csString(resname);

    return true;
}

 /*
  * This is a utility function to calculate fading
  *
  * fading is done in ten steps per second
  * we calculate the number of steps and the volume for each step
  * FIXME csticks
  *
  * fading parameters of the given handle are updated.
  * Fading is done by SndSysMgr::UpdateSound
  *
  */


void SoundHandle::Fade (float volume, int time, int direction)
{
    volume = volume*sndCtrl->GetVolume();

    if (direction == FADE_UP)
    {
      fade = (0 + (time / 100));
    }
    else /* FADE_DOWN + FADE_STOP */
    {
      fade = (0 - (time / 100));
    }

    fade_volume = (volume / (time / 100));

    if (direction == FADE_STOP)
    {
      fade_stop = true;
    }
    else /* sanity */
    {
      fade_stop = false;
    }
}

 /*
  * Convert To 3D ;)
  * adds a 3D Source and a Directional 3D Source if theres a rad bigger 0
  */

void SoundHandle::ConvertTo3D (float mindist, float maxdist, csVector3 pos,
                               csVector3 dir, float rad)
{
    manager->GetSoundSystem()->Create3dSource (sndsource, sndsource3d, mindist, maxdist,
                                 pos);

    /* create a directional source if rad > 0 */
    if (rad > 0)
    {
        manager->GetSoundSystem()->CreateDirectional3dSource (sndsource3d, sndsourcedir,
                                                dir, rad);
    }
}

void SoundHandle::SetAutoRemove (bool toggle)
{
    autoremove = toggle;
}

bool SoundHandle::GetAutoRemove ()
{
    return autoremove;
}

void SoundHandle::SetCallback(void (*object), void (*function) (void *))
{
    callbackobject = object;
    callbackfunction = function;
    hasCallback = true; 
}

void SoundHandle::Callback ()
{
    if (hasCallback == true)
    {
        callbackfunction(callbackobject);
    }
}

void SoundHandle::RemoveCallback()
{
    hasCallback = false;
}
