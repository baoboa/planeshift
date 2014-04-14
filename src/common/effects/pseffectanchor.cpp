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
#include <cstool/csview.h>
#include <imesh/particles.h>

#include "effects/pseffectanchor.h"

#include "util/log.h"
#include "util/pscssetup.h"

// keep track of the interpolation types for each key frame
const int lerpTypes[psEffectAnchorKeyFrame::KA_COUNT] =
{
    psEffectAnchorKeyFrame::IT_NONE,  /* BLANK */
    psEffectAnchorKeyFrame::IT_LERP,  /* POS_X */
    psEffectAnchorKeyFrame::IT_LERP,  /* POS_Y */
    psEffectAnchorKeyFrame::IT_LERP,  /* POS_Z */
    psEffectAnchorKeyFrame::IT_LERP,  /* TOTARGET_X */
    psEffectAnchorKeyFrame::IT_LERP,  /* TOTARGET_Y */
    psEffectAnchorKeyFrame::IT_LERP,  /* TOTARGET_Z */
};

psEffectAnchorKeyFrame::psEffectAnchorKeyFrame()
    :specAction(KA_COUNT)
{
    specAction.Clear();
    SetDefaults();
}

psEffectAnchorKeyFrame::psEffectAnchorKeyFrame(iDocumentNode* node, const psEffectAnchorKeyFrame* prevKeyFrame)
    :specAction(KA_COUNT)
{
    specAction.Clear();

    csRef<iDocumentNodeIterator> xmlbinds;

    time = atof(node->GetAttributeValue("time"));

    xmlbinds = node->GetNodes("action");
    csRef<iDocumentNode> keyNode;
    csRef<iDocumentAttribute> attr;

    if(!prevKeyFrame)
        SetupFirstFrame();

    // set default values, if these are wrong then they'll be replaced on the second (lerp) pass
    SetDefaults();

    while(xmlbinds->HasNext())
    {
        keyNode = xmlbinds->Next();
        csString action = keyNode->GetAttributeValue("name");
        action.Downcase();

        if(action == "position")
        {
            attr = keyNode->GetAttribute("x");
            if(attr)
            {
                specAction.SetBit(KA_POS_X);
                actions[KA_POS_X] = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("y");
            if(attr)
            {
                specAction.SetBit(KA_POS_Y);
                actions[KA_POS_Y] = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("z");
            if(attr)
            {
                specAction.SetBit(KA_POS_Z);
                actions[KA_POS_Z] = attr->GetValueAsFloat();
            }
        }
        else if(action == "totarget")
        {
            attr = keyNode->GetAttribute("x");
            if(attr)
            {
                specAction.SetBit(KA_TOTARGET_X);
                actions[KA_TOTARGET_X] = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("y");
            if(attr)
            {
                specAction.SetBit(KA_TOTARGET_Y);
                actions[KA_TOTARGET_Y] = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("z");
            if(attr)
            {
                specAction.SetBit(KA_TOTARGET_Z);
                actions[KA_TOTARGET_Z] = attr->GetValueAsFloat();
            }
        }
    }
}

void psEffectAnchorKeyFrame::SetDefaults()
{
    actions[KA_POS_X] = 0;
    actions[KA_POS_Y] = 0;
    actions[KA_POS_Z] = 0;
    actions[KA_TOTARGET_X] = 0;
    actions[KA_TOTARGET_Y] = 0;
    actions[KA_TOTARGET_Z] = 0;
}

void psEffectAnchorKeyFrame::SetupFirstFrame()
{
    // this is the first frame, so pretend that everything has been set (if it hasn't, it will use the defaults)
//    for (int a=0; a<KA_COUNT; ++a)
//        specAction.SetBit(a);
}

psEffectAnchorKeyFrame::~psEffectAnchorKeyFrame()
{
}

psEffectAnchorKeyFrameGroup::psEffectAnchorKeyFrameGroup()
{
}

psEffectAnchorKeyFrameGroup::~psEffectAnchorKeyFrameGroup()
{
    keyFrames.DeleteAll();
}

psEffectAnchor::psEffectAnchor()
{
    engine =  csQueryRegistry<iEngine> (psCSSetup::object_reg);

    objEffectPos = csVector3(0,0,0);
    objBasePos = csVector3(0,0,0);
    objTargetOffset = csVector3(0,0,0);
    objOffset = csVector3(0,0,0);
    posTransf.Identity();
    targetTransf.Identity();

    dir = DT_NONE;

    keyFrames.AttachNew(new psEffectAnchorKeyFrameGroup);

    isReady = false;
    animLength = 10;

    FillInLerps();
}

psEffectAnchor::~psEffectAnchor()
{
    if(mesh)
        engine->RemoveObject(mesh);
}

bool psEffectAnchor::Load(iDocumentNode* node)
{
    name = node->GetAttributeValue("name");

    csRef<iDocumentNode> dataNode;

    // direction
    dataNode = node->GetNode("dir");
    dir = DT_NONE;
    if(dataNode)
    {
        SetDirectionType(dataNode->GetContentsValue());
    }

    // KEYFRAMES
    csRef<iDocumentNodeIterator> xmlbinds;
    xmlbinds = node->GetNodes("keyFrame");
    csRef<iDocumentNode> keyNode;

    animLength = 0;
    psEffectAnchorKeyFrame* prevKeyFrame = 0;
    while(xmlbinds->HasNext())
    {
        keyNode = xmlbinds->Next();
        psEffectAnchorKeyFrame* keyFrame = new psEffectAnchorKeyFrame(keyNode, prevKeyFrame);
        if(keyFrame->time > animLength)
            animLength = keyFrame->time;
        prevKeyFrame = keyFrame;
        keyFrames->Push(keyFrame);
    }
    animLength += 10;
    // TODO: sort keyframes by time here

    // linearly interpolate values where an action wasn't specified
    FillInLerps();

    return true;
}

bool psEffectAnchor::Create(const csVector3 & /*offset*/, iMeshWrapper* /*posAttach*/, bool /*rotateWithMesh*/)
{
    return false;
}

bool psEffectAnchor::Update(csTicks elapsed)
{
    if(animLength < 10)
        animLength = 10;

    life += (float)elapsed;
    if(life > animLength)
        life = fmod(life,animLength);
    if(!life)
        life += animLength;

    if(keyFrames->GetSize() == 0)
        return true;

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


    // POSITION
    objOffset = lerpVec(csVector3(keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_X],
                                  keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Y],
                                  keyFrames->Get(currKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Z]),
                        csVector3(keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_X],
                                  keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Y],
                                  keyFrames->Get(nextKeyFrame)->actions[psEffectAnchorKeyFrame::KA_POS_Z]),
                        keyFrames->Get(currKeyFrame)->time, keyFrames->Get(nextKeyFrame)->time, life);

    // adjust position by direction if there is one
    if(dir == DT_TARGET)
        objOffset = targetTransf * csVector3(-objOffset.x, objOffset.y, -objOffset.z);
    else if(dir == DT_ORIGIN)
        objOffset = posTransf * csVector3(-objOffset.x, objOffset.y, -objOffset.z);


    mesh->GetMovable()->SetPosition(objEffectPos + objBasePos + objTargetOffset + objOffset);
    //mesh->GetMovable()->SetTransform(matBase);
    mesh->GetMovable()->UpdateMove();

    return true;
}

