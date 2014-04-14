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

#ifndef EEDIT_SELECT_EDITANCHOR_HEADER
#define EEDIT_SELECT_EDITANCHOR_HEADER

#include "eeditinputboxmanager.h"
#include "paws/pawswidget.h"

class pawsButton;
class pawsEditTextBox;

/**
 * \addtogroup eedit
 * @{ */

/** A dialog window to edit effect anchor.
 */
class EEditSelectEditAnchor : public pawsWidget, public scfImplementation0<EEditSelectEditAnchor>
{
public:
    EEditSelectEditAnchor();
    virtual ~EEditSelectEditAnchor();

    /**
     * Pops up the select dialog.
     *
     * @param currName the name of the existing anchor.
     * @param callback a pointer to the callback that should be called on selection.
     * @param pos Selected position.
     */
    void Select(const csString & currName, EEditInputboxManager::iSelectEditAnchor * callback, const csVector2 & pos);
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    
private:
    pawsEditTextBox * newName;
    pawsButton      * changeName;
    pawsButton      * deleteAnchor;

    EEditInputboxManager::iSelectEditAnchor * selectCallback;
};

CREATE_PAWS_FACTORY(EEditSelectEditAnchor);

/** @} */

#endif

