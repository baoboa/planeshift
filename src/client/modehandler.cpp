/*
 * modehandler.cpp    Keith Fulton <keith@paqrat.com>
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
// Crystal Space Includes
//=============================================================================
#include <csutil/objreg.h>
#include <csutil/sysfunc.h>
#include <csutil/cscolor.h>
#include <csutil/flags.h>
#include <csutil/randomgen.h>
#include <csutil/scf.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/sharevar.h>
#include <iengine/portal.h>
#include <iengine/portalcontainer.h>
#include <imesh/object.h>
#include <imesh/partsys.h>
#include <imesh/spritecal3d.h>
#include <iutil/vfs.h>
#include <imap/ldrctxt.h>
#include <iutil/object.h>
#include <iutil/cfgmgr.h>
#include <ivaria/engseq.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "effects/pseffectmanager.h"

#include "util/psscf.h"
#include "util/psconst.h"
#include "util/log.h"
#include "util/slots.h"

#include "paws/pawsmanager.h"

#include "gui/chatwindow.h"
#include "gui/pawsinfowindow.h"
#include "gui/pawsspellcancelwindow.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "modehandler.h"
#include "entitylabels.h"
#include "pscelclient.h"
#include "pscharcontrol.h"
#include "psclientdr.h"
#include "weather.h"
#include "globals.h"

#define LIGHTING_STEPS 20


/// Callback class for the portal traversing
psPortalCallback::psPortalCallback()
    : scfImplementationType(this)
{
}

psPortalCallback::~psPortalCallback()
{
}

bool psPortalCallback::Traverse(iPortal* portal, iBase* /*base*/)
{
    // Refresh the weather
    psengine->GetModeHandler()->CreatePortalWeather(portal, 0);

    // Remove us from the list, we have created it now
    portal->RemovePortalCallback(this);

    return true;
}

ModeHandler::ModeHandler(psCelClient* cc,
                         MsgHandler* mh,
                         iObjectRegistry* obj_reg)
{
    msghandler   = mh;
    celclient    = cc;
    actorEntity  = 0;
    object_reg   = obj_reg;
    engine =  csQueryRegistry<iEngine> (object_reg);
    vfs    =  csQueryRegistry<iVFS> (object_reg);

    downfall     = NULL;
    fog          = NULL;

    clockHour = 0;
    interpolation_time = 600000; // One game hour.
    interpolation_step = 0;
    stepStage = 0;
    last_interpolation_reset = 0;
    randomgen     = new csRandomGen();
    interpolation_complete = true;  // don't attempt to update lights until you get a net msg to do so

    timeOfDay = TIME_MORNING;
    chatWindow = NULL;
    sound_queued = false;

    last_weather_update = csGetTicks();
    weather_update_time = 200;

    lightningreset = engine->GetVariableList()->New();
    lightningreset->SetName("lightning reset");
    lightningreset->SetColor(csColor(0.0f));
    engine->GetVariableList()->Add(lightningreset);

    csRef<iShaderVarStringSet> svStrings = csQueryRegistryTagInterface<iShaderVarStringSet>(
            obj_reg, "crystalspace.shader.variablenameset");
    ambientId = svStrings->Request("dynamic ambient");
    combinedAmbientId = svStrings->Request("light ambient");

    processWeather = psengine->GetConfig()->GetBool("PlaneShift.Weather.Enabled", true);
}

ModeHandler::~ModeHandler()
{
    RemovePortalWeather();
    RemoveWeather();

    csHash<WeatherInfo*, csString>::GlobalIterator loop(weatherlist.GetIterator());
    WeatherInfo* wi;

    while(loop.HasNext())
    {
        wi = loop.Next();
        delete wi;
    }

    if(msghandler)
    {
        msghandler->Unsubscribe(this,MSGTYPE_MODE);
        msghandler->Unsubscribe(this,MSGTYPE_WEATHER);
        msghandler->Unsubscribe(this,MSGTYPE_NEWSECTOR);
        msghandler->Unsubscribe(this,MSGTYPE_COMBATEVENT);
        msghandler->Unsubscribe(this,MSGTYPE_SPECCOMBATEVENT);
        msghandler->Unsubscribe(this,MSGTYPE_CACHEFILE);
    }
    if(randomgen)
        delete randomgen;
}

bool ModeHandler::Initialize()
{
    Notify1(LOG_STARTUP,"ModeHandler is initializing..");

    // Net messages
    msghandler->Subscribe(this,MSGTYPE_MODE);
    msghandler->Subscribe(this,MSGTYPE_WEATHER);
    msghandler->Subscribe(this,MSGTYPE_NEWSECTOR);
    msghandler->Subscribe(this,MSGTYPE_SPECCOMBATEVENT);
    msghandler->Subscribe(this,MSGTYPE_COMBATEVENT);
    msghandler->Subscribe(this,MSGTYPE_CACHEFILE);

    // Light levels
    if(!LoadLightingLevels())
        return false;

    // Success!
    return true;
}

bool ModeHandler::LoadLightingLevels()
{
    csRef<iVFS> vfs;
    vfs =  csQueryRegistry<iVFS> (object_reg);
    if(!vfs)
        return false;

    csRef<iDataBuffer> buf(vfs->ReadFile("/planeshift/world/lighting.xml"));

    if(!buf || !buf->GetSize())
    {
        Error1("Cannot load /planeshift/world/lighting.xml");
        return false;
    }

    csRef<iDocument> doc = psengine->GetXMLParser()->CreateDocument();

    const char* error = doc->Parse(buf);
    if(error)
    {
        Error2("Error loading lighting levels, /planeshift/world/lighting.xml: %s\n", error);
        return false;
    }

    csRef<iDocumentNodeIterator> iter = doc->GetRoot()->GetNode("lighting")->GetNodes("color");

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        LightingSetting* newlight = new LightingSetting;
        double r,g,b,r2,g2,b2;

        newlight->error = false;
        newlight->value = node->GetAttributeValueAsInt("value");
        newlight->object = node->GetAttributeValue("object");
        newlight->sector= node->GetAttributeValue("sector");
        newlight->type = node->GetAttributeValue("type");
        newlight->density = node->GetAttributeValueAsFloat("density");  // only for fog
        r = node->GetAttributeValueAsFloat("r");
        g = node->GetAttributeValueAsFloat("g");
        b = node->GetAttributeValueAsFloat("b");
        r2 = node->GetAttributeValueAsFloat("weather_r");
        g2 = node->GetAttributeValueAsFloat("weather_g");
        b2 = node->GetAttributeValueAsFloat("weather_b");

        newlight->color.Set(r,g,b);
        if(r2 || g2 || b2)
            newlight->raincolor.Set(r2,g2,b2);
        else
            newlight->raincolor.Set(r,g,b);

        // Now add to lighting setup
        if(newlight->value >= lights.GetSize())
        {
            if(lights.GetSize() == 0)
            {
                LightingList* newset = new LightingList;
                lights.Push(newset);
            }
            else
            {
                // Add as many empty lists as we need.
                for(int i=(int)lights.GetSize()-1; i<(int)newlight->value; i++)
                {
                    LightingList* newset = new LightingList;
                    lights.Push(newset);
                }
            }
        }

        LightingList* set = lights[newlight->value];
        set->colors.Push(newlight);

        if(newlight->type == "light")
        {
            iLight* light = psengine->GetEngine()->FindLight(newlight->object);
            if(light && light->GetMovable()->GetSectors()->GetCount() > 0)
            {
                newlight->sector = light->GetMovable()->GetSectors()->Get(0)->QueryObject()->GetName();
            }
        }
    }
    return true;
}

void ModeHandler::SetEntity(GEMClientActor* ent)
{
    actorEntity = ent;
}

void ModeHandler::AddDownfallObject(WeatherObject* weatherobject)
{
    if(psengine->GetCelClient()->GetMainPlayer())
    {
        iSector* sector = psengine->GetCelClient()->GetMainPlayer()->GetSector();
        if(weatherobject->GetSector() == sector)
        {
            downfall = weatherobject; // Only need one object, safe to overwrite
        }
    }
}

void ModeHandler::RemoveDownfallObject(WeatherObject* weatherobject)
{
    if(downfall == weatherobject)
    {
        delete downfall;
        downfall = NULL;
    }
}

LightingSetting* ModeHandler::FindLight(LightingSetting* light,int which)
{
    LightingList* set = lights[which];

    for(size_t j=0; j<set->colors.GetSize(); j++)
    {
        LightingSetting* found = set->colors[j];

        if(found->object == light->object &&
                found->type   == light->type)
            return found;
    }
    return NULL;
}


