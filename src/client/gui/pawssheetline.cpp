/*
 * pawssheetline.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

#include <psconfig.h>
#include "pawssheetline.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <ivideo/fontserv.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "pawsmusicwindow.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------

// Used by Chord
#define REST_POSITION 7                     // the position of a rest on the staff
#define FIRST_NOTE_DOWN 7                   // the position at which the note start being drawed down.
#define SELECTED_CURSOR_COLOR 255, 0, 0     // the color of the selected cursor

// Used by Measure
#define MIMIMUM_NOTE_SPACE 4                // the minimum number of note's length between chords.

// Used by pawsSheetLine
#define SINGLE_STAFF_FONT_SIZE 0.75f        // the multiplier of the font's size when switching from the double staff to the single one.

// Font's characters
#define SYMBOL_ENCODING_BASE 0xf000
#define G_CLEF_SONORA '&' | SYMBOL_ENCODING_BASE
#define F_CLEF_SONORA '?' | SYMBOL_ENCODING_BASE
#define SHARP_SONORA '#' | SYMBOL_ENCODING_BASE
#define FLAT_SONORA 'b' | SYMBOL_ENCODING_BASE
#define NATURAL_SONORA 'n' | SYMBOL_ENCODING_BASE
#define TEMPO_SONORA (wchar_t)242 | SYMBOL_ENCODING_BASE
#define ZERO_SMALL_SONORA (wchar_t)188 | SYMBOL_ENCODING_BASE
#define ONE_SMALL_SONORA (wchar_t)193 | SYMBOL_ENCODING_BASE
#define TWO_SMALL_SONORA (wchar_t)170 | SYMBOL_ENCODING_BASE
#define THREE_SMALL_SONORA (wchar_t)163 | SYMBOL_ENCODING_BASE
#define FOUR_SMALL_SONORA (wchar_t)162 | SYMBOL_ENCODING_BASE
#define FIVE_SMALL_SONORA (wchar_t)176 | SYMBOL_ENCODING_BASE
#define SIX_SMALL_SONORA (wchar_t)164 | SYMBOL_ENCODING_BASE
#define SEVEN_SMALL_SONORA (wchar_t)166 | SYMBOL_ENCODING_BASE
#define EIGHT_SMALL_SONORA (wchar_t)165 | SYMBOL_ENCODING_BASE
#define NINE_SMALL_SONORA (wchar_t)187 | SYMBOL_ENCODING_BASE
#define SIXTEENTH_DOWN_SONORA 'R' | SYMBOL_ENCODING_BASE
#define SIXTEENTH_UP_SONORA 'r' | SYMBOL_ENCODING_BASE
#define EIGHTH_DOWN_SONORA 'E' | SYMBOL_ENCODING_BASE
#define EIGHTH_UP_SONORA 'e' | SYMBOL_ENCODING_BASE
#define QUARTER_DOWN_SONORA 'Q' | SYMBOL_ENCODING_BASE
#define QUARTER_UP_SONORA 'q' | SYMBOL_ENCODING_BASE
#define HALF_DOWN_SONORA 'H' | SYMBOL_ENCODING_BASE
#define HALF_UP_SONORA 'h' | SYMBOL_ENCODING_BASE
#define WHOLE_SONORA 'L' | SYMBOL_ENCODING_BASE
#define FULL_CIRCLE_SONORA (wchar_t)207 | SYMBOL_ENCODING_BASE
#define EMPTY_CIRCLE_SONORA (wchar_t)146 | SYMBOL_ENCODING_BASE
#define SIXTEENTH_REST_SONORA (wchar_t)197 | SYMBOL_ENCODING_BASE
#define EIGHTH_REST_SONORA (wchar_t)228 | SYMBOL_ENCODING_BASE
#define QUARTER_REST_SONORA (wchar_t)206 | SYMBOL_ENCODING_BASE
#define HALF_REST_SONORA (wchar_t)238 | SYMBOL_ENCODING_BASE
#define WHOLE_REST_SONORA (wchar_t)183 | SYMBOL_ENCODING_BASE
#define DOT_SONORA 'k' | SYMBOL_ENCODING_BASE
#define LEDGER_LINE_SONORA '_' | SYMBOL_ENCODING_BASE

//--------------------------------------------------

Note::Note(short int position, short int alter)
{
    this->position = position;
    this->alter = alter;
}

csString Note::ToXML()
{
    csString pitch;
    int alterXML = 0;
    int octave;
    int iStep;
    char step;

    // computing alter
    switch(alter)
    {
    default:
    case pawsMusicWindow::UNALTERED:
    case pawsMusicWindow::NATURAL:
        alterXML = 0;
        break;
    case pawsMusicWindow::FLAT:
        alterXML = -1;
        break;
    case pawsMusicWindow::SHARP:
        alterXML = 1;
        break;
    }
    // computing octave
    octave = 4 + position / 7;

    if(position < 0 && position % 7 != 0)
    {
        octave--;
    }

    // computing step
    iStep = position % 7;

    switch(iStep)
    {
    case 0:
        step = 'C';
        break;
    case 1:
    case -6:
        step = 'D';
        break;
    case 2:
    case -5:
        step = 'E';
        break;
    case 3:
    case -4:
        step = 'F';
        break;
    case 4:
    case -3:
        step = 'G';
        break;
    case 5:
    case -2:
        step = 'A';
        break;
    case 6:
    case -1:
        step = 'B';
        break;
    }

    // converting to XML
    pitch = "<pitch>";
    pitch.AppendFmt("<step>%c</step><octave>%d</octave><alter>%d</alter>", step, octave, alterXML);
    pitch += "</pitch>";

    return pitch;
}

//--------------------------------------------------

Chord::Chord()
{
    prev = 0;
    next = 0;

    isRest = false;
    duration = QUARTER_DURATION;
}

Chord::Chord(Chord* chord): notes(chord->notes)
{
    prev = 0;
    next = 0;

    isRest = chord->isRest;
    duration = chord->duration;
}

Chord::~Chord()
{
    Empty();

    // updating prev and next chords
    if(prev != 0)
    {
        prev->next = next;
    }

    if(next != 0)
    {
        next->prev = prev;
    }
}

void Chord::SetDuration(int dur)
{
    duration = dur;
}

void Chord::AddNote(int position, int alter, bool rest)
{
    Note note(0, 0);
    size_t notesLength = notes.GetSize();

    if(rest)
    {
        note.position = REST_POSITION;
        note.alter = pawsMusicWindow::UNALTERED;

        if(notesLength > 0)
        {
            Empty();
        }
        notes.Push(note);
    }
    else
    {
        note.position = position;
        note.alter = alter;

        // if this is a rest chord delete the rest
        if(isRest)
        {
            Empty();
            notesLength = 0;
        }

        // keep the highest note first to simplify drawing
        for(size_t i = 0; i < notesLength; i++)
        {
            // if this note is already in the chord: overwrite
            if(position == notes[i].position)
            {
                notes[i].alter = note.alter;
                break;
            }

            if(position > notes[i].position)
            {
                notes.Insert(i, note);
                break;
            }
        }

        // if the note has not been inserted it's the last one, push it
        if(notesLength == notes.GetSize())
        {
            notes.Push(note);
        }
    }

    // updating state
    isRest = rest;
}

void Chord::AddNote(csRef<iDocumentNode> pitchNode)
{
    int alter = pawsMusicWindow::UNALTERED;
    int position = 0;
    csRef<iDocumentNode> stepNode;
    csRef<iDocumentNode> octaveNode;
    csRef<iDocumentNode> alterNode;

    // checking that pitchNode is valid
    if(pitchNode == 0)
    {
        return;
    }

    stepNode = pitchNode->GetNode("step");
    octaveNode = pitchNode->GetNode("octave");
    alterNode = pitchNode->GetNode("alter");

    // checking if the note is valid
    if(stepNode == 0 || octaveNode == 0 || alterNode == 0)
    {
        return;
    }

    char step = stepNode->GetContentsValue()[0];
    int octave = octaveNode->GetContentsValueAsInt();
    int alterXML = alterNode->GetContentsValueAsInt();

    // converting step
    switch(step)
    {
    case 'C':
        position = 0;
        break;
    case 'D':
        position = 1;
        break;
    case 'E':
        position = 2;
        break;
    case 'F':
        position = 3;
        break;
    case 'G':
        position = 4;
        break;
    case 'A':
        position = 5;
        break;
    case 'B':
        position = 6;
        break;
    }

    // converting octave
    position += (octave - 4) * 7;

    // converting alter
    switch(alterXML)
    {
    default:
    case 0:
        alter = pawsMusicWindow::UNALTERED;
        break;
    case 1:
        alter = pawsMusicWindow::SHARP;
        break;
    case -1:
        alter = pawsMusicWindow::FLAT;
        break;
    }

    // adding note
    AddNote(position, alter, false);
}

void Chord::Empty()
{
    notes.DeleteAll();
    isRest = false;
}

void Chord::AttachChord(Chord* chord)
{
    chord->prev = this;
    chord->next = next;

    if(next != 0)
    {
        next->prev = chord;
    }
    next = chord;
}

void Chord::Draw(pawsSheetLine* pawsLine, Chord* selectedChord, int horizontalPos)
{
    bool isDotted;              // true if the duration is with the dot
    bool isNoteDown;            // true if the note must be drawn downwards

    int vCPos;                  // vertical position of the central C

    int prevNotePos;
    size_t lastDownNote;        // position of the last note that must be drawn downwards

    size_t notesLength = notes.GetSize();

    // note char height in positions
    int noteHeight = pawsLine->noteCharHeight * 2 / pawsLine->staffRowHeight;

    // drawing selected and end cursor
    if(this == selectedChord)
    {
        int cursorX = horizontalPos + pawsLine->noteCharLength / 2;
        int color = pawsLine->graphics2D->FindRGB(SELECTED_CURSOR_COLOR);
        pawsLine->graphics2D->DrawLine(cursorX, pawsLine->screenFrame.ymin, cursorX, pawsLine->screenFrame.ymax, color);
    }
    else if(notesLength == 0) // this is the last chord of the song
    {
        int cursorX = horizontalPos + pawsLine->noteCharLength / 2;
        pawsLine->graphics2D->DrawLine(cursorX, pawsLine->screenFrame.ymin, cursorX, pawsLine->screenFrame.ymax, pawsLine->GetFontColour());
        return;
    }

    // setting general printing parameters
    isDotted = (duration != 1 && duration % 3 == 0);
    if(isRest)
    {
        isNoteDown = true;
    }
    else if(notesLength > 0)
    {
        isNoteDown = (notes[0].position >= FIRST_NOTE_DOWN);
    }
    else
    {
        isNoteDown = false;
    }
    vCPos = pawsLine->GetScreenFrame().ymin + pawsLine->centralCPos;
    
    prevNotePos = 1000;     // very high value so that the first note is not a circle

    // deciding which notes will be drawn downwards
    if(isNoteDown)
    {
        for(size_t i = 0; i < notesLength; i++)
        {
            bool isLast = false;

            // checking if this is the last note
            if(i == notesLength - 1)
            {
                isLast = true;
            }
            else if(notes[i].position - notes[i+1].position > 2 * noteHeight
                && notes[i+1].position < FIRST_NOTE_DOWN)
            {
                isLast = true;
            }
            
            // checking if this is a circle note (last is the previous)
            if(i > 0 && isLast
                && notes[i-1].position == notes[i].position + 1)
            {
                lastDownNote = i - 1;
                break;

            }
            else if(isLast)
            {
                lastDownNote = i;
                break;
            }
        }
    }
    else
    {
        lastDownNote = -1;
    }

    // drawing chord
    for(size_t i = 0; i < notesLength; i++)
    {
        int notePos = notes[i].position;

        int nextHPosUpString = horizontalPos;
        int nextHPosLowString = horizontalPos;
        int yNoteUp = vCPos - notePos * pawsLine->staffRowHeight / 2;
        int yNoteLow = yNoteUp - 0.5 * pawsLine->staffRowHeight;

        bool isLast = (i == lastDownNote);
        bool isCircleNote = (prevNotePos == notePos + 1);

        // the up sixteenth and the whole in Sonora are lower
        bool isLowerNote = isCircleNote || (duration >= WHOLE_DURATION && !isRest)
            || (duration == SIXTEENTH_DURATION && !isNoteDown && i - 1 == lastDownNote);

        // is the next note distant?
        bool isDistantNote;
        if(i == notesLength - 1)
        {
            isDistantNote = false;
        }
        else
        {
            isDistantNote = (notePos - notes[i+1].position > noteHeight);
        }

        // has this note ledger lines?
        bool hasLedgerLine;         // true if the drawed note is cut by a line
        int nLedgerLines = 0;

        hasLedgerLine = (notePos % 2) == 0;

        if(notePos == 0)
        {
            nLedgerLines = 1;
        }
        else if(notePos >= 12)
        {
            nLedgerLines = (notePos - 12) / 2 + 1;
        }
        else
        {

            if(pawsLine->isDouble)
            {
                if(notePos <= -12)
                {
                    nLedgerLines = (-notePos - 12) / 2 + 1;
                }
            }
            else
            {
                if(notePos <= 0)
                {
                    nLedgerLines = -notePos / 2 + 1;
                }
            }
        }

        // Sonora has some character printed on a lower line than other
        // so I print two different strings
        wchar_t upString[3];
        wchar_t lowString[3];

        for(int j = 0; j < 3; j++)
        {
            upString[j] = '\0';
            lowString[j] = '\0';
        }

        // handling alteration
        if(notes[i].alter != pawsMusicWindow::UNALTERED)
        {
            switch(notes[i].alter)
            {
            case pawsMusicWindow::FLAT:
                lowString[0] = FLAT_SONORA;
                break;
            case pawsMusicWindow::SHARP:
                lowString[0] = SHARP_SONORA;
                break;
            case pawsMusicWindow::NATURAL:
                lowString[0] = NATURAL_SONORA;
                break;
            }

            nextHPosLowString -= pawsLine->alterCharLength;
        }

        // deciding which note to print
        if(isCircleNote)
        {
            // checking if there is an alteration
            if(nextHPosLowString == horizontalPos)
            {
                lowString[0] = GetCircleNoteChar();
            }
            else
            {
                lowString[1] = GetCircleNoteChar();
            }

            nextHPosLowString += pawsLine->noteCharLength * 3 / 4.0;
        }
        else if(isLowerNote)
        {
            // checking if there is an alteration
            if(nextHPosLowString == horizontalPos)
            {
                lowString[0] = GetNoteChar(isNoteDown);
            }
            else
            {
                lowString[1] = GetNoteChar(isNoteDown);
            }
        }
        else
        {
            if(isLast || i - 1 == lastDownNote || isDistantNote)
            {
                upString[0] = GetNoteChar(isNoteDown);
            }
            else
            {
                upString[0] = GetChordNoteChar(isNoteDown);
            }
        }

        // handling dotted notes
        if(isDotted)
        {
            // checking where to put the dot
            if(upString[0] == '\0') // the note is on the low string
            {
                upString[0] = DOT_SONORA;
                nextHPosUpString += 1.3 * pawsLine->noteCharLength;

                // dot for circle notes
                if(isCircleNote)
                {
                    nextHPosUpString += pawsLine->noteCharLength * 0.75;
                }
            }
            else
            {
                upString[1] = DOT_SONORA;
            }
        }

        // handling ledger lines
        if(nLedgerLines > 0)
        {
            wchar_t tempString[2];
            int hPos = horizontalPos - pawsLine->ledgerLineShift;
            int yPos = yNoteLow;
            
            // fixing initial position
            if(!hasLedgerLine)
            {
                if(notePos > 0)
                {
                    yPos += 0.5 * pawsLine->staffRowHeight;
                }
                else
                {
                    yPos -= 0.5 * pawsLine->staffRowHeight;
                }
            }

            tempString[0] = LEDGER_LINE_SONORA;
            tempString[1] = '\0';

            for(; nLedgerLines > 0; nLedgerLines--)
            {
                pawsLine->graphics2D->Write(pawsLine->GetFont(), hPos, yPos, pawsLine->GetFontColour(), -1, tempString);

                if(notePos > 0)
                {
                    yPos += pawsLine->staffRowHeight;
                }
                else
                {
                    yPos -= pawsLine->staffRowHeight;
                }
            }
        }

        // printing everything
        pawsLine->graphics2D->Write(pawsLine->GetFont(), nextHPosLowString, yNoteLow, pawsLine->GetFontColour(), -1, lowString);
        pawsLine->graphics2D->Write(pawsLine->GetFont(), nextHPosUpString, yNoteUp, pawsLine->GetFontColour(), -1, upString);

        // updating state
        prevNotePos = notePos;
        if(isLast)
        {
            isNoteDown = false;
        }
    }
}

csString Chord::ToXML()
{
    csString note;

    note = "";

    // adding notes
    for(size_t i = 0; i < notes.GetSize(); i++)
    {
        note += "<note>";

        // note
        if(isRest)
        {
            note += "<rest/>";
        }
        else
        {
            if(i != 0)
            {
                note += "<chord/>";
            }
            note += notes[i].ToXML();
        }

        // duration
        note.AppendFmt("<duration>%d</duration>", duration);

        note += "</note>";
    }


    return note;
}

wchar_t Chord::GetNoteChar(bool isNoteDown)
{
    wchar_t noteChar = '\0';

    // setting the character
    switch(duration)
    {
    case WHOLE_DURATION:
    case DOTTED_WHOLE_DURATION:
        if(isRest)
            noteChar = WHOLE_REST_SONORA;
        else
            noteChar = WHOLE_SONORA;
        break;

    case HALF_DURATION:
    case DOTTED_HALF_DURATION:
        if(isRest)
            noteChar = HALF_REST_SONORA;
        else
        {
            if(isNoteDown)
                noteChar = HALF_DOWN_SONORA;
            else
                noteChar = HALF_UP_SONORA;
        }
        break;

    case QUARTER_DURATION:
    case DOTTED_QUARTER_DURATION:
        if(isRest)
            noteChar = QUARTER_REST_SONORA;
        else
        {
            if(isNoteDown)
                noteChar = QUARTER_DOWN_SONORA;
            else
                noteChar = QUARTER_UP_SONORA;
        }
        break;

    case EIGHTH_DURATION:
    case DOTTED_EIGHTH_DURATION:
        if(isRest)
            noteChar = EIGHTH_REST_SONORA;
        else
        {
            if(isNoteDown)
                noteChar = EIGHTH_DOWN_SONORA;
            else
                noteChar = EIGHTH_UP_SONORA;
        }
        break;

    case SIXTEENTH_DURATION:
        if(isRest)
            noteChar = SIXTEENTH_REST_SONORA;
        else
        {
            if(isNoteDown)
                noteChar = SIXTEENTH_DOWN_SONORA;
            else
                noteChar = SIXTEENTH_UP_SONORA;
        }
        break;
    }

    return noteChar;
}

wchar_t Chord::GetChordNoteChar(bool isNoteDown)
{
    wchar_t chordNote = '\0';

    if(duration < HALF_DURATION)
    {
        if(isNoteDown)
        {
            chordNote = QUARTER_DOWN_SONORA;
        }
        else
        {
            chordNote = QUARTER_UP_SONORA;
        }
    }
    else if(duration < WHOLE_DURATION)
    {
        if(isNoteDown)
        {
            chordNote = HALF_DOWN_SONORA;
        }
        else
        {
            chordNote = HALF_UP_SONORA;
        }
    }
    else // duration == whole or dotted_whole
    {
        chordNote = WHOLE_SONORA;
    }

    return chordNote;
}

wchar_t Chord::GetCircleNoteChar()
{
    wchar_t circleNote = '\0';

    if(duration < HALF_DURATION)
    {
        circleNote = FULL_CIRCLE_SONORA;
    }
    else if(duration < WHOLE_DURATION)
    {
        circleNote = EMPTY_CIRCLE_SONORA;

    }
    else // duration == whole or dotted_whole
    {
        circleNote = WHOLE_SONORA;
    }

    return circleNote;
}

//--------------------------------------------------

Measure::Measure(Chord* first)
{
    prev = 0;
    next = 0;

    startRepeat = false;
    endRepeat = 0;
    ending = 0;

    nChords = 1;
    notesSpace = 50;
    firstChord = first;
    lastChord = first;
}

Measure::Measure(csRef<iDocumentNode> measureNode, int quarterDivisions)
{
    // initializing
    prev = 0;
    next = 0;

    startRepeat = false;
    endRepeat = 0;
    ending = 0;

    nChords = 0;
    notesSpace = 50;
    firstChord = 0;
    lastChord = 0;

    // loading node
    csRef<iDocumentNodeIterator> barlineIter = measureNode->GetNodes("barline");
    csRef<iDocumentNodeIterator> noteIter = measureNode->GetNodes("note");

    // barline
    while(barlineIter->HasNext())
    {
        csRef<iDocumentNode> barlineNode = barlineIter->Next();

        csRef<iDocumentNode> endingNode = barlineNode->GetNode("ending");
        csRef<iDocumentNode> repeatNode = barlineNode->GetNode("repeat");

        // ending
        if(endingNode != 0)
        {
            ending = endingNode->GetAttributeValueAsInt("number");
        }
        else
        {
            ending = 0;
        }

        // repeat
        if(repeatNode != 0)
        {
            if(csStrCaseCmp(repeatNode->GetAttributeValue("direction"), "forward") == 0)
            {
                startRepeat = true;
            }
            else
            {
                endRepeat = repeatNode->GetAttributeValueAsInt("times");
            }
        }
    }

    // notes
    while(noteIter->HasNext())
    {
        Chord* c;
        csRef<iDocumentNode> noteNode = noteIter->Next();

        csRef<iDocumentNode> restNode = noteNode->GetNode("rest");
        csRef<iDocumentNode> durationNode = noteNode->GetNode("duration");

        // is this a rest?
        if(restNode != 0)
        {
            c = new Chord();
            c->AddNote(0, 0, true);
            PushChord(c);
        }
        else
        {
            csRef<iDocumentNode> chordNode = noteNode->GetNode("chord");
            csRef<iDocumentNode> pitchNode = noteNode->GetNode("pitch");

            if(chordNode != 0 && lastChord != 0)
            {
                c = lastChord;
                c->AddNote(pitchNode);
            }
            else
            {
                c = new Chord();
                c->AddNote(pitchNode);
                PushChord(c);
            }
        }

        // converting duration into sixteenth divisions
        int duration = durationNode->GetContentsValueAsInt() * quarterDivisions / 4;

        // setting chord duration
        c->SetDuration(GetBiggerDivision(duration));
    }

    // if there are no notes create an empty one
    if(firstChord == 0)
    {
        PushChord(new Chord());
    }
}

Measure::~Measure()
{
    Chord* tempChord;
    Chord* stopChord = lastChord->next;

    // deleting chords
    while(firstChord != stopChord)
    {
        tempChord = firstChord->next;
        delete firstChord;
        firstChord = tempChord;
    }

    // updating prev and next
    if(prev != 0)
    {
        prev->next = next;
    }
    if(next != 0)
    {
        next->prev = prev;

        // updating near ending
        if(ending > 0 && next->GetEnding() > 0)
        {
            ending = 0;
            next->SetEnding(true);
        }
    }
}

uint Measure::GetSize(uint noteLength)
{
    uint size = nChords * notesSpace;

    if(startRepeat)
    {
        size += noteLength;
    }

    if(endRepeat > 0)
    {
        size += noteLength;
    }

    return size;
}

uint Measure::GetMimimumSize(uint noteLength)
{
    uint size = nChords * MIMIMUM_NOTE_SPACE * noteLength;

    if(startRepeat)
    {
        size += noteLength;
    }

    if(endRepeat > 0)
    {
        size += noteLength;
    }

    return size;
}

bool Measure::SetSize(uint size, uint noteLength)
{
    // handling repeats
    if(startRepeat)
    {
        size -= noteLength;
    }

    if(endRepeat)
    {
        size -= noteLength;
    }

    // setting notes' space
    uint newNotesSpace = size / nChords;

    if(newNotesSpace < MIMIMUM_NOTE_SPACE * noteLength)
    {
        return false;
    }

    notesSpace = newNotesSpace;

    return true;
}

void Measure::SetEnding(bool isEnding)
{
    // setting ending number
    if(isEnding)
    {
        if(prev == 0)
        {
            ending = 1;
        }
        else
        {
            ending = prev->ending + 1;
        }

        // updating prev repeat
        if(prev != 0
            && prev->GetEnding() > 0
            && prev->GetEndRepeat() == 0)
        {
            prev->SetEndRepeat(1);
        }

        // updating this repeat
        if(endRepeat == 0
            && next != 0
            && next->GetEnding() > 0)
        {
            endRepeat = 1;
        }
    }
    else
    {
        ending = 0;

        // updating prev repeat
        if(prev != 0
            && prev->GetEnding() > 0)
        {
            prev->SetEndRepeat(0);
        }

        // updating this repeat
        endRepeat = 0;
    }

    // updating following endings
    if(next != 0)
    {
        if(next->GetEnding() > 0)
        {
            next->SetEnding(true);
        }
    }
}

void Measure::AppendMeasure(Measure* measure)
{
    next = measure;
    measure->prev = this;

    lastChord->next = measure->firstChord;
    measure->firstChord->prev = lastChord;
}

void Measure::AttachMeasure(Measure* measure)
{
    // checking if the measure is already here
    if(next == measure)
    {
        return;
    }

    // updating pointers
    measure->prev = this;
    measure->next = next;

    if(next != 0)
    {
        next->prev = measure;
    }
    next = measure;

    // updating chords
    measure->firstChord->prev = lastChord;
    measure->lastChord->next = lastChord->next;

    if(lastChord->next != 0)
    {
        lastChord->next->prev = measure->lastChord;
    }
    lastChord->next = measure->firstChord;
}

void Measure::PushChord(Chord* chord)
{
    if(firstChord == 0) // this is the first chord
    {
        firstChord = chord;
        lastChord = chord;
        nChords = 1;
    }
    else
    {
        InsertChord(lastChord, chord);
    }
}

Chord* Measure::InsertChord(Chord* nearChord, bool before)
{
    Chord* newChord;
    bool found = false;

    // checking the near chord belongs to this measure
    for(Chord* c = firstChord; c != lastChord->next; c = c->next)
    {
        if(c == nearChord)
        {
            found = true;
            break;
        }
    }

    if(!found)
    {
        return 0;
    }

    newChord = new Chord();

    // inserting
    if(before)
    {
        // updating first chord
        if(nearChord == firstChord)
        {
            firstChord = newChord;
        }

        if(nearChord->prev == 0)
        {
            newChord->next = nearChord;
            nearChord->prev = newChord;
            nChords++;
        }
        else
        {
            InsertChord(nearChord->prev, newChord);
        }
    }
    else
    {
        InsertChord(nearChord, newChord);
    }

    return newChord;
}

void Measure::DeleteChord(Chord* chord)
{
    bool found = false;

    for(Chord* c = firstChord; c != lastChord->next; c = c->next)
    {
        if(c == chord)
        {
            found = true;
            break;
        }
    }

    if(found)
    {
        DeleteOwnChord(chord);
    }
}

void Measure::Empty()
{
    Chord* c = firstChord;

    while(c != lastChord->next)
    {
        Chord* tempChord = c->next;
        DeleteOwnChord(c); // DeleteChord empty the last chord
        c = tempChord;
    }
}

bool Measure::Cut(uint divisions)
{
    uint prevDivisions;
    uint totDivisions = 0;
    Chord* chord;

    for(chord = firstChord; chord != lastChord->next; chord = chord->next)
    {
        prevDivisions = totDivisions;
        totDivisions += chord->GetDuration();

        if(totDivisions > divisions)
        {
            totDivisions = prevDivisions;

            if(totDivisions == divisions)
            {
                break;
            }

            while(totDivisions < divisions)
            {
                int rightDuration = GetBiggerDivision(divisions - totDivisions);
                chord->SetDuration(rightDuration);
                totDivisions += rightDuration;

                if(totDivisions < divisions)
                {
                    InsertChord(chord, new Chord(chord));
                    chord = chord->next;
                }
            }

            chord = chord->next; // first chord to delete
            break;
        }
    }

    // deleting exceeding notes, chord is now the first one to be deleted
    while(chord != lastChord->next)
    {
        Chord* tempChord = chord->next;

        // DeleteChord() update the last chord so this must be treated differentely
        if(chord == lastChord)
        {
            DeleteOwnChord(chord);
            break;
        }

        DeleteOwnChord(chord);
        chord = tempChord;
    }

    return (totDivisions == divisions && !lastChord->IsEmpty());
}

void Measure::Fill(uint divisions)
{
    int dur;
    Chord* chord;
    uint totDivisions = 0;

    for(chord = firstChord; /* end condition inside*/; chord = chord->next)
    {
        // converting empty chords
        if(chord->IsEmpty())
        {
            chord->AddNote(0, 0, true);
        }

        totDivisions += chord->GetDuration();

        if(chord == lastChord)
        {
            break;
        }
    }

    // filling
    while(totDivisions < divisions)
    {
        dur = GetBiggerDivision(divisions - totDivisions);

        chord = new Chord();
        chord->AddNote(0, 0, true);
        chord->SetDuration(dur);

        PushChord(chord);

        totDivisions += dur;
    }
}

