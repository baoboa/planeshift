/*
 * connection.cpp 
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
 */

#include <psconfig.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iutil/objreg.h>
#include <csutil/sysfunc.h>

#include "util/pserror.h"
#include "util/log.h"
#include "net/connection.h"
#include "net/messages.h"
#include "net/netpacket.h"

/// check every 3 seconds if we're linkdead
#define LINKCHECK 3000
/// check resend every 200ms
#define RESENDCHECK 200
/// Redisplay network server stats every 60 seconds
#define STATDISPLAYCHECK 60000
/// timeout when the server is considered linkdead
#define LINKDEAD_TIMEOUT    5000

#define LINKDEAD_ATTEMPTS   6    ///< 6 attempts with 5sec timeout gives 30secs

psNetConnection::psNetConnection(int queueLength)
    : NetBase (queueLength)
{
    //@@@ DEBUG
    //    Debug2(LOG_NET, 0,"Creating psNetConnection this=%p!",this);
    //    fflush(stdout);

    server     = NULL;
    object_reg = NULL;

    // Initialize the timeout to 50ms
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
}

psNetConnection::~psNetConnection()
{
    fflush(stdout);
    if (IsReady() || server)
    {
        DisConnect();
    }
}

bool psNetConnection::Initialize(iObjectRegistry* object_reg)
{
    /**
     * The original initialize for the listening socket.
     * Might not need to be run at this stage, connection will do
     * this as well to insure fresh buffers.
     */
    if (!Init())
        return false;

	psNetConnection::object_reg = object_reg;

    return true;
}

bool psNetConnection::Connect(const char *servaddr, int port)
{   
    int err;

    if (IsReady() || server)
        DisConnect();

    //@@@ DEBUG
    //    Debug2(LOG_NET, 0,"psNetConnection::Connect this=%p",this);

    server = new Connection;
#ifdef INCLUDE_IPV6_SUPPORT
    server->addr.sin6_family = AF_INET6;
    server->addr.sin6_port   = htons(port);
#else
    server->addr.sin_family = AF_INET;
    server->addr.sin_port   = htons(port);
#endif
    server->nameAddr = servaddr;
    err = GetIPByName (&server->addr, servaddr);
    if (err)
    {
        delete server;
        server = NULL;
        return false;
    }
    
    shouldRun = true;
    
    /**
     * Reinitialize the socket. If the a socket is already present
     * this will be closed and a new one will be created to insure
     * we don't get old messages that will confuse the communication.
     */
    if (!Init())
    {
        Error1("Couldn't initialize a new socket!");
        return false;
    }

    thread.AttachNew( new CS::Threading::Thread(this, true) );
    if (!thread->IsRunning())
    {
        Error1("Couldn't Start Thread!");
        return false;
    }

    return true;    
}

void psNetConnection::DisConnect()
{
    shouldRun = false;
    if (thread)
    {
        thread->Wait();
        Debug2(LOG_NET,0, "psNetConnection::DisConnect Thread %p supposedly terminated..", (void*)this);
        thread = NULL;
    }
    if (server)
    {
        delete server;
        server = NULL;
    }
    
    // with proper refcounting this should kill all members of the hash
    awaitingack.Empty();
}


psNetConnection::Connection *psNetConnection::GetConnByIP (LPSOCKADDR_IN addr)
{
    if (!server)
        return 0;

    // check if it's the server
#ifdef INCLUDE_IPV6_SUPPORT
    if (server->addr.sin6_port==addr->sin6_port &&
        memcmp(server->addr.sin6_addr.s6_addr,addr->sin6_addr.s6_addr,16)==0)
#else
    if (server->addr.sin_port==addr->sin_port &&
        server->addr.sin_addr.s_addr==addr->sin_addr.s_addr)
#endif
    return server;
   
#ifdef DEBUG
    Error1("Non Server message...");
#endif 
    return 0;
}

psNetConnection::Connection *psNetConnection::GetConnByNum (uint32_t clientnum)
{
#ifdef DEBUG
    if (clientnum!=0)
    Error1 ("clientnum != 0        !?!");
#else
    (void)clientnum; // supress unused var warning
#endif
    return server;
}

