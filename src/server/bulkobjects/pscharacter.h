/*
 * pscharacter.h
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

#ifndef __PSCHARACTER_H__
#define __PSCHARACTER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"
#include "util/psconst.h"
#include "util/scriptvar.h"
#include "util/skillcache.h"
#include "util/slots.h"

#include "net/charmessages.h"

#include "../icachedobject.h"
#include "../playergroup.h"
#include "rpgrules/factions.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "buffable.h"
#include "psskills.h"
#include "psstdint.h"
#include "pscharinventory.h"
#include "psinventorycachesvr.h"
#include "psitemstats.h"
#include "servervitals.h"
#include "psguildinfo.h"
#include "psattackqueue.h"
#include "pscharquestmgr.h"

class psServerVitals;
class MsgEntry;
class psItemStats;
class psItem;
class psQuest;

struct psRaceInfo;
class psSectorInfo;

class ExchangeManager;
class MathScript;
class NpcResponse;
class gemActor;
struct psGuildLevel;
class psGuildMember;
class psLootMessage;
class psMerchantInfo;
class psQuestListMessage;
class psSpell;
class psTradeTransformations;
class psTradeProcesses;
class psTrainerInfo;
struct psTrait;
class psWorkGameEvent;

struct Result;
struct Faction;

/**
 * \addtogroup bulkobjects
 * @{ */

/** "Normalizes" name of character i.e. makes the first letter uppercase and all the rest downcase */
csString NormalizeCharacterName(const csString &name);

////////////////////////////////////////////////////////////////////////////////

enum PSCHARACTER_TYPE
{
    PSCHARACTER_TYPE_PLAYER    = 0,
    PSCHARACTER_TYPE_NPC       = 1,
    PSCHARACTER_TYPE_PET       = 2,
    PSCHARACTER_TYPE_MOUNT     = 3,
    PSCHARACTER_TYPE_MOUNTPET  = 4,
    PSCHARACTER_TYPE_COUNT     = 5,
    PSCHARACTER_TYPE_UNKNOWN   = ~0
} ;

#define PSCHARACTER_BULK_COUNT INVENTORY_BULK_COUNT
#define PSCHARACTER_BANK_BULK_COUNT 16

/**
 * Base class for several other classes which hold character attributes of different sorts.
 */
class CharacterAttribute
{
protected:
    psCharacter* self;
public:
    CharacterAttribute(psCharacter* character) : self(character) { }
};

// Remember to update the translation table in GetModeStr when adding modes.
enum PSCHARACTER_MODE
{
    PSCHARACTER_MODE_UNKNOWN = 0,
    PSCHARACTER_MODE_PEACE,
    PSCHARACTER_MODE_COMBAT,
    PSCHARACTER_MODE_SPELL_CASTING,
    PSCHARACTER_MODE_WORK,
    PSCHARACTER_MODE_DEAD,
    PSCHARACTER_MODE_SIT,
    PSCHARACTER_MODE_OVERWEIGHT,
    PSCHARACTER_MODE_EXHAUSTED,
    PSCHARACTER_MODE_DEFEATED,
    PSCHARACTER_MODE_STATUE,
    PSCHARACTER_MODE_PLAY,
    PSCHARACTER_MODE_COUNT
};

enum PSCHARACTER_CUSTOM
{
    PSCHARACTER_CUSTOM_EYES = 0,
    PSCHARACTER_CUSTOM_HAIR,
    PSCHARACTER_CUSTOM_BEARD,
    PSCHARACTER_CUSTOM_COLOUR,
    PSCHARACTER_CUSTOM_SKIN,
    PSCHARACTER_CUSTOM_COUNT
};

///enum containing the notification status (a bitfield)
enum PSCHARACTER_JOINNOTIFICATION
{
    PSCHARACTER_JOINNOTIFICATION_GUILD    = 1,
    PSCHARACTER_JOINNOTIFICATION_ALLIANCE = 2
};



/**
 * Structure for assigned GM Events.
 */
struct GMEventsAssignment
{
    /// GM controlling running GM Event.
    int runningEventIDAsGM;
    /// Player Running GM Event.
    int runningEventID;
    /// completed GM events as the GM.
    csArray<int> completedEventIDsAsGM;
    /// completed GM events
    csArray<int> completedEventIDs;
};

/**
 * This enumeration and structure tracks
 * the players trade skill efforts
 */
enum PSCHARACTER_WORKSTATE
{
    PSCHARACTER_WORKSTATE_HALTED = 0,
    PSCHARACTER_WORKSTATE_STARTED,
    PSCHARACTER_WORKSTATE_COMPLETE,
    PSCHARACTER_WORKSTATE_OVERDONE
};


/// Set if this slot should continuously attack while in combat
#define PSCHARACTER_EQUIPMENTFLAG_AUTOATTACK       0x00000001
/// Set if this slot can attack when the client specifically requests (and only when the client specifically requests)
#define PSCHARACTER_EQUIPMENTFLAG_SINGLEATTACK     0x00000002
/// Set if this slot can attack even when empty - requires that a default psItem be set in default_if_empty
#define PSCHARACTER_EQUIPMENTFLAG_ATTACKIFEMPTY    0x00000004

//-----------------------------------------------------------------------------

/**
 * Class to handle buddies.
 */
class psBuddyManager
{
public:
    struct Buddy
    {
        csString name;      ///< Character name of buddy.
        PID playerId;       ///< The PID of the player that is the buddy.
    };

    /**
     * Setup the buddy manager.
     *
     *  @param charID The PID of the owner of this buddy manager.
     */
    void Initialize(PID charID)
    {
        characterId = charID;
    }

    /**
     * Add the player with a certain Player ID to this character buddy list.
     *
     * @param buddyID the Player ID which we are going to add to the character buddy list
     * @param name The name of the buddy.
     */
    bool AddBuddy(PID buddyID, csString &name);

    /**
     * Remove the player with a certain Player ID from this character buddy list.
     *
     * @param buddyID the Player ID which we are going to remove from the character buddy list
     */
    void RemoveBuddy(PID buddyID);

    /**
     * Checks if a playerID is a buddy of this character.
     *
     * @param buddyID the Player ID of which we are checking the presence in the character buddy list
     * @return  true if the provided PID was found in this character.
     */
    bool IsBuddy(PID buddyID);