void Measure::Draw(pawsSheetLine* pawsLine, Chord* selectedChord, int startPosition)
{
    Chord* chord;

    // drawing ending line
    if(ending > 0)
    {
        wchar_t* wstrEnding;
        csString strEnding;
        int vNumPos = pawsLine->screenFrame.ymin + pawsLine->centralCPos - 6 * pawsLine->staffRowHeight;
        int vLinePos = pawsLine->screenFrame.ymin + pawsLine->staffMarginUp;

        // setting ending's number string
        strEnding = ending;
        size_t strEndingLength = strEnding.Length();
        wstrEnding = new wchar_t[strEndingLength + 1];

        for(size_t i = 0; i < strEndingLength; i++)
        {
            wstrEnding[i] = pawsSheetLine::GetSmallNumber(strEnding[i]);
            i++;
        }
        wstrEnding[strEndingLength] = '\0';

        // drawing vertical part
        pawsLine->graphics2D->DrawLine(startPosition, vLinePos, startPosition, vLinePos - 2 * pawsLine->staffRowHeight, pawsLine->GetFontColour());

        // writing ending number
        pawsLine->graphics2D->Write(pawsLine->GetFont(), startPosition + 0.5 * pawsLine->noteCharLength, vNumPos, pawsLine->GetFontColour(), -1, wstrEnding);

        // horizontal part
        vLinePos -= 2 * pawsLine->staffRowHeight;
        pawsLine->graphics2D->DrawLine(startPosition, vLinePos, startPosition + GetSize(pawsLine->noteCharLength), vLinePos, pawsLine->GetFontColour());

        //deleting temporary string
        delete[] wstrEnding;
    }

    // drawing start repeat dots
    if(startRepeat)
    {
        DrawRepeatDots(pawsLine, startPosition, true);
        startPosition += pawsLine->noteCharLength;
    }

    // drawing chords
    startPosition += (notesSpace - pawsLine->noteCharLength) / 2;

    for(chord = firstChord; chord != lastChord->next; chord = chord->next)
    {
        chord->Draw(pawsLine, selectedChord, startPosition);
        startPosition += notesSpace;
    }

    // drawing end repeat dots
    if(endRepeat > 0)
    {
        startPosition -= (notesSpace - pawsLine->noteCharLength) / 2;
        DrawRepeatDots(pawsLine, startPosition, false);
    }
}

