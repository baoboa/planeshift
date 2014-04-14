/*
 * pstrade.cpp
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include "util/log.h"
#include "util/strutil.h"

//=============================================================================
// Project Includes
//=============================================================================
#include "../psserver.h"
#include "../cachemanager.h"
#include "../globals.h"
#include "util/serverconsole.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pstrade.h"


///////////////////////////////////////////////////////////////////////
// Combinations
psTradeCombinations::psTradeCombinations()
{
    id          = 0;
    patternId   = 0;
    resultId    = 0;
    resultQty   = 0;
    itemId      = 0;
    minQty      = 0;
    maxQty      = 0;
}

psTradeCombinations::~psTradeCombinations()
{
}

bool psTradeCombinations::Load(iResultRow &row)
{
    id          = row.GetUInt32("id");
    patternId   = row.GetUInt32("pattern_id");
    resultId    = row.GetUInt32("result_id");
    resultQty   = row.GetInt("result_qty");
    itemId      = row.GetUInt32("item_id");
    minQty      = row.GetInt("min_qty");
    maxQty      = row.GetInt("max_qty");

    return true;
}

///////////////////////////////////////////////////////////////////////
// Transformations
psTradeTransformations::psTradeTransformations()
{
    id              = 0;
    patternId       = 0;
    processId       = 0;
    resultId        = 0;
    resultQty       = 0;
    itemId          = 0;
    itemQty         = 0;
    penaltyPct      = 0.0;
    transPoints     = NULL;
    transCached     = true;
}

// Non-cache constructor
psTradeTransformations::psTradeTransformations(uint32 rId, int rQty, uint32 iId, int iQty, char* tPoints)
{
    id              = 0;
    patternId       = 0;
    processId       = 0;
    resultId        = rId;
    resultQty       = rQty;
    itemId          = iId;
    itemQty         = iQty;
    penaltyPct      = 0.0;
    transPoints     = MathExpression::Create(tPoints);
    transCached     = false;
}

psTradeTransformations::~psTradeTransformations()
{
    if(transPoints)
    {
        delete transPoints;
    }
}

bool psTradeTransformations::Load(iResultRow &row)
{
    if(transPoints) delete transPoints;

    id           = row.GetUInt32("id");
    patternId    = row.GetUInt32("pattern_id");
    processId    = row.GetUInt32("process_id");
    resultId     = row.GetUInt32("result_id");
    resultQty    = row.GetInt("result_qty");
    itemId       = row.GetUInt32("item_id");
    itemQty      = row.GetInt("item_qty");
    transPoints  = MathExpression::Create(row["trans_points"]);
    penaltyPct   = row.GetFloat("penalty_pct");
    return true;
}

double psTradeTransformations::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "ItemQualityPenaltyPercent")
    {
        return (double)GetItemQualityPenaltyPercent();
    }
    if(property == "ItemQuantity")
    {
        return (double)GetItemQty();
    }
    if(property == "ItemID")
    {
        return (double)GetItemId();
    }
    if(property == "ResultItemID")
    {
        return (double)GetResultId();
    }
    if(property == "TransformPoints")
    {
        return (double)transPoints->Evaluate(env);
    }

    CPrintf(CON_ERROR, "psTradeTransformations::GetProperty(%s) failed\n",ptr);
    return 0;
}

double psTradeTransformations::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    CPrintf(CON_ERROR, "psTradeTransformations::CalcFunction(%s) failed\n", functionName);
    return 0;
}

const char* psTradeTransformations::ToString()
{
    return "transformation";
}


///////////////////////////////////////////////////////////////////////
// Processes
psTradeProcesses::psTradeProcesses()
{
    processId       = 0;
    subprocess      = 0;
    name           .Clear();
    animation      .Clear();
    workItemId      = 0;
    equipmentId     = 0;
    constraints    .Clear();
    garbageId       = 0;
    garbageQty      = 0;
    priSkillId      = 0;
    minPriSkill     = 0;
    maxPriSkill     = 0;
    priPracticePts  = 0;
    priQualFactor   = 0;
    secSkillId      = 0;
    minSecSkill     = 0;
    maxSecSkill     = 0;
    secPracticePts  = 0;
    secQualFactor   = 0;
    renderEffect   .Clear();
    scriptName     .Clear();
    script          = 0;
}

psTradeProcesses::~psTradeProcesses()
{
}

const char* psTradeProcesses::ToString()
{
    return name.GetData();
}

double psTradeProcesses::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    CPrintf(CON_ERROR, "psTradeProcesses::CalcFunction(%s) failed\n", functionName);
    return 0;
}

double psTradeProcesses::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "PrimarySkillId")
    {
        return GetPrimarySkillId();
    }
    else if(property == "MaxPrimarySkill")
    {
        return GetMaxPrimarySkill();
    }
    else if(property == "MinPrimarySkill")
    {
        return GetMinPrimarySkill();
    }
    else if(property == "PrimarySkillQualityFactor")
    {
        return GetPrimaryQualFactor();
    }
    else if(property == "PrimarySkillPracticePoints")
    {
        return GetPrimaryPracticePts();
    }
    else if(property == "SecondarySkillId")
    {
        return GetSecondarySkillId();
    }
    else if(property == "MaxSecondarySkill")
    {
        return GetMaxSecondarySkill();
    }
    else if(property == "MinSecondarySkill")
    {
        return GetMinSecondarySkill();
    }
    else if(property == "SecondarySkillQualityFactor")
    {
        return GetSecondaryQualFactor();
    }
    else if(property == "SecondarySkillPracticePoints")
    {
        return GetSecondaryPracticePts();
    }
    else if(property == "WorkItemId")
    {
        return GetWorkItemId();
    }
    else if(property == "EquipmentId")
    {
        return GetEquipementId();
    }

    Error2("Requested psTradeProcesses property not found '%s'", ptr);
    return 0;
}


bool psTradeProcesses::Load(iResultRow &row)
{
    processId       = row.GetUInt32("process_id");
    subprocess      = row.GetInt("subprocess_number");
    name            = row["name"];
    animation       = row["animation"];
    workItemId      = row.GetUInt32("workitem_id");
    equipmentId     = row.GetUInt32("equipment_id");
    constraints     = row["constraints"];
    garbageId       = row.GetUInt32("garbage_id");
    garbageQty      = row.GetInt("garbage_qty");
    priSkillId      = row.GetInt("primary_skill_id");
    minPriSkill     = row.GetInt("primary_min_skill");
    maxPriSkill     = row.GetInt("primary_max_skill");
    priPracticePts  = row.GetInt("primary_practice_points");
    priQualFactor   = row.GetInt("primary_quality_factor");
    secSkillId      = row.GetInt("secondary_skill_id");
    minSecSkill     = row.GetInt("secondary_min_skill");
    maxSecSkill     = row.GetInt("secondary_max_skill");
    secPracticePts  = row.GetInt("secondary_practice_points");
    secQualFactor   = row.GetInt("secondary_quality_factor");
    renderEffect    = row["render_effect"];
    scriptName      = row["script"];
    script = psserver->GetMathScriptEngine()->FindScript(scriptName);
    return script != NULL;
}

///////////////////////////////////////////////////////////////////////
// Patterns
psTradePatterns::psTradePatterns()
{
    id              = 0;
    patternName    .Clear();
    groupPatternId   = 0;
    designItemId    = 0;
    KFactor         = 0.0;
}

psTradePatterns::~psTradePatterns()
{
}

bool psTradePatterns::Load(iResultRow &row)
{
    id              = row.GetUInt32("id");
    patternName     = row["pattern_name"];
    groupPatternId   = row.GetUInt32("group_id");
    designItemId    = row.GetUInt32("designitem_id");
    KFactor         = row.GetFloat("k_factor");

    return true;
}