void ModeHandler::HandleMessage(MsgEntry* me)
{
    switch(me->GetType())
    {
        case MSGTYPE_MODE:
            HandleModeMessage(me);
            return;

        case MSGTYPE_WEATHER:
            HandleWeatherMessage(me);
            return;

        case MSGTYPE_NEWSECTOR:
            HandleNewSectorMessage(me);
            return;

        case MSGTYPE_COMBATEVENT:
            HandleCombatEvent(me);
            return;
        case MSGTYPE_SPECCOMBATEVENT:
            HandleSpecialCombatEvent(me);
            return;

        case MSGTYPE_CACHEFILE:
            HandleCachedFile(me);
            return;
    }
}

void ModeHandler::HandleModeMessage(MsgEntry* me)
{
    psModeMessage msg(me);

    GEMClientActor* actor = dynamic_cast<GEMClientActor*>(celclient->FindObject(msg.actorID));
    if(!actor)
    {
        Error2("Received psModeMessage for unknown object %s", ShowID(msg.actorID));
        return;
    }

    actor->SetMode(msg.mode);

    // For the current player
    if(msg.actorID == celclient->GetMainPlayer()->GetEID())
    {
        SetModeSounds(msg.mode);
        if(msg.mode == psModeMessage::OVERWEIGHT)
        {
            psSystemMessage sysMsg(0, MSG_ERROR, "You struggle under the weight of your inventory and must drop something.");
            msghandler->Publish(sysMsg.msg);
        }
        else if(msg.mode == psModeMessage::SPELL_CASTING)
        {
            // start the spell animation
            actor->SetAnimation("cast");

            // show the spell cancel window
            pawsSpellCancelWindow* castWindow = (pawsSpellCancelWindow*) PawsManager::GetSingleton().FindWidget("SpellCancelWindow");
            if(castWindow)
            {
                castWindow->Start(msg.value); // msg.value is the duration
            }
        }
    }
    // For all other entities
    else
    {
        psengine->GetSoundManager()->SetEntityState(msg.mode, actor->GetMesh(), actor->race, false);
    }
}

void ModeHandler::SetModeSounds(uint8_t mode)
{
    if(!celclient->GetMainPlayer() || !psengine->GetSoundStatus())
        return;

    switch(mode)
    {
        case psModeMessage::PEACE:
            psengine->GetSoundManager()->SetCombatStance(iSoundManager::PEACE);
            break;
        case psModeMessage::COMBAT:
            psengine->GetSoundManager()->SetCombatStance(iSoundManager::COMBAT);
            break;
        case psModeMessage::DEAD:
            psengine->GetSoundManager()->SetCombatStance(iSoundManager::DEAD);
            break;
    }
}

WeatherInfo* ModeHandler::GetWeatherInfo(const char* sector)
{
    WeatherInfo* ri = weatherlist.Get(sector, NULL); // Get the weather info
    return ri;
}

WeatherInfo* ModeHandler::CreateWeatherInfo(const char* sector)
{
    WeatherInfo* wi = new WeatherInfo();

    wi->sector = sector;
    wi->downfall_condition = WEATHER_CLEAR;
    wi->downfall_params.value = 0;
    wi->downfall_params.fade_value = 0;
    wi->downfall_params.fade_time = 0;
    wi->fog_params = wi->downfall_params; // Fog and downfall use same type
    wi->fog_condition = WEATHER_CLEAR;
    wi->fog = NULL;
    wi->r = wi->g = wi->b = 0;

    weatherlist.Put(sector, wi); // weatherlist owns data

    return wi;
}


void ModeHandler::DoneLoading(const char* sectorname)
{
    iSector* newsector = psengine->GetEngine()->GetSectors()->FindByName(sectorname);
    // Give us the callback when the portals are working
    CreatePortalWeather(newsector, 0);
}

bool ModeHandler::CreatePortalWeather(iPortal* portal, csTicks delta)
{
    iSector* sect = portal->GetSector();

    if(!sect) // No sector yet assigned? Subscribe to visibility event
    {
        // Check if we already have this subscribed to our class
        bool subbed = false;
        for(int i = 0; i < portal->GetPortalCallbackCount(); i++)
        {
            // Test if this is a psPortalCallback object
            psPortalCallback* psCall = dynamic_cast<psPortalCallback*>(portal->GetPortalCallback(i));
            if(psCall) // Found!
            {
                subbed = true;
                break;
            }
        }

        if(!subbed) // Not a psPortalCallback class, subscribe
        {
            // Set the new callback
            csRef<psPortalCallback> psCall;
            psCall.AttachNew(new psPortalCallback());
            portal->SetPortalCallback(psCall);
        }

        return true;

    }

    // Begin the work with the weather
    WeatherInfo* ri = GetWeatherInfo(sect->QueryObject()->GetName());
    WeatherPortal* weatherP = GetPortal(portal); // Get the portal


    // No weather to Update if no Weather Info
    if(!ri)
    {
        return true;
    }


    if(!weatherP)
    {
        // Create the WeatherPortal entity
        weatherP           = new WeatherPortal;
        weatherP->portal   = portal;
        weatherP->downfall = NULL;
        weatherP->wi       = ri;

        csVector3 min,max;
        csBox3 posCalc;
        // Create the weathermesh
        for(int vertidx = 0; vertidx < portal->GetVertexIndicesCount(); vertidx++)
        {
            int vertexidx = portal->GetVertexIndices()[vertidx];
            csVector3 vertex = portal->GetWorldVertices()[vertexidx];
            posCalc.AddBoundingVertex(vertex);
        }

        // Store the pos
        weatherP->pos = posCalc.GetCenter();

        // Store the box for later use
        posCalc.SetCenter(csVector3(0,0,0));
        posCalc.AddBoundingVertex(csVector3(1,1,1));
        posCalc.AddBoundingVertex(csVector3(-1,-1,-1));
        weatherP->bbox = posCalc;

        // Push it into the array before loading mesh and stuff
        portals.Push(weatherP);
    }

    // Check for removal of downfall
    if(weatherP->downfall && ri->downfall_condition == WEATHER_CLEAR)
    {
        weatherP->downfall->Destroy();
        delete weatherP->downfall;
        weatherP->downfall = NULL;
        Notify2(LOG_WEATHER, "Downfall removed from portal '%s'",
                portal->GetName());
    }
    // Check if downfall has changed between rain/snow
    else if(weatherP->downfall && weatherP->downfall->GetType() != ri->downfall_condition)
    {
        weatherP->downfall->Destroy();
        delete weatherP->downfall;
        weatherP->downfall = CreateDownfallWeatherObject(ri);

        if(!weatherP->downfall->CreateMesh())
        {
            Error1("Failed to create downfall");
            delete weatherP->downfall;
            weatherP->downfall = NULL;
            return false;
        }

        // Calculate the drops
        csBox3 def = weatherP->downfall->CreateDefaultBBox();
        csBox3 box = weatherP->bbox;
        float mod = box.Volume() / def.Volume();
        int drops = (int)((float)ri->downfall_params.value * mod);

        if(drops < 1)
            drops = 1;

        // Setup the mesh
        weatherP->downfall->SetupMesh(weatherP->downfall->CreateDefaultBBox());

        //If the portal is a warp one, then distance is != 0
        csReversibleTransform transform = portal->GetWarp();
        csVector3 distance = transform.GetT2OTranslation();

        // move it to right position
        weatherP->downfall->MoveTo(ri,sect);
        weatherP->downfall->MoveTo(weatherP->pos + distance);

        Notify2(LOG_WEATHER, "Downfall changed type for portal '%s'",
                portal->GetName());
    }
    // Check if downfall need update
    else if(weatherP->downfall && ri->downfall_condition != WEATHER_CLEAR)
    {
        weatherP->downfall->Update(delta);
    }
    // Check if we need to create a downfall object
    else if(!weatherP->downfall && ri->downfall_condition != WEATHER_CLEAR)
    {
        weatherP->downfall = CreateDownfallWeatherObject(ri);

        if(!weatherP->downfall->CreateMesh())
        {
            Error1("Failed to create downfall");
            delete weatherP->downfall;
            weatherP->downfall = NULL;
            return false;
        }

        // Calculate the drops
        csBox3 def = weatherP->downfall->CreateDefaultBBox();
        csBox3 box = weatherP->bbox;
        float mod = box.Volume() / def.Volume();
        int drops = (int)((float)ri->downfall_params.value * mod);

        if(drops < 1)
            drops = 1;

        // Setup the mesh
        weatherP->downfall->SetupMesh(weatherP->downfall->CreateDefaultBBox());

        //If the portal is a warp one, then distance is != 0
        csReversibleTransform transform = portal->GetWarp();
        csVector3 distance = transform.GetT2OTranslation();

        // move it to right position
        weatherP->downfall->MoveTo(ri,sect);
        weatherP->downfall->MoveTo(weatherP->pos + distance);
        Notify2(LOG_WEATHER, "Downfall created in portal '%s'",
                portal->GetName());
    }

    // Check if fog need update
    if(ri->fog)
    {
        ri->fog->Update(delta);
    }
    // Check for removal of fog
    if(ri->fog && ri->fog_condition == WEATHER_CLEAR)
    {
        ri->fog->Destroy();
        delete ri->fog;
        ri->fog = NULL;
        Notify2(LOG_WEATHER, "Fog removed from sector '%s'",
                sect->QueryObject()->GetName());
    }
    // Check for add of fog
    else if(!ri->fog && ri->fog_condition != WEATHER_CLEAR)
    {
        ri->fog = CreateStaticWeatherObject(ri);
        if(ri->fog->CreateMesh())
        {
            Notify2(LOG_WEATHER, "Fog created in sector '%s'",
                    sect->QueryObject()->GetName());
        }
    }

    return true;
}

