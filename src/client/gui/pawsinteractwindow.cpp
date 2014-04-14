
/*
 * pawsinteractwidow.cpp - Author: Andrew Craig
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
#include "globals.h"

// CS INCLUDES
#include <csgeom/vector3.h>
#include <iutil/objreg.h>
#include <iutil/virtclk.h>

// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "psclientchar.h"

// PAWS INCLUDES
#include "pawsinteractwindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsmouse.h"
#include "net/messages.h"
#include "net/cmdhandler.h"
#include "util/log.h"
#include "pawsinfowindow.h"
#include "pawspetstatwindow.h"
#include "pawsnpcdialog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// GUI BUTTONS
//////////////////////////////////////////////////////////////////////
#define INTERACT_EXAMINE     100
#define INTERACT_UNLOCK      200
#define INTERACT_USE         300
#define INTERACT_PICKUP      400
#define INTERACT_DROP        500
#define INTERACT_LOOT        600
#define INTERACT_BUYSELL     700
#define INTERACT_GIVE        800
#define INTERACT_CLOSE       900
#define INTERACT_PLAYERDESC  1000
#define INTERACT_ATTACK      1100
#define INTERACT_COMBINE     1200
#define INTERACT_CONSTRUCT   1300
#define INTERACT_EXCHANGE    1400
#define INTERACT_TRAIN       1500
#define INTERACT_NPCTALK     1600
#define INTERACT_VIEWSTATS   1700
#define INTERACT_DISMISS     1800
#define INTERACT_MARRIAGE    1900
#define INTERACT_DIVORCE     2000
#define INTERACT_PLAYGAME    2100
#define INTERACT_ENTER       2200
#define INTERACT_LOCK        2300
#define INTERACT_ENTERLOCKED 2400
#define INTERACT_BANK        2500
#define INTERACT_INTRODUCE   2600
#define INTERACT_MOUNT       2700
#define INTERACT_UNMOUNT     2800
#define INTERACT_STORAGE     2900
#define INTERACT_GENERIC     3000

//////////////////////////////////////////////////////////////////////

pawsInteractWindow::pawsInteractWindow()
{
    names.Push("ButtonExamine");
    names.Push("ButtonUnlock");
    names.Push("ButtonUse");
    names.Push("ButtonPickup");
    names.Push("ButtonDrop");
    names.Push("ButtonLoot");
    names.Push("ButtonBuySell");
    names.Push("ButtonGive");
    names.Push("ButtonClose");
    names.Push("ButtonPlayerDesc");
    names.Push("ButtonAttack");
    names.Push("ButtonCombine");
    names.Push("ButtonConstruct");
    names.Push("ButtonExchange");
    names.Push("ButtonBank");
    names.Push("ButtonTrain");
    names.Push("ButtonNPCTalk");
    names.Push("ButtonViewStats");
    names.Push("ButtonDismiss");
    names.Push("ButtonMarriage");
    names.Push("ButtonDivorce");
    names.Push("ButtonPlayGame");
    names.Push("ButtonEnter");
    names.Push("ButtonLock");
    names.Push("ButtonEnterLocked");
    names.Push("ButtonIntroduce");
    names.Push("ButtonMount");
    names.Push("ButtonUnmount");
    names.Push("ButtonStorage");
    names.Push("ButtonGeneric");

    types.Push(psGUIInteractMessage::EXAMINE);
    types.Push(psGUIInteractMessage::UNLOCK);
    types.Push(psGUIInteractMessage::USE);
    types.Push(psGUIInteractMessage::PICKUP);
    types.Push(0);
    types.Push(psGUIInteractMessage::LOOT);
    types.Push(psGUIInteractMessage::BUYSELL);
    types.Push(psGUIInteractMessage::GIVE);
    types.Push(psGUIInteractMessage::CLOSE);
    types.Push(psGUIInteractMessage::PLAYERDESC);
    types.Push(psGUIInteractMessage::ATTACK);
    types.Push(psGUIInteractMessage::COMBINE);
    types.Push(psGUIInteractMessage::CONSTRUCT);
    types.Push(psGUIInteractMessage::EXCHANGE);
    types.Push(psGUIInteractMessage::BANK);
    types.Push(psGUIInteractMessage::TRAIN);
    types.Push(psGUIInteractMessage::NPCTALK);
    types.Push(psGUIInteractMessage::VIEWSTATS);
    types.Push(psGUIInteractMessage::DISMISS);
    types.Push(psGUIInteractMessage::MARRIAGE);
    types.Push(psGUIInteractMessage::DIVORCE);
    types.Push(psGUIInteractMessage::PLAYGAME);
    types.Push(psGUIInteractMessage::ENTER);
    types.Push(psGUIInteractMessage::LOCK);
    types.Push(psGUIInteractMessage::ENTERLOCKED);
    types.Push(psGUIInteractMessage::INTRODUCE);
    types.Push(psGUIInteractMessage::MOUNT);
    types.Push(psGUIInteractMessage::UNMOUNT);
    types.Push(psGUIInteractMessage::STORAGE);
    types.Push(psGUIInteractMessage::GENERIC);
    openTick = 0;
}

pawsInteractWindow::~pawsInteractWindow()
{
}

bool pawsInteractWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GUIINTERACT);
    return true;
}

void pawsInteractWindow::HandleMessage( MsgEntry* me )
{
    Show();
    psGUIInteractMessage guimsg(me);

    // Button names
    bool hasOptions = false;
    for ( size_t i=0; i < names.GetSize(); i++ )
    {
        pawsWidget *widget = FindWidget(names[i], false );
        if ( !widget )
        {
            Error2( "Button >%s< not found", names[i].GetData() );
            continue;
        }
        if ( guimsg.options & types[i] )
        {
            hasOptions = true;
            widget->Show();
        }
        else
            widget->Hide();
    }

    psPoint mousePos = PawsManager::GetSingleton().GetMouse()->GetPosition();
    int xPos = mousePos.x - 96;
    int yPos = mousePos.y - 96;

    // Make sure the window is totally on the screen
    if ( xPos < 0 ) xPos = 0;
    if ( yPos < 0 ) yPos = 0;
    if ( xPos + screenFrame.Width() > graphics2D->GetWidth() )
        xPos = graphics2D->GetWidth() - screenFrame.Width();
    if ( yPos + screenFrame.Height() > graphics2D->GetHeight() )
        yPos = graphics2D->GetHeight() - screenFrame.Height();

    MoveTo( xPos , yPos );

    if (!hasOptions)
        Hide();

    // Set the current ticks so we know when to close ourselves
    openTick = psengine->GetVirtualClock()->GetCurrentTicks();
    genericCommand = guimsg.genericCommand;
}

bool pawsInteractWindow::OnMouseDown(int /*button*/, int /*modifiers*/, int /*x*/, int /*y*/)
{
    Hide();
    //PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
    return true;
}


