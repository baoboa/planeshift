/*
 * Author: Andrew Craig
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
#ifndef PAWS_IGNORE_HEADER
#define PAWS_IGNORE_HEADER

#include "globals.h"

#include "paws/pawswidget.h"
#include "paws/pawsstringpromptwindow.h"
#include "net/cmdhandler.h"

class pawsListBox;

/** A player's ignore list window.
 *  From here the player can add or remove messages from a particular
player.
 */
class pawsIgnoreWindow : public pawsWidget,public iOnStringEnteredAction
{
public:
    ~pawsIgnoreWindow();

    void AddIgnore( csString& playerName );
    void RemoveIgnore( csString& playerName );
    bool IsIgnored( csString& playerName );

    bool PostSetup();

    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* widget, int status );

    /// Handle popup question window callback
    void OnStringEntered(const char *name,int param,const char *value);

private:

    /// Current list of people that should be ignored.
    csStringArray   ignoredNames;

    pawsListBox*    ignoreList;

    /// Selected person in ignore list.
    csString        currentIgnored;

    bool LoadIgnoreList();
    void SaveIgnoreList();
};

CREATE_PAWS_FACTORY( pawsIgnoreWindow );

#endif

