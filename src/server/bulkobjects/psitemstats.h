/*
 * psitemstats.h
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

#ifndef __PSITEMSTATS_H__
#define __PSITEMSTATS_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>
#include <csutil/set.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"
#include "util/slots.h"
#include "util/psxmlparser.h"
#include "util/scriptvar.h"
#include "util/psconst.h"

#include "rpgrules/psmoney.h"

#include <idal.h>

//=============================================================================
// Local Includes
//=============================================================================
#include "psskills.h"

class ApplicativeScript;
class CacheManager;
class psCharacter;
class psItem;
struct psItemCategory;

enum PSITEMSTATS_WEAPONSKILL_INDEX
{
    PSITEMSTATS_WEAPONSKILL_INDEX_0 = 0,
    PSITEMSTATS_WEAPONSKILL_INDEX_1,
    PSITEMSTATS_WEAPONSKILL_INDEX_2,
    PSITEMSTATS_WEAPONSKILL_INDEX_COUNT
};

enum PSITEMSTATS_ARMORTYPE
{
    PSITEMSTATS_ARMORTYPE_NONE = -1,
    PSITEMSTATS_ARMORTYPE_LIGHT,
    PSITEMSTATS_ARMORTYPE_MEDIUM,
    PSITEMSTATS_ARMORTYPE_HEAVY,
    PSITEMSTATS_ARMORTYPE_COUNT
};

enum PSITEMSTATS_STAT
{
    PSITEMSTATS_STAT_NONE = -1,
    PSITEMSTATS_STAT_STRENGTH,
    PSITEMSTATS_STAT_AGILITY,
    PSITEMSTATS_STAT_ENDURANCE,
    PSITEMSTATS_STAT_INTELLIGENCE,
    PSITEMSTATS_STAT_WILL,
    PSITEMSTATS_STAT_CHARISMA,
    PSITEMSTATS_STAT_CONSTITUTION,
    PSITEMSTATS_STAT_STAMINA,
    PSITEMSTATS_STAT_COUNT
};

//TODO these should supposedly go away...

/// Convenience function to convert between STAT and SKILL
PSSKILL statToSkill(PSITEMSTATS_STAT stat);


enum PSITEMSTATS_DAMAGETYPE
{
    PSITEMSTATS_DAMAGETYPE_NONE  = -1,
    PSITEMSTATS_DAMAGETYPE_SLASH,
    PSITEMSTATS_DAMAGETYPE_BLUNT,
    PSITEMSTATS_DAMAGETYPE_PIERCE,
    //PSITEMSTATS_DAMAGETYPE_FORCE,
    //PSITEMSTATS_DAMAGETYPE_FIRE,
    //PSITEMSTATS_DAMAGETYPE_ICE,
    //PSITEMSTATS_DAMAGETYPE_AIR,
    //PSITEMSTATS_DAMAGETYPE_POISON,
    //PSITEMSTATS_DAMAGETYPE_DISEASE,
    //PSITEMSTATS_DAMAGETYPE_HOLY,
    //PSITEMSTATS_DAMAGETYPE_UNHOLY,
    PSITEMSTATS_DAMAGETYPE_COUNT
};

enum PSITEMSTATS_AMMOTYPE
{
    PSITEMSTATS_AMMOTYPE_NONE = -1,
    PSITEMSTATS_AMMOTYPE_ARROWS,
    PSITEMSTATS_AMMOTYPE_BOLTS,
    PSITEMSTATS_AMMOTYPE_ROCKS,
    PSITEMSTATS_AMMOTYPE_COUNT
};

enum PSITEMSTATS_CREATIVETYPE
{
    PSITEMSTATS_CREATIVETYPE_NONE = -1,
    PSITEMSTATS_CREATIVETYPE_LITERATURE,
    PSITEMSTATS_CREATIVETYPE_SKETCH,
    PSITEMSTATS_CREATIVETYPE_MUSIC
};

enum PSITEMSTATS_CREATORSTATUS
{
    PSITEMSTATS_CREATOR_VALID = 0,
    PSITEMSTATS_CREATOR_PUBLIC,
    PSITEMSTATS_CREATOR_UNASSIGNED,
    PSITEMSTATS_CREATOR_UNKNOWN          // e.g. creator player deleted
};

/******************************************************/
/* Slots are treated as boolean flag-like values here */
/******************************************************/
#define PSITEMSTATS_SLOT_BULK        0x00000001
#define PSITEMSTATS_SLOT_RIGHTHAND   0x00000002
#define PSITEMSTATS_SLOT_LEFTHAND    0x00000004
#define PSITEMSTATS_SLOT_BOTHHANDS   0x00000008
#define PSITEMSTATS_SLOT_RIGHTFINGER 0x00000010
#define PSITEMSTATS_SLOT_LEFTFINGER  0x00000020
#define PSITEMSTATS_SLOT_HELM        0x00000040
#define PSITEMSTATS_SLOT_NECK        0x00000080
#define PSITEMSTATS_SLOT_BACK        0x00000100
#define PSITEMSTATS_SLOT_ARMS        0x00000200
#define PSITEMSTATS_SLOT_GLOVES      0x00000400
#define PSITEMSTATS_SLOT_BOOTS       0x00000800
#define PSITEMSTATS_SLOT_LEGS        0x00001000
#define PSITEMSTATS_SLOT_BELT        0x00002000
#define PSITEMSTATS_SLOT_BRACERS     0x00004000
#define PSITEMSTATS_SLOT_TORSO       0x00008000
#define PSITEMSTATS_SLOT_MIND        0x00010000
#define PSITEMSTATS_SLOT_BANK        0x00020000
#define PSITEMSTATS_SLOT_CRYSTAL     0x00040000
#define PSITEMSTATS_SLOT_AZURE       0x00080000
#define PSITEMSTATS_SLOT_RED         0x00100000
#define PSITEMSTATS_SLOT_DARK        0x00200000
#define PSITEMSTATS_SLOT_BROWN       0x00400000
#define PSITEMSTATS_SLOT_BLUE        0x00800000

