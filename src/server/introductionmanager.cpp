/*
* introductionmanager.cpp
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>
#include <csutil/set.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharacter.h"

#include "util/psdatabase.h"
#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "introductionmanager.h"
#include "chatmanager.h"
#include "client.h"
#include "clients.h"
#include "gem.h"
#include "globals.h"
#include "psserver.h"
#include "globals.h"



IntroductionManager::IntroductionManager()
{
    Subscribe(&IntroductionManager::HandleIntroduction, MSGTYPE_INTRODUCTION, REQUIRE_READY_CLIENT);
}

IntroductionManager::~IntroductionManager()
{
    //do nothing
}

void IntroductionManager::HandleIntroduction(MsgEntry* me, Client* client)
{
    psCharIntroduction msg(me);
    if(!msg.valid)
        return;

    csArray<PublishDestination> &dest = client->GetActor()->GetMulticastClients();
    for(size_t i = 0; i < dest.GetSize(); i++)
    {
        if(dest[i].dist < CHAT_SAY_RANGE && client->GetClientNum() != (uint32_t) dest[i].client)
        {
            gemObject* obj = (gemObject*) dest[i].object;
            gemActor* destActor = obj->GetActorPtr();
            if(destActor && destActor->GetCharacterData()->Introduce(client->GetCharacterData()))
            {
                client->GetActor()->Send(dest[i].client, false, false);
            }
        }
    }
}