    /**
     * Adds this player as having this character on their buddy list.
     *
     * @param buddyID the Player ID of the character that has this character as a buddy.
     */
    void AddBuddyOf(PID buddyID);

    /**
     * Remove character as having this character on their buddy list.
     *
     * @param buddyID the Player ID of the character that has removed this character as a buddy.
     */
    void RemoveBuddyOf(PID buddyID);

    /**
     * Load the set of my buddies as well as those that have me as a buddy.
     *
     * @param myBuddies The database set of characters that are my buddy.
     * @param buddyOf   The database set of characters that have me as a buddy.
     */
    bool LoadBuddies(Result &myBuddies, Result &buddyOf);


    csArray<psBuddyManager::Buddy>  GetBuddyList()
    {
        return buddyList;
    }
    csArray<PID>                    GetBuddyOfList()
    {
        return buddyOfList;
    }

private:

    csArray<psBuddyManager::Buddy> buddyList;       ///< List of my buddies.
    csArray<PID> buddyOfList;       ///< List of people that have me as a buddy.

    PID characterId;                ///< PID of the owner of this buddy system
};


//-----------------------------------------------------------------------------

// Need to recalculate Max HP/Mana/Stamina and inventory limits whenever stats
// change (whether via buffs or base value changes).
class SkillStatBuffable : public ClampedPositiveBuffable<int>
{
public:
    void Initialize(psCharacter* c)
    {
        chr = c;
    }
protected:
    virtual void OnChange();
    psCharacter* chr;
};

// When base stats change, we also need to recalculate skill training costs.
// However, since training ignores buffs, we don't need to for all changes.
class CharStat : public SkillStatBuffable
{
public:
    void SetBase(int x);
};

typedef SkillStatBuffable SkillRank;

class StatSet : public CharacterAttribute
{
public:
    StatSet(psCharacter* self);

    CharStat &Get(PSITEMSTATS_STAT attrib);
    CharStat &operator [](PSITEMSTATS_STAT which)
    {
        return Get(which);
    }

protected:
    CharStat stats[PSITEMSTATS_STAT_COUNT];
};

/**
 * A structure that holds the knowledge/practice/rank of each player skill.
 */
struct Skill
{
    unsigned short z;        ///< Practice value
    unsigned short y;        ///< Knowledge Level
    SkillRank rank;          ///< Skill rank (buffable)

    unsigned short zCost;    ///< Cost in Z points.
    unsigned short yCost;    ///< Cost in y points.
    unsigned short zCostNext;///< Cost in Z points of next level.
    unsigned short yCostNext;///< Cost in y points of next level.
    bool dirtyFlag;          ///< Flag if this was changed after load from database

    psSkillInfo* info;       ///< Database information about the skill.

    Skill()
    {
        Clear();
    }
    void Clear()
    {
        z=y=0;
        zCost=yCost=0;
        info = NULL;
        dirtyFlag = false;
    }

    void CalculateCosts(psCharacter* user);

    /** Checks to see if this skill can be trained any more at the current rank.
     */
    bool CanTrain()
    {
        return y < yCost;
    }

    /**
     * Train a skill by a particular amount.
     *
     * This does range checking on the training level and will cap it at the
     * max allowable level.
     *
     * @param yIncrease The amount to try to increase the skill by.
     */
    void Train(int yIncrease);

    /**
     * Check if skill will rank and rank it up.
     *
     * This checks a couple of things.
     * 1) If the player has the required knowledge to allow for training.
     * 2) If the amount of practice causes a rank change it will increase
     *    the rank of the skill and reset the knowledge/practice levels.
     *
     * @param user The character this was for.
     *
     * @return True if the practice causes a rank change, false if not.
     */
    bool CheckDoRank(psCharacter* user);

    /**
     * Practice this skill.
     *
     * This checks a couple of things.
     * 1) If the player has the required knowledge to allow for training.
     * 2) If the amount of practice causes a rank change it will increase
     *    the rank of the skill and reset the knowledge/practice levels.
     *
     * @param amount The amount of practice on this skill.
     * @param actuallyAdded [CHANGES] If the amount added causes a rank change
     *                       only the amount required is added and this variable
     *                       stores that.
     * @param user The character this was for.
     *
     * @return True if the practice causes a rank change, false if not.
     */
    bool Practice(unsigned int amount, unsigned int &actuallyAdded, psCharacter* user);
};


/**
 * A list of skills.
 *
 * This maintains a list of all the skills and the player's current levels
 * in them.
 */
class SkillSet : public CharacterAttribute
{
protected:
    csArray<Skill> skills; ///< Array to store all the skills.

public:
    /**
     * Constructor.
     *
     * @param self The psCharacter this skillset is associated with.
     */
    SkillSet(psCharacter* self);

    /**
     * Sets the common skill info for this skill ( data from the database ).
     *
     * @param which  The skill we want to set
     * @param info   The info structure to assign to this skill.
     * @param recalculatestats   if true, stats of player will be recalculated taking into account the new skill
     */
    void SetSkillInfo(PSSKILL which, psSkillInfo* info, bool recalculatestats = true);

    /**
     * Sets the practice level for the skill.
     *
     * Does no error or range checking on the z value.  Simply assigns.
     *
     * @param which The skill we want to set.
     * @param z_value The practice level of that skill.
     */
    void SetSkillPractice(PSSKILL which, int z_value);

    /**
     * Set a skill knowledge level.
     *
     * Sets a skill to a particular knowledge level. This does no checking
     * of allowed values. It just sets it to a particular value.
     *
     * @param which  Skill name. One of the PSSKILL enum values.
     * @param y_value    The value to set this skill knowledge at.
     */
    void SetSkillKnowledge(PSSKILL which, int y_value);

    /**
     * Set a skill rank level.
     *
     * Sets a skill to a particular rank level. This does no checking
     * of allowed values. It just sets it to a particular value.
     *
     * @param which  Skill name. One of the PSSKILL enum values.
     * @param rank    The value to set this skill rank at.
     * @param recalculatestats   if true, stats of player will be recalculated taking into account the new skill rank
     */
    void SetSkillRank(PSSKILL which, unsigned int rank, bool recalculatestats = true);

    /**
     * Update the costs for all the skills.
     */
    void Calculate();

