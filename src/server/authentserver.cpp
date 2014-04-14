/*
 * AuthentServer.cpp by Keith Fulton <keith@paqrat.com>
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
#include <iutil/cfgmgr.h>
#include <csutil/md5.h>
#include <csutil/sha256.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/pscharacterlist.h"
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/pscharacter.h"

#include "net/messages.h"
#include "net/msghandler.h"

#include "util/psdatabase.h"
#include "util/log.h"
#include "util/eventmanager.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "authentserver.h"
#include "adminmanager.h"
#include "weathermanager.h"
#include "client.h"
#include "playergroup.h"
#include "gem.h"
#include "clients.h"
#include "psserver.h"
#include "events.h"
#include "cachemanager.h"
#include "globals.h"
#include "icachedobject.h"
#include "advicemanager.h"
#include "commandmanager.h"

class CachedAuthMessage : public iCachedObject
{
public:
    psAuthApprovedMessage* msg;

    CachedAuthMessage(psAuthApprovedMessage* message)
    {
        msg = message;
    }

    ~CachedAuthMessage()
    {
        delete msg;
    }

    // iCachedObject Functions below

    /// required for iCachedObject but not used here
    virtual void ProcessCacheTimeout()
    {
    }

    /// Turn iCachedObject ptr into psAccountInfo
    virtual void* RecoverObject()
    {
        return this;
    }

    /// Delete must come from inside object to handle operator::delete overrides.
    virtual void DeleteSelf()
    {
        delete this;
    }
};


AuthenticationServer::AuthenticationServer(ClientConnectionSet* pCCS,
        UserManager* usermgr,
        GuildManager* gm)
{
    clients      = pCCS;
    usermanager  = usermgr;
    guildmanager = gm;

    Subscribe(&AuthenticationServer::HandlePreAuthent, MSGTYPE_PREAUTHENTICATE, REQUIRE_ANY_CLIENT);
    Subscribe(&AuthenticationServer::HandleAuthent, MSGTYPE_AUTHENTICATE, REQUIRE_ANY_CLIENT);
    Subscribe(&AuthenticationServer::HandleStringsRequest, MSGTYPE_MSGSTRINGS, REQUIRE_ANY_CLIENT);
    Subscribe(&AuthenticationServer::HandleDisconnect, MSGTYPE_DISCONNECT, REQUIRE_ANY_CLIENT);
    Subscribe(&AuthenticationServer::HandleAuthCharacter, MSGTYPE_AUTHCHARACTER, REQUIRE_ANY_CLIENT);
    Subscribe(&AuthenticationServer::HandleStatusUpdate, MSGTYPE_CLIENTSTATUS, REQUIRE_ANY_CLIENT | REQUIRE_ACTOR);
}

AuthenticationServer::~AuthenticationServer()
{
    //do nothing
}


void AuthenticationServer::HandleAuthCharacter(MsgEntry* me, Client* client)
{
    psCharacterPickerMessage charpick(me);
    if(!charpick.valid)
    {
        Debug1(LOG_NET,me->clientnum,"Mangled psCharacterPickerMessage received.\n");
        return;
    }

    psCharacterList* charlist = psserver->CharacterLoader.LoadCharacterList(client->GetAccountID());

    if(!charlist)
    {
        Error1("Could not load Character List for account! Rejecting client!\n");
        psserver->RemovePlayer(me->clientnum, "Could not load the list of characters for your account.  Please contact a PS Admin for help.");
        return;
    }

    int i;
    for(i=0; i<MAX_CHARACTERS_IN_LIST; i++)
    {
        if(charlist->GetEntryValid(i))
        {
            // Trim out whitespaces from name
            charpick.characterName.Trim();
            csString listName(charlist->GetCharacterFullName(i));
            listName.Trim();

            if(charpick.characterName == listName)
            {
                client->SetPID(charlist->GetCharacterID(i));
                // Set client name in code to just firstname as other code depends on it
                client->SetName(charlist->GetCharacterName(i));
                psCharacterApprovedMessage out(me->clientnum);
                out.SendMessage();
                break;
            }
        }
    }

    // cache will auto-delete this ptr if it times out
    psserver->GetCacheManager()->AddToCache(charlist, psserver->GetCacheManager()->MakeCacheName("list", client->GetAccountID().Unbox()),120);
}


bool AuthenticationServer::CheckAuthenticationPreCondition(int clientnum, bool netversionok, const char* sUser)
{
    /**
     * CHECK 1: Is Network protokol compatible?
     */
    if(!netversionok)
    {
        psserver->RemovePlayer(clientnum, "You are not running the correct version of PlaneShift. Please launch the updater.");
        return false;
    }

    /**
     * CHECK 2: Server has loaded a map
     */

    // check if server is already ready (ie level loaded)
    if(!psserver->IsReady())
    {
        if(psserver->HasBeenReady())
        {
            // Locked
            psserver->RemovePlayer(clientnum,"The server is up but about to shutdown. Please try again in 2 minutes.");

            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: Server does not accept connections anymore.", sUser);
        }
        else
        {
            // Not ready yet
            psserver->RemovePlayer(clientnum,"The server is up but not fully ready to go yet. Please try again in a few minutes.");

            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: Server has not loaded a world map.\n", sUser);
        }

        return false;
    }

    return true;
}