WeatherPortal* ModeHandler::GetPortal(iPortal* portal)
{
    for(size_t i = 0; i < portals.GetSize(); i++)
    {
        if(portals[i]->portal == portal)
            return portals[i];
    }

    return NULL;
}

void ModeHandler::CreatePortalWeather(iSector* sector, csTicks delta)
{
    if(!sector)
    {
        Error1("CreatePortalWeather received invalid sector!");
        return;
    }

    // Check portals
    csSet<csPtrKey<iMeshWrapper> >::GlobalIterator it = sector->GetPortalMeshes().GetIterator();
    while(it.HasNext())
    {
        csPtrKey<iMeshWrapper> mesh = it.Next();
        iPortalContainer* portalc = mesh->GetPortalContainer();

        for(int pn=0; pn < portalc->GetPortalCount(); pn++)
        {
            // Create weather on each portal, or subscribe to create when first visible
            iPortal* portal = portalc->GetPortal(pn);
            CreatePortalWeather(portal, delta);
        }
    }
}


void ModeHandler::RemovePortalWeather()
{
    // Delete the weather meshes
    for(size_t i = 0; i < portals.GetSize(); i++)
    {
        WeatherPortal* portal = portals[i];

        if(portal->downfall)
        {
            portal->downfall->Destroy();
            delete portal->downfall;
            portal->downfall = NULL;
        }

        if(portal->wi->fog)
        {
            portal->wi->fog->Destroy();
            delete portal->wi->fog;
            portal->wi->fog = NULL;
        }

        portals.DeleteIndex(i);
        i--;
    }
}

void ModeHandler::RemoveWeather()
{
    if(downfall)
    {
        //downfall->StopFollow();
        //downfall->Destroy();
        delete downfall;
        downfall = NULL;
    }

    if(fog)
    {
        //fog->Destroy();
        delete fog;
        fog = NULL;
    }
}


void ModeHandler::HandleNewSectorMessage(MsgEntry* me)
{
    psNewSectorMessage msg(me);

    Debug3(LOG_ANY, 0, "Crossed from sector %s to sector %s.",
           msg.oldSector.GetData(),
           msg.newSector.GetData());

    RemovePortalWeather();
    RemoveWeather();

    WeatherInfo* ri= GetWeatherInfo(msg.newSector);
    if(ri)
    {
        CreateWeather(ri, 0);
    }

    // Create new portals
    iSector* sect = psengine->GetEngine()->FindSector(msg.newSector);
    CreatePortalWeather(sect,0);

    /*
     * Now update bg music if possible.  SoundManager will
     * look for this sound in the soundlib.xml file.
     * Name used is sector name + " background".
     * This will also update weather sounds
     */
    SetSectorMusic(msg.newSector);
}

void ModeHandler::UpdateWeatherSounds()
{
    WeatherSound sound;

    if(downfall)
    {
        sound = downfall->GetWeatherSound();
    }
    else
    {
        sound = WEATHER_SOUND_CLEAR;
    }

    psengine->GetSoundManager()->SetWeather((int) sound);
}

void ModeHandler::SetSectorMusic(const char* sectorname)
{
    // Get the current sound for our main weather object
    WeatherSound sound;

    if(downfall && downfall->GetParent()->sector == sectorname)
    {
        sound = downfall->GetWeatherSound();
    }
    else
    {
        sound = WEATHER_SOUND_CLEAR;
    }

    csString sector(sectorname);
    psengine->GetSoundManager()->LoadActiveSector(sector);
    psengine->GetSoundManager()->SetWeather((int) sound);
    psengine->GetSoundManager()->SetTimeOfDay(clockHour);
}

void ModeHandler::ClearLightFadeSettings()
{
    // Set interpolation to 0% completed.
    last_interpolation_reset = csGetTicks();
    interpolation_complete = false;
    interpolation_step = 0;
    stepStage = 0;

    // Set up lights at 0%.
    UpdateLights(last_interpolation_reset, true);
}

void ModeHandler::FinishLightFade()
{
    // Force lights to go to 100%
    ClearLightFadeSettings();
    interpolation_complete = false;
    interpolation_step = LIGHTING_STEPS-1;

    UpdateLights(csGetTicks()+interpolation_time, true);
}

void ModeHandler::PublishTime(int newTime)
{
    Debug2(LOG_WEATHER, 0, "*New time of the Day: %d", newTime);

    psengine->GetSoundManager()->SetTimeOfDay(newTime);

    // Publish raw time first
    PawsManager::GetSingleton().Publish("TimeOfDay", newTime);

    char time[3];
    time[0] = 'A';
    time[1]='M';
    time[2]='\0';

    if(newTime >= 12)
    {
        newTime-= 12;
        time[0] = 'P';
    }
    // "0 o'clock" is really "12 o'clock" for both AM and PM
    if(newTime == 0)
        newTime = 12;

    csString timeString;
    timeString.Format("%d %s(%s)", newTime, PawsManager::GetSingleton().Translate("o'clock").GetData(), time);
    PawsManager::GetSingleton().Publish("TimeOfDayStr", timeString);
}



void ModeHandler::HandleWeatherMessage(MsgEntry* me)
{
    psWeatherMessage msg(me);

    switch(msg.type)
    {
        case psWeatherMessage::WEATHER:
        {
            if(processWeather)
            {
                if(msg.weather.has_lightning)
                {
                    ProcessLighting(msg.weather);
                }
                if(msg.weather.has_downfall)
                {
                    ProcessDownfall(msg.weather);
                }
                if(msg.weather.has_fog)
                {
                    ProcessFog(msg.weather);
                }
            }
            break;
        }

        case psWeatherMessage::DAYNIGHT:
        {
            uint lastClockHour = clockHour;
            clockHour = msg.hour;

            if(clockHour == lastClockHour)
                break;

            Notify2(LOG_WEATHER, "The time is now %d o'clock.\n", clockHour);

            if(clockHour >= 22 || clockHour <= 6)
                timeOfDay = TIME_NIGHT;
            else if(clockHour >=7 && clockHour <=12)
                timeOfDay = TIME_MORNING;
            else if(clockHour >=13 && clockHour <=18)
                timeOfDay = TIME_AFTERNOON;
            else if(clockHour >=19 && clockHour < 22)
                timeOfDay = TIME_EVENING;

            PublishTime(clockHour);

            // Reset the time basis for interpolation
            if((clockHour == lastClockHour+1) || (lastClockHour == 23 && clockHour == 0))
            {
                last_interpolation_reset = csGetTicks();
                interpolation_step = 0;
                interpolation_complete = false;
                stepStage = 0;
                // Update lighting setup for this time period
                UpdateLights(last_interpolation_reset);
            }
            else // If we've skipped back, or forwards more than one hour, just jump to it.
            {
                FinishLightFade();
            }

            break;
        }
    }
}

float ModeHandler::GetDensity(WeatherInfo* wi)
{
    switch(wi->downfall_condition)
    {
        case WEATHER_RAIN:
            return RainWeatherObject::GetDensity(wi->downfall_params.value);
        case WEATHER_SNOW:
            return SnowWeatherObject::GetDensity(wi->downfall_params.value);
        default:
        {
            switch(wi->fog_condition)
            {
                case WEATHER_FOG:
                    return FogWeatherObject::GetDensity(wi->fog_params.value);
                default:
                    break;
            }
        }
    }

    return 0.0f;
}


