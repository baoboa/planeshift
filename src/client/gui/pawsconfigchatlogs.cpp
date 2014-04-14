/*
 * pawsconfigchatlogs.cpp - Author: Stefano Angeleri
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

//CS INCLUDES
#include <psconfig.h>

//PAWS INCLUDES
#include "pawsconfigchatlogs.h"
#include "chatwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "paws/pawslistbox.h"
#include "net/messages.h"

//CLIENT INCLUDES
#include "../globals.h"
#include "autoexec.h"

//needs some fix up these things are being defined globally in chatwindow.cpp
extern const char *CHAT_TYPES[];

//constructor. We set -1 as currentType as an invalid default value.
pawsConfigChatLogs::pawsConfigChatLogs() : currentType(-1)
{ }

bool pawsConfigChatLogs::PostSetup()
{
    //search for the widgets we need to use so we keep a reference to them.
    enabled = dynamic_cast<pawsCheckBox*>(FindWidget("enabled"));
    if (!enabled)
    {
           Error1("Could not locate enabled widget!");
           return false;
    }
    chatTypes = dynamic_cast<pawsListBox*>(FindWidget("logtypes"));
    if (!chatTypes)
    {
           Error1("Could not locate logtypes widget!");
           return false;
    }
    fileName = dynamic_cast<pawsEditTextBox*>(FindWidget("filename"));
    if (!fileName)
    {
           Error1("Could not locate filename widget!");
           return false;
    }
    bracket = dynamic_cast<pawsEditTextBox*>(FindWidget("bracket"));
    if (!bracket)
    {
           Error1("Could not locate bracket widget!");
           return false;
    }
    return true;
}

bool pawsConfigChatLogs::Initialize()
{
    if ( ! LoadFromFile("configchatlogs.xml"))
    {
        return false;
    }

    //fill the listbox with the chat types
    for(int i = 0; i < CHAT_END; i++)
    {
        pawsListBoxRow* row =  chatTypes->NewRow();
        //this is important as it's a corrispondence to the chat types enum/char array
        row->SetID(i);
        pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
        name->SetText(CHAT_TYPES[i]);
    }

    return true;
}

bool pawsConfigChatLogs::LoadConfig()
{
    pawsChatWindow* chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    // Since this is a critical widget, we shouldn't have got this far if it isn't loaded

    chatWindow->LoadChatSettings();

    ChatSettings& settings = chatWindow->GetSettings();

    //set our local config so we don't touch the original settings till the user clicks save
    for(int i = 0; i < CHAT_END; i++)
    {
        logStatus[i] = settings.enabledLogging[i];
        logFile[i] = settings.logChannelFile[i];
        logBracket[i] = settings.channelBracket[i];
    }

    //select the first, if none were selected, or the
    //currently selected chat type in order to force a correct
    //update of the interface
    pawsListBoxRow * row = chatTypes->GetSelectedRow();
    if(row)
        chatTypes->Select(row, true);
    else
        chatTypes->SelectByIndex(0, true);

    dirty = true;
    return true;
}

bool pawsConfigChatLogs::SaveConfig()
{
    pawsChatWindow* chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    // Since this is a critical widget, we shouldn't have got this far if it isn't loaded

    ChatSettings& settings = chatWindow->GetSettings();

    //force updating of the data structures
    OnListAction(chatTypes, 0 /*unused*/);

    //save back the data to the chat primary settings
    for(int i = 0; i < CHAT_END; i++)
    {
        settings.enabledLogging[i] = logStatus[i];
        settings.SetLogChannelFile(i, logFile[i]);
        settings.channelBracket[i] = logBracket[i];
    }

    // Save to file
    chatWindow->SaveChatSettings();
    return true;
}

void pawsConfigChatLogs::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}

void pawsConfigChatLogs::OnListAction(pawsListBox* selected, int /*status*/)
{
    //here we handle a selection of a different chat type
    if(selected == chatTypes)
    {
        //check if the selection is valid
        pawsListBoxRow * row = chatTypes->GetSelectedRow();
        if(row)
        {
            //if it's valid check if the previous selection was valid
            //if it's so update the temporary settings in this widget
            //else don't update it with invalid data
            if(currentType >= 0)
            {
                logStatus[currentType] = enabled->GetState();
                logFile[currentType] = fileName->GetText();
                logBracket[currentType] = bracket->GetText();
            }
            //set the current type with the row which was choosen
            currentType = row->GetID();
            //update the checkboxes and textboxes accordly to the selection
            enabled->SetState(logStatus[currentType]);
            fileName->SetText(logFile[currentType]);
            bracket->SetText(logBracket[currentType]);
        }
    }
}
