/*
 * pawsmusicwindow.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef PAWS_MUSIC_WINDOW_H
#define PAWS_MUSIC_WINDOW_H


//====================================================================================
// Project Includes
//====================================================================================
#include <clientsongmngr.h>
#include <net/cmdbase.h>
#include <paws/pawsmeterpromptwindow.h>
#include <paws/pawsstringpromptwindow.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class Chord;
class Measure;
class SheetLine;
class pawsSheetLine;

/**
 * A window that shows a musical sheet and allows the player to edit it.
 */
class pawsMusicWindow: public pawsWidget, public psClientNetSubscriber,
    public iOnStringEnteredAction, public iOnMeterEnteredListener, iSongManagerListener
{
public:
    enum
    {
        UNALTERED,
        FLAT,
        NATURAL,
        SHARP
    };

    enum
    {
        NO_SELECTED,

        // duration buttons
        SIXTEENTH_BUTTON = 200,
        EIGHTH_BUTTON,
        QUARTER_BUTTON,
        HALF_BUTTON,
        WHOLE_BUTTON,

        // alteration buttons
        FLAT_BUTTON = 300,
        NATURAL_BUTTON,
        SHARP_BUTTON,
    };

    /**
     * Constructor.
     */
    pawsMusicWindow();

    /**
     * Copy constructor.
     *
     * @param origin the pawsMusicWindow to copy.
     */
    pawsMusicWindow(const pawsMusicWindow* origin);

    /**
     * Destructor.
     */
    virtual ~pawsMusicWindow();

    /**
     * Handle a click on a chord.
     *
     * @param sheetLine the selected line.
     * @param measure the selected measure.
     * @param chord the selected chord.
     * @param notePosition the clicked note.
     * @param before true if the click has happened before the selected chord, false if after.
     */
    void OnChordSelection(SheetLine* sheetLine, Measure* measure, Chord* chord, int notePosition, bool before);

    /**
     * Gets beats and beat type of the song.
     *
     * @param beats a reference to the integer that will contain beats.
     * @param beatType a reference to the integer that will contain the beat type.
     */
    void GetMeter(int &beats, int &beatType);

    /**
     * Gets the sheet's number of divisions.
     *
     * @return the number of divisions in sixteenths.
     */
    uint GetDivisions();

    // From iSongManagerListener
    virtual void OnMainPlayerSongStop();

    // From iOnMeterEnteredListener
    virtual void OnMeterEntered(const char* name, csString beats, csString beatType);

    // From iOnStringEnteredAction
    //-----------------------------
    virtual void OnStringEntered(const char* name, int param, const char* value);

    // From psClientNetSubscriber
    //----------------------------
    virtual void HandleMessage(MsgEntry* message);

    // From pawsWidget
    //-----------------
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    virtual void Hide();
    virtual bool PostSetup();
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnScroll(int scrollDirection, pawsScrollBar* widget);

private:
    friend class pawsSheetLine;

    int fifths;                        ///< The tonality of the song.
    csString beats;                    ///< Numerator of the song's meter.
    csString beatType;                 ///< Denominator of the song's meter.
    uint tempo;                        ///< BPM suggested on the score.
    uint playedTempo;                  ///< BPM when playing the song.

    SheetLine* linesHead;              ///< All the lines of the sheet.
    Measure* measuresHead;             ///< All the measures of the sheet.

    Chord* selectedChord;              ///< The current selected chord.
    Measure* selectedMeasure;          ///< The current selected measure.

    /**
     * The selected line. This can be out-of-date if a resize moved the selected measure
     * into another SheetLine. It is more a cache element. Don't access directly to this
     * variable when you need it. Use GetSelectedSheetLine().
     */
    SheetLine* selectedSheetLine;

    int  selectedDurButton;            ///< The selected duration button's ID (whole, half, etc.)
    int  selectedAlterButton;          ///< The selected alteration button's ID (unaltered, flat, etc.).

    uint32_t currentItemID;            ///< The ID of the represented creative item.
    bool readOnly;                     ///< True if the player does not have the right to edit the sheet.
    bool edited;                       ///< True if the player has edited the sheet.
    bool pendingString;                ///< True if there is a pending string.
    bool editMode;                     ///< True if the edit mode is active.
    bool insertMode;                   ///< True if the insert mode is active.

    pawsButton* editButton;
    pawsButton* playButton;
    pawsWidget* loadButton;
    pawsWidget* saveButton;
    pawsWidget* titleButton;

    pawsButton* doubleStaffButton;
    pawsWidget* tonalityButton;
    pawsWidget* meterButton;
    pawsWidget* bpmButton;
    pawsButton* restButton;

    pawsButton* sixteenthButton;
    pawsButton* eighthButton;
    pawsButton* quarterButton;
    pawsButton* halfButton;
    pawsButton* wholeButton;
    pawsButton* dotButton;

    pawsButton* flatButton;
    // pawsButton* naturalButton; TODO support natural notes in song stream before use this
    pawsButton* sharpButton;

    pawsWidget* deleteChordButton;
    pawsWidget* deleteMeasureButton;
    pawsButton* insertNoteButton;
    pawsWidget* insertMeasureButton;

    pawsButton* startRepeatButton;
    pawsButton* endRepeatButton;
    pawsButton* endingButton;

    pawsSheetLine* pawsLine1;
    pawsSheetLine* pawsLine2;

    pawsScrollBar* scrollBar;

    /**
     * Callback of SheetLine. Update the scroll bar and keep the selected
     * SheetLine valid.
     */
    static void OnSheetLineCallback(void* object, SheetLine* sheetLine);

    /**
     * Unloads the previous musical sheet.
     */
    void Unload();

    /**
     * Loads an XML musical sheet and unload the previous one. This does
     * not unload the previous sheet if the new one is not legal and the
     * return value is false.
     *
     * @param sheet the the musical sheet.
     * @return true if the sheet could be load, false otherwise.
     */
    bool LoadXML(csRef<iDocument> sheet);

    /**
     * Converts the musical sheet into an XML document.
     *
     * @param usePlayedTempo true if the XML score should contain the
     * played tempo instead of the one suggested on the sheet.
     * @return the XML document.
     */
    csString ToXML(bool usePlayedTempo);

    /**
     * Returns the index-th SheetLine.
     *
     * @param index the number of the line, < 0 to get the last one.
     * @return the last SheetLine, a null pointer if there are any.
     */
    SheetLine* GetSheetLine(int index);

    /**
     * Returns the SheetLine that contain the selected measure.
     *
     * @return the SheetLine that contain the selected measure.
     */
    SheetLine* GetSelectedSheetLine();

    /**
     * Update the tick value and current position of the scroll bar.
     */
    void UpdateScrollBar();

    /**
     * Resize all the SheetLines from the given one display the selected chord.
     *
     * @param line the first sheet line to resize.
     * @param forceAll true to force the resizing of all the the following lines.
     */
    void UpdateLines(SheetLine* line, bool forceAll);

    /**
     * Displays line containing the selected measure.
     */
    void DisplaySelectedMeasure();

    /**
     * Returns a pointer to the button with the given ID. The button can be
     * a duration button or an alteration button.
     *
     * @param buttonID the ID of the needed button.
     * @return the pawsButton.
     */
    pawsButton* GetButton(int buttonID);

    /**
     * Delete the selected chord.
     */
    void DeleteSelectedChord();

    /**
     * Delete the selected measure.
     */
    void DeleteSelectedMeasure();

    /**
     * Inserts a new empty measure before the selected one.
     */
    void InsertMeasure();

    /**
     * Turn on or off the insert mode.
     *
     * @param toggle true to activate the insert mode, false to deactivate it.
     */
    void SetInsertMode(bool toggle);

    /**
     * Toggle the edit mode.
     *
     * @param toggle true to active the edit mode, false to deactive.
     */
    void ToggleEditMode(bool toggle);

    /**
     * Plays/stops the song.
     */
    void PlayStop();

    /**
     * Saves the musical sheet into an XML format.
     */
    void Save();

    /**
     * Allows the users to Load a musical sheet from a file saved in the hard disk.
     */
    void Load();

    /**
     * Creates a prompt window where the user can change the song's title.
     */
    void ChangeSongTitle();

    /**
     * Switch between the single and the double staff.
     */
    void SwitchDoubleStaff();

    /**
     * Creates a prompt window where the user can change the tonality.
     */
    void ChangeTonality();

    /**
     * Creates a prompt window where the user can change the meter.
     */
    void ChangeMeter();

    /**
     * Creates a prompt window where the user can change the tempo.
     */
    void ChangeBPM();

    /**
     * Creates a prompt window where the user can set the played tempo.
     */
    void SetPlayedBPM();

    /**
     * Changes the selected note duration according to the selected duration buttons.
     */
    void ChangeChordDuration();

    /**
     * Creates a prompt window where the user can set the number of times that the
     * selected measure must be repeated.
     */
    void SetEndRepeat();

    /**
     * Sets the ending for the selected measure and updates the buttons state.
     */
    void SetEnding();

    /**
     * Returns the duration associated to the selected duration buttons.
     *
     * @return the selected duration.
     */
    int GetSelectedDuration();

    /**
     * Sets the duration buttons and the dot button according to the given
     * duration value.
     * 
     * @param duration the duration value.
     */
    void SetDurationButton(int duration);

    /**
     * Selects the given duration button and deselects the previous one.
     *
     * @param buttonID the ID of the new selected duration button.
     */
    void SelectDuration(int buttonID);

    /**
     * Selects the given alteration button and deselects the previous one if
     * any. Use NO_SELECTED to deselect all.
     *
     * @param buttonID the ID of the new selected alteration button.
     */
    void SelectAlteration(int buttonID);

    /**
     * Shows the toolbar's buttons if the edit mode is active or hides them if
     * it is not active.
     */
    void SetToolbarButtons();

    /**
     * Resets the windows state. It does not delete lines and measures, it should
     * be done before calling this method.
     */
    void ResetState();

    /**
     * Deletes all measures and lines of the sheet.
     */
    void DeleteAll();
};

CREATE_PAWS_FACTORY(pawsMusicWindow);

#endif // PAWS_MUSIC_WINDOW_H
