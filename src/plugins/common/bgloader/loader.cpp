/*
 *  loader.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008-10 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cssysdef.h>
#include <cstool/collider.h>
#include <cstool/enginetools.h>
#include <cstool/vfsdirchange.h>
#include <csutil/scanstr.h>
#include <csutil/scfstringarray.h>
#include <iengine/camera.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivaria/engseq.h>
#include <ivideo/graph2d.h>
#include <ivideo/material.h>

#include "util/psconst.h"
#include "loader.h"

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
SCF_IMPLEMENT_FACTORY(BgLoader)

BgLoader::BgLoader(iBase *p)
  : scfImplementationType (this, p), loadOffset(0), delayedOffset(0),
    loadRange(500), validPosition(false), loadStep(0), currRot_h(0), currRot_v(0), resetHitbeam(true)
{
}

BgLoader::~BgLoader()
{
}

bool BgLoader::Initialize(iObjectRegistry* object_reg)
{
    maxPortalDepth = 3;
    maxLingerCount = 30;

    this->object_reg = object_reg;
    parserData.object_reg = object_reg;

    engine = csQueryRegistry<iEngine> (object_reg);
    g2d = csQueryRegistryOrLoad<iGraphics2D> (object_reg, "crystalspace.graphics2d.null");
    tloader = csQueryRegistryOrLoad<iThreadedLoader> (object_reg, "crystalspace.level.threadedloader");
    tman = csQueryRegistry<iThreadManager> (object_reg);
    vfs = csQueryRegistry<iVFS> (object_reg);
    parserData.svstrings = csQueryRegistryTagInterface<iShaderVarStringSet>(object_reg, "crystalspace.shader.variablenameset");
    parserData.strings = csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset");
    cdsys = csQueryRegistry<iCollideSystem> (object_reg); 

    parserData.syntaxService = csQueryRegistryOrLoad<iSyntaxService>(object_reg, "crystalspace.syntax.loader.service.text");

    csRef<iConfigManager> config = csQueryRegistry<iConfigManager> (object_reg);
    
    // Check whether we're caching files for performance.    
    parserData.config.cache = config->GetBool("PlaneShift.Loading.Cache", false);

    // Check whether we want to force a specific culler
    csString forceCuller = config->GetStr("PlaneShift.Loading.ForceCuller");
    if(forceCuller.IsEmpty())
    {
        parserData.config.forceCuller = false;
    }
    else
    {
        parserData.config.forceCuller = true;
        // check whether we want to set the default culler as we have to use NULL in that case
        if(forceCuller == "default")
        {
            parserData.config.culler = (const char*)NULL;
        }
        else
        {
            parserData.config.culler = forceCuller;
        }
    }

    // Check whether we only want to load portal data (e.g. for the server) or only meshes (i.e. no lights, etc.)
    parserData.config.portalsOnly = config->GetBool("PlaneShift.Loading.OnlyPortals", false);
    parserData.config.meshesOnly = config->GetBool("PlaneShift.Loading.OnlyMeshes", false);
    parserData.config.meshesOnly |= parserData.config.portalsOnly;

    // Check whether we want to parse textures
    parserData.config.parseShaderVars = config->GetBool("PlaneShift.Loading.ParseShaderVariables", true);

    // Check whether we want to parse shaders
    parserData.config.parseShaders = config->GetBool("PlaneShift.Loading.ParseShaders",true);
    parserData.config.parsedShaders = false;

    // Check whether we want to load shaders threaded.
    parserData.config.blockShaderLoad = !config->GetBool("PlaneShift.Loading.ThreadedShaders",false);

    // Check the level of shader use.
    csString shader(config->GetStr("PlaneShift.Graphics.Shaders"));
    parserData.config.enabledGfxFeatures = 0;
    if(shader.CompareNoCase("Highest"))
    {
      parserData.config.enabledGfxFeatures |= useHighestShaders;
    }
    else if(shader.CompareNoCase("High"))
    {
      parserData.config.enabledGfxFeatures |= useHighShaders;
    }
    else if(shader.CompareNoCase("Medium"))
    {
      parserData.config.enabledGfxFeatures |= useMediumShaders;
    }
    else if(shader.CompareNoCase("Low"))
    {
      parserData.config.enabledGfxFeatures |= useLowShaders;
    }
    else if(shader.CompareNoCase("Lowest"))
    {
      parserData.config.enabledGfxFeatures |= useLowestShaders;
    }

    // Check if we're using real time shadows.
    if(config->GetBool("PlaneShift.Graphics.Shadows"))
    {
      parserData.config.enabledGfxFeatures |= useShadows;
    }

    // Check if we're using meshgen.
    if(config->GetBool("PlaneShift.Graphics.EnableGrass", true))
    {
      parserData.config.enabledGfxFeatures |= useMeshGen;
    }

    // initialize token list required by the parser
    InitTokenTable(parserData.xmltokens);

    return true;
}

bool BgLoader::LoadZones(iStringArray* regions, bool priority)
{
    // Firstly, get a list of all zones that should be loaded.
    csSet<csPtrKey<Zone> > newLoadedZones;
    if(!regions)
    {
        // if no regions were passed, assume we want to load all
        CS::Threading::ScopedReadLock lock(parserData.zones.lock);
        csHash<csRef<Zone>, csStringID>::GlobalIterator it(parserData.zones.hash.GetIterator());
        while(it.HasNext())
        {
            newLoadedZones.Add(csPtrKey<Zone>(it.Next()));
        }
    }
    else
    {
        for(size_t i = 0; i < regions->GetSize(); ++i)
        {
            csRef<Zone> zone = parserData.zones.Get(regions->Get(i));

            if(zone.IsValid())
            {
                newLoadedZones.Add(csPtrKey<Zone>(zone));
            }
            else
            {
                return false;
            }
        }
    }

    // now go through all new zones adding them as dependency and updating their priority if needed.
    csSet<csPtrKey<Zone> >::GlobalIterator it(newLoadedZones.GetIterator());
    while(it.HasNext())
    {
        csRef<Zone> zone(it.Next());
        if(priority)
        {
            zone->UpdatePriority(true);
        }
        loadedZones.AddDependency(zone);
    }

    // Next check which zones shouldn't be loaded and which should get a priority update.
    csRefArray<Zone> toDelete;
    ObjectLoader<Zone>::HashType::GlobalIterator zoneIt(loadedZones.GetDependencies().GetIterator());
    while(zoneIt.HasNext())
    {
        csRef<Zone> zone(zoneIt.Next().obj);
        if(!newLoadedZones.Contains(csPtrKey<Zone>(zone)))
        {
            if(priority)
            {
                // mark zone as non-priority one, so it'll be removed
                // by the next update of non-priority zones if applicable
                zone->UpdatePriority(false);
            }
            else if(!zone->GetPriority())
            {
                // this is not a priority zone, unload it
                toDelete.Push(zone);
            }
        }
        else if(priority)
        {
            zone->UpdatePriority(true);
        }
    }

    // now clean those zones that shouldn't be loaded.
    for(size_t i = 0; i < toDelete.GetSize(); i++)
    {
        loadedZones.RemoveDependency(toDelete[i]);
    }

    // now start loading the new zones.
    loadedZones.LoadObjects(false);

    return true;
}

bool BgLoader::LoadPriorityZones(iStringArray* regions)
{
    return LoadZones(regions, true);
}


csPtr<iStringArray> BgLoader::GetShaderName(const char* usageType)
{
    csRef<iStringArray> t;
    t.AttachNew(new scfStringArray());
    csArray<csString> all;
    {
        CS::Threading::ScopedReadLock lock(parserData.shaderLock);
        csStringID shaderID = parserData.strings->Request(usageType);
        all = parserData.shadersByUsage.GetAll(shaderID);
    }

    for(size_t i = 0; i < all.GetSize(); ++i)
    {
        t->Push(all[i]);
    }

    return csPtr<iStringArray>(t);
}

void BgLoader::ContinueLoading(bool wait)
{
    bool expensiveChecks = (loadStep == 0) || wait;

    if(expensiveChecks)
    {
        // checks objects passed via LoadZones
        loadedZones.LoadObjects(wait);

        
        if(validPosition)
        {
            // checks regular objects
            UpdatePosition(lastPos, lastSector->GetName(), true);
        }
    }

    // continue loading delayed loaders
    CS::Threading::RecursiveMutexScopedLock lock(loadLock);
    for(delayedOffset = 0; delayedOffset < delayedLoadList.GetSize(); ++delayedOffset)
    {
        delayedLoadList[delayedOffset]->ContinueLoading(wait);
    }

    // find lingering objects that someone attempted to load but never finished
    for(loadOffset = 0; loadOffset < loadList.GetSize(); ++loadOffset)
    {
        Loadable* l = loadList[loadOffset];
        if(!l->IsChecked())
        {
            if(l->GetLingerCount() > maxLingerCount)
            {
                LOADER_DEBUG_MESSAGE("aborting load for lingering object '%s'!\n", l->GetName());
                l->AbortLoad();
            }
            else
            {
                l->IncLingerCount();
            }
        }
        else
        {
            l->ResetChecked();

            // continue loading
            l->LoadObject(wait);
        }
    }

    loadStep = (loadStep + 1) % 10;
}

void BgLoader::UpdatePosition(const csVector3& pos, const char* sectorName, bool force)
{
    csString name(sectorName);

    // Check if we've moved.
    if(!force && csVector3(lastPos - pos).Norm() < loadRange/10
              && (!lastSector.IsValid() || name == lastSector->GetName()))
    {
        ContinueLoading(false);
        return;
    }

    csRef<Sector> sector;
    // Hack to work around the weird sector stuff we do.
    /*if(csString("SectorWhereWeKeepEntitiesResidingInUnloadedMaps").Compare(sectorName))
    {
        sector = lastSector;
    }*/

    if(!sector.IsValid())
    {
        sector = parserData.sectors.Get(name.GetDataSafe());
    }

    if(sector.IsValid())
    {
        validPosition = true;
        lastPos = pos;
        lastSector = sector;

        // Calc load and keep boxes.
        csBox3 loadBox(pos);
        loadBox.SetSize(2*loadRange);

        csBox3 keepBox(pos);
        keepBox.SetSize(2*1.5*loadRange);

        // clean the check flag for all sectors, so they'll be checked, again
        {
            CS::Threading::ScopedReadLock lock(parserData.sectors.lock);
            LockedType<Sector>::HashType::GlobalIterator it(parserData.sectors.hash.GetIterator());
            while(it.HasNext())
            {
                it.Next()->ResetChecked();
            }
        }

        // Check.
        sector->UpdateObjects(loadBox, keepBox, maxPortalDepth);

        // check for no longer reachable sectors
        if(force)
        {
            CleanDisconnectedSectors(sector);
        }
    }
}

