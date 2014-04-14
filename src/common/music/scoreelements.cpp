/*
 * scoreelements.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

#include "scoreelements.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================

//====================================================================================
// Local Includes
//====================================================================================

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
using namespace psMusic;

// 10 octaves are more than enough and each octave contains maximum
// 7 notes so there is really no need of a bigger hash table
#define PREV_ACCIDENTALS_NAME_SIZE 7
#define PREV_ACCIDENTALS_OCTAVE_SIZE 5

//--------------------------------------------------

Note::NoteContext::NoteContext()
: prevAccidentals(PREV_ACCIDENTALS_OCTAVE_SIZE)
{
}

Accidental Note::NoteContext::GetPreviousAccidental(const Note &note) const
{
    Accidental prevAccidental = NO_ACCIDENTAL;
    const csHash<Accidental, char>* nameHash =
        prevAccidentals.GetElementPointer(note.octave);

    if(nameHash != 0)
    {
        prevAccidental = nameHash->Get(note.name, NO_ACCIDENTAL);
    }
    return prevAccidental;
}

void Note::NoteContext::ResetContext()
{
    // we delete only the internal hash elements so that we don't have to reallocate
    // the memory later, octaves are more or less the same in the whole piece anyway
    csHash<Accidental, char> *nameHash;
    csHash<csHash<Accidental, char>, int>::GlobalIterator
        nameIter(prevAccidentals.GetIterator());

    while(nameIter.HasNext())
    {
        nameHash = &nameIter.Next();
        nameHash->DeleteAll();
    }
}

void Note::NoteContext::UpdateContext(const Note &note)
{
    if(note.writtenAccidental != NO_ACCIDENTAL)
    {
        csHash<Accidental, char>* nameHash =
            prevAccidentals.GetElementPointer(note.octave);

        if(nameHash == 0)
        {
            nameHash = &prevAccidentals.Put(note.octave,
                csHash<Accidental, char>(PREV_ACCIDENTALS_NAME_SIZE));
            nameHash->Put(note.name, note.writtenAccidental);
        }
        else
        {
            nameHash->PutUnique(note.name, note.writtenAccidental);
        }
    }
}

void Note::NoteContext::UpdateContext(const MeasureElement &element)
{
    for(size_t i = 0; i < element.GetNNotes(); i++)
    {
        UpdateContext(element.GetNote(i));
    }
}

Note::Note(char name_, int octave_, Accidental writtenAccidental_)
: octave(octave_), writtenAccidental(writtenAccidental_)
{
    SetName(name_);
}

Accidental Note::GetPlayedAccidental(const ScoreContext &context) const
{
    Accidental prevAccidental;
    int fifths = context.measureAttributes.GetFifths(); // convenience variable

    CS_ASSERT(fifths >= -7 && fifths <= 7);

    // accidentals written for this note have priority
    if(writtenAccidental != NO_ACCIDENTAL)
    {
        return writtenAccidental;
    }

    // then accidentals written on previous note with same name and octave
    prevAccidental = context.noteContext.GetPreviousAccidental(*this);
    if(prevAccidental != NO_ACCIDENTAL)
    {
        return prevAccidental;
    }

    // finally the tonality decide
    switch(name)
    {
    case 'A':
        if(fifths <= -3)
        {
            return FLAT;
        }
        else if(fifths >= 5)
        {
            return SHARP;
        }
        break;
    case 'B':
        if(fifths <= -1)
        {
            return FLAT;
        }
        else if(fifths >= 7)
        {
            return SHARP;
        }
        break;
    case 'C':
        if(fifths <= -6)
        {
            return FLAT;
        }
        else if(fifths >= 2)
        {
            return SHARP;
        }
        break;
    case 'D':
        if(fifths <= -4)
        {
            return FLAT;
        }
        else if(fifths >= 4)
        {
            return SHARP;
        }
        break;
    case 'E':
        if(fifths <= -2)
        {
            return FLAT;
        }
        else if(fifths >= 6)
        {
            return SHARP;
        }
        break;
    case 'F':
        if(fifths <= -7)
        {
            return FLAT;
        }
        else if(fifths >= 1)
        {
            return SHARP;
        }
        break;
    case 'G':
        if(fifths <= -5)
        {
            return FLAT;
        }
        else if(fifths >= 3)
        {
            return SHARP;
        }
        break;
    }

    return NO_ACCIDENTAL;
}

bool Note::operator==(const Note &note) const
{
    return this->name == note.name && this->octave == note.octave;
}

void Note::SetName(char newName)
{
    CS_ASSERT(newName >= 'A' && newName <= 'G');
    name = newName;
}

void Note::SetOctave(int newOctave)
{
    octave = newOctave;
}

void Note::SetWrittenAccidental(Accidental accidental)
{
    writtenAccidental = accidental;
}

//--------------------------------------------------

MeasureElement::MeasureElement(Duration duration_)
: duration(duration_)
{
}

bool MeasureElement::AddNote(char name, int octave, Accidental writtenAccidental)
{
    Note note(name, octave, writtenAccidental);
    size_t noteIdx = notes.Find(note);
    if(noteIdx == csArrayItemNotFound)
    {
        notes.Push(note);
        return true;
    }
    else
    {
        notes[noteIdx].SetWrittenAccidental(note.GetWrittenAccidental());
        return false;
    }
}

bool MeasureElement::RemoveNote(char name, int octave)
{
    Note note(name, octave, NO_ACCIDENTAL);
    return notes.Delete(note);
}

//--------------------------------------------------

Measure::MeasureAttributes::MeasureAttributes()
: tempo(UNDEFINED_MEASURE_ATTRIBUTE), beats(UNDEFINED_MEASURE_ATTRIBUTE),
beatType(UNDEFINED_MEASURE_ATTRIBUTE), fifths(UNDEFINED_MEASURE_ATTRIBUTE)
{
}

bool Measure::MeasureAttributes::IsUndefined() const
{
    return tempo == UNDEFINED_MEASURE_ATTRIBUTE &&
        beats == UNDEFINED_MEASURE_ATTRIBUTE &&
        beatType == UNDEFINED_MEASURE_ATTRIBUTE &&
        fifths == UNDEFINED_MEASURE_ATTRIBUTE;
}

void Measure::MeasureAttributes::UpdateAttributes(
    const Measure::MeasureAttributes &attributes)
{
    if(attributes.beats != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        beats = attributes.beats;
    }
    if(attributes.beatType != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        beatType = attributes.beatType;
    }
    if(attributes.fifths != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        fifths = attributes.fifths;
    }
    if(attributes.tempo != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        tempo = attributes.tempo;
    }
}

Measure::Measure()
  : isEnding(false), isStartRepeat(false), nEndRepeat(0), attributes(0)
{
}

Measure::~Measure()
{
    DeleteAttributes();
}

void Measure::Fit(const MeasureAttributes* attributes_)
{
    int measDuration = 0; // the duration that the measure is supposed to be
    int currDuration = 0; // the sum of the duration of the elements
    size_t cutIdx = 0; // the index contains the index of the first element to be cut

    // Convenience variables
    int beats = UNDEFINED_MEASURE_ATTRIBUTE;
    int beatType = UNDEFINED_MEASURE_ATTRIBUTE;

    // Getting beat information
    if(attributes != 0)
    {
        beats = attributes->GetBeats();
        beatType = attributes->GetBeatType();
    }
    if(attributes_ != 0 && (beats == UNDEFINED_MEASURE_ATTRIBUTE ||
        beatType == UNDEFINED_MEASURE_ATTRIBUTE))
    {
            beats = attributes_->GetBeats();
            beatType = attributes_->GetBeatType();
    }

    CS_ASSERT(beats != UNDEFINED_MEASURE_ATTRIBUTE &&
        beatType != UNDEFINED_MEASURE_ATTRIBUTE);

    // Determining the measure duration
    measDuration = DURATION_QUARTER_DIVISIONS * beats / beatType;

    // Determining which notes exceed the measure
    while(cutIdx < elements.GetSize() && currDuration <= measDuration)
    {
        currDuration += elements[cutIdx].GetDuration();
        cutIdx++;
    }

    // Cutting or filling
    if(currDuration > measDuration) // cut
    {
        int prevDuration = currDuration - elements.Get(cutIdx).GetDuration();

        // If there's a note that exceeds the measure duration we can cut it first
        if(prevDuration < measDuration)
        {
            Duration tempDuration = GetBiggestDuration(measDuration - prevDuration);
            elements.Get(cutIdx).SetDuration(tempDuration);
            cutIdx++;
        }
        for(size_t i = elements.GetSize() - 1; i >= cutIdx; i--)
        {
            DeleteElement(i);
        }
    }
    else if(currDuration < measDuration) // fill
    {
        Duration tempDuration;

        while(currDuration < measDuration)
        {
            tempDuration = GetBiggestDuration(measDuration - currDuration);
            PushElement(MeasureElement(tempDuration));
            currDuration += tempDuration;
        }
    }
}

Measure::MeasureAttributes Measure::GetAttributes() const
{
    if(attributes == 0)
    {
        return MeasureAttributes(); // all attributes are undefined
    }
    return *attributes;
}

void Measure::InsertElement(size_t n, const MeasureElement &element)
{
    if(!elements.Insert(n, element))
    {
        PushElement(element);
    }
}

void Measure::SetBeat(int beats, int beatType)
{
    if(beats != UNDEFINED_MEASURE_ATTRIBUTE && beatType != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        // check if beatType_ is a power of 2
        CS_ASSERT((beatType > 0) && ((beatType & (~beatType + 1)) == beatType));
        CS_ASSERT(beats > 0);
        CreateAttributes();
    }

    // doesn't make sense if one is defined and the other is not
    if(beats == UNDEFINED_MEASURE_ATTRIBUTE || beatType == UNDEFINED_MEASURE_ATTRIBUTE)
    {
        beats = UNDEFINED_MEASURE_ATTRIBUTE;
        beatType = UNDEFINED_MEASURE_ATTRIBUTE;
    }

    if(attributes != 0)
    {
        attributes->SetBeats(beats);
        attributes->SetBeatType(beatType);
        UpdateAttributes();
    }
}

void Measure::SetEnding(bool isEnding_)
{
    isEnding = isEnding_;
}

void Measure::SetFifths(int fifths)
{
    if(fifths != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        CS_ASSERT(fifths >= -7 && fifths <= 7);
        CreateAttributes();
    }
    if(attributes != 0)
    {
        attributes->SetFifths(fifths);
        UpdateAttributes();
    }
}

void Measure::SetNEndRepeat(int nEndRepeat_)
{
    CS_ASSERT(nEndRepeat_ >= 0);
    nEndRepeat = nEndRepeat_;
}

void Measure::SetStartRepeat(bool isStartRepeat_)
{
    isStartRepeat = isStartRepeat_;
}

void Measure::SetTempo(int tempo)
{
    if(tempo != UNDEFINED_MEASURE_ATTRIBUTE)
    {
        CS_ASSERT(tempo > 0);
        CreateAttributes();
    }
    if(attributes != 0)
    {
        attributes->SetTempo(tempo);
        UpdateAttributes();
    }
}

void Measure::CreateAttributes()
{
    if(attributes == 0)
    {
        attributes = new MeasureAttributes();
    }
}

void Measure::DeleteAttributes()
{
    if(attributes != 0)
    {
        delete attributes;
        attributes = 0;
    }
}

void Measure::UpdateAttributes()
{
    if(attributes != 0)
    {
        if(attributes->IsUndefined())
        {
            DeleteAttributes();
        }
    }
}

//--------------------------------------------------

ScoreContext::ScoreContext()
: lastStartRepeatID(0)
{
}

int ScoreContext::GetNPerformedRepeats(int measureID) const
{
    return repeatsDone.Get(measureID, 0);
}

int ScoreContext::RestoreLastStartRepeat()
{
    measureAttributes = lastStartRepeatAttributes;
    return lastStartRepeatID;
}

void ScoreContext::Update(const MeasureElement &element)
{
    noteContext.UpdateContext(element);
}

void ScoreContext::Update(int measureID, const Measure &measure)
{
    noteContext.ResetContext();
    measureAttributes.UpdateAttributes(measure.GetAttributes());

    if(measure.IsStartRepeat())
    {
        lastStartRepeatID = measureID;
        lastStartRepeatAttributes = measureAttributes;

        // we don't need previous repeats anymore at this point
        repeatsDone.DeleteAll();
    }

    if(measure.GetNEndRepeat() > 0 && !repeatsDone.Contains(measureID))
    {
        repeatsDone.Put(measureID, 0);
    }
}
