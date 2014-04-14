/*
 * mathscript.cpp by Keith Fulton <keith@planeshift.it>
 *
 * Copyright (C) 2010s Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <psconfig.h>
#include "psconst.h"

#if defined(CS_EXTENSIVE_MEMDEBUG) || defined(CS_MEMORY_TRACKER)
# undef new
# if defined(CS_EXTENSIVE_MEMDEBUG)
#  undef CS_EXTENSIVE_MEMDEBUG
# endif
# if defined(CS_MEMORY_TRACKER)
#  undef CS_MEMORY_TRACKER
# endif
#endif
#include <new>

#include "util/log.h"
#include <csutil/randomgen.h>
#include <csutil/xmltiny.h>

#include "psdatabase.h"

#include "util/mathscript.h"
#include "util/consoleout.h"

//support for limited compilers
#ifdef _MSC_VER
double round(double value)
{
    return (value >= 0) ? floor(value + 0.5) : ceil(value - 0.5);
}
#endif

csString MathVar::ToString() const
{
    switch (Type())
    {
        case VARTYPE_OBJ:
            return GetObject()->ToString();
        case VARTYPE_STR:
            return GetString();
        default:
            if (fabs(floor(value) - value) < EPSILON) // avoid .00 for whole numbers
                return csString().Format("%.0f", value);
            return csString().Format("%.2f", value);
    }
}

csString MathVar::Dump() const
{
    switch (Type())
    {
        case VARTYPE_STR:
            return GetString();
        case VARTYPE_OBJ:
            return csString().Format("%p", (void*)GetObject());
        default:
            return csString().Format("%1.4f", value);
    }
}

MathType MathVar::Type() const
{
    // initialize conversion
    MathScriptEngine::IDConverter ID;

    // assign our masked value
    ID.value = value;
    ID.ID.mask &= 0xFFF0;

    // check the mask
    if (ID.ID.mask == 0xFFF0)
    {
        // matched the string mask
        return VARTYPE_STR;
    }
    else if (ID.ID.mask == 0x7FF0)
    {
        // matched the object mask
        return VARTYPE_OBJ;
    }
    else
    {
        return VARTYPE_VALUE;
    }
}


void MathVar::SetValue(double v)
{
    value = v;

    if (changedVarCallback)
        changedVarCallback(changedVarCallbackArg);
}

void MathVar::SetObject(iScriptableVar* p)
{
    value = parent->GetValue(p);

    if (changedVarCallback)
        changedVarCallback(changedVarCallbackArg);
}

void MathVar::SetString(const char* p)
{
    value = parent->GetValue(p);

    if (changedVarCallback)
        changedVarCallback(changedVarCallbackArg);
}

//----------------------------------------------------------------------------

void MathEnvironment::Init()
{
    // always define the environment as variable
    // as that's required to pass the environment to scriptable
    // objects in custom compound functions
    MathScriptEngine::IDConverter converter;
    converter.p = (uintptr_t)this;

    // don't use Define here as it'd check the parent
    MathVar* env = new MathVar(this);
    env->SetValue(converter.value);
    variables.Put("environment", env);
}

MathEnvironment::~MathEnvironment()
{
    csHash<MathVar*, csString>::GlobalIterator it(variables.GetIterator());
    while (it.HasNext())
    {
        delete it.Next();
    }

    if(!parent)
    {
        delete UID;
    }
}

MathVar* MathEnvironment::Lookup(const char *name) const
{
    MathVar *var = variables.Get(name, NULL);
    if (!var && parent)
        var = parent->Lookup(name);

    return var;
}

MathVar* MathEnvironment::GetVar(const char* name)
{
    MathVar *var = Lookup(name);
    if (!var)
    {
        var = new MathVar(this);
        variables.Put(name,var);
    }
    return var;
}

void MathEnvironment::DumpAllVars() const
{
    csString name;
    csHash<MathVar*, csString>::ConstGlobalIterator it(variables.GetIterator());
    while (it.HasNext())
    {
        MathVar *var = it.Next(name);
        CPrintf(CON_DEBUG, "%25s = %s\n", name.GetData(), var->Dump().GetData());
    }
}

void MathEnvironment::InterpolateString(csString & str) const
{
    csString varName;
    size_t pos = (size_t)-1;
    while ((pos = str.Find("${", pos+1)) != SIZET_NOT_FOUND)
    {
        size_t end = str.Find("}", pos+2);
        if (end == SIZET_NOT_FOUND)
            continue; // invalid - unterminated ${
        if (end < pos+3)
            continue; // invalid - empty ${}

        str.SubString(varName, pos+2, end-(pos+2));
        MathVar *var = Lookup(varName);
        if (!var)
            continue; // invalid - required variable not in environment.  leave it as is.

        str.DeleteAt(pos, end-pos+1);
        str.Insert(pos, var->ToString());
    }
}

void MathEnvironment::Define(const char *name, double value)
{
    MathVar* var = GetVar(name);
    var->SetValue(value);
}

void MathEnvironment::Define(const char *name, iScriptableVar* obj)
{
    MathVar* var = GetVar(name);
    var->SetObject(obj);
}

void MathEnvironment::Define(const char *name, const char* str)
{
    MathVar* var = GetVar(name);
    var->SetString(str);
}

bool MathEnvironment::HasString(const char* p) const
{
    bool result = false;
    if(parent)
    {
        result = parent->HasString(p);
    }

    result |= stringLiterals.Contains(p);
    return result;
}

csString MathEnvironment::GetString(double value) const
{
    // initialize converter
    MathScriptEngine::IDConverter ID;

    // assign value
    ID.value = value;
    ID.ID.mask &= 0xFFF0;
    
    // check whether the ID mask matches the string one
    if(ID.ID.mask != 0xFFF0)
    {
        return "";
    }
    else
    {
        // obtain the string associated witht he ID and
        // return it - first checking in global lookup table
        const char* str = stringLiterals.Request(ID.ID.value);
        if(parent && !str)
        {
            str = parent->GetString(value);
        }
        if(!str)
        {
            str = MathScriptEngine::Request(ID.ID.value);
        }
        return str ? str : "";
    }
    return "";
}

double MathEnvironment::GetValue(const char* p)
{
    // initialize id converter
    MathScriptEngine::IDConverter ID;

    // set the object mask
    ID.ID.value = 0xFFF0;

    // initialize the part we won't use
    ID.ID.ignored = 0;

    // check whether the string is present in the
    // global lookup table
    if(MathScriptEngine::HasString(p))
    {
        // obtain an ID from the global lookup table
        ID.ID.value = MathScriptEngine::RequestID(p);
    }
    else if(parent && parent->HasString(p))
    {
        // const cast is safe here as we verified it won't change
        // the parents environment but is a mere lookup
        ID.ID.value = const_cast<MathEnvironment*>(parent)->GetValue(p);
    }
    else
    {
        // obtain an ID from the local lookup table
        ID.ID.value = stringLiterals.Request(p);
    }

    // return masked value
    return ID.value;
}

bool MathEnvironment::HasObject(iScriptableVar* p) const
{
    bool result = false;
    if(parent && parent->HasObject(p))
    {
        result = true;
    }
    result |= scriptableRegistry.Contains(p);
    return result;
}

iScriptableVar* MathEnvironment::GetPointer(double value) const
{
    // initialize converter
    MathScriptEngine::IDConverter ID;

    // assign value
    ID.value = value;
    ID.ID.mask &= 0xFFF0;
    
    // check whether the ID mask matches the object one
    if(ID.ID.mask != 0x7FF0)
    {
        Error5("requested pointer for value %04X%04X%08X[%f] which is not a pointer", ID.ID.mask, ID.ID.ignored, ID.ID.value, ID.value);
        return NULL;
    }
    else
    {
        // obtain the object associated witht he ID and
        // return it
        iScriptableVar* object = scriptableVariables.Get(ID.ID.value,NULL);
        if(parent && !object)
        {
            object = parent->GetPointer(value);
        }
        return object;
    }
}

double MathEnvironment::GetValue(iScriptableVar* p)
{
    // initialize id converter
    MathScriptEngine::IDConverter ID;

    // set the object mask
    ID.ID.mask = 0x7FF0;

    // initialize the part we won't use
    ID.ID.ignored = 0;

    // obtain an ID and set it as value
    if(scriptableRegistry.Contains(p))
    {
        // already present, retrieve ID
        ID.ID.value = scriptableRegistry.Get(p,(uint32)-1);
    }
    else if(parent && parent->HasObject(p))
    {
        // this const-cast is safe as we verified it won't change the parent
        ID.ID.value = const_cast<MathEnvironment*>(parent)->GetValue(p);
    }
    else
    {
        // not yet part of the lookup table
        // assign a new ID
        ID.ID.value = ++(*UID);

        // add to the lookup table
        scriptableVariables.Put(ID.ID.value,p);
        scriptableRegistry.Put(p,ID.ID.value);
        
    }

    // return masked value
    return ID.value;
}

//----------------------------------------------------------------------------

MathStatement* MathStatement::Create(const csString & line, const char *name)
{
    size_t assignAt = line.FindFirst('=');
    if (assignAt == SIZET_NOT_FOUND || assignAt == 0 || assignAt == line.Length())
        return NULL;

    MathStatement *stmt = new MathStatement;
    stmt->name = name;
    csString & assignee = stmt->assignee;
    line.SubString(assignee, 0, assignAt);
    assignee.Trim();

    bool validAssignee = isupper(assignee.GetAt(0)) != 0;
    for (size_t i = 1; validAssignee && i < assignee.Length(); i++)
    {
        if (!isalnum(assignee[i]) && assignee[i] != '_')
            validAssignee = false;
    }

    if (!validAssignee && assignee != "exit") // exit is special
    {
        Error3("Parse error in MathStatement >%s: %s<: Invalid assignee.", name, line.GetData());
        delete stmt;
        return NULL;
    }

    csString expression;
    line.SubString(expression, assignAt+1);
    if (!stmt->Parse(expression))
    {
        delete stmt;
        return NULL;
    }

    stmt->opcode |= MATH_ASSIGN;
    return stmt;
}

double MathStatement::Evaluate(MathEnvironment *env) const
{
    double result = MathExpression::Evaluate(env);
    env->Define(assignee, result);
    return result;
}

//----------------------------------------------------------------------------

MathScript* MathScript::Create(const char *name, const csString & script)
{
    MathScript* s = new MathScript(name);

    size_t start = 0;
    size_t semicolonAt = 0;
    size_t blockStart = 0;
    size_t blockEnd = 0;

    while (start < script.Length())
    {
        csString trimmedScript = script.Slice(start).LTrim();

        // skip line comments
        if(trimmedScript.StartsWith("//"))
        {
            semicolonAt = script.FindFirst("\r\n", start);
            if(semicolonAt == SIZET_NOT_FOUND)
                start = script.Length();
            else
                start = semicolonAt+1;
            continue;
        }

        semicolonAt = script.FindFirst(';', start);
        if (semicolonAt == SIZET_NOT_FOUND)
            semicolonAt = script.Length();

        // parse code blocks
        blockStart = script.FindFirst('{', start);
        if (blockStart != SIZET_NOT_FOUND && blockStart < semicolonAt)
        {
            // check whether it's a conditional one
            csString line = script.Slice(start, blockStart - start);
            line.Trim();
            size_t opcode = MATH_NONE;
            if(line.StartsWith("if"))
            {
                opcode = MATH_IF;
            }
            else if(line.StartsWith("else"))
            {
                // validate we have a matching if
                size_t lineCount = s->scriptLines.GetSize();
                if (lineCount < 2 || s->scriptLines.Get(lineCount - 2)->GetOpcode() != MATH_IF)
                {
                    Error2("Failed to create MathScript >%s<. Found else without prior if.", name);
                    delete s;
                    return NULL;
                }
                opcode = MATH_ELSE;
            }
            else if(line.StartsWith("while"))
            {
                opcode = MATH_WHILE;
            }
            else if(line.StartsWith("do"))
            {
                opcode = MATH_DO;
            }

            MathExpression *st = NULL;
            if (opcode & MATH_EXP)
            {
                // find expression
                size_t expStart = line.FindFirst("(");
                size_t expEnd = line.FindLast(")");
                if (expStart == SIZET_NOT_FOUND || expEnd == SIZET_NOT_FOUND || expStart > expEnd)
                {
                    Error2("Failed to create MathScript >%s<. Could not find expression in block.", name);
                    delete s;
                    return NULL;
                }
                expStart++; // skip (

                csString exp = line.Slice(expStart, expEnd - expStart);
                exp.Collapse();
                if (!exp.IsEmpty())
                {
                    // disable assignments here for now - we need some better
                    // way to verify it's an assignment and not a comparison
                    if (0/*exp.FindFirst("=") != SIZET_NOT_FOUND*/)
                    {
                        opcode |= MATH_ASSIGN;
                        st = MathStatement::Create(exp, name);
                    }
                    else
                    {
                        st = MathExpression::Create(exp, name);
                    }
                }
            }
            else
            {
                st = new EmptyMathStatement;
            }

            if (opcode != MATH_NONE)
            {
                if (!st)
                {
                    Error2("Failed to create MathScript >%s<. Could not validate expression in block.", name);
                    delete s;
                    return NULL;
                }
                st->SetOpcode(opcode);
                s->scriptLines.Push(st);
            }
            else
            {
                delete st;
            }

            blockStart++; // skip opening { from now on

            size_t nextBlockStart = script.FindFirst('{', blockStart);
            blockEnd = script.FindFirst('}', blockStart);
            if (blockEnd == SIZET_NOT_FOUND)
            {
                Error2("Failed to create MathScript >%s<. Could not find matching close tag for code block", name);
                delete s;
                return NULL;
            }

            // find the real end of the block (take care of nested blocks)
            while (nextBlockStart != SIZET_NOT_FOUND && nextBlockStart < blockEnd)
            {
                // skip {
                nextBlockStart++;

                // find the next block end
                blockEnd = script.FindFirst('}', blockEnd+1);

                // no matching end found
                if (blockEnd == SIZET_NOT_FOUND)
                {
                    Error2("Failed to create MathScript >%s<. Could not find matching close tag for code block.", name);
                    delete s;
                    return NULL;
                }

                nextBlockStart = script.FindFirst('{', nextBlockStart);
            }

            st = MathScript::Create(name, script.Slice(blockStart, blockEnd - blockStart));
            if (!st)
            {
                Error3("Failed to create MathScript >%s<. "
                       "Failed to create sub-script for code block at %zu",name,blockStart);
                delete s;
                return NULL;
            }
            s->scriptLines.Push(st);
            start = blockEnd+1;
            continue;
        }

        if (semicolonAt - start > 0)
        {
            csString line = script.Slice(start, semicolonAt - start);
            line.Collapse();
            if (!line.IsEmpty())
            {
                MathExpression *st;
                if(line.FindFirst("=") != SIZET_NOT_FOUND)
                {
                    st = MathStatement::Create(line, name);
                }
                else
                {
                    st = MathExpression::Create(line, name);
                }

                if (!st)
                {
                    Error3("Failed to create MathScript >%s<. Failed to parse Statement: >%s<", name, line.GetData());
                    delete s;
                    return NULL;
                }
                s->scriptLines.Push(st);
            }
        }
        start = semicolonAt+1;
    }
    return s;
}

