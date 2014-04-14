/*
 * pawsquestrewardwindow.h - Author: Ian Donderwinkel
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

#ifndef PAWS_QUEST_REWARD_WINDOW_HEADER
#define PAWS_QUEST_REWARD_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"

class pawsQuestRewardWindow : public pawsWidget, public psCmdBase
{
public:
    pawsQuestRewardWindow();
    virtual ~pawsQuestRewardWindow();

    void HandleMessage( MsgEntry* message );
    const char* HandleCommand(const char* cmd);

    bool PostSetup();

    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

protected:
    pawsListBox*    rewardList;
};

CREATE_PAWS_FACTORY( pawsQuestRewardWindow );

#endif
