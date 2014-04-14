/*
* guildmanager.cpp
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


#include <psconfig.h>
#include <string.h>
#include <memory.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/serverconsole.h"
#include "util/psxmlparser.h"
#include "util/eventmanager.h"
#include "util/psdatabase.h"        // Database
#include "util/pserror.h"
#include "util/log.h"
#include "util/psconst.h"

#include "bulkobjects/pscharacter.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psaccountinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "guildmanager.h"
#include "client.h"
#include "clients.h"
#include "gem.h"
#include "playergroup.h"
#include "cachemanager.h"
#include "psserver.h"
#include "chatmanager.h"
#include "netmanager.h"
#include "invitemanager.h"
#include "globals.h"
#include "psserverchar.h"


class PendingGuildInvite : public PendingInvite
{
public:
    int guildID;

    PendingGuildInvite(Client* inviter,
                       Client* invitee,
                       const char* question,
                       int guildID)
        : PendingInvite(inviter, invitee, true,
                        question,"Accept","Decline",
                        "You have asked %s to join your guild.",
                        "%s has invited you to join a guild.",
                        "%s has joined your guild.",
                        "You have joined the guild.",
                        "%s has declined your invitation.",
                        "You have declined %s's invitation.",
                        psQuestionMessage::generalConfirm)
    {
        this->guildID = guildID;
    };
    virtual ~PendingGuildInvite() {}

    void HandleAnswer(const csString &answer);
};

/**
 * This invitation isn't really an invitation. It is used when somebody tries to invite a player that is already
 * member of a guild and this guild is secret. We cannot tell the inviter that the player is already in a guild.
 * We must keep this information secret. So we send invitation that explains the situation to the
 * secret member and that cannot be accepted. In this way, his membership in secret guild won't be revealed.
 */
class PendingSecretGuildMemberInvite : public PendingInvite
{
public:
    PendingSecretGuildMemberInvite(Client* inviter,
                                   Client* invitee,
                                   const char* question,
                                   int guildID)
        : PendingInvite(inviter, invitee, true,
                        question,"Decline","",
                        "You have asked %s to join your guild.",
                        "%s has invited you to join a guild. You are already member of a secret guild so you cannot accept this offer.",
                        "",
                        "",
                        "%s has declined your invitation.",
                        "You have declined %s's invitation.",
                        psQuestionMessage::secretGuildNotify)
    {
        cannotAccept = true;
    };
};

/** A structure to hold the clients that are pending on guild war challenges.
 */
class PendingGuildWarInvite : public PendingInvite
{
public:

    PendingGuildWarInvite(Client* inviter,
                          Client* invitee,
                          const char* question)
        : PendingInvite(inviter, invitee, true,
                        question,"Accept","Decline",
                        "You have challenged %s to a guild war.",
                        "%s has challenged you to a guild war.",
                        "%s has accepted your challenge.",
                        "You have accepted %s's challenge.",
                        "%s has declined your challenge.",
                        "You have declined %s's challenge.",
                        psQuestionMessage::generalConfirm)
    {
    }

    virtual ~PendingGuildWarInvite() {}

    virtual void HandleAnswer(const csString &answer);
};

class psGuildCheckEvent : public psGameEvent
{
protected:
    int guildid;
    GuildManager* guildmanager;

public:
    psGuildCheckEvent(int guild_id, GuildManager* gm)
        : psGameEvent(0,GUILD_KICK_GRACE *60*1000,"psGuildCheckEvent")
    {
        guildid      = guild_id;
        guildmanager = gm;
    }

    void Trigger()
    {
        guildmanager->RequirementsDeadline(guildid);
    }
};


/**
  * Returns guild of 'client'.
  * If he is not in a guild, NULL is returned and text message "You are not in a guild." is sent to the client
  */
psGuildInfo* GetClientGuild(Client* client)
{
    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psSystemMessage newmsg(client->GetClientNum(),MSG_ERROR,"You are not in a guild.");
        newmsg.SendMessage();
        return NULL;
    }
    return guild;
}









GuildManager::GuildManager(ClientConnectionSet* cs,
                           ChatManager* chat)
{
    clients    = cs;
    chatserver = chat;  // Needed to GUILDSAY things.

    xml.AttachNew(new csTinyDocumentSystem);
    CS_ASSERT(xml);

    Subscribe(&GuildManager::HandleCmdMessage,MSGTYPE_GUILDCMD,REQUIRE_READY_CLIENT);
    Subscribe(&GuildManager::HandleGUIMessage,MSGTYPE_GUIGUILD,REQUIRE_READY_CLIENT);
    Subscribe(&GuildManager::HandleMOTDSet,MSGTYPE_GUILDMOTDSET,REQUIRE_ANY_CLIENT);
}

GuildManager::~GuildManager()
{
    for(size_t i = 0; i < notifySubscr.GetSize(); i++)
        delete notifySubscr[i];
    notifySubscr.DeleteAll();
}


void GuildManager::HandleCmdMessage(MsgEntry* me,Client* client)
{
    psGuildCmdMessage msg(me);
    if(!msg.valid)
    {
        psserver->SendSystemError(me->clientnum, "Command not supported by server yet.");
        Error2("Failed to parse psGuildCmdMessage from client %u.",me->clientnum);
        return;
    }

    int clientnum = client->GetClientNum();

    if(msg.command == "/newguild")
    {
        CreateGuild(msg,client);
    }
    else if(msg.command == "/endguild")
    {
        EndGuild(msg,client);
    }
    else if(msg.command == "/guildname")
    {
        ChangeGuildName(msg,client);
    }
    else if(msg.command == "/guildinvite")
    {
        Invite(msg,client);
    }
    else if(msg.command == "/guildremove")
    {
        Remove(msg,client);
    }
    else if(msg.command == "/guildlevel")
    {
        Rename(msg,client);
    }
    else if(msg.command == "/guildpromote")
    {
        Promote(msg,client);
    }
    else if(msg.command == "/guildmembers")
    {
        ListMembers(msg,client);
    }
    else if(msg.command == "/guildsecret")
    {
        Secret(msg, client);
    }
    else if(msg.command == "/guildweb")
    {
        Web(msg, client);
    }
    else if(msg.command == "/guildmotd")
    {
        MOTD(msg,client);
    }
    else if(msg.command == "/guildwar")
    {
        GuildWar(msg,client);
    }
    else if(msg.command == "/guildyield")
    {
        GuildYield(msg,client);
    }
    else if(msg.command == "/guildpoints")
    {
        SendGuildPoints(msg,client);
    }
    else if(msg.command == "/newalliance")
    {
        NewAlliance(msg,client);
    }
    else if(msg.command == "/getmemberpermissions")
    {
        GetMemberPermissions(msg,client);
    }
    else if(msg.command == "/setmemberpermissions")
    {
        SetMemberPermissions(msg,client);
    }
    else if(msg.command == "/allianceinvite")
    {
        AllianceInvite(msg,client);
    }
    else if(msg.command == "/allianceremove")
    {
        AllianceRemove(msg,client);
    }
    else if(msg.command == "/allianceleave")
    {
        AllianceLeave(msg,client);
    }
    else if(msg.command == "/allianceleader")
    {
        AllianceLeader(msg,client);
    }
    else if(msg.command == "/endalliance")
    {
        EndAlliance(msg,client);
    }
    else
    {
        psserver->SendSystemError(clientnum,"Command not supported by server yet.");
    }
}

