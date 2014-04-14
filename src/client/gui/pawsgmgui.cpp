/*
* pawsgmgui.cpp - Author: Andrew Robberts
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


// CS INCLUDES
#include <psconfig.h>
#include <ctype.h>
#include "globals.h"

#include <csutil/xmltiny.h>
#include <iutil/evdefs.h>
#include <ivideo/fontserv.h>
#include <iengine/camera.h>
#include <iengine/sector.h>
#include <iutil/object.h>

#include "pscelclient.h"


#include "paws/pawswidget.h"
#include "paws/pawsborder.h"
#include "paws/pawsmanager.h"
#include "paws/pawsprefmanager.h"
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "paws/pawstextbox.h"
#include "paws/pawscheckbox.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawstabwindow.h"
#include "pawsgmgui.h"
#include "paws/pawscrollbar.h"
#include "pscamera.h"
#include "pawsgmaction.h"
#include "pawscharcreatemain.h"
#include "../actionhandler.h"

//////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////
#define CHECKTAB(x,y) \
button = (pawsButton*)FindWidget(x);\
if (!button)\
{\
    Error2(LOG_PAWS, "Couldn't find button '%s'!", x);\
    return;\
}\
button->SetState(tab == y);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
pawsGmGUIWindow::pawsGmGUIWindow()
        : psCmdBase( NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry() )
{
    vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());

    cmdsource = psengine->GetCmdHandler();
    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    actionXML.Clear();
    isVisible = true;
    isInvincible = false;
    isViewAllObjects = false;
    isNeverTired = false;
    isQuestTester = false;
    isInfiniteMana = false;
    isFiniteInv = true;
    isSafeFall = false;
    isInstantCast = false;
    isGiveKillExp = false;
    isAttackable = false;
    isBuddyHide = false;
}

pawsGmGUIWindow::~pawsGmGUIWindow()
{
    msgqueue->Unsubscribe(this, MSGTYPE_GMGUI);
    msgqueue->Unsubscribe(this, MSGTYPE_MAPACTION);
}

bool pawsGmGUIWindow::PostSetup()
{
    // Setup this widget to receive messages and commands
    if ( !psCmdBase::Setup( psengine->GetMsgHandler(), psengine->GetCmdHandler()) )
        return false;

    // Subscribe to certain types of messages (those we want to handle)
    msgqueue->Subscribe(this, MSGTYPE_GMGUI);
    msgqueue->Subscribe(this, MSGTYPE_MAPACTION);

    playerList = (pawsListBox*)FindWidget("PlayerList");
    if (!playerList) return false;

    for (int i=0; i<5; i++)
    {
        playerList->SetSortingFunc(i, textBoxSortFunc);
    }
    playerList->SetSortedColumn(0);

    playerCount = (pawsTextBox*)FindWidget("PlayerCount");
    if (!playerCount) return false;

    actionList = (pawsListBox*)FindWidget("ActionList");
    if (!actionList) return false;

    for (int i=0; i<5; i++)
    {
        actionList->SetSortingFunc(i, textBoxSortFunc);
    }
    actionList->SetSortedColumn(0);

    SetSecurity();

    QueryServer();

    // get a handle on the widgets in our Attributes tab
    cbInvincible = (pawsCheckBox*)FindWidget("toggleInvincible");
    cbInvisible = (pawsCheckBox*)FindWidget("toggleInvisible");
    cbViewAll = (pawsCheckBox*)FindWidget("toggleViewAll");
    cbNeverTired = (pawsCheckBox*)FindWidget("toggleNeverTired");
    cbNoFallDamage = (pawsCheckBox*)FindWidget("toggleNoFallDamage");
    cbInfiniteInventory = (pawsCheckBox*)FindWidget("toggleInfiniteInventory");
    cbQuestTester = (pawsCheckBox*)FindWidget("toggleQuestTester");
    cbInfiniteMana = (pawsCheckBox*)FindWidget("toggleInfiniteMana");
    cbInstantCast = (pawsCheckBox*)FindWidget("toggleInstantCast");
    cbGiveKillExp = (pawsCheckBox*)FindWidget("toggleGiveKillExp");
    cbAttackable = (pawsCheckBox*)FindWidget("toggleAttackable");
    cbBuddyHide = (pawsCheckBox*)FindWidget("toggleBuddyHide");

    return true;
}

void pawsGmGUIWindow::Show()
{
    //We don't want old information
    //SelectTab(1);
    pawsWidget::Show();
    SetSecurity();
    QueryServer();
}

const char* pawsGmGUIWindow::HandleCommand(const char* /*cmd*/)
{
    return NULL;
}

