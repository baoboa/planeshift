/*
* pawspetitionwindow.cpp - Author: Alexander Wiseman
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
//////////////////////////////////////////////////////////////////////

// STANDARD INCLUDE
#include <psconfig.h>
#include "globals.h"

// COMMON/NET INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"
#include "util/strutil.h"
#include <ivideo/fontserv.h>

// PAWS INCLUDES
#include "pawspetitionwindow.h"
#include "paws/pawsprefmanager.h"
#include "paws/pawsmainwidget.h"
#include "gui/pawscontrolwindow.h"

// BUTTON IDs
#define CANCEL_BUTTON        1
#define NEW_BUTTON           2
#define SAVE_BUTTON          3

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsPetitionWindow::pawsPetitionWindow()
    : psCmdBase( NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry() )
{
    petitionList = NULL;
    currentRow = -1;
    petText = NULL;
    updateSelectedPetition = true;
}

pawsPetitionWindow::~pawsPetitionWindow()
{
    /*// Unsubscribe commands and messages
    msgqueue->Unsubscribe(this, MSGTYPE_PETITION);*/
}

bool pawsPetitionWindow::PostSetup()
{
    // Setup this widget to receive messages and commands
    if ( !psCmdBase::Setup( psengine->GetMsgHandler(), psengine->GetCmdHandler()) )
        return false;

    // Subscribe to certain types of messages (those we want to handle)
    msgqueue->Subscribe(this, MSGTYPE_PETITION);

    // Grab the pointer to the petition listbox:
    petitionList  = (pawsListBox*)FindWidget("PetitionList");

    petText = (pawsMultilineEditTextBox*)FindWidget("PetitionText");

    hasPetInterest = false;

    petitionList->SetSortingFunc(0, textBoxSortFunc);
    petitionList->SetSortingFunc(1, textBoxSortFunc);
    petitionList->SetSortingFunc(2, textBoxSortFunc);
    petitionList->SetSortingFunc(3, textBoxSortFunc);

    return true;
}

//////////////////////////////////////////////////////////////////////
// Command and Message Handling
//////////////////////////////////////////////////////////////////////

void pawsPetitionWindow::Show()
{
        pawsControlledWindow::Show(); //actually shows the window

        // Query the server for messages:
        hasPetInterest = true;
        QueryServer();
}

const char* pawsPetitionWindow::HandleCommand(const char* /*cmd*/)
{
    return NULL;
}

void pawsPetitionWindow::HandleMessage ( MsgEntry* me )
{
    // Get the petition message data from the server
    psPetitionMessage message(me);
    if (message.msgType == PETITION_DIRTY)
    {
        if (hasPetInterest)
            QueryServer();
        return;
    }
    if (message.isGM)
        return;

    petitionMessage = message;

    // Check if server reported errors on the query
    if (!message.success)
    {
        psSystemMessage error(0,MSG_INFO,message.error);
        msgqueue->Publish(error.msg);

        petCount = 0;
        petitionList->Clear();
        petText->Clear();
        petitionList->NewRow(0);
        SetText(0, 1, PawsManager::GetSingleton().Translate("No Petitions."));
        return;
    }

    // Check no petitions:
    if (message.petitions.GetSize() == 0)
    {
        petCount = 0;
        petitionList->Clear();
        petText->Clear();
        petitionList->NewRow(0);
        SetText(0, 1, PawsManager::GetSingleton().Translate("No Petitions."));
        return;
    }

    // Check its type (list or confirm):
    if (message.msgType == PETITION_CANCEL)
    {
        // Check if server reported errors on the query
        if (!message.success)
        {
            psSystemMessage error(0,MSG_INFO,message.error);
            msgqueue->Publish(error.msg);
            return;
        }

        // Change the status of the position we wanted to cancel:
        if (currentRow < 0)
        {
            // This should NEVER happen (it means the server gave a bogus message)
            Error1("PetitionWindow reports invalid currentRow!");
            return;
        }

        // Remove the item and refresh the list:
        //petitionMessage.petitions.DeleteIndex(petitionList->GetRow(currentRow)->GetLastIndex());
        AddPetitions(petitionMessage.petitions);
        petitionList->SelectByIndex(currentRow);

        psSystemMessage confirm(0,MSG_INFO,PawsManager::GetSingleton().Translate("The selected petition was cancelled."));
        msgqueue->Publish(confirm.msg);

        return;
    }
    // Update the listbox:
    int modifiedRow = -1;
    if (petitionList->GetSelectedRow())
        modifiedRow = petitionList->GetSelectedRow()->GetLastIndex(); 
    AddPetitions(message.petitions);
    petitionList->SortRows();   ///sort the column specified in xml
    if(updateSelectedPetition && modifiedRow != -1) // if the selected row needs a new selection (save/cancel)
    {
        for(size_t i = 0; i < petitionList->GetRowCount(); i++)
            if(petitionList->GetRow(i)->GetLastIndex()==modifiedRow)
                petitionList->SelectByIndex(i); // reselect the row we've modified
    }
}

void pawsPetitionWindow::Close()
{
    petitionList->Clear();
    hasPetInterest = true;
    pawsControlledWindow::Hide();
}

