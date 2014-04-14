/*
* netbase.cpp by Matze Braun <MatzeBraun@gmx.de>
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

#include <csutil/snprintf.h>
#include <csutil/randomgen.h>
#include <csutil/sysfunc.h>
#include <csutil/set.h>
#include <csutil/threading/thread.h>

#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __APPLE__
#include <sys/time.h>  // timeval struct in different place in Mac OS X
#endif

#include <ctype.h> 

#include <fcntl.h>
#include "util/pserror.h"
#include "net/netbase.h"
#include "net/netpacket.h"
#include "net/message.h"
#include "net/messages.h"
#include "util/psscf.h"
#include "util/psconst.h"
#include "util/log.h"
#include "util/serverconsole.h"
#include "util/strutil.h"

// Static members
int NetBase::socklibrefcount=0;
NetBase::AccessPointers NetBase::accessPointers = {NULL,NULL,NULL};

// warning: this messes your logs with much data
//#define PACKETDEBUG
// Percentage of packets to be simulated as lost. Set to between 0 and 1 to activate.
const float TEST_PACKETLOSS = 0.0f;

const char* NetBase::AccessPointers::Request(csStringID id) const
{
    if (msgstrings)
    {
        return msgstrings->Request(id);
    }
    else if(msgstringshash)
    {
        return msgstringshash->Request(id);
    }
    return NULL;
}

csStringID NetBase::AccessPointers::Request( const char* string) const
{
    if (msgstrings)
    {
        /// TODO: Should not need to call Contains, but as long as the request function
        //        assign new IDs we have to check first.
        if (msgstrings->Contains(string))
        {
            return msgstrings->Request(string);
        }
        else
        {
            return csInvalidStringID;
        }        
    }
    else if(msgstringshash)
    {
        return msgstringshash->Request(string);
    }
    return csInvalidStringID;
}



NetBase::NetBase(int outqueuesize)
: senders(outqueuesize)
{
    randomgen = new csRandomGen;
    
    pipe_fd[0] = 0;
    pipe_fd[1] = 0;

    if (socklibrefcount==0 && initSocket())
        ERRORHALT ("couldn't init socket library!");
    socklibrefcount++;

    NetworkQueue.AttachNew(new NetPacketQueueRefCount(MAXQUEUESIZE));
    if (!NetworkQueue)
        ERRORHALT("No Memory!");

    ready=false;
    totaltransferout  = 0;
    totaltransferin   = 0;
    totalcountin      = 0;
    totalcountout     = 0;
    
    profs = new psNetMsgProfiles();
    
    // Initialize the timeout to 30ms, adds 30ms of latency but gives time for packets to coalesce
    timeout.tv_sec = 0;
    timeout.tv_usec = 30000;

    accessPointers.msgstrings = NULL;
    accessPointers.msgstringshash = NULL;
    accessPointers.engine = NULL;

    logmsgfiltersetting.invert = false;
    logmsgfiltersetting.filterhex = true;
    logmsgfiltersetting.receive = false;
    logmsgfiltersetting.send = false;

    input_buffer = NULL;
    for(int i=0;i < NETAVGCOUNT;i++)
    {
        sendStats[i].senders = sendStats[i].messages = sendStats[i].time = 0;
    }
    for(int i =0;i < RESENDAVGCOUNT;i++)
        resends[i] = 0;
    avgIndex = resendIndex = 0;
    lastSendReport = csGetTicks();
}


NetBase::~NetBase()
{
    if (ready)
        Close();

    Notify2(LOG_ANY,"Total bytes sent out was %li.\n",totaltransferout);
    Notify2(LOG_ANY,"Total bytes received was %li.\n", totaltransferin);

    socklibrefcount--;
    if (socklibrefcount==0)
    {
        exitSocket();
    }

    if (randomgen)
        delete randomgen;
    
    delete profs;

    if (input_buffer)
        cs_free(input_buffer);
}


/* add a Message Queue */
bool NetBase::AddMsgQueue(MsgQueue *q, objID, objID)
{
    inqueues.Push(q);
    return true;
}


void NetBase::RemoveMsgQueue(MsgQueue *q)
{
    inqueues.Delete (q); 
}


void NetBase::ProcessNetwork (csTicks timeout)
{
    // Incoming packets from the wire go to a queue for processing by app.
    while (
        (SendOut() || CheckIn()
        // Outgoing packets from a queue go on the wire.
        ) && csGetTicks() <= timeout
        );
}


