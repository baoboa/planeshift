/*
 * guildmanager.h
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


#ifndef __GUILDMANAGER_H__
#define __GUILDMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/document.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psguildinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class

class ClientConnectionSet;
class ChatManager;
class PendingGuildInvite;
class PendingGuildWarInvite;


/**
 * \addtogroup server
 * @{ */

/**
 * Information about client that asked us to tell him when some guild data change.
 */
class GuildNotifySubscription
{
public:
    GuildNotifySubscription(int guild, int clientnum, bool onlineOnly)
    {
        this->guild       = guild;
        this->clientnum   = clientnum;
        this->onlineOnly  = onlineOnly;
    }
    int guild;            ///< Guild ID
    int clientnum;        ///< Client Id
    bool onlineOnly;      ///< Should we send members that are online only, or all members ?
};


class GuildManager : public MessageManager<GuildManager>
{
    friend class PendingAllianceInvite;
public:

    GuildManager(ClientConnectionSet* pCCS, ChatManager* chat);
    virtual ~GuildManager();

    void HandleJoinGuild(PendingGuildInvite* invite);
    void AcceptWar(PendingGuildWarInvite* invite);

    void ResendGuildData(int id);

    /**
     * After the grace period is up, disband guild if requirements not met.
     */
    void RequirementsDeadline(int guild_id);

    /**
     * Ensure guild has at least the minimum members, and set timer to disband if not.
     */
    void CheckMinimumRequirements(psGuildInfo* guild, gemActor* notify);


protected:
    int  GetClientLevel(Client* client);
    bool CheckAllianceOperation(Client* client, bool checkLeaderGuild, psGuildInfo* &guild, psGuildAlliance* &alliance);

    void HandleCmdMessage(MsgEntry* me,Client* client);
    void HandleGUIMessage(MsgEntry* me,Client* client);
    void HandleMOTDSet(MsgEntry* me,Client* client);
    void HandleSubscribeGuildData(Client* client,iDocumentNode* root);
    void UnsubscribeGuildData(Client* client);
    void HandleSetOnline(Client* client,iDocumentNode* root);
    ///Sets the status of guild notifications when a guild member logins/logsout
    void HandleSetGuildNotifications(Client* client,iDocumentNode* root);
    ///Sets the status of alliance notifications when an alliance member logins/logsout
    void HandleSetAllianceNotifications(Client* client,iDocumentNode* root);
    void HandleSetLevelRight(Client* client,iDocumentNode* root);
    void HandleRemoveMember(Client* client,iDocumentNode* root);
    void HandleSetMemberLevel(Client* client,iDocumentNode* root);
    void HandleSetMemberPoints(Client* client,iDocumentNode* root);

    /**
     * Handles the message from the client asking for a change in max member points.
     *
     * @param client the client asking the operation
     * @param root the document node which starts the data for this command
     */
    void HandleSetMaxMemberPoints(Client* client,iDocumentNode* root);
    void HandleSetMemberNotes(Client* client,iDocumentNode* root, bool isPublic);

    /**
     * Checks if client has right 'priv'.
     */
    bool CheckClientRights(Client* client, GUILD_PRIVILEGE priv);

    /**
     * Checks if client has right 'priv'.
     *
     * If not, it sends him psSystemMessage with text 'denialMsg'
     */
    bool CheckClientRights(Client* client, GUILD_PRIVILEGE priv, const char* denialMsg);

    /**
     * Parses the xml to check if the client wants only a list of online members.
     */
    bool RetrieveOnlineOnly(iDocumentNode* root);

    /**
     * Parses the xml to check if the client wants to be notified of guild member logging in/off.
     */
    bool RetrieveGuildNotifications(iDocumentNode* root);

    /**
     * Parses the xml to check if the client wants to be notified of alliance member logging in/off.
     */
    bool RetrieveAllianceNotifications(iDocumentNode* root);


    void SendGuildData(Client* client);
    void SendLevelData(Client* client);
    void SendMemberData(Client* client, bool onlineOnly);
    void SendAllianceData(Client* client);

    csString MakeAllianceMemberXML(psGuildInfo* member, bool allianceLeader);

    /**
     * Parses a right string in order to be used by the right assignment functions.
     *
     * @param privilege A string with the privilege name
     * @param right Where the result is stored
     * @return true if the right was found
     */
    bool ParseRightString(csString privilege,  GUILD_PRIVILEGE &right);

