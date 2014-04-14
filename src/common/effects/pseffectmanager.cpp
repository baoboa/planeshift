/*
* Author: Andrew Robberts
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <imesh/object.h>
#include <imesh/sprite3d.h>
#include <csutil/syspath.h>

#include "pseffectmanager.h"
#include "pseffect.h"
#include "pseffectlight.h"
#include "pseffectobj.h"

// Bad, should not reference src/client!
#include "../client/pscelclient.h"
#include "util/log.h"
#include "util/pscssetup.h"

//#define DONT_DO_EFFECTS


//---------------------------------------------------------------------------------

psEffectLoader::psEffectLoader()
    : scfImplementationType(this), manager(0)
{
}

void psEffectLoader::SetManager(psEffectManager* manager)
{
    this->manager = manager;
}

csPtr<iBase> psEffectLoader::Parse(iDocumentNode* node, iStreamSource* /*istream*/, iLoaderContext* ldr_context,
                                   iBase* context)
{
#ifndef DONT_DO_EFFECTS
    if(manager)
    {
        csRef<iDocumentNodeIterator> xmlbinds;

        xmlbinds = node->GetNodes("effect");
        csRef<iDocumentNode> effectNode;
        while(xmlbinds->HasNext())
        {
            effectNode = xmlbinds->Next();

            psEffect* newEffect = new psEffect();
            newEffect->Load(effectNode, manager->GetView(), manager->Get2DRenderer(), ldr_context);

            parseLock.UpgradeLock();
            if(manager->FindEffect(newEffect->GetName()))
            {
                parseLock.UpgradeUnlock();
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Duplicate effect '%s' found!", newEffect->GetName().GetData());
                delete newEffect;
            }
            else
            {
                parseLock.UpgradeUnlockAndWriteLock();
                manager->AddEffect((const char*)newEffect->GetName(), newEffect);
                parseLock.WriteUnlock();
            }
        }
    }
#endif
    context->IncRef();
    return context;
}

//---------------------------------------------------------------------------------

psEffectManager::psEffectManager(iObjectRegistry* objReg) : object_reg(objReg)
{
    vc =  csQueryRegistry<iVirtualClock> (psCSSetup::object_reg);

    effectLoader.AttachNew(new psEffectLoader());
    effectLoader->SetManager(this);
    psCSSetup::object_reg->Register((psEffectLoader*)effectLoader, "PSEffects");

    effect2DRenderer = new psEffect2DRenderer();

#ifndef DONT_DO_EFFECTS
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (psCSSetup::object_reg);
    effectsCollection = engine->CreateCollection("effects");
#endif
}

psEffectManager::~psEffectManager()
{
#ifndef DONT_DO_EFFECTS

    csHash<psEffect*, csString>::GlobalIterator it = effectFactories.GetIterator();
    while(it.HasNext())
    {
        psEffect* tmpEffect = it.Next();
        delete tmpEffect;
    }
    effectFactories.DeleteAll();

    csHash<psEffect*>::GlobalIterator itActual = actualEffects.GetIterator();
    while(itActual.HasNext())
    {
        psEffect* tmpEffect = itActual.Next();
        delete tmpEffect;
    }
    actualEffects.DeleteAll();
    effectsCollection->ReleaseAllObjects();

    csArray<psLight*> lights = lightList.GetAll();
    while(lights.GetSize())
    {
        delete lights.Pop();
    }

    lightList.DeleteAll();
#endif
    effectLoader->SetManager(NULL);
    psCSSetup::object_reg->Unregister((psEffectLoader*)effectLoader, "PSEffects");

    delete effect2DRenderer;
}

csPtr<iThreadReturn> psEffectManager::LoadEffects(const csString &fileName, iView* parentView)
{
#ifndef DONT_DO_EFFECTS
    view = parentView;

    csRef<iThreadedLoader> loader = csQueryRegistry<iThreadedLoader> (psCSSetup::object_reg);
    csRef<iVFS> vfs = csQueryRegistry<iVFS> (psCSSetup::object_reg);
    return csPtr<iThreadReturn>(loader->LoadLibraryFile(vfs->GetCwd(), fileName, effectsCollection, 0, 0, KEEP_ALL, true));
#endif
}

