/*
* pawstutorialwindow.cpp - Author: Keith Fulton
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//////////////////////////////////////////////////////////////////////

// STANDARD INCLUDE
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>
#include <ivideo/fontserv.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

#include "util/strutil.h"

#include "gui/psmainwidget.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscelclient.h"
#include "pawstutorialwindow.h"
#include "globals.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsTutorialNotifyWindow::pawsTutorialNotifyWindow()
{
    instr_container = instructions = NULL;
}

pawsTutorialNotifyWindow::~pawsTutorialNotifyWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_TUTORIAL);
}

bool pawsTutorialNotifyWindow::PostSetup()
{
    // Subscribe to the messages we want to handle.
    // Receiving this message will popup the first window.
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_TUTORIAL);

    instr_container = psengine->GetMainWidget()->FindWidget("Tutorial");
    instructions = (pawsMultiLineTextBox*)instr_container->FindWidget("TutorialInstructions");
    if (!instr_container || !instructions)
    {
        Error1("Could not find tutorial paws widget or tutorial instructions text field on it.");
        return false;
    }
    return true;
}

bool pawsTutorialNotifyWindow::NextTutorial()
{
    instrArray.DeleteIndex(0);

    if (instrArray.GetSize())
    {
        // Set it in the other window
        instructions->SetText( instrArray.Get(0) );
        // Reveal the icon to let the player click to see the instrs
        Show();
        return true; // A new message is waiting.
    }
    return false;  // Nothing left.
}


void pawsTutorialNotifyWindow::HandleMessage ( MsgEntry* me )
{
    if (me->GetType() == MSGTYPE_TUTORIAL)
    {
        // Get the tutorial message data from the server
        psTutorialMessage message(me);

        Debug2(LOG_PAWS, 0, "Got tutorial message: %s", message.instrs.GetData());

        // Put these instructions in queue.
        instrArray.Push(message.instrs);

        // If it's the welcome message, just display it on screen
        if (message.instrs.StartsWith("Welcome",true)) {
            instructions->SetText(message.instrs);
            instr_container->Show();
            return;
        }

        if (instrArray.GetSize() == 1)
        {
            // Set it in the other window
            instructions->SetText(message.instrs);
            // Reveal the icon to let the player click to see the instrs
            Show();
        }
        else
        {
            // No action required.  In queue waiting.
        }
    }
}

bool pawsTutorialNotifyWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* /*widget*/)
{
    // This window only has the one button on it, so any call to this
    // is from clicking that button and we know what to do.
    instr_container->Show();
    this->Hide();
    return true;
}

pawsTutorialWindow::pawsTutorialWindow()
{
}

pawsTutorialWindow::~pawsTutorialWindow()
{
}

bool pawsTutorialWindow::PostSetup()
{
    // name of widget found here must match widget found in TutorialNotifyWindow
    instructions = dynamic_cast<pawsMultiLineTextBox*> (FindWidget("TutorialInstructions"));
    if ( !instructions ) 
        return false;

    return true;
}

void pawsTutorialWindow::Close()
{
    //Since there is only one button to press we can easily just close the
    //window.
    Hide();
    PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );

    notifywnd = dynamic_cast<pawsTutorialNotifyWindow*> (psengine->GetMainWidget()->FindWidget("TutorialNotify"));
    if ( !notifywnd) 
        return;

    notifywnd->NextTutorial();

    return;
}

