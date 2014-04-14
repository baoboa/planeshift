/*
 * pawssheetline.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef PAWS_SHEET_LINE_H
#define PAWS_SHEET_LINE_H


//====================================================================================
// Project Includes
//====================================================================================
#include <paws/pawswidget.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class Measure;
class SheetLine;
class pawsSheetLine;

/**
 * \addtogroup client_gui
 * @{ */

//--------------------------------------------------

/**
 * The number associated to each duration is the number of sixteenths in that
 * note.
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

//--------------------------------------------------

struct Note
{
    short int position;     ///< The note's position (0 is C4, 1 is D4 etc).
    short int alter;        ///< The note's alteration (-1 flat, 0 normal, 1 sharp).

    /**
     * Constructor.
     *
     * @param position the note's position.
     * @param alter the note's alteration.
     */
    Note(short int position, short int alter);

    /**
     * Writes this note as a \<pitch\> XML node.
     *
     * @return the \<pitch\> XML node.
     */
    csString ToXML();
};

//--------------------------------------------------

class Chord
{
public:
    /**
     * Constructor. Default duration is a quarter.
     */
    Chord();

    /**
     * Copy constructor. It copies notes and duration.
     *
     * @param chord the Chord to copy.
     */
    Chord(Chord* chord);

    /**
     * Destructor. It removes itself from the list.
     */
    ~Chord();

    /**
     * Returns the next Chord.
     *
     * @return the next Chord.
     */
    Chord* Next() { return next; }

    /**
     * Returns the previous Chord.
     *
     * @return the previous Chord.
     */
    Chord* Prev() { return prev; }

    /**
     * Return true if this chord is a rest, false otherwise.
     *
     * @return true if this is a rest.
     */
    bool IsRest() { return isRest; }

    /**
     * Returns true if there are not notes in this chord.
     *
     * @return true if this chord is empty, false otherwise.
     */
    bool IsEmpty() { return notes.GetSize() == 0; }

    /**
     * Gets the duration of this chord.
     *
     * @return the duration of this chord.
     */
    int GetDuration() { return duration; }

    /**
     * Sets the duration of this chord.
     *
     * @param duration one of the values of the enum Duration.
     */
    void SetDuration(int duration);

    /**
     * Adds a note to the chord. If a note in the same position already exists
     * this overwrites it (i.e. the alteration of the note is updated with the
     * new one). If the added note is a rest and there are already other notes
     * in the chord, these are deleted from the chord. Viceversa if the added
     * note is not a rest and there is already a rest in the chord, the rest is
     * removed.
     *
     * @param position the position where the note (or the rest) must be drawn.
     * @param alter the alteration of the note (-1 flat, 0 normal, 1 sharp).
     * @param isRest true if it is a rest.
     */
    void AddNote(int position, int alter, bool isRest);

    /**
     * Adds a not-rest note to this chord from a \<pitch\> XML node.
     *
     * @param pitch the \<pitch\> XML node.
     */
    void AddNote(csRef<iDocumentNode> pitch);

    /**
     * Deletes all the notes in the chord.
     */
    void Empty();

    /**
     * Inserts the given chord after this one.
     *
     * @param chord the chord to insert.
     */
    void AttachChord(Chord* chord);

    /**
     * Draws this chord.
     *
     * @param pawsLine the pawsSheetLine that store the font-dependent parameters.
     * @param selectedChord the current selected chord.
     * @param horizontalPos the horizontal position of the chord.
     */
    void Draw(pawsSheetLine* pawsLine, Chord* selectedChord, int horizontalPos);

    /**
     * Write this chord as a \<note\> XML node.
     * @return the \<note\> XML node.
     */
    csString ToXML();

private:
    friend class Measure;

    Chord* prev;                ///< The previous chord.
    Chord* next;                ///< The next chord.

