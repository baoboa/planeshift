/*
 * pawsmeterpromptwindow.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef PAWS_MUSIC_METER_PROMPT_H
#define PAWS_MUSIC_METER_PROMPT_H


//====================================================================================
// Local Includes
//====================================================================================
#include "pawspromptwindow.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class pawsTextBox;
class pawsComboBox;
class pawsEditTextBox;

/**
 * \addtogroup common_paws
 * @{ */

/**
 * Interface implemented by a pawsMeterPromptWindow listener
 */
class iOnMeterEnteredListener
{
public:
    /**
     * This function is called when the user input a meter.
     * @param name the action's name.
     * @param beats the beats.
     * @param beatType the beat type.
     */
    virtual void OnMeterEntered(const char* name, csString beats, csString beatType) = 0;
};

//---------------------------------------------------------------------------------------------

/**
 * This is the main input widget of pawsMeterPromptWindow.
 */
class pawsMeterInput: public pawsWidget
{
public:
    /**
     * Constructor.
     */
    pawsMeterInput();

    /**
     * Copy constructor.
     * @param origin the pawsMeterInput to copy.
     */
    pawsMeterInput(const pawsMeterInput &origin);

    /**
     * Initialize pawsMeterInput with its starting values.
     * @param initialBeats the text that is visualized as beats when the window is opened.
     * @param initialBeatType the text that is visualized as beat type when the window is opened.
     * @param beatsMaxLength the maximum number of digits that the user can input for beats.
     */
    void Initialize(const char* initialBeats, const char* initialBeatType, size_t beatsMaxLength);

    /**
     * Gets the current beats.
     * @return the current beats.
     */
    const char* GetBeats();

    /**
     * Gets the current beat type.
     * @return the current beat type.
     */
    csString GetBeatType();

private:
    pawsTextBox* beatsLabel;        ///< Beats label.
    pawsTextBox* beatTypeLabel;     ///< Beat type label.
    pawsEditTextBox* beatsInput;    ///< Text box used to input beats.
    pawsComboBox* beatTypeInput;    ///< Combo box used to input the beat type.
};

CREATE_PAWS_FACTORY(pawsMeterInput);

//---------------------------------------------------------------------------------------------

/**
 *  This window let the user select the music meter for a score.
 */
class pawsMeterPromptWindow: public pawsPromptWindow
{
public:
    /**
     * Constructor.
     */
    pawsMeterPromptWindow();

    /**
     * Copy constructor.
     * @param origin the pawsMeterPromptWindow to copy.
     */
    pawsMeterPromptWindow(const pawsMeterPromptWindow &origin);

    /**
     * Initializes the window with its starting values.
     * @param actionName the name that will be used when OnMeterEntered is called on the listener.
     * @param title the displayed title of the window.
     * @param initialBeats the text that is visualized as beats when the window is opened.
     * @param initialBeatType the text that is visualized as beat type when the window is opened.
     * @param beatsMaxLength the maximum number of digits that the user can input for beats.
     * @param listener the iOnMeterEnteredListener that waits for the user's input.
     */
    void Initialize(const char* actionName, const char* title, const char* initialBeats,
                    const char* initialBeatType, size_t beatsMaxLength, iOnMeterEnteredListener* listener);

    // From pawsWidget
    //-----------------
    virtual bool PostSetup();
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

    /**
     * Creates a new pawsMeterPromptWindow.
     * @param actionName the name that will be used when OnMeterEntered is called on the listener.
     * @param initialBeats the text that is visualized as beats when the window is opened.
     * @param initialBeatType the text that is visualized as beat type when the window is opened.
     * @param beatsMaxLength the maximum number of digits that the user can input for beats.
     * @param listener the iOnMeterEnteredListener that waits for the user's input.
     * @return the new pawsMeterPromptWindow.
     */
    static pawsMeterPromptWindow* Create(const char* actionName, const char* initialBeats,
                                         const char* initialBeatType, size_t beatsMaxLength, iOnMeterEnteredListener* listener);

private:
    csString actionName;                        ///< The name of the action that will be passed to the listener.
    iOnMeterEnteredListener* meterListener;     ///< Listener for the input.
};

CREATE_PAWS_FACTORY(pawsMeterPromptWindow);

/** @} */

#endif // PAWS_MUSIC_METER_PROMPT_H
