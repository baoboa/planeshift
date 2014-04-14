/*
 * psclientdr.h by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __PSCLIENTPERSIST_H__
#define __PSCLIENTPERSIST_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

class psCelClient;
class MsgEntry;
class gemActor;
class MathScript;
class PaladinJr;
class CacheManager;
class EntityManager;

class psServerDR : public MessageManager<psServerDR>
{
public:
    psServerDR(CacheManager* cachemanager, EntityManager* entitymanager);
    virtual ~psServerDR();

    bool Initialize();

    void SendPersist();

protected:

    void HandleDeadReckoning(MsgEntry* me,Client* client);

    /// If the entity was falling and stops falling, this is called.
    void HandleFallDamage(gemActor* actor,int clientnum, const csVector3 &pos, iSector* sector);
    void ResetPos(gemActor* actor);

    MathScript* calc_damage;
    PaladinJr* paladin;

    CacheManager* cacheManager;
    EntityManager* entityManager;
};

#endif

