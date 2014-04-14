/*
 * AuthentServer.h by Keith Fulton <keith@paqrat.com>
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
#ifndef __AUTHENTICATIONSERVER_H__
#define __AUTHENTICATIONSERVER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/hash.h>

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"         // Parent class

class psMsgStringsMessage;
class ClientConnectionSet;
class UserManager;
class GuildManager;
class Client;

struct BanEntry
{
    AccountID account;
    csString  ipRange;
    time_t    start;
    time_t    end;
    csString  reason;
    bool      banIP;
};

/// Holds list of banned accounts and IP ranges
class BanManager
{
public:
    BanManager();
    ~BanManager();

    bool RemoveBan(AccountID account);
    bool AddBan(AccountID account, csString ipRange, time_t duration, csString reason, bool banIP);
    BanEntry* GetBanByAccount(AccountID account);
    BanEntry* GetBanByIPRange(csString IPRange);

protected:
    csHash<BanEntry*, AccountID> banList_IDHash;   /// Full list of all active bans
    csArray<BanEntry*> banList_IPRList;    /// List of active IP range bans
};

/**
 * Authentication handling.
 *
 * This class subscribes to "AUTH" messages and checks userid's and passwords
 * against the database (hardcoded right now, not live db).  If they are valid
 * an "Account Approved" message is sent back, telling the client what to
 * request to instantiate the player.  If not valid, it sends back a
 * "Not Authorized" message.
 *
 */
class AuthenticationServer : public MessageManager<AuthenticationServer>
{
public:

    /**
     * Initializing Constructor.
     *
     * This initializes the AuthenticationServer object. Storing the
     * references to the objects asked for. This also subscribes this object
     * to the message handler listening to messages of type:
     *     - MSGTYPE_AUTHENTICATE
     *     - MSGTYPE_DISCONNECT
     *     .
     * When either of these messages appear on the message queue, the corresponding
     * function is called with the message as the argument.
     *
     * @param pCCS Reference to the client connection set.
     * @param usermgr Reference to the user manager.
     * @param gm Reference to the guild manager.
     */
    AuthenticationServer(ClientConnectionSet* pCCS,
                         UserManager* usermgr,
                         GuildManager* gm);

    /**
     * Destructor.
     *
     * The destructor will un-subscribe this object from the message handler.
     *
     * @see MsgHandler::Unsubscribe()
     */
    virtual ~AuthenticationServer();

    /**
     * Sends a disconnect message to the given client.
     *
     * This will send a disconnect to the given client. Before doing that it
     * makes sure that no other clients have a reference to it before removing
     * it from the world. Sends a message of type NetBase::BC_FINALPACKET to
     * do the disconnect.
     *
     * @param client: The client object for the client we wish to disconnect.
     * @param reason: A sting with the reason we are disconnecting the client.
     * @see psDisconnectMessage
     */
    void SendDisconnect(Client* client,const char* reason);

    /**
     * Util function to send string hash to client, because authentserver and
     * npcmanager both send these.
     */
    void SendMsgStrings(int cnum, bool send_digest);

    /**
     * Updates the status of the client. Currently is used to set client to ready.
     */
    void HandleStatusUpdate(MsgEntry* me, Client* client);

    /**
     * Get the Ban Manager that hold information regarding baned clients.
     */
    BanManager* GetBanManager()
    {
        return &banmanager;
    }


protected:

    /// Holds a list of all the currently connect clients.
    ClientConnectionSet* clients;

    /// Is the user manager.
    UserManager* usermanager;

    /// Is a manager for the guilds.
    GuildManager* guildmanager;

    /// Manages banned users and IP ranges
    BanManager banmanager;

    /**
     * Common preconditions for HandlePreAuthent and HandleAuthent
     */
    bool CheckAuthenticationPreCondition(int clientnum, bool netversionok, const char* sUser);

    /**
     * Handles an authenticate message from the message queue.
     *
     * This method recieves a authenticate message. Uses the following steps to
     * authenticate a client. Sends a psAuthMessageApproved message back to the
     * client if it was successfully authenticated and adds the client to the
     * current client list.
     *
     * @param me      Is a message entry that contains the authenticate message.
     * @param notused Not used.
     * @see psAuthMessageApproved
     */
    void HandleAuthent(MsgEntry* me, Client* notused);

    /**
     * Handles a request for messsage strings from a client.
     */
    void HandleStringsRequest(MsgEntry* me, Client* notused);

    /**
     * This just questsions a random number (clientnum) from server.
     *
     * It is used for authenticating
     */
    void HandlePreAuthent(MsgEntry* me, Client* notused);

    /**
     * Handles a disconnect message from the message queue.
     *
     * This will remove the player from the server using
     * psServer::RemovePlayer()
     *
     * @param me Is the disconnect message that was recieved.
     * @param notused Not used.
     */
    void HandleDisconnect(MsgEntry* me,Client* notused);

    /**
     * Handles a message where a client picks his character to play with.
     *
     * Can remove the player using psServer::RemovePlayer if invalid.
     */
    void HandleAuthCharacter(MsgEntry* me, Client* notused);
};

#endif

