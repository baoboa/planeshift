/*
 * adminmanager.h
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
#ifndef __ADMINMANAGER_H__
#define __ADMINMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"            // Chat Message definitions
#include "util/psconst.h"
#include "util/waypoint.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class
#include "gmeventmanager.h"

///This is used for petitions to choose if it's a player or a gm asking
///to do a certain operation
#define PETITION_GM 0xFFFFFFFF

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class psDatabase;
class psSpawnManager;
class psServer;
class psDatabase;
class EventManager;
class ClientConnectionSet;
class EntityManager;
class psSpawnManager;
class psAdminGameEvent;
class psNPCDialog;
class psItem;
class psSkillInfo;
struct Result;
class iResultSet;
class Client;
class psPathNetwork;
class LocationManager;
class psPath;
class Waypoint;
class WordArray;
class gemNPC;

/// List of GM levels and their security level.
enum GM_LEVEL
{
    GM_DEVELOPER = 30,
    GM_LEVEL_9 = 29,
    GM_LEVEL_8 = 28,
    GM_LEVEL_7 = 27,
    GM_LEVEL_6 = 26,
    GM_LEVEL_5 = 25,
    GM_LEVEL_4 = 24,
    GM_LEVEL_3 = 23,
    GM_LEVEL_2 = 22,
    GM_LEVEL_1 = 21,
    GM_LEVEL_0 = 20,
    GM_TESTER  = 10
};

/**
 * Reward Data is an abstract structure for all kinds of rewards
 */
class psRewardData
{
public:
    /// The different types of rewards that can be assigned
    enum Reward_Type
    {
        REWARD_EXPERIENCE, ///< experience
        REWARD_FACTION, ///< faction points
        REWARD_SKILL, ///< skill points
        REWARD_MONEY, ///< trias, octas, ...
        REWARD_ITEM, ///< an item
        REWARD_PRACTICE ///< skill practice
    };

    Reward_Type rewardType; ///< stores the reward type /see Reward_Type

    /** Only initializes the rewardType
      */
    psRewardData(Reward_Type prewardType);

    /** Destructor.
     */
    virtual ~psRewardData();

    /** @brief checks if the reward is zero
     * returns bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero();

    /** @brief Awards the reward to the specified client.
     * @param gmClientnum: ClientID of the issuer
     * @param target: pointer to the Client of the target
     */
    //virtual void Award(int gmClientNum, Client* target);
};

/// @brief RewardDataExperience holds experience reward data
class psRewardDataExperience : public psRewardData
{
public:
    int expDelta; ///< relative value to adjust exp by

    /** @brief Stores an experience Delta
      * @param pexpDelta: experience delta (relative value)
      */
    psRewardDataExperience(int pexpDelta);

    /** @brief checks if the reward is zero
     * @return bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero(); ///< checks if the reward is zero

};

/// @brief RewardDataFaction holds faction reward data
class psRewardDataFaction : public psRewardData
{
public:
    csString factionName; ///< name of the faction to adjust
    int factionDelta; ///< relative value to adjust faction by

    psRewardDataFaction(csString pfactionName, int pfactionDelta);

    /** @brief checks if the reward is zero
     * @return bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero(); ///< checks if the reward is zero
};

/// @brief RewardDataSkill holds skill reward data
class psRewardDataSkill : public psRewardData
{
public:
    /** name of the skill to adjust.
      * may be "all" to adjust all skills.
      * may be "copy" to set the values of the target
      */
    csString skillName;
    int skillDelta; ///< value to adjust the skill by/set it to
    unsigned int skillCap; ///< maximum value the skill should have

    /** determines whether the value to adjust the skill is relative or not
     */
    bool relativeSkill;

    psRewardDataSkill(csString pskillName, int pskillDelta, int pskillCap, bool prelativeSkill);

    /** @brief checks if the reward is zero.
     *
     * @returns bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero(); ///< checks if the reward is zero
};

/// @brief RewardDataPractice holds practice reward data
class psRewardDataPractice : public psRewardData
{
public:
    /** name of the practice to adjust.
      * may be "all" to adjust all practices.
      * may be "copy" to set the values of the target
      */
    csString skillName;
    int practice; ///< value to adjust the practice by/set it to

    psRewardDataPractice(csString pSkillName, int pDelta);

    /** @brief checks if the reward is zero.
     *
     * @returns bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero(); ///< checks if the reward is zero
};

/// @brief RewardDataMoney holds money reward data
class psRewardDataMoney : public psRewardData
{
public:
    csString moneyType; ///< name of the money type to award (circles, hexas, ...)
    /** relativ amount of money to adjust by
      * if this is 0, a random value is used
      */
    int moneyCount;
    bool random; ///< whether the amount should be randomized or fix

    /** @brief creates a money reward.
     * @param pmoneyType string containing trias,hexas,octas, ...
     * @param pmoneyCount amount of the money
     * @param prandom whether this should be randomized (true) or not (false)
     */
    psRewardDataMoney(csString pmoneyType, int pmoneyCount, bool prandom);

    /** @brief checks if the reward is zero
     * @return bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero(); ///< checks if the reward is zero
};

/// @brief RewardDataItem holds item reward data.
class psRewardDataItem : public psRewardData
{
public:
    csString itemName; ///< name of the item to award
    unsigned short stackCount; ///< number of items to award

    psRewardDataItem(csString pitemName, int pstackCount);

    /** @brief checks if the reward is zero
     * @return bool: true when reward is a zero gain, otherwise false
     */
    virtual bool IsZero(); ///< checks if the reward is zero
};

/** @brief The different generic target types of admin command targets.
 *
 * In this case target does not refer to the target of the client, but to
 * to the target of the command (the object that should be manipulated by the
 * command).
 */
enum ADMINCMD_TARGET_TYPES
{
    ADMINCMD_TARGET_UNKNOWN = 0, ///< when not recognized as any of the below types
    ADMINCMD_TARGET_TARGET = 1, ///< "target"
    ADMINCMD_TARGET_PID = 1 << 1, ///< "pid:PID"
    ADMINCMD_TARGET_AREA = 1 << 2, ///< "area:AREA"
    ADMINCMD_TARGET_ME = 1 << 3, ///< "me"
    ADMINCMD_TARGET_PLAYER = 1 << 4, ///< "name of a player"
    ADMINCMD_TARGET_OBJECT = 1 << 5, ///< "name of an object"
    ADMINCMD_TARGET_NPC = 1 << 6, ///< "name of a npc"
    ADMINCMD_TARGET_EID = 1 << 7, ///< "eid:EID"
    ADMINCMD_TARGET_ITEM = 1 << 8, ///< when object is an item
    ADMINCMD_TARGET_CLIENTTARGET = 1 << 9, ///< the targeted object of the client issuing the admincomman
    ADMINCMD_TARGET_ACCOUNT = 1 << 10, ///< when the target word gives an account name
    ADMINCMD_TARGET_DATABASE = 1 << 11, ///< when it's allowed to lookup the database.
    ADMINCMD_TARGET_STRING = 1 << 14 ///< when the target does not need to be any of the above defined targets
};

/** @brief Base class for all the data classes for admin commands.
 *
 * Contains Data and parsing facilities for admin commands.
 */
class AdminCmdData
{
public:
    csString command; ///< command name this obj contains data for
    bool help; ///< flag for displaying help (true for displaying help)
    bool valid; ///< flag for setting content valid/invalid

    /** @brief creates data object for the specified command.
     * @param commandName name of the command (e.g. /ban)
     */
    AdminCmdData(csString commandName)
        : command(commandName), help(false), valid(true)
    {}

    /** @brief Parses the given message and stores its data.
     * @param commandName name of the command (e.g. /ban)
     * @param words command message to parse
     */
    AdminCmdData(csString commandName, WordArray &words);

    virtual ~AdminCmdData()
    {}

    /** @brief Creates a command data object of the current class containing the parsed data.
     * @param msgManager message manager that handles this command
     * @param me Message sent by the client
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Retrieve help message for the command that fits to the data.
     * @return a help message to send back to the client
     */
    virtual csString GetHelpMessage();

    /** @brief Sends the error message to the gm and invalidates the object.
     *
     * errormessage is sent to the gm client and valid is set to false.
     * @param me The incoming message from the GM
     * @param errmsg error message
     */
    void ParseError(MsgEntry* me, const char* errmsg);

    /** @brief Logs the game master command.
     * @param gmClient issuing the command
     * @param cmd command message issued
     * @return bool: false when database logging fails
     */
    virtual bool LogGMCommand(Client* gmClient, const char* cmd);

    /** @brief Used for overriding. If it's true the command will have no
     *         visual effects and will be discarded.
     *
     * @return bool if true we will ignore this command and not execute it.
     */
    virtual bool IsQuietInvalid()
    {
        return false;
    }

protected:
    /** @brief logs the game master command and target to the database.
     *
     * /slide command cannot be logged with this function.
     * @param gmClient issuing the command
     * @param playerID targeted player (if applicable)
     * @param cmd command message issued
     */
    bool LogGMCommand(Client* gmClient, PID playerID, const char* cmd);

    /** @brief Test for help word and store help state when found.
     *
     * @param word to test
     * @return bool if help word was found, otherwise false
     */
    bool IsHelp(const csString &word)
    {
        help = (word == "help");
        return help;
    }
};

/** @brief Class for parsing a target of a admin command.
 *
 * The class handles the parsing related to the possible targets for the
 * commands.
 * Supported targets are: target (declares explicitly the clients target as a target), pid:PID, area:Area, me, playername, npcname, objectname, accountname, string. The types that the object actually recognizes as correct targets are set in the allowedTypes variables. Recognition is executed in the already given order.
 */
class AdminCmdTargetParser
{
public:
    gemObject* targetObject; ///< set to the object of the target when possible
    gemActor* targetActor; ///< set to the targets actor when possible
    Client* targetClient; ///< set to the targets client when possible
    /** is set to true if a character was found by name and multiple
     * name instances were found.
     */
    bool area; ///< Indicates that this is an area target so requires special handling.
    bool duplicateActor;
    csString target; ///< player that is target for the command
    PID targetID; ///< stores PID when target is a player/npc/???? specified by PID

    /** @brief default constructor.
     * @param targetTypes the allowed target types
     */
    AdminCmdTargetParser(int targetTypes)
        : targetObject(NULL), targetActor(NULL), targetClient(NULL),
        area(false), duplicateActor(false), target(), targetID(),
        allowedTargetTypes(targetTypes), targetTypes(ADMINCMD_TARGET_UNKNOWN),
        targetAccountID()
    {}

    /** @brief the default destructor.
     */
    virtual ~AdminCmdTargetParser()
    {}

