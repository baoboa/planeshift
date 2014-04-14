/*
 * NetManager.cpp by Matze Braun <MatzeBraun@gmx.de>
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
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/pserror.h"
#include "util/serverconsole.h"
#include "util/eventmanager.h"

#include "net/message.h"
#include "net/messages.h"
#include "net/netpacket.h"

#include "bulkobjects/psaccountinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "netmanager.h"
#include "client.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "cachemanager.h"
#include "globals.h"


// Check every 3 seconds for linkdead clients
#define LINKCHECK    3000
// Check for resending every 1000 ms
#define RESENDCHECK    1000
// Redisplay network server stats every 60 seconds
#define STATDISPLAYCHECK 60000



class DelayedMessageSendEvent : public psGameEvent
{
protected:
    bool valid;
    csRef<MsgEntry> myMsg;

public:
    DelayedMessageSendEvent(int delayticks,MsgEntry* msg)
        : psGameEvent(0, delayticks, "DelayedMessageSendEvent")
    {
        valid = true;
        myMsg = msg;
    }
    void CancelEvent()
    {
        valid = false;
    }
    virtual void Trigger()
    {
        if(valid)
        {
            psserver->GetNetManager()->SendMessage(myMsg);
        }
    }
};


NetManager::NetManager()
    : NetBase(1000),stop_network(false)
{
    port=0;
}

NetManager::~NetManager()
{
}

bool NetManager::Initialize(CacheManager* cachemanager, int client_firstmsg, int npcclient_firstmsg, int timeout)
{
    if(!NetBase::Init(false))
        return false;

    NetManager::client_firstmsg = client_firstmsg;
    NetManager::npcclient_firstmsg = npcclient_firstmsg;
    NetManager::timeout = timeout;

    if(!clients.Initialize())
        return false;

    SetMsgStrings(cachemanager->GetMsgStrings(), 0);

    return true;
}

class NetManagerStarter : public CS::Threading::Runnable, public Singleton<NetManagerStarter>
{
    CacheManager* cacheManager;
public:
    NetManager* netManager;
    CS::Threading::Thread* thread;
    CS::Threading::Mutex doneMutex;
    CS::Threading::Condition initDone;
    int client_firstmsg;
    int npcclient_firstmsg;
    int timeout;


    NetManagerStarter(CacheManager* cachemanager, int _client_firstmsg, int _npcclient_firstmsg, int _timeout)
    {
        netManager = NULL;
        thread = NULL;
        cacheManager = cachemanager;
        client_firstmsg = _client_firstmsg;
        npcclient_firstmsg = _npcclient_firstmsg;
        timeout = _timeout;
    }

    void Run()
    {
        {
            CS::Threading::MutexScopedLock lock(doneMutex);
            // construct the netManager is its own thread to avoid wrong warnings of dynamic thread checking via valgrind
            netManager = new NetManager();
            if(!netManager->Initialize(cacheManager, client_firstmsg, npcclient_firstmsg,
                                       timeout))
            {
                Error1("Network thread initialization failed!\nIs there already a server running?");
                delete netManager;
                netManager = NULL;
                initDone.NotifyAll();
                return;
            }
            initDone.NotifyAll();
        }

        /* run the network loop */
        netManager->Run();
    }
};

NetManager* NetManager::Create(CacheManager* cacheManager, int client_firstmsg, int npcclient_firstmsg, int timeout)
{
    NetManagerStarter* netManagerStarter =
        new NetManagerStarter(cacheManager, client_firstmsg, npcclient_firstmsg, timeout);
    netManagerStarter->thread = new CS::Threading::Thread(netManagerStarter);

    // wait for initialization to be finished
    {
        CS::Threading::MutexScopedLock lock(netManagerStarter->doneMutex);
        netManagerStarter->thread->Start();

        if(!netManagerStarter->thread->IsRunning())
        {
            return NULL;
        }

        netManagerStarter->initDone.Wait(netManagerStarter->doneMutex);
    }
    return netManagerStarter->netManager;
}

void NetManager::Destroy()
{
    // Handle stopping and destroying the network thread in the main thread.
    NetManagerStarter* netManagerStarter =
        NetManagerStarter::GetSingletonPtr();
    netManagerStarter->netManager->stop_network = true;
    netManagerStarter->thread->Wait();
    delete netManagerStarter->netManager;
    delete netManagerStarter->thread;
    delete netManagerStarter;
}

