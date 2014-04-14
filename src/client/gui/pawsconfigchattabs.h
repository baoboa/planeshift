/*
* pawsconfigchattabs.h - Author: Enar Vaikene
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PAWSCONFIGCHATTABS_H
#define __PAWSCONFIGCHATTABS_H

#include "paws/pawswidget.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigwindow.h"
#include "chatwindow.h"


/// Configuration panel for chat window tabs.
class pawsConfigChatTabs : public pawsConfigSectionWindow
{

public:
    pawsConfigChatTabs();

    bool Initialize();
    bool LoadConfig();
    bool SaveConfig();
    void SetDefault();

    bool PostSetup();

private:

    pawsCheckBox *isysbase, *ichat, *inpc, *itells, *iguild, *ialliance, *igroup, *iauction, *isys, *ihelp;
    pawsCheckBox *issysbase, *ischat, *isnpc, *istells, *isguild, *isalliance, *isgroup, *isauction, *issys, *ishelp;
    /// Finds the checkbox widget by name and returns the widget or NULL if not found.
    pawsCheckBox *FindCheckbox(const char *name);

    pawsChatWindow* chatWindow;

};

CREATE_PAWS_FACTORY(pawsConfigChatTabs);

#endif

