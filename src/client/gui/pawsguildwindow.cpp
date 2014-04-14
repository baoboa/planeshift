/*
 * pawsguildwidow.cpp - Author: Andrew Craig
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

#include <iutil/objreg.h>

// COMMON INCLUDES
#include "net/messages.h"
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "util/log.h"
#include "util/psconst.h"
#include "util/psxmlparser.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsguildwindow.h"
#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawslistbox.h"
#include "paws/pawscheckbox.h"
#include "paws/pawscombo.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsstringpromptwindow.h"
#include "paws/pawsnumberpromptwindow.h"
#include "paws/pawscombopromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

// Tab definitions
#define BTN_PERMISSIONS_TAB    200
#define BTN_MEMBER_TAB         201
#define BTN_ALLIANCE_TAB       202
#define BTN_SETTINGS_TAB       203

// Control ID definitions
#define EDIT_GUILD_NAME        1001
#define CHECK_GUILD_SECRET     1002
#define CHECK_ONLINE_ONLY      1003
#define EDIT_GUILD_WEB_PAGE    1004
#define CHECK_GUILD_NOTIFY     1005
#define CHECK_ALLIANCE_NOTIFY  1006

#define REMOVE_MEMBER_BUTTON 300
#define EDIT_LEVEL_BUTTON 301
#define EDIT_GUILD_POINTS_BUTTON 302
#define EDIT_PUBLIC_NOTES_BUTTON 303
#define EDIT_PRIVATE_NOTES_BUTTON 304
#define INVITE_BUTTON  305
#define LEAVE_BUTTON   306
#define EDIT_GUILD_MAX_POINTS_BUTTON 311

#define REMOVE_GUILD            500
#define SET_ALLIANCE_LEADER     501
#define INVITE_GUILD            502
#define LEAVE_ALLIANCE          503
#define DISBAND_ALLIANCE        504
#define CREATE_ALLIANCE         505

#define REMOVE_GUILD_CONFIRM            600
#define SET_ALLIANCE_LEADER_CONFIRM     601
#define LEAVE_ALLIANCE_CONFIRM          603
#define DISBAND_ALLIANCE_CONFIRM        604

#define SAVE_MOTD_BUTTON 2000
#define REFRESH_MOTD_BUTTON 2001

#define LEAVE_CONFIRM          307
#define REMOVE_MEMBER_CONFIRM  308
#define CREATE_GUILD_CONFIRM   309
#define CREATE_GUILD_DECLINE   310

// Each name of level in listbox of levels is clickable button, and this is ID of the first of them:
// Other levels have 401, 402, 403, ....
#define LEVEL_NAME_BUTTON  400

int textBoxSortFunc_Level( pawsWidget * widgetA, pawsWidget * widgetB )
{
    pawsTextBox * textBoxA;
    pawsTextBox * textBoxB;


    textBoxA = dynamic_cast <pawsTextBox*> ( widgetA );
    textBoxB = dynamic_cast <pawsTextBox*> ( widgetB );
    assert( textBoxA && textBoxB );
    const char* textA = textBoxA->GetText();
    if(textA == NULL)
        textA = "";
    const char* textB = textBoxB->GetText();
    if(textB == NULL)
        textB = "";

    pawsGuildWindow* window = (pawsGuildWindow*)PawsManager::GetSingleton().FindWidget( "GuildWindow" );
    CS_ASSERT( window );
    size_t levelA = window->GetLevel( textA );
    size_t levelB = window->GetLevel( textB );

    if( levelA < levelB )
        return -1;
    else if( levelA == levelB )
        return 0;
    else
        return 1;
}


/** Function that compares listbox rows which contain pawsTextBoxWidget with numbers inside */
int textBoxSortFunc_number(pawsWidget * widgetA, pawsWidget * widgetB)
{
    pawsTextBox * textBoxA, * textBoxB;
    const char  * textA,    * textB;
    int numA, numB;

    textBoxA = dynamic_cast <pawsTextBox*> (widgetA);
    textBoxB = dynamic_cast <pawsTextBox*> (widgetB);
    assert(textBoxA && textBoxB);
    textA = textBoxA->GetText();
    if (textA == NULL)
        textA = "";
    textB = textBoxB->GetText();
    if (textB == NULL)
        textB = "";

    numA = atoi(textA);
    numB = atoi(textB);

    if (numA < numB)
        return -1;
    else if (numA == numB)
        return 0;
    else
        return 1;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsGuildWindow::pawsGuildWindow()
{
    creatingGuild = false;
}

pawsGuildWindow::~pawsGuildWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_GUIGUILD);
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_MOTD);
}

