/*
 * pawscraftcancelwindow.cpp by Tapped
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

#include <psconfig.h>

// COMMON INCLUDES
#include "util/strutil.h"
#include "net/clientmsghandler.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawscraftcancelwindow.h"
#include "paws/pawsprogressbar.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsCraftCancelWindow::~pawsCraftCancelWindow()
{
    psengine->GetNetManager()->GetMsgHandler()->Unsubscribe(this, MSGTYPE_CRAFT_CANCEL);
}

bool pawsCraftCancelWindow::PostSetup()
{
    craftProgress        = (pawsProgressBar*)FindWidget("CraftProgress");
    if (!craftProgress)
        return false;

    craftProgress->SetTotalValue(1.0);
    craftProgress->SetCurrentValue(0.0);

    psengine->GetNetManager()->GetMsgHandler()->Subscribe(this, MSGTYPE_CRAFT_CANCEL);

    return true;
}

void pawsCraftCancelWindow::Draw()
{
    csTicks currentTime = csGetTicks();
 
    if ((currentTime-startTime) > craftTime)
    {
        craftTime = 0;
        Hide();
        return;
    }

    float delta = (float)(currentTime-startTime)/(float)craftTime;
    craftProgress->SetCurrentValue(delta);

    pawsWidget::Draw();
}

void pawsCraftCancelWindow::Start(csTicks craftTime)
{
    craftProgress->SetCurrentValue(0.0);
    startTime = csGetTicks();
    this->craftTime = craftTime * 1000;
    Show();
}


bool pawsCraftCancelWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if (!strcmp(widget->GetName(),"Cancel"))
    {
        Cancel();
        return true;
    }
    return false;
}

void pawsCraftCancelWindow::Cancel()
{
    psCraftCancelMessage msg;
    msg.SendMessage();
    
    craftTime = 0;
    Hide();
}

void pawsCraftCancelWindow::HandleMessage(MsgEntry* me)
{
    psCraftCancelMessage msg(me);
    Start(msg.craftTime);
}
