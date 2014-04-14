/*
* Author: Andrew Robberts
*
* Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "pawseedit.h"
#include "eeditglobals.h"

#include <cstool/csview.h>

#include "eedittoolboxmanager.h"
#include "eedittoolbox.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawsgenericview.h"


pawsEEdit::pawsEEdit()
{
    preview         = 0;
}

pawsEEdit::~pawsEEdit()
{
}

iView *pawsEEdit::GetView()
{
    if (!preview)
        return NULL;
    
    return preview->GetView();
}

const char * pawsEEdit::GetMapFile() const
{ 
    if (preview) 
        return preview->GetMapName(); 
    return "";
}

bool pawsEEdit::LoadMap(const char * mapFile)
{
    return preview->LoadMap(mapFile);
}

bool pawsEEdit::PostSetup()
{
    preview      = (pawsGenericView *)   FindWidget("preview");     CS_ASSERT(preview);

    return true;
}

bool pawsEEdit::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget->GetID() == 0)
    {
        editApp->Exit();
        return true;
    }
    
    int toolboxID = widget->GetID()-101;
    if (toolboxID >= 0 && toolboxID < EEditToolbox::T_COUNT)
    {
        csString label;
        if (editApp->ToggleToolbox(toolboxID))
        {
            label += "Hide ";
        }
        else
        {
            label += "Show ";
        }
        label += editApp->GetToolboxManager()->GetToolbox(toolboxID)->GetName();
        ((pawsButton *)widget)->SetText(label);
        return true;
    }
    
    return false;
}