typedef unsigned int PSITEMSTATS_SLOTLIST;

#define PSITEMSTATS_FLAG_IS_A_MELEE_WEAPON   0x00000001
#define PSITEMSTATS_FLAG_IS_A_RANGED_WEAPON  0x00000002
#define PSITEMSTATS_FLAG_IS_A_SHIELD         0x00000004
#define PSITEMSTATS_FLAG_IS_AMMO             0x00000008
#define PSITEMSTATS_FLAG_IS_A_CONTAINER      0x00000010
#define PSITEMSTATS_FLAG_USES_AMMO           0x00000020
#define PSITEMSTATS_FLAG_IS_STACKABLE        0x00000040
#define PSITEMSTATS_FLAG_IS_GLYPH            0x00000080
#define PSITEMSTATS_FLAG_CAN_TRANSFORM       0x00000100
#define PSITEMSTATS_FLAG_TRIA                0x00000200
#define PSITEMSTATS_FLAG_HEXA                0x00000400
#define PSITEMSTATS_FLAG_OCTA                0x00000800
#define PSITEMSTATS_FLAG_CIRCLE              0x00001000
#define PSITEMSTATS_FLAG_CONSUMABLE          0x00002000
#define PSITEMSTATS_FLAG_IS_READABLE         0x00004000
#define PSITEMSTATS_FLAG_IS_ARMOR            0x00008000
#define PSITEMSTATS_FLAG_IS_EQUIP_STACKABLE  0x00010000
#define PSITEMSTATS_FLAG_IS_WRITEABLE        0x00020000
#define PSITEMSTATS_FLAG_NOPICKUP            0x00040000
#define PSITEMSTATS_FLAG_AVERAGEQUALITY      0x00080000     ///< Flag if the item can stack and average out quality.
#define PSITEMSTATS_FLAG_CREATIVE            0x00100000     ///< A creative thing, eg book, sketch
#define PSITEMSTATS_FLAG_BUY_PERSONALISE     0x00200000     ///< duplicate & personalise at purchase
#define PSITEMSTATS_FLAG_IS_RECHARGEABLE     0x00400000
#define PSITEMSTATS_FLAG_IS_A_TRAP           0x00800000
#define PSITEMSTATS_FLAG_IS_CONSTRUCTIBLE    0x01000000

