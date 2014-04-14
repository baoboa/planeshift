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
#include <imesh/objmodel.h>
#include <csutil/flags.h>
#include <csutil/cscolor.h>
#include <csgeom/tri.h>

#include "effects/pseffectobjspire.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"

psEffectObjSpire::psEffectObjSpire(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    vert = 0;
    texel = 0;
    colour = 0;
    vertCount = 0;
    meshControl.AttachNew(new MeshAnimControl(this));
}

psEffectObjSpire::~psEffectObjSpire()
{
    delete [] vert;
    delete [] texel;
    delete [] colour;
}

bool psEffectObjSpire::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    // get the attributes
    name.Clear();
    materialName.Clear();
    shape = SPI_CIRCLE;
    segments = 5;
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
        else if(attrName == "shape")
        {
            csString sshape = attr->GetValue();
            sshape.Downcase();
            if(sshape == "circle")
                shape = SPI_CIRCLE;
            else if(sshape == "cylinder")
                shape = SPI_CYLINDER;
            else if(sshape == "asterix")
                shape = SPI_ASTERIX;
            else if(sshape == "star")
                shape = SPI_STAR;
            else if(sshape == "layered")
                shape = SPI_LAYERED;
        }
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

bool psEffectObjSpire::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_spire_";
    effectID += nextUniqueID++;

    // create a mesh wrapper from the factory we just created
    mesh = engine->CreateMeshWrapper(meshFact, effectID.GetData());

    // do the up vector
    objUp = up;
    csReversibleTransform rt;
    rt.LookAt(csVector3(up.x, up.z, up.y), csVector3(0,2,1));
    matUp = rt.GetT2O();
    matBase = matUp;

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

    genState->SetAnimationControl(meshControl);

    vert = new csVector3[vertCount];
    texel = new csVector2[vertCount];
    colour = new csColor4[vertCount];
    for(int a=0; a<vertCount; ++a)
        colour[a].Set(1,1,1,1);

    return true;
}

bool psEffectObjSpire::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    //float scale = 1;
    float topScale = 1;
    float height = 1;
    float padding = 1;

    if(keyFrames->GetSize() > 0)
    {
        // COLOUR
        float lerpfactor = LERP_FACTOR;
        csVector3 lerpColour = LERP_VEC_KEY(KA_COLOUR,lerpfactor);
        float lerpAlpha = LERP_KEY(KA_ALPHA,lerpfactor);
        for(int a=0; a<vertCount; ++a)
            colour[a].Set(lerpColour.x, lerpColour.y, lerpColour.z, lerpAlpha);

        topScale = LERP_KEY(KA_TOPSCALE,lerpfactor);
        height = LERP_KEY(KA_HEIGHT,lerpfactor);
        padding = LERP_KEY(KA_PADDING,lerpfactor);
    }
    CalculateData(shape, segments, vert, texel, topScale, height, padding);

    return true;
}

psEffectObj* psEffectObjSpire::Clone() const
{
    psEffectObjSpire* newObj = new psEffectObjSpire(view, renderer2d);
    CloneBase(newObj);

    // spire specific
    newObj->shape = shape;
    newObj->segments = segments;
    newObj->vertCount = vertCount;

    return newObj;
}

