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
#include <iengine/camera.h>
#include <iengine/scenenode.h>
#include <cstool/csview.h>
#include <imesh/particles.h>
#include <csutil/flags.h>

#include "effects/pseffectobj.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"

// keep track of the interpolation types for each key frame
const int lerpTypes[psEffectObjKeyFrame::KA_VEC_COUNT] =
{
    psEffectObjKeyFrame::IT_NONE,  /* BLANK */
    psEffectObjKeyFrame::IT_LERP,  /* SCALE */
    psEffectObjKeyFrame::IT_LERP,  /* TOP SCALE */
    psEffectObjKeyFrame::IT_FLOOR, /* CELL */
    psEffectObjKeyFrame::IT_LERP,  /* ALPHA */
    psEffectObjKeyFrame::IT_LERP,  /* HEIGHT */
    psEffectObjKeyFrame::IT_LERP,  /* PADDING */
    psEffectObjKeyFrame::IT_FLOOR, /* ANIMATE */
    psEffectObjKeyFrame::IT_LERP,  /* POS */
    psEffectObjKeyFrame::IT_LERP,  /* ROT */
    psEffectObjKeyFrame::IT_LERP,  /* SPIN */
    psEffectObjKeyFrame::IT_LERP  /* COLOUR */
};

psEffectObjKeyFrame::psEffectObjKeyFrame() : specAction(KA_VEC_COUNT)
{
    specAction.Clear();
}

psEffectObjKeyFrame::psEffectObjKeyFrame(const psEffectObjKeyFrame* other) : specAction(KA_VEC_COUNT)
{
    specAction.Clear();

    time = other->time;
    specAction = other->specAction;

    for(size_t i = 0; i < KA_COUNT; i++)
    {
        actions[i] = other->actions[i];
    }
    for(size_t i = 0; i < KA_VEC_COUNT - KA_COUNT; i++)
    {
        vecActions[i] = other->vecActions[i];
    }
    for(size_t i = 0; i < KA_VEC_COUNT; i++)
    {
        useScale[i] = other->useScale[i];
    }
}

