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
#include <imesh/nullmesh.h>

#include "effects/pseffectanchorspline.h"

#include "util/pscssetup.h"
#include "util/log.h"

psEffectAnchorSpline::psEffectAnchorSpline()
    :psEffectAnchor()
{
    spline = new csCatmullRomSpline(3,0);
}

psEffectAnchorSpline::~psEffectAnchorSpline()
{
    delete spline;
}

bool psEffectAnchorSpline::Load(iDocumentNode* node)
{
    // get the attributes
    name.Clear();
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect spline anchor with no name.\n");
        return false;
    }

    if(!psEffectAnchor::Load(node))
        return false;

    return PostSetup();
}

bool psEffectAnchorSpline::Create(const csVector3 &offset, iMeshWrapper* /*posAttach*/, bool rotateWithMesh)
{
    static unsigned long nextUniqueID = 0;
    csString anchorID = "effect_anchor_spline";
    anchorID += nextUniqueID++;

    objBasePos = offset;
    objOffset = csVector3(0,0,0);
    this->rotateWithMesh = rotateWithMesh;

    mesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", anchorID);
    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (mesh->GetMeshObject());
    if(!state)
    {
        Error1("No NullMeshState.");
        return false;
    }
    state->SetRadius(1.0);
    isReady = true;

    return true;
}

bool psEffectAnchorSpline::Update(csTicks elapsed)
{
    life += (float)elapsed;
    if(life > animLength)
        life = fmod(life,animLength);
    if(!life)
        life += animLength;

    if(keyFrames->GetSize() == 0)
        objTargetOffset = csVector3(0,0,0);
    else
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = currKeyFrame+1;
        if(nextKeyFrame >= keyFrames->GetSize())
            nextKeyFrame = 0;

        // TOTARGET
        objTargetOffset = lerpVec(
                              csVector3(keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_TOTARGET_X],
                                        keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_TOTARGET_Y],
                                        keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_TOTARGET_Z]),
                              csVector3(keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_TOTARGET_X],
                                        keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_TOTARGET_Y],
                                        keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_TOTARGET_Z]),
                              keyFrames->Get(currKeyFrame)->time, keyFrames->Get(nextKeyFrame)->time, life);

        TransformOffset(objTargetOffset);
    }

    // POSITION
    spline->Calculate(life);
    objOffset = csVector3(spline->GetInterpolatedDimension(0),
                          spline->GetInterpolatedDimension(1),
                          spline->GetInterpolatedDimension(2));

    // adjust position by direction if there is one
    if(dir == DT_TARGET)
        objOffset = targetTransf * csVector3(-objOffset.x, objOffset.y, -objOffset.z);
    else if(dir == DT_ORIGIN)
        objOffset = posTransf * csVector3(-objOffset.x, objOffset.y, -objOffset.z);

    // apply it
    mesh->GetMovable()->SetPosition(objEffectPos + objBasePos + objTargetOffset + objOffset);
    mesh->GetMovable()->UpdateMove();

    return true;
}

psEffectAnchor* psEffectAnchorSpline::Clone() const
{
    psEffectAnchorSpline* newObj = new psEffectAnchorSpline();
    CloneBase(newObj);

    // spline anchor specific
    newObj->spline = spline->Clone();

    return newObj;
}

bool psEffectAnchorSpline::PostSetup()
{
    int count = 0;
    // fill in the spline according to the position we got from the keyframes
    for(size_t a=0; a<keyFrames->GetSize(); ++a)
    {
        if(keyFrames->Get(a)->specAction.IsBitSet(psEffectAnchorKeyFrame::KA_POS_X)
                || keyFrames->Get(a)->specAction.IsBitSet(psEffectAnchorKeyFrame::KA_POS_Y)
                || keyFrames->Get(a)->specAction.IsBitSet(psEffectAnchorKeyFrame::KA_POS_Z))
        {
            int idx = spline->GetPointCount();
            if(count >= idx)
                spline->InsertPoint(idx-1);
            spline->SetTimeValue(count, keyFrames->Get(a)->time);
            spline->SetDimensionValue(0, count, keyFrames->Get(a)->actions[psEffectAnchorKeyFrame::KA_POS_X]);
            spline->SetDimensionValue(1, count, keyFrames->Get(a)->actions[psEffectAnchorKeyFrame::KA_POS_Y]);
            spline->SetDimensionValue(2, count, keyFrames->Get(a)->actions[psEffectAnchorKeyFrame::KA_POS_Z]);
            ++count;
        }
    }
    return true;
}
