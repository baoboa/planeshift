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
#include "pawsconfigpvp.h"
#include "paws/pawsmanager.h"
#include "paws/pawsradio.h"

bool pawsConfigPvP::Initialize()
{
    if ( ! LoadFromFile("configpvp.xml"))
        return false;
       
    csRef<psCelClient> celclient = psengine->GetCelClient();
    assert(celclient);
    
    return true;
}

bool pawsConfigPvP::PostSetup()
{
    confirmRadioGroup = dynamic_cast<pawsRadioButtonGroup*> (FindWidget("confirm"));
    if (confirmRadioGroup == NULL)
        return false;

    return true;
}

bool pawsConfigPvP::LoadConfig()
{
    int confirmation = psengine->GetDuelConfirm();

    switch (confirmation)
    {
        case 0:
            confirmRadioGroup->SetActive("ConfirmNever");
            break;
        case 1:
            confirmRadioGroup->SetActive("ConfirmConfirm");
            break;
        case 2:
            confirmRadioGroup->SetActive("ConfirmAlways");
            break;
    }
            
    dirty = false;
    return true;
}

bool pawsConfigPvP::SaveConfig()
{
    int confirmation;
    /* Confirmation values:
     * 0 = Never
     * 1 = Confirm
     * 2 = Always
     */
    
    if (confirmRadioGroup->GetActive() == "ConfirmAlways")
        confirmation = 2;
    else if (confirmRadioGroup->GetActive() == "ConfirmNever")
        confirmation = 0;
    else
        confirmation = 1; //If no box is checked, then mark it as default (= Confirm)


    psengine->SetDuelConfirm(confirmation);
    dirty = false;
    return true;
}

void pawsConfigPvP::SetDefault()
{
    confirmRadioGroup->SetActive("ConfirmConfirm");
    dirty = true;
}

bool pawsConfigPvP::OnChange(pawsWidget * widget)
{
    if (widget == confirmRadioGroup)
        dirty = true;

    return true;
}
