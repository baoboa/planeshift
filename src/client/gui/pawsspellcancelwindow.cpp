/*
 * pawsspellcancelwindow.cpp
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

// COMMON INCLUDES
#include "net/messages.h"
#include "util/strutil.h"

// CLIENT INCLUDES
#include "../globals.h"

// PAWS INCLUDES
#include "pawsspellcancelwindow.h"
#include "paws/pawsprogressbar.h"
#include "paws/pawsmanager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsSpellCancelWindow::pawsSpellCancelWindow()
{
}

pawsSpellCancelWindow::~pawsSpellCancelWindow()
{
}

bool pawsSpellCancelWindow::PostSetup()
{
    spellProgress        = (pawsProgressBar*)FindWidget("SpellProgress");
    if (!spellProgress)
        return false;

    spellProgress->SetTotalValue(1.0);
    spellProgress->SetCurrentValue(0.0);

    return true;
}

void pawsSpellCancelWindow::Draw()
{
    csTicks currentTime = csGetTicks();
    if ((currentTime-startTime) > castingTime)
    {
        castingTime = 0;
        Hide();
        return;
    }
    float delta = (float)(currentTime-startTime)/(float)castingTime;
    spellProgress->SetCurrentValue(delta);

    pawsWidget::Draw();
}

void pawsSpellCancelWindow::Start(csTicks castingTime)
{
    spellProgress->SetCurrentValue(0.0);
    startTime = csGetTicks();
    this->castingTime = castingTime;
    Show();
}


bool pawsSpellCancelWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if (!strcmp(widget->GetName(),"Cancel"))
    {
        Cancel();
        return true;
    }
    return false;
}

void pawsSpellCancelWindow::Cancel()
{
    psSpellCancelMessage msg;
    msg.SendMessage();
    
    castingTime = 0;
    Hide();
}
