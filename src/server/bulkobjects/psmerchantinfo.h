/*
 * psmerchantinfo.h
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __PSMERCHANTINFO_H__
#define __PSMERCHANTINFO_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csutil/refcount.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

/**
 * \addtogroup bulkobjects
 * @{ */

struct psItemCategory
{

    unsigned int id;                 ///< Unique identifier for the category
    csString     name;               ///< Human readable name of category
    unsigned int repairToolStatId;   ///< Item_stats id of item required to do a repair on this category.
    bool         repairToolConsumed; ///< Flag to tell us whether the repair tool is consumed in the repair or not.  (Kit or Tool)
    int          repairSkillId;      ///< ID of skill which is used to calculate result of repair
    int          repairDifficultyPct;///< Difficulty level to repair this category of items
    int          identifySkillId;    ///< ID of skill checked when item is examined by player
    int          identifyMinSkill;   ///< Minimum skill level to allow player to identify details of examined item
};



class psMerchantInfo : public csRefCount
{
public:
    /**
     * A character is defined to be a merchant if there are
     * merchant item categories for the character.
     *
     * @param pid The characterid to check.
     * @return Return true if the character is a merchant.
     */
    bool Load(PID pid);

    psItemCategory* FindCategory(int id);
    psItemCategory* FindCategory(const csString &name);

    csArray<psItemCategory*> categories;

};

/** @} */

#endif