    /** @brief Test for unparsable targettype.
     *
     * When no target is detected. This happens when the supplied word is
     * empty or the word does not contain any of the possible (allowed)
     * targettypes.
     * @return bool: true when unknown, otherwise false
     */
    bool IsTargetTypeUnknown()
    {
        return (targetTypes == ADMINCMD_TARGET_UNKNOWN);
    }

    /** Test whether the given target type is allowed or not.
      * @param targetType to test
      * @return bool: true if allowed, otherwise false
      */
    bool IsAllowedTargetType(ADMINCMD_TARGET_TYPES targetType)
    {
        return (allowedTargetTypes & targetType);
    }

    /** @brief Test whether the given target type is of the supplied type or not.
      * @param targetType to test for
      * @return bool: true if allowed, otherwise false
      */
    bool IsTargetType(ADMINCMD_TARGET_TYPES targetType)
    {
        return (targetTypes & targetType);
    }

    /** @brief Tries to parse the supplied string as a destination.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param target string to parse as a destination
     * @return false when parsing the supplied string failed, otherwise true
     */
    virtual bool ParseTarget(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, csString target);

    /** @brief Returns a helpmessage string for the allowed types of the target.
     *
     * Automatically generates a help message for the allowed types of the
     * target.
     * @return csString: containg the helpmessage part for destination
     */
    virtual csString GetHelpMessagePartForTarget();

    /** @brief Returns the AccountID when the target is a player.
     * @param gmClientNum id of gm client
     * @return AccountiD: valid ID when target is a player, otherwise invalid ID
     */
    virtual AccountID GetAccountID(size_t gmClientNum);

    /** @brief Returns true when the 'target' is online.
     * @return bool: true when target online, otherwise false
     */
    bool IsOnline()
    {
        return (targetObject != NULL);
    }

protected:
    int allowedTargetTypes; ///< allowed destination types
    int targetTypes; ///< type of the parsed target
    AccountID targetAccountID; ///< internal cache value for accountID

    /** @brief Tries to resolve player by pid:PID to accountid.
     *
     * Queries the database for the user
     * @param gmClientNum id of gm client
     * @param word to check for a playername
     * @return bool: true, when player found, otherwise false
     */
    bool GetPlayerAccountIDByPID(size_t gmClientNum, const csString &word);

    /** @brief Tries to resolve a player name to pid and accountid.
     *
     * Queries the database, no checks are done that the player is offline
     * @param gmClientNum id of gm client
     * @param word to check for a playername
     * @param reporterror whether to send errormsg on not found player, or not
     * @return bool: true, when player found, otherwise false
     */
    bool GetPlayerAccountIDByPIDFromName(size_t gmClientNum, const csString &word, bool reporterror);

    /** @brief Tries to resolve a player name to pid and accountid.
     *
     * Queries the database, no checks are done that the player is offline
     * does not set targettypes, etc. (or any other internal variable)
     * targetAccountID is the only exception, it is set when a valid one was
     * found.
     * @param gmClientNum id of gm client
     * @param word to check for a playername
     * @param reporterror whether to send errormsg on not found player, or not
     * @return bool: true, when player found, otherwise false
     */
    bool GetPlayerAccountIDByName(size_t gmClientNum, const csString &word, bool reporterror);

    /** @brief Tries to find a client by name and checks for duplicates.
     *
     * @param msgManager adminmanager that utilizes this function
     * @param gmClientNum id of gm client
     * @param playerName name of the player whose client is searched for
     * @param allowduplicate is true when playerName can resolve to more than one client
     * @return true when client found, otherwise false
     */
    bool GetPlayerClient(AdminManager* msgManager, size_t gmClientNum, const csString &playerName, bool allowduplicate);

    /** @brief resets the internal variables to their default values.
     *
     * Is needed for the case that something is targeted that is not valid
     */
    virtual void Reset();
};

/**
 * Class for storing subcommands for a specific word position.
 */
class AdminCmdSubCommandParser
{
public:
    csHash<csString, csString> subCommands; ///< list of subcommands

    /** @brief initializes subcommands list without any commands.
     */
    AdminCmdSubCommandParser()
    {}

    /** @brief initializes the subCommands List by splitting the string.
     *
     * Fast initialization by supplying a string with the commands that
     * are seperated by white spaces.
     * @param commandList space separated list of commands
     */
    AdminCmdSubCommandParser(csString commandList);

    virtual ~AdminCmdSubCommandParser()
    {}

    /** @brief Retrieve help message for the list of subcommands.
     * @return a help message to send back to the client
     */
    virtual csString GetHelpMessage();

    /** @brief Retrieve help message for a specific subcommand.
     * @return a help message for a subcommand when the help was registered.
     */
    virtual csString GetHelpMessage(const csString &subcommand);

    /** @brief test if a single word is a subcommand.
     * @param word single word (no whitespaces)
     * @return true when it is in the list of subcommands, otherwise false.
     */
    bool IsSubCommand(const csString &word);

    /** @brief add a new sub command with help text.
     * @param subcommand to register
     * @param helpmsg help message for the command
     * @return true when it is in the list of subcommands, otherwise false.
     */
    void Push(csString subcommand, csString helpmsg);
};

/** @brief Class for parsing rewards data from a command string and storing it.
 */
class AdminCmdRewardParser
{
public:
    AdminCmdSubCommandParser rewardTypes; ///< reward types

    csPDelArray<psRewardData> rewards;  ///< extracted rewards
    csString error; ///< only set when an error occured during parsing


    /** @brief Creates a reward parser with the default reward types.
     * @return
     */
    AdminCmdRewardParser();

    ~AdminCmdRewardParser()
    {}

    /** @brief Parses the list of words starting at index.
     * @param index to start the parsing in the words array
     * @param words array containing a commands parameters
     * @return false when an error occured, otherwise true
     */
    bool ParseWords(size_t index, const WordArray &words);

    /** @brief Returns a help message describing the rewards syntax.
     * @return help message for the rewards syntax
     */
    virtual csString GetHelpMessage();
};

/** @brief Base class for on, off, toggle string Parser.
 *
 * The class can parse a word for 'on', 'off', 'toggle' and stores
 * the parsed setting internally.
 */
class AdminCmdOnOffToggleParser
{
public:
    /** @brief detected settings recognized by the parser.
     */
    enum ADMINCMD_SETTING_ONOFF
    {
        ADMINCMD_SETTING_UNKNOWN = 0, ///< default is unknown setting
        ADMINCMD_SETTING_ON = 1, ///< when setting is 'on'
        ADMINCMD_SETTING_OFF, ///< when setting is 'off'
        ADMINCMD_SETTING_TOGGLE ///< when setting should be toggled
    };

    ADMINCMD_SETTING_ONOFF value; ///< stores on,off,toggle
    csString error; ///< set to error message when parsing failed

    AdminCmdOnOffToggleParser(ADMINCMD_SETTING_ONOFF defaultValue = ADMINCMD_SETTING_UNKNOWN);

    /** @brief Returns a help message describing the rewards syntax.
     * @return csString: help message for the rewards syntax
     */
    csString GetHelpMessage();

    /** @brief Test whether the value is set to on.
     * @return bool: true when value is ADMINCMD_SETTING_ON
     */
    bool IsOn();

    /** @brief Test whether the value is set to off.
     * @return bool: true when value is ADMINCMD_SETTING_OFF
     */
    bool IsOff();

    /** @brief Test whether the value is set to toggle.
     * @return bool: true when value is ADMINCMD_SETTING_TOGGLE
     */
    bool IsToggle();

    /** @brief Parses a word that is expected to have on|off|toggle.
     *
     * Returning true means the word was as expected 'on', 'off' or
     * 'toggle'.
     * Sets the internal variable error when a parsing error occurs.
     * @return bool: true when parsing succeeds, otherwise false
     */
    bool ParseWord(const csString &word);
};

/** @brief Base class for all commands that need a target to work on.
 */
class AdminCmdDataTarget : public AdminCmdData, public AdminCmdTargetParser
{
public:
    /** @brief Creates obj for the given command and allowed target types.
     * @param commandName name of the command (e.g. /ban)
     * @param targetTypes bitmask based on the ADMINCMD_TARGET_TYPES
     */
    AdminCmdDataTarget(csString commandName, int targetTypes)
        : AdminCmdData(commandName), AdminCmdTargetParser(targetTypes)
    {}

    /** @brief Creates obj for the given command and allowed target types.
     * @param commandName name of the command (e.g. /ban)
     * @param targetTypes bitmask based on the ADMINCMD_TARGET_TYPES
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataTarget(csString commandName, int targetTypes, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataTarget()
    {}

    /** @brief Creates a command data object containing the parsed data.
     *
     * The created object is most likely a derived class that parses the
     * given command string and stores the data internally for further use.
     * That kind of new object is passed back by this function.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData*: pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Retrieve help message for the command that fits to the data.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();

    /** @brief Logs the game master command.
     * @param gmClient issuing the command
     * @param cmd command message issued
     * @return
     */
    virtual bool LogGMCommand(Client* gmClient, const char* cmd);

    /** @brief Used to invalidate commands with area as they will be handled elsewhere.
     *
     * @return bool true if this command evaluated an area command and so should be ignored.
     */
    virtual bool IsQuietInvalid()
    {
        return IsTargetType(ADMINCMD_TARGET_AREA);
    }
};

/** @brief Class for commands that only need a target and a reason.
 */
class AdminCmdDataTargetReason : public AdminCmdDataTarget
{
public:
    csString reason; ///< reason for this deletion command

