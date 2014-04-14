/*
 * weather.cpp Author: Christian Svensson
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 * Holder of all weather types
 */
#include <psconfig.h>

#include <csutil/objreg.h>
#include <csutil/sysfunc.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/light.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/camera.h>
#include <imesh/object.h>
#include <imesh/partsys.h>
#include <imesh/particles.h>
#include <csutil/cscolor.h>
#include <csutil/flags.h>
#include <cstool/fogmath.h>
#include <iutil/vfs.h>
#include <iutil/plugin.h>
#include <iutil/stringarray.h>
#include <ivideo/material.h>
#include <imap/loader.h>
#include <iutil/cfgmgr.h>

#include <ibgloader.h>
#include "pscelclient.h"
#include "globals.h"
#include "weather.h"
#include "pscamera.h"
#include "modehandler.h"


void WeatherInfo::Fade(WeatherParams* wp, int delta)
{
    if (wp->fade_time)
    {
        // Only interpolate to the point where fade stops
        if (delta > wp->fade_time)
        {
            delta = wp->fade_time;
        }

        float pct = (float)delta/(float)wp->fade_time;
        
        wp->value += (int)((wp->fade_value - wp->value)*pct);
        
        // Substract the delta time from the fade.
        wp->fade_time -= delta;
    }
}



/* Keeper of the objects */
WeatherObject::WeatherObject(WeatherInfo* parent)
{
    this->parent = parent;
}
WeatherObject::~WeatherObject()
{
    Destroy();
}

void WeatherObject::Destroy()
{
    if(mesh)
    {
        // Remove
        psengine->GetEngine()->RemoveObject(mesh);
        // CHECK_FINAL_DECREF_CONFIRM(mesh,"WeatherObject iMeshWrapper");
    }
}

bool WeatherObject::Valid()
{
    return mesh.IsValid();
}

iSector* WeatherObject::GetSector()
{
    if(Valid())
        return mesh->GetMovable()->GetSectors()->Get(0);

    return NULL;
}

void WeatherObject::RefreshSector()
{
    if(!Valid())
        return;

    // Refresh the flags
    iSector* sector = mesh->GetMovable()->GetSectors ()->Get(0);
    mesh->GetMovable()->ClearSectors();
    mesh->GetMovable()->SetSector(sector);
    mesh->GetMovable()->UpdateMove();
}

void WeatherObject::MoveTo( WeatherInfo* new_parent ,iSector* sector)
{
    if(!Valid())
        return;

    mesh->GetMovable()->ClearSectors();
    mesh->GetMovable()->SetSector(sector);
    mesh->GetMovable()->UpdateMove();

    SetParent(new_parent);
    SetupMesh(bbox); // Setup the mesh again for the new parent info
}

void WeatherObject::MoveTo(csVector3 pos)
{
    mesh->GetMovable()->SetPosition(pos);
    mesh->GetMovable()->UpdateMove();
}

void WeatherObject::StartFollow()
{
    if(Valid())
    {
        mesh->GetFlags().Set(CS_ENTITY_CAMERA);  // Makes weather mesh follow player around
        RefreshSector();
    }
}

void WeatherObject::StopFollow()
{
    if(Valid())
    {
        mesh->GetFlags().Reset(CS_ENTITY_CAMERA);
        RefreshSector();
    }
}


/***************************************************** Rain object ****/

RainWeatherObject::RainWeatherObject(WeatherInfo* parent) : WeatherObject(parent)
{
}
RainWeatherObject::~RainWeatherObject()
{
}

WeatherSound RainWeatherObject::GetWeatherSoundForced()
{
    if(parent->downfall_params.value > 2000)
        return WEATHER_SOUND_RAIN_HEAVY;
    else if(parent->downfall_params.value > 1000)
	return WEATHER_SOUND_RAIN_MEDIUM;
    else
        return WEATHER_SOUND_RAIN_LIGHT;
}

WeatherSound RainWeatherObject::GetWeatherSound()
{
    if(Valid())
        return GetWeatherSoundForced();
    else
        return WEATHER_SOUND_CLEAR;
}

float RainWeatherObject::GetDensity(int drops)
{
    return (float)drops / (float)WEATHER_MAX_RAIN_DROPS;
}

