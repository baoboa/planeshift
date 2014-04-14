/*
 * groupmanager.cpp by Anders Reggestad <andersr@pvv.org>
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
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psraceinfo.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/psxmlparser.h"
#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "groupmanager.h"
#include "client.h"
#include "clients.h"
#include "invitemanager.h"
#include "playergroup.h"
#include "gem.h"
#include "psserver.h"
#include "chatmanager.h"
#include "globals.h"
#include "netmanager.h"


/** A structure to hold the clients that are pending on joining a group.
 */
class PendingGroupInvite : public PendingInvite
{
public:
    int groupID;

    PendingGroupInvite(Client* inviter,
                       Client* invitee,
                       const char* question,
                       PlayerGroup* g)
        : PendingInvite(inviter, invitee, true,
                        question,"Accept","Decline",
                        "You have invited %s to join your group.",
                        "%s has invited you to group.",
                        "%s has joined your group.",
                        "You have joined the group.",
                        "%s has declined your invitation.",
                        "You have declined %s's invitation.",
                        psQuestionMessage::generalConfirm)
    {
        groupID   = g->GetGroupID();
    }

    virtual ~PendingGroupInvite() {}

    void HandleAnswer(const csString &answer);
};

/** A structure to hold the clients that are pending on challenging a group.
 */
class PendingGroupChallenge : public PendingInvite
{
public:
    int groupID;

    PendingGroupChallenge(Client* inviter,
                          Client* invitee,
                          const char* Question,
                          PlayerGroup* Group)
        : PendingInvite(inviter, invitee, true,
                        Question,"Accept","Decline",
                        "Your group has challenged %s's group.",
                        "%s has challenged your group.",
                        "%s has accepted the challenge.",
                        "You accepted the challenge.",
                        "%s has declined your challenge.",
                        "You have declined %s's challenge.",
                        psQuestionMessage::generalConfirm)
    {
        groupID   = Group->GetGroupID();
    }

    virtual ~PendingGroupChallenge() {}

    void HandleAnswer(const csString &answer);
};

PlayerGroup::PlayerGroup(GroupManager* mgr, gemActor* leader)
    :manager(mgr), leader(leader),id(next_id++)
{
    members.Push(leader);
    leader->SetGroup(this);
}

PlayerGroup::~PlayerGroup()
{
}

void PlayerGroup::Add(gemActor* new_member)
{
    members.Push(new_member);
    new_member->SetGroup(this);
}

void PlayerGroup::Remove(gemActor* member)
{
    psserver->SendSystemInfo(leader->GetClientID(),"%s has left the group",member->GetName());

    // Delete member from group
    member->SetGroup(NULL);
    members.Delete(member);

    // Check if we need to find a new leader for group
    if(member == leader)
    {
        if(members.GetSize())
        {
            leader = members[0];
            psserver->SendSystemInfo(leader->GetClientID(), "You are now group leader.");
        }
        else
        {
            leader = 0;
            manager->Remove(this);
            return;    // return immediately because our instance was deleted
        }

    }

    // If only one member left in the group then disband it.
    if(members.GetSize() == 1)
    {
        Disband();
        manager->Remove(this);
        return;
    }

    if(!IsEmpty())
    {
        BroadcastMemberList();
    }
}

void PlayerGroup::ListMembers(gemActor* client)
{
    int clientnum = client->GetClientID();
    csString str;

    psserver->SendSystemInfo(clientnum, "-------Group members-------");
    for(size_t n = 0; n < members.GetSize(); n++)
    {
        str =  "-";
        str += members[n]->GetName();
        psserver->SendSystemInfo(clientnum, str);
    }
}

