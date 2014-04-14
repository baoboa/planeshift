/*
* tribe.h
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __TRIBE_H__
#define __TRIBE_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csutil/list.h>
#include <csutil/priorityqueue.h>
#include <csutil/weakref.h>
#include <csutil/parray.h>

#include <csgeom/vector3.h>
#include <iengine/sector.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <util/psconst.h>
#include <util/psutil.h>
#include <util/remotedebug.h>

//=============================================================================
// Local Includes
//=============================================================================

/**
 * \addtogroup npcclient
 * @{ */

class iResultRow;
class EventManager;
class NPC;
class gemNPCItem;
class Perception;
class gemNPCActor;
class Recipe;
class RecipeManager;
class RecipeTreeNode;
class EventManager;

#define TRIBE_UNLIMITED_SIZE   100

/**
 * Class used to define a Tribal Object
 */
class Tribe : public ScopedTimerCB, public RemoteDebug
{
public:
    typedef csHash<csString,csString> BufferHash;

    typedef enum
    {
        ASSET_TYPE_ITEM,
        ASSET_TYPE_BUILDING,
        ASSET_TYPE_BUILDINGSPOT
    } AssetType;
    static const char* AssetTypeStr[];

    typedef enum
    {
        ASSET_STATUS_NOT_APPLICABLE,
        ASSET_STATUS_NOT_USED,
        ASSET_STATUS_INCONSTRUCTION,
        ASSET_STATUS_CONSTRUCTED
    } AssetStatus;
    static const char* AssetStatusStr[];

    struct Resource
    {
        int      id;                     ///< Database id
        csString name;
        csString nick;
        int      amount;
    };

    struct Asset
    {
        int                   id;
        AssetType             type;      ///< Type of this asset.
        csString              name;      ///< Name. Especially used for buildings
        uint32_t              itemUID;   ///< The UID of the item, 0 if no item
        csWeakRef<gemNPCItem> item;      ///< Item representing the asset
        int                   quantity;  ///< Quantity of items of this type
        csVector3             pos;       ///< Position // Used only for reservations
        csString              sectorName;///< Name of the sector.
        iSector*              sector;    ///< The Sector
        AssetStatus           status;    ///< Status of this asset. Used for buildings.

        /**
         * Save the asset.
         */
        void        Save();

        /**
         * Get the sector of the Asset.
         */
        iSector*    GetSector();

        /**
         * Get the Item of the Asset.
         */
        gemNPCItem* GetItem();
    };

    struct Memory
    {
        int       id;          ///< Database id
        csString  name;
        csVector3 pos;
        iSector*  sector;
        csString  sectorName;  ///< Keep the sector name until sector is loaded
        float     radius;
        NPC*      npc;         ///< Privat memory if NPC is set

        iSector* GetSector();
    };

    struct MemberID
    {
        PID       pid;
        csString  tribeMemberType; ///< Used to select needSet by index.
    };

    struct CyclicRecipe
    {
        Recipe* recipe;     ///< Link to recipe
        int     timeTotal;  ///< Total number of ticks to cycle
        int     timeLeft;   ///< Number of ticks left before re-execution
    };

    /**
     * Construct a new tribe object.
     */
    Tribe(EventManager* eventmngr, RecipeManager* rm);

    /**
     * Destruct a tribe object.
     */
    virtual ~Tribe();

    /**
     * Load the tribe object.
     */
    bool Load(iResultRow &row);

    /**
     * Load and add a new member to the tribe.
     */
    bool LoadMember(iResultRow &row);

    /**
     * Load and add a new resource to the tribe.
     */
    bool LoadResource(iResultRow &row);

    /**
     * Adds a new member to the tribe.
     */
    bool AddMember(PID pid, const char* tribeMemberType);

    /**
     * Save or update an resource in database.
     */
    void SaveResource(Resource* resource, bool newResource);

    /**
     * Attach a new member to the tribe if the NPC is a member
     */
    bool CheckAttach(NPC* npc);

    /**
     * Attach a new member to the tribe.
     */
    bool AttachMember(NPC* npc, const char* tribeMemberType);

    /**
     * Remove members that die.
     */
    bool HandleDeath(NPC* npc);

    /**
     * Count number of alive members.
     */
    int AliveCount() const;

    /**
     * Handled a perception given to this tribe.
     */
    void HandlePerception(NPC* npc, Perception* perception);

