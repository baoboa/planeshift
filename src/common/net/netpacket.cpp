/*
 * netpacket.cpp
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

#include <csutil/sysfunc.h>  // csTicks

#include "util/log.h"
#include "net/netpacket.h"

// #define PACKETDEBUG

psNetPacketEntry::psNetPacketEntry (psNetPacket* packet, uint32_t cnum,
                    uint16_t sz)
    : clientnum(cnum), packet(packet)
{
    packet->pktsize = sz - sizeof(psNetPacket);
    timestamp = csGetTicks();
    retransmitted = false;
    RTO = 0;
}


/** construct a new PacketEntry for a single or partial message */
psNetPacketEntry::psNetPacketEntry (uint8_t pri, uint32_t cnum,
                    uint32_t id, uint32_t off,
                    uint32_t totalsize, uint16_t sz,
                    psMessageBytes *msg)
{
    packet = (psNetPacket*) cs_malloc (sizeof(psNetPacket) + sz);
    CS_ASSERT(packet != NULL);
    clientnum = cnum;
    packet->flags = pri;
    packet->pktid = id;
    packet->offset = off;
    packet->pktsize = sz;
    packet->msgsize = totalsize;
    timestamp = csGetTicks();
    retransmitted = false;
    RTO = 0;
    if (msg && sz && sz != PKTSIZE_ACK)
        memcpy(packet->data, ((char *)msg) + off, sz);
}


psNetPacketEntry::psNetPacketEntry (uint8_t pri, uint32_t cnum,
    uint32_t id, uint32_t off, uint32_t totalsize, uint16_t sz,
    const char *bytes)
{
    packet = (psNetPacket*) cs_malloc (sizeof(psNetPacket) + sz);
    CS_ASSERT(packet != NULL);
    clientnum = cnum;
    packet->flags = pri;
    packet->pktid = id;
    packet->offset = off;
    packet->pktsize = sz;
    packet->msgsize = totalsize;
    timestamp = csGetTicks();
    retransmitted = false;
    RTO = 0;
    if (bytes && sz && sz != PKTSIZE_ACK)
    memcpy(packet->data, bytes, sz);
}


psNetPacketEntry::~psNetPacketEntry()
{
    if (packet)
        cs_free(packet);
}


bool psNetPacketEntry::Append(psNetPacketEntry* next)
{
#ifdef PACKETDEBUG
    Debug5(LOG_NET,0,"Appending packet into MULTIPACKET (ID %d Size: %d). (ID %d Size: %d)\n", 
        packet->pktid, packet->GetPacketSize(), next->packet->pktid, next->packet->GetPacketSize());
#endif
    
    /// We do not allow multipackets to be appended to multipackets.
    // if the pktid > 0, then this is a resent packet so no appending.
    if(next->packet->IsMultiPacket() || packet->pktid > 0 || next->packet->pktid > 0)
        return false;

    if(next->clientnum != clientnum)
    {
        Error1("Attempted to merge two packets with different destinations!");
        // This should not happen
        return false;
    }
    /**
    * If the msgid is not MULTIPACKET, then we need to set up
    * the initial merged packet structure before copying the 
    * second packet in.
    */
    if (! packet->IsMultiPacket())
    {
        // Check for size
        if (packet->GetPacketSize() + next->packet->GetPacketSize() +
            sizeof(psNetPacket) >= MAXPACKETSIZE)
            return false;
        psNetPacket *merge;

        /**
        * Allocate the MULTIPACKET to max size, so we don't have to realloc
        * or copy data more than once.  Only exact number of bytes will be
        * sent on the wire.
        */
        merge = (psNetPacket*) cs_malloc (MAXPACKETSIZE);
        CS_ASSERT(merge != NULL);

        /**
        * priority is copied from first packet now.  Later, if any packet merged is HIGH
        * then the whole packet is HIGH
        */
        merge->flags = packet->flags | FLAG_MULTIPACKET;
        merge->pktid = 0;
        merge->offset = 0;
        merge->pktsize = (uint16_t)packet->GetPacketSize();
        merge->msgsize = merge->pktsize;

        /**
        * After marshalling for network, copy entire first packet, with header, into data section
        * of new packet.
        */
        uint16_t size = (uint16_t)packet->GetPacketSize();
        packet->MarshallEndian();
        memcpy(merge->data, packet, size);
        packet->UnmarshallEndian();

        cs_free(packet);   // done with old packet
        packet = merge;
    }
    else
    {
        if (packet->GetPacketSize() + next->packet->GetPacketSize() >= MAXPACKETSIZE)
            return false;
    }

    /**
    * now copy the second packet onto the end of the merged one
    * and update appropriate header fields
    */
    if (next->packet->GetPriority() == PRIORITY_HIGH)
        packet->flags = PRIORITY_HIGH | FLAG_MULTIPACKET; // HIGH overrides LOW but not vice versa

    /* Copy size information temporarily, and pack the next packet for transmission.
    * Do not reference the next->packet after MarshallEndian() has been called.
    * copy the entire 2nd packet into 1st packet after existing data
    */
    uint16_t nextSize = (uint16_t)next->packet->GetPacketSize();
    next->packet->MarshallEndian();
    memcpy(packet->data+packet->pktsize, next->packet, nextSize);
    next->packet->UnmarshallEndian();

    /**
    * now update length of outer packet
    */
    packet->pktsize += nextSize;
    packet->msgsize = packet->pktsize;
    timestamp = csGetTicks();

    return true;
}


