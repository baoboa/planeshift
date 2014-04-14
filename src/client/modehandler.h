/*
 * modehandler.h    Keith Fulton <keith@paqrat.com>
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
#ifndef MODEHANDLER_H
#define MODEHANDLER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>
#include <csutil/ref.h>
#include <csutil/cscolor.h>
#include <csutil/weakref.h>
#include <iengine/portal.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================

struct iEngine;
struct iBase;
struct iSoundManager;
struct iLoaderContext;
struct LightingList;
struct LightingSetting;
struct WeatherInfo;

class pawsChatWindow;
class GEMClientActor;
class psCelClient;
class MsgHandler;
class WeatherObject;

/* Defines for the various times of day. Some things may need to trigger on a 
  'general' time of day. */
enum TimeOfDay
{
    TIME_ANY,
    TIME_NIGHT,
    TIME_MORNING,
    TIME_AFTERNOON,
    TIME_EVENING    
};


/// Begin of classes

/* This class is used as callback when a portal is shown.
   This is needed to have correct weather */
class psPortalCallback : public scfImplementation1<psPortalCallback,iPortalCallback>
{
public:
    psPortalCallback();
    virtual ~psPortalCallback();

    /** This function is called when a portal is shown for the first time and is missing
      * and therefore it's here we need to call CreatePortalWeather since we get another potentional
      * sector to add weather to
      * @remarks Not to be used directly, called by CS
      */
    virtual bool Traverse (iPortal* portal,iBase* context);

    const char* GetSector() { return sector; }

private:
    csString sector;
    csRef<iLoaderContext> loaderContext;
};

/// Holds the weather meshes and connected portal
struct WeatherPortal
{
    WeatherInfo*   wi;
    WeatherObject* downfall;
    
    iPortal* portal;
    csBox3 bbox;
    csVector3 pos;
};

/**
 *  This class handles mode messages from the server, specifying
 *  "normal", or "combat" or "death", or whatever.  Right now, all
 *  it does is change the background music, but in the future
 *  this class could also trigger visual effects, in-game cinematics
 *  or whatever else we think of.
 */
class ModeHandler : public psClientNetSubscriber
{
public:
    ModeHandler(psCelClient *cc,MsgHandler* mh,iObjectRegistry* object_reg);
    virtual ~ModeHandler();
    virtual void HandleMessage(MsgEntry* me);

    bool Initialize();

    void ClearLightFadeSettings();
    void FinishLightFade();

    void PreProcess();

    void SetEntity(GEMClientActor *ent);

    // Get the general time of day it is ( from enum )
    TimeOfDay GetGeneralTime() { return timeOfDay; }

    // Get the hour of the day.
    size_t GetTime() { return clockHour; }

    void DoneLoading(const char* sectorname); // Called when every connected sector is loaded

    /// Creates and places a weather object at each portal to a {WEATHER}ing sector
    void CreatePortalWeather(iSector* sector, csTicks delta); 
    bool CreatePortalWeather(iPortal* portal, csTicks delta);
    /// Remove weather from all portals
    void RemovePortalWeather();
    /// Remove fog and downfall from sector
    void RemoveWeather();

    void AddDownfallObject(WeatherObject* weatherobject);
    void RemoveDownfallObject(WeatherObject* weatherobject);

    void SetModeSounds(uint8_t mode);
    void UpdateLights();

protected:
    GEMClientObject* actorEntity;
        
    ///Stores the 'general' time of day it is.  One of the TimeOfDay enums.
    TimeOfDay  timeOfDay;

    /// Stores the time of day in hours.
    uint clockHour;
     
    iObjectRegistry*        object_reg;
    csRef<psCelClient>      celclient;
    csRef<MsgHandler>       msghandler;
    csRef<iEngine>          engine;
    csRef<iVFS>             vfs;

    // Each element represents an hour of the day.
    csPDelArray<LightingList> lights;
    // Track the stage of the current step.
    uint                    stepStage;
    csTicks                 interpolation_time;
    uint                    interpolation_step;
    csTicks                 last_interpolation_reset;
    bool                    interpolation_complete;
    csRandomGen            *randomgen;
    bool                    sound_queued;
    csString                sound_name;
    csTicks                 sound_when;

    // Lighting variable.
    csRef<iSharedVariable> lightningreset;
    CS::ShaderVarStringID ambientId;
    CS::ShaderVarStringID combinedAmbientId;

    // Weather intepolation stuff
    csTicks                 last_weather_update;
    csTicks                 weather_update_time;
    
    csHash<WeatherInfo*, csString>   weatherlist;

    // Weather gfx object for current sector
    WeatherObject*              downfall; // Weather object following the player around
    WeatherObject*              fog;      // Weather object that is always in effect
    csPDelArray<WeatherPortal>  portals;  // Weather for portals out of the sector.
    bool processWeather; // determine whether to show weather effects or not