    /**
     * Figure out if this skill can be trained.
     *
     * Checks the current knowledge of the skill. If it is already maxed out then
     * can train no more.
     *
     * @param skill The skill we want to train.
     * @return  True if the skill still requires Y credits before it is fully trained.
     */
    bool CanTrain(PSSKILL skill);

    /**
     * Checks if a skill should rank and ranks it.
     *
     * It checks if practice and knowledge is reached to rank the skill.
     *
     * @param skill The skill we want to check.
     */
    void CheckDoRank(PSSKILL skill);


    /**
     * Trains a skill.
     *
     *  It will only train up to the cost of the next rank. So the yIncrease is
     *  capped by the cost and anything over will be lost.
     *
     *  @param skill The skill we want to train.
     *  @param yIncrease  The amount we want to train this skill by.
     */
    void Train(PSSKILL skill, int yIncrease);


    /**
     *  Get the current rank of a skill.
     *
     *  @param which The skill that we want the rank for.
     *  @return The rank of the requested skill.
     */
    SkillRank &GetSkillRank(PSSKILL which);

    /**
     *  Get the current knowledge level of a skill.
     *
     *  @param skill the enum of the skill that we want.
     *  @return The Y value of that skill.
     */
    unsigned int GetSkillKnowledge(PSSKILL skill);

    /**
     *  Get the current practice level of a skill.
     *
     *  @param skill the enum of the skill that we want.
     *  @return The Z value of that skill.
     */
    unsigned int GetSkillPractice(PSSKILL skill);

    /**
     *  Add some practice to a skill.
     *
     * @param skill The skill we want to practice
     * @param val The amount we want to practice the skill by. This value could
     *            be capped if the amount of practice causes a rank up.
     * @param added [CHANGES] The amount the skill changed by ( if any )
     *
     * @return True if practice caused a rank up.
     */
    bool  AddToSkillPractice(PSSKILL skill, unsigned int val, unsigned int &added);

    /**
     * Add skill practice.
     */
    int AddSkillPractice(psSkillInfo* skill, unsigned int val);

    /**
     * Add skill practice.
     */
    int AddSkillPractice(PSSKILL skill, unsigned int val);

    /**
     *  Get the slot that is the best skill in the set.
     *
     *  @param withBuff   Apply any skill buffs?
     *  @return The slot the best skill is in.
     */
    unsigned int GetBestSkillSlot(bool withBuff);

    Skill &Get(PSSKILL skill);
};

#define ALWAYS_IMPERVIOUS      1
#define TEMPORARILY_IMPERVIOUS 2


//-----------------------------------------------------------------------------

struct Stance
{
    unsigned int stance_id;
    csString stance_name;
    float stamina_drain_P;
    float stamina_drain_M;
    float attack_speed_mod;
    float attack_damage_mod;
    float defense_avoid_mod;
    float defense_absorb_mod;
};

/**
 * This is used to char a charVariable.
 *
 * This is a variable which can be set and then accessed from various places
 * allowing for example scripts or quests to check if they are set and or
 * with what value. The names must be unique (only one per character)
 */
class charVariable
{
public:
    csString name;          ///< The name of the variable.
    csString value;         ///< The value assigned to this variable.
    bool dirty;             ///< Says if the variable was modified.
    Buffable<int> intBuff;  ///< A buffable interpretation of the variable


    Buffable<int> &GetBuffable()
    {
        return intBuff;
    }

    charVariable() : dirty(false) {}
    charVariable(csString name, csString value) : name(name), value(value), dirty(false)
    {
        intBuff.SetBase(strtoul(value.GetDataSafe(),NULL,0));
    }
    charVariable(csString name, csString value, bool dirty) : name(name), value(value), dirty(dirty)
    {
        intBuff.SetBase(strtoul(value.GetDataSafe(),NULL,0));
    }
};

class OverridableRace : public Overridable<psRaceInfo*>
{
public:

    /**
     * Constructor, wants a @see psRaceInfo pointer which should be the default one usually.
     *
     * @param race A pointer to a @see psRaceInfo.
     */
    OverridableRace(psRaceInfo* race) : Overridable<psRaceInfo*>(race) {}

    /**
     * Destructor.
     */
    virtual ~OverridableRace() { }

    /**
     * Sets which character owns this overridableRace.
     *
     * @param psChar A pointer to a pscharacter.
     */
    void SetCharacter(psCharacter* psChar)
    {
        character = psChar;
    }

protected:

    /**
     * Function called when the overridable is changed.
     *
     * It sets the base values of the race and the helm/belt... groups.
     */
    virtual void OnChange();

    psCharacter* character; ///< Pointer to the psCharacter owning this overridable class.
};

//-----------------------------------------------------------------------------

class psCharacter : public iScriptableVar, public iCachedObject
{
public:
    enum TradingStatus
    {
        NOT_TRADING,
        SELLING,
        BUYING,
        WITHDRAWING,
        STORING
    };

    struct st_location
    {
        psSectorInfo* loc_sector;
        csVector3 loc;
        float loc_yrot;
        InstanceID worldInstance;
    };

    psCharacter();

    virtual ~psCharacter();

    bool Load(iResultRow &row);

    bool IsStatue()
    {
        return isStatue;
    }

    psCharacterInventory &Inventory()
    {
        return inventory;
    }

    psMoney Money()
    {
        return money;
    }

    psMoney &BankMoney()
    {
        return bankMoney;
    }

    void SetMoney(psMoney m);

    /**
     *  Add a money object to the current wallet.
     *
     * This is used when money is picked up and will destory the object passed in.
     *
     * @param moneyObject The money object that was just picked up.
     */
    void SetMoney(psItem* &moneyObject);

    /**
     *  Add a certain amount of a money object to the current wallet based on the baseitem data.
     *
     * This is used to give rewards in gmeventmanager for now.
     *
     * @param MoneyObject The money  base object which was rewarded.
     * @param amount The amount of the base object which was rewarded.
     */
    void SetMoney(psItemStats* MoneyObject,  int amount);

    void AdjustMoney(psMoney m, bool bank);

    void SaveMoney(bool bank);

    void ResetStats();


    /// Checks the bit field for a bit flag from the enum in TutorialManager.h
    bool NeedsHelpEvent(int which)
    {
        return (helpEventFlags & (1 << which))==0;
    }

