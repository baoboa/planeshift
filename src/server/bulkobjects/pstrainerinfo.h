/*
 * pstrainerinfo.h
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
#ifndef __PSTRAINERINFO_H__
#define __PSTRAINERINFO_H__
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
#include "psskills.h"

class psSkillInfo;

struct psTrainerSkill
{
    psSkillInfo* skill;
    int min_rank;
    int max_rank;
    float min_faction;
};


class psTrainerInfo : public csRefCount
{
public:
    bool Load(PID pid);
    /// determines if the player can train this skill
    bool CanTrainSkill(PSSKILL skill, unsigned int rank, float faction);

private:
    csPDelArray<psTrainerSkill> skills;

    psSkillInfo* FindSkill(int id);
    psSkillInfo* FindSkill(const csString &name);
};

#endif