psEffectObjKeyFrame::psEffectObjKeyFrame(iDocumentNode* node, const psEffectObjKeyFrame* prevKeyFrame) : specAction(KA_VEC_COUNT)
{
    specAction.Clear();

    csRef<iDocumentNodeIterator> xmlbinds;

    time = atof(node->GetAttributeValue("time"));

    xmlbinds = node->GetNodes("action");
    csRef<iDocumentNode> keyNode;
    csRef<iDocumentAttribute> attr;

    if(!prevKeyFrame)
    {
        // this is the first frame, so pretend that everything has been set (if it hasn't, it will use the defaults)
        for(int a=0; a<KA_COUNT; ++a)
            specAction.SetBit(a);
    }

    // set default values, if these are wrong then they'll be replaced on the second (lerp) pass
    actions[KA_SCALE] = 1.0f;
    actions[KA_TOPSCALE] = 1.0f;
    actions[KA_CELL] = 0;
    actions[KA_ALPHA] = 1.0f;
    actions[KA_HEIGHT] = 1.0f;
    actions[KA_PADDING] = 0.1f;
    actions[KA_ANIMATE] = 1;
    vecActions[KA_POS - KA_COUNT] = 0;
    vecActions[KA_ROT - KA_COUNT] = 0.0f;
    vecActions[KA_SPIN - KA_COUNT] = 0;
    vecActions[KA_COLOUR - KA_COUNT] = 1;

    for(int a=0; a<KA_VEC_COUNT; ++a)
    {
        useScale[a] = 0;
    }

    while(xmlbinds->HasNext())
    {
        keyNode = xmlbinds->Next();
        csString action = keyNode->GetAttributeValue("name");
        action.Downcase();

        if(action == "scale")
        {
            specAction.SetBit(KA_SCALE);
            actions[KA_SCALE] = keyNode->GetAttributeValueAsFloat("value");
            if(actions[KA_SCALE] == 0.0f)
                actions[KA_SCALE] = 0.001f;
            useScale[KA_SCALE] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "volume")
        {
            specAction.SetBit(KA_SCALE);
            actions[KA_SCALE] = keyNode->GetAttributeValueAsFloat("value");
            useScale[KA_SCALE] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "topscale")
        {
            specAction.SetBit(KA_TOPSCALE);
            actions[KA_TOPSCALE] = keyNode->GetAttributeValueAsFloat("value");
            if(actions[KA_TOPSCALE] == 0.0f)
                actions[KA_TOPSCALE] = 0.001f;
            useScale[KA_TOPSCALE] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "position")
        {
            specAction.SetBit(KA_POS);
            csVector3 &vec = vecActions[KA_POS - KA_COUNT];
            attr = keyNode->GetAttribute("x");
            if(attr)
            {
                vec.x = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("y");
            if(attr)
            {
                vec.y = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("z");
            if(attr)
            {
                vec.z = attr->GetValueAsFloat();
            }
            useScale[KA_POS] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "rotate")
        {
            specAction.SetBit(KA_ROT);
            csVector3 &vec = vecActions[KA_ROT - KA_COUNT];
            attr = keyNode->GetAttribute("x");
            if(attr)
            {
                vec.x = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("y");
            if(attr)
            {
                vec.y = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("z");
            if(attr)
            {
                vec.z = attr->GetValueAsFloat();
            }
            vec *= PI/180.f;
            useScale[KA_ROT] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "spin")
        {
            specAction.SetBit(KA_SPIN);
            csVector3 &vec = vecActions[KA_SPIN - KA_COUNT];
            attr = keyNode->GetAttribute("x");
            if(attr)
            {
                vec.x = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("y");
            if(attr)
            {
                vec.y = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("z");
            if(attr)
            {
                vec.z = attr->GetValueAsFloat();
            }
            vec *= PI/180.f;
            useScale[KA_SPIN] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "cell")
        {
            specAction.SetBit(KA_CELL);
            actions[KA_CELL] = keyNode->GetAttributeValueAsInt("value") + 0.1f;
            useScale[KA_CELL] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "colour")
        {
            specAction.SetBit(KA_COLOUR);
            csVector3 &vec = vecActions[KA_COLOUR - KA_COUNT];
            attr = keyNode->GetAttribute("r");
            if(attr)
            {
                vec.x = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("g");
            if(attr)
            {
                vec.y = attr->GetValueAsFloat();
            }
            attr = keyNode->GetAttribute("b");
            if(attr)
            {
                vec.z = attr->GetValueAsFloat();
            }
            vec /= 255.0f;
            useScale[KA_COLOUR] = keyNode->GetAttributeValueAsInt("use_scale",0);

            attr = keyNode->GetAttribute("a");
            if(attr)
            {
                specAction.SetBit(KA_ALPHA);
                actions[KA_ALPHA] = attr->GetValueAsFloat() / 255.0f;
                useScale[KA_ALPHA] = keyNode->GetAttributeValueAsInt("use_scale_alpha",0);
            }
        }
        else if(action == "alpha")
        {
            specAction.SetBit(KA_ALPHA);
            actions[KA_ALPHA] = keyNode->GetAttributeValueAsFloat("value") / 255.0f;
            useScale[KA_ALPHA] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "height")
        {
            specAction.SetBit(KA_HEIGHT);
            actions[KA_HEIGHT] = keyNode->GetAttributeValueAsFloat("value");
            if(actions[KA_HEIGHT] == 0.0f)
                actions[KA_HEIGHT] = 0.001f;
            useScale[KA_HEIGHT] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "padding")
        {
            specAction.SetBit(KA_PADDING);
            actions[KA_PADDING] = keyNode->GetAttributeValueAsFloat("value");
            useScale[KA_PADDING] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
        else if(action == "animate")
        {
            specAction.SetBit(KA_ANIMATE);
            csString val = keyNode->GetAttributeValue("value");
            val.Downcase();
            if(val == "yes" || val == "true")
                actions[KA_ANIMATE] = 1;
            else
                actions[KA_ANIMATE] = -1;

            useScale[KA_ANIMATE] = keyNode->GetAttributeValueAsInt("use_scale",0);
        }
    }
}

psEffectObjKeyFrame::~psEffectObjKeyFrame()
{
}

bool psEffectObjKeyFrame::SetParamScalings(const float* scale)
{
    bool result = false;
    for(size_t i = 0; i < KA_VEC_COUNT; i++)
    {
        if(useScale[i])
        {
            if(i < KA_COUNT)
            {
                actions[i] = actions[i]*scale[useScale[i]-1];
            }
            if(i >= KA_COUNT)
            {
                vecActions[i-KA_COUNT] =  vecActions[i-KA_COUNT]*scale[useScale[i]-1];
            }
            result = true; // At least one param where adjusted
        }
    }
    return result;
}

psEffectObjKeyFrameGroup::psEffectObjKeyFrameGroup()
{
}

psEffectObjKeyFrameGroup::~psEffectObjKeyFrameGroup()
{
    keyFrames.DeleteAll();
}

csPtr<psEffectObjKeyFrameGroup> psEffectObjKeyFrameGroup::Clone() const    //ticket 6051
{
    csRef<psEffectObjKeyFrameGroup> clone;                                 //ticket 6051
    clone.AttachNew(new psEffectObjKeyFrameGroup());                       //ticket 6051
    for(size_t i = 0; i < keyFrames.GetSize(); i++)
    {
        clone->Push(new psEffectObjKeyFrame(keyFrames[i]));
    }
    return csPtr<psEffectObjKeyFrameGroup> (clone);                        //ticket 6051
}

bool psEffectObjKeyFrameGroup::SetFrameParamScalings(const float* scale)
{
    bool result = false;
    for(size_t i = 0; i < keyFrames.GetSize(); i++)
    {
        if(keyFrames[i]->SetParamScalings(scale))
        {
            result = true; // We did adjust at least one frame
        }
    }
    return result;
}

psEffectObj::psEffectObj(iView* parentView, psEffect2DRenderer* renderer2d)
    : renderer2d(renderer2d)
{
    engine =  csQueryRegistry<iEngine> (psCSSetup::object_reg);
    stringSet = csQueryRegistryTagInterface<iShaderVarStringSet>(psCSSetup::object_reg,
                "crystalspace.shader.variablenameset");
    globalStringSet = csQueryRegistryTagInterface<iStringSet>
                      (psCSSetup::object_reg, "crystalspace.shared.stringset");
    view = parentView;

    killTime = -1;
    animScaling = 1;
    autoScale = SCALING_NONE;
    isAlive = true;
    birth = 0;

    zFunc = CS_ZBUF_TEST;

    anchor = 0;

    scale = 1.0f;
    aspect = 1.0f;

    effectsCollection = engine->GetCollection("effects");
    keyFrames.AttachNew(new psEffectObjKeyFrameGroup);

    baseScale = 1.0f;
}

psEffectObj::~psEffectObj()
{
    if(mesh)
    {
        if(anchorMesh && isAlive)
            mesh->QuerySceneNode()->SetParent(0);

        engine->RemoveObject(mesh);
    }
    // note that we don't delete the mesh factory since that is shared
    // between all instances of this effect object and will be cleaned up by
    // CS's smart pointer system
}

bool psEffectObj::Load(iDocumentNode* node, iLoaderContext* /*ldr_context*/)
{

    csRef<iDocumentNode> dataNode;

    // birth
    dataNode = node->GetNode("birth");
    if(dataNode)
    {
        birth = dataNode->GetContentsValueAsFloat();
        if(birth > 0)
            isAlive = false;
    }

    // death
    dataNode = node->GetNode("death");
    if(dataNode)
    {
        killTime = dataNode->GetContentsValueAsInt();
    }
    else
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Effect obj %s is missing a <death> tag.  If you want an infinite effect use <death>none</death>\n", name.GetData());
        return false; // effect is invalid without a death tag
    }

    // time scaling
    dataNode = node->GetNode("scale");
    if(dataNode)
    {
        if(dataNode->GetAttributeValueAsBool("death"))
            autoScale |= SCALING_DEATH;
        if(dataNode->GetAttributeValueAsBool("birth"))
            autoScale |= SCALING_BIRTH;
        if(dataNode->GetAttributeValueAsBool("frames"))
            autoScale |= SCALING_FRAMES;
        else if(dataNode->GetAttributeValueAsBool("loop"))
            autoScale |= SCALING_LOOP;
    }

    // anchor
    dataNode = node->GetNode("attach");
    if(dataNode)
        SetAnchorName(dataNode->GetContentsValue());

    // render priority
    dataNode = node->GetNode("priority");
    if(dataNode)
        priority = engine->GetRenderPriority(dataNode->GetContentsValue());
    else
        priority = engine->GetAlphaRenderPriority();

    // z func
    if(node->GetNode("znone"))
        zFunc = CS_ZBUF_NONE;
    else if(node->GetNode("zfill"))
        zFunc = CS_ZBUF_FILL;
    else if(node->GetNode("zuse"))
        zFunc = CS_ZBUF_USE;
    else if(node->GetNode("ztest"))
        zFunc = CS_ZBUF_TEST;

    // mix mode
    dataNode = node->GetNode("mixmode");
    if(dataNode)
    {
        csString mix = dataNode->GetContentsValue();
        mix.Downcase();
        if(mix == "copy")
            mixmode = CS_FX_COPY;
        else if(mix == "mult")
            mixmode = CS_FX_MULTIPLY;
        else if(mix == "mult2")
            mixmode = CS_FX_MULTIPLY2;
        else if(mix == "alpha")
            mixmode = CS_FX_ALPHA;
        else if(mix == "transparent")
            mixmode = CS_FX_TRANSPARENT;
        else if(mix == "destalphaadd")
            mixmode = CS_FX_DESTALPHAADD;
        else if(mix == "srcalphaadd")
            mixmode = CS_FX_SRCALPHAADD;
        else if(mix == "premultalpha")
            mixmode = CS_FX_PREMULTALPHA;
        else
            mixmode = CS_FX_ADD;
    }
    else
        mixmode = CS_FX_ADD;

    // direction
    dataNode = node->GetNode("dir");
    if(dataNode)
    {
        csString dirName = dataNode->GetContentsValue();
        dirName.Downcase();
        if(dirName == "target")
            dir = DT_TARGET;
        else if(dirName == "origin")
            dir = DT_ORIGIN;
        else if(dirName == "totarget")
            dir = DT_TO_TARGET;
        else if(dirName == "camera")
            dir = DT_CAMERA;
        else if(dirName == "billboard")
            dir = DT_BILLBOARD;
        else
            dir = DT_NONE;
    }
    else
        dir = DT_NONE;

    // KEYFRAMES
    csRef<iDocumentNodeIterator> xmlbinds;
    xmlbinds = node->GetNodes("keyFrame");
    csRef<iDocumentNode> keyNode;

    animLength = 0;
    psEffectObjKeyFrame* prevKeyFrame = 0;
    while(xmlbinds->HasNext())
    {
        keyNode = xmlbinds->Next();
        psEffectObjKeyFrame* keyFrame = new psEffectObjKeyFrame(keyNode, prevKeyFrame);
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

void psEffectObj::SetAnimationScaling(float s)
{
    if(autoScale & SCALING_DEATH)
        killTime *= s;

    if(autoScale & SCALING_BIRTH)
        birth *= s;

    animScaling = s;
}


bool psEffectObj::Render(const csVector3 & /*up*/)
{
    return false;
}

bool psEffectObj::SetScaling(float scale, float aspect)
{
    this->scale = scale;
    this->aspect = aspect;
    return true;
}

bool psEffectObj::SetFrameParamScalings(const float* scale)
{
    return keyFrames->SetFrameParamScalings(scale);
}

bool psEffectObj::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady() || !anchorMesh->GetMovable()->GetSectors()->GetCount())  // wait for anchor to be ready
        return true;

    const static csMatrix3 UP_FIX(1,0,0,   0,0,1,  0,1,0);
    const static csMatrix3 billboardFix = csXRotMatrix3(-3.14f/2.0f);

    iMovable* anchorMovable = anchorMesh->GetMovable();
    iMovable* meshMovable = mesh->GetMovable();

    csVector3 anchorPosition = anchorMovable->GetFullPosition();

    life += elapsed;
    if(life > animLength && killTime <= 0)
    {
        life %= animLength;
        if(!life)
        {
            life += animLength;
        }
    }

    isAlive |= (life >= birth);

    if(isAlive)
    {
        meshMovable->SetSector(anchorMovable->GetSectors()->Get(0));
        meshMovable->SetPosition(anchorPosition);
        if(dir == DT_NONE)
        {
            matBase = anchorMovable->GetFullTransform().GetT2O();
        }
    }

    csMatrix3 matTransform;
    if(keyFrames->GetSize() == 0)
    {
        if(dir == DT_CAMERA)
        {
            // note that this is *very* expensive
            csVector3 camDir = -view->GetCamera()->GetTransform().GetO2TTranslation()
                               + anchorPosition;
            csReversibleTransform rt;
            rt.LookAt(camDir, csVector3(0.f,1.f,0.f));
            matBase = rt.GetT2O() * UP_FIX;
        }
        else if(dir == DT_BILLBOARD)
        {
            matBase = view->GetCamera()->GetTransform().GetT2O() * billboardFix;
        }

        baseScale = scale;
        matTransform = matBase * baseScale;
    }
    else
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = (currKeyFrame + 1) % keyFrames->GetSize();

        // grab and lerp values - expensive
        float lerpfactor = LERP_FACTOR;
        csVector3 lerpRot = LERP_VEC_KEY(KA_ROT,lerpfactor);
        csVector3 lerpSpin = LERP_VEC_KEY(KA_SPIN,lerpfactor);
        csVector3 objOffset = LERP_VEC_KEY(KA_POS,lerpfactor);

        // calculate rotation from lerped values - expensive
        csMatrix3 matRot = csZRotMatrix3(lerpRot.z) * csYRotMatrix3(lerpRot.y) * csXRotMatrix3(lerpRot.x);
        if(dir != DT_CAMERA && dir != DT_BILLBOARD)
        {
            matRot *= matBase;
        }

        // calculate new position
        csVector3 newPos = matRot * csVector3(-objOffset.x, objOffset.y, -objOffset.z);

        if(dir == DT_CAMERA)
        {
            // note that this is *very* expensive - again
            csVector3 camDir = -view->GetCamera()->GetTransform().GetO2TTranslation()
                               + anchorPosition + newPos;
            csReversibleTransform rt;
            rt.LookAt(camDir, csVector3(sinf(lerpSpin.y),cosf(lerpSpin.y),0.f));
            matBase = rt.GetT2O() * UP_FIX;
            newPos = rt.GetT2O() * newPos;

            // rotate and spin should have no effect on the transform when we want it to face the camera
            matTransform = matBase;
        }
        else if(dir == DT_BILLBOARD)
        {
            matBase = view->GetCamera()->GetTransform().GetT2O() * billboardFix;
            matTransform = matBase;
        }
        else
        {
            matTransform = matRot;
            matTransform *= csZRotMatrix3(lerpSpin.z) * csYRotMatrix3(lerpSpin.y) * csXRotMatrix3(lerpSpin.x);
        }

        // SCALE
        baseScale = LERP_KEY(KA_SCALE,lerpfactor) * scale;
        matTransform *= baseScale;

        // adjust position
        meshMovable->SetPosition(anchorPosition+newPos);
    }

    // set new transform
    meshMovable->SetTransform(matTransform);
    meshMovable->UpdateMove();

    if(killTime > 0)
    {
        killTime -= elapsed;
        if(killTime <= 0)
            return false;
    }

    return true;
}

void psEffectObj::CloneBase(psEffectObj* newObj) const
{
    newObj->name = name;
    newObj->materialName = materialName;
    newObj->killTime = killTime;
    newObj->life = 0;
    newObj->animLength = animLength;
    newObj->anchorName = anchorName;
    newObj->meshFact = meshFact;
    newObj->objUp = objUp;
    newObj->matBase = matBase;
    newObj->matUp = matUp;
    newObj->birth = birth;
    newObj->isAlive = isAlive;
    newObj->zFunc = zFunc;
    newObj->dir = dir;
    newObj->priority = priority;
    newObj->mixmode = mixmode;
    newObj->autoScale = autoScale;
    newObj->animScaling = animScaling;

    newObj->keyFrames = keyFrames->Clone();
}

psEffectObj* psEffectObj::Clone() const
{
    psEffectObj* newObj = new psEffectObj(view, renderer2d);
    CloneBase(newObj);

    return newObj;
}

bool psEffectObj::AttachToAnchor(psEffectAnchor* newAnchor)
{
    if(mesh && newAnchor->GetMesh())
    {
        anchor = newAnchor;
        anchorMesh = anchor->GetMesh();

        /*
        if (isAlive)
            anchorMesh->GetChildren()->Add(mesh);
        anchorMesh->GetMovable()->UpdateMove();
        mesh->GetMovable()->SetPosition(csVector3(0,0,0));
        mesh->GetMovable()->UpdateMove();
        */
        return true;
    }
    return false;
}

size_t psEffectObj::FindKeyFrameByTime(csTicks time) const
{
    if(autoScale & SCALING_FRAMES)
    {
        time /= animScaling;
    }
    else if(autoScale & SCALING_LOOP)
    {
        time %= animLength;
    }

    for(size_t a = keyFrames->GetSize(); a > 0; --a)
    {
        if(keyFrames->Get(a-1)->time < time)
            return (a-1);
    }
    return 0;
}

bool psEffectObj::FindNextKeyFrameWithAction(size_t startFrame, size_t action, size_t &index) const
{
    for(size_t a = startFrame; a < keyFrames->GetSize(); a++)
    {
        if(keyFrames->Get(a)->specAction.IsBitSet(action))
        {
            index = a;
            return true;
        }
    }
    return false;
}

void psEffectObj::FillInLerps()
{
    // this code is crap, but doing it this way allows everything else to be decently nice, clean, and efficient
    size_t a,b,nextIndex;

    for(size_t k=0; k<psEffectObjKeyFrame::KA_VEC_COUNT; ++k)
    {
        a = 0;
        size_t i = k - psEffectObjKeyFrame::KA_COUNT;
        while(FindNextKeyFrameWithAction(a+1, k, nextIndex))
        {
            psEffectObjKeyFrame* keyA = keyFrames->Get(a);
            psEffectObjKeyFrame* keyNext = keyFrames->Get(nextIndex);
            for(b=a+1; b<nextIndex; ++b)
            {
                psEffectObjKeyFrame* keyB = keyFrames->Get(b);
                switch(lerpTypes[k])
                {
                    case psEffectObjKeyFrame::IT_FLOOR:
                    {
                        if(k < psEffectObjKeyFrame::KA_COUNT)
                            keyB->actions[k] = keyA->actions[k];
                        else
                            keyB->vecActions[i] = keyA->vecActions[i];
                        break;
                    }
                    case psEffectObjKeyFrame::IT_CEILING:
                    {
                        if(k < psEffectObjKeyFrame::KA_COUNT)
                            keyB->actions[k] = keyNext->actions[k];
                        else
                            keyB->vecActions[i] = keyNext->vecActions[i];
                        break;
                    }
                    case psEffectObjKeyFrame::IT_LERP:
                    {
                        float lerpFact = lerpFactor(keyA->time, keyNext->time, keyB->time);

                        if(k < psEffectObjKeyFrame::KA_COUNT)
                            keyB->actions[k] = lerp(keyA->actions[k], keyNext->actions[k], lerpFact);
                        else
                            keyB->vecActions[i] = lerpVec(keyA->vecActions[i], keyNext->vecActions[i], lerpFact);
                        break;
                    }
                }
            }
            a = nextIndex;
        }

        // no matter what the interpolation type (as long as we have one), just clamp the end
        if(lerpTypes[k] != psEffectObjKeyFrame::IT_NONE && a < keyFrames->GetSize())
        {
            psEffectObjKeyFrame* keyA = keyFrames->Get(a);
            for(b = a + 1; b < keyFrames->GetSize(); ++b)
            {
                psEffectObjKeyFrame* keyB = keyFrames->Get(b);
                if(k < psEffectObjKeyFrame::KA_COUNT)
                    keyB->actions[k] = keyA->actions[k];
                else
                    keyB->vecActions[i] = keyA->vecActions[i];
            }
        }
    }
}

csMatrix3 psEffectObj::BuildRotMatrix(const csVector3 &up) const
{
    csVector3 forward = csVector3(0, 0, 1);
    csVector3 right = up % forward;
    return csMatrix3(right.x, right.y, right.z, up.x, up.y, up.z, forward.x, forward.y, forward.z);
}

void psEffectObj::Show(bool value)
{
    if(mesh)
    {
        if(value)
            mesh->GetFlags().Reset(CS_ENTITY_INVISIBLEMESH);
        else
            mesh->GetFlags().Set(CS_ENTITY_INVISIBLEMESH);
    }
}
