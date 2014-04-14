/*
 * pawsconfigchatfilter.cpp
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
 * Description : This is the implementation for the pawsConfigChatFilter window
 *               which provides filter settings for the chat window
 *
 */

#include <psconfig.h>
#include "globals.h"

#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigchatfilter.h"

bool pawsConfigChatFilter::PostSetup()
{
    // Fill the arrays
    me[0] = (pawsCheckBox*)FindWidget("osuc");
    me[1] = (pawsCheckBox*)FindWidget("oblo");
    me[2] = (pawsCheckBox*)FindWidget("odod");
    me[3] = (pawsCheckBox*)FindWidget("omis");
    me[4] = (pawsCheckBox*)FindWidget("ofai");
    me[5] = (pawsCheckBox*)FindWidget("osta");

    vicinity[0] = (pawsCheckBox*)FindWidget("vsuc");
    vicinity[1] = (pawsCheckBox*)FindWidget("vblo");
    vicinity[2] = (pawsCheckBox*)FindWidget("vdod");
    vicinity[3] = (pawsCheckBox*)FindWidget("vmis");
    vicinity[4] = (pawsCheckBox*)FindWidget("vfai");
    vicinity[5] = (pawsCheckBox*)FindWidget("vsta");

    return true;
}

bool pawsConfigChatFilter::Initialize()
{
    if (!LoadFromFile("configchatfilter.xml"))
        return false;

    return true;
}

bool pawsConfigChatFilter::LoadConfig()
{
    pawsChatWindow* chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    // Since this is a critical widget, we shouldn't have got this far if it isn't loaded

    chatWindow->LoadChatSettings();

    ChatSettings& settings = chatWindow->GetSettings();
    // Set all filters
    for(int i=0;i < COMBAT_TOTAL_AMOUNT;i++)
    {
        CHAT_COMBAT_FILTERS filter;

        switch(i)
        {
            case 0:
                filter = COMBAT_SUCCEEDED; break;
            case 1:
                filter = COMBAT_BLOCKED; break;
            case 2:
                filter = COMBAT_DODGED; break;
            case 3:
                filter = COMBAT_MISSED; break;
            case 4:
                filter = COMBAT_FAILED; break;
            case 5:
            default:
                filter = COMBAT_STANCE; break;
        }

        me[i]->SetState(false);
        vicinity[i]->SetState(false);

        if(settings.meFilters & filter)
            me[i]->SetState(true);

        if(settings.vicinityFilters & filter)
            vicinity[i]->SetState(true);
    }

    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChatFilter::SaveConfig()
{
    pawsChatWindow* chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    // Since this is a critical widget, we shouldn't have got this far if it isn't loaded

    ChatSettings& settings = chatWindow->GetSettings();
    settings.meFilters = 0;
    settings.vicinityFilters = 0;

    // Set all filters
    for(int i=0;i < COMBAT_TOTAL_AMOUNT;i++)
    {
        CHAT_COMBAT_FILTERS filter;

        switch(i)
        {
            case 0:
                filter = COMBAT_SUCCEEDED; break;
            case 1:
                filter = COMBAT_BLOCKED; break;
            case 2:
                filter = COMBAT_DODGED; break;
            case 3:
                filter = COMBAT_MISSED; break;
            case 4:
                filter = COMBAT_FAILED; break;
            case 5:
            default:
                filter = COMBAT_STANCE; break;
        }

        if(me[i]->GetState())
            settings.meFilters |= filter;
        if(vicinity[i]->GetState())
            settings.vicinityFilters |= filter;
    }

    // Save to file
    chatWindow->SaveChatSettings();

    return true;
}

void pawsConfigChatFilter::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}
