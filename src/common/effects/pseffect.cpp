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

#include <csgeom/vector3.h>
#include <csutil/xmltiny.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/mesh.h>
#include <isndsys/ss_renderer.h>
#include <iutil/object.h>

#include "pseffect.h"

#include "pseffectanchor.h"
#include "pseffectanchorbasic.h"
#include "pseffectanchorspline.h"
#include "pseffectanchorsocket.h"

#include "pseffectobj.h"
#include "pseffectobjquad.h"
#include "pseffectobjspire.h"
#include "pseffectobjmesh.h"
#include "pseffectobjsimpmesh.h"
#include "pseffectobjparticles.h"
#include "pseffectobjsound.h"
#include "pseffectobjstar.h"
#include "pseffectobjtext.h"
#include "pseffectobjtrail.h"
#include "pseffectobjdecal.h"
#include "pseffectobjtext2d.h"
#include "pseffectobjlabel.h"
#include "pseffectobjlight.h"

#include "util/log.h"
#include "util/pscssetup.h"

// used for generating a unique ID
static unsigned int genUniqueID = 0;


psEffect::psEffect()
{
    name.Clear();
    mainTextObj = (size_t)(-1);
    visible = true;
}

psEffect::~psEffect()
{
    effectAnchors.DeleteAll();
    effectObjs.DeleteAll();
}

psEffectAnchor* psEffect::CreateAnchor(const csString &type)
{
    if(type == "basic")
        return new psEffectAnchorBasic();
    else if(type == "spline")
        return new psEffectAnchorSpline();
    else if(type == "socket")
        return new psEffectAnchorSocket();

    csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Unknown anchor type: %s\n", type.GetData());
    return 0;
}

size_t psEffect::AddAnchor(psEffectAnchor* anchor)
{
    effectAnchors.Push(anchor);
    return effectAnchors.GetSize()-1;
}

bool psEffect::Load(iDocumentNode* node, iView* parentView, psEffect2DRenderer* renderer2d, iLoaderContext* ldr_context)
{
    csRef<iDocumentNodeIterator> xmlbinds;

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
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect with no name.\n");
        return false;
    }

    // anchors
    xmlbinds = node->GetNodes("anchor");
    csRef<iDocumentNode> anchorNode;
    while(xmlbinds->HasNext())
    {
        anchorNode = xmlbinds->Next();

        psEffectAnchor* anchor = 0;

        // create a anchor from the given type string
        anchor = CreateAnchor(anchorNode->GetAttributeValue("type"));
        if(anchor)
        {
            if(anchor->Load(anchorNode))
                AddAnchor(anchor);
            else
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Problem with an effect anchor in: %s\n", name.GetData());
            }
        }
        else
        {
            csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't create anchor.\n");
        }
    }

    // objs
    xmlbinds = node->GetNodes("obj");
    csRef<iDocumentNode> objNode;
    while(xmlbinds->HasNext())
    {
        objNode = xmlbinds->Next();

        psEffectObj* obj = 0;

        csString type = objNode->GetAttributeValue("type");
        type.Downcase();
        if(type == "quad")
        {
            obj = new psEffectObjQuad(parentView, renderer2d);
        }
        else if(type == "spire")
        {
            obj = new psEffectObjSpire(parentView, renderer2d);
        }
        else if(type == "mesh")
        {
            obj = new psEffectObjMesh(parentView, renderer2d);
        }
        else if(type == "simpmesh")
        {
            obj = new psEffectObjSimpMesh(parentView, renderer2d);
        }
        else if(type == "particles")
        {
            obj = new psEffectObjParticles(parentView, renderer2d);
        }
        else if(type == "sound")
        {
            obj = new psEffectObjSound(parentView, renderer2d);
        }
        else if(type == "star")
        {
            obj = new psEffectObjStar(parentView, renderer2d);
        }
        else if(type == "text")
        {
            obj = new psEffectObjText(parentView, renderer2d);
        }
        else if(type == "label")
        {
            obj = new psEffectObjLabel(parentView, renderer2d);
        }
        else if(type == "trail")
        {
            obj = new psEffectObjTrail(parentView, renderer2d);
        }
#ifdef PS_EFFECT_ENABLE_DECAL
        else if(type == "decal")
        {
            obj = new psEffectObjDecal(parentView, renderer2d);
#endif // PS_EFFECT_ENABLE_DECAL
        }
        else if(type == "text2d")
        {
            obj = new psEffectObjText2D(parentView, renderer2d);
        }
        else if(type == "light")
        {
            obj = new psEffectObjLight(parentView, renderer2d);
        }

        if(obj)
        {
            if(obj->Load(objNode, ldr_context))
            {
                effectObjs.Push(obj);
                if(mainTextObj == (size_t)(-1))
                {
                    psEffectObjTextable* textObj = dynamic_cast<psEffectObjTextable*>(obj);
                    if(textObj)
                        mainTextObj = effectObjs.GetSize()-1;
                }
            }
            else
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                         "Problem with an effect obj in: %s/%s\n", type.GetData(), name.GetData());

                delete obj;
            }
        }
        else
        {
            csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                     "Unknown effect obj type: %s\n", type.GetData());
        }
    }

    return true;
}

