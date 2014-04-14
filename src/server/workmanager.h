/*
* workmanager.h
*
* Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __WORKMANAGER_H__
#define __WORKMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class
#include "gem.h"

class psWorkGameEvent;
class WorkManager;
class psItem;
class Client;
struct CombinationConstruction;

// Define the work event types
#define MANUFACTURE 0                   ///< Digging, collecting, farming
#define PRODUCTION  1                   ///< Crafting
#define LOCKPICKING 2                   ///< Picking of locks
#define CLEANUP     3                   ///< Cleaning up of public containers
#define REPAIR      4                   ///< Repairing items takes time too

#define GARBAGE_PATTERNID   1           ///< Define for hard coded pattern IDs
#define CLEANUP_DELAY       600         ///< Seconds to wait before performing cleanup event
//#define CLEANUP_DELAY       10 ///< for testing only

/**
 * Holds the possible return values for a test to see if an item is transformable.
 *
 * This is binary since each transform atempt could have multiple reasons for failure
 */
enum TradePatternMatch
{
    TRANSFORM_MATCH                 = 0x0000,       ///< There was a transform that matched correctly
    TRANSFORM_UNKNOWN_PATTERN       = 0x0001,       ///< There was no pattern found
    TRANSFORM_UNKNOWN_ITEM          = 0x0002,       ///< Item is not used with any available transform.
    TRANSFORM_MISSING_ITEM          = 0x0004,       ///< No item found to use with work item.
    TRANSFORM_UNKNOWN_WORKITEM      = 0x0008,       ///< No work item container choosen.
    TRANSFORM_MISSING_EQUIPMENT     = 0x0010,       ///< Transform found but not correct equipment
    TRANSFORM_BAD_TRAINING          = 0x0020,       ///< Did not meet required skills.
    TRANSFORM_BAD_SKILLS            = 0x0040,       ///< Did not meet required skills.
    TRANSFORM_OVER_SKILLED          = 0x0080,       ///< Over skilled for this task.
    TRANSFORM_FAILED_CONSTRAINTS    = 0x0100,       ///< Failed one or more contstraints on the transform
    TRANSFORM_BAD_QUANTITY          = 0x0200,       ///< Item was ok for transform but quantity was not.
    TRANSFORM_BAD_COMBINATION       = 0x0400,       ///< Combination items or ammounts not correct.
    TRANSFORM_TOO_MANY_ITEMS        = 0x0800,       ///< Tried to transform too many items.
    TRANSFORM_BAD_USE               = 0x1000,       ///< Use of item or ammounts not correct.
    TRANSFORM_NO_STAMINA            = 0x2000,       ///< Too tired to do trade skill work.
    TRANSFORM_GONE_WRONG            = 0x4000,       ///< Correct ingredients but wrong number or wrong process
    TRANSFORM_GARBAGE               = 0x8000        ///< There was a any item garbage transform that matched correctly
};

/**
 * Holds the possible transformation types.
 */
enum TradeTransfomType
{
    TRANSFORMTYPE_UNKNOWN=0,        ///< A unknown tranformation
    TRANSFORMTYPE_AUTO_CONTAINER,   ///< Transforming an item by putting it into auto-transform container
    TRANSFORMTYPE_SLOT,             ///< Transforming an item in an equipped slot
    TRANSFORMTYPE_CONTAINER,        ///< Transforming an item that is in a container
    TRANSFORMTYPE_SLOT_CONTAINER,   ///< Transforming an item that is in a container in an equipped slot
    TRANSFORMTYPE_TARGETSLOT,       ///< Transforming an item that is in a targetted actors equipped slot
    TRANSFORMTYPE_TARGET,           ///< Transforming an item that is targeted
    TRANSFORMTYPE_TARGET_TO_NPC,    ///< Transforming an item that is targeted to an npc type
    TRANSFORMTYPE_SELF_CONTAINER    ///< Transforming a container to another item
};

//-----------------------------------------------------------------------------

/**
 * This class keeps natural resource concentrations across the world.
 */
