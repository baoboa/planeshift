/*
 * mathscript.h by Keith Fulton <keith@planeshift.it>
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __MATHSCRIPT_H__
#define __MATHSCRIPT_H__

#include <../tools/fparser/fparser.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/randomgen.h>
#include <csutil/set.h>
#include <csutil/strset.h>
#include <util/scriptvar.h>
#include <csutil/weakreferenced.h>
#include <csutil/weakref.h>

#ifdef _MSC_VER
double round(double value);
#endif

class MathScript;
class MathVar;
struct iDataConnection;

/**
 * \addtogroup common_util
 * @{ */

/**
 * This holds all the formulas loaded from the MathScript table
 * and provides a container for them.  It also enables adding
 * of some needed functions not built-in to the formula parser.
 * On top of that it may be used to format a message based on
 * variables retrieved from a MathEnvironment.
 */
class MathScriptEngine
{
protected:
    csHash<MathScript*, csString> scripts;
    static csRandomGen rng;

    static csStringSet stringLiterals;
    static csStringSet customCompoundFunctions;


    csString mathScriptTable;
    
public:
    union IDConverter
    {
        double value; // masked float value
        struct
        {
#ifdef CS_LITTLE_ENDIAN
            CS_ALIGNED_MEMBER(uint32 value,1); // value of the ID
            CS_ALIGNED_MEMBER(uint16 ignored,1); // ignored part
            CS_ALIGNED_MEMBER(uint16 mask,1); // mask used to identify strings/objects
#else
            CS_ALIGNED_MEMBER(uint16 mask,1); // mask used to identify strings/objects
            CS_ALIGNED_MEMBER(uint16 ignored,1); // ignored part
            CS_ALIGNED_MEMBER(uint32 value,1); // value of the ID
#endif
        } ID;
        uintptr_t p; // packed pointer
    };

    MathScriptEngine(iDataConnection* db, const csString& mathScriptTable);
    ~MathScriptEngine();

    /// retrieve a MathScript given it's name.
    MathScript* FindScript(const csString& name);

    /**
     * Triggers a cleanup and reload of all the scripts.
     */
    void ReloadScripts(iDataConnection* db);

    /**
     * Loads all the scripts from the database.
     * @return TRUE if it was possible to retrieve successfully the data.
     */
    bool LoadScripts(iDataConnection* db, bool reload = false);

    /**
     * Cleans up all the script and data loaded.
     */
    void UnloadScripts();

    /**
     * retrieve the index of a custom compound function given it's name.
     * for internal use only, do *not* call this manually.
     */
    static CS::StringIDValue GetCompoundFunction(const csString& name)
    {
        return customCompoundFunctions.Request(name);
    }

    /// obtain a string literal based on it's actual ID
    static const char* Request(uint32 ID)
    {
        return stringLiterals.Request(ID);
    }

    /// obtain a string literal based on it's masked value
    static const char* Request(double f)
    {
        // initialize ID conversion
        IDConverter ID;
        ID.value = f;
        ID.ID.mask &= 0xFFF0;

        // check the mask matches our string mask
        if(ID.ID.mask != 0xFFF0)
        {
            // not a string
            return "";
        }

        // we seem to have an ID here, retrieve the associated
        // string and return it
        return Request(ID.ID.value);
    }

    /// request an ID for a string
    static uint32 RequestID(const char* str)
    {
        return stringLiterals.Request(str);
    }

    /// request a masked value for a string
    static double Request(const char* str)
    {
        // intialize the ID conversion
        IDConverter ID;
        
        // set our string mask so the value will be recognizable
        ID.ID.mask = 0xFFF0;

        // initialize the part we won't use
        ID.ID.ignored = 0;

        // retrieve the ID
        ID.ID.value = RequestID(str);

        // return the masked value
        return ID.value;
    }

    /// check whether a string is present in the global lookup table.
    static bool HasString(const char* str)
    {
        return stringLiterals.Contains(str);
    }

    /**
     * internal function used for callbacks to scriptable objects from fparser.
     * do *not* call this manually.
     */
    static double CustomCompoundFunc(const double * parms);

    /// rnd(limit) generates a random number between 0 and limit.
    static double RandomGen(const double *dummy);

    /// format a message using csString's Format given a string ID and a number of floating points.
    static csString FormatMessage(const csString& formatString, size_t arg_count, const double* parms);
};

/**
 * A specific MathEnvironment to be used in a MathScript.
 * This holds all currently defined variables in that environment
 * and allows you to define/retrieve those.
 */
class MathEnvironment
{
private:
    MathVar* GetVar(const char* name);

    /// variable used to assign object IDs
    uint32* UID;
    /// environment specific lookup table for scriptable objects
    csHash<iScriptableVar*,uint32> scriptableVariables;
    /// envrionment specific reverse-lookup table for scriptable objects
    csHash<uint32,iScriptableVar*> scriptableRegistry;

    /// environment specific lookup table for string literals
    csStringSet stringLiterals;

    const MathEnvironment *parent;
    csHash<MathVar*, csString> variables;

    void Init();

    // environments mustn't be copy-constructed
    MathEnvironment(const MathEnvironment&);

public:
    MathEnvironment() : UID(new uint32),parent(NULL)
    {
        *UID = 0;
        Init();
    }
    MathEnvironment(const MathEnvironment *parent) : UID(parent->UID),parent(parent)
    {
        Init();
    }
    ~MathEnvironment();

    /// define a regular variable in the environment
    void Define(const char *name, double value);

    /// define an object variable in the environment
    void Define(const char *name, iScriptableVar* obj);

    /// define a string variable in the environment
    void Define(const char *name, const char* str);

    /// test whether we have an ID for a string.
    bool HasString(const char* p) const;

    /// test whether we have an ID for an object.
    bool HasObject(iScriptableVar* p) const;