unsigned int psEffect::Render(iSectorList* sectors, const csVector3 &offset, iMeshWrapper* attachPos,
                              iMeshWrapper* attachTarget, const csVector3 &up, const unsigned int uniqueIDOverride,
                              bool rotateWithMesh)
{
    bool hasSuccess = false;

    if(!sectors)
        return false; // no sectorlist

    if(sectors->GetCount() == 0)
        return false; // no sectors

    // create a listener for the attached position if it exists
    if(attachPos)
    {
        positionListener.AttachNew(new psEffectMovableListener);
        attachPos->GetMovable()->AddListener(positionListener);
        attachPos->GetMovable()->UpdateMove();
    }

    // create a listener for the attached target if it exists
    if(attachTarget)
    {
        targetListener.AttachNew(new psEffectMovableListener);
        attachTarget->GetMovable()->AddListener(targetListener);
        attachTarget->GetMovable()->UpdateMove();
    }

    // build a rotation matrix that points from the position to the target
    csMatrix3 rotBase;
    rotBase.Identity();
    if(attachPos && attachTarget)
    {
        // make the base angle point towards the target
        csVector3 diff = attachPos->GetMovable()->GetFullPosition() - attachTarget->GetMovable()->GetFullPosition();
        csReversibleTransform transf;
        transf.LookAt(diff, csVector3(0,1,0));
        rotBase = transf.GetT2O();
        //rotBase = csYRotMatrix3(3.14159f - atan2(-diff.x, -diff.z));
    }

    // For absolute positioning (not attached to a position mesh), the offset is actually the base pos and not an
    // offset at all. This code ensures that the offset is 0,0,0 and basepos is the offset for such a case.
    csVector3 usedOffset = offset;
    if(!attachPos)
        usedOffset = csVector3(0,0,0);

    // the base position is either the offset (absolute position) or the attached mesh if it exists.
    csVector3 newPos = offset;
    csMatrix3 posTransf;
    posTransf.Identity();
    if(attachPos)
    {
        newPos = attachPos->GetMovable()->GetFullPosition();
        posTransf = attachPos->GetMovable()->GetFullTransform().GetT2O();
    }

    csMatrix3 targetTransf = posTransf;
    if(attachTarget)
        targetTransf = attachTarget->GetMovable()->GetFullTransform().GetT2O();

    // effect anchors
    for(size_t i=0; i<effectAnchors.GetSize(); ++i)
    {
        if(effectAnchors[i]->Create(usedOffset, attachPos, rotateWithMesh))
        {
            hasSuccess = true;

            effectAnchors[i]->SetPosition(newPos, sectors, posTransf);

            // target of the anchor is either the given target if it exists or the position
            if(attachTarget)
                effectAnchors[i]->SetTarget(attachTarget->GetMovable()->GetFullPosition(), targetTransf);
            else
                effectAnchors[i]->SetTarget(newPos, posTransf);

            // if we're attached to both a position and target then orient the anchor
            if(attachPos && attachTarget)
                effectAnchors[i]->SetRotBase(rotBase);
        }
    }

    // effect objs
    for(size_t i=0; i<effectObjs.GetSize(); ++i)
    {
        if(effectObjs[i]->Render(up))
        {
            hasSuccess = true;

            psEffectAnchor* anchor = FindAnchor(effectObjs[i]->GetAnchorName());
            if(!anchor)
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                         "Couldn't find effect anchor: %s, in: %s\n",
                         effectObjs[i]->GetAnchorName().GetData(), name.GetData());
            }
            else
            {
                if(!effectObjs[i]->AttachToAnchor(anchor))
                {
                    csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_WARNING, "planeshift_effects",
                             "Couldn't attach obj to %s in %s\n",
                             effectObjs[i]->GetAnchorName().GetData(), name.GetData());
                }
            }

            // update the position of this obj
            if(attachPos)
            {
                effectObjs[i]->SetPosition(posTransf);
                if(!targetListener)
                    effectObjs[i]->SetTarget(posTransf);
            }

            // update the target of this obj
            if(attachTarget)
                effectObjs[i]->SetTarget(targetTransf);

            // if we're attached to both a position and target then orient the anchor
            if(attachPos && attachTarget)
                effectObjs[i]->SetRotBase(rotBase);
        }
    }

    if(!hasSuccess)
        return 0;

    uniqueID = (uniqueIDOverride > 0 ? uniqueIDOverride : ++genUniqueID);
    return uniqueID;
}