void psEffectAnchor::CloneBase(psEffectAnchor* newAnchor) const
{
    newAnchor->name = name;
    newAnchor->life = 0;
    newAnchor->animLength = animLength;
    newAnchor->objOffset = objOffset;
    newAnchor->dir = dir;

    // the group of keyFrames remains static (don't clone)
    newAnchor->keyFrames = keyFrames;
}

psEffectAnchor* psEffectAnchor::Clone() const
{
    psEffectAnchor* newObj = new psEffectAnchor();
    CloneBase(newObj);

    return newObj;
}

void psEffectAnchor::SetPosition(const csVector3 &basePos, iSector* sector, const csMatrix3 &transf)
{
    if(sector)
    {
        objEffectPos = basePos;
        posTransf = transf;
        if(rotateWithMesh)
        {
            mesh->GetMovable()->SetTransform(transf);
        }
        mesh->GetMovable()->SetPosition(objEffectPos + posTransf*objBasePos + objTargetOffset + objOffset);
        mesh->GetMovable()->SetSector(sector);
    }
}

void psEffectAnchor::SetPosition(const csVector3 &basePos, iSectorList* sectors, const csMatrix3 &transf)
{
    if(sectors)
    {
        objEffectPos = basePos;
        posTransf = transf;
        if(rotateWithMesh)
        {
            mesh->GetMovable()->SetTransform(transf);
        }
        mesh->GetMovable()->SetPosition(objEffectPos + posTransf*objBasePos + objTargetOffset + objOffset);
        mesh->GetMovable()->GetSectors()->RemoveAll();
        mesh->GetMovable()->SetSector(sectors->Get(0));
        for(int a=1; a<sectors->GetCount(); ++a)
            mesh->GetMovable()->GetSectors()->Add(sectors->Get(a));
    }
}