bool NetManager::HandleUnknownClient(LPSOCKADDR_IN addr, MsgEntry* me)
{
    psMessageBytes* msg = me->bytes;

    // The first msg from client must be "firstmsg", "alt_first_msg" or "PING"
    if(msg->type!=client_firstmsg && msg->type!=npcclient_firstmsg && msg->type!=MSGTYPE_PING)
        return false;

    if(msg->type == MSGTYPE_PING)
    {
        psPingMsg ping(me);

        if(!(ping.flags & PINGFLAG_REQUESTFLAGS) && !psserver->IsReady())
            return false;

        int flags = 0;
        if(psserver->IsReady()) flags |= PINGFLAG_READY;
        if(psserver->HasBeenReady()) flags |= PINGFLAG_HASBEENREADY;
        if(psserver->IsFull(clients.Count(),NULL)) flags |= PINGFLAG_SERVERFULL;

        // Create the reply to the ping
        psPingMsg pong(0,ping.id,flags);
        pong.msg->msgid = GetRandomID();

        csRef<psNetPacketEntry> pkt;
        pkt.AttachNew(new
                      psNetPacketEntry(pong.msg->priority, 0,
                                       0, 0, (uint32_t) pong.msg->bytes->GetTotalSize(),
                                       (uint16_t) pong.msg->bytes->GetTotalSize(), pong.msg->bytes));

        SendFinalPacket(pkt, addr);
        return false;
    }

    // Don't accept any new clients when not ready, npcclients is accepted
    if(msg->type==client_firstmsg && !psserver->IsReady())
        return false;

    // Create and add the client object to client list
    Client* client = clients.Add(addr);
    if(!client)
        return false;

    csString ipAddr = client->GetIPAddress();
    Debug3(LOG_CONNECTIONS, client->GetClientNum(), "New client %d connected from %s", client->GetClientNum(), ipAddr.GetDataSafe())

    // This is for the accept message that will be sent back
    me->clientnum = client->GetClientNum();

    return true;
}

void NetManager::CheckResendPkts()
{
    // NOTE: Globaliterators on csHash do not retrieve keys contiguously.
    csHash<csRef<psNetPacketEntry>, PacketKey>::GlobalIterator it(awaitingack.GetIterator());
    csRef<psNetPacketEntry> pkt;
    csArray<csRef<psNetPacketEntry> > pkts;
    csArray<Connection*> resentConnections;

    // Connections that should be avoided because we know they are full
    csArray<Connection*> fullConnections;

    csTicks currenttime = csGetTicks();
    unsigned int resentCount = 0;

    while(it.HasNext())
    {
        pkt = it.Next();
        // Check the connection packet timeout
        if(pkt->timestamp + csMin((csTicks)PKTMAXRTO, pkt->RTO) < currenttime)
            pkts.Push(pkt);
    }
    for(size_t i = 0; i < pkts.GetSize(); i++)
    {
#ifdef PACKETDEBUG
        Debug2(LOG_NET,"Resending nonacked HIGH packet (ID %d).\n", pkt->packet->pktid);
#endif
        pkt = pkts.Get(i);

        // re-add to send queue
        csRef<NetPacketQueueRefCount> outqueue = clients.FindQueueAny(pkt->clientnum);
        if(!outqueue)
        {
            awaitingack.Delete(PacketKey(pkt->clientnum, pkt->packet->pktid), pkt);
            continue;
        }

        Connection* connection = GetConnByNum(pkt->clientnum);
        if(connection)
        {
            if(resentConnections.Find(connection) == csArrayItemNotFound)
                resentConnections.Push(connection);
            if(fullConnections.Find(connection) != csArrayItemNotFound)
                continue;
            // This indicates a bug in the netcode.
            if(pkt->RTO == 0)
            {
                Error1("Unexpected 0 packet RTO.");
                abort();
            }
            pkt->RTO *= 2;
            connection->resends++;
        }
        resentCount++;

        pkt->timestamp = currenttime;   // update stamp on packet
        pkt->retransmitted = true;

        /*  The proper way to send a message is to add it to the queue, and then add the queue to the senders.
        *  If you do it the other way around the net thread may remove the queue from the senders before you add the packet.
        *   Yes - this has actually happened!
        */

        /* We store the result here to return as the result of this function to maintain historic functionality.
        *  In actuality a false response does not actually mean no data was added to the queue, just that
        *  not all of the data could be added.
        */
        if(!outqueue->Add(pkt))
        {
            psNetPacket* packet = pkt->packet;
            int type = 0;

            if(packet->offset == 0)
            {
                psMessageBytes* msg = (psMessageBytes*) packet->data;
                type = msg->type;
            }
            Error4("Queue full. Could not add packet with clientnum %d type %s ID %d.\n", pkt->clientnum, type == 0 ? "Fragment" : (const char*)  GetMsgTypeName(type), pkt->packet->pktid);
            fullConnections.Push(connection);
            continue;
        }

        /**
        * The senders list is a list of busy queues.  The SendOut() function
        * in NetBase clears this list each time through.  This saves having
        * to check every single connection each time through.
        */

        if(!senders.Add(outqueue))
            Error1("Senderlist Full!");

        //printf("pkt=%p, pkt->packet=%p\n",pkt,pkt->packet);
        // take out of awaiting ack pool.
        // This does NOT delete the pkt mem block itself.
        if(!awaitingack.Delete(PacketKey(pkt->clientnum, pkt->packet->pktid), pkt))
        {
#ifdef PACKETDEBUG
            Debug2(LOG_NET,"No packet in ack queue :%d\n", pkt->packet->pktid);
#endif
        }
        else if(connection)
            connection->RemoveFromWindow(pkt->packet->GetPacketSize());
    }
    if(resentCount > 0)
    {
        resends[resendIndex] = resentCount;
        resendIndex = (resendIndex + 1) % RESENDAVGCOUNT;

        csTicks timeTaken = csGetTicks() - currenttime;
        if(resendIndex == 1 || timeTaken > 50)
        {
            size_t peakResend = 0;
            float resendAvg = 0.0f;
            // Calculate averages data here
            for(int i = 0; i < RESENDAVGCOUNT; i++)
            {
                resendAvg += resends[i];
                peakResend = csMax(peakResend, resends[i]);
            }
            resendAvg /= RESENDAVGCOUNT;
            csString status;
            if(timeTaken > 50 || pkts.GetSize() > 300)
            {
                status.Format("Resending high priority packets has taken %u time to process, for %u packets on %zu unique connections %zu full connections (Sample clientnum %u/RTO %u. ", timeTaken, resentCount, resentConnections.GetSize(),fullConnections.GetSize(), resentConnections[0]->clientnum, resentConnections[0]->RTO);
                CPrintf(CON_WARNING, "%s\n", (const char*) status.GetData());
            }
            status.AppendFmt("Resending non-acked packet statistics: %g average resends, peak of %zu resent packets", resendAvg, peakResend);

            if(LogCSV::GetSingletonPtr())
                LogCSV::GetSingleton().Write(CSV_STATUS, status);
        }

    }

}

