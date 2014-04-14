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
#include "pawsconfigmarriage.h"
#include "paws/pawsmanager.h"
#include "paws/pawsradio.h"

bool pawsConfigMarriage::Initialize()
{
    if(!LoadFromFile("configmarriage.xml"))
        return false;

    return true;
}

bool pawsConfigMarriage::PostSetup()
{
    confirmRadioGroup = dynamic_cast<pawsRadioButtonGroup*>(FindWidget("confirmmarriage"));
    if(confirmRadioGroup == NULL)
        return false;

    return true;
}

bool pawsConfigMarriage::LoadConfig()
{
    bool confirm = psengine->GetMarriageProposal();

    if(confirm)
    {
        confirmRadioGroup->SetActive("ConfirmMarriageEach");
    }
    else
    {
        confirmRadioGroup->SetActive("ConfirmMarriageNever");
    }
    dirty = false;
    return true;
}

bool pawsConfigMarriage::SaveConfig()
{
    bool confirm;
    if(confirmRadioGroup->GetActive() == "ConfirmMarriageEach")
        confirm = true;
    else
        confirm = false;

    psengine->SetMarriageProposal(confirm);
    dirty = false;
    return true;
}

void pawsConfigMarriage::SetDefault()
{
    confirmRadioGroup->SetActive("ConfirmMarriageEach");
    dirty = true;
}

bool pawsConfigMarriage::OnChange(pawsWidget* widget)
{
    if(widget == confirmRadioGroup)
        dirty = true;

    return true;
}
