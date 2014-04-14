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
#include <imesh/objmodel.h>
#include <csgeom/tri.h>
#include <csutil/cscolor.h>
#include <csutil/flags.h>

#include "effects/pseffectobjstar.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"

psEffectObjStar::psEffectObjStar(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    rays = 0;
    perp = 0;

    vert = 0;
    colour = 0;
    meshControl.AttachNew(new MeshAnimControl(this));
}

psEffectObjStar::~psEffectObjStar()
{
    delete [] rays;
    delete [] perp;

    delete [] vert;
    delete [] colour;
}

bool psEffectObjStar::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    // get the attributes
    name.Clear();
    materialName.Clear();
    segments = 10;
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
        else if(attrName == "segments")
            segments = attr->GetValueAsInt();
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }
    if(materialName.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj without a material.\n");
        return false;
    }

    if(!psEffectObj::Load(node, ldr_context))
        return false;

    return PostSetup();
}

bool psEffectObjStar::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_star_";
    effectID += nextUniqueID++;

    // create a mesh wrapper from the factory we just created
    mesh = engine->CreateMeshWrapper(meshFact, effectID.GetData());

    // do the up vector
    objUp = up;
    csReversibleTransform rt;
    rt.LookAt(up, csVector3(1,2,0));
    //mesh->HardTransform(rt);
    matBase = rt.GetT2O();
    matUp = matBase;

    // common flags
    mesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    mesh->SetZBufMode(zFunc);
    mesh->SetRenderPriority(priority);

    // disable culling
    csStringID viscull_id = globalStringSet->Request("viscull");
    mesh->GetMeshObject()->GetObjectModel()->SetTriangleData(viscull_id, 0);

    // obj specific
    genState =  scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject());

    mesh->GetMeshObject()->SetColor(csColor(1,1,1)); // to check
    //genState->SetColor(csColor(1,1,1)); was not working with latest CS

    if(mixmode != CS_FX_ALPHA)
        mesh->GetMeshObject()->SetMixMode(mixmode);  // to check
    //genState->SetMixMode(mixmode); was not working with latest CS

    genState->SetAnimationControl((iGenMeshAnimationControl*)meshControl);

    GenerateRays();
    vert = new csVector3[segments*3];
    colour = new csColor4[segments*3];

    return true;
}

bool psEffectObjStar::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    float topScale = 1.0f;
    float height = 1.0f;

    csVector3 lerpColour = csVector3(1,1,1);
    float lerpAlpha = 1;
    if(keyFrames->GetSize() > 0)
    {
        // COLOUR
        float lerpfactor = LERP_FACTOR;
        lerpColour = LERP_VEC_KEY(KA_COLOUR,lerpfactor);
        lerpAlpha = LERP_KEY(KA_ALPHA,lerpfactor);

        topScale = LERP_KEY(KA_TOPSCALE,lerpfactor);
        height = LERP_KEY(KA_HEIGHT,lerpfactor);
    }

    for(int b=0; b<segments; ++b)
    {
        // vertex data
        csVector3 ray = rays[b] * height;
        vert[b*3  ].Set(0, 0, 0);
        vert[b*3+1].Set(perp[b] * -topScale + ray);
        vert[b*3+2].Set(perp[b] *  topScale + ray);

        colour[b*3  ].Set(lerpColour.x, lerpColour.y, lerpColour.z, lerpAlpha);
        colour[b*3+1].Set(lerpColour.x, lerpColour.y, lerpColour.z, lerpAlpha);
        colour[b*3+2].Set(lerpColour.x, lerpColour.y, lerpColour.z, lerpAlpha);
    }

    return true;
}

psEffectObj* psEffectObjStar::Clone() const
{
    psEffectObjStar* newObj = new psEffectObjStar(view, renderer2d);
    CloneBase(newObj);

    // star specific
    newObj->segments = segments;

    return newObj;
}

void psEffectObjStar::GenerateRays()
{
    delete [] rays;
    delete [] perp;

    // ray is the direction of the segment, perp is like the rotation
    rays = new csVector3[segments];
    perp = new csVector3[segments];

    for(int b=0; b<segments; ++b)
    {
        // generate a random unit vector for the direction of this segment
        rays[b].z = -1.0f + 2.0f*(float)rand() / (float)RAND_MAX;
        float a = 6.2831853*rand() / (float)RAND_MAX;
        float c = (float)sqrt(1.0f - rays[b].z*rays[b].z);

        rays[b].x = c * cosf(a);
        rays[b].y = c * sinf(a);

        // generate a second random unit vector
        // this one doesn't have to be perfectly uniform
        csVector3 rperp = csVector3(rand() / (float)RAND_MAX,
                                    rand() / (float)RAND_MAX,
                                    rand() / (float)RAND_MAX);
        perp[b] = rperp % rays[b];
        perp[b].Normalize();
        perp[b] *= 0.5f;
    }
}

bool psEffectObjStar::PostSetup()
{
    static unsigned int uniqueID = 0;
    csString facName = "effect_star_fac_";
    facName += uniqueID++;
    meshFact = engine->CreateMeshFactory("crystalspace.mesh.object.genmesh", facName.GetData());
    effectsCollection->Add(meshFact->QueryObject());

    // create the actual sprite3d data
    iMeshObjectFactory* fact = meshFact->GetMeshObjectFactory();
    csRef<iGeneralFactoryState> facState =  scfQueryInterface<iGeneralFactoryState> (fact);

    // setup the material
    csRef<iMaterialWrapper> mat = effectsCollection->FindMaterial(materialName);
    if(!mat)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't find effect material: %s\n", materialName.GetData());
        return false;
    }
    fact->SetMaterialWrapper(mat);

    GenerateRays();

    float topScale = 1.0f;
    //float height = 1.0f;

    facState->SetVertexCount(3*segments);
    for(int b=0; b<segments; ++b)
    {
        // vertex data
        facState->GetVertices()[b*3  ].Set(0, 0, 0);
        facState->GetVertices()[b*3+1].Set(perp[b] * -topScale + rays[b]);
        facState->GetVertices()[b*3+2].Set(perp[b] *  topScale + rays[b]);

        // texture coordinates
        facState->GetTexels()[b*3  ].Set(0.5f, 0.05f);
        facState->GetTexels()[b*3+1].Set(0,    0.95f);
        facState->GetTexels()[b*3+2].Set(1,    0.95f);

        // normals
        facState->GetNormals()[b*3  ].Set(-rays[b]);
        facState->GetNormals()[b*3+1].Set(rays[b]);
        facState->GetNormals()[b*3+2].Set(rays[b]);
    }

    animLength = 10;
    if(keyFrames->GetSize() > 0)
        animLength += keyFrames->Get(keyFrames->GetSize()-1)->time;

    facState->SetTriangleCount(2*segments);
    for(int q=0; q<segments; ++q)
    {
        facState->GetTriangles()[q*2  ].Set(q*3, q*3+1, q*3+2);
        facState->GetTriangles()[q*2+1].Set(q*3, q*3+2, q*3+1);
    }

    return true;
}

const csVector3* psEffectObjStar::MeshAnimControl::UpdateVertices(csTicks /*current*/, const csVector3* /*verts*/,
        int /*num_verts*/, uint32 /*version_id*/)
{
    return parent->vert;
}

const csColor4* psEffectObjStar::MeshAnimControl::UpdateColors(csTicks /*current*/, const csColor4* /*colors*/,
        int /*num_colors*/, uint32 /*version_id*/)
{
    return parent->colour;
}