void pawsGmGUIWindow::HandleMessage ( MsgEntry* me )
{
    if ( !me ) return;

    switch ( me->GetType() )
    {
    case MSGTYPE_GMGUI:
        {
        psGMGuiMessage message( me );

        if (message.type == psGMGuiMessage::TYPE_PLAYERLIST)
            FillPlayerList( message );

        if (message.type == psGMGuiMessage::TYPE_GETGMSETTINGS)
        {
            int gmSets = message.gmSettings;

            // invisibility
            isVisible = gmSets & 1;
            cbInvisible->SetState(!isVisible);
            cbInvisible->SetText( !isVisible ? "enabled" : "disabled" );

            // invincibility
            isInvincible = ( (gmSets & (1 << 1)) ? true : false);
            cbInvincible->SetState(isInvincible);
            cbInvincible->SetText( isInvincible ? "enabled" : "disabled" );

            // view invisible objects
            isViewAllObjects = ( gmSets & (1 << 2) ? true : false);
            cbViewAll->SetState(isViewAllObjects);
            cbViewAll->SetText( isViewAllObjects ? "enabled" : "disabled" );

            // infinite stamina
            isNeverTired = ( gmSets & (1 << 3) ? true : false);
            cbNeverTired->SetState(isNeverTired);
            cbNeverTired->SetText( isNeverTired ? "enabled" : "disabled" );

            // Quest Tester Mode
            isQuestTester = ( gmSets & (1 << 4) ? true : false);
            cbQuestTester->SetState(isQuestTester);
            cbQuestTester->SetText( isQuestTester ? "enabled" : "disabled" );

            // infinite mana
            isInfiniteMana = ( gmSets & (1 << 5) ? true : false);
            cbInfiniteMana->SetState(isInfiniteMana);
            cbInfiniteMana->SetText( isInfiniteMana ? "enabled" : "disabled" );

            // infinite inventory
            isFiniteInv = ( gmSets & (1 << 6) ? true : false);
            cbInfiniteInventory->SetState(!isFiniteInv);
            cbInfiniteInventory->SetText( !isFiniteInv ? "enabled" : "disabled" );

            // no fall damage
            isSafeFall = ( gmSets & (1 << 7) ? true : false);
            cbNoFallDamage->SetState(isSafeFall);
            cbNoFallDamage->SetText( isSafeFall ? "enabled" : "disabled" );

            // instant spell cast
            isInstantCast = ( gmSets & (1 << 8) ? true : false);
            cbInstantCast->SetState(isInstantCast);
            cbInstantCast->SetText( isInstantCast ? "enabled" : "disabled" );

            // Give kill experience
            isGiveKillExp = ( gmSets & (1 << 9) ? true : false);
            cbGiveKillExp->SetState(isGiveKillExp);
            cbGiveKillExp->SetText( isGiveKillExp ? "enabled" : "disabled" );

            // Attackable flag (allows to avoid challenge)
            isAttackable = ( gmSets & (1 << 10) ? true : false);
            cbAttackable->SetState(isAttackable);
            cbAttackable->SetText( isAttackable ? "enabled" : "disabled" );

            // Buddy Hide flag (Hides from player buddy lists)
            isBuddyHide = ( gmSets & (1 << 11) ? true : false);
            cbBuddyHide->SetState(isBuddyHide);
            cbBuddyHide->SetText( isBuddyHide ? "enabled" : "disabled" );
        }
        }
        break;
    case MSGTYPE_MAPACTION:
        {
        psMapActionMessage actionMsg( me );

            if ( actionMsg.command == psMapActionMessage::LIST )
            {
                actionXML = actionMsg.actionXML;
                FillActionList( actionMsg );
            }
        }
        break;
    }
}

