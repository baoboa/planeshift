/*
 * system.cpp
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


#include "system.h"
#include "util/log.h"

/*
 * Get a renderer und returns false or true
 */

bool SoundSystem::Initialize (iObjectRegistry* objectReg)
{
    sndrenderer = csQueryRegistry<iSndSysRenderer> (objectReg);
    if(!sndrenderer.IsValid())
    {
        Error1("Failed to locate Sound renderer!");
        return false;
    }

    listener = sndrenderer->GetListener();
    if(!listener.IsValid())
    {
        Error1("Failed to get a sound global listener!");
        return false;
    }
    
    // TODO make configurable NOTE: dont use VOLUME_* defines here
    listener->SetRollOffFactor (1.0f);
    return true;
}

/*
 * create a _paused_ stream using a iSndSysData handle
 * loop >= 1 .. make it loop
 * type can be one of those:
 *
 * CS_SND3D_DISABLE=0
 * CS_SND3D_RELATIVE=1
 * CS_SND3D_ABSOLUTE=2
 *
 * modifications are done on objects to caller gave is
 * returns false or true
 */

bool SoundSystem:: CreateStream (csRef<iSndSysData> &snddata, int loop,
                                 int type, csRef<iSndSysStream> &sndstream)
{
    if ( !(sndstream = sndrenderer->CreateStream (snddata, type)))
    {
        Error2 ("Can't create stream for '%s'!",
                snddata->GetDescription());
        return false;
    }

    // make it loop if requested
    if (loop >= 1)
    {
        sndstream->SetLoopState (CS_SNDSYS_STREAM_LOOP);
    }
    else
    {
        sndstream->SetLoopState (CS_SNDSYS_STREAM_DONTLOOP);
    }

    return true;
}

/*
 * Removes the given stream
 */

void SoundSystem::RemoveStream(csRef<iSndSysStream> &sndstream)
{
    sndrenderer->RemoveStream(sndstream);
}

/*
 * Creates a source
 * if its 2D or 3D depends on the stream
 * volume is 1 by default - we set it to 0 - ALWAYS
 */

bool SoundSystem::CreateSource (csRef<iSndSysStream> &sndstream,
                                csRef<iSndSysSource> &sndsource)
{
    sndsource = sndrenderer->CreateSource (sndstream);
    sndsource->SetVolume (0);
    return true;
}

/*
 * removeing the source doesnt remove the stream!
 * this is important!
 */

void SoundSystem::RemoveSource (csRef<iSndSysSource> &sndsource)
{
    sndrenderer->RemoveSource (sndsource);
}


/*
 * this creates the 3d source out of an normal source
 * we need distance and positional parameters as well as volume
 * this thing doesnt make a sound .. yet .. because the stream is still paused
 * doesnt need a special remove functions because this is within openal
 * use RemoveSource to remove it
 */

void SoundSystem::Create3dSource (csRef<iSndSysSource> &sndsource,
                                  csRef<iSndSysSource3D> &sndsource3d,
                                  float mindist, float maxdist, csVector3 pos)
{
    sndsource3d = scfQueryInterface<iSndSysSource3D> (sndsource);
    sndsource3d->SetMinimumDistance(mindist);
    sndsource3d->SetMaximumDistance(maxdist);
    sndsource3d->SetPosition(pos);
}

/*
 * create a directional source out of a 3d source
 * we need the direction its sending to and the radius(?) of the cone it creates into
 * that direction - experimental
 * doesnt need a special remove functions because this is within openal
 * use RemoveSource to remove it
 */

void SoundSystem::CreateDirectional3dSource
                     (csRef<iSndSysSource3D>& /*sndsource3d*/,
                      csRef<iSndSysSource3DDirectionalSimple>& sndsourcedir,
                      csVector3 direction, float rad)
{
    sndsourcedir->SetDirection(direction);
    sndsourcedir->SetDirectionalRadiation(rad);
}

/*
 * Updates the listener position
 * v is position
 * f is front
 * t is top
 */

void SoundSystem::UpdateListener(csVector3 v, csVector3 f, csVector3 t)
{
    listener->SetPosition(v);
    listener->SetDirection(f,t);
}
