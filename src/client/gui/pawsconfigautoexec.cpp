/*
 * pawsconfigautoexec.cpp - Author: Fabian Stock (Aiwendil)
 *
 * Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//CS INCLUDES
#include <psconfig.h>

//PAWS INCLUDES
#include "pawsconfigautoexec.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"

//CLIENT INCLUDES
#include "../globals.h"
#include "autoexec.h"

pawsConfigAutoexec::pawsConfigAutoexec()
{
       enabled = NULL;
       generalCommandBox = NULL;
       charCommandBox = NULL;
}

bool pawsConfigAutoexec::PostSetup()
{
    // setup the widget. Needed in the xml file are a Checkbox (to enable and disable autoexec)
    // and two multiline edit boxes for the general and character commands
    autoexec = psengine->GetAutoexec();
    if(!autoexec)
    {
        Error1("Couldn't find Autoexec!");
        return false;
    }

    enabled = dynamic_cast<pawsCheckBox*>(FindWidget("enabled"));
    if (!enabled) {
           Error1("Could not locate enabled widget!");
           return false;
    }
    generalCommandBox = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("GeneralCommandBox"));
    if (!generalCommandBox) {
           Error1("Could not locate GeneralCommandBox widget!");
           return false;
    }
    charCommandBox = dynamic_cast<pawsMultilineEditTextBox*>(FindWidget("CharCommandBox"));
    if (!generalCommandBox) {
           Error1("Could not locate CharCommandBox widget!");
           return false;
    }

    drawFrame();

    return true;
}

void pawsConfigAutoexec::drawFrame()
{
    //Clear the frame by hiding all widgets
    enabled->Hide();
    enabled->Show();
}

bool pawsConfigAutoexec::Initialize()
{
    if ( ! LoadFromFile("configautoexec.xml"))
    {
        return false;
    }
    return true;
}

bool pawsConfigAutoexec::LoadConfig()
{
    // get the values from the current value of the Autoexec class
    enabled->SetState(autoexec->GetEnabled());
    // "" as name serves for the commands for all chars
    generalCommandBox->SetText(autoexec->getCommands(""));
    charCommandBox->SetText(autoexec->getCommands(psengine->GetMainPlayerName()));
    dirty = false;
    return true;
}

bool pawsConfigAutoexec::SaveConfig()
{
    // set enabled and the player and general commands in the Autoexec class
    autoexec->SetEnabled(enabled->GetState());
    if (generalCommandBox->GetText() && *(generalCommandBox->GetText()) != '\0')
    {
        // "" as name serves for the commands for all chars
        autoexec->addCommand("", generalCommandBox->GetText());
    }
    if (charCommandBox->GetText() && *(charCommandBox->GetText()) != '\0')
    {
        autoexec->addCommand(psengine->GetMainPlayerName(), charCommandBox->GetText());
    }
    // then get the class to save them
    autoexec->SaveCommands();
    return true;
}

void pawsConfigAutoexec::SetDefault()
{
    // make the Autoexec class to load the default values
    autoexec->LoadDefault();
    // and write them to the user configuration files
    autoexec->SaveCommands();
    // and then reload everything in this config tab
    LoadConfig();
}

void pawsConfigAutoexec::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    dirty = true;
}