void GuildManager::HandleGUIMessage(MsgEntry* me,Client* client)
{
    psGUIGuildMessage msg(me);
    if(!msg.valid)
    {
        Error2("Failed to parse psGUIGuildMessage from client %u.",me->clientnum);
        return;
    }
    csRef<iDocument> doc  = xml->CreateDocument();

    const char* error = doc->Parse(msg.commandData);
    if(error)
    {
        Error2("Error in XML: %s", error);
        return;
    }

    switch(msg.command)
    {
        case psGUIGuildMessage::SUBSCRIBE_GUILD_DATA:
            HandleSubscribeGuildData(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::UNSUBSCRIBE_GUILD_DATA:
            UnsubscribeGuildData(client);
            break;
        case psGUIGuildMessage::SET_ONLINE:
            HandleSetOnline(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::SET_GUILD_NOTIFICATION:
            HandleSetGuildNotifications(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::SET_ALLIANCE_NOTIFICATION:
            HandleSetAllianceNotifications(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::SET_LEVEL_RIGHT:
            HandleSetLevelRight(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::SET_MEMBER_POINTS:
            HandleSetMemberPoints(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::SET_MAX_GUILD_POINTS:
            HandleSetMaxMemberPoints(client, doc->GetRoot());
            break;
        case psGUIGuildMessage::SET_MEMBER_PUBLIC_NOTES:
            HandleSetMemberNotes(client, doc->GetRoot(), true);
            break;
        case psGUIGuildMessage::SET_MEMBER_PRIVATE_NOTES:
            HandleSetMemberNotes(client, doc->GetRoot(), false);
            break;
    }
}

void GuildManager::HandleMOTDSet(MsgEntry* me,Client* client)
{
    psGuildMOTDSetMessage msg(me);
    if(!msg.valid)
    {
        Error2("Failed to parse psGuildMOTDSetMessage from client %u.",me->clientnum);
    }

    if(CheckClientRights(client,RIGHTS_EDIT_GUILD,"You don't have permission to change settings of your guild."))
    {
        if(msg.guildmotd.Length() > 200)
        {
            psserver->SendSystemError(client->GetClientNum(),"The MOTD you tried to save is %i character long, the maximum is 200.",msg.guildmotd.Length());
            return;
        }
        psGuildInfo* gi = client->GetActor()->GetGuild();
        gi->SetMOTD(msg.guildmotd);
        //Send notify to all guild members
        psSystemMessage newmsg(client->GetClientNum(), MSG_INFO,
                               "New guild message of the day: %s", gi->GetMOTD().GetData());
        psserver->GetEventManager()->Broadcast(newmsg.msg,NetBase::BC_GUILD,gi->GetID());
        // Refresh MOTD for all guild members
        csString tip;
        if(psserver->GetCacheManager()->GetTipLength() > 0)
            psserver->GetCacheManager()->GetTipByID(psserver->GetRandom(psserver->GetCacheManager()->GetTipLength()), tip);
        psMOTDMessage motd(client->GetClientNum(), tip, psserver->GetMOTD(), gi->GetMOTD().GetData(), gi->GetName());
        psserver->GetEventManager()->Broadcast(motd.msg,NetBase::BC_GUILD,gi->GetID());
    }
}

/** Returns guild level of character connected as 'client' (zero if he has no level)
  */
int GuildManager::GetClientLevel(Client* client)
{
    psGuildLevel* level;

    level = client->GetActor()->GetGuildLevel();
    if(level != NULL)
        return level->level;
    else
        return 0;
}

/** Checks if 'client' has privilege 'priv' in his guild.
  */
bool GuildManager::CheckClientRights(Client* client, GUILD_PRIVILEGE priv)
{
    psGuildMember* member;

    member = client->GetActor()->GetGuildMembership();
    if(member == NULL)
        return false;

    if(member->HasRights(priv))
        return true;
    else
        return false;
}

/** Checks if 'client' has privilege 'priv' in his guild.
  * If not, he is sent 'denialMsg'.
  */
bool GuildManager::CheckClientRights(Client* client, GUILD_PRIVILEGE priv, const char* denialMsg)
{
    if(CheckClientRights(client, priv))
        return true;
    else
    {
        psserver->SendSystemError(client->GetClientNum(),denialMsg);
        return false;
    }
}

bool GuildManager::RetrieveOnlineOnly(iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("r");
    if(!topNode)
        return true;

    csString onlineOnly = topNode->GetAttributeValue("onlineonly");
    return (onlineOnly == "yes");
}

bool GuildManager::RetrieveGuildNotifications(iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("r");
    if(!topNode)
        return false;

    csString guildNotifications = topNode->GetAttributeValue("guildnotifications");
    return (guildNotifications == "yes");
}

bool GuildManager::RetrieveAllianceNotifications(iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("r");
    if(!topNode)
        return false;

    csString allianceNotifications = topNode->GetAttributeValue("alliancenotifications");
    return (allianceNotifications == "yes");
}

void GuildManager::HandleSubscribeGuildData(Client* client,iDocumentNode* root)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
    {
        psGUIGuildMessage cmd(clientnum,psGUIGuildMessage::NOT_IN_GUILD,"");
        cmd.SendMessage();
        return;
    }

    // check if already subscribed
    GuildNotifySubscription* subscr = FindNotifySubscr(client);
    if(subscr != NULL)
        return;

    subscr =
        new GuildNotifySubscription(guild->GetID(), client->GetClientNum(), RetrieveOnlineOnly(root));
    notifySubscr.Push(subscr);

    SendGuildData(client);
    SendLevelData(client);
    SendMemberData(client, subscr->onlineOnly);
    SendAllianceData(client);
}

void GuildManager::UnsubscribeGuildData(Client* client)
{
    GuildNotifySubscription* subscr = FindNotifySubscr(client);
    if(subscr == NULL)
        return;

    delete subscr;
    notifySubscr.Delete(subscr);
}

void GuildManager::HandleSetOnline(Client* client,iDocumentNode* root)
{
    GuildNotifySubscription* subscr = FindNotifySubscr(client);

    if(subscr != NULL)
    {
        subscr->onlineOnly = RetrieveOnlineOnly(root);
        SendMemberData(client, subscr->onlineOnly);
    }
}

void GuildManager::HandleSetGuildNotifications(Client* client,iDocumentNode* root)
{
    if(client && client->GetCharacterData())
        client->GetCharacterData()->SetGuildNotifications(RetrieveGuildNotifications(root));
}

void GuildManager::HandleSetAllianceNotifications(Client* client,iDocumentNode* root)
{
    if(client && client->GetCharacterData())
        client->GetCharacterData()->SetAllianceNotifications(RetrieveAllianceNotifications(root));
}

void GuildManager::SendNotifications(int guild, int msg)
{
    Client* client;
    size_t i;

    for(i=0; i < notifySubscr.GetSize(); i++)
        if(notifySubscr[i]->guild == guild)
        {
            client = clients->Find(notifySubscr[i]->clientnum);
            if(!client)
                continue;

            psGuildInfo* guild = client->GetCharacterData()->GetGuild();
            if(guild == NULL)
            {
                Error2("Error - character %s is not in any guild but it has guild notifications subscribed", client->GetName());
                continue;
            }

            psUpdatePlayerGuildMessage update(client->GetClientNum(),
                                              client->GetActor()->GetEID(),
                                              guild->GetName());

            if(guild->IsSecret())   // If this is a secret guild, we should only broadcast to members
                psserver->GetEventManager()->Broadcast(update.msg, NetBase::BC_GUILD, guild->GetID());
            else
                psserver->GetEventManager()->Broadcast(update.msg, NetBase::BC_EVERYONE);

            switch(msg)
            {
                case psGUIGuildMessage::GUILD_DATA:
                    SendGuildData(client);
                    break;
                case psGUIGuildMessage::LEVEL_DATA:
                    SendLevelData(client);
                    break;
                case psGUIGuildMessage::MEMBER_DATA:
                    SendMemberData(client, notifySubscr[i]->onlineOnly);
                    break;
                case psGUIGuildMessage::ALLIANCE_DATA:
                    SendAllianceData(client);
                    break;
            }
        }
}

void GuildManager::SendAllianceNotifications(psGuildAlliance* alliance)
{
    int memberNum;

    for(memberNum=0; memberNum < (int)alliance->GetMemberCount(); memberNum++)
        SendNotifications(alliance->GetMember(memberNum)->GetID(), psGUIGuildMessage::ALLIANCE_DATA);
}

void GuildManager::SendNoAllianceNotifications(psGuildAlliance* alliance)
{
    int memberNum;

    for(memberNum=0; memberNum < (int)alliance->GetMemberCount(); memberNum++)
    {
        SendNoAllianceNotifications(alliance->GetMember(memberNum));
    }
}

void GuildManager::SendNoAllianceNotifications(psGuildInfo* guild)
{
    size_t notifNum;

    for(notifNum=0; notifNum < notifySubscr.GetSize(); notifNum++)
        if(notifySubscr[notifNum]->guild == guild->GetID())
        {
            Client* client = clients->Find(notifySubscr[notifNum]->clientnum);
            if(client != NULL)
            {
                psGUIGuildMessage cmd(client->GetClientNum(),psGUIGuildMessage::ALLIANCE_DATA,"<alliance name=\"\"></alliance>");
                cmd.SendMessage();
            }
        }
}

GuildNotifySubscription* GuildManager::FindNotifySubscr(Client* client)
{
    int clientnum;
    size_t i;

    clientnum = client->GetClientNum();

    for(i=0; i < notifySubscr.GetSize(); i++)
        if(notifySubscr[i]->clientnum == clientnum)
            return notifySubscr[i];
    return NULL;
}

bool GuildManager::ParseRightString(csString privilege,  GUILD_PRIVILEGE &right)
{
    if(privilege == "view_chat")
    {
        right = RIGHTS_VIEW_CHAT;
    }
    else if(privilege == "chat")
    {
        right = RIGHTS_CHAT;
    }
    else if(privilege == "invite")
    {
        right = RIGHTS_INVITE;
    }
    else if(privilege == "remove")
    {
        right = RIGHTS_REMOVE;
    }
    else if(privilege == "promote")
    {
        right = RIGHTS_PROMOTE;
    }
    else if(privilege == "edit_level")
    {
        right = RIGHTS_EDIT_LEVEL;
    }
    else if(privilege == "edit_points")
    {
        right = RIGHTS_EDIT_POINTS;
    }
    else if(privilege == "edit_guild")
    {
        right = RIGHTS_EDIT_GUILD;
    }
    else if(privilege == "edit_public")
    {
        right = RIGHTS_EDIT_PUBLIC;
    }
    else if(privilege == "edit_private")
    {
        right = RIGHTS_EDIT_PRIVATE;
    }
    else if(privilege == "alliance_view_chat")
    {
        right = RIGHTS_VIEW_CHAT_ALLIANCE;
    }
    else if(privilege == "alliance_chat")
    {
        right = RIGHTS_CHAT_ALLIANCE;
    }
    else if(privilege == "guild_bank")
    {
        right = RIGHTS_USE_BANK;
    }
    else
    {
        Error2("Unkown privilege %s",privilege.GetData());
        return false;
    }
    return true;
}

void GuildManager::HandleSetLevelRight(Client* client, iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("l");
    if(!topNode)
    {
        Error1("Error in XML: Can't find node l");
        return;
    }
    int level = topNode->GetAttributeValueAsInt("level");
    csString privilege = topNode->GetAttributeValue("privilege");
    csString state = topNode->GetAttributeValue("state");

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
    {
        psserver->SendSystemError(client->GetClientNum(),"You are not in a guild.");
        return;
    }

    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, RIGHTS_EDIT_LEVEL, "You do not have the rights to edit privileges in your guild."))
            return;
        if(GetClientLevel(client) <= level)
        {
            psserver->SendSystemError(client->GetClientNum(),"You do not have the rights to edit privileges of this level in your guild.");
            return;
        }
    }

    GUILD_PRIVILEGE right;
    if(!ParseRightString(privilege, right))
        return;

    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, right, "You cannot change a privilege that you do not have yourself."))
        {
            return;
        }
    }

    if(guild->SetPrivilege(level,right,state=="on"))
    {
        psGuildLevel* lev = guild->FindLevel(level);

        if(lev != NULL)
            psserver->SendSystemInfo(client->GetClientNum(),"Privilege %s for level %s was turned %s",
                                     privilege.GetData(),lev->title.GetData(),state.GetData());
    }

    SendNotifications(guild->GetID(), psGUIGuildMessage::LEVEL_DATA);
}

void GuildManager::HandleSetMemberPoints(Client* client,iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("p");
    if(topNode == NULL) return;

    unsigned int char_id = topNode->GetAttributeValueAsInt("char_id");
    int points = topNode->GetAttributeValueAsInt("points");

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    psGuildMember* member = guild->FindMember(char_id);
    if(member == NULL)
        return;

    if(! IsLeader(client))
    {
        if(!CheckClientRights(client, RIGHTS_EDIT_POINTS, "You do not have the rights to edit points in your guild."))
            return;
        if(GetClientLevel(client) <= member->guildlevel->level)
        {
            psserver->SendSystemError(client->GetClientNum(),"You do not have the rights to edit points of this member.");
            return;
        }
    }

    guild->SetMemberPoints(member, points);

    SendNotifications(guild->GetID(), psGUIGuildMessage::MEMBER_DATA);

    if(member->character)
    {
        Client* destclient = psserver->GetConnections()->FindAccount(member->character->GetAccountId());

        if(destclient)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "%s now has %d points", member->name.GetData(), points);
            psserver->SendSystemInfo(destclient->GetClientNum(), "%s sets your guild points to %d",
                                     client->GetActor()->GetName(), points);
        }
        else
        {
            Error2("Actor %s has no client!", member->name.GetData());
        }
    }
    else
    {
        psserver->SendSystemInfo(client->GetClientNum(), "%s, who is not in Yliakum right now, has %d points", member->name.GetData(), points);
    }
}

