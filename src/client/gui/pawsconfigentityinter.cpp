/*
 * pawsconfigentityinter.cpp - Author: Ondrej Hurt
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

// COMMON INCLUDES
#include "util/log.h"
#include "globals.h"

// CLIENT INCLUDES

// PAWS INCLUDES
#include "paws/pawsmanager.h"
#include "paws/pawscombo.h"
#include "gui/pawsconfigentityinter.h"

pawsConfigEntityInteraction::pawsConfigEntityInteraction()
{
    entTypes = dynamic_cast <psMainWidget*> (PawsManager::GetSingleton().GetMainWidget()) -> GetEntityTypes();
}

bool pawsConfigEntityInteraction::Initialize()
{
    return LoadFromFile("configentityinter.xml");
}

bool pawsConfigEntityInteraction::PostSetup()
{
    for (size_t i=0; i < entTypes->types.GetSize(); i++)
    {
        psEntityType * type = entTypes->types[i];
        pawsTextBox * txt = dynamic_cast <pawsTextBox*> (FindWidget(type->id+"_label"));
        if (txt == NULL)
            return false;
        txt->SetText(type->label+":");
        
        pawsComboBox * combo = dynamic_cast <pawsComboBox*> (FindWidget(type->id));
        if (combo == NULL)
            return false;
        
        for (size_t j=0; j < type->labels.GetSize(); j++)
            combo->NewOption(type->labels[j]);
    }
    return true;
}

bool pawsConfigEntityInteraction::LoadConfig()
{
    for (size_t i=0; i < entTypes->types.GetSize(); i++)
    {
        psEntityType * type = entTypes->types[i];
        pawsComboBox * combo = dynamic_cast <pawsComboBox*> (FindWidget(type->id));
        if (combo)
            combo->Select(type->usedCommand);
    }
    dirty = false;
    return true;
}

bool pawsConfigEntityInteraction::SaveConfig()
{
    for (size_t i=0; i < entTypes->types.GetSize(); i++)
    {
        psEntityType * type = entTypes->types[i];
        pawsComboBox * combo = dynamic_cast <pawsComboBox*> (FindWidget(type->id));
        if (combo)
            type->usedCommand = combo->GetSelectedRowNum();
    }
    entTypes->SaveConfigToFile();
    dirty = false;
    return true;
}

void pawsConfigEntityInteraction::SetDefault()
{
    for (size_t i=0; i < entTypes->types.GetSize(); i++)
    {
        psEntityType * type = entTypes->types[i];
        pawsComboBox * combo = dynamic_cast <pawsComboBox*> (FindWidget(type->id));
        if (combo)
            combo->Select(type->dflt);
        type->usedCommand = type->dflt;
    }
    entTypes->SaveConfigToFile();
    dirty = true;
}

void pawsConfigEntityInteraction::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    dirty = true;
}