bool Measure::Hit(int x, Chord* &chord, bool &before)
{
    float hitPos = (float)x / notesSpace;

    uint hitChord = hitPos;     // truncation
    hitPos -= hitChord;         // decimal part

    if(hitChord > nChords)
    {
        chord = 0;
        return false;
    }

    // before or after?
    before = hitPos < 0.5;

    // selecting chord
    chord = firstChord;
    for(size_t i = 0; i < hitChord; i++)
    {
        chord = chord->next;
    }

    return true;
}

csString Measure::ToXML(uint number, csString attributes)
{
    csString measure;

    measure.AppendFmt("<measure number=\"%d\">", number);

    // attributes and direction
    measure += attributes;

    // barlines
    if(startRepeat)
    {
        measure += "<barline location=\"left\"><repeat direction=\"forward\"/></barline>";
    }
    
    if(ending > 0)
    {
        measure.AppendFmt("<barline location=\"right\"><ending number=\"%d\"/>", ending);
        measure.AppendFmt("<repeat direction=\"backward\" times=\"%d\"/></barline>", endRepeat);
    }
    else if(endRepeat > 0)
    {
        measure.AppendFmt("<barline location=\"right\"><repeat direction=\"backward\" times=\"%d\"/></barline>", endRepeat);
    }

    // chords
    for(Chord* c = firstChord; c != lastChord->next; c = c->next)
    {
        measure += c->ToXML();
    }

    measure += "</measure>";

    return measure;
}

