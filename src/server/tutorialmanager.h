/*
 * tutorialmanager.h
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __TUTORIALMANAGER_H__
#define __TUTORIALMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class

/**
 * This manager catches events posted by the rest of the server
 * and decides whether each client needs to be notified about
 * them, in order to show a tutorial page about that event.
 */
class TutorialManager : public MessageManager<TutorialManager>
{
public:

    enum TutorEventType
    {
        CONNECT       = 0, // First welcome to the game.
        MOVEMENT      = 1, // Sentence on arrow keys and pan up/down. Tell to find npc in front and click on him.
        NPC_SELECT    = 2, // You found our npc.  Right-click to select any target, and it shows up in your target window.
        NPC_TALK      = 3, // Whenever you have an intelligent npc targeted, you can talk to it and he will try to respond. Say hello to him now.
        QUEST_ASSIGN  = 4, // You have now been assigned a task by the npc.  Click the Q button to see your list of active quests.  Now find a rat to your right.
        DAMAGE_SELF   = 5, // You just got attacked and hit by a monster.  Your percent of health (hit points) is shown in the red bar in your info window.
        DAMAGE_FALL   = 6, // You just fell too far and hurt yourself.  Don't take too much damage or you can die just from falling.
        DEATH_SELF    = 7, // You have just died.  In a few seconds, you will be transported to the Death Realm and you will have to escape it to come back to Yliakum.
        SPAWN_MOVE    = 8, // gives various informations about the server like general rules for rp
        // ENEMY_SELECT  = 9, // Now that you have a monster selected, you can attack it by pressing 'A' and watching it die.
        // ENEMY_DEATH   = 10, // When you kill a monster, it is often carrying things you can use.  Right-click and select the Hand to loot the monster.
        // LOOT_COMPLETE = 11 // Now that you picked up your loot, check your inventory by pressing 'I' and see that you have those items now.
        TUTOREVENTTYPE_COUNT
    };

    TutorialManager(ClientConnectionSet* pCCS);
    virtual ~TutorialManager();

    /**
     * Specifically handle the message sent by a script
     *
     * @param client The client where the message has to be sent.
     * @param which  The ID of the tip from the database
     */
    void HandleScriptMessage(uint32_t client, unsigned int which);

protected:
    /// Specifically handle the Connect event in the tutorial
    void HandleConnect(MsgEntry* pMsg, Client* client);

    /// Specifically handle the Movement event in the tutorial
    void HandleMovement(MsgEntry* pMsg, Client* client);

    /// Specifically handle the Target event in the tutorial
    void HandleTarget(MsgEntry* pMsg, Client* client);

    /// Specifically handle the Damage event in the tutorial
    void HandleDamage(MsgEntry* pMsg, Client* client);

    /// Specifically handle the Death event in the tutorial
    void HandleDeath(MsgEntry* pMsg, Client* client);

    /// Handle tutorial events which come from events which do not have specific msgtypes.
    void HandleGeneric(MsgEntry* pMsg, Client* client);

    /// Preload all tutorial strings from the Tips db table.
    bool LoadTutorialStrings();

    /// Package up the current event string for the client receiving the tutorial.
    void SendTutorialMessage(int which, Client* client, const char* instrs);
    void SendTutorialMessage(int which, uint32_t clientnum, const char* instrs);

    ClientConnectionSet* clients;

    /**
     * Stores all the messages in the database above id 1001 (biased to 1001 so
     * 1001 is 0).
     * @note Not all messages are handled directly by the tutorial manager,
     *       only the first 32.
     */
    csArray<csString> tutorialMsg;
};

#endif