void GuildManager::HandleSetMaxMemberPoints(Client* client,iDocumentNode* root)
{
    csRef<iDocumentNode> topNode = root->GetNode("mp");
    if(topNode == NULL) return;

    int points = topNode->GetAttributeValueAsInt("max_guild_points");

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    if(! IsLeader(client))
    {
        if(!CheckClientRights(client, RIGHTS_EDIT_GUILD, "You do not have the rights to change the max guild points in your guild."))
            return;
    }

    guild->SetMaxMemberPoints(points);

    SendNotifications(guild->GetID(), psGUIGuildMessage::GUILD_DATA);

}

void GuildManager::HandleSetMemberNotes(Client* client,iDocumentNode* root, bool isPublic)
{
    csRef<iDocumentNode> topNode = root->GetNode("n");
    if(topNode == NULL) return;

    unsigned int char_id = (unsigned int)topNode->GetAttributeValueAsInt("char_id");
    csString notes = topNode->GetAttributeValue("notes");

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    psGuildMember* member = guild->FindMember(char_id);
    if(member == NULL)
        return;

    if(isPublic)
    {
        if(! IsLeader(client))
        {
            if(!CheckClientRights(client, RIGHTS_EDIT_PUBLIC)
                    ||
                    (GetClientLevel(client) < member->guildlevel->level))
            {
                psserver->SendSystemError(client->GetClientNum(),"You do not have the rights to edit public notes of this member.");
                return;
            }
        }
    }
    else
    {
        //allows to add private notes to others and to edit them for the member target of the
        //notes (else he will get an infinitely filling entry which he can't clean when read)
        if(!CheckClientRights(client, RIGHTS_EDIT_PRIVATE) && client->GetPID() != char_id)
        {
            psserver->SendSystemError(client->GetClientNum(),"You do not have the rights to edit private notes.");
            return;
        }
        if(client->GetPID() != char_id) // if this is not my info
        {
            if(member->private_notes.Length()) // append the notes to the end
                notes = member->private_notes + (member->private_notes.Length() ? "\n" : "") + notes;
        }
    }

    guild->SetMemberNotes(member, notes, isPublic);

    SendNotifications(guild->GetID(), psGUIGuildMessage::MEMBER_DATA);
}

const char* DeNULL(const char* str)
{
    return (str == NULL) ? "" : str;
}

void GuildManager::SendGuildData(Client* client)
{
    int clientnum = client->GetClientNum();
    csString open;

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    csString escpxml_guild = EscpXML(guild->GetName());
    csString escpxml_webpage = EscpXML(guild->GetWebPage());
    open.Format("<guild name=\"%s\" secret=\"%s\" web_page=\"%s\" max_points=\"%d\"/>",
                escpxml_guild.GetData(),
                guild->IsSecret() ? "yes" : "no",
                escpxml_webpage.GetData(),
                guild->GetMaxMemberPoints());

    psGUIGuildMessage cmd(clientnum,psGUIGuildMessage::GUILD_DATA,open);
    cmd.SendMessage();
}

const char* BoolToText(bool b)
{
    return b ? "true" : "false";
}

