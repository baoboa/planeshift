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

#ifndef PS_EFFECT_OBJ_LIGHT_HEADER
#define PS_EFFECT_OBJ_LIGHT_HEADER

#include <cssysdef.h>
#include <iengine/light.h>
#include <csutil/cscolor.h>

#include "pseffectobj.h"

/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjLight : public psEffectObj
{
public:

    psEffectObjLight(iView* parentView, psEffect2DRenderer* renderer2d);
    ~psEffectObjLight();

    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    bool Render(const csVector3 &up);
    bool Update(csTicks elapsed);
    psEffectObj* Clone() const;
    bool AttachToAnchor(psEffectAnchor* anchor);

protected:
    csRef<iLight> light;
    float radius;
    csLightType type;
    csLightAttenuationMode mode;
    csColor color;
    csVector3 offset;
};

/** @} */

#endif