void AuthenticationServer::HandlePreAuthent(MsgEntry* me, Client* notused)
{
    psPreAuthenticationMessage msg(me);
    if(!msg.valid)
    {
        Debug1(LOG_NET,me->clientnum,"Mangled psPreAuthenticationMessage received.\n");
        return;
    }

    if(!CheckAuthenticationPreCondition(me->clientnum,msg.NetVersionOk(),"pre"))
        return;

    psPreAuthApprovedMessage reply(me->clientnum);
    reply.SendMessage();
}

void AuthenticationServer::HandleAuthent(MsgEntry* me, Client* notused)
{
    csTicks start = csGetTicks();

    psAuthenticationMessage msg(me); // This cracks message into members.
    if(!msg.valid)
    {
        Debug1(LOG_NET,me->clientnum,"Mangled psAuthenticationMessage received.\n");
        return;
    }

    if(!CheckAuthenticationPreCondition(me->clientnum, msg.NetVersionOk(),msg.sUser))
        return;

    csString status;
    status.Format("%s, %u, Received Authentication Message", (const char*) msg.sUser, me->clientnum);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

    if(msg.sUser.Length() == 0 || msg.sPassword.Length() == 0)
    {
        psserver->RemovePlayer(me->clientnum,"No username or password entered");

        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: No username or password.\n",
                (const char*)msg.sUser);
        return;
    }

    // Check if login was correct
    Notify2(LOG_CONNECTIONS,"Check Login for: '%s'\n", (const char*)msg.sUser);
    psAccountInfo* acctinfo=psserver->GetCacheManager()->GetAccountInfoByUsername((const char*)msg.sUser);

    if(!acctinfo)
    {
        // invalid
        psserver->RemovePlayer(me->clientnum,"Incorrect password or username.");

        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected: No account found with that name.\n",
                (const char*)msg.sUser);
        return;
    }

    // Add account to cache to optimize repeated login attempts
    psserver->GetCacheManager()->AddToCache(acctinfo,msg.sUser,120);

    // Check if password was correct
    csString passwordhashandclientnum(acctinfo->password256);
    passwordhashandclientnum.Append(":");
    passwordhashandclientnum.Append(me->clientnum);

    csString encoded_hash = CS::Utility::Checksum::SHA256::Encode(passwordhashandclientnum).HexString();
    if(encoded_hash != msg.sPassword) // authentication error
    {
        //sha256 autentication failed so we will try with the previous hash support (md5sum)
        //this is just transition code and should be removed with 0.6.0
        passwordhashandclientnum = acctinfo->password;
        passwordhashandclientnum.Append(":");
        passwordhashandclientnum.Append(me->clientnum);

        encoded_hash = csMD5::Encode(passwordhashandclientnum).HexString();
        if(strcmp(encoded_hash.GetData(), msg.sPassword.GetData())) // authentication error
        {
            psserver->RemovePlayer(me->clientnum, "Incorrect password or username.");
            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Bad password).",(const char*)msg.sUser);
            // No delete necessary because AddToCache will auto-delete
            // delete acctinfo;
            return;
        }
        if(msg.sPassword256.Length() > 0) // save the newly  obtained sha256 password
        {
            csString sanitized;
            db->Escape(sanitized, msg.sPassword256);
            db->CommandPump("UPDATE accounts set password256=\"%s\" where id=%d", sanitized.GetData(), acctinfo->accountid);
        }
    }

    /**
     * Check if the client is already logged in
     */
    Client* existingClient = clients->FindAccount(acctinfo->accountid, me->clientnum);
    if(existingClient)   // account already logged in
    {
        // invalid authent message from a different client
        csString reason;
        if(existingClient->IsZombie())
        {
            reason.Format("Your character(%s) was still in combat or casting a spell when you disconnected. "
                          "This connection is being overridden by a new login.", existingClient->GetName());
        }
        else
        {
            reason.Format("You are already logged on to this server as %s. "
                          "This connection is being overridden by a new login..", existingClient->GetName());
        }

        psserver->RemovePlayer(existingClient->GetClientNum(), reason);
        Notify2(LOG_CONNECTIONS,"User '%s' authentication request overrides an existing logged in user.\n",
                (const char*)msg.sUser);

        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
    }


    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time authenticating account ID %u, After password check",
                      csGetTicks() - start, acctinfo->accountid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }


    Client* client = clients->FindAny(me->clientnum);
    if(!client)
    {
        Bug2("Couldn't find client %d?!?",me->clientnum);
        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
        return;
    }

    client->SetName(msg.sUser);
    client->SetAccountID(acctinfo->accountid);


    // Check to see if the client is banned
    time_t now = time(0);
    BanEntry* ban = banmanager.GetBanByAccount(acctinfo->accountid);
    if(ban == NULL)
    {
        // Account not banned; try IP range
        ban = banmanager.GetBanByIPRange(client->GetIPRange());
    }
    if(ban)
    {
        if(now > ban->end)   // Time served
        {
            banmanager.RemoveBan(acctinfo->accountid);
        }
        else  // Notify and block
        {
            tm* timeinfo = gmtime(&(ban->end));
            csString banmsg;
            banmsg.Format("You are banned until %d-%d-%d %d:%d GMT.  Reason: %s",
                          timeinfo->tm_year+1900,
                          timeinfo->tm_mon+1,
                          timeinfo->tm_mday,
                          timeinfo->tm_hour,
                          timeinfo->tm_min,
                          ban->reason.GetData());

            psserver->RemovePlayer(me->clientnum, banmsg);

            Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Banned).",(const char*)msg.sUser);
            // No delete necessary because AddToCache will auto-delete
            // delete acctinfo;
            return;
        }
    }

    client->SetSecurityLevel(acctinfo->securitylevel);

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time authenticating account ID %u, After ban check",
                      csGetTicks() - start, acctinfo->accountid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    /** Check to see if there are any players on that account.  All accounts should have
    *    at least one player in this function.
    */
    psCharacterList* charlist = psserver->CharacterLoader.LoadCharacterList(acctinfo->accountid);

    if(!charlist)
    {
        Error2("Could not load Character List for account! Rejecting client %s!\n",(const char*)msg.sUser);
        psserver->RemovePlayer(me->clientnum, "Could not load the list of characters for your account.  Please contact a PS Admin for help.");
        delete acctinfo;
        return;
    }

    // cache will auto-delete this ptr if it times out
    psserver->GetCacheManager()->AddToCache(charlist, psserver->GetCacheManager()->MakeCacheName("list", client->GetAccountID().Unbox()),120);


    /**
    * CHECK 6: Connection limit
    *
    * We check against number of concurrent connections, but players with
    * security rank of GameMaster or higher are not subject to this limit.
    */
    if(psserver->IsFull(clients->Count(),client))
    {
        // invalid
        psserver->RemovePlayer(me->clientnum, "The server is full right now.  Please try again in a few minutes.");

        Notify2(LOG_CONNECTIONS, "User '%s' authentication request rejected: Too many connections.\n", (const char*)msg.sUser);
        // No delete necessary because AddToCache will auto-delete
        // delete acctinfo;
        status = "User limit hit!";
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
        return;
    }

    Notify3(LOG_CONNECTIONS,"User '%s' (%d) added to active client list\n",(const char*) msg.sUser, me->clientnum);

    // Get the struct to refresh
    // Update last login ip and time
    csString ipAddr = client->GetIPAddress();
    acctinfo->lastloginip = ipAddr;

    tm* gmtm = gmtime(&now);
    csString timeStr;
    timeStr.Format("%d-%02d-%02d %02d:%02d:%02d",
                   gmtm->tm_year+1900,
                   gmtm->tm_mon+1,
                   gmtm->tm_mday,
                   gmtm->tm_hour,
                   gmtm->tm_min,
                   gmtm->tm_sec);

    acctinfo->lastlogintime = timeStr;
    acctinfo->os = msg.os_;
    acctinfo->gfxcard = msg.gfxcard_;
    acctinfo->gfxversion = msg.gfxversion_;
    psserver->GetCacheManager()->UpdateAccountInfo(acctinfo);

    iCachedObject* obj = psserver->GetCacheManager()->RemoveFromCache(psserver->GetCacheManager()->MakeCacheName("auth",acctinfo->accountid));
    CachedAuthMessage* cam;

    if(!obj)
    {
        // Send approval message
        psAuthApprovedMessage* message = new psAuthApprovedMessage(me->clientnum,client->GetPID(), charlist->GetValidCount());

        if(csGetTicks() - start > 500)
        {
            csString status;
            status.Format("Warning: Spent %u time authenticating account ID %u, After approval",
                          csGetTicks() - start, acctinfo->accountid);
            psserver->GetLogCSV()->Write(CSV_STATUS, status);
        }

        // Send out the character list to the auth'd player
        for(int i=0; i<MAX_CHARACTERS_IN_LIST; i++)
        {
            if(charlist->GetEntryValid(i))
            {
                // Quick load the characters to get enough info to send to the client
                psCharacter* character = psserver->CharacterLoader.QuickLoadCharacterData(charlist->GetCharacterID(i), false);
                if(character == NULL)
                {
                    Error2("QuickLoadCharacterData failed for character '%s'", charlist->GetCharacterName(i));
                    continue;
                }

                Notify3(LOG_CHARACTER, "Sending %s to client %d\n", character->GetCharName(), me->clientnum);
                character->AppendCharacterSelectData(*message);

                delete character;
            }
        }
        message->ConstructMsg();
        cam = new CachedAuthMessage(message);
    }
    else
    {
        // recover underlying object
        cam = (CachedAuthMessage*)obj->RecoverObject();
        // update client id since new connection here
        cam->msg->msg->clientnum = me->clientnum;
    }
    // Send auth approved and char list in one message now
    cam->msg->SendMessage();
    psserver->GetCacheManager()->AddToCache(cam, psserver->GetCacheManager()->MakeCacheName("auth",acctinfo->accountid), 10);

    SendMsgStrings(me->clientnum, true);

    client->SetSpamPoints(acctinfo->spamPoints);
    client->SetAdvisorPoints(acctinfo->advisorPoints);

    if(acctinfo->securitylevel >= GM_TESTER)
    {
        psserver->GetAdminManager()->Admin(me->clientnum, client);
    }

    if(psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "default advisor"))
    {
        psserver->GetAdviceManager()->AddAdvisor(client);
    }

    if(psserver->GetCacheManager()->GetCommandManager()->Validate(client->GetSecurityLevel(), "default buddylisthide"))
    {
        client->SetBuddyListHide(true);
    }

    psserver->GetWeatherManager()->SendClientGameTime(me->clientnum);

    if(csGetTicks() - start > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time authenticating account ID %u, After load",
                      csGetTicks() - start, acctinfo->accountid);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    status.Format("%s - %s, %u, Logged in", ipAddr.GetDataSafe(), (const char*) msg.sUser, me->clientnum);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);
}