struct NaturalResource
{
    int          sector;                ///< The id of the sector this resource is in.
    csVector3    loc;                   ///< Centre point of resource location.
    float        radius;                ///< Radius around the centre where resource can be found.
    float        visible_radius;        ///< Radius around the centre where resource is visible
    float        probability;           ///< Probability of finding resource on attempt.
    psSkillInfo* skill;                 ///< Skill used to harvest resource.
    int          skill_level;           ///< Skill level required to be able to harvest resource.
    unsigned int item_cat_id;           ///< Category of tool needed for the ressource
    float        item_quality;          ///< Quality of equipment for the ressource
    csString     anim;                  ///< Name of animation to play while harvesting
    int          anim_duration_seconds; ///< Length of time the animation should play.
    int          reward;                ///< Item ID of the reward
    csString     reward_nickname;       ///< Item name of the reward
    size_t       action;                ///< The action you need to take to get this resource.
    ///< Id Corresponding to resourcesActions index.
};


/**
 * This class keeps the hit of natural resources found for the player and allows ordering of them in an array
 */
class NearNaturalResource
{
public:
    NearNaturalResource(NaturalResource* res, float distance) : resource(res), dist(distance) {}
    NaturalResource* resource;
    float dist;
    bool operator<(const NearNaturalResource &oth) const
    {
        if(oth.dist > dist)
            return true;
        if(oth.resource->probability > resource->probability)
            return true;
        if(oth.resource->skill_level > resource->skill_level)
            return true;
        return false;
    }
};

//-----------------------------------------------------------------------------

struct constraint
{
    bool (*constraintFunction)(WorkManager* that, char* param);
    const char* name;
    const char* message;
};

//-----------------------------------------------------------------------------

/**
 * This class handles all calculations around work, using statistics
 * and information from the pspccharacterinfo Prop Classes for both
 * the worker and the target.
 */
class WorkManager : public MessageManager<WorkManager>
{
public:

    WorkManager(CacheManager* cachemanager, EntityManager* entitymanager);
    virtual ~WorkManager();

//-----------------------------------------------------------------------------
// Entry points
//-----------------------------------------------------------------------------

    /**
     * Handles using an item for working.
     *
     * This is called when an item is tagetted and the use command issued.
     *
     * @param client            The client that placed the item inside
     */
    void HandleUse(Client* client);

    /**
     * Start Combine work
     * This gets called when the player tries to combine items in a container using /combine command.
     *
     * @param client            The client that combines
     */
    void HandleCombine(Client* client);

    /**
     * Begins construction work.
     *
     * This is called when a player attempts to build a constructable item.
     *
     * @param client            The client that construct
     */
    void HandleConstruct(Client* client);

    /**
     * Start a work event for this client.
     *
     * This is called when an item is placed
     * in a container.  If the container is an auto-transform container it can transform items
     * automatically ( ie placing items in automatically triggers the transformation process to start ).
     *
     * @param client            The client that placed the item inside
     * @param container         The work item container that can transform an item.
     * @param autoItem          The item that was placed inside and to be transformed.
     * @param count The stack count of the item placed in.
     */
    void StartAutoWork(Client* client, gemContainer* container, psItem* autoItem, int count);

    /**
     * Checks to see if the progression script generated craft work can be done.
     *
     * @param client            The client for the actor that initiates that progressions script
     * @param target            Targetted item
     * @param pattern           Pattern name passed in progressions cript
     * @return False if there is a problem doing the craft.
     */
    bool StartScriptWork(Client* client, gemObject* target, csString pattern);

    /**
     * Stop work event.
     *
     * This is called when a client removes an item from a container.
     *
     * @param client            The client that removed the item.
     * @param item              The item that was being transformed.
     */
    void StopWork(Client* client, psItem* item);

    /** @name Work Event Handlers
      *  These are the functions fired when a psWorkGameEvent is Triggered.
      * @{ */
    /**
     * Handles a transformation/combination event.
     *
     * Basically the manufacturing events.
     *
     * @param workEvent The work event that was in the queue to fire.
     */
    void HandleWorkEvent(psWorkGameEvent* workEvent);