#define CREATIVEDEF_MAX  65535  // Max length for 'text' field in MySQL db.

typedef unsigned int PSITEMSTATS_FLAGS;

struct st_attribute_bonus
{
    PSITEMSTATS_STAT attribute_id;
    short bonus_max;
};

#define PSITEMSTATS_STAT_BONUS_COUNT 3

/**
 * This is a struct used by item stats to say that a person
 * must have a certain level at a certain skill to use the
 * weapon effectively.
 */
struct ItemRequirement
{
    csString name;
    float    min_value;
};

struct psWeaponType
{
    unsigned int id;
    csString name;
    PSSKILL  skill;    // a skill that it may be effected by
};

/**
 * Each weapon specifies what anims can be used with it.  This is for
 * cosmetic purposes only on the client and is not used in calculations.
 */
struct psItemAnimation
{
    int id;
    csString anim_name;
    csStringID anim_id;
    int      min_level_required;
    int      flags;
};


//-----------------------------------------------------------------------------

/**
 * This class stores an items various armour related information.
 */
class psItemArmorStats
{
public:
    friend class psItemStats;
    psItemArmorStats();

    /** Read the armour related details from the database. */
    void ReadStats(iResultRow &row);

    PSITEMSTATS_ARMORTYPE Type()
    {
        return armor_type;
    }
    char Class()
    {
        return armor_class;
    }
    float Protection(PSITEMSTATS_DAMAGETYPE dmgtype);
    float Hardness()
    {
        return hardness;
    }

private:
    PSITEMSTATS_ARMORTYPE armor_type; ///< See the  PSITEMSTATS_ARMORTYPE enum for the list
    char armor_class;
    float damage_protection[PSITEMSTATS_DAMAGETYPE_COUNT];  ///< See the PSITEMSTATS_DAMAGETYPE for list
    float hardness;
};

/**
 * This class holds the various cached database information relating
 * to the weapon skills of a an item_stat.
 */
class psItemWeaponStats
{

public:
    friend class psItemStats;
    psItemWeaponStats();

    /**
     * Read the weapon related details from the database.
     */
    void ReadStats(iResultRow &row);

    psWeaponType *Type() { return weapon_type; }

    PSSKILL Skill(PSITEMSTATS_WEAPONSKILL_INDEX index);

    float Latency()
    {
        return latency;
    }
    float Damage(PSITEMSTATS_DAMAGETYPE dmgtype);
    float ExtraDamagePercent(PSITEMSTATS_DAMAGETYPE dmgtype);

    float Penetration()
    {
        return penetration;
    }
    float UntargetedBlockValue()
    {
        return untargeted_block_value;
    }
    float TargetedBlockValue()
    {
        return targeted_block_value;
    }
    float CounterBlockValue()
    {
        return counter_block_value;
    }

    PSITEMSTATS_STAT AttributeBonusType(int index);
    float AttributeBonusMax(int index);

private:
    psWeaponType *weapon_type;
    PSSKILL weapon_skill[PSITEMSTATS_WEAPONSKILL_INDEX_COUNT];
    st_attribute_bonus attribute_bonuses[PSITEMSTATS_STAT_BONUS_COUNT];
    float latency;
    float damages[PSITEMSTATS_DAMAGETYPE_COUNT];  // 4.3 precision
    float penetration;
    float untargeted_block_value;
    float targeted_block_value;
    float counter_block_value;
};

/**
 * This little class holds info about Ammunition for Ranged Weapons.
 */
class psItemAmmoStats
{
public:
    psItemAmmoStats();
    ~psItemAmmoStats();

