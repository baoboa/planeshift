/*
* perceptions.cpp by Keith Fulton <keith@paqrat.com>
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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csgeom/transfrm.h>
#include <iutil/document.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "net/clientmsghandler.h"
#include "net/npcmessages.h"

#include "util/log.h"
#include "util/location.h"
#include "util/strutil.h"
#include "util/psutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "gem.h"
#include "globals.h"
#include "networkmgr.h"
#include "npc.h"
#include "npcbehave.h"
#include "npcclient.h"
#include "perceptions.h"

/*----------------------------------------------------------------------------*/

bool Perception::ShouldReact(Reaction* reaction, NPC* npc)
{
    if((GetName() == reaction->GetEventType()) && (reaction->GetType(npc).IsEmpty() || type == reaction->GetType(npc)))
    {
        return true;
    }
    return false;
}

Perception* Perception::MakeCopy()
{
    Perception* p = new Perception(name,type);
    return p;
}

void Perception::ExecutePerception(NPC* npc, float weight)
{
}

const csString &Perception::GetName() const
{
    return name;
}

const csString &Perception::GetType() const
{
    return type;
}

void Perception::SetType(const char* type)
{
    this->type = type;
}

bool Perception::GetLocation(csVector3 &pos, iSector* &sector)
{
    if(GetTarget())
    {
        psGameObject::GetPosition(GetTarget(),pos,sector);
        return true;
    }
    return false;
}


csString Perception::ToString(NPC* npc)
{
    csString result;
    result.Format("Name: '%s[%s]'",GetName().GetDataSafe(), type.GetDataSafe());
    return result;
}

//---------------------------------------------------------------------------------


bool FactionPerception::ShouldReact(Reaction* reaction, NPC* npc)
{
    if(name == reaction->GetEventType())
    {
        if(player)
        {
            if(!reaction->ShouldReact(player))
            {
                return false;
            }
        }

        if(reaction->GetOp() == '>')
        {
            NPCDebug(npc, 15, "Checking %d > %d.",factionDelta,reaction->GetFactionDiff());
            if(factionDelta > reaction->GetFactionDiff())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else if(reaction->GetOp() == '<')
        {
            NPCDebug(npc, 15, "Checking %d < %d.",factionDelta,reaction->GetFactionDiff());
            if(factionDelta < reaction->GetFactionDiff())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            NPCDebug(npc, 15, "Skipping faction check.");
            return true;
        }

    }
    return false;
}

Perception* FactionPerception::MakeCopy()
{
    FactionPerception* p = new FactionPerception(name,factionDelta,player);
    return p;
}

void FactionPerception::ExecutePerception(NPC* npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)player,weight*-factionDelta);
}

//---------------------------------------------------------------------------------

Perception* ItemPerception::MakeCopy()
{
    ItemPerception* p = new ItemPerception(name,item);
    return p;
}

//---------------------------------------------------------------------------------


bool LocationPerception::GetLocation(csVector3 &pos, iSector* &sector)
{
    if(location)
    {
        pos = location->pos;
        sector = location->GetSector(engine);
        return true;
    }
    return false;
}

Perception* LocationPerception::MakeCopy()
{
    LocationPerception* p = new LocationPerception(name, type, location, engine);
    return p;
}

float LocationPerception::GetRadius() const
{
    return location->radius;
}

//---------------------------------------------------------------------------------


bool PositionPerception::GetLocation(csVector3 &pos, iSector* &sector)
{
    pos = this->pos;
    sector = this->sector;
    return true;
}

Perception* PositionPerception::MakeCopy()
{
    PositionPerception* p = new PositionPerception(name,type,instance,sector,pos,yrot,radius);
    return p;
}

float PositionPerception::GetRadius() const
{
    return radius;
}

//---------------------------------------------------------------------------------

Perception* AttackPerception::MakeCopy()
{
    AttackPerception* p = new AttackPerception(name,attacker);
    return p;
}

void AttackPerception::ExecutePerception(NPC* npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)attacker,weight);
}
//---------------------------------------------------------------------------------

Perception* GroupAttackPerception::MakeCopy()
{
    GroupAttackPerception* p = new GroupAttackPerception(name,attacker_ents,bestSkillSlots);
    return p;
}

