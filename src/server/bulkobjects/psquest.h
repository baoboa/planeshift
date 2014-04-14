/*
 * psquest.h
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

#ifndef __PSQUEST_H__
#define __PSQUEST_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <idal.h>

//=============================================================================
// Local Includes
//=============================================================================

using namespace CS;


#define QUEST_OPT_SAVEONCOMPLETE 0x01

/// The quest is disabled and won't be loaded by the server, used for the flags column
#define PSQUEST_DISABLED_QUEST 0x00000001


class psQuestPrereqOp;
class psCharacter;
class NpcTrigger;
class psQuest;

/**
 * Utility function to parse prerequisite scripts.
 *
 * @param prerequisite The variable that will hold the parsed prerequisite
 * @param self         Pointer to the quest if used to load for a quest
 * @param script       The prerequisite to parse \<pre\>...\</pre\>.
 * @return True if successfully parsed.
 */
bool LoadPrerequisiteXML(csRef<psQuestPrereqOp> &prerequisite, psQuest* self, csString script);

/**
 * This class holds the master list of all quests available in the game.
 */
class psQuest : public CS::Utility::WeakReferenced
{
public:
    psQuest();
    virtual ~psQuest();

    bool Load(iResultRow &row);
    bool PostLoad();
    void Init(int id, const char* name);

    int GetID() const
    {
        return id;
    }
    const char* GetName() const
    {
        return name;
    }
    const char* GetImage() const
    {
        return image;
    }
    const char* GetTask() const
    {
        return task;
    }

    /**
     * Gets if the task (quest description/note) contains some text.
     *
     * @return TRUE if the task has some text, FALSE otherwise.
     */
    bool hasTaskText()
    {
        return task.Length() > 0;
    }
    void SetTask(csString mytask)
    {
        task = mytask;
    }
    psQuest* GetParentQuest() const
    {
        return parent_quest;
    }
    void SetParentQuest(psQuest* parent)
    {
        parent_quest=parent;
    }
    int GetStep() const
    {
        return step_id;
    }
    bool HasInfinitePlayerLockout() const
    {
        return infinitePlayerLockout;
    }
    unsigned int GetPlayerLockoutTime() const
    {
        return player_lockout_time;
    }
    unsigned int GetQuestLockoutTime() const
    {
        return quest_lockout_time;
    }
    unsigned int GetQuestLastActivatedTime() const
    {
        return quest_last_activated;
    }
    void SetQuestLastActivatedTime(unsigned int when)
    {
        quest_last_activated=when;
    }
    // csString QuestToXML() const;
    bool AddPrerequisite(csString prerequisitescript);
    bool AddPrerequisite(csRef<psQuestPrereqOp> op);
    void AddTriggerResponse(NpcTrigger* trigger, int responseID);
    void AddSubQuest(int id)
    {
        subquests.Push(id);
    }

    /**
     * Returns an ordered list of the subquests of this quest (so it's steps).
     *
     * @return A reference to an array containing the id of the subquests.
     */
    csArray<int> &GetSubQuests()
    {
        return subquests;
    }

    /**
     * Return the prerequisite for this quest.
     *
     * @return The prerequisite for this quest.
     */
    psQuestPrereqOp* GetPrerequisite()
    {
        return prerequisite;
    }
    csString GetPrerequisiteStr();
    const csString &GetCategory() const
    {
        return category;
    }

    /**
     * Check if the quest is active (and also it's parents).
     *
     * A quest to be active must be active itself and, if so, also it's parents (most probably earlier steps)
     * must be active themselves so check back to them if this quest is active else return as not active directly.
     *
     * @return The active status of the quest.
     */
    bool Active()
    {
        return active ? (parent_quest ? parent_quest->Active() : active) : active;
    }

    /**
     * Sets activation status of the quest.
     */
    void Active(bool state)
    {
        active = state;
    }

protected:
    int id;
    csString name;
    csString task;
    csString image;
    int flags;
    psQuest* parent_quest;
    int step_id;
    csRef<psQuestPrereqOp> prerequisite;
    csString category;
    csString prerequisiteStr;
    bool infinitePlayerLockout;

    unsigned int player_lockout_time;
    unsigned int quest_lockout_time;
    unsigned int quest_last_activated;

    struct TriggerResponse
    {
        NpcTrigger* trigger;
        int responseID;
    };

    csArray<TriggerResponse> triggerPairs;
    csArray<int> subquests;

    bool active;
};

#endif
