/*
 * pawsloginwindow.h - Author: Andrew Craig
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
#include <iutil/cfgmgr.h>
#include <csutil/sysfunc.h>
#include <csutil/xmltiny.h>
#include <csutil/md5.h>
#include <csutil/sha256.h>

#include "pawsloginwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsokbox.h"
#include "paws/pawslistbox.h"
#include "util/pscssetup.h"
#include "net/serverpinger.h"
#include "psmainwidget.h"
#include "pawscharpick.h"

#include "globals.h"

#define LOGIN_WINDOW_QUIT       100
#define LOGIN_WINDOW_LOGIN      101
#define LOGIN_WINDOW_CREDITS    102

#define SERVER_LIST_FILE     "/planeshift/data/servers.xml"
#define ASTERISKS            "********"

#define CNF_REMEMBER_SERVER       "PlaneShift.Connection.RememberServer"
#define CNF_REMEMBER_PASS         "PlaneShift.Connection.RememberPass"
#define CNF_CONVERT_PASS          "PlaneShift.Connection.ConvertPass"
#define CNF_USER                  "PlaneShift.Connection.%s.User"
#define CNF_PASSWORD              "PlaneShift.Connection.%s.Password"
#define CNF_PASSWORDSHA           "PlaneShift.Connection.%s.Password256"
#define CNF_AUTOLOGIN_SERVER      "PlaneShift.Connection.AutologinServer.Id"
#define CNF_AUTOLOGIN_SERVER_NAME "PlaneShift.Connection.AutologinServer.Name"


pawsLoginWindow::pawsLoginWindow()
{
    connecting = false;
    passwdChanged = false;
    serverIP.Clear();
    
    LoadServerList();
}

bool pawsLoginWindow::PostSetup()
{
    
    listBox = dynamic_cast <pawsListBox*> (FindWidget("servers"));
    if (listBox == NULL)
        return false;

    if (servers.GetSize() == 0)
    {
        Error1("List of servers is empty. You must add servers to data/servers.xml file first.");
        return false;
    }
    
    for (size_t x = 0; x < servers.GetSize(); x++ )
    {
        pawsListBoxRow* row = listBox->NewRow();
        
        pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
        if (name == NULL)
            return false;
        name->SetText( servers[x]->GetName() );
        
        pawsTextBox* ip = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
        if (ip == NULL)
            return false;
        ip->SetText( servers[x]->GetAddress() );
    }        

    // Load the stored settings.
    csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());
    
    // Get default server
    int defaultServer = cfg->GetInt(CNF_REMEMBER_SERVER, 0);
    
    if (defaultServer >= (int)servers.GetSize())
        defaultServer = 0;
        
    listBox->Select( listBox->GetRow(defaultServer), false ); // Update but do not notify self

    if ((int)servers.GetSize() > defaultServer)
    {
        serverIP = listBox->GetTextCellValue(defaultServer, 1);
        serverPort = servers[defaultServer]->GetPort();
    }
       
    login   = dynamic_cast <pawsEditTextBox*> (FindWidget("username"));
    if (login == NULL)
        return false;

    passwd  = dynamic_cast <pawsEditTextBox*> (FindWidget("password"));
    if (passwd == NULL)
        return false;
    passwd->SetPassword(true);

    connectingLabel = dynamic_cast <pawsMultiLineTextBox*> (FindWidget("connecting"));
    if (connectingLabel == NULL)
        return false;
        
    connectingLabel->SetText(servers[listBox->GetSelectedRowNum()]->GetDescription());

    UpdateUserPasswdFromConfig();

    remember = cfg->GetBool(CNF_REMEMBER_PASS, true);
    checkBox = dynamic_cast<pawsCheckBox*> (FindWidget("option_password"));
    if (checkBox)
        checkBox->SetState(remember);

    convert = cfg->GetBool(CNF_CONVERT_PASS, true);
    checkBoxC = dynamic_cast<pawsCheckBox*> (FindWidget("convert_password"));
    if (checkBoxC)
        checkBoxC->SetState(convert);

    // Set the version label
    pawsTextBox* version = dynamic_cast <pawsTextBox*> (FindWidget("version"));
    if (version == NULL)
        return false;
    version->SetText(APPNAME);

    PawsManager::GetSingleton().SetCurrentFocusedWidget(login);
    
    static bool first = true;
    //psMainWidget* ps = (psMainWidget*)PawsManager::GetSingleton().GetMainWidget();
    if(first)
    {
        psSystemMessage msg(0,MSG_OK,PawsManager::GetSingleton().Translate("Welcome to").AppendFmt(" %s!", APPNAME));
        msg.FireEvent();
        //handles auto login in case the option is set.
        int autoLoginServer = cfg->GetInt(CNF_AUTOLOGIN_SERVER, -2);
        csString autoLoginServerName = cfg->GetStr(CNF_AUTOLOGIN_SERVER_NAME, "");
        //search for the server name specified if it has some text in it. Not supporting unnamed servers.
        if(autoLoginServerName.Length())
        {
            for(size_t i = 0; i < servers.GetSize(); i++)
            {
                if(autoLoginServerName == servers[i]->GetName())
                {
                    autoLoginServer = i; //named autologin has precedence over id autologin
                    break;
                }
            }
        }
        //only autologin if it's a valid value
        if(autoLoginServer == -1) //autologin to the last used server
        {
            ConnectToServer(true); //connect to the server without updating the gui.
        }
        //autologin to the specified server
        else if(autoLoginServer >= 0 && (int)servers.GetSize() > autoLoginServer )
        {
            listBox->Select( listBox->GetRow(autoLoginServer), true );
            ConnectToServer(true); //connect to the server without updating the gui.
        }
        first = false;
    }
    else
    {
        psSystemMessage msg(0,MSG_RESULT,PawsManager::GetSingleton().Translate("Please re-login"));
        msg.FireEvent();
    }

    // We want to show as soon as we're done loading.
    Show();

    return true;
}

void pawsLoginWindow::UpdateUserPasswdFromConfig()
{
    csString cfg_name,user;

    // Load the stored settings.
    csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());

    cfg_name.Format(CNF_USER,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
    user = cfg->GetStr(cfg_name, "");

    // For backward compability, if not found check PlaneShift.Connection.User
    if (user.IsEmpty())
    {
        cfg_name = "PlaneShift.Connection.User";
        user = cfg->GetStr(cfg_name, "");
    }
        
    login->SetText (user);

    cfg_name.Format(CNF_PASSWORD,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
    storedPasswd = cfg->GetStr(cfg_name, "");

    // For backward compability, if not found check PlaneShift.Connection.Password
    if (storedPasswd.IsEmpty())
    {
        cfg_name = "PlaneShift.Connection.Password";
        storedPasswd = cfg->GetStr(cfg_name, "");
    }
    
    if (!storedPasswd.IsEmpty())
        passwd->SetText(ASTERISKS);
    else
        passwd->SetText("");


    //try fetching the 256 password for sending to server. This is actually not used
    //for autentication for now but it's collected in order to allow an easier migration
    //with the next version. Then this password will be what will be inside the old password field.
    cfg_name.Format(CNF_PASSWORDSHA,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
    storedPasswd256 = cfg->GetStr(cfg_name, "");
}

bool pawsLoginWindow::OnChange(pawsWidget * widget)
{
    if (widget==passwd  &&  !passwdChanged)
    {
        csString text = passwd->GetText();

        for (int i = 0; i < (int)text.Length();i++)
        {
            if (text.GetAt(i) != '*')
            {
                csString newtext = text.GetAt(i);
                passwdChanged = true;
                passwd->SetText(newtext);
                return true;
            }
        }

        passwdChanged = true;
        passwd->SetText("");
    }
    return true;
}

bool pawsLoginWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch( widget->GetID() )
    {
        case LOGIN_WINDOW_QUIT:
        {            
            psengine->QuitClient();
            return true;
        }
        
        case LOGIN_WINDOW_LOGIN:
        {
            if (connecting)
                break;

            if ( strlen(login->GetText()) == 0)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter your account name."));
                error.FireEvent();
                return true;
            }

            if ( strlen(passwd->GetText()) == 0)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter your password."));
                error.FireEvent();
                return true;
            }
            
            if (serverIP.Length() == 0)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please select a server."));
                error.FireEvent();
                return true;
            }

            switch (servers[listBox->GetSelectedRowNum()]->GetStatus())
            {
                case psServerPinger::INIT:
                case psServerPinger::FAILED:
                {
                    psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("The server isn't available!"));
                    error.FireEvent();
                    return true;
                }
                case psServerPinger::FULL:
                {
                    // GM's should be able to connect, even if server is full.
                    // So in this case the server have to decide if it accept the
                    // connection.
                    ConnectToServer();  
                    return true;
                }
                case psServerPinger::LOCKED:
                {
                    psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("The server is locked!"));
                    error.FireEvent();
                    return true;
                }
                case psServerPinger::WAIT:
                {
                    psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("The server isn't ready!"));
                    error.FireEvent();
                    return true;
                }
                case psServerPinger::READY:
                {
                    ConnectToServer();
                    return true;
                }
            }

            return true;
        }

        case LOGIN_WINDOW_CREDITS:
        {
            if (connecting)
                break;
            pawsWidget* wdg = PawsManager::GetSingleton().FindWidget("CreditsWindow");
            if (!wdg)
            {
                PawsManager::GetSingleton().LoadWidget("creditswindow.xml");
                Hide();
                return true;
            }
            
            wdg->Show();
            Hide();
            return true;
        }
    }
    
    return false;
}


void pawsLoginWindow::ConnectToServer(bool automatic)
{
    csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());

    // Set the time out to connect to server
    timeout = csGetTicks() + cfg->GetInt("PlaneShift.Client.User.Connecttimeout", 60) * 1000;

    if(!automatic)
    {
        connectingLabel->SetText(PawsManager::GetSingleton().Translate("Connecting to server... Please wait"));

        // to make sure the "Connecting" label is visible:    
        PawsManager::GetSingleton().GetGraphics3D()->BeginDraw (CSDRAW_2DGRAPHICS);
        PawsManager::GetSingleton().Draw (); 
        PawsManager::GetSingleton().GetGraphics3D()->FinishDraw ();
        PawsManager::GetSingleton().GetGraphics2D()->Print (0);
    }
          
    if ( !psengine->GetNetManager()->Connect( serverIP, serverPort ) )
    {
        psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Cannot connect to server!"));
        error.FireEvent();

        psengine->GetNetManager()->Disconnect();

        ConnectionFailed();
        return;
    }
      
    // Set up the picker window here. 
    if ( PawsManager::GetSingleton().FindWidget("CharPickerWindow") == 0 )
        PawsManager::GetSingleton().LoadWidget("charpick.xml");

    if (checkBoxC)
        convert = checkBoxC->GetState();

    if (passwdChanged) 
    {
        csString  passwordhash =  csMD5::Encode(passwd->GetText()).HexString();
        csString  passwordhashSHA =  CS::Utility::Checksum::SHA256::Encode(passwd->GetText()).HexString();
        static_cast<pawsCharacterPickerWindow*>
            (PawsManager::GetSingleton().FindWidget("CharPickerWindow"))->StoreHashedPassword(passwordhash, passwordhashSHA);
        psengine->GetNetManager()->Authenticate( login->GetText(), passwordhash.GetData(), convert? passwordhashSHA : "");
    }
    else
    {
        static_cast<pawsCharacterPickerWindow*>
            (PawsManager::GetSingleton().FindWidget("CharPickerWindow"))->StoreHashedPassword(storedPasswd, storedPasswd256);
        psengine->GetNetManager()->Authenticate( login->GetText(), storedPasswd, convert ? storedPasswd256  : "");
    }

    //allow the server name to be known from the charpicker to autochose the last server chosen.
    static_cast<pawsCharacterPickerWindow*>(PawsManager::GetSingleton().FindWidget("CharPickerWindow"))->StoreServerName(servers[listBox->GetSelectedRowNum()]->GetName().GetData());

    SaveLoginInformation();
    connecting = true;
}

void pawsLoginWindow::ConnectionFailed()
{
    connecting = false;
    connectingLabel->SetText(servers[listBox->GetSelectedRowNum()]->GetDescription());  
}

void pawsLoginWindow::SaveLoginInformation()
{  
    csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());  
    csString cnf_name;

    cfg->SetInt(CNF_REMEMBER_SERVER, listBox->GetSelectedRowNum());
    
    cnf_name.Format(CNF_USER,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
    cfg->SetStr(cnf_name,     login->GetText() );

    if (checkBox)
        remember = checkBox->GetState();

    cfg->SetBool(CNF_REMEMBER_PASS, remember);
    cfg->SetBool(CNF_CONVERT_PASS, convert);

    if (remember)
    {
        if (passwdChanged) {
            csString  passwordhash =  csMD5::Encode(passwd->GetText()).HexString();
            cnf_name.Format(CNF_PASSWORD,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
            cfg->SetStr(cnf_name, passwordhash.GetData() );

            csString  passwordhashSHA =  CS::Utility::Checksum::SHA256::Encode(passwd->GetText()).HexString();
            cnf_name.Format(CNF_PASSWORDSHA,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
            cfg->SetStr(cnf_name, passwordhashSHA.GetData() );
        }
    }
    else
    {
        cfg->DeleteKey("PlaneShift.Connection.Password");
        // TODO: Loop through all servers and delete password key 
        cnf_name.Format(CNF_PASSWORD,servers[listBox->GetSelectedRowNum()]->GetName().GetData());
        cfg->DeleteKey(cnf_name);
    }
    cfg->Save();    
}

bool pawsLoginWindow::LoadServerList()
{    
    iDocumentSystem* xml = psengine->GetXMLParser ();
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    
    csRef<iDataBuffer> buff = vfs->ReadFile( SERVER_LIST_FILE );

    if ( !buff || !buff->GetSize() )
    {
        Error2("File %s not found!", SERVER_LIST_FILE);
        return false;
    }
    
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( buff );
    if ( error )
    {
        Error2("ERROR: %s", error );
        return false;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root");
        return false;
    }
    
    csRef<iDocumentNode> topNode = root->GetNode("serverlist");
    if (topNode == NULL)
    {
        Error1("No <serverlist> tag");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();
    
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if(node->GetType() == CS_NODE_COMMENT)
            continue;

        int port;
        csRef<iDocumentAttribute> portAttr = node->GetAttribute("port");
        if (portAttr != NULL)
        {
            port = portAttr->GetValueAsInt();
        }
        else
        {
            port = 13331;
        }

        csString name = node->GetAttributeValue( "name" );
        csString description = node->GetAttributeValue( "description");
        csString ip = node->GetAttributeValue( "ip" );

        if (name.IsEmpty() || ip.IsEmpty())
        {
            Error1("Failed to read entry from server file. Skipping bad entry...");
            continue;
        }

        psServerPinger * server = new psServerPinger(name, description, ip, port,
                                                     PawsManager::GetSingleton().GetObjectRegistry());
        servers.Push( server );
    }

    return true;         
}

void pawsLoginWindow::OnListAction(pawsListBox* /*selected*/, int /*status*/) 
{
    if (listBox->GetSelectedRowNum() == -1)
        return;

    serverIP = listBox->GetTextCellValue(listBox->GetSelectedRowNum(), 1);
    serverPort = servers[listBox->GetSelectedRowNum()]->GetPort();
    connectingLabel->SetText(servers[listBox->GetSelectedRowNum()]->GetDescription());

    UpdateUserPasswdFromConfig();
}