void MathScript::Destroy(MathScript* &mathScript)
{
    delete mathScript;
    mathScript = NULL;
}

MathScript::~MathScript()
{
    while (scriptLines.GetSize())
    {
        delete scriptLines.Pop();
    }
}

void MathScript::CopyAndDestroy(MathScript* other)
{
    other->scriptLines.TransferTo(scriptLines);
    delete other;
}

double MathScript::Evaluate(MathEnvironment *env) const
{
    MathVar *exitsignal = env->Lookup("exit");
    if (exitsignal)
    {
        exitsignal->SetValue(0); // clear exit condition before running
    }
    else
    {
        // create exit signal if it doesn't exist
        env->Define("exit",0.f);
        exitsignal = env->Lookup("exit");
    }

    for (size_t i = 0; i < scriptLines.GetSize(); i++)
    {
        MathExpression* s = scriptLines[i];
        size_t op = s->GetOpcode();

        // handle "do { }" and "while { }"
        if(op & MATH_LOOP)
        {
            MathExpression* l = scriptLines[i+1];
            while(!(op & MATH_EXP) || s->Evaluate(env))
            {
                // code blocks(MathScript) shall return a value < 0 to
                // signal an error/break
                if (l->Evaluate(env) < 0)
                {
                    break;
                }

                if (exitsignal && exitsignal->GetValue() != 0.0)
                {
                    // exit the script
                    return 0;
                }
            }
            i++; // skip next statement as it's already handled
        }
        // handle "return x;"
        else if(op & MATH_BREAK)
        {
            return s->Evaluate(env);
        }
        // handle "if { } [ else { } ]"
        else if(op == MATH_IF)
        {
            size_t nextOp = MATH_NONE;

            if (i + 3 < scriptLines.GetSize())
            {
                nextOp = scriptLines[i+2]->GetOpcode();
            }

            double result = 0;
            if (s->Evaluate(env))
            {
                result = scriptLines[i+1]->Evaluate(env);
            }
            else if (nextOp == MATH_ELSE)
            {
                result = scriptLines[i+3]->Evaluate(env);
            }
            if(result < 0)
            {
                // code blocks(MathScript) shall return a value < 0 to
                // signal an error/break
                return result;
            }

            if (nextOp == MATH_ELSE)
            {
                i += 3; // skip next 3 statements as we already handled them
            }
            else
            {
                i++; // skip next statement as we already handled it
            }
        }
        // handle regular expressions, e.g. assignments
        else if(op & MATH_EXP)
        {
            s->Evaluate(env);
        }

        if(exitsignal && exitsignal->GetValue() != 0.0)
        {
            // printf("Terminating mathscript at line %d of %d.\n",i, scriptLines.GetSize());
            break;
        }
    }

    return 0;
}