void GuildManager::SendLevelData(Client* client)
{
    int clientnum = client->GetClientNum();
    csString open;

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    open.Append("<levellist>");
    csArray<psGuildLevel*>::Iterator lIter = guild->GetLevelIterator();
    while(lIter.HasNext())
    {
        open.Append("<l>");
        psGuildLevel* level = lIter.Next();
        csString escpxml = EscpXML(level->title);
        open.AppendFmt("<title text=\"%s\"/>",escpxml.GetData());
        open.AppendFmt("<level text=\"%d\"/>",level->level);
        open.AppendFmt("<view_chat down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_VIEW_CHAT)));
        open.AppendFmt("<chat down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_CHAT)));
        open.AppendFmt("<invite down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_INVITE)));
        open.AppendFmt("<remove down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_REMOVE)));
        open.AppendFmt("<promote down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_PROMOTE)));
        open.AppendFmt("<edit_level down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_EDIT_LEVEL)));
        open.AppendFmt("<edit_points down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_EDIT_POINTS)));
        open.AppendFmt("<edit_guild down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_EDIT_GUILD)));
        open.AppendFmt("<edit_public down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_EDIT_PUBLIC)));
        open.AppendFmt("<edit_private down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_EDIT_PRIVATE)));
        open.AppendFmt("<alliance_view_chat down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_VIEW_CHAT_ALLIANCE)));
        open.AppendFmt("<alliance_chat down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_CHAT_ALLIANCE)));
        open.AppendFmt("<guild_bank down=\"%s\"/>",BoolToText(level->HasRights(RIGHTS_USE_BANK)));

        open.Append("</l>");
    }
    open.Append("</levellist>");

    psGUIGuildMessage cmd(clientnum,psGUIGuildMessage::LEVEL_DATA,open);
    cmd.SendMessage();
}

void GuildManager::SendMemberData(Client* client,bool onlineOnly)
{
    int clientnum = client->GetClientNum();

    csString online, lastOnline;
    csString sectorName;
    psSectorInfo* sector = NULL;
    csString open;

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    psGuildLevel* level = client->GetCharacterData()->GetGuildLevel();
    if(level == NULL)
        return;

    // Member list
    open.Append("<memberlist>");
    csArray<psGuildMember*>::Iterator mIter = guild->GetMemberIterator();
    while(mIter.HasNext())
    {
        sectorName.Clear();

        psGuildMember* member = mIter.Next();
        Client* memberClient = clients->FindPlayer(member->char_id);
        if(memberClient != NULL)
        {
            online = "yes";
            psCharacter* character = memberClient->GetCharacterData();
            if(character)
            {
                sector = character->GetLocation().loc_sector;
                lastOnline = character->GetLastLoginTime();
                lastOnline.Truncate(16);
            }
            if(sector != NULL)
                sectorName = sector->name;
        }
        else
        {
            online = "no";
            lastOnline = member->last_login.Truncate(16);
        }

        if(online=="yes"  ||  !onlineOnly)
        {
            open.Append("<m>");
            csString escpxml = EscpXML(member->name);
            open.AppendFmt("<name text=\"%s\"/>",escpxml.GetData());
            escpxml = EscpXML(member->guildlevel->title);
            open.AppendFmt("<level text=\"%s\"/>",escpxml.GetData());
            open.AppendFmt("<online text=\"%s\"/>",online.GetData());
            open.AppendFmt("<sector text=\"%s\"/>", EscpXML(sectorName).GetData());
            open.AppendFmt("<lastonline text=\"%s\"/>",lastOnline.GetData());
            open.AppendFmt("<points text=\"%d\"/>",member->guild_points);
            open.Append("</m>");
        }
    }
    open.Append("</memberlist>");

    // Information about members that is not in the member listbox
    open.Append("<memberinfo>");
    csArray<psGuildMember*>::Iterator mIter2 = guild->GetMemberIterator();
    while(mIter2.HasNext())
    {
        psGuildMember* member = mIter2.Next();

        // Only show private notes to yourself
        csString escpxml_privatenotes;
        if(client->GetPID() == member->char_id)
        {
            escpxml_privatenotes = EscpXML(member->private_notes);
        }

        open.AppendFmt("<m char_id=\"%i\" name=\"%s\" public=\"%s\" private=\"%s\" points=\"%i\" level=\"%i\"/>",
                       member->char_id.Unbox(), EscpXML(member->name).GetData(),
                       EscpXML(member->public_notes).GetDataSafe(), escpxml_privatenotes.GetDataSafe(),
                       member->guild_points, member->guildlevel->level);
    }

    open.Append("</memberinfo>");

    open.AppendFmt("<playerinfo char_id=\"%i\" guildnotifications=\"%i\" alliancenotifications=\"%i\"/>",
                   client->GetPID().Unbox(), client->GetCharacterData()->IsGettingGuildNotifications(),
                   client->GetCharacterData()->IsGettingAllianceNotifications());

    psGUIGuildMessage cmd(clientnum,psGUIGuildMessage::MEMBER_DATA,open);
    cmd.SendMessage();
}

csString GuildManager::MakeAllianceMemberXML(psGuildInfo* member, bool allianceLeader)
{
    csString xml;
    csString name, isLeader, leaderName, online;
    psGuildMember* leader;

    name = member->GetName();

    if(allianceLeader)
        isLeader = "leader";
    else
        isLeader.Clear();

    leader = member->FindLeader();
    if(leader != NULL)
    {
        leaderName = leader->name;

        if(clients->FindPlayer(leader->char_id) != NULL)
            online = "yes";
        else
            online = "no";
    }

    csString escpxml_name = EscpXML(name);
    csString escpxml_isleader = EscpXML(isLeader);
    csString escpxml_leadername = EscpXML(leaderName);
    xml.AppendFmt("<member name=\"%s\" isleader=\"%s\" leadername=\"%s\" online=\"%s\"/>",
                  escpxml_name.GetData(), escpxml_isleader.GetData(), escpxml_leadername.GetData(), online.GetData());
    return xml;
}

void GuildManager::SendAllianceData(Client* client)
{
    int clientnum = client->GetClientNum();
    psGuildAlliance* alliance;
    csString xml;
    int memberNum;


    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
        return;

    if(guild->GetAllianceID() != 0)
    {
        alliance = psserver->GetCacheManager()->FindAlliance(guild->GetAllianceID());
        if(alliance == NULL) return;

        csString escpxml = EscpXML(alliance->GetName());
        xml.AppendFmt("<alliance IamLeader=\"%i\" name=\"%s\">", guild==alliance->GetLeader(), escpxml.GetData());
        for(memberNum=0; memberNum < (int)alliance->GetMemberCount(); memberNum++)
        {
            psGuildInfo* member = alliance->GetMember(memberNum);
            xml += MakeAllianceMemberXML(member, member == alliance->GetLeader());
        }
    }
    else
        xml.Append("<alliance IamLeader=\"0\">");
    xml.Append("</alliance>");

    psGUIGuildMessage cmd(clientnum,psGUIGuildMessage::ALLIANCE_DATA,xml);
    cmd.SendMessage();
}

void GuildManager::CheckMinimumRequirements(psGuildInfo* guild, gemActor* notify)
{
    int clientid=0;

    if(!guild->MeetsMinimumRequirements())
    {
        Warning5(LOG_ANY,
                 "Guild <%s>(%d) has %zu members and will be deleted in %d minutes.\n",
                 guild->GetName().GetData(), guild->GetID(), guild->GetMemberCount(), GUILD_KICK_GRACE);

        if(!notify)  // If not auto-notifying the leader, find the leader if online
        {
            if(guild->FindLeader() && guild->FindLeader()->character && guild->FindLeader()->character->GetActor())
                clientid = guild->FindLeader()->character->GetActor()->GetClientID();
        }
        else
            clientid = notify->GetClientID();

        if(clientid)
        {
            psserver->SendSystemInfo(clientid,
                                     "You have %d minutes to meet guild minimum requirements "
                                     "(%d+ members) or your guild will be disbanded.",
                                     GUILD_KICK_GRACE, GUILD_MIN_MEMBERS);
        }

        psGuildCheckEvent* event = new psGuildCheckEvent(guild->GetID(), this);
        psserver->GetEventManager()->Push(event);
    }
    else
    {
        // guild ok. do nothing
    }
}

void GuildManager::RequirementsDeadline(int guild_id)
{
    psGuildInfo* guild = psserver->GetCacheManager()->FindGuild(guild_id);

    if(guild == NULL)
    {
        return;
    }

    if(!guild->MeetsMinimumRequirements())
    {
        printf("Removing guild <%s>(%d) for failure to meet minimum requirements.\n",guild->GetName().GetDataSafe(),guild_id);
        EndGuild(guild,0);
    }
}

void GuildManager::CreateGuild(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    // check if the player is already in a guild
    if(client->GetCharacterData()->GetGuild() != NULL)
    {
        psserver->SendSystemError(clientnum,"You are already in a guild.");
        return;
    }

    if(msg.guildname.Length()==0)
    {
        psserver->SendSystemError(clientnum,"Please specify a guild name.");
        return;
    }

    if(msg.guildname.Length()<5)
    {
        psserver->SendSystemError(clientnum,"Guild name must be at least five characters.");
        return;
    }

    if(msg.guildname.Length() > 25)
    {
        psserver->SendSystemError(clientnum,"Guild name must be at under 25 characters.");
        return;
    }

    if(!FilterGuildName(msg.guildname))
    {
        psserver->SendSystemError(clientnum,"Guild name contains invaild characters (A-Z,a-z and space is allowed).");
        return;
    }

    if(psserver->GetCharManager()->IsBanned(msg.guildname))
    {
        psserver->SendSystemError(clientnum, "The name %s is banned", msg.guildname.GetData());
        return;
    }

    psCharacter* chardata=client->GetCharacterData();
    if(chardata==NULL)
    {
        Error2("Guild creation attempted by character '%s' with no character data!",client->GetName());
        psserver->SendSystemError(clientnum,"An internal server error occcured (could not find your character data).");
        return;
    }

    if(psserver->GetCacheManager()->FindGuild(msg.guildname))
    {
        psserver->SendSystemError(clientnum,"A guild already exists with that name.");
        return;
    }

    if(chardata->Money().GetTotal() < GUILD_FEE)
    {
        psserver->SendSystemError(clientnum,"It costs %d trias to create a guild.",GUILD_FEE);
        return;
    }

    if(!psserver->GetCacheManager()->CreateGuild(msg.guildname, client))
    {
        psserver->SendSystemError(clientnum, db->GetLastError());
        return;
    }
    else
    {
        psserver->SendSystemInfo(clientnum,"Guild '%s' created.  You have been charged %d tria.", (const char*)msg.guildname, GUILD_FEE);
    }

    // Charge the price now
    chardata->SetMoney(chardata->Money() - GUILD_FEE);


    // subscribe client to updates of his guild
    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL) return;

    GuildNotifySubscription* subscr =
        new GuildNotifySubscription(guild->GetID(), client->GetClientNum(), true);
    notifySubscr.Push(subscr);

    // send client data about guild - this will open GuildWindow for him
    SendGuildData(client);
    SendLevelData(client);
    SendMemberData(client, false);
    SendAllianceData(client);

    // Update the player's label
    psUpdatePlayerGuildMessage update(client->GetClientNum(),
                                      client->GetActor()->GetEID(),
                                      guild->GetName());

    update.Multicast(client->GetActor()->GetMulticastClients(),0,0);

    // Now kick off guild minimum requirement timer
    CheckMinimumRequirements(guild,client->GetActor());
}

void GuildManager::EndGuild(psGuildCmdMessage &msg,Client* client)
{
    if(client == NULL)
    {
        return;
    }

    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(!(const char*)msg.guildname || msg.guildname=="")
    {
        psserver->SendSystemError(clientnum,"Guild name must be specified for confirmation purposes.");
        return;
    }

    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, RIGHTS_EDIT_GUILD, "You do not have the rights to disband your guild."))
        {
            return;
        }
    }


    if(!guild->GetName().CompareNoCase(msg.guildname))
    {
        psserver->SendSystemError(clientnum,"The guild name you specified did not match the guild you are in. The guild was not disbanded.");
        return;
    }

    EndGuild(guild,clientnum);
}

void GuildManager::EndGuild(psGuildInfo* guild, int clientnum)
{
    if(guild == NULL)
    {
        Error2("Trying to remove (NULL) guild from client %d",clientnum);
        return;
    }


    //if this guild is in an alliance, it must be removed from it
    psGuildAlliance* alliance = psserver->GetCacheManager()->FindAlliance(guild->GetAllianceID());
    if(alliance)
    {
        //if the guild is in an alliance we need to do some consistency checks.
        //first of all check if the alliance would remain with less than 2 members
        //after disbanding this guild
        if(alliance->GetMemberCount() <= 2)
        {
            //if that's the case we end first of all the alliance
            //notice that after this call the pointer to alliance
            //becomes a dangling pointer till this function exits.
            EndAlliance(alliance, clientnum);
        }
        else
        {
            //Then, if there is still an alliance, we need to check if
            //this guild was the leader and reassign leadership
            if(alliance->GetLeader() == guild)
            {
                //search a member different than this.
                //probably a for is a bit overkill here
                for(size_t i = 0; i < alliance->GetMemberCount(); i++)
                {
                    //we set the first guild we find as leader
                    //and bail out from the for
                    if(alliance->GetMember(i) != guild)
                    {
                        alliance->SetLeader(alliance->GetMember(i));
                        break;
                    }
                }
            }
            //finally remove the guild from the alliance, as the alliance
            //outlived the guild
            alliance->RemoveMember(guild);
        }
    }

    if(!guild->RemoveGuild())
    {
        psserver->SendSystemError(clientnum,db->GetLastError());
        return;
    }

    if(clientnum)
    {
        psserver->SendSystemInfo(clientnum, "Guild has been disbanded.");
    }

    UnsubscribeWholeGuild(guild);

    ClientIterator i(*clients);

    while(i.HasNext())
    {
        Client* p = i.Next();
        if(p->GetActor() && (p->GetActor()->GetGuild() == guild))
        {
            psUpdatePlayerGuildMessage update(p->GetClientNum(),
                                              p->GetActor()->GetEID(),
                                              "");

            update.Multicast(p->GetActor()->GetMulticastClients(),0,0);

            psserver->SendSystemInfo(p->GetClientNum(),"Your guild has been disbanded.");
            guild->Disconnect(p->GetActor()->GetCharacterData());
        }
    }

    psserver->GetCacheManager()->RemoveGuild(guild);
}

void GuildManager::UnsubscribeWholeGuild(psGuildInfo* guild)
{
    size_t subscrNum=0;
    while(subscrNum < notifySubscr.GetSize())
    {
        if(notifySubscr[subscrNum]->guild == guild->GetID())
        {
            Client* member = clients->Find(notifySubscr[subscrNum]->clientnum);
            if(member != NULL)
            {
                psGUIGuildMessage msg(member->GetClientNum(), psGUIGuildMessage::CLOSE_WINDOW, "<x/>");
                msg.SendMessage();
            }

            delete notifySubscr[subscrNum];
            notifySubscr.Delete(notifySubscr[subscrNum]);
        }
        else
            subscrNum++;
    }
}

void GuildManager::ChangeGuildName(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(!(const char*)msg.guildname || msg.guildname=="")
    {
        psserver->SendSystemError(clientnum,"New guild name must be specified.");
        return;
    }

    if(! IsLeader(client))
        if(!CheckClientRights(client, RIGHTS_EDIT_GUILD, "You do not have the rights to edit the name of your guild."))
            return;

    if(msg.guildname.Length()<5)
    {
        psserver->SendSystemError(clientnum,"Guild name must be at least five characters.");
        return;
    }

    if(msg.guildname.Length() > 25)
    {
        psserver->SendSystemError(clientnum,"Guild name must be at under 25 characters.");
        return;
    }

    if(!FilterGuildName(msg.guildname))
    {
        psserver->SendSystemError(clientnum,"Guild name contains invalid characters (A-Z,a-z and space is allowed).");
        return;
    }

    if(psserver->GetCacheManager()->FindGuild(msg.guildname))
    {
        psserver->SendSystemError(clientnum,"A guild already exists with that name");
        return;
    }

    if(psserver->GetCharManager()->IsBanned(msg.guildname))
    {
        psserver->SendSystemError(clientnum, "The name %s is banned.", msg.guildname.GetData());
        return;
    }

    // Check to make sure the guild name hasn't been changed too recently
    unsigned int minutesRemaining = guild->MinutesUntilUserChangeName();
    if(minutesRemaining)
    {
        psserver->SendSystemError(clientnum, "Guild name has already changed in the past %i minutes. %u minutes remaining.",
                                  GUILD_NAME_CHANGE_LIMIT / 60000, minutesRemaining);
        return;
    }

    if(guild->SetName(msg.guildname))
    {
        psserver->SendSystemInfo(clientnum,"Your guild name has been changed to: %s",msg.guildname.GetData());
    }

    csString guildName(guild->GetName());
    psMOTDMessage motd(client->GetClientNum(), "", "", "", guildName);
    psserver->GetEventManager()->Broadcast(motd.msg, NetBase::BC_GUILD, guild->GetID());

    SendNotifications(guild->GetID(), psGUIGuildMessage::GUILD_DATA);
}

void GuildManager::Invite(psGuildCmdMessage &msg,Client* client)
{
    csString playerName;
    int clientnum = client->GetClientNum();

    playerName = NormalizeCharacterName(msg.player);

    psGuildInfo* guild = client->GetCharacterData()->GetGuild();
    if(!guild)
    {
        psserver->SendSystemError(clientnum,"You have to be in a guild to invite someone.");
        return;
    }

    if(! IsLeader(client))
        if(! CheckClientRights(client, RIGHTS_INVITE, "You do not have the rights to invite players into your guild."))
            return;

    // invited player must be online when invited
    Client* invitee = clients->Find(playerName);
    if(!invitee)
    {
        psserver->SendSystemError(clientnum,"%s is not online right now.",
                                  (const char*) msg.player);
        return;
    }

    if(invitee == client)
    {
        psserver->SendSystemError(clientnum,"You can't invite yourself.", invitee->GetName());
        return;
    }

    if(invitee->IsSuperClient())
    {
        psserver->SendSystemError(clientnum,"You cannot invite npc %s.",
                                  (const char*) playerName);
        return;
    }

    bool inviteeIsAlreadyInSecretGuild = false;

    psGuildInfo* inviteeGuild = invitee->GetCharacterData()->GetGuild();
    if(inviteeGuild != NULL)
    {
        if(!inviteeGuild->IsSecret())
        {
            psserver->SendSystemError(clientnum, "That player is already a member of a guild.");
            return;
        }
        else
            inviteeIsAlreadyInSecretGuild = true;
    }

    PendingInvite* pnew;
    csString invitation;

    invitation.Format("You have been invited to join the %s.  Click Yes to join this guild.",client->GetActor()->GetGuild()->GetName().GetData());
    if(inviteeIsAlreadyInSecretGuild)
        pnew = new PendingSecretGuildMemberInvite(client,invitee,invitation,guild->GetID());
    else
        pnew = new PendingGuildInvite(client,invitee,invitation,guild->GetID());

    // Track who is invited where, to verify confirmations
    psserver->questionmanager->SendQuestion(pnew);
}

void PendingGuildInvite::HandleAnswer(const csString &answer)
{
    Client* client = psserver->GetConnections()->Find(clientnum);
    if(!client  ||  client->GetCharacterData()->GetGuild() != NULL)
        return;

    PendingInvite::HandleAnswer(answer);
    if(answer == "yes")
        psserver->guildmanager->HandleJoinGuild(this);
}


void GuildManager::HandleJoinGuild(PendingGuildInvite* invite)
{
    Client* inviteeClient = clients->Find(invite->clientnum);
    if(inviteeClient == NULL)
        return;

    Client* inviterClient = clients->Find(invite->inviterClientNum);
    EID inviterEID;
    if(inviterClient != NULL)
        inviterEID = inviterClient->GetActor()->GetEID();

    psGuildInfo* guild = psserver->GetCacheManager()->FindGuild(invite->guildID);
    if(guild == NULL)
        return;

    if(!guild->AddNewMember(inviteeClient->GetActor()->GetCharacterData()))
    {
        psserver->SendSystemError(invite->inviterClientNum, "Error joining guild! (%s)", db->GetLastError());
        Error2("Error joining guild! (%s)",db->GetLastError());
        return;
    }

    csString text;
    text.Format("Player %s has joined the guild!",invite->inviteeName.GetData());
    psChatMessage guildmsg(invite->inviterClientNum,0,"System",0,text,CHAT_GUILD, false);
    chatserver->SendGuild(invite->inviterName.GetData(), inviterEID, guild, guildmsg);

    SendNotifications(guild->GetID(), psGUIGuildMessage::MEMBER_DATA);

    if(!guild->IsSecret()) // update guild labels
    {
        psUpdatePlayerGuildMessage update(
            inviteeClient->GetClientNum(),
            inviteeClient->GetActor()->GetEID(),
            guild->GetName());
        psserver->GetEventManager()->Broadcast(update.msg,NetBase::BC_EVERYONE);
    }

    // forward new motd
    csString tip("");
    csString motdMsg(psserver->GetMOTD());
    csString guildMOTD(guild->GetMOTD());
    csString guildName(guild->GetName());
    psMOTDMessage motd(inviteeClient->GetClientNum(),tip,motdMsg,guildMOTD,guildName);
    motd.SendMessage();
}

void GuildManager::Remove(psGuildCmdMessage &msg,Client* client)
{
    Client* targetClient;
    csString name;
    int clientnum = client->GetClientNum();
    bool isRemoved = false; //holds if the player is being removed by someone else, if false he/she's leaving the guild by himself

    psGuildInfo* gi = client->GetActor()->GetGuild();
    if(!gi)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(msg.player.Length())
        name = NormalizeCharacterName(msg.player);
    else
        name = client->GetName();

    // Find the named player in guild
    psGuildMember* target = gi->FindMember(name);
    if(!target)
    {
        psserver->SendSystemError(clientnum,"That player is not member of your guild.");
        return;
    }

    //check if someone is removing the player or the player is leaving by him/herself
    if(client->GetPID() != target->char_id)
        isRemoved = true;

    // Verify rights if the player is removing someone else from the guild
    if(isRemoved)
    {
        if(! IsLeader(client))
        {
            if(!CheckClientRights(client, RIGHTS_REMOVE, "You do not have rights to remove someone from your guild."))
                return;
            if(GetClientLevel(client) <= target->guildlevel->level)
            {
                psserver->SendSystemError(clientnum,"You can't remove players of same or higher level.");
                return;
            }
        }
        targetClient = clients->Find(target->name);
    }
    else
        targetClient = client;

    if(target->guildlevel->level == MAX_GUILD_LEVEL)
    {
        psserver->SendSystemError(clientnum,"The leader cannot simply leave his guild. He must appoint a new leader first.");
        return;
    }

    gi->RemoveMember(target);

    if(targetClient != NULL)
    {
        if(isRemoved)
            psserver->SendSystemInfo(targetClient->GetClientNum(),"You have been removed from your guild.");
        else
            psserver->SendSystemInfo(targetClient->GetClientNum(),"You have left your guild.");
        UnsubscribeGuildData(targetClient);

        // forward blank MOTD to remove old
        csString blank("");
        csString motdMsg(psserver->GetMOTD());
        psMOTDMessage motd(targetClient->GetClientNum(),blank,motdMsg,blank,blank);
        motd.SendMessage();
    }

    csString text;

    if(isRemoved)  //check if the player left the guild or was removed from it
        text.Format("Player %s has been removed from the guild.", (const char*)msg.player);
    else
        text.Format("Player %s has left the guild.", (const char*)msg.player);

    psChatMessage guildmsg(0,0,"System",0,text,CHAT_GUILD, false);

    if(guildmsg.valid)
        chatserver->SendGuild(client->GetCharacterData()->GetCharName(), client->GetActor()->GetEID(), gi, guildmsg);

    SendNotifications(gi->GetID(), psGUIGuildMessage::MEMBER_DATA);

    // Now kick off guild minimum requirement timer
    CheckMinimumRequirements(gi,NULL);
}

/**
 * Called when player issues /guildlevel command
 *
 * Syntax:
 * /guildlevel <level> <levelname>
 * or
 * /guildlevel  (this will show all current level names)
 */
void GuildManager::Rename(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* gi = client->GetActor()->GetGuild();
    if(!gi)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    // checks if levelnumber has been specified
    if(msg.level <= 0 || msg.level > 9)
    {
        psserver->SendSystemInfo(clientnum,"Your guild has these titles defined:");
        csArray<psGuildLevel*>::Iterator lIter = gi->GetLevelIterator();
        while(lIter.HasNext())
        {
            psGuildLevel* level = lIter.Next();
            psserver->SendSystemInfo(clientnum,"%i: %s", level->level, level->title.GetData());
        }
        return;
    }

    if(! IsLeader(client))
        if(! CheckClientRights(client, RIGHTS_EDIT_LEVEL, "You do not have the rights to rename levels of your guild."))
            return;

    // checks if levelname has been specified
    if(!(const char*)msg.levelname)
    {
        psserver->SendSystemError(clientnum,"You must specify the name after the level to rename a guild level.");
        return;
    }

    if(!FilterGuildName(msg.levelname))
    {
        psserver->SendSystemError(clientnum,"Level name contains invaild characters (A-Z,a-z and space is allowed).");
        return;
    }

    if(msg.levelname.Length()<3)
    {
        psserver->SendSystemError(clientnum,"Guild level name must be at least three characters.");
        return;
    }

    if(msg.levelname.Length() > 25)
    {
        psserver->SendSystemError(clientnum,"Guild level name must be at under 25 characters.");
        return;
    }

    if(!gi->RenameLevel(msg.level,msg.levelname))
    {
        psserver->SendSystemError(clientnum,"SQL Error: %s",db->GetLastError());
        return;
    }

    psserver->SendSystemInfo(clientnum,"Guild level rename was successful.");

    csString text;
    text.Format("Guild level %d is now called: %s", msg.level, (const char*)msg.levelname);
    psChatMessage guildmsg(clientnum,0,"System",0,text,CHAT_GUILD, false);
    if(guildmsg.valid)
        chatserver->SendGuild(client->GetCharacterData()->GetCharName(), client->GetActor()->GetEID(), gi, guildmsg);

    SendNotifications(gi->GetID(), psGUIGuildMessage::LEVEL_DATA);
}

void GuildManager::Promote(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(!(const char*)msg.player)
    {
        psserver->SendSystemError(clientnum,"You must specify the player to promote someone.");
        return;
    }

    if(msg.level <= 0 || msg.level > 9)
    {
        psserver->SendSystemError(clientnum,"You must specify a level between 1 and 9 to promote someone.");
        return;
    }

    psGuildMember* target = guild->FindMember(msg.player);
    if(!target)
    {
        psserver->SendSystemError(clientnum,"Player %s is not a member of your guild.",(const char*)msg.player);
        return;
    }

    if(target->guildlevel->level == MAX_GUILD_LEVEL)
    {
        psserver->SendSystemError(clientnum,"You can't just put leader to lower level. You must promote someone else to his place.");
        return;
    }

    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, RIGHTS_PROMOTE, "You do not have the rights in your guild to promote someone."))
            return;
        if(GetClientLevel(client) <= target->guildlevel->level)
        {
            psserver->SendSystemError(clientnum,"You can't promote players of higher or equal level.",(const char*)msg.player);
            return;
        }
        if(GetClientLevel(client) <= msg.level)
        {
            psserver->SendSystemError(clientnum,"You can't promote players to higher or equal level.",(const char*)msg.player);
            return;
        }
    }

    if(!guild->UpdateMemberLevel(target,msg.level))
    {
        psserver->SendSystemError(clientnum,"SQL Error: %s",db->GetLastError());
        return;
    }

    // If we just promoted someone to highest level, we must also demote the current leader
    // (i.e. the player who did this action).
    if(msg.level == MAX_GUILD_LEVEL)
    {
        psGuildMember* clientMembership = guild->FindMember(client->GetPID());
        if(!clientMembership)
        {
            psserver->SendSystemError(clientnum,"Internal error: membership of player not found.");
            return;
        }
        if(!guild->UpdateMemberLevel(clientMembership, MAX_GUILD_LEVEL-1))
        {
            psserver->SendSystemError(clientnum,"SQL Error: %s",db->GetLastError());
            return;
        }
    }

    psserver->SendSystemInfo(clientnum,"Promotion successful.");

    csString text;
    text.Format("%s has been promoted to '%s'", (const char*)msg.player, target->guildlevel->title.GetData());
    psChatMessage guildmsg(clientnum,0,"System",0,text,CHAT_GUILD, false);
    if(guildmsg.valid)
        chatserver->SendGuild(client->GetCharacterData()->GetCharName(), client->GetActor()->GetEID(), guild, guildmsg);

    SendNotifications(guild->GetID(), psGUIGuildMessage::MEMBER_DATA);
}

void GuildManager::GetMemberPermissions(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(!(const char*)msg.player)
    {
        psserver->SendSystemError(clientnum,"You must specify the player to get the permissions.");
        return;
    }

    psGuildMember* target = guild->FindMember(msg.player);
    if(!target)
    {
        psserver->SendSystemError(clientnum,"Player %s is not a member of your guild.",(const char*)msg.player);
        return;
    }


    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, RIGHTS_EDIT_LEVEL, "You do not have the rights in your guild to get the permissions."))
            return;
        if(GetClientLevel(client) <= target->guildlevel->level)
        {
            psserver->SendSystemError(clientnum,"You can't get the permissions of a player of higher or equal level.",(const char*)msg.player);
            return;
        }
    }

    csString permissionText = "Player " + msg.player + " has these permissions: ";
    if(target->HasRights(RIGHTS_VIEW_CHAT))
        permissionText.Append("view_chat ");
    if(target->HasRights(RIGHTS_CHAT))
        permissionText.Append("chat ");
    if(target->HasRights(RIGHTS_INVITE))
        permissionText.Append("invite ");
    if(target->HasRights(RIGHTS_REMOVE))
        permissionText.Append("remove ");
    if(target->HasRights(RIGHTS_PROMOTE))
        permissionText.Append("promote ");
    if(target->HasRights(RIGHTS_EDIT_LEVEL))
        permissionText.Append("edit_level ");
    if(target->HasRights(RIGHTS_EDIT_POINTS))
        permissionText.Append("edit_points ");
    if(target->HasRights(RIGHTS_EDIT_GUILD))
        permissionText.Append("edit_guild ");
    if(target->HasRights(RIGHTS_EDIT_PUBLIC))
        permissionText.Append("edit_public ");
    if(target->HasRights(RIGHTS_EDIT_PRIVATE))
        permissionText.Append("edit_private ");
    if(target->HasRights(RIGHTS_VIEW_CHAT_ALLIANCE))
        permissionText.Append("alliance_view_chat ");
    if(target->HasRights(RIGHTS_CHAT_ALLIANCE))
        permissionText.Append("alliance_chat ");
    if(target->HasRights(RIGHTS_USE_BANK))
        permissionText.Append("guild_bank ");

    psserver->SendSystemInfo(clientnum,permissionText.Trim().GetDataSafe());
}

void GuildManager::SetMemberPermissions(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(!(const char*)msg.player)
    {
        psserver->SendSystemError(clientnum,"You must specify the player to set the permissions.");
        return;
    }

    psGuildMember* target = guild->FindMember(msg.player);
    if(!target)
    {
        psserver->SendSystemError(clientnum,"Player %s is not a member of your guild.",(const char*)msg.player);
        return;
    }

    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, RIGHTS_EDIT_LEVEL, "You do not have the rights in your guild to get the permissions."))
            return;
        if(GetClientLevel(client) <= target->guildlevel->level)
        {
            psserver->SendSystemError(clientnum,"You can't get the permissions of a player of higher or equal level.",(const char*)msg.player);
            return;
        }
    }

    GUILD_PRIVILEGE right;
    if(!msg.permission.Length() || !ParseRightString(msg.permission, right))
    {
        psserver->SendSystemError(clientnum,"You need to specify a valid permission name.");
        return;
    }

    if(! IsLeader(client))
    {
        if(! CheckClientRights(client, right, "You cannot change a privilege that you do not have yourself."))
        {
            return;
        }
    }

    if(msg.subCmd == "add")
    {
        if(guild->SetMemberPrivilege(target, right, true))
            psserver->SendSystemInfo(clientnum,"The permission %s has been added to %s",msg.permission.GetDataSafe(), msg.player.GetDataSafe());
        else
            psserver->SendSystemInfo(clientnum,"Unable to set the permission %s on %s",msg.permission.GetDataSafe(), msg.player.GetDataSafe());
    }
    else if(msg.subCmd == "remove")
    {
        if(guild->SetMemberPrivilege(target, right, false))
            psserver->SendSystemInfo(clientnum,"The permission %s has been removed %s",msg.permission.GetDataSafe(), msg.player.GetDataSafe());
        else
            psserver->SendSystemInfo(clientnum,"Unable to set the permission %s on %s",msg.permission.GetDataSafe(), msg.player.GetDataSafe());
    }
    else
    {
        psserver->SendSystemError(clientnum,"You need to specify if the permission must be added (add) or removed (remove).");
        return;
    }
}

