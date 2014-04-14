/*
 * musicutil.h, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef MUSIC_UTIL_H
#define MUSIC_UTIL_H


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/refarr.h>
#include <csutil/csstring.h>
#include <iutil/document.h>

/**
 * \addtogroup common_music
 * @{ */

/**
 * This struct keeps general information about a score.
 *
 * All average statistics are
 * computed by using the total length without rests. If the score is empty (or has
 * only rests), the average polyphony and duration are set to 0.
 */
struct ScoreStatistics
{
    /**
     * Constructor.
     *
     * Initialize all statistics to 0.
     */
    ScoreStatistics()
    {
        Reset();
    }

    /**
     * Copy constructor.
     */
    ScoreStatistics(const ScoreStatistics &copy)
    {
        nNotes = copy.nNotes;
        nChords = copy.nChords;
        nBb = copy.nBb;
        nGb = copy.nGb;
        nEb = copy.nEb;
        nDb = copy.nDb;
        nAb = copy.nAb;

        totalLength = copy.totalLength;
        totalLengthNoRests = copy.totalLengthNoRests;

        minimumDuration = copy.minimumDuration;
        maximumPolyphony = copy.maximumPolyphony;
        averageDuration =copy.averageDuration;
        averagePolyphony = copy.averagePolyphony;

        tempo = copy.tempo;
        fifths = copy.fifths;
        beatType = copy.beatType;
    }

    /**
     * Adds all statistics concerning number of notes/chords and length. It does not
     * change minimum,maximum and average statistics nor beat type and fifths.
     * @param addend the statistics that must be added to these.
     * @return these updated statistics.
     */
    ScoreStatistics &operator+=(const ScoreStatistics &addend)
    {
        nNotes += addend.nNotes;
        nChords += addend.nChords;
        nBb += addend.nBb;
        nGb += addend.nGb;
        nEb += addend.nEb;
        nDb += addend.nDb;
        nAb += addend.nAb;

        totalLength += addend.totalLength;
        totalLengthNoRests += addend.totalLengthNoRests;

        return *this;
    }

    /**
     * Subtracts all statistics concerning number of notes/chords and length. It does
     * not change minimum,maximum and average statistics nor beat type and fifths.
     * @param subtrahend the statistics that must be subtracted to these.
     * @return these updated statistics.
     */
    ScoreStatistics &operator-=(const ScoreStatistics &subtrahend)
    {
        nNotes -= subtrahend.nNotes;
        nChords -= subtrahend.nChords;
        nBb -= subtrahend.nBb;
        nGb -= subtrahend.nGb;
        nEb -= subtrahend.nEb;
        nDb -= subtrahend.nDb;
        nAb -= subtrahend.nAb;

        totalLength -= subtrahend.totalLength;
        totalLengthNoRests -= subtrahend.totalLengthNoRests;

        return *this;
    }

    /**
     * Multiplies all statistics concerning number of notes/chords and length for
     * a integer scalar. It does not change minimum,maximum and average statistics
     * nor beat type and fifths.
     * @param scalar the integer that multiplies these statistics.
     * @return these updated statistics.
     */
    ScoreStatistics &operator*=(const int scalar)
    {
        nNotes *= scalar;
        nChords *= scalar;
        nBb *= scalar;
        nGb *= scalar;
        nEb *= scalar;
        nDb *= scalar;
        nAb *= scalar;

        totalLength *= scalar;
        totalLengthNoRests *= scalar;

        return *this;
    }

    /**
     * Multiplies all statistics concerning number of notes/chords and length for a integer scalar.
     *
     * It does not change minimum,maximum and average statistics nor beat type and fifths.
     *
     * @param scalar the integer that multiplies these statistics.
     * @return the updated statistics.
     */
    const ScoreStatistics operator*(const int scalar)
    {
        ScoreStatistics result(*this);
        result *= scalar;
        return result;
    }

    /**
     * Set all statistics to 0.
     */
    void Reset()
    {
        nNotes = 0;
        nChords = 0;
        nBb = 0;
        nGb = 0;
        nEb = 0;
        nDb = 0;
        nAb = 0;

        totalLength = 0.0;
        totalLengthNoRests = 0.0;

        minimumDuration = 0;
        maximumPolyphony = 0;
        averageDuration = 0.0;
        averagePolyphony = 0.0;

        tempo = 0;
        fifths = 0;
        beatType = 0;
    }

    int nNotes;                 ///< total number of played notes in the score.
    int nChords;                ///< total number of played chords in the score.
    int nBb;                    ///< total number of played Bb.
    int nGb;                    ///< total number of played Gb.
    int nEb;                    ///< total number of played Eb.
    int nDb;                    ///< total number of played Db.
    int nAb;                    ///< total number of played Ab.

    float totalLength;          ///< total duration of the score in milliseconds.
    float totalLengthNoRests;   ///< total duration of the score in ms excluding rests.

