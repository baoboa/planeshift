/*
 * messages.cpp by Keith Fulton <keith@paqrat.com>
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

#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <csutil/databuf.h>
#include <csutil/zip.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <imesh/spritecal3d.h>
#include <iutil/object.h>

#include "util/psconst.h"
#include "util/strutil.h"
#include "util/psxmlparser.h"
#include "net/netbase.h"
#include "net/messages.h"
#include "util/skillcache.h"
#include "net/npcmessages.h"
#include "util/log.h"
#include "net/msghandler.h"
#include "util/slots.h"
#include "rpgrules/vitals.h"
#include "engine/linmove.h"

// uncomment this to produce full debug dumping in <class>::ToString functions
#define FULL_DEBUG_DUMP
//

MsgHandler* psMessageCracker::msghandler;


void psMessageCracker::SendMessage()
{
    CS_ASSERT(valid);
    CS_ASSERT(msg);
    msghandler->SendMessage(msg);
}


void psMessageCracker::Multicast(csArray<PublishDestination> &multi, uint32_t except, float range)
{
    CS_ASSERT(valid);
    CS_ASSERT(msg);
    msghandler->Multicast(msg,multi,except,range);
}

void psMessageCracker::FireEvent()
{
    CS_ASSERT(valid);
    CS_ASSERT(msg);
    msghandler->Publish(msg);
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharDeleteMessage,MSGTYPE_CHAR_DELETE);

psCharDeleteMessage::psCharDeleteMessage(const char* name, uint32_t client)
{
    if(!name)
        return;

    msg.AttachNew(new MsgEntry(strlen(name) + 1));

    msg->SetType(MSGTYPE_CHAR_DELETE);
    msg->clientnum = client;
    msg->Add(name);

    valid = !(msg->overrun);
}

psCharDeleteMessage::psCharDeleteMessage(MsgEntry* message)
{
    if(!message)
        return;
    charName = message->GetStr();
}

csString psCharDeleteMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("CharName: '%s'", charName.GetDataSafe());

    return msgtext;
}


// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPreAuthenticationMessage,MSGTYPE_PREAUTHENTICATE);

psPreAuthenticationMessage::psPreAuthenticationMessage(uint32_t clientnum,
        uint32_t version)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)));

    msg->SetType(MSGTYPE_PREAUTHENTICATE);
    msg->clientnum      = clientnum;

    msg->Add(version);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psPreAuthenticationMessage::psPreAuthenticationMessage(MsgEntry* message)
{
    if(!message)
        return;

    netversion = message->GetUInt32();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

bool psPreAuthenticationMessage::NetVersionOk()
{
    return netversion == PS_NETVERSION;
}

csString psPreAuthenticationMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("NetVersion: %d", netversion);

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psAuthenticationMessage,MSGTYPE_AUTHENTICATE);

psAuthenticationMessage::psAuthenticationMessage(uint32_t clientnum,
        const char* userid,const char* password, const char* os, const char* gfxcard, const char* gfxversion, const char* password256, uint32_t version)
{

    if(!userid || !password)
    {
        msg = NULL;
        return;
    }


    msg.AttachNew(new MsgEntry(strlen(userid)+1+strlen(password)+1+strlen(os)+1+strlen(gfxcard)+1+strlen(gfxversion)+1+strlen(password256)+1+sizeof(uint32_t),PRIORITY_LOW));

    msg->SetType(MSGTYPE_AUTHENTICATE);
    msg->clientnum      = clientnum;

    msg->Add(version);
    msg->Add(userid);
    msg->Add(password);
    msg->Add(os);
    msg->Add(gfxcard);
    msg->Add(gfxversion);
    msg->Add(password256);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psAuthenticationMessage::psAuthenticationMessage(MsgEntry* message)
{
    if(!message)
        return;

    netversion = message->GetUInt32();
    sUser = message->GetStr();
    sPassword = message->GetStr();
    os_ = message->GetStr();
    gfxcard_ = message->GetStr();
    gfxversion_ = message->GetStr();
    if(!message->IsEmpty())
    {
        sPassword256 = message->GetStr();
    }

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

bool psAuthenticationMessage::NetVersionOk()
{
    return netversion == PS_NETVERSION;
}

csString psAuthenticationMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("NetVersion: %d User: '%s' Passwd: '%s'",
                      netversion,sUser.GetDataSafe(),sPassword.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psAuthApprovedMessage,MSGTYPE_AUTHAPPROVED);

psAuthApprovedMessage::psAuthApprovedMessage(uint32_t clientnum,
        PID playerID, uint8_t numChars)
{
    msgClientValidToken = clientnum;
    msgPlayerID = playerID;
    msgNumOfChars = numChars;
}

psAuthApprovedMessage::psAuthApprovedMessage(MsgEntry* message)
{
    if(!message)
        return;

    msgClientValidToken = message->GetUInt32();
    msgPlayerID         = PID(message->GetUInt32());
    msgNumOfChars       = message->GetUInt8();
}

void psAuthApprovedMessage::AddCharacter(const char* fullname, const char* race,
        const char* mesh, const char* traits,
        const char* equipment)
{
    contents.Push(fullname);
    contents.Push(race);
    contents.Push(mesh);
    contents.Push(traits);
    contents.Push(equipment);
}

void psAuthApprovedMessage::GetCharacter(MsgEntry* message,
        csString &fullname, csString &race,
        csString &mesh, csString &traits,
        csString &equipment)
{
    fullname  = message->GetStr();
    race      = message->GetStr();
    mesh      = message->GetStr();
    traits    = message->GetStr();
    equipment = message->GetStr();
}

void psAuthApprovedMessage::ConstructMsg()
{
    size_t msgSize = sizeof(uint32_t)*2 + sizeof(uint8_t);

    for(size_t i = 0; i < contents.GetSize(); ++i)
        msgSize += strlen(contents[i]) + 1;

    msg.AttachNew(new MsgEntry(msgSize));

    msg->SetType(MSGTYPE_AUTHAPPROVED);
    msg->clientnum      = msgClientValidToken;

    msg->Add(msgClientValidToken);
    msg->Add(msgPlayerID.Unbox());
    msg->Add(msgNumOfChars);

    for(size_t i = 0; i < contents.GetSize(); ++i)
        msg->Add(contents[i]);
}

csString psAuthApprovedMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ClientValidToken: %d PlayerID: %d NumOfChars: %d",
                      msgClientValidToken, msgPlayerID.Unbox(), msgNumOfChars);

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPreAuthApprovedMessage,MSGTYPE_PREAUTHAPPROVED);

psPreAuthApprovedMessage::psPreAuthApprovedMessage(uint32_t clientnum)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)));

    msg->SetType(MSGTYPE_PREAUTHAPPROVED);
    msg->clientnum      = clientnum;

    msg->Add(clientnum);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psPreAuthApprovedMessage::psPreAuthApprovedMessage(MsgEntry* message)
{
    if(!message)
        return;

    ClientNum = message->GetUInt32();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psPreAuthApprovedMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("CNUM: %d",ClientNum);

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psAuthRejectedMessage,MSGTYPE_AUTHREJECTED);

psAuthRejectedMessage::psAuthRejectedMessage(uint32_t clientnum,
        const char* reason)
{
    if(!reason)
        return;

    msg.AttachNew(new MsgEntry(strlen(reason)+1));

    msg->SetType(MSGTYPE_AUTHREJECTED);
    msg->clientnum      = clientnum;

    msg->Add(reason);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psAuthRejectedMessage::psAuthRejectedMessage(MsgEntry* message)
{
    if(!message)
        return;

    msgReason = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psAuthRejectedMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Reason: '%s'",msgReason.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharacterPickerMessage,MSGTYPE_AUTHCHARACTER);

psCharacterPickerMessage::psCharacterPickerMessage(const char* characterName)
{
    msg.AttachNew(new MsgEntry(strlen(characterName)+1));

    msg->SetType(MSGTYPE_AUTHCHARACTER);
    msg->clientnum      = 0;

    msg->Add(characterName);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psCharacterPickerMessage::psCharacterPickerMessage(MsgEntry* message)
{
    characterName = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psCharacterPickerMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("CharacterName: '%s'",characterName.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharacterApprovedMessage,MSGTYPE_AUTHCHARACTERAPPROVED);

psCharacterApprovedMessage::psCharacterApprovedMessage(uint32_t clientnum)
{
    msg.AttachNew(new MsgEntry());

    msg->SetType(MSGTYPE_AUTHCHARACTERAPPROVED);
    msg->clientnum = clientnum;

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psCharacterApprovedMessage::psCharacterApprovedMessage(MsgEntry* /*message*/)
{
    // No data, always valid
}

csString psCharacterApprovedMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Approved");

    return msgtext;
}


// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psChatMessage,MSGTYPE_CHAT);

psChatMessage::psChatMessage(uint32_t cnum, EID actorid, const char* person, const char* other,
                             const char* chatMessage, uint8_t type, bool translate, uint16_t channelID)
{
    if(!chatMessage || !person)
        return;

    iChatType = type;
    sPerson   = person;
    sOther    = other;
    sText     = chatMessage;
    this->translate = translate;
    this->channelID = channelID;

    bool includeOther = iChatType == CHAT_ADVISOR;
    size_t sz = strlen(person) + 1 + strlen(chatMessage) + 1 + sizeof(uint8_t)*2 + sizeof(uint32_t);
    if(includeOther)
        sz += strlen(other) + 1;

    bool chanMsg = iChatType == CHAT_CHANNEL;
    if(chanMsg)
        sz += sizeof(uint16_t);

    msg.AttachNew(new MsgEntry(sz));

    msg->SetType(MSGTYPE_CHAT);
    msg->clientnum = cnum;

    msg->Add(iChatType);
    msg->Add(person);
    msg->Add(chatMessage);
    msg->Add(translate);
    msg->Add(actorid.Unbox());
    if(chanMsg)
        msg->Add(channelID);
    if(includeOther)
        msg->Add(other);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psChatMessage::psChatMessage(MsgEntry* message)
{
    if(!message)
        return;

    iChatType = message->GetUInt8();
    bool includeOther = iChatType == CHAT_ADVISOR;
    bool chanMsg = iChatType == CHAT_CHANNEL;

    sPerson   = message->GetStr();
    sText     = message->GetStr();
    translate = message->GetBool();
    actor     = message->GetUInt32();
    if(chanMsg)
        channelID = message->GetUInt16();
    if(includeOther && !message->IsEmpty())
        sOther = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
    valid = valid && !(sText.IsEmpty());
}


csString psChatMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ChatType: %s",GetTypeText());
    msgtext.AppendFmt(" Person: %s",sPerson.GetDataSafe());
    msgtext.AppendFmt(" Text: %s",sText.GetDataSafe());
    msgtext.AppendFmt(" Translate: %s",(translate?"true":"false"));
    msgtext.AppendFmt(" actor: %s",ShowID(actor));

    return msgtext;
}

const char* psChatMessage::GetTypeText()
{
    switch(iChatType)
    {
        case CHAT_SYSTEM:
            return "System";
        case CHAT_COMBAT:
            return "Combat";
        case CHAT_SAY:
            return "Say";
        case CHAT_TELL:
            return "Tell";
        case CHAT_GROUP:
            return "GroupMsg";
        case CHAT_GUILD:
            return "GuildChat";
        case CHAT_ALLIANCE:
            return "AllianceChat";
        case CHAT_AUCTION:
            return "Auction";
        case CHAT_SHOUT:
            return "Shout";
        case CHAT_CHANNEL:
            return "Channel";
        case CHAT_TELLSELF:
            return "TellSelf";
        case CHAT_REPORT:
            return "Report";
        case CHAT_ADVISOR:
            return "Advisor";
        case CHAT_ADVICE:
            return "Advice";
        case CHAT_ADVICE_LIST:
            return "Advice List";
        case CHAT_SERVER_TELL:
            return "Server Tell";
        case CHAT_GM:
            return "GM";
        case CHAT_NPC:
            return "TellNPC";
        case CHAT_NPCINTERNAL:
            return "NPCInternal";
        case CHAT_SYSTEM_BASE:
            return "SystemBase";
        case CHAT_PET_ACTION:
            return "Action";
        case CHAT_NPC_ME:
            return "NPC Me";
        case CHAT_NPC_MY:
            return "NPC My";
        case CHAT_NPC_NARRATE:
            return "NPC Narrate";
        case CHAT_AWAY:
            return "Away";
        default:
            return "Unknown";
    }
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psChannelJoinMessage,MSGTYPE_CHANNEL_JOIN);

psChannelJoinMessage::psChannelJoinMessage(const char* name)
{
    msg.AttachNew(new MsgEntry(strlen(name) + 1));
    msg->SetType(MSGTYPE_CHANNEL_JOIN);
    msg->clientnum = 0;

    msg->Add(name);

    valid=!(msg->overrun);
}

psChannelJoinMessage::psChannelJoinMessage(MsgEntry* message)
{
    channel = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psChannelJoinMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString text;
    text.Format("Channel name: %s", channel.GetData());
    return text;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psChannelJoinedMessage,MSGTYPE_CHANNEL_JOINED);

psChannelJoinedMessage::psChannelJoinedMessage(uint32_t clientnum, const char* name, uint16_t id)
{
    msg.AttachNew(new MsgEntry(strlen(name) + sizeof(uint16_t) + 1));
    msg->SetType(MSGTYPE_CHANNEL_JOINED);
    msg->clientnum = clientnum;

    msg->Add(name);
    msg->Add(id);

    valid=!(msg->overrun);
}

psChannelJoinedMessage::psChannelJoinedMessage(MsgEntry* message)
{
    channel = message->GetStr();
    id = message->GetUInt16();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psChannelJoinedMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString text;
    text.Format("Channel ID: %d, name: %s", id, channel.GetData());
    return text;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psChannelLeaveMessage,MSGTYPE_CHANNEL_LEAVE);

psChannelLeaveMessage::psChannelLeaveMessage(uint16_t chanID)
{
    msg.AttachNew(new MsgEntry(sizeof(uint16_t)));
    msg->SetType(MSGTYPE_CHANNEL_LEAVE);
    msg->clientnum = 0;

    msg->Add(chanID);

    valid=!(msg->overrun);
}

psChannelLeaveMessage::psChannelLeaveMessage(MsgEntry* message)
{
    chanID = message->GetUInt16();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psChannelLeaveMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString text;
    text.Format("Channel ID: %d", chanID);
    return text;
}

// ---------------------------------------------------------------------------

psSystemMessageSafe::psSystemMessageSafe(uint32_t clientnum, uint32_t msgtype,
        const char* text)
{
    char str[MAXSYSTEMMSGSIZE];

    strncpy(str, text, MAXSYSTEMMSGSIZE-1);
    str[MAXSYSTEMMSGSIZE-1]=0x00;

    msg.AttachNew(new MsgEntry(strlen(str) + 1 + sizeof(uint32_t)));

    msg->SetType(MSGTYPE_SYSTEM);
    msg->clientnum      = clientnum;

    msg->Add(msgtype);
    msg->Add(str);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSystemMessage,MSGTYPE_SYSTEM);

psSystemMessage::psSystemMessage(uint32_t clientnum, uint32_t msgtype, const char* fmt, ...)
{
    char str[MAXSYSTEMMSGSIZE];
    va_list args;

    va_start(args, fmt);

    /* This appears to work now on MSVC with vsnprintf() defined as _vsnprintf()
     *  If this is not the case please report exactly which version of MSVC this has
     *  a problem on.
     */
    vsnprintf(str,MAXSYSTEMMSGSIZE-1, fmt, args);
    str[MAXSYSTEMMSGSIZE-1]=0x00;
    va_end(args);

    msg.AttachNew(new MsgEntry(strlen(str) + 1 + sizeof(uint32_t)));

    msg->SetType(MSGTYPE_SYSTEM);
    msg->clientnum      = clientnum;

    msg->Add(msgtype);
    msg->Add(str);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psSystemMessage::psSystemMessage(uint32_t clientnum, uint32_t msgtype, const char* fmt, va_list args)
{
    char str[MAXSYSTEMMSGSIZE];

    vsnprintf(str,MAXSYSTEMMSGSIZE-1, fmt, args);
    str[MAXSYSTEMMSGSIZE-1]=0x00;

    msg.AttachNew(new MsgEntry(strlen(str) + 1 + sizeof(uint32_t)));

    msg->SetType(MSGTYPE_SYSTEM);
    msg->clientnum      = clientnum;

    msg->Add(msgtype);
    msg->Add(str);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psSystemMessage::psSystemMessage(MsgEntry* message)
{
    type    = message->GetUInt32();
    msgline = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psSystemMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Type: %d Msg: %s",type,msgline.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPetitionMessage,MSGTYPE_PETITION);

psPetitionMessage::psPetitionMessage(uint32_t clientnum, csArray<psPetitionInfo>* petition, const char* errMsg,
                                     bool succeed, int type, bool gm)
{
    size_t petitionLen = (petition != NULL ? petition->GetSize():0);
    size_t messageSize = 0;
    size_t errLen = 0;
    if(errMsg)
    {
        errLen = strlen(errMsg)+1;
    }
    psPetitionInfo* curr;

    for(size_t i = 0; i < petitionLen; i++)
    {
        curr = &petition->Get(i);

        messageSize+= sizeof(int32_t);                // Petition ID
        messageSize+= curr->petition.Length()+1;      // Petition String
        messageSize+= curr->status.Length()+1;        // Petition status string
        if(!gm)
        {
            messageSize+=curr->created.Length()+1;
            messageSize+=curr->assignedgm.Length()+1;
            messageSize+=curr->resolution.Length()+1;
        }
        else
        {
            messageSize+=sizeof(int32_t);
            messageSize+=curr->assignedgm.Length()+1;
            messageSize+=curr->created.Length()+1;
            messageSize+=curr->player.Length()+1;
            messageSize+=sizeof(bool);                  // online flag for players
        }
    }

    msg.AttachNew(new MsgEntry(sizeof(int32_t) +    // int32_t pettionLen
                               sizeof(gm)      +    // Space for boolean GM flag.
                               messageSize     +    // Space for all petitions
                               errLen          +    // Space for error message.
                               sizeof(succeed) +    // Space for success boolean
                               sizeof(int32_t)));    // Space for type int32_t


    //+ (maxPetitionTextLen + 250)* (petitionLen+1) + sizeof(errMsg) +
    //       sizeof(succeed) + sizeof(int32_t) + sizeof(gm));

    msg->SetType(MSGTYPE_PETITION);
    msg->clientnum      = clientnum;
    msg->priority       = PRIORITY_HIGH;

    msg->Add((int32_t)petitionLen);
    msg->Add(gm);
    psPetitionInfo* current;
    for(size_t i = 0; i < petitionLen; i++)
    {
        current = &petition->Get(i);
        msg->Add((int32_t)current->id);
        msg->Add(current->petition.GetData());
        msg->Add(current->status.GetData());
        msg->Add(current->created.GetData());
        msg->Add(current->assignedgm.GetData());
        // Add specified fields based upon GM status or not
        if(!gm)
        {
            msg->Add(current->resolution.GetData());
        }
        else
        {
            msg->Add((int32_t)current->escalation);
            msg->Add(current->player.GetData());
            msg->Add(current->online);
        }
    }
    msg->Add(errMsg);
    msg->Add(succeed);
    msg->Add((int32_t)type);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);

    if(valid)
        msg->ClipToCurrentSize();
}

psPetitionMessage::psPetitionMessage(MsgEntry* message)
{
    // Get the petitions
    int count = message->GetInt32();
    isGM = message->GetBool();
    psPetitionInfo current;

    for(int i = 0; i < count; i++)
    {
        // Set each property in psPetitionInfo:
        current.id = message->GetInt32();
        current.petition = message->GetStr();
        current.status = message->GetStr();
        current.created = message->GetStr();
        current.assignedgm = message->GetStr();

        // Check GM fields or user fields:
        if(!isGM)
        {
            current.resolution = message->GetStr();
        }
        else
        {
            current.escalation = message->GetInt32();
            current.player = message->GetStr();
            current.online = message->GetBool();
        }

        petitions.Push(current);
    }

    // Get the rest of the data:
    error = message->GetStr();
    success = message->GetBool();
    msgType = message->GetInt32();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psPetitionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("IsGM: %s",(isGM?"true":"false"));

    for(size_t i = 0; i < petitions.GetSize(); i++)
    {
        psPetitionInfo current = petitions[i];

        msgtext.AppendFmt(" %zu(ID: %d Petition: '%s' Status: '%s'",
                          i,current.id,current.petition.GetDataSafe(),
                          current.status.GetDataSafe());

        // Check GM fields or user fields:
        if(!isGM)
        {
            msgtext.AppendFmt(" Created: '%s' AssignedGM: '%s')",
                              current.created.GetDataSafe(),
                              current.assignedgm.GetDataSafe());
        }
        else
        {
            msgtext.AppendFmt(" Escalation: %d Created: '%s' Player: '%s' AssignedGM: '%s'",
                              current.escalation,
                              current.created.GetDataSafe(),
                              current.player.GetDataSafe(),
                              current.assignedgm.GetDataSafe());
        }
    }
    msgtext.AppendFmt(" Error: '%s' Success: %s MsgType: %d",
                      error.GetDataSafe(),(success?"true":"false"),msgType);

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPetitionRequestMessage,MSGTYPE_PETITION_REQUEST);

psPetitionRequestMessage::psPetitionRequestMessage(bool gm, const char* requestCmd, int petitionID, const char* petDesc)
{
    msg.AttachNew(new MsgEntry((strlen(requestCmd) + 1) + (strlen(petDesc)+1) + sizeof(gm) + sizeof(int32_t)));

    msg->SetType(MSGTYPE_PETITION_REQUEST);
    msg->clientnum      = 0;

    msg->Add(gm);
    msg->Add(requestCmd);
    msg->Add((int32_t)petitionID);
    msg->Add(petDesc);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);

    if(valid)
        msg->ClipToCurrentSize();
}

psPetitionRequestMessage::psPetitionRequestMessage(MsgEntry* message)
{
    isGM = message->GetBool();
    request = message->GetStr();
    id = message->GetInt32();
    desc = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psPetitionRequestMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("IsGM: %s Request: '%s' Id: %d Desc: '%s'",
                      (isGM?"true":"false"),request.GetDataSafe(),id,
                      desc.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMGuiMessage,MSGTYPE_GMGUI);

psGMGuiMessage::psGMGuiMessage(uint32_t clientnum, int gmSets)
{
    msg.AttachNew(new MsgEntry(sizeof(gmSettings) + sizeof(type) + 1));
    gmSettings = gmSets;

    msg->SetType(MSGTYPE_GMGUI);
    msg->clientnum = clientnum;

    msg->Add(TYPE_GETGMSETTINGS);
    msg->Add(gmSettings);

    valid=!(msg->overrun);
    if(valid)
        msg->ClipToCurrentSize();
}

psGMGuiMessage::psGMGuiMessage(uint32_t clientnum, csArray<PlayerInfo>* playerArray, int type)
{
    size_t playerCount = (playerArray == NULL ? 0 : playerArray->GetSize());
    msg.AttachNew(new MsgEntry(sizeof(playerCount) + 250 * (playerCount+1) + sizeof(type)));

    msg->SetType(MSGTYPE_GMGUI);
    msg->clientnum = clientnum;

    msg->Add((int32_t)type);
    msg->Add((uint32_t)playerCount);
    for(size_t i=0; i<playerCount; i++)
    {
        PlayerInfo playerInfo = playerArray->Get(i);

        msg->Add(playerInfo.name.GetData());
        msg->Add(playerInfo.lastName.GetData());
        msg->Add((int32_t)playerInfo.gender);
        msg->Add(playerInfo.guild.GetData());
        msg->Add(playerInfo.sector.GetData());
    }

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
    if(valid)
        msg->ClipToCurrentSize();
}


psGMGuiMessage::psGMGuiMessage(MsgEntry* message)
{
    type = message->GetInt32();

    if(type == TYPE_GETGMSETTINGS)
        gmSettings = message->GetInt32();
    else
    {
        size_t playerCount = message->GetUInt32();
        for(size_t i=0; i<playerCount; i++)
        {
            PlayerInfo playerInfo;
            playerInfo.name = message->GetStr();
            playerInfo.lastName = message->GetStr();
            playerInfo.gender = message->GetInt32();
            playerInfo.guild = message->GetStr();
            playerInfo.sector = message->GetStr();

            players.Push(playerInfo);
        }
    }

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGMGuiMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    for(size_t i=0; i < players.GetSize(); i++)
    {
        PlayerInfo p = players[i];
        msgtext.AppendFmt(" %zu(Name: '%s' Last: '%s'"
                          " Gender: %d Guild: '%s'"
                          " Sector: '%s')",i,
                          p.name.GetDataSafe(),p.lastName.GetDataSafe(),
                          p.gender,p.guild.GetDataSafe(),
                          p.sector.GetDataSafe());
    }

    msgtext.AppendFmt(" Type: %d",type);

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGuildCmdMessage,MSGTYPE_GUILDCMD);

psGuildCmdMessage::psGuildCmdMessage(const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_GUILDCMD);
    msg->clientnum      = 0;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGuildCmdMessage::psGuildCmdMessage(MsgEntry* message)
{
    valid = true;
    level = 0;

    WordArray words(message->GetStr());

    command = words[0];

    if(command == "/newguild" || command == "/endguild" || command == "/guildname" || command == "/guildpoints" || command == "/allianceleader")
    {
        guildname = words.GetTail(1);
        return;
    }
    if(command == "/guildinvite" || command == "/guildremove" || command == "/allianceinvite" || command == "/getmemberpermissions")
    {
        player = words[1];
        return;
    }
    if(command == "/setmemberpermissions")
    {
        player = words[1];
        subCmd = words[2];
        permission = words[3];
        return;
    }
    if(command == "/guildlevel")
    {
        level = words.GetInt(1);
        levelname = words.GetTail(2);
        return;
    }
    if(command == "/guildmembers")
    {
        level = words.GetInt(1);
        return;
    }
    if(command == "/guildpromote")
    {
        player = words[1];
        level = words.GetInt(2);
        return;
    }
    if(command == "/guildsecret")
    {
        secret = words[1];
        return;
    }
    if(command == "/guildweb")
    {
        web_page = words.GetTail(1);
        return;
    }
    if(command == "/guildmotd")
    {
        motd = words.GetTail(1);
        return;
    }
    if(command == "/newalliance" || command == "/allianceremove")
    {
        alliancename = words.GetTail(1);
        return;
    }
    if(command == "/endalliance" ||
            command == "/allianceleave" ||
            command == "/guildinfo" ||
            command == "/guildwar"    ||
            command == "/guildyield")
    {
        return;
    }

    valid = false;
}

csString psGuildCmdMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: '%s'",command.GetDataSafe());

    if(command == "/newguild" || command == "/endguild" || command == "/guildname" || command == "/guildpoints")
    {
        msgtext.AppendFmt("GuildName: '%s'",guildname.GetDataSafe());
        return msgtext;
    }
    if(command == "/guildinvite" || command == "/guildremove" || command == "/allianceinvite")
    {
        msgtext.AppendFmt("Player: '%s'",player.GetDataSafe());
        return msgtext;
    }
    if(command == "/guildlevel")
    {
        msgtext.AppendFmt("Level: %d LevelName: '%s'",level,levelname.GetDataSafe());
        return msgtext;
    }
    if(command == "/guildmembers")
    {
        msgtext.AppendFmt("Level: %d",level);
        return msgtext;
    }
    if(command == "/guildpromote")
    {
        msgtext.AppendFmt("Player: '%s' Level: %d",player.GetDataSafe(),level);
        return msgtext;
    }
    if(command == "/guildsecret")
    {
        msgtext.AppendFmt("Secret: '%s'",secret.GetDataSafe());
        return msgtext;
    }
    if(command == "/guildweb")
    {
        msgtext.AppendFmt("Web page: '%s'",web_page.GetDataSafe());
        return msgtext;
    }
    if(command == "/guildmotd")
    {
        msgtext.AppendFmt("Motd: '%s'",motd.GetDataSafe());
        return msgtext;
    }
    if(command == "/newalliance" || command == "/allianceremove" || command == "/allianceleader")
    {
        msgtext.AppendFmt("AlianceName: '%s'",alliancename.GetDataSafe());
        return msgtext;
    }
    if(command == "/endalliance" ||
            command == "/allianceleave" ||
            command == "/guildinfo" ||
            command == "/guildwar"    ||
            command == "/guildyield")
    {
        return msgtext;
    }

    msgtext.AppendFmt("ERROR: Not decoded.");

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIGuildMessage,MSGTYPE_GUIGUILD);

psGUIGuildMessage::psGUIGuildMessage(uint32_t command,
                                     csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(command)      +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUIGUILD);
    msg->clientnum  = 0;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIGuildMessage::psGUIGuildMessage(uint32_t clientNum,
                                     uint32_t command,
                                     csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(command)      +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUIGUILD);
    msg->clientnum  = clientNum;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIGuildMessage::psGUIGuildMessage(MsgEntry* message)
{
    if(!message)
        return;

    command   = message->GetUInt32();
    commandData = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUIGuildMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d Data: '%s'",
                      command,commandData.GetDataSafe());

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGroupCmdMessage,MSGTYPE_GROUPCMD);

