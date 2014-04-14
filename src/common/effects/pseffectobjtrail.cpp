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
#include <imesh/object.h>
#include <imesh/objmodel.h>
#include <csutil/cscolor.h>
#include <csgeom/tri.h>
#include <csutil/flags.h>

#include "effects/pseffect2drenderer.h"
#include "effects/pseffectobjtrail.h"
#include "effects/pseffectanchor.h"

#include "util/log.h"
#include "util/pscssetup.h"

psEffectObjTrail::psEffectObjTrail(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    vert = 0;
    colour = 0;
    missingUpdate = 0;

    setMesh = false;

    spline[0] = 0;
    spline[1] = 0;

    meshControl.AttachNew(new MeshAnimControl(this));
}

psEffectObjTrail::~psEffectObjTrail()
{
    delete [] vert;
    delete [] colour;
    delete [] missingUpdate;

    delete spline[0];
    delete spline[1];
}

bool psEffectObjTrail::Load(iDocumentNode* node, iLoaderContext* ldr_context)
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

    csRef<iDocumentNode> dataNode;

    // updatelen
    updateLen = 1000.0f;
    dataNode = node->GetNode("updatelen");
    if(dataNode)
        updateLen = dataNode->GetContentsValueAsFloat();

    // middle
    dataNode = node->GetNode("middle");
    if(dataNode)
        useMid = true;
    else
        useMid = false;

    if(!psEffectObj::Load(node, ldr_context))
        return false;

    return PostSetup();
}

bool psEffectObjTrail::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_trail_";
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

    vert = new csVector3[segments*2+2];
    colour = new csColor4[segments*2+2];
    missingUpdate = new bool[segments+1];
    for(int a=0; a<=segments; ++a)
        missingUpdate[a] = false;

    currSegment = 0;
    updateLeft = 0.0f;
    updateTotal = 0.0f;

    spline[0] = new csCatmullRomSpline(3, 0);
    spline[1] = new csCatmullRomSpline(3, 0);

    return true;
}

bool psEffectObjTrail::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    life += elapsed;
    if(life > animLength && killTime <= 0)
    {
        life %= animLength;
        if(!life)
            life += animLength;
    }

    isAlive |= (life >= birth);

    if(!setMesh && isAlive)
    {
        if(!anchorMesh)
            return false;

        mesh->GetMovable()->SetSector(anchorMesh->GetMovable()->GetSectors()->Get(0));
        mesh->GetMovable()->SetPosition(anchorMesh->GetMovable()->GetFullPosition());
        mesh->GetMovable()->UpdateMove();

        setMesh = true;
    }
    if(isAlive && anchorMesh && dir == DT_NONE)
        matBase = anchorMesh->GetMovable()->GetFullTransform().GetT2O();

    if(killTime > 0)
    {
        // age the effect obj
        killTime -= elapsed;
        if(killTime <= 0)
            return false;
    }

    csVector3 newColour(1,1,1);
    csVector3 rot(0,0,0);
    csVector3 spin(0,0,0);
    csVector3 posOffset(0,0,0);
    float height = 1.0f;
    alpha = 1.0f;

    if(keyFrames->GetSize() > 0)
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = currKeyFrame + 1;
        if(nextKeyFrame >= keyFrames->GetSize())
            nextKeyFrame = 0;

        // grab and lerp values
        float lerpfactor = LERP_FACTOR;
        rot = LERP_VEC_KEY(KA_ROT,lerpfactor);
        spin = LERP_VEC_KEY(KA_SPIN,lerpfactor);
        newColour = LERP_VEC_KEY(KA_COLOUR,lerpfactor);
        alpha = LERP_KEY(KA_ALPHA,lerpfactor);
        posOffset = LERP_VEC_KEY(KA_POS,lerpfactor);
        height = LERP_KEY(KA_HEIGHT,lerpfactor);
    }

    matBase *= csZRotMatrix3(rot.z) * csYRotMatrix3(rot.y) * csXRotMatrix3(rot.x);
    csMatrix3 matRot = csZRotMatrix3(spin.z) * csYRotMatrix3(spin.y) * csXRotMatrix3(spin.x);

    if(updateTotal < updateLen && currSegment <= segments)
    {
        csVector3 offset = anchorMesh->GetMovable()->GetFullPosition() - mesh->GetMovable()->GetFullPosition();

        updateLeft -= elapsed;
        updateTotal += elapsed;
        if(updateLeft <= 0.0f)
        {
            // calculate data
            for(int a=currSegment; a<=segments; ++a)
            {
                if(useMid)
                {
                    vert[a*2  ].Set(offset + matBase * (posOffset + matRot * csVector3(0, height/2.0f,0)));
                    vert[a*2+1].Set(offset + matBase * (posOffset + matRot * csVector3(0,-height/2.0f,0)));
                }
                else
                {
                    vert[a*2  ].Set(offset + matBase * (posOffset + matRot * csVector3(0,height,0)));
                    vert[a*2+1].Set(offset + matBase * posOffset);
                }

                colour[a*2  ].Set(newColour.x, newColour.y, newColour.z, alpha);
                colour[a*2+1].Set(newColour.x, newColour.y, newColour.z, alpha);
            }

            // update splines
            int idx = spline[0]->GetPointCount();
            spline[0]->InsertPoint(idx-1);
            spline[0]->SetTimeValue(idx, currSegment);
            spline[0]->SetDimensionValue(0, idx, vert[currSegment*2].x);
            spline[0]->SetDimensionValue(1, idx, vert[currSegment*2].y);
            spline[0]->SetDimensionValue(2, idx, vert[currSegment*2].z);

            idx = spline[1]->GetPointCount();
            spline[1]->InsertPoint(idx-1);
            spline[1]->SetTimeValue(idx, currSegment);
            spline[1]->SetDimensionValue(0, idx, vert[currSegment*2+1].x);
            spline[1]->SetDimensionValue(1, idx, vert[currSegment*2+1].y);
            spline[1]->SetDimensionValue(2, idx, vert[currSegment*2+1].z);

            bool done = false;
            while(updateLeft <= 0.0f)
            {
                if(done && currSegment <= segments)
                    missingUpdate[currSegment] = true;
                done = true;
                updateLeft += updateLen / (float)segments;
                ++currSegment;
            }

            // update missing points
            for(int a=0; a<=segments; ++a)
            {
                if(missingUpdate[a])
                {
                    spline[0]->Calculate(a);
                    vert[a*2  ].Set(spline[0]->GetInterpolatedDimension(0),
                                    spline[0]->GetInterpolatedDimension(1),
                                    spline[0]->GetInterpolatedDimension(2));
                    spline[1]->Calculate(a);
                    vert[a*2+1].Set(spline[1]->GetInterpolatedDimension(0),
                                    spline[1]->GetInterpolatedDimension(1),
                                    spline[1]->GetInterpolatedDimension(2));
                }
            }
        }
    }
    return true;
}

