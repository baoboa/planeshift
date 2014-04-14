/*
 * psitem.h
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

#ifndef __PSITEM_H__
#define __PSITEM_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csgeom/vector3.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "util/scriptvar.h"
#include "util/gameevent.h"
#include "util/slots.h"

#include <idal.h>

#include "util/poolallocator.h"

#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"
#include "psitemstats.h"
#include "deleteobjcallback.h"
#include "psactionlocationinfo.h"
#include "lootrandomizer.h"


class Client;
class gemActor;

/**
 * \addtogroup bulkobjects
 * @{ */

// Note: The following 2 options require a clean rebuild when changed.

/// Enable this to be notified of all item saving activity
#define SAVE_DEBUG 0

/// Enable this to keep track of the initial caller of Save() (when used with SAVE_DEBUG, will print full trace)
#define SAVE_TRACER 0


/// These two define how long an item will stay in the world before being auto-removed, in seconds.
#define REMOVAL_INTERVAL_RANGE     300
#define REMOVAL_INTERVAL_MINIMUM  1200

/// This indicates that the item has been deleted from the database and should not be updated
#define  ID_DONT_SAVE_ITEM 0xffffffff

/// The crafter ID field contains the id of the character that made this item
#define PSITEM_FLAG_CRAFTER_ID_IS_VALID 0x00000001
/// The guild ID field contains the id of a guild that certified this item
#define PSITEM_FLAG_GUILD_ID_IS_VALID   0x00000002
/// This item uses its own unique psItemStats data (in the database), and modifications to the data are OK as they will reflect solely on this item
#define PSITEM_FLAG_UNIQUE_ITEM         0x00000004
/** This item is solely based on a basic item template.  It has no modifiers.
* When this flag is set and all effects on this item expire, if the current_stats is not the same as the base_stats pointer,
*  then the object referenced by current_stats is deleted and current_stats is set to point at the base_stats pointer.
*  This allows item stats for base items to use a single area in memory most of the time.  When effects are applied a new copy
*    of the base stats can be made and modified with the effect data.  When all effects fade the new copy can be released back to
*    the pool.
* This flag is not intended to be directly manipulated.  All objects starting from a basic item will have this set to begin with.
*   If a modifier is applied, or if the item is forced to take over unique status, this flag is cleared.
*/
#define PSITEM_FLAG_USES_BASIC_ITEM     0x00000008
/// Flags used for glyphs
#define PSITEM_FLAG_PURIFIED            0x00000010
#define PSITEM_FLAG_PURIFYING           0x00000020

/// Flags for locked status (as in a locked container). If locked it can be opened only with lockpick or key
#define PSITEM_FLAG_LOCKED              0x00000040

/// Flag to allow locking/unlocking of the item. It will add the "lock/unlock" icon in the context menu of the item
#define PSITEM_FLAG_LOCKABLE            0x00000080

/// Flag for an un-pickupable item (remains fixed)
#define PSITEM_FLAG_NOPICKUP            0x00000100

/// Flag defines any item as a key to a locked object
#define PSITEM_FLAG_KEY                 0x00000200

/// Transient items are removed from the world approx 3hrs after creation, if not picked up by someone
#define PSITEM_FLAG_TRANSIENT           0x00000400

/// Flag to indicate that lock is unpickable
#define PSITEM_FLAG_UNPICKABLE          0x00000800

/// Flag to indicate that lock is a security lock that stays locked
#define PSITEM_FLAG_SECURITYLOCK        0x00001000

/// Flag to indicate that players shouldn't be able to put items into this container
#define PSITEM_FLAG_NPCOWNED            0x00002000

/// Flag defines any item as a key to a locked object that can be used to make a copy of the key
#define PSITEM_FLAG_MASTERKEY           0x00004000

/// Flag defines if CD should be used on this item
#define PSITEM_FLAG_USE_CD              0x00008000

/// Flag defines if item can be used while equipped
#define PSITEM_FLAG_ACTIVE              0x00010000

/// Flag defines if item can *not* be stacked
#define PSITEM_FLAG_UNSTACKABLE         0x00020000

/// Flag defines if item can be stacked
#define PSITEM_FLAG_STACKABLE           0x00040000

/// Flag defines if item is done by setting (unused by the server, it's more for ordering in the db)
#define PSITEM_FLAG_SETTINGITEM         0x00080000

/// Flag for an un-pickupable item (remains fixed) - weak variant
#define PSITEM_FLAG_NOPICKUPWEAK        0x00100000

/// Flag for identifiable item (not yet identified)
#define PSITEM_FLAG_IDENTIFIABLE        0x00200000

#define KEY_SKELETON      ((unsigned int)-2)

#define MAX_STACK_COUNT        65  // This is the most items a player can have in a stack

typedef unsigned int PSITEM_FLAGS;

/// The maximum number of modifiers that can be applied to a single instance of an item on top of the base item
#define PSITEM_MAX_MODIFIERS    10

class ActiveSpell;
class psItem;
class psCharacter;
class gemItem;
class psSectorInfo;
struct psItemCategory;
class psScheduledItem;
class psWorkGameEvent;

/**Stores the randomized stats from the loot randomizer, it could be used to apply any global special effect
 * which is able to change various properties of the item: cost, mesh, name...
 * This makes up a 3 levels modification of a single item in order of priority:
 * The first level is the specific instance modifications: like name or descriptions
 * The second level is the specific instance modifications coming from the loot modifiers table
 * (probably should change the names when they will be used also to do changes to crafted items)
 * The third level is the generic item stats.
 * The active boolean variables is mostly an optimization to avoid checking if the overlay has data inside.
 */