csBox3 RainWeatherObject::CreateDefaultBBox()
{
    csBox3 bbox;
    bbox.Set(csVector3(-5,-10,-5),csVector3(5,10,5));

    return bbox;
}

void RainWeatherObject::Destroy()
{
    WeatherObject::Destroy();

    ModeHandler* mh = psengine->GetModeHandler();
    if(mh)
    {
        mh->RemoveDownfallObject(this);
    }
}


void RainWeatherObject::MoveTo(WeatherInfo* wi,iSector* sect)
{
    WeatherObject::MoveTo(wi,sect);
    psengine->GetModeHandler()->AddDownfallObject(this);

    // refresh rainstate
    SetupMesh(bbox);

}

bool RainWeatherObject::CreateMesh()
{
    if(!mfw)
    {
        // Get the shader.
        csRef<iShaderManager> shman = csQueryRegistry<iShaderManager>(psengine->GetObjectRegistry());
        csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>(
            psengine->GetObjectRegistry(), "crystalspace.shared.stringset");
        csRef<iStringArray> shaderName = psengine->GetLoader()->GetShaderName("particles");
        csRef<iShader> shader = shman->GetShader(shaderName->Get(0));
        csRef<iLoader> loader = csQueryRegistry<iLoader> (psengine->GetObjectRegistry());

        // Create new rain
        const char* rainTex = psengine->GetConfig()->GetStr("PlaneShift.Effects.Rain.Texture", "/this/art/effects/raindrop.dds");
        loader->LoadTexture("raindrop", rainTex);
        mat = psengine->GetEngine()->GetMaterialList()->FindByName("raindrop");

        csStringID shadertype = strings->Request("diffuse");
        mat->GetMaterial()->SetShader(shadertype, shader);
        shadertype = strings->Request("ambient");
        mat->GetMaterial()->SetShader(shadertype, shader);
        shadertype = strings->Request("depthwrite");
        mat->GetMaterial()->SetShader(shadertype, 0);
        mfw = psengine->GetEngine ()->CreateMeshFactory ("crystalspace.mesh.object.particles", "rain", false);
        if (!mfw)
        {
            Bug1("Could not create rain factory.");
            return false;
        }
    }

    // Get the sector
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector);
        
    // Create the mesh
    if (mesh.IsValid())
    {
        psengine->GetEngine()->RemoveObject(mesh);
    }

    mesh = psengine->GetEngine ()->CreateMeshWrapper(mfw, "rain", sector,
                                                    csVector3 (0, 0, 0));

    mesh->SetRenderPriority (psengine->GetEngine()->GetAlphaRenderPriority ());
    mesh->SetZBufMode(CS_ZBUF_TEST);
    mesh->GetMeshObject()->SetMixMode (CS_FX_ADD);
    mesh->GetMeshObject()->SetMaterialWrapper(mat);

    csRef<iParticleBuiltinEmitterFactory> emit_factory = 
        csLoadPluginCheck<iParticleBuiltinEmitterFactory> (
            psengine->GetObjectRegistry(), "crystalspace.mesh.object.particles.emitter", false);
    csRef<iParticleBuiltinEffectorFactory> eff_factory = 
        csLoadPluginCheck<iParticleBuiltinEffectorFactory> (
            psengine->GetObjectRegistry(), "crystalspace.mesh.object.particles.effector", false);

    csRef<iParticleBuiltinEmitterBox> boxemit = emit_factory->CreateBox ();
    boxemit->SetBox (bbox);
    boxemit->SetParticlePlacement (CS_PARTICLE_BUILTIN_VOLUME);
    boxemit->SetInitialMass (5.0f, 7.5f);
    boxemit->SetUniformVelocity (true);
    boxemit->SetInitialTTL (2.5f, 2.5f);
    boxemit->SetInitialVelocity (csVector3 (0, -5, 0), csVector3 (0));

    csRef<iParticleBuiltinEffectorLinColor> lincol = eff_factory->
        CreateLinColor ();
    lincol->AddColor (csColor4 (.25,.25,.25,1), 2.5f);

    csRef<iParticleSystem> partstate =
  	scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());

    float rainSize = psengine->GetConfig()->GetFloat("PlaneShift.Effects.Rain.Size", 0.1f);
    float rainRatio = psengine->GetConfig()->GetFloat("PlaneShift.Effects.Rain.Ratio", 50.f);
    partstate->SetParticleSize (csVector2 (rainSize/rainRatio, rainSize));
    partstate->SetParticleRenderOrientation (CS_PARTICLE_ORIENT_COMMON);
    partstate->SetCommonDirection (csVector3 (0, 1, 0));
    partstate->AddEmitter (boxemit);
    partstate->AddEffector (lincol);

    psengine->GetModeHandler()->AddDownfallObject(this);

    return true;
}