bool NetBase::CheckIn()
{    
    // check for incoming packets
    SOCKADDR_IN addr;
    memset (&addr, 0, sizeof(SOCKADDR_IN));
    socklen_t len = sizeof(SOCKADDR_IN);

    if (!input_buffer)
    {
        input_buffer = (char*) cs_malloc(MAXPACKETSIZE);

        if (!input_buffer)
        {
            Error2("Failed to cs_malloc %d bytes for packet buffer!\n",MAXPACKETSIZE);
            return false;
        }
    }

    // Connection must be initialized!
    CS_ASSERT(ready);
    
    int packetlen = RecvFrom (&addr, &len, (void*) input_buffer, MAXPACKETSIZE);

    if (packetlen <= 0)
    {
        return false;
    }
    // Identify the connection
    Connection* connection = GetConnByIP(&addr);

    // Extract the netpacket from the buffer and prep for use locally.
    psNetPacket *bufpacket = psNetPacket::NetPacketFromBuffer(input_buffer,packetlen);
    if (bufpacket==NULL)
    {

        //for win32 for now only inet_ntoa as inet_ntop wasn't supported till vista.
        //it has the same degree of compatibility of the previous code and it's supported till win2000
#ifdef INCLUDE_IPV6_SUPPORT
        char addrText[INET6_ADDRSTRLEN];
        //there was a failure in conversion if null
        if(!inet_ntop(addr.sin6_family,&addr.sin6_addr, addrText, sizeof(addrText)))
        {
            strncpy(addrText, "UNKNOWN", INET6_ADDRSTRLEN);
        }
#else
        char addrText[INET_ADDRSTRLEN];
#ifdef WIN32
        strncpy(addrText, inet_ntoa(addr.sin_addr), INET_ADDRSTRLEN);
#else
        //there was a failure in conversion if null
        if(!inet_ntop(addr.sin_family,&addr.sin_addr, addrText, sizeof(addrText)))
        {
            strncpy(addrText, "UNKNOWN", INET_ADDRSTRLEN);
        }
#endif
#endif

        // The data received was too small to make a full packet.
        if (connection)
        {
            Debug4(LOG_NET, connection->clientnum, "Too short packet received from client %d (IP: %s) (%d bytes)", connection->clientnum, addrText, packetlen);
        }
        else
        {
            Debug3(LOG_NET, 0, "Too short packet received from IP address %s. (%d bytes) No existing connection from this IP.",
                addrText, packetlen);
        }
        return true; // Continue processing more packets if available
    }
    input_buffer = NULL; //input_buffer now hold by the bufpacket pointer.

    // Endian correction
    bufpacket->UnmarshallEndian();

    // Check for too-big packets - no harm in processing them, but probably a bug somewhere
    if (bufpacket->GetPacketSize() < static_cast<unsigned int>(packetlen))
    {
        //for win32 for now only inet_ntoa as inet_ntop wasn't supported till vista.
        //it has the same degree of compatibility of the previous code and it's supported till win2000
#ifdef INCLUDE_IPV6_SUPPORT
        char addrText[INET6_ADDRSTRLEN];
        //there was a failure in conversion if null
        if(!inet_ntop(addr.sin6_family,&addr.sin6_addr, addrText, sizeof(addrText)))
        {
            strncpy(addrText, "UNKNOWN", INET_ADDRSTRLEN);
        }
#else
        char addrText[INET_ADDRSTRLEN];
#ifdef WIN32
        strncpy(addrText, inet_ntoa(addr.sin_addr), INET_ADDRSTRLEN);
#else
        //there was a failure in conversion if null
        if(!inet_ntop(addr.sin_family,&addr.sin_addr, addrText, sizeof(addrText)))
        {
            strncpy(addrText, "UNKNOWN", INET_ADDRSTRLEN);
        }
#endif
#endif
        
        if (connection)
        {
            Debug5(LOG_NET, connection->clientnum, "Too long packet received from client %d (IP: %s) (%d bytes received, header reports %zu bytes)",
                connection->clientnum, addrText, packetlen, bufpacket->GetPacketSize());
        }
        else
        {
            
            Debug4(LOG_NET, 0,"Too long packet received from IP address %s. (%d bytes received, header reports %zu bytes) No existing connection from this IP.",
                   addrText, packetlen, bufpacket->GetPacketSize());
        }
    }

    //Create new net packet entry and transfer ownership of bufpacket to pkt.
    csRef<psNetPacketEntry> pkt;
    pkt.AttachNew(new psNetPacketEntry( bufpacket, 
            connection ? connection->clientnum : 0, packetlen));
    
    if(TEST_PACKETLOSS > 0.0 && randomgen->Get() < TEST_PACKETLOSS)
    {
        psNetPacket* packet = pkt->packet;
        int type = 0;

        if (packet->offset == 0) 
        {
            psMessageBytes* msg = (psMessageBytes*) packet->data;
            type = msg->type;
        }

        Error3("Packet simulated lost. Type %s ID %d.\n", type == 0 ? "Fragment" : (const char *)  GetMsgTypeName(type), pkt->packet->pktid);
        return true;
    }

    // ACK packets can get eaten by HandleAck
    if (HandleAck(pkt, connection, &addr))
    {
        return true;
    }

    // printf("Got packet with sequence %d.\n", pkt->packet->GetSequence());
    //
    // Check for doubled packets and drop them
    if (pkt->packet->pktid != 0)
    {
        if (connection && CheckDoublePackets (connection, pkt))
        {
#ifdef PACKETDEBUG
            Debug2(LOG_NET,0,"Dropping doubled packet (ID %d)\n", pkt->packet->pktid);
#endif
            return true;
        }
    }
    
#ifdef PACKETDEBUG
    Debug7(LOG_NET,0,"Received Pkt, ID: %d, offset %d, from %d size %d (actual %d) flags %d\n", 
        pkt->packet->pktid, pkt->packet->offset, pkt->clientnum, pkt->packet->pktsize,packetlen, pkt->packet->flags);
#endif

    /**
    * Now either send this packet to BuildMessage, or loop through
    * subpackets if they are merged.
    */
    csRef<psNetPacketEntry> splitpacket;
    psNetPacket      *packetdata = NULL;

    do
    {
        splitpacket = pkt->GetNextPacket(packetdata);
        if (splitpacket)
            BuildMessage(splitpacket, connection, &addr);
    } while (packetdata);
    return true;
}


bool NetBase::Flush(MsgQueue* /*queue*/)
{
    //@@@ Dirty, hack, this might be in a diffrent thread
    //@@@ need to syncronize this in some way. But at least
    //@@@ by moving this here the client code dosn't call
    //@@@ SendOut anymore.
    SendOut();

    return true;
}


