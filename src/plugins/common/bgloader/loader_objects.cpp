/*
 *  loader.cpp - Author: Matthieu Kraus
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include "loader.h"

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
namespace ObjectNames
{
    extern const char texture[8] = "texture";
    extern const char material[9] = "material";
    extern const char trigger[8] = "trigger";
    extern const char sequence[9] = "sequence";
    extern const char meshobj[5] = "mesh";
    extern const char meshfact[13] = "mesh factory";
    extern const char meshgen[8] = "meshgen";
    extern const char light[6] = "light";
    extern const char portal[7] = "portal";
    extern const char sector[7] = "sector";
}

void BgLoader::ShaderVar::LoadObject(csShaderVariable* var)
{
    var->SetType(type);
    switch(type)
    {
        case csShaderVariable::TEXTURE:
        {
            const csRef<Texture>& tex = object->GetDependency(value);
            if(tex.IsValid())
            {
                csRef<iTextureWrapper> wrapper(tex->GetObject());
                var->SetValue(wrapper);
            }
        }
        break;

        case csShaderVariable::FLOAT:
        {
            var->SetValue(vec1);
        }
        break;

        case csShaderVariable::VECTOR2:
        {
            var->SetValue(vec2);
        }
        break;

        case csShaderVariable::VECTOR3:
        {
            var->SetValue(vec3);
        }
        break;

        case csShaderVariable::VECTOR4:
        {
            var->SetValue(vec4);
        }
        break;

        default:
        break;
    }
}

bool BgLoader::Material::LoadObject(bool wait)
{
    bool ready = true;
    ready &= ObjectLoader<Texture>::LoadObjects(wait);

    if(!ready)
    {
        return false;
    }

    iEngine* engine = GetParent()->GetEngine();
    csRef<iMaterial> material (engine->CreateBaseMaterial(0));
    materialWrapper = engine->GetMaterialList()->NewMaterial(material, GetName());

    for(size_t i = 0; i < shaders.GetSize(); i++)
    {
        material->SetShader(shaders[i].type, shaders[i].shader);
    }

    for(size_t i = 0; i < shadervars.GetSize(); i++)
    {
        csShaderVariable* var = material->GetVariableAdd(shadervars[i]->GetID());
        shadervars[i]->LoadObject(var);
    }

    return true;
}

void BgLoader::Material::UnloadObject()
{
    Loadable::CheckRemove<iMaterialWrapper,ObjectNames::material>(materialWrapper);
    ObjectLoader<Texture>::UnloadObjects();
}

bool BgLoader::Light::LoadObject(bool wait)
{
    if(!light.IsValid())
    {
        csRef<iSector> parent = sector->GetObject();
        light = GetParent()->GetEngine()->CreateLight(GetName(), pos, radius, colour, dynamic);
        light->SetAttenuationMode(attenuation);
        light->SetType(type);
	light->GetMovable()->SetSector(parent);
	parent->GetLights()->Add(light);
    }

    return true;
}

void BgLoader::Light::UnloadObject()
{
    Loadable::CheckRemove<iLight,ObjectNames::light>(light);
}

bool BgLoader::MeshFact::LoadObject(bool wait)
{
    bool ready = false;
    bool cached = filename.IsEmpty();

    // we can't get the loader to create a new copy
    // of the factory if caching is enabled, so if
    // we want to clone, we have to do it post-load
    // in that case
    if(cloned && cached)
    {
        if(parentFactory.IsValid())
        {
            ready = parentFactory->Load(wait);
        }
        else
        {
            csString msg;
            msg.Format("trying to load cloned mesh factory %s without a parent factory",GetName());
            CS_ASSERT_MSG(msg.GetData(),false);
            ready = true;
        }
    }
    else
    {
        ready = ObjectLoader<Material>::LoadObjects(wait);
        
        if(ready && !status.IsValid() && !cached)
        {
            // it's not exactly a trivial loadable if we don't cache
            // manually start the load in this case
            status = GetParent()->GetLoader()->LoadMeshObjectFactory(path, filename);
        }

        // rest can be handled by trivial loadable
        if(ready)
        {
            ready = TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>::LoadObject(wait);
        }
    }

    return ready;
}

void BgLoader::MeshFact::FinishObject()
{
    bool cached = !filename.IsEmpty();

    // we have to (try to) manually clone
    // the factory if we loaded from cache
    if(cloned && cached)
    {
        // get source factory
        if(parentFactory.IsValid())
        {
            csRef<iMeshFactoryWrapper> fact = parentFactory->GetObject();
            if(fact.IsValid())
            {
                // get source object factory and clone it
                csRef<iMeshObjectFactory> objectFact = fact->GetMeshObjectFactory();
                csRef<iMeshObjectFactory> newObjectFact = objectFact->Clone();
                if(newObjectFact.IsValid())
                {
                    // set new factory
                    factory = newObjectFact->GetMeshFactoryWrapper();
                }
                else
                {
                    csString msg;
                    msg.Format("mesh factory %s doesn't support cloning - falling back to using original factory",parentFactory->GetName());
                    CS_ASSERT_MSG(msg.GetData(),false);
                }
            }
        }
    }

    if(!trans.IsIdentity())
    {
        // get factory to scale
        csRef<iMeshFactoryWrapper> fact;
        if(cloned && cached)
        {
            if(factory.IsValid())
            {
                fact = factory;
            }
        }
        else
        {
            fact = GetObject();
        }

        if(fact.IsValid())
        {
            // scale factory
            csRef<iMeshObjectFactory> objectFact = fact->GetMeshObjectFactory();
            if(objectFact.IsValid() && objectFact->SupportsHardTransform())
            {
                objectFact->HardTransform(trans);
            }
            else
            {
                fact->SetTransform(trans);
            }
        }
    }
}

void BgLoader::MeshFact::UnloadObject()
{
    TrivialLoadable<iMeshFactoryWrapper,ObjectNames::meshfact>::UnloadObject();
    ObjectLoader<Material>::UnloadObjects();
}

bool BgLoader::MeshObj::LoadObject(bool wait)
{
    bool ready = ObjectLoader<MeshFact>::LoadObjects(wait);
    ready &= ObjectLoader<Material>::LoadObjects(wait);
    ready &= ObjectLoader<Texture>::LoadObjects(wait);

    if(ready)
    {
        ready = TrivialLoadable<iMeshWrapper,ObjectNames::meshobj>::LoadObject(wait);
    }

    return ready;
}

void BgLoader::MeshObj::FinishObject()
{
    BgLoader* loader = GetParent();
    iEngine* engine = loader->GetEngine();
    engine->SyncEngineListsNow(loader->GetLoader()); 

    csRef<iMeshWrapper> meshWrapper = GetObject();
    if(!meshWrapper.IsValid())
    {
        printf("Mesh '%s' failed to load.\n", GetName());
        return;
    }

    // Mark the mesh as being realtime lit depending on graphics setting.
    if(!dynamicLighting)
    {
        meshWrapper->GetFlags().Set(CS_ENTITY_NOLIGHTING);
    }

    // Set world position.
    csRef<iSector> sectorObj = sector->GetObject();
    meshWrapper->GetMovable()->SetSector(sectorObj);
    meshWrapper->GetMovable()->UpdateMove();

    // Init collision data.
    csColliderHelper::InitializeCollisionWrapper(loader->GetCDSys(), meshWrapper);

    // Get the correct path for loading heightmap data.
    iVFS* vfs = loader->GetVFS();
    vfs->PushDir(path);
    engine->PrecacheMesh(meshWrapper);
    vfs->PopDir();
    loader->ReleaseVFS();

    // set shader vars
    for(size_t i = 0; i < shadervars.GetSize(); i++)
    {
        csShaderVariable* var = meshWrapper->GetSVContext()->GetVariableAdd(shadervars[i]->GetID());
        shadervars[i]->LoadObject(var);
    }
}

void BgLoader::MeshObj::UnloadObject()
{
    TrivialLoadable<iMeshWrapper,ObjectNames::meshobj>::UnloadObject();

    ObjectLoader<Texture>::UnloadObjects();
    ObjectLoader<Material>::UnloadObjects();
    ObjectLoader<MeshFact>::UnloadObjects();
}

bool BgLoader::MeshGen::LoadObject(bool wait)
{
    bool ready = ObjectLoader<MeshFact>::LoadObjects(wait);
    ready &= ObjectLoader<Material>::LoadObjects(wait);

    if(ready && !status.IsValid())
    {
      ready = mesh->Load(wait);
    }

    if(ready)
    {
      if(!status.IsValid())
      {
          csRef<iSector> sectorObj = sector->GetObject();
          status = GetParent()->GetLoader()->LoadNode(path, data, 0, sectorObj);
      }

      ready = TrivialLoadable<iMeshGenerator,ObjectNames::meshgen>::LoadObject(wait);
    }

    return ready;
}

void BgLoader::MeshGen::UnloadObject()
{
    csRef<iSector> sectorObj = sector->GetObject();
    sectorObj->RemoveMeshGenerator(GetName());
    status.Invalidate();

    mesh->Unload();
    ObjectLoader<Material>::UnloadObjects();
    ObjectLoader<MeshFact>::UnloadObjects();
}

bool BgLoader::Portal::LoadObject(bool /*wait*/)
{
    if(mObject.IsValid())
    {
        return true;
    }

    csRef<iSector> target = targetSector->GetObject();
    csRef<iSector> source = sector->GetObject();

    if(autoresolve)
    {
        target = 0;
    }

    mObject = GetParent()->GetEngine()->CreatePortal(GetName(), source, csVector3(0),
                     target, poly.GetVertices(), (int)poly.GetVertexCount(), pObject);

    if(renderDist > 0.f)
    {
      mObject->SetMaximumRenderDistance(renderDist);
    }

    // set name on portal object for debugging purposes
    pObject->SetName(GetName());

    if(warp)
    {
        pObject->SetWarp(matrix, wv, ww);
    }

    pObject->GetFlags().SetBool(flags,true);

    if(!target)
    {
        csRef<MissingSectorCallback> cb;
        cb.AttachNew(new MissingSectorCallback(targetSector, autoresolve));
        pObject->SetMissingSectorCallback(cb);
    }

    sector->AddPortal(this);

    return true;
}

