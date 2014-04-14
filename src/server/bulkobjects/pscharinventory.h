/*
 * pscharinventory.h  by Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PSCHARINV_H__
#define __PSCHARINV_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/sysfunc.h>
#include <csutil/weakref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"
#include "util/psconst.h"

#include "../icachedobject.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psitemstats.h"
#include "psstdint.h"
#include "psitem.h"
#include "psinventorycachesvr.h"
#include "psspell.h"

struct iDocumentNode;

class MsgEntry;
class psItemStats;
class psItem;
class gemContainer;
struct psRaceInfo;
class gemActor;

/**
 * \addtogroup bulkobjects
 * @{ */

/// Set if this slot should continuously attack while in combat
#define PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK       0x00000001
/// Set if this slot can attack when the client specifically requests (and only when the client specifically requests)
#define PSCHARACTER_EQUIPMENTFLAG_SINGLEATTACK     0x00000002
/// Set if this slot can attack even when empty - requires that a default psItem be set in default_if_empty
#define PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY    0x00000004


//-----------------------------------------------------------------------------

/// used by psCharacter::CreateGlyphList()
struct glyphSlotInfo
{
    psItemStats* glyphType;
    int purifyStatus;   ///< possible values are: 0=not purified    1=purifying    2=purified
};


//-----------------------------------------------------------------------------

/** This class handles the details behind a character's inventory system.
  */
class psCharacterInventory
{
public:
    /// Current owner of the inventory
    psCharacter* owner;

    class psCharacterInventoryItem
    {
        friend class psCharacterInventory;
        psItem* item;               ///< must be ptr for polymorphism
    public:
        psCharacterInventoryItem(psItem* it)
        {
            item = it;
            exchangeOfferSlot  = -1;
            exchangeStackCount = 0;
        }
        int    exchangeOfferSlot;   ///< Slot in exchange offer window on client
        int    exchangeStackCount;  ///< count of items being offered, subset of stack in *item
        psItem* GetItem()
        {
            return item;    ///< read-only, cannot clear
        }
    };

protected:

    /// Flat array of all items owned in this inventory
    csArray<psCharacterInventoryItem> inventory;

    /// Array with all the items stored by the character.
    csArray<psItem*> storageInventory;

    struct psEquipInfo
    {
        unsigned int EquipmentFlags;
        int          eventId;
        size_t       itemIndexEquipped;
        psItem*      default_if_empty;
    };

    /// Items player is using in various slots
    psEquipInfo equipment[PSCHARACTER_SLOT_BULK1]; // bulk1 is the first slot above equipment

    /// Do weight/capacity restrictions?
    bool doRestrictions;

    /// Has this inventory been fully loaded yet?
    bool loaded;

    /// Max weight the inventory can hold
    float maxWeight;

    /// Max capacity in inventory
    float maxSize;

    /// Exchange mode flag
    bool inExchangeMode;

    /// Inventory version
    uint32 version;

public:

    psCharacterInventory(psCharacter* owner);
    ~psCharacterInventory();

    /// Load the inventory for the owner.
    bool Load();

    /**
     * Load the inventory using a particular ID.
     *
     * This is useful for NPCs that have their own
     * inventory as well as a common base one they need to load.
     *
     * @param id The key into the character table for the character who's inventory we should load.
     * @return true if the inventory was loaded without error.
     */
    bool Load(PID id);

    /**
     * Load the bare minimum to know what this character is looks like.
     */
    bool QuickLoad(PID id);

    /**
     * Check to see if the player has a required amount of carrying capacity.
     *
     * @param desiredSpace The amount of space that we want to check.
     * @return true if the player has enough capacity for the requiredSpace.
     */
    bool HasEnoughUnusedSpace(float desiredSpace);
    int GetCurrentTotalSpace();
    int GetCurrentMaxSpace();

    /**
     * Adds the passed item to the storage.
     *
     * The item will be accessible only from storage npc.
     * @param item The item to be added to the storage.
     */
    void AddStorageItem(psItem* &item);

