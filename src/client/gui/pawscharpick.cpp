/*
 * pawscharpick.cpp by Andrew Craig <acraig@paqrat.com>
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
#include <imesh/spritecal3d.h>
#include <csutil/md5.h>
#include <csutil/sha256.h>
#include <iutil/cfgmgr.h>

#include "util/log.h"
#include "globals.h"
#include <ibgloader.h>

#include "charapp.h"
#include "pawscharpick.h"
#include "pawsloginwindow.h"
#include "paws/pawsbutton.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsobjectview.h"
#include "pawscharcreatemain.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsokbox.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "psnetmanager.h"
#include "pscelclient.h"
#include "psclientdr.h"
#include "psclientchar.h"

#define CHARACTER_BUTTON_0 0
#define CHARACTER_BUTTON_1 1
#define CHARACTER_BUTTON_2 2
#define CHARACTER_BUTTON_3 3

#define CHARACTER_DELETE_BUTTON 150
#define CHARACTER_ACTION_BUTTON 100

#define BACK_BUTTON             200
#define QUIT_BUTTON             250

#define YES_DELETE_CHARACTER    300
#define NO_DELETE_CHARACTER     302

#define CNF_AUTOPICK_CHAR_NAME "PlaneShift.Connection.AutoPickChar.Name"
#define CNF_AUTOPICK_CHAR      "PlaneShift.Connection.AutoPickChar.Id"
#define CNF_LAST_CHAR          "PlaneShift.Connection.%s.LastChar"


pawsCharacterPickerWindow::pawsCharacterPickerWindow()
{
    charactersFound = 0;
    selectedCharacter = -1;
    characterCreationScreens = false;
    view = 0;
    connecting = false;
    gotStrings = false;
    lastResend = 0;
    
    charApp = new psCharAppearance(psengine->GetObjectRegistry());
    loaded = true;
}


pawsCharacterPickerWindow::~pawsCharacterPickerWindow()
{
    psengine->UnregisterDelayedLoader(this);
    delete charApp;
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_AUTHAPPROVED);
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_CHAR_DELETE);
}


bool pawsCharacterPickerWindow::PostSetup()
{
    // Subscribe our message types that we are interested in. 
    psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_AUTHAPPROVED);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAR_DELETE);
        
    view = (pawsObjectView*)FindWidget("PaperDollView");
    while(!view->ContinueLoad())
    {
        csSleep(100);
    } 
    view->Rotate(10,0.01f);
    view->EnableMouseControl(true);

    return true;
}
                          

void pawsCharacterPickerWindow::HandleMessage( MsgEntry* me )
{
    switch( me->GetType() )
    {
        // Incomming character description.  At the moment it is just the
//         // character name and the factory of the mesh.
        case MSGTYPE_AUTHAPPROVED:
        {
            if(charactersFound != 0)
                return;

            //get a requested autoconnect charname
            csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());
            //gets the autopick choice
            csString requestedAutoPickName = cfg->GetStr(CNF_AUTOPICK_CHAR_NAME, "");
            int requestedAutoPickId = cfg->GetInt(CNF_AUTOPICK_CHAR, -2);
            //gets the last used name to select it
            csString cnf_name;
            cnf_name.Format(CNF_LAST_CHAR,serverName.GetData());
            csString lastCharName = cfg->GetStr(cnf_name,"");
            
            psAuthApprovedMessage msg(me);            

            for (int i=0; i < msg.msgNumOfChars; i++)
            {
                csString name,race,factName,traits,equipment;

                msg.GetCharacter(me,name,race,factName, traits,equipment);

                // Set the name of the character in the right button.
                csString buff;
                buff.Format("SelectCharacter%i", charactersFound);
                pawsButton* button = (pawsButton*)FindWidget( buff );
                button->SetText( name );
                button->Show();
                buff.Format("ImgCharacter%i", charactersFound);
                pawsWidget* radio = FindWidget(buff);
                radio->Show();
                
                // Store the character data.
                models[charactersFound].factName = factName;
                models[charactersFound].race = race;
                models[charactersFound].traits = traits;
                models[charactersFound].equipment = equipment;                                   
                charactersFound++;

                //if this is the name we have requested for autologin
                //choose it immediately
                if(!connecting && ( requestedAutoPickId == i ||
                                    (requestedAutoPickId == -1 && name == lastCharName) ||
                                    name == requestedAutoPickName))
                {
                    connecting = true;
                    psengine->GetCelClient()->SetMainPlayerName(name);
                    psCharacterPickerMessage msg(name);
                    msg.SendMessage();
                }

                //save the character id we have found
                if(lastCharName == name)
                    selectedCharacter = i;
            }

            // If we have all the chars we have then turn on the rest of the 
            // buttons as new character ones.                            
            // Allow player to join only after all messages have been received
            // to prevent user thinking chars have been 'lost'.
            if ( charactersFound == psengine->GetNumChars() )
            {
                pawsWidget* widget = FindWidget("login");
                if ( widget )
                    widget->Show();           
                if ( charactersFound < 4 )
                {
                    csString buff;
                    buff.Format("SelectCharacter%i", charactersFound);
                    pawsWidget* widget = FindWidget( buff );
                    widget->Show();
                    buff.Format("ImgCharacter%i", charactersFound);
                    pawsWidget* radio = FindWidget(buff);
                    radio->Show();                
                }                    

                // If there's some char created then show delete option
                pawsWidget* deleteCharBut = FindWidget("delete");
                if (deleteCharBut)
                    deleteCharBut->Show();
            }
            return;
        }
        case MSGTYPE_CHAR_DELETE:
        {
            ReturnToLoginWindow();
            break;
        }
    }
    
}

void pawsCharacterPickerWindow::OnStringEntered(const char* /*name*/, int /*param*/, const char* value)
{
    if (!value)
        return;

    csString passwordhash;
    csString passHashorig;
    if(passHash256.Length() > 0)
    {
        passwordhash = CS::Utility::Checksum::SHA256::Encode(value).HexString();
        passHashorig = passHash256;
    }
    else
    {
        passwordhash = csMD5::Encode(value).HexString();
        passHashorig = passHash;
    }

    if (passHashorig != passwordhash)
    {
        PawsManager::GetSingleton().SetModalWidget(NULL);
        PawsManager::GetSingleton().CreateWarningBox("Wrong password entered!");
    }
    else
    {
        csString name;
        name.Format("SelectCharacter%i", selectedCharacter);

        // Get Full name.
        psString charFullName( ((pawsButton*)FindWidget(name))->GetText() );
        psString charFirstName;
        charFullName.GetWord( 0, charFirstName );

        psCharDeleteMessage msg(charFirstName, 0);
        msg.SendMessage();
    }
}