    void HandleModeMessage(MsgEntry* me);
    void HandleWeatherMessage(MsgEntry* me);
    void HandleNewSectorMessage(MsgEntry* me);
    void HandleCombatEvent(MsgEntry* me);
    void HandleSpecialCombatEvent(MsgEntry* me);
	void HandleCachedFile(MsgEntry* me);

    bool ProcessLighting(LightingSetting *color, float pct);
    LightingSetting *FindLight(LightingSetting *light,int which);

    bool CheckCurrentSector(GEMClientObject *entity,
                const char *sectorname,
                csVector3& pos,
                iSector*&  sector);

    void SetSectorMusic(const char *sectorname);
    void PublishTime( int newTime );

    void UpdateLights(csTicks when, bool force = false);

    /* WEATHER FUNCTIONS */
    // Controls the weather.
    void ProcessLighting(const psWeatherMessage::NetWeatherInfo& info);
    void ProcessDownfall(const psWeatherMessage::NetWeatherInfo& info);
    public:
    void ProcessFog(const psWeatherMessage::NetWeatherInfo& info);
protected:
    bool CreateWeather(WeatherInfo* ri, csTicks delta);

    void StartFollowWeather();
    void StopFollowWeather();

    WeatherObject* CreateDownfallWeatherObject(WeatherInfo* ri);
    WeatherObject* CreateStaticWeatherObject(WeatherInfo* ri);

    WeatherPortal* GetPortal(iPortal* portal);
    WeatherInfo*   GetWeatherInfo(const char* sector);
    WeatherInfo*   CreateWeatherInfo(const char* sector);
    float          GetDensity(WeatherInfo* wi);

    void           UpdateWeather(csTicks when);
    void           UpdateWeatherSounds();

    /* END OF WEATHER FUNCTIONS */

    bool LoadLightingLevels();  // Called from Initialize


private:
    pawsChatWindow* chatWindow; ///< Used to get the current chat filtering.
 
    csString MungeName(GEMClientActor* obj, bool startOfPhrase = false);
    
    void SetCombatAnim( GEMClientActor* atObject, csStringID anim );
    
    void Other(  int type, float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void OtherOutOfRange( GEMClientActor* atObject, GEMClientActor* tarObject );
    void OtherMiss( GEMClientActor* atObject, GEMClientActor* tarObject );
    void OtherDodge( GEMClientActor* atObject, GEMClientActor* tarObject );
    void OtherDeath( GEMClientActor* atObject, GEMClientActor* tarObject );
    void OtherDamage( float damage, GEMClientActor* atObject, GEMClientActor* tarObject );
    void OtherBlock( GEMClientActor* atObject, GEMClientActor* tarObject );
    void OtherNearlyDead(GEMClientActor *tarObject);
    
    void Defend( int type, float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void DefendOutOfRange( GEMClientActor* atObject, GEMClientActor* tarObject );
    void DefendMiss( GEMClientActor* atObject, GEMClientActor* tarObject );
    void DefendDodge( GEMClientActor* atObject, GEMClientActor* tarObject );
    void DefendDeath( GEMClientActor* atObject );
    void DefendDamage( float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void DefendBlock( GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void DefendNearlyDead();
    
    void Attack( int type, float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void AttackOutOfRange( GEMClientActor* atObject, GEMClientActor* tarObject );
    void AttackMiss(GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void AttackDodge( GEMClientActor* atObject, GEMClientActor* tarObject );
    void AttackDeath( GEMClientActor* atObject, GEMClientActor* tarObject );
    void AttackDamage(float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
    void AttackBlock( GEMClientActor* atObject, GEMClientActor* tarObject, csString& location );
            
};

/**
 *
 */


struct LightingList
{
    csPDelArray<LightingSetting> colors;
};


struct LightingSetting
{
    bool     error;       /// Has this lighting value had an error yet
    size_t   value;       /// What time of day is this lighting for?
    double   density;     /// What is the density level of the fog, if any.
    csString type;        /// Fog, Light or Ambient
    csString object;      /// Name of light, or name of sector
    csColor  color;       /// color when it is clear skies
    csColor  raincolor;   /// color when it is 100% downpour
    csColor  start_color; /// last color at start of interpolation
    csColor  base_color; /// base ambient light
    csColor  diff;        /// diff between last color and target color in interpolation
    csString sector;      /// sector of light is stored to make searching for rain faster
    // The two cache values below use csWeakRef to make sure that the cache is cleared automatically
    // as soon as the light or sector is removed (this can happen in case the region is unloaded).
    csWeakRef<iLight> light_cache;      // Optimization: pointer to light so we don't have to search it again.
    csWeakRef<iSector> sector_cache;    // Optimization: pointer to sector so we don't have to search it again.
    csRef<csShaderVariable> ambient_cache; // Optimization: pointer to ambient so we don't have to search it again.
    csRef<csShaderVariable> combined_ambient_cache; // Optimization: pointer to combined ambient so we don't have to search it again.
};

#endif