    /**
     * Handles a cleanup event.
     *
     * Basically removing discarded items from public containers.
     *
     * @param workEvent The work event that was in the queue to fire.
     */
    void HandleCleanupEvent(psWorkGameEvent* workEvent);

    /**
     * Handles a resource/harvesting event.
     *
     * Basically the production events.
     *
     * @param workEvent The work event that was in the queue to fire.
     */
    void HandleProductionEvent(psWorkGameEvent* workEvent);

    /**
     * Handles a repair event, which occurs after a few seconds of repairing an item.
     *
     * This function handles the conclusion timer of when a repair is completed.
     * It is not called if the event is cancelled.  It follows the following
     * sequence of steps.
     *
     * -# The values are all pre-calculated, so just adjust the quality of the item directly.
     * -# Consume the repair required item, if flagged to do so.
     * -# Notify the user.
     *
     * @param workEvent The work event that was in the queue to fire.
     */
    void HandleRepairEvent(psWorkGameEvent* workEvent);
    void LockpickComplete(psWorkGameEvent* workEvent);

    /** @} */


    /** @name Constraint Functions
      * @{ */
    static bool constraintTime(WorkManager* that, char* param);
    static bool constraintFriends(WorkManager* that,char* param);
    static bool constraintLocation(WorkManager* that,char* param);
    static bool constraintMode(WorkManager* that,char* param);
    static bool constraintGender(WorkManager* that,char* param);
    static bool constraintRace(WorkManager* that,char* param);
    /** @} */


    /// Lockpicking
    void StartLockpick(Client* client,psItem* item);


    /**
     * Sets up the internal structure of the work manager to handle a particular client.
     *
     * This sets many variables of the workmanager to work with a single user at a time.
     *
     * @param client The client that is the current one to use.
     * @param target The object for which the client is targetting.
     *
     * @return False if there is a problem loading stuff.
     */
    bool LoadLocalVars(Client* client, gemObject* target=NULL);

    /*
     * Send clear client view message to remove items from autocontainers.
     *
     * @param slotID The slot number to clear.
     * @param containerID The container ID that has item that needs to be cleared.
     *
     * @return False if there is a problem sending message.
     *
    bool SendClearUpdate( unsigned int slotID, unsigned int containerID ); */


    /// Handle production events from super clients
    void HandleProduction(gemActor* actor,const char* type,const char* reward);

protected:
    csPDelArray<NaturalResource> resources;       ///< list of all natural resources in game.
    /** List of all actions usable with natural resources.
     *  @note this must be never sorted in any way as it's cross referenced in resources so the
     *        array position of the string is extremely important to be mantained
     */
    csStringArray resourcesActions;
    MathScript* calc_repair_rank;                 ///< This is the calculation for how much skill is required to repair.
    MathScript* calc_repair_time;                 ///< This is the calculation for how long a repair takes.
    MathScript* calc_repair_result;               ///< This is the calculation for how many points of quality are added in a repair.
    MathScript* calc_repair_quality;              ///< This calculates the item ending quality and max quality at the end of repair.
    MathScript* calc_repair_exp;                  ///< This is the calculation for the experience to assign to player for repairing.
    MathScript* calc_lockpicking_exp;             ///< This is the calculation for the experience to assign to player for lockpicking.
    MathScript* calc_mining_chance;               ///< This is the calculation for chance of successful mining.
    MathScript* calc_mining_exp;                  ///< This is the calculation for the experience to assign to player for mining.
    MathScript* calc_transform_exp;               ///< This is the calculation for the experience to assign to player for trasformations.
    MathScript* calc_transform_practice;          ///< This is the calculation for the practice to assign to player for trasformations.
    MathScript* calc_lockpick_time;               ///< This is the calculation for how long it takes to pick a lock.
    MathScript* calc_transform_apply_skill;       ///< This is the calculation for the quality resulting from the item.
    MathScript* calc_transform_time;              ///< This is the calculation for the time needed to accomplish a transformation.
    MathScript* calc_combine_quality;             ///< This is the calculation for the quality of combined work items.


