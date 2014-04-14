/*
 * psguildinfo.h
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



#ifndef __PSGUILDINFO_H__
#define __PSGUILDINFO_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/refarr.h>
#include <csutil/refcount.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"

#include "rpgrules/psmoney.h"

//=============================================================================
// Local Includes
//=============================================================================


class psCharacter;
class iResultRow;

/**
 * \addtogroup bulkobjects
 * @{ */

/**
 * Privileges that may be specifically given/taken from members of a guild
 *
 * There are no restrictions on operations done by the \ref MAX_GUILD_LEVEL member.
 * The member at \ref MAX_GUILD_LEVEL (leader) can promote other members to
 * \ref MAX_GUILD_LEVEL which in turn demotes the leader to one below
 * \ref MAX_GUILD_LEVEL. A leader cannot simply demote his/herself. A leader cannot
 * leave a guild. Consequently each guild has <b>just</b> one \ref MAX_GUILD_LEVEL
 * member.
 */
enum GUILD_PRIVILEGE
{
    RIGHTS_VIEW_CHAT         = 1,      ///< User can view guild chat (command /guild)

    RIGHTS_CHAT              = 2,      ///< User can send guild chat messages (command /guild)

    RIGHTS_INVITE            = 4,      ///< User can invite players to guild

    RIGHTS_REMOVE            = 8,      ///< User can remove player of lower level from guild

    RIGHTS_PROMOTE           = 16,     ///< User can promote/demote players of lower level to another lower level

    RIGHTS_EDIT_LEVEL         = 32,    ///< User can change privileges of lower guild levels

    RIGHTS_EDIT_POINTS        = 64,    ///< User can change guild points of players of lower level

    RIGHTS_EDIT_GUILD         = 128,   ///< User can change guild name, guild secrecy, guild web page, max guild points
    ///< and can disband the guild

    RIGHTS_EDIT_PUBLIC        = 256,   ///< User can edit public notes of players with a lower level.
    ///< Public notes are visible to all guild members

    RIGHTS_EDIT_PRIVATE       = 512,   ///< User can view and edit private notes of players with a lower level.
    ///< Private notes are only visible to people with this flag.

    RIGHTS_VIEW_CHAT_ALLIANCE = 1024,  ///< User can view alliance chat (command /alliance /a)

    RIGHTS_CHAT_ALLIANCE      = 2048,  ///< User can send alliace chat messages (command /alliace /a)

    RIGHTS_USE_BANK           = 4096   ///< User can use guild bank
};

/**
 * Defines a level inside a guild.
 *
 * The level has a name and different flags for the privileges assigned to the level. <br\>
 * Database table: guildlevels
 */
struct psGuildLevel
{
    csString title;      ///< Name of the level.
    int      level;      ///< The rank of the level.
    int      privileges; ///< Bit field for the privileges.

    /**
     * Checks if the request right is possessed by this guild level
     *
     * @param rights The right that is being tested
     * @return True if this is the max guild level or \ref privileges has the bit
     */
    inline bool HasRights(GUILD_PRIVILEGE rights) const
    {
        return (level == MAX_GUILD_LEVEL) || (privileges & rights);
    }
};

//------------------------------------------------------------------------------

/**
 * Defines a guild member in a guild.
 *
 * Database tables: guilds, characters
 */
class psGuildMember
{
public:
    PID char_id;                ///< The character ID of the person.
    csString name;              ///< The name of the member
    psCharacter*  character;    ///< Pointer to the character data of the person.
    psGuildLevel* guildlevel;   ///< Members current level.
    int guild_points;           ///< Their points in the guild.
    csString public_notes;      ///< The public notes the member has.
    csString private_notes;     ///< Private Guild notes for the player.
    csString last_login;        ///< The last login time for that user.
    int privileges;             ///< Bit field for additional privileges.
    int removedPrivileges;      ///< Bit field for privileges removed from this member.

