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
#include <cstool/csview.h>
#include <imesh/objmodel.h>
#include <csutil/flags.h>
#include <csgeom/tri.h>
#include <imesh/nullmesh.h>
#include <iengine/sector.h>

#include "effects/pseffectobjdecal.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"


#ifdef PS_EFFECT_ENABLE_DECAL
psEffectObjDecal::psEffectObjDecal(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    decal = NULL;
    // get the decal manager
    decalMgr = csLoadPluginCheck<iDecalManager> (psCSSetup::object_reg, "crystalspace.decal.manager");
}

psEffectObjDecal::~psEffectObjDecal()
{
    if(decal)
        decalMgr->DeleteDecal(decal);
}

bool psEffectObjDecal::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    if(!decalMgr)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "No decal plugin.\n");
        return false;
    }

    // get the attributes
    name.Clear();
    materialName.Clear();
    float polygonNormalThreshold = -0.05f;
    float decalOffset = 0.02f;
    csVector2 minTexCoord(0,0);
    csVector2 maxTexCoord(1,1);
    bool hasTopClip = true;
    float topClipScale = 0.5f;
    bool hasBottomClip = true;
    float bottomClipScale = 0.5f;
    float perpendicularFaceThreshold = 0.05f;
    float perpendicularFaceOffset = 0.01f;

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
        else if(attrName == "polygonnormalthreshold")
            polygonNormalThreshold = attr->GetValueAsFloat();
        else if(attrName == "decaloffset")
            decalOffset = attr->GetValueAsFloat();
    }

    csRef<iDocumentNode> dataNode = node->GetNode("texCoords");
    if(dataNode)
    {
        minTexCoord.Set(dataNode->GetAttributeValueAsFloat("minU"), dataNode->GetAttributeValueAsFloat("minV"));
        maxTexCoord.Set(dataNode->GetAttributeValueAsFloat("maxU"), dataNode->GetAttributeValueAsFloat("maxV"));
    }

    dataNode = node->GetNode("topClip");
    if(dataNode)
    {
        hasTopClip = dataNode->GetAttributeValueAsBool("enabled");
        topClipScale = dataNode->GetAttributeValueAsFloat("scaleDist");
    }

    dataNode = node->GetNode("bottomClip");
    if(dataNode)
    {
        hasBottomClip = dataNode->GetAttributeValueAsBool("enabled");
        bottomClipScale = dataNode->GetAttributeValueAsFloat("scaleDist");
    }

    dataNode = node->GetNode("perpendicularFace");
    if(dataNode)
    {
        perpendicularFaceThreshold = dataNode->GetAttributeValueAsFloat("threshold");
        perpendicularFaceOffset = dataNode->GetAttributeValueAsFloat("offset");
    }

    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    // load material
    csRef<iMaterialWrapper> mat = effectsCollection->FindMaterial(materialName);
    if(!mat)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    // create decal template
    decalTemplate = decalMgr->CreateDecalTemplate(mat);
    decalTemplate->SetPolygonNormalThreshold(polygonNormalThreshold);
    decalTemplate->SetDecalOffset(decalOffset);
    decalTemplate->SetTopClipping(hasTopClip, topClipScale);
    decalTemplate->SetBottomClipping(hasBottomClip, bottomClipScale);
    decalTemplate->SetTexCoords(minTexCoord, maxTexCoord);
    decalTemplate->SetPerpendicularFaceThreshold(perpendicularFaceThreshold);
    decalTemplate->SetPerpendicularFaceOffset(perpendicularFaceOffset);

    if(!psEffectObj::Load(node, ldr_context))
        return false;

    return PostSetup();
}

bool psEffectObjDecal::Render(const csVector3 &up)
{
    static unsigned int uniqueID = 0;
    csString meshName = "effect_decal_";
    meshName += uniqueID++;

    // create a nullmesh as placeholder
    mesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", meshName);
    mesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (mesh->GetMeshObject());
    if(!state)
    {
        Error1("No NullMeshState.");
        return false;
    }
    state->SetRadius(1.0);

    // do the up vector
    objUp = up;
    csReversibleTransform rt;
    rt.LookAt(csVector3(up.x, up.z, up.y), csVector3(0,2,1));
    matUp = rt.GetT2O();
    matBase = matUp;

    // common flags
    decalTemplate->SetZBufMode(zFunc);
    decalTemplate->SetRenderPriority(priority);
    decalTemplate->SetMixMode(mixmode);

    decal = 0;
    return true;
}

bool psEffectObjDecal::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    float heightScale = 1.0f;
    if(keyFrames->GetSize() > 0)
        heightScale = LERP_KEY(KA_HEIGHT,LERP_FACTOR);

    const csReversibleTransform t = mesh->GetMovable()->GetFullTransform();
    const csVector3 newPos = t.GetOrigin();
    const csVector3 newUp = t.GetFront();
    const csVector3 newNormal = t.GetUp();
    const float newWidth = baseScale;
    const float newHeight = baseScale * heightScale;
    if(!decal
            || newPos != pos
            || newUp != up
            || newNormal != normal
            || newWidth != width
            || newHeight != height)
    {
        decal = decalMgr->CreateDecal(decalTemplate, mesh->GetMovable()->GetSectors()->Get(0), newPos, newUp,
                                      newNormal, newWidth, newHeight, decal);

        pos = newPos;
        up = newUp;
        normal = newNormal;
        width = newWidth;
        height = newHeight;
    }

    return true;
}

psEffectObj* psEffectObjDecal::Clone() const
{
    psEffectObjDecal* newObj = new psEffectObjDecal(view, renderer2d);
    CloneBase(newObj);

    newObj->decalMgr = decalMgr;
    newObj->decalTemplate = decalTemplate;

    return newObj;
}

bool psEffectObjDecal::PostSetup()
{
    return true;
}

#endif // PS_EFFECT_ENABLE_DECAL

