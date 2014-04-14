/*
 * chatmanager.cpp
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
#include <csutil/hashr.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>


//=============================================================================
// Project Space Includes
//=============================================================================
#include "util/serverconsole.h"
#include "util/log.h"
#include "util/pserror.h"
#include "util/eventmanager.h"
#include "util/strutil.h"
#include "netmanager.h"
#include "net/msghandler.h"

#include "bulkobjects/psnpcdialog.h"
#include "bulkobjects/dictionary.h"
#include "bulkobjects/psguildinfo.h"
#include "bulkobjects/pssectorinfo.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "cachemanager.h"
#include "chatmanager.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "globals.h"
#include "psserver.h"
#include "npcmanager.h"
#include "adminmanager.h"


ChatManager::ChatManager() : nextChannelID(2)
{
    Subscribe(&ChatManager::HandleChannelJoinMessage, MSGTYPE_CHANNEL_JOIN, REQUIRE_ANY_CLIENT);
    Subscribe(&ChatManager::HandleChannelLeaveMessage, MSGTYPE_CHANNEL_LEAVE, REQUIRE_ANY_CLIENT);
    Subscribe(&ChatManager::HandleChatMessage, MSGTYPE_CHAT, REQUIRE_ACTOR | REQUIRE_ALIVE);
    Subscribe(&ChatManager::HandleCacheMessage, MSGTYPE_CACHEFILE, REQUIRE_READY_CLIENT);

    // Default channel
    channelIDs.PutUnique("gossip", 1);
    channelNames.PutUnique(1, "gossip");
}

ChatManager::~ChatManager()
{
    //do nothing
}


void ChatManager::HandleChatMessage(MsgEntry* me, Client* client)
{
    psChatMessage msg(me);

    // Dont
    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psChatMessage from client %u.\n",me->clientnum);
        return;
    }

    const char* pType = msg.GetTypeText();

    if(msg.iChatType != CHAT_TELL && msg.iChatType != CHAT_AWAY)
    {
        Debug4(LOG_CHAT, client->GetClientNum(),
               "%s %s: %s\n", client->GetName(),
               pType, msg.sText.GetData());
    }
    else
    {
        Debug5(LOG_CHAT,client->GetClientNum(), "%s %s %s: %s\n", client->GetName(),
               pType, msg.sPerson.GetData(),msg.sText.GetData());
    }

    bool saveFlood = true;

    if(!client->IsMute())
    {
        // Send Chat to other players
        switch(msg.iChatType)
        {
            case CHAT_GUILD:
            {
                SendGuild(client, msg);
                break;
            }
            case CHAT_ALLIANCE:
            {
                SendAlliance(client, msg);
                break;
            }
            case CHAT_GROUP:
            {
                SendGroup(client, msg);
                break;
            }
            case CHAT_AUCTION:
            case CHAT_SHOUT:
            {
                SendShout(client, msg);
                break;
            }
            case CHAT_CHANNEL:
            {
                csArray<uint32_t> subscribed = channelSubscriptions.GetAll(client->GetClientNum());
                bool found = false;
                for(size_t i = 0; i < subscribed.GetSize(); i++)
                {
                    if(subscribed[i] == msg.channelID)
                        found = true;
                }
                if(!found)
                {
                    psserver->SendSystemError(client->GetClientNum(), "You have not yet joined this channel.");
                    break;
                }

                // channel 1 is public
                if(msg.channelID == 1)
                    CPrintf(CON_WARNING, "Gossip %s: %s\n", client->GetName(), msg.sText.GetData());

                psChatMessage newMsg(client->GetClientNum(), client->GetActor()->GetEID(), client->GetName(), 0, msg.sText, msg.iChatType, msg.translate, msg.channelID);

                csArray<uint32_t> subscribers = channelSubscribers.GetAll(msg.channelID);
                csArray<PublishDestination> destArray;
                for(size_t i = 0; i < subscribers.GetSize(); i++)
                {
                    destArray.Push(PublishDestination(subscribers[i], NULL, 0, 0));
                    Client* target = psserver->GetConnections()->Find(subscribers[i]);
                    if(target && target->IsReady())
                        target->GetActor()->LogChatMessage(client->GetActor()->GetFirstName(), newMsg);
                }

                newMsg.Multicast(destArray, 0, PROX_LIST_ANY_RANGE);
                break;
            }
            case CHAT_PET_ACTION:
            {
                gemNPC* pet = NULL;

                // Check if a specific pet's name was specified, in one of these forms:
                // - /mypet Petname ...
                // - /mypet Petname's ...
                size_t numPets = client->GetNumPets();
                for(size_t i = 0; i < numPets; i++)
                {
                    if((pet = dynamic_cast <gemNPC*>(client->GetPet(i)))
                            && msg.sText.StartsWith(pet->GetCharacterData()->GetCharName(), true))
                    {
                        size_t n = strlen(pet->GetCharacterData()->GetCharName());
                        if(msg.sText.Length() >= n + 1 && msg.sText.GetAt(n) == ' ')
                        {
                            msg.sText.DeleteAt(0, n);
                            msg.sText.LTrim();
                            break;
                        }
                        else if(msg.sText.Length() >= n + 3 && msg.sText.GetAt(n) == '\''
                                && msg.sText.GetAt(n + 1) == 's' && msg.sText.GetAt(n + 2) == ' ')
                        {
                            msg.sText.DeleteAt(0, n);
                            break;
                        }
                    }
                    else pet = NULL;
                }
                // If no particular pet was specified, assume the default familiar...
                if(!pet)
                {
                    pet = dynamic_cast <gemNPC*>(client->GetFamiliar());
                }

                // Check if there is a mount
                if(!pet)
                {
                    psCharacter* mount = client->GetActor()->GetMount();
                    if(mount)
                    {
                        SendSay(client->GetClientNum(), client->GetActor(), msg, mount->GetCharFullName());
                        break;
                    }
                }

                // Send the message or an appropriate error...
                if(!pet)
                    psserver->SendSystemInfo(me->clientnum, "You have no familiar to command.");
                else
                    SendSay(client->GetClientNum(), pet, msg, pet->GetCharacterData()->GetCharFullName());

                break;
            }
            case CHAT_SAY:
            {
                // Send to all if there's no NPC response or the response is public
                SendSay(client->GetClientNum(), client->GetActor(), msg, client->GetName());
                break;
            }
            case CHAT_NPC:
            {
                // Only the speaker sees his successful chatting with an npc.
                // This helps quests stay secret.
                //psChatMessage newMsg(client->GetClientNum(), client->GetName(), 0,
                //    msg.sText, msg.iChatType, msg.translate);
                //newMsg.SendMessage();
                saveFlood = false;

                gemObject* target = client->GetTargetObject();
                gemNPC* targetnpc = dynamic_cast<gemNPC*>(target);
                if(targetnpc)
                {
                    if(targetnpc->IsBusy())
                    {
                        psserver->SendSystemInfo(me->clientnum, "%s doesn't pay attention to you.",targetnpc->GetName());
                    }
                    else
                    {
                        // The NPC is spoken to so register this client as a speaker
                        targetnpc->RegisterSpeaker(client);

                        NpcResponse* resp = CheckNPCResponse(msg,client,targetnpc);
                        if(resp)
                        {
                            csTicks delay = resp->ExecuteScript(client->GetActor(), targetnpc);
                            if(delay != (csTicks)-1 && resp->menu)
                                resp->menu->ShowMenu(client, delay, targetnpc);
                        }
                    }
                }
                break;
            }
            case CHAT_AWAY:
            {
                saveFlood = false; //do not check Away messages for flooding
                msg.iChatType = CHAT_TELL; //do regard it as tell message from now on
                //intentionally no break, so it falls through to CHAT_TELL
            }
            case CHAT_TELL:
            {
                if(msg.sPerson.Length() == 0)
                {
                    psserver->SendSystemError(client->GetClientNum(), "You must specify name of player.");
                    break;
                }

                Client* target = FindPlayerClient(msg.sPerson);
                if(target && !target->IsSuperClient())
                {
                    if(!target->IsReady())
                        psserver->SendSystemError(client->GetClientNum(), "%s is not ready yet.", msg.sPerson.GetDataSafe());
                    else
                        SendTell(msg, client->GetName(), client, target);
                }
                else
                    psserver->SendSystemError(client->GetClientNum(), "%s is not found online.", msg.sPerson.GetDataSafe());

                break;
            }
            case CHAT_REPORT:
            {
                // First thing to extract the name of the player to log
                csString targetName;
                int index = (int)msg.sText.FindFirst(' ', 0);
                targetName = (index == -1) ? msg.sText : msg.sText.Slice(0, index);
                targetName = NormalizeCharacterName(targetName);

                if(targetName.Length() == 0)
                {
                    psserver->SendSystemError(client->GetClientNum(), "You must specify name of player.");
                    break;
                }

                Client* target = psserver->GetConnections()->Find(targetName);
                if(!target)
                {
                    psserver->SendSystemError(client->GetClientNum(), "%s is not found online.", targetName.GetData());
                    break;
                }
                if(target->IsSuperClient())
                {
                    psserver->SendSystemError(client->GetClientNum(), "Can't report NPCs.");
                    break;
                }

                // Add an active report to the target.
                if(target->GetActor()->AddChatReport(client->GetActor()))
                {
                    // Add report removal event.
                    psserver->GetEventManager()->Push(new psEndChatLoggingEvent(target->GetClientNum(), 300000));
                    psserver->SendSystemInfo(client->GetClientNum(), "Last 5 minutes of %s's chat were logged. Logging will continue for another 5 minutes.", targetName.GetData());
                }
                else
                    psserver->SendSystemError(client->GetClientNum(), "Could not start logging %s, due to a server error.", targetName.GetData());
                break;
            }
            case CHAT_ADVISOR:
            case CHAT_ADVICE:
                break;

            default:
            {
                Error2("Unknown Chat Type: %d\n",msg.iChatType);
                break;
            }
        }
    }
    else
    {
        //User is muted but tries to chat anyway. Remind the user that he/she/it is muted
        psserver->SendSystemInfo(client->GetClientNum(),"You can't send messages because you are muted.");
    }

    if(saveFlood)
        client->FloodControl(msg.iChatType, msg.sText, msg.sPerson);
}

void ChatManager::HandleCacheMessage(MsgEntry* me, Client* client)
{
    psCachedFileMessage msg(me);
    // printf("Got request for file '%s'\n",msg.hash.GetDataSafe());
    SendAudioFile(client,msg.hash);
}

void ChatManager::HandleChannelJoinMessage(MsgEntry* me, Client* client)
{
    psChannelJoinMessage msg(me);
    msg.channel.Trim();
    if(msg.channel.Length() == 0)
        return;
    if(msg.channel.Length() > 30)
        return;

    csString nocaseChannelName = msg.channel;
    nocaseChannelName.Downcase();
    // Search is case-insensitive
    uint32_t channelID = channelIDs.Get(nocaseChannelName, 0);
    if(channelID == 0)
    {
        uint32_t start = channelID = nextChannelID++;
        while(!channelSubscribers.GetAll(channelID).IsEmpty() && nextChannelID != start - 1)
        {
            channelID = nextChannelID++;
            if(nextChannelID == 0)
                nextChannelID++;
        }
        if(nextChannelID == start - 1)
        {
            psserver->SendSystemError(client->GetClientNum(), "Server channel limit reached!");
            return;
        }

        channelIDs.PutUnique(nocaseChannelName, channelID);
        channelNames.PutUnique(channelID, nocaseChannelName);
    }
    else
    {
        csArray<uint32_t> subscribed = channelSubscriptions.GetAll(client->GetClientNum());
        bool found = false;
        for(size_t i = 0; i < subscribed.GetSize(); i++)
        {
            if(subscribed[i] == channelID)
                found = true;
        }
        if(found)
        {
            psserver->SendSystemError(client->GetClientNum(), "You have already joined this channel.");
            return;
        }
    }
    if(channelSubscriptions.GetAll(client->GetClientNum()).GetSize() > 10)
    {
        psserver->SendSystemError(client->GetClientNum(), "You have joined more than 10 channels, please leave some channels first.");
        return;
    }
    csString reply;
    reply.Format("Welcome to the %s channel! %d players in this channel.", channelNames.Get(channelID, "").GetData(), (int)channelSubscribers.GetAll(channelID).GetSize());

    channelSubscriptions.Put(client->GetClientNum(), channelID);
    channelSubscribers.Put(channelID, client->GetClientNum());

    psserver->SendSystemInfo(client->GetClientNum(), reply);
    psChannelJoinedMessage replyMsg(client->GetClientNum(), channelNames.Get(channelID, "").GetData(), channelID);
    replyMsg.SendMessage();
}

void ChatManager::HandleChannelLeaveMessage(MsgEntry* me, Client* client)
{
    psChannelLeaveMessage msg(me);

    channelSubscriptions.Delete(client->GetClientNum(), msg.chanID);
    channelSubscribers.Delete(msg.chanID, client->GetClientNum());
}

void ChatManager::RemoveAllChannels(Client* client)
{
    csHash<uint32_t, uint32_t>::Iterator iter(channelSubscriptions.GetIterator(client->GetClientNum()));

    while(iter.HasNext())
        channelSubscribers.Delete(iter.Next(), client->GetClientNum());

    channelSubscriptions.DeleteAll(client->GetClientNum());
}

/// TODO: This function is guaranteed not to work atm.-Keith
void ChatManager::SendNotice(psChatMessage &msg)
{
    //SendSay(NULL, msg, "Server");
}

void ChatManager::SendServerChannelMessage(psChatMessage &msg, uint32_t channelID)
{
    csArray<uint32_t> subscribers = channelSubscribers.GetAll(channelID);
    csArray<PublishDestination> destArray;
    for(size_t i = 0; i < subscribers.GetSize(); i++)
    {
        destArray.Push(PublishDestination(subscribers[i], NULL, 0, 0));
        Client* target = psserver->GetConnections()->Find(subscribers[i]);
        if(target && target->IsReady())
            target->GetActor()->LogChatMessage("Server Admin", msg);
    }

    msg.Multicast(destArray, 0, PROX_LIST_ANY_RANGE);
}

void ChatManager::SendShout(Client* c, psChatMessage &msg)
{
    psChatMessage newMsg(c->GetClientNum(), c->GetActor()->GetEID(), c->GetName(), 0, msg.sText, msg.iChatType, msg.translate);

    if(c->GetActor()->GetCharacterData()->GetTotalOnlineTime() > 3600 || c->GetActor()->GetSecurityLevel() >= GM_LEVEL_0)
    {
        csArray<PublishDestination> &clients = c->GetActor()->GetMulticastClients();
        newMsg.Multicast(clients, 0, PROX_LIST_ANY_RANGE);

        // The message is saved to the chat history of all the clients around
        for(size_t i = 0; i < clients.GetSize(); i++)
        {
            Client* target = psserver->GetConnections()->Find(clients[i].client);
            if(target && target->IsReady())
                target->GetActor()->LogChatMessage(c->GetActor()->GetFirstName(), newMsg);
        }
    }
    else
    {
        psserver->SendSystemError(c->GetClientNum(), "You are not allowed to shout or auction until you have been in-game for at least 1 hour.");
        psserver->SendSystemInfo(c->GetClientNum(), "Please use the Help tab or /help if you need help.");
    }
}

void ChatManager::SendSay(uint32_t clientNum, gemActor* actor, psChatMessage &msg,const char* who)
{
    float range = 0;
    psSectorInfo* sectorinfo = NULL;
    iSector* sector = actor->GetMeshWrapper()->GetMovable()->GetSectors()->Get(0);

    if(sector)
    {
        sectorinfo = psserver->GetCacheManager()->GetSectorInfoByName(sector->QueryObject()->GetName());
        if(sectorinfo)
        {
            range = sectorinfo->say_range;
        }
    }

    if(range == 0)  // If 0 set default
        range = CHAT_SAY_RANGE;

    psChatMessage newMsg(clientNum, actor->GetEID(), who, 0, msg.sText, msg.iChatType, msg.translate);
    csArray<PublishDestination> &clients = actor->GetMulticastClients();
    newMsg.Multicast(clients, 0, range);

    // The message is saved to the chat history of all the clients around (PS#2789)
    for(size_t i = 0; i < clients.GetSize(); i++)
    {
        Client* target = psserver->GetConnections()->Find(clients[i].client);
        if(target && clients[i].dist < range)
            target->GetActor()->LogChatMessage(actor->GetFirstName(), newMsg);
    }
}

void ChatManager::SendGuild(Client* client, psChatMessage &msg)
{
    psGuildInfo* guild;
    psGuildMember* member;

    guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not in a guild.");
        return;
    }

    member = client->GetCharacterData()->GetGuildMembership();
    if(member && !member->HasRights(RIGHTS_CHAT))
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not allowed to use your guild's chat channel.");
        return;
    }

    SendGuild(client->GetName(), client->GetActor()->GetEID(), guild, msg);
}

void ChatManager::SendGuild(const csString &sender, EID senderEID, psGuildInfo* guild, psChatMessage &msg)
{
    ClientIterator iter(*psserver->GetConnections());
    psGuildMember* member;

    while(iter.HasNext())
    {
        Client* client = iter.Next();
        if(!client->IsReady()) continue;
        if(client->GetGuildID() != guild->GetID()) continue;
        member = client->GetCharacterData()->GetGuildMembership();
        if((!member) || (!member->HasRights(RIGHTS_VIEW_CHAT))) continue;
        // Send the chat message
        psChatMessage newMsg(client->GetClientNum(), senderEID, sender, 0, msg.sText, msg.iChatType, msg.translate);
        newMsg.SendMessage();
        // The message is saved to the chat history of all the clients in the same guild (PS#2789)
        client->GetActor()->LogChatMessage(sender.GetData(), msg);
    }
}

void ChatManager::SendAlliance(Client* client, psChatMessage &msg)
{
    psGuildInfo* guild;
    psGuildMember* member;

    guild = client->GetCharacterData()->GetGuild();
    if(guild == NULL)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not in a alliance.");
        return;
    }

    member = client->GetCharacterData()->GetGuildMembership();
    if(member && !member->HasRights(RIGHTS_CHAT_ALLIANCE))
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not allowed to use your alliance's chat channel.");
        return;
    }

    psGuildAlliance* alliance = psserver->GetCacheManager()->FindAlliance(guild->GetAllianceID());
    if(alliance == NULL)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "Your guild is not in an alliance.");
        return;
    }

    SendAlliance(client->GetName(), client->GetActor()->GetEID(), alliance, msg);
}

void ChatManager::SendAlliance(const csString &sender, EID senderEID, psGuildAlliance* alliance, psChatMessage &msg)
{
    ClientIterator iter(*psserver->GetConnections());
    psGuildMember* member;

    while(iter.HasNext())
    {
        Client* client = iter.Next();
        if(!client->IsReady()) continue;
        if(client->GetAllianceID() != alliance->GetID()) continue;
        member = client->GetCharacterData()->GetGuildMembership();
        if((!member) || (!member->HasRights(RIGHTS_VIEW_CHAT_ALLIANCE))) continue;
        // Send the chat message
        psChatMessage newMsg(client->GetClientNum(), senderEID, sender, 0, msg.sText, msg.iChatType, msg.translate);
        newMsg.SendMessage();
        // The message is saved to the chat history of all the clients in the same alliance (PS#2789)
        client->GetActor()->LogChatMessage(sender.GetData(), msg);
    }
}

void ChatManager::SendGroup(Client* client, psChatMessage &msg)
{
    csRef<PlayerGroup> group = client->GetActor()->GetGroup();
    if(group)
    {
        psChatMessage newMsg(0, client->GetActor()->GetEID(), client->GetName(), 0, msg.sText, msg.iChatType, msg.translate);
        group->Broadcast(newMsg.msg);
        // Save chat message to grouped clients' history (PS#2789)
        for(size_t i=0; i<group->GetMemberCount(); i++)
        {
            group->GetMember(i)->LogChatMessage(client->GetActor()->GetFirstName(), newMsg);
        }
    }
    else
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not part of any group.");
    }
}


void ChatManager::SendTell(psChatMessage &msg, const char* who,Client* client,Client* target)
{
    Debug2(LOG_CHAT, client->GetClientNum(), "SendTell: %s!", msg.sText.GetDataSafe());

    // Sanity check that we are sending to correct clientnum!
    csString targetName = msg.sPerson;
    targetName = NormalizeCharacterName(targetName);
    CS_ASSERT(strcasecmp(target->GetName(), targetName) == 0);

    // Create a new message and send it to that person if found
    psChatMessage cmsg(target->GetClientNum(), client->GetActor()->GetEID(), who, 0, msg.sText, msg.iChatType, msg.translate);
    cmsg.SendMessage();

    // Echo the message back to the speaker also
    psChatMessage cmsg2(client->GetClientNum(), client->GetActor()->GetEID(), targetName, 0, msg.sText, CHAT_TELLSELF, msg.translate);
    cmsg2.SendMessage();

    // Save to both actors' chat history (PS#2789)
    client->GetActor()->LogChatMessage(who, msg);
    target->GetActor()->LogChatMessage(who, msg);
}

NpcResponse* ChatManager::CheckNPCEvent(Client* client,csString &triggerText,gemNPC* &target)
{
    gemNPC* npc = target;

    if(npc && npc->IsAlive())
    {
        csString trigger(triggerText);
        trigger.Downcase();

        psNPCDialog* npcdlg = npc->GetNPCDialogPtr();

        if(npcdlg)   // if NULL, then NPC never speaks
        {
            float dist = npc->RangeTo(client->GetActor());

            if(dist > MAX_NPC_DIALOG_DIST)
                return NULL;

            Debug3(LOG_NPC, client->GetClientNum(),"%s checking trigger %s.\n",target->GetName(),trigger.GetData());
            return npcdlg->Respond(trigger,client);
        }
        else
        {
            // Admins and GMs can see a better error on the client
            if(client->GetSecurityLevel() > 20)
                psserver->SendSystemError(client->GetClientNum(),"%s cannot speak.",npc->GetName());

            Debug2(LOG_NPC, client->GetClientNum(),"NPC %s cannot speak.\n",npc->GetName());
        }
    }
    return NULL;
}

NpcResponse* ChatManager::CheckNPCResponse(psChatMessage &msg,Client* client,gemNPC* &target)
{
    return CheckNPCEvent(client,msg.sText,target);  // <L MONEY="0,0,0,3"></L>
}

void ChatManager::SendMultipleAudioFileHashes(Client* client, const char* voiceFiles)
{
    if(!voiceFiles || voiceFiles[0]==0)
        return;

    psString files(voiceFiles);
    csStringArray filelist;

    files.Split(filelist,'|');
    csTicks delay=0;
    for(size_t i=0; i<filelist.GetSize(); i++)
    {
        SendAudioFileHash(client, filelist.Get(i), delay);
        delay += 5000; // wait 5 seconds before sending next file for queueing
    }
}

void ChatManager::SendAudioFileHash(Client* client, const char* voiceFile, csTicks delay)
{
    if(!voiceFile || voiceFile[0]==0)
        return;

    // printf("Sending file '%s' in %d msec.\n",voiceFile,delay);

    csString timestamp;
    csRef<iDataBuffer> buffer;

    // printf("Checking cache for audio file '%s'.\n", voiceFile);

    // Check cache for file
    csRef<iDataBuffer> cache;
    size_t i;
    for(i=0; i < audioFileCache.GetSize(); i++)
    {
        // printf("Cached item %d: '%s'\n", i, audioFileCache[i]->key.GetDataSafe() );

        if(audioFileCache[i]->key == voiceFile)  // found
        {
            // printf("Found in cache.  Moving to front.\n");

            buffer = audioFileCache[i]->data;
            timestamp = audioFileCache[i]->alternate;
            // now make a new copy of the entry and move up to front of array
            audioFileCache.DeleteIndex(i);
            CachedData* n = new CachedData(buffer,voiceFile,timestamp);
            audioFileCache.Insert(0,n);
            break;
        }
    }

    if(i == audioFileCache.GetSize())   // not found
    {
        // printf("File not found in cache.  Loading from disk.\n");

        buffer = psserver->vfs->ReadFile(voiceFile);
        if(!buffer.IsValid())
        {
            Error2("Audio file '%s' not found.\n", voiceFile);
            return;
        }
        if(buffer->GetSize() > 63000)  // file is too big
        {
            Error2("Audio file '%s' is too big!\n", voiceFile);
            // buffer ref auto release at this return
            return;
        }
        csFileTime oTime;
        psserver->vfs->GetFileTime(voiceFile,oTime);

        timestamp.Format("(%d/%d/%d %d:%d:%d) ",
                         oTime.mon, oTime.day, oTime.year,
                         oTime.hour, oTime.min, oTime.sec);


        timestamp.Append(voiceFile);

        csString hash = csMD5::Encode(timestamp).HexString();

        // printf("Caching %s to hash value: %s\n", timestamp.GetData(), hash.GetData() );

        // Add newly read file to the MRU cache
        CachedData* n = new CachedData(buffer,voiceFile,hash);
        audioFileCache.Insert(0,n);
        // We added one to the front of our list, so keep the list at no more than 100 items.
        if(audioFileCache.GetSize() > 100)
            audioFileCache.Pop();
    }

    // Send the file message but without the file.  The client will
    // check the file hash.  If the client needs the file, it will reply
    // back with a request for the file, which will come here
    // and call SendAudioFile.
    psCachedFileMessage msg(client->GetClientNum(),
                            client->GetOrderedMessageChannel(MSGTYPE_CACHEFILE)->IncrementSequenceNumber(), //guaranteed to be played in sequence order
                            timestamp,                                          //hash id value
                            NULL);                                              //null buffer means check in client-side cache first
    msg.SendMessage();
    // printf("Cached file hash message sent.");
}

void ChatManager::SendAudioFile(Client* client, const char* voiceFileHash)
{
    if(!voiceFileHash || voiceFileHash[0]==0)
        return;

    csRef<iDataBuffer> buffer;
    csString timestamp,voiceFile;

    // printf("Checking cache for audio file '%s'.\n", voiceFileHash);

    // Check cache for file
    csRef<iDataBuffer> cache;
    for(size_t i=0; i < audioFileCache.GetSize(); i++)
    {
        // printf("Cached item %d: '%s'\n", i, audioFileCache[i]->key.GetDataSafe() );

        if(audioFileCache[i]->alternate == voiceFileHash)  // found
        {
            // printf("Found in cache.  Moving to front.\n");

            buffer = audioFileCache[i]->data;
            timestamp = audioFileCache[i]->alternate;
            voiceFile = audioFileCache[i]->key;

            int sequence = client->GetOrderedMessageChannel(MSGTYPE_CACHEFILE)->IncrementSequenceNumber();
            psCachedFileMessage msg(client->GetClientNum(),sequence, timestamp, buffer);
            msg.SendMessage();
            // printf("Cached file message sent to client with buffer attached, seq=%d.\n",sequence);
            return;
        }
    }
    Error2("Client requested file hash that does not exist. (%s)", voiceFileHash);
}

csString ChatManager::channelsToString()
{
    csString string;
    for(uint32_t i = 1; i <= channelNames.GetSize(); i++)
    {
        csArray<uint32_t> subscribers = channelSubscribers.GetAll(i);
        string += channelNames.Get(i, "");
        string += " : ";
        for(size_t i = 0; i < subscribers.GetSize(); i++)
        {
            Client* target = psserver->GetConnections()->Find(subscribers[i]);
            if(target && target->IsReady())
            {
                string += target->GetActor()->GetName();
                string += " ";
            }
        }
        string += "\n";
    }
    return string;
}




/************************************************************************************************/


psEndChatLoggingEvent::psEndChatLoggingEvent(uint32_t _clientnum, const int delayticks=5000)
    : psGameEvent(0,delayticks,"psEndChatLoggingEvent")
{
#ifdef _psEndChatLoggingEvent_DEBUG_
    CPrintf(CON_DEBUG, "EndOfChatLoggingEvent created for clientnum %i!", _clientnum);
#endif

    clientnum = _clientnum;
}

void psEndChatLoggingEvent::Trigger()
{
#ifdef _psEndChatLoggingEvent_DEBUG_
    CPrintf(CON_DEBUG, "EndOfChatLoggingEvent is about to happen on clientnum %i!", clientnum);
#endif

    Client* client = NULL;

    client = psserver->GetConnections()->Find(clientnum);
    if(!client)
    {
#ifdef _psEndChatLoggingEvent_DEBUG_
        CPrintf(CON_DEBUG, "EndOfChatLoggingEvent on unknown client!");
#endif
        return;
    }
    client->GetActor()->RemoveChatReport();
}



