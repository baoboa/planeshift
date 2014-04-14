/*
 * songstream.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

#include "songstream.h"

//====================================================================================
// Project Includes
//====================================================================================
#include <music/musicutil.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "soundmanager.h"
#include "songdata.h"
#include "instrument.h"


// Keep these as integer values or the stream may feed portions of a sample to the higher layer which may cause problems
// 1 , 1 will decode 1/1 or 1 second of audio in advance
#define SONG_BUFFER_LENGTH_MULTIPLIER  1
#define SONG_BUFFER_LENGTH_DIVISOR     2

SndSysSongStream::SndSysSongStream(csRef<SndSysSongData> sndData, SongData* data, csSndSysSoundFormat* renderFormat, int mode3D):
    SndSysBasicStream(renderFormat, mode3D), soundData(sndData)
{
    isFinished = false;
    currentMeasure = 0;
    currentNote = 0;

    lastRepeatStart = 0;
    lastRepeatEnd = 0;
    repeatCounter = -1;

    if(data->instrument->GetPolyphony() > 1)
    {
        copyNoteBuffer = new char[data->instrument->GetLongestNoteSize()];
    }
    else
    {
        copyNoteBuffer = 0;
    }
    lastNoteSize = 0;

    songData = data;
    timePerDivision = 60.0f / songData->tempo / songData->divisions;

    // conversion variables are set during the first AdvancePosition() because m_OutputFrequency = 0
    conversionFactor = 0;
    conversionNeeded = false;

    // Allocate an advance buffer
    m_pCyclicBuffer = new SoundCyclicBuffer((m_RenderFormat.Bits/8 * m_RenderFormat.Channels) * 
        (m_RenderFormat.Freq * SONG_BUFFER_LENGTH_MULTIPLIER / SONG_BUFFER_LENGTH_DIVISOR));
    CS_ASSERT(m_pCyclicBuffer != 0);
}

SndSysSongStream::~SndSysSongStream()
{
    // songData is deleted by SndSysSongData
    // noteBuffer SHOULD BE NEVER ALLOCATED
    if(copyNoteBuffer != 0)
    {
        delete[] copyNoteBuffer;
    }
}

const char* SndSysSongStream::GetDescription()
{
    const char* desc = soundData->GetDescription();
    if(desc == 0)
    {
        return "Song Stream";
    }
    return desc;
}

size_t SndSysSongStream::GetFrameCount()
{
    return CS_SNDSYS_STREAM_UNKNOWN_LENGTH;
}

void SndSysSongStream::AdvancePosition(size_t frameDelta)
{
    char* noteBuffer;
    size_t noteBufferSize = 0;
    size_t neededBytes;
    int neededBuffer;
    const csSndSysSoundFormat* dataFormat = soundData->GetFormat();

    // if the stream is in pause or we have already finished to read don't advance
    if(m_PauseState != CS_SNDSYS_STREAM_UNPAUSED || m_bPlaybackReadComplete || frameDelta == 0)
    {
        return;
    }

    // Computing how many bytes we need to fill for this advancement
    neededBytes = frameDelta * (m_RenderFormat.Bits / 8) * m_RenderFormat.Channels;

    // If we need more space than is available in the whole cyclic buffer, then we already underbuffered, reduce to just 1 cycle full
    if((size_t)neededBytes > m_pCyclicBuffer->GetLength())
    {
        neededBytes=(size_t)(m_pCyclicBuffer->GetLength() & 0x7FFFFFFF);
    }

    // Free space in the cyclic buffer if necessary
    if ((size_t)neededBytes > m_pCyclicBuffer->GetFreeBytes())
    {
        m_pCyclicBuffer->AdvanceStartValue(neededBytes - (size_t)(m_pCyclicBuffer->GetFreeBytes() & 0x7FFFFFFF));
    }

    // Fill in leftover decoded data if needed
    if (m_PreparedDataBufferUsage > 0)
    {
        neededBytes -= CopyBufferBytes(neededBytes);
    }
    if(CheckPlaybackReadComplete())
    {
        neededBytes = 0; // no more data
    }

    // set back m_NewPosition to invalid if the last note has been
    // completely copied. Needed for PendingSeek() to work.
    if(m_PreparedDataBufferUsage == 0 && m_NewPosition != InvalidPosition)
    {
        m_NewPosition = InvalidPosition;
    }

    while(neededBytes > 0)
    {
        // Handle a frequency change
        if (m_NewOutputFrequency != m_OutputFrequency)
        {
            m_OutputFrequency = m_NewOutputFrequency;

            // Create the pcm sample converter if it's not yet created
            if (m_pPCMConverter == 0)
            {
                m_pPCMConverter = new PCMSampleConverter(dataFormat->Channels, dataFormat->Bits, dataFormat->Freq);
            }

            // check if the conversion is needed
            if((dataFormat->Bits == m_RenderFormat.Bits) &&
                (dataFormat->Channels == m_RenderFormat.Channels) &&
                (dataFormat->Freq == m_RenderFormat.Freq))
            {
                conversionFactor = 1;
                conversionNeeded = false;
            }
            else
            {
                conversionFactor = m_pPCMConverter->GetRequiredOutputBufferMultiple(m_RenderFormat.Channels,m_RenderFormat.Bits,m_OutputFrequency)/1024;
                conversionNeeded = true;
            }
        }

        // converting the musical sheet
        isFinished = GetNextChord(noteBuffer, noteBufferSize);

        // expanding m_pPreparedDataBuffer if needed (it's empty at this point so it can be deleted)
        neededBuffer = conversionFactor * (lastNoteSize + (dataFormat->Bits / 8) * dataFormat->Channels);
        if(m_PreparedDataBufferSize < neededBuffer)
        {
            delete[] m_pPreparedDataBuffer;
            m_pPreparedDataBuffer = new char[neededBuffer];
            m_PreparedDataBufferSize=neededBuffer;
        }

        // copying to m_pPreparedDataBuffer
        CopyNoteIntoBuffer(noteBuffer, noteBufferSize);
        
        // copying to the cyclic buffer
        if(m_PreparedDataBufferUsage > 0)
        {
            neededBytes -= CopyBufferBytes(neededBytes);
        }
        if(CheckPlaybackReadComplete())
        {
            neededBytes = 0; // no more data
        }
    }
}

bool SndSysSongStream::ResetPosition()
{
    currentMeasure = 0;
    currentNote = 0;

    // this will be set to invalid in AdvancePosition()
    // it's need for PendingSeek() to work
    m_NewPosition = 0;

    return true;
}

bool SndSysSongStream::CheckPlaybackReadComplete()
{
    // if the end of the sheet has been reached and the prepared buffer
    // is empty no more data can be copied to the cyclic buffer
    if(isFinished && m_PreparedDataBufferUsage == 0)
    {
        if(!m_bLooping)
        {
            m_bPlaybackReadComplete = true;
        }
        else
        {
            currentMeasure = 0;
            currentNote = 0;
        }
    }

    return m_bPlaybackReadComplete;
}

// TODO support for <tie> and chords with different duration of their notes
bool SndSysSongStream::GetNextChord(char* &noteBuffer, size_t &noteBufferSize)
{
    uint nChordNotes; // current number of notes in the chord
    bool chordFound = false;
    bool notesLeft = false;

    char step;
    int alter;
    uint octave;
    uint divisions;
    float duration;
    csRef<iDocumentNode> measure;
    csRef<iDocumentNode> barline;
    csRef<iDocumentNode> note;
    csRef<iDocumentNode> pitch;
    csRef<iDocumentNode> alterNode;
    csRef<iDocumentNodeIterator> notes;

    measure = songData->measures.Get(currentMeasure);
    barline = measure->GetNode("barline");
    notes = measure->GetNodes("note");

    // checking if this measure is the beginning of a repeat or an ending to skip
    if(barline.IsValid())
    {
        csRef<iDocumentNode> repeat = barline->GetNode("repeat");
        const char* direction = repeat->GetAttributeValue("direction");
        if(csStrCaseCmp(direction, "forward") == 0)
        {
            lastRepeatStart = currentMeasure;
        }

        // checking if this is an ending measure to skip
        if(csStrCaseCmp(direction, "backward") == 0)
        {
            csRef<iDocumentNode> ending = barline->GetNode("ending");
            if(ending.IsValid())
            {
                bool skip = false;

                if(repeatsDone.Find(currentMeasure) != csArrayItemNotFound)
                {
                    skip = true;
                }
                else if(repeatCounter == 0)
                {
                    repeatsDone.Push(currentMeasure);
                    repeatCounter--;
                    skip = true;
                }

                // skipping the measure
                if(skip)
                {
                    if(currentMeasure + 1 < songData->measures.GetSize())
                    {
                        currentMeasure++;
                        return GetNextChord(noteBuffer, noteBufferSize);
                    }
                    else // end reached
                    {
                        currentNote = 0;
                        currentMeasure = 0;
                        return true;
                    }
                }
            }
        }
    }

    // getting the right note
    while(notes->HasNext())
    {
        if(notes->GetNextPosition() == currentNote)
        {
            note = notes->Next();

            // skipping chords that this instrument can't play
            if(note->GetNode("chord").IsValid())
            {
                if(notes->HasNext())
                {
                    currentNote++;
                    continue;
                }
                else if(currentMeasure + 1 == songData->measures.GetSize())
                {
                    noteBufferSize = 0;
                    return true;
                }
                else
                {
                    currentMeasure++;
                    currentNote = 0;
                    return GetNextChord(noteBuffer, noteBufferSize);
                }
            }

            break;
        }
        notes->Next();
    }

    // getting note's data
    pitch = note->GetNode("pitch");
    if(pitch.IsValid())
    {
        step = *(pitch->GetNode("step")->GetContentsValue());
        alterNode = pitch->GetNode("alter");
        if(alterNode == 0)
        {
            alter = 0;
        }
        else
        {
            alter = alterNode->GetContentsValueAsInt();
        }
        octave = pitch->GetNode("octave")->GetContentsValueAsInt();
    }
    else
    {
        step = 'R';
    }
    divisions = note->GetNode("duration")->GetContentsValueAsInt();

    // computing note duration in seconds
    duration = timePerDivision * divisions;

    // adjusting alteration depending on the tonality
    if(step != 'R')
    {
        AdjustAlteration(step, alter);
    }

    // getting first note buffer and length
    lastNoteSize = songData->instrument->GetNoteBuffer(step, alter, octave, duration, noteBuffer, noteBufferSize);
    lastNoteSize += noteBufferSize;

    // handling chords
    nChordNotes = 1;
    while(notes->HasNext() && nChordNotes < songData->instrument->GetPolyphony())
    {
        note = notes->Next();
        if(note->GetNode("chord") == 0)
        {
            notesLeft = true;
            break;
        }

        // copying buffer if needed
        // remember that noteBuffer can't be modified, otherwise the instrument's data will be damaged
        if(!chordFound)
        {
            memcpy(copyNoteBuffer, noteBuffer, noteBufferSize);
            chordFound = true;
        }

        // getting data of the new note
        pitch = note->GetNode("pitch");
        step = *(pitch->GetNode("step")->GetContentsValue());
        alterNode = pitch->GetNode("alter");
        if(alterNode == 0)
        {
            alter = 0;
        }
        else
        {
            alter = alterNode->GetContentsValueAsInt();
        }
        octave = pitch->GetNode("octave")->GetContentsValueAsInt();

        // adjusting alteration depending on the tonality
        AdjustAlteration(step, alter);

        // add waves
        songData->instrument->AddNoteToChord(step, alter, octave, duration, nChordNotes, copyNoteBuffer, noteBufferSize);

        // updating current state
        nChordNotes++;
        currentNote++;
    }

    // fixing noteBuffer if a chord has been found
    if(chordFound)
    {
        noteBuffer = copyNoteBuffer;
    }

    // is this the end of the measure?
    // HasNext() is necessary if we didn't find chords
    if(notesLeft || notes->HasNext())
    {
        currentNote++;
        return false;
    }

    // here it's the end of the musical sheet
    // checking if this measure is the end of a repeat
    if(barline.IsValid())
    {
        csRef<iDocumentNode> repeat = barline->GetNode("repeat");

        if(repeat.IsValid()
            && csStrCaseCmp(repeat->GetAttributeValue("direction"), "backward") == 0
            && !(repeatsDone.Find(currentMeasure) != csArrayItemNotFound))
        {
            if(repeatCounter < 0) // no repeats active
            {
                repeatCounter = repeat->GetAttributeValueAsInt("times");
            }

            repeatCounter--; // if it's 0 it becomes -1 and don't repeat

            if(repeatCounter >= 0) // bring the position back
            {
                currentMeasure = lastRepeatStart;
                currentNote = 0;
                return false;
            }
            else
            {
                repeatsDone.Push(currentMeasure);
            }
        }
    }

    // end of measure, no repeats
    if(currentMeasure + 1 < songData->measures.GetSize())
    {
        currentNote = 0;
        currentMeasure++;
        return false;
    }
    else
    {
        currentNote = 0;
        currentMeasure = 0;
        return true; // end reached
    }
}

void SndSysSongStream::AdjustAlteration(char pitch, int &alter)
{
    if(alter != 0)
    {
        return;
    }

    if(songData->fifths > 0)
    {
        // NOTE: the order of the cases is important!! Do not change it!
        switch(songData->fifths)
        {
        case 7:
            if(pitch == 'B')
            {
                alter++;
                break;
            }
        case 6:
            if(pitch == 'E')
            {
                alter++;
                break;
            }
        case 5:
            if(pitch == 'A')
            {
                alter++;
                break;
            }
        case 4:
            if(pitch == 'D')
            {
                alter++;
                break;
            }
        case 3:
            if(pitch == 'G')
            {
                alter++;
                break;
            }
        case 2:
            if(pitch == 'C')
            {
                alter++;
                break;
            }
        case 1:
            if(pitch == 'F')
            {
                alter++;
                break;
            }
        }
    }
    else if(songData->fifths < 0)
    {
        // NOTE: the order of the cases is important!! Do not change it!
        switch(songData->fifths)
        {
        case -7:
            if(pitch == 'F')
            {
                alter--;
                break;
            }
        case -6:
            if(pitch == 'C')
            {
                alter--;
                break;
            }
        case -5:
            if(pitch == 'G')
            {
                alter--;
                break;
            }
        case -4:
            if(pitch == 'D')
            {
                alter--;
                break;
            }
        case -3:
            if(pitch == 'A')
            {
                alter--;
                break;
            }
        case -2:
            if(pitch == 'E')
            {
                alter--;
                break;
            }
        case -1:
            if(pitch == 'B')
            {
                alter--;
                break;
            }
        }
    }
}

// TODO support loop notes (like organ, the sound can be persisent and does not end into silence)
// TODO support for dynamics
void SndSysSongStream::CopyNoteIntoBuffer(char* noteBuffer, size_t noteBufferSize)
{
    // copying noteBuffer into preparedDataBuffer
    if(!conversionNeeded)
    {
        memcpy(m_pPreparedDataBuffer, noteBuffer, noteBufferSize);
        m_PreparedDataBufferUsage = noteBufferSize;
    }
    else
    {
        m_PreparedDataBufferUsage = m_pPCMConverter->ConvertBuffer(noteBuffer, noteBufferSize,
            m_pPreparedDataBuffer, m_RenderFormat.Channels, m_RenderFormat.Bits, m_OutputFrequency);
    }

    // adding silence if needed
    if(lastNoteSize > noteBufferSize)
    {
        for(size_t i = noteBufferSize; i < lastNoteSize; i++)
        {
            m_pPreparedDataBuffer[i] = 0;
            m_PreparedDataBufferUsage++;
        }
    }
}
