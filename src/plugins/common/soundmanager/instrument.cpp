/*
 * instrument.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

#include "instrument.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <isndsys/ss_structs.h>
#include <isndsys/ss_stream.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <music/musicutil.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "manager.h"
#include "data.h"


Instrument::Instrument(uint pol)
{
    volume = VOLUME_NORM;
    minDist = 0;
    maxDist = 0;

    polyphony = pol;
    longestBufferSize = 0;
    format = 0;
}

Instrument::~Instrument()
{
    if(format != 0)
    {
        delete format;
    }

    // deleting notes
    csHash<csHash<Note*, char>*, uint>::GlobalIterator octaveIter(notes.GetIterator());
    csHash<Note*, char>* oct;
    Note* note;

    while(octaveIter.HasNext())
    {
        oct = octaveIter.Next();
        csHash<Note*, char>::GlobalIterator noteIter(oct->GetIterator());

        while(noteIter.HasNext())
        {
            note = noteIter.Next();
            delete note;
        }
        oct->DeleteAll();

        delete oct;
    }
    notes.DeleteAll();
}

bool Instrument::IsDefined()
{
    if(format == 0 || notes.IsEmpty())
    {
        return false;
    }

    return true;
}

size_t Instrument::GetNoteBuffer(char pitch, int alter, uint octave, float duration, char* &buffer, size_t &length)
{
    length = 0;
    Note* note = 0;
    csHash<Note*, char>* oct;
    size_t requestedBytes; // the number of bytes that are needed for the given duration

    // check if notes have been loaded
    if(!IsDefined())
    {
        return 0;
    }

    // computing the number of requested bytes
    // #bytes = sample frequency * duration of the note *
    // * number of bytes per sample * number of channels
    requestedBytes = format->Freq * duration * (format->Bits / 8) * format->Channels;

    // if the frame has 16 bits we must return an even number of bytes
    if(requestedBytes % 2 != 0 && format->Bits / 8 == 2)
    {
        requestedBytes--;
    }

    // checking if this note is defined or if it's a rest
    if((oct = notes.Get(octave, 0)) != 0)
    {
        note = oct->Get(pitch, 0);
    }

    if(note == 0 || pitch == 'R')
    {
        return requestedBytes;
    }

    // selecting the note
    switch(alter)
    {
    case 0:
        buffer = note->normal;
        length = note->normalLength;
        break;
    case 1:
        buffer = note->sharp;
        length = note->sharpLength;
        break;
    case -1:
        buffer = note->flat;
        length = note->flatLength;
        break;
    default:
        length = 0;
        return requestedBytes;
    }

    // checking length
    if(length >= requestedBytes)
    {
        length = requestedBytes;
        requestedBytes = 0;
    }
    else
    {
        requestedBytes -= length;
    }

    return requestedBytes;
}

void Instrument::AddNoteToChord(char pitch, int alter, uint octave, float duration, uint noteNumber, char* buffer, size_t &bufferLength)
{
    Note* note = 0;
    csHash<Note*, char>* oct;
    size_t requestedBytes;
    size_t phaseShift;
    char* noteBuffer;
    size_t noteLength;

    // check if notes have been loaded
    if(!IsDefined())
    {
        return;
    }

    // check if this note is defined
    if((oct = notes.Get(octave, 0)) != 0)
    {
        note = oct->Get(pitch, 0);
    }

    if(note == 0)
    {
        return;
    }

    // computing the number of requested bytes and phase shift
    // #bytes = sample frequency * duration of the note *
    // * number of bytes per sample * number of channels
    requestedBytes = format->Freq * duration * (format->Bits / 8) * format->Channels;
	phaseShift = format->Freq * 0.3 * (format->Bits / 8) * format->Channels;

    // if the frame has 16 bits we must return an even number of bytes
    if(format->Bits / 8 == 2)
    {
        if(requestedBytes % 2 != 0)
        {
            requestedBytes--;
        }
        if(phaseShift % 2 != 0)
        {
            phaseShift--;
        }
    }

    // selecting the note
    switch(alter)
    {
    case 0:
        noteBuffer = note->normal;
        noteLength = note->normalLength;
        break;
    case 1:
        noteBuffer = note->sharp;
        noteLength = note->sharpLength;
        break;
    case -1:
        noteBuffer = note->flat;
        noteLength = note->flatLength;
        break;
    default:
        return;
    }

    // updating buffer's length and cleaning up
    if(requestedBytes > bufferLength)
    {
        for(size_t i = bufferLength; i < requestedBytes; i++)
        {
            buffer[i] = 0;
        }
        bufferLength = requestedBytes;
    }

    // adjust the copy size on the given note length
    if(requestedBytes > noteLength - phaseShift)
    {
        requestedBytes = noteLength - phaseShift;
    }

    // Adding data: if noteNumber is odd we add the note
    // in phase opposition to avoid to much big numbers.
    if(noteNumber % 2 == 0)
    {
        for(size_t i = 0; i < requestedBytes; i++)
        {
            buffer[i] += noteBuffer[i + phaseShift];
        }
    }
    else
    {
        for(size_t i = 0; i < requestedBytes; i++)
        {
            buffer[i] -= noteBuffer[i + phaseShift];
        }
    }
}

bool Instrument::AddNote(const char* fileName, char pitch, int alter, uint octave)
{
    Note* note;
    size_t streamSize;
    char* buffer;
    csRef<iSndSysData> noteData;
    csRef<iSndSysStream> noteStream;

    // variables used to get the stream data
    void* data1;
    void* data2;
    size_t length1;
    size_t length2;
    size_t positionMarker;

    // checking if parameters are correct
    if(!(alter == 0 || alter == 1 || alter == -1))
    {
        return false;
    }

    // loading data
    if(!SoundSystemManager::GetSingleton().GetSoundDataCache()->GetSoundData(fileName, noteData))
    {
        return false;
    }

    // If format is null this is the first added note: create format
    if(format == 0)
    {
        const csSndSysSoundFormat* f = noteData->GetFormat();
        format = new csSndSysSoundFormat();
        format->Bits = f->Bits;
        format->Channels = f->Channels;
        format->Flags = f->Flags;
        format->Freq = f->Freq;
    }

    // creating the decoded stream
    noteStream.AttachNew(noteData->CreateStream(format, CS_SND3D_ABSOLUTE));
    noteStream->SetLoopState(CS_SNDSYS_STREAM_DONTLOOP);
    streamSize = noteStream->GetFrameCount() * (format->Bits / 8) * format->Channels;

    note = GetNote(pitch, octave);

    // initializing notes' buffer
    switch(alter)
    {
    case 0:
        if(note->normal != 0)
        {
            return false; // the note and its enharmonic already exist
        }
        note->normalLength = streamSize;
        note->normal = new char[streamSize];
        buffer = note->normal;
        break;

    case -1:
        if(note->flat != 0)
        {
            return false;
        }
        note->flatLength = streamSize;
        note->flat = new char[streamSize];
        buffer = note->flat;
        break;

    case 1:
        if(note->sharp != 0)
        {
            return false;
        }
        note->sharpLength = streamSize;
        note->sharp = new char[streamSize];
        buffer = note->sharp;
        break;
    // we checked before that alter is right, no need for default
    }

    SetEnharmonic(pitch, alter, octave, buffer, streamSize);

    // updating longestBufferSize
    if(longestBufferSize < streamSize)
    {
        longestBufferSize = streamSize;
    }

    // reading and copying data
    noteStream->InitializeSourcePositionMarker(&positionMarker);
    noteStream->Unpause(); // unpause otherwise data is never read
    do
    {
        noteStream->AdvancePosition(streamSize);
        noteStream->GetDataPointers(&positionMarker, streamSize, &data1, &length1, &data2, &length2);

        // copying buffer 1
        for(size_t i = 0; i < length1; i++)
        {
            (*buffer) = ((char*)data1)[i];
            buffer++;
        }

        // copying buffer 2
        for(size_t i = 0; i < length2; i++)
        {
            (*buffer) = ((char*)data2)[i];
            buffer++;
        }
    }while(noteStream->GetPauseState() == CS_SNDSYS_STREAM_UNPAUSED);

    return true;
}

Note* Instrument::GetNote(char pitch, uint octave)
{
    Note* note = 0;
    csHash<Note*, char>* oct;
    
    // creating octave if it doesn't exist
    oct = notes.Get(octave, 0);
    if(oct == 0)
    {
        oct = new csHash<Note*, char>();
        notes.Put(octave, oct);
    }
    else
    {
        note = oct->Get(pitch, 0);
    }

    // creating note if it doesn't exist
    if(note == 0)
    {
        note = new Note();
        oct->Put(pitch, note);
    }

    return note;
}

void Instrument::SetEnharmonic(char pitch, int alter, uint octave, char* buffer, size_t length)
{
    Note* enharmonicNote;

    switch(alter)
    {
    case 0:
        switch(pitch)
        {
        case 'E':
        case 'B':
            psMusic::NextPitch(pitch, octave);
            enharmonicNote = GetNote(pitch, octave);
            enharmonicNote->flatLength = length;
            enharmonicNote->flat = buffer;
            break;
        case 'C':
        case 'F':
            psMusic::PreviousPitch(pitch, octave);
            enharmonicNote = GetNote(pitch, octave);
            enharmonicNote->sharpLength = length;
            enharmonicNote->sharp = buffer;
            break;
        }

        break; // first switch

    case -1:
        switch(pitch)
        {
        case 'C':
        case 'F':
            psMusic::PreviousPitch(pitch, octave);
            enharmonicNote = GetNote(pitch, octave);
            enharmonicNote->normalLength = length;
            enharmonicNote->normal = buffer;
            break;
        default:
            psMusic::PreviousPitch(pitch, octave);
            enharmonicNote = GetNote(pitch, octave);
            enharmonicNote->sharpLength = length;
            enharmonicNote->sharp = buffer;
            break;
        }

        break; // first switch

    case 1:
        switch(pitch)
        {
        case 'E':
        case 'B':
            psMusic::NextPitch(pitch, octave);
            enharmonicNote = GetNote(pitch, octave);
            enharmonicNote->normalLength = length;
            enharmonicNote->normal = buffer;
            break;
        default:
            psMusic::NextPitch(pitch, octave);
            enharmonicNote = GetNote(pitch, octave);
            enharmonicNote->flatLength = length;
            enharmonicNote->flat = buffer;
            break;
        }

        break; // first switch
    }
}