    void HandleLockPick(MsgEntry* me,Client* client);
    void HandleWorkCommand(MsgEntry* me,Client* client);

    /**
     * Stop auto work event.
     *
     * This is called when a client removes an item from any container
     * before it has had a chance to transform the item.
     *
     * @param client The client that removed the item.
     * @param autoItem The item that was being transformed.
     */
    void StopAutoWork(Client* client, psItem* autoItem);

    /**
     * Handles stopping the use of the item for working.  This is called when an item
     * is tagetted and the /use command issued and it's already in use.
     *
     * @param me                The message entry.
     * @param client            The client that issues the /use command.
     */
    void StopUseWork(MsgEntry* me,Client* client);

    /**
     * Checks to see if the item can be used for working.
     *
     * @param client  The client that issues the /use command
     */
    void StartUseWork(Client* client);

    /**
     * Handles stopping the combining in the work container.  This is called when an item
     * is tagetted and the /combine command issued and it's already in use.
     *
     * @param client  The client that issues the /combine command
     */
    void StopCombineWork(Client* client);

    /**
     * Checks to see if the work container item can be used for combining.
     *
     * @param client  The client that issues the /combine command
     */
    void StartCombineWork(Client* client);

    /**
     * Checks to see if the item can be constructed
     *
     * @param client  The client that issues the /construct command
     */
    void StartConstructWork(Client* client);

    /**
     * Handles stopping the constructing of an item.  This is called when an item
     * is tagetted and the /construct command issued and it's already in use.
     *
     * @param client  The client that issues the /construct command
     */
    void StopConstructWork(Client* client);

    /**
     * Handles stopping the cleanup event for a particular item.  This is called when an item
     * is removed from a container.
     *
     * @param client  The client that removes the item
     * @param cleanItem  The item that is removed
     */
    void StopCleanupWork(Client* client, psItem* cleanItem);

    /**
     * Sends an error message to the client based on the trade pattern error.
     *
     * @see TradePatternMatch for the list of error conditions.
     *
     * @param clientNum         the client to send the message to.
     * @param result            The error code from the transformable test.
     * @param curItemId         The current item ID.
     * @param CurItemQty        The current item quantity.
     */
    void SendTransformError(uint32_t clientNum, unsigned int result, uint32 curItemId = 0, int CurItemQty = 0);

    /**
     * Returns with the result ID and quantity of the combination
     * if work item container has the correct items in the correct amounts.
     *
     * \note  This assumes that the combination items array is sorted by
     *        resultId and then itemId
     *
     * @param resultId          The item ID of the resulting item.
     * @param resultQty         The stack quantity of the resulting item.
     *
     * @return False if combination is not possible.
     */
    bool IsContainerCombinable(uint32 &resultId, int &resultQty);

    /**
     * Returns with the result ID and quantity of the combination
     * if player has the correct items in the correct amounts in hand
     *
     * \note This assumes that the combination items array is sorted by
     *       resultId and then itemId.
     *
     * @param resultId          The item ID of the resulting item.
     * @param resultQty         The stack quantity of the resulting item.
     *
     * @return False if combination is not possible.
     */
    bool IsHandCombinable(uint32 &resultId, int &resultQty);

