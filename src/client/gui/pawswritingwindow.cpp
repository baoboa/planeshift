/**
 * pawswritingwindow.cpp - Author: Daniel Fryer
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
 
#include <psconfig.h>

#include <iutil/databuff.h>

#include "globals.h"

#include "pawswritingwindow.h"
// PAWS INCLUDES

#include "paws/pawsmanager.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/log.h"

#define CANCEL 101
#define SAVE   103
#define LOAD   104

pawsWritingWindow::pawsWritingWindow(){

}

pawsWritingWindow::~pawsWritingWindow(){
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_WRITE_BOOK);
}

bool pawsWritingWindow::PostSetup()
{
    
    lefttext = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("body"));
    name = dynamic_cast<pawsEditTextBox*> (FindWidget("ItemName"));
    if ( !name || !lefttext) return false;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_WRITE_BOOK);

    return true;
}


void pawsWritingWindow::HandleMessage( MsgEntry* me )
{   
    psWriteBookMessage mesg(me);
    if (mesg.messagetype == mesg.RESPONSE)
    {
        slotID = mesg.slotID;
        containerID = mesg.containerID;
        //if I wasn't retarded I"d just keep mesg around instead
        if (mesg.success)
        {
            name->SetText(mesg.title);
            lefttext->SetText(mesg.content);
            Show();
        }
        else
        {
            //this is bogus of course
            name->SetText(mesg.title);
            lefttext->SetText(mesg.content);
            Show();
        }
    }
    else if (mesg.messagetype == mesg.SAVERESPONSE)
    {
        csString saveReportStr;
        uint32_t msgType;
        if (mesg.success)
        {
            saveReportStr.Format("You have written in \"%s\"", mesg.title.GetDataSafe());
            msgType = MSG_INFO;
            Hide();
            PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
        }
        else
        {
            // currently, only fails here if text is too long for the database
            saveReportStr.Format("You failed to write in \"%s\". It is too long.", mesg.title.GetDataSafe());
            msgType = MSG_ERROR;
        }
        psSystemMessage saveReport(0, msgType, saveReportStr);
        saveReport.FireEvent();
    }
}

//When we close the window, we should send some sort of save or cancel message in case server
//is tracking current writing task.  Right now it doesn't.
bool pawsWritingWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch(widget->GetID())
    {
        case SAVE:
        {
            csString title = name->GetText();
            size_t titleLength = title.Length();
            if (titleLength == 0 || titleLength > MAX_TITLE_LEN)
            {
                csString titleError;
                titleError.Format("Title should be between 1 and %d characters", MAX_TITLE_LEN);
                psSystemMessage error(0, MSG_ERROR, titleError);
                error.FireEvent();
                return false;
            }
            csString contents = lefttext->GetText();
            size_t totalBookLength = contents.Length() + titleLength + 15; // still too big to store in DB it seems
            if (totalBookLength > MAX_MESSAGE_SIZE)
            {
                csString contentError;
                contentError.Format("Book text is too long");
                psSystemMessage error(0, MSG_ERROR, contentError);
                error.FireEvent();
                return false;
            }

            psWriteBookMessage mesg(slotID, containerID, title, contents);
            mesg.SendMessage();
            return true;
        }

        case CANCEL:
        {
            // close the write window
            Hide();
            PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );            

            return true;
        }
        case LOAD:
        {
            pawsStringPromptWindow::Create("Enter local filename", "", false, 220, 20, this, "FileNameBox", 0, true);
        }
    }

    return false;
}

void pawsWritingWindow::OnStringEntered(const char* /*name*/, int /*param*/, const char* value)
{
    const unsigned int MAX_BOOK_FILE_SIZE = 60000;

    csString fileName;
    iVFS* vfs = psengine->GetVFS();
    if (!value)
        return;

    fileName.Format("/planeshift/userdata/books/%s", value);
    if (!vfs->Exists(fileName))
    {
        psSystemMessage msg(0, MSG_ERROR, "File %s not found!", fileName.GetDataSafe());
        msg.FireEvent();
        return;
    }    
    csRef<iDataBuffer> data = vfs->ReadFile(fileName);
    if (data->GetSize() > MAX_BOOK_FILE_SIZE)
    {
        psSystemMessage msg(0, MSG_ERROR, "File too big, data trimmed to %d chars.", MAX_BOOK_FILE_SIZE );
        msg.FireEvent();
    }        
    csString book(data->GetData(), csMin(data->GetSize(), (size_t)MAX_BOOK_FILE_SIZE));
    book.ReplaceAll("\r\n", "\n");
    lefttext->SetText(book, true);
    
    psSystemMessage msg(0, MSG_ACK, "Book Loaded Successfully!" );
    msg.FireEvent();
}

