/*
 * songdata.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef SONGDATA_H
#define SONGDATA_H

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/refarr.h>
#include <iutil/document.h>
#include <isndsys/ss_data.h>
#include <csutil/csstring.h>
#include <csutil/scf_implementation.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <music/musicutil.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class Instrument;
struct ScoreStatistics;


/**
 * This struct keeps the data about the instrument and the musical sheet.
 */
struct SongData
{
    Instrument* instrument;             ///< the instrument that the player uses to play this song.
    uint divisions;                     ///< number of divisions of the quarter coded in the sheet.
    int fifths;                         ///< > 0 is the number of sharps, < 0 the number of flats.
    uint beats;                         ///< numerator of the time signature.
    uint beatType;                      ///< denumerator of the time signature.
    uint tempo;                         ///< suggested tempo in quarter notes per minute.
    ScoreStatistics scoreStats;         ///< keep the statistics of the score.
    csRefArray<iDocumentNode> measures; ///< the measures of the musical sheet.

    /**
     * Constructor. Initialize everything to 0.
     */
    SongData()
    {
        instrument = 0;
        divisions = 0;
        fifths = 0;
        beats = 0;
        beatType = 0;
        tempo = 0;
    }
};


/**
 * This implements a data class that can be used for the CS sound system to play a song
 * from a musical sheet.
 */
class SndSysSongData: public scfImplementation1<SndSysSongData, iSndSysData>
{
public:
    /**
     * Constructor.
     * @param instrument the instrument used to play the song.
     */
    SndSysSongData(Instrument* instrument);

    /**
     * Destructor.
     */
    ~SndSysSongData();

    /**
     * Reads the given musical sheet and extract all the necessary data from it.
     * @param musicalScore the musical score.
     * @return true if the score is valid, false if the initialization fails.
     */
    bool Initialize(csRef<iDocument> musicalScore);

    // From iSndSysData
    //------------------

    /**
     * Return the size of the song in bytes.
     */
    virtual size_t GetDataSize();

    /**
     * Return the size of the song in frames.
     */
    virtual size_t GetFrameCount();
    virtual const char* GetDescription();
    virtual const csSndSysSoundFormat* GetFormat();
    virtual void SetDescription(const char* description);

    // This function is called by the renderer
    virtual iSndSysStream* CreateStream(csSndSysSoundFormat* renderFormat, int mode3D);

private:
    SongData* songData;
    csString description;
};

#endif /* SONGDATA_H */