psGroupCmdMessage::psGroupCmdMessage(const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_GROUPCMD);
    msg->clientnum      = 0;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGroupCmdMessage::psGroupCmdMessage(uint32_t clientnum,const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_GROUPCMD);
    msg->clientnum      = clientnum;
    msg->priority       = PRIORITY_HIGH;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psGroupCmdMessage::psGroupCmdMessage(MsgEntry* message)
{
    valid = true;

    WordArray words(message->GetStr());
    command = words[0];

    if(command == "/invite" || command == "/groupremove" || command == "/groupchallenge")
    {
        player = words[1];
        return;
    }
    if(command == "/disband" ||
            command == "/leavegroup" ||
            command == "/groupmembers" ||
            command == "/groupyield")
    {
        return;
    }

    valid = false;
}

csString psGroupCmdMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: '%s'", command.GetDataSafe());
    if(command == "/invite" || command == "/groupremove")
    {
        msgtext.AppendFmt("Player: '%s'", player.GetDataSafe());
        return msgtext;
    }
    if(command == "/disband" ||
            command == "/leavegroup" ||
            command == "/groupmembers")
    {
        return msgtext;
    }

    msgtext.AppendFmt("Error: Not Decoded");

    return msgtext;
}



// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psUserCmdMessage,MSGTYPE_USERCMD);

psUserCmdMessage::psUserCmdMessage(const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_USERCMD);
    msg->clientnum      = 0;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psUserCmdMessage::psUserCmdMessage(MsgEntry* message)
{
    valid = true;

    WordArray words(message->GetStr());

    command = words[0];

    if(command == "/who")
    {
        filter = words.GetTail(1);
        return;
    }
    if(command == "/buddy")
    {
        player = words[1]; //Holds the name of the player we are going to add/remove from the buddy list
        action = words[2]; //Holds if the player asked explictly to add or remove a buddy
        return;
    }
    if(command == "/pos")
    {
        player = words.GetTail(1);
        return;
    }
    if(command == "/admin")
    {
        if(words.GetCount() > 1)
            level = words.GetInt(1);
        else
            level = -1;
    }
    if(command == "/spawn" ||
            command == "/unstick" ||
            command == "/die" ||
            command == "/train" ||
            command == "/use" ||
            command == "/stopattack" ||
            command == "/starttrading" ||
            command == "/stoptrading" ||
            command == "/quests" ||
            command == "/tip" ||
            command == "/motd" ||
            command == "/challenge" ||
            command == "/yield" ||
            command == "/npcmenu" ||
            command == "/sit" ||
            command == "/stand" ||
            command == "/unmount")
    {
        return;
    }
    if(command == "/attack")
    {
        stance = words.Get(1);
        return;
    }
    if(command == "/queue")
    {
        attack = words.Get(1);
        for(size_t i = 2; i < words.GetCount();i++)
        {
            attack.Append(" ");
            attack.Append(words.Get(i));
        }
        return;
    }
    if (command == "/roll")
    {
        if(words.GetCount() == 1)
        {
            dice  = 1;
            sides = 6;
            dtarget = 0;
        }
        else if(words.GetCount() == 2)
        {
            dice  = 1;
            sides = words.GetInt(1);
            dtarget = 0;
        }
        else if(words.GetCount() == 3)
        {
            dice = words.GetInt(1);
            sides = words.GetInt(2);
            dtarget = 0;
        }
        else
        {
            dice = words.GetInt(1);
            sides = words.GetInt(2);
            dtarget = words.GetInt(3);
        }
        return;
    }
    if(command == "/assist")
    {
        player = words[1];
        return;
    }
    if(command == "/marriage")
    {
        action = words.Get(1);
        if(action == "propose")
        {
            player = words.Get(2);
            text = words.GetTail(3);
        }
        else if(action == "divorce")
        {
            text = words.GetTail(2);
        }
        return;
    }
    if(command == "/bank")
    {
        action = words.Get(1);
        return;
    }
    if(command == "/pickup")
    {
        target = words.Get(1);
        return;
    }
    if(command == "/guard")
    {
        target = words.Get(1);
        action = words.Get(2);
        return;
    }
    if(command == "/mount")
    {
        target = words.Get(1);
        return;
    }
    if(command == "/rotate")
    {
        target = words.Get(1);
        action = words.GetTail(2);
        return;
    }
    if(command == "/loot")
    {
        short i = 1;

        if(words.Get(i) == "roll")
        {
            dice = 1;
            i++;
        }
        else
            dice = 0;

        action = words.Get(i);
        if(action == "items")
        {
            i++;
            text = words.GetTail(i);
        }
        return;
    }

    valid = false;
}

csString psUserCmdMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: '%s'", command.GetDataSafe());
    if(command == "/who")
    {
        msgtext.AppendFmt("Filter: '%s'", filter.GetDataSafe());
        return msgtext;
    }
    if(command == "/buddy" || command == "/pos" || command == "/assist")
    {
        msgtext.AppendFmt("Player: '%s'", player.GetDataSafe());
        return msgtext;
    }
    if(command == "/attack")
    {
        msgtext.AppendFmt("Stance: '%s'", stance.GetDataSafe());
        return msgtext;
    }
    if(command == "/roll")
    {
        if(target)
            msgtext.AppendFmt("Rolled '%d' '%d' sided dice with a target of '%d'", dice, sides, dtarget);
        else
            msgtext.AppendFmt("Rolled '%d' '%d' sided dice", dice, sides);
        return msgtext;
    }
    if(command == "/marriage")
    {
        msgtext.AppendFmt("Action: %s ", action.GetData());
        if(player.Length())
            msgtext.AppendFmt("Player: %s ", player.GetData());
        msgtext.AppendFmt("Message: %s", text.GetData());
        return msgtext;
    }
    if(command == "/loot")
    {
        msgtext.AppendFmt("Action: %s ", action.GetData());
        if(action == "items")
        {
            if(text != "")
                msgtext.AppendFmt("Item Categories: %s ", text.GetData());
            else
                msgtext.AppendFmt("Item Categories: all ");
        }
        msgtext.AppendFmt("Loot Action: %s", (dice) ? "LOOT_ROLL" : "LOOT_SELF");
        return msgtext;
    }
    if(command == "/spawn" || command == "/unstick" ||
            command == "/die" ||  command == "/train" ||
            command == "/use" ||  command == "/stopattack" ||
            command == "/starttrading" ||
            command == "/stoptrading" || command == "/quests" ||
            command == "/tip" || command == "/motd" ||
            command == "/challenge" || command == "/yield" ||
            command == "/admin" ||
            command == "/list" ||
            command == "/sit" || command == "/stand" ||
            command == "/bank")
    {
        return msgtext;
    }

    msgtext.AppendFmt("Error: Not Decoded");

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psWorkCmdMessage,MSGTYPE_WORKCMD);

psWorkCmdMessage::psWorkCmdMessage(const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_WORKCMD);
    msg->clientnum      = 0;
    msg->priority       = PRIORITY_HIGH;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psWorkCmdMessage::psWorkCmdMessage(MsgEntry* message)
{
    valid = true;

    WordArray words(message->GetStr());

    command = words[0].Slice(1);

    if(command == "use" ||
            command == "combine" ||
            command == "construct")
    {
        return;
    }

    if(command == "repair")
    {
        repairSlotName = words[1];
        return;
    }

    //in case all the above fails conside this a message for handle production (natural resources)
    //this means this must be last. workmanager will check if the command is actually valid
    filter = words[1];
    if(filter == "for")
    {
        filter = words[2];
    }
}

csString psWorkCmdMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: '%s'",command.GetDataSafe());
    if(command == "/dig")
    {
        msgtext.AppendFmt(" Filter: '%s'",filter.GetDataSafe());
    }

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psAdminCmdMessage,MSGTYPE_ADMINCMD);

psAdminCmdMessage::psAdminCmdMessage(const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_ADMINCMD);
    msg->clientnum      = 0;
    msg->priority       = PRIORITY_HIGH;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psAdminCmdMessage::psAdminCmdMessage(const char* cmd, uint32_t client)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_ADMINCMD);
    msg->clientnum      = client;
    msg->priority       = PRIORITY_HIGH;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psAdminCmdMessage::psAdminCmdMessage(MsgEntry* message)
{
    valid = true;
    cmd = message->GetStr();
}

csString psAdminCmdMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Cmd: '%s'",cmd.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGenericCmdMessage,MSGTYPE_GENERICCMD);

psGenericCmdMessage::psGenericCmdMessage(const char* cmd)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_ADMINCMD);
    msg->clientnum      = 0;
    msg->priority       = PRIORITY_HIGH;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGenericCmdMessage::psGenericCmdMessage(const char* cmd, uint32_t client)
{
    msg.AttachNew(new MsgEntry(strlen(cmd) + 1));

    msg->SetType(MSGTYPE_GENERICCMD);
    msg->clientnum      = client;
    msg->priority       = PRIORITY_HIGH;

    msg->Add(cmd);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGenericCmdMessage::psGenericCmdMessage(MsgEntry* message)
{
    valid = true;
    cmd = message->GetStr();
}

csString psGenericCmdMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Cmd: '%s'",cmd.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psDisconnectMessage,MSGTYPE_DISCONNECT);

psDisconnectMessage::psDisconnectMessage(uint32_t clientnum, EID actorid, const char* reason)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + strlen(reason) + 1));

    msg->SetType(MSGTYPE_DISCONNECT);
    msg->clientnum      = clientnum;

    msg->Add(actorid.Unbox());
    msg->Add(reason);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psDisconnectMessage::psDisconnectMessage(MsgEntry* message)
{
    // Is this valid?  Do any psDisconnectMessage messages get created with no data?
    if(!message)
        return;

    actor     = message->GetUInt32();
    msgReason = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psDisconnectMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Actor: %u Reason: '%s'", actor.Unbox(), msgReason.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psUserActionMessage,MSGTYPE_USERACTION);

psUserActionMessage::psUserActionMessage(uint32_t clientnum, EID target, const char* action, const char* dfltBehaviors)
{
    msg.AttachNew(new MsgEntry(strlen(action) + 1 + strlen(dfltBehaviors) + 1 + sizeof(uint32_t)));

    msg->SetType(MSGTYPE_USERACTION);
    msg->clientnum      = clientnum;

    msg->Add(target.Unbox());
    msg->Add(action);
    msg->Add(dfltBehaviors);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psUserActionMessage::psUserActionMessage(MsgEntry* message)
{
    if(!message)
        return;

    target = EID(message->GetUInt32());
    action = message->GetStr();
    dfltBehaviors = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psUserActionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Target: %d Action: %s DfltBehaviors: %s",
                      target.Unbox(), action.GetDataSafe(), dfltBehaviors.GetDataSafe());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIInteractMessage,MSGTYPE_GUIINTERACT);

psGUIInteractMessage::psGUIInteractMessage(uint32_t client, uint32_t options, csString command)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + command.Length() + 1));

    msg->SetType(MSGTYPE_GUIINTERACT);
    msg->clientnum      = client;

    msg->Add(options);
    msg->Add(command);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psGUIInteractMessage::psGUIInteractMessage(MsgEntry* message)
{
    if(!message)
        return;

    options = message->GetUInt32();
    genericCommand = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUIInteractMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.Append("Options:");
    if(options & psGUIInteractMessage::PICKUP)
        msgtext.Append(" PICKUP");
    if(options & psGUIInteractMessage::EXAMINE)
        msgtext.Append(" EXAMINE");
    if(options & psGUIInteractMessage::UNLOCK)
        msgtext.Append(" UNLOCK");
    if(options & psGUIInteractMessage::LOCK)
        msgtext.Append(" LOCK");
    if(options & psGUIInteractMessage::LOOT)
        msgtext.Append(" LOOT");
    if(options & psGUIInteractMessage::BUYSELL)
        msgtext.Append(" BUYSELL");
    if(options & psGUIInteractMessage::GIVE)
        msgtext.Append(" GIVE");
    if(options & psGUIInteractMessage::CLOSE)
        msgtext.Append(" CLOSE");
    if(options & psGUIInteractMessage::USE)
        msgtext.Append(" USE");
    if(options & psGUIInteractMessage::PLAYERDESC)
        msgtext.Append(" PLAYERDESC");
    if(options & psGUIInteractMessage::ATTACK)
        msgtext.Append(" ATTACK");
    if(options & psGUIInteractMessage::COMBINE)
        msgtext.Append(" COMBINE");
    if(options & psGUIInteractMessage::CONSTRUCT)
        msgtext.Append(" CONSTRUCT");
    if(options & psGUIInteractMessage::EXCHANGE)
        msgtext.Append(" EXCHANGE");
    if(options & psGUIInteractMessage::BANK)
        msgtext.Append(" BANK");
    if(options & psGUIInteractMessage::TRAIN)
        msgtext.Append(" TRAIN");
    if(options & psGUIInteractMessage::NPCTALK)
        msgtext.Append(" NPCTALK");
    if(options & psGUIInteractMessage::VIEWSTATS)
        msgtext.Append(" VIEWSTATS");
    if(options & psGUIInteractMessage::DISMISS)
        msgtext.Append(" DISMISS");
    if(options & psGUIInteractMessage::MARRIAGE)
        msgtext.Append(" MARRIAGE");
    if(options & psGUIInteractMessage::DIVORCE)
        msgtext.Append(" DIVORCE");
    if(options & psGUIInteractMessage::ENTER)
        msgtext.Append(" ENTER");
    if(options & psGUIInteractMessage::ENTERLOCKED)
        msgtext.Append(" ENTERLOCKED");
    if(options & psGUIInteractMessage::MOUNT)
        msgtext.Append(" MOUNT");
    if(options & psGUIInteractMessage::UNMOUNT)
        msgtext.Append(" UNMOUNT");
    if(options & psGUIInteractMessage::STORAGE)
        msgtext.Append(" STORAGE");
    if(options & psGUIInteractMessage::GENERIC)
        msgtext.Append(" GENERIC");

    msgtext.AppendFmt(" Command: '%s'",genericCommand.GetDataSafe());

    return msgtext;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMapActionMessage,MSGTYPE_MAPACTION);

psMapActionMessage::psMapActionMessage(uint32_t clientnum, uint32_t cmd, const char* xml)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + strlen(xml) + 1));

    msg->SetType(MSGTYPE_MAPACTION);
    msg->clientnum      = clientnum;

    msg->Add(cmd);
    msg->Add(xml);

    // Sets valid flag based on message overrun state
    valid = !(msg->overrun);
}

psMapActionMessage::psMapActionMessage(MsgEntry* message)
{
    if(!message)
        return;

    command   = message->GetUInt32();
    actionXML = message->GetStr();

    // Sets valid flag based on message overrun state
    valid = !(message->overrun);
}

csString psMapActionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext,cmd;
    cmd = "UNKOWN";
    switch(command)
    {
        case QUERY:
            cmd = "QUERY";
            break;
        case NOT_HANDLED:
            cmd = "NOT_HANDLED";
            break;
        case SAVE:
            cmd = "SAVE";
            break;
        case LIST:
            cmd = "LIST";
            break;
        case LIST_QUERY:
            cmd = "LIST_QUERY";
            break;
        case DELETE_ACTION:
            cmd = "DELETE_ACTION";
            break;
        case RELOAD_CACHE:
            cmd = "RELOAD_CACHE";
            break;
    }


    msgtext.AppendFmt("Cmd: %s XML: '%s'",cmd.GetData(), actionXML.GetDataSafe());

    return msgtext;
}

//---------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psAttackQueueMessage, MSGTYPE_ATTACK_QUEUE);

psAttackQueueMessage::psAttackQueueMessage()
{
    msg.AttachNew(new MsgEntry());
    msg->clientnum = 0;
    msg->SetType(MSGTYPE_ATTACK_QUEUE);
    size = 0;
}
psAttackQueueMessage::psAttackQueueMessage(uint32_t client)
{
    this->client = client;
    size = 0;
}
psAttackQueueMessage::psAttackQueueMessage( MsgEntry* me, NetBase::AccessPointers* accessPointers )
{
    size_t length = me->GetUInt32();

    for ( size_t x = 0; x < length; x++ )
    {
        psAttackQueueMessage::AAttack na;

        na.Name = me->GetStr();
        na.Image =accessPointers->Request(csStringID(me->GetUInt32()));
        attacks.Push(na);
    }
}
void psAttackQueueMessage::AddAttack(const csString& name,  const csString& image)
{
    size += (uint32_t)(name.Length() + image.Length() +sizeof(uint32_t) +sizeof(uint32_t));
            
    psAttackQueueMessage::AAttack na;

    na.Name = name;
    na.Image = image;
    attacks.Push(na);
}
void psAttackQueueMessage::Construct(csStringSet* msgstrings)
{
    msg.AttachNew(new MsgEntry( size + sizeof(int) ));
    msg->SetType(MSGTYPE_ATTACK_QUEUE);
    msg->clientnum = client;

    msg->Add( (uint32_t)attacks.GetSize() );
    for ( size_t x = 0; x < attacks.GetSize(); x++ )
    {
        msg->Add( attacks[x].Name );
        msg->Add(msgstrings->Request(attacks[x].Image).GetHash());
    }
}

csString psAttackQueueMessage::ToString(NetBase::AccessPointers* accessPointers)
{
    csString msgtext;

    for ( size_t x = 0; x < attacks.GetSize(); x++ )
    {
        msgtext.AppendFmt("Attack: '%s' image: %s ",
            attacks[x].Name.GetDataSafe(),
            attacks[x].Image.GetDataSafe());

    }

    return msgtext;
}
//---------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY(psModeMessage,MSGTYPE_MODE);

psModeMessage::psModeMessage(uint32_t client, EID actorID, uint8_t mode, uint32_t value)
{
    msg.AttachNew(new MsgEntry(2*sizeof(uint32_t) + sizeof(uint8_t)));

    msg->SetType(MSGTYPE_MODE);
    msg->clientnum      = client;

    msg->Add(actorID.Unbox());
    msg->Add(mode);
    msg->Add(value);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psModeMessage::psModeMessage(MsgEntry* message)
{
    if(!message)
        return;

    actorID = EID(message->GetUInt32());
    mode    = message->GetUInt8();
    value   = message->GetUInt32();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psModeMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ActorID: %u Mode: %u Value: %u", actorID.Unbox(), mode, value);

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMoveLockMessage,MSGTYPE_MOVELOCK);

psMoveLockMessage::psMoveLockMessage(uint32_t client, bool lock)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)));

    msg->SetType(MSGTYPE_MOVELOCK);
    msg->clientnum      = client;

    msg->Add(lock);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psMoveLockMessage::psMoveLockMessage(MsgEntry* message)
{
    if(!message)
        return;

    locked = message->GetBool();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psMoveLockMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Locked: %s",locked?"true":"false");

    return msgtext;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psWeatherMessage,MSGTYPE_WEATHER);

psWeatherMessage::psWeatherMessage(uint32_t client, int minute, int hour, int day, int month, int year)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + 5*sizeof(uint8_t)));

    msg->SetType(MSGTYPE_WEATHER);
    msg->clientnum      = client;

    msg->Add((uint8_t)DAYNIGHT);
    msg->Add((uint8_t)minute);
    msg->Add((uint8_t)hour);
    msg->Add((uint8_t)day);
    msg->Add((uint8_t)month);
    msg->Add((uint32_t)year);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psWeatherMessage::psWeatherMessage(uint32_t client, psWeatherMessage::NetWeatherInfo info, uint /*clientnum*/)
{
    size_t size =
        sizeof(uint8_t) +        // type
        strlen(info.sector) + 1; // sector

    uint8_t type = (uint8_t)WEATHER;
    if(info.has_downfall)
    {
        if(info.downfall_is_snow)
        {
            type |= (uint8_t)SNOW;
        }
        else
        {
            type |= (uint8_t)RAIN;
        }
        size += sizeof(uint32_t) * 2;
    }
    if(info.has_fog)
    {
        type |= (uint8_t)FOG;
        size += sizeof(uint32_t) * 5;
    }
    if(info.has_lightning)
    {
        type |= (uint8_t)LIGHTNING;
    }

    msg.AttachNew(new MsgEntry(size));

    msg->SetType(MSGTYPE_WEATHER);
    msg->clientnum      = client;

    msg->Add(type);
    msg->Add(info.sector);
    if(info.has_downfall)
    {
        msg->Add((uint32_t)info.downfall_drops);
        msg->Add((uint32_t)info.downfall_fade);
    }
    if(info.has_fog)
    {
        msg->Add((uint32_t)info.fog_density);
        msg->Add((uint32_t)info.fog_fade);
        msg->Add((uint32_t)info.r);
        msg->Add((uint32_t)info.g);
        msg->Add((uint32_t)info.b);
    }

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psWeatherMessage::psWeatherMessage(MsgEntry* message)
{
    if(!message)
        return;

    type   = message->GetUInt8();
    if(type == DAYNIGHT)
    {
        minute = (int)message->GetUInt8();
        hour   = (int)message->GetUInt8();
        day    = (int)message->GetUInt8();
        month  = (int)message->GetUInt8();
        year   = (int)message->GetUInt32();
    }
    else
    {
        if(type & (uint8_t)SNOW)
        {
            weather.has_downfall = true;
            weather.downfall_is_snow = true;
        }
        else if(type & (uint8_t)RAIN)
        {
            weather.has_downfall = true;
            weather.downfall_is_snow = false;
        }
        else
        {
            weather.has_downfall = false;
            weather.downfall_is_snow = false;
        }
        if(type & (uint8_t)FOG)
        {
            weather.has_fog = true;
        }
        else
        {
            weather.has_fog = false;
        }
        if(type & (uint8_t)LIGHTNING)
        {
            weather.has_lightning = true;
        }
        else
        {
            weather.has_lightning = false;
        }

        type = WEATHER;

        weather.sector  = message->GetStr();
        if(weather.has_downfall)
        {
            weather.downfall_drops   = (int)message->GetUInt32();
            weather.downfall_fade    = (int)message->GetUInt32();
        }
        else
        {
            weather.downfall_drops = 0;
            weather.downfall_fade = 0;
        }
        if(weather.has_fog)
        {
            weather.fog_density = (int)message->GetUInt32();
            weather.fog_fade    = (int)message->GetUInt32();
            weather.r           = (int)message->GetUInt32();
            weather.g           = (int)message->GetUInt32();
            weather.b           = (int)message->GetUInt32();
        }
        else
        {
            weather.fog_density = 0;
            weather.fog_fade = 0;
            weather.r = weather.g = weather.b = 0;
        }

    }

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psWeatherMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    if(type == DAYNIGHT)
    {
        msgtext.AppendFmt("Type: DAYNIGHT Time: %d:%02d Date: %d-%d-%d",hour,minute,year,month,day);
    }
    else
    {
        msgtext.AppendFmt("Type: WEATHER( ");
        if(weather.has_downfall)
        {
            msgtext.AppendFmt("DOWNFALL");
        }
        if(weather.downfall_is_snow)
        {
            msgtext.AppendFmt(" SNOW");
        }
        if(weather.has_fog)
        {
            msgtext.AppendFmt(" FOG");
        }
        if(weather.has_lightning)
        {
            msgtext.AppendFmt(" LIGHTNING");
        }

        msgtext.AppendFmt(" ) Sector: '%s'",weather.sector.GetDataSafe());

        if(weather.has_downfall)
        {
            msgtext.AppendFmt(" DownfallDrops: %d DownfallFade: %d",
                              weather.downfall_drops,weather.downfall_fade);
        }
        if(weather.has_fog)
        {
            msgtext.AppendFmt(" FogDensity: %d FogFade: %d RGB: (%d,%d,%d)",
                              weather.fog_density,weather.fog_fade,
                              weather.r,weather.g,weather.b);
        }
    }

    return msgtext;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psGUIInventoryMessage,MSGTYPE_GUIINVENTORY);

psGUIInventoryMessage::psGUIInventoryMessage(uint8_t command, uint32_t size)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)  + size));

    msg->SetType(MSGTYPE_GUIINVENTORY);
    msg->clientnum      = 0;

    msg->Add(command);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}


psGUIInventoryMessage::psGUIInventoryMessage(MsgEntry* message, NetBase::AccessPointers* accessPointers)
{
    if(!message)
        return;

    command = message->GetUInt8();

    switch(command)
    {
        case LIST:
        case UPDATE_LIST:
        {
            totalItems = message->GetUInt32();
            if(command == UPDATE_LIST)
                totalEmptiedSlots = message->GetUInt32();
            maxWeight = message->GetFloat();
            version = message->GetUInt32();
            for(size_t x = 0; x < totalItems; x++)
            {
                ItemDescription item;
                item.name          = message->GetStr();
                item.meshName      = accessPointers->Request(csStringID(message->GetUInt32()));
                item.materialName  = accessPointers->Request(csStringID(message->GetUInt32()));
                item.container     = message->GetUInt32();
                item.slot          = message->GetUInt32();
                item.stackcount    = message->GetUInt32();
                item.weight        = message->GetFloat();
                item.size          = message->GetFloat();
                item.iconImage     = message->GetStr();
                item.purifyStatus  = message->GetUInt8();
                items.Push(item);
            }
            if(command == UPDATE_LIST)
            {
                for(size_t x = 0; x < totalEmptiedSlots; x++)
                {
                    ItemDescription emptied;
                    emptied.container = message->GetUInt32();
                    emptied.slot = message->GetUInt32();
                    items.Push(emptied);
                }
            }

            money = psMoney(message->GetStr());
            break;
        }
    }
    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}


psGUIInventoryMessage::psGUIInventoryMessage(uint32_t clientnum,
        uint8_t command,
        uint32_t totalItems,
        uint32_t totalEmptiedSlots,
        float maxWeight,
        uint32_t cache_version,
        size_t msgsize)
{
    // add on this header size
    msg.AttachNew(new MsgEntry(msgsize + sizeof(uint8_t) + sizeof(uint32_t) * 3 + sizeof(float)));
    msg->SetType(MSGTYPE_GUIINVENTORY);
    msg->clientnum      = clientnum;

    msg->Add(command);
    msg->Add(totalItems);
    if(command == UPDATE_LIST)
        msg->Add(totalEmptiedSlots);
    msg->Add(maxWeight);
    msg->Add(cache_version);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

void psGUIInventoryMessage::AddItem(const char* name,
                                    const char* meshName,
                                    const char* materialName,
                                    int containerID,
                                    int slotID,
                                    int stackcount,
                                    float weight,
                                    float size,
                                    const char* icon,
                                    int purifyStatus,
                                    csStringSet* msgstrings)
{
    msg->Add(name);
    msg->Add(msgstrings->Request(meshName).GetHash());
    msg->Add(msgstrings->Request(materialName).GetHash());
    msg->Add((uint32_t)containerID);
    msg->Add((uint32_t)slotID);

    msg->Add((uint32_t)stackcount);
    msg->Add(weight);
    msg->Add(size);

    msg->Add(icon);

    msg->Add((uint8_t)purifyStatus);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

void psGUIInventoryMessage::AddMoney(const psMoney &money)
{
    msg->Add(money.ToString());

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

void psGUIInventoryMessage::AddEmptySlot(int containerID, int slotID)
{
    msg->Add((uint32_t)containerID);
    msg->Add((uint32_t)slotID);
}

csString psGUIInventoryMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d", command);
    if(command == LIST || command == UPDATE_LIST)
    {
        msgtext.AppendFmt(" Total Items: %zu Max Weight: %.3f Cache Version: %d", totalItems, maxWeight, version);

        msgtext.AppendFmt(" List: ");
        for(size_t x = 0; x < totalItems; x++)
        {
            msgtext.AppendFmt("%d x '%s' ", items[x].stackcount, items[x].name.GetDataSafe());

#ifdef FULL_DEBUG_DUMP
            msgtext.AppendFmt("(Container: %d Slot: %d Weight: %.3f Size: %.3f Icon: '%s' Purified: %d), ",
                              items[x].container,
                              items[x].slot,
                              items[x].weight,
                              items[x].size,
                              items[x].iconImage.GetDataSafe(),
                              items[x].purifyStatus);
#endif
        }
        if(command == UPDATE_LIST)
        {
            msgtext.AppendFmt(" Total Emptied Slots: %zu", totalEmptiedSlots);

#ifdef FULL_DEBUG_DUMP
            for(size_t x = 0; x < totalEmptiedSlots; x++)
            {
                msgtext.AppendFmt(" (Container: %d Slot: %d),",
                                  items[totalItems+x].container, items[totalItems+x].slot);
            }
#endif
        }
        msgtext.AppendFmt(" Money: %s", money.ToUserString().GetDataSafe());
    }

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNewSectorMessage,MSGTYPE_NEWSECTOR);

// Leaving this one marshalled the old way.  This message is NEVER sent on
// the network.
psNewSectorMessage::psNewSectorMessage(const csString &oldSector, const csString &newSector, csVector3 pos)
{
    msg.AttachNew(new MsgEntry(1024));

    msg->SetType(MSGTYPE_NEWSECTOR);
    msg->clientnum      = 0; // msg never sent on network. client only

    msg->Add(oldSector);
    msg->Add(newSector);
    msg->Add(pos.x);
    msg->Add(pos.y);
    msg->Add(pos.z);

    // Since this message is never sent, we don't adjust the valid flag
}


psNewSectorMessage::psNewSectorMessage(MsgEntry* message)
{
    if(!message)
        return;

    oldSector = message->GetStr();
    newSector = message->GetStr();
    pos.x = message->GetFloat();
    pos.y = message->GetFloat();
    pos.z = message->GetFloat();

    // Since this message is never sent, we don't adjust the valid flag
}

csString psNewSectorMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Old sector: '%s' New sector: '%s' Pos: (%.3f,%.3f,%.3f)",
                      oldSector.GetDataSafe(), newSector.GetDataSafe(), pos.x, pos.y, pos.z);

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psLootItemMessage,MSGTYPE_LOOTITEM);

