/*
* reaction.h by Anders Reggestad <andersr@pvv.org>
*
* Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __REACTION_H__
#define __REACTION_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <iutil/document.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psconst.h"
#include "util/mathscript.h"

/**
 * \addtogroup npcclient
 * @{ */

class Perception;
class Behavior;
class NPC;
class BehaviorSet;
class gemNPCObject;


/**
* A reaction embodies the change in desire which occurs
* in an NPC when he perceives something.
*/
class Reaction
{
protected:
    friend class NPCType;

    // members making up the "if statement"
    csString  eventType;
    float     range;
    int       factionDiff;
    csString  oper;
    csString  condition;
    MathScript* calcCondition; ///< This is the particular calculation for condition.
    bool      activeOnly;
    bool      inactiveOnly;
    bool      reactWhenDead;
    bool      reactWhenInvisible;
    bool      reactWhenInvincible;
    csArray<bool> valuesValid;
    csArray<int> values;
    csArray<bool>randomsValid;
    csArray<int> randoms;
    csString  type;
    csArray<csString> onlyInterrupt;
    csArray<csString> doNotInterrupt;

    // members making up the "then statement"
    csArray<Behavior*> affected;

    enum DesireType
    {
        DESIRE_NONE,
        DESIRE_DELTA,
        DESIRE_ABSOLUTE,
        DESIRE_GUARANTEED
    };

    DesireType desireType;    ///< Indicate the type of desire change this reaction has.
    float      desireValue;   ///< The value to use for the desire type.
    float      weight;        ///< The weight to apply to deltas.

    csString   lastTriggered; ///< Used for debug, show when this was last triggered

public:
    Reaction();
    Reaction(Reaction &other, BehaviorSet &behaviors)
    {
        DeepCopy(other,behaviors);
    }

    void DeepCopy(Reaction &other, BehaviorSet &behaviors);

    bool Load(iDocumentNode* node, BehaviorSet &behaviors);
    void React(NPC* who, Perception* pcpt);

    /**
     * Check if the player should react to this reaction
     *
     * Caled by perception related to entitis to check
     * for reaction against visibility and invincibility
     *
     * @param entity          The entity tho check for visibility and invincibility
     */
    bool ShouldReact(gemNPCObject* entity);

    /**
     * Check if this is a behavior that shouldn't be interrupted
     *
     * @param behavior The behavior to check.
     * @return True if this is a behavior that shouldn't be interrupted.
     */
    bool DoNotInterrupt(Behavior* behavior);

    /** Check if this is a behavior is on the only interrupt list if set.
     *
     * If there is a limitation on behaviors to interrupt for this
     * reaction this function will return true if the behavior isn't on
     * that list.
     *
     * @param behavior The behavior to check.
     * @return True if this is a behavior that isn't allowed to interrupt.
     */
    bool OnlyInterrupt(Behavior* behavior);

    const csString &GetEventType() const;
    float           GetRange()
    {
        return range;
    }
    int             GetFactionDiff()
    {
        return factionDiff;
    }
    bool            GetValueValid(int i);
    int             GetValue(int i);
    bool            GetRandomValid(int i);
    /** Setting the value
     *
     *  This function ignore the data valid flag. And will not update it
     *  for the value set.
     */
    bool            SetValue(int i, int value);
    csString        GetValue();
    int             GetRandom(int i);
    const csString  GetType(NPC* npc) const;
    char            GetOp();
    csString        GetAffectedBehaviors();
    const csString &GetLastTriggerd()
    {
        return lastTriggered;
    }

};

/** @} */

#endif