class RandomizedOverlay
{
public:
    RandomizedOverlay(); ///< Constructor.
    ~RandomizedOverlay(); ///< Destructor.
    bool active; ///< Notifies if this overlay is active and should be applied.
    csString name; ///< The name which should be replaced for this randomized item.
    float weight; ///< Weigth of the item changed by the loot modifier rules.
    float latency; ///< Latency of the weapon changed by the loot modifiers rules.
    float damageStats[PSITEMSTATS_DAMAGETYPE_COUNT]; ///< Damage/protection stats changed by the loot modifiers rules.
    ApplicativeScript* equip_script; ///< Equip script for this item overriding the main one from loot modifiers rules.
    psMoney price; ///< Price calculated from the loot modifiers rules which overlays the basic one.
    csArray<ItemRequirement> reqs; ///< Array of all the stat prerequisites needed to equip this item.
    csString mesh; ///< Overriden mesh of this item from the basic one.
    csString icon; ///< Overriden icon of this item from the basic one.
};

/** This class embodies item instances in the game.
* Every item that can be picked up, dropped, traded, equipped, bought or sold is a psitem.
*
* This class has some specific design philosophy.
*
* First, each item instance has a unique 32 bit identifier.  The database ensures that there can be only one entry
*  per item identifier.  This means if an item is truly duped it will not persist between world resets since the next
*  load from the database will find only one entry for that item.
* In order to make this mean anything, certain situations must be carefully coded to avoid bypassing this protection.
*  In any case where 1 item becomes multiple items (such as splitting a stack) the logic should be well defined and located
*  in just one place (preferably within the psItem class).  If a bug does arise, it should require a fix to a single location.
* Any instance where an item is legitimately duped will need to do more work to actually copy the item.  The code will be required
*  to basically reconstruct the item from the ground up.  The new item will be a truly new item.  Do not take this lightly.
*
*  ----The only time an item instance should be duplicated is for Game Master level functions.----
*
* Items are built from statistic entries (item stats).  Most items will have a base item stat entry and 0 or more item stat
*  entries that act as modifiers.  Modifiers are permanent additions to an item instance.  For example, you may be able to buy
*  a normal 'short sword' from a merchant.  This would be the base stat of the item.  If you use a trade skill to sharpen the
*  short sword, that may add the 'sharpened' modifier to the sword.  The resulting item instance will still (and always) have
*  the base stat of 'short sword'.  In addition it will have the modifier of 'sharpened'.  It may gain or lose modifiers through
*  other actions.  Temporary effects such as spells, poisons, etc are NOT modifiers.  They should be handled differently.
* At this time there is no code to handle effects on items, but it should not be difficult to add once the parameters of effects
*  are defined.
* The other type of item instance is a unique item.  Unique items are truely unique.  The unique item has its own entry for its
*  stats in the database.  Only one item instance will ever refer to this entry.  If the item instance is destroyed, so is the entry.
*  In some situations the item stats may be altered - either through an extremely difficult quest, or GM intervention.  These alterations
*  to the stats will alter the entries of the unique item stats.  In addition, normal modifiers may also be applied.
*
* ----Unique items should be very difficult to obtain as they are a larger drain on server resources----
*
* An item instance has four persistant-state related operations:
* Load() - Translates data from a database item instance entry into a usable psItem
* Save() - Saves a psItem.  This creates a new Unique Identifier (UID) for the item instance if needed (if the UID is 0).
* DeleteItem() - (Called from psCharacterLoader) Destroys the item instance entry from the database.  The psItem entry in memory should also
*  be destroyed promptly.
*
* Weight Rules:
*   TODO: Containers will have a maximum carrying weight.
*   Weight of stacks is a the count of the stack multiplied by the weight of the base item stat.
*
* Size Rules:
*   Size is single number (float) that relates to the length of the longest dimension measured in centimeters.
*   Stacks do not affect size.
*   Containers have a maximum content size.
*   65535 = infinite size. Items with 65535 cannot be put into any containers.  Containers with 65535 max content size can contain any item except items with 65535 size.
*
*  TODO: Add effect data - such as spell effects or poision or whatever.
*  TODO: Merge various mutually exclusive stats into unions to save space.
*/
class psItem : public iScriptableVar, public iDeleteObjectCallback
{
public:

    /** Constructs a psItem.  Does not assign a UID.
     * After construction the caller should set the location in the world or place the item into a container
     * with the parent's AddItemToContainer() member function.  It should then call Save() to save the instance
     * to the database.
     */
    psItem();

    /** Destructs the psItem, and, if this is a container, all contained items. Does NOT remove any items from the database.
     */
    virtual ~psItem();

    /** Return a string identifying this object as an Item
     */
    virtual const char* GetItemType()
    {
        return "Item";
    }

    gemItem* GetGemObject()
    {
        return gItem;
    }
    void SetGemObject(gemItem* object);

    psWorkGameEvent* GetTransformationEvent()
    {
        return transformationEvent;
    }
    void SetTransformationEvent(psWorkGameEvent* t)
    {
        transformationEvent = t;
    }

    /// Handles deleted gem objects.
    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);

    ///inform clients on view updates
    void UpdateView(Client* fromClient, EID eid, bool clear);

    bool SendItemDescription(Client* client);

    bool SendContainerContents(Client* client, int containerID = CONTAINER_INVENTORY_BULK);

    /** Send an item to the client.
     * @param client The client the message is for.
     * @param containerID the ID of the owning container
     * @param slotID the slot this item is in
     * TODO check if we can't get the data our self
     */
    void ViewItem(Client* client, int containerID, INVENTORY_SLOT_NUMBER slotID);

    bool SendActionContents(Client* client, psActionLocation* action);