void ModeHandler::ProcessLighting(const psWeatherMessage::NetWeatherInfo &info)
{
    Debug2(LOG_WEATHER, 0, "Lightning in sector %s",info.sector.GetData());
    // Get the sequence manager
    csRef<iEngineSequenceManager> seqmgr =  csQueryRegistry<iEngineSequenceManager> (object_reg);
    if(!seqmgr)
    {
        Error2("Couldn't apply thunder in sector %s! No SequenceManager!",info.sector.GetData());
        psSystemMessage sysMsg(0, MSG_INFO, "Error while trying to create a lightning.");
        msghandler->Publish(sysMsg.msg);
        return;
    }

    // Make sure the sequence uses the right ambient light value to return to
    iSector* sector = engine->FindSector(info.sector);
    if(sector)
    {
        lightningreset->SetColor(sector->GetDynamicAmbientLight());
    }

    // Run the lightning sequence.
    csString name(info.sector);
    name.Append(" lightning");
    //We search if the sector has the support for this sequence (lightning)
    if(!seqmgr->FindSequenceByName(name))
    {
        Notify2(LOG_WEATHER,"Couldn't apply thunder in sector %s! No sequence for "
                "this sector!",info.sector.GetData());
        return;
    }
    else
    {
        seqmgr->RunSequenceByName(name,0);
    }

    // Only play thunder if lightning is in current sector
    csVector3 pos;
    if(CheckCurrentSector(actorEntity,info.sector,pos,sector))
    {
        // Queue sound to be played later (actually by UpdateLights below)
        int which = randomgen->Get(5);
        sound_name.Format("amb.weather.thunder[%d]",which);
        sound_when = csGetTicks() + randomgen->Get(4000) + 300;
        sound_queued = true;
    }
}

void ModeHandler::ProcessFog(const psWeatherMessage::NetWeatherInfo &info)
{
    WeatherInfo* wi = GetWeatherInfo(info.sector);

    if(info.fog_density)
    {
        Notify2(LOG_WEATHER, "Fog is coming in %s...",(const char*)info.sector);

        if(!wi)
        {
            // We need a wether info struct for the fog
            wi = CreateWeatherInfo(info.sector);
        }

        wi->fog_condition = WEATHER_FOG;
        wi->r = info.r;
        wi->g = info.g;
        wi->b = info.b;

        wi->fog_params.fade_value = info.fog_density < 10 ? 10 : info.fog_density;
        if(info.fog_fade)
        {
            wi->fog_params.fade_time  = info.fog_fade;
            wi->fog_params.value = 5; // start with some small value
        }
        else
        {
            wi->fog_params.fade_time = 0;
            wi->fog_params.value = wi->fog_params.fade_value;
        }
    }
    else
    {
        // No weather to disable if there are no weather info
        if(!wi)
        {
            return;
        }
        Notify2(LOG_WEATHER, "Fog is fading away in %s...",(const char*)info.sector);

        if(info.fog_fade)
        {
            // Update weatherinfo
            wi->fog_params.fade_value = 0;         // Set no fade
            wi->fog_params.fade_time = info.fog_fade;
        }
        else
        {
            wi->fog_params.value = 0;         // Set no drops
            wi->fog_params.fade_value = 0;    // Set no fade
            wi->fog_params.fade_time = 0;
        }
    }
}

void ModeHandler::ProcessDownfall(const psWeatherMessage::NetWeatherInfo &info)
{
    //    iSector* current = psengine->GetCelClient()->GetMainPlayer()->GetSector();
    WeatherInfo* ri = GetWeatherInfo(info.sector);

    if(info.downfall_drops)  //We are creating rain/snow.
    {
        Notify2(LOG_WEATHER, "It starts raining in %s...",(const char*)info.sector);

        // If no previous weather info create one
        if(!ri)
        {
            ri = CreateWeatherInfo(info.sector);
        }

        ri->downfall_condition  = (info.downfall_is_snow?WEATHER_SNOW:WEATHER_RAIN);

        ri->downfall_params.fade_value = info.downfall_drops < 10 ? 10 : info.downfall_drops;
        if(info.downfall_fade)
        {
            ri->downfall_params.fade_time  = info.downfall_fade;
            ri->downfall_params.value = 5;
        }
        else
        {
            ri->downfall_params.fade_time  = 0;
            ri->downfall_params.value = ri->downfall_params.fade_value;
        }
    }
    else
    {
        //We are removing rain/snow

        // No weather to disable if there are no weather info
        if(!ri)
        {
            return;
        }

        Notify2(LOG_WEATHER, "It stops raining in %s...",(const char*)info.sector);

        if(info.downfall_fade)
        {
            ri->downfall_params.fade_value = 0;         // Set no drops
            ri->downfall_params.fade_time  = info.downfall_fade;
        }
        else
        {
            ri->downfall_params.value      = 0;         // Set no drops
            ri->downfall_params.fade_value = 0;         // Set no fade
            ri->downfall_params.fade_time  = 0;
        }
    }
}


void ModeHandler::PreProcess()
{
    csTicks now = csGetTicks();

    UpdateLights(now);
    UpdateWeather(now);
}

void ModeHandler::UpdateLights()
{
    uint i;

    if(clockHour >= lights.GetSize())
        return;

    LightingList* list = lights[clockHour];

    for(i=0; i<list->colors.GetSize(); i++)
        ProcessLighting(list->colors[i], 0.0f);

    for(i=0; i<list->colors.GetSize(); i++)
        ProcessLighting(list->colors[i], 1.0f);
}

/**
 * This function is called periodically by psclient.  It
 * handles the smooth interpolation of lights to the new
 * values.  It simply calculates a new pct complete and
 * goes through all the light settings of the current time
 * to update them.
 */
void ModeHandler::UpdateLights(csTicks now, bool force)
{
    if(interpolation_complete || clockHour >= lights.GetSize())
    {
        return;
    }

    // Calculate how close to stated values we should be by now
    float pct = (now-last_interpolation_reset);
    pct /= interpolation_time;

    if(pct > 1.0f)
        pct = 1.0f;

    // Divide the interpolation over a number of steps.
    if((100*pct) / (100/LIGHTING_STEPS) < interpolation_step)
    {
        return;
    }

    LightingList* list = lights[clockHour];

    if(list)
    {
        if(force)
        {
            // Process all the lights.
            for(uint i=0; i<list->colors.GetSize(); i++)
            {
                ProcessLighting(list->colors[i], pct);
                stepStage++;
            }
        }
        else
        {
            if(pct == 0.0f)
            {
                // Initialise all lights once.
                for(uint i=0; i<list->colors.GetSize(); i++)
                {
                    ProcessLighting(list->colors[i], pct);
                    stepStage++;
                }
            }
            else
            {
                // Process the lights, 1 a frame.
                ProcessLighting(list->colors[stepStage], pct);
                stepStage++;
            }
        }
    }

    if(stepStage == list->colors.GetSize())
    {
        // Set next step.
        if(interpolation_step+1 == LIGHTING_STEPS)
        {
            interpolation_step = 0;
            interpolation_complete = true;
            last_interpolation_reset = now;
        }
        else
        {
            interpolation_step++;
        }

        stepStage = 0;
    }
}

/**
 * This updates 1 light
 */