void RainWeatherObject::SetupMesh(csBox3 bbox)
{
    this->bbox = bbox; // for reuse

    csRef<iParticleSystem> partstate =
  	scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());
    csRef<iParticleBuiltinEmitterBox> boxemit = 
        scfQueryInterface<iParticleBuiltinEmitterBox>(partstate->GetEmitter(0));

    partstate->SetMinBoundingBox (bbox);
    boxemit->SetBox(bbox);

    SetDrops(parent->downfall_params.value);
}

void RainWeatherObject::SetDrops(int drops)
{
    csRef<iParticleSystem> partstate =
  	scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());

    csRef<iParticleBuiltinEmitterBox> boxemit = 
        scfQueryInterface<iParticleBuiltinEmitterBox>(partstate->GetEmitter(0));

    if(drops > WEATHER_MAX_RAIN_DROPS)
    {
        drops = WEATHER_MAX_RAIN_DROPS;
    }
    if (drops != boxemit->GetEmissionRate()*2.5)
    {
        boxemit->SetEmissionRate (drops / 2.5f);
        Notify2(LOG_WEATHER,"Setting drop emission rate: %.2f",boxemit->GetEmissionRate());

    }
}


void RainWeatherObject::Update(csTicks /*delta*/)
{
    SetDrops(parent->downfall_params.value);
}


/***************************************************** Snow object ****/

SnowWeatherObject::SnowWeatherObject(WeatherInfo* parent)  : WeatherObject(parent)
{
    csRef<iShaderManager> shman = csQueryRegistry<iShaderManager>(psengine->GetObjectRegistry());
    csRef<iShaderVarStringSet> strings = csQueryRegistryTagInterface<iShaderVarStringSet> (
            psengine->GetObjectRegistry(), "crystalspace.shader.variablenameset");

    // set density variable
    snowDensity = strings->Request("snow density");
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector);
    snowDensitySV = sector->GetSVContext()->GetVariableAdd(snowDensity);
    snowDensitySV->SetValue(0.75f);

    // set texture variable
    csString snowNoise = psengine->GetConfig()->GetStr("PlaneShift.Effects.Snow.Noise", "");
    if(!snowNoise.IsEmpty())
    {
        csRef<iLoader> loader = csQueryRegistry<iLoader> (psengine->GetObjectRegistry());
        loader->LoadTexture("snowdiffuse", snowNoise.GetData());
        iMaterialWrapper* material = psengine->GetEngine()->GetMaterialList()->FindByName("snowdiffuse");
        CS::ShaderVarStringID snowTex = strings->Request ("tex snow 1");
        csShaderVariable* snowTexSV = shman->GetVariableAdd(snowTex);
        snowTexSV->SetValue(material->GetMaterial()->GetTexture());
    }
}

SnowWeatherObject::~SnowWeatherObject()
{
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector);
    sector->GetSVContext()->RemoveVariable(snowDensitySV);
}

WeatherSound SnowWeatherObject::GetWeatherSoundForced()
{
    if(parent->downfall_params.value > 2000)
        return WEATHER_SOUND_SNOW_HEAVY;
    else
        return WEATHER_SOUND_SNOW_LIGHT;
}

WeatherSound SnowWeatherObject::GetWeatherSound()
{
    if(Valid())
        return GetWeatherSoundForced();
    else
        return WEATHER_SOUND_CLEAR;
}

float SnowWeatherObject::GetDensity(int drops)
{
    return (float)drops / (float)WEATHER_MAX_SNOW_FALKES;
}

csBox3 SnowWeatherObject::CreateDefaultBBox()
{
    csBox3 bbox;
    bbox.Set(csVector3(-10,-20,-10),csVector3(10,20,10));

    return bbox;
}

