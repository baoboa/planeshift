/*
 * pscharacterloader.h
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




#ifndef __PSCHARACTERLOADER_H__
#define __PSCHARACTERLOADER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"

#include <idal.h>      // Database Abstraction Layer Interface

#include "../gem.h"

//=============================================================================
// Local Includes
//=============================================================================


class csMutex;
class psCharacterList;
class psSectorInfo;
class psCharacter;
class psItem;
class psItemStats;
class gemActor;

/**
 * \addtogroup bulkobjects
 * @{ */

/*  Why isn't there a LoadItem() call here?
 *
 *  At the time of initial design the circumstances under which an item entry will be loaded are unclear.
 *  It is virtually guaranteed that item entries will need to be loaded when a character is loaded as the character's
 *    inventory (list of items) will be necessary.
 *  It is undecided wether some, all or no items in the world and not on a character will be saved to the database.
 *    If they are saved it is likely that many items will be loaded all at once - possibly based on location data
 *    (x,y,z,sector), possibly based on some other criteria.
 */

/**
 * This class controls loading and saving Characters and Character specific data to and from an iDatabase.
 *
 *  The class's primary goal is to keep the lower level psdatabase from knowing too much about how tables relate
 *    and to keep the higher level psCharacter, psItem, etc from knowing how the data is stored at all.
 *  A secondary goal of the interface is that it should allow for delayed processing in the future with minimal changes.
 *    As the project grows in players and complexity the amount of data passing to and from the server will grow as well.
 *    It will become unacceptable to wait for the database to respond to each and every query before continuing processing
 *    other tasks.
 *  In order to achieve this, three conditions must be met:
 *    1) Calls that access the database server must be able to return immediately.
 *    2) Calls that store data in the database must make a local copy of the data to be stored so that the original data may continue
 *       to be modified by the server unrestricted.
 *    3) Calls that retrieve data from the database must have a mechanism to check for retrieved data or errors.
 *
 *
 *  While these are not all currently met, the design is such that it would take only minor restructuring to meet them all.
 */
class psCharacterLoader
{
public:
    /**
     * Construct a new psCharacterLoader, optionally specifying an iDatabase interface to use for database calls.
     */
    psCharacterLoader();

    /**
     * Destructor.
     */
    ~psCharacterLoader();

    /**
     * Initializes settings related to this class.
     *
     * Call this before using any functions.
     */
    bool Initialize();

    /**
     * Loads the names of characters for a given account for charpick screen on login.
     */
    psCharacterList* LoadCharacterList(AccountID accountid);


    psCharacter** LoadAllNPCCharacterData(psSectorInfo* sector,int &count);

    /**
     * Loads data for a character from the database and constructs and fills a new psCharacter object with the data.
     *
     * @note This function call must block for database access.  It will need to be redesigned for deferred database
     *       in the future.
     * @param pid The unique ID of the character to load data for.  Usually obtained from the account.
     * @param forceReload Ignores (and empties) the cache if set to true.
     *
     * @return Pointer to a newly created psCharacter object, or NULL if the data could not be loaded.
     */
    psCharacter* LoadCharacterData(PID pid, bool forceReload);

    /**
     * Load just enough of the character data to know what it looks like (for selection screen).
     */
    psCharacter* QuickLoadCharacterData(PID pid, bool noInventory);

    /**
     * Creates a new character entry to store the provided character data.
     *
     * @note This function call must block for database access.  It will need to be redesigned for deferred database
     *       in the future.
     *
     * @param accountid A numeric account identifier with which this character should be associated.
     * @param chardata Pointer to a psCharacter object containing the Character data to be stored as a new character.
     *
     * @return true - Save progressing.   false - Could not save.
     *         When this function returns the UID member of the chardata structure will contain the unique id for this
     *         character >0 if and only if the save was successful.
     */
    bool NewCharacterData(AccountID accountid, psCharacter* chardata);
    bool NewNPCCharacterData(AccountID accountid,psCharacter* chardata);
    unsigned int InsertNewCharacterData(const char** fieldnames, psStringArray &fieldvalues);