int pawsGmGUIWindow::GetCurrentTab()
{
    pawsTabWindow *tabs = dynamic_cast<pawsTabWindow*>(FindWidget("GM Tabs"));
    return tabs ? tabs->GetActiveTab()->GetID()-1100 : 0;
}

bool pawsGmGUIWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    const char* errorMessage;
    csString cmd;
    bool updateAfter = false;
    bool confirm = true;
    BringToTop(this);

    pawsGMActionWindow * AddEdit = (pawsGMActionWindow *) PawsManager::GetSingleton().FindWidget( "AddEditActionWindow" );

    switch (widget->GetID())
    {
    case -10:
        errorMessage = cmdsource->Publish( cmdToExectute );
        if ( errorMessage )
            chatWindow->ChatOutput( errorMessage );
    case 1000: // Players tab
    case 1002: // Action Tab
    case 1003: // Attributes Tab
        break;

    //////////////////////////////////////////
    ///// Player stuff
    //////////////////////////////////////////
    /// Location
    case 1210: // aid pet
        cmd.Format("/teleport me %s",GetSelectedName());
        break;
    case 1211: // target left
        cmd.Format("/slide %s L",GetSelectedName()); confirm = false;
        break;
    case 1212: // target right
        cmd.Format("/slide %s R",GetSelectedName()); confirm = false;
        break;
    case 1213: // target forward
        cmd.Format("/slide %s F",GetSelectedName()); confirm = false;
        break;
    case 1214: // target back
        cmd.Format("/slide %s B",GetSelectedName()); confirm = false;
        break;

    case 1215: // target up
        cmd.Format("/slide %s U",GetSelectedName()); confirm = false;
        break;
    case 1216: // target down
        cmd.Format("/slide %s D",GetSelectedName()); confirm = false;
        break;

    /////////////////////////////////////////
    ///// Player stuff
    //////////////////////////////////////////
    /// General actions
    case 1230: // kick
        pawsStringPromptWindow::Create(
                        "Please enter a reason","",
                        false,250,20,this,"Reason", 0 );
        break;
    case 1231: // mute
        cmd.Format("/mute %s",GetSelectedName());
        break;
    case 1232: // unmute
        cmd.Format("/unmute %s",GetSelectedName());
        break;
    case 1233: // kill player (death)
        cmd.Format("/death %s",GetSelectedName());
        break;
    case 1234: // new name
        pawsStringPromptWindow::Create(
                        "Please enter new name","",
                        false,250,20,this, "NewName" );
        break;
    case 1235: // new name (random)
        {
            csString first,last;

            psengine->GetCharManager()->GetCreation()->GenerateName(
                GetSelectedGender(),
                first,4,8);

            psengine->GetCharManager()->GetCreation()->GenerateName(
                GetSelectedGender(),
                last,4,8);

            // Make asd => Asd
            first = NormalizeCharacterName(first);
            last = NormalizeCharacterName(last);

            cmd.Format("/changename %s force %s %s",GetSelectedName(),first.GetData(),last.GetData());
            updateAfter = true;

            break;
        }
    case 1236: // Ban player
    {
        pawsStringPromptWindow::Create(
                        "Please enter a reason","",
                        false,250,20,this,"Reason", 1);
        break;
    }
    /////////////////////////////////////////
    ///// Misc stuff
    //////////////////////////////////////////
    case 1250:// list
    {
        cmd.Format("/set me list");
        confirm = false;
        break;
    }
    case 1251:// invincible
    {
        cmd.Format("/set me invincible"); // Toggle
        confirm = false;
        break;
    }
    case 1252:// invisible
    {
        cmd.Format("/set me invisible"); // Toggle
        confirm = false;
        break;
    }
    case 1253:// viewall
    {
        cmd.Format("/set me viewall"); // Toggle
        confirm = false;
        break;
    }
    case 1254:// nevertired
    {
        cmd.Format("/set me nevertired"); // Toggle
        confirm = false;
        break;
    }
    case 1255:// nofalldamage
    {
        cmd.Format("/set me nofalldamage"); // Toggle
        confirm = false;
        break;
    }
    case 1256:// infiniteinventory
    {
        cmd.Format("/set me infiniteinventory"); // Toggle
        confirm = false;
        break;
    }
    case 1257:// questtester
    {
        cmd.Format("/set me questtester"); // Toggle
        confirm = false;
        break;
    }
    case 1258:// infinitemana
    {
        cmd.Format("/set me infinitemana"); // Toggle
        confirm = false;
        break;
    }
    case 1259:// instantcast
    {
        cmd.Format("/set me instantcast"); // Toggle
        confirm = false;
        break;
    }
    case 1267:// givekillexp
    {
        cmd.Format("/set me givekillexp"); // Toggle
        confirm = false;
        break;
    }
    case 1268:// attackable
    {
        cmd.Format("/set me attackable"); // Toggle
        confirm = false;
        break;
    }
    case 1269:// buddyhide
    {
        cmd.Format("/set me buddyhide"); // Toggle
        confirm = false;
        break;
    }
    // Action Location Tab Buttons
    case 1260:    // Refresh List
        QueryActionLocations();
        break;

    case 1261:    // Add Location
        if ( AddEdit )
        {
            AddEdit->LoadAction( "<location></location>" );
        }
        break;

    case 1262:    // Add Linked Location
        if ( AddEdit )
        {
            pawsListBoxRow* row = actionList->GetSelectedRow();
            if (!row)
                return true;

            pawsTextBox* box = (pawsTextBox*)row->GetColumn(0);

            csString msg;
            msg.Append("<location><masterid>");
            msg.Append( box->GetText() );
            msg.Append("</masterid></location>");
            AddEdit->LoadAction( msg );
        }
        break;

    case 1263:    // Edit Location
        if ( AddEdit )
        {
            AddEdit->Close();

            pawsListBoxRow* row = actionList->GetSelectedRow();
            if (!row)
                return true;

            pawsTextBox* box = (pawsTextBox*)row->GetColumn(0);

            csString id = box->GetText();

            // Loop through all Locations
            csRef<iDocument> doc         = ParseString( actionXML );
            if(!doc)
            {
                Error1("Error parsing Edit Location");
                return false;
            }
            csRef<iDocumentNode> root    = doc->GetRoot();
            if(!root)
            {
                Error1("No XML root in Edit Location");
                return false;
            }
            csRef<iDocumentNode> topNode = root->GetNode( "locations" );
            if(!topNode)
            {
                Error1("No <locations> tag in Edit Location");
                return false;
            }
            csRef<iDocumentNodeIterator> locIter = topNode->GetNodes( "location" );


            while ( locIter->HasNext() )
            {
                csRef<iDocumentNode> locationNode = locIter->Next();
                if ( locationNode->GetNode( "id" ) )
                {
                    if ( locationNode->GetNode( "id" )->GetContentsValueAsInt() == atoi( id ) )
                    {
                        AddEdit->LoadAction( locationNode );
                        return true;
                    }
                }
            }

        }
        break;

    case 1264:    // Delete Location
        if ( AddEdit )
        {
            pawsListBoxRow* row = actionList->GetSelectedRow();
            if (!row)
                return true;

            pawsTextBox* box = (pawsTextBox*)row->GetColumn(0);

            csString id = box->GetText();

            psengine->GetActionHandler()->DeleteAction( id );
        };
        break;

    case 1266: // Reload Cache
        psengine->GetActionHandler()->ReloadCache();
        break;

    case 1298: // refresh player list
        QueryServer();
        break;
    }

    // Execute
    if(cmd.Length() > 0)
    {
        if(confirm)
        {
            csString question;
            question.Format("Do you really want to execute the command %s?",cmd.GetData());
            cmdToExectute = cmd;

            pawsYesNoBox::Create(this, question);
        }
        else
        {
            errorMessage = cmdsource->Publish( cmd );
            if ( errorMessage )
                chatWindow->ChatOutput( errorMessage );
        }
    }
    if(updateAfter)
    {
        // Request an update from the server
        QueryServer();
    }

    return true;
}

