/*
 * pawsconfigchattabcompletion.h - Author: Fabian Stock (Aiwendil)
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_CHATTABCOMPLETION_HEADER
#define PAWS_CONFIG_CHATTABCOMPLETION_HEADER

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"

class pawsChatWindow;

class pawsConfigChatTabCompletion : public pawsConfigSectionWindow
{
public:
       pawsConfigChatTabCompletion();

    //from pawsWidget:
       virtual bool PostSetup();

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

       // from pawsWidget
       bool OnChange(pawsWidget* /*widget*/) { dirty = true; return true; }
       virtual bool OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
       {
           dirty = true;
           return true;
       }
       virtual void OnListAction(pawsListBox* /*selected*/, int /*status*/) {dirty = true;};

private:
       pawsChatWindow* chatWindow;
       // needed gui elemets
       pawsMultilineEditTextBox* completionItemsBox;
};

CREATE_PAWS_FACTORY( pawsConfigChatTabCompletion );

#endif //PAWS_CONFIG_CHATTABCOMPLETION_HEADER
