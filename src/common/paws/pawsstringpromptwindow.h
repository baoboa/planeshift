/*
 * pawsstringpromptwindow.h - Author: Ondrej Hurt
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_STRING_PROMPT_WINDOW_HEADER
#define PAWS_STRING_PROMPT_WINDOW_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "pawspromptwindow.h"

class pawsButton;

/**
 * \addtogroup common_paws
 * @{ */

/**
 * This interface defines the callback used by pawsStringPromptWindow
 * to notify another window of a supplied answer.
 */
class iOnStringEnteredAction
{
public:
    /**
     * When the pawsStringPromptWindow is created, a ptr to a class
     * which implements this function is provided, and a "name" string
     * is provided, so that a single window can use 1 callback for
     * many fields.
     */
    virtual void OnStringEntered(const char* name, int param,const char* value) = 0;

    /** Basic deconstructor */
    virtual ~iOnStringEnteredAction() {};
};

/**
 * pawsStringPromptWindow is a window that lets the user enter a string
 */
class pawsStringPromptWindow : public pawsPromptWindow
{
public:
    /** Basic constructor */
    pawsStringPromptWindow();
    pawsStringPromptWindow(const pawsStringPromptWindow &origin);
    /**
     * Called when a button is released inside the window
     * Only relevant if widget is okButton or cancelButton
     * @param mouseButton The mouse button that was released
     * @param keyModifier Any modifiers that were present at release time
     * @param widget The widget which had focus when the button was released
     * @return True if widget == okButton or cancelButton, false otherwise
     */
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget);

    /**
     * Called when a key is pressed
     * If the enter key is pressed, the window is accepted and the action is called
     * @param keyCode Key code for the key pressed
     * @param keyChar Char code for the key pressed
     * @param modifiers Any modifiers that were present at press time
     * @return True if key is Enter, False otherwise
     */
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);

    /**
     * Called when a child widget changes
     * If the widget is the inputWidget and the max length is being tracked, then the max length is updated
     * @param widget The widget that changes
     * @return False
     */
    virtual bool OnChange(pawsWidget* widget);

    /**
     * Entry point to pawsStringPromptWindow
     * Call to create a window
     * @param label Copied to Initialize
     * @param string Copied to Initialize
     * @param multiline Copied to Initialize
     * @param width Copied to Initialize
     * @param height Copied to Initialize
     * @param action Copied to Initialize
     * @param name Copied to Initialize
     * @param param Copied to Initialize
     * @param modal Whether the widget should be modal
     * @param maxlen Copied to Initialize
     * @return The widget that is created
     */
    static pawsStringPromptWindow* Create(
        const csString &label,
        const csString &string, bool multiline, int width, int height,
        iOnStringEnteredAction* action,const char* name,int param = 0,
        bool modal = false, int maxlen = 0);

protected:
    /**
     * Initializes the window and sets up all of the appropriate widgets
     * @param label Label to use as a title
     * @param string Default string in the string prompt
     * @param multiline Whether the input supports multi-line
     * @param width Width of the window to show
     * @param height Height of the window to show
     * @param action The action to call when the prompt window is closed successfully
     * @param name The name of the string that is being prompted - passed to the action
     * @param param An optional parameter (not visible to user) - passed to the action
     * @param maxlen The maximum length of the string that is accepted
     */
    void Initialize(const csString &label, const csString &string, bool multiline, int width,
                    int height, iOnStringEnteredAction* action, const char* name, int param = 0, int maxlen = 0);

    /**
     * Executes action and destroys window.
     * Sends text, the prestored param and this widget's name to the action.
     * @param text The text to send to the action
     */
    void CloseWindow(const csString &text);

    /**
     * Executes action with text entered by user as parameter and destroys window
     */
    void CloseWindow();

    /**
     * Whether the prompt is multi-line or single
     */
    bool multiLine;
    /**
     * The action to call when this window is successfully destroyed
     */
    iOnStringEnteredAction* action;
    /**
     * The name of the window; is sent to the action when OK is pressed
     */
    csString name;
    /**
     * The optional parameter for the window; is sent to the action when OK is pressed
     */
    int param;
};

CREATE_PAWS_FACTORY(pawsStringPromptWindow);

/** @} */

#endif