private:


    /** Fills up the message with the details about the items in the container.
      * @param client The client the message is for. Used to figure out ownership flags.
      * @param outgoing The message that needs to be populated.
      */
    void FillContainerMsg(Client* client, psViewContainerDescription &outgoing);

    void SendCraftTransInfo(Client* client);

    void GetComboInfoString(psCharacter* character, uint32 designID, csString &comboString);

    void GetTransInfoString(psCharacter* character, uint32 designID, csString &transString);

    bool SendBookText(Client* client, int containerID, int slotID);

    void SendSketchDefinition(Client* client);

    void SendMusicalSheet(Client* client);

    /// The 32 bit Unique Identifier of this item.
    uint32 uid;

    /// This flag is true when the item cannot be moved around in inventory, during repairs for example.
    bool item_in_use;

    /// If set, this is a pointer to the item that is currently being trade skill transformed.
    psWorkGameEvent* transformationEvent;

    /// Location in the game world. Only valid if this item is not being carried by a player (directly or indirectly)
    struct
    {
        psSectorInfo* loc_sectorinfo;
        float loc_x,loc_y,loc_z;
        float loc_xrot,loc_yrot,loc_zrot;
        InstanceID worldInstance;
    } location;

    /// 0% means item does not reduce decay at all.  100% means item does not decay.
    float decay_resistance; // percent 3.2


    float item_quality;                     ///< Current quality of item
    float crafted_quality;                  ///< The crafted max quality of the item.


    /// Original quality of item, used to detect save requirements
    float item_quality_original;

    /// The number of items in this stack if this is a stackable item.
    unsigned short stack_count;

    /// This is the number of charges currently held in the item
    int charges;

    /// Whether or not to use natural armour for quality.
    bool useNat;


protected:
    /** Flags for this item instance.  Flags are described at the top of this file.
     * Note that psItemStats also has flags that are stored separately.
     *  If you're looking for a flag that you think should be here, check psItemStats to be sure
     *  it's not a stat flag instead of an instance flag.
     *  Flags are indicated in the database as text, e.g. "NOPICKUP" in the flags column
     */
    PSITEM_FLAGS flags;
private:
    /** This field stores the UID of the character that crafted this item if it was crafted.
     * In this way we can add recognition to crafters as their items traverse the game.
     */
    PID crafter_id;
    /** This field stores the guild UID of a guild that has CERTIFIED this item.
     * The concept of this is that a guild as an entity can certify certain works from any crafter.
     * This gives the ability to mark items after some kind of inspection through a method not based
     * on guild membership.  I.E. a crafter in a guild that crafts an item will not automatically have
     * his wares marked with guild id.  Rather someone of sufficient rank will need to mark the item.
     */
    unsigned int guild_id;

    /* Container Hierarchy Data
     *
     *  The following field allows objects-within-objects (containers) to be described.
     *
     */
    /// Points to the object that contains this object, or NULL if there is none.
    InstanceID parent_item_InstanceID;


    /** Dual use.  Indicates either the slot within the parent item if contained, or the slot in the player's inventory/equipment/bulk if appropriate.
     * If the item is on the ground in the world and not inside of a container item this field should be ignored.
     */
    INVENTORY_SLOT_NUMBER loc_in_parent;


    /** Pointer to the psCharacter that holds this item
     * The character can hold the item either directly in their inventory/bank/equipment or in a container they hold.
     * NULL if this item is on the ground or in another item that is on the ground.
     *
     */
    psCharacter* owning_character;

    /** The owner of the item may not be infact online so have to store the ID since the
        above may be undefined in some cases.
    */
    PID owningCharacterID;
    PID guardingCharacterID;

    /** The basic stats of this item.
     * This can point to a common shared entry from the basic stats list, or a unique entry.
     * Check for the PSITEM_FLAG_UNIQUE_ITEM flag to see which.
     */
    psItemStats* base_stats;
    /** The current stats of this item.  This value should be used for pretty much everything.
     * Initially this points to the same entry as the base_stats.  If modifiers or effects are applied
     * this will point to an allocated temporary stats entry that contains the result of all modifiers
     * and effects on top of the base.
     */
    psItemStats* current_stats;
    /** The list of modifiers for this item.
     * We could keep just the base and current, but then we wouldn't be able to track what kinds of modifiers
     * had been applied to this item instance, and it would be difficult to find where to add the modifiers in
     * the database.
     */
    psItemStats* modifiers[PSITEM_MAX_MODIFIERS];

    // Lock stuff
    unsigned int lockStrength;
    PSSKILL lockpickSkill;

    // A key can open multiple locks
    csArray<unsigned int> openableLocks;

    psScheduledItem* schedule;

    /** Saves this item to the database. Generates an item ID if necessary.
     * This creates a new UID for the item and creates appropriate database entries if necessary.
     * Otherwise it updates the existing entry.
     * This calls a database function directly.
     */
    void Commit(bool children = false);

    /**
     * ItemQuality for items that decay changes so often, writing every change to the db
     * is overly onerous.  Also, any loss of data due to server crashes benefits the player
     * rather than hurting him, so it isn't too bad if it is never updated.  This function
     * is called by the dtor to make sure quality is only updated minimal times.
     */
    void UpdateItemQuality(uint32 id, float qual);

    /// Commit() is called primarily from psDelaySave::Trigger()
    friend class psDelaySave;

    /**
     * Holds a custom item name. Can be empty.
     */
    csString item_name;

    /**
     * Holds a custom item description. Can be empty.
     */
    csString item_description;

    /// The ActiveSpell* of the equip script, for cancelling on unequip.
    ActiveSpell* equipActiveSpell;

    /// A structure which holds the creative data about this item.
    psItemCreativeStats creativeStats;

    ///stores the modifications to apply to this item. This is a cache used to increase performance.
    RandomizedOverlay* itemModifiers;

    ///stores the ids of the modifications to apply to this item. Used for save and copy.
    csArray<uint32_t> modifierIds;

