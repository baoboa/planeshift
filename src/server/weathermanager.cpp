/*
 * weathermanager.cpp
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/vector3.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "bulkobjects/pssectorinfo.h"

#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "weathermanager.h"
#include "psserver.h"
#include "cachemanager.h"
#include "netmanager.h"
#include "client.h"
#include "gem.h"
#include "globals.h"
#include "npcmanager.h"

//#define WEATHER_DEBUG

#define MONTH_COUNT 10
const int monthLengths[MONTH_COUNT] = {32,32,32,32,32,32,32,32,32,32};

WeatherManager::WeatherManager(CacheManager* cachemanager)
{
    cacheManager = cachemanager;

    gameTimeMinute = 0;
    gameTimeHour = 0;
    gameTimeDay = 0;
    gameTimeMonth = 0;
    gameTimeYear = 0;

    randomgen = psserver->rng;
}

WeatherManager::~WeatherManager()
{
}

void WeatherManager::Initialize()
{
    // Start rain/snow for all sectors
    csHash<psSectorInfo*>::GlobalIterator iter = cacheManager->GetSectorIterator();
    while(iter.HasNext())
    {
        psSectorInfo* si = iter.Next();

        StartWeather(si);
    }
}

void WeatherManager::StartGameTime()
{
    csString lastTime,lastDate;

    // Load time
    psserver->GetServerOption("game_time", lastTime);
    sscanf(lastTime.GetDataSafe(),"%d:%d",&gameTimeHour,&gameTimeMinute);

    // Load date
    psserver->GetServerOption("game_date", lastDate);
    sscanf(lastDate.GetDataSafe(),"%d-%d-%d",&gameTimeYear,&gameTimeMonth,&gameTimeDay);

    // Start the time of day clock
    QueueNextEvent(0, psWeatherMessage::DAYNIGHT, 0, 0, 0, "", NULL);
}

void WeatherManager::SaveGameTime()
{
    csString currTimeStr,currDateStr;

    // Save time
    currTimeStr.Format("%d:%02d", gameTimeHour,gameTimeMinute);
    psserver->SetServerOption("game_time", currTimeStr);

    // Save date
    currDateStr.Format("%d-%d-%d",gameTimeYear,gameTimeMonth,gameTimeDay);
    psserver->SetServerOption("game_date", currDateStr);
}

void WeatherManager::SetGameTime(int hour,int minute)
{
    gameTimeHour = hour;
    gameTimeMinute = minute;
    BroadcastGameTime();
}

void WeatherManager::StartWeather(psSectorInfo* si, unsigned int type)
{
    // Is rain/snow enabled for this sector
    if((type == 0 || type == psWeatherMessage::RAIN) && si->GetWeatherEnabled((unsigned int)psWeatherMessage::RAIN))
    {
        // Queue event to start rain
        QueueNextEvent(si->GetRandomWeatherGap((unsigned int)psWeatherMessage::RAIN),
                       psWeatherMessage::RAIN,
                       si->GetRandomWeatherDensity((unsigned int)psWeatherMessage::RAIN),
                       0, // Duration is calculated when stop event is created
                       0, // Fade is calculated when sending weather event
                       si->name,
                       si);
    }
    if((type == 0 || type == psWeatherMessage::SNOW) && si->GetWeatherEnabled((unsigned int)psWeatherMessage::SNOW))
    {
        // Queue event to start snow
        QueueNextEvent(si->GetRandomWeatherGap((unsigned int)psWeatherMessage::SNOW),
                       psWeatherMessage::SNOW,
                       si->GetRandomWeatherDensity((unsigned int)psWeatherMessage::SNOW),
                       0, // Duration is calculated when stop event is created
                       0, // Fade is calculated when sending weather event
                       si->name,
                       si);
    }
    if((type == 0 || type == psWeatherMessage::FOG) && si->GetWeatherEnabled((unsigned int)psWeatherMessage::FOG))
    {
        // Queue event to start snow
        QueueNextEvent(si->GetRandomWeatherGap((unsigned int)psWeatherMessage::FOG),
                       psWeatherMessage::FOG,
                       si->GetRandomWeatherDensity((unsigned int)psWeatherMessage::FOG),
                       0, // Duration is calculated when stop event is created
                       si->GetRandomWeatherFadeIn((unsigned int)psWeatherMessage::FOG),
                       si->name,
                       si, 0, 200, 200, 200);
    }
}

void WeatherManager::StopWeather(psSectorInfo* si, unsigned int type)
{
    CS::Threading::MutexScopedLock lock(eventsMutex);
    psWeatherGameEvent* evt = NULL;
    for(size_t i = 0; i < events.GetSize(); i++)
    {
        evt = events[i];
        if(evt->si == si && ((unsigned int)evt->type == type || (type == 0 && (evt->type == psWeatherMessage::RAIN || evt->type == psWeatherMessage::SNOW))))
        {
            ignored.Push(evt); // Ignore when the eventmanager handles the event
            events.DeleteIndex(i);
            i--;
        }
    }
}

void WeatherManager::UpdateClient(uint32_t cnum)
{
    psWeatherMessage::NetWeatherInfo info;

    // Update client with weather for all sectors
    csHash<psSectorInfo*>::GlobalIterator iter = cacheManager->GetSectorIterator();
    while(iter.HasNext())
    {
        psSectorInfo* sector = iter.Next();
        info.has_downfall  = false;
        info.has_lightning = false;
        info.has_fog       = false;
        info.sector = sector->name;

        if(sector->is_raining) // If it is raining
        {
            info.has_downfall = true;
            info.downfall_is_snow = false;

            info.downfall_drops = sector->current_rain_drops;
            info.downfall_fade = 0; // Don't fade in
        }
        else if(sector->is_snowing)  // If it is snowing
        {
            info.has_downfall = true;
            info.downfall_is_snow = true;

            info.downfall_drops = sector->current_rain_drops;
            info.downfall_fade = 0; // Don't fade in
        }
        else
        {
            // Some default vaules in case neither snowing or raining
            info.downfall_is_snow = false;
            info.downfall_drops = 0;
            info.downfall_fade = 0;
        }
        

        if(sector->fog_density > 0)
        {
            info.has_fog = true;
            info.fog_density = sector->fog_density;
            info.fog_fade = 0; // Don't fade in
            info.r = sector->r;
            info.g = sector->g;
            info.b = sector->b;

        }
        if(info.has_downfall || info.has_fog)
        {
            psWeatherMessage weather(cnum,info);
            weather.SendMessage();
        }
    }

    SendClientGameTime(cnum);

    // Send
#ifdef WEATHER_DEBUG
    printf("Sending weather and time updates to client %u\n",cnum);
#endif

}

void WeatherManager::QueueNextEvent(int delayticks,
                                    int eventtype,
                                    int eventvalue,
                                    int duration,
                                    int fade,
                                    const char* sector,
                                    psSectorInfo* si,
                                    uint clientnum,
                                    int r,int g,int b)
{
    psWeatherGameEvent* event;

    // There are no notify for 7 pars so we create a string first.
    csString note;
    note.Format("QueueNextEvent Delay: %d Type: %d Value: %d "
                "Duration: %d Fade: %d Sector: %s ...",
                delayticks,eventtype,eventvalue,duration,
                fade,sector);
    Notify2(LOG_WEATHER, "%s", note.GetDataSafe());

    event = new psWeatherGameEvent(this,
                                   delayticks,
                                   eventtype,
                                   eventvalue,
                                   duration,
                                   fade,
                                   sector,
                                   si,
                                   clientnum,
                                   r,g,b);
    {
        CS::Threading::MutexScopedLock lock(eventsMutex);
        events.Push(event);
    }
    psserver->GetEventManager()->Push(event);
}

void WeatherManager::SendClientGameTime(int cnum)
{
    psWeatherMessage time(cnum,gameTimeMinute,gameTimeHour,gameTimeDay,gameTimeMonth,gameTimeYear);
    time.SendMessage();
}

void WeatherManager::BroadcastGameTime()
{
    psWeatherMessage time(0,gameTimeMinute,gameTimeHour,gameTimeDay,gameTimeMonth,gameTimeYear);
    psserver->GetEventManager()->Broadcast(time.msg);
}

void WeatherManager::BroadcastGameTimeSuperclients()
{
    psWeatherMessage time(0,gameTimeMinute,gameTimeHour,gameTimeDay,gameTimeMonth,gameTimeYear);
    time.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
}


void WeatherManager::HandleWeatherEvent(psWeatherGameEvent* event)
{
    {
        CS::Threading::MutexScopedLock lock(eventsMutex);
        events.Delete(event); // Delete this from our "db"
    }

    // See if we want to ignore this event
    for(size_t i = 0; i < ignored.GetSize(); i++)
    {
        if(event == ignored[i])
        {
            ignored.DeleteIndex(i);
            return;
        }
    }

    switch(event->type)
    {
        case psWeatherMessage::SNOW:
        case psWeatherMessage::RAIN:
        {
            event->si->current_rain_drops = event->value;

            Notify4(LOG_WEATHER,"New %s in sector '%s': %d",event->type == psWeatherMessage::SNOW ? "snow" : "rain",  event->si->name.GetData(),event->value);

            psWeatherMessage::NetWeatherInfo info;

            info.has_downfall     = true;
            info.downfall_is_snow = (event->type == psWeatherMessage::SNOW);
            info.has_fog          = true;
            info.has_lightning    = false;

            info.downfall_drops = event->value;

            // Save current fog and calculate new.
            if(event->value)
            {
                // Only save the fog 'history' once. After that we just override.
                if(!event->si->densitySaved)
                {
                    event->si->fog_density_old = event->si->fog_density;
                    event->si->densitySaved = true;
                }

                // Set fog colour if there's not already fog.
                if(!event->si->fog_density)
                {
                    event->si->r = 255;
                    event->si->g = 255;
                    event->si->b = 255;
                }

                if(info.downfall_drops < 8000)
                {
                    // Calculate fog to be linear in range 0 to 200
                    event->si->fog_density =
                        (int)(200.0f*(info.downfall_drops-1000.0f)/8000.0f);
                }
                else
                {
                    event->si->fog_density = 200;
                }
            }
            else if(event->si->fog_density)
            {
                // Restore fog, if the fog wasn't turned off.
                event->si->fog_density = event->si->fog_density_old;
                event->si->densitySaved = false;
            }

            info.fog_density = event->si->fog_density;

            info.r = event->si->r;
            info.g = event->si->g;
            info.b = event->si->b;

            info.sector = event->sector;

            if(event->fade)
            {
                info.downfall_fade = event->fade;
                info.fog_fade = event->fade;
            }
            else
            {
                if(event->value)
                {
                    info.downfall_fade = event->si->GetRandomWeatherFadeIn((unsigned int)event->type);
                    info.fog_fade = info.downfall_fade;
                }
                else
                {
                    info.downfall_fade = event->si->GetRandomWeatherFadeOut((unsigned int)event->type);
                    info.fog_fade = info.downfall_fade;
                }
            }

            Notify4(LOG_WEATHER,"Drops: %d Density: %d Sector: %s\n",
                    info.downfall_drops,info.fog_density,info.sector.GetDataSafe());

            psWeatherMessage rain(0,info);
            if(rain.valid)
                psserver->GetEventManager()->Broadcast(rain.msg,NetBase::BC_EVERYONE);
            else
            {
                Bug1("Could not create valid psWeatherMessage (rain) for broadcast.\n");
            }

            // Make sure we don't have any other events in this sector that will disturb
            // Simple case is when event types are equal. In addition we have to test
            // for the mutal exclusive case where we are changing from snow to rain or
            // rain to snow.

            {
                CS::Threading::MutexScopedLock lock(eventsMutex);
                for(size_t i = 0; i < events.GetSize(); i++)
                {
                    psWeatherGameEvent* evt = events[i];
                    if(evt->sector == event->sector && (evt->type == event->type
                                                        || ((evt->type == psWeatherMessage::RAIN || evt->type
                                                                == psWeatherMessage::SNOW) && (event->type
                                                                        == psWeatherMessage::RAIN || event->type
                                                                        == psWeatherMessage::SNOW))))
                    {
                        ignored.Push(evt); // Ignore when the eventmanager handles the event
                        events.DeleteIndex(i);
                        i--;

                        Notify4(LOG_WEATHER,"Removed disturbing event for sector '%s' (%d,%d)",
                                evt->sector.GetData(),evt->value,evt->duration);

                    }
                }
            }

            if(event->value)   // Queue event to turn off rain/snow.
            {
                if(event->type != psWeatherMessage::SNOW)
                    event->si->is_raining = true;
                else
                    event->si->is_snowing = true;

                if(event->si->GetWeatherEnabled((unsigned int) psWeatherMessage::LIGHTNING) &&
                        event->value > 2000 && event->si->is_raining)
                {
                    // Queue lightning during rain storm here first
                    QueueNextEvent(event->si->GetRandomWeatherGap((unsigned int) psWeatherMessage::LIGHTNING),
                                   psWeatherMessage::LIGHTNING,
                                   0,
                                   0,
                                   0,
                                   event->si->name,
                                   event->si,
                                   event->clientnum);
                }

                // Queue event to stop rain/snow
                int duration;
                if(event->duration != -1)
                {
                    if(event->duration)
                    {
                        duration = event->duration;
                    }
                    else
                    {
                        duration = event->si->GetRandomWeatherDuration((unsigned int) event->type);
                    }

                    QueueNextEvent(duration,
                                   event->type,
                                   0,
                                   0,
                                   event->fade,
                                   event->si->name,
                                   event->si);
                }

            }
            else // Stop rain/snow.
            {
                if(event->type== psWeatherMessage::SNOW)
                    event->si->is_snowing = false;
                else
                    event->si->is_raining = false;

                // Queue event to turn on again later if enabled
                StartWeather(event->si);
            }

            break;
        }
        case psWeatherMessage::FOG:
        {
            // Update sector weather info
            event->si->fog_density = event->value;
            event->si->r = event->cr;
            event->si->g = event->cg;
            event->si->b = event->cb;

            // Update the clients
            psWeatherMessage::NetWeatherInfo info;
            info.has_downfall     = false;
            info.downfall_is_snow = false;
            info.has_fog          = true;
            info.has_lightning    = false;
            info.sector           = event->si->name;
            info.fog_density      = event->si->fog_density;
            info.r                = event->cr;
            info.g                = event->cg;
            info.b                = event->cb;
            info.downfall_drops   = 0;
            info.downfall_fade    = 0;

            Notify3(LOG_WEATHER,"New Fog in sector '%s': %d",  event->si->name.GetData(), event->value);

            // Save the fade in so we can reverse it when fading out.
            if(event->fade)
            {
                event->si->fogFade = event->fade;
            }
            else
            {
                // We're removing fog, so removed the 'saved' flag.
                event->si->densitySaved = false;
            }
            info.fog_fade = event->si->fogFade;

            psWeatherMessage fog(0,info);
            if(fog.valid)
                psserver->GetEventManager()->Broadcast(fog.msg,NetBase::BC_EVERYONE);
            else
            {
                Bug1("Could not create valid psWeatherMessage (fog) for broadcast.\n");
            }

            if(event->value)   // Queue event to turn off fog.
            {

                // Queue event to stop rain/snow
                int duration;
                if(event->duration != -1)
                {
                    if(event->duration)
                    {
                        duration = event->duration;
                    }
                    else
                    {
                        duration = event->si->GetRandomWeatherDuration((unsigned int) event->type);
                    }

                    QueueNextEvent(duration,
                                   event->type,
                                   0,
                                   0,
                                   event->si->GetRandomWeatherFadeOut((unsigned int)psWeatherMessage::FOG),
                                   event->si->name,
                                   event->si);
                }

            }
            else // Stop fog.
            {
                // Queue event to turn on again later if enabled
                StartWeather(event->si, psWeatherMessage::FOG);
            }

            break;
        }
        case psWeatherMessage::LIGHTNING:
        {
            if(event->si->is_raining)
            {
                Notify2(LOG_WEATHER,"Lightning in sector '%s'",event->sector.GetData());

                psWeatherMessage::NetWeatherInfo info;
                info.has_downfall  = false;
                info.downfall_is_snow = false;
                info.has_fog       = false;
                info.has_lightning = true;
                info.sector = event->sector;
                info.fog_fade = info.downfall_fade = 0;
                info.r = info.g = info.b = 0;
                info.downfall_drops = info.fog_density = 0;

                psWeatherMessage lightning(0,info, event->clientnum);

                if(lightning.valid)
                {
                    psserver->GetEventManager()->Broadcast(lightning.msg);
                }
                else
                {
                    Bug1("Could not create valid psWeatherMessage (lightning) for broadcast.\n");
                }

                if(event->si->is_raining &&
                        event->si->GetWeatherEnabled((unsigned int) psWeatherMessage::LIGHTNING))
                {
                    QueueNextEvent(event->si->GetRandomWeatherGap((unsigned int) psWeatherMessage::LIGHTNING),
                                   psWeatherMessage::LIGHTNING,
                                   0,
                                   0,
                                   0,
                                   event->si->name,
                                   event->si,
                                   event->clientnum);
                }
            }
            break;
        }
        case psWeatherMessage::DAYNIGHT:
        {
            QueueNextEvent(GAME_MINUTE_IN_TICKS,
                           psWeatherMessage::DAYNIGHT,
                           0,
                           0,
                           0,
                           NULL,
                           NULL);

            gameTimeMinute++;
            if(gameTimeMinute >= 60)
            {
                gameTimeMinute = 0;
                gameTimeHour++;
                if(gameTimeHour >= 24)
                {
                    gameTimeHour = 0;
                    gameTimeDay++;
                    if(gameTimeDay >= monthLengths[gameTimeMonth-1]+1)
                    {
                        gameTimeDay = 1;
                        gameTimeMonth++;
                        if(gameTimeMonth >= MONTH_COUNT+1)
                        {
                            gameTimeMonth = 1;
                            gameTimeYear++;
                        }
                    }

                }
                // Only save and broadcast every game hour.
                SaveGameTime();
                BroadcastGameTime();
            }
            else
            {
                // Super clients should get the time every minute
                BroadcastGameTimeSuperclients();
            }

            break;
        }
        default:
        {
            break;
        }
    }
}





/*---------------------------------------------------------------------*/

psWeatherGameEvent::psWeatherGameEvent(WeatherManager* mgr,
                                       int delayticks,
                                       int eventtype,
                                       int eventvalue,
                                       int length,
                                       int fadevalue,
                                       const char* where,
                                       psSectorInfo* sinfo,
                                       uint client,
                                       int r,int g,int b)
    : psGameEvent(0,delayticks,"psWeatherGameEvent")
{
    weathermanager = mgr;
    type           = eventtype;
    value          = eventvalue;
    sector         = where;
    si             = sinfo;
    duration       = length;
    fade           = fadevalue;
    clientnum      = client;

    cr = r;
    cg = g;
    cb = b;
}


void psWeatherGameEvent::Trigger()
{
    weathermanager->HandleWeatherEvent(this);
}

const char* psWeatherGameEvent::GetType()
{
    switch(type)
    {
        case psWeatherMessage::SNOW:
            return "Snow";

        case psWeatherMessage::LIGHTNING:
            return "Lightning";

        case psWeatherMessage::FOG:
            return "Fog";

        case psWeatherMessage::RAIN:
            return "Rain";

        case psWeatherMessage::DAYNIGHT:
            return "Day/Night";

        default:
            return "Unknown";
    }
}

