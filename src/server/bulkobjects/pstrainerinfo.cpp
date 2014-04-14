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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/psdatabase.h"

#include "../cachemanager.h"
#include "../psserver.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pstrainerinfo.h"

/**
 * A character is defined to be a trainer if there are
 * trainer item categories for the character.
 *
 * @return Return true if the character is a trainer.
 */
bool psTrainerInfo::Load(PID pid)
{
    bool isTrainer = false;

    Result trainerSkills(db->Select("SELECT * from trainer_skills where player_id=%u", pid.Unbox()));
    if(trainerSkills.IsValid())
    {
        int i,count=trainerSkills.Count();

        for(i=0; i<count; i++)
        {
            psSkillInfo* skillInfo = FindSkill(atoi(trainerSkills[i]["skill_id"]));
            if(!skillInfo)
            {
                Error1("Error! Skill could not be loaded. Skipping.\n");
                continue;
            }
            psTrainerSkill* skill = new psTrainerSkill;
            skill->skill = skillInfo;
            skill->max_rank = atoi(trainerSkills[i]["max_rank"]);
            skill->min_rank = atoi(trainerSkills[i]["min_rank"]);
            skill->min_faction = atof(trainerSkills[i]["min_faction"]);
            skills.Push(skill);
            isTrainer = true;
        }
    }

    return isTrainer;
}

bool psTrainerInfo::CanTrainSkill(PSSKILL skill, unsigned int rank, float faction)
{
    csArray<psTrainerSkill*, csPDelArrayElementHandler<psTrainerSkill*> >::Iterator iter = skills.GetIterator();

    while(iter.HasNext())
    {
        psTrainerSkill* info = iter.Next();

        if(info->skill->id == skill)
        {
            /*
                printf("    Matching Skills\n" );
                printf("    Compare rank(%d) >= (%d)info->min_rank\n", rank , info->min_rank );
                printf("    Compare rank(%d) < (%d)info->max_rank\n", rank , info->max_rank );
                printf("    Compare faction(%f) >= (%f)info->min_faction\n", faction, info->min_faction );
            */
            if(rank >= (unsigned int)info->min_rank && rank < (unsigned int)info->max_rank && faction >= info->min_faction)
                return true;
        }
    }
    return false;
}

psSkillInfo* psTrainerInfo::FindSkill(int id)
{
    return psserver->GetCacheManager()->GetSkillByID(id);
}

psSkillInfo* psTrainerInfo::FindSkill(const csString &name)
{
    return psserver->GetCacheManager()->GetSkillByName(name);
}