//----------------------------------------------------------------------------

MathScriptEngine::MathScriptEngine(iDataConnection* db, const csString& mathScriptTable)
    :mathScriptTable(mathScriptTable)
{
    LoadScripts(db);
}

MathScriptEngine::~MathScriptEngine()
{
    UnloadScripts();
}

bool MathScriptEngine::LoadScripts(iDataConnection* db, bool reload)
{
    Result result(db->Select("SELECT * from math_scripts"));
    if (!result.IsValid())
        return false;

    size_t old_count = scripts.GetSize();
    bool ok = true;
    for (unsigned long i = 0; i < result.Count(); i++ )
    {
        const char* name = result[i]["name"];
        MathScript *scr = MathScript::Create(name, result[i][mathScriptTable]);
        if (!scr)
        {
            Error2("Failed to load MathScript >%s<.", name);
            ok = false;
            continue;
        }

        if (reload)
        {
            MathScript* old = scripts.Get(name, NULL);
            if (old)
            {
                old->CopyAndDestroy(scr);
                old_count--;
            }
            else
            {
                scripts.Put(name, scr);
            }
        }
        else
        {
            scripts.Put(name, scr);
        }
    }
    if (reload && old_count != 0)
    {
        Error2("WARNING: %zu MathScripts were not reloaded.", old_count);
    }
    return ok;
}

