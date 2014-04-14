/*
 * pstrade.h
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

#ifndef __PSTRADE_H__
#define __PSTRADE_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <idal.h>
#include "util/mathscript.h"

//=============================================================================
// Local Includes
//=============================================================================


class psTradeCombinations;

//-----------------------------------------------------------------------------

/**
 * \addtogroup bulkobjects
 * @{ */

/**
 * This class holds the master list of all trade combinations possible in the game.
 */
class psTradeCombinations
{
public:
    psTradeCombinations();
    ~psTradeCombinations();

    bool Load(iResultRow &row);

    uint32 GetId() const
    {
        return id;
    }
    uint32 GetPatternId() const
    {
        return patternId;
    }
    uint32 GetResultId() const
    {
        return resultId;
    }
    int GetResultQty() const
    {
        return resultQty;
    }
    uint32 GetItemId() const
    {
        return itemId;
    }
    int GetMinQty() const
    {
        return minQty;
    }
    int GetMaxQty() const
    {
        return maxQty;
    }

protected:
    uint32 id;
    uint32 patternId;
    uint32 resultId;
    int resultQty;
    uint32 itemId;
    int minQty;
    int maxQty;
};

/**
 * This class holds the master list of all trade transformatations possible in the game.
 * This class is read only since it is cached and shared by multiple users.
 */
class psTradeTransformations : public iScriptableVar
{
public:
    psTradeTransformations();
    psTradeTransformations(uint32 rId, int rQty, uint32 iId, int iQty, char* tPoints);
    ~psTradeTransformations();

    bool Load(iResultRow &row);

    uint32 GetId() const
    {
        return id;
    }
    uint32 GetPatternId() const
    {
        return patternId;
    }
    uint32 GetProcessId() const
    {
        return processId;
    }
    uint32 GetResultId() const
    {
        return resultId;
    }

    int GetResultQty() const
    {
        return resultQty;
    }

    uint32 GetItemId() const
    {
        return itemId;
    }
    void SetItemId(uint32 newItemId)
    {
        itemId = newItemId;
    }

    int GetItemQty() const
    {
        return itemQty;
    }
    void SetItemQty(int newItemQty)
    {
        itemQty = newItemQty;
    }

    float GetItemQualityPenaltyPercent() const
    {
        return penaltyPct;
    }

    int GetTransPoints() const
    {
        MathEnvironment env;
        return transPoints->Evaluate(&env);
    }

    /**
     * Cache flag is used for garbage collection.
     *
     * If true transformation is cached and should not be deleted after use
     * otherwise it needs to be cleaned up
     */
    int GetTransformationCacheFlag()
    {
        return transCached;
    }

    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{

    /**
     * Returns the name of the current transform.
     *
     * @note Needed for iScriptableVar.
     * @return just "transform".
     */
    const char* ToString();

    /**
     * Does nothing right now just returns 0 for anything passed.
     *
     * @note Needed for iScriptableVar.
     */
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);

    /**
     * Returns the requested variable stored in this transform.
     *
     * @note Needed for iScriptableVar.
     *
     * @param env The MathEnviroment.
     * @param ptr A pointer to a char array stating the requested variable.
     * @return A double with the value of the requested variable.
     */
    double GetProperty(MathEnvironment* env, const char* ptr);
    ///@}

protected:
    uint32 id;
    uint32 patternId;
    uint32 processId;
    uint32 resultId;
    int resultQty;
    uint32 itemId;
    int itemQty;
    float penaltyPct;
    MathExpression* transPoints;

private:
    /**
     * Cache flag is used for garbage collection.
     *
     * If true transformation is cached and should not be deleted after use
     * otherwise it needs to be cleaned up
     */
    bool transCached;
};

/**
 * This class holds the master list of all trade processes possible in the game.
 *
 * This class is read only since it is cached and shared by multiple users.
 */
class psTradeProcesses : public iScriptableVar
{
public:
    psTradeProcesses();
    ~psTradeProcesses();

    bool Load(iResultRow &row);

    uint32 GetProcessId() const
    {
        return processId;
    }