void Measure::InsertChord(Chord* prevChord, Chord* chord)
{
    if(prevChord == lastChord)
    {
        lastChord = chord;
    }

    // updating chord's pointers
    prevChord->AttachChord(chord);

    nChords++;
}

void Measure::DeleteOwnChord(Chord* chord)
{
    if(chord == firstChord)
    {
        // check if there is only a chord
        if(firstChord == lastChord)
        {
            firstChord->Empty();
            return;
        }

        firstChord = firstChord->next;
    }
    else if(chord == lastChord)
    {
        lastChord = chord->prev;
    }

    delete chord;
    nChords--;
}

void Measure::DrawRepeatDots(pawsSheetLine* pawsLine, int x, bool start)
{
    wchar_t dot[2];
    dot[0] = DOT_SONORA;
    dot[1] = '\0';

    // setting position
    if(start)
    {
        x += 0.5 * pawsLine->noteCharLength;
    }
    int vPos = pawsLine->screenFrame.ymin + pawsLine->centralCPos - 2.5 * pawsLine->staffRowHeight;

    // drawing dots
    pawsLine->graphics2D->Write(pawsLine->GetFont(), x, vPos, pawsLine->GetFontColour(), -1, dot);
    pawsLine->graphics2D->Write(pawsLine->GetFont(), x, vPos - pawsLine->staffRowHeight, pawsLine->GetFontColour(), -1, dot);

    if(pawsLine->isDouble)
    {
        vPos += 6 * pawsLine->staffRowHeight;
        pawsLine->graphics2D->Write(pawsLine->GetFont(), x, vPos, pawsLine->GetFontColour(), -1, dot);
        pawsLine->graphics2D->Write(pawsLine->GetFont(), x, vPos - pawsLine->staffRowHeight, pawsLine->GetFontColour(), -1, dot);
    }
}

