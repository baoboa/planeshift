/*
 * weathermanager.h
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
#ifndef __WEATHERMANAGER_H__
#define __WEATHERMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/sector.h>
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================



class psWeatherGameEvent;
class psSectorInfo;
class CacheManager;

/**
 * This class handles generation of any and all weather events in the game,
 * including rain, snow, locust storms, day/night cycles, etc.
 */
class WeatherManager
{
public:
    WeatherManager(CacheManager* cachemanager);
    ~WeatherManager();

    void Initialize();

    void QueueNextEvent(int delayticks,
                        int eventtype,
                        int eventvalue,
                        int duration,
                        int fade,
                        const char* sector,
                        psSectorInfo* si,
                        uint clientnum = 0,
                        int r = 0,
                        int g = 0,
                        int b = 0);


    /** @brief Starts automatic weather in a sector
     *  Allocates new events for snow, rain and fog
     * @param si The sector in which we start the automatic weather
     * @param type The weather type we will start. The default 0 does it for all.
     */
    void StartWeather(psSectorInfo* si, unsigned int type = 0);

    /** @brief Stops automatic weather in a sector
     *  Puts all events from the automatic weather (rain and snow for now)
     * in the sector into the ignored array,
     * removing them from the events array
     * @param si The sector in which we stop the automatic weather
     * @param type The weather type we will stop. The default 0 does it for all.
     */
    void StopWeather(psSectorInfo* si, unsigned int type = 0);
    void HandleWeatherEvent(psWeatherGameEvent* event);
    void SendClientGameTime(int cnum);
    void BroadcastGameTime();
    void BroadcastGameTimeSuperclients();
    void UpdateClient(uint32_t cnum);
    int GetGameTODMinute()
    {
        return gameTimeMinute;
    }
    int GetGameTODHour()
    {
        return gameTimeHour;
    }
    int GetGameTODDay()
    {
        return gameTimeDay;
    }
    int GetGameTODMonth()
    {
        return gameTimeMonth;
    }
    int GetGameTODYear()
    {
        return gameTimeYear;
    }

    /** Look into the database for the saved time.
      * This starts the event pushing for the time.
      */
    void StartGameTime();
    void SaveGameTime();
    void SetGameTime(int hour,int minute);


protected:
    csRandomGen* randomgen;
    int gameTimeMinute;
    int gameTimeHour;
    int gameTimeDay;
    int gameTimeMonth;
    int gameTimeYear;

    csArray<psWeatherGameEvent*> ignored; ///< Used for overriding commands like /rain

    CS::Threading::Mutex eventsMutex;
    csArray<psWeatherGameEvent*> events; // Ugly, but we need a copy of our events

    CacheManager* cacheManager;
};

/**
 * When a weather event is created, it goes here.
 */
class psWeatherGameEvent : public psGameEvent
{
public:
    psWeatherGameEvent(WeatherManager* mgr,
                       int delayticks,
                       int eventtype,
                       int eventvalue,
                       int duration,
                       int fade,
                       const char* sector,
                       psSectorInfo* si,
                       uint client,
                       int r = 0,
                       int g = 0,
                       int b = 0);

    virtual void Trigger();  ///< Abstract event processing function

    const char* GetType();

    int cr,cg,cb;
    int type, value, duration,fade;
    csString sector;
    psSectorInfo* si;
    uint clientnum;


protected:
    WeatherManager* weathermanager;


};

#endif

