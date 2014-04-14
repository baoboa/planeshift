/*
 * chatmanager.h - Author: Matthias Braun <MatzeBraun@gmx.de>
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
 * This implements a simple chatmanager which resend every chat packet to each
 * client...
 *
 */

#ifndef __CHATMANAGER_H__
#define __CHATMANAGER_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/hashr.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "msgmanager.h"             // parent class

class Client;
class ClientConnectionSet;
class psServer;
class psEndChatLoggingEvent;
class NpcResponse;
class psGuildInfo;
class psGuildAlliance;
class gemNPC;
class gemActor;
class psChatMessage;


#define CHAT_SAY_RANGE 15

struct CachedData
{
    csString key;
    csString alternate;
    csRef<iDataBuffer> data;

    CachedData(iDataBuffer* buffer, const char* n, const char* alt)
    {
        data = buffer;
        key = n;
        alternate = alt;
    }
};

class ChatManager : public MessageManager<ChatManager>
{
public:

    ChatManager();
    virtual ~ChatManager();

    void HandleChatMessage(MsgEntry* me, Client* client);
    void HandleCacheMessage(MsgEntry* me, Client* client);
    void HandleChannelJoinMessage(MsgEntry* me, Client* client);
    void HandleChannelLeaveMessage(MsgEntry* me, Client* client);

    void SendNotice(psChatMessage &msg);
    void SendServerChannelMessage(psChatMessage &msg, uint32_t channelID);

    NpcResponse* CheckNPCEvent(Client* client,csString &trigger,gemNPC* &target);

    void RemoveAllChannels(Client* client);

    void SendGuild(const csString &sender, EID senderEID, psGuildInfo* guild, psChatMessage &msg);
    ///actually sends the message to all connected members of the alliance
    void SendAlliance(const csString &sender, EID senderEID, psGuildAlliance* alliance, psChatMessage &msg);

    /// Starts the process of sending the specified list of files to the client
    void SendMultipleAudioFileHashes(Client* client, const char* voiceFile);

    csString channelsToString();

protected:
    csPDelArray<CachedData> audioFileCache;

    void SendTell(psChatMessage &msg, const char* who, Client* client, Client* target);
    void SendSay(uint32_t clientNum, gemActor* actor, psChatMessage &msg, const char* who);
    void SendGuild(Client* client, psChatMessage &msg);
    ///gets the message from a client to dispatch to the alliance chat
    void SendAlliance(Client* client, psChatMessage &msg);
    void SendGroup(Client* client, psChatMessage &msg);
    void SendShout(Client* client, psChatMessage &msg);

    NpcResponse* CheckNPCResponse(psChatMessage &msg,Client* client,gemNPC* &target);

    /// Starts the process of sending the specified file to the client
    void SendAudioFileHash(Client* client, const char* voiceFile, csTicks delay);
    /// Sends the actual file to the client if needed
    void SendAudioFile(Client* client, const char* voiceFile);

    /// If this returns true, all went well.  If it returns false, the client was muted
    bool FloodControl(csString &newMessage, Client* client);

    // Chat channel state
    // uint32_t here to allow hashing
    // csHashReversible is not used because it does not allow a many to many mapping
    csHash<uint32_t, uint32_t> channelSubscriptions;
    csHash<uint32_t, uint32_t> channelSubscribers;

    // case-insensitive
    csHash<uint32_t, csString> channelIDs;
    // case-sensitive
    csHash<csString, uint32_t> channelNames;

    uint32_t nextChannelID;
};


class psEndChatLoggingEvent : public psGameEvent
{
public:
    uint32_t clientnum;

    psEndChatLoggingEvent(uint32_t clientNum, const int delayTicks);

    virtual void Trigger();

};

#endif

