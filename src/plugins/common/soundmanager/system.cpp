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

#include <iutil/cfgmgr.h>


bool SoundSystem::Initialize(iObjectRegistry* objectReg)
{
    float rollOff;

    sndRenderer = csQueryRegistry<iSndSysRenderer>(objectReg);
    if(!sndRenderer.IsValid())
    {
        Error1("Failed to locate Sound renderer!");
        return false;
    }

    listener = sndRenderer->GetListener();
    if(!listener.IsValid())
    {
        Error1("Failed to get a sound global listener!");
        return false;
    }

    // configuring roll off factor
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        rollOff = configManager->GetFloat("Planeshift.Sound.ListenerRollOff", DEFAULT_LISTENER_ROLL_OFF);
    }
    else
    {
        rollOff = DEFAULT_LISTENER_ROLL_OFF;
    }

    listener->SetRollOffFactor(rollOff);
    return true;
}

bool SoundSystem::CreateStream(csRef<iSndSysData> &sndData, bool loop,
                                 int type, csRef<iSndSysStream> &sndStream)
{
    // checking that SoundSystem has been correctly initialized
    if(!sndRenderer.IsValid())
    {
        return false;
    }

    // the sound renderer must create the stream and keep track of it
    sndStream = sndRenderer->CreateStream(sndData, type);
    if(!sndStream.IsValid())
    {
        Error2("Can't create stream for '%s'!", sndData->GetDescription());
        return false;
    }

    // make it loop if requested
    if(loop)
    {
        sndStream->SetLoopState(CS_SNDSYS_STREAM_LOOP);
    }
    else
    {
        sndStream->SetLoopState(CS_SNDSYS_STREAM_DONTLOOP);
    }

    return true;
}

void SoundSystem::RemoveStream(csRef<iSndSysStream> &sndStream)
{
    // checking that SoundSystem has been correctly initialized
    if(!sndRenderer.IsValid())
    {
        return;
    }

    // making sound renderer remove the stream from the system
    sndRenderer->RemoveStream(sndStream);
}

bool SoundSystem::CreateSource(csRef<iSndSysStream> &sndStream,
                               csRef<iSndSysSource> &sndSource)
{
    // checking that SoundSystem has been correctly initialized
    if(!sndRenderer.IsValid())
    {
        return false;
    }

    // the sound renderer must create the source and keep track of it
    sndSource = sndRenderer->CreateSource(sndStream);
    if(!sndSource.IsValid())
    {
        Error2("Can't create source for '%s'!", sndStream->GetDescription());
        return false;
    }

    // default volume is 1, we set to 0
    sndSource->SetVolume(0);

    return true;
}

void SoundSystem::RemoveSource(csRef<iSndSysSource> &sndSource)
{
    // checking that SoundSystem has been correctly initialized
    if(!sndRenderer.IsValid())
    {
        return;
    }

    // making sound renderer remove the source from the system
    sndRenderer->RemoveSource(sndSource);
}

void SoundSystem::Create3DSource(csRef<iSndSysSource> &sndSource,
                                 csRef<iSndSysSource3D> &sndSource3D,
                                 float minDist, float maxDist, csVector3 pos)
{
    // the renderer keep track of only one source; if the 2D source has been
    // created with type != CS_SND3D_DISABLE the it will have this interface
    sndSource3D = scfQueryInterface<iSndSysSource3D>(sndSource);

    // setting parameters
    sndSource3D->SetMinimumDistance(minDist);
    sndSource3D->SetMaximumDistance(maxDist);
    sndSource3D->SetPosition(pos);
}

/*
 * We need the direction its sending to and the radius(?) of the cone it creates into
 * that direction - experimental
 */
void SoundSystem::CreateDirectional3DSource
                     (csRef<iSndSysSource3D>& /*sndSource3D*/,
                      csRef<iSndSysSource3DDirectionalSimple> &sndSourceDir,
                      csVector3 direction, float rad)
{
    sndSourceDir->SetDirection(direction);
    sndSourceDir->SetDirectionalRadiation(rad);
}

void SoundSystem::UpdateListener(csVector3 v, csVector3 f, csVector3 t)
{
    // checking that SoundSystem has been correctly initialized
    if(!listener.IsValid())
    {
        return;
    }

    listener->SetPosition(v);
    listener->SetDirection(f,t);
}

csVector3 SoundSystem::GetListenerPosition() const
{
    // checking that SoundSystem has been correctly initialized
    if(!listener.IsValid())
    {
        return csVector3(0, 0, 0);
    }

    return listener->GetPosition();
}
