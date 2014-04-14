/*
 * pawsConfigChatLogs.h - Author: Stefano Angeleri
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

#ifndef PAWS_CONFIG_CHAT_LOGS_HEADER
#define PAWS_CONFIG_CHAT_LOGS_HEADER

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "chatwindow.h"


class pawsCheckBox;

class pawsConfigChatLogs : public pawsConfigSectionWindow
{
public:
    pawsConfigChatLogs(); ///< Constructor
    void drawFrame();

    virtual bool PostSetup(); ///< @see pawsWidget

    virtual bool Initialize();///< @see pawsConfigSectionWindow
    virtual bool LoadConfig();///< @see pawsConfigSectionWindow
    virtual bool SaveConfig();///< @see pawsConfigSectionWindow
    virtual void SetDefault();///< @see pawsConfigSectionWindow

     virtual void OnListAction( pawsListBox* selected, int status ); ///< @see pawsWidget

    /// @see pawsWidget
    virtual bool OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
    {
         dirty = true;
         return true;
    }

private:
       pawsCheckBox *enabled;         ///< Checkbox which mantains if the selected chat type logging is enabled.
       pawsListBox *chatTypes;        ///< Listbox which mantains a list of all chat types available.
       pawsEditTextBox *fileName;     ///< Textbix used to edit the log file for the selected chat type.
       pawsEditTextBox *bracket;      ///< Textbix used to edit the brackets for the selected chat type.
       bool logStatus[CHAT_END];      ///< Used to temporarily store the enabled status of a chat type logging.
       csString logFile[CHAT_END];    ///< Used to temporarily store the log file names for the various chat types.
       csString logBracket[CHAT_END]; ///< Used to temporarily store the brackets for the various chat types.
       int currentType;               ///< Stores the last selected chat type.


};

CREATE_PAWS_FACTORY( pawsConfigChatLogs );

#endif //PAWS_CONFIG_CHAT_LOGS_HEADER