void BgLoader::Portal::UnloadObject()
{
    sector->RemovePortal(this);

    CheckRemove<iMeshWrapper,ObjectNames::portal>(mObject);
    pObject = 0;
}

void BgLoader::Sector::FindConnectedSectors(csSet<csPtrKey<Sector> >& connectedSectors)
{
    if(connectedSectors.Contains(this))
    {
        // we have already been checked
        return;
    }

    // add ourself to the list
    connectedSectors.Add(this);

    // Travers into sectors linked to by active portals.
    CS::Threading::RecursiveMutexScopedLock lock(busy);
    csSet<csPtrKey<Portal> >::GlobalIterator it(activePortals.GetIterator());
    while(it.HasNext())
    {
        Portal* portal = it.Next();
        portal->targetSector->FindConnectedSectors(connectedSectors);
    }
}

void BgLoader::Sector::ForceUpdateObjectCount()
{
    objectCount = 0;
    objectCount += ObjectLoader<Portal>::GetObjectCount();
    objectCount += ObjectLoader<MeshGen>::GetObjectCount();
    objectCount += ObjectLoader<MeshObj>::GetObjectCount();
    objectCount += ObjectLoader<Light>::GetObjectCount();
    objectCount += ObjectLoader<Sequence>::GetObjectCount();
    objectCount += ObjectLoader<Trigger>::GetObjectCount();
}

