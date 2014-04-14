/*
 * serverpinger.cpp - Author: Ondrej Hurt
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

#include <psconfig.h>
#include <iutil/cfgmgr.h>
#include <csutil/sysfunc.h>

#include "net/messages.h"

#include "serverpinger.h"

#define PING_INTERVAL   1000    ///< how long do we wait after receiving a response or a timeout
#define PING_TIMEOUT    4000    ///< when do we give up waiting for response

psServerPinger::psServerPinger(const csString & name, const csString& description, const csString & address, int port, iObjectRegistry * objReg) :
    name         (name),
    description  (description),
    address      (address),
    port         (port),

    connection   (NULL),
    queue        (NULL),

    ping         (9999),
    lastPingTime (0),
    sent         (0),
    lost         (0),
    waiting      (false),
    pingID       (0),
    flags        (0),
    objReg       (objReg)
{
}

bool psServerPinger::Initialize()
{
    connection = new psNetConnection;
    if (!connection->Initialize(objReg))
    {
        Error1("psServerPinger connection failed to initialize");
        delete connection;
        connection = NULL;
        return false;
    }
    return true;
}

bool psServerPinger::Connect()
{
    queue = new MsgQueue();
    connection->AddMsgQueue(queue);
    if (!connection->Connect(address, port))
    {
        Notify2(LOG_CONNECTIONS,"psServerPinger connection couldn't resolve hostname %s", address.GetData());
        ping = -1;
        connection->RemoveMsgQueue(queue);
        delete queue;
        queue = NULL;
        return false;
    }

    return true;
}

void psServerPinger::Disconnect()
{
    if (connection != NULL)
    {
        connection->RemoveMsgQueue(queue);
        delete connection;
        delete queue;
        connection = NULL;
        queue = NULL;
    }
}

psServerPinger::~psServerPinger()
{
    Disconnect();
}

void psServerPinger::DoYourWork()
{
    if(!connection)
        if(!Initialize()) return;

    if(!queue)
        if(!Connect()) return;

    int timeNow = csGetTicks();

    if (waiting)
    {
        csRef<MsgEntry> incoming = queue->Get();
        if (incoming != NULL && incoming->GetType() == MSGTYPE_PING)
        {
            connection->LogMessages('R',incoming);

            psPingMsg response(incoming);
            if (response.id == pingID)
            {
                waiting = false;
                ping = timeNow-lastPingTime;
                lastPingTime = timeNow;
                flags = response.flags;
            }
        }
        else if (timeNow - lastPingTime >= PING_TIMEOUT)
        {
            ping = -1;
            waiting = false;
            lost++;
        }
    }
    else if (timeNow - lastPingTime >= PING_INTERVAL)
    {
        pingID++;
        psPingMsg msg(0, pingID, PINGFLAG_REQUESTFLAGS|PINGFLAG_READY);
        connection->SendMessage(msg.msg);
        lastPingTime = timeNow;
        waiting = true;
        sent++;
    }
}

psServerPinger::SERVERSTATUS psServerPinger::GetStatus()
{
    if (ping == 9999)
    {
        return psServerPinger::INIT;
    }
    else
    {
        if (ping == -1)
        {
            return psServerPinger::FAILED;
        }
        else
        {
            if (GetFlags() & PINGFLAG_SERVERFULL)
            {
                return psServerPinger::FULL;
            }
            else
            {
                if (GetFlags() & PINGFLAG_READY)
                {
                    return psServerPinger::READY;
                }
                else
                {
                    if (GetFlags() & PINGFLAG_HASBEENREADY)
                    {
                        return psServerPinger::LOCKED;
                    }
                    else
                    {
                        return psServerPinger::WAIT;
                    }
                }
            }
        }
    }
    
    return psServerPinger::FAILED;
}
