/*
 * pscharquestmgr.h
 *
 * Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PSCHARQUESTMGR_H__
#define __PSCHARQUESTMGR_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/weakref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/charmessages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psquest.h"


class gemObject;

#define PSQUEST_DELETE   'D'
#define PSQUEST_ASSIGNED 'A'
#define PSQUEST_COMPLETE 'C'

/**
 * This structure tracks assigned quests.
 */
struct QuestAssignment
{
    /// Character id of player who assigned quest to this player.  This is used to make sure you cannot get two quests from the same guy at the same time.
    PID assigner_id;
    /// This status determines whether the quest was assigned, completed, or is marked for deletion.
    char status;
    /// Dirty flag determines minimal save on exit
    bool dirty;
    /// When a quest is completed, often it cannot immediately be repeated.  This indicate the time when it can be started again.
    unsigned long lockout_end;
    /// To avoid losing a chain of responses in a quest, last responses are stored per assigned quest.
    int last_response;
    /// To avoid losing a chain of responses in a quest, last responses are stored per assigned quest.
    PID last_response_from_npc_pid;

    /// In case this questassignment was completed it will contain the order of completion.
    unsigned int completionOrder;

    /// Since "quest" member can be nulled without notice, this accessor function attempts to refresh it if NULL
    csWeakRef<psQuest> &GetQuest();

    /**
     * Checks if the quest(/step) assignment is marked as complete.
     *
     * @return True if the quest assignment is marked as complete.
     */
    bool IsCompleted();

    void SetQuest(psQuest* q);
protected:

    /// Quest ID saved in case quest gets nulled out from us
    int quest_id;

    /// Weak pointer to the underlying quest relevant here
    csWeakRef<psQuest> quest;
};


//-----------------------------------------------------------------------------
/** Class to manager a characters quest details
*/
class psCharacterQuestManager
{
public:
    /** Destroy the manager and delete all the assigned quests.
    */
    ~psCharacterQuestManager();

    /** Setup this manager for a character.
    *
    *   @param owner The characther that this quest manager is for.
    */
    void Initialize(psCharacter* owner);

    /** Load the quests from the database.
    *   Make sure that the owner has a valid PID before loading because it uses that to
    *   figure out what to load.
    */
    bool LoadQuestAssignments();

    /** Check to see if a quest is assigned.
    *
    *   @param id The unique database ID the quest has.
    *
    *   @return  A valid QuestAssignment if the quest had been assigned.
    *           Otherwise will return NULL if the quest has not been assigned.
    */
    QuestAssignment* IsQuestAssigned(int id);

    /**
     * Assign a quest.
     *
     * This assigns a quest to the character.
     *
     * @param quest             The quest object that will be assigned to the character.
     * @param assignerId        The PID of the character that assigned the quest.
     *
     * @return A QuestAssignement object for the quest.
     */
    QuestAssignment* AssignQuest(psQuest* quest, PID assignerId);

    /** Complete a quest.
    *   This sets a quest as completed for this character.
    *   @param quest The quest object that was completed.
    *
    *   @return true if the quest complete was successful, false otherwise.
    */
    bool CompleteQuest(psQuest* quest);

    /** Discard a quest
    *
    *   @param q The QuestAssignment object for the quest we want to discard.
    *   @param force Force the quest to be discarded regardless of any conditions.
    */
    void DiscardQuest(QuestAssignment* q, bool force = false);

    /**
     * Discard a quest.
     *
     * @param quest             The Quest object for the quest we want to discard.
     * @param force             Force the quest to be discarded regardless of any conditions.
     */
    bool DiscardQuest(psQuest* quest, bool force = false);

    /** Set an assigned quest last response.
    *
    *   @param quest The quest object to set the last response for.
    *   @param response The UID of the response used.
    *   @param npc The gemObject of the npc that was used.
    *
    *   @return true if response could be set.  false if it could not be.
    */
    bool SetAssignedQuestLastResponse(psQuest* quest, int response, gemObject* npc);

    /** Gets the number of currently assigned quests.
    *
    *   @return The number of assigned quests.
    */
    size_t GetNumAssignedQuests()
    {
        return assignedQuests.GetSize();
    }

    /** Get the response of a particular quest.
    *
    *   @param i The index into the list of quests to look for response.
    *
    *   @return The last response ID for that quest or -1 if quest could not be found.
    */
    int GetAssignedQuestLastResponse(size_t i);

    /**
     * Update the quest assignment list.
     *
     * This updates the database to write the current quest assignment lists for the
     * character.
     *
     * @param forceUpdate      If set to true, write the quest information even if it is not 'dirty'
     *
     * @return true if the quest information successfully written to database.
     */
    bool UpdateQuestAssignments(bool forceUpdate = false);

    /** Create a network message for all the assigned quests.
    *
    *   @param quests  The network message to populate with the current quest assignment information.
    *   @param cNum The client number the network message is for.
    *
    *   @return The number of quests that have been assigned.
    */
    size_t GetAssignedQuests(psQuestListMessage &quests, int cNum);

    /** Get a list of quests.
    *
    *   @return A csArray of the current QuestAssignments
    */
    csArray<QuestAssignment*> &GetAssignedQuests()
    {
        return assignedQuests;
    }

    /** Checks to see if a quest has been assigned.
    *
    *   @param quest The quest object to check if is assigned or not.
    *
    *   @return true if the quest status is PSQUEST_ASSIGNED
    */
    bool CheckQuestAssigned(psQuest* quest);

    /** Checks to see if a quest has been completed.
    *
    *   @param quest The quest object to check if it has been completed or not.
    *
    *   @return true if the quest status is PSQUEST_COMPLETE
    */
    bool CheckQuestCompleted(psQuest* quest);

    /** This incorrectly named function checks if the npc (assigner_id) is supposed to answer
     *  in the (parent)quest at this moment.
     *
     *  @param quest The quest object to check.
     *  @param assignerId The character that assigned the quest.
     *
     *  @return True if the assigner is supposed to answer?
     */
    bool CheckQuestAvailable(psQuest* quest, PID assignerId);

    /** Checks to see how many quests complete.
    *
    *   @param category The category of quests to check against.
    *
    *   @return The number of quests that have been complete in the given category.
    */
    int NumberOfQuestsCompleted(csString category);



private:
    csArray<QuestAssignment*> assignedQuests;   ///< The current list of assigned quests.

    psCharacter* owner;                         ///< The owner of this quest list.
};
//-----------------------------------------------------------------------------

#endif