void pawsGmGUIWindow::SetSecurity()
{

    // hide all controls
    HideWidget("Players Button");
    HideWidget("Action Button");
    HideWidget("Attributes Button");

    //Player
    HideWidget("PlayerList");
    HideWidget("RefreshPlayerList");
    HideWidget("AidPet");
    HideWidget("MoveTarget");
    HideWidget("TargetLeft");
    HideWidget("TargetRight");
    HideWidget("TargetForward");
    HideWidget("TargetBackwards");
    HideWidget("TargetUp");
    HideWidget("TargetDown");
    HideWidget("Kick");
    HideWidget("Mute");
    HideWidget("Unmute");
    HideWidget("KillWarn");
    HideWidget("Kill");
    HideWidget("ChangeName");
    HideWidget("ChangeNameRnd");
    HideWidget("SectorLbl");

    HideWidget("Ban");
    HideWidget("BanMin");
    HideWidget("BanHrs");
    HideWidget("BanDays");
    HideWidget("BanMinlbl");
    HideWidget("BanHrslbl");
    HideWidget("BanDayslbl");

    //Attribute buttons
    HideWidget("listattributes");
    HideWidget("invincible");
    HideWidget("toggleInvincible");
    HideWidget("invisible");
    HideWidget("toggleInvisible");
    HideWidget("viewall");
    HideWidget("toggleViewAll");
    HideWidget("nevertired");
    HideWidget("toggleNeverTired");
    HideWidget("nofalldamage");
    HideWidget("toggleNoFallDamage");
    HideWidget("infiniteinventory");
    HideWidget("toggleInfiniteInventory");
    HideWidget("questtester");
    HideWidget("toggleQuestTester");
    HideWidget("infinitemana");
    HideWidget("toggleInfiniteMana");
    HideWidget("instantcast");
    HideWidget("toggleInstantCast");
    HideWidget("givekillexp");
    HideWidget("toggleGiveKillExp");
    HideWidget("attackable");
    HideWidget("toggleAttackable");
    HideWidget("buddyhide");
    HideWidget("toggleBuddyHide");

    // int to hold the access level
    int level = psengine->GetCelClient()->GetMainPlayer()->GetType();
    if (level > 29)
        level = 29;

    // show the stuff that is visible under the selected tab
    // Falling through to lower security levels
    switch(level)
    {
    case 29:
        ShowWidget("Action Button");
        ShowWidget("KillWarn");
        ShowWidget("Kill");
        // Falling through to lower security levels
    case 28:
    case 27:
    case 26:
    case 25:
    case 24:
    case 23:
    case 22:
        ShowWidget("Ban");
        ShowWidget("BanMin");
        ShowWidget("BanHrs");
        ShowWidget("BanDays");
        ShowWidget("BanMinlbl");
        ShowWidget("BanHrslbl");
        ShowWidget("BanDayslbl");
        ShowWidget("Kick");
        ShowWidget("ChangeName");
        ShowWidget("ChangeNameRnd");
        // Falling through to lower security levels
    case 21:
        ShowWidget("Players Button");
        ShowWidget("PlayerList");
        ShowWidget("RefreshPlayerList");
        ShowWidget("AidPet");
        ShowWidget("MoveTarget");
        ShowWidget("TargetLeft");
        ShowWidget("TargetRight");
        ShowWidget("TargetUp");
        ShowWidget("TargetDown");
        ShowWidget("TargetForward");
        ShowWidget("TargetBackwards");
        ShowWidget("Mute");
        ShowWidget("Unmute");
        ShowWidget("SectorLbl");
        ShowWidget("Attributes Button");
        ShowWidget("listattributes");
        ShowWidget("invincible");
        ShowWidget("toggleInvincible");
        ShowWidget("invisible");
        ShowWidget("toggleInvisible");
        ShowWidget("viewall");
        ShowWidget("toggleViewAll");
        ShowWidget("nevertired");
        ShowWidget("toggleNeverTired");
        ShowWidget("nofalldamage");
        ShowWidget("toggleNoFallDamage");
        ShowWidget("infiniteinventory");
        ShowWidget("toggleInfiniteInventory");
        ShowWidget("questtester");
        ShowWidget("toggleQuestTester");
        ShowWidget("infinitemana");
        ShowWidget("toggleInfiniteMana");
        ShowWidget("instantcast");
        ShowWidget("toggleInstantCast");
        ShowWidget("givekillexp");
        ShowWidget("toggleGiveKillExp");
        ShowWidget("attackable");
        ShowWidget("toggleAttackable");
        ShowWidget("buddyhide");
        ShowWidget("toggleBuddyHide");
    case 0:
        break;
    }
}