psLootItemMessage::psLootItemMessage(int client, EID entity, int item, int action)
{
    msg.AttachNew(new MsgEntry(2*sizeof(int32_t) + sizeof(uint8_t)));

    msg->SetType(MSGTYPE_LOOTITEM);
    msg->clientnum      = client;

    msg->Add(entity.Unbox());
    msg->Add((int32_t) item);
    msg->Add((uint8_t) action);
    valid=!(msg->overrun);
}

psLootItemMessage::psLootItemMessage(MsgEntry* message)
{
    entity   = EID(message->GetUInt32());
    lootitem = message->GetInt32();
    lootaction = message->GetUInt8();
    valid=!(message->overrun);
}

csString psLootItemMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Entity: %d Item: %d Action: %d", entity.Unbox(), lootitem, lootaction);

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psLootMessage,MSGTYPE_LOOT);

psLootMessage::psLootMessage()
{
    msg = NULL;
}

psLootMessage::psLootMessage(MsgEntry* msg)
{
    entity_id = EID(msg->GetUInt32());
    lootxml = msg->GetStr();
    valid=!(msg->overrun);
}

void psLootMessage::Populate(EID entity, csString &lootstr, int cnum)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + lootstr.Length() + 1));

    msg->SetType(MSGTYPE_LOOT);
    msg->clientnum      = cnum;

    msg->Add(entity.Unbox());
    msg->Add(lootstr);
    valid=!(msg->overrun);
}

csString psLootMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Entity: %u ", entity_id.Unbox());
#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt("XML: '%s'", lootxml.GetDataSafe());
#endif

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psQuestListMessage,MSGTYPE_QUESTLIST);

psQuestListMessage::psQuestListMessage()
{
    msg = NULL;
    valid=false;
}

psQuestListMessage::psQuestListMessage(MsgEntry* msg)
{
    questxml = msg->GetStr();
    valid=!(msg->overrun);
}

void psQuestListMessage::Populate(csString &queststr, int cnum)
{
    msg.AttachNew(new MsgEntry(sizeof(int) + queststr.Length() + 1));

    msg->SetType(MSGTYPE_QUESTLIST);
    msg->clientnum      = cnum;

    msg->Add(queststr);
    valid=!(msg->overrun);
}

csString psQuestListMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("XML: '%s'", questxml.GetDataSafe());

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psQuestInfoMessage,MSGTYPE_QUESTINFO);

psQuestInfoMessage::psQuestInfoMessage(int cnum, int cmd, int id, const char* name, const char* info)
{
    if(name)
    {
        csString escpxml_info = EscpXML(info);
        xml.Format("<QuestNotebook><Description text=\"%s\"/></QuestNotebook>",escpxml_info.GetData());
    }
    else
        xml.Clear();

    msg.AttachNew(new MsgEntry(sizeof(int32_t) + sizeof(uint8_t) + xml.Length() + 1));

    msg->SetType(MSGTYPE_QUESTINFO);
    msg->clientnum = cnum;

    msg->Add((uint8_t) cmd);

    if(cmd == CMD_QUERY || cmd == CMD_DISCARD)
    {
        msg->Add((int32_t) id);
    }
    else if(cmd == CMD_INFO)
    {
        msg->Add(xml);
    }
    msg->ClipToCurrentSize();

    valid=!(msg->overrun);
}

psQuestInfoMessage::psQuestInfoMessage(MsgEntry* msg)
{
    command = msg->GetUInt8();
    if(command == CMD_QUERY || command == CMD_DISCARD)
        id = msg->GetInt32();
    else if(command == CMD_INFO)
        xml = msg->GetStr();
    valid=!(msg->overrun);
}


csString psQuestInfoMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d", command);
    if(command == CMD_QUERY || command == CMD_DISCARD)
        msgtext.AppendFmt(" Id: %d", id);
    else if(command == CMD_INFO)
        msgtext.AppendFmt(" XML: '%s'", xml.GetDataSafe());

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psOverrideActionMessage,MSGTYPE_OVERRIDEACTION);

psOverrideActionMessage::psOverrideActionMessage(int client, EID entity, const char* action, int duration)
{
    size_t strlength = 0;
    if(action)
        strlength = strlen(action) + 1;

    msg.AttachNew(new MsgEntry(sizeof(entity) +
                               strlength +
                               sizeof(duration)));

    msg->SetType(MSGTYPE_OVERRIDEACTION);
    msg->clientnum  = client;

    msg->Add(entity.Unbox());
    msg->Add(action);
    msg->Add((uint32_t)duration);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psOverrideActionMessage::psOverrideActionMessage(MsgEntry* message)
{
    if(!message)
        return;

    entity_id = EID(message->GetUInt32());
    action    = message->GetStr();
    duration  = message->GetUInt32();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psOverrideActionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Entity: %d Action: '%s' Duration: %d", entity_id.Unbox(), action.GetDataSafe(), duration);

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psEquipmentMessage,MSGTYPE_EQUIPMENT);

psEquipmentMessage::psEquipmentMessage(uint32_t clientNum,
                                       EID actorid,
                                       uint8_t type,
                                       int slot,
                                       csString &meshName,
                                       csString &part,
                                       csString &texture,
                                       csString &partMesh,
                                       csString &removedMesh)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)*2 +
                               sizeof(uint8_t) +
                               meshName.Length()+1 +
                               part.Length()+1 +
                               texture.Length()+1 +
                               partMesh.Length()+1 +
                               removedMesh.Length()+1));

    msg->SetType(MSGTYPE_EQUIPMENT);
    msg->clientnum  = clientNum;

    msg->Add(type);
    msg->Add(actorid.Unbox());
    msg->Add((uint32_t)slot);
    msg->Add(meshName);
    msg->Add(part);
    msg->Add(texture);
    msg->Add(partMesh);
    msg->Add(removedMesh);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psEquipmentMessage::psEquipmentMessage(MsgEntry* message)
{
    if(!message)
        return;

    type        = message->GetUInt8();
    player      = message->GetUInt32();
    slot        = message->GetUInt32();
    mesh        = message->GetStr();
    part        = message->GetStr();
    texture     = message->GetStr();
    partMesh    = message->GetStr();
    removedMesh = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psEquipmentMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Type: %d",type);
    msgtext.AppendFmt(" Player: %d",player);
    msgtext.AppendFmt(" Slot: %d",slot);
    msgtext.AppendFmt(" Mesh: '%s'",mesh.GetDataSafe());
    msgtext.AppendFmt(" Part: '%s'",part.GetDataSafe());
    msgtext.AppendFmt(" Texture: '%s'",texture.GetDataSafe());
    msgtext.AppendFmt(" PartMesh: '%s'",partMesh.GetDataSafe());
    msgtext.AppendFmt(" RemovedMesh: '%s'",removedMesh.GetDataSafe());

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIMerchantMessage,MSGTYPE_GUIMERCHANT);

psGUIMerchantMessage::psGUIMerchantMessage(uint8_t command,
        csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUIMERCHANT);
    msg->clientnum  = 0;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIMerchantMessage::psGUIMerchantMessage(uint32_t clientNum,
        uint8_t command,
        csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUIMERCHANT);
    msg->clientnum  = clientNum;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIMerchantMessage::psGUIMerchantMessage(MsgEntry* message)
{
    if(!message)
        return;

    command   = message->GetUInt8();
    commandData = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUIMerchantMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;


    msgtext.AppendFmt("Command: %d", command);
#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt(" Data: '%s'", commandData.GetDataSafe());
#endif

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIStorageMessage,MSGTYPE_GUIMERCHANT);

psGUIStorageMessage::psGUIStorageMessage(uint8_t command,
        csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUISTORAGE);
    msg->clientnum  = 0;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIStorageMessage::psGUIStorageMessage(uint32_t clientNum,
        uint8_t command,
        csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUISTORAGE);
    msg->clientnum  = clientNum;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIStorageMessage::psGUIStorageMessage(MsgEntry* message)
{
    if(!message)
        return;

    command   = message->GetUInt8();
    commandData = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUIStorageMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;


    msgtext.AppendFmt("Command: %d", command);
#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt(" Data: '%s'", commandData.GetDataSafe());
#endif

    return msgtext;
}


//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIGroupMessage,MSGTYPE_GUIGROUP);

psGUIGroupMessage::psGUIGroupMessage(uint8_t command,
                                     csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUIGROUP);
    msg->clientnum  = 0;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIGroupMessage::psGUIGroupMessage(uint32_t clientNum,
                                     uint8_t command,
                                     csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_GUIGROUP);
    msg->clientnum  = clientNum;

    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIGroupMessage::psGUIGroupMessage(MsgEntry* message)
{
    if(!message)
        return;

    command   = message->GetUInt8();
    commandData = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUIGroupMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d", command);
#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt(" Data: '%s'", commandData.GetDataSafe());
#endif

    return msgtext;
}

//----------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY(psCraftCancelMessage,MSGTYPE_CRAFT_CANCEL);

void psCraftCancelMessage::SetCraftTime(int craftTime, uint32_t client)
{
    this->craftTime = craftTime;
    msg.AttachNew(new MsgEntry(sizeof(int)));
    msg->SetType(MSGTYPE_CRAFT_CANCEL);
    msg->clientnum = client;
    msg->Add(craftTime);
}

csString psCraftCancelMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Craft cancel");

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSpellCancelMessage,MSGTYPE_SPELL_CANCEL);

csString psSpellCancelMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Spell cancel");

    return msgtext;
}

//--------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psAttackBookMessage,MSGTYPE_ATTACK_BOOK);

psAttackBookMessage::psAttackBookMessage()
{
    msg.AttachNew(new MsgEntry());
    msg->clientnum = 0;
    msg->SetType(MSGTYPE_ATTACK_BOOK);
    client = 0;
    size = 0;
}

psAttackBookMessage::psAttackBookMessage( uint32_t client )
{
    this->client = client;
    size = 0;
}

psAttackBookMessage::psAttackBookMessage( MsgEntry* me, NetBase::AccessPointers* accessPointers )
{
    size_t length = me->GetUInt32();

    for ( size_t x = 0; x < length; x++ )
    {
        psAttackBookMessage::NetworkAttack na;

        na.name = me->GetStr();
        na.description = me->GetStr();
        na.type = me->GetStr();
        na.image = accessPointers->Request(csStringID(me->GetUInt32()));
        printf ("DEBUG: psAttackBookMessage::psAttackBookMessage name: %s image: %s\n",na.name.GetData(),na.image.GetData());
        attacks.Push(na);
    }
}

void psAttackBookMessage::AddAttack(const csString& name, const csString& description, const csString& type, const csString& image)
{
    size +=(uint32_t)(name.Length() + description.Length() + type.Length() + 3 + /*+ image.Length()*/ + sizeof(uint32_t));
            
    psAttackBookMessage::NetworkAttack na;

    na.name = name;
    na.description = description;
    na.type = type;
    na.image = image;
    attacks.Push(na);


}


void psAttackBookMessage::Construct(csStringSet* msgstrings)
{
    msg.AttachNew(new MsgEntry( size + sizeof(int) ));
    msg->SetType(MSGTYPE_ATTACK_BOOK);
    msg->clientnum = client;

    msg->Add( (uint32_t)attacks.GetSize() );
    for ( size_t x = 0; x < attacks.GetSize(); x++ )
    {
        msg->Add( attacks[x].name );
        msg->Add( attacks[x].description );
        msg->Add( attacks[x].type );

        msg->Add(msgstrings->Request(attacks[x].image).GetHash());
        printf ("DEBUG: psAttackBookMessage::Construct %s %u %s\n",attacks[x].name.GetData(),msgstrings->Request(attacks[x].image).GetHash(),attacks[x].image.GetData());
    }
}



csString psAttackBookMessage::ToString(NetBase::AccessPointers * /*accessPointers*/)
{
    csString msgtext;

    for ( size_t x = 0; x < attacks.GetSize(); x++ )
    {
        msgtext.AppendFmt("AttackList Attack: '%s' Type: '%s' image: %s",
            attacks[x].name.GetDataSafe(),
            attacks[x].type.GetDataSafe(),
            attacks[x].image.GetDataSafe());

#ifdef FULL_DEBUG_DUMP
        msgtext.AppendFmt("Description: '%s'",
            attacks[x].description.GetDataSafe());
#endif
    }

    return msgtext;
}

//--------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psSpellBookMessage,MSGTYPE_SPELL_BOOK);

psSpellBookMessage::psSpellBookMessage()
{
    msg.AttachNew(new MsgEntry());
    msg->clientnum = 0;
    msg->SetType(MSGTYPE_SPELL_BOOK);
    size = 0;
}

psSpellBookMessage::psSpellBookMessage(uint32_t client)
{
    this->client = client;
    size = 0;
}

psSpellBookMessage::psSpellBookMessage(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    size_t length = me->GetUInt32();

    for(size_t x = 0; x < length; x++)
    {
        psSpellBookMessage::NetworkSpell ns;
        ns.name = me->GetStr();
        ns.description = me->GetStr();
        ns.way = me->GetStr();
        ns.realm = me->GetUInt32();
        ns.glyphs[0] = me->GetStr();
        ns.glyphs[1] = me->GetStr();
        ns.glyphs[2] = me->GetStr();
        ns.glyphs[3] = me->GetStr();
        ns.image = accessPointers->Request(csStringID(me->GetUInt32()));
        spells.Push(ns);
    }

}

void psSpellBookMessage::AddSpell(const csString &name, const csString &description, const csString &way, int realm, const csString &glyph0, const csString &glyph1, const csString &glyph2, const csString &glyph3, const csString &image)
{
    size+=(uint32_t)(name.Length() + description.Length() + way.Length()+ sizeof(int)+ 8 +
                     glyph0.Length()+glyph1.Length()+glyph2.Length()+glyph3.Length() + sizeof(uint32_t));

    psSpellBookMessage::NetworkSpell ns;
    ns.name = name;
    ns.description = description;
    ns.way = way;
    ns.realm = realm;
    ns.glyphs[0] = glyph0;
    ns.glyphs[1] = glyph1;
    ns.glyphs[2] = glyph2;
    ns.glyphs[3] = glyph3;
    ns.image = image;

    spells.Push(ns);
}

void psSpellBookMessage::Construct(csStringSet* msgstrings)
{
    msg.AttachNew(new MsgEntry(size + sizeof(int)));
    msg->SetType(MSGTYPE_SPELL_BOOK);
    msg->clientnum = client;

    msg->Add((uint32_t)spells.GetSize());
    for(size_t x = 0; x < spells.GetSize(); x++)
    {
        msg->Add(spells[x].name);
        msg->Add(spells[x].description);
        msg->Add(spells[x].way);
        msg->Add((uint32_t)spells[x].realm);
        msg->Add(spells[x].glyphs[0]);
        msg->Add(spells[x].glyphs[1]);
        msg->Add(spells[x].glyphs[2]);
        msg->Add(spells[x].glyphs[3]);
        msg->Add(msgstrings->Request(spells[x].image).GetHash());
    }
}



csString psSpellBookMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    for(size_t x = 0; x < spells.GetSize(); x++)
    {
        msgtext.AppendFmt("Spell: '%s' Way: '%s' Realm: %d image: %s",
                          spells[x].name.GetDataSafe(),
                          spells[x].way.GetDataSafe(),
                          spells[x].realm,
                          spells[x].image.GetDataSafe());
        msgtext.AppendFmt("Glyphs: '%s', '%s', '%s', '%s' ",
                          spells[x].glyphs[0].GetDataSafe(),
                          spells[x].glyphs[1].GetDataSafe(),
                          spells[x].glyphs[2].GetDataSafe(),
                          spells[x].glyphs[3].GetDataSafe());
#ifdef FULL_DEBUG_DUMP
        msgtext.AppendFmt("Description: '%s'",
                          spells[x].description.GetDataSafe());
#endif
    }

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPurifyGlyphMessage,MSGTYPE_PURIFY_GLYPH);

psPurifyGlyphMessage::psPurifyGlyphMessage(uint32_t glyphID)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)));
    msg->clientnum = 0;
    msg->SetType(MSGTYPE_PURIFY_GLYPH);
    msg->Add(glyphID);
}

psPurifyGlyphMessage::psPurifyGlyphMessage(MsgEntry* me)
{
    glyph = me->GetUInt32();
}

csString psPurifyGlyphMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Glyph: %d", glyph);

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSpellCastMessage,MSGTYPE_SPELL_CAST);

psSpellCastMessage::psSpellCastMessage(csString &spellName, float kFactor)
{
    msg.AttachNew(new MsgEntry(spellName.Length() + 1 + sizeof(float)));
    msg->clientnum = 0;
    msg->SetType(MSGTYPE_SPELL_CAST);
    msg->Add(spellName);
    msg->Add(kFactor);
}

psSpellCastMessage::psSpellCastMessage(MsgEntry* me)
{
    spell = me->GetStr();
    kFactor = me->GetFloat();
}

csString psSpellCastMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Spell: '%s' kFactor: %.3f", spell.GetDataSafe(), kFactor);

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGlyphAssembleMessage,MSGTYPE_GLYPH_ASSEMBLE);

psGlyphAssembleMessage::psGlyphAssembleMessage(int slot0, int slot1, int slot2, int slot3, bool info)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) * 4 + sizeof(uint8_t)));
    msg->SetType(MSGTYPE_GLYPH_ASSEMBLE);
    msg->clientnum = 0;
    msg->Add((uint32_t)slot0);
    msg->Add((uint32_t)slot1);
    msg->Add((uint32_t)slot2);
    msg->Add((uint32_t)slot3);
    msg->Add((uint8_t)info);
}

psGlyphAssembleMessage::psGlyphAssembleMessage(uint32_t client,
        csString name,
        csString image,
        csString description)
{
    msg.AttachNew(new MsgEntry(name.Length() + description.Length() + image.Length() + 3));
    msg->SetType(MSGTYPE_GLYPH_ASSEMBLE);
    msg->clientnum = client;
    msg->Add(name);
    msg->Add(image);
    msg->Add(description);

}

psGlyphAssembleMessage::psGlyphAssembleMessage(MsgEntry* me)
{
    if(me->GetSize() == sizeof(uint32_t) * 4 + sizeof(uint8_t))
    {
        FromClient(me);
        msgFromServer = false;
    }
    else
    {
        FromServer(me);
        msgFromServer = true;
    }
}

void psGlyphAssembleMessage::FromClient(MsgEntry* me)
{
    glyphs[0] = me->GetUInt32();
    glyphs[1] = me->GetUInt32();
    glyphs[2] = me->GetUInt32();
    glyphs[3] = me->GetUInt32();
    info = me->GetBool();
}

void psGlyphAssembleMessage::FromServer(MsgEntry* me)
{
    name = me->GetStr();
    image = me->GetStr();
    description = me->GetStr();
}

csString psGlyphAssembleMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    if(msgFromServer)
    {
        msgtext.AppendFmt("Name: '%s' Image: '%s' Description: '%s'",
                          name.GetDataSafe(), image.GetDataSafe(), description.GetDataSafe());
    }
    else
    {
        msgtext.AppendFmt("Glyphs: %d, %d, %d, %d", glyphs[0], glyphs[1], glyphs[2], glyphs[3]);
    }

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psRequestGlyphsMessage,MSGTYPE_GLYPH_REQUEST);

psRequestGlyphsMessage::psRequestGlyphsMessage(uint32_t client)
{
    this->client = client;
    size = 0;
    if(client == 0)
    {
        msg.AttachNew(new MsgEntry());
        msg->SetType(MSGTYPE_GLYPH_REQUEST);
        msg->clientnum = client;
    }
}

psRequestGlyphsMessage::~psRequestGlyphsMessage()
{
}

void psRequestGlyphsMessage::AddGlyph(csString name, csString image, int purifiedStatus,
                                      int way, int statID)
{
    size+=name.Length() + image.Length() + sizeof(int)*4+2;

    psRequestGlyphsMessage::NetworkGlyph ng;
    ng.name = name;
    ng.image = image;
    ng.purifiedStatus = purifiedStatus;
    ng.way = way;
    ng.statID = statID;

    glyphs.Push(ng);
}


void psRequestGlyphsMessage::Construct()
{
    msg.AttachNew(new MsgEntry(size + sizeof(int)));
    msg->SetType(MSGTYPE_GLYPH_REQUEST);
    msg->clientnum = client;

    msg->Add((uint32_t)glyphs.GetSize());
    for(size_t x = 0; x < glyphs.GetSize(); x++)
    {
        msg->Add(glyphs[x].name);
        msg->Add(glyphs[x].image);
        msg->Add(glyphs[x].purifiedStatus);
        msg->Add(glyphs[x].way);
        msg->Add(glyphs[x].statID);
    }
}

psRequestGlyphsMessage::psRequestGlyphsMessage(MsgEntry* me)
{
    size_t length = me->GetUInt32();

    for(size_t x = 0; x < length; x++)
    {
        psRequestGlyphsMessage::NetworkGlyph ng;
        ng.name = me->GetStr();
        ng.image = me->GetStr();
        ng.purifiedStatus = me->GetUInt32();
        ng.way = me->GetUInt32();
        ng.statID = me->GetUInt32();

        glyphs.Push(ng);
    }
}

csString psRequestGlyphsMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    for(size_t x = 0; x < glyphs.GetSize(); x++)
    {
        msgtext.AppendFmt("Name: '%s' Image: '%s' Purified Status: %d Way: %d Stat Id: %d; ",
                          glyphs[x].name.GetDataSafe(),
                          glyphs[x].image.GetDataSafe(),
                          glyphs[x].purifiedStatus,
                          glyphs[x].way,
                          glyphs[x].statID);
    }

    return msgtext;
}

//--------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY(psStopEffectMessage,MSGTYPE_EFFECT_STOP);

PSF_IMPLEMENT_MSG_FACTORY(psEffectMessage,MSGTYPE_EFFECT);

psEffectMessage::psEffectMessage(uint32_t clientNum, const csString &effectName,
                                 const csVector3 &effectOffset, EID anchorID,
                                 EID targetID, uint32_t uid, float scale1, float scale2, float scale3, float scale4)
{
    msg.AttachNew(new MsgEntry(effectName.Length() + 1 + sizeof(csVector3)
                               + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)
                               + (scale4!=0.0?4*sizeof(float):(scale3!=0.0?3*sizeof(float):(scale2!=0.0?2*sizeof(float):(scale1!=0.0?sizeof(float):0))))));


    msg->SetType(MSGTYPE_EFFECT);
    msg->clientnum = clientNum;

    msg->Add(effectName);
    msg->Add(effectOffset.x);
    msg->Add(effectOffset.y);
    msg->Add(effectOffset.z);
    msg->Add(anchorID.Unbox());
    msg->Add(targetID.Unbox());
    msg->Add((uint32_t)0);
    msg->Add((uint32_t)uid);
    if(scale1 != 0.0 || scale2 != 0.0 || scale3 != 0.0 || scale4 != 0.0)
    {
        msg->Add(scale1);
    }
    if(scale2 != 0.0 || scale3 != 0.0 || scale4 != 0.0)
    {
        msg->Add(scale2);
    }
    if(scale3 != 0.0 || scale4 != 0.0)
    {
        msg->Add(scale3);
    }
    if(scale4 != 0.0)
    {
        msg->Add(scale4);
    }
    valid = !(msg->overrun);
}

psEffectMessage::psEffectMessage(uint32_t clientNum, const csString &effectName,
                                 const csVector3 &effectOffset, EID anchorID,
                                 EID targetID, uint32_t duration, uint32_t uid,
                                 float scale1, float scale2, float scale3, float scale4)
{
    msg.AttachNew(new MsgEntry(effectName.Length() + 1 + sizeof(csVector3)
                               + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)
                               + (scale4!=0.0?4*sizeof(float):(scale3!=0.0?3*sizeof(float):(scale2!=0.0?2*sizeof(float):(scale1!=0.0?sizeof(float):0))))));

    msg->SetType(MSGTYPE_EFFECT);
    msg->clientnum = clientNum;

    msg->Add(effectName);
    msg->Add(effectOffset.x);
    msg->Add(effectOffset.y);
    msg->Add(effectOffset.z);
    msg->Add(anchorID.Unbox());
    msg->Add(targetID.Unbox());
    msg->Add(duration);
    msg->Add(uid);
    if(scale1 != 0.0 || scale2 != 0.0 || scale3 != 0.0 || scale4 != 0.0)
    {
        msg->Add(scale1);
    }
    if(scale2 != 0.0 || scale3 != 0.0 || scale4 != 0.0)
    {
        msg->Add(scale2);
    }
    if(scale3 != 0.0 || scale4 != 0.0)
    {
        msg->Add(scale3);
    }
    if(scale4 != 0.0)
    {
        msg->Add(scale4);
    }

    valid = !(msg->overrun);
}

psEffectMessage::psEffectMessage(MsgEntry* message)
{
    if(!message)
        return;

    name = message->GetStr();
    offset.x = message->GetFloat();
    offset.y = message->GetFloat();
    offset.z = message->GetFloat();
    anchorID = EID(message->GetUInt32());
    targetID = EID(message->GetUInt32());
    duration = message->GetUInt32();
    uid = message->GetUInt32();
    if(message->HasMore(sizeof(float)))
    {
        scale[0] = message->GetFloat();
    }
    else
    {
        scale[0] = 0.0;
    }
    if(message->HasMore(sizeof(float)))
    {
        scale[1] = message->GetFloat();
    }
    else
    {
        scale[1] = 0.0;
    }
    if(message->HasMore(sizeof(float)))
    {
        scale[2] = message->GetFloat();
    }
    else
    {
        scale[2] = 0.0;
    }
    if(message->HasMore(sizeof(float)))
    {
        scale[3] = message->GetFloat();
    }
    else
    {
        scale[3] = 0.0;
    }
    valid = !(message->overrun);
}

csString psEffectMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Name: '%s' Offset: (%.3f, %.3f, %.3f) Anchor: %d Target: %d Duration: %d UID: %d Scale1: %.3f Scale2: %.3f Scale3: %.3f Scale4: %.3f",
                      name.GetDataSafe(),
                      offset.x, offset.y, offset.z,
                      anchorID.Unbox(),
                      targetID.Unbox(),
                      duration,
                      uid, scale[0],scale[1],scale[2],scale[3]);

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUITargetUpdateMessage,MSGTYPE_GUITARGETUPDATE);

psGUITargetUpdateMessage::psGUITargetUpdateMessage(uint32_t clientNum,
        EID targetID)
{
    msg.AttachNew(new MsgEntry(2*sizeof(uint32_t)));

    msg->SetType(MSGTYPE_GUITARGETUPDATE);
    msg->clientnum      = clientNum;

    msg->Add(targetID.Unbox());

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUITargetUpdateMessage::psGUITargetUpdateMessage(MsgEntry* message)
{
    if(!message)
        return;

    targetID = message->GetUInt32();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUITargetUpdateMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Target ID: %d", targetID.Unbox());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMsgStringsMessage,MSGTYPE_MSGSTRINGS);

#define PACKING_BUFFSIZE  (MAX_MESSAGE_SIZE/2)*3
psMsgStringsMessage::psMsgStringsMessage()
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)));
    msg->SetType(MSGTYPE_MSGSTRINGS);
    msg->clientnum = 0;
}

psMsgStringsMessage::psMsgStringsMessage(uint32_t clientnum, csMD5::Digest &digest)
{
    msg.AttachNew(new MsgEntry(2*sizeof(uint32_t) + sizeof(csMD5::Digest) + sizeof(bool)));
    msg->SetType(MSGTYPE_MSGSTRINGS);
    msg->clientnum = clientnum;

    msg->Add(&digest, sizeof(csMD5::Digest));
    msg->Add(true);
}

psMsgStringsMessage::psMsgStringsMessage(uint32_t clientnum, csMD5::Digest &digest, char* stringsdata,
        unsigned long size, uint32_t num_strings)
{
    msg.AttachNew(new MsgEntry(4*sizeof(uint32_t) + sizeof(csMD5::Digest) +
                               sizeof(bool) + sizeof(char) * size));
    msg->SetType(MSGTYPE_MSGSTRINGS);
    msg->clientnum = clientnum;

    msg->Add(&digest, sizeof(csMD5::Digest));
    if(num_strings > 0)
    {
        msg->Add(false);
        msg->Add(num_strings);
        msg->Add(stringsdata, size);
    }
    else
    {
        msg->Add(true);
    }
    // Sets valid flag based on message overrun state
    valid = !(msg->overrun);
}