    /**
     * Check to see if the player has the ability to carry and additional weight.
     *
     * @param requiredWeight The amount of weight that we want to check.
     * @return true if the player has can carry the additional weight.
     */
    bool HasEnoughUnusedWeight(float requiredWeight);
    float GetCurrentTotalWeight();

    /**
     * How may items can fit.
     *
     * @param item the item to check for.
     * @return the max number of the given item that can fit.
     */
    size_t HowManyCanFit(psItem* item);

    /// @return the number of empty bulk slots
    ////  int HasEmptyBulkSlots();

    /**
     * @return The max allowable weight for this inventory.
     */
    float MaxWeight()
    {
        return maxWeight;
    }

    /// @return The max capacity ( ie space ) this inventory can handle.
    /////float MaxCapacity() { return maxSize; }

    /// @return The current weight level of the inventory.
    // float Weight() { return 0; }

    /**
     * @return The current capacity ( ie space ) of the inventory.
     */
    float GetTotalSizeOfItemsInContainer(psItem* container=NULL);

    /**
     * @return The current inventory version.
     */
    uint32 GetInventoryVersion() const
    {
        return version;
    }

    /**
     * Updates (increases) the current inventory version.
     */
    void IncreaseInventoryVersion()
    {
        version++;
    }

    /**
     * Put an item into the bulk inventory, and stack if needed.
     *
     * @param item The item we want to place into the slot.
     * @param test Are we just testing if we can put this into the slot
     * @param stack Should the item be stacked?
     * @param slot The slot in which we want to place an item.
     *             (may be a slot number, or PSCHARACTER_SLOT_NONE)
     * @param container Pointer to container.
     * @param precise Says whathever if the stacking should be precise and not ignore
     *                properties like quality.
     * @return true  if the item could be placed fully in the slot.
     *         false if the item could not be placed.
     */
    bool Add(psItem* &item, bool test = false, bool stack = true,
             INVENTORY_SLOT_NUMBER slot = PSCHARACTER_SLOT_NONE, gemContainer* container = NULL, bool precise = true);

    /**
     * Attempt to stack an item on an existing one if Add failed.
     *
     * @param item The item we want to place into the slot.
     * @param added number of items that have been taken off the stack
     * @return The item if the item could be placed partially in the slot. else NULL.
     */
    psItem* AddStacked(psItem* &item, int &added);

    /**
     * Add an item to the inventory, or drop if inventory is full.
     */
    bool AddOrDrop(psItem* &item, bool stack = true);

    /**
     * Get an item that is in the bulk inventory.
     *
     * @param container The container of the slot, if any. NULL if normal bulk.
     * @param slot The slot in which we want to retrive the item ( if any ).
     * @return The item in the given slot. NULL if no item was found.
     */
    psItem* GetItem(psItem* container, INVENTORY_SLOT_NUMBER slot);

    /**
     * Check if the item of a certain name is in the character inventory.
     *
     * @param itemname The name of the item to search for.
     * @param includeEquipment include equipment in the search.
     * @param includeBulk include bulk in the search.
     * @param qualityMin The minimum quality the item must have, 0 to ignore.
     * @param qualityMax The maximum quality the item must have, 0 to ignore.
     * @return The item in the given slot. NULL if no item was found.
     */
    bool hasItemName(csString &itemname, bool includeEquipment, bool includeBulk, float qualityMin = 0, float qualityMax = 0);

    /**
     * Get an item that is in the equipment inventory.
     *
     * @param slot The slot in which we want to retrive the item ( if any ).
     * @return The item in the given slot. NULL if no item was found.
     */
    psItem* GetInventoryItem(INVENTORY_SLOT_NUMBER slot);

    /**
     * Restore all items in this inventory to their max quality.
     *
     * This function is usually used by npc at their respawn after death.
     */
    void RestoreAllInventoryQuality();

    /**
     * Get item held, either in right or left hand.
     */
    psItem* GetItemHeld();

    psCharacterInventoryItem* GetCharInventoryItem(INVENTORY_SLOT_NUMBER slot);

