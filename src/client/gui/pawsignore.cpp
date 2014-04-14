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

#include <csutil/xmltiny.h>
#include <ctype.h>

#include "../globals.h"

#include "pscelclient.h"

#include "pawsignore.h"
#include "paws/pawslistbox.h"
#include "paws/pawstextbox.h"
#include "gui/chatwindow.h"



#define ADD     3000
#define REMOVE  2000


pawsIgnoreWindow::~pawsIgnoreWindow()
{
    SaveIgnoreList();
}

bool pawsIgnoreWindow::PostSetup()
{
    ignoreList = (pawsListBox*)FindWidget( "IgnoreList" );

    if ( !ignoreList )
        return true;

    if ( !LoadIgnoreList() )
        return false;

    return true;
}

void pawsIgnoreWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        pawsListBoxRow* row =  widget->GetSelectedRow();
        if ( row )
        {
            currentIgnored = ((pawsTextBox*)row->GetColumn(0))->GetText();
        }
    }
}



bool pawsIgnoreWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{

    switch ( widget->GetID() )
    {
        case REMOVE:
        {
            if ( currentIgnored.Length() == 0 )
               return false;

            pawsStringPromptWindow::Create("Remove", currentIgnored,
                                        false, 220, 20, this, "remove" );
            return true;
        }

        case ADD:
        {
            csString empty("");
            pawsStringPromptWindow::Create("Add", csString(""),
                                       false, 220, 20, this, "add" );
            return true;
        }
    }
    return false;
}


bool pawsIgnoreWindow::LoadIgnoreList()
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (PawsManager::GetSingleton().GetObjectRegistry());
    iDocumentSystem* xml = psengine->GetXMLParser ();


    if (vfs && xml)
    {
        csRef<iFile> file = vfs->Open ("/planeshift/userdata/options/ignore.xml", VFS_FILE_READ);

        if ( file )
        {
            csString ignored;

            csRef<iDocument> doc = xml->CreateDocument();

            const char* error = doc->Parse(file);
            if (error)
            {
                Error2("Error Loading Ignorelist: %s", error);
                return false;
            }

            csRef<iDocumentNodeIterator> iter = doc->GetRoot()->GetNode("Ignorelist")->GetNodes("Ignored");
            while(iter->HasNext())
            {
                ignored = iter->Next()->GetAttributeValue("name");
                AddIgnore(ignored);
            }
            return true;
        }
        else
        {
            Warning1(LOG_PAWS, "No ignore.xml file found. Assuming empty");
            return true;
        }
    }
    else
    {
        return false;
    }

}



void pawsIgnoreWindow::SaveIgnoreList()
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot();
    csRef<iDocumentNode> listNode = root->CreateNodeBefore(CS_NODE_ELEMENT);
    listNode->SetValue("Ignorelist");

    for (size_t i = 0; i < ignoredNames.GetSize(); i++)
    {
        csRef<iDocumentNode> node = listNode->CreateNodeBefore(CS_NODE_ELEMENT);
        node->SetValue("Ignored");
        node->SetAttribute("name",ignoredNames[i]);
    }
    doc->Write(vfs, "/planeshift/userdata/options/ignore.xml");
}



void pawsIgnoreWindow::AddIgnore( csString& name )
{
    pawsChatWindow* chat = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");

    if (!strcmp(name, psengine->GetMainPlayerName()))
    {
        if ( chat )
            chat->ChatOutput(PawsManager::GetSingleton().Translate("You can not ignore yourself."));
        return;
    }

    if ( chat )
    {
        csString temp;
        temp.Format(PawsManager::GetSingleton().Translate("You will now ignore %s."), name.GetData());
        chat->ChatOutput(temp);
    }

    if (ignoredNames.FindSortedKey(name) == csArrayItemNotFound)
    {
        pawsListBoxRow* row = ignoreList->NewRow();
        pawsTextBox* textname = (pawsTextBox*)row->GetColumn(0);
        ignoredNames.InsertSorted(name);
        row->SetName( name );
        textname->SetText( name );
        textname->SetColour( graphics2D->FindRGB( 255,0,0 ) );
    }
}




void pawsIgnoreWindow::RemoveIgnore( csString& name )
{
    pawsChatWindow* chat = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if ( !chat )
        return;
    else
    {
        csString temp;
        temp.Format(PawsManager::GetSingleton().Translate("You will no longer ignore %s."), name.GetData());
        chat->ChatOutput(temp);
        size_t idx = ignoredNames.FindSortedKey(name);
        if ( idx != csArrayItemNotFound )
        {
            ignoredNames.DeleteIndex(idx);
        }
        pawsListBoxRow* row = (pawsListBoxRow*)ignoreList->FindWidget( name );
        ignoreList->Select( row );
        ignoreList->RemoveSelected();
    }
}

bool pawsIgnoreWindow::IsIgnored(csString &name)
{
    bool result = (ignoredNames.FindSortedKey(name) != csArrayItemNotFound);
    return result;
}

void pawsIgnoreWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    if (!value || !strlen(value))
        return;

    csString command;
    csString person = value;
    //normalize the name
    person.Downcase();
    person.SetAt(0,toupper(person.GetAt(0))); //TODO: maybe we need to do this better

    if (!strcmp(name,"add")) //If we got the add command...
    {
            AddIgnore(person); //...add the person to the ignore list
    }
    else //If we got the remove command...
    {
        if (IsIgnored(person)) //... check if the person is ignored and if so remove him/her from the list
            RemoveIgnore(person);
    }

    psengine->GetCmdHandler()->Execute(command);
}