    /**
     * Returns with the result ID and quantity of the combination
     * if the item list matches every item in a valid combination
     *
     * @param itemArray         The array of items that need to be checked.
     * @param resultId          The item ID of the resulting item.
     * @param resultQty         The stack quantity of the resulting item.
     *
     * @return False if combination is not possible.
     */
    bool ValidateCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty);

    /**
     * Returns with the result ID and quantity of the combination
     * if the item list is in set of unique ingredients for that pattern.
     *
     * @param itemArray The array of items that need to be checked.
     * @param resultId The item ID of the resulting item.
     * @param resultQty The stack quantity of the resulting item.
     *
     * @return False if combination is not possible.
     */
    bool AnyCombination(csArray<psItem*> itemArray, uint32 &resultId, int &resultQty);

    /**
     * Returns true then item array matchs combination array regardless of order.
     *
     * @param itemArray The array of items that need to be checked.
     * @param current The combination structure that includes an array of items
     *
     * @return False if combination is not possible.
     */
    bool MatchCombinations(csArray<psItem*> itemArray, CombinationConstruction* current);

    /**
     * Check to see if there is a possible trasnform available.
     *
     * @param patterns  The list of all applicable patters to apply (including group patterns)
     * @param KFactor   The KFactor
     * @param targetId  The id of the item trying to transform.
     * @param targetQty The stack count of the item to transform.
     *
     * @return An indicator of pattern match status.
     */
    unsigned int AnyTransform(csArray<psTradePatterns*> &patterns, float &KFactor, uint32 targetId, int targetQty);

    /**
     * Check to see if there is a possible trasnform available.
     *
     * @param patternId The current pattern to use
     * @param targetId the id of the item trying to transform.
     * @param targetQty The stack count of the item to transform.
     *
     * @return An indicator of pattern match status.
     */
    unsigned int IsTransformable(uint32 patternId, uint32 targetId, int targetQty);

    bool ScriptNoTarget();
    bool ScriptActor(gemActor* gemAct);
    bool ScriptItem(gemItem* gemItm);
    bool ScriptAction(gemActionLocation* gemAction);

    bool CombineWork();
    bool IsIngredient(uint32 patternId, uint32 targetId);

    /**
     * @param oldItem           The item which is going to be destructed (the old item).
     * @param newId             The id of the new.
     * @param newQty            The new number of items.
     * @param itemQuality       The new items quality.
     * @param process           The process which was applied in order to generate this item, if any, else NULL.
     * @param trans             The transformation which was applied in order to generate this item, if any, else NULL.
     */
    psItem* TransformSelfContainerItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans);

    /**
     * @param oldItem           The item which is going to be destructed (the old item).
     * @param newId             The id of the new.
     * @param newQty            The new number of items.
     * @param itemQuality       The new items quality.
     * @param process           The process which was applied in order to generate this item, if any, else NULL.
     * @param trans             The transformation which was applied in order to generate this item, if any, else NULL.
     */
    psItem* TransformContainedItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans);

    /**
     *
     * @param newId             The new ID
     * @param newQty            The new number of items.
     * @param itemQuality       The new items quality.
     * @param containerItem     The container.
     */
    psItem* CombineContainedItem(uint32 newId, int newQty, float itemQuality, psItem* containerItem);

    /**
     * @param slot              The slot
     * @param newId             The new ID
     * @param newQty            The new number of items.
     * @param itemQuality       The new items quality.
     * @param process           The process which was applied in order to generate this item, if any, else NULL.
     * @param trans             The transformation which was applied in order to generate this item, if any, else NULL.
     */
    psItem* TransformSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans);

    /**
     * @param slot              The slot
     * @param newId             The new ID
     * @param newQty            The new number of items.
     * @param itemQuality       The new items quality.
     * @param process           The process which was applied in order to generate this item, if any, else NULL.
     * @param trans             The transformation which was applied in order to generate this item, if any, else NULL.
     */
    psItem* TransformTargetSlotItem(INVENTORY_SLOT_NUMBER slot, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans);

    /**
     * @param oldItem           The item which is going to be destructed (the old item).
     * @param newId             The id of the new.
     * @param newQty            The new number of items.
     * @param itemQuality       The new items quality.
     * @param process           The process which was applied in order to generate this item, if any, else NULL.
     * @param trans             The transformation which was applied in order to generate this item, if any, else NULL.
     */
    psItem* TransformTargetItem(psItem* oldItem, uint32 newId, int newQty, float itemQuality, psTradeProcesses* process, psTradeTransformations* trans);

    void TransformTargetItemToNpc(psItem* workItem, Client* client);

    /**
     * Applies the mathscript defined for the process over items being produced.
     *
     * It checks if there is a mathscript defined (empty string to disable),
     * if it's defined it will try to recover the mathscript if available,
     * if it succeeds it sets NewItem OldItem Worker and Process in the math environment
     * and runs the script. The function has no return as the mathscript is supposed to apply the effects
     * directly on the targets passed in the environmnent.
     *
     * @param oldItem The item which is going to be destructed (the old item).
     * @param newItem The newly created item.
     * @param worker  The actor which is working on the item.
     * @param process The process applied to create the new item by the worker.
     * @param trans   The transform applied to create the new item by the worker.
     */
    void ApplyProcessScript(psItem* oldItem, psItem* newItem, gemActor* worker, psTradeProcesses* process, psTradeTransformations* trans);