unsigned int psEffect::Render(iSector* sector, const csVector3 &offset, iMeshWrapper* attachPos,
                              iMeshWrapper* attachTarget, const csVector3 &up, const unsigned int uniqueIDOverride)
{
    bool hasSuccess = false;

    if(!sector)
        return false; // no sector

    // create a listener for the attached position if it exists
    if(attachPos)
    {
        positionListener.AttachNew(new psEffectMovableListener);
        attachPos->GetMovable()->AddListener(positionListener);
        attachPos->GetMovable()->UpdateMove();
    }

    // create a listener for the attached target if it exists
    if(attachTarget)
    {
        targetListener.AttachNew(new psEffectMovableListener);
        attachTarget->GetMovable()->AddListener(targetListener);
        attachTarget->GetMovable()->UpdateMove();
    }

    // build a rotation matrix that points from the position to the target
    csMatrix3 rotBase;
    rotBase.Identity();
    if(attachPos && attachTarget)
    {
        // make the base angle point towards the target
        csVector3 diff = attachPos->GetMovable()->GetFullPosition() - attachTarget->GetMovable()->GetFullPosition();
        csReversibleTransform transf;
        transf.LookAt(diff, csVector3(0,1,0));
        rotBase = transf.GetT2O();
        //rotBase = csYRotMatrix3(3.14159f - atan2(-diff.x, -diff.z));
    }

    // For absolute positioning (not attached to a position mesh), the offset is actually the base pos and not an
    // offset at all. This code ensures that the offset is 0,0,0 and basepos is the offset for such a case.
    csVector3 usedOffset = offset;
    if(!attachPos)
        usedOffset = csVector3(0,0,0);

    // the base position is either the offset (absolute position) or the attached mesh if it exists.
    csVector3 newPos = offset;
    csMatrix3 posTransf;
    posTransf.Identity();
    if(attachPos)
    {
        newPos = attachPos->GetMovable()->GetFullPosition();
        posTransf = attachPos->GetMovable()->GetFullTransform().GetT2O();
    }

    csMatrix3 targetTransf = posTransf;
    if(attachTarget)
        targetTransf = attachTarget->GetMovable()->GetFullTransform().GetT2O();

    // effect anchors
    for(size_t i=0; i<effectAnchors.GetSize(); ++i)
    {
        if(effectAnchors[i]->Create(usedOffset, attachPos))
        {
            hasSuccess = true;

            effectAnchors[i]->SetPosition(newPos, sector, posTransf);

            // target of the anchor is either the given target if it exists or the position
            if(attachTarget)
                effectAnchors[i]->SetTarget(attachTarget->GetMovable()->GetFullPosition(),
                                            targetTransf);
            else
                effectAnchors[i]->SetTarget(newPos, posTransf);

            // if we're attached to both a position and target then orient the anchor
            if(attachPos && attachTarget)
                effectAnchors[i]->SetRotBase(rotBase);
        }
    }

    // effect objs
    for(size_t i=0; i<effectObjs.GetSize(); ++i)
    {
        if(effectObjs[i]->Render(up))
        {
            hasSuccess = true;

            psEffectAnchor* anchor = FindAnchor(effectObjs[i]->GetAnchorName());
            if(!anchor)
            {
                csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                         "Couldn't find effect anchor: %s, in: %s\n",
                         effectObjs[i]->GetAnchorName().GetData(), name.GetData());
            }
            else
            {
                if(!effectObjs[i]->AttachToAnchor(anchor))
                {
                    csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_WARNING, "planeshift_effects",
                             "Couldn't attach obj to %s in %s\n",
                             effectObjs[i]->GetAnchorName().GetData(), name.GetData());
                }
            }

            // update the position of this obj
            if(attachPos)
            {
                effectObjs[i]->SetPosition(posTransf);
                if(!targetListener)
                    effectObjs[i]->SetTarget(posTransf);
            }

            // update the target of this obj
            if(attachTarget)
                effectObjs[i]->SetTarget(targetTransf);


            // if we're attached to both a position and target then orient the anchor
            if(attachPos && attachTarget)
                effectObjs[i]->SetRotBase(rotBase);
        }
    }

    if(!hasSuccess)
        return 0;

    uniqueID = (uniqueIDOverride > 0 ? uniqueIDOverride : ++genUniqueID);
    return uniqueID;
}

