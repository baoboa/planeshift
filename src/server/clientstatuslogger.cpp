/*
 * clientstatuslogger.cpp - Author: Dewitt Colclough <dewitt@twcny.rr.com>
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
/** @file clientstatuslogger.cpp */
/** @author Dewitt Colclough */

#include <psconfig.h>


#include "clientstatuslogger.h"
#include "clients.h"
#include "gem.h"
#include "playergroup.h"

#include "csutil/xmltiny.h"
#include "iutil/document.h"
#include "bulkobjects/psguildinfo.h"

/**
*
*/
void ClientStatusLogger::AddBasicNode(iDocumentNode* parent, const char* fieldName,
                                      const char* text)
{
    csRef<iDocumentNode> infoNode;

    infoNode = parent->CreateNodeBefore(CS_NODE_ELEMENT);
    infoNode->SetValue(fieldName);
    infoNode = infoNode->CreateNodeBefore(CS_NODE_TEXT);
    infoNode->SetValue(text);
}

/**
*
*/
void ClientStatusLogger::AddBasicNode(iDocumentNode* parent, const char* fieldName,
                                      int data)
{
    csRef<iDocumentNode> infoNode;

    infoNode = parent->CreateNodeBefore(CS_NODE_ELEMENT);
    infoNode->SetValue(fieldName);
    infoNode = infoNode->CreateNodeBefore(CS_NODE_TEXT);
    infoNode->SetValueAsInt(data);
}

/**
*
*/
csPtr<iDocumentNode> ClientStatusLogger::AddContainerNode(iDocumentNode* parent,
        const char* fieldName)
{
    csRef<iDocumentNode> containerNode;
    containerNode = parent->CreateNodeBefore(CS_NODE_ELEMENT);
    containerNode->SetValue(fieldName);

    return csPtr<iDocumentNode>(containerNode);
}

/**
*
*/
void ClientStatusLogger::LogBasicInfo(Client* client, iDocumentNode* node)
{
    AddBasicNode(node, "PlayerName", client->GetName());
    AddBasicNode(node, "SecurityLevel", client->GetSecurityLevel());
    AddBasicNode(node, "ClientNum", client->GetClientNum());
    AddBasicNode(node, "PlayerID", client->GetPID().Unbox());
}

/**
* @param client
*/
void ClientStatusLogger::LogClientInfo(Client* client)
{
    CS_ASSERT(client);
    CS_ASSERT(client->GetActor());

    if(client == NULL) return;  // errors, don't know how to log these yet so just return
    if(client->GetActor() == NULL) return;

    csRef<iDocumentNode> node = AddContainerNode(statusRootNode, "Client");

    LogBasicInfo(client, node);
    LogGuildInfo(client, node);
    LogConnectionInfo(client, node);

}

/**
*
*/
void ClientStatusLogger::LogConnectionInfo(Client* client, iDocumentNode* node)
{
    csRef<iDocumentNode> connNode = AddContainerNode(node, "Connection");

    // IP address
    csString ipAddr = client->GetIPAddress();
    AddBasicNode(connNode, "IPAddress", ipAddr);

    NetBase::Connection* conn = client->GetConnection();

    // ticks since last packet
    csTicks lastPacket = startTime - conn->lastRecvPacketTime;
    AddBasicNode(connNode, "LastPacket", lastPacket);



}

/**
*
*/
void ClientStatusLogger::LogGuildInfo(Client* client, iDocumentNode* node)
{

    gemActor* actor = client->GetActor();
    psGuildInfo* guild = actor->GetGuild();
    if(guild == NULL) return;  // is this an error or indicates no guild?
    if(guild->GetID() == 0) return;


    csRef<iDocumentNode> guildNode = AddContainerNode(node, "GuildData");

    AddBasicNode(guildNode, "GuildName", guild->GetName().GetData());

    if(psGuildLevel* guildLevel = actor->GetGuildLevel())
    {
        AddBasicNode(guildNode, "GuildTitle", guildLevel->title.GetData());
        AddBasicNode(guildNode, "IsGuildSecret", guild->IsSecret());
    }
}





/**
*  This constructor must always be used.
*/
ClientStatusLogger::ClientStatusLogger(iDocumentNode* statusNode)
    : statusRootNode(statusNode)
{
    startTime = csGetTicks();
}