/** Sends list of guild members of given level as text messages to client
  */
void ListMembersOfLevel(psGuildInfo* guild, int clientnum, int level)
{
    csArray<psGuildMember*>::Iterator mIter = guild->GetMemberIterator();
    while(mIter.HasNext())
    {
        psGuildMember* member = mIter.Next();
        if(member->guildlevel->level == level)
            psserver->SendSystemInfo(clientnum,"%s: %s",
                                     member->name.GetData(),
                                     member->guildlevel->title.GetData());
    }
}

void GuildManager::ListMembers(psGuildCmdMessage &msg,Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(guild == NULL)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(!msg.level)
        for(int i=9; i>0; i--)
            ListMembersOfLevel(guild, clientnum, i);
    else
        ListMembersOfLevel(guild, clientnum, msg.level);
}

void GuildManager::Secret(psGuildCmdMessage &msg, Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psserver->SendSystemError(clientnum, "You are not in a guild.");
        return;
    }

    // If the player doesn't provide an argument, just report the current
    // status of the secrecy flag.
    if(msg.secret.IsEmpty())
    {
        psserver->SendSystemInfo(clientnum,
                                 "Your guild's secrecy setting is: %s",
                                 (client->GetActor()->GetGuild()->IsSecret() ? "ON" : "OFF"));
        return;
    }

    if(! IsLeader(client))
        if(! CheckClientRights(client, RIGHTS_EDIT_GUILD, "You do not have rights to change secrecy of your guild."))
            return;

    // Change the argument to lowercase to get a case-insensitive match.
    msg.secret.Downcase();

    // Check for invalid arguments.
    if(msg.secret != "on" && msg.secret != "off")
    {
        psserver->SendSystemError(clientnum,
                                  "Unrecognized /guildsecret argument \"%s\". Choose \"on\" or \"off\".",
                                  msg.secret.GetData());
        return;
    }

    const bool new_secrecy_value = (msg.secret == "on");

    // Only update the database if the value is really being changed.
    if(new_secrecy_value != guild->IsSecret())
    {
        if(!guild->SetSecret(new_secrecy_value))
        {
            psserver->SendSystemError(clientnum, "SQL Error: %s",
                                      db->GetLastError());
            return;
        }
    }

    // Report the new value to the player.
    psserver->SendSystemInfo(clientnum, "Your guild's secrecy setting is now: %s",
                             (new_secrecy_value ? "ON" : "OFF"));

    SendNotifications(guild->GetID(), psGUIGuildMessage::GUILD_DATA);

    // If secrecy is on send a message to all others clearing guild name
    // else send a message to all others to show the guild name.
    if(new_secrecy_value)
    {
        // Turn label off for everybody
        psUpdatePlayerGuildMessage update(
            clientnum,
            client->GetActor()->GetEID(),
            "");
        psserver->GetEventManager()->Broadcast(update.msg,NetBase::BC_EVERYONE);

        // Update guild members with the label
        psUpdatePlayerGuildMessage update2(
            clientnum,
            client->GetActor()->GetEID(),
            guild->GetName());
        psserver->GetEventManager()->Broadcast(update2.msg,NetBase::BC_GUILD,guild->GetID());
    }
    else
    {
        // turn label on for everybody
        psUpdatePlayerGuildMessage update(
            clientnum,
            client->GetActor()->GetEID(),
            guild->GetName());
        psserver->GetEventManager()->Broadcast(update.msg,NetBase::BC_EVERYONE);
    }
}

