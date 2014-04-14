/*
 * pawsconfigpopup.cpp
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//==============================================================================
// CS INCLUDES
//==============================================================================
#include <psconfig.h>
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>
#include <iutil/vfs.h>

//==============================================================================
// COMMON INCLUDES
//==============================================================================
#include "util/log.h"

//==============================================================================
// CLIENT INCLUDES
//==============================================================================
#include "../globals.h"
#include "pscelclient.h"

//==============================================================================
// PAWS INCLUDES
//==============================================================================
#include "gui/pawsactivemagicwindow.h"
#include "gui/pawsnpcdialog.h"
#include "pawsconfigpopup.h"
#include "gui/psmainwidget.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"

#define MSGCONFIG_FILE_NAME         "/planeshift/userdata/options/screenmsg.xml"


pawsConfigPopup::pawsConfigPopup(void)
{
}

bool pawsConfigPopup::Initialize()
{
    if (!LoadFromFile("configpopup.xml"))
        return false;

    return true;
}

bool pawsConfigPopup::PostSetup()
{
    magicWindow = (pawsActiveMagicWindow*)PawsManager::GetSingleton().FindWidget("ActiveMagicWindow");
    if(!magicWindow)
    {
        Error1("Couldn't find ActiveMagicWindow!");
        return false;
    }
    //check if we can get the pawsnpcdialog else fail.
    npcDialog = (pawsNpcDialogWindow*)PawsManager::GetSingleton().FindWidget("NPCDialogWindow");
    if(!npcDialog)
    {
        Error1("Couldn't find NPCDialogWindow!");
        return false;
    }
    //check if we can get the psmainwidget else fail.
    mainWidget = psengine->GetMainWidget();
    if(!mainWidget)
    {
        Error1("Couldn't find psmainwidget!");
        return false;
    }

    showActiveMagicConfig = (pawsCheckBox*)FindWidget("ShowActiveMagicWindowConfig");
    if (!showActiveMagicConfig)
        return false;

    useNpcDialogBubbles = (pawsCheckBox*)FindWidget("UseNpcDialogBubbles");
    if (!useNpcDialogBubbles)
        return false;

    //we set all checkboxes as true by default
    for(size_t i = 0; i < children.GetSize(); i++)
    {
        pawsCheckBox *check = dynamic_cast<pawsCheckBox*>(children.Get(i));
        if(check)
        {
            check->SetState(true);
        }
    }

    return true;
}

bool pawsConfigPopup::LoadConfig()
{
    if(!magicWindow->LoadSetting())
        return false;

    if(!mainWidget->LoadConfigFromFile())
        return false;

    if(!npcDialog->LoadSetting())
        return false;

    showActiveMagicConfig->SetState(!magicWindow->showWindow->GetState());
    useNpcDialogBubbles->SetState(npcDialog->GetUseBubbles());

    //we take for granted ids of the widgets correspond to message types id
    csHash<psMainWidget::mesgOption, int>::GlobalIterator iter = mainWidget->GetMesgOptionsIterator();
    while(iter.HasNext())
    {
        //get to the next option
        psMainWidget::mesgOption &opt = iter.Next();
        //check if we have a widget with that id
        pawsWidget *wdg = FindWidget(opt.type, false);
        if(wdg)
        {
            pawsCheckBox *check = dynamic_cast<pawsCheckBox*>(wdg);
            if(check)
            {
                //set the option in the interface
                check->SetState(opt.value);
            }
        }
    }


    dirty = true;

    return true;
}

bool pawsConfigPopup::SaveConfig()
{
    magicWindow->showWindow->SetState(!showActiveMagicConfig->GetState());
    npcDialog->SetUseBubbles(useNpcDialogBubbles->GetState());

    //we take for granted ids of the widgets correspond to message types id
    csHash<psMainWidget::mesgOption, int>::GlobalIterator iter = mainWidget->GetMesgOptionsIterator();
    while(iter.HasNext())
    {
        //get to the next option
        psMainWidget::mesgOption &opt = iter.Next();
        //check if we have a widget with that id
        pawsCheckBox *check = dynamic_cast<pawsCheckBox*>(FindWidget(opt.type, false));
        if(check)
        {
            //set the option in backend
            opt.value = check->GetState();
        }
    }

    //save the settings to the config files
    magicWindow->SaveSetting();

    mainWidget->SaveConfigToFile();

    npcDialog->SaveSetting();

    return true;
}


void pawsConfigPopup::SetDefault()
{
    psengine->GetVFS()->DeleteFile(CONFIG_ACTIVEMAGIC_FILE_NAME);
    psengine->GetVFS()->DeleteFile(MSGCONFIG_FILE_NAME);
    psengine->GetVFS()->DeleteFile(CONFIG_NPCDIALOG_FILE_NAME);
    LoadConfig();
}
