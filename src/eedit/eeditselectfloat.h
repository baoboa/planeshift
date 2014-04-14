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

#ifndef EEDIT_SELECT_FLOAT_HEADER
#define EEDIT_SELECT_FLOAT_HEADER

#include "eeditinputboxmanager.h"
#include "paws/pawswidget.h"

class pawsButton;
class pawsSpinBox;

/**
 * \addtogroup eedit
 * @{ */

/** A dialog window to select a floating point value.
 */
class EEditSelectFloat : public pawsWidget, public scfImplementation0<EEditSelectFloat>
{
public:
    EEditSelectFloat();
    virtual ~EEditSelectFloat();

    /**
     * Pops up the select float dialog.
     *
     * @param startValue the value that first appears in the dialog box.
     * @param callback a pointer to the callback that should be called on selection.
     * @param pos Selected position.
     */
    void Select(float startValue, EEditInputboxManager::iSelectFloat * callback, const csVector2 & pos);
    
    // inheritted from pawsWidget
    virtual bool PostSetup(); 
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    
private:
    
    pawsSpinBox * value;
    pawsButton  * ok;

    EEditInputboxManager::iSelectFloat * selectCallback;
};

CREATE_PAWS_FACTORY(EEditSelectFloat);

/** @} */

#endif 