void GuildManager::Web(psGuildCmdMessage &msg, Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psserver->SendSystemError(clientnum,"You are not in a guild.");
        return;
    }

    if(! IsLeader(client))
        if(! CheckClientRights(client, RIGHTS_EDIT_GUILD, "You do not have rights to change web page of your guild."))
            return;

    if(!msg.web_page.Length())
    {
        psserver->SendSystemError(clientnum,"You did not specify anything as your url.");
        return;
    }

    // Only update the database if the value is really being changed.
    if(msg.web_page != guild->GetWebPage())
    {
        if(!guild->SetWebPage(msg.web_page))
        {
            psserver->SendSystemError(clientnum, "Setting of web page failed");
            return;
        }
    }

    // Report the new value to the player.
    psserver->SendSystemInfo(clientnum, "Your guild's web page url is now: %s", guild->GetWebPage().GetData());

    SendNotifications(guild->GetID(), psGUIGuildMessage::GUILD_DATA);
}

bool GuildManager::IsLeader(Client* client)
{
    return GetClientLevel(client) == MAX_GUILD_LEVEL;
}

void GuildManager::MOTD(psGuildCmdMessage &msg, Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = client->GetActor()->GetGuild();
    if(!guild)
    {
        psSystemMessage newmsg(clientnum,MSG_ERROR,"You are not in a guild.");
        newmsg.SendMessage();
        return;
    }

    if(msg.motd.IsEmpty())
    {
        psserver->SendSystemInfo(clientnum,
                                 "Guild Message Of The Day: %s",
                                 (client->GetActor()->GetGuild()->GetMOTD().GetData()));
        return;
    }

    if(! IsLeader(client))
        if(! CheckClientRights(client, RIGHTS_EDIT_PUBLIC, "You do not have rights to change the message of the day in your guild."))
            return;

    //All fine, let's change it already
    guild->SetMOTD(msg.motd);

    //Send notify to all guild members
    psSystemMessage newmsg(clientnum, MSG_INFO,
                           "Guild message of the day updated");
    psserver->GetEventManager()->Broadcast(newmsg.msg,NetBase::BC_GUILD,guild->GetID());


    SendNotifications(guild->GetID(), psGUIGuildMessage::GUILD_DATA);
}

