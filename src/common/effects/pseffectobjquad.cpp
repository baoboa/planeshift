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
#include <iengine/camera.h>
#include <ivideo/material.h>
#include <cstool/csview.h>
#include <imesh/objmodel.h>
#include <csutil/flags.h>
#include <csgeom/tri.h>

#include "effects/pseffectobjquad.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"


psEffectObjQuad::psEffectObjQuad(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    meshControl.AttachNew(new MeshAnimControl(this));
    quadAspect = 1.0f;
}

psEffectObjQuad::~psEffectObjQuad()
{
    if(UseUniqueMeshFact() && meshFact)
        engine->RemoveObject(meshFact);
}

bool psEffectObjQuad::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    // get the attributes
    name.Clear();
    materialName.Clear();
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
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    if(!psEffectObj::Load(node, ldr_context))
        return false;

    return PostSetup();
}

bool psEffectObjQuad::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_quad_";
    effectID += nextUniqueID++;

    if(UseUniqueMeshFact() && !CreateMeshFact())
        return false;

    // create a mesh wrapper from the factory we just created
    mesh = engine->CreateMeshWrapper(meshFact, effectID.GetData());
    if(!mesh)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't create meshwrapper in quad effect\n");
        return false;
    }

    // do the up vector
    objUp = up;
    csReversibleTransform rt;
    rt.LookAt(csVector3(up.x, up.z, up.y), csVector3(0,2,1));
    matUp = rt.GetT2O();
    matBase = matUp;

    // common flags
    mesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    mesh->GetFlags().Set(CS_ENTITY_NODECAL);
    mesh->SetZBufMode(zFunc);
    mesh->SetRenderPriority(priority);

    // disable culling
    csStringID viscull_id = globalStringSet->Request("viscull");
    mesh->GetMeshObject()->GetObjectModel()->SetTriangleData(viscull_id, 0);

    // obj specific
    genState =  scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject());

    if(mixmode != CS_FX_ALPHA)
        mesh->GetMeshObject()->SetMixMode(mixmode);  // to check
    //genState->SetMixMode(mixmode); was not working with latest CS

    genState->SetAnimationControl((iGenMeshAnimationControl*)meshControl);

    // initialize the data we'll pass to the genmesh animation control
    float halfAspect = 0.5f * aspect;
    vert[0].Set(0.5f, 0,  halfAspect);
    vert[1].Set(-0.5f, 0,  halfAspect);
    vert[2].Set(-0.5f, 0, -halfAspect);
    vert[3].Set(0.5f, 0, -halfAspect);

    texel[0].Set(1.0f, 0.0f);
    texel[1].Set(0.0f, 0.0f);
    texel[2].Set(0.0f, 1.0f);
    texel[3].Set(1.0f, 1.0f);

    return true;
}

bool psEffectObjQuad::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    float halfHeightScale = 0.5f;
    if(keyFrames->GetSize() > 0)
    {
        // COLOUR
        float lerpfactor = LERP_FACTOR;
        csVector3 lerpColour = LERP_VEC_KEY(KA_COLOUR,lerpfactor);
        //float lerpAlpha = LERP_KEY(KA_ALPHA);
        CS::ShaderVarStringID varName = stringSet->Request("color modulation");
        csShaderVariable* var = mat->GetMaterial()->GetVariableAdd(varName);
        if(var)
        {
            var->SetValue(lerpColour);
        }

        // HEIGHT
        halfHeightScale *= LERP_KEY(KA_HEIGHT,lerpfactor);
    }

    halfHeightScale *= quadAspect * aspect;
    vert[0].Set(0.5f, 0,  halfHeightScale);
    vert[1].Set(-0.5f, 0,  halfHeightScale);
    vert[2].Set(-0.5f, 0, -halfHeightScale);
    vert[3].Set(0.5f, 0, -halfHeightScale);


    return true;
}

void psEffectObjQuad::CloneBase(psEffectObj* newObj) const
{
    psEffectObj::CloneBase(newObj);

    psEffectObjQuad* newQuadObj = dynamic_cast<psEffectObjQuad*>(newObj);

    // quad specific
    newQuadObj->mat = mat;
}

psEffectObj* psEffectObjQuad::Clone() const
{
    psEffectObjQuad* newObj = new psEffectObjQuad(view, renderer2d);
    CloneBase(newObj);

    return newObj;
}

bool psEffectObjQuad::PostSetup()
{
    if(!UseUniqueMeshFact() && !CreateMeshFact())
        return false;

    animLength = 10;
    if(keyFrames->GetSize() > 0)
        animLength += keyFrames->Get(keyFrames->GetSize()-1)->time;

    return true;
}

bool psEffectObjQuad::CreateMeshFact()
{
    static unsigned int uniqueID = 0;
    csString facName = "effect_quad_fac_";
    facName += uniqueID++;

    meshFact = engine->CreateMeshFactory("crystalspace.mesh.object.genmesh", facName.GetData());
    effectsCollection->Add(meshFact->QueryObject());

    // create the actual sprite3d data
    iMeshObjectFactory* fact = meshFact->GetMeshObjectFactory();
    csRef<iGeneralFactoryState> facState =  scfQueryInterface<iGeneralFactoryState> (fact);
    if(!facState)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't create genmesh: %s\n", name.GetData());
        return false;
    }

    // setup the material
    mat = effectsCollection->FindMaterial(materialName);
    if(mat)
        fact->SetMaterialWrapper(mat);

    facState->SetVertexCount(4);

    // we have to set the vertices so that the quad actually gets rendered
    facState->GetVertices()[0].Set(0.5f, 0,  0.5f);
    facState->GetVertices()[1].Set(-0.5f, 0,  0.5f);
    facState->GetVertices()[2].Set(-0.5f, 0, -0.5f);
    facState->GetVertices()[3].Set(0.5f, 0, -0.5f);

    facState->GetNormals()[0].Set(0,1,0);
    facState->GetNormals()[1].Set(0,1,0);
    facState->GetNormals()[2].Set(0,1,0);
    facState->GetNormals()[3].Set(0,1,0);

    facState->SetTriangleCount(4);

    facState->GetTriangles()[0].Set(0, 2, 1);
    facState->GetTriangles()[1].Set(0, 2, 3);
    facState->GetTriangles()[2].Set(2, 0, 1);
    facState->GetTriangles()[3].Set(2, 0, 3);

    return true;
}

bool psEffectObjQuad::UseUniqueMeshFact() const
{
    // default quad implementation doesn't need unique mesh fact
    return false;
}

const csVector3* psEffectObjQuad::MeshAnimControl::UpdateVertices(csTicks /*current*/, const csVector3* /*verts*/,
        int /*num_verts*/, uint32 /*version_id*/)
{
    return parent->vert;
}

const csVector2* psEffectObjQuad::MeshAnimControl::UpdateTexels(csTicks /*current*/, const csVector2* /*texels*/,
        int /*num_texels*/, uint32 /*version_id*/)
{
    return parent->texel;
}