bool BgLoader::Sector::Initialize()
{
    if(!object.IsValid())
    {
        {
            csString msg;
            msg.AppendFmt("Attempting to load uninit sector %s!\n", GetName());
            CS_ASSERT_MSG(msg.GetData(), init);
            if(!init) return false;
        }
        object = GetParent()->GetEngine()->CreateSector(GetName());
        object->SetDynamicAmbientLight(ambient);
        object->SetVisibilityCullerPlugin(culler);
        object->QueryObject()->SetObjectParent(parent);
    }
    return true;
}

bool BgLoader::Sector::LoadObject(bool wait)
{
    if(!Initialize())
    {
        // don't make further attempts to load us
        return true;
    }

    bool ready = true;
    ready &= ObjectLoader<Portal>::LoadObjects(wait);
    ready &= ObjectLoader<Light>::LoadObjects(wait);
    ready &= ObjectLoader<MeshObj>::LoadObjects(wait);
    ready &= ObjectLoader<MeshGen>::LoadObjects(wait);
    ready &= ObjectLoader<Sequence>::LoadObjects(wait);
    ready &= ObjectLoader<Trigger>::LoadObjects(wait);
    ready &= alwaysLoaded.LoadObjects(wait);

    ForceUpdateObjectCount();

    return ready;
}

