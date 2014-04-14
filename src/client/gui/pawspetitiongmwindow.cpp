/*
* pawscontrolwindow.cpp - Author: Alexander Wiseman
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

#include "pscelclient.h"

// PAWS INCLUDES
#include "pawspetitionwindow.h"
#include "pawspetitiongmwindow.h"
#include "paws/pawsmainwidget.h"

#define QUIT_BUTTON         1000
#define ASSIGN_BUTTON       2
#define DEASSIGN_BUTTON     3
#define ESCALATE_BUTTON     4
#define DESCALATE_BUTTON    5
#define CLOSE_BUTTON        6
#define CANCEL_BUTTON       7
#define ADMIN_BUTTON        8

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsPetitionGMWindow::pawsPetitionGMWindow()
: psCmdBase( NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry() )
{
}

pawsPetitionGMWindow::~pawsPetitionGMWindow()
{
    msgqueue->Unsubscribe(this, MSGTYPE_PETITION);
}

bool pawsPetitionGMWindow::PostSetup()
{
    // Setup this widget to receive messages and commands
    if ( !psCmdBase::Setup( psengine->GetMsgHandler(), psengine->GetCmdHandler()) )
        return false;

    // Subscribe to certain types of messages (those we want to handle)
    msgqueue->Subscribe(this, MSGTYPE_PETITION);

    // Grab the pointer to the petition listbox:
    petitionList = (pawsListBox*)FindWidget("PetitionList");

    petText = (pawsMessageTextBox*)FindWidget("PetitionText");

    hasPetInterest = false;

    petCount = -1; //default value: we didn't receive petitions yet

    //TODO: this should be removed when autocompletion autocompletes /show
    // If player is GM, give him the /petition_manage command
    if ( psengine->GetCelClient()->GetMainPlayer()->GetType() > 20 )
    {
        cmdsource->Subscribe("/managepetitions", this);
        hasPetInterest = true;   //gm are going to want to get informations about the petitions
        QueryServer();           //Query the server for messages
    }

    petitionList->SetSortingFunc(0, textBoxSortFunc);
    petitionList->SetSortingFunc(1, textBoxSortFunc);
    petitionList->SetSortingFunc(2, textBoxSortFunc);
    petitionList->SetSortingFunc(3, textBoxSortFunc);
    petitionList->SetSortingFunc(4, textBoxSortFunc);
    petitionList->SetSortingFunc(5, textBoxSortFunc);
    petitionList->SetSortingFunc(6, textBoxSortFunc);

    return true;
}

//////////////////////////////////////////////////////////////////////
// Command and Message Handling
//////////////////////////////////////////////////////////////////////

//TODO: To be removed when autocompletion for /show is implemented
const char* pawsPetitionGMWindow::HandleCommand( const char* cmd )
{
    WordArray words (cmd, false);

    // Check which command was invoked:
    if ( words[0].CompareNoCase("/managepetitions") )
    {
        //display this window to the user
        Show();
    }

    return NULL;
}

void pawsPetitionGMWindow::Show()
{
    if(psengine->GetCelClient()->GetMainPlayer()->GetType() > 20) //allow access only to who has really access
    {
        hasPetInterest = true;
        pawsWidget::Show();       //show the window
        QueryServer();           //Query the server for messages
    }
}

void pawsPetitionGMWindow::HandleMessage ( MsgEntry* me )
{
    Debug1(LOG_PAWS, 0, "Incoming Petition GM Petition Message");
    // Get the petition message data from the server
    psPetitionMessage message(me);

    if (!message.isGM)
    {
        Debug1(LOG_PAWS, 0, "Bailing out because of message.isGM");
        return;
    }

    if (message.msgType == PETITION_DIRTY && psengine->GetCelClient()->GetMainPlayer()->GetType() > 20)
    {
        if (hasPetInterest)
        {
            Debug1(LOG_PAWS, 0, "Looking for pet interest and bailing out");
            QueryServer();
        }
        return;
    }

    // if the server gave us a list of petitions, then use it
//    if (message.petitions.GetSize() > 0)
    petitionMessage = message;

    // Check if server reported errors on the query
    if (!message.success)
    {
        printf("Message was not a success\n");
        psSystemMessage error(0,MSG_INFO,message.error);
        msgqueue->Publish(error.msg);

        QueryServer(); //request the server a new petition list
        return;
    }

    // Check no petitions:
    if (message.petitions.GetSize() == 0)
    {
        //printf("No petitions in list\n");
        petCount = 0;
        petitionList->Clear();
        petitionList->NewRow(0);
        SetText(0, 1, PawsManager::GetSingleton().Translate("No Petitions"));
        return;
    }

    // Check its type (list, confirm, or assign):
    if (message.msgType == PETITION_CANCEL || message.msgType == PETITION_CLOSE)
    {
        // Check if server reported errors on the query
        if (!message.success)
        {
            Debug1(LOG_PAWS, 0, "!message.success");
            psSystemMessage error(0,MSG_INFO,message.error);
            msgqueue->Publish(error.msg);
            return;
        }

        // Change the status of the position we wanted to cancel:
        if (currentRow < 0)
        {
            // This should NEVER happen (it means the server gave a bogus message)
            Error1("PetitionGMWindow reports invalid currentRow!");
            return;
        }

        // Remove the item and refresh the list:
//        petitionMessage.petitions.DeleteIndex(currentRow);

        AddPetitions(petitionMessage.petitions);

        petitionList->Select(petitionList->GetRow(currentRow));

        csString report;
        csString translation1 = PawsManager::GetSingleton().Translate("The selected petition was %s");
        csString translation2 = PawsManager::GetSingleton().Translate(message.msgType == PETITION_CANCEL ? "cancelled.":"closed.");
        report.Format(translation1.GetData(), translation2.GetData());
        psSystemMessage confirm(0,MSG_INFO,report.GetData());
        msgqueue->Publish(confirm.msg);


        return;
    }

    if (message.msgType == PETITION_ASSIGN)
    {
        // Check if server reported errors on the query
        if (!message.success)
        {
            Error1("Assign fail\n");
            psSystemMessage error(0,MSG_INFO,message.error);
            msgqueue->Publish(error.msg);
            return;
        }

        // Change the status of the position we wanted to assign:
        if (currentRow < 0)
        {
            // This should NEVER happen (it means the server gave a bogus message)
            Error1("PetitionGMWindow reports invalid currentRow!");
            return;
        }

        // Change the status of the item in the list box:
//        petitionMessage.petitions.Get(currentRow).status = "In Progress";
        AddPetitions(petitionMessage.petitions);

        psSystemMessage confirm(0,MSG_INFO,PawsManager::GetSingleton().Translate("You have been assigned to the selected petition."));
        msgqueue->Publish(confirm.msg);

        return;
    }

    if (message.msgType == PETITION_DEASSIGN)
    {
        // Check if server reported errors on the query
        if (!message.success)
        {
            Error1("deassign fail\n");
            psSystemMessage error(0,MSG_INFO,message.error);
            msgqueue->Publish(error.msg);
            return;
        }

        // Change the status of the position we wanted to assign:
        if (currentRow < 0)
        {
            // This should NEVER happen (it means the server gave a bogus message)
            Error1("PetitionGMWindow reports invalid currentRow!");
            return;
        }

        // Change the status of the item in the list box:
        AddPetitions(petitionMessage.petitions);

        psSystemMessage confirm(0,MSG_INFO,PawsManager::GetSingleton().Translate("You have been deassigned from the selected petition."));
        msgqueue->Publish(confirm.msg);

        return;
    }

    if (message.msgType == PETITION_ESCALATE)
    {
        // Check if server reported errors on the query
        if (!message.success)
        {
            psSystemMessage error(0,MSG_INFO,message.error);
            msgqueue->Publish(error.msg);
            return;
        }

        // Change the status of the position we wanted to assign:
        if (currentRow < 0)
        {
            // This should NEVER happen (it means the server gave a bogus message)
            Error1("PetitionGMWindow reports invalid currentRow!");
            return;
        }

        // Change the status of the item in the list box:
//        petitionMessage.petitions.Get(currentRow).escalation += 1;
//        petitionMessage.petitions.Get(currentRow).status = "Open";
        AddPetitions(petitionMessage.petitions);

        psSystemMessage confirm(0,MSG_INFO,PawsManager::GetSingleton().Translate("The selected petition has been escalated."));
        msgqueue->Publish(confirm.msg);

        return;
    }
    if (message.msgType == PETITION_DESCALATE)
    {
        // Check if server reported errors on the query
        if (!message.success)
        {
            psSystemMessage error(0,MSG_INFO,message.error);
            msgqueue->Publish(error.msg);
            return;
        }

        // Change the status of the position we wanted to assign:
        if (currentRow < 0)
        {
            // This should NEVER happen (it means the server gave a bogus message)
            Error1("PetitionGMWindow reports invalid currentRow!");
            return;
        }

        // Change the status of the item in the list box:
        AddPetitions(petitionMessage.petitions);

        psSystemMessage confirm(0,MSG_INFO,PawsManager::GetSingleton().Translate("The selected petition has been descalated."));
        msgqueue->Publish(confirm.msg);

        return;
    }

    // Update the listbox:
    int tempCount = petCount;
    AddPetitions(message.petitions);
    petitionList->SortRows();   ///sort the column specified in xml

    //alert GM that there are petitions waiting
    if (tempCount == -1)
    {
        csString message = PawsManager::GetSingleton().Translate("There are ");
        message += petCount;
        message += PawsManager::GetSingleton().Translate(" unanswered petitions."); //make a nice message

        psSystemMessage alert(0,MSG_INFO,message); //and send it
        msgqueue->Publish(alert.msg);
    }
    else if (petCount > tempCount)
    {
        psPetitionInfo info = message.petitions.Get(petCount-1); //get the last petition (supposed to be the new one)
        csString message = PawsManager::GetSingleton().Translate("New petition from ") + info.player.GetDataSafe(); //make a nice message

        psSystemMessage alert(0,MSG_INFO,message); //and send it
        msgqueue->Publish(alert.msg);
    }
}

bool pawsPetitionGMWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    // We know that the calling widget is a button.
    int button = widget->GetID();

    switch( button )
    {
        case QUIT_BUTTON:
        {
            // Hide our petition window
            petitionList->Clear();
            Hide();
            break;
        }

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

                if (csString("In Progress") == petitionList->GetSelectedText(PGMCOL_STATUS)
                    && ((csString)psengine->GetMainPlayerName()).StartsWith(petitionList->GetSelectedText(PGMCOL_GM)))
                {

                    currentRow = sel;

                    // Send a message to the server requesting cancel
                    psPetitionRequestMessage queryMsg(true, "cancel", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id);
                    msgqueue->SendMessage(queryMsg.msg);
                }
                else
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must be assigned to a petition in order to cancel it."));
                    msgqueue->Publish(error.msg);
                }
            }
            break;
        }

        case ASSIGN_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must select a petition from the list in order to assign yourself."));
                    msgqueue->Publish(error.msg);
                    return true;
                }

                if (csString("Open") == petitionList->GetSelectedText(PGMCOL_STATUS))
                {
                    currentRow = sel;

                    // Send a message to the server requesting assignment:
                    psPetitionRequestMessage queryMsg(true, "assign", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id);
                    msgqueue->SendMessage(queryMsg.msg);
                }
                else
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("The selected petition isn't Open."));
                    msgqueue->Publish(error.msg);
                }
            }
            break;
        }

        case DEASSIGN_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must select a petition from the list in order to deassign yourself."));
                    msgqueue->Publish(error.msg);
                    return true;
                }

                if ((csString("In Progress") == petitionList->GetSelectedText(PGMCOL_STATUS)
                    && ((csString)psengine->GetMainPlayerName()).StartsWith(petitionList->GetSelectedText(PGMCOL_GM)))
                    || psengine->GetCelClient()->GetMainPlayer()->GetType() >= 25)
                {
                    currentRow = sel;

                    // Send a message to the server requesting deassignment:
                    psPetitionRequestMessage queryMsg(true, "deassign", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id);
                    msgqueue->SendMessage(queryMsg.msg);
                }
                else
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("The selected petition isn't assigned."));
                    msgqueue->Publish(error.msg);
                }
            }
            break;
        }

        case CLOSE_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must select a petition from the list in order to close it."));
                    msgqueue->Publish(error.msg);
                    return true;
                }

                if (csString("In Progress") == petitionList->GetSelectedText(PGMCOL_STATUS)
                    && ((csString)psengine->GetMainPlayerName()).StartsWith(petitionList->GetSelectedText(PGMCOL_GM)))
                {
                    currentRow = sel;

                    pawsStringPromptWindow::Create(PawsManager::GetSingleton().Translate("Describe the petition and how you've helped"), "",
                                                   true, 400, 60, this, "CloseDesc");
                }
                else
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must be assigned to a petition in order to close it."));
                    msgqueue->Publish(error.msg);
                }
            }
            break;
        }

        case ESCALATE_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must select a petition from the list in order to escalate it."));
                    msgqueue->Publish(error.msg);
                    return true;
                }

                if (psengine->GetCelClient()->GetMainPlayer()->GetType() + 1 >= atoi(petitionList->GetSelectedText(PGMCOL_LVL)))
                {
                    int level = atoi(petitionList->GetSelectedText(PGMCOL_LVL));
                    if ( level == 10 )
                    {
                        psSystemMessage error(0,MSG_INFO,"Cannot escalate above 10");
                        msgqueue->Publish(error.msg);
                        return true;
                    }

                    currentRow = sel;

                    // Send a message to the server requesting assignment:
                    psPetitionRequestMessage queryMsg(true, "escalate", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id);
                    msgqueue->SendMessage(queryMsg.msg);
                }
            }
            break;
        }

        case DESCALATE_BUTTON:
        {
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                {
                    psSystemMessage error(0,MSG_INFO,PawsManager::GetSingleton().Translate("You must select a petition from the list in order to descalate it."));
                    msgqueue->Publish(error.msg);
                    return true;
                }

                if (psengine->GetCelClient()->GetMainPlayer()->GetType() + 1 >= atoi(petitionList->GetSelectedText(PGMCOL_LVL)))
                {
                    int level = atoi(petitionList->GetSelectedText(PGMCOL_LVL));
                    if ( level == 0 )
                    {
                        psSystemMessage error(0,MSG_INFO,"Cannot descalate below 0");
                        msgqueue->Publish(error.msg);
                        return true;
                    }

                    currentRow = sel;

                    // Send a message to the server requesting assignment:
                    psPetitionRequestMessage queryMsg(true, "descalate", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id);
                    msgqueue->SendMessage(queryMsg.msg);
                }
            }
            break;
        }

        case ADMIN_BUTTON:
        {
            // store the target petitioner data
            if (petCount > 0)
            {
                // Get the currently selected row:
                int sel = petitionList->GetSelectedRowNum();
                if (sel < 0)
                    psengine->SetTargetPetitioner("None");
                else
                {
                    if (csString("In Progress") == petitionList->GetSelectedText(PGMCOL_STATUS))
                        psengine->SetTargetPetitioner(petitionList->GetSelectedText(PGMCOL_PLAYER));
                    else
                        psengine->SetTargetPetitioner("None");
                }
            }
            else
                psengine->SetTargetPetitioner("None");

            // load the gm gui so that the gm can access functions to aid the pet
            const char* errorMessage = cmdsource->Publish( "/show gm" );
            if ( errorMessage )
            {
                psSystemMessage error(0,MSG_INFO,errorMessage);
                msgqueue->Publish(error.msg);
            }
        }
    }

    return true;
}

void pawsPetitionGMWindow::OnStringEntered(const char* /*name*/, int /*param*/, const char* value)
{
    if (value && strlen(value))
        CloseCurrPetition(value);
}

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////