bool NetBase::HandleAck(psNetPacketEntry* pkt, Connection* connection,
                        LPSOCKADDR_IN addr)
{
    psNetPacket* packet = pkt->packet;

    // REVERTED 3/8/03 Until I can figure out why this is causing client conencts to fail -  Andrew Mann (Rhad)
    // If we don't know who this connection is, don't handle ACKs
    //if (!connection)
    //    return false;


    if (packet->pktsize == PKTSIZE_ACK)  // special pktsize means ACK packet here
    {
#ifdef PACKETDEBUG
        Debug1(LOG_NET,0,"Ack received.\n");
#endif

        // receipt of ack packet is good enough to keep alive connection
        if (connection)
        {
            connection->heartbeat          = 0;
            connection->lastRecvPacketTime = csGetTicks();
            connection->pcknumin++;
        }

        csRef<psNetPacketEntry> ack;
        
        // The hash only keys on the clientnum and pktid so we need to go looking for the offset
        csArray<csRef<psNetPacketEntry> > acks = awaitingack.GetAll(PacketKey(pkt->clientnum, pkt->packet->pktid));
        for(size_t i = 0;i < acks.GetSize(); i++)
        {
            if(acks[i]->packet->offset == pkt->packet->offset)
                ack = acks[i];
        }
                

        if (ack) // if acked pkt is found, simply remove it.  We're done.
        {
            // Only update RTT estimate when packet has not been retransmitted
            if (!ack->retransmitted && connection)
            {
                csTicks elapsed = csGetTicks() - ack->timestamp;
      
                if (connection->estRTT > 0)
                {
                    int diff = (int) (elapsed - connection->estRTT);
                    connection->estRTT += (int) (0.125 * diff);
                    if(diff < 0)
                        diff = -diff;

                    connection->devRTT += (int) (0.125 * (diff - connection->devRTT));
                }
                else
                {
                    // Initialise the RTT estimates
                    connection->estRTT = elapsed;
                    connection->devRTT = elapsed / 2;
                }

                // Update the packet timeout
                connection->RTO = (int) (connection->estRTT + 4 * connection->devRTT);
                if (connection->RTO > PKTMAXRTO)
                {
                    connection->RTO = PKTMAXRTO;
                }
                else if(connection->RTO < PKTMINRTO)
                {
                    connection->RTO = PKTMINRTO;
                }

                netInfos.AddPingTicks(elapsed);
            }
            // printf ("Ping time: %i, average: %i\n", elapsed, netInfos.GetAveragePingTicks());


            if (!awaitingack.Delete(PacketKey(pkt->clientnum, pkt->packet->pktid), ack))
            {
#ifdef PACKETDEBUG
                Debug2(LOG_NET,0,"No packet in ack queue :%d\n", ack->packet->pktid);
#endif
            }
            else if(connection)
            {
                connection->RemoveFromWindow(ack->packet->GetPacketSize());
            }
        }
        else // if not found, it is probably a resent ACK which is redundant so do nothing
        {
#ifdef PACKETDEBUG
            Debug1(LOG_NET,0,"No matching packet found. No problem though.\n");
#endif
        }

        return true;   // eat the packet
    }

    if (pkt->packet->GetPriority() == PRIORITY_HIGH) // a HIGH_PRIORITY packet -> ack
    {
        
#ifdef PACKETDEBUG
        Debug1(LOG_NET,0,"High priority packet received.\n");
#endif
        
        if (connection)
        {
            csRef<psNetPacketEntry> ack;
            ack.AttachNew(new psNetPacketEntry(PRIORITY_LOW,
                    pkt->clientnum,
                    pkt->packet->pktid,
                    pkt->packet->offset,
                    pkt->packet->msgsize,
                    PKTSIZE_ACK,(char *)NULL));
            
            SendFinalPacket(ack, addr);
            // ack should be unre'd here
        }
    }

    return false;
}


bool NetBase::CheckDoublePackets(Connection* connection, psNetPacketEntry* pkt)
{
    csArray<uint32_t> offsets = connection->packethistoryhash.GetAll(pkt->packet->pktid);
    if(!offsets.IsEmpty())
    {
        for(size_t i = 0; i < offsets.GetSize(); i++)
        {
            if(offsets[i] == pkt->packet->offset)
            {
                return true;
            }
        }
    }

    // No duplicate packet found
    connection->packethistoryhash.Put(pkt->packet->pktid, pkt->packet->offset);

    // Push new packet onto history
    connection->packethistoryhash.Delete(connection->packethistoryid[connection->historypos], connection->packethistoryoffset[connection->historypos]);
    connection->packethistoryid[connection->historypos] = pkt->packet->pktid;
    connection->packethistoryoffset[connection->historypos] = pkt->packet->offset;
    connection->historypos = (connection->historypos + 1) % MAXPACKETHISTORY;

    return false;
}


void NetBase::CheckResendPkts()
{
    // NOTE: Globaliterators on csHash do not retrieve keys contiguously.
    csHash<csRef<psNetPacketEntry> , PacketKey>::GlobalIterator it(awaitingack.GetIterator());
    csRef<psNetPacketEntry> pkt;
    csArray<csRef<psNetPacketEntry> > pkts;
    csArray<Connection*> resentConnections;

    csTicks currenttime = csGetTicks();
    unsigned int resentCount = 0;

    while(it.HasNext())
    {
        pkt = it.Next();
        // Check the connection packet timeout
        if (pkt->timestamp + csMin((csTicks)PKTMAXRTO, pkt->RTO) < currenttime)
            pkts.Push(pkt);
    }
    for (size_t i = 0; i < pkts.GetSize(); i++)
    {
        pkt = pkts.Get(i);
#ifdef PACKETDEBUG
        Debug2(LOG_NET,0,"Resending nonacked HIGH packet (ID %d).\n", pkt->packet->pktid);
#endif
        Connection* connection = GetConnByNum(pkt->clientnum);
        if (connection)
        {
            if (resentConnections.Find(connection) == csArrayItemNotFound)
                resentConnections.Push(connection);

            // This indicates a bug in the netcode.
            if (pkt->RTO == 0)
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
        
        // re-add to send queue
        if(NetworkQueue->Add(pkt))
        {
            //printf("pkt=%p, pkt->packet=%p\n",pkt,pkt->packet);
            // take out of awaiting ack pool.
            // This does NOT delete the pkt mem block itself.
            if (!awaitingack.Delete(PacketKey(pkt->clientnum, pkt->packet->pktid), pkt))
            {
#ifdef PACKETDEBUG
                Debug2(LOG_NET,0,"No packet in ack queue :%d\n", pkt->packet->pktid);
#endif
            }
            else if(connection)
            {
                connection->RemoveFromWindow(pkt->packet->GetPacketSize());
            }
        }
    }

    if(resentCount > 0)
    {
        resends[resendIndex] = resentCount;
        resendIndex = (resendIndex + 1) % RESENDAVGCOUNT;
        
        csTicks timeTaken = csGetTicks() - currenttime;
        if(resentCount > 300 || resendIndex == 1 || timeTaken > 50)
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
            if(timeTaken > 50)
            {
                status.Format("Resending high priority packets has taken %u time to process, for %u packets.", timeTaken, resentCount);
                CPrintf(CON_WARNING, "%s\n", (const char *) status.GetData());
            }
            status.AppendFmt("Resending non-acked packet statistics: %g average resends, peak of %zu resent packets", resendAvg, peakResend);
 
            if(LogCSV::GetSingletonPtr())
                LogCSV::GetSingleton().Write(CSV_STATUS, status);
        }
        
    }
}
    

