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

#include <psconfig.h>
#include <iutil/objreg.h>

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "../globals.h"
#include "paws/pawslistbox.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"
#include "pscelclient.h"

#include "pawsbuddy.h"

#define TELL    1000
#define REMOVE  2000
#define ADD     3000
#define EDIT    4000

pawsBuddyWindow::pawsBuddyWindow()
{
    buddyList = NULL;
    editBuddy.Clear();
}

bool pawsBuddyWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_BUDDY_LIST );
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_BUDDY_STATUS );
                
    buddyList = (pawsListBox*)FindWidget("BuddyList");
    
    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if (!chatWindow)
    {
        Error1("BuddyWindow failed because Chat window was not found.");
        return false;
    }    
    return true;
}

void pawsBuddyWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        pawsListBoxRow* row =  widget->GetSelectedRow();
        if ( row )
        {
            currentBuddy = ((pawsTextBox*)row->GetColumn(0))->GetText();
        }
    }

    else if (status == LISTBOX_SELECTED)
    {
        if (currentBuddy.IsEmpty())
            return;

        csString title(PawsManager::GetSingleton().Translate("Tell"));
        csString name = GetRealName(currentBuddy);
        title.Append(" " + name);
        pawsStringPromptWindow::Create(title, csString(""),
                                       false, 220, 20, this, name);
    }
}

void pawsBuddyWindow::Show()
{
    pawsControlledWindow::Show();
    psUserCmdMessage cmdmsg("/buddy");
    psengine->GetMsgHandler()->SendMessage(cmdmsg.msg); 
}

void pawsBuddyWindow::OnResize()
{
    pawsWidget::OnResize();

    if (buddyList)
    {
        buddyList->CalculateDrawPositions();
    }
}	

void pawsBuddyWindow::HandleMessage( MsgEntry* me )
{
    if ( me->GetType() == MSGTYPE_BUDDY_LIST )
    {
        psBuddyListMsg mesg(me);
    
        onlineBuddies.DeleteAll();
        offlineBuddies.DeleteAll();

        // Load aliases
        LoadAliases(psengine->GetMainPlayerName());

        for (size_t x = 0; x < mesg.buddies.GetSize(); x++ )
        {
            if (mesg.buddies[x].online)
                onlineBuddies.Push(GetAlias(mesg.buddies[x].name));
            else
                offlineBuddies.Push(GetAlias(mesg.buddies[x].name));
            chatWindow->AddAutoCompleteName(mesg.buddies[x].name);
        }

    }
    else if ( me->GetType() == MSGTYPE_BUDDY_STATUS )
    {
        psBuddyStatus mesg(me);
        
        // If player is online now remove from the offline list
        // else remove them from offline list and add to the online list.
        if ( mesg.onlineStatus )       
        {   
            size_t loc = offlineBuddies.Find(GetAlias(mesg.buddy.GetData()));
            if ( loc != csArrayItemNotFound )
            {
                offlineBuddies.DeleteIndex(loc);
            }                
            onlineBuddies.Push(GetAlias(mesg.buddy));
        }
        else
        {
            size_t loc = onlineBuddies.Find(GetAlias(mesg.buddy.GetData()));
            if ( loc != csArrayItemNotFound )
            {
                onlineBuddies.DeleteIndex(loc);
            }                

            offlineBuddies.Push(GetAlias(mesg.buddy));
        }

        chatWindow->AddAutoCompleteName(mesg.buddy);
    }

    FillBuddyList();
}


bool pawsBuddyWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {
        case TELL:
        {
            if ( currentBuddy.Length() == 0 )
                return true;

            csString title(PawsManager::GetSingleton().Translate("Tell"));
            csString name = GetRealName(currentBuddy);
            title.Append(" " + name);
            pawsStringPromptWindow::Create(title, csString(""),
                                           false, 220, 20, this, name);
            return true;
        }
        
        case REMOVE:
        {          
            if ( currentBuddy.Length() == 0 )
                return true;

            pawsStringPromptWindow::Create(PawsManager::GetSingleton().Translate("Remove"), 
                                            GetRealName(currentBuddy), false, 220, 20, this, "RemoveBuddy" );            
            return true;
        }
        
        case ADD:
        {
            pawsStringPromptWindow::Create(PawsManager::GetSingleton().Translate("Add"), csString(""),
                                           false, 220, 20, this, "AddBuddy");            
            return true;
        }

        case EDIT:
        {
            if (currentBuddy.IsEmpty())
                return true;

            if (!editBuddy.IsEmpty())
            {
                // Already editing.
                // Since we store only one name that is being edited, we cannot edit any other names.
                psSystemMessage err(0, MSG_ERROR, 
                                   PawsManager::GetSingleton().Translate("You are already renaming a buddy."));
                err.FireEvent();
                return true;
            }

            // Save the real name of the buddy. Player may select another buddy or delete the buddy
            // before clicking OK on the rename window. It is also possible that server removes the
            // buddy while it is edited.
            editBuddy = GetRealName(currentBuddy);

            pawsStringPromptWindow::Create(PawsManager::GetSingleton().Translate("Rename") + " " + editBuddy, 
                                           currentBuddy, false, 220, 20, this, "EditBuddy");
            return true;
        }
    }
    return false;
}

void pawsBuddyWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    if (!value || !strlen(value))
    {
        if (!strcmp(name, "EditBuddy"))
            editBuddy.Clear();
        return;
    }

    if (!strcmp(name,"AddBuddy"))
    {
        // Is the new buddy name unique?
        if (!IsUniqueAlias(value))
        {
            psSystemMessage err(0, MSG_ERROR, "%s '%s' %s", 
                                PawsManager::GetSingleton().Translate("Buddy with the name").GetData(),value,
                                PawsManager::GetSingleton().Translate("already exists.").GetData());
            err.FireEvent();
            return;
        }
        csString command;
        command.Format("/buddy %s add", value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"RemoveBuddy"))
    {
        csString command;
        command.Format("/buddy %s remove", value);
        psengine->GetCmdHandler()->Execute(command);
        
        currentBuddy.Clear();

        if (editBuddy.Compare(value))
        {
            // Removing the buddy that is being edited
            editBuddy.Clear();
        }

        // Remove from the alias list. It may fail on the server, but we don't have enough
        // information to remove the alias when the buddy was actually removed.
        if (aliases.DeleteAll(value))
            SaveAliases(psengine->GetMainPlayerName());
    }
    else if (!strcmp(name, "EditBuddy"))
    {
        // It might be that the player removed the buddy from the list before clicking OK
        if (editBuddy.IsEmpty())
        {
            psSystemMessage err(0, MSG_ERROR,
                                "%s '%s' %s",
                                PawsManager::GetSingleton().Translate("Buddy with the new name").GetData(),
                                value,
                                PawsManager::GetSingleton().Translate("cannot be found. Perhaps it was removed.").GetData());
            err.FireEvent();
            return;
        }

        csString alias = GetAlias(editBuddy);
        ChangeAlias(editBuddy, alias, value);

        editBuddy.Clear();

    }
    else
    {
        csString command;
        command.Format("/tell %s %s", name, value );
        psengine->GetCmdHandler()->Execute(command, false);
    }
}

csString pawsBuddyWindow::GetAlias(const csString & name) const
{
    return aliases.Get(name, name);
}

csString pawsBuddyWindow::GetRealName(const csString & alias) const
{
    csHash<csString, csString>::ConstGlobalIterator it = aliases.GetIterator();
    while (it.HasNext())
    {
        csString name;
        if (it.Next(name) == alias)
            return name;
    }

    return alias;
}

void pawsBuddyWindow::LoadAliases(const csString & charName)
{
    aliases.DeleteAll();

    csRef<iDocument> doc;
    csRef<iDocumentNode> root;

    csString fileName = ALIASES_FILE_PREFIX + charName + ".xml";
    fileName.FindReplace(" ", "_");
    if (!psengine->GetVFS()->Exists(fileName))
        return;

    doc = ParseFile(psengine->GetObjectRegistry(), fileName);
    if (doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return;
    }

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("%s has no XML root", fileName.GetData());
        return;
    }

    csRef<iDocumentNode> aliasesNode = root->GetNode("aliases");
    if (aliasesNode == NULL)
    {
        Error2("%s has no <aliases> tag", fileName.GetData());
        return;
    }

    csRef<iDocumentNodeIterator> nodes = aliasesNode->GetNodes();
    while (nodes->HasNext())
    {
        csRef<iDocumentNode> alias = nodes->Next();
        if (strcmp(alias->GetValue(), "alias"))
            continue;

        // Check for duplicate aliases
        if (!IsUniqueAlias(alias->GetAttributeValue("alias")))
        {
            Error2("Ignoring duplicate alias '%s'", alias->GetAttributeValue("alias"));
            continue;
        }

        aliases.PutUnique(alias->GetAttributeValue("name"), alias->GetAttributeValue("alias"));

    }
}

