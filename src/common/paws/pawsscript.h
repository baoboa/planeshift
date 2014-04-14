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

#ifndef PAWS_SCRIPT_H
#define PAWS_SCRIPT_H

#include <csutil/array.h>
#include "util/mathscript.h"

class pawsWidget;

/**
 * \addtogroup common_paws
 * @{ */

struct pawsScriptResult
{
    pawsWidget* widget;
    MathVar*    var;
    csString    property;
};

class pawsScriptStatement
{
private:
    MathScript*                 script;
    MathEnvironment             env;
    csArray<pawsScriptResult>   scriptResults;

    static void ChangedResultsVarCallback(void* arg);
public:
    pawsScriptStatement(const char* script);
    virtual ~pawsScriptStatement();
    
    void AddLHSResult(pawsWidget* widget, const char* property, const char* name);
    void AddRHSVar(iScriptableVar* v, const char* name);

    void Execute();
};

class pawsScript
{
private:
    pawsScriptStatement* statement;
    pawsWidget* widget;
    csString scriptText;

    bool NextChar(const char* script, size_t &currIndex, char &c, char &n);
    bool Parse(const char* script);

    pawsWidget* FindWidget(pawsWidget* widget, const char* name);
public:
    pawsScript(pawsWidget* widget, const char* script);
    ~pawsScript();

    void Execute();
};

/** @} */

#endif // PAWS_SCRIPT_H