    /**
     * Saves character data to the database.
     *
     * This function call does not need to block for database access. It can copy the psCharacter object and
     * related sub-objects and verify some integrity of the data before queueing a database write and returning
     * true.
     *
     * @param chardata Pointer to a psCharacter object containing the Character data to be updated.
     *         The uid member of chardata must be valid and unique.  The UID value can only be set
     *         by creating a new character and calling NewCharacterData().
     * @param actor A pointer to the actor.
     * @param charRecordOnly Should only char record be saved.
     * @return true - Save progressing.   false - Could not save.
     */
    bool SaveCharacterData(psCharacter* chardata,gemActor* actor,bool charRecordOnly=false);

    /**
     * Deletes character data to the database.
     *
     * This function call does not need to block for database access.
     *
     * @param pid The unique ID of the character to erase data for.  Usually obtained from the account.
     * @param error A error message in case character could not be deleted.
     * @return true - Delete progressing.   false - Could not delete.
     */
    bool DeleteCharacterData(PID pid, csString &error);


    /**
     * This function finds a character's ID given a character name.
     *
     *  We go straight to the database currently for this information.
     *
     *  @param character_name The name of the character we are looking for
     *  @param excludeNPCs If we should exclude NPC's from the search.
     *  @return The ID of the character or 0 if not found
     */
    PID FindCharacterID(const char* character_name, bool excludeNPCs=true);

    /**
     * This function finds a character's ID given a character name and the ID of the
     * account that this character is supposed to be on.
     *
     * We go straight to the database currently for this information.
     *
     * @param accountID The account the character is supposedly on
     * @param character_name The name of the character we are looking for
     * @return The ID of the character or 0 if not found
     */
    PID FindCharacterID(AccountID accountID, const char* character_name);

    /**
     * Checks to see if the character name is owned by a particular ID.
     *
     * @param characterName The character name we want to check
     * @param accountID The account this character should belong to.
     * @return true If the character name given belongs to the accountID given.
     */
    bool AccountOwner(const char* characterName, AccountID accountID);

    /**
     * Update the skill in the database.
     */
    bool UpdateCharacterSkill(PID pid, unsigned int skill_id,
                              unsigned int skill_z, unsigned int skill_y, unsigned int skill_rank);


private:

    bool ClearCharacterAdvantages(PID pid);
    bool SaveCharacterAdvantage(PID pid, unsigned int advantage_id);
    bool ClearCharacterSkills(PID pid);
    bool ClearCharacterTraits(PID pid);
    bool SaveCharacterSkill(PID pid,unsigned int skill_id,
                            unsigned int skill_z, unsigned int skill_y, unsigned int skill_rank);
    bool SaveCharacterTrait(PID pid, unsigned int trait_id);
    bool UpdateQuestAssignments(psCharacter* chr);
    bool ClearCharacterSpell(psCharacter* character);
    bool SaveCharacterSpell(psCharacter* character);

};

//-----------------------------------------------------------------------------


/**
 * This is an event that schedules regular saving of character data.
 *
 * This is done to allow some preservation of character data on a server crash.
 * When a player logs in this event is pushed into the queue with a default
 * delay.  When this message triggers it will re-add itself to the queue until
 * the player logs out.
 */
class psSaveCharEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    enum Type
    {
        SAVE_PERIOD = 600000
    };

    gemActor* actor;     /// The GEM object want to save data with.

    /**
     * Construct the message.
     *
     * @param actor The actor that has the character that we want to save.
     */
    psSaveCharEvent(gemActor* actor);

    /**
     * Destory Message.
     */
    ~psSaveCharEvent();

    /**
     * Trigger this message and re-insert it back into queue.
     */
    virtual void Trigger();  // Abstract event processing function

    /**
     * This is called when the client we were tracking has logged out and this mesage
     * is no longer required.
     *
     * @param object The gem object that we had registered to.
     */
    virtual void DeleteObjectCallback(iDeleteNotificationObject* object);
};

/** @} */

#endif