int Measure::GetBiggerDivision(uint divisions)
{
    if(DOTTED_WHOLE_DURATION <= divisions)
        return DOTTED_WHOLE_DURATION;
    else if(WHOLE_DURATION <= divisions)
        return WHOLE_DURATION;
    else if(DOTTED_HALF_DURATION <= divisions)
        return DOTTED_HALF_DURATION;
    else if(HALF_DURATION <= divisions)
        return HALF_DURATION;
    else if(DOTTED_QUARTER_DURATION <= divisions)
        return DOTTED_QUARTER_DURATION;
    else if(QUARTER_DURATION <= divisions)
        return QUARTER_DURATION;
    else if(DOTTED_EIGHTH_DURATION <= divisions)
        return DOTTED_EIGHTH_DURATION;
    else if(EIGHTH_DURATION <= divisions)
        return EIGHTH_DURATION;
    
    return SIXTEENTH_DURATION;
}

//--------------------------------------------------

SheetLine::SheetLine(Measure* measure)
{
    prev = 0;
    next = 0;

    size = 0;
    noteLength = 0;

    firstMeasure = measure;
    lastMeasure = measure;

    hasCallback = false;
    callbackObject = 0;
    callbackFunction = 0;
}

SheetLine::~SheetLine()
{
    // measures are deleted by PawsMusicWindow

    // updating prev and next
    if(prev != 0)
    {
        prev->next = next;
    }
    if(next != 0)
    {
        next->prev = prev;
    }

    // updating prev and next so that the call back object
    // knows if this line is being deleting or creating
    prev = 0;
    next = 0;

    Callback();
}

