/*
* pawsquestrewardwindow.cpp - Author: Ian Donderwinkel
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
#include "pscelclient.h"

// PAWS INCLUDES
#include "pawsquestrewardwindow.h"
#include "inventorywindow.h"
#include "pawsitemdescriptionwindow.h"
#include "pawsslot.h"

#define VIEW_BUTTON 1000
#define GET_BUTTON  1001

#define COLUMN_ICON 0
#define COLUMN_NAME 1
#define COLUMN_ID   2
#define COLUMN_DESC 3

pawsQuestRewardWindow::pawsQuestRewardWindow() 
    : psCmdBase( NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry() )
{
}

pawsQuestRewardWindow::~pawsQuestRewardWindow()
{
    msgqueue->Unsubscribe(this, MSGTYPE_QUESTREWARD);
}

bool pawsQuestRewardWindow::PostSetup()
{
    if ( !psCmdBase::Setup( psengine->GetMsgHandler(), psengine->GetCmdHandler()) )
        return false;

    msgqueue->Subscribe(this, MSGTYPE_QUESTREWARD);

    rewardList  = (pawsListBox*)FindWidget("QuestRewardList");

    return true;
}


const char* pawsQuestRewardWindow::HandleCommand(const char* /*cmd*/)
{
    return NULL;
}

void pawsQuestRewardWindow::HandleMessage ( MsgEntry* me )
{
    psQuestRewardMessage message(me);
    if (message.msgType==psQuestRewardMessage::offerRewards)
    {
        rewardList->Clear();
        rewardList->SelfPopulateXML(message.newValue);
        Show();
    }
}


bool pawsQuestRewardWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch( widget->GetID() )
    {
        case VIEW_BUTTON:
        {
            int row = rewardList->GetSelectedRowNum();
            if (row>=0)
            {
                // view item description (using pawsItemDescriptionWindow)
                pawsSlot* itemSlot = (pawsSlot*)rewardList->GetRow(row)->GetColumn(COLUMN_ICON);

                csString icon;
                if(itemSlot->Image())
                    icon = itemSlot->Image()->GetName();
                else
                    icon = "Standard Background";

                csString name = rewardList->GetTextCell(row, COLUMN_NAME)->GetText();
                csString desc = rewardList->GetTextCell(row, COLUMN_DESC)->GetText();

                pawsItemDescriptionWindow* itemDescWindow = (pawsItemDescriptionWindow*)PawsManager::GetSingleton().FindWidget("ItemDescWindow");
                itemDescWindow->ShowItemDescription(desc, name, icon,1); // This sets count to 1, is it possible to give more than 1 in reward?
            }
            break;
        }

        case GET_BUTTON:
        {
            int row = rewardList->GetSelectedRowNum();
            if (row>=0)
            {
                // let questmanager know that the client has wants this
                // item as a reward
                csString item = rewardList->GetTextCell(row, COLUMN_ID)->GetText();
                psQuestRewardMessage message(0, item, psQuestRewardMessage::selectReward);
                msgqueue->SendMessage(message.msg);
                SetModalState(false);
                Hide();
            }
            break;
        }
    }
    return true;
}
