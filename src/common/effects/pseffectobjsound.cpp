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

#include <iengine/camera.h>
#include <cstool/csview.h>
#include <csutil/xmltiny.h>
#include <iutil/plugin.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>

#include <imap/loader.h>

#include "effects/pseffectobjsound.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/pscssetup.h"
#include "util/log.h"

#include "isoundmngr.h"


psEffectObjSound::psEffectObjSound(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    soundManager = csQueryRegistryOrLoad<iSoundManager>(psCSSetup::object_reg, "iSoundManager");
    if(!soundManager)
    {
        // if the main sound manager is not found load the dummy plugin
        soundManager = csQueryRegistryOrLoad<iSoundManager>(psCSSetup::object_reg, "crystalspace.planeshift.sound.dummy");
        if(!soundManager)
        {
            csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Could not find plugin iSoundManager.\n");
        }
    }

    soundID = 0;
    playedOnce = false;
}

psEffectObjSound::~psEffectObjSound()
{
    // if the sound is not active the sound manager does nothing
    soundManager->StopSound(soundID);
}

bool psEffectObjSound::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    if(!psEffectObj::Load(node, ldr_context))
        return false;

    // get the attributes
    name.Clear();
    soundName.Clear();
    minDist = 25.0f;
    maxDist = 100000.0f;
    loop = false;
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
        else if(attrName == "resource")
            soundName = attr->GetValue();
        else if(attrName == "loop")
            loop = attr->GetValueAsBool();
    }

    if(!loop && killTime <= 0)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Can't have a looping sound effect without a death.\n");
        return false;
    }

    csRef<iDocumentNode> dataNode;

    // min distance
    dataNode = node->GetNode("mindist");
    if(dataNode)
    {
        minDist = dataNode->GetContentsValueAsFloat();
    }

    // max distance
    dataNode = node->GetNode("maxdist");
    if(dataNode)
    {
        maxDist = dataNode->GetContentsValueAsFloat();
    }

    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    return PostSetup();
}

bool psEffectObjSound::Render(const csVector3 & /*up*/)
{
    static unsigned long nextUniqueID = 0;
    effectID += nextUniqueID++;

    isAlive = false;
    return true;
}

bool psEffectObjSound::AttachToAnchor(psEffectAnchor* newAnchor)
{
    if(newAnchor && newAnchor->GetMesh())
        anchorMesh = newAnchor->GetMesh();
    anchor = newAnchor;
    return true;
}

bool psEffectObjSound::Update(csTicks elapsed)
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

    if(killTime > 0)
    {
        killTime -= elapsed;
        if(killTime <= 0)
            return false;
    }

    // getting source position
    csVector3 soundPos = anchorMesh->GetMovable()->GetPosition();

    if(keyFrames->GetSize() > 0)
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = (currKeyFrame + 1) % keyFrames->GetSize();

        // position
        soundPos += LERP_VEC_KEY(KA_POS,LERP_FACTOR);
    }

    // checking actual state of the sound and updating sound position
    if(isAlive)
    {
        isAlive = soundManager->SetSoundSource(soundID, soundPos);
    }

    // playing sound
    if(!isAlive && life >= birth)
    {
        if(loop || !playedOnce)
        {
            soundID = soundManager->PlaySound(soundName, loop, iSoundManager::EFFECT_SNDCTRL,
                                              soundPos, csVector3(0,0,0), minDist, maxDist);

            if(soundID != 0) // sound not played
            {
                isAlive = true;
                playedOnce = true;
            }
        }
    }

    return true;
}

psEffectObj* psEffectObjSound::Clone() const
{
    psEffectObjSound* newObj = new psEffectObjSound(view, renderer2d);
    CloneBase(newObj);

    // simp mesh specific
    newObj->soundName = soundName;

    newObj->minDist = minDist;
    newObj->maxDist = maxDist;

    newObj->loop = loop;

    return newObj;
}

bool psEffectObjSound::PostSetup()
{
    return true;
}
