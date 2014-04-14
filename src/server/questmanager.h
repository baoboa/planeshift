/*
 * questmanager.h
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
#ifndef __QUESTMANAGER_H__
#define __QUESTMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psquest.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class


class Client;
class psItemStats;

struct QuestRewardOffer;
struct QuestRewardItem;
class gemNPC;
class NpcDialogMenu;
class NpcResponse;
class WordArray;
class psString;

/**
 * This class handles quest management for the player,
 * tracking who has what quests assigned, etc.
 */
class QuestManager : public MessageManager<QuestManager>
{
protected:
    csPDelArray<QuestRewardOffer>  offers;

    csString lastError;     ///< Last error message to send to client on loadquest.

    CacheManager* cacheManager;

    /**
     * Load all scripts from db
     */
    bool LoadQuestScripts();
    /** Does a first parsing of the script.
     *  @note curretly it just find substeps and creates the related quest in advance, so they can be used
     *        anywhere in prerequisites.
     *  @param mainQuest A pointer to the main psQuest. This can't be null it must be valid!
     *  @param script The entire quest script.
     *  @return the line number of the parse error if any.
     */
    int  PreParseQuestScript(psQuest* mainQuest,const char* script);
    void CutOutParenthesis(csString &response, csString &within,char start_char,char end_char) const;

    bool GetResponseText(csString &block,csString &response,csString &file_path,
                         csString &him, csString &her, csString &it, csString &them) const;
    bool BuildTriggerList(csString &block,csStringArray &list) const;

    bool BuildMenu(const csString &block,const csStringArray &list, psQuest* quest, NpcDialogMenu* menu) const;

    int GetNPCFromBlock(WordArray words,csString &current_npc);
    bool ParseItemList(const csString &input, csString &parsedItemList);
    bool ParseItem(const char* text, psStringArray &xmlItems, psMoney &money);

    NpcResponse* AddResponse(const csString &current_npc,const char* response_text,
                             int &last_response_id, psQuest* quest,
                             csString &him, csString &her, csString &it, csString &them, csString &file_path);
    bool         AddTrigger(const csString &current_npc,const char* trigger,
                            int prior_response_id,int trig_response, psQuest* quest, const psString &postfix);
    void         MergeTriggerMenus(NpcDialogMenu* pending_menu, const csString &current_npc);

    void GetNextScriptLine(psString &scr, csString &block, size_t &start, int &line_number);
    bool PrependPrerequisites(csString &substep_requireop,
                              csString &response_requireop,
                              bool quest_assigned_already,
                              NpcResponse* last_response,
                              psQuest* mainQuest);

    bool HandlePlayerAction(csString &block,
                            size_t &which_trigger,
                            csString &current_npc,
                            csStringArray &pending_triggers);

    bool HandleScriptCommand(csString &block,
                             csString &response_requireop,
                             csString &substep_requireop,
                             NpcResponse* last_response,
                             psQuest* mainQuest,
                             bool &quest_assigned_already,
                             psQuest* quest);

    /** Parses a require command inner part.
     *  @note this functions doesn't handle the negations which are handled
     *        by HandleRequireCommand
     *  @param block A csString which is the block to be parsed stripped of "require" and the leading no, if any.
     *  @param result A bool which will store if it was possible to parse the passed string
     *  @param mainQuest A pointer to the main quest, used for quest name autocompletion
     *  @return A csString which has the xml result of the parsing
     */
    csString ParseRequireCommand(csString &block, bool &result, psQuest* mainQuest);

    /** Parses a require command.
     *  @note this function handles no and not and calls parseRequireCommand for the inner parsing
     *  @param block A csString which is the block to be parsed stripped of "require".
     *  @param response_requireop A csString where to append the response prerequisites.
     *  @param mainQuest A pointer to the main quest, used for quest name autocompletion
     *  @return A boolean indicating if the operation was successfull.
     */
    bool HandleRequireCommand(csString &block, csString &response_requireop, psQuest* mainQuest);

    /** Checks if the quest name is existant, if it isn't it attemps to complete it
     *  with the name of the passed quest but only if the autocompleted version is found.
     *  @param questname Name of the quest we have to check for. Output is written directly
     *                   in the passed variable.
     *  @param mainQuest The quest where this questname was found to check for in quest completion.
     */
    void AutocompleteQuestName(csString &questname, psQuest* mainQuest);

    void HandleQuestInfo(MsgEntry* pMsg,Client* client);
    void HandleQuestReward(MsgEntry* pMsg,Client* client);

private:
    /**
     * This class is only for internal use for HandleQuestInfo and
     * it's used to allow ordered population of the quest notes data.
     */
    class TaskEntry
    {
    public:
        csString text;        ///< The text contained by this note.
        int completionOrder;  ///< The completion order of the step, used to order entries by completion.
        int questOrder;       ///< The quest step order used in case the completion order is invalid.

        /**
         * Returns if the entry comes before another, notice
         * that here the effect of the ordering is inverted than
         * what would be expected, this is because items with a lower
         * number must come before items with an higher number and
         * the priority queue implements higher number comes first.
         *
         * @param other The other task entry to compare with.
         * @return TRUE if the current TaskEntry comes before the
         *              other TaskEntry.
         */
        bool operator<(const TaskEntry &other) const
        {
            //first check if the completion order is different
            //in that case use that as order
            if(completionOrder != other.completionOrder)
            {
                return completionOrder > other.completionOrder;
            }
            //if they are the same check for the case the completion
            //order is invalid and check if quest order is different
            return questOrder > other.questOrder;
        }
    };

public:

    QuestManager(CacheManager* cachemanager);
    virtual ~QuestManager();

    bool Initialize();

    /** Parase a new quest script.
     */
    int  ParseQuestScript(int id,const char* script);

    /** Parase a new custom script.
     *
     *  This is the same as ParseQuestScript, but with all the limitaiton
     *  that is needed to allow players to script this.
     */
    int  ParseCustomScript(int id, const csString& current_npc, const char* script);

    /**
     */
    void Assign(psQuest* quest, Client* who, gemNPC* assigner,csTicks timeDelay=0);

    /**
     */
    bool Complete(psQuest* quest, Client* who, csTicks timeDelay = 0);

    /** Discards the requested step this is used by dictionary.
     *
     * @todo evaluate if this should be moved somewhere else togheter with the two above.
     *
     * @param quest            The quest we are discarding.
     * @param who              The client which is discarding the quest.
     * @param timeDelay        The delay for messages.
     */
    bool Uncomplete(psQuest* quest, Client* who, csTicks timeDelay = 0);


    void OfferRewardsToPlayer(Client* who, csArray<QuestRewardItem>& offer, csTicks& timeDelay);
    bool GiveRewardToPlayer(Client* who, QuestRewardItem& reward);
    bool LoadQuestScript(int id);
    const char* LastError()
    {
        return lastError.GetData();
    }
};


#endif