bool ModeHandler::ProcessLighting(LightingSetting* setting, float pct)
{
    csColor interpolate_color;
    csColor target_color;

    target_color.red = 0.0f;
    target_color.blue = 0.0f;
    target_color.green = 0.0f;

    // set up target color and differentials if the interpolation is just starting.
    if(pct==0)
    {
        // determine whether to interpolate using weathering color or daylight color
        WeatherInfo* ri = NULL;
        if((const char*)setting->sector)
        {
            ri = GetWeatherInfo(setting->sector);
        }

        // then weathering in sector in which light is located
        if(ri && downfall && (setting->raincolor.red || setting->raincolor.green || setting->raincolor.blue))
        {
            // Interpolate between daylight color and full weather color depending on weather intensity
            target_color.red   = setting->color.red   + ((setting->raincolor.red   - setting->color.red)  * GetDensity(ri));
            target_color.green = setting->color.green + ((setting->raincolor.green - setting->color.green)* GetDensity(ri));
            target_color.blue  = setting->color.blue  + ((setting->raincolor.blue  - setting->color.blue) * GetDensity(ri));
        }
        else
        {
            target_color = setting->color;
        }
    }

    float gamma=psengine->GetBrightnessCorrection();
    target_color.red += gamma;
    target_color.green += gamma;
    target_color.blue += gamma;
    target_color.red = target_color.red<0? 0.0f: target_color.red>=1.0f?
                       1.0f: target_color.red;
    target_color.green = target_color.green<0? 0.0f: target_color.green>=1.0f?
                         1.0f: target_color.green;
    target_color.blue = target_color.blue<0? 0.0f: target_color.blue>=1.0f?
                        1.0f: target_color.blue;

    if(setting->type == "light")
    {
        iLight* light = setting->light_cache;
        if(!light)
        {
            // Light is not in the cache. Try to find it and update cache.
            setting->light_cache = psengine->GetEngine()->FindLight(setting->object);
            light = setting->light_cache;
        }
        if(light)
        {
            if(light->GetDynamicType() != CS_LIGHT_DYNAMICTYPE_PSEUDO)
            {
                Warning2(LOG_WEATHER, "Light '%s' is not marked as <dynamic /> in the world map!", (const char*)setting->object);
                setting->error = true;
                return false;
            }
            csColor existing = light->GetColor(); // only update the light if it is in fact different.
            if(pct == 0)  // save starting color of light for interpolation
            {
                setting->start_color = existing;
                // precalculate diff to target color so only percentages have to be applied later
                setting->diff.red   = target_color.red   - setting->start_color.red;
                setting->diff.green = target_color.green - setting->start_color.green;
                setting->diff.blue  = target_color.blue  - setting->start_color.blue;
            }

            interpolate_color.red   = setting->start_color.red   + (setting->diff.red*pct);
            interpolate_color.green = setting->start_color.green + (setting->diff.green*pct);
            interpolate_color.blue  = setting->start_color.blue  + (setting->diff.blue*pct);

            if(existing.red   != interpolate_color.red ||
                    existing.green != interpolate_color.green ||
                    existing.blue  != interpolate_color.blue)
            {
                light->SetColor(interpolate_color);
            }

            return true;
        }
        else
        {
            // Warning3(LOG_WEATHER, "Light '%s' was not found in lighting setup %d.",(const char*)setting->object, setting->value);
            setting->error = true;
            return false;
        }
    }
    else if(setting->type == "ambient")
    {
        csShaderVariable* ambient = setting->ambient_cache;
        csShaderVariable* combinedAmbient = setting->combined_ambient_cache;
        iSector* sector = setting->sector_cache;
        if(!setting->object.IsEmpty())
        {
            if(!ambient)
            {
                // variable not cached, yet, obtain SV context and get variable
                iShaderVariableContext* context = nullptr;
                iSector* sectorMesh = nullptr;
                if(!setting->object.IsEmpty())
                {
                    // find the mesh object and get the context
                    iMeshWrapper* mesh = psengine->GetEngine()->FindMeshObject(setting->object);
                    if(mesh)
                    {
                        context = mesh->GetSVContext();
                        iMovable* movable = mesh->GetMovable();
                        sectorMesh = movable->GetSectors()->Get(0);
                    }
                }
                if(context)
                {
                    // we got a context, get and cache shader vars now
                    setting->ambient_cache = context->GetVariableAdd(ambientId);
                    ambient = setting->ambient_cache;

                    setting->combined_ambient_cache = context->GetVariable(combinedAmbientId);
                    if(!setting->combined_ambient_cache.IsValid())
                    {
                        // there's no combined ambient SV, yet, create it and initialize it
                        setting->combined_ambient_cache = context->GetVariableAdd(combinedAmbientId);
                        context = sectorMesh->GetSVContext();
                        if(context)
                        {
                            // initialize it to the ambient of the sector + our dynamic ambient part
                            csColor ambientDynamicSector;
                            csColor ambientDynamic;

                            csShaderVariable* ambientSector = context->GetVariableAdd(ambientId);

                            ambientSector->GetValue(ambientDynamicSector);
                            setting->ambient_cache->GetValue(ambientDynamic);

                            setting->combined_ambient_cache->SetValue(ambientDynamicSector + ambientDynamic);
                        }
                    }
                    combinedAmbient = setting->combined_ambient_cache;
                }
            }
        }
        else if(!setting->sector.IsEmpty())
        {
            if(!sector)
            {
                setting->sector_cache = psengine->GetEngine()->FindSector(setting->sector);
                sector = setting->sector_cache;
            }
        }
        if((ambient && combinedAmbient) || sector)
        {
            if(pct == 0)
            {
                if(ambient)
                {
                    ambient->GetValue(setting->start_color);
                    combinedAmbient->GetValue(setting->base_color);
                    setting->base_color -= setting->start_color;
                }
                else
                {
                    setting->start_color = sector->GetDynamicAmbientLight();
                }

                // precalculate diff to target color so only percentages have to be applied later
                setting->diff = target_color - setting->start_color;
            }

            interpolate_color = setting->start_color + pct * setting->diff;

            if(ambient)
            {
                ambient->SetValue(interpolate_color);
                combinedAmbient->SetValue(interpolate_color + setting->base_color);
            }
            else
            {
                sector->SetDynamicAmbientLight(interpolate_color);
            }
            return true;
        }
        else
        {
            // Warning3(LOG_WEATHER, "Object '%s'/Sector '%s' for ambient light was not found in lighting setup %d.",
            //           setting->object.GetData(), setting->sector.GetData(), setting->value);
            setting->error = true;

            return false;
        }
    }
    else if(setting->type == "fog")
    {
        iSector* sector = setting->sector_cache;
        if(!sector)
        {
            // Sector is not in the cache. Try to find it and update cache.
            setting->sector_cache = psengine->GetEngine()->FindSector(setting->object);
            sector = setting->sector_cache;
        }
        if(sector)
        {
            if(setting->density)
            {
                if(pct == 0)
                {
                    if(sector->HasFog())
                    {
                        csColor fogColor = sector->GetFog().color;

                        setting->start_color.Set(fogColor.red,fogColor.green,fogColor.blue);
                    }
                    else
                        setting->start_color.Set(0,0,0);

                    // precalculate diff to target color so only percentages have to be applied later
                    setting->diff.red   = target_color.red   - setting->start_color.red;
                    setting->diff.green = target_color.green - setting->start_color.green;
                    setting->diff.blue  = target_color.blue  - setting->start_color.blue;
                }

                interpolate_color.red   = setting->start_color.red   + (setting->diff.red*pct);
                interpolate_color.green = setting->start_color.green + (setting->diff.green*pct);
                interpolate_color.blue  = setting->start_color.blue  + (setting->diff.blue*pct);
                sector->SetFog(setting->density,interpolate_color);
            }
            else
            {
                sector->DisableFog();
            }

            return true;
        }
        else
        {
            Warning3(LOG_WEATHER, "Sector '%s' for fog was not found in lighting setup %i.",(const char*)setting->object, (int)setting->value);
            setting->error = true;

            return false;
        }
    }
    else
    {
#ifdef DEBUG_LIGHTING
        printf("Lighting setups do not currently support objects of type '%s'.\n",
               (const char*)setting->type);
#endif
        return false;
    }
}


void ModeHandler::UpdateWeather(csTicks when)
{
    // Only update weather if more than updat_time have elapsed
    // since last update
    if(when - last_weather_update < weather_update_time)
    {
        return ;
    }

    int delta = when - last_weather_update;

    // Set a flag to indicate if we are ready to update the gfx yet
    int update_gfx = psengine->GetCelClient()->GetMainPlayer() &&
                     psengine->GetCelClient()->GetMainPlayer()->GetSector();

    csString current_sector;

    if(update_gfx)
    {
        current_sector = psengine->GetCelClient()->GetMainPlayer()->GetSector()->QueryObject()->GetName();
    }

    /*
     * Update fade in weather info so
     * that rain etc are at the right level when
     * entering a new sector.
     */
    csHash<WeatherInfo*, csString>::GlobalIterator loop(weatherlist.GetIterator());
    WeatherInfo* wi, *current_wi = NULL;

    while(loop.HasNext())
    {
        wi = loop.Next();

        wi->Fade(&wi->downfall_params,delta);
        if(wi->downfall_params.value <= 0)
        {
            wi->downfall_params.value = 0;
            wi->downfall_condition = WEATHER_CLEAR;
        }

        wi->Fade(&wi->fog_params,delta);
        if(wi->fog_params.value <= 0)
        {
            wi->fog_params.value = 0;
            wi->fog_condition = WEATHER_CLEAR;
        }

        // Remember wi for current sector for use later
        if(wi->sector == current_sector)
        {
            current_wi = wi;
        }
    }
    wi = current_wi; // Use wi for rest of code, easier to read wi than current_wi

    // Create/Update/Remove weather gfx
    if(update_gfx)
    {
        // Update for current sector if wi
        if(wi)
        {
            CreateWeather(wi,delta);
        }

        // Refresh the portals
        iSector* current = psengine->GetCelClient()->GetMainPlayer()->GetSector();
        CreatePortalWeather(current,delta);

        // Update sounds
        UpdateWeatherSounds();
    }


    last_weather_update = when;
}