void pawsPetitionGMWindow::CloseCurrPetition(const char *desc)
{
    // Send a message to the server requesting assignment:
    psPetitionRequestMessage queryMsg(true, "close", petitionMessage.petitions.Get(petitionList->GetRow(currentRow)->GetLastIndex()).id, desc);
    msgqueue->SendMessage(queryMsg.msg);
}

void pawsPetitionGMWindow::QueryServer()
{
    // Construct the special petition request message to ask the server for info
    psPetitionRequestMessage queryMsg(true, "query");
    msgqueue->SendMessage(queryMsg.msg);
}

void pawsPetitionGMWindow::SetText(size_t rowNum, int colNum, const char* fmt, ...)
{
    pawsListBoxRow *row = petitionList->GetRow(rowNum);
    if (!row)
        return;

    pawsTextBox *curCol = (pawsTextBox*)row->GetColumn(colNum);
    if (!curCol)
        return;

    char text[1024];
    va_list args;

    va_start(args, fmt);
    cs_vsnprintf(text,sizeof(text),fmt,args);
    va_end(args);

    // Check text overflow:
    csString overflow = text;
    if (overflow.Length() > MAX_GMPETITION_LENGTH)
    {
        overflow.Truncate(MAX_GMPETITION_LENGTH);
        overflow << "...";
    }

    curCol->SetText(overflow.GetData());
}


