/*
 * pawscombobox.h - Author: Andrew Craig
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

#ifndef PAWS_COMBO_BOX_HEADER
#define PAWS_COMBO_BOX_HEADER

#include "pawswidget.h"
#include "pawslistbox.h"

class pawsButton;
class pawsTextBox;
class pawsListBox;
class pawsListBoxRow;

/**
 * \addtogroup common_paws
 * @{ */

/** A basic combo box widget.
    This widget consists of a textbox and a drop down list box. When an
    item from the list box is selected the textbox is updated.

    This is the format of how it is defined in a widget file:

    \<widget name="comboBox" factory="pawsComboBox"\>
            \<!-- Size of text box with the current choice --\>
            \<frame x="100" y="100" width="100" height="30" border="no" /\>

            \<!-- Number of list box choices with height of a choice row --\>
            \<listbox rows="5" height="30" text="InitialText" /\>
    \</widget\>

    This class sets the listbox with the id of the combo widget so if you want
    to track messages from this you override the Selected( pawsListBox* widget )
    funtion in your controlling widget and have a case for this widget's ID number.
*/
class pawsComboBox : public pawsWidget
{
public:
    pawsComboBox();
    ~pawsComboBox();
    pawsComboBox(const pawsComboBox &origin);

    bool Setup(iDocumentNode* node);
    bool PostSetup();
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    void OnListAction(pawsListBox* widget, int status);

    void SetNumRows(int numRows)
    {
        rows      = numRows;
    }
    void SetRowHeight(int height)
    {
        rowHeight = height;
    }
    void SetUpButtonImage(const char* name)
    {
        upButton = name;
    }
    void SetUpDownButtonImage(const char* name)
    {
        downButton = name;
    }
    void SetUpButtonPressedImage(const char* name)
    {
        upButtonPressed = name;
    }
    void SetUpDownButtonPressedImage(const char* name)
    {
        downButtonPressed = name;
    }

    ///sets if the list has to be sorted. NOTE: call before adding entries.
    void SetSorted(bool sorting);
    pawsListBoxRow* NewOption();
    pawsListBoxRow* NewOption(const csString &text);

    /** @@@ Hack: please someone tell me how to do this better? */
    pawsListBox* GetChoiceList()
    {
        return listChoice;
    }

    /** Returns index of selected option (-1 if none is selected */
    int GetSelectedRowNum();

    /** Returns string of selected option */
    csString GetSelectedRowString();

    /** Selects given option in combo */
    pawsListBoxRow* Select(int optionNum);
    pawsListBoxRow* Select(const char* text);

    int GetRowCount();
    bool Clear();

private:
    pawsTextBox* itemChoice;
    pawsListBox* listChoice;
    pawsButton* arrow;
    csString initalText;

    int oldHeight;
    int oldWidth;
    bool closed;
    bool fliptotop;
    bool useScrollBar;

    int rows;
    int rowHeight;
    int listalpha;
    bool sorted;

    csString text, upButton, upButtonPressed, downButton, downButtonPressed;
};

CREATE_PAWS_FACTORY(pawsComboBox);

/** @} */

#endif