void ModeHandler::StartFollowWeather()
{
    downfall->StartFollow();
}

void ModeHandler::StopFollowWeather()
{
    downfall->StopFollow();
    downfall->Destroy();
}

bool ModeHandler::CheckCurrentSector(GEMClientObject* entity,
                                     const char* sectorname,
                                     csVector3 &pos,
                                     iSector*  &sector)
{
    if(!entity || !entity->GetMesh())
    {
        Bug1("No Mesh found on entity!\n");
        return false;
    }

    iMovable* movable = entity->GetMesh()->GetMovable();

    sector = movable->GetSectors()->Get(0);

    // Skip the weather if sector is named and not the same as the player sector
    if(sectorname && strlen(sectorname) && strcmp(sectorname,sector->QueryObject()->GetName()))
    {
        return false;
    }

    pos = movable->GetPosition();

    return true;
}

bool ModeHandler::CreateWeather(WeatherInfo* ri, csTicks delta)
{
    if(!actorEntity)
    {
        Bug1("No player entity has been assigned so weather cannot be created.");
        return false;
    }

    // Check that player is in same sector that the weather and
    // get sector pointer and position.
    csVector3 pos;
    iSector* sector;
    if(!CheckCurrentSector(actorEntity,ri->sector,pos,sector))
    {
        return false;
    }


    // Check for removal of downfall
    if(downfall && ri->downfall_condition == WEATHER_CLEAR)
    {
        downfall->StopFollow();
        downfall->Destroy();
        if(downfall)
        {
            delete downfall;
            downfall = NULL;
        }
        Notify2(LOG_WEATHER, "Downfall removed from sector '%s'",
                sector->QueryObject()->GetName());
    }
    // Check if downfall has changed between rain/snow
    else if(downfall && downfall->GetType() != ri->downfall_condition)
    {
        downfall->StopFollow();
        downfall->Destroy();
        if(downfall)
        {
            delete downfall;
        }

        downfall = CreateDownfallWeatherObject(ri);
        if(downfall->CreateMesh())
        {
            downfall->SetupMesh(downfall->CreateDefaultBBox());
            downfall->StartFollow();
            Notify2(LOG_WEATHER, "Downfall created in sector '%s'",
                    sector->QueryObject()->GetName());
        }
        else
        {
            Error1("Failed to create downfall");
            delete downfall;
            downfall = NULL;
        }
        Notify2(LOG_WEATHER, "Downfall changed type in sector '%s'",
                sector->QueryObject()->GetName());
    }
    // Check if downfall need update
    else if(downfall && ri->downfall_condition != WEATHER_CLEAR)
    {
        downfall->Update(delta);
    }
    // Check for add of downfall
    else if(!downfall && ri->downfall_condition != WEATHER_CLEAR)
    {
        downfall = CreateDownfallWeatherObject(ri);
        if(downfall->CreateMesh())
        {
            downfall->SetupMesh(downfall->CreateDefaultBBox());
            downfall->StartFollow();
            Notify2(LOG_WEATHER, "Downfall created in sector '%s'",
                    sector->QueryObject()->GetName());
        }
        else
        {
            Error1("Failed to create downfall");
            delete downfall;
            downfall = NULL;
        }
    }

    // Check if fog need update
    if(fog)
    {
        fog->Update(delta);
    }

    // Check for removal of fog
    if(fog && ri->fog_condition == WEATHER_CLEAR)
    {
        fog->Destroy();
        delete fog;
        fog = NULL;
        Notify2(LOG_WEATHER, "Fog removed from sector '%s'",
                sector->QueryObject()->GetName());
    }
    // Check for add of fog
    else if(!fog && ri->fog_condition != WEATHER_CLEAR)
    {
        fog = CreateStaticWeatherObject(ri);
        if(fog->CreateMesh())
        {
            Notify2(LOG_WEATHER, "Fog created in sector '%s'",
                    sector->QueryObject()->GetName());
        }
    }


    return true;
}

WeatherObject* ModeHandler::CreateDownfallWeatherObject(WeatherInfo* ri)
{
    WeatherObject* object = NULL;

    switch(ri->downfall_condition)
    {
        case WEATHER_CLEAR:
            return NULL;

        case WEATHER_RAIN:
        {
            object = new RainWeatherObject(ri);
            break;
        }
        case WEATHER_SNOW:
        {
            object = new SnowWeatherObject(ri);
            break;
        }
        default:
        {
            CS_ASSERT(false); // Should not be used to create anything else
            // than downfall
            break;
        }
    }

    return object;

}

WeatherObject* ModeHandler::CreateStaticWeatherObject(WeatherInfo* ri)
{
    WeatherObject* object = NULL;

    switch(ri->fog_condition)
    {
        case WEATHER_CLEAR:
            return NULL;

        case WEATHER_FOG:
        {
            object = new FogWeatherObject(ri);
            break;
        }
        default:
        {
            CS_ASSERT(false); // Should not be used to create anything else
            // than fog
            break;
        }
    }

    return object;

}
void ModeHandler::HandleSpecialCombatEvent(MsgEntry* me)
{
    psSpecialCombatEventMessage event(me);

    if(!psengine->IsGameLoaded())
        return; // Drop if we haven't loaded

    // Get the relevant entities
    GEMClientActor* atObject =  (GEMClientActor*)celclient->FindObject(event.attacker_id);
    GEMClientActor* tarObject = (GEMClientActor*)celclient->FindObject(event.target_id);

    if (!atObject || !tarObject )
    {
        Bug1("NULL Attacker or Target combat event sent to client!");
        return;
    }

    SetCombatAnim( atObject, event.attack_anim );
    
    SetCombatAnim( tarObject, event.defense_anim );





}
void ModeHandler::HandleCombatEvent(MsgEntry* me)
{
    psCombatEventMessage event(me);

    //printf("Got Combat event...\n");
    if(!psengine->IsGameLoaded())
        return; // Drop if we haven't loaded

    // Get the relevant entities
    GEMClientActor* atObject = (GEMClientActor*)celclient->FindObject(event.attacker_id);
    GEMClientActor* tarObject = (GEMClientActor*)celclient->FindObject(event.target_id);

    csString location = psengine->slotName.GetSecondaryName(event.target_location);
    if(!atObject || !tarObject)
    {
        Bug1("NULL Attacker or Target combat event sent to client!");
        return;
    }

    SetCombatAnim(atObject, event.attack_anim);

    if(event.event_type == event.COMBAT_DEATH)
    {
        tarObject->StopMoving();
    }
    else
    {
        SetCombatAnim(tarObject, event.defense_anim);
    }


    if(!chatWindow)
    {
        chatWindow = (pawsChatWindow*)PawsManager::GetSingleton().FindWidget("ChatWindow");
        if(!chatWindow)
        {
            Error1("Couldn't find the communications window (ChatWindow), won't print combat.");
            return;
        }
    }


    // Display the text that goes with it
    if(atObject == celclient->GetMainPlayer())   // we're attacking here
    {
        Attack(event.event_type, event.damage, atObject, tarObject, location);
    }
    else if(tarObject == celclient->GetMainPlayer())    // we're being attacked
    {
        Defend(event.event_type, event.damage, atObject, tarObject, location);
    }
    else // 3rd party msg
    {
        Other(event.event_type, event.damage, atObject, tarObject, location);
    }
}


csString ModeHandler::MungeName(GEMClientActor* obj, bool startOfPhrase)
{
    csString nameTarget = obj->GetName();

    if(nameTarget == obj->race)
    {
        return csString(startOfPhrase ? "The " : "the ").Append(nameTarget);
    }
    else
    {
        return nameTarget;
    }
}