    /**
     * Add a new resource to the tribe resource table.
     */
    void AddResource(csString resource, int amount, csString nick = "");

    /**
     * Return the amount of a given resource
     */
    int CountResource(csString resource) const;

    /**
     * Advance the tribe.
     */
    void Advance(csTicks when,EventManager* eventmgr);

    int GetID()
    {
        return id;
    }
    const char* GetName()
    {
        return name.GetDataSafe();
    }
    size_t GetMemberIDCount()
    {
        return membersId.GetSize();
    }
    size_t GetMemberCount()
    {
        return members.GetSize();
    }
    NPC* GetMember(size_t i)
    {
        return members[i];
    }
    size_t GetResourceCount()
    {
        return resources.GetSize();
    }
    const Resource &GetResource(size_t n)
    {
        return resources[n];
    }
    csList<Memory*>::Iterator GetMemoryIterator()
    {
        csList<Memory*>::Iterator it(memories);
        return it;
    };
    csString GetNPCIdleBehavior()
    {
        return npcIdleBehavior;
    }
    csVector3 GetHomePosition()
    {
        return homePos;
    }
    iSector* GetHomeSector();
    csString GetHomeSectorName()
    {
        return homeSectorName;
    }

    /**
     * Calculate the maximum number of members for the tribe.
     */
    int GetMaxSize() const;


    /**
     * Return the reproduction cost for this tribe.
     */
    int GetReproductionCost() const;

    /**
     * Get home position for the tribe.
     */
    void GetHome(csVector3 &pos, float &radius, iSector* &sector);

    /**
     * Set home position for the tribe.
     */
    void SetHome(const csVector3 &pos, float radius, iSector* sector);

    /**
     * Check if the position is within the bounds of the tribe home
     *
     * @param npc    The npc responsible for this checking
     * @param pos    The position to check
     * @param sector The sector to check
     * @return True if position is within bounds of the tribe home
     */
    bool CheckWithinBoundsTribeHome(NPC* npc, const csVector3 &pos, const iSector* sector);

    /**
     * Get a memorized location for resources
     */
    bool GetResource(NPC* npc, csVector3 startPos, iSector* startSector,
                     csVector3 &pos, iSector* &sector, float range, bool random);

    /**
     * Get the closest Memory regarding a resource or the closest
     * non-prospected mine.
     */
    Tribe::Memory* GetResource(csString resourceName, NPC* npc);

    /**
     * Get the most needed resource for this tribe.
     */
    const char* GetNeededResource();

    /**
     * Get the nick for the most needed resource for this tribe.
     */
    const char* GetNeededResourceNick();

    /**
     * Get a area for the most needed resource for this tribe.
     */
    const char* GetNeededResourceAreaType();

    /**
     * Get wealth resource growth rate
     *
     * Get the rate that the wealth will grow when all members are dead.
     *
     * @return Return the wealth growth rate
     */
    float GetWealthResourceGrowth() const;

    /**
     * Get wealth resource growth rate active
     *
     * Get the rate that the wealth will grow when there are alive members.
     *
     * @return Return the wealth growth rate
     */
    float GetWealthResourceGrowthActive() const;

    /**
     * Get wealth resource growth active limit
     *
     * Get the limit that the wealth will be capped at when there are alive members.
     *
     * @return Return the wealth growth limit
     */
    int GetWealthResourceGrowthActiveLimit() const;


    /**
     * Check if the tribe can grow by checking the tribes wealth
     */
    bool CanGrow() const;

    /**
     * Check if the tribe should grow by checking number of members
     * against max size.
     */
    bool ShouldGrow() const;

    /**
     * Memorize a perception. The perception will be marked as
     * personal until NPC return home. Personal perceptions
     * will be deleted if NPC die.
     */
    void Memorize(NPC* npc, Perception* perception);

    /**
     * Find a privat memory
     */
    Memory* FindPrivMemory(csString name,const csVector3 &pos, iSector* sector, float radius, NPC* npc);

    /**
     * Find a memory
     */
    Memory* FindMemory(csString name,const csVector3 &pos, iSector* sector, float radius);

    /**
     * Find a memory
     */
    Memory* FindMemory(csString name);

    /**
     * Add a new memory to the tribe
     */
    void AddMemory(csString name,const csVector3 &pos, iSector* sector, float radius, NPC* npc);