    /**
     * Read the Ammo related details from the database.
     */
    void ReadStats(iResultRow &row);

    PSITEMSTATS_AMMOTYPE AmmoType()
    {
        return ammunition_type;
    }
private:
    PSITEMSTATS_AMMOTYPE ammunition_type;
};

/**
 * This class holds info about Creative items such as books, etc.
 */
class psItemCreativeStats
{
public:

    //Both psitemstats and psitem make use of this structure. So they need to access
    //all it's elements
    friend class psItemStats;
    friend class psItem;

    ///constructor
    psItemCreativeStats();
    ///destructor
    ~psItemCreativeStats();

    /**
     * Checks if the passed PID is the creator of this creative.
     *
     * @param characterID The PID of the player we are checking if it's the creator.
     * @return TRUE if the passed PID corresponds to the creator of this creative item.
     */
    bool IsThisTheCreator(PID characterID);

    /**
     * Sets the player passed as the creator of this creative, in case someone doesn't own creation status
     * already, in that case it will be ignored.
     *
     * @param characterID The PID of the character who will be the creator of this creative defintion.
     * @param creatorStatus A status from PSITEMSTATS_CREATORSTATUS to which setting the creative.
     */
    void SetCreator(PID characterID, PSITEMSTATS_CREATORSTATUS creatorStatus);

    /**
     * Gets the creator and the status of the creator of this creative.
     *
     * @param creatorStatus A reference to a PSITEMSTATS_CREATORSTATUS which will be filled with
     *                      the informations about the status of the creator.
     * @return PID The pid of the character who created this creative in case it's valid, else will
     *             return 0 as PID.
     */
    PID GetCreator(PSITEMSTATS_CREATORSTATUS &creatorStatus);

private:
    /**
     * Read the Creative data from the database.
     *
     * @param row The database row as loaded from the database: must have at least a creative_definition column.
     */
    void ReadStats(iResultRow &row);

    /**
     * Read the Creative data from the internal creativeDefinitionXML, populating all the fields from it.
     */
    void ReadStats();

    /**
     * general write creative content.
     */
    bool SetCreativeContent(PSITEMSTATS_CREATIVETYPE, const csString &, uint32);

    /**
     * Format content for database.
     */
    bool FormatCreativeContent(void);

    /**
     * Save creation in database.
     */
    void SaveCreation(uint32);

    /**
     * Update the description of the item.
     */
    csString UpdateDescription(PSITEMSTATS_CREATIVETYPE, csString, csString);

    /**
     * Used to set if this creativedefinition has to operate on item_instances in place of item_stats when
     * saving data on the database. Pass TRUE if you wish that this creative definition saves data in the
     * instances in place of the stats.
     *
     * @param value TRUE to make this creative definition operate on item_instances, otherwise it will operate
     *              on item_stats.
     */
    void setInstanceBased(bool value)
    {
        instanceBased = value;
    }

    PSITEMSTATS_CREATIVETYPE creativeType;

    psXMLString creativeDefinitionXML;
    csString content;
    csString backgroundImg; ///< The name of the image which will be put as background of this creative.

    PID creatorID;
    PSITEMSTATS_CREATORSTATUS creatorIDStatus;

    /**
     * Indicates that when we save on the database we have to operate on item_instances, in place
     * of item_stats.
     */
    bool instanceBased;

};

//-----------------------------------------------------------------------------

/**
 * This huge class stores all the properties of any object
 * a player can have in the game.  All stats, bonuses, maluses,
 * magic stat alterations, combat properties, spell effects,
 * and so forth are all stored here.
 */
class psItemStats : public iScriptableVar
{
public:
    psItemStats();
    ~psItemStats();

private:
    uint32       uid;
    csString     uid_str;
    csString     name;
    csString     description;
    /// Underlying quality of item.  This is the maximum quality any one instance of this stat can be.
    float        item_quality;
    /// String defining any Sketch object
    csString     sketch_def;

    psItemArmorStats armorStats;
    psItemWeaponStats weaponStats;
    psItemAmmoStats ammoStats;
    psItemCreativeStats creativeStats;