NetManager::Connection* NetManager::GetConnByIP(LPSOCKADDR_IN addr)
{
    Client* client = clients.Find(addr);

    if(!client)
        return NULL;

    return client->GetConnection();
}

NetManager::Connection* NetManager::GetConnByNum(uint32_t clientnum)
{
    Client* client = clients.FindAny(clientnum);

    if(!client)
        return NULL;

    return client->GetConnection();
}

bool NetManager::SendMessageDelayed(MsgEntry* me, csTicks delay)
{
    if(delay)
    {
        DelayedMessageSendEvent* event = new DelayedMessageSendEvent(delay,me);
        psserver->GetEventManager()->Push(event);
        return true;
    }
    else
    {
        return SendMessage(me);
    }
}

bool NetManager::SendMessage(MsgEntry* me)
{
    bool sendresult;
    csRef<NetPacketQueueRefCount> outqueue = clients.FindQueueAny(me->clientnum);
    if(!outqueue)
        return false;

    /*  The proper way to send a message is to add it to the queue, and then add the queue to the senders.
     *  If you do it the other way around the net thread may remove the queue from the senders before you add the packet.
     *   Yes - this has actually happened!
     */

    /* We store the result here to return as the result of this function to maintain historic functionality.
     *  In actuality a false response does not actually mean no data was added to the queue, just that
     *  not all of the data could be added.
     */
    sendresult=NetBase::SendMessage(me,outqueue);

    /**
     * The senders list is a list of busy queues.  The SendOut() function
     * in NetBase clears this list each time through.  This saves having
     * to check every single connection each time through.
     */

    /*  The senders queue does not hold a reference itself, so we have to manually add one before pushing
     *  this queue on.  The queue is decref'd in the network thread when it's taken out of the senders queue.
     */
    if(!senders.Add(outqueue))
    {
        Error1("Senderlist Full!");
    }

    return sendresult;
}