    /**
     * Checks if the request right is possessed by this guild member
     *
     * @param rights The right that is being tested
     * @return True if the member has the privilege exclusively set or if the guild member's
     *         level has the privilege and the user does not have it exclusively removed.
     */
    bool HasRights(GUILD_PRIVILEGE rights)
    {
        return (guildlevel->HasRights(rights) && !(removedPrivileges & rights)) || (privileges & rights);
    }
};

//------------------------------------------------------------------------------

/**
 *  Holds data for a guild.
 *
 * Primary database table   : guilds <br\>
 * Secondary database tables: characters, guild_wars, guildlevels
 */
class psGuildInfo
{
protected:
    int id;                          ///< UID of the guild.
    csString name;                   ///< Name of the guild.
    PID founder;                     ///< Character id for the founder of the guild.
    int karma_points;                ///< Guild's current karma points.
    csString web_page;               ///< URL for the guild.
    csString motd;                   ///< The guild's Message of the day.
    bool secret;                     ///< Flag if the guild is secret or not.
    psMoney bankMoney;               ///< Money stored in the guild bank account.
    int max_guild_points;            ///< Maximum guild points obtainable in the guild.
    csTicks lastNameChange;          ///< Last time the name of this guild was changed <i>Default: 0</i>
    int alliance;                    ///< Alliance ID that this guild belongs to <i>Default: 0(no alliance)</i>
    csArray<psGuildMember*> members; ///< All of the members of the guild
    csArray<psGuildLevel*>  levels;  ///< All of the levels of the guild
    csArray<int> guild_war_with_id;  ///< IDs of guild that this guild is at war with
    friend class psGuildAlliance;

public:
    /**
     * Basic constructor
     *
     * Initializes an empty guild that is invalid. \ref Load or \ref InsertNew()
     * must be called before this guild is populated with anything useful.
     */
    psGuildInfo();

    /**
     * Basic constructor
     *
     * Initializes an empty guild that is invalid. \ref Load or \ref InsertNew()
     * must be called before this guild is populated with anything useful.
     *
     * @param name The name of the guild
     * @param founder The PID of the founder of the guild
     */
    psGuildInfo(csString name, PID founder);

    /**
     * Basic deconstructor
     */
    ~psGuildInfo();

    /**
     * Loads the guild based on the ID of the guild
     *
     * Loads the guild information from the database.
     * If any of the information fails to load, false is immediately returned
     *
     * @param id The ID of the guild to load
     * @return True if successful, false otherwise
     */
    bool Load(unsigned int id);

    /**
     * Loads the guild based on the name of the guild
     *
     * Loads the guild information from the database.
     * If any of the information fails to load, false is immediately returned
     *
     * @param name The name of the guild to load
     * @return True if successful, false otherwise
     */
    bool Load(const csString &name);

    /**
     * Loads guild information out of a DB result
     *
     * Extracts all of the guild information out of the result row.
     * Also performs queries to get members, levels and wars.
     *
     * @param row The result to extract information out of
     * @return True if successful, false otherwise
     */
    bool Load(iResultRow &row);

    /**
     * Inserts a new guild into the DB
     *
     * Uses the name and founder currently stored to insert. Updates id with
     * the ID received from the DB.
     *
     * @return True on success, false otherwise
     */
    bool InsertNew();

    /**
     * Removes the guild from the DB.
     *
     * Deletes everything associated with this guild from the DB.
     * This includes guild wars, guild levels, updating characters
     * and the guild itself. If the guild was in an alliance, the
     * alliance object is also notified.
     *
     * @return True if all the deletions/updates succeeded, false otherwise
     */
    bool RemoveGuild();

    /**
     * Marks a player as being connected.
     *
     * If player matches one of \ref members then that object
     * has its actor updated.
     *
     * @param player The player that connected
     */
    void Connect(psCharacter* player);

    /**
     * Marks a player as being disconnted.
     *
     * If player matches one of \ref members then that member
     * has its actor reset back to null and player
     * has its guild set to null
     *
     * @param player The player that disconnected
     */
    void Disconnect(psCharacter* player);

