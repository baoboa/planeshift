/*
 * pawsconfigpopup.h
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_POPUP
#define PAWS_CONFIG_POPUP

//==============================================================================
// CS INCLUDES
//==============================================================================
#include <csutil/array.h>
#include <iutil/document.h>

//==============================================================================
// PAWS INCLUDES
//==============================================================================
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"

class pawsCheckBox;
class pawsActiveMagicWindow;
class pawsNpcDialogWindow;

class pawsConfigPopup : public pawsConfigSectionWindow
{
public:
    pawsConfigPopup(void); ///< Constructor

    virtual bool PostSetup(); ///< @see pawsWidget

    virtual bool Initialize(); ///< @see pawsWidget
    virtual bool LoadConfig(); ///< @see pawsWidget
    virtual bool SaveConfig(); ///< @see pawsWidget
    virtual void SetDefault(); ///< @see pawsWidget

    /** Check-box which gives the user a opportunity to show or
     *  not to show the Active Magic Window
     */
    pawsCheckBox* showActiveMagicConfig;
    /** Check-box which gives the user a opportunity to use or
     *  not the bubble based npcdialog interface
     */
    pawsCheckBox* useNpcDialogBubbles;

protected:
    pawsActiveMagicWindow* magicWindow; ///< This is used to point to a instance of magic window
    pawsNpcDialogWindow* npcDialog; ///< This is used to point to a instance of npcdialog
    psMainWidget* mainWidget; ///< This is used to point to the main widget
};

CREATE_PAWS_FACTORY(pawsConfigPopup);

#endif