// This function is the network thread
// Thread: Network
void NetManager::Run()
{
    csTicks currentticks    = csGetTicks();
    csTicks lastlinkcheck   = currentticks;
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

    float   kbpsOutMax = 0;
    float   kbpsInMax  = 0;

    size_t clientCountMax = 0;

    printf("Network thread started!\n");

    while(!stop_network)
    {
        if(!IsReady())
        {
            csSleep(100);
            continue;
        }

        timeout = csGetTicks() + maxTime;
        ProcessNetwork(timeout);

        currentticks = csGetTicks();

        // Check for link dead clients.
        if(currentticks - lastlinkcheck > LINKCHECK)
        {
            CheckLinkDead();
            lastlinkcheck = csGetTicks();
        }

        // Check to resend packages that have not been ACK'd yet
        if(currentticks - lastresendcheck > RESENDCHECK)
        {
            CheckResendPkts();
            CheckFragmentTimeouts();
            lastresendcheck = csGetTicks();
        }

        // Display Network statistics
        if(currentticks - laststatdisplay > STATDISPLAYCHECK)
        {
            kbpsin = (float)(totaltransferin - lasttotaltransferin) / (float)STATDISPLAYCHECK;
            if(kbpsin > kbpsInMax)
            {
                kbpsInMax = kbpsin;
            }
            lasttotaltransferin = totaltransferin;

            kbpsout = (float)(totaltransferout - lasttotaltransferout) / (float)STATDISPLAYCHECK;
            if(kbpsout > kbpsOutMax)
            {
                kbpsOutMax = kbpsout;
            }

            lasttotaltransferout = totaltransferout;

            laststatdisplay = currentticks;

            if(clients.Count() > clientCountMax)
            {
                clientCountMax = clients.Count();
            }
            csString status;
            status.Format("Currently using %1.2fKbps out, %1.2fkbps in. Packets: %ld out, %ld in", kbpsout, kbpsin, totalcountout-lasttotalcountout,totalcountin-lasttotalcountin);

            if(LogCSV::GetSingletonPtr())
                LogCSV::GetSingleton().Write(CSV_STATUS, status);

            if(pslog::disp_flag[LOG_LOAD])
            {
                CPrintf(CON_DEBUG, "Currently %d (Max: %d) clients using %1.2fKbps (Max: %1.2fKbps) outbound, "
                        "%1.2fkbps (Max: %1.2fKbps) inbound...\n",
                        clients.Count(),clientCountMax, kbpsout, kbpsOutMax,
                        kbpsin, kbpsInMax);
                CPrintf(CON_DEBUG, "Packets inbound %ld , outbound %ld...\n",
                        totalcountin-lasttotalcountin,totalcountout-lasttotalcountout);
            }

            lasttotalcountout = totalcountout;
            lasttotalcountin = totalcountin;
        }
    }
    printf("Network thread stopped!\n");
}

void NetManager::Broadcast(MsgEntry* me, int scope, int guildID)
{
    switch(scope)
    {
        case NetBase::BC_EVERYONE:
        case NetBase::BC_EVERYONEBUTSELF:
        {
            // send the message to each client (except perhaps the client that originated it)
            uint32_t originalclient = me->clientnum;

            // Copy message to send out to everyone
            csRef<MsgEntry> newmsg;
            newmsg.AttachNew(new MsgEntry(me));
            newmsg->msgid = GetRandomID();

            // Message is copied again into packet sections, so we can reuse same one.
            ClientIterator i(clients);

            while(i.HasNext())
            {
                Client* p = i.Next();
                if(scope==NetBase::BC_EVERYONEBUTSELF
                        && p->GetClientNum() == originalclient)
                    continue;

                // send to superclient only the messages he needs
                if(p->IsSuperClient())
                {
                    // time of the day is needed
                    if(me->GetType()!=MSGTYPE_WEATHER)
                        continue;
                }

                // Only clients that finished connecting get broadcastet
                // stuff
                if(!p->IsReady())
                    continue;

                newmsg->clientnum = p->GetClientNum();
                SendMessage(newmsg);
            }

            CHECK_FINAL_DECREF(newmsg, "BroadcastMsg");
            break;
        }
        // TODO: NetBase::BC_GROUP
        case NetBase::BC_GUILD:
        {
            CS_ASSERT_MSG("NetBase::BC_GUILD broadcast must specify guild ID", guildID != -1);

            /**
             * Send the message to each client with the same guildID
             */

            // Copy message to send out to everyone
            csRef<MsgEntry> newmsg;
            newmsg.AttachNew(new MsgEntry(me));
            newmsg->msgid = GetRandomID();

            // Message is copied again into packet sections, so we can reuse same one.
            ClientIterator i(clients);

            while(i.HasNext())
            {
                Client* p = i.Next();
                if(p->GetGuildID() == guildID)
                {
                    newmsg->clientnum = p->GetClientNum();
                    SendMessage(newmsg);
                }
            }

            CHECK_FINAL_DECREF(newmsg, "GuildMsg");
            break;
        }
        case NetBase::BC_FINALPACKET:
        {
            csRef<MsgEntry> newmsg;
            newmsg.AttachNew(new MsgEntry(me));
            newmsg->msgid = GetRandomID();

            LogMessages('F',newmsg);

            // XXX: This is hacky, but we need to send the message to the client
            // here and now! Because in the next moment he'll be deleted

            csRef<psNetPacketEntry> pkt;
            pkt.AttachNew(new psNetPacketEntry(me->priority, newmsg->clientnum,
                                               0, 0, (uint32_t) newmsg->bytes->GetTotalSize(),
                                               (uint16_t) newmsg->bytes->GetTotalSize(), newmsg->bytes));
            // this will also delete the pkt
            SendFinalPacket(pkt);

            CHECK_FINAL_DECREF(newmsg, "FinalPacket");
            break;
        }
        default:
            psprintf("\nIllegal Broadcast scope %d!\n",scope);
            return;
    }
}