psMsgStringsMessage::psMsgStringsMessage(MsgEntry* message)
    :msgstrings(NULL), nstrings(0)
{
    if(!message)
        return;

    uint32_t length = 0;
    digest = (csMD5::Digest*)message->GetBufferPointerUnsafe(length);

    only_carrying_digest = message->GetBool();
    if(only_carrying_digest)
        return;

    nstrings = message->GetUInt32();

    /* Somewhat arbitrary max size - currently the maximum message size of 2048
     *  So the maximum number of strings that could be in a message is 2048-sizeof(lengthtype)
     *  We cannot allow this to be any value, a malicious client could sent maxuint
     *  and cause the new call to fail below, or just an obscene value and cause memory
     *  to thrash.
     */
    if(nstrings<1 || nstrings>2044)
    {
        printf("Threw away strings message, invalid number of strings %d\n",(int)nstrings);
        valid=false;
        return;
    }

    // Read the data
    const void* data = message->GetBufferPointerUnsafe(length);

    if(message->overrun)
    {
        valid=false;
        return;
    }

    char* buff = new char[PACKING_BUFFSIZE];  // Holds packed strings after decompression

    // Ready
    z_stream z;
    z.zalloc = NULL;
    z.zfree = NULL;
    z.opaque = NULL;
    z.avail_in = 0; // We pretend we don't have any data before after we call init
    z.next_in = NULL;

    // Set
    if(inflateInit(&z) != Z_OK)
    {
        valid=false;
        delete [] buff;
        return;
    }

    // Fill inn points to the data we have.
    z.next_in = (z_Byte*)data;
    z.avail_in = (uInt)length;
    z.next_out = (z_Byte*)buff;
    z.avail_out = PACKING_BUFFSIZE;

    // Go
    int res = inflate(&z,Z_FINISH);
    inflateEnd(&z);

    if(res != Z_STREAM_END)
    {
        valid=false;
        delete [] buff;
        return;
    }

    size_t pos = 0;
    msgstrings = new csStringHashReversible(nstrings);
    for(uint32_t i=0; i<nstrings; i++)
    {
        // Unpack ID
        uint32* p = (uint32*)(buff+pos);
        uint32 id = csLittleEndian::UInt32(*p);
        pos += sizeof(uint32);

        // Unpack string
        const char* string = buff+pos;
        pos += strlen(string)+1;

        // csStringSet::Register() cannot handle NULL pointers
        if(string[0] == '\0')
            msgstrings->Register("",id);
        else
            msgstrings->Register(string,id);

        if(pos > z.total_out)
        {
            delete msgstrings;
            delete [] buff;
            msgstrings=NULL;
            valid=false;
            return;
        }
    }
    delete[] buff;
}

csString psMsgStringsMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Digest: %s ", digest?digest->HEXString().GetDataSafe():"(NULL)");
    msgtext.AppendFmt("Only carrying digest: %s",only_carrying_digest?"True":"False");
    if(only_carrying_digest)
    {
        return msgtext;
    }

    msgtext.AppendFmt(" Number of Strings: %d", nstrings);

#ifdef FULL_DEBUG_DUMP
    uint32_t s = 1;
    if(msgstrings)
    {
        csStringSet::GlobalIterator it = msgstrings->GetIterator();
        while(it.HasNext())
        {
            const char* string;
            it.Next(string);
            msgtext.AppendFmt(", %d: '%s'", s++, string);
        }
    }
#endif

    return msgtext;
}

//---------------------------------------------------------------------------
#if 0
PSF_IMPLEMENT_MSG_FACTORY(psCharacterDataMessage,MSGTYPE_CHARACTERDATA);

psCharacterDataMessage::psCharacterDataMessage(uint32_t clientnum,
        csString fullname,
        csString race_name,
        csString mesh_name,
        csString traits,
        csString equipment)
{
    msg.AttachNew(new MsgEntry(fullname.Length() + race_name.Length()
                               + mesh_name.Length() + traits.Length()
                               + equipment.Length() + 5);

                  msg->SetType(MSGTYPE_CHARACTERDATA);
                  msg->clientnum      = clientnum;

                  msg->Add((const char*) fullname);
                  msg->Add((const char*) race_name);
                  msg->Add((const char*) mesh_name);
                  msg->Add((const char*) traits);
                  msg->Add((const char*) equipment);

                  // Sets valid flag based on message overrun state
                  valid=!(msg->overrun);
}

              psCharacterDataMessage::psCharacterDataMessage(MsgEntry* message)
{
    fullname = message->GetStr();
    race_name = message->GetStr();
    mesh_name = message->GetStr();
    traits = message->GetStr();
    equipment = message->GetStr();

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psCharacterDataMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Name: '%s'",fullname.GetData());
    msgtext.AppendFmt(" Race: '%s'",race_name.GetData());
    msgtext.AppendFmt(" Mesh: '%s'",mesh_name.GetData());
    msgtext.AppendFmt(" Traits: '%s'",traits.GetData());
    msgtext.AppendFmt(" Equipment: '%s'",equipment.GetData());

    return msgtext;
}
#endif

//---------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY(psSpecialCombatEventMessage,MSGTYPE_SPECCOMBATEVENT);

psSpecialCombatEventMessage::psSpecialCombatEventMessage(uint32_t clientnum,  EID attacker, EID target, int attack_anim,int defense_anim)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t) + 4 * sizeof(uint32_t) + sizeof(int8_t) ));
    msg->SetType(MSGTYPE_SPECCOMBATEVENT);
    msg->clientnum      = clientnum;

    msg->Add( (uint32_t) attack_anim );
    msg->Add( (uint32_t) defense_anim );
    msg->Add(attacker.Unbox());
    msg->Add(target.Unbox());
    valid=!(msg->overrun);
}
void psSpecialCombatEventMessage::SetClientNum(int cnum)
{
    msg->clientnum = cnum;
}

psSpecialCombatEventMessage::psSpecialCombatEventMessage(MsgEntry *message)
{
    attack_anim = message->GetUInt32();
    defense_anim = message->GetUInt32();
    attacker_id = EID(message->GetUInt32());
    target_id = EID(message->GetUInt32());

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}
csString psSpecialCombatEventMessage::ToString(NetBase::AccessPointers * /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Attack Anim: %d Defense Anim: %d Attacker: %d Target: %d",
            attack_anim, defense_anim, attacker_id.Unbox(), target_id.Unbox());

    return msgtext;
}

PSF_IMPLEMENT_MSG_FACTORY(psCombatEventMessage,MSGTYPE_COMBATEVENT);

psCombatEventMessage::psCombatEventMessage(uint32_t clientnum,
        int event_type,
        EID attacker,
        EID target,
        int targetLocation,
        float damage,
        int attack_anim,
        int defense_anim)
{
    switch(event_type)
    {
        case COMBAT_DODGE:
        case COMBAT_BLOCK:
        case COMBAT_MISS:
        case COMBAT_DEATH:

            msg.AttachNew(new MsgEntry(sizeof(uint8_t) + 4 * sizeof(uint32_t) + sizeof(int8_t)));

            msg->SetType(MSGTYPE_COMBATEVENT);
            msg->clientnum      = clientnum;

            msg->Add((uint8_t)  event_type);
            msg->Add((uint32_t) attack_anim);
            msg->Add((uint32_t) defense_anim);

            msg->Add(attacker.Unbox());
            msg->Add(target.Unbox());
            msg->Add((int8_t)   targetLocation);

            // Sets valid flag based on message overrun state
            valid=!(msg->overrun);
            break;

        case COMBAT_DAMAGE:
        case COMBAT_DAMAGE_NEARLY_DEAD:

            msg.AttachNew(new MsgEntry(sizeof(uint8_t) + 4 * sizeof(uint32_t) + sizeof(int8_t) + sizeof(float)));

            msg->SetType(MSGTYPE_COMBATEVENT);
            msg->clientnum      = clientnum;

            msg->Add((uint8_t)  event_type);
            msg->Add((uint32_t) attack_anim);
            msg->Add((uint32_t) defense_anim);

            msg->Add(attacker.Unbox());
            msg->Add(target.Unbox());
            msg->Add((int8_t)   targetLocation);
            msg->Add((float)    damage);

            // Sets valid flag based on message overrun state
            valid=!(msg->overrun);
            break;
        default:
            Warning2(LOG_NET,"Attempted to construct a psCombatEventMessage with unknown event type %d.\n",event_type);
            valid=false;
            break;
    }

}

void psCombatEventMessage::SetClientNum(int cnum)
{
    msg->clientnum = cnum;
}

psCombatEventMessage::psCombatEventMessage(MsgEntry* message)
{
    event_type  = message->GetUInt8();
    attack_anim = message->GetUInt32();
    defense_anim = message->GetUInt32();
    attacker_id = EID(message->GetUInt32());
    target_id = EID(message->GetUInt32());
    target_location = message->GetInt8();

    if(event_type == COMBAT_DAMAGE || event_type == COMBAT_DAMAGE_NEARLY_DEAD)
    {
        damage = message->GetFloat();
    }
    else
        damage = 0;

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}


csString psCombatEventMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Event Type: %d Attack Anim: %d Defense Anim: %d Attacker: %d Target: %d Location: %d",
                      event_type, attack_anim, defense_anim, attacker_id.Unbox(), target_id.Unbox(), target_location);
    if(event_type == COMBAT_DAMAGE)
    {
        msgtext.AppendFmt(" Damage: %.3f", damage);
    }

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSoundEventMessage,MSGTYPE_SOUND_EVENT);

psSoundEventMessage::psSoundEventMessage(uint32_t clientnum, uint32_t type)
{
    msg.AttachNew(new MsgEntry(1 * sizeof(int)));
    msg->SetType(MSGTYPE_SOUND_EVENT);
    msg->clientnum = clientnum;
    msg->Add((uint32_t)type);
    valid=!(msg->overrun);
}

psSoundEventMessage::psSoundEventMessage(MsgEntry* msg)
{
    type = msg->GetInt32();
    valid=!(msg->overrun);
}

csString psSoundEventMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Type: %d", type);

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psStatDRMessage,MSGTYPE_STATDRUPDATE);

psStatDRMessage::psStatDRMessage(uint32_t clientnum, EID eid, csArray<float> fVitals, csArray<uint32_t> uiVitals, uint8_t version, int flags)
{
    msg.AttachNew(new MsgEntry(2*sizeof(uint32_t) + sizeof(float) * fVitals.GetSize() + sizeof(uint32_t) * uiVitals.GetSize() + sizeof(uint8_t), PRIORITY_LOW));

    msg->clientnum = clientnum;
    msg->SetType(MSGTYPE_STATDRUPDATE);

    msg->Add(eid.Unbox());
    msg->Add((uint32_t)flags);

    for(size_t i = 0; i < fVitals.GetSize(); i++)
    {
        msg->Add(fVitals[i]);
    }
    for(size_t i = 0; i < uiVitals.GetSize(); i++)
    {
        msg->Add(uiVitals[i]);
    }

    msg->Add((uint8_t)version);

    msg->ClipToCurrentSize();

    valid = !(msg->overrun);
}

/** Send a request to the server for a full stat update. */
psStatDRMessage::psStatDRMessage()
{
    msg.AttachNew(new MsgEntry(0, PRIORITY_HIGH));

    msg->clientnum = 0;
    msg->SetType(MSGTYPE_STATDRUPDATE);
    valid = !(msg->overrun);
}

psStatDRMessage::psStatDRMessage(MsgEntry* me)
{
    entityid = EID(me->GetUInt32());
    if(me->GetSize() <= 0)
    {
        request = true;
        return;
    }
    else
    {
        request = false;

        statsDirty = me->GetUInt32();

        if(statsDirty & DIRTY_VITAL_HP)
        {
            hp = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_HP_RATE)
        {
            hp_rate = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_MANA)
        {
            mana = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_MANA_RATE)
        {
            mana_rate = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_PYSSTAMINA)
        {
            pstam = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_PYSSTAMINA_RATE)
        {
            pstam_rate = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_MENSTAMINA)
        {
            mstam = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_MENSTAMINA_RATE)
        {
            mstam_rate = me->GetFloat();
        }

        if(statsDirty & DIRTY_VITAL_EXPERIENCE)
        {
            exp = me->GetInt32();
        }

        if(statsDirty & DIRTY_VITAL_PROGRESSION)
        {
            prog = me->GetInt32();
        }

        counter = me->GetUInt8();
    }

    valid=!(me->overrun);
}

csString psStatDRMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    if(request)
    {
        msgtext.AppendFmt("Request for StatDR Update");
    }
    else
    {
        msgtext.AppendFmt("Ent: %d", entityid.Unbox());

        msgtext.AppendFmt(" Dirty: %X ", statsDirty);

        if(statsDirty & DIRTY_VITAL_HP)
        {
            msgtext.AppendFmt(" HP: %.2f",hp);
        }

        if(statsDirty & DIRTY_VITAL_HP_RATE)
        {
            msgtext.AppendFmt(" HP RATE: %.2f",hp_rate);
        }

        if(statsDirty & DIRTY_VITAL_MANA)
        {
            msgtext.AppendFmt(" MANA: %.2f",mana);
        }

        if(statsDirty & DIRTY_VITAL_MANA_RATE)
        {
            msgtext.AppendFmt(" MANA RATE: %.2f",mana_rate);
        }

        if(statsDirty & DIRTY_VITAL_PYSSTAMINA)
        {
            msgtext.AppendFmt(" PYSSTA: %.2f",pstam);
        }

        if(statsDirty & DIRTY_VITAL_PYSSTAMINA_RATE)
        {
            msgtext.AppendFmt(" PYSSTA RATE: %.2f",pstam_rate);
        }

        if(statsDirty & DIRTY_VITAL_MENSTAMINA)
        {
            msgtext.AppendFmt(" MENSTA: %.2f",mstam);
        }

        if(statsDirty & DIRTY_VITAL_MENSTAMINA_RATE)
        {
            msgtext.AppendFmt(" MENSTA RATE: %.2f",mstam_rate);
        }

        if(statsDirty & DIRTY_VITAL_EXPERIENCE)
        {
            msgtext.AppendFmt(" EXP: %.2f",exp);
        }

        if(statsDirty & DIRTY_VITAL_PROGRESSION)
        {
            msgtext.AppendFmt(" PP: %.2f",prog);
        }

        msgtext.AppendFmt(" C: %d",counter);
    }

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psStatsMessage,MSGTYPE_STATS);

psStatsMessage::psStatsMessage(uint32_t client, float maxHP, float maxMana, float maxWeight, float maxCapacity)
{
    msg.AttachNew(new MsgEntry(sizeof(float)*4));
    msg->Add(maxHP);
    msg->Add(maxMana);
    msg->Add(maxWeight);
    msg->Add(maxCapacity);
    msg->SetType(MSGTYPE_STATS);
    msg->clientnum = client;

    valid=!(msg->overrun);
}

psStatsMessage::psStatsMessage()
{
    msg.AttachNew(new MsgEntry());
    msg->SetType(MSGTYPE_STATS);
    msg->clientnum = 0;
    valid=!(msg->overrun);
}


psStatsMessage::psStatsMessage(MsgEntry* me)
{
    if(me->GetSize() <= 0)
    {
        request = true;
    }
    else
    {
        request = false;

        hp = me->GetFloat();
        mana = me->GetFloat();
        weight = me->GetFloat();
        capacity = me->GetFloat();
    }
}


csString psStatsMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    if(request)
    {
        msgtext.AppendFmt("Request for Stats");
    }
    else
    {
        msgtext.AppendFmt("HP: %.3f Mana: %.3f Weight: %.3f Capacity: %.3f",
                          hp, mana, weight, capacity);
    }

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUISkillMessage, MSGTYPE_GUISKILL);

const char* psGUISkillMessage::SkillCommandString[] =
{"REQUEST","BUY_SkILL","SKILL_LIST","SKILL_SELECTED","DESCRIPTION","QUIT"};