psEffectObj* psEffectObjTrail::Clone() const
{
    psEffectObjTrail* newObj = new psEffectObjTrail(view, renderer2d);
    CloneBase(newObj);

    // trail specific
    newObj->segments = segments;
    newObj->updateLen = updateLen;
    newObj->useMid = useMid;

    return newObj;
}

bool psEffectObjTrail::PostSetup()
{
    static unsigned int uniqueID = 0;
    csString facName = "effect_trail_fac_";
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

    facState->SetVertexCount(2*segments+2);
    for(int b=0; b<=segments; ++b)
    {
        facState->GetVertices()[b*2  ].Set(0, 1, (float)b / (float)segments);
        facState->GetVertices()[b*2+1].Set(0, 0, (float)b / (float)segments);

        facState->GetTexels()[b*2  ].Set((float)b / (float)segments, 0);
        facState->GetTexels()[b*2+1].Set((float)b / (float)segments, 1);
    }

    animLength = 10;
    if(keyFrames->GetSize() > 0)
        animLength += keyFrames->Get(keyFrames->GetSize()-1)->time;

    facState->SetTriangleCount(4*segments);
    for(int q=0; q<segments; ++q)
    {
        facState->GetTriangles()[q*4  ].Set(q*2, q*2+1, q*2+2);
        facState->GetTriangles()[q*4+1].Set(q*2, q*2+2, q*2+1);

        facState->GetTriangles()[q*4+2].Set(q*2+3, q*2+1, q*2+2);
        facState->GetTriangles()[q*4+3].Set(q*2+3, q*2+2, q*2+1);
    }
    return true;
}

const csVector3* psEffectObjTrail::MeshAnimControl::UpdateVertices(csTicks /*current*/, const csVector3* /*verts*/,
        int /*num_verts*/, uint32 /*version_id*/)
{
    return parent->vert;
}

const csColor4* psEffectObjTrail::MeshAnimControl::UpdateColors(csTicks /*current*/, const csColor4* /*colors*/,
        int /*num_colors*/, uint32 /*version_id*/)
{
    return parent->colour;
}

