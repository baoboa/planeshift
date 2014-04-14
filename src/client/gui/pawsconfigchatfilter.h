/*
 * pawsconfigchatfilter.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * Credits : Christian Svensson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Creation Date: 11/Jan 2005
 * Description : This is the header file for the pawsConfigChatFilter window
 *               which provides filter settings for the chat window
 *
 */

#ifndef HEADER_CONFIG_CHAT_FILTER
#define HEADER_CONFIG_CHAT_FILTER

#include "paws/pawswidget.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigwindow.h"
#include "chatwindow.h"

class pawsConfigChatFilter : public pawsConfigSectionWindow
{
public:
    // Functions needed for being a config window
    bool Initialize();
    bool LoadConfig();
    bool SaveConfig();
    void SetDefault();

    bool PostSetup();

private:

    // Arrays with the checkboxes
    pawsCheckBox* me[COMBAT_TOTAL_AMOUNT];
    pawsCheckBox* vicinity[COMBAT_TOTAL_AMOUNT];
};

CREATE_PAWS_FACTORY(pawsConfigChatFilter);

#endif