bool psEffectManager::LoadFromEffectsList(const csString &fileName, iView* parentView)
{
#ifndef DONT_DO_EFFECTS
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psCSSetup::object_reg);
    assert(vfs);

    if(!vfs->Exists(fileName))
        return false;

    csRef<iDocument> doc;
    csRef<iDocumentNodeIterator> xmlbinds;
    csRef<iDocumentSystem>  xml;
    const char* error;

    csRef<iDataBuffer> buff = vfs->ReadFile(fileName);
    if(buff == 0)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not find file: %s", fileName.GetData());
        return false;
    }
    xml =  csQueryRegistry<iDocumentSystem> (psCSSetup::object_reg);
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse(buff);
    if(error)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                 "Parse error in %s: %s", fileName.GetData(), error);
        return false;
    }
    if(doc == 0)
        return false;

    csRef<iDocumentNode> root = doc->GetRoot();
    if(root == 0)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "No root in XML");
        return false;
    }

    csRefArray<iThreadReturn> threadReturns;
    csRef<iDocumentNode> listNode = root->GetNode("effectsFileList");
    if(listNode != 0)
    {
        xmlbinds = listNode->GetNodes("effectsFile");
        csRef<iDocumentNode> fileNode;
        while(xmlbinds->HasNext())
        {
            fileNode = xmlbinds->Next();

            csString effectFile = fileNode->GetAttributeValue("file");
            if(vfs->Exists(effectFile))
            {
                csRef<iThreadReturn> eff = LoadEffects(effectFile, parentView);
                threadReturns.Push(eff);
            }
        }
    }
    csRef<iThreadManager> threadman = csQueryRegistry<iThreadManager>(object_reg);
    return threadman->Wait(threadReturns);
#endif
    return true;
}

bool psEffectManager::LoadFromDirectory(const csString &path, bool includeSubDirs, iView* parentView)
{
#ifndef DONT_DO_EFFECTS
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psCSSetup::object_reg);
    assert(vfs);

    if(!vfs->Exists(path))
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Failed to load effects from directory; %s does not exist", path.GetData());
        return false;
    }

    if(!vfs->ChDir(path))
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Failed to load effects from directory; %s read failed", path.GetData());
        return false;
    }

    csRefArray<iThreadReturn> threadReturns;

    csRef<iStringArray> files = vfs->FindFiles("*");
    for(size_t a=0; a<files->GetSize(); ++a)
    {
        const char* file = files->Get(a);
        if(file[0] == '.')
            continue;

        if(strcmp(file, path) == 0)
            continue;

        size_t len = strlen(file);
        char lastChar = file[len-1];
        if(lastChar == '/' || lastChar == '\\')
        {
            if(includeSubDirs)
            {
                if(!LoadFromDirectory(file, true, parentView))
                {
                    return false;
                }
            }
        }
        else if(len > 4 && ((lastChar == 'f' || lastChar == 'F') && (file[len-2] == 'f' || file[len-2] == 'F') && (file[len-3] == 'e' || file[len-3] == 'E') && file[len-4] == '.'))
        {
            csRef<iThreadReturn> eff = LoadEffects(file, parentView);
            threadReturns.Push(eff);
        }
    }
#endif
    csRef<iThreadManager> threadman = csQueryRegistry<iThreadManager>(object_reg);
    return threadman->Wait(threadReturns);
}

bool psEffectManager::DeleteEffect(unsigned int effectID)
{
#ifndef DONT_DO_EFFECTS
    if(effectID == 0)
        return true;

    csArray<psEffect*> effects;

    effects = actualEffects.GetAll(effectID);
    while(effects.GetSize())
    {
        delete effects.Pop();
    }

    actualEffects.DeleteAll(effectID);
#endif
    return true;
}

unsigned int psEffectManager::RenderEffect(const csString &effectName, const csVector3 &offset,
        iMeshWrapper* attachPos, iMeshWrapper* attachTarget, const csVector3 &up,
        const unsigned int uniqueIDOverride, bool rotateWithMesh, const float* scale)
{
#ifndef DONT_DO_EFFECTS
    if(!attachPos)
        return 0;

    // check if it's a single effect
    psEffect* currEffect = FindEffect(effectName);
    if(currEffect != 0)
    {
        currEffect = currEffect->Clone();
        if(scale != NULL)
        {
            if(!currEffect->SetFrameParamScalings(scale))
            {
                Error2("Received scale factor for effect %s that don't use param scaling",effectName.GetDataSafe());
            }
        }

        const unsigned int uniqueID = currEffect->Render(attachPos->GetMovable()->GetSectors(), offset, attachPos,
                                      attachTarget, up.Unit(), uniqueIDOverride, rotateWithMesh);

        actualEffects.Put(uniqueID, currEffect);
        return uniqueID;
    }
#endif
    return 0;
}

unsigned int psEffectManager::RenderEffect(const csString &effectName, iSector* sector, const csVector3 &pos,
        iMeshWrapper* attachTarget, const csVector3 &up,
        const unsigned int uniqueIDOverride, const float* scale)
{
#ifndef DONT_DO_EFFECTS
    // check if it's a single effect
    psEffect* currEffect = FindEffect(effectName);
    if(currEffect != 0)
    {
        currEffect = currEffect->Clone();
        if(scale != NULL)
        {
            if(!currEffect->SetFrameParamScalings(scale))
            {
                Error2("Received scale factor for effect %s that don't use param scaling",effectName.GetDataSafe());
            }
        }

        unsigned int uniqueID = currEffect->Render(sector, pos, 0, attachTarget, up.Unit(), uniqueIDOverride);
        actualEffects.Put(uniqueID, currEffect);
        return uniqueID;
    }
    Error2("Failed to find effect with name: %s",effectName.GetDataSafe());
#endif
    return 0;
}