void MathScriptEngine::UnloadScripts()
{
    csHash<MathScript*, csString>::GlobalIterator it(scripts.GetIterator());
    while (it.HasNext())
    {
        delete it.Next();
    }
    scripts.DeleteAll();

    MathScriptEngine::customCompoundFunctions.Empty();
    MathScriptEngine::stringLiterals.Empty();
}

MathScript* MathScriptEngine::FindScript(const csString & name)
{
    return scripts.Get(name, NULL);
}

void MathScriptEngine::ReloadScripts(iDataConnection* db)
{
    LoadScripts(db, true);
}

csRandomGen MathScriptEngine::rng;
csStringSet MathScriptEngine::customCompoundFunctions;
csStringSet MathScriptEngine::stringLiterals;

double MathScriptEngine::RandomGen(const double *limit)
{
   return MathScriptEngine::rng.Get()*limit[0];
}


double MathScriptEngine::CustomCompoundFunc(const double * parms)
{
    size_t funcIndex = (size_t)parms[0];
    csString funcName(customCompoundFunctions.Request(funcIndex));

    MathScriptEngine::IDConverter converter;
    converter.value = parms[1];

    MathEnvironment* env = (MathEnvironment*)converter.p;

    iScriptableVar* v = env->GetPointer(parms[2]);

    if(!v || !env)
    {
        Error5("custom compound function %s called with invalid scriptable variable %p(%f) and env %p", funcName.GetData(), (void*)v, parms[2], (void*)env);
    }

    if (funcName == "IsValid")
    {
        // check validity of the object
        return (v != NULL);
    }
    else if (funcName == "GetProperty")
    {
        // retrieve a property of an anonymous object
        return (v ? v->GetProperty(env, env->GetString(parms[3])) : 0);
    }
    else
    {
        // calculate a function on the object
        return (v ? v->CalcFunction(env, funcName.GetData(), &parms[3]) : 0);
    }
}

