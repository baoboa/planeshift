/*
 * pawsselector.h - Author: Andrew Craig
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
#ifndef PAWS_SELECTOR_BOX_HEADER
#define PAWS_SELECTOR_BOX_HEADER

#include "pawswidget.h"

////////////////////////////////////////////////////////////////////////////////
//  FORWARD DECLARATIONS
////////////////////////////////////////////////////////////////////////////////
class pawsListBox;
class pawsListBoxRow;
class pawsButton;


/**
 * \addtogroup common_paws
 * @{ */

////////////////////////////////////////////////////////////////////////////////
#define SELECTOR_ADD_BUTTON    -100
#define SELECTOR_REMOVE_BUTTON -200

////////////////////////////////////////////////////////////////////////////////
//  WIDGET IDENTIFIERS
////////////////////////////////////////////////////////////////////////////////
#define AVAILABLE_BOX -1000
#define SELECTED_BOX  -2000
////////////////////////////////////////////////////////////////////////////////


/**
 * This a available->selected widget.
 *
 * This is a fairly complex widget set.  It is used for when you have a list of
 * available options in one list box and a list of selected items in a second box
 * and arrows to add/remove them from one to the other.
 *
 * This widget assumes that the left side is the available ones and the right side
 * is the selected ones.
 *
 * Widget is defined as:
 *
 *    \<widget name="TEST" factory="pawsSelectorBox" \>
 *          \<frame x="10" y="10" width="500" height="400" border="yes" /\>
 *          \<available width="200" rowheight="30" /\>
 *          \<selected width="200" rowheight="30" /\>
 *     \</widget\>
 */
class pawsSelectorBox : public pawsWidget
{
public:
    pawsSelectorBox();
    pawsSelectorBox(const pawsSelectorBox &origin);
    ~pawsSelectorBox();

    bool Setup(iDocumentNode* node);
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    void OnListAction(pawsListBox* widget, int status);

    /// Create a new entry in the available box.
    pawsListBoxRow* CreateOption();

    /// Get the row that was moved from one list to the other.
    pawsListBoxRow* GetMoved()
    {
        return moved;
    }

    /// Remove the row with id from available list
    void RemoveFromAvailable(int id);

    /// Remove the row with id from selected list
    void RemoveFromSelected(int id);

    /// Return number of items in available list
    int GetAvailableCount(void);

    /// Move row automatically, default available->selected
    bool SelectAndMoveRow(int rowNo, bool toSelected=true);

private:
    pawsListBox* available;
    pawsListBox* selected;

    pawsButton* add;
    pawsButton* remove;

    pawsListBoxRow* moved;
};

CREATE_PAWS_FACTORY(pawsSelectorBox);


/** @} */

#endif