    bool isRest;                ///< True if this chord is a rest.
    csArray<Note> notes;        ///< The sequence of notes in the chord. Notes are in descending order (from the higher to the lower).
    short int duration;         ///< The duration of this chord.

    /**
     * Returns the note to print according to the chord's duration.
     *
     * @param isNoteDown true if the note must be drawn down.
     * @return the character to print.
     */
    wchar_t GetNoteChar(bool isNoteDown);

    /**
     * Returns the note to print in a chord.
     *
     * @param isNoteDown true if the note must be drawn down.
     * @return the character to print.
     */
    wchar_t GetChordNoteChar(bool isNoteDown);

    /**
     * Returns the note to print without stem.
     * @return the character to print.
     */
    wchar_t GetCircleNoteChar();
};

//--------------------------------------------------

class Measure
{
public:
    /**
     * Constructor. Creates a non empty measure.
     * @param firstChord the first chord of the measure.
     */
    Measure(Chord* firstChord);

    /**
     * Constructor. Creates a measure from a \<measure\> XML node.
     * @param measure the \<measure\> XML node.
     * @param quarterDivisions the number of divisions in a quarter.
     */
    Measure(csRef<iDocumentNode> measure, int quarterDivisions);

    /**
     * Destructor.
     */
    ~Measure();

    /**
     * Returns the next Measure.
     * @return the next Measure.
     */
    Measure* Next() { return next; }

    /**
     * Returns the previous Measure.
     * @return the previous Measure.
     */
    Measure* Prev() { return prev; }

    /**
     * Returns the number of chords in this measure.
     * @return the number of chords in this measure.
     */
    uint GetNChords() { return nChords; }

    /**
     * Returns the measure's size.
     * @param noteLength the length of a note.
     * @return the measure's size.
     */
    uint GetSize(uint noteLength);

    /**
     * Gets the mimimum size of this measure.
     * @param noteLength the length of a note.
     * @return the mimimum size of this measure.
     */
    uint GetMimimumSize(uint noteLength);

    /**
     * Sets the measure's size.
     *
     * @param size the new size of the measure.
     * @param noteLength the length of the note with the current font.
     * @return true if the given space is greater than the minimum space allowed.
     */
    bool SetSize(uint size, uint noteLength);

    /**
     * Gets the start repeat state.
     * @return the start repeat state.
     */
    bool GetStartRepeat() { return startRepeat; }

    /**
     * Sets the start repeat of this measure.
     * @param repeat true to start a repeat, false otherwise.
     */
    void SetStartRepeat(bool repeat) { startRepeat = repeat; }

    /**
     * Gets the number of times this part must be repeat.
     * @return the number of times this part must be repeat.
     */
    uint GetEndRepeat() { return endRepeat; }

    /**
     * Sets the number of times this part must be repeat.
     * @param repeat 0 to not repeat.
     */
    void SetEndRepeat(uint repeat) { endRepeat = repeat; }

    /**
     * Gets the number of this ending.
     * @return the number of the ending, 0 if this is not an ending.
     */
    uint GetEnding() { return ending; }

    /**
     * Sets this measure as an ending and updates all the following endings.
     * @param isEnding true if this is an ending.
     */
    void SetEnding(bool isEnding);

    /**
     * Append the given measure after this one. This is not an insertion and the
     * next measure is lost.
     * @param measure the measure to insert.
     */
    void AppendMeasure(Measure* measure);

    /**
     * Insert the given measure after this one and update the chord list. If the
     * given measure is already the next one it does nothing.
     * @param measure the measure to insert.
     */
    void AttachMeasure(Measure* measure);

    /**
     * Returns the first chord of this measure.
     * @return the first chord of this measure.
     */
    Chord* GetFirstChord() { return firstChord; }

    /**
     * Returns the last chord of this measure.
     * @return the last chord of this measure.
     */
    Chord* GetLastChord() { return lastChord; }

    /**
     * Pushes a chord at the end of the measure.
     * @param chord the new chord.
     */
    void PushChord(Chord* chord);