csString MathScriptEngine::FormatMessage(const csString& format, size_t arg_count, const double* parms)
{
    if(format.IsEmpty() || arg_count == 0)
    {
        return format;
    }

    // cap argument count to max (10)
    arg_count = arg_count > 10 ? 10 : arg_count;

    // @@@RlyDontKnow:
    // we always pass the maximum amount of arguments
    // because we can't build va_list dynamically in a sane way
    // and a switch on argc seems unnecessary
    double args[10];
    memcpy(args,parms,sizeof(double)*arg_count);

    csString result;
    result.Format(format.GetData(),args[0],args[1],args[2],args[3],args[4],
                                   args[5],args[6],args[7],args[8],args[9]);

    return result;
}

//----------------------------------------------------------------------------

MathExpression::MathExpression() : opcode(MATH_EXP)
{
    fp.AddFunction("rnd", MathScriptEngine::RandomGen, 1);

    // first 3 arguments are reserved for function index, scriptable object
    // and MathEnvironment
    for (int n = 3; n < 13; n++)
    {
        csString funcName("customCompoundFunc");
        funcName.Append(n);
        fp.AddFunction(funcName.GetData(), MathScriptEngine::CustomCompoundFunc, n);
    }
}

MathExpression* MathExpression::Create(const char *expression, const char *name)
{
    MathExpression* exp = new MathExpression;
    exp->name = name;

    if (!expression || expression[0] == '\0' || !exp->Parse(expression))
    {
        delete exp;
        return NULL;
    }

    return exp;
}