    csString GetName() const
    {
        return name;
    }
    csString GetAnimation() const
    {
        return animation;
    }
    uint32 GetWorkItemId() const
    {
        return workItemId;
    }
    uint32 GetEquipementId() const
    {
        return equipmentId;
    }
    const char* GetConstraintString() const
    {
        return constraints;
    }
    uint32 GetGarbageId() const
    {
        return garbageId;
    }
    int GetGarbageQty() const
    {
        return garbageQty;
    }
    int GetPrimarySkillId() const
    {
        return priSkillId;
    }
    int GetSubprocessId() const
    {
        return subprocess;
    }
    unsigned int GetMinPrimarySkill() const
    {
        return minPriSkill;
    }
    unsigned int GetMaxPrimarySkill() const
    {
        return maxPriSkill;
    }
    int GetPrimaryPracticePts() const
    {
        return priPracticePts;
    }    int GetPrimaryQualFactor() const
    {
        return priQualFactor;
    }
    int GetSecondarySkillId() const
    {
        return secSkillId;
    }
    unsigned int GetMinSecondarySkill() const
    {
        return minSecSkill;
    }
    unsigned int GetMaxSecondarySkill() const
    {
        return maxSecSkill;
    }
    int GetSecondaryPracticePts() const
    {
        return secPracticePts;
    }
    int GetSecondaryQualFactor() const
    {
        return secQualFactor;
    }
    csString &GetRenderEffect()
    {
        return renderEffect;
    }

    /**
     * Gets the script associated to this process.
     *
     * This can be set from the users of the class and it's a cache for the mathscript.
     *
     * @return A pointer to the associated mathscript, if any.
     */
    MathScript* GetScript()
    {
        return script;
    }

    /**
     * Gets the name of the script associated to this process.
     *
     * It will be run after the process is done during the generation of
     * the new item.
     *
     * @return A csString with the name of the script as taken from the
     *         database.
     */
    csString GetScriptName() const
    {
        return scriptName;
    }

    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    /**
     * Returns the name of the current process.
     *  @note Needed for iScriptableVar.
     *  @return the name of the process.
     */
    const char* ToString();

    /**
     * Does nothing right now just returns 0 for anything passed.
     *
     * @note Needed for iScriptableVar.
     */
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);

    /**
     * Returns the requested variable stored in this process.
     *
     *  @note Needed for iScriptableVar.
     *
     *  @param env The MathEnviroment.
     *  @param ptr A pointer to a char array stating the requested variable.
     *  @return A double with the value of the requested variable.
     */
    double GetProperty(MathEnvironment* env, const char* ptr);
    ///@}

protected:
    uint32 processId;
    int subprocess;
    csString name;
    csString animation;
    uint32 workItemId;
    uint32 equipmentId;
    csString constraints;
    uint32 garbageId;
    int garbageQty;
    int priSkillId;
    int minPriSkill;
    int maxPriSkill;
    int priPracticePts;
    int priQualFactor;
    int secSkillId;
    int minSecSkill;
    int maxSecSkill;
    int secPracticePts;
    int secQualFactor;
    csString renderEffect;
    csString scriptName;
    MathScript* script;
};

/**
 * This class holds the master list of all trade patterns possible in the game.
 */
class psTradePatterns
{
public:
    psTradePatterns();
    ~psTradePatterns();
    bool Load(iResultRow &row);

    uint32 GetId() const
    {
        return id;
    }
    uint32 GetGroupPatternId() const
    {
        return groupPatternId;
    }
    const char* GetPatternString() const
    {
        return patternName;
    }
    uint32 GetDesignItemId() const
    {
        return designItemId;
    }
    float GetKFactor() const
    {
        return KFactor;
    }

protected:
    uint32 id;
    uint32 groupPatternId;
    csString patternName;
    uint32 designItemId;
    float KFactor;
};

//-----------------------------------------------------------------------------

/**
 * Each item has a list of items required for its construction.
 */
struct CombinationConstruction
{
    uint32 resultItem;
    int resultQuantity;
    csPDelArray<psTradeCombinations> combinations;
};

//-----------------------------------------------------------------------------

/**
 * Each item contains the craft skills for the craft step.
 */
struct CraftSkills
{
    int priSkillId;
    int minPriSkill;
    int secSkillId;
    int minSecSkill;
};

/**
 * Each item contains craft information about a craft transformation step.
 */
struct CraftTransInfo
{
    int priSkillId;
    int minPriSkill;
    int secSkillId;
    int minSecSkill;
    csString craftStepDescription;
};

/**
 * Each item contains craft information about a craft combination.
 */
struct CraftComboInfo
{
    csArray<CraftSkills*>* skillArray;
    csString craftCombDescription;
};

/** @} */

#endif
