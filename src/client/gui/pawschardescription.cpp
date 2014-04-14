/*
 * pawschardescription.cpp - Author: Christian Svensson
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
#include "net/clientmsghandler.h"
#include "net/messages.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"

// PAWS INCLUDES
#include "pawschardescription.h"
#include "pawsdetailwindow.h"
#include "paws/pawsmanager.h"

#define BTN_OK     100
#define BTN_CANCEL 101
#define BTN_SAVE   102
#define BTN_LOAD   103

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsCharDescription::pawsCharDescription()
{
    editingtype = DESC_IC;
}

pawsCharDescription::~pawsCharDescription()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_CHARACTERDETAILS);
}

bool pawsCharDescription::PostSetup()
{
    description = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget( "Description" ));
    if ( !description )
        return false;

    return true;
}

void pawsCharDescription::Show()
{
    RequestDetails();
    pawsWidget::Show();
}

void pawsCharDescription::RequestDetails()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHARACTERDETAILS);

    psCharacterDetailsRequestMessage requestMsg(true, true, "pawsCharDescription");
    requestMsg.SendMessage();
}

void pawsCharDescription::HandleMessage( MsgEntry* me )
{

    if (me->GetType() == MSGTYPE_CHARACTERDETAILS)
    {
        psCharacterDetailsMessage msg(me);

        if (msg.requestor == "pawsCharDescription")
        {
            if(editingtype == DESC_IC)
                description->SetText(msg.desc);
            else if(editingtype == DESC_OOC)
                description->SetText(msg.desc_ooc);
            else
                description->SetText(msg.creationinfo);
                
            psengine->GetMsgHandler()->Unsubscribe( this, MSGTYPE_CHARACTERDETAILS );
        }
        return;
    }
}

bool pawsCharDescription::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {
        case BTN_OK:
        {
            csString newTxt(description->GetText());
            psCharacterDescriptionUpdateMessage descUpdate(newTxt,editingtype);
            psengine->GetMsgHandler()->SendMessage(descUpdate.msg);
            Hide();

            // Show the details window to let the user see the changes
            pawsDetailWindow *detailWindow = (pawsDetailWindow*) PawsManager::GetSingleton().FindWidget("DetailWindow");
            if(detailWindow)
                detailWindow->RequestDetails();
            return true;
        }

        case BTN_CANCEL:
        {
            this->Close();
            return true;
        }
        case BTN_SAVE:
        {
            iVFS* vfs = psengine->GetVFS();
            csString FileName; //stores the filename to use for saving: char_name_description.txt
            csString DescriptionData; //the data taken from char description

            //make up the filename
            FileName.Format("/planeshift/userdata/descriptions/%s_description.txt", psengine->GetMainPlayerName());
            FileName.ReplaceAll(" ", "_");

            DescriptionData = description->GetText(); //get the text to save
            #ifdef _WIN32
            DescriptionData.ReplaceAll("\n", "\r\n"); //replace new lines with newlines+linefeeds for windows
            #endif
            vfs->WriteFile(FileName, DescriptionData, DescriptionData.Length()); //write the file to disk

            //show the success with the filename excluding the path to it
            psSystemMessage msg(0, MSG_ACK, "Description saved to %s", FileName.GetData()+21 );
            msg.FireEvent();
            return true;
        }
        case BTN_LOAD:
        {
            iVFS* vfs = psengine->GetVFS();
            csString FileName; //stores the filename to use for loading: char_name_description.txt
            csString DescriptionData; //the data taken from char description

            //make up the filename
            FileName.Format("/planeshift/userdata/descriptions/%s_description.txt", psengine->GetMainPlayerName());
            FileName.ReplaceAll(" ", "_");

            if (!vfs->Exists(FileName)) //check if there is a file saved
            {
                psSystemMessage msg(0, MSG_ERROR, "File not found!" );
                msg.FireEvent();
                return true;
            }

            csRef<iDataBuffer> data = vfs->ReadFile(FileName); //load the file
            DescriptionData = data->GetData();
            DescriptionData.ReplaceAll("\r\n", "\n"); //remove linefeeds
            description->SetText(DescriptionData,true); //set the new text

            //show the success with the filename excluding the path to it
            psSystemMessage msg(0, MSG_ACK, "Description loaded from %s", FileName.GetData()+21 );
            msg.FireEvent();
            return true;
        }
    }
    return false;
}