void GuildManager::GuildWar(psGuildCmdMessage &msg, Client* client)
{
    int clientnum = client->GetClientNum();

    // Check target dead
    gemObject* target = client->GetTargetObject();
    if(!target)
    {
        psserver->SendSystemError(clientnum,"You don't have another player targeted.");
        return;
    }

    if(!target->IsAlive())
    {
        psserver->SendSystemError(clientnum, "You can't challenge a dead person");
        return;
    }

    Client* targetClient = psserver->GetNetManager()->GetClient(target->GetClientID());
    if(!targetClient)
    {
        psserver->SendSystemError(clientnum, "You can challenge other players only");
        return;
    }

    if(targetClient == client)
    {
        psserver->SendSystemError(clientnum,"You can't begin war with yourself.", targetClient->GetName());
        return;
    }

    gemActor* wartarget = targetClient->GetActor();

    // Check for pre-existing war with this person
    psGuildInfo* attackguild = GetClientGuild(client);
    if(attackguild == NULL)
        return;

    psGuildInfo* targetguild = wartarget->GetGuild();
    if(!targetguild)
    {
        psserver->SendSystemError(clientnum, "This player is not in a guild");
        return;
    }

    if(attackguild->FindLeader()->character != client->GetActor()->GetCharacterData())
    {
        psserver->SendSystemError(clientnum, "You are not guild leader");
        return;
    }

    // Check to see if it can find a leader.  If no leader is found then the guild is in a bad
    // state and needs to be fixed by an admin.
    if(targetguild->FindLeader() == NULL)
    {
        psserver->SendSystemError(clientnum, "The guild %s has no leader. Please inform PlaneShift admins", targetguild->GetName().GetData());
        return;
    }

    if(targetguild->FindLeader()->character != wartarget->GetCharacterData())
    {
        psserver->SendSystemError(clientnum, "This player is not guild leader");
        return;
    }

    if(attackguild->IsGuildWarActive(targetguild))
    {
        psserver->SendSystemError(clientnum,"You are already in a guild war with your target.");
        return;
    }

    // Challenge
    csString question;
    question.Format("%s has challenged your guild to a war!  Click on Accept to allow the war or Reject to ignore it.",
                    client->GetName());
    PendingGuildWarInvite* invite = new PendingGuildWarInvite(client,
            targetClient,
            question);
    psserver->questionmanager->SendQuestion(invite);
}

void GuildManager::AcceptWar(PendingGuildWarInvite* invite)
{
    Client* inviteeClient = clients->Find(invite->clientnum);
    Client* inviterClient = clients->Find(invite->inviterClientNum);

    if(!inviteeClient  ||  !inviterClient)
        return;

    psGuildInfo* attackguild = inviterClient->GetActor()->GetGuild();
    psGuildInfo* targetguild = inviteeClient->GetActor()->GetGuild();

    if(!attackguild || !targetguild)
        return;

    attackguild->AddGuildWar(targetguild);
    targetguild->AddGuildWar(attackguild);

    psSystemMessage sys(0,MSG_INFO,"%s and %s have started a guild war!",
                        attackguild->GetName().GetData(),
                        targetguild->GetName().GetData());
    psserver->GetEventManager()->Broadcast(sys.msg,NetBase::BC_GUILD,attackguild->GetID());
    psserver->GetEventManager()->Broadcast(sys.msg,NetBase::BC_GUILD,targetguild->GetID());
}

void PendingGuildWarInvite::HandleAnswer(const csString &answer)
{
    Client* client = psserver->GetConnections()->Find(inviterClientNum);
    if(!client) return;
    psGuildInfo* attackguild = GetClientGuild(client);
    if(!attackguild) return;

    Client* targetClient = psserver->GetConnections()->Find(clientnum);
    if(!targetClient) return;
    psGuildInfo* targetguild = GetClientGuild(targetClient);
    if(!targetguild) return;

    if(attackguild->IsGuildWarActive(targetguild))
        return;

    PendingInvite::HandleAnswer(answer);
    if(answer == "yes")
        psserver->guildmanager->AcceptWar(this);
}

void GuildManager::GuildYield(psGuildCmdMessage &msg, Client* client)
{
    int clientnum = client->GetClientNum();

    int winnerid = client->GetTargetClientID();
    if(!winnerid)
    {
        psserver->SendSystemError(client->GetClientNum(),"You don't have another player targeted.");
        return;
    }

    psGuildInfo* loserguild = client->GetActor()->GetGuild();
    if(!loserguild)
    {
        psserver->SendSystemError(clientnum, "You are not in a guild");
        return;
    }

    if(loserguild->FindLeader()->character != client->GetActor()->GetCharacterData())
    {
        psserver->SendSystemError(clientnum, "You are not guild leader");
        return;
    }

    if(client->GetTargetObject() == NULL)
    {
        psserver->SendSystemError(clientnum, "You must target the leader of the guild.");
        return;
    }

    gemActor* target = client->GetTargetObject()->GetActorPtr();
    if(!target)
        return;

    psGuildInfo* winnerguild = target->GetGuild();
    if(!winnerguild)
    {
        psserver->SendSystemError(clientnum, "Your target is not in a guild");
        return;
    }

    if(!loserguild->IsGuildWarActive(winnerguild))
    {
        psserver->SendSystemError(clientnum,"You are not in a guild war with your target.");
        return;
    }

    // Remove the duel from both sides.
    loserguild->RemoveGuildWar(winnerguild);
    winnerguild->RemoveGuildWar(loserguild);

    // TODO: Stop all fights in progress.
    //client->GetActor()->SetMode(PSCHARACTER_MODE_PEACE);

    // Award guild karma points (33% of lower-ranked guild's points)
    int winnerkarmapoints = winnerguild->GetKarmaPoints();
    int loserkarmapoints = loserguild->GetKarmaPoints();

    float delta = loserkarmapoints;
    if(winnerkarmapoints < delta)
        delta = winnerkarmapoints;
    delta /= 3;

    if(!(int)delta)
        delta=1;

    csString str;
    str.Format("%1.0f karma points from %s awarded to %s.",
               delta,loserguild->GetName().GetData(),winnerguild->GetName().GetData());

    CPrintf(CON_DEBUG, str);

    psSystemMessage sys(0,MSG_INFO,str);
    psserver->GetEventManager()->Broadcast(sys.msg,NetBase::BC_GUILD,loserguild->GetID());
    psserver->GetEventManager()->Broadcast(sys.msg,NetBase::BC_GUILD,winnerguild->GetID());

    // Actually apply karma point changes
    loserguild->SetKarmaPoints(loserkarmapoints - (int)delta);
    winnerguild->SetKarmaPoints(winnerkarmapoints - (int)delta);
}




/******************************************************************************
*
*                Handlers of alliance management network messages
*
******************************************************************************/

class PendingAllianceInvite : public PendingInvite
{
public:
    int allianceID;

    PendingAllianceInvite(Client* inviter,
                          Client* invitee,
                          const char* question,
                          int allianceID)
        : PendingInvite(inviter, invitee, true,
                        question,"Accept","Decline",
                        "You have asked %s to join your alliance.",
                        "%s has invited you to join an alliance.",
                        "%s has joined your alliance.",
                        "You have joined the alliance.",
                        "%s has declined your invitation.",
                        "You have declined %s's invitation.",
                        psQuestionMessage::generalConfirm)
    {
        this->allianceID = allianceID;
    }

    void HandleAnswer(const csString &answer)
    {
        Client* client = psserver->GetConnections()->Find(clientnum);
        if(!client) return;
        psGuildInfo* guild = GetClientGuild(client);
        if(!guild) return;
        if(guild->GetAllianceID() != 0)
            return;

        PendingInvite::HandleAnswer(answer);
        if(answer != "yes")
            return;

        Client* inviteeClient = psserver->GetConnections()->Find(clientnum);
        if(inviteeClient == NULL) return;

        psGuildInfo* inviteeGuild = GetClientGuild(inviteeClient);
        if(inviteeGuild == NULL) return;

        psGuildAlliance* alliance = psserver->GetCacheManager()->FindAlliance(allianceID);
        if(alliance == NULL) return;

        alliance->AddNewMember(inviteeGuild);
        psserver->GetGuildManager()->SendAllianceNotifications(alliance);

        if(client->GetActor())
        {
            csString text;
            text.Format("The guild %s has been added in the alliance!", guild->GetName().GetDataSafe());
            psChatMessage guildmsg(client->GetClientNum(),0,"System",0,text,CHAT_ALLIANCE, false);
            psserver->GetChatManager()->SendAlliance(client->GetName(), client->GetActor()->GetEID(), alliance, guildmsg);
        }

    }
};


/** Does common checks needed for most alliance operations.
  * If one of the checks fails, then the user is sent text description of the problem.
  *
  * Parameter 'checkLeaderGuild' says if we should check that player's guild is leader in the alliance.
  *
  * This function also returns pointers to player's guild and alliance as a secondary product.
  */
bool GuildManager::CheckAllianceOperation(Client* client, bool checkLeaderGuild, psGuildInfo* &guild, psGuildAlliance* &alliance)
{
    int clientnum = client->GetClientNum();

    guild = NULL;
    alliance = NULL;

    guild = GetClientGuild(client);
    if(guild == NULL) return false;

    if(guild->GetAllianceID() == 0)
    {
        psserver->SendSystemError(clientnum,"You guild is not in an alliance.");
        return false;
    }

    if(GetClientLevel(client) != MAX_GUILD_LEVEL)
    {
        psserver->SendSystemError(clientnum,"You are not guild leader.");
        return false;
    }

    alliance = psserver->GetCacheManager()->FindAlliance(guild->GetAllianceID());
    if(alliance == NULL)
    {
        psserver->SendSystemError(clientnum,"Internal error - alliance %d not found.", guild->GetAllianceID());
        return false;
    }

    if(checkLeaderGuild   &&   alliance->GetLeader() != guild)
    {
        psserver->SendSystemError(clientnum,"Your guild is not alliance leader.");
        return false;
    }

    return true;
}