void pawsPetitionGMWindow::AddPetitions(csArray<psPetitionInfo> &petitions)
{
    // figure out the selected petition


    if (petitionList->GetRowCount() <= 0)
        petitionList->Select(NULL);

    int selRow = petitionList->GetSelectedRowNum();
    if (selRow != -1)
    {
        csString escalation = petitionList->GetTextCellValue(selRow, PGMCOL_LVL);
        if (escalation.GetData() != NULL)
            selectedPet.escalation = atoi(escalation.GetData());
        else
            selectedPet.escalation = -1;
        selectedPet.assignedgm =      petitionList->GetTextCellValue(selRow, PGMCOL_GM);
        selectedPet.player =          petitionList->GetTextCellValue(selRow, PGMCOL_PLAYER);
        selectedPet.status =          petitionList->GetTextCellValue(selRow, PGMCOL_STATUS);
        selectedPet.created =         petitionList->GetTextCellValue(selRow, PGMCOL_CREATED);
        selectedPet.petition =        petitionList->GetTextCellValue(selRow, PGMCOL_PETITION);
        selectedPet.online =          petitionList->GetTextCellValue(selRow, PGMCOL_ONLINE) == "yes";
    }
    else
    {
        selectedPet.escalation = -1;
    }
    petitionList->Select(NULL);

    // Clear the list box and add the user's petitions
    petitionList->Clear();
    psPetitionInfo info;
    petCount = 0;
    for (size_t i = 0; i < petitions.GetSize(); i++)
    {
        info = petitions.Get(i);
        petCount++;

        // Set the data for this row:
        petitionList->NewRow(i);
        SetText(i, PGMCOL_LVL, "%d", info.escalation);
        SetText(i, PGMCOL_GM, "%s", info.assignedgm.GetDataSafe());
        SetText(i, PGMCOL_PLAYER, "%s", info.player.GetData());
        SetText(i, PGMCOL_ONLINE, "%s", (info.online ? "yes" : "no"));
        SetText(i, PGMCOL_STATUS, "%s", info.status.GetData());
        SetText(i, PGMCOL_CREATED, "%s", info.created.GetData());
        SetText(i, PGMCOL_PETITION, "%s", info.petition.GetData());

        // reselect the petition
        if (selectedPet.escalation != -1)
        {
            if (selectedPet.created == info.created
             && selectedPet.player == info.player)
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


void pawsPetitionGMWindow::OnListAction(pawsListBox* /*selected*/, int status)
{
    if (status == LISTBOX_HIGHLIGHTED)
    {
        size_t sel = petitionList->GetSelectedRowNum();
        if (sel >= petitionMessage.petitions.GetSize())
            return;
        petText->Clear();
        petText->AddMessage(petitionMessage.petitions.Get(petitionList->GetRow(sel)->GetLastIndex()).petition);
    }
}