    float weight;

    /**
     * Size of the longest dimension of this object, in cm
     * This is used to determine wether an item can fit inside of a container.
     */
    float size;

    /**
     * Size of the longest dimension of the longest item this container can hold.
     * This is used to determine wether an item can fit inside of a container.
     * Only valid if this is a container.
     */
    unsigned short container_max_size;

    /**
     * Maximum amount of slots available for this container. This is used
     * to decide how many slots to show client side to store items.
     */
    int container_max_slots;

    /// How much is the default decay to item_quality per use
    float decay_rate;

    /// Scripts The name of a progression script that this event can trigger on equip, unequip, or consuming.
    ApplicativeScript* equipScript;
    csString consumeScriptName;

    PSITEMSTATS_SLOTLIST valid_slots;
    csArray<INVENTORY_SLOT_NUMBER> valid_slots_array;
    PSITEMSTATS_FLAGS flags;

    /**
     * The visible distance is used to determine the range at which this item becomes visible.
     *
     * (At this time, this means when lying on the ground, but it could just as easily be
     *  used for equipped objects)
     * This number will not always be a hard and fast rule.  Other numbers such as situation
     * visibility modifiers and character perception enhancements may be combined with this
     * to determine wether an object is ultimately visible.
     *
     * At this time the default proximity distance is 100.0  (DEF_PROX_DIST in psconst.h), and
     * visible distances greater than this value will have no effect.
     */
    float visible_distance;

    csSet<unsigned int> ammo_types;

    csString stat_type;

    csArray<ItemRequirement> reqs;
    int spell_id_on_hit;
    float spell_on_hit_probability;
    int spell_id_feature;
    int spell_feature_charges;
    int spell_feature_timing;

    /// The maximum number of charges an item of this type
    /// can ever hold. -1 Indicate that this is an item without
    /// the possiblity to be charged.
    int maxCharges;

    /// Tracks if this item can be consumed, can be potions/scrolls/etc
    bool isConsumable;

    /// Declares if the item can be spawned
    bool spawnable;

    /// Animation eye candy things
    csPDelArray<psItemAnimation>* anim_list;

    /// These temporary fields point to strings used for the mesh, texture, part and image names respectively
    csString mesh_name;

    /// Stores a texture change
    csString texture_name;

    /// Stores the mesh that the above texture should go on.
    csString texturepart_name;

    /// Stores the new mesh to be attached on a mesh change.
    csString partmesh_name;

    csString image_name;

    /* mesh names, texture names, texture part names, and image names are currently stored as strings
     * on the server and passed over to the client in normal communication.  This is inefficient, and
     * ultimately the client should use a numeric index of these resources, and the server should not
     * need to be aware of the resource name at all.
     *
     * Higher level design tools should be aware of the mapping between resource name and index, and
     * should be able to adjust both the server database references to the index as well as the client
     * index->resource mapping.
     */
    /*
    unsigned int mesh_index;
    unsigned int texture_index;
    unsigned int texturepart_index;
    unsigned int image_index;
    */


    psMoney price;
    psItemCategory* category;

    csString sound;
    csString weapon_type;

    csString itemCommand; ///< contains if the item has a special command where it's used.

    csHash<csHash<csString,int> > meshRemovalInfo;

    /**
     * Loads in the the slot based removal informations.
     *
     * @param row The database row to load information from.
     */
    void LoadMeshRemoval(iResultRow &row);

    /**
     * Loads in the valid slots from the database for this particular item.
     *
     * @param row The database row to load information from.
     */
    void LoadSlots(iResultRow &row);

    /**
     * Parses the flag information and sets the internal flags.
     *
     * @param row The database row to load information from.
     */
    void ParseFlags(iResultRow &row);

public:
    psItemArmorStats &Armor()
    {
        return armorStats;
    }
    psItemWeaponStats &Weapon()
    {
        return weaponStats;
    }
    psItemAmmoStats &Ammunition()
    {
        return ammoStats;
    }