    /// retrieve a string literal based on it's id.
    csString GetString(double id) const;

    /// retrieve an ID for a scriptable object.
    double GetValue(iScriptableVar* p);

    /// retrieve a pointer to a scriptable object based on it's id.
    iScriptableVar* GetPointer(double id) const;

    /// retrieve an ID for a string literal.
    double GetValue(const char* p);

    MathVar* Lookup(const char *name) const;
    void DumpAllVars() const;

    /// Perform string interpolation, i.e. replacing ${...} with the appropriate variable.
    void InterpolateString(csString & str) const;
};

/// possible types of variables.
enum MathType
{
    VARTYPE_VALUE,
    VARTYPE_STR,
    VARTYPE_OBJ
};

/**
 * This holds information about a specific variable
 * in a specific MathEnvironment to be used for MathScripts
 * and allows setting/retrieving all data related to it.
 */
class MathVar
{
protected:
    double value;
    MathEnvironment* parent;

    typedef void (*MathScriptVarCallback)(void* arg);
    MathScriptVarCallback changedVarCallback;
    void* changedVarCallbackArg;

public:
    MathVar(MathEnvironment* parent) : value(0.f),parent(parent),
                                       changedVarCallback(NULL),
                                       changedVarCallbackArg(NULL)
    {
    }

    void SetChangedCallback(MathScriptVarCallback callback, void* arg)
    {
        changedVarCallback = callback;
        changedVarCallbackArg = arg;
    }

    void SetValue(double v);
    void SetObject(iScriptableVar* p);
    void SetString(const char* p);

    MathType Type() const;

    inline int GetRoundValue() const
    {
        return (int)round(GetValue());
    }

    inline double GetValue() const
    {
        return value;
    }

    inline iScriptableVar* GetObject() const
    {
        return parent->GetPointer(value);
    }

    inline csString GetString() const
    {
        return parent->GetString(value);
    }

    csString ToString() const;
    csString Dump() const;
};

/**
 * The base expression class.
 * It is able to perform basic operations (e.g. "atan(a,b)")
 * and also holds additional information about the type
 * of the operator this expression represents.
 * All other possible statements must derive from this.
 */
class MathExpression
{
protected:
    enum
    {
        MATH_NONE   =   0, // NOP
        MATH_COND   =   1, // is a conditional
        MATH_EXP    =   2, // has expression
        MATH_LOOP   =   4, // is a loop
        MATH_ASSIGN =   8, // is an assignment
        MATH_BREAK  =  16, // flow control break

        // aliases
        MATH_WHILE = MATH_COND | MATH_EXP | MATH_LOOP,
        MATH_IF    = MATH_COND | MATH_EXP,
        MATH_ELSE  = MATH_COND,
        MATH_DO    = MATH_EXP | MATH_LOOP
    };
    size_t opcode; // opcode of this expression

    MathExpression(); // may only be constructed via MathExpression::Create

    bool Parse(const char *expression);

    struct PropertyRef
    {
        csString object; // name of the object this property refers to
        csString property; // property to be retrieved

        // required to be used in csHash as used by csSet
        uint GetHash() const
        {
            return (object+":"+property).GetHash();
        }

        // required by csComparator used in csHash used by csSet
        bool operator<(const PropertyRef& rhs) const
        {
            return (object+":"+property) < (rhs.object+":"+rhs.property);
        }
    };

    csSet<csString> requiredVars; ///< variables required to execute this expression
    csSet<csString> requiredObjs; ///< a subset of requiredVars which are known to be objects; for type checking
    csSet<PropertyRef> propertyRefs; ///< properties that have to be resolved prior to evaluation
    mutable FunctionParser fp;

    const char *name; // used for debugging

public:
    virtual ~MathExpression() {} /// Empty destructor
    static MathExpression* Create(const char *expression, const char *name = "");
    
    virtual double Evaluate(MathEnvironment *env) const;

    size_t GetOpcode() const
    {
        return opcode;
    }

    void SetOpcode(size_t newOpcode)
    {
        opcode = newOpcode;
    }
};

/**
 * This holds one line of a (potentially) multi-line script.
 * It knows what \<var\> is on the left, and what \<formula\> is
 * on the right of the = sign.  When run, it executes the
 * formula and sets the result in the Var.  These vars are
 * shared across Lines, which means the next Line can use
 * that Var as an input.
 */
class MathStatement : public MathExpression
{
protected:
    MathStatement() { } // may only be constructed via MathStatement::Create

    csString assignee; ///< variable the result will be assinged to

public:
    static MathStatement* Create(const csString & expression, const char *name);
    double Evaluate(MathEnvironment *env) const;
};

/**
 * This holds an empty statement that shall not be executed
 * but is used for control flow statements, e.g. else or do
 */
class EmptyMathStatement : public MathExpression
{
public:
    EmptyMathStatement()
    {
        opcode = MATH_NONE;
    }

    double Evaluate(MathEnvironment* /*env*/) const
    {
        return 0;
    }
};


/**
 *  A MathScript is a mini-program to run.
 *  It allows multiple lines. Each line must be
 *  in the form of:  \<var\> = \<formula\>.  When
 *  it parses, it makes a hashmap of all the variables
 *  for quick access.
 */
class MathScript : private MathExpression
{
protected:
    MathScript(const char *name) : name(name) { } // may only be constructed using MathScript::Create
    csString name;
    csArray<MathExpression*> scriptLines;

public:
    static MathScript* Create(const char *name, const csString & script);
    static void Destroy(MathScript* &mathScript);
    
    ~MathScript();

    const csString & Name() const
    {
        return name;
    }

    void CopyAndDestroy(MathScript* other);

    double Evaluate(MathEnvironment *env) const;
};

/** @} */

#endif

