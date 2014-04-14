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

#ifndef PS_EFFECT_OBJ_MESH_HEADER
#define PS_EFFECT_OBJ_MESH_HEADER

#include "pseffectobj.h"
#include <imesh/sprite3d.h>

class psEffect2DRenderer;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjMesh : public psEffectObj
{
public:

    psEffectObjMesh(iView* parentView, psEffect2DRenderer* renderer2d);
    ~psEffectObjMesh();

    // inheritted function overloads
    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    bool Render(const csVector3 &up);
    bool Update(csTicks elapsed);
    psEffectObj* Clone() const;

private:

    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    bool PostSetup(iLoaderContext* ldr_context);

    csString factName;
    csRef<iThreadReturn> factory;
    csRef<iSprite3DState> sprState;
};

/** @} */

#endif
