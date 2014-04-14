/*
 * creationmanager.h - author: Andrew Craig
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
#ifndef PS_CHAR_CREATION_MANAGER_H
#define PS_CHAR_CREATION_MANAGER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Space Includes
//=============================================================================
#include "net/charmessages.h"


//=============================================================================
// Local Space Includes
//=============================================================================
#include "msgmanager.h"

#define MAX_PLAYER_NAME_LENGTH 27

enum ReservedNames {NAME_RESERVED_FOR_YOU, NAME_RESERVED, NAME_AVAILABLE};

/** Server manager for character creation.
 * This guy loads all the data that may be needed to be sent to the client
 * for the character creation.  It loads data from these tables:
 *   race_info
 *   character_creation
 *
 */
class CharCreationManager : public MessageManager<CharCreationManager>
{
public:
    CharCreationManager(GEMSupervisor* gemsupervisor, CacheManager* cachemanager, EntityManager* entitymanager);
    virtual ~CharCreationManager();

    /** Caches the data from the database needed for character creation.
     */
    bool Initialize();

    bool Validate(psCharUploadMessage &mesg, csString &errorMsg);

    /** Check to see a name is unique in the characters table.
      * @param playerName The name to check to see if it is unique.
      * @param dbUniqueness If false checks if it's still missing from the db, if true checks if there is only one entry in the db
      * @return true if the name is unique.
      */
    static bool IsUnique(const char* playerName,  bool dbUniqueness = false);

    /** Check whether a given last name is available.  Last names are available
      * if (a) unused, or (b) used by another character on the same account.
      * @param lastname The lastname to check for availability.
      * @param requestingAcct The account requesting the last name (optional).
      * @return true if allowed.
      */
    static bool IsLastNameAvailable(const char* lastname, AccountID requestingAcct = 0);

    // Returns true if the name is ok
    static bool FilterName(const char* name);

protected:

    /// Cache in the data about the race CP starting values.
    bool LoadCPValues();

    /// Caches in creation choices from the database.
    bool LoadCreationChoices();

    bool LoadLifeEvents();

    int raceCPValuesLength;    // length of the raceCPValues array

    /** Takes a string name of a choice and converts it to it's enum value.
     * This is useful to have string names in the database ( ie human readable ) but
     * then use them as ints in the application ( ie easier to use ).
     *
     * @param area The name of the area to convert.
     * @return The int value of that area. Will be one of the defined enums of areas.
     */
    int ConvertAreaToInt(const char* area);

    /** Handle and incomming message from a client about parents.
     * This handles a request from the client to send it all the data about choice
     * areas in the parents screen.
     */
    void HandleParents(MsgEntry* me,Client* client);

    void HandleChildhood(MsgEntry* me,Client* client);

    void HandleLifeEvents(MsgEntry* me,Client* client);

    void HandleTraits(MsgEntry* me,Client* client);

    void HandleUploadMessage(MsgEntry* me, Client* client);

    /** Handles the creation of a character from the char creation screen
     */
    void HandleCharCreateCP(MsgEntry* me, Client* client);

    /** Handles a check on a name.
      * This will check the name against the migration and the current
      * characters table.  It will send back a message to the client to
      * notify them of the name status. If rejected it will include a
      * message why.
      */
    void HandleName(MsgEntry* me ,Client* client);

    /** Handles the deletion of a character from the char pick screen.
      */
    void HandleCharDelete(MsgEntry* me, Client* client);

    int CalculateCPChoices(csArray<uint32_t> &choices, int fatherMod, int motherMod);
    int CalculateCPLife(csArray<uint32_t> &events);

    /** Assign a script to a character.
      * Takes whatever script is stored in the migration table and adds it to the
      * character to be run on character login.
      * @param chardata The character data class to append the script to.
      */
    void AssignScript(psCharacter* chardata);

    /** Check to see if a name is on the reserve list for database migration.
      *  @param playerName The name to check to see if is on a the reserve list.
      *  @param acctID The acct that is trying to create this character.
      *
      *  @return <UL>
      *          <LI>NAME_RESERVED_FOR_YOU - if you can take this name.
      *          <LI>NAME_RESERVED - you are not allowed this name.
      *          <LI>NAME_AVAILABLE - name is free for use.
      *          </UL>
      */
    int IsReserved(const char* playerName, AccountID acctID);
    bool PlayerHasFinishedTutorial(AccountID acctID, uint32 tutorialsecid);

private:
    // Structure to hold the initial CP race values.
    struct RaceCP
    {
        int id;
        int value;
    };

    ////////////////////////////////////////////////////
    // Structure for cached character creation choices
    ////////////////////////////////////////////////////
    struct CreationChoice
    {
        int id;
        csString name;
        csString description;
        int choiceArea;
        int cpCost;
        csString eventScript;
    };
    ////////////////////////////////////////////////////


    //////////////////////////////////////////////////////
    // Structure for cached character creation life event
    //////////////////////////////////////////////////////
    class LifeEventChoiceServer : public LifeEventChoice
    {
    public:
        csString eventScript;
    };
    //////////////////////////////////////////////////////

    LifeEventChoiceServer* FindLifeEvent(int id);
    CreationChoice* FindChoice(int id);


    RaceCP* raceCPValues;

    /// A list of all the for the parent screen.
    csPDelArray<CreationChoice> parentData;
    csPDelArray<CreationChoice>  childhoodData;
    csPDelArray<LifeEventChoiceServer> lifeEvents;

    GEMSupervisor* gemSupervisor;
    CacheManager* cacheManager;
    EntityManager* entityManager;
};



#endif