bool psNetConnection::HandleUnknownClient (LPSOCKADDR_IN , MsgEntry* )
{
    Error1("Message from outside received... hackint?!?");
    return false;
}

void psNetConnection::Run ()
{
    csTicks currentticks = csGetTicks();
    csTicks lastlinkcheck = currentticks;
    csTicks lastresendcheck = currentticks;
    csTicks laststatdisplay = currentticks;

    // Maximum time to spend in ProcessNetwork
    csTicks maxTime = csMin(csMin(LINKCHECK, RESENDCHECK), STATDISPLAYCHECK);
    csTicks timeout;

    long    lasttotaltransferin=0;
    long    lasttotaltransferout=0;
    long    lasttotalcountin=0;
    long    lasttotalcountout=0;
    float   kbpsout = 0;
    float   kbpsin = 0;

    while ( shouldRun )
    {
        if (!ready)
        {
            csSleep(100);
            continue;
        }

        timeout = csGetTicks() + maxTime;
        ProcessNetwork (timeout);

        currentticks = csGetTicks();

        if (currentticks - lastlinkcheck > LINKCHECK)
        {
            lastlinkcheck = currentticks;
            CheckLinkDead (currentticks);
        }
        if (currentticks - lastresendcheck > RESENDCHECK)
        {
            lastresendcheck = currentticks;
            CheckResendPkts ();
            CheckFragmentTimeouts();
        }
        if (currentticks - laststatdisplay > STATDISPLAYCHECK)
        {
            kbpsin = (float)(totaltransferin - lasttotaltransferin) / (float)(currentticks - laststatdisplay);
            lasttotaltransferin = totaltransferin;

            kbpsout = (float)(totaltransferout - lasttotaltransferout) / (float)(currentticks - laststatdisplay);
            lasttotaltransferout = totaltransferout;

            laststatdisplay = currentticks;

            
            if (pslog::disp_flag[LOG_NET])
            {
                printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                printf("+ Currently using %1.2fKbps out, %1.2fkbps in...\n",
                       kbpsout, kbpsin);
                printf("+ Packets: %ld out ,  %ld in\n",
                       totalcountout-lasttotalcountout,totalcountin-lasttotalcountin);
                printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            }
            lasttotalcountout = totalcountout;
            lasttotalcountin = totalcountin;
        }
    }

    Debug2(LOG_NET, 0, "Run Stopped this=%p\n", (void*)this);
}

void psNetConnection::CheckLinkDead (csTicks currenttime)
{
    if (currenttime - server->lastRecvPacketTime > LINKDEAD_TIMEOUT)
    {
        if (server->heartbeat < LINKDEAD_ATTEMPTS)
        {
            server->heartbeat++;
            psHeartBeatMsg heart((uint32_t)0);
            SendMessage(heart.msg);  // This should cause an ack to update the timestamp
        }
        else
        {
            // Simulate message to self to inform user of quitting.
            psSystemMessage quit(0,MSG_INFO,"Server is not responding, try again in 5 minutes. Exiting PlaneShift...");
            HandleCompletedMessage(quit.msg, server, &server->addr,NULL);
            
            csSleep(1000);

            csString disconnectTextMsg;
            if(server->pcknumin)
            {
                disconnectTextMsg.Format("Server is not responding, try again in 5 minutes. "
                                         "Check http://%s/ page for status.", server->nameAddr.GetData());
            }
            else
            {
                disconnectTextMsg.Format("The server is not running or is not reachable.  "
                                         "Please check http://%s/ or forums for more info.", server->nameAddr.GetData());
            }

            // If no packets received ever, then use -1 to indicate cannot connect 
            psDisconnectMessage msgb(0,server->pcknumin?0:-1, disconnectTextMsg.GetData());
            HandleCompletedMessage(msgb.msg, server, &server->addr,NULL);
        }
    }
    else
    {
        server->heartbeat = 0;
    }
}

void psNetConnection::Broadcast(MsgEntry* me, int, int)
{
    me->clientnum=0;
    SendMessage(me);
}

void psNetConnection::Multicast(MsgEntry* me, const csArray<PublishDestination>& , uint32_t , float )
{
    me->clientnum=0;
    SendMessage(me);
}