void PlayerGroup::Broadcast(MsgEntry* me)
{
    // Copy message to send out to everyone
    csRef<MsgEntry> newmsg;
    newmsg.AttachNew(new MsgEntry(me));
    if(newmsg->overrun)
    {
        Bug1("Could not copy MsgEntry for PlayerGroup::Broadcast.\n");
        return;
    }

    newmsg->msgid = psserver->GetNetManager()->GetRandomID();

    for(size_t n = 0; n < members.GetSize(); n++)
    {
        newmsg->clientnum = members[n]->GetClientID();
        psserver->GetEventManager()->SendMessage(newmsg);
    }

    CHECK_FINAL_DECREF(newmsg,"GroupMsg");
}


bool PlayerGroup::IsLeader(gemActor* client)
{
    return (leader == client);
}

gemActor* PlayerGroup::GetLeader()
{
    return leader;
}

void PlayerGroup::Disband()
{
    //first of all we yield from all the duels if in place
    DuelYield();

    psGUIGroupMessage msg(0,psGUIGroupMessage::LEAVE,"Group Disbanded By Leader");
    if(!msg.valid)
    {
        Bug1("Could not create valid psGUIGroupMessage for group.\n");
        return;
    }


    for(size_t n = 0; n < members.GetSize(); n++)
    {
        msg.msg->clientnum = members[n]->GetClientID();
        psserver->GetEventManager()->SendMessage(msg.msg);
        members[n]->SetGroup(NULL);
    }
    members.DeleteAll();

    leader = 0;
}

bool PlayerGroup::IsEmpty()
{
    return (members.GetSize() == 0);
}

float fixZero(float f)
{
    return f == 0 ? 1 : f;
}

void PlayerGroup::BroadcastMemberList()
{
    csString list;
    list.Append("<L>");
    for(size_t n = 0; n < members.GetSize(); n++)
    {
        csString buff;
        psCharacter* charData = members[n]->GetCharacterData();

        csString name = EscpXML(members[n]->GetCharacterData()->GetCharName());
        csString race = EscpXML(members[n]->GetCharacterData()->GetRaceInfo()->ReadableRaceGender().GetData());
        buff.Format("<M N=\"%s\" R=\"%s\" H=\"%.2f\" M=\"%.2f\" PS=\"%.2f\" MS=\"%.2f\"/>",
                    name.GetData(), race.GetData(),
                    charData->GetHP() / fixZero(charData->GetMaxHP().Current()) * 100,
                    charData->GetMana() / fixZero(charData->GetMaxMana().Current()) * 100,
                    charData->GetStamina(true) / fixZero(charData->GetMaxPStamina().Current()) * 100,
                    charData->GetStamina(false) / fixZero(charData->GetMaxMStamina().Current()) * 100);

        list.Append(buff);
    }
    list.Append("</L>");

    psGUIGroupMessage msg(0,psGUIGroupMessage::MEMBERS,list.GetData());
    if(msg.valid)
        Broadcast(msg.msg);
    else
    {
        Bug1("Could not create valid psGUIGroupMessage for broadcast.\n");
    }

}

bool PlayerGroup::HasMember(gemActor* member, bool IncludePets)
{
    for(size_t i = 0; i < members.GetSize(); i++)
    {
        if(members[i] == member || (IncludePets && members[i]->IsMyPet(member)))
        {
            return true;
        }
    }
    return false;
}

// group duel functions

bool PlayerGroup::AddDuelGroup(PlayerGroup* OtherGroup)
{
    if(IsInDuelWith(OtherGroup)) //duplicate
        return false;

    DuelGroups.Push(OtherGroup);

    return true;
}

void PlayerGroup::RemoveDuelGroup(PlayerGroup* OtherGroup)
{
    DuelGroups.Delete(OtherGroup);
}

void PlayerGroup::NotifyDuelYield(PlayerGroup* OtherGroup)
{
    manager->GroupChat(leader,"%s's group has yielded!",OtherGroup->GetLeader()->GetName());
}

void PlayerGroup::DuelYield()
{
    for(size_t pos = 0; pos < DuelGroups.GetSize(); pos++)
    {
        DuelGroups.Get(pos)->RemoveDuelGroup(this);
        DuelGroups.Get(pos)->NotifyDuelYield(this);
    }

    DuelGroups.DeleteAll();
}