void GroupAttackPerception::ExecutePerception(NPC* npc,float weight)
{
    for(size_t i=0; i<attacker_ents.GetSize(); i++)
    {
        npc->AddToHateList((gemNPCActor*)attacker_ents[i],bestSkillSlots[i]*weight);
    }
}

//---------------------------------------------------------------------------------

Perception* DamagePerception::MakeCopy()
{
    DamagePerception* p = new DamagePerception(name,attacker,damage);
    return p;
}

void DamagePerception::ExecutePerception(NPC* npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)attacker,damage*weight);
}

//---------------------------------------------------------------------------------

SpellPerception::SpellPerception(const char* name,
                                 gemNPCObject* caster, gemNPCObject* target,
                                 const char* type, float severity)
    : Perception(name)
{
    this->caster = (gemNPCActor*) caster;
    this->target = (gemNPCActor*) target;
    this->spell_severity = severity;
    this->type = type;
}

bool SpellPerception::ShouldReact(Reaction* reaction, NPC* npc)
{
    csString eventName = GetName();

    NPCDebug(npc, 20, "Spell percpetion checking for match beween %s[%s] and %s[%s]",
             eventName.GetData(), type.GetDataSafe(),
             reaction->GetEventType().GetDataSafe(), reaction->GetType(npc).GetDataSafe());

    if(Perception::ShouldReact(reaction, npc))
    {
        // For target it has to be checked if this is actually a target of this npc that is casting
        if(eventName == "spell:target")
        {
            if(!npc->GetEntityHate((gemNPCActor*)caster) && !npc->GetEntityHate((gemNPCActor*)target))
            {
                NPCDebug(npc, 10, "Spell for target and target or caster in not on hate list!!");
                return false;
            }
        }

        NPCDebug(npc, 15, "%s spell cast by %s on %s, severity %1.1f.",
                 eventName.GetData(), (caster)?caster->GetName():"(Null caster)",
                 (target)?target->GetName():"(Null target)", spell_severity);

        return true;
    }
    return false;
}

Perception* SpellPerception::MakeCopy()
{
    SpellPerception* p = new SpellPerception(name,caster,target,type,spell_severity);
    return p;
}

void SpellPerception::ExecutePerception(NPC* npc,float weight)
{
    npc->AddToHateList((gemNPCActor*)caster,spell_severity*weight);
}

//---------------------------------------------------------------------------------