bool pawsCharacterPickerWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifer*/, pawsWidget* widget)
{
    if(!widget)
        return false;

    switch ( widget->GetID() )
    {
        case YES_DELETE_CHARACTER:
        {
            PawsManager::GetSingleton().SetModalWidget(NULL);
            widget->GetParent()->Hide();

            pawsStringPromptWindow::Create("Please enter your account password to confirm:", csString(""),false, 220, 20, this, "DeletionConfirm", 0, true); 
            pawsEditTextBox* passbox =  dynamic_cast<pawsEditTextBox*>
                                        (PawsManager::GetSingleton().FindWidget("stringPromptEntry"));

            if (passbox)
            {
               passbox->SetPassword(true);
            }
            return true;
        }

        case NO_DELETE_CHARACTER:
        {
            return true;
        }
        case CONFIRM_YES:
        {
            //Delete the selected character
            csString name;
            name.Format("SelectCharacter%i", selectedCharacter);

            // Get Full name.
            psString charFullName( ((pawsButton*)FindWidget(name))->GetText() );
            psString charFirstName;
            charFullName.GetWord( 0, charFirstName );

            psCharDeleteMessage msg(charFirstName, 0);
            msg.SendMessage();

            return true;
        }

        case CONFIRM_NO:
        {
            PawsManager::GetSingleton().SetModalWidget(NULL);
            widget->GetParent()->Hide();
            return true;
        }
    }
    return false;
}

