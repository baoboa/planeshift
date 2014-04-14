/*
 * pawsconfigshadows.cpp - Author: Prashanth Menon
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
#include "pawsconfigshadows.h"
#include "paws/pawsmanager.h"
#include "paws/pawscheckbox.h"

//CLIENT INCLUDES
#include "../globals.h"
#include "shadowmanager.h"

// Used to set default config files if the userdata ones are not found.
#define SAVE_USER     true
#define DEFAULT_FILE  "/planeshift/data/options/shadows_def.xml"
#define USER_FILE     "/planeshift/userdata/options/shadows.xml"

#define LINES_IN_FRAME	13
#define NO_CHAT_TYPES	13
#define	SCROLLBAR_SIZE	15
#define SCROLLBAR_LINES NO_CHAT_TYPES * 4 + 4 - LINES_IN_FRAME
#define Y_START_POS		10
#define HEIGHT			20
#define X_ENABLED_POS	260
#define X_TEXT_POS		10
#define X_R_POS			140
#define X_G_POS			180
#define X_B_POS			220

pawsConfigShadows::pawsConfigShadows()
{
	enabled = NULL;
}

bool pawsConfigShadows::PostSetup()
{
    shadowManager = psengine->GetCelClient()->GetShadowManager();
    if(!shadowManager)
    {
        Error1("Couldn't find ShadowManager!");
        return false;
    }

	enabled = (pawsCheckBox*)FindWidget("enabled");
	if (!enabled) {
		Error1("Could not locate enabled widget!");
		return false;
	}
		
	drawFrame();
	
	return true;
}

void pawsConfigShadows::drawFrame()
{
	//Clear the frame by hiding all widgets
	enabled->Hide();
    enabled->Show();
}

bool pawsConfigShadows::Initialize()
{
    if ( ! LoadFromFile("configshadows.xml"))
        return false;
       
    return true;
}

bool pawsConfigShadows::LoadConfig()
{
	enabled->SetState(shadowManager->ShadowsEnabled());
    dirty = false;
	return true;
}

bool pawsConfigShadows::SaveConfig()
{
	//Save to file, then tell psChatBubbles to load it
	csString xml = "<?xml version=\"1.0\"?>\n";
	xml += "<!-- Simple file to disable/enable shadows drawn in-game.  It's a simple true/false attribute to switch -->\n";
	xml.AppendFmt("<shadows enabled=\"%s\">\n", enabled->GetState() ? "true" : "false");
    
	if (psengine->GetVFS()->WriteFile(USER_FILE, xml, xml.Length()))
		return shadowManager->Load(USER_FILE);
	else 
		return false;
    
    return true;
}

void pawsConfigShadows::SetDefault()
{
	shadowManager->Load(DEFAULT_FILE);
	LoadConfig();
    SaveConfig();
}

void pawsConfigShadows::Show()
{
	pawsWidget::Show();
}

void pawsConfigShadows::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    dirty = true;
}

