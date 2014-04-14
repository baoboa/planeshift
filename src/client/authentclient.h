/*
 * AuthentClient.h by Keith Fulton <keith@paqrat.com>
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
 *
 * This class subscribes to "AUTH" messages and receives either
 * a "Character ID" message, telling the client what to request
 * to instantiate the player, or a "Not Authorized" message which rejects
 * the client.
 *
 */
#ifndef __AUTHENTICATIONCLIENT_H__
#define __AUTHENTICATIONCLIENT_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"   // Subscriber class

//=============================================================================
// Local Includes
//=============================================================================


//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================
class MsgEntry;
class MsgHandler;

/**
 * \addtogroup client
 * @{ */

///////////////////////////////////////////////////////////////////////////////
// Possible response types for logging in.
///////////////////////////////////////////////////////////////////////////////
#define NO_RESPONSE   0
#define APPROVED      1
#define REJECTED      2


/**
 * Handles Authentication details from the client to the server.
 *
 * The basic role of the authentication client is to handle the 
 * login/logout functions on the client.  When it is created it will 
 * subscribe to these types of messages:
 * <UL>
 *     <LI>MSGTYPE_AUTHAPPROVE
 *     <LI>MSGTYPE_AUTHREJECTED
 *     <LI>MSGTYPE_PREAUTHAPPROVED
 *     <LI>MSGTYPE_DISCONNECT
 *     <LI>MSGTYPE_AUTHCHARACTERAPPROVED
 * </UL>
 *
 * This class will also handle disconnect messages from other 
 * clients/entities. Thus when other players disconnect or move out of 
 * range this class will be responsible for removing them from this client.  
 */
class psAuthenticationClient : public psClientNetSubscriber
{
public:    

    psAuthenticationClient(MsgHandler *myMsgQueue);
    virtual ~psAuthenticationClient();

    /**
     * Send a message to the server to login.
     *
     * @param user The account name
     * @param pwd The account password
     * @param pwd256 The account password
     *
     * @return Always true.
     */
    bool Authenticate (const csString & user, const csString & pwd, const csString & pwd256);

    /**
     * Handle incomming messages based on the subscribed types.
     */
    virtual void HandleMessage(MsgEntry *mh);

    /**
     * Return the reason ( if any ) for a client to be rejected.
     */
    const char* GetRejectMessage();

    /**
     * Get the current status of authentication.
     *
     * Possible return values are:
     * <UL>
     *  <LI>NO_RESPONSE
     *  <LI>APPROVED   
     *  <LI>REJECTED   
     * </UL>
     */
    int Status(void)
    {
        return iClientApproved;
    };

    /**
     * Clear the status of the login attempt.
     */
    void Reset(void)
    {
        iClientApproved = NO_RESPONSE;
    };

protected:
    csRef<MsgHandler> msghandler;
    csString          rejectmsg;
    int               iClientApproved;
    csString          password;
    csString          password256;
    csString          username;    
    
    
private:
    void ShowError();
    
    /** Handles incoming message about pre-auth. */
    void HandlePreAuth( MsgEntry* me );
    
    /** Handle incoming Auth message. */
    void HandleAuthApproved( MsgEntry* me );

    /** Handle an incoming disconnect message. */
    void HandleDisconnect( MsgEntry* me );    
};

/** @} */

#endif

