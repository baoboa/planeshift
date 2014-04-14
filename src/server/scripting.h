/*
 * scripting.h - by Kenneth Graunke <kenneth@whitecape.org>
 *
 * Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef SCRIPTING_HEADER
#define SCRIPTING_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>
#include <csutil/parray.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

class MathEnvironment;
class MathExpression;
class ActiveSpell;
class EntityManager;
class CacheManager;

/**
 * Events that can trigger scripts, i.e. \<on type="attack"\>
 */
enum SCRIPT_TRIGGER
{
    ATTACK,
    DEFENSE,
    MOVE,
    NEARLYDEAD
};

class ImperativeOp; ///< private script internals

/** ProgressionScripts
 *
 * This is PlaneShift's XML based scripting system.
 * A RelaxNG schema is in data/ProgressionSchema.rng, both defining the
 * language and allowing one to check the validity of scripts.
 *
 * Scripts have two modes: imperative and applied.
 * - Imperative operations are one-shot, and can do basically anything
 *   one would like.
 * - Applied operations describe the effect of magic that lasts for a time,
 *   but which eventually will wear off.  Thus, every applied operation must
 *   have a well defined inverse.  These become ActiveSpells.
 *
 * See scripting.cpp for the implementation of all the various operations.
 */
class ProgressionScript
{
public:
    ~ProgressionScript();
    static ProgressionScript* Create(EntityManager* entitymanager,CacheManager* cachemanager, const char* name, const char* script);
    static ProgressionScript* Create(EntityManager* entitymanager,CacheManager* cachemanager, const char* name, iDocumentNode* top);

    const csString &Name()
    {
        return name;
    }
    void Run(MathEnvironment* env);

protected:
    ProgressionScript(const char* name) : name(name) { }
    csString name;
    csArray<ImperativeOp*> ops;
};

class AppliedOp; ///< private script internals

class ApplicativeScript
{
public:
    ~ApplicativeScript();
    static ApplicativeScript* Create(EntityManager* entitymanager, CacheManager* cachemanager, const char* script);
    static ApplicativeScript* Create(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top);
    static ApplicativeScript* Create(EntityManager* entitymanager, CacheManager* cachemanager, iDocumentNode* top, SPELL_TYPE type, const char* name, const char* duration);

    ActiveSpell* Apply(MathEnvironment* env, bool registerCancelEvent = true);
    const csString &GetDescription();

protected:
    ApplicativeScript();

    SPELL_TYPE type;            ///< spell type...buff, debuff, etc.
    csString aim;               ///< name of the MathScript var to aim at
    csString name;              ///< the name of the spell
    csString description;       ///< textual representation of the effect
    csString image;             ///< graphical representation of the effect
    MathExpression* duration;   ///< an embedded MathExpression
    csPDelArray<AppliedOp> ops; ///< all the sub-operations
};

#if 0
// Eventually, we'll want to be able to target items in the inventory,
// and specific points on the ground, as well as entities in the world.
class Target
{
public:
    Target(psItem*  item)   : type(TARGET_PSITEM),    item(item) { }
    Target(gemObject*  obj) : type(TARGET_GEMOBJECT), obj(obj)   { }
    //Target(Location loc) : loc(loc), type(TARGET_LOCATION) { }

protected:
    enum Type
    {
        TARGET_GEMOBJECT,
        TARGET_PSITEM,
        TARGET_LOCATION
    };
    Type type;

    union
    {
        psItem*    item;
        gemObject* obj;
        //Location loc;
    };
};
#endif

#endif
