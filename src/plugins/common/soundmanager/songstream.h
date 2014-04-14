/*
 * songstream.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef SONGSTREAM_H
#define SONGSTREAM_H


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/ref.h>
#include <csutil/randomgen.h>
#include <csplugincommon/sndsys/sndstream.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class Instrument;
class SndSysSongData;
struct SongData;
struct csSndSysSoundFormat;

using namespace CS::SndSys;

class SndSysSongStream: public SndSysBasicStream
{
public:
    SndSysSongStream(csRef<SndSysSongData> soundData, SongData* songData, csSndSysSoundFormat* renderFormat, int mode3D);
    virtual ~SndSysSongStream();

    // From iSndSysStream
    //--------------------
    virtual void AdvancePosition(size_t frameDelta);
    virtual const char* GetDescription();
    virtual size_t GetFrameCount();

    // SndSysBasicStream overriding
    //------------------------------
    virtual bool AlwaysStream() const { return true; }
    virtual bool ResetPosition();
    virtual bool SetPosition(size_t newPosition) { return false; } // not supported
    virtual bool SetLoopBoundaries(size_t startPosition, size_t endPosition) { return false; } // not supported

private:
    bool isFinished;                    ///< true if it has been reach the end of the musical sheet.
    size_t currentMeasure;              ///< keeps track of the current measure of the musical sheet.
    size_t currentNote;                 ///< keeps track of the current note in the current measure.

    size_t lastRepeatStart;             ///< keeps track of where the repeat start.
    size_t lastRepeatEnd;               ///< keeps track of where the repeat end.
    int repeatCounter;                  ///< how many times the repeat must be execute again. -1 means no repeats active, 0 means no more repeats.
    csArray<uint> repeatsDone;          ///< The measures that have been already repeated.
    char* copyNoteBuffer;               ///< temporary buffer used for copying purposes. Allocated only if the instrument support polyphony.
    size_t lastNoteSize;                ///< duration of the last chord read by GetNextChord() in bytes.

    SongData* songData;                 ///< the song's data.
    float timePerDivision;              ///< the time per divisionin seconds of this song.
    csRef<SndSysSongData> soundData;    ///< the sound data object.

    int conversionFactor;               ///< the multiplier used to get the data size from the data's format to the stream's one.
    bool conversionNeeded;              ///< true if the data from the instrument must be converted into the stream's format.

    /**
     * Check if m_bPlaybackReadComplete must be set to true and if it must it does
     * it. This function should be called everytime CopyBufferBytes() is used.
     *
     * @return the new value of m_bPlaybackReadComplete.
     */
    bool CheckPlaybackReadComplete();

    /**
     * Fills noteBuffer with the next chord of the song if it is not finished yet.
     * Updates lastDuration, currentMeasure and currentNote.
     *
     * @param noteBuffer the pointer at the end will contain the note data.
     * @param noteBufferSize at the end this parameter will contain the size of the
     * data buffer in bytes.
     * @return true if the musicalSheet has still notes to be read, false otherwise.
     */
    bool GetNextChord(char* &noteBuffer, size_t &noteBufferSize);

    /**
     * This is an helper function that adjust the alteration of the note depending
     * on the tonality of the song. If the note is already altered (alter != 0)
     * nothing happens.
     *
     * @param pitch a char representing the note in the British English notation
     * (i.e. A, B, C, ..., G).
     * @param alter the alteration variable that must be adjusted.
     */
    void AdjustAlteration(char pitch, int &alter);

    /**
     * Copies the note in noteBuffer into m_pPreparedDataBuffer and add 0 at its
     * end if lastNoteDuration > noteBufferSize. This method assumes that the
     * prepared data buffer is already large enough.
     *
     * @param noteBuffer the note's data.
     * @param noteBufferSize the size of the buffer.
     */
    void CopyNoteIntoBuffer(char* noteBuffer, size_t noteBufferSize);
};

#endif /* SONGSTREAM_H */