/*
 * combatmanager.h
 *
 * Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __COMBATMANAGER_H__
#define __COMBATMANAGER_H__

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"


#define SECONDS_BEFORE_SPARING_DEFEATED 30

class psCombatGameEvent;
struct Stance;

enum COMBATMANAGER_ATTACKTYPE
{
    ATTACK_NOTCALCULATED = -1,
    ATTACK_DAMAGE,
    ATTACK_DODGED,
    ATTACK_BLOCKED,
    ATTACK_MISSED,
    ATTACK_OUTOFRANGE,
    ATTACK_BADANGLE,
    ATTACK_OUTOFAMMO
};

class LocationType;
class MathScriptEngine;
class MathScript;

/**
 *  This class handles all calculations around combat, using statistics
 *  and information from the pspccharacterinfo Prop Classes for both
 *  the attacker and the target.
 */
class CombatManager: public MessageManager<CombatManager>
{
public:

    CombatManager(CacheManager* cachemanager, EntityManager* entitymanager);
    bool InitializePVP();

    virtual ~CombatManager();

    /// This is how you start an attack sequence
    bool AttackSomeone(gemActor* attacker, gemActor* target, const Stance& stance);

    /// This is how you break an attack sequence off, through death or user command.
    void StopAttack(gemActor* attacker);

    bool InPVPRegion(csVector3 &pos, iSector* sector);

    /** @brief Gets combat stance by name
     *
     *  If combat stance name is invalid returns default global stance
     *
     *  @param cachemanager: manager that holds details about stances from database.
     *  @param name: name of requested stance
     *  @return Returns combat stance.
     */
    static const Stance &GetStance(CacheManager* cachemanager, csString name);

    /** @brief Gets increased combat stance of particular attacker
     *
     *  If current combat stance is the highest returns current combat stance
     *
     *  @param cachemanager: manager that holds details about stances from database.
     *  @param attacker: gemActor that requests to raise his stance
     *  @return Returns combat stance.
     */
    static const Stance &GetRaisedActorStance(CacheManager* cachemanager, gemActor* attacker);

    /** @brief Gets increased combat stance of particular attacker
     *
     *  If current combat stance is the lowest returns current combat stance
     *
     *  @param cachemanager: manager that holds details about stances from database.
     *  @param attacker: gemActor that requests to lower his stance
     *  @return Returns combat stance.
     */
    static const Stance &GetLoweredActorStance(CacheManager* cachemanager, gemActor* attacker);

    EntityManager* GetEntityManager()
    {
        return entityManager;
    };

    csArray<INVENTORY_SLOT_NUMBER> targetLocations;
    void SetCombat(gemActor* combatant, const Stance& stance);
    void NotifyTarget(gemActor* attacker, gemObject* target);
    void sendAttackList(MsgEntry* me, Client* client);
    void sendAttackQueue(MsgEntry* me, Client* client);
    void sendAttackQueue(psCharacter* character);
private:
    csRandomGen* randomgen;
    LocationType* pvp_region;
    CacheManager* cacheManager;
    EntityManager* entityManager;


    void HandleDeathEvent(MsgEntry* me,Client* client);

};

class psSpareDefeatedEvent: public psGameEvent
{
public:
    psSpareDefeatedEvent(gemActor* losr);
    void Trigger();

protected:
    csWeakRef<Client> loser;

};

#endif