psGUISkillMessage::psGUISkillMessage(uint8_t command,
                                     csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(bool) + sizeof(uint8_t) +
                               commandData.Length() + 1 +
                               skillCache.size()));

    msg->SetType(MSGTYPE_GUISKILL);
    msg->clientnum  = 0;

    msg->Add(false);   //We didn't add stats
    msg->Add(command);
    msg->Add(commandData);
    skillCache.write(msg);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUISkillMessage::psGUISkillMessage(uint32_t clientNum,
                                     uint8_t command,
                                     csString commandData,
                                     psSkillCache* skills,
                                     uint32_t str,
                                     uint32_t end,
                                     uint32_t agi,
                                     uint32_t inl,
                                     uint32_t wil,
                                     uint32_t chr,
                                     uint32_t hp,
                                     uint32_t man,
                                     uint32_t physSta,
                                     uint32_t menSta,
                                     uint32_t hpMax,
                                     uint32_t manMax,
                                     uint32_t physStaMax,
                                     uint32_t menStaMax,
                                     bool openWin,
                                     int32_t focus,
                                     int32_t selSkillCat,
                                     bool isTraining)
{
    //Function begins
    msg.AttachNew(new MsgEntry(sizeof(bool) + sizeof(uint8_t) +
                               commandData.Length() + 1+
                               (skills ? skills->size() : skillCache.size()) +
                               sizeof(str)+
                               sizeof(end)+
                               sizeof(agi)+
                               sizeof(inl)+
                               sizeof(wil)+
                               sizeof(chr)+
                               sizeof(hp)+
                               sizeof(man)+
                               sizeof(physSta)+
                               sizeof(menSta)+
                               sizeof(hpMax)+
                               sizeof(manMax)+
                               sizeof(physStaMax)+
                               sizeof(menStaMax)+
                               sizeof(bool) +
                               sizeof(focus)+
                               sizeof(selSkillCat)+
                               sizeof(bool)
                              ));

    msg->SetType(MSGTYPE_GUISKILL);
    msg->clientnum  = clientNum;

    msg->Add(true);   //We added stats
    msg->Add(command);
    msg->Add(commandData);
    if(skills)
        skills->write(msg);
    else
        skillCache.write(msg);
    msg->Add(str);
    msg->Add(end);
    msg->Add(agi);
    msg->Add(inl);
    msg->Add(wil);
    msg->Add(chr);
    msg->Add(hp);
    msg->Add(man);
    msg->Add(physSta);
    msg->Add(menSta);
    msg->Add(hpMax);
    msg->Add(manMax);
    msg->Add(physStaMax);
    msg->Add(menStaMax);
    msg->Add(openWin);
    msg->Add(focus);
    msg->Add(selSkillCat);
    msg->Add(isTraining);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUISkillMessage::psGUISkillMessage(MsgEntry* message)
{
    if(!message)
        return;

    includeStats = message->GetBool(); //First added value indicates if we added stats or not
    command   = message->GetUInt8();
    commandData = message->GetStr();
    skillCache.read(message);

    if(includeStats)
    {
        strength = message->GetUInt32();
        endurance = message->GetUInt32();
        agility = message->GetUInt32();
        intelligence = message->GetUInt32();
        will = message->GetUInt32();
        charisma = message->GetUInt32();
        hitpoints = message->GetUInt32();
        mana = message->GetUInt32();
        physStamina = message->GetUInt32();
        menStamina = message->GetUInt32();

        hitpointsMax = message->GetUInt32();
        manaMax = message->GetUInt32();
        physStaminaMax = message->GetUInt32();
        menStaminaMax = message->GetUInt32();
        openWindow = message->GetBool();
        focusSkill = message->GetInt32();
        skillCat = message->GetInt32();
        trainingWindow = message->GetBool();
    }
    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUISkillMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %s(%d) ", SkillCommandString[command], command);
#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt("Data: '%s' ", commandData.GetDataSafe());
    msgtext.AppendFmt("SkillCache: %s ", skillCache.ToString().GetData());
#endif
    if(includeStats)
    {
        msgtext.AppendFmt("Str: %d End: %d Agi: %d Int: %d Wil: %d Cha: %d ",
                          strength, endurance, agility, intelligence, will, charisma);
        msgtext.AppendFmt("HP: %d (Max: %d) Mana: %d (Max: %d) ", hitpoints, hitpointsMax, mana, manaMax);
        msgtext.AppendFmt("Physical Stamina: %d (Max: %d) Mental Stamina: %d (Max %d) ",
                          physStamina, physStaminaMax, menStamina, menStaminaMax);
        msgtext.AppendFmt("Focus Skill: %d Skill Cat: %d ", focusSkill, skillCat);
        msgtext.AppendFmt("Window '%s' Training '%s'",
                          (openWindow ? "open" : "closed"), (trainingWindow ? "open" : "closed"));
    }

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIBankingMessage, MSGTYPE_BANKING);

psGUIBankingMessage::psGUIBankingMessage(uint32_t clientNum, uint8_t command, bool guild,
        int circles, int octas, int hexas, int trias,
        int circlesBanked, int octasBanked, int hexasBanked,
        int triasBanked, int maxCircles, int maxOctas, int maxHexas,
        int maxTrias, float exchangeFee, bool forceOpen)
{
    msg.AttachNew(new MsgEntry(sizeof(bool) +
                               sizeof(bool) +
                               sizeof(uint8_t) +
                               sizeof(bool) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(float) +
                               sizeof(bool)));

    msg->SetType(MSGTYPE_BANKING);
    msg->clientnum  = clientNum;
    msg->Add(true);
    msg->Add(false);
    msg->Add(command);
    msg->Add(guild);
    msg->Add(circles);
    msg->Add(octas);
    msg->Add(hexas);
    msg->Add(trias);
    msg->Add(circlesBanked);
    msg->Add(octasBanked);
    msg->Add(hexasBanked);
    msg->Add(triasBanked);
    msg->Add(maxCircles);
    msg->Add(maxOctas);
    msg->Add(maxHexas);
    msg->Add(maxTrias);
    msg->Add(exchangeFee);
    msg->Add(forceOpen);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIBankingMessage::psGUIBankingMessage(uint8_t command, bool guild,
        int circles, int octas, int hexas,int trias)
{
    msg.AttachNew(new MsgEntry(sizeof(bool) +
                               sizeof(bool) +
                               sizeof(uint8_t) +
                               sizeof(bool) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int) +
                               sizeof(int)));

    msg->clientnum = 0;
    msg->SetType(MSGTYPE_BANKING);
    msg->Add(false);
    msg->Add(false);
    msg->Add(command);
    msg->Add(guild);
    msg->Add(circles);
    msg->Add(octas);
    msg->Add(hexas);
    msg->Add(trias);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIBankingMessage::psGUIBankingMessage(uint8_t command, bool guild,
        int coins, int coin)
{
    msg.AttachNew(new MsgEntry(sizeof(bool) +
                               sizeof(bool) +
                               sizeof(uint8_t) +
                               sizeof(bool) +
                               sizeof(int) +
                               sizeof(int)));

    msg->clientnum = 0;
    msg->SetType(MSGTYPE_BANKING);
    msg->Add(false);
    msg->Add(true);
    msg->Add(command);
    msg->Add(guild);
    msg->Add(coins);
    msg->Add(coin);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psGUIBankingMessage::psGUIBankingMessage(MsgEntry* message)
{
    if(!message)
        return;

    sendingFull = message->GetBool();
    sendingExchange = message->GetBool();
    command = message->GetUInt8();
    guild = message->GetBool();

    if(sendingExchange)
    {
        coins = message->GetInt32();
        coin = message->GetInt32();
    }
    else
    {
        circles = message->GetInt32();
        octas = message->GetInt32();
        hexas = message->GetInt32();
        trias = message->GetInt32();
    }

    if(sendingFull)
    {
        circlesBanked = message->GetInt32();
        octasBanked = message->GetInt32();
        hexasBanked = message->GetInt32();
        triasBanked = message->GetInt32();
        maxCircles = message->GetInt32();
        maxOctas = message->GetInt32();
        maxHexas = message->GetInt32();
        maxTrias = message->GetInt32();
        exchangeFee = message->GetFloat();
        openWindow = message->GetBool();
    }

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psGUIBankingMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d ", command);
    msgtext.AppendFmt("Guild Bank: %s", (guild ? "Yes" : "No"));
    msgtext.AppendFmt("Circles Banked: %i, OctasBanked: %i, HexasBanked: %i, TriasBanked: %i", circlesBanked, octasBanked, hexasBanked, triasBanked);
    msgtext.AppendFmt("Circles: %i, Octas: %i, Hexas: %i, Trias: %i", circles, octas, hexas, trias);
    msgtext.AppendFmt("Max Circles: %i, Max Octas: %i, Max Hexas: %i, Max Trias: %i", maxCircles, maxOctas, maxHexas, maxTrias);
    msgtext.AppendFmt("Exchange Fee: %f%%", exchangeFee);
    msgtext.AppendFmt("Bank Window '%s'", (openWindow ? "open" : "closed"));

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPetSkillMessage,MSGTYPE_PET_SKILL);

psPetSkillMessage::psPetSkillMessage(uint8_t command,
                                     csString commandData)
{
    msg.AttachNew(new MsgEntry(sizeof(bool) + sizeof(uint8_t) +
                               commandData.Length() +
                               1));

    msg->SetType(MSGTYPE_PET_SKILL);
    msg->clientnum  = 0;

    msg->Add(false);   //We didn't add stats
    msg->Add(command);
    msg->Add(commandData);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psPetSkillMessage::psPetSkillMessage(uint32_t clientNum,
                                     uint8_t command,
                                     csString commandData,
                                     uint32_t str,
                                     uint32_t end,
                                     uint32_t agi,
                                     uint32_t inl,
                                     uint32_t wil,
                                     uint32_t chr,
                                     uint32_t hp,
                                     uint32_t man,
                                     uint32_t physSta,
                                     uint32_t menSta,
                                     uint32_t hpMax,
                                     uint32_t manMax,
                                     uint32_t physStaMax,
                                     uint32_t menStaMax,
                                     bool openWin,
                                     int32_t focus)
{
    //Function begins
    msg.AttachNew(new MsgEntry(sizeof(bool) + sizeof(uint8_t) +
                               commandData.Length() + 1+
                               sizeof(str)+
                               sizeof(end)+
                               sizeof(agi)+
                               sizeof(inl)+
                               sizeof(wil)+
                               sizeof(chr)+
                               sizeof(hp)+
                               sizeof(man)+
                               sizeof(physSta)+
                               sizeof(menSta)+
                               sizeof(hpMax)+
                               sizeof(manMax)+
                               sizeof(physStaMax)+
                               sizeof(menStaMax)+
                               sizeof(bool) +
                               sizeof(focus)
                              ));

    msg->SetType(MSGTYPE_PET_SKILL);
    msg->clientnum  = clientNum;

    msg->Add(true);   //We added stats
    msg->Add(command);
    msg->Add(commandData);
    msg->Add(str);
    msg->Add(end);
    msg->Add(agi);
    msg->Add(inl);
    msg->Add(wil);
    msg->Add(chr);
    msg->Add(hp);
    msg->Add(man);
    msg->Add(physSta);
    msg->Add(menSta);
    msg->Add(hpMax);
    msg->Add(manMax);
    msg->Add(physStaMax);
    msg->Add(menStaMax);
    msg->Add(openWin);
    msg->Add(focus);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psPetSkillMessage::psPetSkillMessage(MsgEntry* message)
{
    if(!message)
        return;

    includeStats = message->GetBool(); //First added value indicates if we added stats or not
    command   = message->GetUInt8();
    commandData = message->GetStr();

    if(includeStats)
    {
        strength = message->GetUInt32();
        endurance = message->GetUInt32();
        agility = message->GetUInt32();
        intelligence = message->GetUInt32();
        will = message->GetUInt32();
        charisma = message->GetUInt32();
        hitpoints = message->GetUInt32();
        mana = message->GetUInt32();
        physStamina = message->GetUInt32();
        menStamina = message->GetUInt32();

        hitpointsMax = message->GetUInt32();
        manaMax = message->GetUInt32();
        physStaminaMax = message->GetUInt32();
        menStaminaMax = message->GetUInt32();
        openWindow = message->GetBool();
        focusSkill = message->GetInt32();
    }
    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psPetSkillMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d ", command);

    if(includeStats)
    {
        msgtext.AppendFmt("Str: %d End: %d Agi: %d Int: %d Wil: %d Cha: %d ",
                          strength, endurance, agility, intelligence, will, charisma);
        msgtext.AppendFmt("HP: %d (Max: %d) Mana: %d (Max: %d) ", hitpoints, hitpointsMax, mana, manaMax);
        msgtext.AppendFmt("Physical Stamina: %d (Max: %d) Mental Stamina: %d (Max %d) ",
                          physStamina, physStaminaMax, menStamina, menStaminaMax);
        msgtext.AppendFmt("Focus Skill: %d ", focusSkill);
        msgtext.AppendFmt("Window '%s'", (openWindow ? "open" : "closed"));
    }
#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt(" Data: '%s' ", commandData.GetDataSafe());
#endif

    return msgtext;
}

//--------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psDRMessage,MSGTYPE_DEAD_RECKONING);

void psDRMessage::CreateMsgEntry(uint32_t client, NetBase::AccessPointers* accessPointers,
                                 iSector* sector, csString sectorName)
{
    if(sector)
    {
        sectorName = sector->QueryObject()->GetName();
    }

    csStringID sectorNameStrId = accessPointers->Request(sectorName.GetDataSafe());

    int sectorNameLen = (sectorNameStrId == csInvalidStringID) ? sectorName.Length() : 0;

    msg.AttachNew(new MsgEntry(sizeof(uint32)*13 + sizeof(uint8)*4 + sectorNameLen+1));

    msg->SetType(MSGTYPE_DEAD_RECKONING);
    msg->clientnum = client;
}

psDRMessage::psDRMessage(uint32_t client, EID mappedid, uint8_t counter,
                         NetBase::AccessPointers* accessPointers,
                         psLinearMovement* linmove, uint8_t mode)
{
    linmove->GetDRData(on_ground,pos,yrot,sector,vel,worldVel,ang_vel);

    CreateMsgEntry(client, accessPointers, sector, csString());

    WriteDRInfo(client, mappedid,
                on_ground, mode, counter, pos, yrot, sector, csString(),
                vel,worldVel, ang_vel, accessPointers->msgstrings);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psDRMessage::psDRMessage(uint32_t client, EID mappedid,
                         bool on_ground, uint8_t mode, uint8_t counter,
                         const csVector3 &pos, float yrot,iSector* sector, csString sectorName,
                         const csVector3 &vel, csVector3 &worldVel, float ang_vel,
                         NetBase::AccessPointers* accessPointers)
{
    CreateMsgEntry(client, accessPointers, sector, sectorName);

    WriteDRInfo(client, mappedid,
                on_ground, mode, counter, pos, yrot, sector, sectorName,
                vel,worldVel, ang_vel , accessPointers->msgstrings);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

#define ISNONZERO(x) (fabsf(x) > SMALL_EPSILON)

uint8_t psDRMessage::GetDataFlags(const csVector3 &v, const csVector3 &wv, float yrv, uint8_t mode)
{
    uint8_t flags = NOT_MOVING;
    if(mode != ON_GOUND)
        flags |= ACTOR_MODE;
    if(ISNONZERO(yrv))
        flags |= ANG_VELOCITY;
    if(ISNONZERO(v.x))
        flags |= X_VELOCITY;
    if(ISNONZERO(v.y))
        flags |= Y_VELOCITY;
    if(ISNONZERO(v.z))
        flags |= Z_VELOCITY;
    if(ISNONZERO(wv.x))
        flags |= X_WORLDVELOCITY;
    if(ISNONZERO(wv.y))
        flags |= Y_WORLDVELOCITY;
    if(ISNONZERO(wv.z))
        flags |= Z_WORLDVELOCITY;
    return flags;
}

void psDRMessage::WriteDRInfo(uint32_t /*client*/, EID mappedid,
                              bool on_ground, uint8_t mode, uint8_t counter,
                              const csVector3 &pos, float yrot, iSector* sector,
                              csString sectorName, const csVector3 &vel, csVector3 &worldVel,
                              float ang_vel, csStringSet* msgstrings, bool donewriting)
{
    if(sector)
        sectorName = sector->QueryObject()->GetName();
    csStringID sectorNameStrId = msgstrings ? msgstrings->Request(sectorName) : csInvalidStringID;

    msg->Add(mappedid.Unbox());
    msg->Add(counter);

    if(on_ground)
        mode |= ON_GOUND;  // Pack falling status with mode

    // Store packing information
    uint8_t dataflags = GetDataFlags(vel, worldVel, ang_vel, mode);
    msg->Add(dataflags);

    if(dataflags & ACTOR_MODE)
        msg->Add(mode);
    if(dataflags & ANG_VELOCITY)
        msg->Add(ang_vel);
    if(dataflags & X_VELOCITY)
        msg->Add(vel.x);
    if(dataflags & Y_VELOCITY)
        msg->Add(vel.y);
    if(dataflags & Z_VELOCITY)
        msg->Add(vel.z);
    if(dataflags & X_WORLDVELOCITY)
        msg->Add(worldVel.x);
    if(dataflags & Y_WORLDVELOCITY)
        msg->Add(worldVel.y);
    if(dataflags & Z_WORLDVELOCITY)
        msg->Add(worldVel.z);

    msg->Add(pos.x);
    msg->Add(pos.y);
    msg->Add(pos.z);

    msg->Add((uint8_t)(yrot * 256 / TWO_PI));    // Quantize radians to 0-255

    msg->Add((uint32_t) sectorNameStrId);

    if(sectorNameStrId == csInvalidStringID)
        msg->Add(sectorName);

    if(donewriting)   // If we're not writing anymore data after this, shrink to fit
        msg->ClipToCurrentSize();
}

psDRMessage::psDRMessage(void* data, int size, NetBase::AccessPointers* accessPointers)
{
    msg.AttachNew(new MsgEntry(size,PRIORITY_HIGH));
    memcpy(msg->bytes->payload,data,size);
    ReadDRInfo(msg, accessPointers);
}

psDRMessage::psDRMessage(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    msg = NULL;
    ReadDRInfo(me,accessPointers);
}

void psDRMessage::operator=(psDRMessage &other)
{
    entityid   = other.entityid;
    counter    = other.counter;
    ang_vel    = other.ang_vel;
    vel        = other.vel;
    worldVel   = other.worldVel;
    pos        = other.pos;
    on_ground  = other.on_ground;
    yrot       = other.yrot;
    sector     = other.sector;
//    sectorName = other.sectorName;
}

void psDRMessage::ReadDRInfo(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    entityid = me->GetUInt32();
    filterNumber = entityid.Unbox(); // Set the filter number to be used when filtering this in console output
    counter  = me->GetUInt8();

    // Find out what's packed here
    uint8_t dataflags = me->GetUInt8();

    if(dataflags & ACTOR_MODE)
    {
        mode = me->GetInt8();
        on_ground = (mode & ON_GOUND) != 0;
        mode &= ~ON_GOUND;  // Unpack
    }
    else  // Normal
    {
        mode = 0;
        on_ground = true;
    }

    ang_vel = (dataflags & ANG_VELOCITY) ? me->GetFloat() : 0.0f;
    vel.x = (dataflags & X_VELOCITY) ? me->GetFloat() : 0.0f;
    vel.y = (dataflags & Y_VELOCITY) ? me->GetFloat() : 0.0f;
    vel.z = (dataflags & Z_VELOCITY) ? me->GetFloat() : 0.0f;
    worldVel.x = (dataflags & X_WORLDVELOCITY) ? me->GetFloat() : 0.0f;
    worldVel.y = (dataflags & Y_WORLDVELOCITY) ? me->GetFloat() : 0.0f;
    worldVel.z = (dataflags & Z_WORLDVELOCITY) ? me->GetFloat() : 0.0f;

    pos.x = me->GetFloat();
    pos.y = me->GetFloat();
    pos.z = me->GetFloat();

    yrot = me->GetInt8();
    yrot *= TWO_PI/256;

    csStringID sectorNameStrId = (csStringID)me->GetUInt32();
    sectorName = (sectorNameStrId != csInvalidStringID) ? accessPointers->Request(sectorNameStrId) : me->GetStr() ;
    sector = (sectorName.Length()) ? accessPointers->engine->GetSectors()->FindByName(sectorName) : NULL ;
}

bool psDRMessage::IsNewerThan(uint8_t oldCounter)
{
    /** The value of the DR counter goes from 0-255 and then loops back around.
     *  For this message to be newer, we test that the distance from us back to
     *  the given value is no more than half-way back around the loop.
     *  (if the difference is negative, the cast will loop it back to the top)
     */
    return (uint8_t)(counter-oldCounter) <= 127;
}

csString psDRMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("EID: %d C: %d ",entityid.Unbox(),counter);
    msgtext.AppendFmt("Sector: %s ",sectorName.GetDataSafe());
    msgtext.AppendFmt("Pos(%.2f,%.2f,%.2f) ",pos.x,pos.y,pos.z);

#ifdef FULL_DEBUG_DUMP
    msgtext.AppendFmt("Vel(%.2f,%.2f,%.2f) ",vel.x,vel.y,vel.z);
    msgtext.AppendFmt("WVel(%.2f,%.2f,%.2f) ",worldVel.x,worldVel.y,worldVel.z);
    if(on_ground)
        msgtext.Append("OnGround ");
    else
        msgtext.Append("Flying ");
    msgtext.AppendFmt("yrot: %.2f ",yrot);
    msgtext.AppendFmt("ang_vel: %.2f ",ang_vel);
    msgtext.AppendFmt("sector: %s ",sectorName.GetData());
#endif

    return msgtext;
}

//--------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psForcePositionMessage, MSGTYPE_FORCE_POSITION);

psForcePositionMessage::psForcePositionMessage(uint32_t client, uint8_t sequenceNumber,
        const csVector3 &pos, float yRot, iSector* sector, float vel,
        csStringSet* msgstrings, uint32_t time, csString loadBackground,
        csVector2 start, csVector2 dest, csString loadWidget)
{
    CS_ASSERT(sector);
    csString sectorName = sector->QueryObject()->GetName();
    csStringID sectorNameStrId = msgstrings ? msgstrings->Request(sectorName) : csInvalidStringID;

    msg.AttachNew(new MsgEntry(MSG_SIZEOF_FLOAT*9 + sizeof(uint8_t) + sizeof(uint32_t) + (sectorNameStrId == csInvalidStringID ? sectorName.Length() + 1 : 0) + sizeof(uint32_t) + loadBackground.Length() + 1 + loadWidget.Length() + 1, PRIORITY_HIGH, sequenceNumber));

    msg->SetType(MSGTYPE_FORCE_POSITION);
    msg->clientnum = client;

    msg->Add(pos);
    msg->Add(yRot);

    msg->Add((uint32_t) sectorNameStrId);
    if(sectorNameStrId == csInvalidStringID)
        msg->Add(sectorName);

    msg->Add(time);
    msg->Add(loadBackground);

    msg->Add(start.x);
    msg->Add(start.y);
    msg->Add(dest.x);
    msg->Add(dest.y);

    //widget which replaces the standard load one
    msg->Add(loadWidget);

    msg->Add(vel);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psForcePositionMessage::psForcePositionMessage(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    pos = me->GetVector3();
    yrot = me->GetFloat();

    csStringID sectorNameStrId = (csStringID) me->GetUInt32();
    sectorName = sectorNameStrId != csInvalidStringID ? accessPointers->Request(sectorNameStrId) : me->GetStr();
    sector = !sectorName.IsEmpty() ? accessPointers->engine->GetSectors()->FindByName(sectorName) : NULL;

    loadTime = me->GetUInt32();
    backgroundname = me->GetStr();

    start.x = me->GetFloat();
    start.y = me->GetFloat();
    dest.x = me->GetFloat();
    dest.y = me->GetFloat();

    loadWidget = me->GetStr();
    vel = me->GetFloat();

    valid = !(me->overrun);
}

void psForcePositionMessage::operator=(psForcePositionMessage &other)
{
    pos    = other.pos;
    yrot   = other.yrot;
    sector = other.sector;
    vel    = other.vel;
}

csString psForcePositionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    msgtext.AppendFmt("Sector: %s ", sectorName.GetDataSafe());
    msgtext.AppendFmt("Pos(%.2f,%.2f,%.2f), yRot: %.2f vel: %.2f", pos.x, pos.y, pos.z, yrot, vel);
    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPersistWorldRequest,MSGTYPE_PERSIST_WORLD_REQUEST);

psPersistWorldRequest::psPersistWorldRequest()
{
    msg.AttachNew(new MsgEntry());

    msg->SetType(MSGTYPE_PERSIST_WORLD_REQUEST);
    msg->clientnum  = 0;
}

psPersistWorldRequest::psPersistWorldRequest(MsgEntry* /*me*/)
{
}

csString psPersistWorldRequest::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Request world");

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psRequestAllObjects,MSGTYPE_PERSIST_ALL);

psRequestAllObjects::psRequestAllObjects()
{
    msg.AttachNew(new MsgEntry());

    msg->SetType(MSGTYPE_PERSIST_ALL);
    msg->clientnum  = 0;

    valid=!(msg->overrun);
}

psRequestAllObjects::psRequestAllObjects(MsgEntry* /*me*/)
{
}

csString psRequestAllObjects::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Request All objects");

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPersistWorld,MSGTYPE_PERSIST_WORLD);

psPersistWorld::psPersistWorld(uint32_t clientNum, csVector3 pos, const char* sector)
{
    msg.AttachNew(new MsgEntry(sizeof(csVector3) + strlen(sector) + 1));

    msg->SetType(MSGTYPE_PERSIST_WORLD);
    msg->clientnum  = clientNum;

    msg->Add(pos.x);
    msg->Add(pos.y);
    msg->Add(pos.z);
    msg->Add(sector);
}

psPersistWorld::psPersistWorld(MsgEntry* me)
{
    float x = me->GetFloat();
    float y = me->GetFloat();
    float z = me->GetFloat();
    pos = csVector3(x,y,z);
    sector = me->GetStr();
}

csString psPersistWorld::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Position: (%f, %f, %f)\n", pos.x, pos.y, pos.z);
    msgtext.AppendFmt("Sector: '%s'", sector.GetDataSafe());

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPersistActorRequest,MSGTYPE_PERSIST_ACTOR_REQUEST);

psPersistActorRequest::psPersistActorRequest(MsgEntry* /*me*/)
{
}

csString psPersistActorRequest::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Request actors");

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPersistAllEntities,MSGTYPE_PERSIST_ALL_ENTITIES);


psPersistAllEntities::psPersistAllEntities(MsgEntry* me)
{
    msg = me;
}

csString psPersistAllEntities::ToString(NetBase::AccessPointers* accessPointers)
{
    csString msgtext;
    while(true)
    {
        MsgEntry* entity = GetEntityMessage();
        if(!entity) break;

        if(entity->GetType() == MSGTYPE_PERSIST_ACTOR)
        {
            psPersistActor mesg(entity, accessPointers, true);
            msgtext += " Actor: ";
            msgtext += mesg.ToString(accessPointers);
            msgtext += "\n";
        }
        else if(entity->GetType() == MSGTYPE_PERSIST_ITEM)
        {
            psPersistItem mesg(entity, accessPointers);
            msgtext += " Item: ";
            msgtext += mesg.ToString(accessPointers);
            msgtext += "\n";
        }
        else
        {
            Error2("Unhandled type of entity (%d) in AllEntities message.",entity->GetType());
        }

        delete entity;
    }

    return msgtext;
}

bool psPersistAllEntities::AddEntityMessage(MsgEntry* newEnt)
{
    size_t addSize = newEnt->GetSize();

    if(msg->GetSize() > msg->current + addSize + 2*sizeof(uint32_t))  // big enough for next msg
    {
        // we are copying the bytes out of one message into the buffer of another.  handle with care
        msg->Add(newEnt->bytes,(uint32_t)newEnt->bytes->GetTotalSize());
        return true;
    }
    return false;
}

MsgEntry* psPersistAllEntities::GetEntityMessage()
{
    if(msg->overrun)
        return NULL;

    uint32_t len = 0;
    void* data = msg->GetBufferPointerUnsafe(len);

    if(data)
    {
        MsgEntry* newEnt = new MsgEntry((psMessageBytes*)data);
        return newEnt;  // Caller must delete this ptr itself
    }
    return NULL;
}


//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psPersistActor,MSGTYPE_PERSIST_ACTOR);

psPersistActor::psPersistActor(uint32_t clientNum,
                               int type,
                               int masqueradeType,
                               bool control,
                               const char* name,
                               const char* guild,
                               const char* factname,
                               const char* matname,
                               const char* race,
                               const char* mountFactname,
                               const char* MounterAnim,
                               unsigned short int gender,
                               float scale,
                               float mountScale,
                               const char* helmGroup,
                               const char* bracerGroup,
                               const char* beltGroup,
                               const char* cloakGroup,
                               csVector3 collTop, csVector3 collBottom, csVector3 collOffSet,
                               const char* texParts,
                               const char* equipmentParts,
                               uint8_t counter,
                               EID mappedid, csStringSet* msgstrings, psLinearMovement* linmove,
                               uint8_t movementMode,
                               uint8_t serverMode,
                               PID playerID,
                               uint32_t groupID,
                               EID ownerEID,
                               uint32_t flags,
                               PID masterID,
                               bool forNPClient)
{
    msg.AttachNew(new MsgEntry(MAX_MESSAGE_SIZE));

    msg->SetType(MSGTYPE_PERSIST_ACTOR);
    msg->clientnum  = clientNum;

    linmove->GetDRData(on_ground,pos,yrot,sector,vel, worldVel, ang_vel);

    WriteDRInfo(clientNum, mappedid, on_ground, movementMode, counter, pos, yrot, sector, csString(),
                vel, worldVel, ang_vel, msgstrings, false);

    msg->Add((uint32_t) type);
    msg->Add((uint32_t) masqueradeType);
    msg->Add(control);
    msg->Add(name);

    if(guild == 0)
        msg->Add(" ");
    else
        msg->Add(guild);

    msg->Add(msgstrings->Request(factname).GetHash());
    msg->Add(msgstrings->Request(matname).GetHash());
    msg->Add(msgstrings->Request(race).GetHash());
    msg->Add(msgstrings->Request(mountFactname).GetHash());
    msg->Add(msgstrings->Request(MounterAnim).GetHash());
    msg->Add(gender);
    msg->Add(helmGroup);
    msg->Add(bracerGroup);
    msg->Add(beltGroup);
    msg->Add(cloakGroup);
    msg->Add(collTop);
    msg->Add(collBottom);
    msg->Add(collOffSet);
    msg->Add(texParts);
    msg->Add(equipmentParts);
    msg->Add(serverMode);
    posPlayerID = (int) msg->current;
    if(forNPClient)
    {
        // Only NPC client should have the playerID.
        msg->Add(playerID.Unbox());
    }
    else
    {
        msg->Add((uint32_t)0);
    }
    msg->Add(groupID);
    msg->Add(ownerEID.Unbox());
    posInstance = (int) msg->current;
    msg->Add((int32_t)0);
    msg->Add(scale);
    msg->Add(mountScale);
    //add this only for the npcclient probably other data can be added to this like playerid, ownerid and instance
    if(forNPClient)
    {
        msg->Add(masterID.Unbox());
    }
    if(flags)  // No point sending 0, has to be at the end
    {
        msg->Add(flags);
    }

    msg->ClipToCurrentSize();
}

psPersistActor::psPersistActor(MsgEntry* me, NetBase::AccessPointers* accessPointers, bool forNPClient)
{
    ReadDRInfo(me, accessPointers);

    type        = me->GetUInt32();
    masqueradeType = me->GetUInt32();
    control     = me->GetBool();
    name        = me->GetStr();
    guild       = me->GetStr();
    if(guild == " ")
        guild.Clear();

    factname      = accessPointers->Request(csStringID(me->GetUInt32()));
    matname       = accessPointers->Request(csStringID(me->GetUInt32()));
    race          = accessPointers->Request(csStringID(me->GetUInt32()));
    mountFactname = accessPointers->Request(csStringID(me->GetUInt32()));
    MounterAnim   = accessPointers->Request(csStringID(me->GetUInt32()));

    gender      = me->GetInt16();
    helmGroup   = me->GetStr();
    bracerGroup = me->GetStr();
    beltGroup   = me->GetStr();
    cloakGroup  = me->GetStr();

    top         = me->GetVector3();
    bottom      = me->GetVector3();
    offset      = me->GetVector3();

    texParts    = me->GetStr();
    equipment   = me->GetStr();

    serverMode = me->GetUInt8();
    playerID   = PID(me->GetUInt32());
    groupID    = me->GetUInt32();
    ownerEID   = EID(me->GetUInt32());
    instance   = me->GetUInt32();

    scale      = me->GetFloat();
    mountScale = me->GetFloat();

    if(forNPClient || playerID.IsValid())
    {
        masterID = PID(me->GetUInt32());
    }
    else
    {
        masterID = 0;
    }

    if(!me->IsEmpty())
        flags   = me->GetUInt32();
    else
        flags   = 0;
    csString msgtext;
}

csString psPersistActor::ToString(NetBase::AccessPointers* accessPointers)
{
    csString msgtext;

    msgtext.AppendFmt("DR: %s ", psDRMessage::ToString(accessPointers).GetData());
    msgtext.AppendFmt(" Type: %d",type);
    msgtext.AppendFmt(" MaskType: %d",masqueradeType);
    msgtext.AppendFmt(" Control: %s",(control?"true":"false"));
    msgtext.AppendFmt(" Name: '%s'",name.GetDataSafe());
    msgtext.AppendFmt(" Guild: '%s'",guild.GetDataSafe());
    msgtext.AppendFmt(" Factname: '%s'",factname.GetDataSafe());
    msgtext.AppendFmt(" Base Material: '%s'", matname.GetDataSafe());
    msgtext.AppendFmt(" Race: '%s'",race.GetDataSafe());
    msgtext.AppendFmt(" Gender: '%u'",gender);
    msgtext.AppendFmt(" Scale: '%.3f'",scale);
    msgtext.AppendFmt(" Mount Race: '%s'",mountFactname.GetDataSafe());
    msgtext.AppendFmt(" Mounter Anim: '%s'",MounterAnim.GetDataSafe());
    msgtext.AppendFmt(" Top: (%.3f,%.3f,%.3f)",top.x,top.y,top.z);
    msgtext.AppendFmt(" Bottom: (%.3f,%.3f,%.3f)",bottom.x,bottom.y,bottom.z);
    msgtext.AppendFmt(" Offset: (%.3f,%.3f,%.3f)",offset.x,offset.y,offset.z);
    msgtext.AppendFmt(" TexParts: '%s'",texParts.GetDataSafe());
    msgtext.AppendFmt(" Equipment: '%s'",equipment.GetDataSafe());
    msgtext.AppendFmt(" Mode: %d",serverMode);
    msgtext.AppendFmt(" PlayerID: %d",playerID.Unbox());
    msgtext.AppendFmt(" GroupID: %d",groupID);
    msgtext.AppendFmt(" OwnerEID: %d",ownerEID.Unbox());
    msgtext.AppendFmt(" Instance: %d",instance);
    msgtext.AppendFmt(" MasterID: %d",masterID.Unbox());
    msgtext.AppendFmt(" Flags:");
    if(flags & INVISIBLE) msgtext.AppendFmt(" INVISIBLE");
    if(flags & INVINCIBLE) msgtext.AppendFmt(" INVINCIBLE");
    if(flags & NPC) msgtext.AppendFmt(" NPC");
    if(flags & IS_ALIVE) msgtext.AppendFmt(" IS_ALIVE");
    if(flags & NAMEKNOWN) msgtext.AppendFmt(" NAMEKNOWN");

    return msgtext;
}

void psPersistActor::SetInstance(InstanceID instance)
{
    msg->Reset(posInstance);
    msg->Add(instance);
}

uint32_t psPersistActor::PeekEID(MsgEntry* me)
{
    uint32_t eid = me->GetUInt32();
    me->Reset();
    return eid;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psPersistItem,MSGTYPE_PERSIST_ITEM);

psPersistItem::psPersistItem(uint32_t clientNum,
                             EID eid,
                             int type,
                             const char* name,
                             const char* factname,
                             const char* matname,
                             const char* sector,
                             csVector3 pos,
                             float xRot,
                             float yRot,
                             float zRot,
                             uint32_t flags,
                             csStringSet* msgstrings,
                             uint32_t tribeID,
                             uint32_t uid)
{
    msg.AttachNew(new MsgEntry(MAX_MESSAGE_SIZE));

    msg->SetType(MSGTYPE_PERSIST_ITEM);
    msg->clientnum  = clientNum;

    msg->Add(eid.Unbox());
    msg->Add((uint32_t) type);
    msg->Add(name);
    msg->Add(msgstrings->Request(factname).GetHash());
    msg->Add(msgstrings->Request(matname).GetHash());
    msg->Add(msgstrings->Request(sector).GetHash());
    msg->Add(pos);
    msg->Add(xRot);
    msg->Add(yRot);
    msg->Add(zRot);
    if(tribeID != 0)
    {
        flags |= TRIBEID;
    }
    if(uid != 0)
    {
        flags |= ITEM_UID;
    }
    if(flags)  // No point sending 0, only enties called out by flags can follow
    {
        msg->Add(flags);
    }
    // Only entities called out by flags can follow flags
    if(flags & TRIBEID)
    {
        msg->Add(tribeID);
    }
    if(flags & ITEM_UID)
    {
        msg->Add(uid);
    }

    msg->ClipToCurrentSize();
}


psPersistItem::psPersistItem(MsgEntry* me, NetBase::AccessPointers* accessPointers)
    :tribeID(0),flags(0)
{
    eid         = EID(me->GetUInt32());
    type        = me->GetUInt32();
    name        = me->GetStr();
    factname    = accessPointers->Request(csStringID(me->GetUInt32()));
    matname     = accessPointers->Request(csStringID(me->GetUInt32()));
    sector      = accessPointers->Request(csStringID(me->GetUInt32()));
    pos         = me->GetVector3();
    xRot        = me->GetFloat();
    yRot        = me->GetFloat();
    zRot        = me->GetFloat();
    if(!me->IsEmpty())
    {
        flags   = me->GetUInt32();
        if(flags & TRIBEID)
        {
            tribeID     = me->GetInt32();
        }
        if(flags & ITEM_UID)
        {
            uid = me->GetUInt32();
        }
    }
}

csString psPersistItem::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("EID: %d type: %d Name: %s",eid.Unbox(),type,name.GetData());
    msgtext.AppendFmt(" Factname: %s",factname.GetData());
    msgtext.AppendFmt("Sector: %s ",sector.GetDataSafe());
    msgtext.AppendFmt("Pos(%.2f,%.2f,%.2f) ",pos.x,pos.y,pos.z);
    msgtext.AppendFmt("yrot: %.2f Flags:",yRot);
    if(flags & NOPICKUP) msgtext.AppendFmt(" NOPICKUP");
    if(flags & COLLIDE)  msgtext.AppendFmt(" COLLIDE");
    if(flags & TRIBEID)
    {
        msgtext.AppendFmt(" TRIBEID");
        msgtext.AppendFmt(" Belongs to tribe: (id:%d)\n", tribeID);
    }
    if(flags & ITEM_UID)
    {
        msgtext.AppendFmt(" ITEM_UID");
        msgtext.AppendFmt(" uid: %d\n", uid);
    }

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPersistActionLocation,MSGTYPE_PERSIST_ACTIONLOCATION);

psPersistActionLocation::psPersistActionLocation(uint32_t clientNum,
        EID eid,
        int type,
        const char* name,
        const char* sector,
        const char* mesh
                                                )
{
    msg.AttachNew(new MsgEntry(5000));

    msg->SetType(MSGTYPE_PERSIST_ACTIONLOCATION);
    msg->clientnum  = clientNum;

    msg->Add(eid.Unbox());
    msg->Add((uint32_t) type);
    msg->Add(name);
    msg->Add(sector);
    msg->Add(mesh);

    msg->ClipToCurrentSize();
}


psPersistActionLocation::psPersistActionLocation(MsgEntry* me)
{
    eid         = EID(me->GetUInt32());
    type        = me->GetUInt32();
    name        = me->GetStr();
    sector      = me->GetStr();
    mesh        = me->GetStr();
}

csString psPersistActionLocation::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("EID: %d Type: %d Name: '%s' Sector: '%s' Mesh: '%s'",
                      eid.Unbox(),type,name.GetDataSafe(),sector.GetDataSafe(),mesh.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psRemoveObject,MSGTYPE_REMOVE_OBJECT);

psRemoveObject::psRemoveObject(uint32_t clientNum, EID objectEID)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)));

    msg->SetType(MSGTYPE_REMOVE_OBJECT);
    msg->clientnum  = clientNum;

    msg->Add(objectEID.Unbox());
    valid=!(msg->overrun);
}

psRemoveObject::psRemoveObject(MsgEntry* me)
{
    objectEID = EID(me->GetUInt32());
}

csString psRemoveObject::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ObjectEID: %d", objectEID.Unbox());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psBuddyListMsg,MSGTYPE_BUDDY_LIST);

psBuddyListMsg::psBuddyListMsg(uint32_t client, int totalBuddies)
{
    // Possible overflow here!
    msg.AttachNew(new MsgEntry(totalBuddies*100+10));
    msg->SetType(MSGTYPE_BUDDY_LIST);
    msg->clientnum = client;

    buddies.SetSize(totalBuddies);
}

psBuddyListMsg::psBuddyListMsg(MsgEntry* me)
{
    int totalBuddies = me->GetUInt32();
    buddies.SetSize(totalBuddies);

    for(int x = 0; x < totalBuddies; x++)
    {
        buddies[x].name = me->GetStr();
        buddies[x].online = me->GetBool();
    }
}

void psBuddyListMsg::AddBuddy(int num, const char* name, bool onlineStatus)
{
    buddies[num].name = name;
    buddies[num].online = onlineStatus;
}

void psBuddyListMsg::Build()
{
    msg->Add((uint32_t)buddies.GetSize());

    for(size_t x = 0; x < buddies.GetSize(); x++)
    {
        msg->Add(buddies[x].name);
        msg->Add(buddies[x].online);
    }
    msg->ClipToCurrentSize();

    valid=!(msg->overrun);
}

csString psBuddyListMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    for(size_t x = 0; x < buddies.GetSize(); x++)
    {
        msgtext.AppendFmt("Name: '%s' %s ",
                          buddies[x].name.GetDataSafe(), (buddies[x].online ? "Online" : "Offline"));
    }

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psBuddyStatus,MSGTYPE_BUDDY_STATUS);

csString psBuddyStatus::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Buddy: '%s' %s ", buddy.GetDataSafe(), (onlineStatus ? "online" : "offline"));

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMOTDMessage,MSGTYPE_MOTD);

csString psMOTDMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Tip: '%s' MOTD: '%s' Guild: '%s' Guild MOTD: '%s'",
                      tip.GetDataSafe(), motd.GetDataSafe(), guild.GetDataSafe(), guildmotd.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMOTDRequestMessage,MSGTYPE_MOTDREQUEST);

csString psMOTDRequestMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("MOTD Request");

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psQuestionResponseMsg,MSGTYPE_QUESTIONRESPONSE);

csString psQuestionResponseMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Question ID: %d Answer: '%s'", questionID, answer.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psQuestionMessage,MSGTYPE_QUESTION);

csString psQuestionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Question ID: %d Question: '%s' Type: %d", questionID, question.GetDataSafe(), type);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psAdviceMessage,MSGTYPE_ADVICE);