void psEffectObjSpire::CalculateData(int shape, int segments, csVector3* verts, csVector2* texels, float topScale, float height, float padding)
{
    switch(shape)
    {
        case SPI_CIRCLE:
            for(int b=0; b<=segments; ++b)
            {
                float currAng = b * 2.0f*PI / segments;
                float cosCurr = cosf(currAng);
                float sinCurr = sinf(currAng);

                // vertex data
                verts[b*2  ].Set(cosCurr * topScale, height, sinCurr * topScale);
                verts[b*2+1].Set(cosCurr, 0, sinCurr);

                // texture coordinates
                float u1 = (float)b / (float)segments;
                texels[b*2  ].Set(u1, 0.01f);
                texels[b*2+1].Set(u1, 0.99f);
            }
            break;
        case SPI_CYLINDER:
            for(int b=0; b<=segments; ++b)
            {
                float currAng = b * 2.0f*PI / segments;
                float cosCurr = topScale*cosf(currAng);
                float sinCurr = topScale*sinf(currAng);

                // vertex data
                verts[b*2  ].Set(cosCurr, height, sinCurr);
                verts[b*2+1].Set(cosCurr, 0, sinCurr);

                // texture coordinates
                float u1 = (float)b / (float)segments;
                texels[b*2  ].Set(u1, 0.01f);
                texels[b*2+1].Set(u1, 0.99f);
            }
            break;
        case SPI_ASTERIX:
            for(int b=0; b<segments; ++b)
            {
                float currAng = b * PI / segments;
                float cosCurr = cosf(currAng);
                float sinCurr = sinf(currAng);

                // vertex data
                verts[b*4  ].Set(cosCurr * topScale, height, sinCurr * topScale);
                verts[b*4+1].Set(-cosCurr * topScale, height, -sinCurr * topScale);
                verts[b*4+2].Set(-cosCurr, 0, -sinCurr);
                verts[b*4+3].Set(cosCurr, 0, sinCurr);

                // texture coordinates
                texels[b*4  ].Set(1, 0.05f);
                texels[b*4+1].Set(0, 0.05f);
                texels[b*4+2].Set(0, 0.95f);
                texels[b*4+3].Set(1, 0.95f);
            }
            break;
        case SPI_STAR:
            for(int b=0; b<segments; ++b)
            {
                float currAng = b * 2.0f*PI / segments;
                float cosCurr = cosf(currAng);
                float sinCurr = sinf(currAng);
                float nextAng = currAng + 2.0f*PI/segments;
                float cosNext = cosf(nextAng);
                float sinNext = sinf(nextAng);
                float midAng = currAng + (nextAng - currAng) / 2.0f;
                float cosMid = cosf(midAng);
                float sinMid = sinf(midAng);

                // vertex data
                verts[b*6  ].Set(cosCurr * topScale, height, sinCurr * topScale);
                verts[b*6+1].Set(cosMid * topScale * 0.5f, height, sinMid * topScale * 0.5f);
                verts[b*6+2].Set(cosNext * topScale, height, sinNext * topScale);
                verts[b*6+3].Set(cosCurr, 0, sinCurr);
                verts[b*6+4].Set(cosMid * 0.5f, 0, sinMid * 0.5f);
                verts[b*6+5].Set(cosNext, 0, sinNext);

                // texture coordinates
                float u1 = (float)b / (float)segments;
                float u2 = (float)(b+0.5f) / (float)segments;
                float u3 = (float)(b+1.f) / (float)segments;

                texels[b*8  ].Set(u1, 0.05f);
                texels[b*8+1].Set(u2, 0.05f);
                texels[b*8+2].Set(u3, 0.05f);
                texels[b*8+3].Set(u1, 0.95f);
                texels[b*8+4].Set(u2, 0.95f);
                texels[b*8+5].Set(u3, 0.95f);
            }
            break;
        case SPI_LAYERED:
            csVector3 basePoint = -csVector3(0, 0, padding*(segments-1)/2.0f);
            for(int b=0; b<segments; ++b)
            {
                // vertex data
                verts[b*4  ].Set((basePoint.x - topScale / 2.0f), (basePoint.y + padding * b) + height, (basePoint.z - topScale / 2.0f));
                verts[b*4+1].Set((basePoint.x + topScale / 2.0f), (basePoint.y + padding * b) + height, (basePoint.z + topScale / 2.0f));
                verts[b*4+2].Set((basePoint.x + 1.0f     / 2.0f), (basePoint.y + padding * b) + height, (basePoint.z + 1.0f     / 2.0f));
                verts[b*4+3].Set((basePoint.x - 1.0f     / 2.0f), (basePoint.y + padding * b) + height, (basePoint.z - 1.0f     / 2.0f));

                // texture coordinates
                texels[b*4  ].Set(1, 0.05f);
                texels[b*4+1].Set(0, 0.05f);
                texels[b*4+2].Set(0, 0.95f);
                texels[b*4+3].Set(1, 0.95f);
            }
            break;
    }
}

