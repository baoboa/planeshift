/*
 * pawsactionlocationwindow.cpp
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */

#include <psconfig.h>

// COMMON INCLUDES
#include "net/clientmsghandler.h"
#include "net/messages.h"

// CLIENT INCLUDES
#include "../globals.h"
#include "psclientchar.h"

// PAWS INCLUDES
#include "pawsactionlocationwindow.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsActionLocationWindow::pawsActionLocationWindow()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_VIEW_ACTION_LOCATION);
}

pawsActionLocationWindow::~pawsActionLocationWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_VIEW_ACTION_LOCATION);
}

bool pawsActionLocationWindow::PostSetup()
{
    name = (pawsTextBox*) FindWidget("Name");
    description = (pawsMultiLineTextBox*) FindWidget("Description");
    if (!description || !name)
        return false;
    return true;
}

void pawsActionLocationWindow::HandleMessage(MsgEntry* me)
{
    if (me->GetType() == MSGTYPE_VIEW_ACTION_LOCATION)
    {
        psViewActionLocationMessage msg(me);

        name->SetText(msg.name);
        description->SetText(msg.description);
        Show();
    }
}
