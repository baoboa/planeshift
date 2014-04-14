/*
 * progressionmanager.h
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
#ifndef __PROGRESSIONMANAGER_H__
#define __PROGRESSIONMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csutil/hash.h>
#include <iutil/document.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/gameevent.h"

#include "bulkobjects/psskills.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "msgmanager.h"                 // Subscriber class

class psCharacter;
class ProgressionScript;

/**
 * \addtogroup server
 * @{ */

class ProgressionManager : public MessageManager<ProgressionManager>
{
public:

    ProgressionManager(ClientConnectionSet* ccs, CacheManager* cachemanager);

    virtual ~ProgressionManager();

    /**
     * Send the skill list to the client.
     *
     * @param client The client that the message is for.
     * @param forceOpen  If true it will force open the skills screen on the client.
     * @param focus The skill in focus.
     * @param isTraining Is training.
     */
    void SendSkillList(Client* client, bool forceOpen, PSSKILL focus = PSSKILL_NONE, bool isTraining = false);

    void StartTraining(Client* client, psCharacter* trainer);

    csHash<csString, csString> &GetAffinityCategories()
    {
        return affinitycategories;
    }

    // Internal utility functions for the progression system
    void QueueEvent(psGameEvent* event);
    void SendMessage(MsgEntry* me);
    void Broadcast(MsgEntry* me);

    ProgressionScript* FindScript(char const* name);

    /**
     * Load progression script from db
     */
    bool Initialize(iObjectRegistry* object_reg);

    bool progressionRequiresTraining;
    int progressionMaxSkillValue;
    int progressionMaxStatValue;

protected:

    void HandleSkill(MsgEntry* me, Client* client);
    void HandleDeathEvent(MsgEntry* me, Client* notused);
    void HandleZPointEvent(MsgEntry* me, Client* client);

    void AllocateKillDamage(gemActor* deadActor, int exp);

    csHash<csString, csString> affinitycategories;
    ClientConnectionSet*    clients;
    MathScript* calc_dynamic_experience; ///< Math script used to calculate the dynamic experience
    CacheManager* cacheManager;
};

/** @} */

#endif