    /// Sets a bit field complete for a specified flag from the enum in tutorialmanager.h
    void CompleteHelpEvent(int which)
    {
        helpEventFlags |= (1 << which);
    }

    /// Set the active guild for the character.
    void SetGuild(psGuildInfo* g)
    {
        guildinfo = g;
    }
    /// Return the active guild, if any for this character.
    psGuildInfo* GetGuild()
    {
        return guildinfo;
    }
    /// Return the guild level for this character, if any.
    psGuildLevel* GetGuildLevel();
    /// Return the guild membership for this character, if any.
    psGuildMember* GetGuildMembership();

    ///Returns if the client should receive notifications about guild members logging in
    bool IsGettingGuildNotifications()
    {
        return (joinNotifications & PSCHARACTER_JOINNOTIFICATION_GUILD);
    }
    ///Returns if the client should receive notifications about alliance members logging in
    bool IsGettingAllianceNotifications()
    {
        return (joinNotifications & PSCHARACTER_JOINNOTIFICATION_ALLIANCE);
    }
    ///Sets if the client should receive notifications about guild members logging in
    void SetGuildNotifications(bool enabled)
    {
        if(enabled) joinNotifications |= PSCHARACTER_JOINNOTIFICATION_GUILD;
        else joinNotifications &= ~PSCHARACTER_JOINNOTIFICATION_GUILD;
    }
    ///Sets if the client should receive notifications about alliance members logging in
    void SetAllianceNotifications(bool enabled)
    {
        if(enabled) joinNotifications |= PSCHARACTER_JOINNOTIFICATION_ALLIANCE;
        else joinNotifications &= ~PSCHARACTER_JOINNOTIFICATION_ALLIANCE;
    }

    ///gets the notification bitfield directly: to be used only by the save functon
    int GetNotifications()
    {
        return joinNotifications;
    }
    ///sets the notification bitfield directly: to be used only by the loader functon
    void SetNotifications(int notifications)
    {
        joinNotifications = notifications;
    }

    SkillSet &Skills()
    {
        return skills;
    }

    /**
     * Returns a pointer to the skill cache for this character
     */
    psSkillCache* GetSkillCache()
    {
        return &skillCache;
    }

    /**
     * Function to get the base and current skill values.
     */
    void GetSkillValues(MathEnvironment* env);
    void GetSkillBaseValues(MathEnvironment* env);

    // iCachedObject Functions below
    virtual void ProcessCacheTimeout() {}  ///< required for iCachedObject but not used here
    virtual void* RecoverObject()
    {
        return this;    ///< Turn iCachedObject ptr into psCharacter
    }
    virtual void DeleteSelf()
    {
        delete this;    ///< Delete must come from inside object to handle operator::delete overrides.
    }



    /// Load the bare minimum to know what this character is looks like
    bool QuickLoad(iResultRow &row, bool noInventory);

    void LoadIntroductions();

    void LoadActiveSpells();
    void AddSpell(psSpell* spell);
    bool Store(const char* location,const char* slot,psItem* what);

    void SetPID(PID characterID)
    {
        pid = characterID;
    }
    PID GetPID() const
    {
        return pid;
    }

    PID GetMasterNPCID() const
    {
        return npcMasterId ? npcMasterId : pid;
    }

    void SetAccount(AccountID id)
    {
        accountid = id;
    }
    AccountID GetAccount() const
    {
        return accountid;
    }

    void SetName(const char* newName)
    {
        SetFullName(newName,lastname.GetData());
    }
    void SetLastName(const char* newLastName)
    {
        SetFullName(name.GetData(),newLastName);
    }
    void SetFullName(const char* newFirstName, const char* newLastName);
    void SetOldLastName(const char* oldLastName)
    {
        this->oldlastname = oldLastName;
    }

    const char* GetCharName() const
    {
        return name.GetData();
    }
    const char* GetCharLastName() const
    {
        return lastname.GetData();
    }
    const char* GetCharFullName() const
    {
        return fullName.GetData();
    }
    const char* GetOldLastName() const
    {
        return oldlastname.GetData();
    }

    // Introductions
    /// Answers whether this character knows the given character or not.
    bool Knows(PID charid);
    bool Knows(psCharacter* c)
    {
        return (c ? Knows(c->GetPID()) : false);
    }
    /// Introduces this character to the given character; answers false if already introduced.
    bool Introduce(psCharacter* c);
    /// Unintroduces this character to the given character; answers false if not introduced.
    bool Unintroduce(psCharacter* c);

    unsigned int GetCharType() const
    {
        return characterType;
    }
    void SetCharType(unsigned int v)
    {
        CS_ASSERT(v < PSCHARACTER_TYPE_COUNT);
        characterType = v;
    }
    const char* GetCharTypeName()
    {
        return characterTypeName[characterType];
    }

    /**
     * Set the last login time for the character.
     */
    void SetLastLoginTime();

    /**
     * Retrive the last login time in the format YYYY-MM-DD HH:MM:SS.
     */
    csString GetLastLoginTime() const;

    /**
     * Set the elapsed time for pets.
     */
    void SetPetElapsedTime(double elapsedTime);

    /**
     * Retrive the elapsed time for pets.
     */
    double GetPetElapsedTime() const;

    /**
     * Set the name of the spouse.
     */
    void SetSpouseName(const char* name);

    /**
     *  Gets Spouse Name of a character.
     *
     *  @return  SpouseName or "" if not married.
     */
    const char* GetSpouseName() const
    {
        return spouseName.GetData();
    }
    void SetIsMarried(bool married)
    {
        isMarried = married;
    }
    bool GetIsMarried() const
    {
        return isMarried;
    }

    /**
     * Sets the provided race info as the base default one.
     *
     * Don't use this to change temporarily the raceinfo but only definitely
     * (or upon load)
     *
     * @param rinfo A pointer to a @see psRaceInfo
     */
    void SetRaceInfo(psRaceInfo* rinfo);

    /**
     * Gets the pointer to the global current raceinfo applied to this character.
     *
     * @return A @see psRaceInfo pointer
     */
    psRaceInfo* GetRaceInfo();

    /**
     * Gets a reference to the overridable race allowing to override it or check the base race.
     *
     * @return A @see OverridableRace reference in this psCharacter
     */
    OverridableRace &GetOverridableRace();

