/*
 * pawsscripthirednpcwindow.h  creator <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================
#include "net/clientmsghandler.h"
#include "paws/pawstextbox.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "../globals.h"
#include "pawsscripthirednpcwindow.h"

// Define a button ID values
#define VERIFY_BUTTON        101
#define COMMIT_BUTTON        102
#define LOAD_BUTTON          103
#define CANCEL_BUTTON        104
#define WORK_LOCATION_BUTTON 105
#define SAVE_BUTTON          106

pawsScriptHiredNPCWindow::pawsScriptHiredNPCWindow():
    verified(false),script(NULL),verifyButton(NULL),okButton(NULL)
{
    // Subscribe to messages.
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_HIRED_NPC_SCRIPT);
}

pawsScriptHiredNPCWindow::~pawsScriptHiredNPCWindow()
{
    // Unsubscribe from messages.
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_HIRED_NPC_SCRIPT);
}

bool pawsScriptHiredNPCWindow::PostSetup()
{
    script = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("script"));
    if(!script) return false;

    verifyButton = dynamic_cast<pawsButton*>(FindWidget("verify"));
    if(!verifyButton) return false;

    okButton = dynamic_cast<pawsButton*>(FindWidget("commit"));
    if(!okButton) return false;

    okButton->SetEnabled(verified);
    verifyButton->SetEnabled(!verified);

    PawsManager::GetSingleton().Publish("sigHiredScriptStatus", "Not verified.");
    PawsManager::GetSingleton().Publish("sigHiredScriptError",  "");
    PawsManager::GetSingleton().Publish("sigHiredWorkPosition", "");
    PawsManager::GetSingleton().Publish("sigHiredWorkPositionValid", "");

    return true;
}

void pawsScriptHiredNPCWindow::HandleMessage(MsgEntry* me)
{

    if(me->GetType() == MSGTYPE_HIRED_NPC_SCRIPT)
    {
        psHiredNPCScriptMessage msg(me);
        HandleHiredNPCScript(msg);
    }
}

void pawsScriptHiredNPCWindow::HandleHiredNPCScript(const psHiredNPCScriptMessage &msg)
{
    switch(msg.command)
    {
        case psHiredNPCScriptMessage::COMMIT_REPLY:
            hiredEID = msg.hiredEID;

            SetVerified(msg.choice);

            PawsManager::GetSingleton().Publish("sigHiredScriptStatus", verified?"OK":"Not OK");
            PawsManager::GetSingleton().Publish("sigHiredScriptError", msg.errorMessage);

            if(verified)
            {
                // Close the window
                Hide();
                PawsManager::GetSingleton().SetCurrentFocusedWidget(NULL);
            }
            break;
        case psHiredNPCScriptMessage::REQUEST_REPLY:
            hiredEID = msg.hiredEID;
            script->SetText(msg.script);
            PawsManager::GetSingleton().Publish("sigHiredWorkPosition", msg.workLocation);
            PawsManager::GetSingleton().Publish("sigHiredWorkPositionValid",msg.workLocationValid?"OK":"Failed");
            Show(); // Show window upon reception of a script from server
            break;
        case psHiredNPCScriptMessage::VERIFY_REPLY:
            hiredEID = msg.hiredEID;

            SetVerified(msg.choice);

            PawsManager::GetSingleton().Publish("sigHiredScriptStatus", verified?"OK":"Not OK");
            PawsManager::GetSingleton().Publish("sigHiredScriptError", msg.errorMessage);
            break;
        case psHiredNPCScriptMessage::WORK_LOCATION_UPDATE:
            PawsManager::GetSingleton().Publish("sigHiredWorkPosition",msg.workLocation);
            break;
        case psHiredNPCScriptMessage::WORK_LOCATION_RESULT:
            PawsManager::GetSingleton().Publish("sigHiredWorkPositionValid",msg.choice?"OK":"Failed");
            break;
    }
}

bool pawsScriptHiredNPCWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch(widget->GetID())
    {
        case VERIFY_BUTTON:
        {
            psHiredNPCScriptMessage msg(0, psHiredNPCScriptMessage::VERIFY, hiredEID,
                                        script->GetText()?script->GetText():"");
            msg.SendMessage();
            return true;
        }
        case COMMIT_BUTTON:
        {
            // Send message to server that the hire script was confirmed.
            psHiredNPCScriptMessage msg(0, psHiredNPCScriptMessage::COMMIT, hiredEID);
            msg.SendMessage();

            // COMMIT_REPLY will close the dialoge if commited ok.
            return true;
        }
        case CANCEL_BUTTON:
        {
            psHiredNPCScriptMessage msg(0, psHiredNPCScriptMessage::CANCEL, hiredEID);
            msg.SendMessage();

            // close the window
            Hide();
            PawsManager::GetSingleton().SetCurrentFocusedWidget(NULL);
            return true;
        }
        case LOAD_BUTTON:
        {
            pawsStringPromptWindow::Create("Enter local filename", "", false, 220, 20, this,
                                           "LoadFileNameBox", 0, true);
            return true;
        }
        case SAVE_BUTTON:
        {
            pawsStringPromptWindow::Create("Enter local filename", "", false, 220, 20, this,
                                           "SaveFileNameBox", 0, true);
            return true;
        }
        case WORK_LOCATION_BUTTON:
        {
            psHiredNPCScriptMessage msg(0, psHiredNPCScriptMessage::WORK_LOCATION, hiredEID);
            msg.SendMessage();

            PawsManager::GetSingleton().Publish("sigHiredWorkPositionValid","");

            return true;
        }
    }

    return false;
}

void pawsScriptHiredNPCWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    const unsigned int MAX_SCRIPT_FILE_SIZE = 6000;

    csString fileName;
    iVFS* vfs = psengine->GetVFS();
    if(!value)
        return;

    fileName.Format("/planeshift/userdata/scripts/%s", value);

    if (strcmp(name,"LoadFileNameBox") == 0)
    {
        if(!vfs->Exists(fileName))
        {
            psSystemMessage msg(0, MSG_ERROR, "File %s not found!", fileName.GetDataSafe());
            msg.FireEvent();
            return;
        }
        csRef<iDataBuffer> data = vfs->ReadFile(fileName);
        if(data->GetSize() > MAX_SCRIPT_FILE_SIZE)
        {
            psSystemMessage msg(0, MSG_ERROR, "File too big, data trimmed to %d chars.", MAX_SCRIPT_FILE_SIZE);
            msg.FireEvent();
        }
        csString newScript(data->GetData(), csMin(data->GetSize(), (size_t)MAX_SCRIPT_FILE_SIZE));
        
        newScript.ReplaceAll("\r\n", "\n");
        script->SetText(newScript);
        
        SetVerified(false);
        
        PawsManager::GetSingleton().Publish("sigHiredScriptStatus", "Not verified.");
        PawsManager::GetSingleton().Publish("sigHiredScriptError", "");
        
        psSystemMessage msg(0, MSG_ACK, "Script Loaded Successfully!");
        msg.FireEvent();
    }
    else if (strcmp(name,"SaveFileNameBox") == 0)
    {
        csString newScript = script->GetText();
        newScript.ReplaceAll("\n", "\r\n");

        if (!vfs->WriteFile(fileName,newScript.GetDataSafe(),
                            strlen(newScript.GetDataSafe())))
        {
            psSystemMessage msg(0, MSG_ERROR, "Failed to write file.");
            msg.FireEvent();
        }
        else
        {
        psSystemMessage msg(0, MSG_ACK, "Script Saved Successfully!");
        msg.FireEvent();
            
        }
    }
}

bool pawsScriptHiredNPCWindow::OnChange(pawsWidget* widget)
{
    if(widget == script)
    {
        // Script changes so now it is not verified anymore.
        SetVerified(false);

        PawsManager::GetSingleton().Publish("sigHiredScriptStatus", "Not verified.");
        PawsManager::GetSingleton().Publish("sigHiredScriptError", "");
    }

    return true;
}

void pawsScriptHiredNPCWindow::SetVerified(bool status)
{
    verified = status;

    okButton->SetEnabled(verified);
    verifyButton->SetEnabled(!verified);
}