bool NetBase::SendMergedPackets(NetPacketQueue *q)
{
    csRef<psNetPacketEntry> queueget;
    csRef<psNetPacketEntry> candidate, final;

    queueget = q->Peek(); // csRef required for q->Get()
    // If there's not at least one packet in the queue, we're done.
    if(!queueget)
        return false;

    Connection* connection = GetConnByNum(queueget->clientnum);
//    if(connection) printf("Window size: %d\n", connection->window);
    
    // Always send something if queue is full, otherwise only send if window is not full.
    // This prevents some deadlock issues.
    if(connection && connection->IsWindowFull() && !q->IsFull())
        return false;
    
    queueget = q->Get();
    
    final = queueget;  // This was packet 0 in the pending queue

    // Try to merge additional packets into a single send.
    while ((queueget=q->Get()))  // This is now looping through packets 1-N
    {
//      if(connection) printf("Window size2: %d\n", connection->window);
        candidate = queueget;
        if (candidate->packet->GetSequence() != 0) // sequenced packet is following a non-sequenced packet
        {
            SendSinglePacket(candidate); // Go ahead and send the sequenced one, but keep building the merged one.
            if(connection && connection->IsWindowFull())
                break;

            continue;
        }

        if(!final->Append(candidate))
        {
            // A failed append means that the packet can't fit or is a resent packet.
            // Resent packets MUST NOT be merged because it circumvents clientside packet dup
            // detection
            SendSinglePacket(final);
            
            // Start the process again with the packet that wouldn't fit
            final = candidate;
        }

        if(connection && connection->IsWindowFull())
            break;
    }

    // There is always data in final here
    SendSinglePacket(final);  // this deletes if necessary

    return true;
}


bool NetBase::SendSinglePacket(psNetPacketEntry* pkt)
{
    if (!SendFinalPacket (pkt))
    {
        return false;
    }

    /**
    * Add to tree of packets we need acks on.  These will be
    * checked in Recv code and removed from tree.  Old packets
    * will be checked by a timer and resent.
    */
    if (pkt->packet->GetPriority() == PRIORITY_HIGH)
    {
#ifdef PACKETDEBUG
        Debug3(LOG_NET,0,"Sent HIGH pkt id %d to client %d.\n", 
            pkt->packet->pktid, pkt->clientnum);
#endif
        Connection* connection = GetConnByNum(pkt->clientnum);
        if(connection)
        {
            // Add to window
            connection->AddToWindow(pkt->packet->GetPacketSize());
            connection->sends++;
            // Set timeout for resending.
            pkt->RTO = connection->RTO;
            awaitingack.Put(PacketKey(pkt->clientnum, pkt->packet->pktid), pkt);
        }
    }

    return true;
}


bool NetBase::SendFinalPacket(psNetPacketEntry* pkt)
{
    Connection* connection = GetConnByNum(pkt->clientnum);
    if (!connection)
    {
        // This is probably happening because packets needed to be
        // resent to a linkdead client.
#ifdef PACKETDEBUG
        Debug2(LOG_NET,0,"Packet for clientnum %d is being discarded.\n", pkt->clientnum);
#endif
        return false;
    }
    if(pkt->packet->pktid == 0)
    {
        pkt->packet->pktid = connection->GetNextPacketID();
    }
    return SendFinalPacket(pkt,&(connection->addr));
    
}


bool NetBase::SendFinalPacket(psNetPacketEntry* pkt, LPSOCKADDR_IN addr)
{
    // send packet...
#ifdef PACKETDEBUG
    Debug5(LOG_NET,0,"SendPacket ID: %d to %d size %d flags %d\n",
        pkt->packet->pktid, pkt->clientnum, pkt->packet->pktsize, pkt->packet->flags);
#endif

    // printf("Sending packet sequence %d, length %d on the wire.\n", pkt->packet->GetSequence(),pkt->packet->GetPacketSize() );

    uint16_t size = (uint16_t)pkt->packet->GetPacketSize();
    void *data = pkt->GetData();

    pkt->packet->MarshallEndian();

    int err = SendTo (addr, data, size);
    if (err != (int)size )
    {
        Error4("Send error %d: %d bytes sent and %d bytes expected to be sent.\n", errno,err,size);
        pkt->packet->UnmarshallEndian();
        return false;
    }

    pkt->packet->UnmarshallEndian();
    return true;
}