    void CreateGuild(psGuildCmdMessage &msg,Client* client);

    /**
     * This function actually removes the guild.
     */
    void EndGuild(psGuildInfo* guild,int clientnum);

    /**
     * This handles the command from the player to end the guild, validates and calls the other EndGuild.
     */
    void EndGuild(psGuildCmdMessage &msg,Client* client);

    void ChangeGuildName(psGuildCmdMessage &msg,Client* client);
    bool FilterGuildName(const char* name);
    void Invite(psGuildCmdMessage &msg,Client* client);
    void Remove(psGuildCmdMessage &msg,Client* client);
    void Rename(psGuildCmdMessage &msg,Client* client);
    void Promote(psGuildCmdMessage &msg,Client* client);

    /**
     * Handles the /getmemberpermissions command and returns the permissions
     * of the member to the requesting client.
     *
     * The client can only check
     * permissions of members he can change permissions too.
     *
     * @param msg    the message coming from the client
     * @param client the client sending the request
     */
    void GetMemberPermissions(psGuildCmdMessage &msg,Client* client);

    /**
     * Handles the /setmemberpermissions command and allows to change
     * the permissions of a particular member of a guild (addition/removal
     * from the guild level permissions).
     *
     * @param msg the message coming from the client
     * @param client the client sending the request
     */
    void SetMemberPermissions(psGuildCmdMessage &msg,Client* client);
    void ListMembers(psGuildCmdMessage &msg,Client* client);
    void Secret(psGuildCmdMessage &msg, Client* client);
    void Web(psGuildCmdMessage &msg, Client* client);
    void MOTD(psGuildCmdMessage &msg, Client* client);
    void GuildWar(psGuildCmdMessage &msg, Client* client);
    void GuildYield(psGuildCmdMessage &msg, Client* client);

    void NewAlliance(psGuildCmdMessage &msg, Client* client);
    void AllianceInvite(psGuildCmdMessage &msg, Client* client);
    void AllianceRemove(psGuildCmdMessage &msg, Client* client);
    void AllianceLeave(psGuildCmdMessage &msg, Client* client);
    void AllianceLeader(psGuildCmdMessage &msg, Client* client);
    void EndAlliance(psGuildCmdMessage &msg, Client* client);

    /**
     * This is the function which actually end the alliance.
     *
     * This is needed as the alliance could be removed also for the lack of the
     * prerequisites automatically on guild removal, which itself can
     * happen automatically.
     *
     * @param alliance  A pointer to the alliance being removed.
     * @param clientNum The number of the client who issued the command,
     *                  if any.
     */
    void EndAlliance(psGuildAlliance* alliance, int clientNum);
    void RemoveMemberFromAlliance(Client* client, psGuildInfo* guild, psGuildAlliance* alliance,
                                  psGuildInfo* removedGuild);

    bool AddPlayerToGuild(int guild,const char* guildname,Client* client,int level);
    GuildNotifySubscription* FindNotifySubscr(Client* client);

    /**
     * Sends changed guild data to notification subscribers.
     *
     * Value of 'msg' says which kind of data and it can be:
     *       <ul>
     *         <li>psGUIGuildMessage::GUILD_DATA</li>
     *         <li>psGUIGuildMessage::LEVEL_DATA</li>
     *         <li>psGUIGuildMessage::MEMBER_DATA</li>
     *         <li>psGUIGuildMessage::ALLIANCE_DATA</li>
     *       </ul>
     */
    void SendNotifications(int guild, int msg);

    /**
     * Calls SendNotifications() with type psGUIGuildMessage::ALLIANCE_DATA for all alliance members.
     */
    void SendAllianceNotifications(psGuildAlliance* alliance);

    /**
     * Sends psGUIGuildMessage::ALLIANCE_DATA messages saying "you are not in any alliance"
     * to all notification subscribers from given alliance.
     *
     * This is used when an alliance is being disbanded or when one of its members is removed.
     */
    void SendNoAllianceNotifications(psGuildAlliance* alliance);
    void SendNoAllianceNotifications(psGuildInfo* guild);

    void SendGuildPoints(psGuildCmdMessage &msg,Client* client);

    void UnsubscribeWholeGuild(psGuildInfo* guild);

    bool IsLeader(Client* client);

    ChatManager* chatserver;
    ClientConnectionSet* clients;
    csArray<GuildNotifySubscription*> notifySubscr;

    csRef<iDocumentSystem>  xml;
};

/** @} */

#endif

