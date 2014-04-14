/*
 * buffable.h by Kenneth Graunke <kenneth@whitecape.org>
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef BUFFABLE_HEADER
#define BUFFABLE_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <cstypes.h>
#include <csutil/list.h>
#include <csutil/tuple.h>

class ActiveSpell;

/** iSpellModifiers
 *
 * An abstract base class for Buffables, Overridables, and Multipliers.
 * ActiveSpells needs to keep a heterogeneous list of these (with possibly
 * different template parameters, even), but they only need to call Cancel().
 */
class iSpellModifier
{
public:
    virtual ~iSpellModifier() { }
    virtual void Cancel(const ActiveSpell* owner) = 0;
};

//-----------------------------------------------------------------------------

/** Overridables
 *
 * Overridables provide a straightforward way for the scripting system to
 * temporarily override some opaque value (like a string or position), yet
 * be able to undo the change at some later time.
 *
 * Note that having spells simply set the new value and reset it to the old one
 * on expiration does not work - consider overlapping intervals:
 *
 *                         [====CAST CLACKERFORM====]
 *         [========CAST ULBERFORM=======] (wrong!) | (wrong until relog!)
 * enkim.. | ..ulbernaut.. | ..clacker.. | enkim!!  | ...ulbernaut!!
 * <--------------------------------------------------------------------->
 *                                                                  time
 *
 * An overridable is basically a list of key-value pairs.  For example:
 * (B, clacker) | (A, ulbernaut) | (NULL, enkim)
 * Here, the character is normally an enkim; spell A turned him into an
 * ulbernaut, and spell B made him look like a clacker.  When A expires,
 * it simply removes the middle link, leaving:
 * (B, clacker) | (NULL, enkim)
 * ...which is correct.
 */
template <typename T>
class Overridable : public iSpellModifier
{
public:

    Overridable(T x)
    {
        values.PushBack(csTuple2<const ActiveSpell*,T>(NULL, x));
    }

    virtual ~Overridable() { }

    T Current() const
    {
        return values.Front().second;
    }

    T Base() const
    {
        CS_ASSERT(values.Last().first == NULL);
        return values.Last().second;
    }

    void SetBase(T x)
    {
        CS_ASSERT(values.Last().first == NULL);

        const T &old = Current();

        values.PopBack();
        values.PushBack(csTuple2<const ActiveSpell*,T>(NULL, x));

        if(Current() != old)
            OnChange();
    }

    void Override(const ActiveSpell* owner, T x)
    {
        const T &old = Current();

        values.PushFront(csTuple2<const ActiveSpell*,T>(owner, x));

        if(x != old)
            OnChange();
    }

    virtual void Cancel(const ActiveSpell* owner)
    {
        const T &old = Current();

        typename csList< csTuple2<const ActiveSpell*, T> >::Iterator it(values);
        while(it.HasNext())
        {
            csTuple2<const ActiveSpell*, T> &curr = it.Next();
            if(curr.first == owner)
            {
                values.Delete(it);
            }
        }

        if(Current() != old)
            OnChange();
    }

protected:
    /// Called whenever the current value changes; implemented in derived classes.
    virtual void OnChange()
    {
    }

    csList< csTuple2<const ActiveSpell*, T> > values;
};

//-----------------------------------------------------------------------------

/** Buffables
 *
 * Buffables provide a consistent interface for numerical stats that can be
 * temporarily buffed by magic.  These are additive in nature.
 */
template <typename T>
class Buffable : public iSpellModifier
{
public:
    Buffable()
    {
        base = cached = 0;
    }
    Buffable(T x)
    {
        base = cached = x;
    }
    virtual ~Buffable() { }

    T Current() const
    {
        return cached;
    }
    T Base() const
    {
        return base;
    }

    void SetBase(T x)
    {
        if(x == base)
            return;

        cached += x - base;
        base = x;

        OnChange();
    }

    void Buff(const ActiveSpell* owner, T x)
    {
        cached += x;
        buffs.Push(csTuple2<const ActiveSpell*,T>(owner, x));

        OnChange();
    }

    virtual void Cancel(const ActiveSpell* owner)
    {
        bool changed = false;

        // Count backwards since we're deleting and things may shift on the right
        for(size_t i = buffs.GetSize() - 1; i != (size_t) -1; i--)
        {
            if(buffs[i].first == owner)
            {
                cached -= buffs[i].second;
                buffs.DeleteIndexFast(i);
                changed = true;
            }
        }

        if(changed)
            OnChange();
    }

protected:
    /// Called whenever the value changes; implemented in derived classes.
    virtual void OnChange()
    {
    }

    T base;
    T cached;
    csArray< csTuple2<const ActiveSpell*, T> > buffs;
};

//-----------------------------------------------------------------------------

/** Multipliers
 *
 * While most things are additive, some are purely multiplier values - for
 * example, attack and defense modifiers.  This is fully analogous to
 * buffables, multiplicative.
 */
class Multiplier : public iSpellModifier
{
public:
    Multiplier()
    {
        cached = 1;
    }
    virtual ~Multiplier() { }

    float Value()
    {
        return cached;
    }

    void Buff(const ActiveSpell* owner, float x)
    {
        cached *= x;
        buffs.PushBack(csTuple2<const ActiveSpell*, float>(owner, x));
    }

    virtual void Cancel(const ActiveSpell* owner)
    {
        // Recompute the cache...avoid rounding errors.
        cached = 1;

        csList< csTuple2<const ActiveSpell*, float> >::Iterator it(buffs);
        while(it.HasNext())
        {
            csTuple2<const ActiveSpell*, float> &curr = it.Next();
            if(curr.first == owner)
            {
                buffs.Delete(it);
            }
            else
            {
                cached *= curr.second;
            }
        }
    }

protected:
    float cached;
    csList< csTuple2<const ActiveSpell*, float> > buffs;
};

//-----------------------------------------------------------------------------

/// A special form of buffable that is clamped to always return a positive number.
template <typename T>
class ClampedPositiveBuffable : public Buffable<T>
{
public:
    using Buffable<T>::cached;
    int Current()
    {
        // Clamp to avoid underflow problems with negative buffs.
        return cached > 0 ? cached : 0;
    }
};

#endif