unsigned int psEffectManager::RenderEffect(const csString &effectName, iSectorList* sectors, const csVector3 &pos,
        iMeshWrapper* attachTarget, const csVector3 &up,
        const unsigned int uniqueIDOverride, const float* scale)
{
#ifndef DONT_DO_EFFECTS
    // check if it's a single effect
    psEffect* currEffect = FindEffect(effectName);
    if(currEffect != 0)
    {
        currEffect = currEffect->Clone();

        if(scale != NULL)
        {
            if(!currEffect->SetFrameParamScalings(scale))
            {
                Error2("Received scale factor for effect %s that don't use param scaling",effectName.GetDataSafe());
            }
        }

        const unsigned int uniqueID = currEffect->Render(sectors, pos, 0, attachTarget, up.Unit(), uniqueIDOverride);
        actualEffects.Put(uniqueID, currEffect);
        return uniqueID;
    }
#endif
    return 0;

}

unsigned int psEffectManager::AttachLight(const char* name, const csVector3 &pos,
        float radius, const csColor &colour, iMeshWrapper* mw)
{
    psLight* pslight = new psLight(object_reg);
    unsigned int uniqueID = pslight->AttachLight(name, pos, radius, colour, mw);
    lightList.Put(uniqueID, pslight);
    return uniqueID;
}

void psEffectManager::DetachLight(unsigned int lightID)
{
    if(lightID == 0)
        return;

    csArray<psLight*> lights;

    lights = lightList.GetAll(lightID);
    while(lights.GetSize())
    {
        delete lights.Pop();
    }

    lightList.DeleteAll(lightID);
}

void psEffectManager::Update(csTicks elapsed)
{
#ifndef DONT_DO_EFFECTS

    if(elapsed == 0)
        elapsed = vc->GetElapsedTicks();

    csHash<psEffect*, unsigned int>::GlobalIterator it = actualEffects.GetIterator();
    csList<unsigned int> ids_to_delete;
    csList<psEffect*> effects_to_delete;
    while(it.HasNext())
    {
        unsigned int id = 0;
        psEffect* effect = it.Next(id);

        if(!effect)
            continue;

        // update the effect itself
        if(!effect->Update(elapsed))
        {
            ids_to_delete.PushBack(id);
            effects_to_delete.PushBack(effect);
        }
    }

    while(!effects_to_delete.IsEmpty())
    {
        psEffect* effect = effects_to_delete.Front();
        actualEffects.Delete(ids_to_delete.Front(),effect);
        delete effect;
        effects_to_delete.PopFront();
        ids_to_delete.PopFront();
    }

    csList<psLight*> lights_to_delete;
    csHash<psLight*, unsigned int>::GlobalIterator itr = lightList.GetIterator();
    while(itr.HasNext())
    {
        unsigned int id = 0;
        psLight* light = itr.Next(id);

        if(!light)
            continue;

        // update the effect itself
        if(!light->Update())
        {
            ids_to_delete.PushBack(id);
            lights_to_delete.PushBack(light);
        }
    }

    while(!lights_to_delete.IsEmpty())
    {
        psLight* light = lights_to_delete.Front();
        lightList.Delete(ids_to_delete.Front(),light);
        delete light;
        lights_to_delete.PopFront();
        ids_to_delete.PopFront();
    }
#endif
}

void psEffectManager::Clear()
{
#ifndef DONT_DO_EFFECTS
    csHash<psEffect*, csString>::GlobalIterator it = effectFactories.GetIterator();
    while(it.HasNext())
        delete it.Next();
    effectFactories.DeleteAll();

    csHash<psEffect*>::GlobalIterator itActual = actualEffects.GetIterator();
    while(itActual.HasNext())
        delete itActual.Next();
    actualEffects.DeleteAll();

    effectsCollection->ReleaseAllObjects();

#endif
}

psEffect* psEffectManager::FindEffect(unsigned int ID) const
{
#ifndef DONT_DO_EFFECTS
    if(ID == 0)
        return 0;

    return actualEffects.Get(ID, 0);
#endif
    return 0;
}

psEffect* psEffectManager::FindEffect(const csString &name) const
{
#ifndef DONT_DO_EFFECTS
    return effectFactories.Get((const char*)name, 0);
#endif
    return 0;
}

csHash<psEffect*, csString>::GlobalIterator psEffectManager::GetEffectsIterator()
{
    return effectFactories.GetIterator();
}

void psEffectManager::ShowEffect(unsigned int id,bool value)
{
    psEffect* eff = FindEffect(id);
    if(!eff)
        return;

    // No change?
    if(value == eff->IsVisible())
        return;

    if(value)
        eff->Show();
    else
        eff->Hide();
}

void psEffectManager::AddEffect(const char* name, psEffect* effect)
{
    effectFactories.Put(name, effect);
}

void psEffectManager::Render2D(iGraphics3D* g3d, iGraphics2D* g2d)
{
    effect2DRenderer->Render(g3d, g2d);
}
