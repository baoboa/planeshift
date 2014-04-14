/*
 * psattack.cpp              author: hartra344 [at] gmail [dot] com
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef psAttack_HEADER
#define psAttack_HEADER
//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <csutil/csstring.h>
#include <csutil/weakreferenced.h>
//====================================================================================
// Project Includes
//====================================================================================
#include <idal.h>
#include "util/scriptvar.h"
#include "util/gameevent.h"


//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"
#include "psitemstats.h"
#include "psitem.h"
#include "deleteobjcallback.h"
#include "combatmanager.h"

using namespace CS;

class psQuestPrereqOp;
class psCharacter;
class psAttack;
class Client;
class gemActor;
class ProgressionScript;
class MathExpression;
class CombatManager;
struct Stance;
class psCombatAttackGameEvent;

/** This stuct holds data for generic attack types
 * This could be for example an assassin attack which would require maybe daggers. and require dual wielded daggers at that, so each dagger does part of the attack specifically.
 * each attack type would also be based off a specic skill, such as strength or agility.
 */
struct psAttackType
{
    unsigned int           id;/// the id number, corresponding to its id in the database
    csString               name;/// the name of the attack type
    csString               weapon;/// if it requires 1 specific weapon, maybe its an attack that is special adn specific to 1 particular weapon.
    csArray<psWeaponType*> weaponTypes;/// if it requires multiple types of weapons they would be listed here.
    bool                   OneHand;/// if it is a dual weilding based attack it willb e flagged here, that means it requires 2 weapons and each will perform part of the attack, if it is false then only one hand will execute the attack.
    PSSKILL                related_stat; /// each attack will have a stat related to it that will be part of the power computation

};

struct psAttackCost// may be expanded later if more balance is needed
{
    float physStamina;
};

/**
 * Represents an Attack.  This is mostly data that is cached from the
 * database to represent what an attack is.
 *
 * This is going to be very close to how a spell works but more in touch with melee and range style.
 */
class psAttack : public csRefCount
{
public:
    psAttack();
    ~psAttack();

    /**
     * Initialize the attack from the database entry.
     */
    bool Load(iResultRow& row);

    /**
     * This is a check to see if the attack can be used by the character.
     */
    bool CanAttack(Client* client);

    /**
     * Schedules an attack.
     */
    bool Attack(gemActor* attacker, gemActor* target, INVENTORY_SLOT_NUMBER slot);

    /**
     * Once the combat event is called, this meathod runs preattack checks
     * and runs all calculations needed before the combat event is applied.
     */
    virtual void Affect(psCombatAttackGameEvent* event);

    /** gets the id
     * @return the attack id
     */
    int GetID() const
    {
        return id;
    }

    /** Gets the Name
     * @return returns the name of attack
     */
    const csString &GetName() const
    {
        return name;
    }

    /** Gets the attack icon string
     *@return attack icon image string
     */
    const csString &GetImage() const
    {
        return image;
    }

    /** Gets the attack description
     *  @return attack description
     */
    const csString &GetDescription() const
    {
        return description;
    }

    psAttackType* GetType() const
    {
        return type;
    }

protected:
    int CalculateAttack(psCombatAttackGameEvent* event, psItem* subWeapon = NULL);
    void AffectTarget(gemActor* target, psCombatAttackGameEvent* event, int attack_result);

    int id;                     ///< The stored ID of the attack
    csString name;              ///< The stored name of the attack
    csString image;             ///< the address/id of the icon for the attack
    csString animation;         ///< possible attack animation
    csString description;       ///<the attack description
    psAttackType* type;         ///< the attack type
    csRef<psQuestPrereqOp> requirements; ///< all non weapon based attack requirements.
    csRef<psQuestPrereqOp> TypeRequirements; ///< all Attack Type based requirements(not handled as a script)

    /// Delay in milliseconds before attack "hits" or has an effect.
    MathScript* attackDelay;
    /// The Max Range of the attack (Meters)
    MathExpression* attackRange;
    /// AOE Radius: (Power, WaySkill, RelatedStat) -> Meters
    MathExpression* aoeRadius;
    /// AOE Angle: (Power, WaySkill, RelatedStat) -> Degrees
    MathExpression* aoeAngle;
    /// Chance of Success
    MathScript* damage_script;
    /// The progression script: (Power, Caster, Target) -> (side effects)
    ProgressionScript* outcome;
};

//-----------------------------------------------------------------------------

/**
 * This event actually triggers an attack
 */
class psCombatAttackGameEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    psCombatAttackGameEvent(csTicks delayticks,
                            psAttack* attack,
                            gemActor* attacker,
                            gemActor* target,
                            INVENTORY_SLOT_NUMBER weaponslot,
                            psItem* weapon);

    virtual ~psCombatAttackGameEvent();

    virtual void Trigger();  // Abstract event processing function

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    psAttack* GetAttack()
    {
        return attack;
    }

    gemActor* GetAttacker()
    {
        return attacker;
    }

    gemActor* GetTarget()
    {
        return target;
    }

    INVENTORY_SLOT_NUMBER GetWeaponSlot()
    {
        return weaponSlot;
    }

    psItem* GetWeapon()
    {
        return weapon;
    }

    uint32_t GetTargetID()
    {
        return TargetCID;
    }

    uint32_t GetAttackerID()
    {
        return AttackerCID;
    }

    // These are just here to make it easy to pass data from Affect().
    MathEnvironment env;
    INVENTORY_SLOT_NUMBER AttackLocation;  ///< Which slot should we check the armor of?
    float FinalDamage;               ///< Final damage applied to target

protected:
    psAttack* attack;                ///< The attack
    csWeakRef<gemActor> attacker;    ///< Entity who instigated this attack
    csWeakRef<gemActor> target;      ///< Entity who is target of this attack
    uint32_t AttackerCID;            ///< ClientID of attacker
    uint32_t TargetCID;              ///< ClientID of target

    INVENTORY_SLOT_NUMBER weaponSlot; ///< Identifier of the slot for which this attack event should process
    // XXX There is some risk this pointer could point to trash
    // if the item is deleted/stored.
    psItem* weapon;                   ///< the attacking weapon
};

#endif
