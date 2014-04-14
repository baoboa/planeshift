/*
 * groupmanager.h by Anders Reggestad <andersr@pvv.org>
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
#ifndef __GROUPMANAGER_H__
#define __GROUPMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/refarr.h>
#include <csutil/array.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"   // Subscriber class
#include "playergroup.h"

class gemActor;
class GroupManager;
class Client;
class ChatManager;
class ClientConnectionSet;

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
class PendingGroupInvite;
class PendingGroupChallenge;

/** Main PlayerGroup Manager that holds all the groups.
 * This maintains all the groups on the servers and is responsible for all
 * the group functions.
 */
class GroupManager : public MessageManager<GroupManager>
{
public:

    GroupManager(ClientConnectionSet* pCCS, ChatManager* chat);
    virtual ~GroupManager();

    void HandleJoinGroup(PendingGroupInvite* invite);

    /** Handles the answer to the request of challenge.
     *
     *  @param invite The class which handled the question to the
     *                challenged client.
     */
    void HandleChallengeGroup(PendingGroupChallenge* invite);

protected:
    friend class PlayerGroup;

    void HandleGroupCommand(MsgEntry* pMsg,Client* client);

    void Invite(psGroupCmdMessage &msg,Client* client);
    void Disband(psGroupCmdMessage &msg,gemActor* client);
    void Leave(psGroupCmdMessage &msg,gemActor* client);
    void ListMembers(psGroupCmdMessage &msg,gemActor* client);
    void RemovePlayerFromGroup(psGroupCmdMessage &msg,gemActor* client);

    /** Handles /groupchallenge and challenges another group after checking all requirements
     *  are in order.
     *
     *  @param msg the prepared message from the network.
     *  @param Challenger A pointer to the client which issued this command.
     */
    void Challenge(psGroupCmdMessage &msg,Client* Challenger);
    /** Handles /groupyield and yields to all the group in duel with the
     *  one requesting it after checking all requirements are in order.
     *
     *  @param msg the prepared message from the network.
     *  @param Yielder A pointer to the client which issued this command.
     */
    void Yield(psGroupCmdMessage &msg,Client* Yielder);

    /** Makes two group be in challenge each other.
     *
     *  @param ChallengerGroup the group which is starting the challenge.
     *  @param ChallengedGroup the group which is being challenged.
     */
    bool AddGroupChallenge(PlayerGroup* ChallengerGroup, PlayerGroup* ChallengedGroup);

    void SendGroup(gemActor* client);
    void SendLeave(gemActor* client);
    csPtr<PlayerGroup> NewGroup(gemActor* leader);
    bool AddPlayerToGroup(PlayerGroup* group, gemActor* client);
    void Remove(PlayerGroup* group);
    void GroupChat(gemActor* client, const char* fmt, ...);
    PlayerGroup* FindGroup(int id);


    ChatManager* chatserver;
    ClientConnectionSet* clients;
    csRef<EventManager> eventmanager;
    csRefArray<PlayerGroup> groups;
};

#endif