bool SheetLine::Contains(Measure* measure)
{
    for(Measure* m = firstMeasure; m != lastMeasure->next; m = m->next)
    {
        if(measure == m)
        {
            return true;
        }
    }

    return false;
}

bool SheetLine::SetSize(uint newSize, pawsSheetLine* pawsLine)
{
    bool isChanged = false;

    if(size != newSize)
    {
        isChanged = true;
        size = newSize;
    }

    if(noteLength != (uint)pawsLine->noteCharLength)
    {
        isChanged = true;
        noteLength = pawsLine->noteCharLength;
    }

    return isChanged;
}

SheetLine* SheetLine::AttachNewLine(Measure* firstMeasure)
{
    if(next != 0)
    {
        return 0;
    }

    SheetLine* sheetLine = new SheetLine(firstMeasure);

    // sizing
    sheetLine->size = size;
    sheetLine->noteLength = noteLength;

    // callback
    sheetLine->hasCallback = hasCallback;
    sheetLine->callbackObject = callbackObject;
    sheetLine->callbackFunction = callbackFunction;

    // attaching
    sheetLine->prev = this;
    next = sheetLine;
    lastMeasure->AttachMeasure(firstMeasure);

    Callback();

    return sheetLine;
}

Measure* SheetLine::InsertNewMeasure(Measure* measureAfter)
{
    bool found = false;
    Chord* newChord;
    Measure* newMeasure;

    // checking if the given measure belongs to this line
    for(Measure* m = firstMeasure; m != lastMeasure->next; m = m->next)
    {
        if(m == measureAfter)
        {
            found = true;
            break;
        }
    }

    if(!found)
    {
        return 0;
    }

    newChord = new Chord();
    newMeasure = new Measure(newChord);

    // adjusting first measure
    if(measureAfter == firstMeasure)
    {
        firstMeasure = newMeasure;
    }

    // inserting
    if(measureAfter->prev == 0)
    {
        newMeasure->AppendMeasure(measureAfter);
    }
    else
    {
        measureAfter->prev->AttachMeasure(newMeasure);
    }

    return newMeasure;
}

void SheetLine::PushMeasure(Measure* measure)
{
    lastMeasure->AttachMeasure(measure);
    lastMeasure = measure;
}

void SheetLine::DeleteMeasure(Measure* measure)
{
    bool found = false;

    // checking if this measure belongs to this line
    for(Measure* m = firstMeasure; m != lastMeasure->next; m = m->next)
    {
        if(m == measure)
        {
            found = true;
            break;
        }
    }

    if(!found)
    {
        return;
    }

    // if this is the last measure empty it
    if(measure->next == 0)
    {
        measure->Empty();
    }
    else // the measure must be deleted
    {
        if(firstMeasure == lastMeasure)
        {
            firstMeasure = 0;
            lastMeasure = 0;
        }
        else if(measure == firstMeasure)
        {
            firstMeasure = measure->next;
        }
        else if(measure == lastMeasure)
        {
            lastMeasure = measure->prev;
        }

        delete measure;
    }
}

bool SheetLine::Resize()
{
    bool resizeNext;
    uint totSize = 0;
    uint totChords = 0;
    uint sizePerChord;
    Measure* newFirstMeasure;
    Measure* newLastMeasure;

    // getting the new first measure
    if(prev != 0)
    {
        newFirstMeasure = prev->lastMeasure->next;
    }
    else
    {
        newFirstMeasure = firstMeasure;
    }

    // getting the new last measure
    newLastMeasure = newFirstMeasure;
    while(true)
    {
        totSize += newLastMeasure->GetMimimumSize(noteLength);
        totChords += newLastMeasure->GetNChords();

        // print at least one measure
        if(totSize > size && newLastMeasure != newFirstMeasure)
        {
            totChords -= newLastMeasure->GetNChords();
            newLastMeasure = newLastMeasure->prev;
            break;
        }

        if(newLastMeasure->next == 0)
        {
            break;
        }

        newLastMeasure = newLastMeasure->next;
    }

    // should the following lines be resized?
    if(lastMeasure == newLastMeasure)
    {
        resizeNext = false;
    }
    else
    {
        resizeNext = true;
    }

    // updating measure state
    firstMeasure = newFirstMeasure;
    lastMeasure = newLastMeasure;

    // setting new sizes for measures
    sizePerChord = size / totChords;
    for(Measure* m = firstMeasure; m != lastMeasure->next; m = m->next)
    {
        if(lastMeasure->GetLastChord()->IsEmpty())
        {
            m->SetSize(m->GetMimimumSize(noteLength), noteLength);
        }
        else
        {
            m->SetSize(sizePerChord * m->GetNChords(), noteLength);
        }
    }

    return resizeNext;
}

