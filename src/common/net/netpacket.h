/*
 * netpacket.h
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
#ifndef __NETPACKET_H__
#define __NETPACKET_H__

#include <csutil/csendian.h>
#include <csutil/refcount.h>
#include <csutil/hash.h>

#include "net/packing.h"
#include "net/message.h"

/**
 * \addtogroup common_net
 * @{ */

enum
{
  PKTSIZE_ACK    = 0,        // 0 pktsize means ACK pkt
  PKTMINRTO    = 250,        // Minimum of 250 mseconds till packet resend
  PKTMAXRTO = 60000,        // Maximum of 1 minute till packet resend
  PKTINITRTO = 3000,         // Initially assume 3 seconds till packet resend
  FLAG_MULTIPACKET = 0x02,   // priority bit 1 is multipacket on/off 
  /// MAXPACKETSIZE includes header length ( sizeof(struct psNetPacket) )
  MAXPACKETSIZE       = 1400    // 1400 bytes is max size of network packet
};

/**
 * psNetPacket gives the networking code
 * the context it needs, and all of this info
 * goes out on the wire.  None of NetPacketEntry's info goes
 * on the wire.
 * A packet is exactly one udp packet that goes over the net,
 *      you can embed multiple messages in one packet
 *      or you can split up one message over multiple
 *      packets
 */
#pragma pack(1)
struct psNetPacket
{
    /** each packet must know what message it's a part of
     *  this should be 0 in most cases as it's assigned at the last minute when sending
     *  depending on the connection it's sent on.
     */
    uint32_t    pktid;

    /** offset and size together specify a memory block within the psMessageBytes
     * struct 
     */
    uint32_t    offset;
    uint32_t    msgsize;
    uint16_t    pktsize;
    
    /** Each packet needs to know its own priority. This is stuck on the end
        to minimize alignment penalties */
    uint8_t    flags;
        
    /** this can be used as a pointer to the data */
    char    data[0];

    
    size_t GetPacketSize() const
    {
        if (pktsize == PKTSIZE_ACK)
            return sizeof(psNetPacket);
        else
            return sizeof(psNetPacket) + pktsize;
    };

    bool IsMultiPacket() const
    {
        return (flags & FLAG_MULTIPACKET)? true: false;
    }
    
    uint8_t GetPriority() const
    {
        return flags & PRIORITY_MASK;
    }

    uint8_t GetSequence() const
    {
        return flags>>2; // upper 6 bits of flags should be sequence id
    }

    void MarshallEndian() {
        // Pack up for transmission.  This endian-izes the packet.
        pktid = csLittleEndian::Convert((uint32)pktid);
        offset = csLittleEndian::Convert((uint32)offset);
        msgsize = csLittleEndian::Convert((uint32)msgsize);
        pktsize = csLittleEndian::Convert((uint16)pktsize);
    }

    void UnmarshallEndian() {
        // Unpack from transmission.  This deendian-izes the packet.
        pktid = csLittleEndian::UInt32(pktid);
        offset = csLittleEndian::UInt32(offset);
        msgsize = csLittleEndian::UInt32(msgsize);
        pktsize = csLittleEndian::UInt16(pktsize);
    }

    /*  The goal here is to verify fields that later parsing can't.
     *  Specifically we need to make sure the buffer can hold at least the 
     *  fields that must be present in every packet, and also that the reported
     *  packet length is less than or equal to the data provided in the buffer.
     *  (Really it should be the same length, but extra won't cause us to crash)
     */
    static struct psNetPacket *NetPacketFromBuffer(void *buffer,int buffer_length)
    {
        if ( !buffer )
            return NULL;

        struct psNetPacket *potentialpacket;
        
        if (buffer_length < (int)sizeof(struct psNetPacket))
            return NULL; // Buffer data too short for even the header
        
        potentialpacket=(struct psNetPacket *)buffer;
        
        if ((unsigned int)buffer_length < csLittleEndian::Convert(potentialpacket->pktsize) + sizeof(struct psNetPacket))
            return NULL; // Buffer data too short for the packet length reported in the header
        
        return potentialpacket;
    }

};
#pragma pack()


//-----------------------------------------------------------------------------


class PacketKey
{
    uint32_t clientnum;
    uint32_t pktid;
public:
    PacketKey(uint32_t Clientnum, uint32_t Pktid):clientnum(Clientnum), pktid(Pktid) { };
    
    bool operator < (const PacketKey& other) const
    {
        if (clientnum < other.clientnum)
            return true;
        if (clientnum > other.clientnum)
            return false;
        
        return (pktid < other.pktid);
    };
};


//-----------------------------------------------------------------------------


template<> class csHashComputer<PacketKey> : public csHashComputerStruct<PacketKey> 
{
};


//-----------------------------------------------------------------------------


class psNetPacketEntry : public csSyncRefCount
{
public:
    /** clientnum this packet comes from/goes to */
    uint32_t clientnum;

    csTicks  timestamp;
    
    /** has this packet been retransmitted */
    bool retransmitted;
    
    /** timeout */
    csTicks RTO;

    /** The Packet like it is returned from reading UDP socket / will be
     * written to socket
     */
    psNetPacket* packet;

    /** construct a new PacketEntry from a packet, not that this classe calls
     * free on the packet pointer later!
     */
    psNetPacketEntry (psNetPacket* packet, uint32_t cnum, uint16_t sz);
    
    /** construct a new PacketEntry for a single or partial message */
    psNetPacketEntry (uint8_t pri, uint32_t cnum, uint32_t id,
                      uint32_t off, uint32_t totalsize, uint16_t sz,
              psMessageBytes *msg);

    /** construct a new PacketEntry for a specified buffer of bytes */
    psNetPacketEntry (uint8_t pri, uint32_t cnum,
                      uint32_t id, uint32_t off, uint32_t totalsize, uint16_t sz,
                      const char *bytes);

    psNetPacketEntry (psNetPacketEntry* )
    {
        CS_ASSERT(false);
    }

    ~psNetPacketEntry();
    
    bool Append(psNetPacketEntry* next);
    csPtr<psNetPacketEntry> GetNextPacket(psNetPacket* &packetdata);


    void* GetData()
    {
        return packet;
    }

    bool operator < (const psNetPacketEntry& other) const
    {
        if (clientnum < other.clientnum)
            return true;
        if (clientnum > other.clientnum)
            return false;
    
        if (packet->pktid < other.packet->pktid)
            return true;
        if (packet->pktid > other.packet->pktid)
            return false;
    
        if (packet->offset < other.packet->offset)
            return true;

        return false;
    };

    /////////////////////////////////
    // Dummy functions required by GenericQueue
    ////////////////////////////////////
    void SetPending(bool /*flag*/)
    {   }
    bool GetPending()
    { return false; }
};


//-----------------------------------------------------------------------------


template<>
class csComparator<csRef<psNetPacketEntry> , csRef<psNetPacketEntry> >
{
public:
    static int Compare(csRef<psNetPacketEntry> const &r1, csRef<psNetPacketEntry> const &r2)
    {
        return csComparator<psNetPacketEntry, psNetPacketEntry>::Compare(*r1, *r2);
    }
};

/** @} */

#endif