    /**
     * Get the equip slot of the specified item, if equipped.
     *
     * @param item The item that we want to find.
     * @return The index of the slot where the item is equipped. -1 if no item was found.
     */
    INVENTORY_SLOT_NUMBER FindSlotHoldingItem(psItem* item);

    /**
     * Return the direct index into inventory of the defined item.
     */
    size_t FindItemStatIndex(psItemStats* itemstats,size_t startAt=1);

    /**
     * Remove an item from the bulk slots.
     *
     * @param container
     * @param bulkslot The slot we want to remove the item from.
     * @param count The number we want to take. -1 Means the entire stack.
     * @return pointer to the item that was removed.
     */
    psItem* RemoveItem(psItem* container, INVENTORY_SLOT_NUMBER bulkslot, int count = -1);

    /**
     * Remove an item from the bulk slots.
     *
     * @param itemID The UID of the item we are looking for.
     * @param count The number we want to take. -1 Means the entire stack.
     * @param storage TRUE if we are looking in the storage.
     * @return pointer to the item that was removed.
     */
    psItem* RemoveItemID(uint32 itemID,int count = -1, bool storage = false);

    /**
     * Removes an item that is in the equipment inventory.
     *
     * @param slot The slot we want to remove the item from.
     * @param count The number we want to take. -1 Means the entire stack.
     * @return pointer to the item that was removed.
     */
    psItem* RemoveInventoryItem(INVENTORY_SLOT_NUMBER slot, int count = -1);


    /**
     * Find the total stack count in inventory for a particular type of item.
     *
     * @param item The base stats of the item we want to count.
     * @return the count of the total items matching the stats description.
     */
    unsigned int TotalStackOfItem(psItemStats* item);

    /**
     * Finds an item inside the bulk inventory or the storage.
     *
     * @param itemID The UID of the item we are looking for.
     * @param storage TRUE if we have to look in the item storage.
     * @return The item if found, NULL otherwise.
     */
    psItem* FindItemID(uint32 itemID, bool storage = false);

    void SetExchangeOfferSlot(psItem* Container,INVENTORY_SLOT_NUMBER slot,int toSlot,int stackCount);
    int  GetOfferedStackCount(psItem* item);
    psCharacterInventoryItem* FindExchangeSlotOffered(int slotID);
    void PurgeOffered();

    /**
     * Checks to see if anything in inventoy counts as the key for a lock.
     *
     * @param lock The lock we want to try and open.
     * @return true if a key was found that can open the lock.
     */
    bool HaveKeyForLock(uint32 lock);

    /**
     * Check to see if a particular slot can be attacked.
     *
     * @param slot The slot to query.
     * @return true if the slot can be attacked.
     */
    bool CanItemAttack(INVENTORY_SLOT_NUMBER slot);

    /**
     * Query if a slot is auto attack.
     *
     * @param slot The slot to query.
     * @return true if the slot can auto attack.
     */
    bool IsItemAutoAttack(INVENTORY_SLOT_NUMBER slot);

    /**
     * Acquire a reference to the equipment object inside inventory.
     *
     * This function does NOT do bounds checking so idx has to be in range.
     * @param idx The equipment slot we want the equipment structure for.
     * @return The equipment data structure.
     */
    psEquipInfo &GetEquipmentObject(INVENTORY_SLOT_NUMBER idx);

    /**
     * Checks the equipment slot to see if it is a weapon.
     *
     * An item is considered a weapon if: IsMeleeWeapon() || GetIsRangeWeapon()
     * @param slot The equipment slot to check weapon status.
     * @param includeShield Tell if a shield can be returned as a weapon, useful when calculating
     *                      the shield blocking like a weapon.
     * @return a pointer to the item if it is a weapon. If no weapon found will use default fists.
     */
    psItem* GetEffectiveWeaponInSlot(INVENTORY_SLOT_NUMBER slot, bool includeShield = false);