void GuildManager::NewAlliance(psGuildCmdMessage &msg, Client* client)
{
    int clientnum = client->GetClientNum();

    psGuildInfo* guild = GetClientGuild(client);
    if(guild == NULL) return;

    if(GetClientLevel(client) != MAX_GUILD_LEVEL)
    {
        psserver->SendSystemError(clientnum,"You are not guild leader.");
        return;
    }

    if(guild->GetAllianceID() != 0)
    {
        psserver->SendSystemError(clientnum,"Your guild is already member of an alliance.");
        return;
    }

    if(msg.alliancename.Length()==0)
    {
        psserver->SendSystemError(clientnum,"Please specify name of the alliance.");
        return;
    }

    if(psserver->GetCacheManager()->CreateAlliance(msg.alliancename, guild, client))
        psserver->SendSystemInfo(clientnum, "Alliance \"%s\" was created.", msg.alliancename.GetData());
    else
        psserver->SendSystemInfo(clientnum, "Alliance creation failed: %s", psGuildAlliance::lastError.GetData());


    psGuildAlliance* alliance = psserver->GetCacheManager()->FindAlliance(guild->GetAllianceID());
    if(alliance != NULL)
    {
        SendAllianceNotifications(alliance);

        if(client->GetActor())
        {
            csString text;
            text.Format("The alliance %s has been created by %s!", msg.alliancename.GetDataSafe(), guild->GetName().GetDataSafe());
            psChatMessage guildmsg(client->GetClientNum(),0,"System",0,text,CHAT_ALLIANCE, false);
            chatserver->SendAlliance(client->GetName(), client->GetActor()->GetEID(), alliance, guildmsg);
        }
    }
}

void GuildManager::AllianceInvite(psGuildCmdMessage &msg, Client* client)
{
    psGuildInfo* guild;
    psGuildAlliance* alliance;
    csString inviteeName;

    int clientnum = client->GetClientNum();

    if(! CheckAllianceOperation(client, true, guild, alliance))
        return;

    if(msg.player.Length()==0)
    {
        psserver->SendSystemError(clientnum,"Please specify name of guild leader to be invited.");
        return;
    }

    inviteeName = NormalizeCharacterName(msg.player);

    Client* inviteeClient = clients->Find(inviteeName);
    if(inviteeClient == NULL)
    {
        psserver->SendSystemError(clientnum,"\"%s\" is not online right now.", inviteeName.GetData());
        return;
    }

    if(inviteeClient == client)
    {
        psserver->SendSystemError(clientnum,"You can't invite yourself.", inviteeName.GetData());
        return;
    }

    psGuildInfo* inviteeGuild = GetClientGuild(inviteeClient);
    if(inviteeGuild == NULL)
    {
        psserver->SendSystemError(clientnum,"Player \"%s\" is not member of any guild.", inviteeName.GetData());
        return;
    }

    if(alliance->CheckMembership(inviteeGuild))
    {
        psserver->SendSystemError(clientnum,"They are already members of your alliance.");
        return;
    }

    if(inviteeGuild->GetAllianceID() != 0)
    {
        psserver->SendSystemError(clientnum,"%s is already a member of an alliance.", inviteeName.GetData());
        return;
    }

    if(GetClientLevel(inviteeClient) != MAX_GUILD_LEVEL)
    {
        psserver->SendSystemError(clientnum,"Player \"%s\" is not guild leader.", inviteeName.GetData());
        return;
    }


    csString invitation;
    invitation.Format("You have been invited to join the %s alliance.  Click Accept to join this alliance.",alliance->GetName().GetData());
    PendingAllianceInvite* pnew = new PendingAllianceInvite(client, inviteeClient, invitation, alliance->GetID());
    psserver->questionmanager->SendQuestion(pnew);
}

void GuildManager::AllianceRemove(psGuildCmdMessage &msg, Client* client)
{
    psGuildInfo* guild;
    psGuildAlliance* alliance;

    int clientnum = client->GetClientNum();

    if(! CheckAllianceOperation(client, false, guild, alliance))
        return;

    if(msg.guildname.Length()==0)
    {
        psserver->SendSystemError(clientnum,"Please specify name of the guild to be removed from alliance.");
        return;
    }

    psGuildInfo* removedGuild = psserver->GetCacheManager()->FindGuild(msg.guildname);
    if(removedGuild == NULL)
    {
        psserver->SendSystemError(clientnum,"No such guild exists.");
        return;
    }

    RemoveMemberFromAlliance(client, guild, alliance, removedGuild);
}

void GuildManager::AllianceLeave(psGuildCmdMessage &msg, Client* client)
{
    psGuildInfo* guild;
    psGuildAlliance* alliance;

    if(! CheckAllianceOperation(client, false, guild, alliance))
        return;

    RemoveMemberFromAlliance(client, guild, alliance, guild);
}

void GuildManager::RemoveMemberFromAlliance(Client* client, psGuildInfo* guild, psGuildAlliance* alliance,
        psGuildInfo* removedGuild)
{
    int clientnum = client->GetClientNum();

    if(removedGuild == alliance->GetLeader())
    {
        psserver->SendSystemError(clientnum,"Alliance leader cannot leave. Choose another leader first.");
        return;
    }

    // if one guild removes another one (and not just itself), it must be alliance leader
    if(guild != removedGuild)
        if(alliance->GetLeader() != guild)
        {
            psserver->SendSystemError(clientnum,"Your guild must be alliance leader to be able to remove other guilds.");
            return;
        }

    if(alliance->RemoveMember(removedGuild))
        psserver->SendSystemInfo(clientnum,"Guild \"%s\" was removed from alliance.", removedGuild->GetName().GetData());
    else
    {
        psserver->SendSystemInfo(clientnum,"Failed to remove guild from alliance: %s", psGuildAlliance::lastError.GetData());
        return;
    }

    SendNoAllianceNotifications(removedGuild);
    SendAllianceNotifications(alliance);

    if(client->GetActor())
    {
        csString text;
        text.Format("The guild %s is no longer in the alliance!", guild->GetName().GetDataSafe());
        psChatMessage guildmsg(client->GetClientNum(),0,"System",0,text,CHAT_ALLIANCE, false);
        chatserver->SendAlliance(client->GetName(), client->GetActor()->GetEID(), alliance, guildmsg);
    }

}

void GuildManager::AllianceLeader(psGuildCmdMessage &msg, Client* client)
{
    psGuildInfo* guild;
    psGuildAlliance* alliance;

    int clientnum = client->GetClientNum();

    if(! CheckAllianceOperation(client, true, guild, alliance))
        return;

    if(msg.guildname.Length()==0)
    {
        psserver->SendSystemError(clientnum,"Please specify name of the guild to be new alliance leader.");
        return;
    }

    psGuildInfo* newLeader = psserver->GetCacheManager()->FindGuild(msg.guildname);
    if(newLeader == NULL)
    {
        psserver->SendSystemError(clientnum,"No such guild exists.");
        return;
    }

    if(guild == newLeader)
    {
        psserver->SendSystemError(clientnum,"You must specify other guild than your own.");
        return;
    }

    if(! alliance->CheckMembership(newLeader))
    {
        psserver->SendSystemError(clientnum,"This guild is not member of your alliance !");
        return;
    }

    if(alliance->SetLeader(newLeader))
        psserver->SendSystemInfo(clientnum,"Alliance leadership was transfered to %s.", newLeader->GetName().GetData());
    else
    {
        psserver->SendSystemInfo(clientnum,"Failed to transfer alliance leadership: %s.", psGuildAlliance::lastError.GetData());
        return;
    }

    SendAllianceNotifications(alliance);

    if(client->GetActor())
    {
        csString text;
        text.Format("The alliance leader is now %s!", newLeader->GetName().GetDataSafe());
        psChatMessage guildmsg(client->GetClientNum(),0,"System",0,text,CHAT_ALLIANCE, false);
        chatserver->SendAlliance(client->GetName(), client->GetActor()->GetEID(), alliance, guildmsg);
    }
}

void GuildManager::EndAlliance(psGuildCmdMessage &msg, Client* client)
{
    psGuildInfo* guild;
    psGuildAlliance* alliance;

    int clientnum = client->GetClientNum();

    if(! CheckAllianceOperation(client, true, guild, alliance))
        return;


    if(client->GetActor())
    {
        csString text = "The alliance is being disbanded!";
        psChatMessage guildmsg(clientnum,0,"System",0,text,CHAT_ALLIANCE, false);
        chatserver->SendAlliance(client->GetName(), client->GetActor()->GetEID(), alliance, guildmsg);
    }
    EndAlliance(alliance, clientnum);
}

void GuildManager::EndAlliance(psGuildAlliance* alliance, int clientNum)
{
    SendNoAllianceNotifications(alliance);
    if(psserver->GetCacheManager()->RemoveAlliance(alliance))
        psserver->SendSystemInfo(clientNum,"Alliance was disbanded.");
    else
        psserver->SendSystemInfo(clientNum,"Failed to disband alliance: %s.", psGuildAlliance::lastError.GetData());

}

bool GuildManager::FilterGuildName(const char* name)
{
    if(name == NULL)
        return false;

    size_t len = strlen(name);

    if((strspn(name,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ")
            != len))
        return false;

    if((strspn(((const char*)name)+1,
               (const char*)"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ") != len - 1))
        return false;

    return true;
}

void GuildManager::SendGuildPoints(psGuildCmdMessage &msg,Client* client)
{
    psGuildInfo* guild = NULL;

    if(msg.guildname.Length() == 0)
        guild = psserver->GetCacheManager()->FindGuild(client->GetGuildID());
    else
        guild = psserver->GetCacheManager()->FindGuild(msg.guildname);

    if(!guild)
    {
        csString error;
        error = "Couldn't find the guild " + msg.guildname;
        error += "you specified";

        psserver->SendSystemError(client->GetClientNum(),error);
        return;
    }

    csString info;
    info.Format("The guild %s has %d karma points",guild->GetName().GetData(),guild->GetKarmaPoints());

    psserver->SendSystemInfo(client->GetClientNum(),info);
}

void GuildManager::ResendGuildData(int id)
{
    SendNotifications(id, psGUIGuildMessage::GUILD_DATA);
}