    bool ReadItemStats(iResultRow &row);
    PSITEMSTATS_FLAGS GetFlags()
    {
        return flags;
    }
    // Interface
    bool GetIsMeleeWeapon();
    bool GetIsArmor();
    bool GetIsRangeWeapon();
    bool GetIsAmmo();
    bool GetIsShield();
    bool GetIsContainer();
    bool GetIsTrap();
    bool GetIsConstructible();
    bool GetCanTransform();
    bool GetUnmovable();
    bool GetUsesAmmo();
    bool GetIsStackable();
    bool GetIsEquipStackable();
    bool GetIsGlyph();
    bool GetIsConsumable();
    bool GetIsReadable();
    bool GetIsWriteable();

    /**
     * Gets if the item should be allowed to be spawned.
     *
     * @return BOOL saying if the item can be spawned from the item command.
     */
    bool IsSpawnable();
    PSITEMSTATS_CREATIVETYPE GetCreative();
    bool GetBuyPersonalise();
    float GetRange() const;
    PID GetCreator(PSITEMSTATS_CREATORSTATUS &creatorStatus);
    csSet<unsigned int> GetAmmoTypeID() const
    {
        return ammo_types;
    }

    float weaponRange;

    /**
     * @return True if the object is a money object.
     */
    bool IsMoney();

    uint32 GetUID();
    const char* GetUIDStr();
    const char* GetDescription() const;
    void SetDescription(const char* v);
    void SaveDescription(void);
    PSITEMSTATS_AMMOTYPE GetAmmoType();
    float GetQuality() const
    {
        return item_quality;
    }
    void SetQuality(float f)
    {
        item_quality = f;
    }

    void GetArmorVsWeaponType(csString &buff);
    void SetArmorVsWeaponType(const char* v);

    float GetWeight();
    float GetSize();
    unsigned short GetContainerMaxSize();

    /**
     * Gets the slots available in this item (only containers) which means
     * also the maximum amount of items which can be stored in this container.
     *
     *  @return The number of slots available in this container.
     */
    int GetContainerMaxSlots();
    float GetVisibleDistance();
    PSITEMSTATS_SLOTLIST GetValidSlots();
    bool FitsInSlots(PSITEMSTATS_SLOTLIST slotmask);

    /**
     * Gets the list of mesh to remove when this item is equipped in the specified slot.
     *
     * If the race specified is not found the race -1 (aka no race specific) will be searched too.
     * @param slot The slot this item is being equipped into.
     * @param meshName The meshName we are sarching for the slot to remove for it.
     * @return The list of meshes to remove when equipping this item.
     */
    csString GetSlotRemovedMesh(int slot, csString meshName = "");
    float GetDecayRate();

    int GetAttackAnimID(unsigned int skill_level);

    csString FlagsToText();


    const char* GetName() const;
    const csString GetDownCaseName();
    void SetUnique();
    bool GetUnique();
    void SetRandom();
    bool GetRandom();

    /**
     * Used to get the creative definition so psitem can make it's own copy of it.
     *
     * @return psXMLString The creative definition xml data of this item.
     */
    psXMLString getCreativeXML()
    {
        return creativeStats.creativeDefinitionXML;
    }
    void SetName(const char* v);
    void SaveName(void);

    /**
     * Get the Mesh Name for the item.
     *
     * Used for standalone or weilded mesh.
     */
    const char* GetMeshName();

    void SetMeshName(const char* v);

    /**
     * Get the Texture Name for the item.
     *
     * Used when worn and attached to the mesh given by part name.
     */
    const char* GetTextureName();

    void SetTextureName(const char* v);

    /**
     * Get the Part Name for the item.
     *
     * This is the name of the part that the texture should be attached to
     * if no change of mesh.
     */
    const char* GetPartName();

    void SetPartName(const char* v);

    /**
     * Get the Part Mesh Name for the item.
     *
     * This is the new mesh to be attached to the location given
     * by the pattern Part Name.
     */
    const char* GetPartMeshName();