bool pawsGuildWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GUIGUILD);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_MOTD);
    //Request a MOTD for the guild
    psMOTDRequestMessage motdReq;
    motdReq.SendMessage();

    xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());

    // Tab setup
    permissionsTab = dynamic_cast<pawsButton*>(FindWidget("Permissions_Tab"));
    permissionsPanel = FindWidget("Permissions_Panel");
    membersTab = dynamic_cast<pawsButton*>(FindWidget("Members_Tab"));
    membersPanel = FindWidget("Members_Panel");
    alliancesTab = dynamic_cast<pawsButton*>(FindWidget("Alliances_Tab"));
    alliancesPanel = FindWidget("Alliances_Panel");
    settingsTab = dynamic_cast<pawsButton*>(FindWidget("Settings_Tab"));
    settingsPanel = FindWidget("Settings_Panel");

    if (!permissionsTab || !permissionsPanel ||
        !membersTab || !membersPanel ||
        !alliancesTab || !alliancesPanel ||
        !settingsTab || !settingsPanel)
        return false;

    permissionsPanel->Hide();
    membersPanel->Hide();
    alliancesPanel->Hide();
    settingsPanel->Hide();

    currentPanel = settingsPanel;
    currentTab = settingsTab;
    currentPanel->Show();
    currentTab->SetState(true);

    // Info setup
    guildName = dynamic_cast<pawsTextBox*>(FindWidget("GuildName"));
    allianceName = dynamic_cast<pawsTextBox*>(FindWidget("AllianceName"));
    guildSecret = dynamic_cast<pawsCheckBox*>(FindWidget("GuildSecret"));
    guildWebPage = dynamic_cast<pawsTextBox*>(FindWidget("GuildWebPage"));
    levelList = dynamic_cast<pawsListBox*>(FindWidget("LevelList"));
    motdEdit = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("motd"));
    memberCount = dynamic_cast<pawsTextBox*>(FindWidget("MemberCount"));
    if (!guildName || !allianceName || !guildSecret || !levelList || !guildWebPage || !memberCount)
        return false;

    // Member setup
    memberList = dynamic_cast<pawsListBox*>(FindWidget("MemberList"));
    if (!memberList)
        return false;

    for (int i=0; i<6; i++)
        if (i == 5)
            memberList->SetSortingFunc(i, textBoxSortFunc_number);
        else if ( i == 1 )
            memberList->SetSortingFunc(i, textBoxSortFunc_Level);
        else
            memberList->SetSortingFunc(i, textBoxSortFunc);

    onlineOnly = dynamic_cast<pawsCheckBox*>(FindWidget("OnlineOnly"));
    if (!onlineOnly)
        return false;
    onlineOnly->SetState(true);

    guildNotifications = dynamic_cast<pawsCheckBox*>(FindWidget("GuildMemberNotifications"));
    if (!guildNotifications)
        return false;
    guildNotifications->SetState(false);
    
    allianceNotifications = dynamic_cast<pawsCheckBox*>(FindWidget("AllianceMemberNotifications"));
    if (!allianceNotifications)
        return false;
    allianceNotifications->SetState(false);

    allianceMemberList = dynamic_cast<pawsListBox*>(FindWidget("AllianceMemberList"));
    if (!allianceMemberList)
        return false;

    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if (!chatWindow)
        return false;

    return true;
}

void pawsGuildWindow::HideLeaderCheckboxes()
{
    pawsListBoxRow * row;
    pawsCheckBox * checkBox;
    int numCols;
    int i;

    row = levelList->GetRow(levelList->GetRowCount()-1);
    numCols = levelList->GetTotalColumns();
    for (i=2; i < numCols; i++)
    {
        checkBox = dynamic_cast<pawsCheckBox*> (row->GetColumn(i));
        if (checkBox != NULL)
            checkBox->Hide();
    }
}