bool pawsInteractWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {
        case INTERACT_CLOSE:
        {
            Hide();
            //PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
            return true;
        }

        case INTERACT_EXAMINE:
        {
            GEMClientObject * object = psengine->GetCharManager()->GetTarget();
            if ( !object )
                return false;

            // Find entity id
            EID id = object->GetEID();

            // Send message to server
            psViewItemDescription out(id.Unbox(), -1);   
            out.SendMessage();
           
            Hide();
            //PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
            return true;
        }

        case INTERACT_ENTER:
        case INTERACT_ENTERLOCKED:
        {
            GEMClientObject * object = psengine->GetCharManager()->GetTarget();
            if ( !object )
                return false;

            // Find entity id
            EID id = object->GetEID();
            psEntranceMessage out(id);
            out.SendMessage();
           
            Hide();
            //PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
            return true;
        }

        case INTERACT_PICKUP:
        {
            psengine->GetCmdHandler()->Execute("/pickup");
            Hide();
            return true;
        }

        case INTERACT_LOOT:
        {
            psengine->GetCmdHandler()->Execute("/loot");
            Hide();
            return true;
        }
        case INTERACT_BUYSELL:
        {
            psengine->GetCmdHandler()->Execute("/buy");
            Hide();
            return true;
        }
        case INTERACT_GIVE:
        {
            psengine->GetCmdHandler()->Execute("/give");
            Hide();
            return true;
        }
        case INTERACT_COMBINE:
        {
            psengine->GetCmdHandler()->Execute("/combine");
            Hide();
            return true;
        }
        case INTERACT_CONSTRUCT:
        {
            psengine->GetCmdHandler()->Execute("/construct");
            Hide();
            return true;
        }
        case INTERACT_USE:
        {
            psengine->GetCmdHandler()->Execute("/use");
            Hide();
            return true;
        }
        case INTERACT_UNLOCK:
        case INTERACT_LOCK:
        {
            psengine->GetCmdHandler()->Execute("/picklock");
            Hide();
            return true;
        }
        case INTERACT_PLAYERDESC:
        {
            psengine->GetCmdHandler()->Execute("/targetinfo");
            Hide();
            return true;
        }
        case INTERACT_ATTACK:
        {
            psengine->GetCmdHandler()->Execute("/attack");
            Hide();
            return true;
        }
        case INTERACT_EXCHANGE:
        {
            psengine->GetCmdHandler()->Execute("/trade");
            Hide();
            return true;
        }
        case INTERACT_BANK:
        {
            psengine->GetCmdHandler()->Execute("/bank personal");
            Hide();
            return true;
        }
        case INTERACT_INTRODUCE:
        {
            psengine->GetCmdHandler()->Execute("/introduce");
            Hide();
            return true;
        }
        case INTERACT_TRAIN:
        {
            psengine->GetCmdHandler()->Execute("/train");
            Hide();
            return true;
        }
        case INTERACT_NPCTALK: //speak to NPCs
        {
            psengine->GetCmdHandler()->Execute("/npcmenu");
			Hide();
            pawsNpcDialogWindow* npcDialogWindow = dynamic_cast<pawsNpcDialogWindow*>(PawsManager::GetSingleton().FindWidget("NPCDialogWindow"));

            if(npcDialogWindow)
            {
                npcDialogWindow->ShowIfBubbles();
            }
            return true;
        }
        case INTERACT_VIEWSTATS: //Request Viewing of Stats of PET
        {
            pawsPetStatWindow* petStatWindow = (pawsPetStatWindow*)PawsManager::GetSingleton().FindWidget("PetStatWindow");
            if (!petStatWindow)
                return false;
            Hide();

            GEMClientObject * object = psengine->GetCharManager()->GetTarget();
            if (!object)
                return false;
            
            if (!petStatWindow->IsVisible())//If the window is not visible, open it.
                petStatWindow->Show();
            PawsManager::GetSingleton().SetCurrentFocusedWidget( petStatWindow );//Put focus on the chat
            BringToTop( petStatWindow );
            petStatWindow->SetTarget( (GEMClientActor*)object );
            
            return true;
        }
        case INTERACT_DISMISS: //Dismiss the PET
        {
            psengine->GetCmdHandler()->Execute("/pet dismiss");
            Hide();
            return true;
        }
        case INTERACT_MARRIAGE:
        {
           //Pop up a prompt window where you add the personal message for marrying.
           pawsStringPromptWindow::Create("Enter the message for your marriage proposal.", "",
                                           true, 250, 50, this,"MarriageMessage");
           Hide();  
           return true;
        }
        case INTERACT_DIVORCE:
        {
            //Pop up a prompt window where you add the message for divorcing.
            pawsStringPromptWindow::Create("Explain your motivations for asking a divorce.", "",
                                           true, 250, 50, this,"DivorceMessage");
            Hide();
            return true;
        }
        case INTERACT_PLAYGAME:
        {
            psengine->GetCmdHandler()->Execute("/game");
            Hide();
            return true;
        }
        case INTERACT_MOUNT:
        {
            psengine->GetCmdHandler()->Execute("/mount");
            Hide();
            return true;
        }
        case INTERACT_UNMOUNT:
        {
            psengine->GetCmdHandler()->Execute("/unmount");
            Hide();
            return true;
        }
        case INTERACT_STORAGE:
        {
            psengine->GetCmdHandler()->Execute("/storage");
            Hide();
            return true;
        }
        case INTERACT_GENERIC:
        {
            psengine->GetCmdHandler()->Execute(genericCommand);
            Hide();
            return true;
        }
    }

    return false;
}