    /** Gets the scale including any buffs.
     *
     *  Scale consist of a baseScale value from RaceInfo mutliplied with any buffs.
     *
     *  @return A float which contains the final scale of the model.
     */
    float GetScale();

    /** Gets the user scale value.
     *
     *  Use @see GetScale to get final model scale value. This function
     *  return the scale from character_values and any buffs.
     *
     *  @return A float which contains the user scale value.
     */
    float GetScaleValue();

    /**
     * Add a new explored area.
     *
     * @param explored The PID of the area npc.
     */
    bool AddExploredArea(PID explored);

    /**
     * Check if an area has already been explored.
     *
     * @param explored The PID of the area npc.
     */
    bool HasExploredArea(PID explored);

    FactionSet* GetFactions()
    {
        return factions;
    }

    void SetLootCategory(int id)
    {
        lootCategoryId = id;
    }
    int  GetLootCategory() const
    {
        return lootCategoryId;
    }

    /**
     * Removes an item from the loot loaded on the character due to it's defeat.
     *
     * It searches for an item with the base stats item having the passed id and returns it
     * if it was found, removing it from the list of the lootable items.
     *
     * @param id The base item stats id of the item.
     * @return The psItem whch  void DiscardQuest(QuestAssignment *q, bool force = false);corresponds to the base stats item searched for.
     */
    psItem* RemoveLootItem(int id);

    /**
     * Removes either all or only those items with specified categories from the loot of a defeated character.
     *
     * Checks whether the items in lootPending fit the specified categories and removes them accordingly.
     *
     * @param [categories] A csArray containing csStrings with category names. If
     *                     parameter is not provided, all items will be removed.
     * @return A csArray containing pointers to the removed psItem(s)
     */
    csArray<psItem*> RemoveLootItems(csArray<csString> categories = csArray<csString>());

    /**
     * Adds the supplied psItem to the list of lootable items from this character.
     *
     * @param item A pointer to the item being lootable from this character.
     */
    void AddLootItem(psItem* item);
    void AddLootMoney(int money)
    {
        lootMoney += money;
    }
    size_t GetLootItems(psLootMessage &msg, EID entity, int cnum);

    /// Gets and zeroes the loot money
    int  GetLootMoney();

    /// Clears the pending loot items array and money
    void ClearLoot();

    /// The last_response given by an npc to this player.
    int GetLastResponse()
    {
        return lastResponse;
    }
    void SetLastResponse(int response)
    {
        lastResponse=response;
    }
    bool CheckResponsePrerequisite(NpcResponse* resp);

    void CombatDrain(int);

    /**
     * Checks if a variable was defined for this character.
     *
     * @param name The name of the variable to search for.
     * @return TRUE if the variable was found.
     */
    bool HasVariableDefined(const csString &name);

    /**
     * Returns the value of a variable.
     *
     * @param name The name of the variable to search for.
     * @return A csString with the value of the variable.
     */
    csString GetVariableValue(const csString &name);

    /**
     * Returns the reference to a buffable variable.
     *
     * If not existant a temporary one is created with the passed value.
     * @param name The name of the variable to search for.
     * @param value The value.
     * @return A buffable<int> with the value of the variable.
     */
    Buffable<int> &GetBuffableVariable(const csString &name, const csString &value = "0");

    /**
     * Sets a new variable for this character.
     *
     * @param name The name of the new variable.
     * @param value The value of the new variable.
     */
    void SetVariable(const csString &name, const csString &value);

    /**
     * Sets a new variable for this character.
     *
     * The value will be set to a default empty string.
     * @param name The name of the new variable.
     */
    void SetVariable(const csString &name);

    /**
     * Remove a variable from this character.
     * @param name The name of the variable to remove.
     */
    void UnSetVariable(const csString &name);

    /**
     * Retrive const iterator for character variables.
     */
    csHash<charVariable, csString>::ConstGlobalIterator GetVariables() const;

    unsigned int GetExperiencePoints(); // W
    void SetExperiencePoints(unsigned int W);
    unsigned int AddExperiencePoints(unsigned int W);
    unsigned int AddExperiencePointsNotify(unsigned int W);
    unsigned int CalculateAddExperience(PSSKILL skill, unsigned int awardedPoints, float modifier = 1);
    unsigned int GetProgressionPoints(); // X
    void SetProgressionPoints(unsigned int X,bool save);
    void UseProgressionPoints(unsigned int X);

    /// Get the maximum realm the caster can cast with given skill
    int GetMaxAllowedRealm(PSSKILL skill);
    SkillRank &GetSkillRank(PSSKILL skill)
    {
        return skills.GetSkillRank(skill);
    }

    void KilledBy(psCharacter* attacker)
    {
        deaths++;
        if(!attacker) suicides++;
    }
    void Kills(psCharacter* target)
    {
        kills++;
    }

    unsigned int GetKills() const
    {
        return kills;
    }
    unsigned int GetDeaths() const
    {
        return deaths;
    }
    unsigned int GetSuicides() const
    {
        return suicides;
    }

    /**
     *  Drops an item into the world (one meter from this character's position).
     *
     * @param item to be dropped
     * @param pos the position
     * @param rot the rotation
     * @param guarded TRUE if guarded
     * @param transient flag (decay?) (default=true)
     * @param inplace TRUE if in place.
     */
    void DropItem(psItem* &item, csVector3 pos = 0, const csVector3 &rot = csVector3(0), bool guarded = true, bool transient = true, bool inplace = false);

    float GetHP();
    float GetMana();
    float GetStamina(bool pys);

    void SetHitPoints(float v);
    void SetMana(float v);
    void SetStamina(float v, bool pys);

    void AdjustHitPoints(float adjust);
    void AdjustMana(float adjust);
    void AdjustStamina(float adjust, bool pys);

    VitalBuffable &GetMaxHP();
    VitalBuffable &GetMaxMana();
    VitalBuffable &GetMaxPStamina();
    VitalBuffable &GetMaxMStamina();

    VitalBuffable &GetHPRate();
    VitalBuffable &GetManaRate();
    VitalBuffable &GetPStaminaRate();
    VitalBuffable &GetMStaminaRate();