    int minimumDuration;        ///< duration of the shortest note in the score in ms.
    int maximumPolyphony;       ///< maximum number of notes played at the same time.
    float averageDuration;      ///< average duration of the score's notes in ms.
    float averagePolyphony;     ///< average number of notes played at the same time.

    int tempo;                  ///< tempo of the score.
    int fifths;                 ///< tonality as number of sharps (if > 0) or flats (if < 0).
    int beatType;               ///< beat type of the score.
};


/**
 * This namespace contains a set of functions that are usefull for the processing of music
 * and musical scores.
 */
namespace psMusic
{

enum Accidental
{
    NO_ACCIDENTAL,
    DOUBLE_FLAT,
    FLAT,
    NATURAL,
    SHARP,
    DOUBLE_SHARP
};

/**
 * Unit measure for duration in in terms of divisions per quarter. If you change this
 * constants you must change the values in the enum Duration too.
 */
#define DURATION_QUARTER_DIVISIONS 16 // unit measure is a sixteenth

/**
 * The number associated to each duration is the number of quarter divisions as specified
 * in DURATION_QUARTER_DIVISIONS.
 */
enum Duration
{
    SIXTEENTH_DURATION      = 1,
    EIGHTH_DURATION         = 2,
    DOTTED_EIGHTH_DURATION  = 3,
    QUARTER_DURATION        = 4,
    DOTTED_QUARTER_DURATION = 6,
    HALF_DURATION           = 8,
    DOTTED_HALF_DURATION    = 12,
    WHOLE_DURATION          = 16,
    DOTTED_WHOLE_DURATION   = 24
};


/**
 * Returns the biggest duration that can be represented on the score which is less
 * or equal to the given one.
 *
 * @param duration The maximum duration as the number of DURATION_QUARTER_DIVISIONS.
 * @return The duration value.
 */
Duration GetBiggestDuration(int duration);

/**
 * Turns the given pitch into the next one in the scale.
 *
 * @param pitch the pitch of the note.
 * @param octave the octave of the note.
 */
void NextPitch(char &pitch, uint &octave);

/**
 * Turns the given pitch into the previous one in the scale.
 *
 * @param pitch the pitch of the note.
 * @param octave the octave of the note.
 */
void PreviousPitch(char &pitch, uint &octave);

/**
 * Turns the given pitch into the enharmonic equivalent.
 * @param pitch the pitch of the note.
 * @param accidental the alteration of the note.
 */
void EnharmonicPitch(char &pitch, int &accidental);

/**
 * Gets the XML nodes representing the measures contained in the musical score.
 *
 * @param score the musical score.
 * @param measures a reference to a csRefArray that will contain the XML nodes.
 * @return true if the document is a valid musical score, false otherwise.
 */
bool GetMeasures(iDocument* score, csRefArray<iDocumentNode> &measures);

/**
 * Returns the statistics of the score.
 *
 * @param musicalScore the musical score.
 * @param stats the retrieved statistics of the given score.
 * @return true if the document is a valid musical score, false otherwise.
 */
bool GetStatistics(iDocument* musicalScore, ScoreStatistics &stats);

/**
 * Gets the attributes in the first measure of the given score.
 *
 * The provided attributes are always valid. If the musical score contains non valid attributes
 * (e.g. beatType <= 0), they are set to a valid default value.
 *
 * @param musicalScore the musical score.
 * @param quarterDivisions the number of divisions in a quarter for the score.
 * @param fifths the tonality of the score.
 * @param beats beats of the song.
 * @param beatType the beat type of the song.
 * @param tempo the beat per minutes of the song.
 * @return true if the document is a valid musical score and the attributes could be
 * found in the first measure, false otherwise. False is returned also if there are
 * not measures in the given score.
 */
bool GetAttributes(iDocument* musicalScore, int &quarterDivisions,
                   int &fifths, int &beats, int &beatType, int &tempo);

/**
 * Compress a song with the zlib compression algorithm.
 *
 * @attention the output string is not a normal string but a sequence of bytes. It
 * could contain null characters. Do not treat it like a null terminated string.
 *
 * @param inputScore the musical sheet in uncompressed XML format.
 * @param outputScore at the end this string will contain the compressed score.
 * @return true if the compression is done without errors, false otherwise.
 */
bool ZCompressSong(const csString &inputScore, csString &outputScore);

/**
 * Decompress a song with the zlib compression algorithm.
 *
 * @param inputScore the compressed musical sheet.
 * @param outputScore at the end this string will contain the uncompressed score.
 * @return true if the decompression is done without errors, false otherwise.
 */
bool ZDecompressSong(const csString &inputScore, csString &outputScore);

/**
 * Checks if the given document is a valid musical score and provide the \<part\> node.
 *
 * @param musicalScore the musical score.
 * @param partNode a reference that will contain the part XML node.
 * @return true if the document is valid, false otherwise.
 */
bool CheckValidity(iDocument* musicalScore, csRef<iDocumentNode> &partNode);

}

/** @} */

#endif // MUSIC_UTIL_H