void pawsInteractWindow::Hide()
{
    for ( size_t i=0; i < names.GetSize(); i++ )
    {
        pawsWidget *widget = FindWidget(names[i], false );
        if ( !widget )
            continue;

        widget->Hide();
    }

    pawsWidget::Hide();
    openTick = 0;
}

void pawsInteractWindow::Draw()
{
    // Autohide
    if(psengine->GetVirtualClock()->GetCurrentTicks() - openTick > 10000)
    {
        //PawsManager::GetSingleton().SetCurrentFocusedWidget( NULL );
        Hide();
    }

    pawsWidget::Draw();

}

void pawsInteractWindow::OnStringEntered(const char* name, int /*param*/, const char* value)
{
    if ( value && strlen(value) )
    {
        if ( !strcmp(name,"MarriageMessage") )
        {   
            csString command;
            GEMClientObject * object = psengine->GetCharManager()->GetTarget();
            if ( !object )
            {
                return ;
            }
            command.Format("/marriage propose %s %s", object->GetName(), value);
            psengine->GetCmdHandler()->Execute(command);
        }
        else if (!strcmp(name,"DivorceMessage"))
        { 
            csString command;
            command.Format("/marriage divorce %s", value);
            psengine->GetCmdHandler()->Execute(command);
        }
    }
}
