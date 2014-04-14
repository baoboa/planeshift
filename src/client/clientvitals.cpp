/*
 * clientvitals.cpp
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
#include "net/clientmsghandler.h"

#include "gui/chatwindow.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "clientvitals.h"
#include "pscharcontrol.h"
#include "globals.h"

psClientVitals::psClientVitals()
{
    counter = 0;
    counterSet = false;

    for(int i = 0; i < VITAL_COUNT; i++) //ticket 6048
    {
        vitals[i].drRate = 0;
    }
}

void psClientVitals::HandleDRData(psStatDRMessage& msg, const char *labelname )
{
    csString buff;

    // Skip out of date stat dr updates
    if (counterSet && (unsigned char)(msg.counter - counter) > 127)
    {
        Error4("Skipping out of date StatDR packet for '%s', version %d not %d.", labelname, msg.counter, counter);
        return;
    }
    else
    {
        counter = msg.counter;  // update for next time.
        counterSet = true;     // accept the first counter and drop anything out of date compared to that
    }

    if (msg.statsDirty & DIRTY_VITAL_HP)
    {
        vitals[VITAL_HITPOINTS].value = msg.hp;
        buff.Format("fVitalValue%d:%s",VITAL_HITPOINTS,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].value);
    }

    if (msg.statsDirty & DIRTY_VITAL_HP_RATE)
    {
        vitals[VITAL_HITPOINTS].drRate = msg.hp_rate;
        buff.Format("fVitalRate%d:%s",VITAL_HITPOINTS,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_MANA)
    {
        vitals[VITAL_MANA].value = msg.mana;
        buff.Format("fVitalValue%d:%s",VITAL_MANA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MANA].value);
    }

    if (msg.statsDirty & DIRTY_VITAL_MANA_RATE)
    {
        vitals[VITAL_MANA].drRate = msg.mana_rate;
        buff.Format("fVitalRate%d:%s",VITAL_MANA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MANA].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_PYSSTAMINA)
    {
        vitals[VITAL_PYSSTAMINA].value = msg.pstam;
        buff.Format("fVitalValue%d:%s",VITAL_PYSSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_PYSSTAMINA].value);
    }

    if (msg.statsDirty & DIRTY_VITAL_PYSSTAMINA_RATE)
    {
        vitals[VITAL_PYSSTAMINA].drRate = msg.pstam_rate;
        buff.Format("fVitalRate%d:%s",VITAL_PYSSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_PYSSTAMINA].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_MENSTAMINA)
    {
        vitals[VITAL_MENSTAMINA].value = msg.mstam;
        buff.Format("fVitalValue%d:%s",VITAL_MENSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MENSTAMINA].value);
    }

    if (msg.statsDirty & DIRTY_VITAL_MENSTAMINA_RATE)
    {
        vitals[VITAL_MENSTAMINA].drRate = msg.mstam_rate;
        buff.Format("fVitalRate%d:%s",VITAL_MENSTAMINA,labelname);
        PawsManager::GetSingleton().Publish(buff,vitals[VITAL_MENSTAMINA].drRate);
    }

    if (msg.statsDirty & DIRTY_VITAL_EXPERIENCE)
    {
        experiencePoints = (unsigned int)msg.exp;
        buff.Format("fExpPts:%s",labelname);
        PawsManager::GetSingleton().Publish(buff,(float)experiencePoints/200.0F);
    }

    if (msg.statsDirty & DIRTY_VITAL_PROGRESSION)
    {
        progressionPoints = (unsigned int)msg.prog;
        buff.Format("fProgrPts:%s",labelname);
        PawsManager::GetSingleton().Publish(buff,progressionPoints);
    }
}

void psClientVitals::HandleDeath(const char *labelname )
{
    csString buff;

    vitals[VITAL_HITPOINTS].drRate = 0.0;
    buff.Format("fVitalRate%d:%s",VITAL_HITPOINTS,labelname);
    PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].drRate);

    vitals[VITAL_HITPOINTS].value = 0.0;
    buff.Format("fVitalValue%d:%s",VITAL_HITPOINTS,labelname);
    PawsManager::GetSingleton().Publish(buff,vitals[VITAL_HITPOINTS].value);
}

void psClientVitals::Predict( csTicks now, const char *labelname )
{
    // Find delta time since last update
    float delta = (now-lastDRUpdate)/1000.0;
    lastDRUpdate = now;

    csString buff;

    // iterate over all fields and predict their values based on their recharge rate
    for ( int x = 0; x < VITAL_COUNT; x++ )
    {
        float oldvalue = vitals[x].value;
        vitals[x].value += vitals[x].drRate*delta;
        if ( vitals[x].value< 0 )
            vitals[x].value = 0;

        if ( vitals[x].value > 1.0 )
            vitals[x].value = 1.0;

        if (oldvalue != vitals[x].value) // need to republish info
        {
            buff.Format("fVitalValue%d:%s",x,labelname);
            PawsManager::GetSingleton().Publish(buff,vitals[x].value);
        }
    }
}