void AuthenticationServer::HandleDisconnect(MsgEntry* me,Client* client)
{
    psDisconnectMessage mesg(me);

    // Check if this client is allowed to disconnect or if the
    // zombie state should be set
    if(!client->AllowDisconnect())
        return;

    psserver->RemovePlayer(me->clientnum, "Your client has disconnected. If you are seeing this message a connection error has likely occurred.");
}

void AuthenticationServer::SendDisconnect(Client* client, const char* reason)
{
    if(client->GetActor())
    {
        psDisconnectMessage msg(client->GetClientNum(), client->GetActor()->GetEID(), reason);
        if(msg.valid)
        {
            msg.msg->priority = PRIORITY_LOW;
            psserver->GetEventManager()->Broadcast(msg.msg, NetBase::BC_FINALPACKET);
        }
        else
        {
            Bug2("Could not create a valid psDisconnectMessage for client %u.\n",client->GetClientNum());
        }
    }
    else
    {
        psDisconnectMessage msg(client->GetClientNum(), 0, reason);
        if(msg.valid)
        {
            psserver->GetEventManager()->Broadcast(msg.msg, NetBase::BC_FINALPACKET);
        }
    }
}

void AuthenticationServer::HandleStringsRequest(MsgEntry* me, Client* client)
{
    SendMsgStrings(me->clientnum, false);
}

