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

#ifndef PS_EFFECT_OBJ_DECAL_HEADER
#define PS_EFFECT_OBJ_DECAL_HEADER

#include "pseffectobj.h"

#define PS_EFFECT_ENABLE_DECAL

#ifdef PS_EFFECT_ENABLE_DECAL
#include <ivaria/decal.h>

class psEffect2DRenderer;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjDecal : public psEffectObj
{
public:
    psEffectObjDecal(iView* parentView, psEffect2DRenderer* renderer2d);
    virtual ~psEffectObjDecal();

    // inheritted function overloads
    virtual bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    virtual bool Render(const csVector3 &up);
    virtual bool Update(csTicks elapsed);
    virtual psEffectObj* Clone() const;


protected:

    /**
     * Performs the post setup (after the effect obj has been loaded).
     *
     * Things like create mesh factory, etc are initialized here.
     */
    virtual bool PostSetup();

    csRef<iDecalManager>    decalMgr;
    iDecal*                 decal;
    csRef<iDecalTemplate>   decalTemplate;

    // cached values of the current decal so we know if we have to refresh the decal
    csVector3           pos;
    csVector3           up;
    csVector3           normal;
    float               width;
    float               height;
};

#endif // PS_EFFECT_ENABLE_DECAL

/** @} */

#endif