void pawsBuddyWindow::SaveAliases(const csString & charName) const
{
    csString fileName = ALIASES_FILE_PREFIX + charName + ".xml";
    fileName.FindReplace(" ", "_");

    csString xml = "<aliases>\n";

    csHash<csString, csString>::ConstGlobalIterator it = aliases.GetIterator();
    while (it.HasNext())
    {
        csString name, alias;
        alias = it.Next(name);
        xml.AppendFmt("    <alias name=\"%s\" alias=\"%s\" />\n", name.GetDataSafe(), alias.GetDataSafe());
    }
    xml += "</aliases>\n";

    if (!psengine->GetVFS()->WriteFile(fileName, xml, xml.Length()))
        Error2("Failed to write to %s", fileName.GetDataSafe());
}

void pawsBuddyWindow::ChangeAlias(const csString & name, const csString &oldAlias, const csString & newAlias)
{
    // Check for duplicates
    if (!IsUniqueAlias(newAlias))
    {
        psSystemMessage err(0, MSG_ERROR,
                            "%s '%s' %s",
                            PawsManager::GetSingleton().Translate("Buddy with the name").GetData(), newAlias.GetDataSafe(),
                            PawsManager::GetSingleton().Translate("already exists.").GetData());
        err.FireEvent();
        return;
    }

    bool found = false;

    // Find the old alias in both lists and replace with the new alias.
    size_t loc = offlineBuddies.Find(oldAlias);
    if (loc != csArrayItemNotFound)
    {
        found = true;
        offlineBuddies.DeleteIndex(loc);
        offlineBuddies.Push(newAlias);
    }
    loc = onlineBuddies.Find(oldAlias);
    if (loc != csArrayItemNotFound)
    {
        found = true;
        onlineBuddies.DeleteIndex(loc);
        onlineBuddies.Push(newAlias);
    }

    if (!found)
    {
        // Old alias not found, perhaps the buddy was removed from the list
        psSystemMessage err(0, MSG_ERROR,
                            "%s '%s' %s",
                            PawsManager::GetSingleton().Translate("Buddy with the name").GetData(),
                            name.GetDataSafe(), 
                            PawsManager::GetSingleton().Translate("cannot be found. Perhaps it was removed.").GetData());
        err.FireEvent();

        return;
    }

    // Add/replace in the aliases table and save it.
    aliases.PutUnique(name, newAlias);
    SaveAliases(psengine->GetMainPlayerName());

    // Update the listbox
    FillBuddyList();
}

void pawsBuddyWindow::FillBuddyList()
{
    buddyList->Clear();
    currentBuddy.Clear();

    onlineBuddies.Sort();
    offlineBuddies.Sort();

    for (size_t x = 0; x < onlineBuddies.GetSize(); x++ )
    {
        pawsListBoxRow* row = buddyList->NewRow();
        pawsTextBox* buddyname = (pawsTextBox*)row->GetColumn(0);
        buddyname->SetText( onlineBuddies[x] );
        buddyname->SetName( onlineBuddies[x] );
        buddyname->SetToolTip(onlineBuddies[x]);
        buddyname->SetColour( graphics2D->FindRGB( 0,255,0 ) );
    }

    for (size_t x = 0; x < offlineBuddies.GetSize(); x++ )
    {
        pawsListBoxRow* row = buddyList->NewRow();
        pawsTextBox* buddyname = (pawsTextBox*)row->GetColumn(0);
        buddyname->SetText( offlineBuddies[x] );
        buddyname->SetName( offlineBuddies[x] );
        buddyname->SetToolTip(offlineBuddies[x]);
        buddyname->SetColour( graphics2D->FindRGB( 255,0,0 ) );
    }
}

bool pawsBuddyWindow::IsUniqueAlias(const csString & alias) const
{
    // Check in current aliases
    csHash<csString, csString>::ConstGlobalIterator it = aliases.GetIterator();
    while (it.HasNext())
    {
        csString name;
        if (it.Next(name) == alias)
            return false;
    }

    // Check in online and offline lists for real names
    if (offlineBuddies.Find(alias) != csArrayItemNotFound)
        return false;
    if (onlineBuddies.Find(alias) != csArrayItemNotFound)
        return false;

    // Should be unique
    return true;
}