void pawsGuildWindow::HandleMessage( MsgEntry* me )
{
    if (me->GetType() == MSGTYPE_MOTD)
    {
        psMOTDMessage motd(me);
        motdEdit->SetText(motd.guildmotd);
        psengine->SetGuildName(motd.guild);
        return;
    }

    psGUIGuildMessage incomming(me);

    // Don't open the window twice
    if(incomming.command == psGUIGuildMessage::NOT_IN_GUILD && IsCreatingGuild() && PawsManager::GetSingleton().FindWidget("YesNoWindow")->IsVisible())
        return;

    switch (incomming.command)
    {
    case psGUIGuildMessage::GUILD_DATA:
        HandleGuildData(incomming.commandData);
        break;
    case psGUIGuildMessage::LEVEL_DATA:
        HandleLevelData(incomming.commandData);
        break;
    case psGUIGuildMessage::MEMBER_DATA:
        HandleMemberData(incomming.commandData);
        break;
    case psGUIGuildMessage::ALLIANCE_DATA:
        HandleAllianceData(incomming.commandData);
        break;
    case psGUIGuildMessage::NOT_IN_GUILD:
        {
            SetCreatingGuild(true);
            csString msg;
            msg.Format("You are not in a guild.\nNew guilds must have a minimum of %d members, and the fee is %d trias. Beware that a guild will be automatically disbanded if it has less than 5 members for more than %d minutes\nDo you wish to found a guild now?",
                       GUILD_MIN_MEMBERS, GUILD_FEE, GUILD_KICK_GRACE);
            pawsYesNoBox::Create(this, msg, CREATE_GUILD_CONFIRM, CREATE_GUILD_DECLINE);
            break;
        }
    case psGUIGuildMessage::CLOSE_WINDOW:
        Hide();
        break;
    }
}


