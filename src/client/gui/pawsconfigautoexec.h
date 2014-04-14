/*
 * pawsconfigautoexec.h - Author: Fabian Stock (Aiwendil)
 *
 * Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_AUTOEXEC_HEADER
#define PAWS_CONFIG_AUTOEXEC_HEADER

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"

class Autoexec;
class pawsCheckBox;

class pawsConfigAutoexec : public pawsConfigSectionWindow
{
public:
    pawsConfigAutoexec(); ///< Constructor
    void drawFrame();

    virtual bool PostSetup(); ///< @see pawsWidget

    virtual bool Initialize();///< @see pawsConfigSectionWindow
    virtual bool LoadConfig();///< @see pawsConfigSectionWindow
    virtual bool SaveConfig();///< @see pawsConfigSectionWindow
    virtual void SetDefault();///< @see pawsConfigSectionWindow

    bool OnChange(pawsWidget* /*widget*/) { dirty = true; return true; } ///< @see pawsWidget
    /// @see pawsWidget
    virtual bool OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
    {
         dirty = true;
         return true;
    }
    virtual void OnListAction( pawsListBox* selected, int status ); ///< @see pawsWidget

private:
       Autoexec *autoexec; ///< Pointer to the autoexec handler

       pawsCheckBox *enabled; ///< Checkbox which mantains if the autoexec function is enabled
       pawsMultilineEditTextBox* generalCommandBox; ///< Textbox with all the commands to execute for all characters
       pawsMultilineEditTextBox* charCommandBox;    ///< Textbox with all the commands to execute only for the current character
};

CREATE_PAWS_FACTORY( pawsConfigAutoexec );

#endif //PAWS_CONFIG_AUTOEXEC_HEADER