csString psAdviceMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: '%s' Target: '%s' Message: '%s'",
                      sCommand.GetDataSafe(), sTarget.GetDataSafe(), sMessage.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGUIActiveMagicMessage,MSGTYPE_ACTIVEMAGIC);

csString psGUIActiveMagicMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext(command == Add ? "Add " : "Remove ");

    if(type == BUFF)
        msgtext.Append("buff ");
    else if(type == DEBUFF)
        msgtext.Append("debuff ");

    if (name.GetSize())
        msgtext.Append(name[0]);
    else
        msgtext.Append("NULL");
    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSlotMovementMsg,MSGTYPE_SLOT_MOVEMENT);

csString psSlotMovementMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("FROM Container: %d Slot: %d ", fromContainer, fromSlot);
    msgtext.AppendFmt("TO Container: %d Slot: %d ", toContainer, toSlot);
    msgtext.AppendFmt("Stack Count: %d", stackCount);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCmdDropMessage,MSGTYPE_CMDDROP);

psCmdDropMessage::psCmdDropMessage(int quantity, csString &itemName, bool container, bool guarded, bool inplace)
{
    msg.AttachNew(new MsgEntry(sizeof(int32_t) + itemName.Length() + sizeof(bool)*3 + 1));

    msg->SetType(MSGTYPE_CMDDROP);
    msg->Add((int32_t) quantity);
    msg->Add(itemName);
    msg->Add(container);
    msg->Add(guarded);
    msg->Add(inplace);
}

psCmdDropMessage::psCmdDropMessage(MsgEntry* me)
{
    quantity  = me->GetInt32();
    itemName  = me->GetStr();
    container  = me->GetBool();
    guarded = me->GetBool();
    inplace = me->GetBool();
}

csString psCmdDropMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Trying to remove");

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psQuestionCancelMessage,MSGTYPE_QUESTIONCANCEL);

csString psQuestionCancelMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Question ID: %d", questionID);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGuildMOTDSetMessage,MSGTYPE_GUILDMOTDSET);

csString psGuildMOTDSetMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Guild '%s' MOTD: '%s'", guild.GetDataSafe(), guildmotd.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharacterDetailsMessage,MSGTYPE_CHARACTERDETAILS);

psCharacterDetailsMessage::psCharacterDetailsMessage(int clientnum,
        const csString &name2s,
        unsigned short int gender2s,
        const csString &race2s,
        const csString &desc2s,
        const csArray<NetworkDetailSkill> &skills2s,
        const csString &desc_ooc,
        const csString &creationinfo,
        const csString &requestor)
{
    size_t size = sizeof(uint32_t);
    for(size_t x = 0; x < skills2s.GetSize(); x++)
    {
        size += sizeof(uint32_t) + skills2s[x].text.Length()+1;
    }

    msg.AttachNew(new MsgEntry(desc2s.Length()+1 + sizeof(gender2s) + name2s.Length() + 1 + race2s.Length() + 1 + desc_ooc.Length() + 1 + creationinfo.Length() + 1 + requestor.Length() + 1 + size));

    msg->SetType(MSGTYPE_CHARACTERDETAILS);
    msg->clientnum = clientnum;

    msg->Add(name2s);
    msg->Add(gender2s);
    msg->Add(race2s);
    msg->Add(desc2s);
    msg->Add(desc_ooc);
    msg->Add(creationinfo);
    msg->Add(requestor);

    msg->Add((uint32_t)skills2s.GetSize());
    for(size_t x = 0; x < skills2s.GetSize(); x++)
    {
        msg->Add((uint32_t)skills2s[x].category);
        msg->Add(skills2s[x].text);
    }
}

psCharacterDetailsMessage::psCharacterDetailsMessage(MsgEntry* me)
{
    name         = me->GetStr();
    gender       = me->GetUInt16();
    race         = me->GetStr();
    desc         = me->GetStr();
    desc_ooc     = me->GetStr();
    creationinfo = me->GetStr();
    requestor    = me->GetStr();
    uint32_t len = me->GetUInt32();
    for(uint32_t x = 0; x < len; x++)
    {
        NetworkDetailSkill s;

        s.category = me->GetUInt32();
        s.text = me->GetStr();
        skills.Push(s);
    }
}

csString psCharacterDetailsMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Name: '%s' Gender: %d Race: '%s' Description: '%s' OOC Description: %s Character Creation Info: %s Requestor: '%s'",
                      name.GetDataSafe(), gender, race.GetDataSafe(), desc.GetDataSafe(), desc_ooc.GetDataSafe(), creationinfo.GetDataSafe(), requestor.GetDataSafe());
    for(size_t x = 0; x < skills.GetSize(); x++)
    {
        msgtext.AppendFmt(" Skill: '%s' Category: '%d'",
                          skills[x].text.GetDataSafe(), skills[x].category);
    }

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharacterDetailsRequestMessage,MSGTYPE_CHARDETAILSREQUEST);

csString psCharacterDetailsRequestMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Is Me? %s Is Simple? %s Requestor: '%s'",
                      (isMe?"True":"False"), (isSimple?"True":"False"), requestor.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharacterDescriptionUpdateMessage,MSGTYPE_CHARDESCUPDATE);

csString psCharacterDescriptionUpdateMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("New Description: '%s'", newValue.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psViewActionLocationMessage,MSGTYPE_VIEW_ACTION_LOCATION);

psViewActionLocationMessage::psViewActionLocationMessage(uint32_t clientnum, const char* name, const char* description)
{
    msg.AttachNew(new MsgEntry(strlen(name)+1 + strlen(description)+1));

    msg->SetType(MSGTYPE_VIEW_ACTION_LOCATION);
    msg->clientnum = clientnum;

    msg->Add(name);
    msg->Add(description);
}

psViewActionLocationMessage::psViewActionLocationMessage(MsgEntry* me)
{
    name        = me->GetStr();
    description = me->GetStr();
}

csString psViewActionLocationMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Name: '%s' Description: '%s'", name, description);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psViewItemDescription,MSGTYPE_VIEW_ITEM);

psViewItemDescription::psViewItemDescription(int containerID, int slotID)
{
    msg.AttachNew(new MsgEntry(sizeof(int32_t)*2 + sizeof(uint8_t)));
    msg->SetType(MSGTYPE_VIEW_ITEM);
    msg->clientnum = 0;

    msg->Add((uint8_t)REQUEST);
    msg->Add((int32_t)containerID);
    msg->Add((uint32_t)slotID);
}

psViewItemDescription::psViewItemDescription(uint32_t to, const char* itemName, const char* description, const char* icon, uint32_t stackCount)
{
    csString name(itemName);
    csString desc(description);
    csString iconName(icon);

    msg.AttachNew(new MsgEntry(sizeof(uint8_t) + sizeof(bool) + name.Length() + desc.Length() + iconName.Length() + 3 + sizeof(uint32_t)));
    msg->SetType(MSGTYPE_VIEW_ITEM);
    msg->clientnum = to;

    msg->Add((uint8_t)DESCR);
    msg->Add(!IS_CONTAINER);   // Allways false
    msg->Add(itemName);
    msg->Add(description);
    msg->Add(icon);
    msg->Add(stackCount);
}

psViewItemDescription::psViewItemDescription(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    format = me->GetUInt8();

    if(format == REQUEST)
    {
        containerID = me->GetInt32();
        slotID      = me->GetUInt32();

        if(containerID == CONTAINER_INVENTORY_BULK)  // must adjust slot number up
            slotID += PSCHARACTER_SLOT_BULK1;
    }
    else if(format == DESCR)
    {
        hasContents = me->GetBool(); // Will return false.

        itemName = me->GetStr();
        itemDescription = me->GetStr();
        itemIcon = me->GetStr();

        stackCount = me->GetUInt32();
    }
}

csString psViewItemDescription::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    if(format == REQUEST)
    {
        msgtext.AppendFmt("Container ID: %d Slot ID: %d", containerID, slotID);
    }
    else if(format == DESCR)
    {
        msgtext.AppendFmt("Name: '%s' Description: '%s' Icon: '%s' Stack Count: %d ",
                          itemName,
                          itemDescription,
                          itemIcon,
                          stackCount);
    }

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psViewContainerDescription,MSGTYPE_VIEW_CONTAINER);

psViewContainerDescription::psViewContainerDescription(int containerID, int slotID)
{
    msg.AttachNew(new MsgEntry(sizeof(int32_t)*2 + sizeof(uint8_t)));
    msg->SetType(MSGTYPE_VIEW_CONTAINER);
    msg->clientnum = 0;

    msg->Add((uint8_t)REQUEST);
    msg->Add((int32_t)containerID);
    msg->Add((uint32_t)slotID);
}

psViewContainerDescription::psViewContainerDescription(uint32_t to, const char* itemName, const char* description, const char* icon, uint32_t stackCount)
{
    csString name(itemName);
    csString desc(description);
    csString iconName(icon);

    this->itemName = itemName;
    this->itemDescription = description;
    this->itemIcon = icon;
    this->to = to;
    msgSize = (int)(sizeof(uint8_t) + sizeof(bool) + name.Length() + desc.Length() + iconName.Length() + 3 + sizeof(int32_t) + sizeof(int32_t) + sizeof(uint32_t));
}

void psViewContainerDescription::AddContents(const char* name, const char* meshName, const char* materialName, const char* icon, int purifyStatus, int slot, int stack)
{
    ContainerContents item;
    item.name = name;
    item.icon = icon;
    item.meshName = meshName;
    item.materialName = materialName;
    item.purifyStatus = purifyStatus;
    item.slotID = slot;
    item.stackCount = stack;

    contents.Push(item);
    int namesize = name?(int)strlen(name):0;
    int iconsize = icon?(int)strlen(icon):0;
    msgSize += (int)(namesize + iconsize + 3 + sizeof(int)*5);
}

void psViewContainerDescription::ConstructMsg(csStringSet* msgstrings)
{
    msg.AttachNew(new MsgEntry(msgSize));
    msg->SetType(MSGTYPE_VIEW_CONTAINER);
    msg->clientnum = to;

    msg->Add((uint8_t)DESCR);
    msg->Add(IS_CONTAINER);
    msg->Add(itemName);
    msg->Add(itemDescription);
    msg->Add(itemIcon);
    msg->Add((int32_t)containerID);
    msg->Add((int32_t)maxContainerSlots);
    msg->Add((uint32_t)contents.GetSize());
    for(size_t n = 0; n < contents.GetSize(); n++)
    {
        msg->Add(contents[n].name);
        msg->Add(contents[n].icon);
        msg->Add(msgstrings->Request(contents[n].meshName).GetHash());
        msg->Add(msgstrings->Request(contents[n].materialName).GetHash());
        msg->Add(contents[n].purifyStatus);
        msg->Add((uint32_t)contents[n].slotID);
        msg->Add((uint32_t)contents[n].stackCount);
    }
}

psViewContainerDescription::psViewContainerDescription(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    format = me->GetUInt8();

    if(format == REQUEST)
    {
        containerID = me->GetInt32();
        slotID      = me->GetUInt32();

        if(containerID == CONTAINER_INVENTORY_BULK)  // must adjust slot number up
            slotID += PSCHARACTER_SLOT_BULK1;
    }
    else if(format == DESCR)
    {
        hasContents = me->GetBool();

        itemName = me->GetStr();
        itemDescription = me->GetStr();
        itemIcon = me->GetStr();

        if(me->GetType() == MSGTYPE_VIEW_ITEM)
            stackCount = me->GetUInt32();
        else
            stackCount = 0;

        if(hasContents)
        {
            containerID = me->GetInt32();
            maxContainerSlots = me->GetInt32();
            size_t length = me->GetUInt32();

            for(size_t n = 0; n < length; n++)
            {
                ContainerContents item;
                item.name = me->GetStr();
                item.icon = me->GetStr();
                item.meshName = accessPointers->Request(csStringID(me->GetUInt32()));
                item.materialName = accessPointers->Request(csStringID(me->GetUInt32()));
                item.purifyStatus = me->GetUInt32();
                item.slotID = me->GetUInt32();
                item.stackCount = me->GetUInt32();
                contents.Push(item);
            }
        }
    }
}

csString psViewContainerDescription::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    if(format == REQUEST)
    {
        msgtext.AppendFmt("Container ID: %d Slot ID: %d", containerID, slotID);
    }
    else if(format == DESCR)
    {
        msgtext.AppendFmt("Name: '%s' Description: '%s' Icon: '%s' Stack Count: %d ",
                          itemName,
                          itemDescription,
                          itemIcon,
                          stackCount);

        if(hasContents)
        {
            msgtext.AppendFmt("Container ID: %d Max Slots: %d Contains", containerID, maxContainerSlots);

            for(size_t n = 0; n < contents.GetSize(); n++)
            {
                msgtext.AppendFmt("%sName: '%s' Icon: '%s' Purify Status: %d Slot ID: %d Stack Count: %d ",
                                  n?", ":"; ",
                                  contents[n].name.GetData(),
                                  contents[n].icon.GetData(),
                                  contents[n].purifyStatus,
                                  contents[n].slotID,
                                  contents[n].stackCount);
            }
        }
    }

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psViewItemUpdate,MSGTYPE_UPDATE_ITEM);

psViewItemUpdate::psViewItemUpdate(uint32_t to, EID containerID, uint32_t slotID, bool clearSlot,
                                   const char* itemName, const char* icon, const char* meshName,
                                   const char* materialName, uint32_t stackCount, EID ownerID, csStringSet* msgstrings)
{
    msg.AttachNew(new MsgEntry(sizeof(containerID)+1 + sizeof(slotID) + sizeof(clearSlot) + strlen(itemName)+1 + strlen(icon)+1 + strlen(meshName) + strlen(materialName) + 1 + sizeof(stackCount) + sizeof(uint32_t)));
    msg->SetType(MSGTYPE_UPDATE_ITEM);
    msg->clientnum = to;
    msg->Add(containerID.Unbox());
    msg->Add(slotID);
    msg->Add(clearSlot);
    msg->Add(itemName);
    msg->Add(icon);
    msg->Add(stackCount);
    msg->Add(ownerID.Unbox());
    msg->Add(msgstrings->Request(meshName).GetHash());
    msg->Add(msgstrings->Request(materialName).GetHash());
}

//void psViewItemUpdate::ConstructMsg()
//{
//    msg.AttachNew(new MsgEntry( sizeof(containerID) + itemName.Length() + description.Length() + icon.Length() + sizeof(stackCount) );
//    msg->data->type = MSGTYPE_UPDATE_ITEM;
//    msg->clientnum = to;

//    msg->Add( containerID );
//    msg->Add( itemName );
//    msg->Add( icon );
//    msg->Add( (uint32_t)contents[slotID].slotID );
//    msg->Add( stackCount );
//}

psViewItemUpdate::psViewItemUpdate(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    containerID = EID(me->GetUInt32());
    slotID = me->GetUInt32();
    clearSlot = me->GetBool();
    name = me->GetStr();
    icon = me->GetStr();
    stackCount = me->GetUInt32();
    ownerID = EID(me->GetUInt32());
    meshName = accessPointers->Request(csStringID(me->GetUInt32()));
    materialName = accessPointers->Request(csStringID(me->GetUInt32()));
}

csString psViewItemUpdate::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Container ID: %d Slot ID: %d Clear Slot? %s Name: '%s' Icon: '%s' meshName: '%s' materialName: '%s' Stack Count: %d",
                      containerID.Unbox(),
                      slotID,
                      (clearSlot?"True":"False"),
                      name.GetDataSafe(),
                      icon.GetDataSafe(),
                      meshName.GetDataSafe(),
                      materialName.GetDataSafe(),
                      stackCount);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psReadBookTextMessage,MSGTYPE_READ_BOOK);

psReadBookTextMessage::psReadBookTextMessage(uint32_t clientNum, csString &itemName, csString &bookText, bool canWrite, int slotID, int containerID, csString backgroundImg)
{
    msg.AttachNew(new MsgEntry(itemName.Length()+1 + bookText.Length()+1+1+2*sizeof(uint32_t)+backgroundImg.Length()+1));
    msg->SetType(MSGTYPE_READ_BOOK);
    msg->clientnum = clientNum;
    msg->Add(itemName);
    msg->Add(bookText);
    msg->Add((uint8_t) canWrite);
    msg->Add(slotID);
    msg->Add(containerID);
    msg->Add(backgroundImg);

}

psReadBookTextMessage::psReadBookTextMessage(MsgEntry* me)
{
    name=me->GetStr();
    text=me->GetStr();
    canWrite = me->GetUInt8() ? true : false;
    slotID = me->GetUInt32();
    containerID = me->GetUInt32();
    backgroundImg = me->GetStr();
}

csString psReadBookTextMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Name: '%s'", name.GetDataSafe());
#ifdef FULL_DEBUG_DUMP
    csString textForDebug(text);
    textForDebug.ReplaceAll("%", "[pc]");
    msgtext.AppendFmt(" Text:\n%s\n*the end*", textForDebug.GetDataSafe());
#endif
    return msgtext;
}

//------------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY(psWriteBookMessage, MSGTYPE_WRITE_BOOK);
psWriteBookMessage::psWriteBookMessage(uint32_t clientNum, csString &title, csString &content, bool success, int slotID, int containerID)
{
    //uint8_t for the flag, a bool, 2 uints for the item reference, content length + 1 for the content
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+sizeof(bool)+2*sizeof(uint32_t)+title.Length()+1+content.Length()+1));
    msg->SetType(MSGTYPE_WRITE_BOOK);
    msg->clientnum = clientNum;
    msg->Add((uint8_t)RESPONSE);
    msg->Add(success);
    msg->Add(slotID);
    msg->Add(containerID);
    msg->Add(title);
    msg->Add(content);
}

//Request to write on this book
psWriteBookMessage::psWriteBookMessage(int slotID, int containerID)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+2*sizeof(uint32_t)));
    msg->SetType(MSGTYPE_WRITE_BOOK);
    msg->clientnum = 0;
    msg->Add((uint8_t)REQUEST);
    msg->Add(slotID);
    msg->Add(containerID);
}

psWriteBookMessage::psWriteBookMessage(int slotID, int containerID, csString &title, csString &content)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+2*sizeof(uint32_t)+title.Length()+1+content.Length()+1));
    msg->SetType(MSGTYPE_WRITE_BOOK);
    msg->clientnum = 0;
    msg->Add((uint8_t)SAVE);
    msg->Add(slotID);
    msg->Add(containerID);
    msg->Add(title);
    msg->Add(content);
}

psWriteBookMessage::psWriteBookMessage(uint32_t clientNum, csString &title, bool success)
{
    msg = new MsgEntry(sizeof(uint8_t)+title.Length()+1+sizeof(bool));
    msg->SetType(MSGTYPE_WRITE_BOOK);
    msg->clientnum = clientNum;
    msg->Add((uint8_t)SAVERESPONSE);
    msg->Add(title);
    msg->Add(success);
}

psWriteBookMessage::psWriteBookMessage(MsgEntry* me):
    slotID(0), containerID(0), success(false)
{
    messagetype = me->GetUInt8();
    switch(messagetype)
    {
        case REQUEST:
            slotID = me->GetUInt32();
            containerID = me->GetUInt32();
            break;
        case RESPONSE:
            success = me->GetBool();
            slotID = me->GetUInt32();
            containerID = me->GetUInt32();
            title = me->GetStr();
            content = me->GetStr();
            break;
        case SAVE:
            slotID = me->GetUInt32();
            containerID = me->GetUInt32();
            title = me->GetStr();
            content = me->GetStr();
            break;
        case SAVERESPONSE:
            title = me->GetStr();
            success = me->GetBool();
            break;
    }
}

csString psWriteBookMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
#ifdef FULL_DEBUG_DUMP
    csString textForDebug(content);
#endif

    switch(messagetype)
    {
        case REQUEST:
            msgtext.AppendFmt("Write Book REQUEST for slot %d, container %d", slotID, containerID);
            break;
        case RESPONSE:
            msgtext.AppendFmt("Write Book RESPONSE for slot %d, container %d.  Successful? %s  Title: \"%s\"",
                              slotID, containerID, success?"true":"false", title.GetDataSafe());
            break;
        case SAVE:
            msgtext.AppendFmt("Write Book SAVE for slot %d, container %d. Title: \"%s\"",
                              slotID, containerID, title.GetDataSafe());
#ifdef FULL_DEBUG_DUMP
            textForDebug.ReplaceAll("%", "[pc]");
            msgtext.AppendFmt(" Text:\n%s\n*the end*", textForDebug.GetDataSafe());
#endif
            break;
        case SAVERESPONSE:
            msgtext.AppendFmt("Write Book SAVERESPONSE Successful? %s  Title: \"%s\"",
                              success?"true":"false", title.GetDataSafe());
            break;
    }

    return msgtext;
}

//------------------------------------------------------------------------------
PSF_IMPLEMENT_MSG_FACTORY(psQuestRewardMessage,MSGTYPE_QUESTREWARD);

csString psQuestRewardMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Quest Reward: '%s' Type: %d", newValue.GetDataSafe(), msgType);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psExchangeMoneyMsg,MSGTYPE_EXCHANGE_MONEY);

psExchangeMoneyMsg::psExchangeMoneyMsg(uint32_t client, int container,
                                       int trias, int hexas, int circles,int octas)
{
    msg.AttachNew(new MsgEntry(sizeof(int) * 5));
    msg->SetType(MSGTYPE_EXCHANGE_MONEY);
    msg->clientnum = client;

    msg->Add((uint32_t)container);
    msg->Add((uint32_t)trias);
    msg->Add((uint32_t)hexas);
    msg->Add((uint32_t)circles);
    msg->Add((uint32_t)octas);
}


psExchangeMoneyMsg::psExchangeMoneyMsg(MsgEntry* me)
{
    container = me->GetUInt32();
    trias = me->GetUInt32();
    hexas = me->GetUInt32();
    circles = me->GetUInt32();
    octas = me->GetUInt32();
}

csString psExchangeMoneyMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Container: %d %d trias, %d hexas, %d circles, %d octas",
                      container, trias, hexas, circles, octas);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psExchangeRequestMsg,MSGTYPE_EXCHANGE_REQUEST);

psExchangeRequestMsg::psExchangeRequestMsg(bool withPlayer)
{
    msg.AttachNew(new MsgEntry(1 + 1));
    msg->SetType(MSGTYPE_EXCHANGE_REQUEST);
    msg->clientnum = 0;

    msg->Add("");
    msg->Add(withPlayer);
}

psExchangeRequestMsg::psExchangeRequestMsg(uint32_t client, csString &playerName, bool withPlayer)
{
    msg.AttachNew(new MsgEntry(playerName.Length() + 1 + 1));
    msg->SetType(MSGTYPE_EXCHANGE_REQUEST);
    msg->clientnum = client;

    msg->Add(playerName);
    msg->Add(withPlayer);
}


psExchangeRequestMsg::psExchangeRequestMsg(MsgEntry* me)
{
    player = me->GetStr();
    withPlayer = me->GetBool();
}

csString psExchangeRequestMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Player: '%s' WithPlayer: %s", player.GetDataSafe(), (withPlayer?"True":"False"));

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psExchangeAddItemMsg,MSGTYPE_EXCHANGE_ADD_ITEM);

psExchangeAddItemMsg::psExchangeAddItemMsg(uint32_t clientNum,
        const csString &name,
        const csString &meshFactName,
        const csString &materialName,
        int containerID,
        int slot,
        int stackcount,
        const csString &icon,
        csStringSet* msgstrings)
{
    msg.AttachNew(new MsgEntry(1000));
    msg->SetType(MSGTYPE_EXCHANGE_ADD_ITEM);
    msg->clientnum = clientNum;

    msg->Add(name);
    msg->Add(msgstrings->Request(meshFactName));
    msg->Add(msgstrings->Request(materialName));
    msg->Add((uint32_t) containerID);
    msg->Add((uint32_t) slot);
    msg->Add((uint32_t) stackcount);
    msg->Add(icon);
    msg->ClipToCurrentSize();
}

psExchangeAddItemMsg::psExchangeAddItemMsg(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    name         = me->GetStr();
    meshFactName = accessPointers->Request(csStringID(me->GetUInt32()));
    materialName = accessPointers->Request(csStringID(me->GetUInt32()));
    container    = me->GetUInt32();
    slot         = me->GetUInt32();
    stackCount   = me->GetUInt32();
    icon         = me->GetStr();
}

csString psExchangeAddItemMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Name: '%s' Container: %d Slot: %d Stack Count: %d Icon: '%s'",
                      name.GetDataSafe(), container, slot, stackCount, icon.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psExchangeRemoveItemMsg,MSGTYPE_EXCHANGE_REMOVE_ITEM);

psExchangeRemoveItemMsg::psExchangeRemoveItemMsg(uint32_t client, int container, int slot, int newStack)
{
    msg.AttachNew(new MsgEntry(sizeof(int) * 3));
    msg->SetType(MSGTYPE_EXCHANGE_REMOVE_ITEM);
    msg->clientnum = client;

    msg->Add((uint32_t)container);
    msg->Add((uint32_t)slot);
    msg->Add((uint32_t)newStack);
}

psExchangeRemoveItemMsg::psExchangeRemoveItemMsg(MsgEntry* msg)
{
    container       = msg->GetUInt32();
    slot            = msg->GetUInt32();
    newStackCount   = msg->GetUInt32();
}


csString psExchangeRemoveItemMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Container: %d Slot %d Stack Count: %d", container, slot, newStackCount);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psExchangeAcceptMsg,MSGTYPE_EXCHANGE_ACCEPT);

psExchangeAcceptMsg::psExchangeAcceptMsg(uint32_t client)
{
    msg.AttachNew(new MsgEntry());
    msg->SetType(MSGTYPE_EXCHANGE_ACCEPT);
    msg->clientnum = client;
}

psExchangeAcceptMsg::psExchangeAcceptMsg(MsgEntry* /*me*/)
{
}

csString psExchangeAcceptMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Exchange Accept");

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psExchangeStatusMsg,MSGTYPE_EXCHANGE_STATUS);

psExchangeStatusMsg::psExchangeStatusMsg(uint32_t client, bool playerAccept, bool targetAccept)
{
    msg.AttachNew(new MsgEntry(sizeof(bool)*2));
    msg->SetType(MSGTYPE_EXCHANGE_STATUS);
    msg->clientnum = client;
    msg->Add(playerAccept);
    msg->Add(targetAccept);

}

psExchangeStatusMsg::psExchangeStatusMsg(MsgEntry* me)
{
    playerAccept = me->GetBool();
    otherAccept = me->GetBool();
}

csString psExchangeStatusMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Player %s, Other Player %s",
                      (playerAccept?"accepted":"rejected"),
                      (otherAccept?"accepted":"rejected"));

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psExchangeEndMsg,MSGTYPE_EXCHANGE_END);

psExchangeEndMsg::psExchangeEndMsg(uint32_t client)
{
    msg.AttachNew(new MsgEntry());
    msg->SetType(MSGTYPE_EXCHANGE_END);
    msg->clientnum = client;
}

psExchangeEndMsg::psExchangeEndMsg(MsgEntry* /*me*/)
{
}

csString psExchangeEndMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Exchange End");

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psUpdateObjectNameMessage,MSGTYPE_NAMECHANGE);

psUpdateObjectNameMessage::psUpdateObjectNameMessage(uint32_t client, EID eid, const char* newName)
{
    msg.AttachNew(new MsgEntry(strlen(newName)+1 + sizeof(uint32_t)));

    msg->SetType(MSGTYPE_NAMECHANGE);
    msg->clientnum = client;

    msg->Add(eid.Unbox());
    msg->Add(newName);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psUpdateObjectNameMessage::psUpdateObjectNameMessage(MsgEntry* me)
{
    objectID      = EID(me->GetUInt32());
    newObjName    = me->GetStr();
}

csString psUpdateObjectNameMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Object ID: %d New Name: '%s'", objectID.Unbox(), newObjName.GetDataSafe());

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psUpdatePlayerGuildMessage,MSGTYPE_GUILDCHANGE);

psUpdatePlayerGuildMessage::psUpdatePlayerGuildMessage(uint32_t client, int total, const char* newGuild)
{
    msg.AttachNew(new MsgEntry(strlen(newGuild)+1 + (sizeof(uint32_t) * (total+1))));

    msg->SetType(MSGTYPE_GUILDCHANGE);
    msg->clientnum = client;

    msg->Add((uint32_t)total);
    msg->Add(newGuild);

    valid = false; // need to add first
}

psUpdatePlayerGuildMessage::psUpdatePlayerGuildMessage(uint32_t client, EID entity, const char* newGuild)
{
    msg.AttachNew(new MsgEntry(strlen(newGuild) + 1 + sizeof(uint32_t)*2));

    msg->SetType(MSGTYPE_GUILDCHANGE);
    msg->clientnum = client;

    msg->Add((uint32_t) 1);
    msg->Add(newGuild);

    AddPlayer(entity);
}

psUpdatePlayerGuildMessage::psUpdatePlayerGuildMessage(MsgEntry* me)
{
    int total     = (int)me->GetUInt32();
    newGuildName  = me->GetStr();

    for(int i = 0; i < total; i++)
        objectID.Push(me->GetUInt32());
}

