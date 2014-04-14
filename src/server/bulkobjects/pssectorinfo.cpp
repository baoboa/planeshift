/*
 * pssectorinfo.cpp
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pssectorinfo.h"
#include "util/mathscript.h"

psSectorInfo::psSectorInfo()
{
    uid = 0;
    is_raining = false;
    is_snowing = false;
    current_rain_drops = 0;
    fog_density = 0;
    fog_density_old = 0;
    densitySaved = false;
    fogFade = 0;
    say_range = 0;
    god_name = "";
    is_teleporting = false;
    has_penalty = false;
    teleportingSector = "";
    deathSector = "";
    teleportingCords = csVector3(0,0,0);
    deathCords = csVector3(0,0,0);
    deathRot = 0;
    teleportingRot = 0;
    deathRestoreHP = true;
    deathRestoreMana = true;
}

psSectorInfo::~psSectorInfo()
{
}

void psSectorInfo::SetWeatherEnabled(unsigned int id, bool newStatus)
{
    weatherTypeData* data = weatherData.GetElementPointer(id);
    data->enabled = newStatus;
}

bool psSectorInfo::GetWeatherEnabled(unsigned int id)
{
    weatherTypeData* data = weatherData.GetElementPointer(id);
    return (data->enabled && data->min_density > 0 && data->min_gap > 0);
}

unsigned int psSectorInfo::GetRandomWeatherGap(unsigned int id)
{
    //it can't be null at least for now
    weatherTypeData* data = weatherData.GetElementPointer(id);
    return psserver->GetRandom(data->max_gap-data->min_gap) + data->min_gap;
}

unsigned int psSectorInfo::GetRandomWeatherDuration(unsigned int id)
{
    //it can't be null at least for now
    weatherTypeData* data = weatherData.GetElementPointer(id);
    return psserver->GetRandom(data->max_duration-data->min_duration) + data->min_duration;
}

unsigned int psSectorInfo::GetRandomWeatherDensity(unsigned int id)
{
    //it can't be null at least for now
    weatherTypeData* data = weatherData.GetElementPointer(id);
    return psserver->GetRandom(data->max_density-data->min_density) + data->min_density;
}

unsigned int psSectorInfo::GetRandomWeatherFadeIn(unsigned int id)
{
    //it can't be null at least for now
    weatherTypeData* data = weatherData.GetElementPointer(id);
    return psserver->GetRandom(data->max_fade_in-data->min_fade_in) + data->min_fade_in;
}

unsigned int psSectorInfo::GetRandomWeatherFadeOut(unsigned int id)
{
    //it can't be null at least for now
    weatherTypeData* data = weatherData.GetElementPointer(id);
    return psserver->GetRandom(data->max_fade_out-data->min_fade_out) + data->min_fade_out;
}

void psSectorInfo::AddWeatherTypeData(weatherTypeData newWeatherData, unsigned int id)
{
    weatherData.PutUnique(id, newWeatherData);
}

double psSectorInfo::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString prop(ptr);
    /*if(prop == "interior")
    {
        return (double)!rain_enabled;
    }
    else */
    if(prop == "uid")
    {
        return (double)uid;
    }

    return 0;
}

double psSectorInfo::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    csString function(functionName);
    if(function == "IsSector")
    {
        const char* sector = env->GetString(params[0]);
        return (double)(name == sector);
    }
    return 0;
}