void pawsGuildWindow::HandleGuildData( csString& openString )
{
    if ( ! IsVisible() )
        pawsControlledWindow::Show();

    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse( openString );
    if ( error )
    {
        Error2( "Error Parsing Open List: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error1("No XML root in Open List");
            return;
    }

    csRef<iDocumentNode> guildNode = root->GetNode("guild");
    if (guildNode)
    {
        guildName->SetText(guildNode->GetAttributeValue("name"));
        guildSecret->SetState(strcmp(guildNode->GetAttributeValue("secret"),"yes")==0);
        guildWebPage->SetText(guildNode->GetAttributeValue("web_page"));
        max_guild_points = guildNode->GetAttributeValueAsInt("max_points");
    }
    else
        Error1("No <guild> tag in Open List");

}

void pawsGuildWindow::HandleLevelData( csString& openString )
{
    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse( openString );
    if ( error )
    {
        Error2( "Error Parsing Open List: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in %s", openString.GetData());
        return;
    }

    csRef<iDocumentNode> levelNode = root->GetNode("levellist");
    levelList->Clear();
    if (levelNode)
    {
        levelList->SelfPopulate(levelNode);
        SetupLevelNameButtons();
        ExtractLevelInfo(levelNode);
    }
    else
    {
        Error2("No <levellist> tag in %s", openString.GetData());
        return;
    }
    HideLeaderCheckboxes();
}

void pawsGuildWindow::HandleMemberData( csString& openString )
{
    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse( openString );
    if ( error )
    {
        Error2( "Error Parsing Open List: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in %s", openString.GetData());
        return;
    }

    csRef<iDocumentNode> memberNode = root->GetNode("memberlist");
    memberList->Clear();
    if (memberNode)
    {
        memberList->SelfPopulate(memberNode);
        memberList->SortRows();
    }
    else
    {
        Error2("No <memberlist> tag in %s", openString.GetData());
        return;
    }

    guildMemberInfo member;
    csRef<iDocumentNode> memberInfoNode = root->GetNode("memberinfo");
    members.DeleteAll();
    if (memberNode)
    {
        csRef<iDocumentNodeIterator> memberIter = memberInfoNode->GetNodes();
        while (memberIter->HasNext())
        {
            csRef<iDocumentNode> memberNode = memberIter->Next();
            member.char_id         =  memberNode->GetAttributeValueAsInt("char_id");
            member.name            =  memberNode->GetAttributeValue("name");
            member.public_notes    =  memberNode->GetAttributeValue("public");
            member.private_notes   =  memberNode->GetAttributeValue("private");
            member.points          =  memberNode->GetAttributeValueAsInt("points");
            member.level           =  memberNode->GetAttributeValueAsInt("level");
            chatWindow->AddAutoCompleteName( memberNode->GetAttributeValue("name") );

            //we crop the points client side less strain on the server and less complexity of code
            //this way if someone makes a wrong change they can recover the points

            if(member.points > max_guild_points)
                member.points = max_guild_points;

            members.Push(member);
        }
    }
    else
    {
        Error2("No <memberinfo> tag in %s", openString.GetData());
        return;
    }

    memberCount->SetText(csString().Format("%zu members", members.GetSize()));

    csRef<iDocumentNode> playerNode = root->GetNode("playerinfo");
    if (playerNode)
    {
        char_id = playerNode->GetAttributeValueAsInt("char_id");
        if(guildNotifications)
            guildNotifications->SetState(playerNode->GetAttributeValueAsBool("guildnotifications"));
        if(allianceNotifications)
            allianceNotifications->SetState(playerNode->GetAttributeValueAsBool("alliancenotifications"));
    }
    else
    {
        Error2("No <playerinfo> tag in %s", openString.GetData());
        return;
    }

    guildMemberInfo * player;
    player = FindMemberInfo(char_id);
    if (player != NULL)
        playerLevel = player->level;
    else
        playerLevel = 0;
}

void pawsGuildWindow::HandleAllianceData( csString& openString )
{
    bool IamLeader;
    int numMembers=0;

    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse( openString );
    if ( error )
    {
        Error2( "Error Parsing Open List: %s\n", error );
        return;
    }

    allianceMemberList->Clear();

    csRef<iDocumentNode> root = doc->GetRoot();
    if (!root)
    {
        Error2("No XML root in %s", openString.GetData());
        return;
    }
    csRef<iDocumentNode> allianceNode = root->GetNode("alliance");
    if (allianceNode)
    {
        IamLeader = allianceNode->GetAttributeValueAsInt("IamLeader") == 1;

        allianceName->SetText(allianceNode->GetAttributeValue("name"));

        csRef<iDocumentNodeIterator> memberIter = allianceNode->GetNodes();
        while (memberIter->HasNext())
        {
            csRef<iDocumentNode> memberNode = memberIter->Next();
            allianceMemberList->NewRow();
            numMembers++;
            allianceMemberList->SetTextCellValue(numMembers-1, 0, memberNode->GetAttributeValue("name"));
            allianceMemberList->SetTextCellValue(numMembers-1, 1, memberNode->GetAttributeValue("isleader"));
            allianceMemberList->SetTextCellValue(numMembers-1, 2, memberNode->GetAttributeValue("leadername"));
            allianceMemberList->SetTextCellValue(numMembers-1, 3, memberNode->GetAttributeValue("online"));
        }
    }
    else
    {
        Error2("No <alliance> tag in %s", openString.GetData());
        return;
    }

    if (numMembers == 0)
        SetAllianceWidgetVisibility(0);
    else if (IamLeader)
        SetAllianceWidgetVisibility(1);
    else
        SetAllianceWidgetVisibility(2);
}

void pawsGuildWindow::SetAllianceWidgetVisibility(int mode)
{
    pawsButton * remove, * setLeader, * invite, * leave, * disband, * create;
    bool removeVis, setLeaderVis, inviteVis, leaveVis, disbandVis, createVis;

    remove    = dynamic_cast <pawsButton*> (FindWidget(REMOVE_GUILD));
    setLeader = dynamic_cast <pawsButton*> (FindWidget(SET_ALLIANCE_LEADER));
    invite    = dynamic_cast <pawsButton*> (FindWidget(INVITE_GUILD));
    leave     = dynamic_cast <pawsButton*> (FindWidget(LEAVE_ALLIANCE));
    disband   = dynamic_cast <pawsButton*> (FindWidget(DISBAND_ALLIANCE));
    create    = dynamic_cast <pawsButton*> (FindWidget(CREATE_ALLIANCE));

    switch (mode)
    {
        case 0:
            removeVis    = false;
            setLeaderVis = false;
            inviteVis    = false;
            leaveVis     = false;
            disbandVis   = false;
            createVis    = true;
            break;

        case 1:
            removeVis    = true;
            setLeaderVis = true;
            inviteVis    = true;
            leaveVis     = true;
            disbandVis   = true;
            createVis    = false;
            break;

        case 2:
            removeVis    = false;
            setLeaderVis = false;
            inviteVis    = false;
            leaveVis     = true;
            disbandVis   = false;
            createVis    = false;
            break;

        default:
            removeVis    = false;
            setLeaderVis = false;
            inviteVis    = false;
            leaveVis     = false;
            disbandVis   = false;
            createVis    = false;
            break;
    }

    remove->SetVisibility(removeVis);
    setLeader->SetVisibility(setLeaderVis);
    invite->SetVisibility(inviteVis);
    leave->SetVisibility(leaveVis);
    disband->SetVisibility(disbandVis);
    create->SetVisibility(createVis);
}

void pawsGuildWindow::SetupLevelNameButtons()
{
    for (size_t i=0; i < levelList->GetRowCount(); i++)
    {
        pawsListBoxRow * row = levelList->GetRow(i);
        pawsButton * cell = dynamic_cast <pawsButton*> (row->GetColumn(0));
        if (cell != NULL)
        {
            cell->SetID(LEVEL_NAME_BUTTON+i);
            cell->SetNotify(this);
        }
    }
}

size_t pawsGuildWindow::GetLevel( const char* levelName )
{
    for (size_t z = 0; z < levels.GetSize(); z++ )
    {
        if ( levels[z] == levelName )
            return z;
    }
    return 0;
}

void pawsGuildWindow::ExtractLevelInfo(csRef<iDocumentNode> levelNode)
{
    int levelNum;

    levels.SetSize(0);

    csRef<iDocumentNodeIterator> iter = levelNode->GetNodes();
    while (iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        csRef<iDocumentNode> titleNode = node->GetNode("title");
        if (titleNode == NULL)
            continue;
        csRef<iDocumentNode> levelNode = node->GetNode("level");
        if (levelNode == NULL)
            continue;
        levelNum = atoi(levelNode->GetAttributeValue("text"));
        if (levelNum < 0)
            continue;
        if ((size_t)levelNum >= levels.GetSize())
            levels.SetSize(levelNum+1);
        levels[levelNum] = titleNode->GetAttributeValue("text");
    }
}

bool pawsGuildWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    int row,col;
    if (levelList->ConvertFromAutoID(widget->GetID(),row,col))
    {
        col = col-3;
        int level = row;
        pawsCheckBox * checkBox = dynamic_cast<pawsCheckBox*>(widget);

        if (checkBox){
            csString command;
            command.Format("<l level=\"%d\" privilege=\"%s\" state=\"%s\"/>",
                           level+1,widget->GetXMLBinding().GetData(),checkBox->GetState()?"on":"off");
            psGUIGuildMessage msg(psGUIGuildMessage::SET_LEVEL_RIGHT,command);
            msg.SendMessage();

            /* We revert the checkbox back and let the server do the change.
               If the server allows the change, it will be set to new state by a LEVEL_DATA message.
               It the server won't allow the change, it will simply remain unchanged */
            checkBox->SetState( ! checkBox->GetState() );
        }
        return true;
    }
    return false;
}

