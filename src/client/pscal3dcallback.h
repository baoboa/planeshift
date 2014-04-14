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

#ifndef PS_CAL3D_CALLBACK_HEADER
#define PS_CAL3D_CALLBACK_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/ref.h>
#include <imap/reader.h>

#include <cal3d/animcallback.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

class psCal3DCallbackLoader : public scfImplementation1<psCal3DCallbackLoader,iLoaderPlugin>
{
public:
    psCal3DCallbackLoader(iObjectRegistry * objreg);
    virtual ~psCal3DCallbackLoader();

    // used to handle and parse the <addon> tags.
    csPtr<iBase> Parse(iDocumentNode * node, iStreamSource*, iLoaderContext * ldr_context,
      iBase * context);

    bool IsThreadSafe() { return true; }
};

/**
 * A cal3d callback to handle displaying an effect.
 */
class psCal3DCallbackEffect : public CalAnimationCallback
{
public:
    psCal3DCallbackEffect(iDocumentNode * node);
    virtual ~psCal3DCallbackEffect();

    void AnimationUpdate(float anim_time, CalModel * model, void * userData);
    void AnimationComplete(CalModel * model, void * userData);

protected:

    /// name of the effect to call
    csString effectName;

    /// time to trigger the effect
    float triggerTime;

    /// keep track of the animation time of last update
    float lastUpdateTime;
    
    /// keep track if the effect has triggered this cycle
    bool hasTriggered;

    /// if true, effect will be rerendered every cycle, otherwise it will be rendered every animation
    bool resetPerCycle;

    /// true if the effect should die when the animation dies
    bool killWithAnimEnd;

    /// keep track of the effect ID
    unsigned int effectID;
};

/// Callback to repeat animation
struct vmAnimCallback : public CalAnimationCallback
{
    csRef<iSpriteCal3DFactoryState> callbacksprite;
    csRef<iSpriteCal3DState> callbackspstate;
    csString callbackanimation;
    unsigned int callbackcount;

    // Not using this for now.
    void AnimationUpdate(float /*anim_time*/, CalModel* /*model*/, void* /*userData*/)
    {
        //printf("Anim Update at time %.2f.\n",anim_time);
    }

    // This is used to keep animations repeating.
    void AnimationComplete(CalModel* /*model*/, void* /*userData*/)
    {
        //printf("Anim Completed (%d)!\n",callbackcount);
        if (callbackcount > 0)
        {
            callbackcount--;
            callbackspstate->SetAnimAction(callbackanimation.GetDataSafe(),0.0,0.0);
        }
        else  // remove callback when repeat count is completed
        {
            callbacksprite->RemoveAnimCallback(callbackanimation,this);
        }
    }
};

#endif