bool PlayerGroup::IsInDuel()
{
    return DuelGroups.GetSize() > 0;
}

bool PlayerGroup::IsInDuelWith(PlayerGroup* OtherGroup)
{
    return DuelGroups.Find(OtherGroup) != csArrayItemNotFound;
}


//---------------------------------------------------------------------------

int PlayerGroup::next_id = 1;

GroupManager::GroupManager(ClientConnectionSet* cs, ChatManager* chat)
{
    clients    = cs;
    chatserver = chat;  // Needed to GROUPSAY things.

    eventmanager = psserver->GetEventManager();
    Subscribe(&GroupManager::HandleGroupCommand, MSGTYPE_GROUPCMD, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
}

GroupManager::~GroupManager()
{
    //do nothing
}

void GroupManager::HandleGroupCommand(MsgEntry* me, Client* client)
{
    psGroupCmdMessage msg(me);
    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Failed to parse psGroupCmdMessage from client %u.\n",me->clientnum);
        return;
    }

    if(msg.command == "/invite")
    {
        Invite(msg,client);
    }
    else if(msg.command == "/groupremove")
    {
        RemovePlayerFromGroup(msg,client->GetActor());
    }
    else if(msg.command == "/disband")
    {
        Disband(msg,client->GetActor());
    }
    else if(msg.command == "/leavegroup")
    {
        Leave(msg,client->GetActor());
    }
    else if(msg.command == "/groupmembers")
    {
        ListMembers(msg,client->GetActor());
    }
    else if(msg.command == "/groupchallenge")
    {
        Challenge(msg,client);
    }
    else if(msg.command == "/groupyield")
    {
        Yield(msg,client);
    }
    else
    {
        Error2("Group command %s is not supported by server.", msg.command.GetDataSafe());
    }
}

void GroupManager::Challenge(psGroupCmdMessage &msg,Client* Challenger)
{
    csRef<PlayerGroup> ChallengerGroup = Challenger->GetActor()->GetGroup();

    if(!ChallengerGroup)
    {
        psserver->SendSystemError(Challenger->GetClientNum(),"You aren't a member of a group.");
        return;
    }

    if(!ChallengerGroup->IsLeader(Challenger->GetActor())) //only the group leader can challenge other groups
    {
        psserver->SendSystemError(Challenger->GetClientNum(),"Only group leaders can challenge other groups.");
        return;
    }

    csString PlayerName = msg.player;
    //Check for empty player name. If we don't do this, the server crashes
    if(PlayerName=="")
    {
        psserver->SendSystemError(Challenger->GetClientNum(), "Please specify the player name of a player in a group to challenge your group.");
        return;
    }

    PlayerName = NormalizeCharacterName(PlayerName);

    // Check to see if the player is trying to invite themself.
    if(PlayerName == Challenger->GetName())
    {
        psserver->SendSystemError(Challenger->GetClientNum(), "Cannot challenge yourself.");
        return;
    }

    // challenged player must be online when invited
    Client* Challenged = clients->Find(PlayerName);

    if(!Challenged)
    {
        psserver->SendSystemError(Challenger->GetClientNum(),"%s is not online right now.", PlayerName.GetData());
        return;
    }

    if(!Challenged->GetActor()->InGroup())
    {
        psserver->SendSystemError(Challenger->GetClientNum(),"That player is not a member of a group.");
        return;
    }

    csRef<PlayerGroup> ChallengedGroup = Challenged->GetActor()->GetGroup();

    if(ChallengerGroup == ChallengedGroup)
    {
        psserver->SendSystemError(Challenger->GetClientNum(),"Your group cannot challenge itself.");
        return;
    }

    if(ChallengerGroup->IsInDuelWith(ChallengedGroup))
    {
        psserver->SendSystemError(Challenger->GetClientNum(),"This group is already in challenge with you.");
        return;
    }

    //we have to work with the guildleader: so now that we know all is ok we change our "target" to the leader
    Challenged = ChallengedGroup->GetLeader()->GetClient();

    /**
     * Notify the challenger that the invitation has been sent.
     * Notify the challenged that he/she has been invited and ask them to confirm.
     */
    psserver->SendSystemInfo(Challenger->GetClientNum(),"Confirming group challenge invitation with %s now...",Challenged->GetName());

    csString Invitation;
    Invitation.Format("%s has challenged your group.  Would you like to allow it?", Challenger->GetName());
    PendingGroupChallenge* pnew = new PendingGroupChallenge(Challenger, Challenged, Invitation, ChallengerGroup);

    // Track who is invited where, to verify confirmations
    psserver->questionmanager->SendQuestion(pnew);
}

