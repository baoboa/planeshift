/*
 * psskills.h
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

#ifndef __PSSKILLS_H__
#define __PSSKILLS_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"

#include "rpgrules/psmoney.h"
#include "util/mathscript.h"

//=============================================================================
// Local Includes
//=============================================================================

//used for testing the degree of hardcoding of skills
//typedef int PSSKILL;
//#define PSSKILL_NONE -1

enum PSSKILL
{
    PSSKILL_NONE            =   -1,
    PSSKILL_LIGHTARMOR      =   7,
    PSSKILL_MEDIUMARMOR     =   8,
    PSSKILL_HEAVYARMOR      =   9,
    PSSKILL_AGI             =   46,
    PSSKILL_CHA             =   47,
    PSSKILL_END             =   48,
    PSSKILL_INT             =   49,
    PSSKILL_STR             =   50,
    PSSKILL_WILL            =   51,
};
//These flags define the possible skills categories
enum PSSKILLS_CATEGORY
{
    PSSKILLS_CATEGORY_STATS = 0,//Intelligence, Charisma, etc.
    PSSKILLS_CATEGORY_COMBAT,
    PSSKILLS_CATEGORY_MAGIC,
    PSSKILLS_CATEGORY_JOBS,        //Crafting, mining, etc.
    PSSKILLS_CATEGORY_VARIOUS,
    PSSKILLS_CATEGORY_FACTIONS
};

class psSkillInfo
{
public:
    psSkillInfo();
    ~psSkillInfo();

    ///  The new operator is overriden to call PoolAllocator template functions
//    void *operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
//    void operator delete(void *);

    PSSKILL id;
    csString name;
    csString description;
    int practice_factor;
    int mental_factor;
    psMoney price;
    PSSKILLS_CATEGORY category;
    /// The base cost that this skill requires. Used to calculate costs for next rank.
    int baseCost;
    csString costScript; ///< Keeps the script used to calculate the cost of this skill.

private:
    /// Static reference to the pool for all psSkillInfo objects
//    static PoolAllocator<psSkillInfo> skillinfopool;

};



#endif