bool pawsPetitionWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    // We know that the calling widget is a button.
    int button = widget->GetID();
    updateSelectedPetition = false;
    
    switch( button )
    {
        case CANCEL_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must select a petition from the list in order to cancel it."));
                    msgqueue->Publish(error.msg);
                    return true;
                }

                currentRow = sel;
                updateSelectedPetition = true; // need an update

                // Send a message to the server requesting cancel
                psPetitionRequestMessage queryMsg(false, "cancel", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id);
                msgqueue->SendMessage(queryMsg.msg);
            }
            break;
        }
        case NEW_BUTTON:
        {
            // Popup Input Window
            pawsStringPromptWindow::Create(PawsManager::GetSingleton().Translate("Add Petition"), csString(""),
                true, 500, 300, this, "petition" );
            break;
        }
        case SAVE_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    // no petition selected, create new one
                    OnStringEntered (0, 0, petText->GetText());
                    return true;
                }

                currentRow = sel;
                updateSelectedPetition = true; // need an update

                // check petitions status
                if (petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).status != "Open")
                {
                    psSystemMessage error(0, MSG_INFO, "You can only change open petitions.");
                    msgqueue->Publish(error.msg);
                    return true;
                }

                // save changes to selected petition
                csString text(petText->GetText());
                text.Trim();

                if (!text.IsEmpty())
                {
                    // Send a message to the server requesting change
                    psPetitionRequestMessage changeMsg(false, "change", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id,petText->GetText());
                    msgqueue->SendMessage(changeMsg.msg);
                }
                else
                {
                    // Show an error
                    psSystemMessage error(0, MSG_INFO, "You must enter a text for the petition.");
                    msgqueue->Publish(error.msg);
                }
            }
            else
            {
                // no petition selected, create new one
                OnStringEntered (0, 0, petText->GetText());
            }
            break;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////

void pawsPetitionWindow::QueryServer()
{
    // Construct the special petition request message to ask the server for info
    psPetitionRequestMessage queryMsg(false, "query");
    msgqueue->SendMessage(queryMsg.msg);
}

void pawsPetitionWindow::SetText(size_t rowNum, int colNum, const char* fmt, ...)
{
    pawsListBoxRow *row = petitionList->GetRow(rowNum);
    if (!row)
    {
        return;
    }

    pawsTextBox *curCol = (pawsTextBox*)row->GetColumn(colNum);
    if (!curCol)
    {
        return;
    }

    csString text;
    va_list args;

    va_start(args, fmt);
    text.FormatV(fmt, args);
    va_end(args);

    // Check text overflow:
    if (text.Length() > MAX_PETITION_LENGTH)
    {
        text.Truncate(MAX_PETITION_LENGTH);
        text << "...";
    }

    curCol->SetText(text.GetData());
}

void pawsPetitionWindow::AddPetitions(csArray<psPetitionInfo> &petitions)
{
    // get info about the selected petition
    if (petitionList->GetRowCount() <= 0)
        petitionList->Select(NULL);

    if (petitionList->GetSelectedRowNum() >= 0)
    {
        selectedPet.escalation = 0;
        selectedPet.player =    petitionList->GetSelectedText(PCOL_GM);
        selectedPet.status =    petitionList->GetSelectedText(PCOL_STATUS);
        selectedPet.created =   petitionList->GetSelectedText(PCOL_CREATED);
        selectedPet.petition =  petitionList->GetSelectedText(PCOL_PETITION);
    }
    else
    {
        selectedPet.escalation = -1;
    }
    petitionList->Select(NULL);

    // Clear the list box and add the user's petitions
    petitionList->Clear();
    petText->Clear();

    psPetitionInfo info;
    petCount = 0;
    for (size_t i = 0; i < petitions.GetSize(); i++)
    {
        info = petitions.Get(i);
        petCount++;

        // Set the data for this row:
        petitionList->NewRow(i);
        SetText(i, PCOL_GM, "%s", info.assignedgm.GetData());
        SetText(i, PCOL_STATUS, "%s", info.status.GetData());
        SetText(i, PCOL_CREATED, "%s", info.created.GetData());
        // Add resolution info for Closed petitions
        if ( info.status=="Closed" )
        {
            SetText(i, PCOL_PETITION, "%s", (info.petition+(" "+PawsManager::GetSingleton().Translate("Resolution")+": "+info.resolution)).GetData());
        }
        else
        {
            SetText(i, PCOL_PETITION, "%s", info.petition.GetData());
        }


        // reselect the petition
        if (selectedPet.escalation != -1)
        {
            if (selectedPet.created == info.created)
            {
                petitionList->Select(petitionList->GetRow(i));
            }
        }
    }
    // make absolutely sure that there are petitions
    if (petCount <= 0)
    {
        petitionList->NewRow(0);
        SetText(0, 1, PawsManager::GetSingleton().Translate("No Petitions"));
    }
}

void pawsPetitionWindow::OnListAction(pawsListBox* /*selected*/, int status)
{
    if (status == LISTBOX_HIGHLIGHTED)
    {
        size_t sel = petitionList->GetSelectedRowNum();
        if ( sel >= petitionMessage.petitions.GetSize() )
        {
            return;
        }
        petText->Clear();
        size_t petitionIndex = petitionList->GetRow(sel)->GetLastIndex();
        if( petitionMessage.petitions.Get(petitionIndex).status=="Closed" )
        {
            petText->SetText(petitionMessage.petitions.Get(petitionIndex).petition+" "+PawsManager::GetSingleton().Translate("Resolution")+": "+petitionMessage.petitions.Get(petitionIndex).resolution );
        }
        else
        {
            petText->SetText(petitionMessage.petitions.Get(petitionIndex).petition);
        }
    }
}

void pawsPetitionWindow::OnStringEntered(const char* /*name*/, int /*param*/, const char* value)
{
    if (!value)
        return; // Cancel was clicked

    csString text(value);
    text.Trim();

    if (!text.IsEmpty())
    {
        csString command;
        command.Format("/petition %s", text.GetData());
        command.Collapse();
        psengine->GetCmdHandler()->Execute(command);
    }
    else
    {
        // Show an error
        psSystemMessage err(0, MSG_ERROR, "You must enter a text for the petition.");
        err.FireEvent();
    }
}