void psUpdatePlayerGuildMessage::AddPlayer(EID id)
{
    msg->Add(id.Unbox());

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

csString psUpdatePlayerGuildMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("New Guild Name: '%s' Player IDs: ", newGuildName.GetDataSafe());

    for(size_t i = 0; i < objectID.GetSize(); i++)
    {
        msgtext.AppendFmt("%d, ", objectID[i]);
    }

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psUpdatePlayerGroupMessage,MSGTYPE_GROUPCHANGE);

psUpdatePlayerGroupMessage::psUpdatePlayerGroupMessage(int clientnum, EID objectID, uint32_t groupID)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t)*2));

    msg->SetType(MSGTYPE_GROUPCHANGE);
    msg->clientnum = clientnum;

    msg->Add(objectID.Unbox());
    msg->Add(groupID);

    // Sets valid flag based on message overrun state
    valid=!(msg->overrun);
}

psUpdatePlayerGroupMessage::psUpdatePlayerGroupMessage(MsgEntry* me)
{
    objectID      = EID(me->GetUInt32());
    groupID       = me->GetUInt32();
}

csString psUpdatePlayerGroupMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("EID: %d Group ID: %d", objectID.Unbox(), groupID);

    return msgtext;
}

//------------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psNameCheckMessage,MSGTYPE_CHAR_CREATE_NAME);

psNameCheckMessage::psNameCheckMessage(const char* newname)
{
    csString name(newname);
    msg.AttachNew(new MsgEntry(strlen(newname)+2));

    msg->SetType(MSGTYPE_CHAR_CREATE_NAME);

    csString lastname,firstname(name.Slice(0,name.FindFirst(' ')));
    if(name.FindFirst(' ') != SIZET_NOT_FOUND)
        lastname = name.Slice(name.FindFirst(' ')+1,name.Length());

    msg->Add(firstname);
    msg->Add(lastname);
}

psNameCheckMessage::psNameCheckMessage(const char* firstName, const char* lastName)
{
    msg.AttachNew(new MsgEntry(strlen(firstName)+strlen(lastName)+2));

    msg->SetType(MSGTYPE_CHAR_CREATE_NAME);

    msg->Add(firstName);
    msg->Add(lastName);
}


psNameCheckMessage::psNameCheckMessage(uint32_t client, bool accept, const char* reason)
{
    msg.AttachNew(new MsgEntry(sizeof(bool) + strlen(reason)+1));

    msg->SetType(MSGTYPE_CHAR_CREATE_NAME);
    msg->clientnum = client;
    msg->Add(accept);
    msg->Add(reason);
}

psNameCheckMessage::psNameCheckMessage(MsgEntry* /*me*/)
{
    /*
      TODO: Check if this is this way or the other way around.
    if (me->clientnum)
        FromClient(me);
    msgFromServer = false;
    else
        FromServer(me);
    msgFromServer = true;
    */
}


void psNameCheckMessage::FromClient(MsgEntry* me)
{
    firstName = me->GetStr();
    lastName = me->GetStr();
}

void psNameCheckMessage::FromServer(MsgEntry* me)
{
    accepted = me->GetBool();
    reason = me->GetStr();
}

csString psNameCheckMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("TODO");
    /* When the FromClient/FromServer issue is sorted in psNameCheckMessage(MsgEntry *) function...
        if (msgFromServer)
        {
            msgtext.AppendFmt("Name Check: %s Reason: '%s'",
                    (accepted?"Accepted":"Rejected"), reason.GetDataSafe);
        }
        else
        {
           msgText.AppendFmt("Name: '%s %s'", firstName.GetDataSafe(), lastName.GetDataSafe());
        }
    */
    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPingMsg,MSGTYPE_PING);

psPingMsg::psPingMsg(MsgEntry* me)
{
    id = me->GetInt32();
    flags = me->GetUInt8();
}

psPingMsg::psPingMsg(uint32_t client, uint32_t id, uint8_t flags)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + sizeof(uint8_t) ,PRIORITY_LOW));

    msg->SetType(MSGTYPE_PING);
    msg->clientnum = client;
    msg->Add(id);
    msg->Add(flags);
}

csString psPingMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ID: %d flags:", id);
    if(flags & PINGFLAG_REQUESTFLAGS)
        msgtext.Append(" REQUESTFLAGS");
    if(flags & PINGFLAG_READY)
        msgtext.Append(" READY");
    if(flags & PINGFLAG_HASBEENREADY)
        msgtext.Append(" HASBEENREADY");
    if(flags & PINGFLAG_SERVERFULL)
        msgtext.Append(" SERVERFULL");

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psHeartBeatMsg,MSGTYPE_HEART_BEAT);

psHeartBeatMsg::psHeartBeatMsg(MsgEntry* /*me*/)
{
}

psHeartBeatMsg::psHeartBeatMsg(uint32_t client)
{
    msg.AttachNew(new MsgEntry(0 ,PRIORITY_HIGH));

    msg->SetType(MSGTYPE_HEART_BEAT);
    msg->clientnum = client;
}

csString psHeartBeatMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Alive?");

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psLockpickMessage,MSGTYPE_LOCKPICK);

psLockpickMessage::psLockpickMessage(const char* password)
{
    msg.AttachNew(new MsgEntry(strlen(password) +1));

    msg->SetType(MSGTYPE_LOCKPICK);
    msg->clientnum = 0;
    msg->Add(password);
}

psLockpickMessage::psLockpickMessage(MsgEntry* me)
{
    password = me->GetStr();
}

csString psLockpickMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Password: '%s'", password.GetDataSafe());

    return msgtext;
}

// ---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMSpawnItems,MSGTYPE_GMSPAWNITEMS);

psGMSpawnItems::psGMSpawnItems(uint32_t client,const char* type,unsigned int size)
{
    msg.AttachNew(new MsgEntry(
                      strlen(type) +1 +
                      sizeof(bool) + size + sizeof(uint32_t)
                  ));

    msg->SetType(MSGTYPE_GMSPAWNITEMS);
    msg->clientnum = client;
    msg->Add(type);
    msg->Add(false);
}

psGMSpawnItems::psGMSpawnItems(const char* type)
{
    msg.AttachNew(new MsgEntry(
                      strlen(type) +1 +
                      sizeof(bool)
                  ));

    msg->SetType(MSGTYPE_GMSPAWNITEMS);
    msg->clientnum = 0;
    msg->Add(type);
    msg->Add(true);
}

psGMSpawnItems::psGMSpawnItems(MsgEntry* me)
{
    type = me->GetStr();
    request = me->GetBool();

    if(!request)
    {
        unsigned int length = me->GetUInt32();
        for(unsigned int i = 0; i < length; i++)
        {
            Item item;
            item.name = me->GetStr();
            item.mesh = me->GetStr();
            item.icon = me->GetStr();

            items.Push(item);
        }
    }
}

csString psGMSpawnItems::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Type: '%s' ", type.GetDataSafe());

#ifdef FULL_DEBUG_DUMP
    if(!request)
    {
        for(size_t i = 0; i < items.GetSize(); i++)
        {
            msgtext.AppendFmt("Name: '%s' Mesh: '%s', ",
                              items[i].name.GetDataSafe(),
                              items[i].mesh.GetDataSafe());
        }
    }
#endif

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMSpawnTypes,MSGTYPE_GMSPAWNTYPES);

psGMSpawnTypes::psGMSpawnTypes(uint32_t client,unsigned int size)
{
    msg.AttachNew(new MsgEntry(size + sizeof(uint32_t)));

    msg->SetType(MSGTYPE_GMSPAWNTYPES);
    msg->clientnum = client;
}

psGMSpawnTypes::psGMSpawnTypes(MsgEntry* me)
{
    unsigned int length = me->GetUInt32();
    for(unsigned int i = 0; i < length; i++)
    {
        types.Push(me->GetStr());
    }
}

csString psGMSpawnTypes::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Types: ");

    for(size_t i = 0; i < types.GetSize(); i++)
    {
        msgtext.AppendFmt("'%s', ", types[i].GetDataSafe());
    }

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMSpawnGetMods, MSGTYPE_GMSPAWNGETMODS);

psGMSpawnGetMods::psGMSpawnGetMods(const char *itemname)
{
    msg.AttachNew(new MsgEntry(strlen(itemname) + 1));

    msg->SetType(MSGTYPE_GMSPAWNGETMODS);
    msg->Add(itemname);
}

psGMSpawnGetMods::psGMSpawnGetMods(MsgEntry* me)
{
    item = me->GetStr();
}

csString psGMSpawnGetMods::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Get Item Modifiers: %s", item.GetDataSafe());
    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMSpawnMods, MSGTYPE_GMSPAWNMODS);

psGMSpawnMods::psGMSpawnMods(uint32_t client, csArray<ItemModifier>& imods)
{
    size_t size = sizeof(uint32_t); // For array size count
    for(size_t i = 0; i < imods.GetSize(); i++)
    {
        size += strlen(imods[i].name) + 1;
        size += sizeof(uint32_t) * 2;
    }

    msg.AttachNew(new MsgEntry(size));

    msg->SetType(MSGTYPE_GMSPAWNMODS);
    msg->clientnum = client;
    msg->Add((uint32_t)imods.GetSize());
    for(size_t i = 0; i < imods.GetSize(); i++)
    {
        msg->Add(imods[i].name);
        msg->Add(imods[i].id);
        msg->Add(imods[i].type);
    }
}

psGMSpawnMods::psGMSpawnMods(MsgEntry* me)
{
    unsigned int length = me->GetUInt32();
    for(unsigned int i = 0; i < length; i++)
    {
        struct ItemModifier mod;
        mod.name = me->GetStr();
        mod.id = me->GetUInt32();
        mod.type = me->GetUInt32();
        mods.Push(mod);
    }
}

csString psGMSpawnMods::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Item Modifiers: ");

    for(size_t i = 0; i < mods.GetSize(); i++)
    {
        msgtext.AppendFmt("'%s' %u %u, ",
                mods[i].name.GetDataSafe(), mods[i].id, mods[i].type);
    }

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMSpawnItem,MSGTYPE_GMSPAWNITEM);

psGMSpawnItem::psGMSpawnItem(const char* item,
                             unsigned int count,
                             bool lockable,
                             bool locked,
                             const char* lskill,
                             int lstr,
                             bool pickupable,
                             bool collidable,
                             bool Unpickable,
                             bool Transient,
                             bool SettingItem,
                             bool NPCOwned,
                             bool pickupableWeak,
                             bool random,
                             float quality,
                             csArray<uint32_t>* mods
                            )
{
    size_t modsize = 1;
    if (mods)
    {
        modsize += mods->GetSize() * sizeof(uint32_t);
    }
    msg.AttachNew(new MsgEntry(
                      strlen(item) +1    // item
                      + sizeof(uint32_t) // count
                      + sizeof(bool)     // locked
                      + sizeof(bool)     // lockable
                      + strlen(lskill)+1 // lskill
                      + sizeof(int32_t)  // lstr
                      + sizeof(bool)     // pickupable
                      + sizeof(bool)     // collidable
                      + sizeof(bool)     // pickable
                      + sizeof(bool)     // transient
                      + sizeof(bool)     // settingitem
                      + sizeof(bool)     // npc owned
                      + sizeof(bool)     // random
                      + sizeof(float)    // quality
                      + sizeof(bool)     // pickupableWeak
                      + modsize          // item modifiers
                  ));

    msg->SetType(MSGTYPE_GMSPAWNITEM);
    msg->clientnum = 0;
    msg->Add(item);
    msg->Add((uint32_t)count);
    msg->Add(lockable);
    msg->Add(locked);
    msg->Add(lskill);
    msg->Add((int32_t)lstr);
    msg->Add(pickupable);
    msg->Add(collidable);
    msg->Add(Unpickable);
    msg->Add(Transient);
    msg->Add(SettingItem);
    msg->Add(NPCOwned);
    msg->Add(random);
    msg->Add(quality);
    msg->Add(pickupableWeak);
    if (mods)
    {
        msg->Add((uint8_t)mods->GetSize());
        for (size_t i = 0; i < mods->GetSize(); i++)
            msg->Add(mods->Get(i));
    }
    else
    {
        msg->Add((uint8_t)0);
    }
}

psGMSpawnItem::psGMSpawnItem(MsgEntry* me)
{
    item = me->GetStr();
    count = me->GetUInt32();
    lockable = me->GetBool();
    locked = me->GetBool();
    lskill = me->GetStr();
    lstr = me->GetInt32();
    pickupable = me->GetBool();
    collidable = me->GetBool();
    Unpickable = me->GetBool();
    Transient = me->GetBool();
    SettingItem = me->GetBool();
    NPCOwned = me->GetBool();
    random = me->GetBool();
    quality = me->GetFloat();
    if(!me->IsEmpty())
    {
        pickupableWeak = me->GetBool();
    }
    else
    {
        pickupableWeak = true;
    }
    if(!me->IsEmpty())
    {
        mods.SetSize(me->GetUInt8());
        for (size_t i = 0; i < mods.GetSize(); i++)
        {
            mods[i] = me->GetUInt32();
        }
    }
}

csString psGMSpawnItem::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Item: '%s' Count: %d Lockable: %s, Is %s, Skill: '%s' Str: %d Pickupable: %s Collidable: %s Random: %s Unpickable: %s SettingItem: %s NPC Owned: %s Transient: %s Quality %f pickupable Weak %s prefix %d suffix %d adjective %d",
                      item.GetDataSafe(),
                      count,
                      (lockable ? "True" : "False"),
                      (locked ? "Locked" : "Unlocked"),
                      lskill.GetDataSafe(),
                      lstr,
                      (pickupable ? "True" : "False"),
                      (collidable ? "True" : "False"),
                      (random ? "True" : "False"),
                      (Unpickable ? "True" : "False"),
                      (SettingItem ? "True" : "False"),
                      (NPCOwned ? "True" : "False"),
                      (Transient ? "True" : "False"),
                      quality,
                      (pickupableWeak ? "True" : "False"),
                      mods.GetSize() > 0 ? mods[0] : 0,
                      mods.GetSize() > 1 ? mods[1] : 0,
                      mods.GetSize() > 2 ? mods[2] : 0);

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psLootRemoveMessage,MSGTYPE_LOOTREMOVE);

psLootRemoveMessage::psLootRemoveMessage(uint32_t client,int item)
{
    msg.AttachNew(new MsgEntry(
                      sizeof(int)
                  ));

    msg->SetType(MSGTYPE_LOOTREMOVE);
    msg->clientnum = client;
    msg->Add((int32_t)item);
}

psLootRemoveMessage::psLootRemoveMessage(MsgEntry* me)
{
    id = me->GetInt32();
}

csString psLootRemoveMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("ID: %d", id);

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharCreateTraitsMessage,MSGTYPE_CHAR_CREATE_TRAITS);

csString psCharCreateTraitsMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Data: '%s'",string.GetDataSafe());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psClientStatusMessage,MSGTYPE_CLIENTSTATUS);

psClientStatusMessage::psClientStatusMessage(bool /*ready*/)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)));

    msg->SetType(MSGTYPE_CLIENTSTATUS);
    msg->clientnum      = 0; // To server only

    msg->Add((uint8_t) READY);

    valid=true;
}

psClientStatusMessage::psClientStatusMessage(MsgEntry* message)
{
    uint8_t status = message->GetUInt8();

    ready = status & READY;

    // Sets valid flag based on message overrun state
    valid=!(message->overrun);
}

csString psClientStatusMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Ready: %s",(ready?"true":"false"));

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMoveModMsg,MSGTYPE_MOVEMOD);

psMoveModMsg::psMoveModMsg(uint32_t client, ModType type, const csVector3 &move, float Yrot)
{
    msg.AttachNew(new MsgEntry(1 + 4*sizeof(uint32)));
    msg->SetType(MSGTYPE_MOVEMOD);
    msg->clientnum = client;

    msg->Add((uint8_t)type);

    if(type != NONE)
    {
        msg->Add(move);
        msg->Add(Yrot);
    }
    else
    {
        msg->ClipToCurrentSize();
    }

    valid = !(msg->overrun);
}

psMoveModMsg::psMoveModMsg(MsgEntry* me)
{
    type = (ModType)me->GetUInt8();

    if(type != NONE)
    {
        movementMod = me->GetVector3();
        rotationMod = me->GetFloat();
    }
    else
    {
        movementMod = 0.0f;
        rotationMod = 0.0f;
    }
}

csString psMoveModMsg::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("type %d move(%.2f,%.2f,%.2f) rot(%.2f)",
                      (int)type,
                      movementMod.x, movementMod.y, movementMod.z,
                      rotationMod);

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMsgRequestMovement,MSGTYPE_REQUESTMOVEMENTS);

psMsgRequestMovement::psMsgRequestMovement()
{
    msg.AttachNew(new MsgEntry(10));
    msg->SetType(MSGTYPE_REQUESTMOVEMENTS);
    msg->clientnum = 0;
    msg->ClipToCurrentSize();
}

psMsgRequestMovement::psMsgRequestMovement(MsgEntry* /*me*/)
{
}

csString psMsgRequestMovement::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    msgtext.Format("Requesting movements");
    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMovementInfoMessage,MSGTYPE_MOVEINFO);

psMovementInfoMessage::psMovementInfoMessage(size_t modes, size_t moves)
{
    msg.AttachNew(new MsgEntry(10000));
    msg->SetType(MSGTYPE_MOVEINFO);
    msg->clientnum = 0;

    this->modes = modes;
    this->moves = moves;

    msg->Add((uint32_t)modes);
    msg->Add((uint32_t)moves);
}

psMovementInfoMessage::psMovementInfoMessage(MsgEntry* me)
{
    msg = me;

    modes = msg->GetUInt32();
    moves = msg->GetUInt32();
}

void psMovementInfoMessage::AddMode(uint32 id, const char* name, csVector3 move_mod, csVector3 rotate_mod, const char* idle_anim)
{
    msg->Add(id);
    msg->Add(name);
    msg->Add(move_mod);
    msg->Add(rotate_mod);
    msg->Add(idle_anim);
}

void psMovementInfoMessage::AddMove(uint32 id, const char* name, csVector3 base_move, csVector3 base_rotate)
{
    msg->Add(id);
    msg->Add(name);
    msg->Add(base_move);
    msg->Add(base_rotate);
}

void psMovementInfoMessage::GetMode(uint32 &id, const char* &name, csVector3 &move_mod, csVector3 &rotate_mod, const char* &idle_anim)
{
    id = msg->GetUInt32();
    name = msg->GetStr();
    move_mod = msg->GetVector3();
    rotate_mod = msg->GetVector3();
    idle_anim = msg->GetStr();
}

void psMovementInfoMessage::GetMove(uint32 &id, const char* &name, csVector3 &base_move, csVector3 &base_rotate)
{
    id = msg->GetUInt32();
    name = msg->GetStr();
    base_move = msg->GetVector3();
    base_rotate = msg->GetVector3();
}

csString psMovementInfoMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    msgtext.Format("%zu modes and %zu moves",modes,moves);
    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMsgCraftingInfo,MSGTYPE_CRAFT_INFO);


csString psMsgCraftingInfo::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("CraftInfo: %s", craftInfo.GetData());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psTraitChangeMessage,MSGTYPE_CHANGE_TRAIT);

csString psTraitChangeMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Target: %d String: '%s'", target.Unbox(), string.GetDataSafe());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psTutorialMessage,MSGTYPE_TUTORIAL);

csString psTutorialMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Which Message: %d Instructions: '%s'", whichMessage, instrs.GetDataSafe());

    return msgtext;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSketchMessage,MSGTYPE_VIEW_SKETCH);

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMusicalSheetMessage, MSGTYPE_MUSICAL_SHEET);

psMusicalSheetMessage::psMusicalSheetMessage(uint32_t client, uint32_t itemID, bool readOnly, bool play, const char* songTitle, const char* musicalSheet)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + sizeof(bool) + sizeof(bool) + strlen(songTitle) + 1 + strlen(musicalSheet) + 1));

    msg->SetType(MSGTYPE_MUSICAL_SHEET);
    msg->clientnum = client;

    msg->Add(itemID);
    msg->Add(readOnly);
    msg->Add(play);
    msg->Add(songTitle);
    msg->Add(musicalSheet);
}

psMusicalSheetMessage::psMusicalSheetMessage(MsgEntry* me)
{
    itemID = me->GetUInt32();
    readOnly = me->GetBool();
    play = me->GetBool();
    songTitle = me->GetStr();
    musicalSheet = me->GetStr();
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPlaySongMessage, MSGTYPE_PLAY_SONG);

psPlaySongMessage::psPlaySongMessage(uint32_t client, uint32_t songID, bool toPlayer,
                                     const char* instrName, uint32_t scoreSize, const char* musicalScore)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + sizeof(bool) + strlen(instrName) + 1 + sizeof(uint32_t) + scoreSize + 1));

    msg->SetType(MSGTYPE_PLAY_SONG);
    msg->clientnum = client;

    msg->Add(songID);
    msg->Add(toPlayer);
    msg->Add(instrName);
    msg->Add(musicalScore, scoreSize);
}

psPlaySongMessage::psPlaySongMessage(MsgEntry* me)
{
    char* scoreBuffer;
    uint32_t scoreSize = 0;

    songID = me->GetUInt32();
    toPlayer = me->GetBool();
    instrName = me->GetStr();

    scoreBuffer = (char*)me->GetBufferPointerUnsafe(scoreSize);
    musicalScore.Append(scoreBuffer, scoreSize);
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psStopSongMessage, MSGTYPE_STOP_SONG);

psStopSongMessage::psStopSongMessage()
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + sizeof(bool) + sizeof(int8_t)));

    msg->SetType(MSGTYPE_STOP_SONG);
    msg->clientnum = 0;

    msg->Add((uint32_t)0);
    msg->Add(false);
    msg->Add((int8_t)NO_SONG_ERROR);
}

psStopSongMessage::psStopSongMessage(uint32_t client, uint32_t songID, bool toPlayer, int8_t errorCode)
{
    msg.AttachNew(new MsgEntry(sizeof(uint32_t) + sizeof(bool) + sizeof(int8_t)));

    msg->SetType(MSGTYPE_STOP_SONG);
    msg->clientnum = client;

    msg->Add(songID);
    msg->Add(toPlayer);
    msg->Add(errorCode);
}

psStopSongMessage::psStopSongMessage(MsgEntry* me)
{
    songID = me->GetUInt32();
    toPlayer = me->GetBool();
    errorCode = me->GetInt8();
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMGStartStopMessage, MSGTYPE_MINIGAME_STARTSTOP);

psMGStartStopMessage::psMGStartStopMessage(uint32_t client, bool start)
{
    msg.AttachNew(new MsgEntry(sizeof(bool)));

    msg->SetType(MSGTYPE_MINIGAME_STARTSTOP);
    msg->clientnum = client;

    msg->Add(start);
}

psMGStartStopMessage::psMGStartStopMessage(MsgEntry* me)
{
    msgStart = me->GetBool();
}

csString psMGStartStopMessage::ToString(NetBase::AccessPointers* /* accessPointers */)
{
    csString msgText;
    msgText.AppendFmt("Start: %s", msgStart ? "Yes" : "No");
    return msgText;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMGBoardMessage, MSGTYPE_MINIGAME_BOARD);

psMGBoardMessage::psMGBoardMessage(uint32_t client, uint8_t counter,
                                   uint32_t gameID, uint16_t options, int8_t cols, int8_t rows, uint8_t* layout,
                                   uint8_t numOfPieces, uint8_t piecesSize,
                                   uint8_t* pieces)
    : msgLayout(0)
{
    // We need tiles/2 number of bytes and one extra byte for odd number of tiles
    int layoutSize = cols * rows / 2;
    if(cols * rows % 2 != 0)
        layoutSize++;

    msg.AttachNew(new MsgEntry(
                      sizeof(uint8_t) +       // counter
                      sizeof(uint32_t) +      // game ID
                      sizeof(uint16_t) +      // options
                      sizeof(int8_t) +        // cols
                      sizeof(int8_t) +        // rows
                      sizeof(uint32_t) +      // number of bytes in layout
                      layoutSize +            // layout
                      sizeof(uint8_t) +       // number of available pieces
                      sizeof(uint32_t) +      // number of bytes in the pieces array
                      piecesSize));            // available pieces

    msg->SetType(MSGTYPE_MINIGAME_BOARD);
    msg->clientnum = client;

    msg->Add(counter);
    msg->Add(gameID);
    msg->Add(options);
    msg->Add(cols);
    msg->Add(rows);
    msg->Add(layout, layoutSize);
    msg->Add(numOfPieces);
    msg->Add(pieces, piecesSize);

}

psMGBoardMessage::psMGBoardMessage(MsgEntry* me)
    : msgLayout(0)
{
    msg = me;

    msgCounter = msg->GetUInt8();
    msgGameID = msg->GetUInt32();
    msgOptions = msg->GetUInt16();
    msgCols = msg->GetInt8();
    msgRows = msg->GetInt8();

    uint32_t size = 0;
    msgLayout = (uint8_t*)msg->GetBufferPointerUnsafe(size);

    msgNumOfPieces = msg->GetInt8();
    msgPieces = (uint8_t*)msg->GetBufferPointerUnsafe(size);
}

bool psMGBoardMessage::IsNewerThan(uint8_t oldCounter)
{
    return (uint8_t)(msgCounter-oldCounter) <= 127;
}

csString psMGBoardMessage::ToString(NetBase::AccessPointers* /* accessPointers */)
{
    csString msgText;
    msgText.AppendFmt("GameID: %u", msgGameID);
    msgText.AppendFmt(" Options: 0x%04X", msgOptions);
    msgText.AppendFmt(" Rows: %d", msgRows);
    msgText.AppendFmt(" Cols: %d", msgCols);
    msgText.Append(" Layout: ");
    if(msgLayout)
    {
        int layoutSize = msgCols * msgRows / 2;
        if(msgCols * msgRows % 2 != 0)
            layoutSize++;
        for(int i = 0; i < layoutSize; i++)
            msgText.AppendFmt("%02X", msgLayout[i]);
    }
    else
    {
        msgText.Append("<default>");
    }
    msgText.Append(" Pieces: %d ", msgNumOfPieces);
    if(msgNumOfPieces > 0)
    {
        for(int i = 0; i < msgNumOfPieces; i++)
            msgText.AppendFmt("%02X", msgPieces[i]);
    }
    else
    {
        msgText.Append("<default>");
    }
    return msgText;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psEntranceMessage, MSGTYPE_ENTRANCE);

psEntranceMessage::psEntranceMessage(EID entranceID)
{
    msg.AttachNew(new MsgEntry(sizeof(int32_t)));
    msg->SetType(MSGTYPE_ENTRANCE);
    msg->clientnum = 0;

    msg->Add(entranceID.Unbox());
}

psEntranceMessage::psEntranceMessage(MsgEntry* me)
{
    entranceID = EID(me->GetUInt32());
}

csString psEntranceMessage::ToString(NetBase::AccessPointers* /* accessPointers */)
{
    csString msgText;
    msgText.AppendFmt("EntranceID: %u", entranceID.Unbox());
    return msgText;
}


//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMGUpdateMessage, MSGTYPE_MINIGAME_UPDATE);

psMGUpdateMessage::psMGUpdateMessage(uint32_t client, uint8_t counter,
                                     uint32_t gameID, uint8_t numUpdates, uint8_t* updates)
    : msgUpdates(0)
{
    msg.AttachNew(new MsgEntry(
                      sizeof(uint8_t) +       // counter
                      sizeof(uint32_t) +      // game iD
                      sizeof(uint8_t) +       // numUpdates
                      sizeof(uint32_t) +      // number of bytes in updates
                      2*numUpdates));          // updates

    msg->SetType(MSGTYPE_MINIGAME_UPDATE);
    msg->clientnum = client;

    msg->Add(counter);
    msg->Add(gameID);
    msg->Add(numUpdates);
    msg->Add(updates, 2*numUpdates);

}

psMGUpdateMessage::psMGUpdateMessage(MsgEntry* me)
    : msgUpdates(0)
{
    msg = me;

    msgCounter = msg->GetUInt8();
    msgGameID = msg->GetUInt32();
    msgNumUpdates = msg->GetUInt8();

    uint32_t size = 0;
    msgUpdates = (uint8_t*)msg->GetBufferPointerUnsafe(size);
}

bool psMGUpdateMessage::IsNewerThan(uint8_t oldCounter)
{
    return (uint8_t)(msgCounter-oldCounter) <= 127;
}

csString psMGUpdateMessage::ToString(NetBase::AccessPointers* /* accessPointers */)
{
    csString msgText;
    msgText.AppendFmt("GameID: %u", msgGameID);
    msgText.AppendFmt(" NumUpdates: %u", msgNumUpdates);
    msgText.Append(" Updates: ");
    if(msgUpdates)
    {
        for(uint8_t i = 0; i < msgNumUpdates; i++)
        {
            int col = (int)((msgUpdates[2*i] & 0xF0) >> 4);
            int row = (int)(msgUpdates[2*i] & 0x0F);
            msgText.AppendFmt("(%d,%d)=%X", col, row, msgUpdates[2*i + 1]);
        }
    }
    else
    {
        msgText.Append("<null>");
    }
    return msgText;
}

//---------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMEventListMessage, MSGTYPE_GMEVENT_LIST);

psGMEventListMessage::psGMEventListMessage()
{
    msg = NULL;
    valid = false;
}

psGMEventListMessage::psGMEventListMessage(MsgEntry* msg)
{
    if(!msg)
        return;

    gmEventsXML = msg->GetStr();
    valid =!(msg->overrun);
}

