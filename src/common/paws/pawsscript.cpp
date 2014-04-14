/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 * http://www.atomicblue.org)
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

#include <csutil/csstring.h>

#include "pawsscript.h"
#include "pawswidget.h"
#include "util/mathscript.h"

pawsScriptStatement::pawsScriptStatement(const char* scriptText) : env(&PawsManager::GetSingleton().ExtraScriptVars())
{
    script = MathScript::Create("pawsScript", scriptText);
    if(!script)
    {
        Error2("pawsScript: failed to parse script: %s", scriptText);
    }
}

pawsScriptStatement::~pawsScriptStatement()
{
    MathScript::Destroy(script);
}

void pawsScriptStatement::ChangedResultsVarCallback(void* arg)
{
    pawsScriptResult* res = (pawsScriptResult*)arg;
    res->widget->SetProperty(res->property, res->var->GetValue());
}

void pawsScriptStatement::AddLHSResult(pawsWidget* widget, const char* property, const char* name)
{
    pawsScriptResult res;
    res.widget = widget;
    res.property = property;
    env.Define(name, 0.0); // Bogus value...it'll get set when we Execute().  We just need a var.
    res.var = env.Lookup(name);
    scriptResults.Push(res);
}

void pawsScriptStatement::AddRHSVar(iScriptableVar* v, const char* name)
{
    env.Define(name, v);
}

void pawsScriptStatement::Execute()
{
    for(size_t i = 0; i < scriptResults.GetSize(); ++i)
    {
        scriptResults[i].var->SetValue(scriptResults[i].widget->GetProperty(&env,scriptResults[i].property));
        scriptResults[i].var->SetChangedCallback(pawsScriptStatement::ChangedResultsVarCallback, (void*) &scriptResults[i]);
    }
    script->Evaluate(&env);
}

bool pawsScript::NextChar(const char* script, size_t &currIndex, char &c, char &n)
{
    c = script[currIndex];
    if(!c)
        return false;

    ++currIndex;
    n = script[currIndex];
    return true;
}

pawsWidget* pawsScript::FindWidget(pawsWidget* widget, const char* name)
{
    // a better FindWidget that will go back up the parents to really try and find the thing
    pawsWidget* ret = 0;
    while(!ret && widget)
    {
        ret = widget->FindWidget(name, false);
        widget = widget->GetParent();
    }
    return ret;
}

bool pawsScript::Parse(const char* script)
{
    char c = 0;
    char n = 0;
    size_t currIndex = 0;
    bool openQuotes = false;
    csString currToken;
    csArray<csString> tokens;

    // SCANNER: creates a list of tokens
    while(NextChar(script, currIndex, c, n))
    {
        // handle quotation marks; basically anything between two matching quotation marks is a single token
        if(c == '\"')
        {
            openQuotes = !openQuotes;
            if(!openQuotes)
            {
                tokens.Push(currToken);
                currToken.Clear();
            }
            continue;
        }

        // if we're in quotation marks then handle everything
        if(openQuotes)
        {
            currToken += c;
            continue;
        }

        // whitespace breaks a token, but doesn't get added to a token
        if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            if(!currToken.IsEmpty())
            {
                tokens.Push(currToken);
                currToken.Clear();
            }
            continue;
        }

        // alpha, numeric, and underscores don't break a token
        if(isalnum(c) || c == '_')
        {
            currToken += c;
            continue;
        }

        // everything else breaks the token and is considered a token by itself
        if(!currToken.IsEmpty())
            tokens.Push(currToken);
        currToken = c;
        tokens.Push(currToken);
        currToken.Clear();
    }

    // PARSER: simple parsing that grabs key tokens
    size_t a;
    size_t len = tokens.GetSize();
    bool onRHS = false;
    csArray<pawsScriptResult> lhsResults;
    csArray<pawsWidget*> rhsDependencies;
    size_t startToken = 0;
    for(a=0; a<len; ++a)
    {
        csString token = tokens[a];
        if(tokens[a] == ":" && onRHS)
        {
            // check if it's an extra var
            bool isExtraVar = PawsManager::GetSingleton().ExtraScriptVars().Lookup(tokens[a-1]) != NULL;
            if(!isExtraVar)
            {
                pawsWidget* dep;
                if(tokens[a-1] == "Me" || tokens[a-1] == "me")
                    dep = widget;
                else
                    dep = FindWidget(widget, tokens[a-1]);
                if(!dep)
                {
                    Error2("pawsWidget '%s' referenced in a PAWS Script could not be found\n", tokens[a-1].GetData());
                    return false;
                }

                // widget names can have bad characters, so just rename all widgets to be safe
                tokens[a-1] = "W";
                tokens[a-1] += (unsigned int)rhsDependencies.GetSize();

                rhsDependencies.Push(dep);
            }
        }
        else if(tokens[a] == '=')
        {
            onRHS = true;
            if(a == startToken+3 && tokens[a-2] == ":")     // have 'widget:property' on lhs
            {
                // keep track of result variable
                pawsScriptResult result;
                if(tokens[startToken] == "Me" || tokens[startToken] == "me")
                    result.widget = widget;
                else
                    result.widget = FindWidget(widget, tokens[startToken]);
                if(result.widget)
                {
                    result.property = tokens[startToken+2];
                    result.var = NULL;
                    lhsResults.Push(result);

                    // rename variable to something mathscript can set
                    tokens[startToken].Clear();
                    tokens[startToken+1].Clear();
                    tokens[startToken+2] = "R";
                    tokens[startToken+2] += (unsigned int)lhsResults.GetSize()-1;
                }
            }
        }
        else if(tokens[a] == ';')
        {
            if(!onRHS)
            {
                // if we didn't get a rhs then we might have something like 'widget:Blah()', so try to resolve the widget name
                for(size_t b=a-1; b>0 && tokens[b] != ';'; --b)
                {
                    if(tokens[b] == ':')
                    {
                        pawsWidget* w;
                        if(tokens[b-1] == "Me" || tokens[b-1] == "me")
                            w = widget;
                        else
                            w = FindWidget(widget, tokens[b-1]);
                        if(w)
                        {
                            tokens[b-1] = "W";
                            tokens[b-1] += (unsigned int)rhsDependencies.GetSize();
                            rhsDependencies.Push(w);
                        }
                        break;
                    }
                }
            }

            onRHS = false;
            startToken = a+1;
        }
    }

    csString augmentedScript;
    len = tokens.GetSize();
    for(a=0; a<len; ++a)
        augmentedScript += tokens[a];

    statement = new pawsScriptStatement(augmentedScript);

    for(a=0; a<lhsResults.GetSize(); ++a)
    {
        csString varName = "R";
        varName += (unsigned int)a;
        statement->AddLHSResult(lhsResults[a].widget, lhsResults[a].property, varName);
    }
    for(a=0; a<rhsDependencies.GetSize(); ++a)
    {
        csString varName = "W";
        varName += (unsigned int)a;
        statement->AddRHSVar(rhsDependencies[a], varName);
    }
    return true;
}

pawsScript::pawsScript(pawsWidget* widget, const char* script)
    : widget(widget)
{
    statement = 0;
    scriptText = script;
}

pawsScript::~pawsScript()
{
    delete statement;
}

void pawsScript::Execute()
{
    if(!scriptText.IsEmpty())
    {
        Parse(scriptText);
        scriptText.Clear();
    }

    if(statement)
        statement->Execute();
}