    void SetStaminaRegenerationNone(bool physical = true, bool mental = true);
    void SetStaminaRegenerationWalk(bool physical = true, bool mental = true);
    void SetStaminaRegenerationSitting();
    void SetStaminaRegenerationStill(bool physical = true, bool mental = true);
    void SetStaminaRegenerationWork(int skill);

    /**
     * Recalculate the maximm stamina for the character.
     */
    void CalculateMaxStamina();

    /**
     * Return the dirty flags for vitals.
     */
    unsigned int GetStatsDirtyFlags() const;

    /**
     * Set all vital stats dirty.
     */
    void SetAllStatsDirty();

    /**
     * Cleare the dirty flags for vitals.
     */
    void ClearStatsDirtyFlags(unsigned int dirtyFlags);

    const char* GetHelmGroup()
    {
        return helmGroup.GetData();
    }
    const char* GetBracerGroup()
    {
        return BracerGroup.GetData();
    }
    const char* GetBeltGroup()
    {
        return BeltGroup.GetData();
    }
    const char* GetCloakGroup()
    {
        return CloakGroup.GetData();
    }

    void SetHelmGroup(const char* Group)
    {
        helmGroup = Group;
    }
    void SetBracerGroup(const char* Group)
    {
        BracerGroup = Group;
    }
    void SetBeltGroup(const char* Group)
    {
        BeltGroup = Group;
    }
    void SetCloakGroup(const char* Group)
    {
        CloakGroup = Group;
    }

    size_t GetAssignedGMEvents(psGMEventListMessage &gmevents, int clientnum);
    void AssignGMEvent(int id, bool playerIsGM);
    void CompleteGMEvent(bool playerIsGM);
    void RemoveGMEvent(int id, bool playerIsGM=false);

    /**
     * Update a npc's default spawn position with given data.
     */
    void UpdateRespawn(csVector3 pos, float yrot, psSectorInfo* sector, InstanceID instance);


    /**
     * Update this faction for this player with delta value.
     */
    bool UpdateFaction(Faction* faction, int delta);

    /**
     * Check player for given faction.
     */
    bool CheckFaction(Faction* faction, int value);

    /**
     * Takes care of setting correctly all the data needed to keep track of the song.
     */
    void StartSong();

    /**
     * Resets the song's execution data. This must always be called when a song ends.
     *
     * @param bonusTime when the next song starts, bonusTime will be summed to the
     * actual execution time.
     */
    void EndSong(csTicks bonusTime);

    /**
     * Gets the starting time of the song that the player is currently playing.
     */
    csTicks GetSongStartTime() const
    {
        return songExecutionTime;
    }

    /**
     * Check if the character is a banker.
     */
    bool IsBanker() const
    {
        return banker;
    }

    /**
     * Check if the character is a storage.
     *
     * @return TRUE if the character mantains a storage
     */
    bool IsStorage() const
    {
        return IsBanker();
    }
    void RecalculateStats();

    bool IsNPC()
    {
        return characterType == PSCHARACTER_TYPE_NPC;
    }

    /**
     * Used to determine if this character is a player.
     *
     * @return TRUE if the character is a player.
     */
    bool IsPlayer()
    {
        return characterType == PSCHARACTER_TYPE_PLAYER;
    }

    /// Used to determine if this NPC is a pet
    bool IsPet()
    {
        return (characterType == PSCHARACTER_TYPE_PET || characterType == PSCHARACTER_TYPE_MOUNTPET);
    }
    /// Used to determine if this NPC is a mount
    bool IsMount()
    {
        return (characterType == PSCHARACTER_TYPE_MOUNT || characterType == PSCHARACTER_TYPE_MOUNTPET);
    }
    PID  GetFamiliarID(size_t id)
    {
        return familiarsId.GetSize() > id ? familiarsId.Get(id) : 0;
    }
    void SetFamiliarID(PID v);
    size_t GetNumFamiliars()
    {
        return familiarsId.GetSize();
    }
    bool CanSummonFamiliar()
    {
        return canSummonFamiliar.Current() > 0;
    }
    Buffable<int> &GetCanSummonFamiliar()
    {
        return canSummonFamiliar;
    }
    const char* GetAnimalAffinity()
    {
        return animalAffinity.GetDataSafe();
    }
    void SetAnimialAffinity(const char* v)
    {
        animalAffinity = v;
    }
    PID  GetOwnerID()
    {
        return ownerId;
    }
    void SetOwnerID(PID v)
    {
        ownerId = v;
    }
    bool IsHired()
    {
        return hired;
    }
    void SetHired(bool hired)
    {
        this->hired = hired;
    }

    bool UpdateStatDRData(csTicks now);
    bool SendStatDRMessage(uint32_t clientnum, EID eid, int flags, csRef<PlayerGroup> group = NULL);

    /**
     * Returns true if the character is able to attack with the current slot.
     *
     * This could be true even if the slot is empty (as in fists).
     * It could also be false due to effects or other properties.
     */
    bool GetSlotAttackable(INVENTORY_SLOT_NUMBER slot);

    bool GetSlotAutoAttackable(INVENTORY_SLOT_NUMBER slot);
    bool GetSlotSingleAttackable(INVENTORY_SLOT_NUMBER slot);
    void ResetSwings(csTicks timeofattack);
    void TagEquipmentObject(INVENTORY_SLOT_NUMBER slot,int eventId);
    int GetSlotEventId(INVENTORY_SLOT_NUMBER slot);