void pawsGmGUIWindow::QueryServer()
{
    if (psengine->IsGameLoaded())
    {
        if (psengine->GetCelClient()->GetMainPlayer()->GetType() < 21)
        {
            this->Hide();
            return;
        }

        if (IsVisible())
        {
            psGMGuiMessage queryMsg(0, NULL, psGMGuiMessage::TYPE_QUERYPLAYERLIST);
            msgqueue->SendMessage( queryMsg.msg );

            psGMGuiMessage queryMsgSets(0, NULL, psGMGuiMessage::TYPE_GETGMSETTINGS);
            msgqueue->SendMessage( queryMsgSets.msg );

            QueryActionLocations();
        }
    }
}

void pawsGmGUIWindow::QueryActionLocations()
{
    if (psengine->IsGameLoaded())
    {
        if (psengine->GetCelClient()->GetMainPlayer()->GetType() < 29)
        {
            return;
        }
    }

    if ( IsVisible() )
    {
        csString query( "" );
        iSector* sector = psengine->GetPSCamera()->GetICamera()->GetCamera()->GetSector();
        const char* sectorname = sector->QueryObject()->GetName();

        query.Append( "<location>" );
        query.Append( "<sector>" ); query.Append( sectorname ); query.Append( "</sector>" );
        query.Append( "</location>" );
        psMapActionMessage actionMsg( 0, psMapActionMessage::LIST_QUERY, query);
        msgqueue->SendMessage(actionMsg.msg);
    }
}