bool NetBase::SendOut()
{
    bool sent_anything = false;
    csTicks begin = csGetTicks();

    CS_ASSERT(ready);

    // Part1: Client (This is only used when we're the client)
    if (SendMergedPackets(NetworkQueue))  // client uses this queue
    {
        sent_anything=true;
    }

    // Part2: Server (only used when we're the server)
    unsigned int senderCount = senders.Count();
    unsigned int sentCount = 0;
    csRef<NetPacketQueueRefCount> q; 
    csArray<csRef<NetPacketQueueRefCount> > readd;
    while (q = senders.Get())
    {
        sentCount += q->Count();
        if (SendMergedPackets(q))
        {
            sent_anything = true;
        }
        // If the window is full so there are still packets left, ensure we re-check the queue later.
        sentCount -= q->Count();
        
        if(q->Count() > 0)
            readd.Push(q);
    }

    for(size_t i = 0; i < readd.GetSize(); i++)
        senders.Add(readd[i]);

    // Statistics updating
    csTicks timeTaken = csGetTicks() - begin;
    sendStats[avgIndex].senders = senderCount;
    sendStats[avgIndex].messages = sentCount;
    sendStats[avgIndex].time = timeTaken;
    
    if(senderCount > 0 && (avgIndex == 1 || timeTaken > 100))
    {
        unsigned int peakSenders = 0;
        float peakMessagesPerSender = 0.0f;
        csTicks peakTime = 0;
        
        float sendAvg = 0.0f;
        float messagesAvg = 0.0f;
        float timeAvg = 0.0f;
        // Calculate averages/peak data here
        for(int i = 0; i < NETAVGCOUNT; i++)
        {
            peakSenders = csMax(peakSenders, sendStats[i].senders);
            sendAvg += sendStats[i].senders;
            peakMessagesPerSender = csMax(peakMessagesPerSender, (float) sendStats[i].messages / (float) sendStats[i].senders);
            messagesAvg += sendStats[i].messages;
            timeAvg += sendStats[i].time;
            peakTime = csMax(peakTime, sendStats[i].time);
        }

        sendAvg /= NETAVGCOUNT;
        messagesAvg /= NETAVGCOUNT;
        timeAvg /= NETAVGCOUNT;
        csString status;
        if(timeTaken > 50)
        {
            status.Format("Sending network messages has taken %u time to process, for %u senders and %u messages.", timeTaken, senderCount, sentCount);
            CPrintf(CON_WARNING, "%s\n", (const char *) status.GetData());
        }

        status.AppendFmt("Network average statistics for last %u ticks: %g senders, %g messages/sender, %g time per iteration. Peak %u senders, %g messages/sender %u time", csGetTicks() - lastSendReport, sendAvg, messagesAvg, timeAvg, peakSenders, peakMessagesPerSender, peakTime);
        
        if(LogCSV::GetSingletonPtr())
            LogCSV::GetSingleton().Write(CSV_STATUS, status);

        lastSendReport = csGetTicks();
    }
    
    if(senderCount > 0)
        avgIndex = (avgIndex + 1) % NETAVGCOUNT;

    return sent_anything;
}


