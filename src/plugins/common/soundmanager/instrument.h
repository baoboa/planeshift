/*
 * instrument.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef INSTRUMENT_H
#define INSTRUMENT_H

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/hash.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
struct iSndSysStream;
struct csSndSysSoundFormat;


/**
 * This struct represents a musical note. It keeps the stream buffer of the normal
 * note, the sharp and the flat ones when available;
 */
struct Note
{
    char* normal;           ///< the buffer of the unaltered note.
    char* sharp;            ///< the buffer of the sharp note.
    char* flat;             ///< the buffer of the flat note.

    size_t normalLength;    ///< the length of the buffer of the unaltered note.
    size_t sharpLength;     ///< the length of the buffer of the sharp note.
    size_t flatLength;      ///< the length of the buffer of the flat note.

    /**
     * Constructor. It initializes everything to 0.
     */
    Note()
    {
        normal = 0;
        sharp = 0;
        flat = 0;

        normalLength = 0;
        sharpLength = 0;
        flatLength = 0;
    }
};

/**
* This class represent a musical instrument.
*/
class Instrument
{
public:
    float volume;
    float minDist;
    float maxDist;

    /**
     * Constructor. It sets the polyphony capabilities of this instrument.
     * @param polyphony the maximum number of notes that this instrument can
     * play at the same time.
     */
    Instrument(uint polyphony);

    /**
     * Destructor.
     */
    ~Instrument();

    /**
     * Checks if the instrument has a format and a set of defined notes.
     * @return true if it is defined, false otherwise.
     */
    bool IsDefined();

    /**
     * Gets the number of notes that this instrument can play at the same time.
     * @return the number of notes that this instrument can play at the same time.
     */
    uint GetPolyphony() const { return polyphony; }

    /**
     * Gets the csSndSysSoundFormat of the notes of this instrument.
     * return the csSndSysSoundFormat.
     */
    const csSndSysSoundFormat* GetFormat() const { return format; }

    /**
     * Gets the size of the note with the longest buffer.
     * @return the size of the note with the longest buffer.
     */
    size_t GetLongestNoteSize() const { return longestBufferSize; }

    /**
     * Add the given note to the sounds that this instruments can play. If the
     * given note has already been defined, nothing happens.
     *
     * @param fileName the file that contain the note's sound.
     * @param note a char representing the note in the British English notation
     * (i.e. A, B, C, ..., G).
     * @param alter 1 if the note is altered by a sharp, -1 for a flat and 0 if
     * it is not altered.
     * @param octave 4 for the central octave in piano.
     * @return true if the note could be loaded, false otherwise.
     */
    bool AddNote(const char* fileName, char note, int alter, uint octave);

    /**
     * Provide a buffer containing the decoded data for the given note of this
     * instrument. If there are no notes loaded, at the end of the method both
     * the return value and length are 0. DO NOT MODIFY THE OBTAINED BUFFER
     * because the data is not copied to save time and modifying it would damage
     * the instrument's data. If you must modify it, copy it first and work on
     * the copy.
     *
     * @param note a char representing the note in the British English notation
     * (i.e. A, B, C, ..., G).
     * @param alter 1 if the note is altered by a sharp, -1 for a flat and 0 if
     * it is not altered.
     * @param octave 4 for the central octave in piano.
     * @param duration the duration of the note in seconds.
     * @param buffer when the method is done this will contain the decoded data
     * for the given note, or a null pointer if the note does not exist.
     * @param length when the method is done this will contain the length of
     * the provided buffer.
     * @return the number of bytes that are missing to reach the duration. This
     * can be greater than 0 if the sound in the file used to load the note data
     * does not last enough.
     */
    size_t GetNoteBuffer(char note, int alter, uint octave, float duration, char* &buffer, size_t &length);

    /**
     * Add a note to an already existing buffer.
     *
     * @param note a char representing the note in the British English notation
     * (i.e. A, B, C, ..., G).
     * @param alter 1 if the note is altered by a sharp, -1 for a flat and 0 if
     * it is not altered.
     * @param octave 4 for the central octave in piano.
     * @param duration the duration of the note in seconds.
     * @param noteNumber the number of the note already in the chord.
     * @param buffer the buffer where the note is added for the given length.
     * @param length the current length of the buffer.
     */
    void AddNoteToChord(char note, int alter, uint octave, float duration, uint noteNumber, char* buffer, size_t &bufferLength);

private:
    uint polyphony;                            ///< number of notes that this instrument can play at the same time.
    size_t longestBufferSize;                  ///< keeps the size of note with the longest buffer.
    csSndSysSoundFormat* format;               ///< the format shared by all the notes' streams.
    csHash<csHash<Note*, char>*, uint> notes;  ///< the notes' streams.

    /**
     * Get the given note in the hash notes. If it is not defined it is created
     * and added to the data structure.
     *
     * @param pitch a char representing the note in the British English notation
     * (i.e. A, B, C, ..., G).
     * @param octave 4 for the central octave in piano.
     * @return the asked note.
     */
    Note* GetNote(char pitch, uint octave);

    /**
     * Set the buffer of the enharmonic note of the given one.
     * 
     * @param pitch the pitch of the given note.
     * @param alter 1 if the given note is altered by a sharp, -1 a flat and 0
     * if it is not altered.
     * @param octave the octave of the given note.
     * @param buffer the buffer containing the sound data of the note.
     * @param length the length of the buffer.
     */
    void SetEnharmonic(char pitch, int alter, uint octave, char* buffer, size_t length);
};

#endif /* INSTRUMENT_H */