public:
    /** Loads data from a database row into the current psItem.
     * This is used only by the character loader.
     *  On return parentid is set to the UID of the parent item.  UID 0 is reserved for no parent.
     */
    virtual bool Load(iResultRow &row);

    /** Queues this item to be saved to the database.  (DB action will be executed after 500ms)
     *  Call this after EVERY change of a persistant property, and the system
     *  will ensure that any sequence of changes will result in only one DB hit.
     */
    void Save(bool children);

    /// Saves this item now if it hasn't been saved before, to force the generation of a UID
    void ForceSaveIfNew();

    /** Flags that this item has been fully loaded, and is allowed to be saved on changes.
     *  This MUST be called after this item is constructed AND it's initial
     *  properties are set.  (includes normal loading, InstantiateBasicItem(), etc.)
     */
    void SetLoaded()
    {
        loaded = true;
    }

    /** Check to see if the character meets the requirements for this item.
     */
    bool CheckRequirements(psCharacter* character, csString &resp);

    /** Returns the UID of this item instance.
     * 0 is reserved for no UID assigned yet.
     * Items are initially created with a UID of 0.  Save() generates a unique item id.
     */
    uint32 GetUID()
    {
        return uid;
    }

    /** Sets the Unique ID of this item. DO NOT CALL THIS.
     * This should ONLY be called internally or from psCharacterLoader.
     */
    void SetUID(uint32 v);

    /// Returns true if the crafter character ID is valid
    bool GetIsCrafterIDValid();
    /// Set wether the crafter ID is valid.  SetCrafterID() sets this to true for you.
    void SetIsCrafterIDValid(bool v);

    /// Returns true if the certifying guild ID is valid
    bool GetIsGuildIDValid();
    /// Set wether the guild ID is valid.  SetGuildID() sets this to true for you.
    void SetIsGuildIDValid(bool v);
    /// Returns the UID for the crafter of this item.  Be sure to check GetIsCrafterIDValid()!

    PID GetCrafterID() const
    {
        return crafter_id;
    }
    /// Sets the UID for the cracter of this item.  Generally used immediately after completing the crafting work.
    void SetCrafterID(PID v);

    /// Returns the UID for the guild who has certified this item.  Be sure to check GetIsGuildIDValid()!
    unsigned int GetGuildID() const
    {
        return guild_id;
    }
    /// Sets the UID for the guild certifying this item.
    void SetGuildID(unsigned int v);

    /// Gets the maximum possible quality of the item by reading the item_stat.
    float GetMaxItemQuality() const;
    /// Sets the maximum allowed quality of the item by changing the item_stat.
    void SetMaxItemQuality(float v);

    /// Gets the quality of the item.
    float GetItemQuality() const;
    /// Sets the quality of the item.
    void SetItemQuality(float v);
    /// Returns a description of the quality level of item.
    const char* GetQualityString();

    /// Parses an item flags list and returns the flags.
    PSITEM_FLAGS ParseItemFlags(csString flagstr);

    /// Returns item_stats id of repair tool required to fix this item, or 0.
    int GetRequiredRepairTool();
    /// Returns the is_consumed flag of repair tool required to fix this item.
    bool GetRequiredRepairToolConsumed();

    /// Returns skill id of skill needed to identify examined items.
    int GetIdentifySkill();
    /// Returns minimum skill level to identify certain examined items.
    int GetIdentifyMinSkill();

    /// Set the decay resistance percentage for the item
    void SetDecayResistance(float v);
    /// Set the item decay factor
    void SetItemDecayRate(float v);
    /// Get the item decay factor
    float GetItemDecayRate()
    {
        return base_stats->GetDecayRate();
    }
    /// Adjust the item quality by an amount of decay
    float AddDecay(float severityFactor);

    bool IsInUse()
    {
        return item_in_use;
    }
    void SetInUse(bool flag)
    {
        item_in_use = flag;
    }

    /** Returns true if this item is based off of unique statistics.
     * Items with unique statistics will be especially rare.  Some special operations are
     * possible on unique items
     */
    bool GetIsUnique() const;

    /** Returns true if this item has this modifier in its modifier list.
     * This can be used to make sure an item doesn't get the same modifier multiple times or if an
     * item needs to have a specific modifier to be used for something.
     *
     * TODO: In the future we may expand this to functions that check for "similar" modifiers to a given
     *       modifier.
     */
    bool HasModifier(psItemStats* modifier);

    /** Adds a modifier to this item instance if there is space available.
     * Current stats are automaticaly updated.
     *
     * TODO: A DeleteModifier() function may be warranted in the future.
     */
    bool AddModifier(psItemStats* modifier);

    /** Returns the modifier at a specific index 0 through PSITEM_MAX_MODIFIERS-1
     *
     */
    psItemStats* GetModifier(int index);

    /** Creates a new blank instance of it's class (i.e. psItem or its subclass) */
    virtual psItem* CreateNew()
    {
        return new psItem();
    }

    /** Duplicates an item instance.
     *  The parameter is the stack count of the new item.
     *  The return value must be checked for NULL.
     *  The returned item will not have a valid location nor a valid UID.  The location in the world or
     *  on an owning player must be set first, and then Save() called to generate a UID and save to the database.
     */
    virtual psItem* Copy(unsigned short newstackcount);

    /** Copies values of its attributes to item 'target'.
      */
    virtual void Copy(psItem* target);

    /** Splits an item instance representing a stack into two smaller stacks.
     *  The parameter is the size of the new stack.
     *  The return value must be checked for NULL.
     *  The returned item will not have a valid location nor a valid UID.  The location in the world or
     *  on an owning player must be set first, and then Save() called to generate a UID and save to the database.
     */
    psItem* SplitStack(unsigned short newstackcount);


    /** Combines 'stackme' with this item.
     *  You must call CheckStackableWith first; this ASSERTS if the two are not stackable. */
    void CombineStack(psItem* &stackme);


    /** TODO:  Comment me with something more than "Gets the attack animation ID"
     *
     */
    int GetAttackAnimID(psCharacter* pschar);

    /// Returns the decay value of the item.  See the decay member for a description of what this is.
    float GetDecay();
    /// Sets the decay value of the item.  See the decay member for a description of what this is.
    void SetDecay(float v);

    /// Returns the stack count.  Be sure to call GetIsStackable() first!
    unsigned short GetStackCount() const
    {
        return stack_count;
    }
    /** Sets the stack count.  Be sure to call GetIsStackable() first!
     *  Don't call this to try and combine or split stacks!  That logic is done in
     *  CombineStack() and SplitStack().
     */
    void SetStackCount(unsigned short v);


    /**  Returns a pointer to the character who is holding this item directly or indirectly (including containers and bank slots) or NULL if none.
     * Returns NULL if on the ground or in a container on the ground.
     *
     * This should always be valid since items should be destroyed when the character logs off
     */
    psCharacter* GetOwningCharacter()
    {
        return owning_character;
    }
    /** Get the ID of the owning character.  This is required in cases where the owning
      * character may not be online and the above pointer is undefined.
      */
    PID GetOwningCharacterID() const
    {
        return owningCharacterID;
    }

    /// Alters the owning character of this item.  Also see UpdateInventoryStatus.
    virtual void SetOwningCharacter(psCharacter* owner);

    /** Item guardians: items dropped in the world or placed in a public
     *  container are not owned, but "guarded" by a character. */
    PID  GetGuardingCharacterID() const
    {
        return guardingCharacterID;
    }
    void SetGuardingCharacterID(PID guardian)
    {
        guardingCharacterID = guardian;
    }

    /// Returns the item that contains this item, or NULL if it's not contained by another item.
    uint32 GetContainerID() const
    {
        return parent_item_InstanceID;
    }
    void SetContainerID(uint32 parentId)
    {
        parent_item_InstanceID = parentId;
    }


    /**
     * Returns the location of this item in its parent item or in the
     * player's equipment, bulk or bank as appropriate.
     */
    INVENTORY_SLOT_NUMBER GetLocInParent(bool adjustSlot=false);

    /// Please consider using UpdateInventoryStatus() instead.
    void SetLocInParent(INVENTORY_SLOT_NUMBER location);

    /** Used to set the UNIQUE flag in addition to setting base stats pointer
     * TODO: This is pretty much untested at this point.
     */
    void SetUniqueStats(psItemStats* statptr);

    /** Alters the base stats of this item instance.
     * If called on a unique item it revokes the unique item status of this item!
     * If called on a unique item the unique item stats should be destroyed elsewhere.
     */
    void SetBaseStats(psItemStats* statptr);

    /** Returns the base stats for this item instance.  You may want to use this only to see this item instance is a specific type of item
     * Do not use for retrieval of values for calculations (such as damage).
     */
    psItemStats* GetBaseStats() const
    {
        return base_stats;
    }

    /** Sets the current item stats.  DO NOT USE!
     * This is used mostly internally.  The current stats either point to the same entry as the base stats or a stats entry that
     * is the sum of the base stats plus modifiers and effects.
     */
    void SetCurrentStats(psItemStats* statptr);

    /** Gets a pointer to the current stats.  You probably dont want to use this.
     * Use the functions below which may apply additional logic to determine what value to return.
     */
    psItemStats* GetCurrentStats() const
    {
        return current_stats;
    }

    /** In some cases some modifiers or effects may be difficult to reverse (for example percent bonuses applied and removed in different orders).
     * In these cases it should be safe to call RecalcCurrentStats() which takes a bit longer, but builds the current stats back up from the base.
     */
    void RecalcCurrentStats();

    PSITEM_FLAGS GetFlags()
    {
        return flags;
    }

    void SetFlags(int f)
    {
        flags = f;
    }

    virtual int GetPurifyStatus() const
    {
        return 0;
    }

    // Interface to itemstats

    /*  The following functions mostly call into the current stats to return the requested values.
     *  In the future additional logic about value modification may be added to these functions.
     *  For this reason these functions should ALWAYS be used rather than calling GetCurrentStats()->Get*()
     *   unless you REALLY know what you're doing and REALLY mean to bypass the intended value of the item instance.
     */

    bool GetIsMeleeWeapon();
    bool GetIsRangeWeapon();
    bool GetIsAmmo();
    bool GetIsArmor();
    bool GetIsShield();
    bool GetIsContainer();
    bool GetIsTrap();
    bool GetIsConstructible();
    bool GetCanTransform();
    bool GetUsesAmmo();
    bool GetIsStackable() const;
    bool GetIsEquipStackable() const;
    PSITEMSTATS_CREATIVETYPE GetCreative();
    /// Write creative stuff such as lit text (eg book) or map data.
    bool SetCreation(PSITEMSTATS_CREATIVETYPE, const csString &, csString);
    /** Gets the creator of this creative and the creator setting status.
     *  @param creatorStatus The status of the creator setting in this creative
     *  @return PID The PID of the player who created this creative or 0 if not set or invalid.
     */
    PID GetCreator(PSITEMSTATS_CREATORSTATUS &creatorStatus);
    /// sets the creator (i.e. author, artist, etc) of creative things
    void SetCreator(PID, PSITEMSTATS_CREATORSTATUS);

    /**
     * Checks if the creator of the book is the one passed as argument.
     *
     * @param pid the PID of the character we are checking creator status.
     */
    bool IsThisTheCreator(PID pid);
    bool GetBuyPersonalise();
    const char* GetName() const;
    const char* GetDescription() const;
    void SetName(const char* newName);
    void SetDescription(const char* newDescription);
    const char* GetStandardName();
    const char* GetStandardDescription();

    /** Allows to add a modification to the item.
     *  @todo For now the system, even if adapted to allow an unspecified amount of modifications now, in the LOAD
     *        and Save it supports only 3: a prefix, a suffix and an adjective. Should make the schema allow
     *        to receive an unspecified amount of modifiers too. so the pos variable should be removed when that
     *        will be implemented.
     *  @param id The id of the modification to apply to this item.
     *  @param pos The position in the array where to put this modifications (note it overwrites what's there)
     */
    void AddLootModifier(uint32_t id, int pos = -1);

    ///  Recalculates the modifications for this item
    void UpdateModifiers();

    float GetRange() const
    {
        return current_stats->GetRange();
    }
    csSet<unsigned int> GetAmmoTypeID() const
    {
        return current_stats->GetAmmoTypeID();
    }

    psWeaponType *GetWeaponType();
    PSSKILL GetWeaponSkill(PSITEMSTATS_WEAPONSKILL_INDEX index);
    float GetLatency();
    float GetDamage(PSITEMSTATS_DAMAGETYPE dmgtype);
    PSITEMSTATS_AMMOTYPE GetAmmoType();

    float GetPenetration();
    float GetUntargetedBlockValue();
    float GetTargetedBlockValue();
    float GetCounterBlockValue();
    PSITEMSTATS_ARMORTYPE GetArmorType();
    float GetDamageProtection(PSITEMSTATS_DAMAGETYPE dmgtype);
    float GetHardness();
    float GetWeaponAttributeBonus(PSITEMSTATS_STAT stat);
    PSITEMSTATS_STAT GetWeaponAttributeBonusType(int index);
    float GetWeaponAttributeBonusMax(int index);

    float GetWeight();
    float GetItemSize();
    /// Gets the total size of the items in the stack.
    float GetTotalStackSize()
    {
        return GetItemSize()*stack_count;
    }

    unsigned short GetContainerMaxSize();

    /** Gets the slots available in this item (only containers) which means
     *  also the maximum amount of items which can be stored in this container.
     *
     *  @note this function uses the base item stats of this item.
     *  @return The number of slots available in this container.
     */
    int GetContainerMaxSlots();
    PSITEMSTATS_SLOTLIST GetValidSlots();
    bool FitsInSlots(PSITEMSTATS_SLOTLIST slotmask);
    bool FitsInSlot(INVENTORY_SLOT_NUMBER slot);

    /** Proxies the same function in psitemstats which gets the list of mesh to remove when this item is equipped
     *  in the specified slot.
     *  @param slot The slot this item is being equipped into.
     *  @param meshName The meshName we are sarching for the slot to remove for it.
     *  @return The list of meshes to remove when equipping this item.
     */
    csString GetSlotRemovedMesh(int slot, csString meshName = "")
    {
        return GetBaseStats()->GetSlotRemovedMesh(slot, meshName);
    }
    float GetDecayResistance();
    psMoney GetPrice();
    psMoney GetSellPrice();    ///< Merchants want a percentage
    psItemCategory* GetCategory();


    float GetVisibleDistance();

    void GetLocationInWorld(InstanceID &instance,psSectorInfo** sectorinfo,float &loc_x,float &loc_y,float &loc_z,float &loc_yrot) const;
    void SetLocationInWorld(InstanceID instance,psSectorInfo* sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot);

    /** Get the x and z axis rotations for the item (y obtained in GetLocationInWorld)
     * @param loc_xrot the variable in which the x rotation will be stored
     * @param loc_zrot the variable in which the z rotation will be stored
     */
    void GetXZRotationInWorld(float &loc_xrot, float &loc_zrot);

    /** Set the x and z axis rotations for the item (y set in SetLocationInWorld)
     * @param loc_xrot the variable used to set the x rotation of the item
     * @param loc_zrot the variable used to set the z rotation of the item
     */
    void SetXZRotationInWorld(float loc_xrot, float loc_zrot);

    /** Get the x,y and z axis rotations for the item
     * @param loc_xrot the variable in which the x rotation will be stored
     * @param loc_yrot the variable in which the y rotation will be stored
     * @param loc_zrot the variable in which the z rotation will be stored
     */
    void GetRotationInWorld(float &loc_xrot, float &loc_yrot, float &loc_zrot);

    /** Set the x, y and z axis rotations for the item
     * @param loc_xrot the variable used to set the x rotation of the item
     * @param loc_yrot the variable used to set the x rotation of the item
     * @param loc_zrot the variable used to set the z rotation of the item
     */
    void SetRotationInWorld(float loc_xrot, float loc_yrot, float loc_zrot);

    psSectorInfo* GetSector() const
    {
        return location.loc_sectorinfo;
    }

    /** Get the Mesh Name for the item.
     *
     *  Used for standalone or weilded mesh.
     */
    const char* GetMeshName();

    /** Get the Texture Name for the item.
     *
     *  Used when worn and attached to the mesh given by part name.
     */
    const char* GetTextureName();

    /** Get the Part Name for the item.
     *
     *  This is the name of the part that the texture should be attached to
     *  if no change of mesh.
     */
    const char* GetPartName();

    /** Get the Part Mesh Name for the item.
     *
     *  This is the new mesh to be attached to the location given
     *  by the pattern Part Name.
     */
    const char* GetPartMeshName();

    /** Get the Image Name for the item.
     *
     *  Used in inventory and other location where item has to be presented
     *  by a 2D image.
     */
    const char* GetImageName();

    csString GetQuantityName(); // returns the quantity and plural name in a string
    static csString GetQuantityName(const char* namePtr, int stack_count, PSITEMSTATS_CREATIVETYPE creativeType, bool giveDetail = false);

    // For future use, see notes for mesh_index variables, etc.
    /*
    unsigned int GetMeshIndex();
    unsigned int GetTextureIndex();
    unsigned int GetTexturePartIndex();
    unsigned int GetImageIndex();
    */

    void UpdateInventoryStatus(psCharacter* owner,uint32 parent_id, INVENTORY_SLOT_NUMBER slot);

    bool IsEquipped() const;
    // an item can be equipped, but not active. This happens when its equip requirements
    // fail to be met while the item is still equipped.
    //   IsActive = true means the progression script for this item is active
    //   IsActive = false means the progression script for this item is inactive
    bool IsActive() const;
    void SetActive(bool state);

    void RunEquipScript(gemActor* actor);
    void CancelEquipScript();

    const char* GetModifiersDescription();

    /**
     * Check if otheritem is stackable with this item.
     *
     * @param otheritem The item to check if this item is stackable with.
     * @param precise Is the quality, max quality and crafter the same.
     * @param checkStackCount Sheck the stack count.
     * @param checkWorld Checks if stackability is possible in the world (eg: instances comparing)
     */
    bool CheckStackableWith(const psItem* otheritem, bool precise, bool checkStackCount = true, bool checkWorld = true) const;

    const char* GetSound();

    /// This is used by the math scripting engine to get various values.
    double GetProperty(MathEnvironment* env, const char* ptr);
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    const char* ToString()
    {
        return item_name.GetDataSafe();
    }

    bool GetIsLocked()
    {
        return ((flags & PSITEM_FLAG_LOCKED)? true : false);
    }
    void SetIsLocked(bool v);

    bool GetIsLockable()
    {
        return ((flags & PSITEM_FLAG_LOCKABLE)? true : false);
    }
    void SetIsLockable(bool v);

    bool GetIsSecurityLocked()
    {
        return ((flags & PSITEM_FLAG_SECURITYLOCK)? true : false);
    }
    void SetIsSecurityLocked(bool v);

    bool GetIsUnpickable()
    {
        return ((flags & PSITEM_FLAG_UNPICKABLE)? true : false);
    }
    void SetIsUnpickable(bool v);

    /** Gets if the item has a no pickup flag set.
     *  @return TRUE if the item has a no pickup flag set.
     */
    bool GetIsNoPickup()
    {
        return (GetIsNoPickupStrong() || GetIsNoPickupWeak());
    }

    /** Checks if the item has a strong no pickup flag set.
     *  @return TRUE has a strong no pickup flag set.
     */
    bool GetIsNoPickupStrong()
    {
        return ((flags & PSITEM_FLAG_NOPICKUP)? true : false);
    }

    /** Checks if the item has a weak no pickup flag set.
     *  @return TRUE has a weak no pickup flag set.
     */
    bool GetIsNoPickupWeak()
    {
        return ((flags & PSITEM_FLAG_NOPICKUPWEAK)? true : false);
    }

    bool GetIsCD() const
    {
        return ((flags & PSITEM_FLAG_USE_CD)? true : false);
    }
    void SetIsCD(bool v);

    bool GetIsNpcOwned() const
    {
        return (flags & PSITEM_FLAG_NPCOWNED) != 0;
    }
    void SetIsNpcOwned(bool v);

    bool GetIsKey() const
    {
        return ((flags & PSITEM_FLAG_KEY)? true : false);
    }
    void SetIsKey(bool v);

    bool GetIsMasterKey() const
    {
        return ((flags & PSITEM_FLAG_MASTERKEY)? true : false);
    }
    void SetIsMasterKey(bool v);

    bool IsTransient()
    {
        return ((flags & PSITEM_FLAG_TRANSIENT) ? true : false);
    }
    void SetIsTransient(bool v);

    /** Sets the pickupable flag in order to not allow/allow the item to be picked up.
     *  @param v FALSE if the no pickup flag should be set.
     */
    void SetIsPickupable(bool v);

    /** Sets the weak pickupable flag in order to not allow/allow the item to be picked up.
     *  @param v FALSE if the weak no pickup flag should be set.
     */
    void SetIsPickupableWeak(bool v);

    void SetIsItemStackable(bool v);
    void ResetItemStackable();

    bool GetIsSettingItem() const
    {
        return ((flags & PSITEM_FLAG_SETTINGITEM)? true : false);
    }
    void SetIsSettingItem(bool v);

    PSSKILL GetLockpickSkill()
    {
        return lockpickSkill;
    }
    void SetLockpickSkill(PSSKILL v);

    unsigned int GetLockStrength()
    {
        return lockStrength;
    }
    void SetLockStrength(unsigned int v);

    bool CompareOpenableLocks(const psItem* key) const;
    bool CanOpenLock(uint32 id, bool includeSkel) const;
    void AddOpenableLock(uint32 v);
    void RemoveOpenableLock(uint32 v);
    void ClearOpenableLocks();
    void CopyOpenableLock(psItem* origKey);
    void MakeSkeleton(bool b);
    bool GetIsSkeleton();
    csString GetOpenableLockNames();

    psScheduledItem* GetScheduledItem()
    {
        return schedule;
    }
    void SetScheduledItem(psScheduledItem* item)
    {
        schedule = item;
    }
    void ScheduleRespawn();

    /// Gets the reduction of this weapon against the armor given
    float GetArmorVSWeaponResistance(psItemStats* armor);

    /// Gets the book text, should only be used if this is a book.
    csString GetBookText()
    {
        return GetLiteratureText();
    }
    /// Sets the book text, should only be used if this is a book.
    bool SetBookText(const csString &newText);
    /// Sets sketch data
    bool SetSketch(const csString &newSketchData);
    /// Sets the musical sheet
    bool SetMusicalSheet(const csString &newMusicalSheet);

    bool HasCharges() const;
    bool IsRechargeable() const;
    void SetCharges(int charges);
    int GetCharges() const;
    int GetMaxCharges() const;

    ///returns the special command assigned to this item
    csString GetItemCommand()
    {
        return GetBaseStats()->GetItemCommand();
    }

    /** Called when an item is completely destroyed from the persistant world
     *
     *  Persistant refers to the item existing through a reset of the server.  This definition can affect when an item
     *  should be "destroyed" (removed from the database).
     *  For example, if items dropped on the ground will be forever lost if the server goes down, then when an item
     *  is dropped on the group, DestroyItem() should be used instead of UpdateItem().  When the item is picked up
     *  again, CreateItem() should be called.
     *
     */
    bool Destroy();

    /**
     * This function handles creating an event manager event at the right time
     * to get transient objects removed from the world automatically.
     */
    void ScheduleRemoval();

    bool DeleteFromDatabase();

    /*  Objects of this class utilize a pool allocator.  The pool allocator can be found in poolallocator.h in the psutil library.
     *  The pool allocator allows us to avoid allocating memory from and releasing memory to the heap over and over for things that are
     *  constantly shifting in the number of allocations (such as items in the game).  It also reduces memory usage overhead slightly.
     *
     *  It should be possible to debug memory leaks from a specific pool easier as well since you can analyze allocations solely from that pool
     *  without getting flooded with allocations from everywhere else.
     */

    ///  The new operator is overriden to call PoolAllocator template functions
    void* operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void*);

    /** Checks the writeable flag in the item_stats and tell if this is the case.
     *  @return TRUE if the item is writeable.
     */
    bool GetIsWriteable();
    /** Checks the readable flag in the item_stats and tell if this is the case.
     *  @return TRUE if the item is readable.
     */
    bool GetIsReadable();

    /// return creative contents of sketches.
    const csString &GetSketch(void)
    {
        return creativeStats.creativeType == PSITEMSTATS_CREATIVETYPE_NONE ? GetBaseStats()->GetSketch() : creativeStats.content;
    }
    /// return creative contents of musical sheets.
    const csString &GetMusicalSheet(void)
    {
        return creativeStats.creativeType == PSITEMSTATS_CREATIVETYPE_NONE ? GetBaseStats()->GetSketch() : creativeStats.content;
    }
    /// return creative contents of books.
    const csString &GetLiteratureText(void)
    {
        return creativeStats.creativeType == PSITEMSTATS_CREATIVETYPE_NONE ? GetBaseStats()->GetSketch() : creativeStats.content;
    }
    /// return the background image used in this creative.
    const csString &GetCreativeBackgroundImg()
    {
        return creativeStats.creativeType == PSITEMSTATS_CREATIVETYPE_NONE ? GetBaseStats()->GetCreativeBackgroundImg() : creativeStats.backgroundImg;
    }

    void PrepareCreativeItemInstance();

    /// return Rarity as 0-100% range
    float GetRarity();

    /// return Rarity description. example: "rare (0.7%)"
    csString GetRarityName();

    /// Get the identifiable status of the item
    bool GetIsIdentifiable()
    {
        return ((flags & PSITEM_FLAG_IDENTIFIABLE)? true : false);
    }
    /// Set the Identifiable flag of the item
    void SetIsIdentifiable(bool v);