    void SetPartMeshName(const char* v);

    /**
     * Get the Image Name for the item.
     *
     * Used in inventory and other location where item has to be presented
     * by a 2D image.
     */
    const char* GetImageName();

    void SetImageName(const char* v);

    // For future use, see notes for mesh_index variables, etc.
    /*
    unsigned int GetMeshIndex();
    void SetMeshIndex(unsigned int v);
    unsigned int GetTextureIndex();
    void SetTextureIndex(unsigned int v);
    unsigned int GetTexturePartIndex();
    void SetTexturePartIndex(unsigned int v);
    unsigned int GetImageIndex();
    void SetImageIndex(unsigned int v);
    */

    void SetPrice(int trias);
    psMoney &GetPrice();
    void SetCategory(psItemCategory* category);
    psItemCategory* GetCategory();
    csArray<INVENTORY_SLOT_NUMBER> GetSlots()
    {
        return valid_slots_array;
    }

    //
    const char* GetSound();
    void SetSound(const char* v);

    ApplicativeScript* GetEquipScript()
    {
        return equipScript;
    }
    const csString &GetConsumeScriptName()
    {
        return consumeScriptName;
    }

    /**
     * Sets a new equip script and saves it to the DB.  Mostly for the loot generator.
     */
    bool SetEquipScript(const csString &script);

    /**
     * Gets the list of requirements of this item, used by psitem to check them over a player.
     *
     * @return A pointer to the start of the array of itemRequirement (they are 3 elements)
     */
    csArray<ItemRequirement> &GetRequirements();
    bool SetRequirement(const csString &statName, float statValue);

    /**
     *  Called to create an instance of an item using basic item stats.
     *
     *  The item instance can be modified by the caller after it's created (modifiers, uniqueness, etc)
     *  You MUST call Loaded() on the returned item once it is in a ready state, to allow saving.
     */
    psItem* InstantiateBasicItem(bool transient=false);

    bool Save();
    bool SetAttribute(const csString &op, const csString &attrName, float modifier);

    /// return creative contents
    const csString &GetMusicalSheet(void)
    {
        return creativeStats.content;
    }
    const csString &GetSketch(void)
    {
        return creativeStats.content;
    }
    const csString &GetLiteratureText(void)
    {
        return creativeStats.content;
    }

    /**
     * Write creative stuff such as lit text (eg book) or map data.
     */
    bool SetCreation(PSITEMSTATS_CREATIVETYPE, const csString &, csString);

    /**
     * creator (i.e. author, artist, etc) of creative things.
     */
    void SetCreator(PID, PSITEMSTATS_CREATORSTATUS);
    bool IsThisTheCreator(PID);

    /**
     * Returns the background image assigned to the creative item.
     *
     * @return A csString containing the name of the background image assigned to the creative item.
     */
    const csString &GetCreativeBackgroundImg();

    /// Return true if this item was equipted with charges.
    bool HasCharges() const;
    bool IsRechargeable() const;
    void SetMaxCharges(int charges);
    int GetMaxCharges() const;

    ///returns the special command assigned to this item
    csString GetItemCommand()
    {
        return itemCommand;
    }

    /**
     * Returns the name of the item.
     *
     * @note Needed for iScriptableVar.
     * @return the name of the item.
     */
    const char* ToString()
    {
        return name.GetDataSafe();
    }

    /**
     * Needed for iScriptableVar.
     *
     * Does nothing right now just returns 0 for anything passed.
     */
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);

    /**
     * Returns the requested variable stored in this item stats.
     *
     * @note Needed for iScriptableVar.
     *
     * @param env A math environment.
     * @param ptr A pointer to a char array stating the requested variable.
     * @return A double with the value of the requested variable.
     */
    double GetProperty(MathEnvironment* env, const char* ptr);


public:

    ///  The new operator is overriden to call PoolAllocator template functions
    void* operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void*);

private:
    /// Static reference to the pool for all psItem objects
    static PoolAllocator<psItemStats> itempool;
};

/** @} */

#endif