void SnowWeatherObject::Destroy()
{
    WeatherObject::Destroy();
    
    ModeHandler* mh = psengine->GetModeHandler();
    if(mh)
    {
        mh->RemoveDownfallObject(this);
    }
}

void SnowWeatherObject::MoveTo(WeatherInfo* wi,iSector* sect)
{
    // get current density
    float density = 0.f;
    snowDensitySV->GetValue(density);

    // remove density from old position
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector);
    sector->GetSVContext()->RemoveVariable(snowDensitySV);

    // perform the basic move
    WeatherObject::MoveTo(wi,sect);
    psengine->GetModeHandler()->AddDownfallObject(this);

    // add density to new position
    snowDensitySV = sect->GetSVContext()->GetVariableAdd(snowDensity);
    snowDensitySV->SetValue(density);

    // setup particle emittor
    SetupMesh(bbox);
}

bool SnowWeatherObject::CreateMesh()
{
    if(!mfw)
    {
        // Get the shader.
        csRef<iShaderManager> shman = csQueryRegistry<iShaderManager>(psengine->GetObjectRegistry());
        csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet>(
            psengine->GetObjectRegistry(), "crystalspace.shared.stringset");
        csRef<iStringArray> shaderName = psengine->GetLoader()->GetShaderName("particles");
        csRef<iShader> shader = shman->GetShader(shaderName->Get(0));
        csRef<iLoader> loader = csQueryRegistry<iLoader> (psengine->GetObjectRegistry());

        // Create new snow
        const char* snowTex = psengine->GetConfig()->GetStr("PlaneShift.Effects.Snow.Texture", "/this/art/effects/snowflakes.dds");
        loader->LoadTexture("snowflake", snowTex);
        mat = psengine->GetEngine()->GetMaterialList()->FindByName("snowflake");

        csStringID shadertype = strings->Request("diffuse");
        mat->GetMaterial()->SetShader(shadertype, shader);
        shadertype = strings->Request("ambient");
        mat->GetMaterial()->SetShader(shadertype, shader);
        shadertype = strings->Request("depthwrite");
        mat->GetMaterial()->SetShader(shadertype, 0);

        mfw = psengine->GetEngine ()->CreateMeshFactory ("crystalspace.mesh.object.particles", "snow", false);
        if (!mfw)
        {
            Bug1("Could not create snow factory.");
            return false;
        }
    }

    // Get the sector
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector);

    // Create the mesh
    if (mesh.IsValid())
    {
        psengine->GetEngine()->RemoveObject(mesh);
    }

    mesh = psengine->GetEngine ()->CreateMeshWrapper(mfw, "snow", sector,
                                                    csVector3 (0, 0, 0));

    mesh->SetRenderPriority(psengine->GetEngine()->GetAlphaRenderPriority());
    mesh->SetZBufMode(CS_ZBUF_TEST);
    mesh->GetMeshObject()->SetMixMode(CS_FX_ADD);
    mesh->GetMeshObject()->SetMaterialWrapper(mat);

    csRef<iParticleBuiltinEmitterFactory> emit_factory = 
        csLoadPluginCheck<iParticleBuiltinEmitterFactory> (
            psengine->GetObjectRegistry(), "crystalspace.mesh.object.particles.emitter", false);
    csRef<iParticleBuiltinEffectorFactory> eff_factory = 
        csLoadPluginCheck<iParticleBuiltinEffectorFactory> (
            psengine->GetObjectRegistry(), "crystalspace.mesh.object.particles.effector", false);

    csRef<iParticleBuiltinEmitterBox> boxemit = emit_factory->CreateBox ();
    boxemit->SetBox (bbox);
    boxemit->SetParticlePlacement (CS_PARTICLE_BUILTIN_VOLUME);
    boxemit->SetInitialMass (5.0f, 7.5f);
    boxemit->SetUniformVelocity (true);
    boxemit->SetInitialTTL (5.0f, 5.0f);
    boxemit->SetInitialVelocity (csVector3 (0, -1.5f, 0), csVector3 (0));

    csRef<iParticleBuiltinEffectorLinColor> lincol = eff_factory->
        CreateLinColor ();
    lincol->AddColor (csColor4 (.25,.25,.25,1), 2.5f);

    csRef<iParticleBuiltinEffectorForce> force = eff_factory->
        CreateForce ();
    force->SetRandomAcceleration (csVector3 (1.5f, 0.0f, 1.5f));

    csRef<iParticleSystem> partstate =
  	scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());

    float snowSize = psengine->GetConfig()->GetFloat("PlaneShift.Effects.Snow.Size", 0.07f);
    float snowRatio = psengine->GetConfig()->GetFloat("PlaneShift.Effects.Snow.Ratio", 1.f);
    partstate->SetParticleSize (csVector2 (snowSize/snowRatio, snowSize));
    partstate->AddEmitter (boxemit);
    partstate->AddEffector (lincol);
    partstate->AddEffector (force);

    psengine->GetModeHandler()->AddDownfallObject(this);

    return true;
}