    /**
     * Share privat memories with the other npcs. Should be called when npc return to home.
     */
    void ShareMemories(NPC* npc);

    /**
     * Save a memory to the db
     */
    void SaveMemory(Memory* memory);

    /**
     * Load all stored memories from db.
     */
    bool LoadMemory(iResultRow &row);

    /**
     * Forget privat memories. Should be called when npc die.
     */
    void ForgetMemories(NPC* npc);

    /**
     * Find nearest memory to a position.
     */
    Memory* FindNearestMemory(const char* name,const csVector3 &pos, const iSector* sector, float range = -1.0, float* foundRange = NULL);

    /**
     * Find a random memory within range to a position.
     */
    Memory* FindRandomMemory(const char* name,const csVector3 &pos, const iSector* sector, float range = -1.0, float* foundRange = NULL);

    /**
     * Send a perception to all members of the tribe
     *
     * This will send the perception to all the members of the tribe.
     * If the maxRange has been set to something greater than 0.0
     * a range check will be applied. Only members within the range
     * from the base position and sector will be triggered.
     *
     * @param pcpt       Perception to be sent.
     * @param maxRange   If greater then 0.0 then max range apply
     * @param basePos    The base position for range checks.
     * @param baseSector The base sector for range checks.
     */
    void TriggerEvent(Perception* pcpt, float maxRange=-1.0,
                      csVector3* basePos=NULL, iSector* baseSector=NULL);

    /**
     * Sends the given perception to the given list of npcs
     *
     * Used by the recipe manager to send perceptions to tribe members
     * selected by the active recipe.
     *
     * @param pcpt    The perception name to be sent
     * @param npcs    The list of npcs to send perception to.
     */
    void SendPerception(const char* pcpt, csArray<NPC*> npcs);

    /**
     * Sends the given perception to the tribe.
     *
     * Used to send perceptions to all tribe members.
     *
     * @param pcpt    The perception name to be sent
     */
    void SendPerception(const char* pcpt);

    /**
     * Find the most hated entity for tribe within range
     *
     *  Check the hate list and retrive most hated entity within range
     *  of the given NPC.
     *
     *  @param  npc               The senter of the search.
     *  @param  range             The range to search for hated entities.
     *  @param  includeOutsideRegion  Include outside region entities in the search.
     *  @param  includeInvisible  Include invisible entities in the search.
     *  @param  includeInvincible Include invincible entities in the search.
     *  @param  hate              If diffrent from NULL, set upon return to the hate of the hated.
     *  @return The hated entity
     */
    gemNPCActor* GetMostHated(NPC* npc, float range, bool includeOutsideRegion,
                              bool includeInvisible, bool includeInvincible, float* hate=NULL);

    /**
     * Callback for debug of long time used in scopes.
     */
    virtual void ScopedTimerCallback(const ScopedTimer* timer);


    /**
     * Retrive death rate average value from tribe.
     *
     * @return The Exponential smoothed death rate.
     */
    float GetDeathRate()
    {
        return deathRate;
    }

    /**
     * Retrive resource rate average value from tribe.
     *
     * @return The Exponential smoothed resource rate.
     */
    float GetResourceRate()
    {
        return resourceRate;
    }

    /**
     * Sets the tribe's recipe manager.
     */
    void SetRecipeManager(RecipeManager* rm)
    {
        recipeManager = rm;
    }

    /**
     * Get the main recipe.
     */
    Recipe* GetTribalRecipe();

    /**
     * Updates recipe wait times.
     */
    void UpdateRecipeData(int delta);

    /**
     * Add a recipe.
     *
     * Adds a recipe to activeRecipes array in respect of it's
     * priority and cost.
     *
     * @param recipe       Recipe to add.
     * @param parentRecipe Recipe that requires the new recipe.
     * @param reqType      True for meeting requirements distributed-way
     */
    void AddRecipe(Recipe* recipe, Recipe* parentRecipe, bool reqType = false);

    /**
     * Add a cyclic recipe
     *
     * Cyclic recipes are executed once in a pre-defined quantum of time
     * and are set with the highest priority.
     *
     * @param recipe        Recipe to add
     * @param time          Tribe advances required to execute this cyclic recipe
     */
    void AddCyclicRecipe(Recipe* recipe, int time);