bool pawsCharacterPickerWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifer*/, pawsWidget* widget)
{
    if(!widget)
        return false;

    switch ( widget->GetID() )
    {
        case CHARACTER_DELETE_BUTTON:
        {
            csString name;
            name.Format("SelectCharacter%i", selectedCharacter);
            csString confirm;
            // Catch empty selection.
            if(selectedCharacter < 0)
            {
                confirm.Format( "You must select a character to delete!" );
                PawsManager::GetSingleton().CreateWarningBox( confirm, this );
                return false;
            }
            csString charName(((pawsButton*)FindWidget(name))->GetText());
            // Catch invalid selection.
            if(charName == "New Character")
                return false;

            csString msg("Warning! this will PERMANENTLY DELETE your character.  Are you sure you want to continue?");
            pawsYesNoBox::Create(this, msg, YES_DELETE_CHARACTER, NO_DELETE_CHARACTER);

            return true;
        }

        case BACK_BUTTON:
        {
            ReturnToLoginWindow();
            return true;
        }

        case CHARACTER_BUTTON_0:
        case CHARACTER_BUTTON_1:
        case CHARACTER_BUTTON_2:
        case CHARACTER_BUTTON_3:
        {
            if(!connecting)
            {
                SelectCharacter(widget->GetID(),widget);
            }
            break;    
        }
        
        case CHARACTER_ACTION_BUTTON:
        {
            // if we have a character then we play as that one.
            if ( selectedCharacter != -1 && !connecting )
            {
                // Disable the button so that we don't send 2 picker messages
                connecting = true;

                csString name;
                name.Format("SelectCharacter%i", selectedCharacter);
                
                // Send the name of the character to the server. 
                csString charname( ((pawsButton*)FindWidget(name))->GetText() );

                psengine->GetCelClient()->SetMainPlayerName(charname);
                psCharacterPickerMessage msg(charname);
                msg.SendMessage();

                //save the selected charname to the configuration file
                csString cnf_name;
                cnf_name.Format(CNF_LAST_CHAR,serverName.GetData());
                csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());
                cfg->SetStr(cnf_name, charname);
            }
            
            return true;
        }
        
        case QUIT_BUTTON:
        {
            psengine->QuitClient();
            return true;
        }
       
    }
    return false;
}

void pawsCharacterPickerWindow::ReturnToLoginWindow()
{
    psengine->Disconnect();

    psengine->GetCelClient()->GetClientDR()->ResetMsgStrings(); // Remove the message to prevent big CRASH in the client

    pawsLoginWindow* loginWin = (pawsLoginWindow*)PawsManager::GetSingleton().FindWidget("LoginWindow");
    if ( !loginWin )
    {
        PawsManager::GetSingleton().LoadWidget("loginwindow.xml");
        loginWin = (pawsLoginWindow*)PawsManager::GetSingleton().FindWidget("LoginWindow");
    }

    loginWin->Show();
    this->Hide();
    this->DeleteYourself();
    return;
}


void pawsCharacterPickerWindow::SetupCharacterCreationScreens()
{
    if (!characterCreationScreens )
    {
        static const char* screenFiles[] = 
        {
           "charcreate.xml",
           "birth.xml",
           "parents.xml",
           "childhood.xml",
           "lifeevents.xml",           
           "paths.xml",
           "summary.xml",
           0
        };
    
        int i = 0;
        while ( screenFiles[i] != 0 )
        {
            if ( !PawsManager::GetSingleton().LoadWidget( screenFiles[i] ) )
            {
                Error2("FAILED to load screen: %s\n", screenFiles[i] );
                return;
            }
            i++;
        }
                   
        characterCreationScreens = true;            
    }
        
    pawsCreationMain* widget = (pawsCreationMain*)PawsManager::GetSingleton().FindWidget("CharCreateMain");       
    
    psengine->GetCharManager()->GetCreation()->ClearChoices();    
    if ( widget )
    {
        widget->Reset();          
        widget->Show();
    }        
        
    Hide();     
    
}

void pawsCharacterPickerWindow::Show()
{
    pawsWidget* credits = PawsManager::GetSingleton().FindWidget("CreditsWindow");
    if(credits)
        credits->Hide();

    // Sync the gotStrings with psClientDR
    gotStrings = psengine->GetCelClient()->GetClientDR()->GotStrings();
    if(gotStrings)
        ReceivedStrings();
    else
        lastResend = csGetTicks(); // Resend 5 secs from now

    // If there are no characters to find then turn on one of the new account
    // buttons        
    if ( psengine->GetNumChars() == 0 )
    {
        pawsWidget* widget = FindWidget("SelectCharacter0");
        if ( widget ) widget->Show();
    }        
        
    // Play some music
    psengine->GetSoundManager()->LoadActiveSector("main");
    
    PawsManager::GetSingleton().SetCurrentFocusedWidget(this);

    pawsWidget::Show();
}

