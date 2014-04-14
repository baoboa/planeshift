/*
 * pawsnumberpromptwindow.h - Author: Ondrej Hurt
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

#ifndef PAWS_NUMBER_PROMPT_WINDOW_HEADER
#define PAWS_NUMBER_PROMPT_WINDOW_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "pawspromptwindow.h"

class pawsButton;
class pawsEditTextBox;
class pawsScrollbar;

/**
 * \addtogroup common_paws
 * @{ */

class iOnNumberEnteredAction
{
public:
    virtual void OnNumberEntered(const char* name,int param,int number) = 0;
    // can be -1
    virtual ~iOnNumberEnteredAction() {};
};

/**
 * pawsNumberPromptWindow is window that lets the user enter a number
 */
class pawsNumberPromptWindow : public pawsPromptWindow
{
public:
    pawsNumberPromptWindow();
    pawsNumberPromptWindow(const pawsNumberPromptWindow &origin);
    //from pawsWidget:
    virtual bool PostSetup();
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnScroll(int scrollDirection, pawsScrollBar* widget);
    virtual bool OnChange(pawsWidget* widget);
    virtual void Close();

    void Initialize(const csString &label, int number, int minNumber, int maxNumber,
                    iOnNumberEnteredAction* action,const char* name, int param=0);

    static pawsNumberPromptWindow* Create(const csString &label,
                                          int number, int minNumber, int maxNumber,
                                          iOnNumberEnteredAction* action,const char* name, int param=0);


protected:
    void SetBoundaries(int minNumber, int maxNumber);

    void LayoutWindow();

    /** Will we allow user to enter 'text' during editation ? */
    bool TextIsValidForEditing(const csString &text);

    /** Will we allow user to return 'text' as the final result ?
        The difference is that TextIsValidForOutput() won't allow zero and empty string */
    bool TextIsValidForOutput(const csString &text);

    /** This is called when the user enters final count (which is valid) */
    void NumberWasEntered(int count);

    int maxNumber, minNumber;
    int maxDigits;

    /** This is last valid input from user - we use it to fall back from invalid input */
    csString lastValidText;

    pawsEditTextBox* editBox;
    pawsScrollBar* scrollBar;

    iOnNumberEnteredAction* action;
    csString name;
    int param;
};

CREATE_PAWS_FACTORY(pawsNumberPromptWindow);

/** @} */

#endif