void PendingGroupChallenge::HandleAnswer(const csString &answer)
{
    Client* client = psserver->GetConnections()->Find(clientnum);
    //is the client valid, still in a group and is still leader?
    if(!client  ||  !client->GetActor()->InGroup())
        return;


    PendingInvite::HandleAnswer(answer);

    if(answer == "yes")
        psserver->groupmanager->HandleChallengeGroup(this);
}

void GroupManager::HandleChallengeGroup(PendingGroupChallenge* invite)
{
    csRef<PlayerGroup> ChallengerGroup = FindGroup(invite->groupID);
    if(!ChallengerGroup)
        return;

    Client* ChallengedClient = clients->Find(invite->clientnum);
    if(!ChallengedClient)
        return;

    csRef<PlayerGroup> ChallengedGroup = ChallengedClient->GetActor()->GetGroup();
    if(!ChallengedGroup || !ChallengedGroup->IsLeader(ChallengedClient->GetActor()))
        return;

    Client* ChallengerClient = clients->Find(invite->inviterClientNum);


    if(!AddGroupChallenge(ChallengerGroup, ChallengedGroup))
    {
        Error1("Error challenging group!\n");
        return;
    }

    if(ChallengerClient != NULL)
        GroupChat(ChallengerClient->GetActor(),"%s's group is now in challenge with this group!",invite->inviteeName.GetData());
    GroupChat(ChallengedClient->GetActor(),"%s's group is now in challenge with this group!",invite->inviterName.GetData());
}

bool GroupManager::AddGroupChallenge(PlayerGroup* ChallengerGroup, PlayerGroup* ChallengedGroup)
{
    //we have to add the challenge to both groups. If at this point a group has a duplicate we have a bug
    if(ChallengerGroup->AddDuelGroup(ChallengedGroup))
        if(ChallengedGroup->AddDuelGroup(ChallengerGroup))
            return true;
    return false;
}

void GroupManager::Yield(psGroupCmdMessage &msg,Client* Yielder)
{
    csRef<PlayerGroup> Group = Yielder->GetActor()->GetGroup();

    if(!Group)
    {
        psserver->SendSystemError(Yielder->GetClientNum(),"You aren't a member of a group.");
        return;
    }

    if(Group->IsLeader(Yielder->GetActor())) //only the group leader can yield to the group challengers
    {
        Group->DuelYield();
        GroupChat(Yielder->GetActor(),"Our group has yielded to all our challengers!");
    }
    else
    {
        psserver->SendSystemError(Yielder->GetClientNum(), "Only the group leader can yield.");
    }
}