bool pawsGuildWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    guildMemberInfo * member;

    Notify3(LOG_ANY,"Widget %s(%d) released",widget->GetName(),widget->GetID());

    // If the user clicked on clickable level titles in level listbox, then ask for new level title
    if (widget->GetID()>=LEVEL_NAME_BUTTON  &&  widget->GetID()<LEVEL_NAME_BUTTON+MAX_GUILD_LEVEL)
    {
        int levelNum = widget->GetID()-LEVEL_NAME_BUTTON+1;
        pawsStringPromptWindow::Create("Enter new title of guild level", levels[levelNum],
                                       false, 200, 20, this, "LevelTitle", levelNum);
        return true;
    }

    bool retVal = false;
    switch ( widget->GetID() )
    {
        case CREATE_GUILD_CONFIRM:
        {
            pawsStringPromptWindow::Create("Enter name of the new guild", "",
                                           false, 250, 25, this,"NewGuildName");
            retVal = true;
            break;
        }
        case CREATE_GUILD_DECLINE:
        {
            SetCreatingGuild(false);
            retVal = true;
            break;
        }

        case BTN_PERMISSIONS_TAB:
        {
            currentPanel->Hide();
            currentTab->SetState(false);
            currentPanel = permissionsPanel;
            currentTab = permissionsTab;
            currentPanel->Show();
            currentTab->SetState(true);
            retVal = true;
            break;
        }
        case BTN_MEMBER_TAB:
        {
            currentPanel->Hide();
            currentTab->SetState(false);
            currentPanel = membersPanel;
            currentTab = membersTab;
            currentPanel->Show();
            currentTab->SetState(true);
            retVal = true;
            break;
        }
        case BTN_ALLIANCE_TAB:
        {
            currentPanel->Hide();
            currentTab->SetState(false);
            currentPanel = alliancesPanel;
            currentTab = alliancesTab;
            currentPanel->Show();
            currentTab->SetState(true);
            retVal = true;
            break;
        }
        case BTN_SETTINGS_TAB:
        {
            currentPanel->Hide();
            currentTab->SetState(false);
            currentPanel = settingsPanel;
            currentTab = settingsTab;
            currentPanel->Show();
            currentTab->SetState(true);
            retVal = true;
            break;
        }

        case CHECK_GUILD_SECRET:
        {
            csString command;
            command.Format("/guildsecret %s",(guildSecret->GetState()?"on":"off"));
            psengine->GetCmdHandler()->Execute(command);
            retVal = true;
            break;
        }

        case CHECK_ONLINE_ONLY:
        {
            csString command;
            command.Format("<r onlineonly=\"%s\"/>", onlineOnly->GetState() ? "yes":"no");
            psGUIGuildMessage msg(psGUIGuildMessage::SET_ONLINE, command);
            msg.SendMessage();
            retVal = true;
            break;
        }

        case CHECK_GUILD_NOTIFY:
        {
            csString command;
            command.Format("<r guildnotifications=\"%s\"/>", guildNotifications->GetState() ? "yes":"no");
            psGUIGuildMessage msg(psGUIGuildMessage::SET_GUILD_NOTIFICATION, command);
            msg.SendMessage();
            retVal = true;
            break;
        }

        case CHECK_ALLIANCE_NOTIFY:
        {
            csString command;
            command.Format("<r alliancenotifications=\"%s\"/>", allianceNotifications->GetState() ? "yes":"no");
            psGUIGuildMessage msg(psGUIGuildMessage::SET_ALLIANCE_NOTIFICATION, command);
            msg.SendMessage();
            retVal = true;
            break;
        }

        case REMOVE_MEMBER_BUTTON:
        {
            member = FindSelectedMemberInfo();
            if (member != NULL)
            {
                if (member->char_id == char_id)
                    OpenGuildLeaveConfirm();
                else
                    pawsYesNoBox::Create(this, "Do you really want to remove this member from the guild ?", REMOVE_MEMBER_CONFIRM, -1);
            }
            retVal = true;
            break;
        }
        case REMOVE_MEMBER_CONFIRM:
        {
            member = FindSelectedMemberInfo();
            if (member != NULL)
            {
                csString command;
                command.Format("/guildremove %s", member->name.GetData());
                psengine->GetCmdHandler()->Execute(command);
            }
            retVal = true;
            break;
        }
        case EDIT_LEVEL_BUTTON:
        {
            member = FindSelectedMemberInfo();
            if (member != NULL)
            {
                int maxVisibleLevel;
                if (playerLevel == MAX_GUILD_LEVEL)
                    maxVisibleLevel = MAX_GUILD_LEVEL;
                else
                    maxVisibleLevel = playerLevel -1;

                pawsComboPromptWindow * pw = pawsComboPromptWindow::Create("Choose level",
                                                this, "Level", member->char_id);
                CS_ASSERT((size_t)maxVisibleLevel < levels.GetSize());
                for (int i=1; i <= maxVisibleLevel; i++)
                    pw->NewOption(levels[i]);
                pw->Select(0);
            }
            retVal = true;
            break;
        }
        case EDIT_GUILD_POINTS_BUTTON:
        {
            member = FindSelectedMemberInfo();
            if (member != NULL)
                pawsNumberPromptWindow::Create("Enter points", member->points, 0, max_guild_points,
                                               this, "GuildPoints",member->char_id);
            retVal = true;
            break;
        }
        case EDIT_GUILD_MAX_POINTS_BUTTON:
        {
                pawsNumberPromptWindow::Create("Enter Maximum points", max_guild_points, 0, MAX_GUILD_POINTS_LIMIT,
                                               this, "MaxGuildPoints");
            retVal = true;
            break;
        }
        case EDIT_PUBLIC_NOTES_BUTTON:
        {
            member = FindSelectedMemberInfo();
            if (member != NULL)
                pawsStringPromptWindow::Create("Public notes", member->public_notes,
                                               true, 400, 300, this, "PublicNotes", member->char_id, false, 255); // 255 length from MySQL characters.guild_public_notes
            retVal = true;
            break;
        }
        case EDIT_PRIVATE_NOTES_BUTTON:
        {
            member = FindSelectedMemberInfo();
            if (member != NULL)
                pawsStringPromptWindow::Create("Private notes", member->private_notes,
                                               true, 400, 300, this, "PrivateNotes",member->char_id, false, 255); // 255 length from MySQL characters.guild_private_notes
            retVal = true;
            break;
        }
        case INVITE_BUTTON:
        {
            pawsStringPromptWindow* pmwindow =
                pawsStringPromptWindow::Create("Enter name of the person to invite", "",
                                               false, 250, 20, this, "Invitee");
            pmwindow->SetAlwaysOnTop(true);
            retVal = true;
            break;
        }
        case LEAVE_BUTTON:
        {
            OpenGuildLeaveConfirm();
            retVal = true;
            break;
        }
        case LEAVE_CONFIRM:
        {
            retVal = true;
            guildMemberInfo * player = FindMemberInfo(char_id);
            if (player == NULL)
                break;

            csString command;
            command.Format("/guildremove %s", player->name.GetData());
            psengine->GetCmdHandler()->Execute(command);

            Hide();
            break;
        }
        case EDIT_GUILD_NAME:
        {
            pawsStringPromptWindow::Create("Enter new name of the guild", guildName->GetText(),
                                           false, 220, 20, this, "GuildName");
            retVal = true;
            break;
        }
        case EDIT_GUILD_WEB_PAGE:
        {
            pawsStringPromptWindow::Create("Enter new web page of the guild", guildWebPage->GetText(),
                                           false, 220, 20, this, "NewURL");
            retVal = true;
            break;
        }
        case SAVE_MOTD_BUTTON:
        {
            csString motdMsg(motdEdit->GetText());
            csString nameMsg(guildName->GetText());

            psGuildMOTDSetMessage motdSet(motdMsg,nameMsg);
            motdSet.SendMessage();
            retVal = true;
            break;
        }
        case REFRESH_MOTD_BUTTON:
        {
            psMOTDRequestMessage motdReq;
            motdReq.SendMessage();
            retVal = true;
            break;
        }
        case REMOVE_GUILD:
        {
            pawsYesNoBox::Create(this, "Do you really want to remove this member from the alliance ?", REMOVE_GUILD_CONFIRM, -1);
            retVal = true;
            break;
        }
        case SET_ALLIANCE_LEADER:
        {
            if (allianceMemberList->GetSelectedRow() != NULL)
                pawsYesNoBox::Create(this, "Do you really want to transfer leadership to this guild ?", SET_ALLIANCE_LEADER_CONFIRM, -1);
            retVal = true;
            break;
        }
        case INVITE_GUILD:
        {
            pawsStringPromptWindow::Create("Enter name of the guild leader to invite", "",
                                           false, 270, 20, this, "NewAllianceInvitee");
            retVal = true;
            break;
        }
        case LEAVE_ALLIANCE:
        {
            pawsYesNoBox::Create(this, "Do you really want to leave the alliance ?", LEAVE_ALLIANCE_CONFIRM, -1);
            retVal = true;
            break;
        }
        case DISBAND_ALLIANCE:
        {
            pawsYesNoBox::Create(this, "Do you really want to disband the alliance ?", DISBAND_ALLIANCE_CONFIRM, -1);
            retVal = true;
            break;
        }
        case CREATE_ALLIANCE:
        {
            pawsStringPromptWindow::Create("Enter name of the new alliance", "",
                                           false, 220, 20, this, "NewAllianceName");
            retVal = true;
            break;
        }
        case REMOVE_GUILD_CONFIRM:
        {
            csString command;
            command.Format("/allianceremove %s", allianceMemberList->GetSelectedText(0));
            psengine->GetCmdHandler()->Execute(command);
            retVal = true;
            break;
        }
        case SET_ALLIANCE_LEADER_CONFIRM:
        {
            csString command;
            command.Format("/allianceleader %s", allianceMemberList->GetSelectedText(0));
            psengine->GetCmdHandler()->Execute(command);
            retVal = true;
            break;
        }
        case LEAVE_ALLIANCE_CONFIRM:
        {
            psengine->GetCmdHandler()->Execute("/allianceleave");
            retVal = true;
            break;
        }
        case DISBAND_ALLIANCE_CONFIRM:
        {
            psengine->GetCmdHandler()->Execute("/endalliance");
            retVal = true;
            break;
        }
    }

    return retVal;
}