const csArray<psGMGuiMessage::PlayerInfo> * sortedPlayers;

int cmpPlayers(const void * a, const void * b)
{
    int numA = *((int*)a);
    int numB = *((int*)b);
    const csString * nameA = & (*sortedPlayers)[numA].name;
    const csString * nameB = & (*sortedPlayers)[numB].name;
    if (*nameA < *nameB)
        return -1;
    if (*nameA == *nameB)
        return 0;
    else
        return 1;
}

void pawsGmGUIWindow::FillPlayerList(psGMGuiMessage& msg)
{
    csArray<int> playerOrder;

    // Clear the list from old data
    int sortedCol = playerList->GetSortedColumn();
    csString selectedPlayer;
    if (playerList->GetSelectedRow())
        selectedPlayer = ((pawsTextBox*)playerList->GetSelectedRow()->GetColumn(0))->GetText();
    playerList->Clear();

    // Loop through all players
    for (size_t i=0; i < msg.players.GetSize(); i++)
    {
        // Make stuff easier
        //int playerNum = playerOrder[i];
        psGMGuiMessage::PlayerInfo playerInfo = msg.players.Get(i);

        playerList->NewRow(i);
        pawsTextBox* nameBox = (pawsTextBox*)((pawsListBoxRow*)playerList->GetRow(i))->GetColumn(0);
        pawsTextBox* lastNameBox = (pawsTextBox*)((pawsListBoxRow*)playerList->GetRow(i))->GetColumn(1);
        pawsTextBox* genderBox = (pawsTextBox*)((pawsListBoxRow*)playerList->GetRow(i))->GetColumn(2);
        pawsTextBox* guildBox = (pawsTextBox*)((pawsListBoxRow*)playerList->GetRow(i))->GetColumn(3);
        pawsTextBox* sectorBox = (pawsTextBox*)((pawsListBoxRow*)playerList->GetRow(i))->GetColumn(4);

        if (!nameBox)
            return;
        if (!lastNameBox)
            return;
        if (!genderBox)
            return;
        if (!guildBox)
            return;
        if (!sectorBox)
            return;

        // Set the name
        csString name = playerInfo.name;
        nameBox->SetText(name);

        // Select if needed
        if (name == selectedPlayer)
            playerList->Select(playerList->GetRow(i), true);

        // Set the family name
        csString lastName = playerInfo.lastName;
        if (lastName)
            lastNameBox->SetText(lastName);

        // If target == current name, auto select
        if (psengine->GetTargetPetitioner() && name == psengine->GetTargetPetitioner())
            playerList->Select(playerList->GetRow(i+1));

        // Set the gender
        csString gender;
        if (playerInfo.gender == PSCHARACTER_GENDER_MALE)
            gender = "M";
        else if (playerInfo.gender == PSCHARACTER_GENDER_FEMALE)
            gender = "F";
        else
            gender = "N";

        genderBox->SetText(gender);

        // Set the guild name
        csString guildName = playerInfo.guild;
        if (guildName)
            guildBox->SetText(guildName);

        //Set the sector (invisible fact, used for lookup)
        sectorBox->SetText(playerInfo.sector);
    }

    playerList->SetSortedColumn(sortedCol);
    if (!playerList->GetSelectedRow())
        playerList->Select(playerList->GetRow(0));

    csString str;
    str.Format("%u players currently online",(unsigned int)msg.players.GetSize());
    playerCount->SetText(str);
}