    /**
     * Updates the last login time for the player.
     *
     * Updates the local information for the last login time
     * of the player.
     *
     * @note The DB is not updated because it is managed separately
     * @param player The player that has changed its last login time
     */
    void UpdateLastLogin(psCharacter* player);

    /**
     * Finds a member.
     *
     * Searches through \ref members for a member with a name
     * that matches \ref name (case-insensitive).
     *
     * @return null if member was not found, the member otherwise
     */
    psGuildMember* FindMember(const char* name) const;

    /**
     * Finds a member.
     *
     * Searches through \ref members for a member with a PID
     * that matches char_id.
     *
     * @return null if member was not found, the member otherwise
     */
    psGuildMember* FindMember(PID char_id) const;

    /**
     * Finds the leader.
     *
     * Searches through \ref members for the member with level
     * equal to \ref MAX_GUILD_LEVEL.
     *
     * @note This function should never return null
     * @return null if the leader could not be found, the member otherwise
     */
    psGuildMember* FindLeader() const;

    /**
     * Finds a level.
     *
     * Searches through level for the level equal to level.
     *
     * @param level The level to look for
     * @return null if the level could not be found, the level otherwise
     */
    psGuildLevel* FindLevel(int level) const;

    /**
     * Tests if the guild meets minimum requirements.
     *
     * Currently only checks to see if \ref members has at least
     * \ref GUILD_MIN_MEMBERS members.
     *
     * @return True if requirements are met, false otherwise
     */
    bool MeetsMinimumRequirements() const;

    /**
     * Adds a character to this guild.
     *
     * Checks if member is already part of a guild and fails if they are. If they are
     * not the database is updated and this object is updated accordingly to reflect a
     * new member by adding the new member to \ref members.
     *
     * @param player The character that is joining
     * @param level The level the character is joining at
     * @note No checks are done to ensure the joining level is valid
     * @return True if everything went okay, false if the member was already part of
     * a guild or the DB update failed.
     */
    bool AddNewMember(psCharacter* player, int level=1);

    /**
     * Removes a member from this guild.
     *
     * First validates that the member is actually a member of this guild. If it is
     * then the DB is updated and this object removes the member from \ref members.
     *
     * @param target The member that is being remvoed
     * @return True if everything went okay, false if the member wasn't part of this
     * guild or if the DB update failed.
     */
    bool RemoveMember(psGuildMember* target);

    /**
     * Renames a level within this guild.
     *
     * First validates that the level exists in this guild. If it does then the DB
     * is updated with the new guild level name and the corresponding \ref psGuildLevel
     * in \ref levels is updated.
     *
     * @param level The level to rename
     * @param levelname The name for the level
     * @return False if the level did not exist or the DB update failed, true otherwise
     */
    bool RenameLevel(int level, const char* levelname);

    /**
     * Sets a privilege flag on a level.
     *
     * First validates that the level exists in this guild. If it does then the DB
     * is updated with the new guild level privilege and the corresponding \ref psGuildLevel
     * in \ref levels is updated.
     *
     * @param level The level to update privileges
     * @param privilege The privilege to add or remove
     * @param on If true the privilege is added, otherwise it is removed from the level
     * @return False if the level did not exist or the DB update failed, true otherwise
     */
    bool SetPrivilege(int level, GUILD_PRIVILEGE privilege, bool on);

    /**
     * Sets a privilege flag on a specific member.
     *
     * First validates that the member does not already have the flag and that the
     * level is valid. After that depending on whether the flag is being allowed or
     * disallowed and if psGuildLevel::HasRights returns true determines whether the
     * \ref psGuildMember::removedPrivileges is updated or \ref psGuildMember::privileges.
     * Finally the DB is updated accordingly.
     *
     * @param member The member that is changing privileges
     * @param privilege Privilige to add or remove
     * @param on If true the privilege is added, otherwise it is removed from the member
     * @return True if successful, false if the level did not exist, the member already
     * had the correct privileges or the DB update failed.
     */
    bool SetMemberPrivilege(psGuildMember* member, GUILD_PRIVILEGE privilege, bool on);

