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

#ifndef EEDIT_SELECT_YESNO_HEADER
#define EEDIT_SELECT_YESNO_HEADER

#include "eeditinputboxmanager.h"
#include "paws/pawswidget.h"

class pawsRadioButton;

/**
 * \addtogroup eedit
 * @{ */

/** A dialog window to select a yes or no value.
 */
class EEditSelectYesNo : public pawsWidget, public scfImplementation0<EEditSelectYesNo>
{
public:
    EEditSelectYesNo();
    virtual ~EEditSelectYesNo();

    /** Pops up the select dialog.
     *   @param startValue the value that first appears in the dialog box.
     *   @param callback a pointer to the callback that should be called on selection.
     *   @param pos the starting window position.
     */
    void Select(bool startValue, EEditInputboxManager::iSelectYesNo * callback, const csVector2 & pos);
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget);
    
private:
    
    pawsRadioButton * yes;
    pawsRadioButton * no;

    EEditInputboxManager::iSelectYesNo * selectCallback;
};

CREATE_PAWS_FACTORY(EEditSelectYesNo);

/** @} */

#endif 