//    bool TransformHandItem(uint32 newId, int newQty, float itemQuality);
    //bool SendItemUpdate( INVENTORY_SLOT_NUMBER slotID, psItem *newItem );

    void StartTransformationEvent(int transType, INVENTORY_SLOT_NUMBER transSlot, int resultQty,
                                  float resultQuality, psItem* autoItem);
    void StartCleanupEvent(int transType, Client* client, psItem* item, gemActor* worker);

    bool ValidateTarget(Client* client);
    bool ValidateWork();
    bool ValidateMind();
    bool ValidateStamina(Client* client);
    bool IsOnHand(uint32 equipId);
    psItem* CreateTradeItem(uint32 newId, int newQty, float itemQuality, bool transient = false);
    bool ValidateTraining(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    bool ValidateSkills(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    bool ValidateNotOverSkilled(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);
    bool ValidateConstraints(psTradeTransformations* transCandidate, psTradeProcesses* processCandidate);

    /**
     * Calculates the event duration to accomplish a work using a math script.
     *
     * @param trans             The transformation which is being applied.
     * @param process           The process.
     * @param transItem         The item which is being transformed.
     * @param worker            The actor which is working on the object.
     * @return The amount of time which will be needed to complete the work.
     */
    int CalculateEventDuration(psTradeTransformations* trans, psTradeProcesses* process, psItem* transItem, gemActor* worker);

    bool CheckStamina(psCharacter* owner) const;

    void Initialize();

    /**
      * Calculates the quality of the item based on the skills applied
      */
    bool CalculateQuality(float factor, psItem* transItem, gemActor* worker, bool amountModifier, float &currentQuality, psTradeProcesses* process, psTradeTransformations* trans, csTicks time);

    /**
     * This function handles commands like "/repair" using
     * the following sequence of steps.
     *
     * -# Make sure client isn't already busy digging, etc.
     * -# Check for repairable item in right hand slot
     * -# Check for required repair kit item in any inventory slot
     * -# Calculate time required for repair based on item and skill level
     * -# Calculate result after repair
     * -# Queue time event to trigger when repair is complete, if not canceled.
     * @param actor The actor that issues the command
     * @param client The client that issues the command, NULL if NPCClient
     * @param repairSlotName the slot name which should be repaired.
     */
    void HandleRepair(gemActor* actor, Client* client, const csString &repairSlotName);

    /**
     * Handle production events from clients.
     *
     * This function handles commands like "/dig for gold" using
     * the following sequence of steps:
     *
     * -# Make sure client isn't already busy digging, etc.
     * -# Find closest natural resource
     * -# Validate category of equipped item
     * -# Calculate time required
     * -# Send anim and confirmation message to client
     * -# Queue up game event for success
     *
     * @param actor             The actor
     * @param type              The position in the resourcesActions array of the requested production type.
     * @param reward            The name of the natural resource we are looking for.
     * @param client            The client that issues the command
     */
    void HandleProduction(gemActor* actor, size_t type,const char* reward, Client* client = NULL);

    bool SameProductionPosition(gemActor* actor, const csVector3 &startPos);

    /**
     * Find the nearest resource to the player of the requested type.
     *
     * @param sector            A pointer to the iSector the player is currently in.
     * @param pos               A csVector3 with the position of the player in the current sector.
     * @param action            The position in the resourcesActions array of the requested production type.
     * @param reward            The name of the natural resource we are looking for.
     */
    csArray<NearNaturalResource> FindNearestResource(iSector* sector, csVector3 &pos, const size_t action, const char* reward = NULL);

private:

    csWeakRef<gemActor> worker;     ///< Current worker that the work manager is dealing with.

    uint32_t clientNum;             ///< Current client the work manager is dealing with.
    psItem* workItem;               ///< The current work item that is in use ( example, ore furnace )
    psItem* autoItem;               ///< The current item that is being transformed by auto-transformation container
    gemActor* owner;                ///< The character pointer of the current character being used.
    gemObject* gemTarget;           ///< The object being targeted by the player.
    csArray<psTradePatterns*> patterns; ///< Current pattern ID
    uint32 groupPatternId;          ///< Current group pattern ID
    float patternKFactor;           ///< Pattern factor that's part of quality calculation
    float currentQuality;           ///< Current result item quality
    psTradeTransformations* trans;  ///< Current work transformation
    psTradeProcesses* process;      ///< Current work process
    const char* preworkModeString;  ///< Mode string prior to work
    bool secure;                    ///< Client reached required security level

    CacheManager* cacheManager;
    EntityManager* entityManager;
};



//-----------------------------------------------------------------------------

/// work event class
class psWorkGameEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    psWorkGameEvent(WorkManager* mgr,
                    gemActor* worker,
                    int delayticks,
                    int cat,
                    csVector3 &pos,
                    csArray<NearNaturalResource>* natres=NULL,
                    Client* c=NULL,
                    psItem* object=NULL,
                    float repairAmount=0.0F);
    virtual ~psWorkGameEvent();

    void Interrupt();

    virtual void Trigger();  ///< Abstract event processing function

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    /// Set the active trade transformation for the event.
    void SetTransformation(psTradeTransformations* t)
    {
        transformation = t;
    }

    /// Return the active transformation, if any for this event.
    psTradeTransformations* GetTransformation()
    {
        return transformation;
    }

    /// Set the active trade process for the event.
    void SetProcess(psTradeProcesses* p)
    {
        process = p;
    }

    /// Return the active process, if any for this event.
    psTradeProcesses* GetProcess()
    {
        return process;
    }

    /// result quantity is only used when transaction result is zero
    int GetResultQuantity()
    {
        return resultQuantity;
    }
    void SetResultQuantity(int newQuantity)
    {
        resultQuantity = newQuantity;
    }

    /// result quality is calculated immediately before the event
    float GetResultQuality()
    {
        return resultQuality;
    }
    void SetResultQuality(float newQuality)
    {
        resultQuality = newQuality;
    }

    /// pattern Kfactor is based on the current pattern
    float GetKFactor()
    {
        return KFactor;
    }
    void SetKFactor(float newFactor)
    {
        KFactor = newFactor;
    }

    /// slot to perform the transformation
    INVENTORY_SLOT_NUMBER GetTransformationSlot()
    {
        return transSlot;
    }
    void SetTransformationSlot(INVENTORY_SLOT_NUMBER curSlot)
    {
        transSlot = curSlot;
    }

    psItem* GetTranformationItem()
    {
        return item;
    }
    void SetTransformationItem(psItem* i)
    {
        item = i;
    }

    psItem* GetWorkItem()
    {
        return workItem;
    }
    void SetWorkItem(psItem* w)
    {
        workItem = w;
    }

    gemObject* GetTargetGem()
    {
        return gemTarget;
    }
    void SetTargetGem(gemObject* g)
    {
        gemTarget = g;
    }

    /// transformation type
    int GetTransformationType()
    {
        return transType;
    }
    void SetTransformationType(int t)
    {
        transType = t;
    }

    WorkManager* workmanager;
    csWeakRef<gemActor> worker;
    csArray<NearNaturalResource> nrr;
    Client* client;
    gemObject* gemTarget;
    int category;
    csVector3 position;
    psItem*    object;
    psTradeTransformations* transformation;
    psTradeProcesses* process;
    float repairAmount;

    uint32_t effectID;      ///< The id of the psEffect tied to event.
    csArray<PublishDestination> multi;

private:
    int resultQuantity;
    float resultQuality;
    float KFactor;
    INVENTORY_SLOT_NUMBER transSlot;
    psItem* item;
    psItem* workItem;
    int transType;
};

#endif