    /**
     * Changes a member's level.
     *
     * First validates that the level exists in this guild. If it does then the DB
     * is updated with the new member level and the target's level is also updated.
     *
     * @param target The member to update
     * @param level The level to update to
     * @return False if the level did not exist or the DB update failed, true otherwise
     */
    bool UpdateMemberLevel(psGuildMember* target, int level);

    /**
     * Override for \ref BankManager.
     *
     * @param money Passed to other func
     * @param unused Unused
     */
    inline void AdjustMoney(const psMoney &money, bool unused)
    {
        unused = unused;
        AdjustMoney(money);
    }

    /**
     * Adjusts the money in this guild's bank.
     *
     * Simply updates \ref bankMoney and then calls \ref SaveBankMoney.
     * @param money The money values to adjust by
     */
    void AdjustMoney(const psMoney &money);

    /**
     * Returns the amount of money this guild has in it's bank.
     *
     * @return \ref bankMoney
     */
    inline psMoney &GetBankMoney()
    {
        return bankMoney;
    }

    /**
     * Performs the DB save for this bank's money.
     *
     * If an error occurs it is simply written out of \ref Error3
     */
    void SaveBankMoney();

    /**
     * Gets how long before a user change their name.
     *
     * Used to ensure a user can actually change their guild name. If a user
     * has changed their name too recently this will return the number of minutes
     * until the user can change their name, otherwise this will return 0.
     * Uses \ref lastNameChange and \ref GUILD_NAME_CHANGE_LIMIT.
     *
     * @return 0 if the user can change their name, minutes until they can otherwise
     */
    unsigned int MinutesUntilUserChangeName() const;

    /**
     * Simply returns this guild's name.
     *
     * @return \ref name
     */
    inline const csString &GetName() const
    {
        return name;
    }

    /**
     *  Changes the max amount of points a member may have for this guild
     *
     * @note points has an upper bound of \ref MAX_GUILD_POINTS_LIMIT
     * @param points The amount of points to set
     * @return True if the DB update was successful, false otherwise
     */
    bool SetMaxMemberPoints(int points);

    /**
     * Simply returns the max member points allowed for this guild.
     *
     *  @return \ref max_guild_points
     */
    inline int GetMaxMemberPoints() const
    {
        return max_guild_points;
    }

    /**
     * Sets a member's points in the guild.
     *
     * Checks if desired points is greater than \ref max_guild_points. If it is
     * false is returned. Otherwise the database and the member have their points
     * updated.
     *
     * @param member The member to adjust guild points
     * @param points The amount of points to adjust to
     * @return True if successful, false if points > \ref max_guild_points or if
     * the DB update failed.
     */
    bool SetMemberPoints(psGuildMember* member, int points);

    /**
     * Sets a member's notes for the guild.
     *
     * Depending on if isPublic is true determines where \ref psGuildMember::public_notes
     * or \ref psGuildMember::private_notes is updated for the member. DB is also
     * updated.
     *
     * @param member The member to adjust notes
     * @param notes The new value for the notes
     * @param isPublic True for public notes, false for private
     * @return True if successful, false if DB update failed.
     */
    bool SetMemberNotes(psGuildMember* member, const csString &notes, bool isPublic);

    /**
     * Sets the name for the guild.
     *
     * Updates the cached copy as well as the DB. Udates \ref lastNameChange
     * to the current time (in minutes)
     *
     * @param guildName The desired new name
     * @return True if successful, false if DB update failed.
     */
    bool SetName(csString guildName);

    /**
     * Gets the web page.
     *
     * @return web_page
     */
    inline const csString &GetWebPage() const
    {
        return web_page;
    }

