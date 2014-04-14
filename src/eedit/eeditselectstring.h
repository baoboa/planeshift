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

#ifndef EEDIT_SELECT_STRING_HEADER
#define EEDIT_SELECT_STRING_HEADER

#include "eeditinputboxmanager.h"
#include "paws/pawswidget.h"

class pawsButton;
class pawsEditTextBox;

/**
 * \addtogroup eedit
 * @{ */

/** A dialog window to select a string value.
 */
class EEditSelectString : public pawsWidget, public scfImplementation0<EEditSelectString>
{
public:
    EEditSelectString();
    virtual ~EEditSelectString();

    /**
     * Pops up the select dialog.
     *
     * @param startValue the value that first appears in the dialog box.
     * @param callback a pointer to the callback that should be called on selection.
     * @param pos Selected position.
     */
    void Select(const csString & startValue, EEditInputboxManager::iSelectString * callback, const csVector2 & pos);
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    
private:
    
    pawsEditTextBox * value;
    pawsButton  * ok;

    EEditInputboxManager::iSelectString * selectCallback;
};

CREATE_PAWS_FACTORY(EEditSelectString);

/** @} */

#endif 
