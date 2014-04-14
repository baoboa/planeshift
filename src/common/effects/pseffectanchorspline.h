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

#ifndef PS_EFFECT_ANCHOR_SPLINE_HEADER
#define PS_EFFECT_ANCHOR_SPLINE_HEADER

#include "pseffectanchor.h"

#include <csgeom/spline.h>

/**
 * \addtogroup common_effects
 * @{ */

class psEffectAnchorSpline : public psEffectAnchor
{
public:

    psEffectAnchorSpline();
    ~psEffectAnchorSpline();

    // inheritted function overloads
    bool Load(iDocumentNode* node);
    bool Create(const csVector3 &offset, iMeshWrapper* posAttach, bool rotateWithMesh = false);
    bool Update(csTicks elapsed);
    psEffectAnchor* Clone() const;

    /** Performs things like building the spline from the keyframes.
     *   @return true on success, false otherwise
     */
    bool PostSetup();

private:

    csCatmullRomSpline* spline;
};

/** @} */

#endif