bool TimePerception::ShouldReact(Reaction* reaction, NPC* npc)
{
    if(name == reaction->GetEventType())
    {
        if(npc->IsDebugging(15))
        {
            csString dbgOut;
            dbgOut.AppendFmt("Time is now %d:%02d %d-%d-%d and I need ",
                             gameHour,gameMinute,gameYear,gameMonth,gameDay);
            // Hours
            if(reaction->GetValueValid(0))
            {
                dbgOut.AppendFmt("%d:",reaction->GetValue(0));
            }
            else
            {
                dbgOut.Append("*:");
            }
            // Minutes
            if(reaction->GetValueValid(1))
            {
                dbgOut.AppendFmt("%02d ",reaction->GetValue(1));
            }
            else
            {
                dbgOut.Append("* ");
            }
            // Year
            if(reaction->GetValueValid(2))
            {
                dbgOut.AppendFmt("%d-",reaction->GetValue(2));
            }
            else
            {
                dbgOut.Append("*-");
            }
            // Month
            if(reaction->GetValueValid(3))
            {
                dbgOut.AppendFmt("%d-",reaction->GetValue(3));
            }
            else
            {
                dbgOut.Append("*-");
            }
            // Day
            if(reaction->GetValueValid(4))
            {
                dbgOut.AppendFmt("%d",reaction->GetValue(4));
            }
            else
            {
                dbgOut.Append("*");
            }

            NPCDebug(npc, 15, dbgOut);
        }

        if((!reaction->GetValueValid(0) || reaction->GetValue(0) == gameHour) &&
                (!reaction->GetValueValid(1) || reaction->GetValue(1) == gameMinute) &&
                (!reaction->GetValueValid(2) || reaction->GetValue(2) == gameYear) &&
                (!reaction->GetValueValid(3) || reaction->GetValue(3) == gameMonth) &&
                (!reaction->GetValueValid(4) || reaction->GetValue(4) == gameDay))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

Perception* TimePerception::MakeCopy()
{
    TimePerception* p = new TimePerception(gameHour,gameMinute,gameYear,gameMonth,gameDay);
    return p;
}

csString TimePerception::ToString(NPC* npc)
{
    csString result;
    result.Format("Name: '%s' : '%d:%02d %d-%d-%d'",GetName().GetDataSafe(),
                  gameHour, gameMinute, gameYear, gameMonth, gameDay);
    return result;
}

void TimePerception::NormalizeReaction(Reaction* reaction)
{
    // Minutes
    if(reaction->GetValueValid(1))
    {
        int minutes = reaction->GetValue(1)%60;
        int hours = reaction->GetValue(1)/60;
        reaction->SetValue(1,minutes);
        if(reaction->GetValueValid(0))
        {
            reaction->SetValue(0,reaction->GetValue(0)+hours);
        }
    }
    // Hours
    if(reaction->GetValueValid(0))
    {
        int hours = reaction->GetValue(0)%24;
        int days = reaction->GetValue(0)/24;
        reaction->SetValue(0,hours);
        if(reaction->GetValueValid(2))
        {
            reaction->SetValue(2,reaction->GetValue(2)+days);
        }
    }
    // Day
    if(reaction->GetValueValid(2))
    {
        int days = reaction->GetValue(2)%32;
        int months = reaction->GetValue(2)/32;
        reaction->SetValue(2,days);
        if(reaction->GetValueValid(3))
        {
            reaction->SetValue(3,reaction->GetValue(3)+months);
        }
    }
    // Month
    if(reaction->GetValueValid(3))
    {
        int months = reaction->GetValue(3)%10;
        int years = reaction->GetValue(3)/10;
        reaction->SetValue(3,months);
        if(reaction->GetValueValid(4))
        {
            reaction->SetValue(4,reaction->GetValue(4)+years);
        }
    }
}

//---------------------------------------------------------------------------------

Perception* DeathPerception::MakeCopy()
{
    DeathPerception* p = new DeathPerception(who);
    return p;
}

void DeathPerception::ExecutePerception(NPC* npc,float weight)
{
    npc->RemoveFromHateList(who);
}

//---------------------------------------------------------------------------------
Perception* InventoryPerception::MakeCopy()
{
    InventoryPerception* p = new InventoryPerception(name,type,count,pos,sector,radius);
    return p;
}


/// NPC Pet Perceptions ===========================================================
//---------------------------------------------------------------------------------

OwnerCmdPerception::OwnerCmdPerception(const char* name,
                                       psPETCommandMessage::PetCommand_t command,
                                       gemNPCObject* owner,
                                       gemNPCObject* pet,
                                       gemNPCObject* target) : Perception(BuildName(command))
{
    this->command = command;
    this->owner = owner;
    this->pet = pet;
    this->target = (gemNPCActor*) target;
}

bool OwnerCmdPerception::ShouldReact(Reaction* reaction, NPC* npc)
{
    if(name == reaction->GetEventType())
    {
        return true;
    }
    return false;
}

Perception* OwnerCmdPerception::MakeCopy()
{
    OwnerCmdPerception* p = new OwnerCmdPerception(name, command, owner, pet, target);
    return p;
}

void OwnerCmdPerception::ExecutePerception(NPC* pet, float weight)
{
    gemNPCObject* t = NULL;
    if(target)
    {
        t = npcclient->FindEntityID(target->GetEID());
    }
    pet->SetTarget(t);

    switch(this->command)
    {
        case psPETCommandMessage::CMD_WALK : // Walk
        case psPETCommandMessage::CMD_RUN : // Run
        case psPETCommandMessage::CMD_FOLLOW : // Follow
        case psPETCommandMessage::CMD_STAY : // Stay
        case psPETCommandMessage::CMD_GUARD : // Guard
        case psPETCommandMessage::CMD_ASSIST : // Assist
        case psPETCommandMessage::CMD_NAME : // Name
        case psPETCommandMessage::CMD_TARGET : // Target
        case psPETCommandMessage::CMD_SUMMON : // Summon
        case psPETCommandMessage::CMD_DISMISS : // Dismiss
            break;

        case psPETCommandMessage::CMD_ATTACK : // Attack
            if(pet->GetTarget())
            {
                pet->AddToHateList((gemNPCActor*)target, 1 * weight);
            }
            else
            {
                NPCDebug(pet, 5, "No target to add to hate list");
            }

            break;

        case psPETCommandMessage::CMD_STOPATTACK : // StopAttack
            break;
        case psPETCommandMessage::CMD_LAST : // Last
            break;
    }
}

csString OwnerCmdPerception::BuildName(psPETCommandMessage::PetCommand_t command)
{
    csString event("ownercmd");
    event.Append(':');

    switch(command)
    {
        case psPETCommandMessage::CMD_FOLLOW:
            event.Append("follow");
            break;
        case psPETCommandMessage::CMD_STAY :
            event.Append("stay");
            break;
        case psPETCommandMessage::CMD_SUMMON :
            event.Append("summon");
            break;
        case psPETCommandMessage::CMD_DISMISS :
            event.Append("dismiss");
            break;
        case psPETCommandMessage::CMD_ATTACK :
            event.Append("attack");
            break;
        case psPETCommandMessage::CMD_STOPATTACK :
            event.Append("stopattack");
            break;
        case psPETCommandMessage::CMD_GUARD :
            event.Append("guard");
            break;
        case psPETCommandMessage::CMD_ASSIST :
            event.Append("assist");
            break;
        case psPETCommandMessage::CMD_NAME :
            event.Append("name");
            break;
        case psPETCommandMessage::CMD_TARGET :
            event.Append("target");
            break;
        case psPETCommandMessage::CMD_WALK:
            event.Append("walk");
            break;
        case psPETCommandMessage::CMD_RUN:
            event.Append("run");
            break;
        case psPETCommandMessage::CMD_LAST:
            event.Append("last");
            break;
    }
    return event;
}


//---------------------------------------------------------------------------------

OwnerActionPerception::OwnerActionPerception(const char* name  ,
        int action,
        gemNPCObject* owner ,
        gemNPCObject* pet) : Perception(name)
{
    this->action = action;
    this->owner = owner;
    this->pet = pet;
}

bool OwnerActionPerception::ShouldReact(Reaction* reaction, NPC* npc)
{
    csString event("ownercmd");
    event.Append(':');

    switch(this->action)
    {
        case 1:
            event.Append("attack");
            break;
        case 2:
            event.Append("damage");
            break;
        case 3:
            event.Append("logon");
            break;
        case 4:
            event.Append("logoff");
            break;
        default:
            event.Append("unknown");
            break;
    }

    if(event == reaction->GetEventType())
    {
        return true;
    }
    return false;
}

Perception* OwnerActionPerception::MakeCopy()
{
    OwnerActionPerception* p = new OwnerActionPerception(name, action, owner, pet);
    return p;
}

void OwnerActionPerception::ExecutePerception(NPC* pet, float weight)
{
    switch(this->action)
    {
        case 1: // Find and Set owner
            break;
        case 2: // Clear Owner and return to astral plane
            break;
    }
}

//---------------------------------------------------------------------------------
/// NPC Pet Perceptions ===========================================================


//---------------------------------------------------------------------------------

NPCCmdPerception::NPCCmdPerception(const char* command, NPC* self) : Perception(command)
{
    this->self = self;
}

bool NPCCmdPerception::ShouldReact(Reaction* reaction, NPC* npc)
{
    if(name == reaction->GetEventType() &&
            (name.StartsWith("npccmd:global:", true) ||
             (name.StartsWith("npccmd:self:", true) && npc == self)))
    {
        NPCDebug(npc, 15, "Matched reaction '%s' to perception '%s' with npc(%s).",
                 reaction->GetEventType().GetDataSafe(), name.GetData(), npc->GetName());
        return true;
    }
    else
    {
        NPCDebug(npc, 16, "No matched reaction '%s' with npc(%s) to perception '%s' with npc(%s).",
                 reaction->GetEventType().GetDataSafe(), npc->GetName(),
                 name.GetData(), self->GetName());
    }

    return false;
}

Perception* NPCCmdPerception::MakeCopy()
{
    NPCCmdPerception* p = new NPCCmdPerception(name, self);
    return p;
}

//---------------------------------------------------------------------------------