    /**
     * Inserts a new empty chord before or after the given one.
     *
     * @param nearChord the chord near the new one.
     * @param before true to insert before the given chord, false to insert it after.
     * @return the new Chord.
     */
    Chord* InsertChord(Chord* nearChord, bool before);

    /**
     * Deletes the chord if it belong to this measure.
     * @param chord the chord to delete.
     */
    void DeleteChord(Chord* chord);

    /**
     * Deletes all the chords in this line and keeps only an empty chord.
     */
    void Empty();

    /**
     * Cut the last chords of the measures that exceeds the given number of durations.
     *
     * @param divisions the number of divisions that this measure must have.
     * @return true if this measure is completed, false otherwise.
     */
    bool Cut(uint divisions);

    /**
     * Fills the measure with rests if it is incomplete. Converts all the empty chords
     * into rests of the same duration.
     *
     * @param divisions the number of divisions that this measure must have.
     */
    void Fill(uint divisions);

    /**
     * Draws this measure.
     * @param pawsLine the pawsSheetLine that store the font-dependent parameters.
     * @param selectedChord the current selected chord.
     * @param startPosition the horizontal position where the first chord must be drawn.
     */
    void Draw(pawsSheetLine* pawsLine, Chord* selectedChord, int startPosition);

    /**
     * Check if a chord in this measure is hit.
     *
     * @param x the horizontal position of the input from the beginning of the measure.
     * @param chord at the end this pointer will contains the hit chord or null
     * if any chord has been hit.
     * @param before at the end this boolean will be true if the click has happened
     * before the selected chord, false if after.
     * @return true if a note has been hit, false otherwise.
     */
    bool Hit(int x, Chord* &chord, bool &before);

    /**
     * Writes this measure as a \<measure\> XML node.
     *
     * @param number the number of this measure.
     * @param attributes the attributes and direction nodes.
     * @return the \<measure\> XML node.
     */
    csString ToXML(uint number, csString attributes = "");

private:
    friend class SheetLine;

    Measure* prev;          ///< The previous measure.
    Measure* next;          ///< The next measure.

    bool startRepeat;       ///< True if this is the start of a repeat.
    uint endRepeat;         ///< The number of times that this part must be repeated.
    uint ending;            ///< The ending number.

    uint nChords;           ///< The number of chords in this measure.
    uint notesSpace;        ///< Space between notes.
    Chord* firstChord;      ///< First chord of the measure.
    Chord* lastChord;       ///< Last chord of the measure.

    /**
     * Inserts a chord after the given one.
     * @param prevChord the previous chord.
     * @param chord the chord to insert.
     */
    void InsertChord(Chord* prevChord, Chord* chord);

    /**
     * Not-safe version of DeleteChord(). It does not check if the chord belong to this
     * measure. It never empties a measure. There is always at least an empty chord.
     * @param chord the chord to delete.
     */
    void DeleteOwnChord(Chord* chord);

    /**
     * Draws the repeats dots at the given position.
     * @param pawsLine the pawsSheetLine containing the drawing parameters.
     * @param x the horizontal position where to draw the two dots.
     * @param start true if the two dots are for a start repeat.
     */
    void DrawRepeatDots(pawsSheetLine* pawsLine, int x, bool start);

    /**
     * Returns the bigger duration with the number of divisions less than the given one.
     *
     * @param divisions the maximum number of divisions that the returned duration must
     * have.
     * @return the duration value.
     */
    int GetBiggerDivision(uint divisions);
};

//--------------------------------------------------

class SheetLine
{
public:
    /**
     * Constructor.
     * @param firstMeasure the first measure of the SheetLine.
     */
    SheetLine(Measure* firstMeasure);

    /**
     * Destructor. It does not delete its measures.
     */
    ~SheetLine();

    /**
     * Returns the next SheetLine.
     * @return the next SheetLine.
     */
    SheetLine* Next() { return next; }