bool psEffectObjSpire::PostSetup()
{
    static unsigned int uniqueID = 0;
    csString facName = "effect_spire_fac_";
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

    // ensure the minimum number of segments for the shape
    switch(shape)
    {
        case SPI_CIRCLE:
        case SPI_CYLINDER:
            segments = (segments >= 3 ? segments : 3);
            facState->SetVertexCount(2 * segments + 2);
            break;
        case SPI_ASTERIX:
            segments = (segments >= 3 ? segments : 3);
            facState->SetVertexCount(4 * segments);
            break;
        case SPI_STAR:
            segments = (segments >= 3 ? segments : 3);
            facState->SetVertexCount(6 * segments);
            break;
        case SPI_LAYERED:
            segments = (segments >= 1 ? segments : 1);
            facState->SetVertexCount(4 * segments);
            break;
    }
    vertCount = facState->GetVertexCount();

    CalculateData(shape, segments, facState->GetVertices(), facState->GetTexels());

    animLength = 10;
    if(keyFrames->GetSize() > 0)
        animLength += keyFrames->Get(keyFrames->GetSize()-1)->time;

    // create the tris
    switch(shape)
    {
        case SPI_CIRCLE:
        case SPI_CYLINDER:
            facState->SetTriangleCount(4*segments);
            for(int q=0; q<segments; ++q)
            {
                facState->GetTriangles()[q*4  ].Set(q*2,   q*2+1, q*2+2);
                facState->GetTriangles()[q*4+1].Set(q*2+1, q*2+3, q*2+2);

                facState->GetTriangles()[q*4+2].Set(q*2,   q*2+2, q*2+1);
                facState->GetTriangles()[q*4+3].Set(q*2+1, q*2+2, q*2+3);
            }
            break;
        case SPI_ASTERIX:
            facState->SetTriangleCount(4*segments);
            for(int q=0; q<segments; ++q)
            {
                facState->GetTriangles()[q*4  ].Set(q*4, q*4+2, q*4+1);
                facState->GetTriangles()[q*4+1].Set(q*4, q*4+2, q*4+3);

                facState->GetTriangles()[q*4+2].Set(q*4+2, q*4, q*4+1);
                facState->GetTriangles()[q*4+3].Set(q*4+2, q*4, q*4+3);
            }
            break;
        case SPI_STAR:
            facState->SetTriangleCount(8*segments);
            for(int q=0; q<segments; ++q)
            {
                facState->GetTriangles()[q*8  ].Set(q*8,   q*8+1, q*8+3);
                facState->GetTriangles()[q*8+1].Set(q*8,   q*8+1, q*8+4);
                facState->GetTriangles()[q*8+2].Set(q*8+1, q*8+2, q*8+4);
                facState->GetTriangles()[q*8+3].Set(q*8+1, q*8+2, q*8+5);

                facState->GetTriangles()[q*8+4].Set(q*8,   q*8+3, q*8+1);
                facState->GetTriangles()[q*8+5].Set(q*8,   q*8+4, q*8+1);
                facState->GetTriangles()[q*8+6].Set(q*8+1, q*8+4, q*8+2);
                facState->GetTriangles()[q*8+7].Set(q*8+1, q*8+5, q*8+2);
            }
            break;
        case SPI_LAYERED:
            facState->SetTriangleCount(4*segments);
            for(int q=0; q<segments; ++q)
            {
                facState->GetTriangles()[q*4  ].Set(q*4, q*4+2, q*4+1);
                facState->GetTriangles()[q*4+1].Set(q*4, q*4+2, q*4+3);

                facState->GetTriangles()[q*4+2].Set(q*4, q*4+4, q*4+1);
                facState->GetTriangles()[q*4+3].Set(q*4, q*4+4, q*4+3);
            }
            break;
    }

    return true;
}

const csVector3* psEffectObjSpire::MeshAnimControl::UpdateVertices(csTicks /*current*/, const csVector3* /*verts*/,
        int /*num_verts*/, uint32 /*version_id*/)
{
    return parent->vert;
}

const csVector2* psEffectObjSpire::MeshAnimControl::UpdateTexels(csTicks /*current*/, const csVector2* /*texels*/,
        int /*num_texels*/, uint32 /*version_id*/)
{
    return parent->texel;
}

const csColor4* psEffectObjSpire::MeshAnimControl::UpdateColors(csTicks /*current*/, const csColor4* /*colors*/,
        int /*num_colors*/, uint32 /*version_id*/)
{
    return parent->colour;
}