int BgLoader::Sector::UpdateObjects(const csBox3& loadBox, const csBox3& keepBox, size_t recursions)
{
    if(isLoading || IsChecked())
    {
        return 0;
    }

    int oldObjectCount = objectCount;
    isLoading = true;
    MarkChecked();

    if(IsLoaded())
    {
        // this is a priority sector, so we don't use the load box here
        LoadObject(false);
        return 0;
    }
    else
    {
        // not a priority sector - load/unload normally
        if(!Initialize())
        {
            return 0;
        }

        objectCount += ObjectLoader<Portal>::UpdateObjects(loadBox, keepBox);
        objectCount += ObjectLoader<Light>::UpdateObjects(loadBox, keepBox);
        objectCount += ObjectLoader<MeshObj>::UpdateObjects(loadBox, keepBox);
        objectCount += ObjectLoader<MeshGen>::UpdateObjects(loadBox, keepBox);
        objectCount += ObjectLoader<Sequence>::UpdateObjects(loadBox, keepBox);
        objectCount += ObjectLoader<Trigger>::UpdateObjects(loadBox, keepBox);

        if(objectCount > 0)
        {
            // we have visible objects, load those that are marked always visible
            alwaysLoaded.LoadObjects();
        }
        else
        {
            // no objects visible, unload always loaded objects
            alwaysLoaded.UnloadObjects();
        }
    }

    if(recursions > 0)
    {
        // load target sectors with decreased recursion count
        --recursions;

        // Check other sectors linked to by active portals.
        csSet<csPtrKey<Portal> >::GlobalIterator it(activePortals.GetIterator());
        while(it.HasNext())
        {
            Portal* portal = it.Next();
            csRef<Sector> target = portal->targetSector;

            csBox3 newLoadBox(loadBox);
            csBox3 newKeepBox(keepBox);
            if(portal->warp)
            {
                newLoadBox *= portal->transform;
                newKeepBox *= portal->transform;
            }

            target->UpdateObjects(newLoadBox, newKeepBox, recursions);
        }
    }

    isLoading = false;

    return (int)objectCount - oldObjectCount;
}

void BgLoader::Sector::UnloadObject()
{
    alwaysLoaded.UnloadObjects();
    ObjectLoader<Trigger>::UnloadObjects();
    ObjectLoader<Sequence>::UnloadObjects();
    ObjectLoader<MeshGen>::UnloadObjects();
    ObjectLoader<MeshObj>::UnloadObjects();
    ObjectLoader<Light>::UnloadObjects();
    ObjectLoader<Portal>::UnloadObjects();

    ForceUpdateObjectCount();

    if(objectCount > 0)
    {
        // could not unload all items, some may still be loading
        // note that this shall never occur with blocked loading,
        // because in that case this would mean there's a memleak
        csString msg;
        msg.Format("Error cleaning sectors. Sector is not empty! %zu objects remaining!", objectCount);
        CS_ASSERT_MSG(msg.GetData(),false);
        return;
    }

    CS_ASSERT_MSG("Error cleaning sector. Sector is invalid!", object.IsValid());

    // Remove the sector from the engine.
    Loadable::CheckRemove<iSector,ObjectNames::sector>(object);
}

}
CS_PLUGIN_NAMESPACE_END(bgLoader)