bool psEffect::Update(csTicks elapsed)
{
    bool updatePos = false;
    bool updateTarget = false;
    iSectorList* newSectors = NULL;
    csVector3 newPos;
    csVector3 newTarget;
    csMatrix3 newPosTransf;
    csMatrix3 newTargetTransf;

    if(positionListener)
    {
        // if the attachMesh is gone, remove the effect
        if(positionListener->IsMovableDead())
            return false;

        // if the attachMesh has moved, then flag a position update
        if(positionListener->GrabNewData(newSectors, newPos, newPosTransf))
        {
            if(!newSectors)
                return false; // no sector list

            if(newSectors->GetCount() == 0)
                return false; // not in any sector

            csString sector(newSectors->Get(0)->QueryObject()->GetName());
            if(sector.Compare("SectorWhereWeKeepEntitiesResidingInUnloadedMaps"))
                return true; // in a sector not loaded

            updatePos = true;
        }
    }
    if(targetListener)
    {
        // if the targetMesh has moved, then flag a target update
        if(targetListener->GrabNewData(newTarget, newTargetTransf))
            updateTarget = true;
    }

    // build a rotation matrix that points from the anchor to the target
    csMatrix3 rotBase;
    rotBase.Identity();
    if(updatePos || updateTarget)
    {
        if(targetListener && positionListener)
        {
            // make the base angle point towards the target
            csVector3 diff = positionListener->GetPosition() - targetListener->GetPosition();
            csReversibleTransform transf;
            transf.LookAt(diff, csVector3(0,1,0));
            rotBase = transf.GetT2O();
            //rotBase = csYRotMatrix3(3.14159f - atan2(-diff.x, -diff.z));
        }
    }

    // effect anchors
    for(size_t a = effectAnchors.GetSize(); a-- > 0;)
    {
        psEffectAnchor* anchor = effectAnchors[a];
        if(!anchor)
            continue;

        // update the position of this anchor
        if(updatePos)
        {
            effectAnchors[a]->SetPosition(newPos, newSectors, newPosTransf);
            if(!targetListener)
                anchor->SetTarget(newPos, newPosTransf);
        }

        // update the target of this anchor
        if(updateTarget)
            anchor->SetTarget(newTarget, newTargetTransf);

        // update the direction/orientation of this anchor
        if(updatePos || updateTarget)
            anchor->SetRotBase(rotBase);

        // update the actual anchor
        if(!anchor->Update(elapsed))
            effectAnchors.DeleteIndex(a); // anchor has told us it doesn't want to live anymore
    }

    // effect objs
    for(size_t a = effectObjs.GetSize(); a-- > 0;)
    {
        if(!effectObjs[a])
            continue;

        // update the position of this obj
        if(updatePos)
        {
            effectObjs[a]->SetPosition(newPosTransf);
            if(!targetListener)
                effectObjs[a]->SetTarget(newPosTransf);
        }

        // update the target of this obj
        if(updateTarget)
            effectObjs[a]->SetTarget(newTargetTransf);

        // update the orientation of the obj
        if(updatePos || updateTarget)
            effectObjs[a]->SetRotBase(rotBase);

        // update the actual obj
        if(!effectObjs[a]->Update(elapsed))
            effectObjs.DeleteIndex(a); // obj has told us it doesn't want to live anymore
    }

    return (effectObjs.GetSize() != 0);
}