void SnowWeatherObject::SetupMesh(csBox3 bbox)
{
    this->bbox = bbox; // for reuse

    csRef<iParticleSystem> partstate =
  	scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());
    csRef<iParticleBuiltinEmitterBox> boxemit = 
        scfQueryInterface<iParticleBuiltinEmitterBox>(partstate->GetEmitter(0));

    partstate->SetMinBoundingBox (bbox);
    boxemit->SetBox(bbox);

    SetDrops(parent->downfall_params.value);
}

void SnowWeatherObject::SetDrops(int drops)
{
    csRef<iParticleSystem> partstate =
  	scfQueryInterface<iParticleSystem> (mesh->GetMeshObject ());

    csRef<iParticleBuiltinEmitterBox> boxemit = 
        scfQueryInterface<iParticleBuiltinEmitterBox>(partstate->GetEmitter(0));

    if(drops > WEATHER_MAX_SNOW_FALKES)
    {
        drops = WEATHER_MAX_SNOW_FALKES;
    }
    if (drops != boxemit->GetEmissionRate()*2.5)
    {
        boxemit->SetEmissionRate (drops / 2.5f);
    }
    snowDensitySV->SetValue(GetDensity(drops));
}



void SnowWeatherObject::Update(csTicks /*delta*/)
{
    //    Notify2(LOG_WEATHER,"Updating snow object, Setting drops: %d",parent->downfall_params.value);
    
    SetDrops(parent->downfall_params.value);
}


/***************************************************** Fog object ****/

FogWeatherObject::FogWeatherObject(WeatherInfo* parent) : WeatherObject(parent)
{
    sector = psengine->GetEngine()->FindSector(parent->sector);
    SetColor(parent->r / 255.0f, parent->g / 255.0f, parent->b / 255.0f);
    applied = false;
}

FogWeatherObject::~FogWeatherObject()
{
}

void FogWeatherObject::Destroy()
{   
    //It is necessary to get the new sector to which everything is referring to.
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector); 
    if (sector)
    {
        sector->DisableFog();
    }

    applied = false;
}

bool FogWeatherObject::Valid()
{
    return applied;
}

void FogWeatherObject::SetColor(float r,float g,float b)
{
    color = csColor(r,g,b);
    if(applied)
    {
        CreateMesh(); // Refresh
    }
}

bool FogWeatherObject::CreateMesh()
{
    iSector* sector = psengine->GetEngine()->FindSector(parent->sector);
    if(!sector)
        return false;

    float density = GetDensity(parent->fog_params.value);
    sector->SetFog(density,color);

    applied = true;
    return true;
}

void FogWeatherObject::Update(csTicks /*delta*/)
{
    //    Notify2(LOG_WEATHER,"Updating fog object, Setting fog: %d",parent->fog_params.value);
    SetColor(parent->r / 255.0f, parent->g / 255.0f, parent->b / 255.0f);
    CreateMesh();
}


float FogWeatherObject::GetDensity(int density)
{
    return (float)density / 100000.0f;
}

iSector* FogWeatherObject::GetSector()
{
    return sector;
}

WeatherSound FogWeatherObject::GetWeatherSoundForced()
{
    return WEATHER_SOUND_FOG;
}

WeatherSound FogWeatherObject::GetWeatherSound()
{
    if(Valid())
        return GetWeatherSoundForced();
    else
        return WEATHER_SOUND_CLEAR;
}

void FogWeatherObject::MoveTo(WeatherInfo* new_parent,iSector* to)
{
    Destroy(); // Reset old fog

    parent = new_parent;
    sector = to;

    CreateMesh();
}
