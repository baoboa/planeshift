/* marriagemanager.h- Author: DivineLight
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */

#ifndef __MARRIGEMANAGER_H__
#define __MARRIGEMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================

/**
 * \addtogroup server
 * @{ */

/**
 * This class manages all marriage related stuff.
 *
 * This class manages all marriage related functionality and has
 * functions to retrieve/edit character's marriage related details.
 */
class psMarriageManager
{
public:

    /// Propose
    void Propose(Client* client, csString proposedCharName, csString proposeMsg);

    /// Confirm divorce
    void ContemplateDivorce(Client* client, csString divorceMsg);

    /// Divorce
    void Divorce(Client* client, csString divorceMsg);

    /**
     * Sets spouse name for a given character Name
     *
     * @param charData    Pointer to the character's data
     * @param spouseData  Pointer to spouse character's data.
     * @return true if successfull else returns false.
     */
    bool PerformMarriage(psCharacter* charData, psCharacter* spouseData);

    /**
     * Deletes marriage information of given character name from DB and
     * reverts any lastname changes and updates cached data.
     */
    void DeleteMarriageInfo(psCharacter* charData);

    /**
     * Used to update name in labels, guilds, targets, etc.
     */
    void UpdateName(psCharacter* charData);

private:
    /**
     * Creates a character's entry in "character_marriage_details"
     * table of DB if one is not present already.
     */
    bool CreateMarriageEntry(psCharacter* charData,  psCharacter* spouseData);
};

/** @} */

#endif