void psEffect::SetKillTime(const int newKillTime)
{
    float scaling = (float)newKillTime/(float)GetKillTime();
    for(size_t a = 0; a < effectObjs.GetSize(); ++a)
    {
        effectObjs[a]->SetAnimationScaling(scaling);
    }
}

int psEffect::GetKillTime() const
{
    int killTime = 0;
    for(size_t a = 0; a < effectObjs.GetSize(); ++a)
    {
        int time = effectObjs[a]->GetKillTime();
        killTime = csMax(killTime, time);
    }
    return killTime;
}

bool psEffect::SetFrameParamScalings(const float* scale)
{
    bool result = false;
    for(size_t a = 0; a < effectObjs.GetSize(); ++a)
    {
        if(effectObjs[a]->SetFrameParamScalings(scale))
        {
            result = true;
        }
    }
    return result;
}

psEffect* psEffect::Clone() const
{
    psEffect* newEffect = new psEffect();
    newEffect->name = name;

    for(size_t a=0; a<effectAnchors.GetSize(); ++a)
        newEffect->effectAnchors.Push(effectAnchors[a]->Clone());

    for(size_t b=0; b<effectObjs.GetSize(); ++b)
        newEffect->effectObjs.Push(effectObjs[b]->Clone());

    newEffect->mainTextObj = mainTextObj;
    return newEffect;
}

unsigned int psEffect::GetUniqueID() const
{
    return uniqueID;
}

float psEffect::GetAnimLength() const
{
    float ret = 0;
    for(size_t a=0; a<effectObjs.GetSize(); ++a)
        ret = (effectObjs[a]->GetAnimLength() > ret ? effectObjs[a]->GetAnimLength() : ret);
    return ret;
}

size_t psEffect::GetAnchorCount() const
{
    return effectAnchors.GetSize();
}

psEffectAnchor* psEffect::GetAnchor(size_t idx) const
{
    return effectAnchors[idx];
}

size_t psEffect::GetObjCount() const
{
    return effectObjs.GetSize();
}

psEffectObj* psEffect::GetObj(size_t idx) const
{
    return effectObjs[idx];
}

const csString &psEffect::GetName() const
{
    return name;
}

psEffectObjTextable* psEffect::GetMainTextObj() const
{
    if(effectObjs.GetSize() <= mainTextObj)
        return 0;

    return dynamic_cast<psEffectObjTextable*>(effectObjs[mainTextObj]);
}

/*
bool psEffect::SetText(const char * text)
{
    if (mainTextObj == (size_t)(-1))
        return false;

    psEffectObjText * textObj = dynamic_cast<psEffectObjText *>(effectObjs[mainTextObj]);
    if (!textObj)
        return false;

    //return textObj->SetAttributedText(text);
    return true;
}
*/

psEffectObj* psEffect::FindObj(const csString &objName) const
{
    for(size_t a=0; a<effectObjs.GetSize(); ++a)
    {
        if(effectObjs[a]->GetName() == objName)
            return effectObjs[a];
    }
    return 0;
}

psEffectAnchor* psEffect::FindAnchor(const csString &anchorName) const
{
    for(size_t a=0; a<effectAnchors.GetSize(); ++a)
    {
        if(effectAnchors[a]->GetName() == anchorName)
            return effectAnchors[a];
    }
    return 0;
}

bool psEffect::IsVisible()
{
    return visible;
}

void psEffect::Show()
{
    visible = true;
    for(size_t i = 0; i < effectObjs.GetSize(); i++)
        effectObjs[i]->Show(visible);
}

void psEffect::Hide()
{
    visible = false;
    for(size_t i = 0; i < effectObjs.GetSize(); i++)
        effectObjs[i]->Show(visible);
}

void psEffect::SetScaling(float scale, float aspect)
{
    for(size_t i = 0; i < effectObjs.GetSize(); i++)
        effectObjs[i]->SetScaling(scale, aspect);
}