void psEffectAnchor::TransformOffset(csVector3 &offset)
{
    if(offset.x != 0 || offset.y != 0 || offset.z != 0)
    {
        // distance scale
        float tarDiff = -(target - objEffectPos).Norm();
        offset *= tarDiff;

        // direction
        offset = matBase * offset;
    }
}

const char* psEffectAnchor::GetDirectionType() const
{
    switch(dir)
    {
        case DT_TARGET:
            return "target";
        case DT_ORIGIN:
            return "origin";
    }
    return "none";
}

void psEffectAnchor::SetDirectionType(const char* newDir)
{
    csString dirName = newDir;
    dirName.Downcase();
    if(dirName == "target")
        dir = DT_TARGET;
    else if(dirName == "origin")
        dir = DT_ORIGIN;
    else if(dirName == "none")
        dir = DT_NONE;
}

size_t psEffectAnchor::AddKeyFrame(float time)
{
    psEffectAnchorKeyFrame* newKeyFrame = new psEffectAnchorKeyFrame();

    // if there are no other keys then this is the first frame.
    if(keyFrames->GetSize() == 0)
        newKeyFrame->SetupFirstFrame();

    newKeyFrame->time = time;

    // add it to the list
    keyFrames->Push(newKeyFrame);

    // return the index where the new keyframe can be found
    return keyFrames->GetSize()-1;
}

size_t psEffectAnchor::FindKeyFrameByTime(float time) const
{
    for(size_t a = keyFrames->GetSize(); a-- > 0;)
    {
        if(keyFrames->Get(a)->time < time)
            return a;
    }
    return 0;
}

bool psEffectAnchor::FindNextKeyFrameWithAction(size_t startFrame, size_t action, size_t &index) const
{
    for(size_t a=startFrame; a<keyFrames->GetSize(); a++)
    {
        if(a == 0 || keyFrames->Get(a)->specAction.IsBitSet(action))
        {
            index = a;
            return true;
        }
    }
    return false;
}

void psEffectAnchor::FillInLerps()
{
    // this code is crap, but doing it this way allows everything else to be decently nice, clean, and efficient
    size_t a,b,nextIndex;

    for(size_t k=0; k<psEffectAnchorKeyFrame::KA_COUNT; ++k)
    {
        a = 0;
        while(FindNextKeyFrameWithAction(a+1, k, nextIndex))
        {
            for(b=a+1; b<nextIndex; ++b)
            {
                switch(lerpTypes[k])
                {
                    case psEffectAnchorKeyFrame::IT_FLOOR:
                        keyFrames->Get(b)->actions[k] = keyFrames->Get(a)->actions[k];
                        break;
                    case psEffectAnchorKeyFrame::IT_CEILING:
                        keyFrames->Get(b)->actions[k] = keyFrames->Get(nextIndex)->actions[k];
                        break;
                    case psEffectAnchorKeyFrame::IT_LERP:
                        keyFrames->Get(b)->actions[k] = lerp(keyFrames->Get(a)->actions[k],
                                                             keyFrames->Get(nextIndex)->actions[k],
                                                             keyFrames->Get(a)->time,
                                                             keyFrames->Get(nextIndex)->time,
                                                             keyFrames->Get(b)->time);
                        break;
                }
            }
            a = nextIndex;
        }

        // no matter what the interpolation type (as long as we have one), just clamp the end
        if(lerpTypes[k] != psEffectAnchorKeyFrame::IT_NONE)
        {
            for(b=a+1; b<keyFrames->GetSize(); ++b)
                keyFrames->Get(b)->actions[k] = keyFrames->Get(a)->actions[k];
        }
    }
}
