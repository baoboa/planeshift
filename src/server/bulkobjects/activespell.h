/*
 * activespell.h by Kenneth Graunke <kenneth@whitecape.org>
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */

#ifndef ACTIVESPELL_HEADER
#define ACTIVESPELL_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "buffable.h"


struct iDocumentNode;
class gemActor;

/**
 * \addtogroup bulkobjects
 * @{ */

/**
 * iCancelAction
 *
 * A generic way to make necessary stuff happen when an ActiveSpell
 * is cancelled.  For example, sending the undo half of a \<msg\>, or
 * unhooking an event triggered spell.
 */
class iCancelAction
{
public:
    virtual ~iCancelAction() { }
    virtual void Cancel() = 0;
};

/**
 * ActiveSpells
 *
 * A description of an active spell, containing all the information needed to
 * - Cancel it, reverting all its effects
 * - Persist it to the database (as a ProgressionScript that can recreate it).
 *
 * These correspond to "applied mode" in ProgressionScripts - that is, the
 * contents of \<apply\> nodes.
 */
class ActiveSpell : public CS::Utility::WeakReferenced
{
public:
    ActiveSpell(const csString &name, SPELL_TYPE type, csTicks duration);
    ActiveSpell(const csString &name, SPELL_TYPE type, csTicks duration, const csString &image);
    ~ActiveSpell() { }

    // These are only used by progression scripts, for loading/initializing it.
    void Add(iSpellModifier &mod, const char* fmt, ...);
    void Add(iCancelAction* action, const char* fmt, ...);

    void Register(gemActor* target);
    void Link(ActiveSpell* other);

    void SetCancelOnDeath(bool x)
    {
        cancelOnDeath = x;
    }
    bool CancelOnDeath()
    {
        return cancelOnDeath;
    }

    void MarkAsDamagingHP()
    {
        damagesHP = true;
    }
    bool DamagesHP()
    {
        return damagesHP;
    }

    const csString &Name() const
    {
        return name;
    }
    const csString &Image() const
    {
        return image;
    }
    SPELL_TYPE      Type() const
    {
        return type;
    }
    csTicks Duration() const
    {
        return duration;
    }
    csTicks RegistrationTime() const
    {
        return registrationTime;
    }
    void SetImage(csString imageName);
    csString GetImage();

    /**
     * If Cancel() returns true, you're responsible for freeing the ActiveSpell's memory.
     *
     * If it returns false, it ignored the (second) attempt to cancel it (due to links).
     */
    bool Cancel();

    bool HasExpired() const;

    /**
     * Return (XML for) a ProgressionScript that would restore this spell effect.
     */
    csString Persist() const;

protected:
    csString name;            ///< The name of the spell
    csString image;            ///< The icon representing the spell
    SPELL_TYPE type;          ///< Spell type - buff, debuff, etc.
    csString script;          ///< the contents of an \<apply\> node which recreates this effect
    csTicks duration;         ///< How long this spell lasts
    bool cancelOnDeath;       ///< Whether or not this spell should be cancelled on death
    bool damagesHP;           ///< Whether or not this spell damages HP (for cancel on duel defeat)

    gemActor* target;         ///< Who this spell is registered with
    csTicks registrationTime; ///< Timestamp when the spell was registered, for comparison with csGetTicks().

    csArray<iSpellModifier*> modifiers; ///< The iSpellModifiers this ActiveSpell altered, for cancelling.
    csPDelArray<iCancelAction> actions; ///< Actions to take when cancelling (i.e. unattaching scripts).
};

/** @} */

#endif
