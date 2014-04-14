/*
 * servervitals.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//=============================================================================
// Project Includes
//=============================================================================
#include "net/msghandler.h"
#include "net/netbase.h"

#include "../gem.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "servervitals.h"
#include "pscharacter.h"

psServerVitals::psServerVitals(psCharacter* character)
{
    this->character = character;
    statsDirty = 0;
    version    = 0;

    // Set up callbacks for updating the dirty flag.
    vitals[VITAL_HITPOINTS].drRate.Initialize(&statsDirty,  DIRTY_VITAL_HP_RATE);
    vitals[VITAL_HITPOINTS].max.Initialize(&statsDirty,     DIRTY_VITAL_HP_MAX);
    vitals[VITAL_MANA].drRate.Initialize(&statsDirty,       DIRTY_VITAL_MANA_RATE);
    vitals[VITAL_MANA].max.Initialize(&statsDirty,          DIRTY_VITAL_MANA_MAX);
    vitals[VITAL_PYSSTAMINA].drRate.Initialize(&statsDirty, DIRTY_VITAL_PYSSTAMINA_RATE);
    vitals[VITAL_PYSSTAMINA].max.Initialize(&statsDirty,    DIRTY_VITAL_PYSSTAMINA_MAX);
    vitals[VITAL_MENSTAMINA].drRate.Initialize(&statsDirty, DIRTY_VITAL_MENSTAMINA_RATE);
    vitals[VITAL_MENSTAMINA].max.Initialize(&statsDirty,    DIRTY_VITAL_MENSTAMINA_MAX);

    //initialize values to a safe value:
    vitals[VITAL_HITPOINTS].value = 0;
    vitals[VITAL_MANA].value = 0;
    vitals[VITAL_PYSSTAMINA].value = 0;
    vitals[VITAL_MENSTAMINA].value = 0;

    vitals[VITAL_HITPOINTS].drRate.SetBase(HP_REGEN_RATE);
    vitals[VITAL_MANA].drRate.SetBase(MANA_REGEN_RATE);

    SetOrigVitals();
}

#define PERCENT_VALUE(v) vitals[v].max.Current() ? vitals[v].value / vitals[v].max.Current() : 0
#define PERCENT_RATE(v)  vitals[v].max.Current() ? vitals[v].drRate.Current() / vitals[v].max.Current() : 0
bool psServerVitals::SendStatDRMessage(uint32_t clientnum, EID eid, unsigned int flags, csRef<PlayerGroup> group)
{
    bool backup=0;
    if(flags)
    {
        backup = statsDirty ? true : false;
        statsDirty = flags;
    }
    else if(version % 10 == 0)   // every 10th msg to this person, send everything
    {
        statsDirty = DIRTY_VITAL_ALL;
    }

    if(!statsDirty)
        return false;

    csArray<float> fVitals;
    csArray<uint32_t> uiVitals;

    if(statsDirty & DIRTY_VITAL_HP)
        fVitals.Push(PERCENT_VALUE(VITAL_HITPOINTS));

    if(statsDirty & DIRTY_VITAL_HP_RATE)
        fVitals.Push(PERCENT_RATE(VITAL_HITPOINTS));

    if(statsDirty & DIRTY_VITAL_MANA)
        fVitals.Push(PERCENT_VALUE(VITAL_MANA));

    if(statsDirty & DIRTY_VITAL_MANA_RATE)
        fVitals.Push(PERCENT_RATE(VITAL_MANA));

    // Physical Stamina
    if(statsDirty & DIRTY_VITAL_PYSSTAMINA)
        fVitals.Push(PERCENT_VALUE(VITAL_PYSSTAMINA));

    if(statsDirty & DIRTY_VITAL_PYSSTAMINA_RATE)
        fVitals.Push(PERCENT_RATE(VITAL_PYSSTAMINA));

    // Mental Stamina
    if(statsDirty & DIRTY_VITAL_MENSTAMINA)
        fVitals.Push(PERCENT_VALUE(VITAL_MENSTAMINA));

    if(statsDirty & DIRTY_VITAL_MENSTAMINA_RATE)
        fVitals.Push(PERCENT_RATE(VITAL_MENSTAMINA));

    if(statsDirty & DIRTY_VITAL_EXPERIENCE)
        uiVitals.Push(GetExp());

    if(statsDirty & DIRTY_VITAL_PROGRESSION)
        uiVitals.Push(GetPP());

    psStatDRMessage msg(clientnum, eid, fVitals, uiVitals, ++version, statsDirty);
    if(group == NULL)
        msg.SendMessage();
    else
        group->Broadcast(msg.msg);

    statsDirty = backup;
    return true;
}

bool psServerVitals::Update(csTicks now)
{
    float delta;
    /* It is necessary to check when lastDRUpdate is 0 because, if not when a character login his stats
    are significantly incremented, which is instead unnecessary. As tested, delta cannot be 0 or the actors
    with 0 HP and negative drRate will enter a loop of death, but it has to be really small in order to guarantee
    a smooth increment of the stats. Dirty all vitals to force a stats update.*/
    if(!lastDRUpdate)
    {
        delta = 0.1f; //The value can be adjusted but it has to be really small.
        statsDirty = DIRTY_VITAL_ALL;
    }
    else
    {
        delta = (now-lastDRUpdate)/1000.0;
    }
    lastDRUpdate = now;

    // iterate over all fields and predict their values based on their recharge rate
    for(int i = 0; i < VITAL_COUNT; i++)
    {
        vitals[i].value += vitals[i].drRate.Current() * delta;
        ClampVital(i);
    }

    if(vitals[VITAL_HITPOINTS].value == 0 && vitals[VITAL_HITPOINTS].drRate.Current() < 0)
    {
        character->GetActor()->Kill(NULL);
    }

    // Return true if there are dirty vitals
    return (statsDirty) ? true : false;
}


void psServerVitals::SetExp(unsigned int W)
{
    experiencePoints = W;
    statsDirty |= DIRTY_VITAL_EXPERIENCE;
}

void psServerVitals::SetPP(unsigned int pp)
{
    progressionPoints = pp;
    statsDirty |= DIRTY_VITAL_PROGRESSION;
}

Vital &psServerVitals::DirtyVital(int vital, int dirtyFlag)
{
    statsDirty |= dirtyFlag;
    return GetVital(vital);
}

void psServerVitals::SetVital(int vital, int dirtyFlag, float value)
{
    DirtyVital(vital, dirtyFlag).value = value;
    ClampVital(vital);
}

void psServerVitals::AdjustVital(int vital, int dirtyFlag, float delta)
{
    DirtyVital(vital, dirtyFlag).value += delta;
    ClampVital(vital);
}

unsigned int psServerVitals::GetStatsDirtyFlags() const
{
    return statsDirty;
}

void psServerVitals::SetAllStatsDirty()
{
    statsDirty = DIRTY_VITAL_ALL;
}


void psServerVitals::ClearStatsDirtyFlags(unsigned int dirtyFlags)
{
    statsDirty &= ~dirtyFlags;
}

void psServerVitals::ClampVital(int v)
{
    if(vitals[v].value < 0)
        vitals[v].value = 0;

    if(vitals[v].value > vitals[v].max.Current())
        vitals[v].value = vitals[v].max.Current();
}