private:
    /// Static reference to the pool for all psItem objects
    static PoolAllocator<psItem> itempool;

    gemItem* gItem;

    bool pendingsave;

    float rarity;

    /** Calculates the rarity of an item based on its modifiers
     */
    float CalculateItemRarity();

protected:
    bool loaded;

#if SAVE_TRACER
private:
    csString last_save_queued_from;
#endif

};


class psScheduledItem
{
public:
    psScheduledItem(int spawnID,uint32 itemID,csVector3 &position, psSectorInfo* sector,InstanceID instance, int interval,int maxrnd,
                    float range, int lock_str = 0, int lock_skill = -1, csString flags = "");

    psItem* CreateItem();
    uint32 GetItemID()
    {
        return itemID;
    }

    psSectorInfo* GetSector()
    {
        return sector;
    }
    csVector3 &GetPosition()
    {
        return pos;
    }
    int MakeInterval(); ///< This will return a randomized interval
    int GetInterval()
    {
        return interval;
    }
    int GetMaxModifier()
    {
        return maxrnd;
    }

    /// Gets the lock strength which will be associated to this item.
    int GetLockStrength()
    {
        return lock_str;
    }

    /// Gets the lock skill which will be associated to this item.
    int GetLockSkill()
    {
        return lock_skill;
    }

    /// Gets the flags which will be associated to this item.
    csString GetFlags()
    {
        return flags;
    }

    void UpdatePosition(csVector3 &positon, const char* sector);
    void ChangeIntervals(int newint, int newrand);
    void ChangeRange(float newRange);
    void ChangeAmount(int newAmount);
    void Remove(); ///< Deletes from the DB and everything

    bool WantToDie()
    {
        return wantToDie;
    }

private:
    bool wantToDie;
    int spawnID;           ///< database id
    uint32 itemID;         ///< Item
    csVector3 pos;         ///< Position
    psSectorInfo* sector;  ///< Sector
    InstanceID worldInstance;     ///< Instance ID to spawn in
    int interval;          ///< Interval in msecs
    int maxrnd;            ///< Maximum random interval modifier in msecs
    float range;           ///< Range in which to spawn item
    int lock_str;          ///< The lock strength, if any, else 0.
    int lock_skill;        ///< The lock skill, if any, else 0.
    csString flags;        ///< The flags to apply to this item.
};

/** @} */

#endif
