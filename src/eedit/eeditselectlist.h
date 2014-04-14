/*
 * Author: Andrew Robberts
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

#ifndef EEDIT_SELECT_LIST_HEADER
#define EEDIT_SELECT_LIST_HEADER

#include "eeditinputboxmanager.h"
#include "paws/pawswidget.h"

class pawsButton;
class pawsListBox;

/**
 * \addtogroup eedit
 * @{ */

/** A dialog window to select from a list of values.
 */
class EEditSelectList : public pawsWidget, public scfImplementation0<EEditSelectList>
{
public:
    EEditSelectList();
    virtual ~EEditSelectList();

    /**
     * Pops up the select list dialog.
     *
     * @param list the list of possible values.
     * @param listCount the number of possible values.
     * @param defaultValue the default value.
     * @param callback a pointer to the callback that should be called on selection.
     * @param pos Selected position.
     */
    void Select(csString * list, size_t listCount, const csString & defaultValue, EEditInputboxManager::iSelectList * callback, const csVector2 & pos);
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual void OnListAction(pawsListBox * selected, int status);
    
private:
    
    pawsListBox * value;
    pawsButton  * ok;

    EEditInputboxManager::iSelectList * selectCallback;
};

CREATE_PAWS_FACTORY(EEditSelectList);

/** @} */

#endif 