    /// Retrieves the calculated Attack Value for the given weapon-slot
    //float GetAttackValue(psItem *slotitem);
    //float GetAttackValueForWeaponInSlot(int slot);
    float GetTargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot);
    float GetUntargetedBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot);
    float GetTotalTargetedBlockValue();
    float GetTotalUntargetedBlockValue();
    float GetCounterBlockValueForWeaponInSlot(INVENTORY_SLOT_NUMBER slot);
    float GetDodgeValue();

    Multiplier &AttackModifier()
    {
        return attackModifier;
    }
    Multiplier &DefenseModifier()
    {
        return defenseModifier;
    }

    /// Practice skills for armor and weapons
    void PracticeArmorSkills(unsigned int practice, INVENTORY_SLOT_NUMBER attackLocation);
    void PracticeWeaponSkills(unsigned int practice);
    void PracticeWeaponSkills(psItem* weapon, unsigned int practice);

    void SetTraitForLocation(PSTRAIT_LOCATION location,psTrait* trait);
    psTrait* GetTraitForLocation(PSTRAIT_LOCATION location);

    void GetLocationInWorld(InstanceID &instance,psSectorInfo* &sectorinfo,float &loc_x,float &loc_y,float &loc_z,float &loc_yrot);
    void SetLocationInWorld(InstanceID instance,psSectorInfo* sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot);
    void SaveLocationInWorld();

    /// Construct an XML format string of the player's texture choices.
    void MakeTextureString(csString &textureString);

    /// Construct an XML format string of the player's equipment.
    void MakeEquipmentString(csString &equipmentString);

    /// Returns a level of character based on his 6 base stats.
    unsigned int GetCharLevel(bool physical);

    bool IsMerchant()
    {
        return (merchantInfo != NULL);
    }
    psMerchantInfo* GetMerchantInfo()
    {
        return merchantInfo;
    }
    bool IsTrainer()
    {
        return (trainerInfo != NULL);
    }
    psTrainerInfo* GetTrainerInfo()
    {
        return trainerInfo;
    }
    psCharacter* GetTrainer()
    {
        return trainer;
    }
    void SetTrainer(psCharacter* trainer)
    {
        this->trainer = trainer;
    }

    /**
     *  Figure out if this skill can be trained.
     *
     * Checks the current knowledge of the skill. If it is already maxed out then
     * can train no more.
     *
     * @param skill The skill we want to train.
     * @return  True if the skill still requires Y credits before it is fully trained.
     */
    bool CanTrain(PSSKILL skill);

    /**
     *  Trains a skill.
     *
     *  It will only train up to the cost of the next rank. So the yIncrease is
     *  capped by the cost and anything over will be lost.
     *
     *  @param skill The skill we want to train.
     *  @param yIncrease  The amount we want to train this skill by.
     */
    void Train(PSSKILL skill, int yIncrease);

    /**
     * Directly sets rank of given skill. It completely bypasses the skill logic,
     * it is used for testing only.
     */
    void SetSkillRank(PSSKILL which, unsigned int rank);

    psSpell* GetSpellByName(const csString &spellName);
    psSpell* GetSpellByIdx(int index);
    csArray<psSpell*> &GetSpellList()
    {
        return spellList;
    }


    psCharacter* GetMerchant()
    {
        return merchant;
    }
    TradingStatus GetTradingStatus()
    {
        return tradingStatus;
    }
    void SetTradingStatus(TradingStatus trading, psCharacter* merchant)
    {
        tradingStatus = trading;
        this->merchant = merchant;
    }

    gemActor* GetActor()
    {
        return actor;
    }
    void SetActor(gemActor* actor);

    bool SetTradingStopped(bool stopped);

    bool ReadyToExchange();

    /// Number of seconds online this session in seconds.
    unsigned int GetOnlineTimeThisSession()
    {
        return (csGetTicks() - startTimeThisSession)/1000;
    }

    /// Number of seconds online ever including this session in seconds.
    unsigned int GetTotalOnlineTime()
    {
        return timeconnected + GetOnlineTimeThisSession();
    }


    unsigned int GetTimeConnected()
    {
        return timeconnected;
    }

    /**
     * This is used to get the stored player description.
     * @return Returns a pointer to the stored player description.
     */
    const char* GetDescription();

    /**
     * This is used to store the player description.
     * @param newValue: A pointer to the new string to set as player description.
     */
    void SetDescription(const char* newValue);

    /**
     * This is used to get the stored player OOC description.
     * @return Returns a pointer to the stored player OOC description.
     */
    const char* GetOOCDescription();

    /**
     * This is used to store the player OOC description.
     * @param newValue: A pointer to the new string to set as player OOC description.
     */
    void SetOOCDescription(const char* newValue);

    /**
     * This is used to get the stored informations from the char creation.
     * @return Returns a pointer to the stored informations from the char creation.
     */
    const char* GetCreationInfo();

    /**
     * This is used to store the informations from the char creation. Players shouldn't be able to edit this.
     * @param newValue: A pointer to the new string to set as char creation data.
     */
    void SetCreationInfo(const char* newValue);

    /**
     * Gets dynamic life events generated from the factions of the character.
     * @param factionDescription: where to store the dynamically generated data.
     * @return Returns true if there were some dynamic life events founds else false.
     */
    bool GetFactionEventsDescription(csString &factionDescription);

    /**
     * This is used to get the stored informations from the custom life events made by players.
     * @return Returns a pointer to the stored informations from the custom life events made by players.
     */
    const char* GetLifeDescription();

    /**
     * This is used to store the informations sent by players for their custom life events.
     * @param newValue: A pointer to the new string to set as custom life events.
     */
    void SetLifeDescription(const char* newValue);

    /// This is used by the math scripting engine to get various values.
    double GetProperty(MathEnvironment* env, const char* ptr);
    double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    const char* ToString()
    {
        return fullName.GetData();
    }

    /// The exp to be handed out when this actor dies
    int GetKillExperience()
    {
        return killExp;
    }
    void SetKillExperience(int newValue)
    {
        killExp=newValue;
    }

    void SetImperviousToAttack(int newValue)
    {
        imperviousToAttack=newValue;
    }
    int GetImperviousToAttack()
    {
        return imperviousToAttack;
    }

    void CalculateEquipmentModifiers();
    float GetStatModifier(PSITEMSTATS_STAT attrib);

    bool AppendCharacterSelectData(psAuthApprovedMessage &auth);

    // NPC based functions - should these go here?
    int NPC_GetSpawnRuleID()
    {
        return npcSpawnRuleId;
    }
    void NPC_SetSpawnRuleID(int v)
    {
        npcSpawnRuleId=v;
    }

    psBuddyManager &GetBuddyMgr()
    {
        return buddyManager;
    }
    psCharacterQuestManager &GetQuestMgr()
    {
        return questManager;
    }

    ///  The new operator is overriden to call PoolAllocator template functions
    void* operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void*);

    /**
     * Get the account ID that this character is attached to.
     * @return The account ID that this character is attached to.
     */
    AccountID GetAccountId()
    {
        return accountid;
    }

    /**
     * Get the character spawn location.
     * @return The struct that has the information about where the character spawns.
     */
    st_location GetSpawnLocation()
    {
        return spawnLoc;
    }

    /**
     * Get the character location
     * @return The struct that has the information about where the character is
     */
    st_location GetLocation() const
    {
        return location;
    }

     /** @brief Get the attack queue
    *   @return attack queue member
    */    
    psAttackQueue* GetAttackQueue() { return attackQueue; } 
    friend class psCharacterLoader;