void GroupManager::Invite(psGroupCmdMessage &msg,Client* inviter)
{
    csRef<PlayerGroup> group = inviter->GetActor()->GetGroup();

    csString playerName = msg.player;
    //Check for empty player name. If we don't do this, the server crashes
    if(playerName=="")
    {
        psserver->SendSystemError(inviter->GetClientNum(), "Please specify the player name to invite to your group.");
        return;
    }

    playerName = NormalizeCharacterName(playerName);

    // Check to see if the player is trying to invite themself.
    if(playerName == inviter->GetName())
    {
        psserver->SendSystemError(inviter->GetClientNum(), "Cannot invite yourself to a group.");
        return;
    }

    if(group && !group->IsLeader(inviter->GetActor()))
    {
        psserver->SendSystemError(inviter->GetClientNum(),"Only group leaders can invite new members.");
        return;
    }

    // invited player must be online when invited
    Client* invitee = clients->Find(playerName);

    if(!invitee)
    {
        psserver->SendSystemError(inviter->GetClientNum(),"%s is not online right now.",
                                  (const char*) playerName);
        return;
    }

    if(invitee->IsSuperClient())
    {
        psserver->SendSystemError(inviter->GetClientNum(),"You cannot invite npc %s.",
                                  (const char*) playerName);
        return;
    }

    if(invitee->GetActor()->InGroup())
    {
        psserver->SendSystemError(inviter->GetClientNum(),"That player is already a member of a group.");
        return;
    }

    if(!group)
    {
        // Create a new group with client as leader
        group = NewGroup(inviter->GetActor());
        psserver->SendSystemInfo(inviter->GetClientNum(),"You are group leader for the new group.");
    }

    /**
     * Notify the inviter that the invitation has been sent.
     * Notify the invitee that he/she has been invited and ask them to confirm.
     */
    psserver->SendSystemInfo(inviter->GetClientNum(),"Confirming group invitation with %s now...",invitee->GetName());

    csString invitation;
    invitation.Format("%s has invited you to join a group%s.  Would you like to join it?",
                      inviter->GetName(), group->IsInDuel() ? " which is currently in a duel" : "");
    PendingGroupInvite* pnew = new PendingGroupInvite(inviter,
            invitee,
            invitation,
            group);

    // Track who is invited where, to verify confirmations
    psserver->questionmanager->SendQuestion(pnew);
}

void PendingGroupInvite::HandleAnswer(const csString &answer)
{
    Client* client = psserver->GetConnections()->Find(clientnum);
    if(!client  ||  client->GetActor()->InGroup())
        return;


    PendingInvite::HandleAnswer(answer);

    if(answer == "yes")
        psserver->groupmanager->HandleJoinGroup(this);
}


void GroupManager::Disband(psGroupCmdMessage &msg,gemActor* client)
{
    csRef<PlayerGroup> group = client->GetGroup();

    if(!group)
    {
        psserver->SendSystemInfo(client->GetClientID(),"You are not in a group.");
        return;
    }

    if(!group->IsLeader(client))
    {
        psserver->SendSystemError(client->GetClientID(), "Only the group leader can disband a group.");
        return;
    }

    group->Disband();
    Remove(group);

    psserver->SendSystemInfo(client->GetClientID(),"You have disbanded the group.");
}

PlayerGroup* GroupManager::FindGroup(int id)
{
    // TODO: Change to hashmap
    for(size_t grpNum=0; grpNum < groups.GetSize(); grpNum++)
        if(groups[grpNum]->GetGroupID() == id)
            return groups[grpNum];
    return NULL;
}

void GroupManager::HandleJoinGroup(PendingGroupInvite* invite)
{
    PlayerGroup* group = FindGroup(invite->groupID);
    if(group == NULL)
        return;

    Client* inviteeClient = clients->Find(invite->clientnum);
    Client* inviterClient = clients->Find(invite->inviterClientNum);

    if(!inviteeClient   ||   !AddPlayerToGroup(group,inviteeClient->GetActor()))
    {
        Error1("Error joining group!\n");
        return;
    }

    if(inviterClient != NULL)
        GroupChat(inviterClient->GetActor(),"Player %s has joined the group!",invite->inviteeName.GetData());
}

