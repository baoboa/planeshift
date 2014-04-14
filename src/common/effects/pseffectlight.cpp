/*
* Author: Andrew Robberts
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/cscolor.h>
#include <iengine/engine.h>
#include <iengine/light.h>
#include <iengine/halo.h>
#include <iengine/mesh.h>
#include <iengine/scenenode.h>
#include <iutil/object.h>
#include <iutil/objreg.h>

#include "pseffectlight.h"

// used for generating a unique ID
static unsigned int genUniqueID = 0;

psLight::psLight(iObjectRegistry* object_reg)
{
    vclock = csQueryRegistry<iVirtualClock>(object_reg);
    engine = csQueryRegistry<iEngine>(object_reg);
    dim = true;
    sequencenum = 0;
    stepsPerSecond = 60;
    stepsPerCycle = 60;
    dimStrength = 100;
}

psLight::~psLight()
{
    light->QuerySceneNode()->SetParent(0);
    light->GetMovable()->UpdateMove();
    engine->RemoveObject(light);
}

unsigned int psLight::AttachLight(const char* name, const csVector3 &pos,
                                  float radius, const csColor &colour, iMeshWrapper* mw)
{
    baseColour = colour;
    csString lightName = name;
    lightName.Append("_%u", genUniqueID);
    light = engine->CreateLight(lightName, pos, radius, colour, CS_LIGHT_DYNAMICTYPE_DYNAMIC);
    light->SetAttenuationMode(CS_ATTN_INVERSE);

    // Attach to mesh.
    light->QuerySceneNode()->SetParent(mw->QuerySceneNode());
    mw->GetMovable()->UpdateMove();

    lastTime = vclock->GetCurrentTicks();

    return ++genUniqueID;
}

bool psLight::Update()
{
    int advanceSteps = int((vclock->GetCurrentTicks() - lastTime) / (1000.0f/stepsPerSecond));

    if(0 < advanceSteps)
    {
        lastTime = vclock->GetCurrentTicks();

        if(dim)
        {
            if(sequencenum < stepsPerCycle/2)
            {
                csColor n = light->GetColor();

                for(int i=0; sequencenum < stepsPerCycle/2 && i<advanceSteps; ++i)
                {
                    n.red -= baseColour.red/dimStrength;
                    n.green -= baseColour.green/dimStrength;
                    n.blue -= baseColour.blue/dimStrength;
                    ++sequencenum;
                }

                light->SetColor(n);
            }
            else
            {
                dim = false;
            }
        }
        else
        {
            if(sequencenum > 0)
            {
                csColor n = light->GetColor();

                for(int i=0; sequencenum > 0 && i<advanceSteps; ++i)
                {
                    n.red += baseColour.red/dimStrength;
                    n.green += baseColour.green/dimStrength;
                    n.blue += baseColour.blue/dimStrength;
                    --sequencenum;
                }

                light->SetColor(n);
            }
            else
            {
                dim = true;
            }
        }
    }

    return true;
}