    /**
     * Returns the previous SheetLine.
     * @return the previous SheetLine.
     */
    SheetLine* Prev() { return prev; }

    /**
     * Returns true if this line contains the first measure.
     * @return true if this is the first line, false otherwise.
     */
    bool HasFirstMeasure() { return (prev == 0); }

    /**
     * Returns true if this line contains the last measure.
     * @return true if this is the last line, false otherwise.
     */
    bool HasLastMeasure() { return (lastMeasure->next == 0); }

    /**
     * Check if this line contains the given measure.
     * @param measure the measure to look for.
     * @return true if this line contains the given measure, false otherwise.
     */
    bool Contains(Measure* measure);

    /**
     * Returns the first measure of the line.
     * @return the first measure of the line.
     */
    Measure* GetFirstMeasure() { return firstMeasure; }

    /**
     * Returns the last measure of the line.
     * @return the last measure of the line.
     */
    Measure* GetLastMeasure() { return lastMeasure; }

    /**
     * Set the size and drawing parameters for this line.
     * @param size the new size.
     * @param pawsLine the pawsSheetLine with the drawing parameters.
     * @return true if the line's size is changed.
     */
    bool SetSize(uint size, pawsSheetLine* pawsLine);

    /**
     * If this is the last measure, this method creates a new empty line and attaches it
     * after this one. The new line has the same size and the same callback as this one.
     *
     * @param firstMeasure the first measure of the new line.
     * @return the attached SheetLine, a null pointer if this is not the last line.
     */
    SheetLine* AttachNewLine(Measure* firstMeasure);

    /**
     * Inserts a new measure before the given one.
     * @param measureAfter the measure after the new one.
     * @return the new measure.
     */
    Measure* InsertNewMeasure(Measure* measureAfter);

    /**
     * Push a measure at the end of the line. After this method a resize should be done.
     * @param measure the measure to push.
     */
    void PushMeasure(Measure* measure);

    /**
     * Delete the given measure if it belongs to this line. If it is the last measure,
     * the method just empties it.
     *
     * @param measure the measure to delete.
     */
    void DeleteMeasure(Measure* measure);

    /**
     * Fixes the measures of this line and their size.
     * @return true if the following lines should be updated too, false otherwise.
     */
    bool Resize();

    /**
     * Fixes the measures and their size of this line and all the following.
     * @param forceAll if true all the sheet lines are resized, otherwise it continues only
     * until the last measure is the same as before the resize.
     */
    void ResizeAll(bool forceAll = false);

    /**
     * Draws this line.
     *
     * @param pawsLine the pawsSheetLine that store the font-dependent parameters.
     * @param selectedChord the current selected chord.
     * @param startPosition the horizontal position where the first measure must
     * be drawn.
     * @param height the height of the staff.
     */
    void Draw(pawsSheetLine* pawsLine, Chord* selectedChord, int startPosition, int height);

    /**
     * Check if a chord in this line is hit and return the chord and its measure.
     *
     * @param x the horizontal position of the input from the beginning of the line.
     * @param chord at the end this pointer will contain the hit chord or null
     * if any chord has been hit.
     * @param measure at the end this pointer will contain the measure containing
     * the hit chord or a null pointer if nothing is hit.
     * @param before at the end this boolean will be true if the click has happened
     * before the selected chord, false if after.
     * @return true if a note has been hit, false otherwise.
     */
    bool Hit(int x, Chord* &chord, Measure* &measure, bool &before);

    /**
     * This function pushes a new empty chord at the end of the line and it returns.
     * It should be called only to move the final cursor forward. If the line is
     * completed, it creates a new line.
     *
     * @param divisions the number of sixteenths per measure.
     * @return the new chord.
     */
    Chord* Advance(uint divisions);

