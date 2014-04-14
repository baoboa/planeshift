/*
 * songdata.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

#include "songdata.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <isndsys/ss_structs.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "instrument.h"
#include "songstream.h"


SndSysSongData::SndSysSongData(Instrument* instrument)
    : scfImplementationType(this)
{
    // initializing SongData
    songData = new SongData();
    songData->instrument = instrument;
}

SndSysSongData::~SndSysSongData()
{
    delete songData;
}

bool SndSysSongData::Initialize(csRef<iDocument> musicalScore)
{
    // these will be copied to the uint variables of songData
    int iDivisions;
    int iBeats;
    int iBeatType;
    int iTempo;

    if(!psMusic::GetMeasures(musicalScore, songData->measures))
    {
        return false;
    }

    if(!psMusic::GetAttributes(musicalScore, iDivisions,
        songData->fifths, iBeats, iBeatType, iTempo))
    {
        return false;
    }

    if(!psMusic::GetStatistics(musicalScore, songData->scoreStats))
    {
        return false;
    }

    // setting songData's variables
    songData->divisions = (uint)iDivisions;
    songData->beats = (uint)iBeats;
    songData->beatType = (uint)iBeatType;
    songData->tempo = (uint)iTempo;

    return true;
}

size_t SndSysSongData::GetDataSize()
{
    const csSndSysSoundFormat* format = songData->instrument->GetFormat();

    return GetFrameCount() * (format->Bits / 8) * format->Channels;
}

size_t SndSysSongData::GetFrameCount()
{
    size_t numberOfSamples = songData->scoreStats.totalLength / 1000 * songData->instrument->GetFormat()->Freq;

    return numberOfSamples;
}

const char* SndSysSongData::GetDescription()
{
    return description.GetData();
}

const csSndSysSoundFormat* SndSysSongData::GetFormat()
{
    return songData->instrument->GetFormat();
}

void SndSysSongData::SetDescription(const char* desc)
{
    description = desc;
}

iSndSysStream* SndSysSongData::CreateStream(csSndSysSoundFormat* renderFormat, int mode3D)
{
    SndSysSongStream* s = new SndSysSongStream(this, songData, renderFormat, mode3D);
    return s;
}
