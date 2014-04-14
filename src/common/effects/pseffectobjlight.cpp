/*
* Author: Matthieu Kraus
*
* Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "pseffectobjlight.h"
#include "pseffectanchor.h"

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/scenenode.h>
#include <iengine/sector.h>

psEffectObjLight::psEffectObjLight(iView* parentView, psEffect2DRenderer* renderer2d)
    :psEffectObj(parentView, renderer2d)
{
}

psEffectObjLight::~psEffectObjLight()
{
    if(light.IsValid())
    {
        light->QuerySceneNode()->SetParent(0);
        light->GetMovable()->UpdateMove();
        engine->RemoveObject(light);
        if(anchorMesh)
            anchorMesh->GetMovable()->UpdateMove();
    }
}

bool psEffectObjLight::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    if(!psEffectObj::Load(node, ldr_context))
        return false;

    radius = 10.f;
    type = CS_LIGHT_POINTLIGHT;
    mode = CS_ATTN_INVERSE;
    color = csColor(0);
    offset = csVector3(0);

    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "radius")
        {
            radius = attr->GetValueAsFloat();
        }
        else if(attrName == "mode")
        {
            csString modeName = attr->GetValue();
            modeName.Downcase();
            if(modeName == "linear")
                mode = CS_ATTN_LINEAR;
            else if(modeName == "realistic")
                mode = CS_ATTN_REALISTIC;
            else if(modeName == "clq")
                mode = CS_ATTN_CLQ;
        }
        else if(attrName == "type")
        {
            csString typeName = attr->GetValue();
            typeName.Downcase();
            if(typeName == "spot")
                type = CS_LIGHT_SPOTLIGHT;
            else if(typeName == "directional")
                type = CS_LIGHT_DIRECTIONAL;
        }
        else if(attrName == "r")
        {
            color.red = attr->GetValueAsFloat();
        }
        else if(attrName == "g")
        {
            color.green = attr->GetValueAsFloat();
        }
        else if(attrName == "b")
        {
            color.blue = attr->GetValueAsFloat();
        }
        else if(attrName == "x")
        {
            offset.x = attr->GetValueAsFloat();
        }
        else if(attrName == "y")
        {
            offset.y = attr->GetValueAsFloat();
        }
        else if(attrName == "z")
        {
            offset.z = attr->GetValueAsFloat();
        }
    }

    return true;
}

bool psEffectObjLight::AttachToAnchor(psEffectAnchor* anchor)
{
    if(!anchor)
        return false;

    csRef<iMeshWrapper> mw = anchor->GetMesh();
    if(light && mw)
    {
        light->QuerySceneNode()->SetParent(mw->QuerySceneNode());
        mw->GetMovable()->UpdateMove();
        if(anchorMesh)
            anchorMesh->GetMovable()->UpdateMove();

        this->anchor = anchor;
        anchorMesh = mw;
        return true;
    }
    else
    {
        csPrintf("failed to attach light\n");
    }
    return false;
}

bool psEffectObjLight::Render(const csVector3 & /*up*/)
{
    if(light.IsValid())
        return true;

    light = engine->CreateLight(0, offset, radius, color, CS_LIGHT_DYNAMICTYPE_DYNAMIC);
    if(!light.IsValid())
    {
        csPrintf("failed to create effect light\n");
        return false;
    }

    light->SetAttenuationMode(mode);
    light->SetType(type);
    return true;
}

bool psEffectObjLight::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady() || !anchorMesh->GetMovable()->GetSectors()->GetCount())  // wait for anchor to be ready
        return true;

    life += elapsed;
    if(life > animLength && killTime <= 0)
    {
        life %= animLength;
        if(!life)
        {
            life += animLength;
        }
    }

    if(keyFrames->GetSize() == 0)
    {
        light->SetColor(color);
    }
    else
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = (currKeyFrame + 1) % keyFrames->GetSize();
        float lerpfactor = LERP_FACTOR;
        csVector3 newColor = LERP_VEC_KEY(KA_COLOUR,lerpfactor)*255.f;
        csColor c(newColor.x, newColor.y, newColor.z);
        light->SetColor(c);
    }

    if(killTime > 0)
    {
        killTime -= elapsed;
        if(killTime <= 0)
            return false;
    }

    return true;
}

psEffectObj* psEffectObjLight::Clone() const
{
    psEffectObjLight* newLight = new psEffectObjLight(view, renderer2d);
    CloneBase(newLight);

    newLight->radius = radius;
    newLight->type = type;
    newLight->mode = mode;
    newLight->color = color;
    newLight->offset = offset;
    return newLight;
}