void SheetLine::ResizeAll(bool forceAll)
{
    SheetLine* emptyLine = 0;
    SheetLine* sheetLine = this;

    while(sheetLine->Resize() || forceAll)
    {
        if(sheetLine->HasLastMeasure())
        {
            emptyLine = sheetLine->next;
            break;
        }
        else if(sheetLine->next == 0)
        {
            Measure* m = sheetLine->lastMeasure->next;
            sheetLine = sheetLine->AttachNewLine(m);
        }
        else
        {
            sheetLine = sheetLine->next;
        }
    }

    // deleting useless sheet lines
    while(emptyLine != 0)
    {
        SheetLine* tempLine = emptyLine->next;
        delete emptyLine;
        emptyLine = tempLine;
    }
}

void SheetLine::Draw(pawsSheetLine* pawsLine, Chord* selectedChord, int startPosition, int height)
{
    Measure* measure;
    int yMin = pawsLine->screenFrame.ymin + pawsLine->staffMarginUp;

    for(measure = firstMeasure; /* end condition inside */; measure = measure->next)
    {
        measure->Draw(pawsLine, selectedChord, startPosition);

        if(measure == lastMeasure)
        {
            break;
        }

        // printing end measure if this is not the last measure
        startPosition += measure->GetSize(noteLength);
        pawsLine->graphics2D->DrawLine(startPosition, yMin, startPosition, yMin + height, pawsLine->GetFontColour());
    }
}

bool SheetLine::Hit(int x, Chord* &chord, Measure* &measure, bool &before)
{
    int prevSize;
    int size = 0;

    for(measure = firstMeasure; measure != lastMeasure->next; measure = measure->next)
    {
        prevSize = size;
        size += measure->GetSize(noteLength);

        if(x <= size)
        {
            return measure->Hit(x - prevSize, chord, before);
        }
    }

    chord = 0;
    measure = 0;
    return false;
}

Chord* SheetLine::Advance(uint divisions)
{
    Chord* endChord = new Chord();

    // pushing the chord in the last measure
    // if the last measure is completed create a new measure
    if(lastMeasure->Cut(divisions))
    {
        PushMeasure(new Measure(endChord));
    }
    else
    {
        lastMeasure->PushChord(endChord);
    }

    ResizeAll();
    return endChord;
}

void SheetLine::SetCallback(void* object, void (*function)(void*, SheetLine*))
{
    hasCallback = true;
    callbackObject = object;
    callbackFunction = function;
}

void SheetLine::Callback()
{
    if(hasCallback)
    {
        callbackFunction(callbackObject, this);
    }
}

//--------------------------------------------------

pawsSheetLine::pawsSheetLine()
{
    isDouble = true;
    line = 0;
}

pawsSheetLine::~pawsSheetLine()
{
    // Note: line is deleted by pawsMusicWindow.
}

void pawsSheetLine::SetLine(SheetLine* sheetLine, bool resize)
{
    int nAlter;
    int newSize;
    pawsMusicWindow* musicWindow = static_cast<pawsMusicWindow*>(parent);

    line = sheetLine;

    if(line == 0)
    {
        return;
    }

    // setting size of the line
    nAlter = (musicWindow->fifths >= 0 ? musicWindow->fifths : -musicWindow->fifths);
    newSize = screenFrame.xmax - screenFrame.xmin - 2 * staffMarginLateral - cleffCharLength - nAlter * alterCharLength - doubleLineWidth;;

    if(line->HasFirstMeasure())
    {
        newSize -= metricCharLength;
    }

    if(line->SetSize(newSize, this) && resize)
    {
        line->ResizeAll();
    }
}

void pawsSheetLine::SetDoubleStaff(bool doubleStaff)
{
    if(isDouble == doubleStaff)
    {
        return;
    }

    isDouble = doubleStaff;

    // changing size
    if(isDouble)
    {
        ChangeFontSize(myFont->GetSize() * SINGLE_STAFF_FONT_SIZE);
    }
    else
    {
        ChangeFontSize(myFont->GetSize() / SINGLE_STAFF_FONT_SIZE);
    }

    // updating drawing parameters
    SetDrawParameters();
}

void pawsSheetLine::DrawDoubleLine(int x, int y, int height)
{
    // first thin line
    graphics2D->DrawLine(x - doubleLineWidth, y, x - doubleLineWidth, y + height, GetFontColour());

    // second thick line
    int width = 0.8 * doubleLineWidth;
    graphics2D->DrawBox(x - width, y, width + 1, height + 1, GetFontColour());
}