void pawsCharacterPickerWindow::SelectCharacter(int character)
{        
    if(!gotStrings)
        return;

    pawsWidget* wdg;

    for (int i = 0;i < 4; i++)
    {
        csString name;
        name = "ImgCharacter";
        name += i;
        wdg = FindWidget(name);
        if (!wdg)
            return;

        if (character == i && charactersFound != 0 )
            wdg->SetBackground("radioon2");
        else
            wdg->SetBackground("radiooff2");
    }
    view->Show();
    if ( charactersFound == 0 )
        return;
        
    pawsButton* loginWidget = (pawsButton*)FindWidget("login");
    csString name;
    name.Format("SelectCharacter%i", character);
    pawsWidget* box = FindWidget(name);    
    if ( box )    
        selectedCharacter = box->GetID();
    else 
        selectedCharacter = 0;        
    
    if(gotStrings)
        loginWidget->SetText( PawsManager::GetSingleton().Translate("Join") );
            
    // If the button selected has an empty character then button will
    // be for creating a new character
    if ( selectedCharacter >= charactersFound  )
    {
        return;
    }
    else
    {
        // Show the model for the selected character.
        loaded = false;
        view->Show();
        charApp->SetMesh(0);
        CheckLoadStatus();
    }
}

void pawsCharacterPickerWindow::SelectCharacter(int character, pawsWidget* widget)
{
    if(!gotStrings)
        return;

    pawsWidget* wdg;

    for (int i = 0;i < 4; i++)
    {
        csString name;
        name = "ImgCharacter";
        name += i;
        wdg = FindWidget(name);
        if (!wdg)
            return;

        if (character == i)
            wdg->SetBackground("radioon2");
        else
            wdg->SetBackground("radiooff2");
    }

    pawsButton* loginWidget = (pawsButton*)FindWidget("login");

    selectedCharacter = widget->GetID();                                    
    loginWidget->SetText( PawsManager::GetSingleton().Translate("Join") );
                
    // If the button selected has an empty character then button will
    // be for creating a new character
    if ( widget->GetID() >= charactersFound  )
    {            
        SetupCharacterCreationScreens();
    }
    else
    {
        // Show the model for the selected character.
        loaded = false;
        charApp->SetMesh(0);
        CheckLoadStatus();
    }
}

bool pawsCharacterPickerWindow::CheckLoadStatus()
{
    if(!loaded)
    {
        if(view->View(models[selectedCharacter].factName))
        {
            psengine->UnregisterDelayedLoader(this);

            iMeshWrapper* mesh = view->GetObject();
            if (!mesh)
            {
                PawsManager::GetSingleton().CreateWarningBox("Couldn't find mesh! Please run the updater");
                return true;
            }
            charApp->SetMesh(mesh);

            csRef<iSpriteCal3DState> spstate = scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
            if (spstate)
            {
                // Setup cal3d to select random 0 velocity anims
                spstate->SetVelocity(0.0,&psengine->GetRandomGen());
                csString traits(models[selectedCharacter].traits);
                csString equipment( models[selectedCharacter].equipment );

                charApp->ApplyTraits(traits);
                charApp->ApplyEquipment(equipment);

                psengine->GetLoader()->ContinueLoading(true); // make sure everything is loaded *now*
            }
            loaded = true;

            return true;
        }
        else
        {
            psengine->RegisterDelayedLoader(this);
        }
    }

    return false;
}

void pawsCharacterPickerWindow::Draw()
{
    pawsWidget::Draw();
}

void pawsCharacterPickerWindow::ReceivedStrings()
{
    gotStrings = true;

    pawsTextBox* label = (pawsTextBox*)FindWidget("wait_label");
    if(label)
        label->Hide();

    if(charactersFound > 0 && charactersFound == psengine->GetNumChars())
    {
        if(selectedCharacter != -1)
            SelectCharacter(selectedCharacter);
        else
            SelectCharacter(0);
    }
}