    /**
     * Gets the item in the slot.
     *
     * @param slot The equipment slot to checks.
     * @return a pointer to the item if one is there. Uses base_cloths otherwise.
     */
    psItem* GetEffectiveArmorInSlot(INVENTORY_SLOT_NUMBER slot);

    /**
     * Check if the item of a certain category is in the character inventory.
     *
     * @param category  pointer to the psItemCategory structure to search for.
     * @param includeEquipment include equipment in the search.
     * @param includeBulk include bulk in the search.
     * @param includeStorage include storage in the search.
     * @return true if the item was found, false if it wasn't.
     */
    bool hasItemCategory(psItemCategory* category, bool includeEquipment, bool includeBulk, bool includeStorage = false);

    /**
     * Check if the item of a certain category is in the character inventory.
     *
     * @param categoryname The name of the category of item to search for.
     * @param includeEquipment include equipment in the search.
     * @param includeBulk searches for the item only in the equipment if true.
     * @param includeStorage include storage in the search.
     * @param qualityMin The minimum quality the item must have, 0 to ignore.
     * @param qualityMax The maximum quality the item must have, 0 to ignore.
     * @return true if the item was found, false if it wasn't.
     */
    bool hasItemCategory(csString &categoryname, bool includeEquipment, bool includeBulk, bool includeStorage = false, float qualityMin = 0, float qualityMax = 0);

    /**
     * Iterates over the entire inventory to get a list of all the items from a particular category.
     *
     * This is useful for npc merchants/storage when you want to get a list of all the items they have.
     * @param category A pointer to the category that we want to match against.
     * @param storage A boolean telling if we have to look in the storage in place of the main inventory.
     * @return The list of all items in category.
     */
    csArray<psItem*> GetItemsInCategory(psItemCategory* category, bool storage = false);

    /**
     * Iterates over the inventory, stacking items.
     *
     * @param itemname Name of the item to be stacked
     * @param count quantity up to which items should be stacked
     * @param container decide whether to look into containers in the inventory
     * @return The pointer to the stacked item, or NULL if nothing is found
     */
    psItem* StackNumberItems(const csString &itemname, int count, bool container);


    size_t GetContainedItemCount(psItem* container);
    float GetContainedWeight(psItem* container);
    float GetContainedSize(psItem* container);

    /// Uses Mathscript formulas to determine the proper max weight and max space limits
    void CalculateLimits();

    /// Iterates over the equipped items running any scripts on them.
    void RunEquipScripts();

    /// Contain error messge for last function failing.
    csString lastError;

    /// Changes this inventories handling of weight/capacity restrictions
    void SetDoRestrictions(bool v);

    /// Get the status of the weight/capacity restrictions
    bool GetDoRestrictions()
    {
        return doRestrictions;
    }

    /// Get the inventory cache handler
    psInventoryCacheServer* GetInventoryCacheServer(void)
    {
        return &inventoryCacheServer;
    }

    /// Check if the character has a list of required purified glyphs.
    bool HasPurifiedGlyphs(const glyphList_t glyphsToCheck);

    /// Make an array of types of glyphs in the inventory - prefer purified.
    void CreateGlyphList(csArray <glyphSlotInfo> &slots);

    /**
     * Places loaded items into the correct locations when loading from database.
     *
     * @param parentID The parent.
     * @param slot Name where item should go.
     * @param item The item we are placing.
     * @return Item was placed correctly.
     */
    bool AddLoadedItem(uint32 parentID, INVENTORY_SLOT_NUMBER slot, psItem* item);

    void Equip(psItem* item);
    void Unequip(psItem* item);
    bool CheckSlotRequirements(psItem* item, INVENTORY_SLOT_NUMBER proposedSlot, unsigned short stackCount = 0);

    /**
     * Track what is equipped where here.
     *
     * The item must already be in inventory to equip, and if something is
     * in the equip slot already, it is removed with no effect on anything
     * else.
     */
    bool EquipItem(psItem* newItem, INVENTORY_SLOT_NUMBER slotID);

