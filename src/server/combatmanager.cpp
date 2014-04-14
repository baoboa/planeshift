/*
 * combatmanager.cpp
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
#include <psconfig.h>
//=============================================================================
// Project Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/location.h"
#include "util/mathscript.h"
#include "engine/psworld.h"


//=============================================================================
// Local Includes
//=============================================================================
#include "events.h"
#include "gem.h"
#include "entitymanager.h"
#include "npcmanager.h"
#include "combatmanager.h"
#include "netmanager.h"
#include "globals.h"
#include "psserverchar.h"
#include "cachemanager.h"
#include "bulkobjects/psattack.h"


/**
 * When a player or NPC attacks someone, the first combat is queued.  When
 * that event is processed, if appropriate at the end, the next combat event
 * is immediately created and added to the schedule to be triggered at the
 * appropriate time.
 *
 * Note that this event is just a hint that a combat event might occur at this
 * time.  Other events (like equiping items, removing items, spell effects, etc)
 * may alter this time.
 *
 * When a combat event fires, the first task is to check whether the action can actually
 * occur at this time.  If it cannot, the event should not create another event since
 * the action that caused the "delay" or "acceleration" change should have created its
 * own event.
 *
 * TODO: psGEMEvent makes this depend only on the attacker when in fact
 * this event depends on both attacker and target being present at the time the
 * event fires.
 */

CombatManager::CombatManager(CacheManager* cachemanager, EntityManager* entitymanager) : pvp_region(NULL)
{
    randomgen = psserver->rng;
    cacheManager = cachemanager;
    entityManager = entitymanager;


    targetLocations.Push(PSCHARACTER_SLOT_HELM);
    targetLocations.Push(PSCHARACTER_SLOT_TORSO);
    targetLocations.Push(PSCHARACTER_SLOT_ARMS);
    targetLocations.Push(PSCHARACTER_SLOT_GLOVES);
    targetLocations.Push(PSCHARACTER_SLOT_LEGS);
    targetLocations.Push(PSCHARACTER_SLOT_BOOTS);


    Subscribe(&CombatManager::HandleDeathEvent, MSGTYPE_DEATH_EVENT, NO_VALIDATION);
    Subscribe(&CombatManager::sendAttackList, MSGTYPE_ATTACK_BOOK, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&CombatManager::sendAttackQueue, MSGTYPE_ATTACK_QUEUE, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
}

CombatManager::~CombatManager()
{
    if(pvp_region)
    {
        delete pvp_region;
        pvp_region = NULL;
    }
}

bool CombatManager::InitializePVP()
{
    Result rs(db->Select("select * from sc_location_type where name = 'pvp_region'"));

    if(!rs.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError());
        return false;
    }

    // no PVP defined
    if(rs.Count() == 0)
    {
        return true;
    }

    if(rs.Count() > 1)
    {
        Error1("More than one pvp_region defined!");
        // return false; // not really fatal
    }

    LocationType* loctype = new LocationType();

    if(loctype->Load(rs[0],NULL,db))
    {
        pvp_region = loctype;
    }
    else
    {
        Error2("Could not load location: %s",db->GetLastError());
        delete loctype;
        // return false; // not fatal
    }
    return true;
}

bool CombatManager::InPVPRegion(csVector3 &pos,iSector* sector)
{
    if(pvp_region && pvp_region->CheckWithinBounds(entityManager->GetEngine(), pos, sector))
        return true;

    return false;
}

const Stance &CombatManager::GetStance(CacheManager* cachemanager, csString name)
{
    name.Downcase();
    size_t id = cachemanager->stanceID.Find(name);
    if(id == csArrayItemNotFound)
    {
        //getting global default combat stance
        id = cachemanager->stanceID.Find(cachemanager->getOption("combat:default_stance")->getValue());

        if(id == csArrayItemNotFound)
        {
            id = cachemanager->stanceID.Find("normal");
        }

    }
    return cachemanager->stances.Get(id);
}

const Stance &CombatManager::GetLoweredActorStance(CacheManager* cachemanager, gemActor* attacker)
{
    Stance currentStance = attacker->GetCombatStance();

    if(currentStance.stance_id == (cachemanager->stances.GetSize()-1)) // The lowest possible stance is already choosen
    {
        // Propably nothing to do here
        return cachemanager->stances.Get(currentStance.stance_id);
    }

    return cachemanager->stances.Get(currentStance.stance_id+1);
}