    /**
     * Sets the web page.
     *
     * If current web_page is equal to parameter, true is returned. Otherwise
     * \ref web_page is updated and the DB is updated.
     *
     * @param web_page The value to overwrite into \ref web_page
     * @return True if successful, false if DB update failed.
     */
    bool SetWebPage(const csString &web_page);

    /**
     * Sets whether the guild is secret or not.
     *
     * If \ref secret is already the requested value, true is returned. Otherwise
     * \ref secret and the DB are updated.
     *
     * @param secretGuild The new value to set \ref secret to
     * @return True if successful, false if DB update failed.
     */
    bool SetSecret(bool secretGuild);

    /**
     * Returns whether this guild is secret.
     *
     * @return \ref secret
     */
    inline bool IsSecret() const
    {
        return secret;
    }

    /**
     *  Gets the MOTD string
     *
     * @return \ref motd
     */
    inline const csString &GetMOTD() const
    {
        return motd;
    }

    /**
     * Sets the MOTD string.
     *
     * Updates DB and sets \ref motd.
     *
     * @param str The new value for the MOTD
     * @return True if successful, false if DB update failed.
     */
    bool SetMOTD(const csString &str);

    /**
     * Gets the karma points.
     *
     * @return \ref karma_points
     */
    inline int GetKarmaPoints() const
    {
        return karma_points;
    }

    /**
     * Sets the karma points.
     *
     * Updates DB and sets \ref karma_points
     *
     * @param n_karma_points New value for karma points
     * @return True if successful, false if DB update failed.
     */
    bool SetKarmaPoints(int n_karma_points);

    /**
     * Adds a guild war with another guild.
     *
     * Adds the other guild to \ref guild_war_with_id. DB is updated
     * through the command pump.
     *
     * @param other The guild this guild is going to war with
     */
    void AddGuildWar(psGuildInfo* other);

    /**
     * Checks if a guild war is active.
     *
     * Loops through \ref guild_war_with_id to see if the other guild's ID
     * is present.
     *
     * @param other The other guild to check if there is a guild war
     * @return True if ther other guild is at war with this one
     */
    bool IsGuildWarActive(psGuildInfo* other);

    /**
     * Removes a guild war.
     *
     * Removes the other guild's ID from \ref guild_war_with_id. DB is
     * then updated through the command pump.
     */
    void RemoveGuildWar(psGuildInfo* other);

    /**
     * Gets the ID of the alliance this guild belongs to.
     *
     * @return \ref alliance, 0 denotes no alliance
     */
    inline int GetAllianceID() const
    {
        return alliance;
    }

    /**
     *  Gets the ID of this guild
     *
     * @return \ref id
     */
    inline int GetID() const
    {
        return id;
    }

    /**
     * Gets a const iterator for levels.
     *
     * @return A const iterator from \ref levels
     */
    inline csArray<psGuildLevel*>::ConstIterator GetLevelIterator() const
    {
        return levels.GetIterator();
    }

    /**
     * Gets an iterator for level.
     *
     * @return An iterator from \ref levels
     */
    inline csArray<psGuildLevel*>::Iterator GetLevelIterator()
    {
        return levels.GetIterator();
    }

    /**
     * Gets a const iterator for levels.
     *
     * @return A const iterator from \ref levels
     */
    inline csArray<psGuildMember*>::ConstIterator GetMemberIterator() const
    {
        return members.GetIterator();
    }

    /**
     * Gets an iterator for level.
     *
     * @return An iterator from \ref levels
     */
    inline csArray<psGuildMember*>::Iterator GetMemberIterator()
    {
        return members.GetIterator();
    }

    /**
     * Gets the number of members.
     *
     * @return Size of \ref members
     */
    inline size_t GetMemberCount() const
    {
        return members.GetSize();
    }
};

//-----------------------------------------------------------------------------

/**
 * A guild alliance between 2+ guilds.
 *
 * Primary database table   : alliances <br/>
 * Secondary database tables: guilds
 */
class psGuildAlliance : public csRefCount
{
public:

    /**
     * Basic constructor.
     */
    psGuildAlliance();

