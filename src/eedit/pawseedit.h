/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_EEDIT_HEADER
#define PAWS_EEDIT_HEADER

#include "paws/pawswidget.h"

struct iView;

class pawsEditTextBox;
class pawsButton;
class pawsGenericView;

/**
 * \addtogroup eedit
 * @{ */

class pawsEEdit : public pawsWidget
{
public:
    pawsEEdit();
    ~pawsEEdit();

    /** Returns the CS viewport of the preview window
     *   @return the iView viewport
     */
    iView *GetView();

    /** Returns the file path of the loaded map.
     *   \return    The file path of the loaded map.
     */
    const char * GetMapFile() const;

    /** Loads the given map.
     *   \param mapFile Path to the map to load.
     *   \return        True on success.
     */
    bool LoadMap(const char * mapFile);

    // implemented virtual functions from pawsWidgets
    bool PostSetup();
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

private:
    
    // widget refs
    pawsGenericView * preview;

    pawsButton      * showPosition;
};

CREATE_PAWS_FACTORY( pawsEEdit );

/** @} */

#endif