void pawsGuildWindow::Show()
{
    // Guild Window cannot be shown directly: only via the /guildinfo command
    csString command;
    command.Format("/guildinfo %s",onlineOnly->GetState() ? "yes":"no");
    psengine->GetCmdHandler()->Execute(command);
}

void pawsGuildWindow::OpenGuildLeaveConfirm()
{
    pawsYesNoBox::Create(this, "Do you really want to leave your guild ?", LEAVE_CONFIRM, -1);
}

guildMemberInfo * pawsGuildWindow::FindMemberInfo(const csString & name)
{
    for (size_t i=0; i < members.GetSize(); i++)
        if (name == members[i].name)
            return & members[i];
    return NULL;
}

guildMemberInfo * pawsGuildWindow::FindMemberInfo(int char_id)
{
    for (size_t i=0; i < members.GetSize(); i++)
        if (char_id == members[i].char_id)
            return & members[i];
    return NULL;
}

guildMemberInfo * pawsGuildWindow::FindSelectedMemberInfo()
{
    return FindMemberInfo(memberList->GetSelectedText(0));
}

int pawsGuildWindow::FindMemberInListBox(const csString & name)
{
    pawsListBoxRow * row;
    pawsTextBox * nameTextBox;
    csString nameInList;

    for (size_t i=0; i < memberList->GetRowCount(); i++)
    {
        row = memberList->GetRow(i);
        nameTextBox = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
        if (nameTextBox != NULL)
        {
            nameInList = nameTextBox->GetText();
            if (nameInList == name)
                return i;
        }
    }
    return -1;
}