void AuthenticationServer::SendMsgStrings(int cnum, bool send_digest)
{
    char* data;
    unsigned long size;
    uint32_t num_strings;
    csMD5::Digest digest;

    // Get strings data.
    psserver->GetCacheManager()->GetCompressedMessageStrings(data, size, num_strings, digest);

    // send message strings hash table to client
    if(send_digest)
    {
        psMsgStringsMessage msg(cnum, digest);
        CS_ASSERT(msg.valid);
        msg.SendMessage();
    }
    else
    {
        psMsgStringsMessage msg(cnum, digest, data, size, num_strings);
        CS_ASSERT(msg.valid);
        msg.SendMessage();
    }
}

void AuthenticationServer::HandleStatusUpdate(MsgEntry* me, Client* client)
{
    psClientStatusMessage msg(me);
    // printf("Got ClientStatus message!\n");

    // We ignore !ready messages because of abuse potential.
    if(msg.ready)
    {
        psConnectEvent evt(client->GetClientNum());
        evt.FireEvent();
        client->SetReady(msg.ready);
    }
}

BanManager::BanManager()
{
    Result result(db->Select("SELECT * FROM bans"));
    if(!result.IsValid())
        return;

    time_t now = time(0);
    for(unsigned int i=0; i<result.Count(); i++)
    {
        AccountID account = AccountID(result[i].GetUInt32("account"));
        time_t end = result[i].GetUInt32("end");

        if(now > end)   // Time served
        {
            db->Command("DELETE FROM bans WHERE account='%u'", account.Unbox());
            continue;
        }

        BanEntry* newentry = new BanEntry;
        newentry->account = account;
        newentry->end = end;
        newentry->start = result[i].GetUInt32("start");
        newentry->ipRange = result[i]["ip_range"];
        newentry->reason = result[i]["reason"];
        newentry->banIP = result[i].GetUInt32("ban_ip") != 0;

        bool added = false;

        // If account ban, add to list
        if(newentry->account.IsValid())
        {
            banList_IDHash.Put(newentry->account,newentry);
            added = true;
        }

        // If IP range ban, add to list
        if(newentry->ipRange.Length() && newentry->banIP /*(!end || now < newentry->start + IP_RANGE_BAN_TIME)*/)
        {
            banList_IPRList.Push(newentry);
            added = true;
        }
        if (!added)
        {
            delete newentry;
            Error1("Ban entry not added to any lists.");
        }
    }
}