    /**
     * Sets the callback function that will be called when a new sheet line is added
     * or deleted.
     *
     * @param object the callback object.
     * @param function the callback function.
     */
    void SetCallback(void* object, void (*function)(void*, SheetLine*));

private:
    SheetLine* prev;                                  ///< The previous SheetLine.
    SheetLine* next;                                  ///< The next SheetLine.

    uint size;                                        ///< The size of this SheetLine.
    uint noteLength;                                  ///< The width of notes.
    Measure* firstMeasure;                            ///< The first measure of this SheetLine.
    Measure* lastMeasure;                             ///< The last measure of this SheetLine.

    bool hasCallback;                                 ///< True if there is a Callback set, false otherwise.
    void* callbackObject;                             ///< Callback object.
    void (*callbackFunction)(void*, SheetLine*);      ///< Callback function.

    /**
     * Calls the callback function.
     */
    void Callback();
};

//--------------------------------------------------

/**
 * This class draws a musical staff on the widget and creates notes and
 * chords that it can represent on the staff. At the moment it supports
 * only the font Sonora.
 */
class pawsSheetLine: public pawsWidget
{
public:
    /**
     * Constructor.
     */
    pawsSheetLine();

    /**
     * Destructor.
     */
    virtual ~pawsSheetLine();

    /**
     * Gets the sheet line drawn by this widget.
     * @return the sheet line drawn by this widget.
     */
    SheetLine* GetLine() { return line; }

    /**
     * Sets the line to draw on the staff and update its size.
     * @param sheetLine the line to be drawn.
     * @param resize true to resize the given line and its following, false to not resize.
     */
    void SetLine(SheetLine* sheetLine, bool resize = true);

    /**
     * Gets the staff status (double or single).
     * @return true if staff is double, false if it is single.
     */
    bool GetDoubleStaff() { return isDouble; }

    /**
     * Sets the staff to be double or single.
     * @param doubleStaff true to set double staff, false to set single.
     */
    void SetDoubleStaff(bool doubleStaff);

    /**
     * Draw the double line for the end of the sheet or for a repeat.
     *
     * @param x the ending horizontal position of the thick line (the second one).
     * @param y the starting vertical position.
     * @param height the height of the double line.
     */
    void DrawDoubleLine(int x, int y, int height);


    // From pawsWidget
    //-----------------
    virtual void Draw();
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool PostSetup();

private:
    friend class Chord;
    friend class Measure;
    friend class SheetLine;

    bool isDouble;              ///< True if it's a double staff, false if single.
    SheetLine* line;            ///< The line to draw.

    // positioning parameters
    int staffMarginUp;          ///< Vertical margin before the staff.
    int staffMarginLateral;     ///< Horizontal margin before the staff.

    int cleffCharLength;        ///< Width of the font's cleff char.
    int alterCharLength;        ///< Width of the font's alteration chars.
    int metricCharLength;       ///< Width of the font's big number char.
    int noteCharLength;         ///< Width of the font's note char.
    int doubleLineWidth;        ///< Width of the final vertical double line.
    int ledgerLineShift;        ///< Space between the start of the ledger line and the beginning of the note.

    int noteCharHeight;         ///< Height of the font's note char.
    float staffRowHeight;       ///< Height of a staff's row.
    int centralCPos;            ///< The position of the central C with this font.

    /**
     * Decode the char into the symbol format.
     * @param symbol the char to decode.
     * @return the decoded character.
     */
    static wchar_t GetSymbolChar(char symbol);

    /**
     * Gets the font's small number.
     * @param number the char representing the digit to decode.
     * @return the font's character of the small version of the given number or the
     * null character if the given char is not a number.
     */
    static wchar_t GetSmallNumber(char number);

    /**
     * Sets the parameters used to draw depending on the font and its size.
     * At the moment it supports only the font Sonora.
     */
    void SetDrawParameters();
};

CREATE_PAWS_FACTORY(pawsSheetLine);

/** @} */

#endif // PAWS_SHEET_LINE_H
