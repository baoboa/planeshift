/*
 * pawsconfigwindow.cpp - Author: Ondrej Hurt
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
#include <assert.h>

// CS INCLUDES
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>

#include "../globals.h"

// COMMON INCLUDES
#include "util/log.h"

// CLIENT INCLUDES
#include "util/localization.h"

// PAWS INCLUDES
#include "pawsconfigwindow.h"
#include "paws/pawsbutton.h"
#include "paws/pawsborder.h"
#include "paws/pawsmanager.h"
#include "gui/pawscontrolwindow.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsokbox.h"
#include "gui/pawsconfigtextpage.h"

#define TREE_FILE_NAME "configtree.xml"
#define RESET_OPTIONS_CONFIRM_TEXT "Do you really want to reset these settings to default values?"
#define RESET_OPTIONS_WARNING_TEXT "You didn't select an option category!"

// config window layout:
#define SPACING             10            // space between parts of window
// #define SECT_WND_WIDTH      500           // width of section config window on the right
// #define SECT_WND_HEIGHT     300           // height of section config window on the right

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsConfigWindow::pawsConfigWindow()
{
    sectionTree   = NULL;
    sectionParent = NULL;
    currSectWnd   = NULL;
    okButton      = resetButton = NULL;
    notify        = NULL;
    object_reg = PawsManager::GetSingleton().GetObjectRegistry();
}

bool pawsConfigWindow::PostSetup()
{
    sectionTree = dynamic_cast <pawsSimpleTree*>(FindWidget("ConfigTree"));
    if(sectionTree == NULL)
        return false;

    sectionTree->SetNotify(this);
    sectionTree->SetScrollBars(false, true);
    sectionTree->UseBorder("line");

    sectionParent = FindWidget("PanelParent");
    if(sectionParent == NULL)
        return false;

    // sets up buttons:
    okButton = FindWidget("OKButton");
    if(okButton == NULL)
        return false;

    resetButton = FindWidget("ResetButton");
    if(resetButton == NULL)
        return false;

    return true;
}

void pawsConfigWindow::SetNotify(pawsWidget* _notify)
{
    notify = _notify;
}


bool pawsConfigWindow::OnButtonReleased(int /*mouseButton*/, int keyModifier, pawsWidget* widget)
{

    if(widget == okButton)
    {
        for(size_t x = 0; x < sectWnds.GetSize(); x++)
        {
            if(sectWnds[x].sectWnd->IsDirty())
                sectWnds[x].sectWnd->SaveConfig();
        }
        if(notify != NULL)
            notify->OnButtonPressed(0, keyModifier, this);
        Hide();
    }
    else if(currSectWnd == NULL)    // there's no option category selected, inform the player
        PawsManager::GetSingleton().CreateWarningBox(RESET_OPTIONS_WARNING_TEXT, this);
    else if(widget == resetButton)  // If the user has selected some options category
        PawsManager::GetSingleton().CreateYesNoBox(RESET_OPTIONS_CONFIRM_TEXT, this);
    else
    {
        switch(widget->GetID())
        {
                // User pressed yes on confirmation box
            case CONFIRM_YES:
            {
                // Reset the selected options category
                currSectWnd->SetDefault();
                break;
            }
            case CONFIRM_NO:
            {
                PawsManager::GetSingleton().SetModalWidget(0);
                widget->GetParent()->Hide();
                break;
            }
        }
    }
    return true;
}

bool pawsConfigWindow::OnSelected(pawsWidget* widget)
{
    csString sectName, factory, textName;
    pawsTreeNode* node;
    pawsWidget* sectWndAsWidget;
    pawsConfigSectionWindow* newCurrSectWnd;


    node = dynamic_cast<pawsTreeNode*>(widget);
    if(node == NULL)
    {
        Error1("Selected widget is not pawsTreeNode");
        return false;
    }

    sectName = node->GetAttr("SectionName");
    textName = node->GetAttr("TextName");
    if(sectName.IsEmpty())
        return false;    // .... this tree node does not invoke any configuration section

    newCurrSectWnd = FindSectionWindow(sectName);
    if(newCurrSectWnd == NULL)
    {
        factory = node->GetAttr("factory");

        sectWndAsWidget = PawsManager::GetSingleton().CreateWidget(factory);
        if(sectWndAsWidget == NULL)
        {
            Error1("Could not create pawsConfigSectionWindow widget from factory");
            return false;
        }
        newCurrSectWnd = dynamic_cast<pawsConfigSectionWindow*>(sectWndAsWidget);
        if(newCurrSectWnd == NULL)
        {
            Error1("Widget is not pawsConfigSectionWindow");
            return false;
        }
        sectionParent->AddChild(newCurrSectWnd);
        newCurrSectWnd->SetRelativeFrame(8,8,sectionParent->GetActualWidth()-16 , sectionParent->GetActualHeight()-16);

        // newCurrSectWnd->UseBorder("line");
        if(!strcmp(sectName, "configsound") && !psengine->GetSoundStatus())
        {
            PawsManager::GetSingleton().CreateWarningBox("Your sound settings are disabled!");
            return false;
        }
        if(!newCurrSectWnd->Initialize())
        {
            Error1("pawsConfigSectionWindow failed to initialize");
            return false;
        }

        // additional parameter for pawsConfigTextPage
        if(!textName.IsEmpty())
        {
            pawsConfigTextPage* configGeneric = dynamic_cast<pawsConfigTextPage*>(newCurrSectWnd);
            if(newCurrSectWnd == NULL)
            {
                Error1("Widget is not pawsConfigTextPage");
                return false;
            }
            else
            {
                configGeneric->setTextName(&textName);
            }
        }

        // Ignore sizing info loaded from file and override to fit tab here
        newCurrSectWnd->SetSectionName(sectName);
        newCurrSectWnd->SetRelativeFrame(8,8,sectionParent->GetActualWidth()-16 , sectionParent->GetActualHeight()-16);

        if(!newCurrSectWnd->LoadConfig())
        {
            Error1("pawsConfigSectionWindow could not load configuration");
            return false;
        }

        sectWnds.Push(sectWnd_t(sectName, newCurrSectWnd));
    }

    if(currSectWnd != NULL)
        currSectWnd->Hide();
    newCurrSectWnd->Show();

    currSectWnd = newCurrSectWnd;
    return true;
}

pawsConfigSectionWindow* pawsConfigWindow::FindSectionWindow(const csString &sectName)
{
    for(size_t x = 0; x < sectWnds.GetSize(); x++)
    {
        if(sectWnds[x].sectName == sectName)
            return sectWnds[x].sectWnd;
    }
    return NULL;
}