void BgLoader::CleanDisconnectedSectors(Sector* sector)
{
    // Create a list of connectedSectors;
    csSet<csPtrKey<Sector> > connectedSectors;
    sector->FindConnectedSectors(connectedSectors);

    // Check for disconnected sectors.
    csBox3 unloadAll; // uninit bboxes don't overlap with anything

    // clean sectors that aren't (indirectly) connected to this one
    CS::Threading::ScopedReadLock lock(parserData.sectors.lock);
    LockedType<Sector>::HashType::GlobalIterator it(parserData.sectors.hash.GetIterator());
    while(it.HasNext())
    {
        csRef<Sector> sector(it.Next());
        csRef<iSector> sectorObj = sector->GetObject();
        if(sectorObj.IsValid() && !connectedSectors.Contains(csPtrKey<Sector>(sector)))
        {
            // unload everything, but don't traverse into linked sectors
            sector->UpdateObjects(unloadAll, unloadAll, 0);
        }
    }
}

bool BgLoader::InWaterArea(const char* sectorName, csVector3* pos, csColor4** colour)
{
    // Hack to work around the weird sector stuff we do.
    /*if(!strcmp("SectorWhereWeKeepEntitiesResidingInUnloadedMaps", sectorName))
        return false;*/

    /*csRef<Sector> sector;
    {
        CS::Threading::ScopedReadLock lock(parserData.sectors.lock);
        csStringID sectorID = parserData.sectors.stringSet.Request(sectorName);
        sector = parserData.sectors.hash.Get(sectorID, csRef<Sector>());
    }

    CS_ASSERT_MSG("Invalid sector passed to InWaterArea().", sector.IsValid());

    for(size_t i = 0; i < sector->waterareas.GetSize(); ++i)
    {
        if(sector->waterareas[i].bbox.In(*pos))
        {
            *colour = &sector->waterareas[i].colour;
            return true;
        }
    }*/

    return false;
}

