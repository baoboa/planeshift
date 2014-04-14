/*
 * psskills.cpp
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


#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"



/*****
// Definition of the itempool for psCharacter(s)
PoolAllocator<psSkillInfo> psSkillInfo::skillinfopool;

void *psSkillInfo::operator new(size_t allocSize)
{
    CS_ASSERT(allocSize<=sizeof(psSkillInfo));
    return (void *)skillinfopool.CallFromNew();
}

void psSkillInfo::operator delete(void *releasePtr)
{
    skillinfopool.CallFromDelete((psSkillInfo *)releasePtr);
}
********/

psSkillInfo::psSkillInfo()
    :id(PSSKILL_NONE),
     practice_factor(0),
     mental_factor(0),
     price(42),
     category(PSSKILLS_CATEGORY_STATS),
     baseCost(0)
{
}

psSkillInfo::~psSkillInfo()
{
}