    /**
     * Delete a cyclic recipe
     */
    void DeleteCyclicRecipe(Recipe* recipe);

    /**
     * Add a knowledge token
     */
    void AddKnowledge(csString knowHow)
    {
        knowledge.Push(knowHow);
    }

    /**
     * Check if knowledge is known.
     */
    bool CheckKnowledge(csString knowHow);

    /**
     * Save a knowledge piece in the database.
     */
    void SaveKnowledge(csString knowHow);

    /**
     * Dump knowledge to console.
     */
    void DumpKnowledge();

    /**
     * Dumps all information about recipes to console.
     */
    void DumpRecipesToConsole();

    /**
     * Set npcs memory buffers
     *
     * Loads locations into the Memory buffers of the npcs.
     * Useful when assigning tasks to them.
     *
     * @param name Name of the memory/resource to load.
     * @param npcs Link to the npcs on which to load.
     * @return     False if no desired location exists.
     */
    bool LoadNPCMemoryBuffer(const char* name, csArray<NPC*> npcs);

    /**
     * Set npcs memory buffers - Using a memory.
     */
    void LoadNPCMemoryBuffer(Tribe::Memory* memory, csArray<NPC*> npcs);

    /**
     * Check to see if enough members are idle.
     */
    bool CheckMembers(const csString &type, int number);

    /**
     * Check to see if enough resources are available.
     */
    bool CheckResource(csString resource, int number);

    /**
     * Check to see if enough assets are available.
     */
    bool CheckAsset(Tribe::AssetType type, csString name, int number);

    /**
     * Return the quanitity of the given asset type matching the name.
     */
    size_t AssetQuantity(Tribe::AssetType type, csString name);

    /**
     * Build a building on the current NPC building spot.
     */
    void Build(NPC* npc, bool pickupable);

    /**
     * Tear down a building.
     */
    void Unbuild(NPC* npc, gemNPCItem* building);

    /**
     * Handle persist items that should be assets.
     */
    void HandlePersistItem(gemNPCItem* item);

    /**
     * Returns pointers to required npcs for a task.
     */
    csArray<NPC*> SelectNPCs(const csString &type, const char* number);

    /**
     * Return a named buffer from the NPC.
     *
     * Function used to get data from buffers containing information.
     *
     * @param  bufferName The name of the buffer to retrive.
     * @return Buffer value
     */
    csString GetBuffer(const csString &bufferName);

    /**
     * Set/Update the value of a named buffer.
     *
     * @param bufferName      The buffer name.
     * @param value           The value to put in the buffer.
     */
    void SetBuffer(const csString &bufferName, const csString &value);

    /**
     * Replace $TBUFFER[x] with values from the NPC buffer.
     *
     * @param result String to replace buffers in.
     */
    void ReplaceBuffers(csString &result);

    /**
     * Modify Wait Time for a recipe.
     */
    void ModifyWait(Recipe* recipe, int delta);

    /**
     * Load an asset from an iResultRow.
     */
    void LoadAsset(iResultRow &row);

    /**
     * Save an asset to the db - responsable for deleting assets to.
     */
    void SaveAsset(Tribe::Asset* asset, bool deletion = false);

    /**
     * Add an item asset.
     */
    void AddAsset(Tribe::AssetType type, csString name, gemNPCItem* item, int quantity, int id = -1);

    /**
     * Add an asset to the tribe.
     */
    Tribe::Asset* AddAsset(Tribe::AssetType type, csString name, csVector3 position, iSector* sector, Tribe::AssetStatus status);

    /**
     * Remove an asset.
     */
    void RemoveAsset(Tribe::Asset* asset);

    /**
     * Get asset.
     */
    Asset* GetAsset(Tribe::AssetType type, csString name);

    /**
     * Get asset.
     */
    Asset* GetAsset(Tribe::AssetType type, csString name, Tribe::AssetStatus status);

    /**
     * Get an asset based on name and position.
     */
    Asset* GetAsset(Tribe::AssetType type, csString name, csVector3 where, iSector* sector);

    /**
     * Get asset.
     */
    Asset* GetAsset(gemNPCItem* item);

    /**
     * Get asset.
     */
    Asset* GetAsset(uint32_t itemUID);