csPtr<iThreadReturn> BgLoader::LoadFactory(const char* name, bool wait)
{
    csRef<MeshFact> meshfact;
    {
        CS::Threading::ScopedReadLock lock(parserData.factories.lock);
        csStringID factoryID = parserData.factories.stringSet.Request(name);
        meshfact = parserData.factories.hash.Get(factoryID, csRef<MeshFact>());
    }

    if(meshfact.IsValid())
    {
        // create delayed loader
        csRef<iDelayedLoader> result;
        result.AttachNew(new DelayedLoader<MeshFact>(meshfact));

        // start loading now
        result->ContinueLoading(wait);

        return csPtr<iThreadReturn>(result);
    }
    else
    {
        return csPtr<iThreadReturn>(0);
    }
}

void BgLoader::CloneFactory(const char* name, const char* newName, bool* failed, const csReversibleTransform& trans)
{
    // Find meshfact to clone.
    csRef<MeshFact> meshfact;
    {
        CS::Threading::ScopedReadLock lock(parserData.factories.lock);
        csStringID factoryID = parserData.factories.stringSet.Request(name);
        meshfact = parserData.factories.hash.Get(factoryID, csRef<MeshFact>());
    }

    if(!meshfact.IsValid())
    {
        // Validation.
        csString msg;
        msg.Format("Invalid factory reference '%s' passed for cloning.", name);
        CS_ASSERT_MSG(msg.GetData(), false);

        if(failed)
        {
            *failed = true;
        }
    }

    // check whether newName already exists
    csRef<MeshFact> newMeshFact = parserData.factories.Get(newName);

    if(newMeshFact.IsValid() && !(*meshfact == *newMeshFact))
    {
        // validation
        csString msg;
        msg.Format("Cloning factory '%s' to '%s' that already exists and doesn't match.", name, newName);
        CS_ASSERT_MSG(msg.GetData(), false);

        if(failed)
        {
            *failed = true;
        }
        return;
    }

    // Create a clone.
    if(!newMeshFact.IsValid())
    {
        newMeshFact = meshfact->Clone(newName);
        newMeshFact->SetTransform(trans);
        parserData.factories.Put(newMeshFact);
    }
}

csPtr<iThreadReturn> BgLoader::LoadMaterial(const char* name, bool wait)
{
    csRef<Material> material;
    {
        CS::Threading::ScopedReadLock lock(parserData.materials.lock);
        csStringID materialID = parserData.materials.stringSet.Request(name);
        material = parserData.materials.hash.Get(materialID, csRef<Material>());
    }

    if(material.IsValid())
    {
        // create delayed loader
        csRef<iDelayedLoader> result;
        result.AttachNew(new DelayedLoader<Material>(material));

        // start loading now
        result->ContinueLoading(wait);

        return csPtr<iThreadReturn>(result);
    }
    else
    {
        return csPtr<iThreadReturn>(0);
    }
}

}
CS_PLUGIN_NAMESPACE_END(bgLoader)

