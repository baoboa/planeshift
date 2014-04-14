/*
 * pawsconfigchattabcompletion.cpp - Author: Fabian Stock (Aiwendil)
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

//CS INCLUDES
#include <psconfig.h>

//PAWS INCLUDES
#include "pawsconfigchattabcompletion.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"

//CLIENT INCLUDES
#include "../globals.h"
#include "gui/chatwindow.h"

pawsConfigChatTabCompletion::pawsConfigChatTabCompletion()
{
    completionItemsBox = NULL;
}

bool pawsConfigChatTabCompletion::PostSetup()
{
    // setup the widget. Needed in the xml file is one a MuiltLine Edit Box for the words
    chatWindow = dynamic_cast<pawsChatWindow*>(PawsManager::GetSingleton().FindWidget("ChatWindow"));
    if(!chatWindow)
    {
        Error1("Couldn't find ChatWindow!");
        return false;
    }
    completionItemsBox = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("CompletionItemsBox"));
    if (!completionItemsBox) {
           Error1("Could not locate CompletionItemsBox widget!");
           return false;
    }

    return true;
}

bool pawsConfigChatTabCompletion::Initialize()
{
    if ( ! LoadFromFile("configchattabcompletion.xml"))
    {
        return false;
    }
    return true;
}

bool pawsConfigChatTabCompletion::LoadConfig()
{
    ChatSettings &settings = chatWindow->GetSettings();

    csString items("");
    //fill the text box with the items by putten them all in a single string separated by newlines
    for (size_t i = 0; i < settings.completionItems.GetSize(); i++)
    {
        items.Append(settings.completionItems[i]).Append("\n");
    }

    completionItemsBox->SetText(items);
    dirty = false;
    return true;
}

bool pawsConfigChatTabCompletion::SaveConfig()
{
    ChatSettings &settings = chatWindow->GetSettings();

    // clear the whole array with tab completion items
    settings.completionItems.DeleteAll();
    // then refill it with the values from the text box
    // after splitting the content of the textbox in single strings again
    csString items = completionItemsBox->GetText();
    csString tmpItem;
    size_t oldNewLine = 0;
    size_t foundNewLine = items.Find("\n", oldNewLine);
    while (foundNewLine != (size_t)-1)
    {
        items.SubString(tmpItem, oldNewLine, foundNewLine-oldNewLine);
        tmpItem.Collapse();
        if (!tmpItem.IsEmpty())
        {
            settings.completionItems.Push(tmpItem);
        }
        oldNewLine = foundNewLine;
        foundNewLine = items.Find("\n", oldNewLine+1);
    }
    items.SubString(tmpItem, oldNewLine, items.Length()-oldNewLine);
    tmpItem.Collapse();
    if (!tmpItem.IsEmpty())
    {
        settings.completionItems.Push(tmpItem);
    }

    // Save to file
    chatWindow->SaveChatSettings();
    LoadConfig();
    return true;
}

void pawsConfigChatTabCompletion::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}