bool MathExpression::Parse(const char *exp)
{
    CS_ASSERT(exp);

    // SCANNER: creates a list of tokens.
    csArray<csString> tokens;
    size_t start = SIZET_NOT_FOUND;
    char quote = '\0';
    for (size_t i = 0; exp[i] != '\0'; i++)
    {
        char c = exp[i];

        // are we in a string literal?
        if (quote)
        {
            // found a closing quote?
            if (c == quote && exp[i-1] != '\\')
            {
                quote = '\0';
                csString token(exp+start, i-start);

                // strip slashes.
                for (size_t j = 0; j < token.Length(); j++)
                {
                    if (token[j] == '\\')
                        token.DeleteAt(j++); // remove and skip what it escaped
                }

                tokens.Push(token);
                start = SIZET_NOT_FOUND;
            }
            // otherwise, it's part of the string...ignore it.
            continue;
        }

        // alpha, numeric, and underscores don't break a token
        if (isalnum(c) || c == '_')
        {
            if (start == SIZET_NOT_FOUND) // and they can start one
                start = i;
            continue;
        }
        // everything else breaks the token...
        if (start != SIZET_NOT_FOUND)
        {
            tokens.Push(csString(exp+start, i-start));
            start = SIZET_NOT_FOUND;
        }

        // check if it's starting a string literal
        if (c == '\'' || c == '"')
        {
            quote = c;
            start = i;
            continue;
        }

        // ...otherwise, if it's not whitespace, it's a token by itself.
        if (!isspace(c))
        {
            tokens.Push(csString(c));
        }
    }
    // Push the last token, too
    if (start != SIZET_NOT_FOUND)
        tokens.Push(exp+start);

    // return statements are treated specially
    if (tokens.GetSize() && tokens[0] == "return")
    {
        tokens.DeleteIndex(0);
        opcode |= MATH_BREAK;
        if (tokens.IsEmpty())
        {
            // return -1 per default
            tokens.Push("-1");
        }
    }

    //for (size_t i = 0; i < tokens.GetSize(); i++)
    //    printf("Token[%d] = %s\n", int(i), tokens[i].GetDataSafe());

    // used to assign UIDs to string literals and to disable optimizations
    // if string literals are used to prevent them from being optimized out
    // due to their NaN value
    size_t stringCount = 0;

    // PARSER: (kind of)
    for (size_t i = 0; i < tokens.GetSize(); i++)
    {
        if (tokens[i] == ":")
        {
            if (i+1 == tokens.GetSize() || !isalpha(tokens[i+1].GetAt(0)))
            {
                Error4("Parse error in MathExpression >%s: '%s'<: Expected property or method after ':' operator; found >%s<.", name, exp, tokens[i+1].GetData());
                return false;
            }
            if (!isupper(tokens[i-1].GetAt(0)))
            {
                Error4("Parse error in MathExpression >%s: '%s'<: ':' Expected variable before ':' operator; found >%s<.", name, exp, tokens[i-1].GetData());
                return false;
            }

            // Is it a method call?
            if (i+2 < tokens.GetSize() && tokens[i+2] == "(")
            {
                // variables may not be objects in the same expressions
                if(!requiredVars.Contains(tokens[i-1]))
                {
                    Error4("Parse error in MathExpression >%s: '%s'<: ':' Expected object before ':' operator; found >%s<.", name, exp, tokens[i-1].GetData());
                    return false;
                }

                // unknown object, add it to the list
                requiredObjs.Add(tokens[i-1]);

                // Methods start as Target:WeaponAt(Slot) and turn into customCompoundFuncN(X,Env,Target,Slot)
                // where N-3 is the number of parameters and X is the index in a global lookup table and
                // Env is the environment the expression is executed in.
                // customCompoundFunc takes three args as boilerplate.
                int paramCount = 3;

                // Count the number of params via the number of commas.  First one doesn't come with a comma.
                if (i+3 < tokens.GetSize() && tokens[i+3] != ")")
                    paramCount++;
                for (size_t j = i+3; j < tokens.GetSize() && tokens[j] != ")"; j++)
                {
                    if (tokens[j] == "(")
                        while (tokens[++j] != ")"); // fast forward; skip over nested calls for now.

                    if (tokens[j] == ",")
                        paramCount++;
                }

                // Build the replacement call - replace all four tokens.
                csString object = tokens[i-1];
                csString method = tokens[i+1];
                tokens[i-1] = "customCompoundFunc";
                tokens[ i ].Format("%d", paramCount);
                tokens[i+1] = "(";
                tokens[i+2].Format("%u,environment,%s", MathScriptEngine::GetCompoundFunction(method), object.GetData());
                if (paramCount > 3)
                    tokens[i+2].Append(',');
                i += 2; // skip method name & paren - we just dealt with them.
            }
            else
            {
                // The previous variable must be an object.
                requiredObjs.Add(tokens[i-1]);

                // Found a property reference, i.e. Actor:HP
                tokens[i] = "_"; // fparser can't deal with ':', so change it to '_'.
                PropertyRef ref = {tokens[i-1],tokens[i+1]};
                propertyRefs.Add(ref);
                i++; // skip next token - we already dealt with the property.
            }
        }
        else // not dealing with a colon
        {
            // Record any string literals and replace them with their table index.
            if (tokens[i].GetAt(0) == '"' || tokens[i].GetAt(0) == '\'')
            {
                // remove quote (scanner already omitted the closing quote)
                tokens[i].DeleteAt(0);

                // build placeholder token
                csString placeholder("string");
                placeholder.Append(++stringCount);

                // add the placeholder token as constant so we can have an exact value
                // (i.e. different NaNs/Infs)
                fp.AddConstant(placeholder.GetData(), MathScriptEngine::Request(tokens[i]));

                // put the placeholder as token to parse
                tokens[i] = placeholder;
            }
            else
            {
                // Jot down any variable names (tokens starting with [A-Z])
                if (isupper(tokens[i].GetAt(0)))
                    requiredVars.Add(tokens[i]);
            }
        }
    }

    // make sure environment will be resolved upon execution
    requiredVars.Add("environment");

    // Parse the formula.
    csString fpVars;
    {
        // add all required variables
        csSet<csString>::GlobalIterator it(requiredVars.GetIterator());
        while (it.HasNext())
        {
            fpVars.Append(it.Next());
            fpVars.Append(',');
        }
    }

    {
        // add all property references as variables
        csSet<PropertyRef>::GlobalIterator it = propertyRefs.GetIterator();
        while (it.HasNext())
        {
            const PropertyRef& ref = it.Next();
            csString var;
            var.Format("%s_%s", ref.object.GetData(), ref.property.GetData());
            fpVars.Append(var);
            fpVars.Append(',');
        }
    }

    if(!fpVars.IsEmpty())
    {
        fpVars.Truncate(fpVars.Length() - 1); // remove the trailing ',' 
    }

    // Rebuild the expression now that method calls & properties are transformed
    csString expression;
    for (size_t i = 0; i < tokens.GetSize(); i++)
        expression.Append(tokens[i]);

    Debug3(LOG_SCRIPT, 0, "Final expression: '%s' (%s)\n", expression.GetData(), fpVars.GetDataSafe());

    size_t ret = fp.Parse(expression.GetData(), fpVars.GetDataSafe());
    if (ret != (size_t) -1)
    {
        Error5("Parse error in MathExpression >%s: '%s'< at column %zu: %s", name, expression.GetData(), ret, fp.ErrorMsg());
        return false;
    }

    if(!stringCount)
        fp.Optimize();
    return true;
}