Client* NetManager::GetClient(int cnum)
{
    return clients.Find(cnum);
}

Client* NetManager::GetAnyClient(int cnum)
{
    return clients.FindAny(cnum);
}


void NetManager::Multicast(MsgEntry* me, const csArray<PublishDestination> &multi, uint32_t except, float range)
{
    for(size_t i=0; i<multi.GetSize(); i++)
    {
        if(multi[i].client==except)   // skip the exception client to avoid circularity
            continue;

        Client* c = clients.Find(multi[i].client);
        if(c && c->IsReady())
        {
            if(range == 0 || multi[i].dist < range)
            {
                me->clientnum = multi[i].client;
                SendMessage(me);      // This copies the mem block, so we can reuse.
            }
        }
    }
}

// This function is called from the network thread, no acces to server
// internal data should be made
void NetManager::CheckLinkDead()
{
    csTicks currenttime = csGetTicks();

    csArray<uint32_t> checkedClients;
    Client* pClient = NULL;

    // Delete all clients marked for deletion already
    clients.SweepDelete();

    while(true)
    {
        pClient = NULL;

        // Put the iterator in a limited scope so we don't hold on to the lock which may cause deadlock
        {
            ClientIterator i(clients);

            while(i.HasNext())
            {
                Client* candidate = i.Next();

                // Skip if already seen
                if(checkedClients.FindSortedKey(csArrayCmp<uint32_t, uint32_t> (candidate->GetClientNum())) != csArrayItemNotFound)
                    continue;

                checkedClients.InsertSorted(candidate->GetClientNum());
                pClient = candidate;
                break;
            }
        }

        // No more clients to check so break
        if(!pClient)
            break;

        // Shortcut here so zombies may immediately disconnect
        if(pClient->IsZombie() && pClient->ZombieAllowDisconnect())
        {
            /* This simulates receipt of this message from the client
             ** without any network access, so that disconnection logic
             ** is all in one place.
             */
            psDisconnectMessage discon(pClient->GetClientNum(), 0, "You should not see this.");
            if(discon.valid)
            {
                Connection* connection = pClient->GetConnection();
                HandleCompletedMessage(discon.msg, connection, NULL,NULL);
            }
            else
            {
                Bug2("Failed to create valid psDisconnectMessage for client id %u.\n", pClient->GetClientNum());
            }
        }
        else if(pClient->GetConnection()->lastRecvPacketTime+timeout < currenttime)
        {
            if(pClient->GetConnection()->heartbeat < 10 && pClient->GetConnection()->lastRecvPacketTime+timeout * 10 > currenttime)
            {
                psHeartBeatMsg ping(pClient->GetClientNum());
                Broadcast(ping.msg, NetBase::BC_FINALPACKET);
                pClient->GetConnection()->heartbeat++;
            }
            else
            {
                if(!pClient->AllowDisconnect())
                    continue;

                csString ipAddr = pClient->GetIPAddress();

                csString status;
                status.Format("%s, %u, Client (%s) went linkdead.", ipAddr.GetDataSafe(), pClient->GetClientNum(), pClient->GetName());
                psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

                /* This simulates receipt of this message from the client
                 ** without any network access, so that disconnection logic
                 ** is all in one place.
                 */
                psDisconnectMessage discon(pClient->GetClientNum(), 0, "You are linkdead.");
                if(discon.valid)
                {
                    Connection* connection = pClient->GetConnection();
                    HandleCompletedMessage(discon.msg, connection, NULL,NULL);
                }
                else
                {
                    Bug2("Failed to create valid psDisconnectMessage for client id %u.\n", pClient->GetClientNum());
                }
            }
        }
    }
}

