/*
 * songhandle.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "songhandle.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "songdata.h"
#include "songstream.h"
#include "manager.h"
#include "system.h"


SongHandle::SongHandle(csRef<iDocument> musicalSheet, Instrument* instr)
    : sheet(musicalSheet)
{
    instrument = instr;
}

SongHandle::~SongHandle()
{
    // instrument is deleted by InstrumentsManager
}

bool SongHandle::Init(const char* /* resName */, bool loop, float volumePreset,
                       int type, SoundControl* &ctrl, bool doppler)
{
    csRef<iSndSysData> sndData;
    SndSysSongData* songData = new SndSysSongData(instrument);

    if(!songData->Initialize(sheet))
    {
        return false;
    }

    sndData.AttachNew(songData);

    if(!SoundSystemManager::GetSingleton().GetSoundSystem()->CreateStream(sndData, loop, type, sndstream))
    {
        return false;
    }

    if(!SoundSystemManager::GetSingleton().GetSoundSystem()->CreateSource(sndstream, sndsource))
    {
        return false;
    }

    currentVolume = volumePreset;
    sndCtrl = ctrl;

    // name is not important here

    dopplerEffect = doppler;

    return true;
}
