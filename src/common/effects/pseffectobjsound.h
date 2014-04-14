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

#ifndef PS_EFFECT_OBJ_SOUND_HEADER
#define PS_EFFECT_OBJ_SOUND_HEADER

#include "pseffectobj.h"

class psEffect2DRenderer;
struct iSoundManager;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjSound : public psEffectObj
{
public:

    psEffectObjSound(iView* parentView, psEffect2DRenderer* renderer2d);
    ~psEffectObjSound();

    // inheritted function overloads
    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    bool Render(const csVector3 &up);
    bool Update(csTicks elapsed);
    bool AttachToAnchor(psEffectAnchor* newAnchor);
    psEffectObj* Clone() const;


private:

    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    bool PostSetup();

    csRef<iSoundManager> soundManager;
    csString soundName;
    uint soundID;

    float minDist;
    float maxDist;

    csString effectID;

    float volumeMultiplier;
    bool loop;
    bool playedOnce;
};

/** @} */

#endif
