/*
 * AuthentClient.cpp by Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/md5.h>
#include <csutil/sha256.h>
#include <csver.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/message.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "util/pserror.h"
#include "util/log.h"

#include "gui/pawsinfowindow.h"
#include "gui/pawsloginwindow.h"
#include "gui/psmainwidget.h"

#include "paws/pawsokbox.h"
#include "paws/pawsmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "authentclient.h"
#include "pscelclient.h"
#include "psclientdr.h"
#include "psengine.h"
#include "pscharcontrol.h"
#include "globals.h"


psAuthenticationClient::psAuthenticationClient(MsgHandler *mymsghandler)
{
    msghandler = mymsghandler;
    rejectmsg.Clear();

    msghandler->Subscribe(this,MSGTYPE_AUTHAPPROVED);
    msghandler->Subscribe(this,MSGTYPE_PREAUTHAPPROVED);
    msghandler->Subscribe(this,MSGTYPE_AUTHREJECTED);
    msghandler->Subscribe(this,MSGTYPE_DISCONNECT);
    msghandler->Subscribe(this,MSGTYPE_AUTHCHARACTERAPPROVED);
    
    Reset();
}


psAuthenticationClient::~psAuthenticationClient()
{
    if (msghandler)
    {
        msghandler->Unsubscribe(this,MSGTYPE_AUTHREJECTED);
        msghandler->Unsubscribe(this,MSGTYPE_AUTHAPPROVED);
        msghandler->Unsubscribe(this,MSGTYPE_PREAUTHAPPROVED);
        msghandler->Unsubscribe(this,MSGTYPE_DISCONNECT);
        msghandler->Unsubscribe(this,MSGTYPE_AUTHCHARACTERAPPROVED);
    }
}


void psAuthenticationClient::HandleMessage(MsgEntry *me)
{
    switch (me->GetType())
    {
        // This means that the character selected was ok and we should start 
        // loading the game.
        case MSGTYPE_AUTHCHARACTERAPPROVED:
        {
            PawsManager::GetSingleton().FindWidget("CharPickerWindow")->Hide(); 
            psengine->StartLoad();                        
            break;
        }
        
        case MSGTYPE_PREAUTHAPPROVED:
        {
            HandlePreAuth(me);                
            break;
        }
        
        case MSGTYPE_AUTHAPPROVED:
        {
            HandleAuthApproved( me );
            break;
        }
        
        case MSGTYPE_AUTHREJECTED:
        {
            psAuthRejectedMessage msg(me);
            Notify2( LOG_CONNECTIONS, "Connection request rejected.  Reason is %s.\n",(const char *)msg.msgReason);
            iClientApproved = REJECTED;
            rejectmsg = msg.msgReason;            
            break;
        }
        
        case MSGTYPE_DISCONNECT:
        {

            if ( psengine->GetNetManager()->IsConnected() )
            {
#ifndef CS_DEBUG
                HandleDisconnect(me);
#else
                // If debugging leave time to experiment, only drop the network.
                psengine->GetNetManager()->Disconnect();
#endif
            }
            else
            {
                Notify1( LOG_CONNECTIONS, "Received disconnect will not connected.\n");
            }
            
            break;          
        }
        
        default:
        {
            Warning2( LOG_CONNECTIONS, "Unknown message type received by AuthClient! (%d)\n",me->GetType());            
            break;
        }
    }
}


bool psAuthenticationClient::Authenticate (const csString & user, const csString & pwd, const csString & pwd256)
{
    Notify3( LOG_CONNECTIONS, "Prelog in as: (%s,%s)\n", user.GetData(), pwd.GetData() );    
    psPreAuthenticationMessage request(0,PS_NETVERSION);
    request.SendMessage();
    username = user;
    password = pwd;
    password256 = pwd256;
   
    rejectmsg.Clear();

    return true;
}


const char* psAuthenticationClient::GetRejectMessage ()
{
    return rejectmsg;
}


void psAuthenticationClient::ShowError()
{
    pawsLoginWindow* loginWdg = (pawsLoginWindow*)PawsManager::GetSingleton().FindWidget("LoginWindow");

    //If we have logged in
    if ( !loginWdg || !loginWdg->IsVisible() )
    {
        psMainWidget* mainWdg = (psMainWidget*)PawsManager::GetSingleton().GetMainWidget();
        mainWdg->LockPlayer();
        if (psengine->GetCharControl())
        {
            psengine->GetCharControl()->CancelMouseLook();
        }
        psengine->FatalError(rejectmsg.GetData());
    }
    else
    {
        psengine->GetNetManager()->Disconnect();
        loginWdg->ConnectionFailed();
        PawsManager::GetSingleton().CreateWarningBox(rejectmsg.GetData());
    }
}


void psAuthenticationClient::HandlePreAuth( MsgEntry* me )
{

//now we send only the sha256 hash. we don't send the cleartext password256 anymore
//after the second phase it will be removed.
    
    psPreAuthApprovedMessage msg(me);
            
    //if it's not a new user, encrypt password with clientnum
    csString passwordhashandclientnum (password256);
    passwordhashandclientnum.Append(":");
    passwordhashandclientnum.Append(msg.ClientNum);

    csString hexstring = CS::Utility::Checksum::SHA256::Encode(passwordhashandclientnum).HexString();

    //TODO: convert this to use csstrings
    // Get os and graphics card info
    iGraphics2D *graphics2D = PawsManager::GetSingleton().GetGraphics3D()->GetDriver2D();
    csString HWRender =  graphics2D->GetHWRenderer();
    csString HWGLVersion =  graphics2D->GetHWGLVersion();
    psAuthenticationMessage request(0,username.GetData(), hexstring.GetData(), CS_PLATFORM_NAME "-" CS_PROCESSOR_NAME "(" CS_VER_QUOTE(CS_PROCESSOR_SIZE) ")-" CS_COMPILER_NAME, HWRender.GetDataSafe(), HWGLVersion.GetDataSafe(), "" );
    
    request.SendMessage();                
}


void psAuthenticationClient::HandleAuthApproved( MsgEntry* me )
{           
    psAuthApprovedMessage msg(me);            
    Notify3(LOG_CONNECTIONS, "Connection request approved.  Token is %d.  PlayerID is %s.\n", msg.msgClientValidToken, ShowID(msg.msgPlayerID));
    iClientApproved = APPROVED;
                
    psengine->SetNumChars( msg.msgNumOfChars );
    psengine->SetLoggedIn(true);

    if(psengine->GetCelClient()->GetClientDR()->GotStrings())
    {
        PawsManager::GetSingleton().FindWidget("CharPickerWindow")->Show();
        delete PawsManager::GetSingleton().FindWidget("LoginWindow");
        delete PawsManager::GetSingleton().FindWidget("CreditsWindow");
    }
}


void psAuthenticationClient::HandleDisconnect( MsgEntry* me )
{
    psDisconnectMessage dc(me);
        
    // Special case to drop login failure reply from server immediately after we have already logged in.
    if (dc.actor == 0 || dc.actor == (uint32_t)-1)
    {
        if (rejectmsg.IsEmpty())
        {
            if (!dc.msgReason.IsEmpty())
            {
                rejectmsg = dc.msgReason;
            }
            else
            {
                if (dc.actor.IsValid())
                {
                    rejectmsg = 
                        "Cannot connect to the PlaneShift server.  "
                        "Please double-check IP address and firewall.";
                }
                else
                {
                    rejectmsg = 
                        "Server dropped us for an unknown reason, and is probably down.  "
                        "Please contact technical support.";
                }
                
            }
        }
                
        ShowError();
                              
        return;
    }
    
    // We are the only possible receiver
    if (dc.msgReason.IsEmpty())
    {
        rejectmsg = "Server dropped us. (Invalid error message)";
    }
    else
    {
        rejectmsg = dc.msgReason;
    }
                
    // are we expecting the disconnect message?
    if(psengine->loadstate != psEngine::LS_ERROR)
    {
        ShowError();  
    }
}
