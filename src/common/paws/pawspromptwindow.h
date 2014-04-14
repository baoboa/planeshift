/*
 * pawspromptwindow.h - Author: Ondrej Hurt
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

#ifndef PAWS_PROMPT_WINDOW_HEADER
#define PAWS_PROMPT_WINDOW_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "pawswidget.h"

class pawsButton;
class pawsTextBox;

/**
 * \addtogroup common_paws
 * @{ */

/**
 * pawsPromptWindow is the common base for windows that let the user enter one piece of information.
 * It has a label telling the user what is expected, OK and Cancel buttons and *some* widget
 * that is used to enter the information. Different widgets are used for different kinds of input.
 */

class pawsPromptWindow : public pawsWidget
{
public:
    /** Basic constructor */
    pawsPromptWindow();
    pawsPromptWindow(const pawsPromptWindow &origin);
    /**
     * Executed after the widget is setup
     * @return true
     */
    virtual bool PostSetup();

    /**
     * Sets the label/title for the widget
     * @param label The new value for the label/title
     */
    void SetLabel(const csString &label);

    /**
     * Sets the spacing in between the widgets
     * @param spacing The new amount of spacing in between each widget
     */
    void SetSpacing(int spacing);

protected:
    /** Sets good position on screen (=under mouse cursor) */
    void SetAppropriatePos();

    /** Sets positions of widgets and size of whole window */
    virtual void LayoutWindow();

    /** The label that is used as a title for the window */
    pawsTextBox* label;
    /** The button that is used to accept the input for the window */
    pawsButton* okButton;
    /** The button that is used to cancel the input for the window */
    pawsButton* cancelButton;
    /** A generic widget that simply helps describe or facilitate the inputWidget, does not have to be defined */
    pawsWidget* helperWidget;
    /** The primary widget for gathering input from the user */
    pawsWidget* inputWidget;
    /** The spacing in between each of the widgets */
    int spacing;
};

CREATE_PAWS_FACTORY(pawsPromptWindow);

/** @} */

#endif