void ModeHandler::AttackBlock(GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    psengine->GetEffectManager()->RenderEffect("combatBlock", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
    if(!(chatWindow->GetSettings().meFilters & COMBAT_BLOCKED))
        return;

    psSystemMessage ev(0,MSG_COMBAT_BLOCK,"You attack %s on the %s but are blocked", MungeName(tarObject).GetData(), location.GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::AttackDamage(float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    if(damage)
    {
        psengine->GetEffectManager()->RenderEffect("combatYourHit", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());

        if(!(chatWindow->GetSettings().meFilters & COMBAT_SUCCEEDED))
            return;

        psSystemMessage ev(0,MSG_COMBAT_YOURHIT,"You hit %s on the %s for %1.2f damage!", MungeName(tarObject).GetData(), location.GetData(), damage);
        msghandler->Publish(ev.msg);

    }
    else
    {
        psengine->GetEffectManager()->RenderEffect("combatYourHitFail", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
        if(!(chatWindow->GetSettings().meFilters & COMBAT_FAILED))
            return;

        psSystemMessage ev(0,MSG_COMBAT_YOURHIT,"You hit %s on the %s but fail to do any damage!", MungeName(tarObject).GetData(), location.GetData());
        msghandler->Publish(ev.msg);

    }

}

void ModeHandler::AttackDeath(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    if(atObject != tarObject)  //not killing self
    {
        if(psengine->GetSoundManager()->GetCombatStance() == 1)
        {
            psengine->GetEffectManager()->RenderEffect("combatVictory", csVector3(0, 0, 0), atObject->GetMesh()); /* FIXMESOUND sound played by effect */
        } /* FIXMESOUND */

        psSystemMessage ev(0,MSG_COMBAT_VICTORY,"You have killed %s!", MungeName(tarObject).GetData());
        msghandler->Publish(ev.msg);


    }
    else //killing self
    {
        psSystemMessage ev(0,MSG_COMBAT_DEATH,"You have killed yourself!");
        msghandler->Publish(ev.msg);
    }
}

void ModeHandler::AttackDodge(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    psengine->GetEffectManager()->RenderEffect("combatDodge", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
    if(!(chatWindow->GetSettings().meFilters & COMBAT_DODGED))
        return;

    psSystemMessage ev(0,MSG_COMBAT_DODGE,"%s has dodged your attack!", MungeName(tarObject, true).GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::AttackMiss(GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    psengine->GetEffectManager()->RenderEffect("combatMiss", csVector3(0, 0, 0), atObject->GetMesh(), tarObject->GetMesh());
    if(!(chatWindow->GetSettings().meFilters & COMBAT_MISSED))
        return;

    psSystemMessage ev(0,MSG_COMBAT_MISS,"You attack %s but missed the %s.", MungeName(tarObject).GetData(), location.GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::AttackOutOfRange(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    psengine->GetEffectManager()->RenderEffect("combatMiss", csVector3(0, 0, 0), atObject->GetMesh(), tarObject->GetMesh());

    psSystemMessage ev(0,MSG_COMBAT_MISS,"You are too far away to attack %s.", MungeName(tarObject).GetData());
    msghandler->Publish(ev.msg);

}



void ModeHandler::Attack(int type, float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    switch(type)
    {
        case psCombatEventMessage::COMBAT_BLOCK:
        {
            AttackBlock(atObject, tarObject, location);
            break;
        }
        case psCombatEventMessage::COMBAT_DAMAGE_NEARLY_DEAD:
        {
            OtherNearlyDead(tarObject);
            // no break by intention
        }
        case psCombatEventMessage::COMBAT_DAMAGE:
        {
            AttackDamage(damage, atObject, tarObject, location);
            break;
        }
        case psCombatEventMessage::COMBAT_DEATH:
        {
            AttackDeath(atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_DODGE:
        {
            AttackDodge(atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_MISS:
        {
            AttackMiss(atObject, tarObject, location);
            break;
        }
        case psCombatEventMessage::COMBAT_OUTOFRANGE:
        {
            AttackOutOfRange(atObject, tarObject);
            break;
        }
    }
}


void ModeHandler::DefendBlock(GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    psengine->GetEffectManager()->RenderEffect("combatBlock", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());

    if(!(chatWindow->GetSettings().meFilters & COMBAT_BLOCKED))
        return;

    psSystemMessage ev(0,MSG_COMBAT_BLOCK,"%s attacks you but your %s blocks it.", MungeName(atObject, true).GetData(), location.GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::DefendDamage(float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    if(damage)
    {
        psengine->GetEffectManager()->RenderEffect("combatHitYou", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
        if(!(chatWindow->GetSettings().meFilters & COMBAT_SUCCEEDED))
            return;

        psSystemMessage ev(0,MSG_COMBAT_HITYOU,"%s hits you on the %s for %1.2f damage!",  MungeName(atObject, true).GetData(), location.GetData(), damage);
        msghandler->Publish(ev.msg);

    }
    else
    {
        psengine->GetEffectManager()->RenderEffect("combatHitYouFail", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
        if(!(chatWindow->GetSettings().meFilters & COMBAT_FAILED))
            return;

        psSystemMessage ev(0,MSG_COMBAT_HITYOU,"%s hits you on the %s but fails to do any damage!",  MungeName(atObject, true).GetData(), location.GetData());
        msghandler->Publish(ev.msg);

    }

}

void ModeHandler::DefendDeath(GEMClientActor* atObject)
{
    //atObject->
    psengine->GetEffectManager()->RenderEffect("combatDeath", csVector3(0,0,0), atObject->GetMesh());

    psSystemMessage ev(0,MSG_COMBAT_OWN_DEATH,"You have been killed by %s!", MungeName(atObject).GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::DefendDodge(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    psengine->GetEffectManager()->RenderEffect("combatDodge", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
    if(!(chatWindow->GetSettings().meFilters & COMBAT_DODGED))
        return;

    psSystemMessage ev(0,MSG_COMBAT_DODGE,"%s attacks you but you dodge.", MungeName(atObject, true).GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::DefendMiss(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    psengine->GetEffectManager()->RenderEffect("combatMiss", csVector3(0, 0, 0), atObject->GetMesh(), tarObject->GetMesh());
    if(!(chatWindow->GetSettings().meFilters & COMBAT_MISSED))
        return;

    psSystemMessage ev(0,MSG_COMBAT_MISS,"%s attacks you but misses.", MungeName(atObject, true).GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::DefendOutOfRange(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    psengine->GetEffectManager()->RenderEffect("combatMiss", csVector3(0, 0, 0), atObject->GetMesh(), tarObject->GetMesh());

    psSystemMessage ev(0,MSG_COMBAT_MISS,"%s attacks but is too far away to reach you.", MungeName(atObject, true).GetData());
    msghandler->Publish(ev.msg);

}

void ModeHandler::DefendNearlyDead()
{
    if(!(chatWindow->GetSettings().meFilters & COMBAT_SUCCEEDED))
        return;
    psSystemMessage ev(0, MSG_COMBAT_NEARLY_DEAD, "You are nearly dead!");
    msghandler->Publish(ev.msg);
}

void ModeHandler::Defend(int type, float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString &location)
{
    switch(type)
    {
        case psCombatEventMessage::COMBAT_BLOCK:
        {
            DefendBlock(atObject, tarObject, location);
            break;
        }

        case psCombatEventMessage::COMBAT_DAMAGE_NEARLY_DEAD:
        {
            DefendNearlyDead();
            // no break by intention
        }
        case psCombatEventMessage::COMBAT_DAMAGE:
        {
            DefendDamage(damage, atObject, tarObject, location);
            break;
        }

        case psCombatEventMessage::COMBAT_DEATH:
        {
            DefendDeath(atObject);
            break;
        }

        case psCombatEventMessage::COMBAT_DODGE:
        {
            DefendDodge(atObject, tarObject);
            break;
        }

        case psCombatEventMessage::COMBAT_MISS:
        {
            DefendMiss(atObject, tarObject);
            break;
        }

        case psCombatEventMessage::COMBAT_OUTOFRANGE:
        {
            DefendOutOfRange(atObject, tarObject);
            break;
        }
    }
}

void ModeHandler::OtherBlock(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    bool isGrouped = celclient->GetMainPlayer()->IsGroupedWith(atObject) ||
                     celclient->GetMainPlayer()->IsGroupedWith(tarObject);
    bool isYours   = celclient->GetMainPlayer()->IsOwnedBy(atObject) ||
                     celclient->GetMainPlayer()->IsOwnedBy(tarObject);
    int level = celclient->GetMainPlayer()->GetType();

    psengine->GetEffectManager()->RenderEffect("combatBlock", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());

    if(!((level > 0 || isGrouped || isYours) && (chatWindow->GetSettings().vicinityFilters & COMBAT_BLOCKED)))
        return;

    psSystemMessage ev(0,MSG_COMBAT_BLOCK,"%s attacks %s but they are blocked.", MungeName(atObject, true).GetData(), MungeName(tarObject).GetData());
    msghandler->Publish(ev.msg);
}

void ModeHandler::OtherDamage(float damage, GEMClientActor* atObject, GEMClientActor* tarObject)
{
    bool isGrouped = celclient->GetMainPlayer()->IsGroupedWith(atObject) ||
                     celclient->GetMainPlayer()->IsGroupedWith(tarObject);
    bool isYours   = celclient->GetMainPlayer()->IsOwnedBy(atObject) ||
                     celclient->GetMainPlayer()->IsOwnedBy(tarObject);
    int level = celclient->GetMainPlayer()->GetType();

    if(damage)
    {
        psengine->GetEffectManager()->RenderEffect("combatHitOther", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
        if(!((level > 0 || isGrouped || isYours) && (chatWindow->GetSettings().vicinityFilters & COMBAT_SUCCEEDED)))
            return;

        psSystemMessage ev(0,MSG_COMBAT_HITOTHER,"%s hits %s for %1.2f damage!", MungeName(atObject, true).GetData(), MungeName(tarObject).GetData(), damage);
        msghandler->Publish(ev.msg);
    }
    else
    {
        psengine->GetEffectManager()->RenderEffect("combatHitOtherFail", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
        if(!((level > 0 || isGrouped || isYours) && (chatWindow->GetSettings().vicinityFilters & COMBAT_FAILED)))
            return;
        psSystemMessage ev(0,MSG_COMBAT_HITOTHER,"%s hits %s, but fails to do any damage!", MungeName(atObject, true).GetData(), MungeName(tarObject).GetData());
        msghandler->Publish(ev.msg);
    }

}

void ModeHandler::OtherDeath(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    if(atObject != tarObject)  //not killing self
    {
        psSystemMessage ev(0,MSG_COMBAT_DEATH,"%s has been killed by %s!", MungeName(tarObject, true).GetData(), MungeName(atObject).GetData());
        msghandler->Publish(ev.msg);
    }
    else //killing self
    {
        psSystemMessage ev(0,MSG_COMBAT_DEATH,"%s has died!", MungeName(tarObject, true).GetData());
        msghandler->Publish(ev.msg);
    }
}

void ModeHandler::OtherDodge(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    bool isGrouped = celclient->GetMainPlayer()->IsGroupedWith(atObject) ||
                     celclient->GetMainPlayer()->IsGroupedWith(tarObject);
    bool isYours   = celclient->GetMainPlayer()->IsOwnedBy(atObject) ||
                     celclient->GetMainPlayer()->IsOwnedBy(tarObject);
    int level = celclient->GetMainPlayer()->GetType();

    psengine->GetEffectManager()->RenderEffect("combatDodge", csVector3(0, 0, 0), tarObject->GetMesh(), atObject->GetMesh());
    if(!((level > 0 || isGrouped || isYours) && (chatWindow->GetSettings().vicinityFilters & COMBAT_DODGED)))
        return;
    psSystemMessage ev(0,MSG_COMBAT_DODGE,"%s attacks %s but %s dodges.", MungeName(atObject, true).GetData(),MungeName(tarObject).GetData(),MungeName(tarObject).GetData());
    msghandler->Publish(ev.msg);
}

void ModeHandler::OtherMiss(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    bool isGrouped = celclient->GetMainPlayer()->IsGroupedWith(atObject) ||
                     celclient->GetMainPlayer()->IsGroupedWith(tarObject);
    bool isYours   = celclient->GetMainPlayer()->IsOwnedBy(atObject) ||
                     celclient->GetMainPlayer()->IsOwnedBy(tarObject);
    int level = celclient->GetMainPlayer()->GetType();

    psengine->GetEffectManager()->RenderEffect("combatMiss", csVector3(0, 0, 0), atObject->GetMesh(), tarObject->GetMesh());
    if(!((level > 0 || isGrouped || isYours) && (chatWindow->GetSettings().vicinityFilters & COMBAT_MISSED)))
        return;
    psSystemMessage ev(0,MSG_COMBAT_MISS,"%s attacks %s but misses.", MungeName(atObject, true).GetData(),MungeName(tarObject).GetData());
    msghandler->Publish(ev.msg);
}

void ModeHandler::OtherOutOfRange(GEMClientActor* atObject, GEMClientActor* tarObject)
{
    psengine->GetEffectManager()->RenderEffect("combatMiss", csVector3(0, 0, 0), atObject->GetMesh(), tarObject->GetMesh());
    psSystemMessage ev(0,MSG_COMBAT_MISS,"%s attacks but is too far away to reach %s.", MungeName(atObject, true).GetData(),MungeName(tarObject).GetData());
    msghandler->Publish(ev.msg);
}

void ModeHandler::OtherNearlyDead(GEMClientActor* tarObject)
{
    if(!(chatWindow->GetSettings().vicinityFilters & COMBAT_SUCCEEDED))
        return;
    psSystemMessage ev(0, MSG_COMBAT_NEARLY_DEAD, "%s is nearly dead!", MungeName(tarObject, true).GetData());
    msghandler->Publish(ev.msg);
}

void ModeHandler::Other(int type, float damage, GEMClientActor* atObject, GEMClientActor* tarObject, csString & /*location*/)
{
    switch(type)
    {
        case psCombatEventMessage::COMBAT_BLOCK:
        {
            OtherBlock(atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_DAMAGE_NEARLY_DEAD:
        {
            OtherNearlyDead(tarObject);
            // no break by intention
        }
        case psCombatEventMessage::COMBAT_DAMAGE:
        {
            OtherDamage(damage, atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_DEATH:
        {
            OtherDeath(atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_DODGE:
        {
            OtherDodge(atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_MISS:
        {
            OtherMiss(atObject, tarObject);
            break;
        }
        case psCombatEventMessage::COMBAT_OUTOFRANGE:
        {
            OtherOutOfRange(atObject, tarObject);
            break;
        }
    }
}

void ModeHandler::SetCombatAnim(GEMClientActor* atObject, csStringID anim)
{
    // Set the relevant anims
    if(anim != csStringID(uint32_t(csInvalidStringID)))
    {
        iSpriteCal3DState* state = atObject->cal3dstate;
        if(state)
        {
            int idx = atObject->GetAnimIndex(celclient->GetClientDR()->GetMsgStrings(), anim);
            state->SetAnimAction(idx,0.5F,0.5F);
        }
    }
}

void ModeHandler::HandleCachedFile(MsgEntry* me)
{
    psCachedFileMessage msg(me);
    if(!msg.valid)
    {
        csPrintf("Cached File Message received was not valid!\n");
        return;
    }
    if(psengine->GetSoundManager()->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->GetToggle() == true)
    {
        csString fname;
        fname.Format("/planeshift/userdata/cache/%s",msg.hash.GetDataSafe());

        csPrintf(">>Got audio file '%s' to play, in '%s'.\n", msg.hash.GetDataSafe(), fname.GetDataSafe());

        // Check for cached version
        if(!msg.databuf.IsValid())
        {
            csPrintf(">>Checking if file exists locally already.\n");
            if(!vfs->Exists(fname))   // doesn't exist so we need to request it
            {
                psengine->GetMsgHandler()->GetNextSequenceNumber(MSGTYPE_CACHEFILE);
                csPrintf(">>Requesting file '%s' from server.\n", msg.hash.GetDataSafe());
                psCachedFileMessage request(0,0,msg.hash,NULL); // cheating here to send the hash back in the filename field
                request.SendMessage();
            }
            else // does exist, and we're done
            {
                csPrintf("Yes, it is cached already.  Playing immediately.\n");
                // Queue that file
                psengine->GetSoundManager()->PushQueueItem(iSoundManager::VOICE_QUEUE, fname.GetData());
            }
        }
        else
        {
            csPrintf(">>Received file from server.  Putting in cache.\n");
            // Save sound file
            if(!vfs->WriteFile(fname,msg.databuf->GetData(), msg.databuf->GetSize()))
            {
                Error2(">>Could not write cached file '%s'.",fname.GetData());
                return;
            }
            // Queue that file
            psengine->GetSoundManager()->PushQueueItem(iSoundManager::VOICE_QUEUE, fname.GetData());
        }
    }
}