void pawsSheetLine::Draw()
{
    pawsWidget::Draw();
    ClipToParent(false);

    if(line == 0)
    {
        return;
    }

    pawsMusicWindow* musicWindow = static_cast<pawsMusicWindow*>(parent);
    int lastRow = 4;
    int xMin = screenFrame.xmin + staffMarginLateral;
    int yMin = screenFrame.ymin + staffMarginUp;
    int staffLength = screenFrame.xmax - staffMarginLateral;

    // creating temporary buffer, the longest string to represent is tempo
    csString strTempo;
    strTempo = musicWindow->tempo;
    size_t tempoLength = strTempo.Length();
    wchar_t* printSymbol = new wchar_t[tempoLength + 2];
    printSymbol[1] = (wchar_t)'\0';

    // drawing staff
    if(isDouble)
    {
        lastRow = 10;
    }

    for(int i = 0; i <= lastRow; i++)
    {
        if(i == 5)
        {
            continue;
        }

        graphics2D->DrawLine(xMin, yMin + i*staffRowHeight, staffLength, yMin + i*staffRowHeight, GetFontColour());
    }

    // drawing starting and ending line
    graphics2D->DrawLine(xMin, yMin, xMin, yMin + lastRow*staffRowHeight - 1, GetFontColour());

    if(line->HasLastMeasure())
    {
        DrawDoubleLine(staffLength + 1, yMin, lastRow*staffRowHeight - 1);
    }
    else
    {
        graphics2D->DrawLine(staffLength + 1, yMin, staffLength + 1, yMin + lastRow*staffRowHeight - 1, GetFontColour());
    }

    // drawing clefs
    printSymbol[0] = G_CLEF_SONORA;
    graphics2D->Write(myFont, xMin, yMin - 3*staffRowHeight, GetFontColour(), -1, printSymbol);

    if(isDouble)
    {
        printSymbol[0] = F_CLEF_SONORA;
        graphics2D->Write(myFont, xMin, yMin + 2*staffRowHeight, GetFontColour(), -1, printSymbol);
    }

    // drawing alterations
    int nAlter;     // number of alterations
    float row;      // keep track of the row where the alteration must be printed (0 is central C)
    int alterFlag;  // 1 for tonalities with sharps, -1 for tonalities with flats

    if(musicWindow->fifths > 0)
    {
        printSymbol[0] = SHARP_SONORA;
        nAlter = musicWindow->fifths;
        row = 5;    // F5 is the first sharp
        alterFlag = 1;
    }
    else
    {
        printSymbol[0] = FLAT_SONORA;
        nAlter = -musicWindow->fifths;
        row = 3;    // B4 is the first flat
        alterFlag = -1;
    }

    for(int i = 0; i < nAlter; i++)
    {
        graphics2D->Write(myFont, xMin + cleffCharLength + i*alterCharLength,
            yMin - row*staffRowHeight, GetFontColour(), -1, printSymbol);

        row += alterFlag * 2; // increase/decrease of a fifth

        if(alterFlag == 1 && row > 5.5)
        {
            row -= 3.5; // decrease of an octave
        }
        if(alterFlag == -1 && row < 1.5)
        {
            row += 3.5;
        }
    }

    if(isDouble)
    {
        if(alterFlag == 1)
        {
            row = 4;
        }
        else
        {
            row = 2;
        }

        for(int i = 0; i < nAlter; i++)
        {
            graphics2D->Write(myFont, xMin + cleffCharLength + i*alterCharLength,
                yMin + (6 - row)*staffRowHeight, GetFontColour(), -1, printSymbol);

            row += alterFlag * 2; // increase/decrease of a fifth

            if(alterFlag == 1 && row > 4.5)
            {
                row -= 3.5; // decrease of an octave
            }
            if(alterFlag == -1 && row < 1.5)
            {
                row += 3.5;
            }
        }
    }

    // drawing line
    int lineStartPos = xMin + cleffCharLength + nAlter*alterCharLength;

    if(line->HasFirstMeasure())
    {
        wchar_t beats[3];
        wchar_t beatType[3];

        size_t beatsLength = musicWindow->beats.Length();
        size_t beatTypeLength = musicWindow->beatType.Length();

        // getting meter
        for(size_t i = 0; i < beatsLength; i++)
        {
            beats[i] = pawsSheetLine::GetSymbolChar(musicWindow->beats[i]);
        }
        beats[beatsLength] = '\0';

        for(size_t i = 0; i < beatTypeLength; i++)
        {
            beatType[i] = pawsSheetLine::GetSymbolChar(musicWindow->beatType[i]);
        }
        beatType[beatTypeLength] = '\0';

        //drawing metric
        graphics2D->Write(myFont, lineStartPos, yMin - 4*staffRowHeight, GetFontColour(), -1, beats);
        graphics2D->Write(myFont, lineStartPos, yMin - 2*staffRowHeight, GetFontColour(), -1, beatType);

        if(isDouble)
        {
            graphics2D->Write(myFont, lineStartPos, yMin + 2*staffRowHeight, GetFontColour(), -1, beats);
            graphics2D->Write(myFont, lineStartPos, yMin + 4*staffRowHeight, GetFontColour(), -1, beatType);
        }

        lineStartPos += metricCharLength; 

        // drawing tempo
        printSymbol[0] = TEMPO_SONORA;
        for(size_t i = 0; i < tempoLength; i++)
        {
            printSymbol[i+1] = pawsSheetLine::GetSmallNumber(strTempo.GetAt(i));
        }
        printSymbol[tempoLength+1] = (wchar_t)'\0';

        graphics2D->Write(myFont, xMin + cleffCharLength, yMin - 7*staffRowHeight, GetFontColour(), -1, printSymbol);
    }

    // drawing notes
    line->Draw(this, musicWindow->selectedChord, lineStartPos, lastRow * staffRowHeight);

    // deleting temporary buffer
    delete[] printSymbol;
}

bool pawsSheetLine::OnMouseDown(int button, int modifiers, int x, int y)
{
    int nAlter;
    int startPosition;
    pawsMusicWindow* musicWindow;

    if(line == 0)
    {
        return pawsWidget::OnMouseDown(button, modifiers, x, y);
    }

    musicWindow = static_cast<pawsMusicWindow*>(parent);
    nAlter = (musicWindow->fifths >= 0 ? musicWindow->fifths : -musicWindow->fifths);
    startPosition = screenFrame.xmin + staffMarginLateral + cleffCharLength + nAlter * alterCharLength;
    if(line->HasFirstMeasure())
    {
        startPosition += metricCharLength;
    }

    // checking if the click is inside the staff's part with notes
    if(x > startPosition)
    {
        bool before;
        Chord* chord;
        Measure* measure;

        // selecting chord
        line->Hit(x - startPosition, chord, measure, before);

        if(chord != 0)
        {
            int notePosition;
            float relativeVertPos;  // vertical position from the central C line

            relativeVertPos = y - screenFrame.ymin - staffMarginUp - 5 * staffRowHeight;

            if(relativeVertPos >= 0)
            {
                relativeVertPos += 0.25 * staffRowHeight;
            }
            else
            {
                relativeVertPos -= 0.25 * staffRowHeight;
            }

            // the first minus sign is there because note position is > 0 above the central C
            notePosition = -(relativeVertPos/ staffRowHeight) * 2;

            // taking action
            musicWindow->OnChordSelection(line, measure, chord, notePosition, before);
        }
    }

    return true;
}

bool pawsSheetLine::PostSetup()
{
    SetDrawParameters();

    return true;
}

wchar_t pawsSheetLine::GetSymbolChar(char symbol)
{
    return symbol | SYMBOL_ENCODING_BASE;
}

wchar_t pawsSheetLine::GetSmallNumber(char number)
{
    switch(number)
    {
    case '0':
        return ZERO_SMALL_SONORA;
    case '1':
        return ONE_SMALL_SONORA;
    case '2':
        return TWO_SMALL_SONORA;
    case '3':
        return THREE_SMALL_SONORA;
    case '4':
        return FOUR_SMALL_SONORA;
    case '5':
        return FIVE_SMALL_SONORA;
    case '6':
        return SIX_SMALL_SONORA;
    case '7':
        return SEVEN_SMALL_SONORA;
    case '8':
        return EIGHT_SMALL_SONORA;
    case '9':
        return NINE_SMALL_SONORA;
    }

    return '\0';
}

// TODO make it font dependent
// Right now it supports only the font Sonora
void pawsSheetLine::SetDrawParameters()
{
    float fontSize = GetFontSize();

    // all the numbers here are from direct measurements on the font Sonora
    cleffCharLength = fontSize * 19 / 35 * 1.25; // 1.25 of the cleff's length
    alterCharLength = fontSize * 8 / 35;
    metricCharLength = fontSize * 0.225;
    ledgerLineShift = fontSize * 0.085;

    noteCharLength = fontSize * 0.24;
    doubleLineWidth = 0.8 * noteCharLength;

    staffRowHeight = fontSize * 7.5 / 35;
    noteCharHeight = 4 * staffRowHeight;

    staffMarginLateral = 20;        // TODO make it size dependent when you'll be able to draw the brace

    // staffMarginUp = (frame's height - staff space) / 2
    staffMarginUp = (screenFrame.ymax - screenFrame.ymin - 10 * staffRowHeight) / 2;

    if(!isDouble)
    {
        staffMarginUp += 2.5 * staffRowHeight;
    }

    // centralCPos = margin up + 3 positions - staff's vertical shift
    centralCPos = staffMarginUp + 1.5 * staffRowHeight - fontSize * 0.22;
}