    /**
     * Return a free equipment slot which the proposed item will fit in.
     */
    INVENTORY_SLOT_NUMBER FindFreeEquipSlot(psItem* itemToPlace);

    /**
     * Return a count of items in the base inventory array.
     *
     * This should be used only for debug output.
     */
    size_t GetInventoryIndexCount()
    {
        return inventory.GetSize();
    }

    /**
     * Return the psItem at the inventory index specified.
     *
     * This should be used only for debug output
     */
    psItem* GetInventoryIndexItem(size_t which)
    {
        return inventory[which].item;
    }

    /// Returns the exchange info and the item in the index specified
    psCharacterInventoryItem* GetIndexCharInventoryItem(size_t index)
    {
        return &inventory[index];
    }

    /// This creates an xml tree for npcloader saving.
    void WriteAllInventory(iDocumentNode* npcRoot);

    /**
     * The one true way to remove an item or split a stack from inventory.
     *
     * Other removal functions all find the index and call this.
     *
     * @param index The index to remove
     * @param count The number of items from that index to remove.
     * @param storage A boolean TRUE if we are removing from the storage in place of the inventory.
     */
    psItem* RemoveItemIndex(size_t index,int count=-1, bool storage = false);

    /// Copies current inventory to backup inventory
    bool BeginExchange();

    /// Make sure inventory knows that an exchange is completed.
    void CommitExchange();

    /// Cancel pending inventory exchange and rollback to original start point of inv.
    void RollbackExchange();

    /// Update encumbrance/OVERWEIGHT mode to match current status.
    void UpdateEncumbrance();

    /**
     * Sets the basic armor when not equipped depending on the passed race.
     *
     * @note if race is NULL or the armor group is 0 it will set the basicloths else the defined ones.
     * @param race Pointer to the race to use to set the basic armor.
     */
    void SetBasicArmor(psRaceInfo* race = NULL);

    /**
     * Sets the basic weapon when not equipped depending on the passed race.
     *
     * @note if race is NULL or the armor group is 0 it will set the fist item else the defined ones.
     * @param race Pointer to the race to use to set the basic armor.
     */
    void SetBasicWeapon(psRaceInfo* race = NULL);

    /**
     * Allocate either a psItem or a psGlyph.
     */
    static psItem* GetItemFactory(psItemStats* stats);

private:
    void WriteItem(csRef<iDocumentNode> equipmentNode, psItem* item, int bulk, INVENTORY_SLOT_NUMBER slot);

    size_t GetItemIndex(psItem* item);

    /**
     * Returns the array index of the item in the specified slot, or
     * SIZET_NOT_FOUND if the slot is open.
     */
    size_t FindSlotIndex(psItem* container,INVENTORY_SLOT_NUMBER slot);

    /**
     * Return the first "slot number" not in use by the specified container.
     */
    int FindFirstOpenSlot(psItem* container);

    /**
     * Returns the array index of the item that matches the specified
     * ones, so they can be combined.
     */
    size_t FindCompatibleStackedItem(psItem* item, bool checkStackCount = true);

    /**
     * Returns an array of array indices for items that match the specified
     * ones, so they can be combined.
     *
     * @param item The item
     * @param checkStackCount Should the stack count be checked.
     * @param precise Says whathever if the stacking should be precise and not ignore
     *                properties like quality.
     */
    csArray<size_t> FindCompatibleStackedItems(psItem* item, bool checkStackCount = true, bool precise = true);

    psInventoryCacheServer inventoryCacheServer;    ///< handle inventory cache

};

class InventoryTransaction
{
public:
    InventoryTransaction(psCharacterInventory* what)
    {
        myInv    = what;
        committed = false;
        myInv->BeginExchange();
    }
    ~InventoryTransaction()
    {
        if(!committed && myInv)
            myInv->RollbackExchange();
    }
    void Commit()
    {
        myInv->CommitExchange();
        committed = true;
    }
    void Cancel()
    {
        myInv = NULL; // character deleted or somesuch
    }

protected:
    psCharacterInventory* myInv;
    bool committed;
};

/** @} */

#endif

