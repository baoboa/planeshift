/*
* Author: Andrew Robberts
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <iutil/objreg.h>

#include <cal3d/cal3d_wrapper.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "effects/pseffectmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscal3dcallback.h"
#include "pscelclient.h"
#include "globals.h"

psCal3DCallbackLoader::psCal3DCallbackLoader(iObjectRegistry * objreg)
    : scfImplementationType(this)
{
    objreg->Register(this, "Cal3DCallbackLoader");
}

psCal3DCallbackLoader::~psCal3DCallbackLoader()
{
}

csPtr<iBase> psCal3DCallbackLoader::Parse(iDocumentNode* node, iStreamSource*, iLoaderContext* /*ldr_context*/,
                                          iBase* context)
{
    // turn the iBase into an iSpriteCal3DFactoryState so we can use it
    csRef<iMeshFactoryWrapper> meshFactoryWrapper =  scfQueryInterface<iMeshFactoryWrapper > ( context);
    csRef<iSpriteCal3DFactoryState> fcal3dsprite =  scfQueryInterface<iSpriteCal3DFactoryState> ( meshFactoryWrapper );

    if (fcal3dsprite)
    {
        csRef<iDocumentNodeIterator> xmlBinds;
        csRef<iDocumentNode> dataNode;
        xmlBinds = node->GetNodes("callback");
        while (xmlBinds->HasNext())
        {
            dataNode = xmlBinds->Next();
            
            csString type = dataNode->GetAttributeValue("type");
      
            csString anim = dataNode->GetAttributeValue("anim");
            if (anim)
            {
                float interval = dataNode->GetAttributeValueAsFloat("interval");
                if (interval <= 0.0f)
                    interval = 0.5f;
            
                if (type == "effect")
                    fcal3dsprite->RegisterAnimCallback(anim.GetData(), (CalAnimationCallback *)(new psCal3DCallbackEffect(dataNode)), interval);
            }
        }
        return context;
    }
    else
    {
        printf("Warning: Using Cal3DCallbackLoader on something other than a sprcal3d factory.");
        return 0;
    }
    return 0;
}

psCal3DCallbackEffect::psCal3DCallbackEffect(iDocumentNode * node)
{
    csRef<iDocumentNode> dataNode;

    // effect name
    dataNode = node->GetNode("effect");
    if (dataNode)
        effectName = dataNode->GetContentsValue();

    // trigger time
    triggerTime = node->GetAttributeValueAsFloat("time");

    // reset per cycle
    dataNode = node->GetNode("resetperanim");
    if (dataNode)
        resetPerCycle = true;
    else
        resetPerCycle = false;

    // kill with anim death
    dataNode = node->GetNode("diewithanim");
    if (dataNode)
        killWithAnimEnd = true;
    else
        killWithAnimEnd = false;
    
    lastUpdateTime = 0.0f;
    effectID = 0;
}

psCal3DCallbackEffect::~psCal3DCallbackEffect()
{
}

void psCal3DCallbackEffect::AnimationUpdate(float anim_time, CalModel* /*model*/, void* userData)
{
    if (resetPerCycle)
    {
        if (anim_time < lastUpdateTime)
            hasTriggered = false;
    }
    if (anim_time >= triggerTime && !hasTriggered)
    {
        hasTriggered = true;
        GEMClientObject* gemObj = (GEMClientObject*)userData;
        if (gemObj)
        {
            effectID = psengine->GetEffectManager()->RenderEffect(effectName, csVector3(0,0,0), gemObj->GetMesh());
        }

    }
    lastUpdateTime = anim_time;
}

void psCal3DCallbackEffect::AnimationComplete(CalModel* /*model*/, void* /*userData*/)
{
    hasTriggered = false;
    
    if (killWithAnimEnd)
    {
        psengine->GetEffectManager()->DeleteEffect(effectID);
        effectID = 0;
    }
}