    /** @brief Creates obj for specified command that needs a reason.
     * @param command name of the command (e.g. /ban)
     */
    AdminCmdDataTargetReason(csString command)
        : AdminCmdDataTarget(command, ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Creates obj for specified command that needs a reason.
     * @param command name of the command (e.g. /ban)
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataTargetReason(csString command, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataTargetReason()
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Retrieve help message for the command that fits to the data.
     * @return a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for commands for instantly killing a user (player).
 */
class AdminCmdDataDeath : public AdminCmdDataTarget
{
public:
    csString requestor; ///< gm requesting this deletion command

    /** @brief Creates obj for specified command that needs a requestor.
     */
    AdminCmdDataDeath()
        : AdminCmdDataTarget("/death",  ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID)
    {};

    /** @brief Creates obj for specified command that needs a requestor.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataDeath(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataDeath()
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Retrieve help message for the command that fits to the data.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for commands deleting a player.
 */
class AdminCmdDataDeleteChar : public AdminCmdDataTarget
{
public:
    AdminCmdTargetParser requestor; ///< gm requesting this deletion command

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataDeleteChar()
        : AdminCmdDataTarget("/deletechar", ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID), requestor(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
    {};

    /** @brief Creates obj for specified command that needs a reason.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataDeleteChar(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataDeleteChar()
    {};

    /** Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** Retrieve help message for the command that fits to the data.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};


/** @brief Class for player/npc respawn update.
 */
class AdminCmdDataUpdateRespawn : public AdminCmdDataTarget
{
public:
    csString place; ///< named place to respawn (currently 'here' or nothing)

    /** @brief Creates obj for specified command that updates a respawn point.
     */
    AdminCmdDataUpdateRespawn()
        : AdminCmdDataTarget("/updaterespawn", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Creates obj for specified command that updates a respawn point.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataUpdateRespawn(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataUpdateRespawn()
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Retrieve help message for the command that fits to the data.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for banning a player.
 */
class AdminCmdDataBan : public AdminCmdDataTarget
{
public:
    int minutes; ///< minutes part of the time to ban
    int hours;   ///< hours part of the time to ban
    int days;    ///< days part of the time to ban
    bool banIP;  ///< true if the IP address is to ban
    csString reason; ///< reason for the ban

    /** @brief Creates obj for specified command that bans a player.
     */
    AdminCmdDataBan()
        : AdminCmdDataTarget("/ban", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_ACCOUNT | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
    {};

    /** Creates obj for specified command parsing the words banning a player.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataBan(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataBan()
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Retrieve help message for the command that fits to the data.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for killing and reloading a npc.
 */
class AdminCmdDataKillNPC : public AdminCmdDataTarget
{
public:
    bool   reload; ///< whether NPC should be reloaded after the kill or not
    float  damage; ///< If <> 0 than only do this damage

    /** @brief Creates obj for specified command that kills/reloads a npc.
     */
    AdminCmdDataKillNPC()
        : AdminCmdDataTarget("/killnpc", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE), reload(false), damage(0.0)
    {};

    /** @brief Parses the given message for killing/reloadin a npc.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataKillNPC(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataKillNPC()
    {};

    /** @brief Creates a command data object of the current class containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for percepting a npc.
 */
class AdminCmdDataPercept : public AdminCmdDataTarget
{
public:
    csString perception; ///< the perception to fire.
    csString type;       ///< the type data for the perception.

    /** @brief Creates obj for specified command that kills/reloads a npc.
     */
    AdminCmdDataPercept()
        : AdminCmdDataTarget("/percept", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message for percepting a npc.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataPercept(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataPercept()
    {};

    /** @brief Creates a command data object of the current class containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for changing npc types.
 */
class AdminCmdDataChangeNPCType : public AdminCmdDataTarget
{
public:
    csString npcType; ///< The type of npc which will be assigned.

    /** @brief Creates obj for specified command that changes npctype of a npc.
     */
    AdminCmdDataChangeNPCType()
        : AdminCmdDataTarget("/changenpctype", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), npcType("")
    {};

    /** @brief Parses the given message to change type of a npc.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataChangeNPCType(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataChangeNPCType()
    {};

    /** @brief Creates a command data object of the current class containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for changing npc debug level.
 */
class AdminCmdDataDebugNPC : public AdminCmdDataTarget
{
public:
    int debugLevel; ///< The debug level to set for npc.

    /** @brief Creates obj for specified command that changes NPC Debug level.
     */
    AdminCmdDataDebugNPC()
        : AdminCmdDataTarget("/debugnpc", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), debugLevel(0)
    {};

    /** @brief Parses the given message to set debug level for npc.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataDebugNPC(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataDebugNPC()
    {};

    /** @brief Creates a command data object of the current class containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for changing tribe debug level.
 */
class AdminCmdDataDebugTribe : public AdminCmdDataTarget
{
public:
    int debugLevel; ///< The debug level to set for npc.

    /** @brief Creates obj for specified command that changes Tribe Debug level.
     */
    AdminCmdDataDebugTribe()
        : AdminCmdDataTarget("/debugtribe", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), debugLevel(0)
    {};

    /** @brief Parses the given message to set debug level for npc.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataDebugTribe(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /**
     * Destructor.
     */
    virtual ~AdminCmdDataDebugTribe() {};

    /** @brief Creates a command data object of the current class containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for un/setting, displaying information on stackable items.
 */
class AdminCmdDataSetStackable : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of subcommands
    csString stackableAction; ///< subcommand storage

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataSetStackable()
        : AdminCmdDataTarget("/setstackable", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), subCommandList("info on off reset help")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetStackable(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetStackable()
    {};

    /** @brief Creates a object of the current class containing parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData*: pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for loading/reloading quests.
 */
class AdminCmdDataLoadQuest : public AdminCmdData
{
public:
    csString questName; ///< name of the specified quest

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataLoadQuest()
        : AdminCmdData("/loadquest")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataLoadQuest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataLoadQuest()
    {};

    /** @brief Creates an object of the current class containing parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for commands creating items.
 */
class AdminCmdDataInfo : public AdminCmdDataTarget
{
public:
    csString subCmd; ///< Sub of commands

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataInfo()
        : AdminCmdDataTarget("/info", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE), subCmd("summary")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataInfo(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataInfo()
    {};

    /** @brief Creates an object of the current class containing parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for commands creating items.
 */
class AdminCmdDataItem : public AdminCmdDataTarget
{
public:
    bool random; ///< random item modifiers are added
    int quality; ///< quality in numbers for the item
    int quantity; ///< quantity of the item

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataItem()
        : AdminCmdDataTarget("/item", ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_STRING), random(false), quality(50), quantity(1)
    {}

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataItem(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataItem()
    {}

    /** @brief Creates an object of the current class containing parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for key (lock) related commands.
 *
 * Can create/destroy locks, lock/unlock them, create/destroy keys.
 */
class AdminCmdDataKey : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of sub commands
    AdminCmdSubCommandParser subTargetCommandList; ///< list of sub commands requiring a target
    csString subCommand; ///< subcommand storage

    /** @brief Creates obj for specified command managing locks.
     */
    AdminCmdDataKey()
        : AdminCmdDataTarget("/key", ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_CLIENTTARGET),
          subCommandList("make makemaster copy clearlocks skel"),
          subTargetCommandList("changelock makeunlockable securitylockable addlock removelock")
    {};

    /** @brief Parses the given lock manage message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataKey(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataKey()
    {};

    /** @brief Creates an object containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for a command running a script.
 */
class AdminCmdDataRunScript : public AdminCmdDataTarget
{
public:
    csString scriptName; ///< name of the script to execute
    AdminCmdTargetParser origObj; ///< Optional argument to specify a different origin for the script being run.

    /** @brief Creates obj for specified command that execute a script.
     */
    AdminCmdDataRunScript()
        : AdminCmdDataTarget("/runscript", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET), origObj(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataRunScript(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataRunScript()
    {};

    /** @brief Creates an object containing the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Base class for creating treasure hunts.
 */
class AdminCmdDataCrystal : public AdminCmdDataTarget
{
public:
    int interval; ///< respawn interval ??
    int random; ///< ??
    int amount; ///< amount of items
    float range; ///< spawn range ??
    csString itemName; ///< name of the item to hunt

    /** @brief Creates obj for specified command that define a treasure hunt.
     */
    AdminCmdDataCrystal()
        : AdminCmdDataTarget("/hunt_location", ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_STRING)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataCrystal(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataCrystal()
    {};

    /** Creates an object containing the parsed data for a treasure hunt.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for teleport command.
 */
class AdminCmdDataTeleport : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser destList; ///< list of destination commands

    /** when specified at the cmd line, that an ingame object is the destination
     */
    AdminCmdTargetParser destObj;
    InstanceID destInstance; ///< id for the destination instance (when given)
    bool destInstanceValid; ///< true when the instance is valid
    csString dest; ///< destination command (when not a generic target)
    csString destMap; ///< mapname
    csString destSector; ///< sectorname
    float x; ///< point to teleport to x coordinate
    float y; ///< point to teleport to y coordinate
    float z; ///< point to teleport to z coordinate

    /** @brief Creates obj for teleport command data
     */
    AdminCmdDataTeleport()
        : AdminCmdDataTarget("/teleport", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_DATABASE), destList("here there last spawn restore"), destObj(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM)
    {};

    /** @brief Parses the given message and stores its data for teleporting.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataTeleport(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataTeleport()
    {};

    /** @brief Creates an object containing the parsed data for teleporting.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData*: pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for sliding a player command.
 *
 * Sliding is some sort of moving the player around.
 */
class AdminCmdDataSlide : public AdminCmdDataTarget
{
public:
    csString direction; ///< direction where the slide should done
    float slideAmount; ///< the distance to slide

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataSlide()
        : AdminCmdDataTarget("/slide", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_OBJECT | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_CLIENTTARGET), slideAmount(0)
    {}

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSlide(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSlide()
    {};

    /** @brief Creates an object the parsed data for sliding.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for name changing of a player.
 */
class AdminCmdDataChangeName : public AdminCmdDataTarget
{
public:
    bool uniqueName; ///< whether the name should be unique
    bool uniqueFirstName; ///< whether the first name should be unique
    csString newName; ///< new Firstname
    csString newLastName; ///< new Lastname

    /** @brief Creates obj for specified command that changes a players name.
     */
    AdminCmdDataChangeName()
        : AdminCmdDataTarget("/changename", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE), uniqueName(true), uniqueFirstName(true)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataChangeName(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataChangeName()
    {};

    /** @brief Creates an object containing the parsed data for a name change.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};


/** @brief Class for name changing a guild.
 */
class AdminCmdDataChangeGuildName : public AdminCmdData
{
public:
    csString guildName; ///< name of the guild
    csString newName; ///< new guildname

    /** Creates obj for specified command that changes a guild name.
     */
    AdminCmdDataChangeGuildName()
        : AdminCmdData("/changeguildname")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataChangeGuildName(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataChangeGuildName()
    {};

    /** Creates an object containing the parsed data for a guildname change.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for guild leader changing command.
 */
class AdminCmdDataChangeGuildLeader : public AdminCmdDataTarget
{
public:
    csString guildName; ///< guildname whos leader is to change

    /** @brief Creates obj for specified command that changes a guild leader.
     */
    AdminCmdDataChangeGuildLeader()
        : AdminCmdDataTarget("/changeguildleader", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataChangeGuildLeader(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataChangeGuildLeader()
    {};

    /** Creates an object containing the parsed data for a guildleader change.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for petitions.
 */
class AdminCmdDataPetition : public AdminCmdData
{
public:
    csString petition; ///< text of the petition

    /** @brief Creates obj for specified command that creates petitions.
     */
    AdminCmdDataPetition()
        : AdminCmdData("/petition")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataPetition(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataPetition()
    {};

    /** @brief Creates an object containing the parsed data of a petition.
    * @param msgManager message manager that handles this command
    * @param msg psAdminCmdMessage containing the message
    * @param me The incoming message from the GM
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for impersonation command.
 *
 * Basically impersonation is for sending text in the name of someone else
 */
class AdminCmdDataImpersonate : public AdminCmdDataTarget
{
public:
    csString commandMod; ///< whether say,shout,worldshout should be used
    csString text; ///< text to send impersonated
    int duration; ///< The duration of the anim if any (0 default one run)

    /** @brief Creates obj for specified command that impersonates someone.
     */
    AdminCmdDataImpersonate()
        : AdminCmdDataTarget("/impersonate", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_OBJECT)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataImpersonate(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataImpersonate()
    {};

    /** @brief Creates an object containing the parsed data for impersonate.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for deputize command.
 *
 * Elevates the access rights of a player temporarily.
 */
class AdminCmdDataDeputize : public AdminCmdDataTarget
{
public:
    csString securityLevel; ///< security level to grant temporarily

    /** @brief Creates obj for specified command that elevates someones rights.
     */
    AdminCmdDataDeputize()
        : AdminCmdDataTarget("/deputize", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataDeputize(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataDeputize()
    {};

    /** @brief Creates an object containing the parsed data for deputizing.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for awarding rewards command.
 */
class AdminCmdDataAward : public AdminCmdDataTarget
{
public:
    AdminCmdRewardParser rewardList; ///< rewards parser and storage

    /** @brief Creates obj for specified command that awards rewards.
     */
    AdminCmdDataAward()
        : AdminCmdDataTarget("/award", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataAward(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataAward()
    {};

    /** @brief Creates an object containing the parsed data for rewards.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for transfering items.
 */
class AdminCmdDataItemTarget : public AdminCmdDataTarget
{
public:
    csString itemName; ///< name of the item to transfer
    int stackCount; ///< amount to transfer

    /** @brief Creates obj for specified command that transfers items.
     * @param command name of the command (e.g. /ban)
     */
    AdminCmdDataItemTarget(csString command)
        : AdminCmdDataTarget(command, ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA |ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET), stackCount(0)
    {};

    /** @brief Parses the given message and stores its data.
     * @param command name of the command (e.g. /ban)
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataItemTarget(csString command, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataItemTarget()
    {};

    /** @brief Creates an object containing the parsed data for item transfer.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for item check command.
 */
class AdminCmdDataCheckItem : public AdminCmdDataItemTarget
{
public:
    /** @brief Creates obj for specified item check command.
     */
    AdminCmdDataCheckItem()
        : AdminCmdDataItemTarget("/checkitem")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataCheckItem(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataCheckItem()
    {};

    /** @brief Creates an object containing the parsed data for an item check.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for commands with a sector target.
 */
class AdminCmdDataSectorTarget : public AdminCmdData
{
public:
    bool isClientSector; ///< true when sector of client is stored
    csString sectorName;    ///< name of the sector (when parsed successfully)
    psSectorInfo* sectorInfo; ///< sector either found by sectorname or client position

    /** @brief Creates obj for specified command that target a sector.
     * @param command name of the command (e.g. /ban)
     */
    AdminCmdDataSectorTarget(csString command)
        : AdminCmdData(command), isClientSector(false), sectorInfo(NULL)
    {};

    /** @brief Parses the given message and stores its data.
     * @param command name of the command (e.g. /ban)
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSectorTarget(csString command, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSectorTarget()
    {};

    /** @brief Creates an object containing the parsed data targeting a sector.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();

    /** @brief Tries to parse the supplied string as a destination.
      * @param msgManager message manager that handles this command
      * @param me The incoming message from the GM
      * @param msg psAdminCmdMessage containing the message
      * @param client client of the network communication
      * @param target string to parse as a destination
      * @return false when parsing the supplied string failed, otherwise true
      */
    virtual bool ParseTarget(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, csString target);


    /** @brief Retrieves the current sector of the command issuing client.
     * @param client is the gm client issuing the command
     * @return bool: true when sector was successfully found, otherwise false.
     */
    virtual bool GetSectorOfClient(Client* client);
};

/** @brief Class for weather command.
 */
class AdminCmdDataWeather : public AdminCmdDataSectorTarget
{
public:
    bool enabled; ///< whether to enable/disable the effect
    int type; ///< Which weather effect we should disable or enable

    /** Creates obj for specified command that change the weather.
     */
    AdminCmdDataWeather()
        : AdminCmdDataSectorTarget("/weather"), enabled(false), type(0)
    {};

    /** Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataWeather(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataWeather()
    {};

    /** Creates an object containing the parsed data for weather control.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Base class for manipulating weather effects.
 */
class AdminCmdDataWeatherEffect : public AdminCmdDataSectorTarget
{
public:
    bool enabled; ///< whether the weather effect is enabled or not
    int particleCount; ///< number of particles for the effect
    int interval; ///< lenght of the weather effect
    int fadeTime; ///< in millisecs the time to have the particles fade

    /** @brief Creates obj for specified command that change weather effects.
     * @param command name of the command (e.g. /ban)
     */
    AdminCmdDataWeatherEffect(csString command)
        : AdminCmdDataSectorTarget(command), enabled(false), particleCount(4000), interval(600000), fadeTime(10000)
    {};

    /** @brief Parses the given message and stores its data.
     * @param command name of the command (e.g. /ban)
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataWeatherEffect(csString command, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataWeatherEffect()
    {};

    /** @brief Creates an object containing the parsed weather effect data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for fog command.
 */
class AdminCmdDataFog : public AdminCmdDataSectorTarget
{
public:
    bool enabled; ///< whether the weather effect is enabled or not
    int density; ///< density of the fog
    int fadeTime; ///< in millisecs the time to have the particles fade
    int interval; ///< lenght of the weather effect
    int r; ///< fog color, red component
    int g; ///< fog color, green component
    int b; ///< fog color, blue component

    /** @brief Creates obj for specified command that control fog.
     */
    AdminCmdDataFog()
        : AdminCmdDataSectorTarget("/fog"), enabled(false), density(200), fadeTime(10000), interval(0), r(200), g(200), b(200)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataFog(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataFog()
    {};

    /** @brief Creates an object containing the parsed data for fog.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for modifying command
 */
class AdminCmdDataModify : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser attributeList;
    csString subCommand; ///< sub command envoked
    int interval; ///< interval value
    int maxinterval; ///< maximum
    int amount; ///< amount
    int range; ///< range
    float x; ///< movement related data, x coordinate
    float y; ///< movement related data, y coordinate
    float z; ///< movement related data, z coordinate
    float rot; ///< movement related data, rotation
    csString skillName; ///< name of the skill picked
    int level; ///< level for a skill
    bool enabled; ///< enable or disable a setting

    /** @brief Creates obj for specified command that modifies an item.
     */
    AdminCmdDataModify()
        : AdminCmdDataTarget("/modify", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), attributeList("pickupable unpickable transient npcowned collide settingitem")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataModify(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataModify()
    {};

    /** @brief Creates an object containing the parsed data for item modify.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for morph command.
 */
class AdminCmdDataMorph : public AdminCmdDataTarget
{
public:
    csString subCommand; ///< subcommand (if any)
    csString raceName;   ///< name of the race to morph to
    csString genderName; ///< name of the gender to morph to
    csString scale;      ///< optional scale

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataMorph()
        : AdminCmdDataTarget("/morph", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataMorph(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataMorph()
    {};

    /** @brief Creates an object containing the parsed data for morphing.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for scale command.
 */
class AdminCmdDataScale : public AdminCmdDataTarget
{
public:
    csString subCommand; ///< subcommand (if any)
    csString scale;      ///< optional scale

    /** @brief Creates obj for specified command that needs a reason.
     */
    AdminCmdDataScale()
        : AdminCmdDataTarget("/scale", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_EID)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataScale(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataScale()
    {};

    /** @brief Creates an object containing the parsed data for morphing.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};


/** @brief Class for setskill command.
 */
class AdminCmdDataSetSkill : public AdminCmdDataTarget
{
public:
    /** target parser for possible source player
     * can also be optional, but then the target player must be specified.
     */
    AdminCmdTargetParser sourcePlayer;
    csString subCommand; ///< subcommand (copy)
    psRewardDataSkill skillData; ///< data about the skill to set
    /*
    csString skillName; ///< Name of the skill to set
    int value; ///< value of the skill
    bool relative; ///< when the value should be relative
    */

    /** @brief Creates obj for specified command that change skill levels.
     */
    AdminCmdDataSetSkill()
        : AdminCmdDataTarget("/setskill", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), sourcePlayer(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET), subCommand(), skillData("",0,0,false)
    {};

    /** Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetSkill(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetSkill()
    {};

    /** @brief Creates an object containing the parsed data for skill levels.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for setting attributes on a player.
 */
class AdminCmdDataSet : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of valid subcommands
    AdminCmdSubCommandParser attributeList; ///< list of valid attributes
    AdminCmdOnOffToggleParser setting; ///< the specified setting on|off|toggle

    csString subCommand; ///< the parsed subcommand if any
    csString attribute; ///< the parsed attribute name if any

    /** @brief Creates obj for specified command that set attribute values.
     */
    AdminCmdDataSet()
        : AdminCmdDataTarget("/set", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_ME), subCommandList("list gm player"), attributeList("invincible invincibility invisibility invisible viewall nevertired nofalldamage infiniteinventory questtester infinitemana instantcast givekillexp attackable buddyhide"), setting()
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSet(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSet()
    {};

    /** @brief Creates an object containing the parsed data for attributes.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for setting labels.
 */
class AdminCmdDataSetLabelColor : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser labelTypeList; ///< list of valid label types

    csString labelType; ///< name of the label 'color'

    /** @brief Creates obj for specified command setting labels.
     */
    AdminCmdDataSetLabelColor()
        : AdminCmdDataTarget("/setlabelcolor", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA  | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_EID), labelTypeList("normal alive dead npc tester gm gm1 player")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetLabelColor(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetLabelColor()
    {};

    /** @brief Creates an object containing the parsed data for labelchange.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for some action (guildentrance creation).
 */
class AdminCmdDataAction : public AdminCmdDataSectorTarget
{
public:
    AdminCmdSubCommandParser subCommandList; ///< possible sub commands
    csString subCommand; ///< storage for the given subcommand
    csString guildName; ///< guildname for create_entrance
    csString description; ///< description for the guild entrance

    /** @brief Creates obj for specified command creating a guildentrance.
     */
    AdminCmdDataAction()
        : AdminCmdDataSectorTarget("/action"), subCommandList("create_entrance")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataAction(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataAction()
    {};

    /** @brief Creates an object containing the parsed data for the action.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for location command
 */
class AdminCmdDataLocation : public AdminCmdData
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of subcommands
    csString subCommand; ///< subcommand storage
    csString locationType; ///< type of the location
    csString locationName; ///< name of the location
    float radius; ///< radius of the location
    float searchRadius; ///< The radius to search for the location.
    float rotAngle; ///< rotation angle for this location

    /** @brief Creates obj for specified command that needs a reason
     */
    AdminCmdDataLocation()
        : AdminCmdData("/location"), subCommandList("help adjust display hide")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataLocation(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataLocation()
    {};

    /** @brief Creates an object the parsed data.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for handling path commands.
 */
class AdminCmdDataPath : public AdminCmdData
{
public:
    AdminCmdSubCommandParser subCommandList; ///< allowed subcommands
    AdminCmdSubCommandParser aliasSubCommandList; ///< subsubcmds of alias
    AdminCmdSubCommandParser wpList; ///< targetnames of hide/show
    csString subCmd;    ///< storage for subcommand
    csString aliasSubCmd; ///< storage for alias subcommand
    bool wpOrPathIsWP; ///< Is it the path or the wp that should be updated.
    float defaultRadius; ///< default radius for the case radius == 0.0
    float radius; ///< radius for waypoint/path operations
    float newRadius; ///< The radius to set for new radius for wps.
    csString waypoint; ///< waypoint name
    csString alias; ///< waypoint alias name
    csString flagName; ///< single flag name
    csString waypointPathName; ///< pathname for waypoints
    int firstIndex; ///< index for waypoint paths
    csString waypointFlags; ///< waypoint flags
    csString pathFlags; ///< path flags
    csString cmdTarget; ///< 'W' or 'P' as a target for hide/show
    int waypointPathIndex; ///< Index of the wp or point
    bool wpOrPathIsIndex; ///< True when an index is used.
    float rotationAngle; ///< The rotation angle of a alias


    /** @brief Creates obj for specified command that manipulate paths.
     */
    AdminCmdDataPath()
        : AdminCmdData("/path"), subCommandList("adjust alias display format help hide info point start stop"), aliasSubCommandList("add remove"), wpList("waypoints points"), subCmd(), defaultRadius(2.0)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataPath(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataPath()
    {};

    /** @brief Creates an object containing the parsed data for paths.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for game event command
 */
class AdminCmdDataGameMasterEvent : public AdminCmdData
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of subcommands
    csString subCmd; ///< subcommand storage

    AdminCmdTargetParser player; ///< when a playertarget needs to be parsed
    AdminCmdRewardParser rewardList; ///< rewards parser and storage

    csString gmeventName; ///< name of the event
    csString gmeventDesc; ///< description of the event

    csString commandMod; ///< subsubcommand
    RangeSpecifier rangeSpecifier; ///< range type
    float range; ///< range to search for targets

    /** @brief Creates obj for specified command that concern game events.
     */
    AdminCmdDataGameMasterEvent()
        : AdminCmdData("/event"), subCommandList("help list"), subCmd(), player(ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataGameMasterEvent(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataGameMasterEvent()
    {};

    /** @brief Creates an object containing the parsed data for an event.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for game event command
 */
class AdminCmdDataHire : public AdminCmdData
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of subcommands
    csString  subCmd;      ///< subcommand storage
    csString  typeName;    ///< type name storage
    csString  typeNPCType; ///< type NPC type storage
    PID       masterPID;   ///< master PID storage

    gemActor* owner;
    gemNPC*   hiredNPC;

    /** @brief Creates obj for specified command that concern hire.
     */
    AdminCmdDataHire()
        : AdminCmdData("/hire"), subCommandList("help list"), subCmd(), owner(NULL), hiredNPC(NULL)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataHire(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataHire()
    {};

    /** @brief Creates an object containing the parsed data for hire.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for searching for bad npc text.
 */
class AdminCmdDataBadText : public AdminCmdDataTarget
{
public:
    int first; ///< first textline to fetch from npc
    int last; ///< last textline to fetch

    /** @brief Creates obj for specified command examine npc dialogs.
     */
    AdminCmdDataBadText()
        : AdminCmdDataTarget("/badtext", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_NPC | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataBadText(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataBadText()
    {};

    /** @brief Creates an object containing the parsed data for npc texts.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for examing/modifying quests.
 */
class AdminCmdDataQuest : public AdminCmdDataTarget
{
public:
    AdminCmdSubCommandParser subCommandList; ///< list of allowed subcommands
    csString subCmd; ///< possibly one subcommand here
    csString questName; ///< name of quest to modify/examine

    /** @brief Creates obj for specified command that examine/modify quests.
     */
    AdminCmdDataQuest()
        : AdminCmdDataTarget("/quest", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE),
          subCommandList("complete list discard assign")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataQuest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataQuest()
    {};

    /** @brief Creates an object containing the parsed data for the quest.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for setting quality for an item command.
 */
class AdminCmdDataSetQuality : public AdminCmdDataTarget
{
public:
    int quality; ///< quality for the item
    int qualityMax; ///< qualitymax for the item

    /** @brief Creates obj for specified command that changes item quality.
     */
    AdminCmdDataSetQuality()
        : AdminCmdDataTarget("/setquality", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA |ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetQuality(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetQuality()
    {};

    /** @brief Creates an object containing the parsed data item quality.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for setting traits or retrieving info
 */
class AdminCmdDataSetTrait : public AdminCmdDataTarget
{
public:
    csString subCmd; ///< subcommand
    csString race; ///< specifier for the race of the character
    PSCHARACTER_GENDER gender; ///< gender of the character
    csString traitName; ///< name of the trait to add (if adding is possible)

    /** @brief Creates obj for specified command that sets/displays traits.
     */
    AdminCmdDataSetTrait()
        : AdminCmdDataTarget("/settrait", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetTrait(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetTrait()
    {};

    /** @brief Creates an object containing the parsed data for traits.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for setting item names and descriptions
 */
class AdminCmdDataSetItem : public AdminCmdDataTarget
{
public:
    csString name; ///< name for the item
    csString description; ///< description for the item

    /** @brief Creates obj for specified command changing item name/description
     */
    AdminCmdDataSetItem()
        : AdminCmdDataTarget("/setitemname", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ITEM | ADMINCMD_TARGET_EID |ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetItem(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetItem()
    {};

    /** @brief Creates an object containing the parsed data for item naming.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for reloading item stats.
 */
class AdminCmdDataReload : public AdminCmdData
{
public:
    csString subCmd; ///< sub command
    int itemID; ///< item id
    AdminCmdSubCommandParser subCommandList; ///< list of subcommands

    /** @brief Creates obj for specified command that reloads item stats.
     */
    AdminCmdDataReload()
        : AdminCmdData("/reload"), itemID(0)
    {};

    /** Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataReload(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataReload()
    {};

    /** @brief Creates an object containing the parsed data for item reloading.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for issuing player warnings.
 */
class AdminCmdDataListWarnings : public AdminCmdDataTarget
{
public:
    /** @brief Creates obj for specified command that issues a player warning.
     */
    AdminCmdDataListWarnings()
        : AdminCmdDataTarget("/listwarnings", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_CLIENTTARGET | ADMINCMD_TARGET_DATABASE)
    {};

    /** Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataListWarnings(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataListWarnings()
    {};

    /** @brief Creates an object containing the parsed data for warnings.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for disabling/enabling a quest.
 */
class AdminCmdDataDisableQuest : public AdminCmdData
{
public:
    csString questName; ///< quest to disable
    bool saveToDb; ///< whether to save this to the db or not

    /** @brief Creates obj for specified command for quest dis-/en-abling.
     */
    AdminCmdDataDisableQuest()
        : AdminCmdData("/disablequest")
    {};

    /** Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataDisableQuest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataDisableQuest()
    {};

    /** @brief Creates an object the parsed data for quest dis-/en-abling.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for setting the killing exp. for a character.
 */
class AdminCmdDataSetKillExp : public AdminCmdDataTarget
{
public:
    int expValue; ///< experience to set

    /** @brief Creates obj for specified command that sets the killing exp.
     */
    AdminCmdDataSetKillExp()
        : AdminCmdDataTarget("/setkillexp", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSetKillExp(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSetKillExp()
    {};

    /** @brief Creates an object the parsed data for killing exp.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for assigning awards for factions.
 */
class AdminCmdDataAssignFaction : public AdminCmdDataTarget
{
public:
    csString factionName; ///< faction name to assign
    int factionPoints; ///< faction points to assign

    /** @brief Creates obj for specified command that assigns factions.
     */
    AdminCmdDataAssignFaction()
        : AdminCmdDataTarget("/assignfaction", ADMINCMD_TARGET_TARGET | ADMINCMD_TARGET_PID | ADMINCMD_TARGET_AREA | ADMINCMD_TARGET_ME | ADMINCMD_TARGET_PLAYER | ADMINCMD_TARGET_EID | ADMINCMD_TARGET_CLIENTTARGET)
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataAssignFaction(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataAssignFaction()
    {};

    /** @brief Creates an object containing the parsed data for factions.
    * @param msgManager message manager that handles this command
    * @param me The incoming message from the GM
    * @param msg psAdminCmdMessage containing the message
    * @param client client of the network communication
    * @param words command message to parse
    * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for terminating the server.
 */
class AdminCmdDataServerQuit : public AdminCmdData
{
public:
    csString reason; ///< reason for the shutdown
    int time; ///< time until shutdown

    /** @brief Creates obj for specified command that ends the server.
     */
    AdminCmdDataServerQuit()
        : AdminCmdData("/serverquit")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataServerQuit(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataServerQuit()
    {};

    /** @brief Creates an object containing the parsed data for server stop.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Class for terminating the server.
 */
class AdminCmdDataNPCClientQuit : public AdminCmdData
{
public:
    /** @brief Creates obj for specified command that ends the server.
     */
    AdminCmdDataNPCClientQuit()
        : AdminCmdData("/npcclientquit")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataNPCClientQuit(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataNPCClientQuit()
    {};

    /** @brief Creates an object containing the parsed data for server stop.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/** @brief Simple one word commands.
 */
class AdminCmdDataSimple : public AdminCmdData
{
public:
    /** @brief Creates obj for specified command that ends the server.
     */
    AdminCmdDataSimple(csString commandName)
        : AdminCmdData(commandName)
    {};

    /**
     * Parses the given message and stores its data.
     *
     * @param commandName name of the command
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataSimple(csString commandName, AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataSimple()
    {};

    /** @brief Creates an object containing the parsed data for server stop.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /** @brief Returns a helpmessage that fits to the parser of the class.
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};


/** @brief Class for sending sequenced test message.
 */
class AdminCmdDataRndMsgTest : public AdminCmdData
{
public:
    csString text; ///< msg to test send
    bool sequential; ///< true is for sending messages in sequende, otherwise randomly

    /** @brief Creates obj for specified command sending test message
     */
    AdminCmdDataRndMsgTest()
        : AdminCmdData("/rndmsgtest")
    {};

    /** @brief Parses the given message and stores its data.
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataRndMsgTest(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataRndMsgTest()
    {};

    /**
     * Creates an object the parsed data for the test message.
     *
     * @param msgManager message manager that handles this command
     * @param me The incoming message from the GM
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
    */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /**
     * Returns a helpmessage that fits to the parser of the class.
     *
     * @return csString: a help message to send back to the client
     */
    virtual csString GetHelpMessage();
};

/**
 * Class for list command.
 *
 * Currently lists only available maps.
 */
class AdminCmdDataList : public AdminCmdData
{
public:
    AdminCmdSubCommandParser subCommandList; ///< List of possible sub commands
    csString subCommand; ///< given subcommand

    /**
     * Creates obj for specified command that needs a reason
     */
    AdminCmdDataList()
        : AdminCmdData("/list"), subCommandList("map")
    {};

    /**
     * Parses the given message and stores its data.
     *
     * @param msgManager message manager that handles this command
     * @param me message entry
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataList(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataList()
    {};

    /**
     * Creates a command data object of the current class containing the parsed data.
     *
     * @param msgManager message manager that handles this command
     * @param me message entry
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /**
     * Returns the Helpmessageportion for the destination according to the type
     *
     * @return csString containg the helpmessage part for destination
     */
    virtual csString GetHelpMessage();
};

/**
 * Class for time command.
 *
 * Currently lists only available maps.
 */
class AdminCmdDataTime : public AdminCmdData
{
public:
    AdminCmdSubCommandParser subCommandList; ///< List of possible sub commands
    csString subCommand; ///< given subcommand
    int hour,minute;

    /** Creates obj for specified command that needs a reason
     */
    AdminCmdDataTime()
        : AdminCmdData("/time"), subCommandList("show set"), hour(0), minute(0)
    {}

    /**
     * Parses the given message and stores its data.
     *
     * @param msgManager message manager that handles this command
     * @param me message entry
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     */
    AdminCmdDataTime(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    virtual ~AdminCmdDataTime()
    {};

    /**
     * Creates a command data object of the current class containing the parsed data.
     *
     * @param msgManager message manager that handles this command
     * @param me message entry
     * @param msg psAdminCmdMessage containing the message
     * @param client client of the network communication
     * @param words command message to parse
     * @return AdminCmdData* pointer to object containing parsed data. When parsing failed the valid flag is set to false.
     */
    virtual AdminCmdData* CreateCmdData(AdminManager* msgManager, MsgEntry* me, psAdminCmdMessage &msg, Client* client, WordArray &words);

    /**
     * Returns the Helpmessageportion for the destination according to the type.
     *
     * @return csString containg the helpmessage part for destination
     */
    virtual csString GetHelpMessage();
};


/**
 * Class containing object factory for AdminCmdData objects.
 *
 * Creates specific AdminCmdData objects on demand.
 * The objects are stored and accessed by the command name that they have
 * stored internally.
 * AdminCmdData objects can be registered, so that objects of their type
 * can be generated.
 *
 */
class AdminCmdDataFactory
{
public:
    AdminCmdDataFactory();
    virtual ~AdminCmdDataFactory();

    /** @brief Search for a specific command data.
     * @param datatypename Name of the command to search for (including /)
     * @return AdminCmdData is the search result or null if not found.
     */
    AdminCmdData* FindFactory(csString datatypename);

    /*/* @brief Register a new AdminCmdData for a command.
     * @param func Factory function that creates data objects for the command
     * @param datatypename Name of the command to search for (including /)
     */
    //void RegisterMsgFactoryFunction(AdminCmdMsgFactoryFunc func, const char* datatypename);

    /** @brief Register a new AdminCmdData class.
     * @param obj for a new AdminCmdData class
     */
    void RegisterMsgFactoryFunction(AdminCmdData* obj);

protected:
    csPDelArray<AdminCmdData> adminCmdDatas; ///< storage for command data factory functions
};

/** @brief Admin manager that handles GM commands and general game control.
 */
class AdminManager : public MessageManager<AdminManager>, public iEffectIDAllocator
{
public:
    AdminManager();
    virtual ~AdminManager();

    /** @brief This is called when a player does /admin.
     *
     * This builds up the list of commands that are available to the player
     * at their current GM rank. This commands allows player to check the
     * commands of a different gm level (or not gm) provided they have some
     * sort of security level ( > 0 ) in that case the commands won't be
     * subscribed in the client.
     *
     * @param clientnum client identifier of message sending client
     * @param client object of the client sending message.
     * @param requestedLevel the requested level
     */
    void Admin(int clientnum, Client* client, int requestedLevel = -1);

    /** @brief This awards a certain amount of exp to the target.
      * @param gmClientnum ClientID of the issuer
      * @param target pointer to the Client of the target
      * @param ppAward amount of exp to award. might be negative to punish instead
      */
    void AwardExperienceToTarget(int gmClientnum, Client* target, int ppAward);

    /** @brief adjusts a faction standing of the target by a given value.
      * @param gmClientnum ClientID of the issuer
      * @param target pointer to the Client of the target
      * @param factionName name of the faction to adjust
      * @param standingDelta value to adjust the standing by
      */
    void AdjustFactionStandingOfTarget(int gmClientnum, Client* target, csString factionName, int standingDelta);

    /** @brief adjusts a skill of the target by a given value.
      * @param gmClientNum ClientID of the issuer
      * @param target pointer to the Client of the target
      * @param skill pointer to the info about the skill to adjust
      * @param value amount to set/adjust by
      * @param relative determines whether the value is absolute or relative
      * @param cap if nonzero the skill will be set to a value smaller or equal to the specified one
      * @return bool FALSE if an error occured
      */
    bool ApplySkill(int gmClientNum, Client* target, psSkillInfo* skill, int value, bool relative = false, int cap = 0);

    /** @brief universal function to award a target.
      * @param gmClientNum ClientID of the issuer
      * @param target pointer to the Client of the target
      * @param data struct holding the awards to apply
      * @see psRewardData
      */
    void AwardToTarget(unsigned int gmClientNum, Client* target, psRewardData* data);

    /**
     * Get sector and coordinates of starting point of a map.
     *
     * @return return the success (true when successful).
     */
    bool GetStartOfMap(int clientnum, const csString &map, iSector* &targetSector,  csVector3 &targetPoint);

    /** @brief wrapper for internal use from npc
     * @param pMsg Message sent by npc
     * @param client ??
     */
    void HandleNpcCommand(MsgEntry* pMsg, Client* client);

    /** @brief return the path network
     */
    psPathNetwork* GetPathNetwork()
    {
        return pathNetwork;
    }

    LocationManager* GetLocationManager()
    {
        return locations;
    }

protected:
    /** Data object factory for parsing command data
     */
    AdminCmdDataFactory* dataFactory;

    /** @brief Test the first 8 chars after the command for 'me reset'.
     * @param command the command string to test for 'me reset'.
     * @return bool: true when 'me reset' is found, otherwise false.
     */
    bool IsReseting(const csString &command);

    /** @brief Parses message and executes contained admin command
     * @param pMsg Message sent by gm client
     * @param client sending gm client
     */
    void HandleAdminCmdMessage(MsgEntry* pMsg, Client* client);

    /** @brief Parses and executes petition related commands
     * @param pMsg Message sent by gm client
     * @param client sending gm client
     */
    void HandlePetitionMessage(MsgEntry* pMsg, Client* client);

    /** @brief Parses and executes commands sent by a gm GUI
     * @param pMsg gm GUI message sent by gm client
     * @param client sending gm client
     */
    void HandleGMGuiMessage(MsgEntry* pMsg, Client* client);

    /** @brief Parses and executes commands for spawning items for a gm.
     *
     * Internally a psGMSpawnItem struct is created for calling the
     * the other SpawnItemInv function.
     * @param me The incoming message from the GM
     * @param client sending gm client
     */
    void SpawnItemInv(MsgEntry* me, Client* client);

    /** @brief Parses and executes commands for spawning items for a gm.
     * @param me The incoming message from the GM
     * @param msg gm item spawn message sent by gm client
     * @param client sending gm client
     */
    void SpawnItemInv(MsgEntry* me, psGMSpawnItem &msg, Client* client);

    /** @brief Handles a request to reload a quest from the database.
     *  @param msg The text name is in the msg.text field.
     * @param data A pointer to the command parser object with target data
     *  @param client The client we will send error codes back to.
     */
    void HandleLoadQuest(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Get the list of characters in the same account of the provided one.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param data A pointer to the command parser object with target data
     *  @param client The GM client the command came from.
     */
    void GetSiblingChars(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);


    /** @brief Retrieves information like ID, etc. about a target.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void GetInfo(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* data,Client* client);


    /** @brief Creates a new NPC by copying data from a master NPC.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void CreateNPC(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Kill an npc in order to make it respawn, It can be used also to reload the data of the npc from the database
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param data A pointer to the command parser object with target datat
     *  @param client The GM client the command came from.
     */
    void KillNPC(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Percept a NPC
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param data A pointer to the command parser object with target datat
     *  @param client The GM client the command came from.
     */
    void Percept(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Change the npctype (brain) of the npc.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param cmddata A pointer to the command parser object with target datat
     *  @param client The GM client the command came from.
     */
    void ChangeNPCType(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client);

    /** @brief Change the NPC Debug level.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param cmddata A pointer to the command parser object with target datat
     *  @param client The GM client the command came from.
     */
    void DebugNPC(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client);

    /** @brief Change the Tribe Debug level.
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param cmddata A pointer to the command parser object with target datat
     *  @param client The GM client the command came from.
     */
    void DebugTribe(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* cmddata, Client* client);

    /** @brief Creates an item or loads GUI for item creation.
     *
     * Gui for item creation is loaded when no item was specified.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void CreateItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Creates, destroys and changes keys and locks.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void ModifyKey(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Exchanges the lock on an item.
     *
     * This removes the ability of all keys of the 'original' lock
     * of unlocking this item.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void ChangeLock(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Toggles the lockability of a lock.
     *
     * A lockable lock is set to unlockable and an
     * unlockable lock is set to lockable.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void MakeUnlockable(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Toggles the lock to be a security lock or not.
     *
     * The security locked flag on an item is removed when it was already set,
     * otherwise the security locked flag is set.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void MakeSecurity(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);


    /** @brief Sets an item as a key or master key.
     *
     * The item needs to be placed in the right hand of the clients (gm)
     * character for the key creation.
     * Master key is a key that can unlock all locks.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     * @param masterkey true for masterkey otherwise false.
     */
    void MakeKey(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client, bool masterkey);

    /** @brief Creates a copy of a master key.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     * @param key to copy
     */
    void CopyKey(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client, psItem* key);

    /** @brief A lock is added or removed from the list of locks a key can unlock.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     * @param key to remove the lock from.
     */
    void AddRemoveLock(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client,psItem* key);

    /** @brief Runs a progression script on the targetted client
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param data A pointer to the command parser object with target data
     *  @param client The GM client the command came from.
     */
    void RunScript(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Creates a new hunt location.
     *
     * Hunt locations spawn items. The conditions like
     * how many are spawned is given by the command line options that
     * are parsed and stored by the AdminCmdDataCrystal object.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void CreateHuntLocation(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Changes various parameters associated to the item
     *  @param me The incoming message from the GM
     *  @param msg The cracked command message.
     *  @param data A pointer to the command parser object with target data
     *  @param client The GM client the command came from.
     */
    void ModifyItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Moves an object along a direction or turns it around the y axis.
     *
     * Movement is specified as a certain amount along one direction.
     * Direction can be: up, down, left, right, forward, backward.
     * Can also turn an object around the y axis.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Slide(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Teleport an offline character.
     * @param client The GM client the command came from.
     * @param data A pointer to the command parser object with target data
     */
    void TeleportOfflineCharacter(Client* client, AdminCmdDataTeleport* data);

    /** @brief Move an object to a certain position.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Teleport(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Get sector and coordinates of target of teleportation described by 'msg'.
     * @param client The GM client the command came from.
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param targetSector the target sector of the teleport.
     * @param targetPoint the target point to teleport to.
     * @param yRot is the rotation on the y-axis for the teleport.
     * @param subject is the target object to teleport
     * @param instance is the instance to teleport to.
     * @return true on success, otherwise fals
     */
    bool GetTargetOfTeleport(Client* client, psAdminCmdMessage &msg, AdminCmdData* data, iSector* &targetSector,  csVector3 &targetPoint, float &yRot, gemObject* subject, InstanceID &instance);

    /** @brief Handles movement of objects for teleport and slide.
     * @param client The GM client the command came from.
     * @param target The target object to move around.
     * @param pos The destination position.
     * @param yrot The y-axis rotation (if any).
     * @param sector The destination sector.
     * @param instance The destination instance.
     */
    bool MoveObject(Client* client, gemObject* target, csVector3 &pos, float yrot, iSector* sector, InstanceID instance);

    /** @brief This function sends a warning message from a GM to a player.
     *
     * It is displayed in the client GUI as a
     * big, red, un-ignorable text on the screen and in the chat window.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void WarnMessage(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief This function kicks a player off the server.
     *
     * The gm (client) needs to have sufficient priviledges.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void KickPlayer(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief This function will mute a player until logoff.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void MutePlayer(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief This function will unmute a player.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void UnmutePlayer(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Kills by doing a large amount of damage to target.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Death(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Impersonate a character.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Impersonate(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Set various GM/debug abilities.
     *
     * The setable abilites are: invisible, invincible, etc.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void SetAttrib(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Set the label color for char.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void SetLabelColor(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /** @brief Divorce char1 and char2, if they're married.
     * @param me The incoming message from the GM
     * @param data A pointer to the command parser object with target data
     */
    void Divorce(MsgEntry* me, AdminCmdData* data);

    /** @brief Get the marriage info of a player.
     *  @param me The incoming message from the GM
     *  @param data AdminCmdData object containing parsed data.
     */
    void ViewMarriage(MsgEntry* me, AdminCmdData* data);

    /** @brief Add new Path point to DB.
     * @param pathID the path to create the point for.
     * @param prevPointId of the (ancestor) previous point in the path.
     * @param pos vector specifying the point
     * @param sectorName is the name of the sector for the point.
     */
    int PathPointCreate(int pathID, int prevPointId, csVector3 &pos, csString &sectorName);

    /** @brief Lookup path information close to a point.
     *
     * Waypoints, paths or pointpaths can be retrieved.
     * When wp and pointPath pointers are given, then only the
     * nearest will be returned.
     * @param pos position of the point to lookup.
     * @param sector of the point to lookup.
     * @param radius is the searchrange around the point.
     * @param wp output of waypoints found
     * @param rangeWP distance of the waypoint to the point
     * @param path output of paths found
     * @param rangePath distance of the path to the point
     * @param indexPath index of the path
     * @param fraction ?
     * @param pointPath output of point of path
     * @param rangePoint range of the point of path to the point
     * @param indexPoint index of the point of path on the path
     */
    void FindPath(csVector3 &pos, iSector* sector, float radius,
                  Waypoint** wp, float* rangeWP,
                  psPath** path, float* rangePath, int* indexPath, float* fraction,
                  psPath** pointPath, float* rangePoint, int* indexPoint);

    /** Implement the abstract function from the iEffectIDAllocator
     */
    virtual uint32_t GetEffectID();

    /**
     * Hide all paths for all clients.
     *
     * @param clearSelected Clear selected path when iterating.
     */
    void HideAllPaths(bool clearSelected);

    /**
     * Hide all paths for a client.
     *
     * @param client The client to hide paths for.
     */
    void HidePaths(Client* client);

    /**
     * Hide all waypoints for a client.
     *
     * @param client The client to hide waypoints for.
     */
    void HideWaypoints(Client* client);

    /**
     * Hide all paths for a client in a sector.
     *
     * @param client The client to hide paths for.
     * @param sector The sector to hide paths in.
     */
    void HidePaths(Client* client, iSector* sector);

    /**
     * Hide all waypoints for a client in a sector.
     *
     * @param client The client ot hide waypoints for.
     * @param sector The sector to hide paths in.
     */
    void HideWaypoints(Client* client, iSector* sector);

    /**
     * Show paths and waypoints for all clients that have enabled display.
     */
    void RedisplayAllPaths();

    /**
     * Show paths for client in all sectors that has been enabled.
     *
     * @param client The client to show paths for.
     */
    void ShowPaths(Client* client);

    /**
     * Show waypoint for client in all sectors that has been enabled.
     *
     * @param client The client to show waypoints for.
     */
    void ShowWaypoints(Client* client);

    /**
     * Show path for a client in a given sector.
     *
     * @param client The client to show paths for.
     * @param sector The sector to show paths in.
     */
    void ShowPaths(Client* client, iSector* sector);

    /**
     * Show waypoints for a client in a given sector.
     *
     * @param client The client to show waypoints for.
     * @param sector The sector to show paths in.
     */
    void ShowWaypoints(Client* client, iSector* sector);

    /**
     * Update the display of paths in clients.
     */
    void UpdateDisplayPath(psPathPoint* point);

    /**
     * Update the display of waypoints in clients.
     */
    void UpdateDisplayWaypoint(Waypoint* wp);

    /**
     * Handle online path editing.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandlePath(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Add new location point to DB.
     *
     * @param typeID type of the location
     * @param pos position of the new location.
     * @param sectorName name of the sector for the new location.
     * @param name for the location.
     * @param radius for the location.
     */
    int LocationCreate(int typeID, csVector3 &pos, csString &sectorName, csString &name, int radius);

    /**
     * Handle display of locations
     */
    void ShowLocations(Client* client);

    /**
     * Handle display of locations
     */
    void ShowLocations(Client* client, iSector* sector);

    /**
     * Hide all locations for all clients.
     *
     * @param clearSelected Clear selected location when iterating.
     */
    void HideAllLocations(bool clearSelected);

    /** Hide all locations for one client.
     */
    void HideLocations(Client* client);

    /** Hide all locations in one sector for one client.
     */
    void HideLocations(Client* client, iSector* sector);

    /**
     * Show locations for all clients that have enabled display.
     */
    void RedisplayAllLocations();

    /** Update location to all clients displaying locations.
     */
    void UpdateDisplayLocation(Location* location);

    /**
     * Handle online path editing.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleLocation(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Handle action location entrances.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleActionLocation(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Handles a user submitting a petition.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleAddPetition(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data,Client* client);

    /**
     * Handles broadcasting the petition list dirty signal.
     *
     * Used by: CancelPetition, GMHandlePetition, HandleAddPetition
     * @param clientNum of the client issuing the command.
     * @param includeSelf Whether to include the sending client in the broadcast or not.
     */
    void BroadcastDirtyPetitions(int clientNum, bool includeSelf=false);

    /**
     * Gets the list of petitions and returns an array with the parsed data.
     *
     * @param petitions an array with the data of the petition: It's empty if it wasn't possible to obtain results.
     * @param client The GM client which requested the informations.
     * @param IsGMrequest manages if the list should be formated for gm or players. True is for gm.
     * @return true if the query succeded, false if there were errors.
     */
    bool GetPetitionsArray(csArray<psPetitionInfo> &petitions, Client* client, bool IsGMrequest = false);

    /**
     * Retrieves a list of petitions.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param client The GM client the command came from.
     */
    void ListPetitions(MsgEntry* me, psPetitionRequestMessage &msg, Client* client);

    /**
     * Cancels a petition.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param client The GM client the command came from.
     */
    void CancelPetition(MsgEntry* me, psPetitionRequestMessage &msg, Client* client);

    /**
     * Modifies a petition.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param client The GM client the command came from.
     */
    void ChangePetition(MsgEntry* me, psPetitionRequestMessage &msg, Client* client);

    /**
     * Handles GM queries for dealing with petitions.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param client The GM client the command came from.
     */
    void GMListPetitions(MsgEntry* me, psPetitionRequestMessage &msg, Client* client);

    /**
     * Handles GM changes to petitions.
     *
     * Possible changes are: canceling, closing, assigning, escalating,
     * deescalationg.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param client The GM client the command came from.
     */
    void GMHandlePetition(MsgEntry* me, psPetitionRequestMessage &msg, Client* client);

    /**
     * Lists all players for a GM.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param client The GM client the command came from.
     */
    void SendGMPlayerList(MsgEntry* me, psGMGuiMessage &msg, Client* client);

    /**
     * List command for information retrieval.
     *
     * Currently only supports listing the servers map.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleList(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data,Client* client);

    /**
     * Time command for setting of game time.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleTime(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data,Client* client);

    /**
     * Changes the name of the player to the specified one.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void ChangeName(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Changes the NPC's default spawn location.
     *
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void UpdateRespawn(AdminCmdData* data, Client* client);

    /**
     * Controls the weather.
     *
     * To turn it on/off.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Weather(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Controls the rain.
     *
     * Turn off, turn on, setting rain parameters.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Rain(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Controls the snow.
     *
     * Turn off, turn on, setting snow parameters.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Snow(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Controls the thunder.
     *
     * Sending a sound command to the clients.
     * Works only when it is raining.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Thunder(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Controls the rain.
     *
     * Turn off, turn on, setting rain parameters.
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Fog(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Deletes a character from the database.
     *
     * Should be used with caution.
     * This function will also send out reasons why a delete failed. Possible
     * reasons are not found or the requester is not the same account as the
     * one to delete.  Also if the character is a guild leader they must resign
     * first and assign a new leader.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void DeleteCharacter(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Bans a name from being used.
     *
     * The given name is added to the list of not allowed names
     * for characters.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void BanName(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Unbans a name from not being used.
     *
     * The given name is removed from the list of not allowed names
     * for characters.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void UnBanName(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Bans an account
     *
     * Bans can be done by player or account (pid,name,targeting).
     * Their length can be defined and set according to the security level of
     * the issuing GM.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void BanClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Unbans an account
     *
     * Unbanning can be done by player or account (pid,name).
     * Depending on the ban length, the unbanning is executed when
     * the gm has the right to issue the kind of ban.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void UnbanClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Bans an account from advising
     *
     * Bans can be done by player or account (pid,name,targeting).
     * Their length can be defined and set according to the security level of
     * the issuing GM.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */

    void BanAdvisor(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Unbans an account for advising
     *
     * Unbanning can be done by player or account (pid,name).
     * Depending on the ban length, the unbanning is executed when
     * the gm has the right to issue the kind of ban.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void UnbanAdvisor(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Retrieves a list of spawn types for from all items of the server.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void SendSpawnTypes(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Sends items of the specified type to the GMs client.
     *
     * @param me The incoming message from the GM
     * @param client The GM client the command came from.
     */
    void SendSpawnItems(MsgEntry* me, Client* client);

    /**
     * Sends item modifiers of the specified type to the GMs client.
     *
     * @param me The incoming message from the GM
     * @param client The GM client the command came from.
     */
    void SendSpawnMods(MsgEntry* me, Client* client);

    /**
     * Changes the name of a guild.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void RenameGuild(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Changes the leader of a guild.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void ChangeGuildLeader(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Sends information about an award (reward) to the target and GM.
     *
     * @param gmClientnum The GM awarding something.
     * @param target the client that should get the award information.
     * @param awardname is a name for the award.
     * @param awarddesc is a description of the award.
     * @param awarded > 0, means its a real award, <0 means it is a penalty.
     */
    void SendAwardInfo(size_t gmClientnum, Client* target, const char* awardname, const char* awarddesc, int awarded);

    /**
     * Awards experience to a player, by a GM.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     * @param target The target to award the experience to.
     */
    void AwardExperience(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client, Client* target);

    /**
     * Awards something to a player, by a GM.
     *
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.gm
     */
    void Award(AdminCmdData* data, Client* client);

    /**
     * Transfers an item from one client to another.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void TransferItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Checks the presence of items.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     */
    void CheckItem(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data);

    /**
     * Freezes a client, preventing it from doing anything.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void FreezeClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Thaws a client, reversing a freeze command.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void ThawClient(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Retrieves detailed information about a character.
     *
     * For a targeted player this lists the inventory of the player.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Inspect(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Changes the skill of the target.
     *
     * TODO: should be changed to alias /award
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void SetSkill(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Temporarily changes the mesh for a player.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Morph(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Temporarily changes the scale for an actor.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void Scale(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Temporarily changes the security level for a player.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void TempSecurityLevel(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Gets the given account number's security level from the DB.
     *
     * @param accountID to retrieve the security level for.
     * @return the security level of the specified account.
     */
    int GetTrueSecurityLevel(AccountID accountID);

    /**
     * Handle GM Event command.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleGMEvent(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Handle Hire command.
     *
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleHire(AdminCmdData* data, Client* client);

    /**
     * Handle request to view bad text from the targeted NPC.
     *
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleBadText(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Manipulate quests from characters.
     *
     * @param me incoming message from gm client
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleQuest(MsgEntry* me,psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Change quality of items.
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleSetQuality(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Decide whether an item is stackable or not.
     *
     * This is bypassing the flag for this type of item.
     * @param me incoming message from gm client
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void ItemStackable(MsgEntry* me, AdminCmdData* data, Client* client);

    /**
     * Set trait of a char.
     *
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleSetTrait(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Change name of items.
     *
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleSetItemName(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Handle reloads from DB.
     *
     * Currently can only reload items.
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleReload(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * List warnings given to account.
     *
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleListWarnings(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Shows what the target used in command will afflict in the game.
     *
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void CheckTarget(psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Allows to disable/enable quests temporarily (server cache) or definitely (database).
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void DisableQuest(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Allows to set an experience given to who kills the issing player.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void SetKillExp(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Allows to change faction points of players.
     *
     * @param me The incoming message from the GM.
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data.
     * @param client The target client which will have his faction points changed.
     */
    void AssignFaction(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Allows to quit/reboot the server remotely from a client.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleServerQuit(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Allows to quit/reboot the npcclient remotely from a client.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleNPCClientQuit(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Allows to get version the server remotely from a client.
     *
     * @param me The incoming message from the GM
     * @param msg The cracked command message.
     * @param data A pointer to the command parser object with target data
     * @param client The GM client the command came from.
     */
    void HandleVersion(MsgEntry* me, psAdminCmdMessage &msg, AdminCmdData* data, Client* client);

    /**
     * Adds a petition under the passed user's name to the 'petitions' table in the database.
     *
     * Will automatically add the date and time of the petition's submission
     * in the appropriate table columns
     * @param playerID: PID of the player who is submitting the petition.
     * @param petition: The player's request.
     * @return Returns either success or failure.
     */
    bool AddPetition(PID playerID, const char* petition);

    /**
     * Returns a list of all the petitions for the specified player.
     *
     * @param playerID: Is the ID of the player who is requesting the list.
     *                   if the ID is -1, that means a GM is requesting a complete listing
     * @param gmID: Is the id of the GM who is requesting petitions, ignored if playerID != -1
     * @return Returns a iResultSet which contains the set of all matching petitions for the user
     */
    iResultSet* GetPetitions(PID playerID, PID gmID = PETITION_GM);

    /**
     * Cancels the specified petition if the player was its creator.
     *
     * @param playerID: Is the ID of the player who is requesting the change.
     * @param petitionID: The petition id
     * @param isGMrequest: if true that means a GM is cancelling someone's petition
     * @return Returns either success or failure.
     */
    bool CancelPetition(PID playerID, int petitionID, bool isGMrequest = false);

    /**
     * Changes the description of the specified petition if the player was its creator.
     *
     * @param playerID: Is the ID of the player who is requesting the change.
     *                   if ID is -1, that means a GM is changing someone's petition
     * @param petitionID: The petition id
     * @param petition new petition text
     * @return Returns either success or failure.
     */
    bool ChangePetition(PID playerID, int petitionID, const char* petition);

    /**
     * Closes the specified petition (GM only).
     *
     * @param gmID: Is the ID of the GM who is requesting the close.
     * @param petitionID: The petition id
     * @param desc: the closing description
     * @return Returns either success or failure.
     */
    bool ClosePetition(PID gmID, int petitionID, const char* desc);

    /**
     * Assignes the specified GM to the specified petition.
     *
     * @param gmID: Is the ID of the GM who is requesting the assignment.
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool AssignPetition(PID gmID, int petitionID);

    /**
     * Deassignes the specified GM to the specified petition.
     *
     * @param gmID: Is the ID of the GM who is requesting the deassignment.
     * @param gmLevel: The security level of the gm
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool DeassignPetition(PID gmID, int gmLevel, int petitionID);

    /**
     * Escalates the level of the specified petition.
     *
     * Changes the assigned_gm to -1, and the status to 'Open'
     * @param gmID: Is the ID of the GM who is requesting the escalation.
     * @param gmLevel: The security level of the gm
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool EscalatePetition(PID gmID, int gmLevel, int petitionID);

    /**
     * Descalates the level of the specified petition.
     *
     * Changes the assigned_gm to -1, and the status to 'Open'
     * @param gmID: Is the ID of the GM who is requesting the descalation.
     * @param gmLevel: The security level of the gm
     * @param petitionID: The petition id
     * @return Returns either success or failure.
     */
    bool DescalatePetition(PID gmID, int gmLevel, int petitionID);

    /**
     * Logs all gm commands
     *
     * @param accountID: of the GM
     * @param gmID: the ID of the GM
     * @param playerID: the ID of the player
     * @param cmd: the command the GM executed
     * @return Returns either success or failure.
     */
    bool LogGMCommand(AccountID accountID, PID gmID, PID playerID, const char* cmd);

    /**
     * Messagetest sending 10 random messages to a client.
     *
     * Sends 10 messages in random order to the client for
     * testing sequential delivery.
     * @param client: Client to be the recepient of the messages
     * @param data A pointer to the command parser object with target data
     * @return void
     */
    void RandomMessageTest(AdminCmdData* data, Client* client);

    /**
     * Returns the last error generated by SQL
     *
     * @return Returns a string that describes the last sql error.
     * @see iConnection::GetLastError()
     */
    const char* GetLastSQLError();

    csString lasterror; ///< internal string used for formating and passing back errormessages

    ClientConnectionSet* clients; ///< internal list of clients connected to the server


    /**
     * Sends the gm client the current gm attributes.
     *
     * @param client of the GM.
     */
    void SendGMAttribs(Client* client);

    /**
     * Holds a dummy dialog.
     *
     * We may need this later on when NPC's are inserted.  This also
     * insures that the dicitonary will always exist.  There where some
     * problems with the dictionary getting deleted just after the
     * initial npc was added. This prevents that
     */
    psNPCDialog* npcdlg;

    /**
     * Holds the entire PathNetwork for editing of paths.
     */
    psPathNetwork* pathNetwork;

    /**
     * Hold every location in the world for editing of locations.
     */
    LocationManager* locations;
};

#endif