const Stance &CombatManager::GetRaisedActorStance(CacheManager* cachemanager, gemActor* attacker)
{
    Stance currentStance = attacker->GetCombatStance();

    if(currentStance.stance_id == 1) // The greatest possible stance is already choosen
    {
        // Propably nothing to do here
        return cachemanager->stances.Get(currentStance.stance_id);
    }

    return cachemanager->stances.Get(currentStance.stance_id-1);
}

bool CombatManager::AttackSomeone(gemActor* attacker,gemActor* target,const Stance& stance)
{
    psCharacter* attackerChar = attacker->GetCharacterData();

    //we don't allow an overweight or defeated char to fight
    if(attacker->GetMode() == PSCHARACTER_MODE_DEFEATED ||
            attacker->GetMode() == PSCHARACTER_MODE_OVERWEIGHT)
        return false;

    if(attacker->GetMode() == PSCHARACTER_MODE_COMBAT)   // Already fighting
    {
        SetCombat(attacker, stance);  // switch stance from Bloody to Defensive, etc.
        return true;
    }
    else
    {
        if(attacker->GetMode() == PSCHARACTER_MODE_SIT)  //we are sitting force the char to stand
            attacker->Stand();
        attackerChar->ResetSwings(csGetTicks());
    }

    bool startedAttacking=false;
    bool haveWeapon=false;

    for(int slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
    {
        // See if this slot is able to attack
        if(attackerChar->Inventory().CanItemAttack((INVENTORY_SLOT_NUMBER) slot))
        {
            INVENTORY_SLOT_NUMBER weaponSlot = (INVENTORY_SLOT_NUMBER) slot;
            psItem* weapon = attackerChar->Inventory().GetEffectiveWeaponInSlot(weaponSlot);
            haveWeapon = true;
            csString response;
            // Note that we don't update the client's view until
            // the attack is executed.
            psAttack* attack = attackerChar->GetAttackQueue()->First();
            if(!attack || !attack->CanAttack(attacker->GetClient()))
            {
                // Use the default attack type.
                attack = psserver->GetCacheManager()->GetAttackByID(1);
            }
            if(weapon && weapon->CheckRequirements(attackerChar, response))
            {
                Debug5(LOG_COMBAT,attacker->GetClientID(),"%s tries to attack with %s weapon %s at %.2f range", attacker->GetName(),(weapon->GetIsRangeWeapon()?"range":"melee"),weapon->GetName(), attacker->RangeTo(target,false));
                Debug3(LOG_COMBAT,attacker->GetClientID(),"%s started attacking with %s",attacker->GetName(), weapon->GetName());
                //start the first attack
                attack->Attack(attacker, target, weaponSlot);
                startedAttacking = true;
            }
            else if(weapon)
            {
                Debug3(LOG_COMBAT,attacker->GetClientID(),"%s tried attacking with %s but can't use it.",
                       attacker->GetName(),weapon->GetName());
            }
        }
    }

    /* Only notify the target if any attacks were able to start.  Otherwise there are
    * no available weapons with which to attack.
    */
    if(haveWeapon)
    {
        if(startedAttacking)
        {
            // The attacker should now enter combat mode
            if(attacker->GetMode() != PSCHARACTER_MODE_COMBAT)
            {
                SetCombat(attacker,stance);
            }
        }
        else
        {
            psserver->SendSystemError(attacker->GetClientID(),"You are too far away to attack!");
            return false;
        }
    }
    else
    {
        psserver->SendSystemError(attacker->GetClientID(),"You have no weapons equipped!");
        return false;
    }

    return true;
}

void CombatManager::SetCombat(gemActor* combatant, const Stance& stance)
{
    // Sanity check
    if(!combatant || !combatant->GetCharacterData() || !combatant->IsAlive())
        return;

    // Change stance if new and for a player (NPCs don't have different stances)
    if(combatant->GetClientID() && combatant->GetCombatStance().stance_id != stance.stance_id)
    {
        psSystemMessage msg(combatant->GetClientID(), MSG_COMBAT_STANCE, "%s changes to a %s stance", combatant->GetName(), stance.stance_name.GetData());
        msg.Multicast(combatant->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);

        combatant->SetCombatStance(stance);
    }

    combatant->SetMode(PSCHARACTER_MODE_COMBAT); // Set mode and multicast new mode and/or stance
    Debug3(LOG_COMBAT,combatant->GetClientID(), "%s starts attacking with stance %s", combatant->GetName(), stance.stance_name.GetData());
}

void CombatManager::StopAttack(gemActor* attacker)
{
    if(!attacker)
        return;

    // TODO: I'm not sure this is a wise idea after all...spells may not be offensive...
    switch(attacker->GetMode())
    {
        case PSCHARACTER_MODE_SPELL_CASTING:
            attacker->InterruptSpellCasting();
            break;
        case PSCHARACTER_MODE_COMBAT:
            // Clear the queue of attacks
            attacker->GetCharacterData()->GetAttackQueue()->Purge();
            attacker->SetMode(PSCHARACTER_MODE_PEACE);
            break;
        default:
            return;
    }

    Debug2(LOG_COMBAT, attacker->GetClientID(), "%s stops attacking", attacker->GetName());
}

void CombatManager::NotifyTarget(gemActor* attacker,gemObject* target)
{
    // Queue Attack percetion to npc clients
    gemNPC* targetnpc = dynamic_cast<gemNPC*>(target);
    if(targetnpc)
        psserver->GetNPCManager()->QueueAttackPerception(attacker,targetnpc);
}

void CombatManager::HandleDeathEvent(MsgEntry* me,Client* client)
{
    psDeathEvent death(me);

    Debug1(LOG_COMBAT,death.deadActor->GetClientID(),"Combat Manager handling Death Event\n");

    // Stop any duels.
    if(death.deadActor->GetClient())
    {
        death.deadActor->GetClient()->ClearAllDuelClients();
    }

    // Clear the queue of attacks
    death.deadActor->GetCharacterData()->GetAttackQueue()->Purge();
    
    // set out of combat mode for char 
    death.deadActor->SetMode(PSCHARACTER_MODE_PEACE);

    // Stop actor moving.
    death.deadActor->StopMoving();

    // Send out the notification of death, which plays the anim, etc.
    psCombatEventMessage die(death.deadActor->GetClientID(),
                             psCombatEventMessage::COMBAT_DEATH,
                             death.killer ? death.killer->GetEID() : 0,
                             death.deadActor->GetEID(),
                             -1, // no target location
                             0,  // no dmg on a death
                             (unsigned int)-1,  // TODO: "killing blow" matrix of mob-types vs. weapon types
                             (unsigned int)-1);  // Death anim on client side is handled by the death mode message

    die.Multicast(death.deadActor->GetMulticastClients(),0,MAX_COMBAT_EVENT_RANGE);
}

void CombatManager::sendAttackQueue(MsgEntry* me, Client* client)
{
    sendAttackQueue(client->GetCharacterData());
}

void CombatManager::sendAttackQueue(psCharacter* character)
{
    if(!character->IsNPC())
    {
        Client* client = character->GetActor()->GetClient();
        if(!client)
            return;
        psAttackQueueMessage mesg(client->GetClientNum());
        psAttackQueue* attackqueue = client->GetCharacterData()->GetAttackQueue();
        csList<csRef<psAttack> >::Iterator it(attackqueue->getAttackList());
        while(it.HasNext())
        {
            psAttack* attack = it.Next();
            mesg.AddAttack(attack->GetName(),attack->GetImage());
        }

        mesg.Construct(cacheManager->GetMsgStrings());
        mesg.SendMessage();
    }
}

void CombatManager::sendAttackList(MsgEntry* me, Client* client)
{
    psAttackBookMessage mesg(client->GetClientNum());
    csArray<psAttack*> attacks = psserver->GetCacheManager()->GetAllAttacks();

    for(size_t i = 0; i < attacks.GetSize(); i++)
    {
        if(attacks[i]->CanAttack(client))
        {
            csString aName = attacks[i]->GetName();
            csString aDesc = attacks[i]->GetDescription();
            csString aType = attacks[i]->GetType()->name;
            csString aimage = attacks[i]->GetImage();
            mesg.AddAttack(aName,aDesc,aType,aimage);
        }
    }
    mesg.Construct(cacheManager->GetMsgStrings());
    mesg.SendMessage();

}
/*-------------------------------------------------------------*/

psSpareDefeatedEvent::psSpareDefeatedEvent(gemActor* losr) : psGameEvent(0, SECONDS_BEFORE_SPARING_DEFEATED * 1000, "psSpareDefeatedEvent")
{
    loser = losr->GetClient();
}

void psSpareDefeatedEvent::Trigger()
{
    // Ignore stale events: perhaps the character was already killed and resurrected...
    if(!loser.IsValid() || !loser->GetActor() || loser->GetActor()->GetMode() != PSCHARACTER_MODE_DEFEATED)
        return;

    psserver->SendSystemInfo(loser->GetClientNum(), "Your opponent has spared your life.");
    loser->ClearAllDuelClients();
    loser->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);
}
