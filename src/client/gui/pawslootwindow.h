/*
 * pawslootwindow.h - Author: Keith Fulton
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

#ifndef PAWS_LOOT_WINDOW_HEADER
#define PAWS_LOOT_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"

/// Enum of the columns for the listbox:
enum
{
    LCOL_ICON = 0,
    LCOL_NAME = 1,
    LCOL_ID   = 2
};


/**
 * Window contains a list of the available loot items.
 * with options to take them, roll for them or cancel the window.
 *
 * NOTE: the current expected columns for the listbox are
 *     as follows:
 *
 *    1) icon
 *    2) item name
 */
class pawsLootWindow : public pawsWidget, public psCmdBase
{
public:
    /// Constructor
    pawsLootWindow();

    /// Virtual destructor
    virtual ~pawsLootWindow();

    /// Handles petition server messages
    void HandleMessage( MsgEntry* message );

    /// Handles commands
    const char* HandleCommand(const char* cmd);

    /// Setup the widget with command/message handling capabilities
    bool PostSetup();

    /// Handle button clicks
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);

    EID GetLootingActor() { return lootEntity; }

protected:

    /// Quicker way to set text for each column in the listbox:
    void SetText(int rowNum, int colNum, const char* fmt, ...);

    /// Function to add the petitions to the listbox given a csArray;
    void AddLootItem();

    /// List widget of petitions for easy access
    pawsListBox* lootList;

    EID lootEntity;
};

/** The pawsLootWindow factory
 */
CREATE_PAWS_FACTORY( pawsLootWindow );

#endif
