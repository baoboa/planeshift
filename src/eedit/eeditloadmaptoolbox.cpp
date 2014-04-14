/*
 * Author: Andrew Robberts
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

#include "eeditloadmaptoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"

EEditLoadMapToolbox::EEditLoadMapToolbox() : scfImplementationType(this)
{
}

EEditLoadMapToolbox::~EEditLoadMapToolbox()
{
}

void EEditLoadMapToolbox::SetMapFile(const char * mapFile)
{
    mapPath->SetText(mapFile);
}

void EEditLoadMapToolbox::Update(unsigned int elapsed)
{
}

size_t EEditLoadMapToolbox::GetType() const
{
    return T_LOAD_MAP;
}

const char * EEditLoadMapToolbox::GetName() const
{
    return "Load Map";
}
    
bool EEditLoadMapToolbox::PostSetup()
{
    mapPath   = (pawsEditTextBox *) FindWidget("map_path");         CS_ASSERT(mapPath);
    loadMap   = (pawsButton *) FindWidget("load_map_button");       CS_ASSERT(loadMap);
    browseMap = (pawsButton *) FindWidget("browse_map_button");     CS_ASSERT(browseMap);
    
    return true;
}

bool EEditLoadMapToolbox::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget == loadMap)
    {
        editApp->LoadMap(mapPath->GetText());
        return true;
    }
    else if (widget == browseMap)
    {
        pawsFileNavigation::Create(mapPath->GetText(),"world",
                                   new OnFileSelected(mapPath), "data/eedit/filenavigation.xml");
        return true;
    }

    return false;
}

void EEditLoadMapToolbox::OnFileSelected::Execute(const csString & text)
{
    size_t len = text.Length();
    if (len > 6)
    {
        char formatted[256];
        strcpy(formatted, text);
        formatted[len-6] = '\0';

        textBox->SetText(formatted);
//        editApp->LoadMap(formatted);
    }
}

