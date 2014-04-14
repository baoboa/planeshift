/*
 * cmdusers.h - Author: Keith Fulton
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

#ifndef __CMDUSERS_H__
#define __CMDUSERS_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================


class MsgHandler;
class GEMClientObject;

// Maximum range for the nearby target searches.  NOTE: If you change this,
// update the command documentation in the constructor.
#define NEARBY_TARGET_MAX_RANGE 50

/** Manages different commands that are player based in nature.
 */
class psUserCommands : public psCmdBase
{

public:
    psUserCommands(ClientMsgHandler* mh,CmdHandler *ch,iObjectRegistry* obj);
    virtual ~psUserCommands();

    static void HandleSlay(bool answeredYes, void *data);

    // Load emotes from xml.
    bool LoadEmotes();

    // iCmdSubscriber interface
    virtual const char *HandleCommand(const char *cmd);

    // iNetSubscriber interface
    virtual void HandleMessage(MsgEntry *msg);

    
protected:
 // Directions for a search.
    enum SearchDirection
    {
        SEARCH_FORWARD,
        SEARCH_BACK,
        SEARCH_NONE
    };
        
    enum EntityTypes
    {
        PSENTITYTYPE_PLAYER_CHARACTER,
        PSENTITYTYPE_NON_PLAYER_CHARACTER,
        PSENTITYTYPE_ITEM,
        PSENTITYTYPE_NAME,
        PSENTITYTYPE_NO_TARGET
    };

    GEMClientObject* FindEntityWithName(const char *name);

    /** @brief Returns a target string formatted to be sent in a message
     * 
     * This function would be called on a target input by the player,
     * and will return a string indicating the same target, formatted
     * to be sent in a net message to the server. If this can't be done,
     * it will send back an empty string (error)
     * 
     * @param target The target to be formatted
     * 
     * @return Returns the formatted target, or an empty string
     */
    csString FormatTarget(const csString& target = "target");

    void UpdateTarget(SearchDirection searchDirection,
                      EntityTypes entityType,
                      const char* name);


    /// Struct to hold our emote data.
    struct EMOTE 
    {
        csString command;
        csString general;
        csString specific;
        csString anim;
        bool enabled; ///< changed type of enabled from csString to bool 
    };

    csArray<EMOTE> emoteList;

    void AskToSlayBeforeSending(psMessageCracker *msg);    
};

#endif
