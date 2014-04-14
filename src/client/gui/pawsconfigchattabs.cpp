/*
* pawsconfigchattabs.cpp - Author: Enar Vaikene
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
#include "globals.h"

#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"
#include "pawsconfigchattabs.h"


pawsConfigChatTabs::pawsConfigChatTabs()
{
    chatWindow = NULL;

}

bool pawsConfigChatTabs::Initialize()
{
    if (!LoadFromFile("configchattabs.xml"))
        return false;

    return true;
}

bool pawsConfigChatTabs::PostSetup()
{
    chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
    if(!chatWindow)
    {
        Error1("Couldn't find ChatWindow!");
        return false;
    }
    // Find widgets. These sets the main tab output
    if ((isysbase = FindCheckbox("isysbase")) == NULL)
        return false;
    if ((inpc = FindCheckbox("inpc")) == NULL)
        return false;
    if ((ichat = FindCheckbox("ichat")) == NULL)
        return false;
    if ((itells = FindCheckbox("itells")) == NULL)
        return false;
    if ((iguild = FindCheckbox("iguild")) == NULL)
        return false;
    if ((ialliance = FindCheckbox("ialliance")) == NULL)
        return false;
    if ((igroup = FindCheckbox("igroup")) == NULL)
        return false;
    if ((iauction = FindCheckbox("iauction")) == NULL)
        return false;
    if ((isys = FindCheckbox("isys")) == NULL)
        return false;
    if ((ihelp = FindCheckbox("ihelp")) == NULL)
        return false;

    //these sets the tabs to actually show
    if ((issysbase = FindCheckbox("issysbase")) == NULL)
        return false;
    if ((isnpc = FindCheckbox("isnpc")) == NULL)
        return false;
    if ((ischat = FindCheckbox("ischat")) == NULL)
        return false;
    if ((istells = FindCheckbox("istells")) == NULL)
        return false;
    if ((isguild = FindCheckbox("isguild")) == NULL)
        return false;
    if ((isalliance = FindCheckbox("isalliance")) == NULL)
        return false;
    if ((isgroup = FindCheckbox("isgroup")) == NULL)
        return false;
    if ((isauction = FindCheckbox("isauction")) == NULL)
        return false;
    if ((issys = FindCheckbox("issys")) == NULL)
        return false;
    if ((ishelp = FindCheckbox("ishelp")) == NULL)
        return false;

    return true;
}

bool pawsConfigChatTabs::LoadConfig()
{
    chatWindow->LoadChatSettings();

    ChatSettings settings = chatWindow->GetSettings();

    isysbase->SetState(false);
    ichat->SetState(false);
    inpc->SetState(false);
    itells->SetState(false);
    iguild->SetState(false);
    ialliance->SetState(false);
    igroup->SetState(false);
    iauction->SetState(false);
    isys->SetState(false);
    ihelp->SetState(false);
    issysbase->SetState(false);
    ischat->SetState(false);
    isnpc->SetState(false);
    istells->SetState(false);
    isguild->SetState(false);
    isalliance->SetState(false);
    isgroup->SetState(false);
    isauction->SetState(false);
    issys->SetState(false);
    ishelp->SetState(false);

    csArray<csString> allMainBindings = settings.bindings.GetAll("subMainText");
    for(size_t i = 0; i < allMainBindings.GetSize(); i++)
    {
        if(allMainBindings[i] == "CHAT_SYSTEM_BASE")
            isysbase->SetState(true);
        if(allMainBindings[i] == "CHAT_CHANNEL1")
            ichat->SetState(true);
        if(allMainBindings[i] == "CHAT_NPC")
            inpc->SetState(true);
        if(allMainBindings[i] == "CHAT_TELL")
            itells->SetState(true);
        if(allMainBindings[i] == "CHAT_GUILD")
            iguild->SetState(true);
        if(allMainBindings[i] == "CHAT_ALLIANCE")
            ialliance->SetState(true);
        if(allMainBindings[i] == "CHAT_GROUP")
            igroup->SetState(true);
        if(allMainBindings[i] == "CHAT_AUCTION")
            iauction->SetState(true);
        if(allMainBindings[i] == "CHAT_SYSTEM")
            isys->SetState(true);
        if(allMainBindings[i] == "CHAT_ADVICE")
            ihelp->SetState(true);
    }

    //TODO: these magic numbers must disappear
    unsigned int state = settings.tabSetting;
    issysbase->SetState(state & 1);
    ischat->SetState(state & 2);
    isnpc->SetState(state & 4);
    istells->SetState(state & 8);
    isguild->SetState(state & 16);
    isgroup->SetState(state & 32);
    isalliance->SetState(state & 64);
    isauction->SetState(state & 128);
    issys->SetState(state & 256);
    ishelp->SetState(state & 512);


    // Check boxes doesn't send OnChange :(
    dirty = true;

    return true;
}

bool pawsConfigChatTabs::SaveConfig()
{
    ChatSettings& settings = chatWindow->GetSettings();

    settings.bindings.Delete("subMainText", "CHAT_SYSTEM_BASE");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL1");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL2");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL3");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL4");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL5");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL6");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL7");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL8");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL9");
    settings.bindings.Delete("subMainText", "CHAT_CHANNEL10");
    settings.bindings.Delete("subMainText", "CHAT_NPC");
    settings.bindings.Delete("subMainText", "CHAT_NPC_ME");
    settings.bindings.Delete("subMainText", "CHAT_NPC_MY");
    settings.bindings.Delete("subMainText", "CHAT_NPC_NARRATE");
    settings.bindings.Delete("subMainText", "CHAT_NPCINTERNAL");
    settings.bindings.Delete("subMainText", "CHAT_TELL");
    settings.bindings.Delete("subMainText", "CHAT_TELLSELF");
    settings.bindings.Delete("subMainText", "CHAT_GUILD");
    settings.bindings.Delete("subMainText", "CHAT_ALLIANCE");
    settings.bindings.Delete("subMainText", "CHAT_GROUP");
    settings.bindings.Delete("subMainText", "CHAT_AUCTION");
    settings.bindings.Delete("subMainText", "CHAT_SYSTEM");
    settings.bindings.Delete("subMainText", "CHAT_ADVICE");
    settings.bindings.Delete("subMainText", "CHAT_COMBAT");
    settings.bindings.Delete("subMainText", "CHAT_SERVER_TELL");
    settings.bindings.Delete("subMainText", "CHAT_ADVISOR");
    settings.bindings.Delete("subMainText", "CHAT_ADVICE_LIST");
    settings.bindings.Delete("subMainText", "CHAT_REPORT");

    if(isysbase->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_SYSTEM_BASE");
    }
    if(ichat->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_CHANNEL1");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL2");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL3");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL4");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL5");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL6");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL7");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL8");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL9");
        settings.bindings.Put("subMainText", "CHAT_CHANNEL10");
    }
    if(inpc->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_NPC");
        settings.bindings.Put("subMainText", "CHAT_NPC_ME");
        settings.bindings.Put("subMainText", "CHAT_NPC_MY");
        settings.bindings.Put("subMainText", "CHAT_NPC_NARRATE");
        settings.bindings.Put("subMainText", "CHAT_NPCINTERNAL");
    }
    if(itells->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_TELL");
        settings.bindings.Put("subMainText", "CHAT_TELLSELF");
    }
    if(iguild->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_GUILD");
    }
    if(ialliance->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_ALLIANCE");
    }
    if(igroup->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_GROUP");
    }
    if(iauction->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_AUCTION");
    }
    if(isys->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_SYSTEM");
        settings.bindings.Put("subMainText", "CHAT_SERVER_TELL");
        settings.bindings.Put("subMainText", "CHAT_REPORT");
        settings.bindings.Put("subMainText", "CHAT_COMBAT");
    }
    if(ihelp->GetState())
    {
        settings.bindings.Put("subMainText", "CHAT_ADVICE");
        settings.bindings.Put("subMainText", "CHAT_ADVISOR");
        settings.bindings.Put("subMainText", "CHAT_ADVICE_LIST");
    }

    //set the bit state for each tab in the "state" parameter
    //TODO: these magic numbers must disappear
    unsigned int state = 0x00000000;
    if(issysbase->GetState()) state += 1;
    if(ischat->GetState()) state += 2;
    if(isnpc->GetState()) state += 4;
    if(istells->GetState()) state += 8;
    if(isguild->GetState()) state += 16;
    if(isgroup->GetState()) state += 32;
    if(isalliance->GetState()) state += 64;
    if(isauction->GetState()) state += 128;
    if(issys->GetState()) state += 256;
    if(ishelp->GetState()) state += 512;

    settings.tabSetting = state;

    // Save to file
    chatWindow->SaveChatSettings();

    // Apply settings
    chatWindow->LoadChatSettings();

    //Reload the chat window.
    chatWindow->ReloadChatWindow();

    return true;
}

void pawsConfigChatTabs::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_CHAT_FILE_NAME);
    LoadConfig();
}

pawsCheckBox *pawsConfigChatTabs::FindCheckbox(const char *name)
{
    pawsCheckBox *t = dynamic_cast<pawsCheckBox *>(FindWidget(name));
    if (!t)
        Error2("Couldn't find widget %s", name);

    return t;
}