void psGMEventListMessage::Populate(csString &gmeventStr, int clientnum)
{
    msg.AttachNew(new MsgEntry(sizeof(int)+gmeventStr.Length()+1));

    msg->SetType(MSGTYPE_GMEVENT_LIST);
    msg->clientnum = clientnum;

    msg->Add(gmeventStr);

    valid=!(msg->overrun);
}

csString psGMEventListMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("XML: \'%s\'", gmEventsXML.GetDataSafe());

    return msgtext;
}
//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psGMEventInfoMessage,MSGTYPE_GMEVENT_INFO);

psGMEventInfoMessage::psGMEventInfoMessage(int cnum, int cmd, int id, const char* name, const char* info, bool Evaluatable)
{
    if(name)
    {
        csString escpxml_info = EscpXML(info);
        xml.Format("<QuestNotebook><Description text=\"%s\"/></QuestNotebook>",escpxml_info.GetData());
    }
    else if(cmd == CMD_EVAL && info)
        xml = info;
    else
        xml.Clear();

    msg.AttachNew(new MsgEntry(sizeof(int32_t) + 2*sizeof(uint8_t) + xml.Length() + 1));

    msg->SetType(MSGTYPE_GMEVENT_INFO);
    msg->clientnum = cnum;

    msg->Add((uint8_t) cmd);

    if(cmd == CMD_QUERY || cmd == CMD_DISCARD)
    {
        msg->Add((int32_t) id);
    }
    else if(cmd == CMD_INFO)
    {
        msg->Add(xml);
        msg->Add((uint8_t) Evaluatable);
    }
    else if(cmd == CMD_EVAL)
    {
        msg->Add((int32_t) id);
        msg->Add(xml);
    }
    msg->ClipToCurrentSize();

    valid=!(msg->overrun);
}

psGMEventInfoMessage::psGMEventInfoMessage(MsgEntry* msg)
{
    command = msg->GetUInt8();
    if(command == CMD_QUERY || command == CMD_DISCARD)
        id = msg->GetInt32();
    else if(command == CMD_INFO)
    {
        xml = msg->GetStr();
        Evaluatable = msg->GetBool();
    }
    else if(command == CMD_EVAL)
    {
        id = msg->GetInt32();
        xml = msg->GetStr();
    }

    valid=!(msg->overrun);
}


csString psGMEventInfoMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Command: %d", command);
    if(command == CMD_QUERY || command == CMD_DISCARD)
        msgtext.AppendFmt(" Id: %d", id);
    else if(command == CMD_INFO)
        msgtext.AppendFmt(" XML: '%s'", xml.GetDataSafe());

    return msgtext;
}

//-----------------------------------------------------------------------------


psFactionMessage::psFactionMessage(int cnum, int cmd)
{
    this->client = cnum;
    this->cmd = cmd;

}


psFactionMessage::psFactionMessage(MsgEntry* message)
{
    cmd         = message->GetInt8();
    int facts   = message->GetInt32();

    for(int z = 0; z < facts; z++)
    {
        psFactionMessage::FactionPair* fp = new psFactionMessage::FactionPair;
        fp->faction = message->GetStr();
        fp->rating  = message->GetInt32();

        factionInfo.Push(fp);
    }
}


void psFactionMessage::AddFaction(csString factionName, int rating)
{
    psFactionMessage::FactionPair* pair = new psFactionMessage::FactionPair;
    pair->faction = factionName;
    pair->rating = rating;

    factionInfo.Push(pair);
}


void psFactionMessage::BuildMsg()
{
    size_t size = sizeof(uint8_t)+sizeof(int32_t);

    for(size_t z = 0; z < factionInfo.GetSize(); z++)
    {
        size += factionInfo[z]->faction.Length()+1;
        size += sizeof(int32_t);
    }

    msg.AttachNew(new MsgEntry(size));
    msg->SetType(MSGTYPE_FACTION_INFO);
    msg->clientnum = client;

    msg->Add((uint8_t)cmd);
    msg->Add((int32_t)factionInfo.GetSize());

    for(size_t i = 0; i < factionInfo.GetSize(); i++)
    {
        msg->Add(factionInfo[i]->faction);
        msg->Add((int32_t)factionInfo[i]->rating);
    }

    valid=!(msg->overrun);
}

csString psFactionMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    msgtext.AppendFmt("Command: %d", cmd);
    return msgtext;
}

PSF_IMPLEMENT_MSG_FACTORY(psFactionMessage, MSGTYPE_FACTION_INFO);






//---------------------------------------------------------------------------

/* TODO: When all messages are converted to use PSF_
   this case statement should be deleted. Possibly
   replace with a new function in the PSF that return
   the message type in a string instead of looping
   through the factory list.
 */
csString GetMsgTypeName(int msgType)
{
    return psfMsgTypeName(msgType);
}

void DecodeMessage(MsgEntry* me, NetBase::AccessPointers* accessPointers, bool filterhex, csString &msgtext, int &filterNumber)
{
    MsgEntry msg(me); // Take a copy to make sure we dont destroy the message.
    // Can't do this const since current pointers are modified
    // when parsing messages.
    psMessageCracker* cracker = NULL;


    csString msgname = GetMsgTypeName(me->bytes->type).GetDataSafe();
    msgname.AppendFmt("(%d)",me->bytes->type);

    msgtext.AppendFmt("%7d %-32s %c %8d %4d",csGetTicks(),
                      msgname.GetData(),
                      ((me->priority&PRIORITY_MASK)==PRIORITY_LOW?'L':'H'),
                      me->clientnum,me->bytes->size);


    // First print the hex of the message if not filtered
    if(!filterhex)
    {
        msgtext.Append(" : ");
        size_t size = me->bytes->GetSize();

        for(size_t i = 0; i < size; i++)
        {
            msgtext.AppendFmt(" %02X",(unsigned char)me->bytes->payload[i]);
        }
    }

    // Than get the cracker and print the decoded message from the ToString function.
    cracker = psfCreateMsg(me->bytes->type,&msg,accessPointers);
    if(cracker)
    {
        msgtext.Append(" > ");
        msgtext.Append(cracker->ToString(accessPointers));

        filterNumber = cracker->filterNumber;

        delete cracker;
    }
}

typedef struct
{
    int msgtype;
    csString msgtypename;
    psfMsgFactoryFunc factoryfunc;
} MsgFactoryItem;

typedef struct
{
    csPDelArray<MsgFactoryItem> messages;
} MsgFactory;

static MsgFactory* msgfactory = NULL;

class MsgFactoryImpl
{
public:
    MsgFactoryImpl()
    {
        if(msgfactory == NULL) msgfactory = new MsgFactory;
    }

    virtual ~MsgFactoryImpl()
    {
        if(msgfactory) delete msgfactory;
        msgfactory = NULL;
    }
} psfMsgFactoryImpl;

MsgFactoryItem* psfFindFactory(int msgtype)
{
    for(size_t n = 0; n < msgfactory->messages.GetSize(); n++)
    {
        if(msgfactory->messages[n]->msgtype == msgtype)
            return msgfactory->messages[n];
    }
    return NULL;
}

MsgFactoryItem* psfFindFactory(const char* msgtypename)
{
    for(size_t n = 0; n < msgfactory->messages.GetSize(); n++)
    {
        if(msgfactory->messages[n]->msgtypename == msgtypename)
            return msgfactory->messages[n];
    }
    return NULL;
}


void psfRegisterMsgFactoryFunction(psfMsgFactoryFunc factoryfunc, int msgtype, const char* msgtypename)
{
    if(msgfactory == NULL) msgfactory = new MsgFactory;

    MsgFactoryItem* factory = psfFindFactory(msgtype);
    if(factory)
    {
        Error2("Multiple factories for %s",msgtypename);
        return;
    }

    MsgFactoryItem* newfac = new MsgFactoryItem;
    newfac->factoryfunc = factoryfunc;
    newfac->msgtype = msgtype;
    newfac->msgtypename = msgtypename;

    msgfactory->messages.Push(newfac);
}

void psfUnRegisterMsgFactories(void)
{
    if (msgfactory)
        msgfactory->messages.DeleteAll();
}

psMessageCracker* psfCreateMsg(int msgtype,
                               MsgEntry* me,
                               NetBase::AccessPointers* accessPointers)
{
    if(!msgfactory) return NULL;

    MsgFactoryItem* factory = psfFindFactory(msgtype);
    if(factory)
        return factory->factoryfunc(me,accessPointers);

    return NULL;
}

csString psfMsgTypeName(int msgtype)
{
    if(!msgfactory) return "No factory";

    MsgFactoryItem* factory = psfFindFactory(msgtype);
    if(factory)
        return factory->msgtypename;

    return csString().Format("unknown (type=%i)", msgtype);
}

int psfMsgType(const char* msgtypename)
{
    if(!msgfactory) return -1;

    MsgFactoryItem* factory = psfFindFactory(msgtypename);
    if(factory)
        return factory->msgtype;

    return -1;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psSequenceMessage,MSGTYPE_SEQUENCE);

psSequenceMessage::psSequenceMessage(int cnum, const char* name, int cmd, int count)
{
    msg.AttachNew(new MsgEntry(strlen(name) + 1 + sizeof(uint8_t) + sizeof(int32_t)));

    msg->SetType(MSGTYPE_SEQUENCE);
    msg->clientnum = cnum;

    msg->Add(name);
    msg->Add((uint8_t)cmd);
    msg->Add((int32_t) count);
    msg->ClipToCurrentSize();

    valid=!(msg->overrun);
}

psSequenceMessage::psSequenceMessage(MsgEntry* msg)
{
    name    = msg->GetStr();
    command = msg->GetUInt8();
    count   = msg->GetInt32();

    valid=!(msg->overrun);
}


csString psSequenceMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    msgtext.AppendFmt("Sequence: '%s' Cmd: %d Count: %d",
                      name.GetDataSafe(),command,count);

    return msgtext;
}

//-----------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psPlaySoundMessage, MSGTYPE_PLAYSOUND);

psPlaySoundMessage::psPlaySoundMessage(uint32_t clientnum, csString /*snd*/)
{
    msg.AttachNew(new MsgEntry(sound.Length()+1));
    msg->SetType(MSGTYPE_PLAYSOUND);
    msg->clientnum = clientnum;
    msg->Add(sound);
    valid=!(msg->overrun);
}

psPlaySoundMessage::psPlaySoundMessage(MsgEntry* msg)
{
    sound = msg->GetStr();
    valid=!(msg->overrun);
}

csString psPlaySoundMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext("Message Sound:");
    return msgtext+sound;
}

//-----------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psCharCreateCPMessage,MSGTYPE_CHAR_CREATE_CP);

psCharCreateCPMessage::psCharCreateCPMessage(uint32_t client, int32_t rID, int32_t CPVal)
{
    msg.AttachNew(new MsgEntry(100));
    msg->SetType(MSGTYPE_CHAR_CREATE_CP);
    msg->clientnum = client;
    msg->Add(rID);
    msg->Add(CPVal);
    msg->ClipToCurrentSize();
    valid = !(msg->overrun);
}

psCharCreateCPMessage::psCharCreateCPMessage(MsgEntry* message)
{
    if(!message)
    {
        return;
    }
    raceID = message->GetUInt32();
    CPValue = message->GetUInt32();
}

csString psCharCreateCPMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    //msgtext.AppendFmt("Race '%i': has '%i' cppoints", raceID, CPValue);
    return msgtext;
}

PSF_IMPLEMENT_MSG_FACTORY(psCharIntroduction,MSGTYPE_INTRODUCTION);

psCharIntroduction::psCharIntroduction()
{
    msg.AttachNew(new MsgEntry(100));
    msg->SetType(MSGTYPE_INTRODUCTION);
    msg->clientnum = 0;
    msg->ClipToCurrentSize();
    valid = !(msg->overrun);
}

psCharIntroduction::psCharIntroduction(MsgEntry* /*message*/)
{
}

csString psCharIntroduction::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    return msgtext;
}

PSF_IMPLEMENT_MSG_FACTORY(psCachedFileMessage,MSGTYPE_CACHEFILE);

psCachedFileMessage::psCachedFileMessage(uint32_t client, uint8_t sequence, const char* pathname, iDataBuffer* contents)
{
    printf("::Building cached file message for '%s', sequence %d, size %zu.\n",
           pathname, sequence, contents?contents->GetSize():0);

    // We send the hash along with it to save as the filename on the client
    if(pathname[0] == '(')   // timestamp always starts with '('
    {
        hash = csMD5::Encode(pathname).HexString();
        printf("::Hashed %s to %s.\n", pathname, hash.GetData());
    }
    else
        hash = pathname;

    uint32_t size = contents ? (uint32_t)contents->GetSize() : 0;
    msg.AttachNew(new MsgEntry(hash.Length()+1 + size + sizeof(uint32_t),PRIORITY_HIGH,sequence));

    msg->SetType(MSGTYPE_CACHEFILE);
    msg->clientnum = client;
    msg->Add(hash);
    if(contents)
        msg->Add(contents->GetData(), size);
    else
        msg->Add(size);
}

psCachedFileMessage::psCachedFileMessage(MsgEntry* me)
{
    hash = me->GetStr();
    printf("::Received cached message for file '%s'.\n", hash.GetDataSafe());

    uint32_t size=0;
    char* ptr = (char*)me->GetBufferPointerUnsafe(size);
    if(ptr)
    {
        databuf.AttachNew(new csDataBuffer (ptr, size, false));
    }
}

csString psCachedFileMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    return csString("not implemented");
}

PSF_IMPLEMENT_MSG_FACTORY(psDialogMenuMessage,MSGTYPE_DIALOG_MENU);

psDialogMenuMessage::psDialogMenuMessage()
{
    valid = false;
}

psDialogMenuMessage::psDialogMenuMessage(MsgEntry* me)
{
    xml = me->GetStr();
}

void psDialogMenuMessage::AddResponse(uint32_t id, const csString &menuText,
                                      const csString &triggerText, uint32_t flags)
{
    psDialogMenuMessage::DialogResponse new_response;

    csString escTriggerText(triggerText);
    if(triggerText.GetAt(0) == '<')   // escape xml characters before putting in xml
    {
        escTriggerText.ReplaceAll("<","&lt;");
        escTriggerText.ReplaceAll(">","&gt;");
    }

    new_response.id          = id;
    new_response.menuText    = menuText;
    new_response.triggerText = escTriggerText;
    new_response.flags       = flags;

    responses.Push(new_response);
}

void psDialogMenuMessage::BuildMsg(int clientnum)
{
    xml = "<dlgmenu><options>";

    int counter=1;
    for(size_t i = 0; i < responses.GetSize(); i++)
    {
        csString choice = responses[i].menuText.GetData();
        if(choice.GetAt(0) == 'h' && choice.GetAt(1) == ':')  // heading tag
        {
            choice.DeleteAt(0,2); // take out tag
            xml.AppendFmt("<row heading=\"1\"><text>%s</text>", choice.GetData());
        }
        else
        {
            xml.AppendFmt("<row><text>%d. %s</text>",counter++, responses[i].menuText.GetData());
        }
        xml.AppendFmt("<trig>%s</trig></row>",responses[i].triggerText.GetData());
    }
    xml += "</options></dlgmenu>";

    msg.AttachNew(new MsgEntry(xml.Length() + 1));
    msg->SetType(MSGTYPE_DIALOG_MENU);
    msg->clientnum = clientnum;

    msg->Add(xml);

    valid = !(msg->overrun);
}

csString psDialogMenuMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;
    
    msgtext.AppendFmt("XML: %s",xml.GetDataSafe());

    return msgtext;
}

PSF_IMPLEMENT_MSG_FACTORY(psSimpleStringMessage,MSGTYPE_SIMPLE_STRING);


psSimpleStringMessage::psSimpleStringMessage(uint32_t client,MSG_TYPES type, const char* string)
{
    msg.AttachNew(new MsgEntry(strlen(string)+1));
    msg->SetType(type);
    msg->clientnum = client;
    msg->Add(string);
    valid=!(msg->overrun);
}

psSimpleStringMessage::psSimpleStringMessage(MsgEntry* me)
{
    str   = me->GetStr();
    valid = !(me->overrun);
}


PSF_IMPLEMENT_MSG_FACTORY_ACCESS_POINTER(psSimpleRenderMeshMessage, MSGTYPE_SIMPLE_RENDER_MESH);


psSimpleRenderMeshMessage::psSimpleRenderMeshMessage(uint32_t client, NetBase::AccessPointers* accessPointers, const char* name, uint16_t index, uint16_t count, const iSector* sector, const csSimpleRenderMesh &simpleRenderMesh)
{
    msg.AttachNew(new MsgEntry(MAX_MESSAGE_SIZE));
    msg->SetType(MSGTYPE_SIMPLE_RENDER_MESH);
    msg->clientnum = client;

    msg->Add(sector, accessPointers->msgstrings, accessPointers->msgstringshash);
    msg->Add(name);
    msg->Add(index);
    msg->Add(count);
    msg->Add(simpleRenderMesh.alphaType.autoAlphaMode);
    if(simpleRenderMesh.alphaType.autoAlphaMode == false)
    {
        msg->Add((uint32_t)simpleRenderMesh.alphaType.alphaType);
    }
    else
    {
        // TODO: autoModeTexture
    }
    msg->Add((uint32_t)simpleRenderMesh.indexCount);
    for(size_t i = 0; i < simpleRenderMesh.indexCount; i++)
    {
        msg->Add((uint32_t)simpleRenderMesh.indices[i]);
    }
    msg->Add((uint32_t)simpleRenderMesh.meshtype);
    msg->Add((uint32_t)simpleRenderMesh.mixmode);
    msg->Add((uint32_t)simpleRenderMesh.vertexCount);
    msg->Add((bool)(simpleRenderMesh.colors != NULL));
    for(size_t i = 0; i < simpleRenderMesh.vertexCount; i++)
    {
        msg->Add(simpleRenderMesh.vertices[i]);
        if(simpleRenderMesh.colors)
        {
            msg->Add(simpleRenderMesh.colors[i]);
        }
    }
    msg->Add((uint32_t)simpleRenderMesh.z_buf_mode);

    msg->ClipToCurrentSize();

    valid=!(msg->overrun);
}

psSimpleRenderMeshMessage::psSimpleRenderMeshMessage(MsgEntry* me, NetBase::AccessPointers* accessPointers)
{
    sector = me->GetSector(accessPointers->msgstrings, accessPointers->msgstringshash, accessPointers->engine);
    name = me->GetStr();
    index = me->GetUInt16();
    count = me->GetUInt16();
    simpleRenderMesh.alphaType.autoAlphaMode = me->GetBool();
    if(simpleRenderMesh.alphaType.autoAlphaMode == false)
    {
        simpleRenderMesh.alphaType.alphaType = (csAlphaMode::AlphaType)me->GetUInt32();
    }
    else
    {
        // TODO: autoModeTexture
    }
    simpleRenderMesh.indexCount = (uint)me->GetUInt32();
    simpleRenderMesh.indices = new uint[simpleRenderMesh.indexCount];
    for(size_t i = 0; i < simpleRenderMesh.indexCount; i++)
    {
        msg->Add((uint32_t)simpleRenderMesh.indices[i]);
    }
    simpleRenderMesh.meshtype = (csRenderMeshType)me->GetUInt32();
    simpleRenderMesh.mixmode = (uint)me->GetUInt32();
    simpleRenderMesh.vertexCount = (uint)me->GetUInt32();
    bool hasColors = me->GetBool();
    csVector3* vertices = new csVector3[simpleRenderMesh.vertexCount];
    simpleRenderMesh.vertices = vertices;
    csVector4* colors = NULL;
    if(hasColors)
    {
        colors = new csVector4[simpleRenderMesh.vertexCount];
        simpleRenderMesh.colors = colors;
    }
    for(size_t i = 0; i < simpleRenderMesh.vertexCount; i++)
    {
        vertices[i] = me->GetVector3();
        if(hasColors)
        {
            colors[i] = me->GetVector4();
        }
    }
    simpleRenderMesh.z_buf_mode = (csZBufMode)me->GetUInt32();

    valid = !(me->overrun);
}

csString psSimpleRenderMeshMessage::ToString(NetBase::AccessPointers* accessPointers)
{
    csString msgtext;

    msgtext.AppendFmt("sector: %s meshtype: %d vertexcount: %d",
                      sector ? sector->QueryObject()->GetName():"(null)",
                      simpleRenderMesh.meshtype,simpleRenderMesh.vertexCount);
    for(size_t i = 0; i < simpleRenderMesh.vertexCount; i++)
    {
        msgtext.AppendFmt(" %zu: v: %s",i,toString(simpleRenderMesh.vertices[i]).GetData());
        if(simpleRenderMesh.colors)
        {
            msgtext.AppendFmt(" c: %s",toString(simpleRenderMesh.colors[i]).GetData());
        }
    }

    return msgtext;
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psMechanismActivateMessage, MSGTYPE_MECS_ACTIVATE);

psMechanismActivateMessage::psMechanismActivateMessage(uint32_t client, const char* meshName,
        const char* move, const char* rot)
{
    msg.AttachNew(new MsgEntry(strlen(meshName) + 1 + strlen(move) + 1 + strlen(rot) + 1));

    msg->SetType(MSGTYPE_MECS_ACTIVATE);
    msg->clientnum = client;

    msg->Add(meshName);
    msg->Add(move);
    msg->Add(rot);
}

psMechanismActivateMessage::psMechanismActivateMessage(MsgEntry* msg)
{
    meshName = msg->GetStr();
    move = msg->GetStr();
    rot = msg->GetStr();
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psOrderedMessage,MSGTYPE_ORDEREDTEST);

psOrderedMessage::psOrderedMessage(uint32_t client, int valueToSend, int sequenceNumber)
{
    printf("Creating orderedMessage with sequence number %d, value %d.\n", sequenceNumber, valueToSend);

    msg.AttachNew(new MsgEntry(sizeof(uint32_t),PRIORITY_HIGH, (uint8_t)sequenceNumber));
    msg->SetType(MSGTYPE_ORDEREDTEST);
    msg->clientnum = client;
    msg->Add(valueToSend);
    valid=!(msg->overrun);
}

psOrderedMessage::psOrderedMessage(MsgEntry* me)
{
    value   = me->GetUInt32();
    valid = !(me->overrun);
}

csString psOrderedMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    return csString("not implemented");
}

//--------------------------------------------------------------------------

PSF_IMPLEMENT_MSG_FACTORY(psHiredNPCScriptMessage,MSGTYPE_HIRED_NPC_SCRIPT);

psHiredNPCScriptMessage::psHiredNPCScriptMessage(uint32_t client, uint8_t command, EID hiredEID):
    command(command),hiredEID(hiredEID),choice(false),workLocationValid(false)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+sizeof(uint32_t)));
    msg->SetType(MSGTYPE_HIRED_NPC_SCRIPT);
    msg->clientnum = client;

    msg->Add(command);
    msg->Add((uint32_t)hiredEID.Unbox());

    valid=!(msg->overrun);
}

psHiredNPCScriptMessage::psHiredNPCScriptMessage(uint32_t client, uint8_t command, EID hiredEID, bool choice, const char* errorMsg):
    command(command),hiredEID(hiredEID),choice(choice),workLocationValid(false),errorMessage(errorMsg)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+sizeof(uint32_t)+sizeof(bool)+(strlen(errorMsg)+1)));
    msg->SetType(MSGTYPE_HIRED_NPC_SCRIPT);
    msg->clientnum = client;

    msg->Add(command);
    msg->Add((uint32_t)hiredEID.Unbox());
    msg->Add(choice);
    msg->Add(errorMsg);

    valid=!(msg->overrun);
}

psHiredNPCScriptMessage::psHiredNPCScriptMessage(uint32_t client, uint8_t command, EID hiredEID, const char* script):
    command(command),hiredEID(hiredEID),choice(false),workLocationValid(false),script(script)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+sizeof(uint32_t)+(strlen(script)+1)));
    msg->SetType(MSGTYPE_HIRED_NPC_SCRIPT);
    msg->clientnum = client;

    msg->Add(command);
    msg->Add((uint32_t)hiredEID.Unbox());
    msg->Add(script);

    valid=!(msg->overrun);
}

psHiredNPCScriptMessage::psHiredNPCScriptMessage(uint32_t client, uint8_t command, EID hiredEID,
                                                 const char* locationType, const char* locationName):
    command(command),hiredEID(hiredEID),choice(false),workLocationValid(false)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+sizeof(uint32_t)+
                               (strlen(locationName)+1)+
                               (strlen(locationType)+1)));
    msg->SetType(MSGTYPE_HIRED_NPC_SCRIPT);
    msg->clientnum = client;

    msg->Add(command);
    msg->Add((uint32_t)hiredEID.Unbox());
    msg->Add(locationType);
    msg->Add(locationName);

    valid=!(msg->overrun);
}

psHiredNPCScriptMessage::psHiredNPCScriptMessage(uint32_t client, uint8_t command, EID hiredEID,
                                                 const char* workLocation, bool workLocationValid,
                                                 const char* script):
    command(command),hiredEID(hiredEID),choice(false),workLocation(workLocation),
    workLocationValid(workLocationValid),script(script)
{
    msg.AttachNew(new MsgEntry(sizeof(uint8_t)+sizeof(uint32_t)+
                               (strlen(workLocation)+1)+sizeof(bool)+
                               (strlen(script)+1)));
    msg->SetType(MSGTYPE_HIRED_NPC_SCRIPT);
    msg->clientnum = client;

    msg->Add(command);
    msg->Add((uint32_t)hiredEID.Unbox());
    msg->Add(workLocation);
    msg->Add(workLocationValid);
    msg->Add(script);

    valid=!(msg->overrun);
}

psHiredNPCScriptMessage::psHiredNPCScriptMessage(MsgEntry* me):
    choice(false),workLocationValid(false)
{
    command  = me->GetUInt8();
    hiredEID = me->GetUInt32();
    
    switch (command)
    {
    case REQUEST_REPLY:
        workLocation      = me->GetStr();
        workLocationValid = me->GetBool();
        script            = me->GetStr();
        break;
    case VERIFY:
        script = me->GetStr();
        break;
    case VERIFY_REPLY:
    case WORK_LOCATION_RESULT:
    case CHECK_WORK_LOCATION_RESULT:
    case COMMIT_REPLY:
        choice = me->GetBool();
        errorMessage = me->GetStr();
        break;
    case WORK_LOCATION_UPDATE:
        workLocation = me->GetStr();
        break;
    case CHECK_WORK_LOCATION:
        locationType = me->GetStr();
        locationName = me->GetStr();
        break;
    }

    valid = !(me->overrun);
}

csString psHiredNPCScriptMessage::ToString(NetBase::AccessPointers* /*accessPointers*/)
{
    csString msgtext;

    switch (command)
    {
    case CHECK_WORK_LOCATION:
        msgtext.AppendFmt("Cmd: CHECK_WORK_LOCATION Hired: %s LocationType: %s LocationName: %s",
                          ShowID(hiredEID), locationType.GetDataSafe(),locationName.GetDataSafe());
        break;
    case CHECK_WORK_LOCATION_RESULT:
        msgtext.AppendFmt("Cmd: CHECK_WORK_LOCATION_RESULT Hired: %s Choice: %s ErrorMessage: %s",
                          ShowID(hiredEID), choice?"True":"False", errorMessage.GetDataSafe());
        break;
    case COMMIT_REPLY:
        msgtext.AppendFmt("Cmd: COMMIT_REPLY Hired: %s Choice: %s ErrorMessage: %s",
                          ShowID(hiredEID), choice?"True":"False", errorMessage.GetDataSafe());
        break;
    case REQUEST:
        msgtext.AppendFmt("Cmd: REQUEST Hired: %s",
                          ShowID(hiredEID));
        break;
    case REQUEST_REPLY:
        msgtext.AppendFmt("Cmd: REQUEST_REPLY Hired: %s WorkLocation: %s WorkLocationValid: %s Script: %s",
                          ShowID(hiredEID), workLocation.GetDataSafe(), workLocationValid?"True":"False",
                          script.GetDataSafe());
        break;
    case VERIFY:
        msgtext.AppendFmt("Cmd: VERIFY Hired: %s Script: %s",
                          ShowID(hiredEID), script.GetDataSafe());
        break;
    case VERIFY_REPLY:
        msgtext.AppendFmt("Cmd: VERIFY_REPLY Hired: %s Choice: %s ErrorMessage: %s",
                          ShowID(hiredEID), choice?"True":"False", errorMessage.GetDataSafe());
        break;
    case COMMIT:
        msgtext.AppendFmt("Cmd: COMMIT Hired: %s",
                          ShowID(hiredEID));
        break;
    case WORK_LOCATION:
        msgtext.AppendFmt("Cmd: WORK_LOCATION Hired: %s",
                          ShowID(hiredEID));
        break;
    case WORK_LOCATION_RESULT:
        msgtext.AppendFmt("Cmd: WORK_LOCATION_RESULT Hired: %s Choice: %s",
                          ShowID(hiredEID), choice?"True":"False");
        break;
    case WORK_LOCATION_UPDATE:
        msgtext.AppendFmt("Cmd: WORK_LOCATION_UPDATE Hired: %s WorkLocation: %s",
                          ShowID(hiredEID), workLocation.GetDataSafe());
        break;
    case CANCEL:
        msgtext.AppendFmt("Cmd: CANCEL Hired: %s",
                          ShowID(hiredEID));
        break;
    }
    
    return msgtext;
}