protected:

    bool LoadSpells(PID use_id);
    bool LoadAdvantages(PID use_id);
    bool LoadSkills(PID use_id);
    bool LoadTraits(PID use_id);
    bool LoadRelationshipInfo(PID pid);
    bool LoadBuddies(Result &myBuddy, Result &buddyOf);
    bool LoadMarriageInfo(Result &result);
    bool LoadFamiliar(Result &pet, Result &owner);
    /// Helper function which loads the factions from the database.
    bool LoadFactions(PID pid);
    /// Helper function which saves the factions to the database.
    void UpdateFactions();

    /**
     * Helper function which loads the character variables from the database.
     * @return TRUE always. bool was used for consistancy.
     */
    bool LoadVariables(PID pid);

    /**
     * Helper function which saves the character variables to the database.
     */
    void UpdateVariables();

    bool LoadExploration(Result &exploration);
    bool LoadGMEvents();

    psCharacterInventory        inventory;                    ///< Character's inventory handler.
    psGuildInfo*                guildinfo;
    StatSet                     modifiers;
    SkillSet                    skills;
    csSet<PID>                  acquaintances;
    int                         npcMasterId;
    unsigned int                deaths;
    unsigned int                kills;
    unsigned int                suicides;
    bool                        loaded;
    psBuddyManager              buddyManager;
    psCharacterQuestManager     questManager;

    csRef<psMerchantInfo>       merchantInfo;           ///< Character's merchant info
    psCharacter*                merchant;               ///< The merchant this charcter trade with

    bool                        tradingStopped;
    TradingStatus               tradingStatus;          ///< See the enum for the various status.

    gemActor* actor;                ///< The current game entity object this characater is attached to.
    PID pid;                        ///< Current  PID for this characater ( or is it the above entity? )
    AccountID accountid;            ///< Account ID that this characater is attached to.

    csRef<psTrainerInfo>    trainerInfo;        ///< Character's trainer information
    psCharacter*            trainer;            ///<  The current character that is being trained?

    csString description;     ///<Player description
    csString oocdescription;  ///<Player OOC description
    csString creationinfo;    ///<Creation manager informations
    csString lifedescription; ///<Custom life events informations

    int killExp; ///< Kill Exp

    Multiplier attackModifier;  ///< Attack  value is multiplied by this
    Multiplier defenseModifier; ///< Defense value is multiplied by this

    st_location spawnLoc;

    csString name;
    csString lastname;
    csString fullName;
    csString oldlastname;

    csString spouseName;
    bool     isMarried;

    OverridableRace race; ///< Holds the race of this character and it's overriden values.

    FactionSet* factions;

    csString progressionScriptText; ///< flat string loaded from the DB.

    int     imperviousToAttack;

    /// Bitfield for which help events a character has already encountered.
    unsigned int     helpEventFlags;

    st_location location;
    psServerVitals* vitals;

    psTrait* traits[PSTRAIT_LOCATION_COUNT];

    /// NPC specific data.  Should this go here?
    int npcSpawnRuleId;

    /// Id of Loot category to use if this char has extra loot
    int  lootCategoryId;

    csString animalAffinity;
    PID ownerId;
    csArray<PID> familiarsId;
    Buffable<int> canSummonFamiliar;

    /// Total number of seconds online.  Updated at logoff.
    unsigned int timeconnected;
    csTicks startTimeThisSession;

    csString lastLoginTime;///< String value copied from the database containing the last login time
    double petElapsedTime;
    double lastSavedPetElapsedTime; ///< Store time for when the petElapsedTime was last saved to db.

    psMoney money;                                          ///< Current cash set on player.
    psMoney bankMoney;                                      ///< Money stored in the players bank account.

    csArray<psSpell*>         spellList;

    csHash<charVariable, csString> charVariables; ///< Used to store character variables for this character.

    psSkillCache              skillCache;
    GMEventsAssignment        assignedEvents;
    csArray<PID>              exploredAreas;

    ///< A bitfield which contains the notifications the player will get if a guild or alliance member login/logoff.
    int joinNotifications;

    float overrideMaxHp,overrideMaxMana;  ///< These values are loaded from base_hp_max,base_mana_max in the db and
    ///< should prevent normal HP calculations from taking place

    static const char* characterTypeName[];
    unsigned int characterType;

    /// Array of items waiting to be looted.
    csArray<psItem*> lootPending;
    /// Last response of an NPC to this character (not saved)
    int  lastResponse;
    /// Amount of money ready to be looted
    int  lootMoney;
    /// Says if this npc is a statue
    bool isStatue;

    //The attack queue
    csRef<psAttackQueue> attackQueue;
private:
    void CalculateArmorForSlot(INVENTORY_SLOT_NUMBER slot, float &heavy_p, float &med_p, float &light_p);
    bool ArmorUsesSkill(INVENTORY_SLOT_NUMBER slot, PSITEMSTATS_ARMORTYPE skill);

    int FindGlyphSlot(const csArray<glyphSlotInfo> &slots, psItemStats* glyphType, int purifyStatus);

    csTicks songExecutionTime;   ///< Keeps track of the execution time of a player's song.

    /**
     * Some races share helms so this tells which
     * group it's in. If empty assume in racial group.
     */
    csString helmGroup;

    /**
     * Some races share bracers so this tells which
     * group it's in. If empty assume in racial group.
     */
    csString BracerGroup;

    /**
     * Some races share belts so this tells which
     * group it's in. If empty assume in racial group.
     */
    csString BeltGroup;

    /**
     * Some races share cloaks so this tells which
     * group it's in. If empty assume in racial group.
     */
    csString CloakGroup;

    bool banker;    ///< Whether or not the character is a banker

    bool hired;     ///< Whether or not this character is hired.

    /// Static reference to the pool for all psItem objects
    static PoolAllocator<psCharacter> characterpool;
};

/** @} */

#endif