void GroupManager::Leave(psGroupCmdMessage &msg,gemActor* client)
{
    csRef<PlayerGroup> group = client->GetGroup();
    if(!group)
    {
        psserver->SendSystemInfo(client->GetClientID(), "You are not in a group.");
        return;
    }

    SendLeave(client);
    psserver->SendSystemInfo(client->GetClientID(),"You have left the group.");

    // If this leave the group empty it will be deleted when going out of this scope.
    group->Remove(client);
}

void GroupManager::ListMembers(psGroupCmdMessage &msg,gemActor* client)
{
    csRef<PlayerGroup> group = client->GetGroup();
    if(group)
    {
        group->ListMembers(client);
    }
    else
    {
        psserver->SendSystemInfo(client->GetClientID(),"You are not in a group");
    }
}

void GroupManager::SendGroup(gemActor* client)
{
    csRef<PlayerGroup> group = client->GetGroup();
    csString buff;
    buff.Format("<GROUP ID=\"%d\" />",group->GetGroupID());
    psGUIGroupMessage msg(client->GetClientID(),psGUIGroupMessage::GROUP,buff);
    msg.SendMessage();
}

void GroupManager::SendLeave(gemActor* client)
{
    psGUIGroupMessage msg(client->GetClientID(),psGUIGroupMessage::LEAVE,"");
    msg.SendMessage();
}

csPtr<PlayerGroup> GroupManager::NewGroup(gemActor* leader)
{
    csRef<PlayerGroup> group;
    group.AttachNew(new PlayerGroup(this,leader));

    groups.Push(group);

    SendGroup(leader);

    group->BroadcastMemberList();

    return csPtr<PlayerGroup>(group);
}

bool GroupManager::AddPlayerToGroup(PlayerGroup* group, gemActor* client)
{
    group->Add(client);

    SendGroup(client);

    group->BroadcastMemberList();

    return true;
}

void GroupManager::RemovePlayerFromGroup(psGroupCmdMessage &msg,gemActor* client)
{
    Client* targetClient;
    gemActor* targetClientActor;
    csRef<PlayerGroup> group;

    group = client->GetGroup();
    if(!group)
    {
        psserver->SendSystemInfo(client->GetClientID(), "You are not in a group.");
        return;
    }

    if(!msg.player || msg.player.IsEmpty())
    {
        psserver->SendSystemInfo(client->GetClientID(), "You must specify the name of the member you want to remove.");
        return;
    }

    targetClient = clients->Find(NormalizeCharacterName(msg.player));
    if(!targetClient)
    {
        psserver->SendSystemInfo(client->GetClientID(), "Cound not find a player named %s.", msg.player.GetData());
        return;
    }

    if(!group->IsLeader(client))
    {
        psserver->SendSystemInfo(client->GetClientID(), "You must be the group leader to remove members.", msg.player.GetData());
        return;
    }

    targetClientActor = targetClient->GetActor();
    csRef<PlayerGroup> targetGroup = targetClientActor->GetGroup();
    if(!targetGroup || targetGroup->GetGroupID() != group->GetGroupID())
    {
        psserver->SendSystemInfo(client->GetClientID(), "Player %s is not a member of your group.", msg.player.GetData());
        return;
    }

    SendLeave(targetClientActor);
    psserver->SendSystemInfo(targetClientActor->GetClientID(),"You were removed from the group.");
    group->Remove(targetClientActor);
}

void GroupManager::Remove(PlayerGroup* group)
{
    groups.Delete(group);
}

void GroupManager::GroupChat(gemActor* client, const char* fmt, ...)
{
    csString text;
    va_list args;

    va_start(args, fmt);
    text.FormatV(fmt,args);
    va_end(args);

    psChatMessage groupmsg(client->GetClientID(),0,"System",0,text,CHAT_GROUP, false);
    if(groupmsg.valid)
    {
        groupmsg.msg->Reset();
        groupmsg.FireEvent();
    }
    else
    {
        Bug2("Could not create a valid psChatMessage for client %u.\n",client->GetClientID());
    }
}