    /**
     * Get a random asset.
     */
    Asset* GetRandomAsset(Tribe::AssetType type, Tribe::AssetStatus status, csVector3 pos, iSector* sector, float range);

    /**
     * Get nearest asset.
     */
    Asset* GetNearestAsset(Tribe::AssetType type, Tribe::AssetStatus status, csVector3 pos, iSector* sector, float range, float* locatedRange = NULL);

    /**
     * Get an asset.
     */
    Asset* GetRandomAsset(Tribe::AssetType type, csString name, Tribe::AssetStatus status, csVector3 pos, iSector* sector, float range);

    /**
     * Get an asset.
     */
    Asset* GetNearestAsset(Tribe::AssetType type, csString name, Tribe::AssetStatus status, csVector3 pos, iSector* sector, float range, float* locatedRange = NULL);

    /**
     * Delete item assets.
     */
    void DeleteAsset(csString name, int quantity);

    /**
     * Delete a building asset.
     */
    void DeleteAsset(csString name, csVector3 pos);

    /**
     * Dump Assets
     */
    void DumpAssets();

    /**
     * Dump Buffers.
     */
    void DumpBuffers();

    /**
     * Assigned a resource to a prospect Mine.
     *
     * Upon 'Explore' behavior, tribe members may find locations
     * named 'mine'. After mining from an unkown deposit, the npcclient
     * will receive a psWorkDoneMessage telling it what his npc just
     * mined. This function renames the former 'mine' Memory with it's
     * real deposit name.
     *
     * @param npc      The npc who prospected it to access his memoryBuffer.
     * @param resource The resulted resource received from psServer.
     * @param nick     The nick name of the resource for this mine.
     */
    void ProspectMine(NPC* npc, csString resource, csString nick);

private:
    /**
     * Callback function to report local debug.
     */
    virtual void LocalDebugReport(const csString &debugString);

    /**
     * Callback function to report remote debug.
     */
    virtual void RemoteDebugReport(uint32_t clientNum, const csString &debugString);

    /**
     * Update the deathRate variable.
     */
    void UpdateDeathRate();

    /**
     * Update the resourceRate variable.
     */
    void UpdateResourceRate(int amount);

    int                       id;                               ///< The id of the tribe.
    csString                  name;                             ///< The name of the tribe.
    csArray<MemberID>         membersId;                        ///< List of ID for members.
    csArray<NPC*>             members;                          ///< List of attached NPCs.
    csArray<NPC*>             deadMembers;                      ///< List of dead members.
    csArray<Resource>         resources;                        ///< List of resources.
    csArray<csString>         knowledge;                        ///< Array of knowledge tokens
    csPDelArray<Asset>        assets;                           ///< Array of items / buildings
    csArray<CyclicRecipe>     cyclicRecipes;                    ///< Array of cycle recipes
    RecipeTreeNode*           tribalRecipe;                     ///< The tribal recipe root of the recipe tree

    /** @name Home Position
     * @{ */
    csVector3                 homePos;                          ///< The position
    float                     homeRadius;                       ///< The radius of the home
    csString                  homeSectorName;                   ///< The sector name where home is.
    iSector*                  homeSector;                       ///< The resolved sector name.
    /**  @} */

    int                       maxSize;
    BufferHash                tribeBuffer;                      ///< Buffer used to hold data for recipe's functions
    csString                  wealthResourceName;
    csString                  wealthResourceNick;
    csString                  wealthResourceArea;
    float                     wealthResourceGrowth;
    float                     wealthResourceGrowthActive;
    int                       wealthResourceGrowthActiveLimit;
    float                     accWealthGrowth;                  ///< Accumulated rest of wealth growth.
    int                       reproductionCost;
    csString                  npcIdleBehavior;                  ///< The name of the behavior that indicate that the member is idle
    csString                  wealthGatherNeed;
    csList<Memory*>           memories;

    csTicks                   lastGrowth;
    csTicks                   lastAdvance;


    float                     deathRate;                        ///< The average time in ticks between deaths
    float                     resourceRate;                     ///< The average time in ticks between new resource is found
    csTicks                   lastDeath;                        ///< Time when a member was last killed
    csTicks                   lastResource;                     ///< Time when a resource was last added.

    EventManager*             eventManager;                     ///< Link to the event manager
    RecipeManager*            recipeManager;                    ///< The Recipe Manager handling this tribe
};

/** @} */

#endif
