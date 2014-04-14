/*
 * pawsgroupwidow.cpp - Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>

// COMMON INCLUDES
#include "net/messages.h"
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"
#include "clientvitals.h"

// PAWS INCLUDES
#include "pawsgroupwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawslistbox.h"
#include "paws/pawsprogressbar.h"
#include "paws/pawsstringpromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

#include "net/messages.h"
#include "util/log.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsGroupWindow::pawsGroupWindow()
{
    memberList = 0;
}

pawsGroupWindow::~pawsGroupWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_GUIGROUP);
}

bool pawsGroupWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GUIGROUP);

    memberList = (pawsListBox*)FindWidget("List");
    if ( !memberList ) return false;

    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if (!chatWindow)
        return false;

    return true;
}


void pawsGroupWindow::HandleMessage( MsgEntry* me )
{
    psGUIGroupMessage incomming(me);

    switch ( incomming.command )
    {
        case psGUIGroupMessage::GROUP:
        {
            Show();
            HandleGroup( incomming.commandData );
            break;
        }

        case psGUIGroupMessage::MEMBERS:
        {
            HandleMembers( incomming.commandData );
            break;
        }

        case psGUIGroupMessage::LEAVE:
        {
            Hide();
            memberList->Clear();

            if ( incomming.commandData.Length() > 0 )
            {
                psSystemMessage note(0,MSG_INFO, incomming.commandData );
                psengine->GetMsgHandler()->Publish(note.msg);
            }


            break;
        }
    }
}

void pawsGroupWindow::HandleGroup(csString& /*group*/)
{
    Show();
}

void pawsGroupWindow::HandleMembers( csString& members )
{
    memberList->Clear();

    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( members );

    if ( error )
    {
        Error2("Error in XML: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", members.GetData());
        return;
    }
    csRef<iDocumentNode> xmlmembers = root->GetNode("L");
    if(!xmlmembers)
    {
        Error2("No <L> tag in %s", members.GetData());
        return;

    }

    csRef<iDocumentNodeIterator> iter = xmlmembers->GetNodes("M");

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> member = iter->Next();

        pawsListBoxRow* box = memberList->NewRow();
        pawsWidget* stats = box->GetColumn(0)->FindWidget("stats");
        pawsProgressBar *progress;
        pawsTextBox *text;
        pawsWidget *icon;

        box->SetName ( member->GetAttributeValue("N") );

        icon = stats->FindWidget("ICON");
        icon->SetBackground(csString(member->GetAttributeValue("R")) + " Icon");

        chatWindow->AddAutoCompleteName( member->GetAttributeValue("N") );

        //Set name
        text = (pawsTextBox*)stats->FindWidget("NAME");
        text->SetText( member->GetAttributeValue("N") );

        //Set HP
        progress = (pawsProgressBar*)stats->FindWidget("HP");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue(member->GetAttributeValueAsFloat("H")/100);

        //Set mana
        progress = (pawsProgressBar*)stats->FindWidget("MANA");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue(member->GetAttributeValueAsFloat("M")/100);

    }
}

void pawsGroupWindow::SetStats( GEMClientActor* actor )
{

    csString firstName(actor->GetName());
    size_t pos=firstName.FindFirst(' ');
    firstName = firstName.Slice(0,pos);

    pawsListBoxRow* row = (pawsListBoxRow*)FindWidget( firstName.GetData() );

    if ( row )
    {
        float hp;
        float mana;

        // Adjust the hitpoints
        hp = actor->GetVitalMgr()->GetHP();

        // Adjust the mana
        mana = actor->GetVitalMgr()->GetMana();

        pawsProgressBar *progress;

        pawsWidget* stats = row->GetColumn(0)->FindWidget("stats");

        //Set HP
        progress = (pawsProgressBar*)stats->FindWidget("HP");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue( hp );

        //Set mana
        progress = (pawsProgressBar*)stats->FindWidget("MANA");
        progress->SetTotalValue( 1 );
        progress->SetCurrentValue( mana );
    }
}

void pawsGroupWindow::Draw()
{
    GEMClientActor* player = psengine->GetCelClient()->GetMainPlayer();

    player->GetVitalMgr()->Predict( csGetTicks(),"Self" );

    if (memberList->GetRowCount()>0)
        SetStats( player );
    pawsWidget::Draw();
}

void pawsGroupWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    if (value && strlen(value))
    {
        if (!strcmp(name,"NewInvitee"))
        {
            csString cmd;
            cmd.Format("/invite %s", value);
            psengine->GetCmdHandler()->Execute(cmd);
        }
    }
}

bool pawsGroupWindow::OnMenuAction(pawsWidget *widget, const pawsMenuAction & action)
{
    if (action.name == "Invite")
    {
        pawsStringPromptWindow::Create("Who do you want to invite?", "",
                                       false, 250, 20, this, "NewInvitee");
    }
    else if (action.name == "Leave")
    {
        psengine->GetCmdHandler()->Execute("/leavegroup");
    }
    else if (action.name == "Disband")
    {
        psengine->GetCmdHandler()->Execute("/disband");
    }

    return pawsWidget::OnMenuAction(widget, action);
}

