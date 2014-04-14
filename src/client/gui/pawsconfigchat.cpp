/*
 * pawsconfigpvp.cpp - Author: Christian Svensson
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
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

// COMMON INCLUDES
#include "util/log.h"

// CLIENT INCLUDES
#include "../globals.h"
#include "pscelclient.h"

// PAWS INCLUDES
#include "gui/chatwindow.h"
#include "pawsconfigchat.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "paws/pawstextbox.h"
#include "paws/pawsradio.h"

//This will set back the color settings in the chat window
#define SET_CHAT_VALUE(x) settings.x##Color = PawsManager::GetSingleton().GetGraphics2D()->FindRGB(atoi(x##R->GetText()),atoi(x##G->GetText()),atoi(x##B->GetText()));
//this will get the color setting from the chat window and split it in the r, g, b components to display them
#define SET_WIDGET_VALUE(x) graphics2D->GetRGB(settings.x##Color,ir,ig,ib); \
                            r.Format("%d", ir); g.Format("%d", ig); b.Format("%d", ib); \
                            x##R->SetText(r); x##G->SetText(g); x##B->SetText(b);

pawsConfigChat::pawsConfigChat()
{
    chatWindow=NULL;
    systemR = NULL;
    systemG = NULL;
    systemB = NULL;
    adminR = NULL;
    adminG = NULL;
    adminB = NULL;
    playerR = NULL;
    playerG = NULL;
    playerB = NULL;
    chatR = NULL;
    chatG = NULL;
    chatB = NULL;
    tellR = NULL;
    tellG = NULL;
    tellB = NULL;
    shoutR = NULL;
    shoutG = NULL;
    shoutB = NULL;
    guildR = NULL;
    guildG = NULL;
    guildB = NULL;
    allianceR = NULL;
    allianceG = NULL;
    allianceB = NULL;
    yourR = NULL;
    yourG = NULL;
    yourB = NULL;
    groupR = NULL;
    groupG = NULL;
    groupB = NULL;
    auctionR = NULL;
    auctionG = NULL;
    auctionB = NULL;
    helpR = NULL;
    helpG = NULL;
    helpB = NULL;
    loose = NULL;
    mouseFocus = NULL;
    badwordsIncoming = NULL;
    badwordsOutgoing = NULL;
    echoScreenInSystem = NULL;
    mainBrackets = NULL;
    yourColorMix = NULL;
    joinDefaultChannel = NULL;
    defaultlastchat = NULL;
}

bool pawsConfigChat::Initialize()
{
    if ( ! LoadFromFile("configchat.xml"))
        return false;
       
    csRef<psCelClient> celclient = psengine->GetCelClient();
    assert(celclient);
    
    return true;
}

bool pawsConfigChat::PostSetup()
{

    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if(!chatWindow)
    {
        Error1("Couldn't find ChatWindow!");
        return false;
    }

    systemR = (pawsEditTextBox*)FindWidget("systemtextr");
    systemG = (pawsEditTextBox*)FindWidget("systemtextg");
    systemB = (pawsEditTextBox*)FindWidget("systemtextb");
    adminR = (pawsEditTextBox*)FindWidget("admintextr");
    adminG = (pawsEditTextBox*)FindWidget("admintextg");
    adminB = (pawsEditTextBox*)FindWidget("admintextb");
    playerR = (pawsEditTextBox*)FindWidget("playernametextr");
    playerG = (pawsEditTextBox*)FindWidget("playernametextg");
    playerB = (pawsEditTextBox*)FindWidget("playernametextb");
    chatR = (pawsEditTextBox*)FindWidget("chattextr");
    chatG = (pawsEditTextBox*)FindWidget("chattextg");
    chatB = (pawsEditTextBox*)FindWidget("chattextb");
    npcR = (pawsEditTextBox*)FindWidget("npctextr");
    npcG = (pawsEditTextBox*)FindWidget("npctextg");
    npcB = (pawsEditTextBox*)FindWidget("npctextb");
    tellR = (pawsEditTextBox*)FindWidget("telltextr");
    tellG = (pawsEditTextBox*)FindWidget("telltextg");
    tellB = (pawsEditTextBox*)FindWidget("telltextb");
    shoutR = (pawsEditTextBox*)FindWidget("shouttextr");
    shoutG = (pawsEditTextBox*)FindWidget("shouttextg");
    shoutB = (pawsEditTextBox*)FindWidget("shouttextb");
    guildR = (pawsEditTextBox*)FindWidget("guildtextr");
    guildG = (pawsEditTextBox*)FindWidget("guildtextg");
    guildB = (pawsEditTextBox*)FindWidget("guildtextb");
    allianceR = (pawsEditTextBox*)FindWidget("alliancetextr");
    allianceG = (pawsEditTextBox*)FindWidget("alliancetextg");
    allianceB = (pawsEditTextBox*)FindWidget("alliancetextb");
    yourR = (pawsEditTextBox*)FindWidget("yourtextr");
    yourG = (pawsEditTextBox*)FindWidget("yourtextg");
    yourB = (pawsEditTextBox*)FindWidget("yourtextb");
    groupR = (pawsEditTextBox*)FindWidget("grouptextr");
    groupG = (pawsEditTextBox*)FindWidget("grouptextg");
    groupB = (pawsEditTextBox*)FindWidget("grouptextb");
    auctionR = (pawsEditTextBox*)FindWidget("auctiontextr");
    auctionG = (pawsEditTextBox*)FindWidget("auctiontextg");
    auctionB = (pawsEditTextBox*)FindWidget("auctiontextb");
    helpR = (pawsEditTextBox*)FindWidget("helptextr");
    helpG = (pawsEditTextBox*)FindWidget("helptextg");
    helpB = (pawsEditTextBox*)FindWidget("helptextb");
    loose = (pawsCheckBox*)FindWidget("loosefocus");
    mouseFocus = (pawsCheckBox*)FindWidget("mousefocus");
    badwordsIncoming = (pawsCheckBox*)FindWidget("badwordsincoming");
    badwordsOutgoing = (pawsCheckBox*)FindWidget("badwordsoutgoing");
    selectTabStyleGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("selecttabstyle"));
    echoScreenInSystem = (pawsCheckBox*)FindWidget("echoscreeninsystem");
    mainBrackets = (pawsCheckBox*)FindWidget("mainbrackets");
    yourColorMix = (pawsCheckBox*)FindWidget("yourcolormix");
    joinDefaultChannel = (pawsCheckBox*)FindWidget("joindefaultchannel");
    defaultlastchat = (pawsCheckBox*)FindWidget("defaultlastchat");
                
    return true;
}

bool pawsConfigChat::LoadConfig()
{
    // Need to reload settings
    chatWindow->LoadChatSettings();

    ChatSettings &settings = chatWindow->GetSettings();

    loose->SetState(settings.looseFocusOnSend);
    mouseFocus->SetState(settings.mouseFocus);
    echoScreenInSystem->SetState(settings.echoScreenInSystem);
    mainBrackets->SetState(settings.mainBrackets);
    yourColorMix->SetState(settings.yourColorMix);
    joinDefaultChannel->SetState(settings.joindefaultchannel);
    defaultlastchat->SetState(settings.defaultlastchat);

    //gets the tabstyle from the chat settings
    switch (settings.selectTabStyle)
    {
        case 0: selectTabStyleGroup->SetActive("none");
            break;
        case 1: selectTabStyleGroup->SetActive("fitting");
            break;
        case 2: selectTabStyleGroup->SetActive("alltext");
            break;
    }
    
    //variables used to hold the temporary values
    int ir,ig,ib;
    csString r,g,b;

    //loads color settings
    SET_WIDGET_VALUE(system);
    SET_WIDGET_VALUE(admin);
    SET_WIDGET_VALUE(player);
    SET_WIDGET_VALUE(chat);
    SET_WIDGET_VALUE(npc);
    SET_WIDGET_VALUE(tell);
    SET_WIDGET_VALUE(shout);
    SET_WIDGET_VALUE(guild);
    SET_WIDGET_VALUE(alliance);
    SET_WIDGET_VALUE(your);
    SET_WIDGET_VALUE(group);
    SET_WIDGET_VALUE(auction);
    SET_WIDGET_VALUE(help);

    //gets badwords settings from the chat settings
    badwordsIncoming->SetState(settings.enableBadWordsFilterIncoming);
    badwordsOutgoing->SetState(settings.enableBadWordsFilterOutgoing);

    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChat::SaveConfig()
{
    ChatSettings &settings = chatWindow->GetSettings();
    SET_CHAT_VALUE(admin);
    SET_CHAT_VALUE(system);
    SET_CHAT_VALUE(player);
    SET_CHAT_VALUE(auction);
    SET_CHAT_VALUE(help);
    SET_CHAT_VALUE(group);
    SET_CHAT_VALUE(your);
    SET_CHAT_VALUE(guild);
    SET_CHAT_VALUE(alliance);
    SET_CHAT_VALUE(shout);
    SET_CHAT_VALUE(npc);
    SET_CHAT_VALUE(tell);
    SET_CHAT_VALUE(chat);
    settings.looseFocusOnSend = loose->GetState();
    settings.mouseFocus = mouseFocus->GetState();
    settings.enableBadWordsFilterIncoming = badwordsIncoming->GetState();
    settings.enableBadWordsFilterOutgoing = badwordsOutgoing->GetState();    

    if (selectTabStyleGroup->GetActive() == "none") settings.selectTabStyle = 0;
    else if (selectTabStyleGroup->GetActive() == "fitting") settings.selectTabStyle = 1;
    else settings.selectTabStyle = 2;

    settings.echoScreenInSystem = echoScreenInSystem->GetState();
    settings.mainBrackets = mainBrackets->GetState();
    settings.yourColorMix = yourColorMix->GetState();
    settings.joindefaultchannel = joinDefaultChannel->GetState();
    settings.defaultlastchat = defaultlastchat->GetState();

    // Save to file
    chatWindow->SaveChatSettings();
    
    return true;
}

void pawsConfigChat::Show()
{
    // Check boxes doesn't send OnChange :(
    dirty = true;
    pawsWidget::Show();
}

void pawsConfigChat::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}

bool pawsConfigChat::OnChange(pawsWidget* /*widget*/)
{
    return true;
}
