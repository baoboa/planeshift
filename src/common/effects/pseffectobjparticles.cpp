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
#include <iengine/sector.h>
#include <imap/ldrctxt.h>
#include <imap/loader.h>
#include <imesh/particles.h>
#include <imesh/partsys.h>
#include <imesh/objmodel.h>
#include <csutil/flags.h>

#include "effects/pseffectobjparticles.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/pscssetup.h"
#include "util/log.h"

psEffectObjParticles::psEffectObjParticles(iView* parentView, psEffect2DRenderer* renderer2d)
    :psEffectObj(parentView, renderer2d)
{
}

psEffectObjParticles::~psEffectObjParticles()
{
    //if (pstate)
    //    pstate->Stop();
}

bool psEffectObjParticles::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{

    // get the attributes
    name.Clear();
    materialName.Clear();
    factName.Clear();
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
        else if(attrName == "material")
            materialName = attr->GetValue();
        else if(attrName == "fact")
            factName = attr->GetValue();

    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    if(!psEffectObj::Load(node, ldr_context))
        return false;

    return PostSetup(ldr_context);
}

bool psEffectObjParticles::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_particles_";
    effectID += nextUniqueID++;

    mesh = engine->CreateMeshWrapper(meshFact, effectID);

    // mesh has been loaded before hand
    if(!mesh)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't load desired mesh!\n");
        return false;
    }

    // do the up vector
    objUp = up;
    csReversibleTransform rt;
    rt.LookAt(up, csVector3(1,2,0));
    matUp = rt.GetT2O();
    matBase = matUp;

    // common flags
    mesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    mesh->SetZBufMode(zFunc);
    mesh->SetRenderPriority(priority);
    mesh->GetMeshObject()->SetMixMode(mixmode);

    // disable culling
    csStringID viscull_id = globalStringSet->Request("viscull");
    mesh->GetMeshObject()->GetObjectModel()->SetTriangleData(viscull_id, 0);

    // obj specific
    //pstate =  scfQueryInterface<iParticlesObjectState> (mesh->GetMeshObject());
    //if (!pstate)
    //{
    //    csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Effect obj is of type particles, but the mesh is not!\n");
    //    return false;
    //}

    // add the custom material if set
    if(!materialName.IsEmpty())
    {
        csRef<iMaterialWrapper> mat = effectsCollection->FindMaterial(materialName);
        if(mat != 0)
            mesh->GetMeshObject()->SetMaterialWrapper(mat);
    }

    //if (mixmode != CS_FX_ALPHA)
    //pstate->SetMixMode(mixmode);
    //pstate->Start();

    isAnimating = true;
    return true;
}

bool psEffectObjParticles::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    if(keyFrames->GetSize() == 0)
        return true;

    // do stuff here
    if(keyFrames->Get(currKeyFrame)->actions[psEffectObjKeyFrame::KA_ANIMATE] < 0 && isAnimating)
    {
        //pstate->Stop();
        isAnimating = false;
    }
    else if(keyFrames->Get(currKeyFrame)->actions[psEffectObjKeyFrame::KA_ANIMATE] > 0 && !isAnimating)
    {
        //pstate->Start();
        isAnimating = true;
    }

    return true;
}

psEffectObj* psEffectObjParticles::Clone() const
{
    psEffectObjParticles* newObj = new psEffectObjParticles(view, renderer2d);
    CloneBase(newObj);

    // simp mesh specific
    newObj->factName = factName;
    newObj->isAnimating = isAnimating;

    return newObj;
}

bool psEffectObjParticles::PostSetup(iLoaderContext* ldr_context)
{
    static unsigned int uniqueID = 0;
    csString facName = "effect_particles_fac_";
    facName += uniqueID++;

    meshFact = ldr_context->FindMeshFactory(factName);
    if(!meshFact)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                 "Could not find factory: %s\n", factName.GetData());
        return false;
    }

    return true;
}