void pawsGuildWindow::Hide()
{
    if (IsVisible())
    {
        pawsControlledWindow::Hide();

        psGUIGuildMessage msg(psGUIGuildMessage::UNSUBSCRIBE_GUILD_DATA, "<x/>");
        msg.SendMessage();
    }
}

void pawsGuildWindow::OnStringEntered(const char *name,int param,const char *value)
{
    if (!strcmp(name,"NewGuildName"))
    {
        SetCreatingGuild(false);
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/newguild %s", value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"GuildName"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/guildname %s",value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"LevelTitle"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/guildlevel %i %s", param, value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"PublicNotes"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        csString escpxml = EscpXML(value);
        command.Format("<n char_id=\"%i\" notes=\"%s\"/>", param, escpxml.GetData());
        psGUIGuildMessage msg(psGUIGuildMessage::SET_MEMBER_PUBLIC_NOTES, command);
        msg.SendMessage();
    }
    else if (!strcmp(name,"PrivateNotes"))
    {
        if (!value)
            return;
        csString command;

        csString escpxml = EscpXML(value);

        command.Format("<n char_id=\"%i\" notes=\"%s\"/>", param, escpxml.GetData());
        psGUIGuildMessage msg(psGUIGuildMessage::SET_MEMBER_PRIVATE_NOTES, command);
        msg.SendMessage();
    }
    else if (!strcmp(name,"Invitee"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/guildinvite %s", value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"NewURL"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/guildweb %s",value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"NewAllianceName"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/newalliance %s",value);
        psengine->GetCmdHandler()->Execute(command);
    }
    else if (!strcmp(name,"NewAllianceInvitee"))
    {
        if (!value || !strlen(value))
            return;

        csString command;
        command.Format("/allianceinvite %s",value);
        psengine->GetCmdHandler()->Execute(command);
    }
}

void pawsGuildWindow::OnNumberEntered(const char *name,int param,int value)
{
    if (value == -1)
        return;

    if (!strcmp(name,"GuildPoints"))
    {
        csString command;
        command.Format("<p char_id=\"%i\" points=\"%i\"/>", param, value);
        psGUIGuildMessage msg(psGUIGuildMessage::SET_MEMBER_POINTS, command);
        msg.SendMessage();
    }
    else if (!strcmp(name,"MaxGuildPoints"))
    {
        csString command;
        command.Format("<mp max_guild_points=\"%i\"/>", value);
        psGUIGuildMessage msg(psGUIGuildMessage::SET_MAX_GUILD_POINTS, command);
        msg.SendMessage();
    }
}

void pawsGuildWindow::OnItemChosen(const char* /*name*/, int param, int itemNum, const csString& /*itemText*/)
{
    guildMemberInfo * member;

    if (itemNum == -1)
        return;

    member = FindMemberInfo(param);
    if (member == NULL)
        return;

    csString command;
    command.Format("/guildpromote %s %i", member->name.GetData(), itemNum+1);

    psengine->GetCmdHandler()->Execute(command);
}


