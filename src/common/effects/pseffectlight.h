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

#ifndef PS_EFFECT_LIGHT_HEADER
#define PS_EFFECT_LIGHT_HEADER

#include <csgeom/matrix3.h>
#include <csutil/cscolor.h>
#include <csutil/scf_implementation.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iutil/virtclk.h>

class csColor;
struct iEngine;
struct iLight;
struct iMeshWrapper;

/**
 * \addtogroup common_effects
 * @{ */

class psLight
{
public:
    psLight(iObjectRegistry* object_reg);
    ~psLight();

    unsigned int AttachLight(const char* name, const csVector3 &pos,
                             float radius, const csColor &colour, iMeshWrapper* mw);
    bool Update();

private:
    csRef<iLight> light;
    csRef<iVirtualClock> vclock;
    csRef<iEngine> engine;

    csTicks lastTime;
    csColor baseColour;
    bool dim;
    uint sequencenum;
    uint stepsPerSecond;
    uint stepsPerCycle;
    uint dimStrength;
};

/** @} */

#endif