    /**
     * Basic constructor with name parameter.
     *
     * Used when inserting a new alliance.
     *
     * @param n_name Sets \ref name
     */
    psGuildAlliance(const csString &n_name);

    /**
     * INSERTs alliance data in database.
     *
     * Uses \ref name to insert alliance into database.
     * No leader will be set.
     *
     * @todo Why not include leader at same time?
     * @return True if successful, false if DB insert failed.
     */
    bool InsertNew();

    /**
     * Removes alliance.
     *
     * Deletes alliance data from database and updates \ref psGuildInfo::alliance
     * of \ref members to 0
     *
     * @return True if successful, false if either the update or the delete failed.
     */
    bool RemoveAlliance();

    /**
     * Loads alliance data from database.
     *
     * Uses parameter to overwrite \ref id. Uses \ref id select from the DB the \ref name
     * and leading guild's ID. If select fails, false is returned. The leader guild is
     * then pulled from the cache manager. If it fails, false is returned. All of the members
     * are then selected from the DB based on this alliance's ID sorted by name. Each member
     * is then actually pulled from the cache manager. If any of the members fail, false is
     * returned.
     *
     * @param id The ID to pull from the DB
     * @return True if everything was successful, false if an error occurred.
     */
    bool Load(int id);

    /**
     * Adds a new guild to the alliance.
     *
     * Returns false if the guild to add is already in the alliance. Also returns false if
     * the DB update fails. Otherwise, the DB is updated and the new guild is added to
     * \ref members.
     *
     * @param member The new guild to add
     * @return True if everything was successful, false if either the update failed or
     * the guild is already part of the alliance.
     */
    bool AddNewMember(psGuildInfo* member);

    /**
     * Checks if a guild is in this alliance.
     *
     * Checks if the guild is in \ref members.
     *
     * @param member The guild to check
     * @return True if the guild is in the alliance, false if it is not.
     */
    bool CheckMembership(psGuildInfo* member) const;

    /**
     * Removes a guild from the alliance.
     *
     * Checks to make sure the guild is actually a member of this alliance and that it
     * is not the leader, if both are true then the guild is removed from \ref members.
     *
     * @param member The guild to remove
     * @return True if successful, false if the guild is not a member or is the leader or
     * if the DB update fails.
     */
    bool RemoveMember(psGuildInfo* member);

    /**
     * Gets the number of members in this alliance.
     *
     * @return The size of \ref members
     */
    inline size_t GetMemberCount() const
    {
        return members.GetSize();
    }

    /**
     * Gets a guild based off its index.
     *
     * Asserts that the index is > 0 and < \ref GetMemberCount().
     *
     * @param memberNum Index of the guild to look up
     * @return The member in \ref members with the given index
     */
    psGuildInfo* GetMember(int memberNum);

    /**
     * Gets the ID.
     *
     * @return \ref id
     */
    inline int GetID() const
    {
        return id;
    }

    /**
     * Gets the name.
     *
     * @return \ref name
     */
    inline const csString &GetName() const
    {
        return name;
    }

    /**
     * Gets the leader.
     *
     * @return \ref leader
     */
    inline psGuildInfo* GetLeader() const
    {
        return leader;
    }

    /**
     * Sets a new alliance leader.
     *
     * Calls \ref CheckMembership with the new leader, then updates the DB and
     * sets \ref leader to the new leader.
     *
     * @param newLeader The guild that will be the new leader
     * @return True if successful, false if \ref CheckMembership fails or the DB
     * update fails.
     */
    bool SetLeader(psGuildInfo* newLeader);

    static csString lastError;       ///< When a psGuildAlliance method fails (returns false), this contains a description of the problem

protected:
    int id;                          ///< The ID of the alliance, used for storing in the DB
    csString name;                   ///< Viewable name of the alliance
    psGuildInfo* leader;             ///< Leader of the alliance, must also be a member
    csArray<psGuildInfo*> members;   ///< Array of the members of this alliance
};

/** @} */

#endif