csPtr<psNetPacketEntry> psNetPacketEntry::GetNextPacket(psNetPacket * &packetdata)
{
    if (!packet->IsMultiPacket())
    {
        packetdata = NULL;
        // csPtr() doesn't do an IncRef but the return value will be
        // DecRef()'ed so we have to account for that here.
        this->IncRef();
        return csPtr<psNetPacketEntry>(this);
    }
    else
    {
        if (!packetdata) 
        {
            // There must be at least enough data in the packet for the header
            if (packet->pktsize < sizeof(psNetPacket))
            {
                // Only debug because we don't have any information about the remote client here, so this isn't useful as a security warning
                Debug1(LOG_NET,0,"Dropped multipacket too short to contain even one internal packet header.\n");
                return NULL;
            }
            
            packetdata = (psNetPacket *)packet->data;  // we need the packet INSIDE the packet
            packetdata->UnmarshallEndian();
            
            /* The packet size plus offset must be less than or equal to the message size in all sub packets of a multipacket 
             * (packet cannot terminate after end of messsage)
             */
            if (packetdata->offset + packetdata->pktsize > packetdata->msgsize)
            {
                // Only debug because we don't have any information about the remote client here, so this isn't useful as a security warning
                Debug4(LOG_NET,0,"Dropped multipacket containing internal packet with bad header. offset=%d msgsize=%d  pktsize=%d\n",
                    packetdata->offset,packetdata->msgsize,packetdata->pktsize);
                packetdata=NULL;
                return NULL;
            }

            // There also must be enough data for the contained message section
            if (packet->pktsize < (int)(packetdata->data - packet->data) + packetdata->pktsize)
            {
                // Only debug because we don't have any information about the remote client here, so this isn't useful as a security warning
                Debug2(LOG_NET,0,"Dropped multipacket containing internal packet with invalid length in header. pktsize=%d\n",packet->pktsize);
                packetdata=NULL;
                return NULL;
            }
        }
        else
        {
            // We've been handed a partially unpacked packet.
            packetdata = (psNetPacket *)((char *)packetdata + packetdata->GetPacketSize());

            // Before we convert endian, make sure we are within the buffer
            if ((packet->pktsize - (int)((char *)packetdata - packet->data)) < (int)sizeof(psNetPacket))
            {
                // This just means we've reached the end of the buffer
                packetdata=NULL;
                return NULL;
            }

            packetdata->UnmarshallEndian();

            /* The packet size plus offset must be less than or equal to the message size in all sub packets of a multipacket 
             * (packet cannot terminate after end of messsage)
             */
            if (packetdata->offset + packetdata->pktsize > packetdata->msgsize)
            {
                // Only debug because we don't have any information about the remote client here, so this isn't useful as a security warning
                Debug4(LOG_NET,0,"Dropped remainder of multipacket containing internal packet with bad header. offset=%d msgsize=%d  pktsize=%d\n",
                    packetdata->offset,packetdata->msgsize,packetdata->pktsize);
                packetdata=NULL;
                return NULL;
            }

            // There also must be enough data for the contained message section
            if (packet->pktsize < (int)(packetdata->data - packet->data) + packetdata->pktsize)
            {
                // Only debug because we don't have any information about the remote client here, so this isn't useful as a security warning
                Debug2(LOG_NET,0,"Dropped remainder of multipacket containing internal packet with invalid length in header. pktsize=%d\n",packet->pktsize);
                packetdata=NULL;
                return NULL;
            }
        }

#ifdef PACKETDEBUG
        Debug4(LOG_NET,0,"Extracting packet (ID %d) from MULTIPACKET (%p - %d).\n", 
        packetdata->pktid, this, packet->pktid);
#endif

        psNetPacketEntry* pnew = new psNetPacketEntry (packetdata->flags,
                                                       clientnum,
                                                       packetdata->pktid,
                                                       packetdata->offset,
                                                       packetdata->msgsize,
                                                       packetdata->pktsize, 
                                                       (const char *)packetdata->data);

        return csPtr<psNetPacketEntry>(pnew);
    }
}