bool NetBase::Init(bool autobind)
{
    int err;

    if (ready)
        Close();
    
#ifndef CS_PLATFORM_WIN32 
    // win32 doesn't support select on pipes so we don't support lag-free communication yet
    if(pipe(pipe_fd) != 0)
        Error1("Error creating self pipe");
#endif

#ifdef INCLUDE_IPV6_SUPPORT    
    mysocket = socket (AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
#else
    mysocket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif
    if (mysocket < 0)
    {
        Error1("Failed to create socket");
        return false;
    }
    
    /* set to non blocking operation */
    unsigned long arg=1;
    err = SOCK_IOCTL (mysocket, FIONBIO, &arg);
    if (err < 0)
    {
        Error1("set to nonblock failed");
        Close(SOCKET_CLOSE_FORCED);
        return false;
    }

    if (autobind)
    {
#ifdef INCLUDE_IPV6_SUPPORT
        struct in6_addr anyaddr = IN6ADDR_ANY_INIT;
#else
        struct in_addr anyaddr;
        anyaddr.s_addr = INADDR_ANY;
#endif

        if (!Bind ( anyaddr, 0))
        {
            Close(SOCKET_CLOSE_FORCED);
            return false;
        }
    }

    return true;
}


bool NetBase::Bind (const char* str, int port)
{
    IN_ADDR addr;

#ifdef INCLUDE_IPV6_SUPPORT
    if (!inet_pton(AF_INET6, str, &addr))
    {
        return false;
    }
#else
    addr.s_addr = inet_addr(str);
#endif

    return Bind(addr, port);
}


bool NetBase::Bind (const IN_ADDR &addr, int port)
{
    int err;
    SOCKADDR_IN sockaddr;

    if (!mysocket)
        return false;

    memset ((void*) &sockaddr, 0, sizeof(SOCKADDR_IN));
#ifdef INCLUDE_IPV6_SUPPORT
    sockaddr.sin6_family = AF_INET6;                         
    sockaddr.sin6_addr = addr;                       
    sockaddr.sin6_port = htons(port);
#else
    sockaddr.sin_family = AF_INET;                         
    sockaddr.sin_addr = addr;                       
    sockaddr.sin_port = htons(port);
#endif
    err = bind (mysocket, (LPSOCKADDR) &sockaddr, sizeof(SOCKADDR_IN));
    if (err < 0)
    {                    
        Error1("bind() failed");
        return false;
    }

    ready = true;
    return true;
}


void NetBase::Close(bool force)
{
    if (ready || force)
        SOCK_CLOSE(mysocket); 

    ready = false;
}


int NetBase::GetIPByName(LPSOCKADDR_IN addr, const char *name)
{
#ifdef INCLUDE_IPV6_SUPPORT
    struct addrinfo  hints;
    struct addrinfo* result;
    struct addrinfo* res;


    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6; // AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
    hints.ai_protocol = 0;          /* Any protocol */

    int error = getaddrinfo(name, NULL, &hints, &result);
    if (error != 0)
    {
        Error3("gettaddrinfo: %s\n->Host: %s",gai_strerror(error),name);
        return -1;
    }
    for (res = result; res != NULL; res = res->ai_next)
    {     
        void *ptr;
        
        switch (res->ai_family)
        {
        case AF_INET:
          ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
          break;
        case AF_INET6:
          ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
          break;
        }
        char addrStr[INET6_ADDRSTRLEN] = {0};
        inet_ntop(res->ai_family, ptr, addrStr, INET6_ADDRSTRLEN );
        printf("===============0> IP for %s is %s\n",name, addrStr);

        addr->sin6_family = res->ai_family;
        memcpy(&addr->sin6_addr,ptr,res->ai_addrlen);
    }
    
    freeaddrinfo(result);
    return 0;
#else
    struct hostent *hentry;

    hentry = gethostbyname (name);
    if (!hentry)
        return -1;

    addr->sin_addr.s_addr = ((struct in_addr *) hentry->h_addr)->s_addr;
    return 0;
#endif
}


void NetBase::LogMessages(char dir,MsgEntry* me)
{
    if (DoLogDebug(LOG_MESSAGES) && !FilterLogMessage(me->bytes->type,dir))
    {
        csString msgtext;
        int filterNumber;
        DecodeMessage(me, &accessPointers, logmsgfiltersetting.filterhex, msgtext, filterNumber);
        Debug3(LOG_MESSAGES,filterNumber,"%c: %s\n",dir,msgtext.GetDataSafe());
               
    }
}


csString NetBase::LogMessageFilter(const char *arg)
{
    if (strlen(arg) == 0)
    {
        return "Please specify message to be filtered.\nSyntax: filtermsg [+/-<msg_type>|invert|filterhex [no]|clear|receive|send|show]";
    }
    WordArray words(arg);
    if (words[0] == "invert")
    {
        InvertLogMessageFilter();
    } 
    else if (words[0] == "filterhex")
    {
        bool filterhex = true;
        if (words[1] == "no") 
            filterhex = false;
        
        SetLogMessageFilterHex(filterhex);
    } 
    else if (words[0] == "show")
    {
        LogMessageFilterDump();
    } 
    else if (words[0] == "clear")
    {
        LogMessageFilterClear();
    } 
    else if (words[0] == "receive")
    {
        ToggleReceiveMessageFilter();
    } 
    else if (words[0] == "send")
    {
        ToggleSendMessageFilter();
    } 
    else
    {
        bool add = true;
        int type = -1;
        if (words[0].GetAt(0) == '+')
        {
            type = atoi(words[0]);
        }
        else if (words[0].GetAt(0) == '-')
        {
            add = false;
            type = -atoi(words[0]);
        }
        else
        {
            type = atoi(words[0]);
        }

        if (type == 0)
        {
            csString name = words[0];
            if (name.GetAt(0) == '+' || name.GetAt(0) == '-')
            {
                name.DeleteAt(0,1);
            }
                
            type = psfMsgType(name);
        }
        
        if (type != -1)
        {
            if (add)
            {
                AddFilterLogMessage(type);
                return csString().Format("Add %s(%d)",psfMsgTypeName(type).GetDataSafe(),type);
            }
            else
            {
                RemoveFilterLogMessage(type);
                return csString().Format("Remove %s(%d)",psfMsgTypeName(type).GetDataSafe(),type);
            }
        }
    }
    
    return "";
}


void NetBase::AddFilterLogMessage(int type)
{
    size_t n;
    for (n = 0; n < logmessagefilter.GetSize(); n++)
    {
        if (logmessagefilter[n] == type)
        {
            // Message already in filter list
            return;
        }
    }
    logmessagefilter.Push(type);
}


void NetBase::RemoveFilterLogMessage(int type)
{
    size_t n;
    for (n = 0; n < logmessagefilter.GetSize(); n++)
    {
        if (logmessagefilter[n] == type) 
            logmessagefilter.DeleteIndex(n);
    }
}


void NetBase::LogMessageFilterClear()
{
    logmessagefilter.DeleteAll();
}


bool NetBase::FilterLogMessage(int type, char dir)
{
    size_t n;
    bool result = false;

    // Check global receive and transmit filters
    if ((dir == 'R' || dir == 'r') && logmsgfiltersetting.receive)
    {
        return true;
    }
    if ((dir == 'S' || dir == 's') && logmsgfiltersetting.send)
    {
        return true;
    }
    
    for (n = 0; n < logmessagefilter.GetSize(); n++)
    {
        if (logmessagefilter[n] == type) result = true;
    }
    if (logmsgfiltersetting.invert)
        result = !result;
    
    return result;
}


void NetBase::LogMessageFilterDump()
{
    size_t n;
    
    CPrintf(CON_CMDOUTPUT,"\nLogMessage filter setting\n");
    CPrintf(CON_CMDOUTPUT,"Invert filter  : %s\n",(logmsgfiltersetting.invert?"true":"false"));
    CPrintf(CON_CMDOUTPUT,"Filter hex     : %s\n",(logmsgfiltersetting.filterhex?"true":"false"));
    CPrintf(CON_CMDOUTPUT,"Filter receive : %s\n",(logmsgfiltersetting.receive?"true":"false"));
    CPrintf(CON_CMDOUTPUT,"Filter send    : %s\n",(logmsgfiltersetting.send?"true":"false"));
    CPrintf(CON_CMDOUTPUT,"Filter msgs    :\n");
    for (n = 0; n < logmessagefilter.GetSize(); n++)
    {
        CPrintf(CON_CMDOUTPUT,"%s(%d)\n",GetMsgTypeName(logmessagefilter[n]).GetDataSafe(),logmessagefilter[n]);
    }
}


bool NetBase::SendMessage(MsgEntry* me)
{
    if (me->overrun)
    {
        CS_ASSERT(!"NetBase::SendMessage() Failed to send message in overrun state!\n");
        return false;
    }
    return SendMessage(me,NULL);
}


bool NetBase::SendMessage(MsgEntry* me,NetPacketQueueRefCount *queue)
{
    profs->AddSentMsg(me);

    size_t bytesleft = me->bytes->GetTotalSize();
    size_t offset    = 0;
    uint32_t id = 0;

    if (!queue)
        queue = NetworkQueue;

    LogMessages('S',me);
    
    // fragments must have the same packet id, this needs to be fixed to use sequential numbering at some time in the future
    if (bytesleft > MAXPACKETSIZE-sizeof(struct psNetPacket))
        id = GetRandomID();

    
    while (bytesleft > 0)
    {
        // The pktlen does not include the length of the headers, but the MAXPACKETSIZE does
        size_t pktlen = csMin(MAXPACKETSIZE-sizeof(struct psNetPacket), bytesleft);
        
        csRef<psNetPacketEntry> pNewPkt;
        pNewPkt.AttachNew(new psNetPacketEntry(me->priority, me->clientnum, id, (uint16_t)offset,
          (uint16_t)me->bytes->GetTotalSize(), (uint16_t)pktlen, me->bytes));

        //if (me->GetSequenceNumber())
        //  printf("Just created packet with sequence number %d.\n", me->GetSequenceNumber());

        if (!queue->Add(pNewPkt))
        {
            if(queue == NetworkQueue)
            {
                Error1("NetworkQueue full. Could not add packet.\n");
            }
            else
            {
                Error2("Target full. Could not add packet with clientnum %d.\n", me->clientnum);
            }
            return false;
        }
        else 
        {
            // printf("Sent packet.  Length %u of %u total.\n", pktlen, me->bytes->GetTotalSize());
        }
        
#ifndef CS_PLATFORM_WIN32
        // tell the network code there is a transmission waiting, this minimises lag time for sending network messages
        // DISABLED: This prevents multiple packets coalescing.
        //char notify = '!';
        //if(pipe_fd[1] > 0)
           // write(pipe_fd[1], &notify, 1);
#endif
        
        bytesleft -= pktlen;
        offset    += pktlen;
    }

    return true;
}


void NetBase::CheckFragmentTimeouts(void)
{
    csRef<psNetPacketEntry> pkt;

    // A set of packet ids that should NOT be discarded
    csSet<unsigned> newids;

    csRef<psNetPacketEntry> oldpackets[10];
    int count=0,index=0;
    csTicks current = csGetTicks();

    // Max age = 12 seconds

    // If we've been up less than 12 seconds, or the ticks just wrapped, don't check
    if (current<12000)
        return;

    // Iterate through all packets in the list searching for old packets
    csHash<csRef<psNetPacketEntry> , PacketKey>::GlobalIterator it(packets.GetIterator());
    while(it.HasNext())
    {
        pkt = it.Next();
        // If this id is already known to be new then go to the next one
        if(newids.In(pkt->packet->pktid))
            continue;

        // Remove fragment if timed out and (low priority or lost connection)
        if ((pkt->timestamp > current || pkt->timestamp < current-12000) && (pkt->packet->GetPriority() == PRIORITY_LOW || !GetConnByNum(pkt->clientnum)))
        {
            // Maximum of 10 ol
            if (count<10)
                oldpackets[count++]=pkt;
        }
        else // This id should not be discarded
            newids.AddNoTest(pkt->packet->pktid);
    }
     
    // Delete any old packet entries from the awaiting-assembly list, and also delete the packets themselves
    for (index=0;index<count;index++)
    {
        if(newids.In(oldpackets[index]->packet->pktid))
            continue;

        Debug2(LOG_NET,0,"Fragment reassembly timeout.  Discarding packet with packet id %u.\n", (oldpackets[index])->packet->pktid);
        /* Drop refcount until the packet is removed from the list.
         *  If we don't do this all at once we'll do this again the very next check since the time wont be altered.
         */
        while (packets.Delete(PacketKey(oldpackets[index]->clientnum, oldpackets[index]->packet->pktid), oldpackets[index]))
        {
            // all oldpackets should be reffed only by the oldpackets array
            // previous implementation deleted oldpüackets several times
        }
    }
}


bool NetBase::BuildMessage(psNetPacketEntry* pkt, Connection* &connection,
                           LPSOCKADDR_IN addr)
{
    if (connection)
    {
        // Update last packet time in connection
        connection->heartbeat          = 0;
        connection->lastRecvPacketTime = csGetTicks();
    }

    psNetPacket* packet = pkt->packet;


    // already received the whole packet?
    if (packet->offset == 0 && packet->pktsize == packet->msgsize) 
    {
        /* Before reading a message header, make sure the packet is large enough to contain a message header
        *  Note that at this point packet->pktsize should already be verified to be valid (within the length of read data)
        */
        if (packet->pktsize < sizeof(psMessageBytes))
        {
            Debug1(LOG_NET,connection->clientnum,"Dropping packet too short to contain message header.\n");
            return false;
        }

        psMessageBytes* msg = (psMessageBytes*) packet->data;
        // The packet's report of the message size should agree with the message header
        if (packet->msgsize != msg->GetTotalSize())
        {
            Debug3(LOG_NET,connection ? connection->clientnum : 0, "Dropping packet with different message size in packet header (%d) and message header (%zu).\n",
                packet->msgsize,msg->GetTotalSize());
        }
        else
        {
            csRef<MsgEntry> me;
            me.AttachNew(new MsgEntry(msg));
            me->priority = packet->flags;
            HandleCompletedMessage(me, connection, addr,pkt);
        }
        return false;
    }


    // This is a fragment. Add to the waiting packet list.  It may be removed ahead.
    packets.Put(PacketKey(pkt->clientnum, pkt->packet->pktid), pkt);

    // Build message from packet or add packet to existing partial message
    csRef<MsgEntry> me = 
        CheckCompleteMessage(pkt->clientnum,pkt->packet->pktid);
    if (me)
    {
        HandleCompletedMessage(me, connection, addr, pkt);
    }
                            
    return true;
}


csPtr<MsgEntry> NetBase::CheckCompleteMessage(uint32_t client, uint32_t id)
{
    csArray<csRef<psNetPacketEntry> > pkts;
    csRef<psNetPacketEntry> pkt;

    // This search is FAST, and without the first packet, you can't build the message.
    pkts = packets.GetAll(PacketKey(client, id));
    if (pkts.IsEmpty())
    {
#ifdef PACKETDEBUG
        Debug1(LOG_NET,0,"No First Packet found!?! Packets out of order, but should be no problem.\n");
        Debug2(LOG_NET,0,"MSGID was: %d\n", id);
#endif
        return NULL;
    }

    // Check total size of packets before bothering with msg building
    size_t total = 0;
    for (size_t i=0; i<pkts.GetSize(); i++)
    {
        total += pkts[i]->packet->pktsize;
    }
    if (total != pkts[0]->packet->msgsize)
    {
        // printf("Msg not complete yet. %u vs %u\n", total, pkts[0]->packet->msgsize);
        return NULL;  // not all packets are here yet
    }
    // Sort the packets on offsets
    pkts.Sort();
    
    pkt = pkts[0];

    unsigned int length=0, totallength = pkt->packet->msgsize;
    // Invalidated is set to true if something nasty is detected with the packets in this message and the whole message should be discarded
    bool invalidated=false;

    csRef<MsgEntry> me;
    me.AttachNew(new MsgEntry(totallength, pkt->packet->GetPriority(),pkt->packet->GetSequence()));    
    
    // if (pkt->packet->GetSequence() != 0)
    //  printf("Got packet sequence number %d.\n",pkt->packet->GetSequence());

    for (size_t i = 0; i < pkts.GetSize(); i++)
    {
        pkt = pkts[i];

        // Verify that all pieces have the same msgsize as the first piece
        if (totallength != pkt->packet->msgsize)
        {
            Warning3(LOG_NET,"Discarding invalid message.  Message fragments have inconsistant total message size.  First fragment reports %u. Later fragment reports %u.\n",
                              totallength,pkt->packet->msgsize);
            invalidated=true;
            break;
        }

        // Verify that no packet can overflow the message buffer
        if (pkt->packet->offset + pkt->packet->pktsize > totallength)
        {
            Warning4(LOG_NET,"Discarding MALICIOUS message.  Message fragment attempted to overflow message buffer.  Message size %u. Fragment offset %u.  Fragment length %u.\n",
                              totallength,pkt->packet->offset,pkt->packet->pktsize);
            invalidated=true;
            break;
        }


        // Copy the contents into the total message data in the right location
        memcpy( ((char *)(me->bytes)) + pkt->packet->offset, pkt->packet->data,
            pkt->packet->pktsize);
        // Add to our length count
        length += pkt->packet->pktsize;
        
    }

    /*  Early exit.
     *  We can return early only if we do not have to delete any packets from this list.
     *  This means we do not have a complete message and there is no sign of invalid data.
     */
    if (!invalidated && length != totallength) // sum of all packets is entire message
    {
        Error1("Passed initial check of msg size in packets but now not complete.");
        // We didn't get the entire message.  The smartpointer will take care of releasing the message object
        return NULL;
    }

    /* The entire message is now read or invalid.  A BinaryTreeIterator is NOT SAFE to use when deleting entries from the tree!
    *  This would be relatively slow if we had a lot of packets pending, but it's better than risking a crash.
    *
    * We start out with the known client id, packet id, and 0 offset.  We add the length field of the found packet
    *   to the offset of the test packet, delete the found packet and repeat until the total length is accounted for
    *   or we don't find any more (the latter should not happen).
    *
    */
    uint32_t offset = 0;
    while ((offset < totallength) && (pkt = packets.Get(PacketKey(client, id), NULL)))
    {
        offset+=pkt->packet->pktsize;

        // We don't care how many times it's in the queue, there's a mistake if it's over 1. Let's fix it now.
        packets.Delete(PacketKey(client, id), pkt);
    }

    /* If the search offset didnt finish at the end of the packet, we have gaps, not all pieces were deleted,
     * someone is trying to play games, and we should ignore the message, and 
     * TODO:  disconnect the connection (if connected) and blacklist the IP for a short period (5 minutes?)
     */
    if (!invalidated && offset != totallength)
    {
        Error3("Discarding MALICIOUS message.  Gaps in fragments attempt to capture data from uninitialized memory.  Missing packet with offset %u total length %u. (but total length of remaining fragments equals total message length).\n",
                            offset, totallength);
        invalidated=true;
    }

    // Something nasty was detected.  The message is invalid.
    if (invalidated)
        return NULL;
   
    // MsgEntry msg is filled in completely and the packet pieces that make up the message are deleted.
    return csPtr<MsgEntry> (me);
}


bool NetBase::QueueMessage(MsgEntry *me)
{
    bool success = true;
    for (size_t i=0; i<inqueues.GetSize(); i++)
    {
        if (!inqueues[i]->Add(me))
        {
            Error4("*** Input Buffer Full! Yielding for packet, client %u, type %s, input queue %zu!",
                   me->clientnum,GetMsgTypeName(me->GetType()).GetData(),i);

            // Block permanently on the full queue so we don't have a problem with fake acks
            CS_ASSERT(inqueues[i]->AddWait(me));
        }
        // printf("Just queued inbound message to be processed.\n");
    }
    return success;
}


void NetBase::HandleCompletedMessage(MsgEntry *me, Connection* &connection,
                                     LPSOCKADDR_IN addr, psNetPacketEntry* pkt)
{
    profs->AddReceivedMsg(me);
    
    if (!connection)
    {
        if (!HandleUnknownClient (addr, me))
        {
            // Authentication wasn't successfull
            CHECK_FINAL_DECREF(me,"Inbound completed msg");
            return;
        }

        // This one should work since handleUnknownClient was successful
        connection = GetConnByIP(addr);
        if (!connection)
        {
            Error1("couldn't get client by num...");
            return;
        }

        // Here we have a second chance to handle acking the packet
        // because at the first spot, the connection was unknown
        // so the packet wasn't acked.  The initial connection
        // packet must be acked here.
        HandleAck(pkt, connection, addr);
        

        // Update last packet time in connection
        // (Mostly handled in packet, but must be handled here for very first message)
        connection->heartbeat          = 0;
        connection->lastRecvPacketTime = csGetTicks();
    }

    // put the message in the queue
    me->clientnum = connection->clientnum;

    QueueMessage(me);
}


uint32_t NetBase::GetRandomID()
{
    CS::Threading::MutexScopedLock lock(mutex);
    return (unsigned) randomgen->Get(0x7fffffff) + 1;
}


// --------------------------------------------------------------------------


NetBase::Connection::Connection(uint32_t num): sequence(1), packethistoryhash(MAXPACKETHISTORY)
{
    pcknumin=0;
    pcknumout=0;
    clientnum=num;
    valid=false;
    heartbeat=0;
    lastRecvPacketTime = csGetTicks();
    window = 0;
    
    // Round trip time estimates
    estRTT = 0;
    devRTT = 0;
    sends = 0;
    resends = 0;

    RTO = PKTINITRTO;

    memset(&addr, 0, sizeof(SOCKADDR_IN));
    memset(&packethistoryid, 0, MAXPACKETHISTORY * sizeof(uint32_t));
    memset(&packethistoryoffset, 0, MAXPACKETHISTORY * sizeof(uint32_t));
    historypos = 0;
    buf = NULL;
}


NetBase::Connection::~Connection()
{
    if (buf) 
        cs_free(buf);
}