void pawsGmGUIWindow::FillActionList( psMapActionMessage& msg )
{
    // Clear the list from old data
    actionList->Clear();

    // Loop through all Locations
    csRef<iDocument> doc         = ParseString( msg.actionXML );
    if(!doc)
    {
        Error1("Parse error in Action List");
        return;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Action List");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode( "locations" );
    if(!topNode)
    {
        Error1("No <locations> tag in Action List");
        return;
    }

    csRef<iDocumentNodeIterator> locIter = topNode->GetNodes( "location" );

    size_t i = 0;
    while ( locIter->HasNext() )
    {
        csRef<iDocumentNode> locationNode = locIter->Next();
        csRef<iDocumentNode> node;

        actionList->NewRow( i );
        pawsTextBox* idBox =       (pawsTextBox*)( (pawsListBoxRow*)actionList->GetRow( i ) )->GetColumn( 0 );
        pawsTextBox* nameBox =     (pawsTextBox*)( (pawsListBoxRow*)actionList->GetRow( i ) )->GetColumn( 1 );
        pawsTextBox* meshNameBox = (pawsTextBox*)( (pawsListBoxRow*)actionList->GetRow( i ) )->GetColumn( 2 );
        pawsTextBox* polyBox =     (pawsTextBox*)( (pawsListBoxRow*)actionList->GetRow( i ) )->GetColumn( 3 );
        pawsTextBox* posBox =      (pawsTextBox*)( (pawsListBoxRow*)actionList->GetRow( i ) )->GetColumn( 4 );
        pawsTextBox* typeBox =     (pawsTextBox*)( (pawsListBoxRow*)actionList->GetRow( i ) )->GetColumn( 5 );

        if ( !idBox )
            return;
        if ( !nameBox )
            return;
        if ( !meshNameBox )
            return;
        if ( !polyBox )
            return;
        if ( !posBox )
            return;
        if ( !typeBox )
            return;

        if ( locationNode->GetNode( "id" ) )
        {
            idBox->SetText( locationNode->GetNode( "id" )->GetContentsValue() );
        }

        if ( locationNode->GetNode( "name" ) )
        {
            nameBox->SetText( locationNode->GetNode( "name" )->GetContentsValue() );
        }

        if ( locationNode->GetNode( "mesh" ) )
        {
            meshNameBox->SetText( locationNode->GetNode( "mesh" )->GetContentsValue() );
        }
        if ( locationNode->GetNode( "poly" ) )
        {
            polyBox->SetText( locationNode->GetNode( "poly" )->GetContentsValue() );
        }

        node = locationNode->GetNode( "position" );
        if ( node )
        {
            float posx, posy, posz;
            const char * posFormat = "<%.2f, %.2f, %.2f>";
            posx = posy = posz = 0.0f;
            csString posStr( "" );

            if ( node->GetNode( "x" ) ) posx = node->GetNode( "x" )->GetContentsValueAsFloat();
            if ( node->GetNode( "y" ) ) posy = node->GetNode( "y" )->GetContentsValueAsFloat();
            if ( node->GetNode( "z" ) ) posz = node->GetNode( "z" )->GetContentsValueAsFloat();

            posStr.Format( posFormat, posx, posy, posz );

            posBox->SetText( posStr.GetData() );
        }

        if ( locationNode->GetNode( "responsetype" ) )
        {
            typeBox->SetText( locationNode->GetNode( "responsetype" )->GetContentsValue() );
        }

        i++;
    }
}

void pawsGmGUIWindow::HideWidget(const char* name)
{
    pawsWidget* widget = FindWidget(name);
    if (!widget)
    {
        Error2("Couldn't find widget %s!", name);
        return;
    }

    widget->Hide();
}

void pawsGmGUIWindow::ShowWidget(const char* name)
{
    pawsWidget* widget = FindWidget(name);
    if (!widget)
    {
        Error2("Couldn't find widget %s!", name);
        return;
    }

    widget->Show();
}

const char* pawsGmGUIWindow::GetSelectedName()
{
    pawsListBoxRow* row = playerList->GetSelectedRow();
    if (!row)
        return "Error";

    pawsTextBox* box = (pawsTextBox*)row->GetColumn(0);

    return box->GetText();
}

int pawsGmGUIWindow::GetSelectedGender()
{
    pawsListBoxRow* row = playerList->GetSelectedRow();
    if (!row)
        return PSCHARACTER_GENDER_NONE;

    pawsTextBox* box = (pawsTextBox*)row->GetColumn(1);

    csString gender = box->GetText();
    if (gender == "M")
        return PSCHARACTER_GENDER_MALE;
    else if (gender == "F")
        return PSCHARACTER_GENDER_FEMALE;
    else
        return PSCHARACTER_GENDER_NONE;

}

const char* pawsGmGUIWindow::GetSelectedSector()
{
    const char *val = playerList->GetSelectedText(4);
    if (!val)
        return "Error";
	else
		return val;
}


void pawsGmGUIWindow::OnListAction( pawsListBox* widget, int status )
{
    if ( !widget ) return;

    switch ( widget->GetID() )
    {
    case 1201: // Player List
        if (status==LISTBOX_HIGHLIGHTED)
        {
            pawsTextBox* lbl = (pawsTextBox*)FindWidget("SectorLbl");
            if (!lbl)
                return;

            csString text;
            text.Format("Sector: %s",GetSelectedSector());

            lbl->SetText(text);
        }
    case 1265: // Action List
        break;
    }

}

void pawsGmGUIWindow::OnStringEntered(const char *name,int param,const char *value)
{
    if (!value || !strlen(value))
        return;

    if (!strcmp(name,"NewName"))
    {
        csString text(value);
        int place = (int)text.FindFirst(' ');

        csString firstName,lastName;

        // TODO Does this make sense? Split the name to concat it?
        if (place == -1)
        {
            firstName = text;
        }
        else
        {
            firstName = text.Slice(0,place);
            lastName = text.Slice(place+1,text.Length());
        }

        csString cmd;
        cmd.Format("/changename %s force %s %s", GetSelectedName(), firstName.GetData(), lastName.GetData());
        psengine->GetCmdHandler()->Execute(cmd);
        //Need to update our list
        QueryServer();
    }
    else if (!strcmp(name,"Reason"))
    {
        csString cmd;

        if (param) // ban flag
        {
            pawsEditTextBox* mins = (pawsEditTextBox*)FindWidget("BanMin");
            pawsEditTextBox* hrs = (pawsEditTextBox*)FindWidget("BanHrs");
            pawsEditTextBox* days = (pawsEditTextBox*)FindWidget("BanDays");

            if (!strcmp(mins->GetText(),""))
                return;
            if (!strcmp(hrs->GetText(),""))
                return;
            if (!strcmp(days->GetText(),""))
                return;

            cmd.Format("/ban %s %s %s %s %s",GetSelectedName() ,mins->GetText(),hrs->GetText(), days->GetText(),value);
        }
        else
            cmd.Format("/kick %s %s",GetSelectedName(), value);

        psengine->GetCmdHandler()->Execute(cmd);

        //Need to update our list
        QueryServer();
    }
}