void pawsLoginWindow::Show()
{
    // Play some music
    psengine->GetSoundManager()->LoadActiveSector("main");
    pawsWidget::Show();
}

void pawsLoginWindow::Hide()
{
    for (size_t i=0; i < servers.GetSize(); i++)
        servers[i]->Disconnect();
    
    pawsWidget::Hide();
}

void pawsLoginWindow::Draw()
{
    // Check timeout
    if(connecting && csGetTicks() >= timeout)
    {
        psengine->GetNetManager()->Disconnect();
        ConnectionFailed();
        PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate("The server is not running or is not reachable.  Please check the website or forums for more info."));
    }

    csString pingStr;

    for (size_t i=0; i < servers.GetSize(); i++)
    {
        servers[i]->DoYourWork();

        int ping = servers[i]->GetPing();
        int loss = (int)(servers[i]->GetLoss()*100.0f);
        
        switch (servers[i]->GetStatus())
        {
        case psServerPinger::INIT:
            pingStr.Clear();
            break;
        case psServerPinger::FAILED:
            pingStr = PawsManager::GetSingleton().Translate("Failed");
            break;
        case psServerPinger::FULL:
            pingStr = PawsManager::GetSingleton().Translate("Full");
            break;
        case psServerPinger::READY:
            if (loss == 100)
                pingStr = PawsManager::GetSingleton().Translate("Failed");
            else if (loss)
                pingStr.Format("%d %d%%",ping,loss);
            else
                pingStr.Format("%d",ping);
            break;
        case psServerPinger::LOCKED:
            pingStr = PawsManager::GetSingleton().Translate("Locked");
            break;
        case psServerPinger::WAIT:
            pingStr = PawsManager::GetSingleton().Translate("Wait");
            break;
        default:
            pingStr = PawsManager::GetSingleton().Translate("Error");
            break;
        }

        listBox->SetTextCellValue((int)i, 2, pingStr);
    }

    pawsWidget::Draw();
}