double MathExpression::Evaluate(MathEnvironment *env) const
{
    double *values = new double [requiredVars.GetSize() + propertyRefs.GetSize()];
    size_t i = 0;

    // retrieve the values of all required variables
    csSet<csString>::GlobalIterator it(requiredVars.GetIterator());
    while (it.HasNext())
    {
        const csString & varName = it.Next();
        MathVar *var = env->Lookup(varName);

        if (!var) // invalid variable
        {
            csString msg;
            msg.Format("Error in >%s<: Required variable >%s< not supplied in environment.", name, varName.GetData());
            CS_ASSERT_MSG(msg.GetData(),false);
            Error2("%s",msg.GetData());
            delete [] values;
            return 0.0;
        }
        values[i++] = var->GetValue();
    }

    // retrieve the objects requried to retrieve
    // calculated values or properties
    it = requiredObjs.GetIterator();
    while (it.HasNext())
    {
        const csString & objName = it.Next();
        MathVar *var = env->Lookup(objName);

        CS_ASSERT(var); // checked as part of requiredVars

        if (var->Type() != VARTYPE_OBJ) // invalid type
        {
            csString msg;
            msg.Format("Error in >%s<: Type inference requires >%s< to be an iScriptableVar, but it isn't.", name, objName.GetData());
            CS_ASSERT_MSG(msg.GetData(),false);
            Error2("%s",msg.GetData());
            delete [] values;
            return 0.0;
        }
        else if (!var->GetObject()) // invalid object
        {
            csString msg;
            msg.Format("Error in >%s<: Given a NULL iScriptableVar* for >%s<.", name, objName.GetData());
            CS_ASSERT_MSG(msg.GetData(),false);
            Error2("%s",msg.GetData());
            delete [] values;
            return 0.0;
        }
    }

    // retrieve the required properties
    csSet<PropertyRef>::GlobalIterator propIt(propertyRefs.GetIterator());
    while (propIt.HasNext())
    {
        const PropertyRef& ref = propIt.Next();

        MathVar *var = env->Lookup(ref.object);
        CS_ASSERT(var); // checked as part of requiredVars

        iScriptableVar *obj = var->GetObject();
        CS_ASSERT(obj); // checked as part of requiredObjs

        values[i++] = obj->GetProperty(env,ref.property.GetData());
    }

    double ret = fp.Eval(values);
    delete [] values;
    return ret;
}