BanManager::~BanManager()
{
    csHash<BanEntry*, AccountID>::GlobalIterator it(banList_IDHash.GetIterator());
    while(it.HasNext())
    {
        BanEntry* entry = it.Next();
        delete entry;
    }
}

bool BanManager::RemoveBan(AccountID account)
{
    BanEntry* ban = GetBanByAccount(account);
    if(!ban)
        return false;  // Not banned

    db->Command("DELETE FROM bans WHERE account='%u'", account.Unbox());
    banList_IDHash.Delete(account,ban);
    banList_IPRList.Delete(ban);
    delete ban;
    return true;
}

bool BanManager::AddBan(AccountID account, csString ipRange, time_t duration, csString reason, bool banIP)
{
    if(GetBanByAccount(account))
        return false;  // Already banned

    time_t now = time(0);

    BanEntry* newentry = new BanEntry;
    newentry->account = account;
    newentry->start = now;
    newentry->end = now + duration;
    newentry->ipRange = ipRange;
    newentry->reason = reason;
    newentry->banIP = banIP;

    csString escaped;
    db->Escape(escaped, reason.GetData());
    int ret = db->Command("INSERT INTO bans "
                          "(account,ip_range,start,end,reason,ban_ip) "
                          "VALUES ('%u','%s','%u','%u','%s','%u')",
                          newentry->account.Unbox(),
                          newentry->ipRange.GetData(),
                          newentry->start,
                          newentry->end,
                          escaped.GetData(),
                          newentry->banIP);
    if(ret == -1)
    {
        delete newentry;
        return false;
    }

    bool added = false;

    if(newentry->account.IsValid())
    {
        banList_IDHash.Put(newentry->account,newentry);
        added = true;
    }
    

    if(newentry->ipRange.Length() && newentry->banIP)
    {
        banList_IPRList.Push(newentry);
        added = true;
    }
    
    if(!added)
    {
        delete newentry;
        return false;
    }

    return true;
}

BanEntry* BanManager::GetBanByAccount(AccountID account)
{
    return banList_IDHash.Get(account,NULL);
}

BanEntry* BanManager::GetBanByIPRange(csString IPRange)
{
    for(size_t i=0; i<banList_IPRList.GetSize(); i++)
        if(IPRange.StartsWith(banList_IPRList[i]->ipRange))
            return banList_IPRList[i];
    return NULL;
}
