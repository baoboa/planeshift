/*
 * activespell.cpp by Kenneth Graunke <kenneth@whitecape.org>
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
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "../gem.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "activespell.h"

class LinkedSpellCancel : public iCancelAction
{
public:
    LinkedSpellCancel(ActiveSpell* linked) : linked(linked) { }
    virtual ~LinkedSpellCancel() { }

    void Cancel()
    {
        if(!linked.IsValid())
            return;

        if(linked->Cancel())
            delete linked;
    }
protected:
    csWeakRef<ActiveSpell> linked;
};

//----------------------------------------------------------------------------

ActiveSpell::ActiveSpell(const csString &name, SPELL_TYPE type, csTicks duration) : name(name), type(type), duration(duration), cancelOnDeath(true), damagesHP(false), target(NULL), registrationTime(0)
{
}

ActiveSpell::ActiveSpell(const csString &name, SPELL_TYPE type, csTicks duration, const csString &image) : name(name), image(image), type(type), duration(duration), cancelOnDeath(true), damagesHP(false), target(NULL), registrationTime(0)
{
}

void ActiveSpell::Add(iSpellModifier &mod, const char* fmt, ...)
{
    CS_ASSERT(registrationTime == 0);

    // push the modifier on the list and append the script.
    // this just does printf-style formatting to make the progression script code a little tidier.
    modifiers.Push(&mod);

    va_list args;
    va_start(args, fmt);
    script.AppendFmtV(fmt, args);
    va_end(args);
}

void ActiveSpell::Add(iCancelAction* action, const char* fmt, ...)
{
    CS_ASSERT(registrationTime == 0);

    // push the cancel action on the list and append the script.
    // this just does printf-style formatting to make the progression script code a little tidier.
    actions.Push(action);

    va_list args;
    va_start(args, fmt);
    script.AppendFmtV(fmt, args);
    va_end(args);
}

void ActiveSpell::Register(gemActor* tgt)
{
    registrationTime = csGetTicks();

    target = tgt;
    target->AddActiveSpell(this);
}

void ActiveSpell::Link(ActiveSpell* other)
{
    actions.Push(new LinkedSpellCancel(other));

    if(type == DEBUFF)
    {
        // prevent target from logging out while this is active.
    }
}

bool ActiveSpell::HasExpired() const
{
    return csGetTicks() - registrationTime >= duration;
}

bool ActiveSpell::Cancel()
{
    // Mark it as already canceled to prevent linked spells from canceling each other repeatedly
    if(registrationTime == 0)
        return false;
    registrationTime = 0;

    target->RemoveActiveSpell(this);

    for(size_t i = 0; i < modifiers.GetSize(); i++)
    {
        modifiers[i]->Cancel(this);
    }
    for(size_t i = 0; i < actions.GetSize(); i++)
    {
        actions[i]->Cancel();
    }
    return true;
}

csString ActiveSpell::Persist() const
{
    if(HasExpired())
        return csString();
    CS_ASSERT(type == BUFF || type == DEBUFF);
    CS_ASSERT(!script.IsEmpty());

    csString apply;
    apply.Format("<apply aim=\"Actor\" name=\"%s\" type=\"%s\" duration=\"%u\">%s</apply>",
                 name.GetData(),
                 type == BUFF ? "buff" : "debuff",
                 duration - (csGetTicks() - registrationTime),
                 script.GetData());
    return apply;
}
void ActiveSpell::SetImage(csString imageName)
{
    image = imageName;
}
csString ActiveSpell::GetImage()
{
    return image;
}
