/*
 * message.h
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
 * This finally defines the format of the queue entries for the communication
 * with the network thread
 *
 * Author: Matthias Braun <MatzeBraun@gmx.de>
 */

#ifndef __MSGQUEUE_H__
#define __MSGQUEUE_H__

#include <csutil/threading/atomicops.h>
#include <csutil/refcount.h>
#include <csutil/csendian.h>
#include <csutil/strhashr.h>
#include <csutil/strset.h>
#include <csgeom/vector3.h>
#include <iengine/sector.h>
#include <iengine/engine.h>
#include <csutil/csobject.h>

#include "util/log.h"
#include "net/packing.h"
#include "net/pstypes.h"
#include "util/genrefqueue.h"

using namespace CS::Threading;

/**
 * \addtogroup common_net
 * @{ */

enum // note that 0x02 (bit1) is also reserved for marking messages as multipacket
{
    PRIORITY_LOW  = 0x00,
    PRIORITY_HIGH = 0x01,
    PRIORITY_MASK = 0x01
};

// 8 bit should hopefully be enough
typedef uint8_t msgtype;


/*
 * csSyncRefCount is safe in most cases. A case where it isn't is when the
 * read to check for null is done just as something tries to inc (and the
 * read is done first).
 * If the code is written correctly, this should never happen as otherwise
 * the thread that is trying to inc would be reading deleted data.
 */
class csSyncRefCount
{
protected:
    int32 ref_count;

    virtual ~csSyncRefCount () {}

public:
    /// Initialize object and set reference to 1.
    csSyncRefCount() : ref_count(1)
    {
    }

    /// Increase the number of references to this object.
    void IncRef ()
    {
        AtomicOperations::Increment(&ref_count);
    }

    /// Decrease the number of references to this object.
    void DecRef ()
    {
        if(AtomicOperations::Decrement(&ref_count) == 0)
        {
            CS_ASSERT_MSG("Race condition on destroying csSyncRef pointer", ref_count == 0);
            delete this;
        }
    }

    /// Get the reference count (only for debugging).
    int32 GetRefCount()
    {
        return AtomicOperations::Read(&ref_count);
    }
};


//-----------------------------------------------------------------------------


/**
 * this struct represents the data that is sent out through the network (all
 * additional stuff should go into the struct MsgEntry
 */
#pragma pack (1)
struct psMessageBytes
{
    msgtype  type;       ///< Version
    uint16_t size;       ///< the size of the following data
    char     payload[0]; ///< this can be used as a pointer to the data

    size_t GetTotalSize() const { return sizeof(psMessageBytes) + csLittleEndian::Convert(size); }
    size_t GetSize() const      { return csLittleEndian::Convert(size); }
    void SetSize(size_t len)      { size = csLittleEndian::Convert((uint16)(len & 0xFFFF)); }
    void SetTotalSize(size_t len) { size = csLittleEndian::Convert((uint16)((len-sizeof(psMessageBytes)) & 0xFFFF)); }
};
// set to default packing
#pragma pack()

/*  To protect the server from a client with ill intent sending a simply HUGE message that requires tons of
 *  memory to reassemble, messages beyond this size will be dropped.
 *  Changing this value requires a NETVERSION change in messages.h.
 */
const unsigned int MAX_MESSAGE_SIZE = 65535 - sizeof(psMessageBytes) - 1;  // Current max, -1 for safety

#define MSG_SIZEOF_VECTOR2   (2*sizeof(uint32))
#define MSG_SIZEOF_VECTOR3   (3*sizeof(uint32))
#define MSG_SIZEOF_VECTOR4   (4*sizeof(uint32))
#define MSG_SIZEOF_SECTOR    100  // Sector can be a string in the message!!!
#define MSG_SIZEOF_FLOAT     sizeof(uint32)

//-----------------------------------------------------------------------------


/**
 * The structure of 1 queue entry (pointer to a message)
 */
class MsgEntry : public csSyncRefCount
{
public:
    MsgEntry (size_t datasize = 0, uint8_t msgpriority=PRIORITY_HIGH, uint8_t sequence=0)
        : clientnum(0), priority((sequence << 2) | msgpriority), msgid(0), overrun(false)
    {
        if (sequence && msgpriority==PRIORITY_LOW)
        {
            Error1("MsgEntry created with sequenced delivery but not guaranteed delivery.  This is not reliable and probably won't work.");
        }

        if (datasize > MAX_MESSAGE_SIZE)
        {
            Debug3(LOG_NET,0,"Call to MsgEntry construction truncated data.  Requested size %u > max size %u.\n",(unsigned int)datasize,(unsigned int)MAX_MESSAGE_SIZE);
            datasize=MAX_MESSAGE_SIZE;
        }

        bytes = (psMessageBytes*) cs_malloc(sizeof(psMessageBytes) + datasize);
        CS_ASSERT(bytes != NULL);

        current = 0;
        bytes->SetSize(datasize);
    }

    MsgEntry (const psMessageBytes* msg)
        : clientnum(0), priority(PRIORITY_LOW), msgid(0), overrun(false)
    {
        size_t msgsize = msg->GetTotalSize();
        if (msgsize > MAX_MESSAGE_SIZE)
        {
            Debug2(LOG_NET,0,"Call to MsgEntry construction (from network message psMessageBytes) truncated data.  Source data > %u length.\n",MAX_MESSAGE_SIZE);
            msgsize = MAX_MESSAGE_SIZE;
        }

        current = 0;

        bytes = (psMessageBytes*) cs_malloc(msgsize);
        CS_ASSERT(bytes != NULL);

        memcpy (bytes, msg, msgsize);
        // If we truncated the message, make sure the size reflects that
        bytes->SetTotalSize(msgsize);
    }

    MsgEntry (const MsgEntry* me)
    {
        size_t msgsize = me->bytes->GetTotalSize();
        if (msgsize > MAX_MESSAGE_SIZE)
        {
            Bug2("Call to MsgEntry copy constructor truncated data.  Source data > %u length.\n",MAX_MESSAGE_SIZE);
            msgsize = MAX_MESSAGE_SIZE;
        }
        bytes = (psMessageBytes*) cs_malloc(msgsize);
        CS_ASSERT(bytes != NULL);
        memcpy (bytes, me->bytes, msgsize);

        // If we truncated the message, make sure the size reflects that
        bytes->SetTotalSize(msgsize);

        clientnum = me->clientnum;
        priority = me->priority;
        msgid = me->msgid;
        current = 0;
        overrun = false;
    }

    virtual ~MsgEntry()
    {
        cs_free ((void*) bytes);
    }

    void ClipToCurrentSize()
    {
        bytes->SetSize(current);
    }

    void Reset(int pos = 0) { current = pos; }

    /// Add a string to the current psMessageBytes buffer
    void Add(const char *str)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const char *) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        if ( str == 0 )
        {
            // No space left!  Don't overwrite the buffer.
            if (current + 1 > bytes->GetSize())
            {
                Bug4("MsgEntry::Add(const char *) call for msgid=%u len=%u would overflow buffer! str = (null) type = %u\n",
                    msgid,0, bytes->type);
                overrun=true;
                return;
            }
            bytes->payload[current] = 0;
            current++;
            return;
        }

        // Not enough space left!  Don't overwrite the buffer.
        if (current + strlen(str)+1 > bytes->GetSize())
        {
            Bug6("MsgEntry::Add(const char *) call for msgid=%u len=%u would overflow buffer! str = %s type = %u size %zu\n",
                (unsigned int)msgid,(unsigned int)strlen(str),str, bytes->type, bytes->GetSize());
            overrun=true;
            return;
        }

        strcpy(bytes->payload+current,str);
        current += strlen(str)+1;
    }

    /// Add a float to the current psMessageBytes buffer
    void Add(const float f)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const float) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(uint32) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const float) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint32 *pf = (uint32 *)(bytes->payload+current);
        *pf = csLittleEndian::Convert( csIEEEfloat::FromNative(f) );
        current += sizeof(uint32);
    }

    /// Add a double the current psMessageBytes buffer
#if 0
    // disabled for now, because CS has no convert function for double :-(
    void Add(const double d)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const double) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(double) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const double) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        double* p = (double*) (bytes->payload+current);
        *p = csLittleEndian::Convert(d);
        current += sizeof(double);
    }
#endif

    /// Add unsigned 16 bit int to current psMessageBytes buffer
    void Add(const uint16_t s)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const uint16_t) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(uint16_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const uint16_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint16_t *p = (uint16_t*) (bytes->payload+current);
        *p = csLittleEndian::Convert(s);
        current += sizeof(uint16_t);
    }

    /// Add signed 16 bit int to current psMessageBytes buffer
    void Add(const int16_t s)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const int16_t) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(int16_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const int16_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        int16_t *p = (int16_t *)(bytes->payload+current);
        *p = csLittleEndian::Convert(s);
        current += sizeof(int16_t);
    }
    /// Add unsigned 32Bit Int to current psMessageBytes buffer
    void Add(const int32_t i)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const int32_t) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(int32_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const int32_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        int32_t *p = (int32_t *)(bytes->payload+current);
        *p = csLittleEndian::Convert((int32) i);
        current += sizeof(int32_t);
    }

       /// Add unsigned 4byte int to current psMessageBytes buffer

    void Add(const uint32_t i)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const uint32_t) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(uint32_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const uint32_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint32_t *p = (uint32_t *)(bytes->payload+current);
        *p = csLittleEndian::Convert((uint32) i);
        current += sizeof(uint32_t);
    }

      /// Add a pointer to the current psMessageBytes buffer. Pointers must never be sent over the network!

    void AddPointer(const uintptr_t i)
     {
         if (bytes == NULL)
         {
             Bug1("MsgEntry::Add(const uintptr_t) bytes=NULL!\n");
             CS_ASSERT(false);
             return;
         }

         // If the message is in overrun state, don't add anymore, it's already invalid
         if (overrun)
             return;

        // Not enough space left!  Don't overwrite the buffer.
         if (current + sizeof(uintptr_t) > bytes->GetSize())
         {
             Bug3("MsgEntry::Add(const uintptr_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
             overrun=true;
             return;
         }

         uintptr_t *p = (uintptr_t *)(bytes->payload+current);
         *p = i; // Pointers can only be used locally, and the endian won't change locally!
         current += sizeof(uintptr_t);
     }

       /// Add 8bit unsigned int to current psMessageBytes buffer

    void Add(const uint8_t i)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const uint8_t) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(uint8_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const uint8_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint8_t *p = (uint8_t *)(bytes->payload+current);
        *p = i; // no need to endian convert a byte :)
        current += sizeof(uint8_t);
    }

    /// Add 8 bit signed int to buffer
    void Add(const int8_t i)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const int8_t) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(int8_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const int8_t) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        int8_t *p = (int8_t*) (bytes->payload+current);
        *p = i; // no need to endian convert a byte :)
        current += sizeof(int8_t);
    }

    /// Add bool to current psMessageBytes buffer
    void Add(const bool b)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const bool) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(uint8_t) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const bool) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint8_t t = b ? 1 : 0;
        Add(t);
    }

    /// Add an x,y vector to the buffer
    void Add(const csVector2& v)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const csVector2) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + 2*sizeof(uint32) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const csVector2) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint32 *pf = (uint32 *)(bytes->payload+current);
        *(pf++) = csLittleEndian::Convert( csIEEEfloat::FromNative(v.x) );
        *pf = csLittleEndian::Convert( csIEEEfloat::FromNative(v.y) );
        current += 2*sizeof(uint32);
    }

    /// Add an x,y,z vector to the buffer
    void Add(const csVector3& v)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const csVector3) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + 3*sizeof(uint32) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const csVector3) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint32 *pf = (uint32 *)(bytes->payload+current);
        *(pf++) = csLittleEndian::Convert( csIEEEfloat::FromNative(v.x) );
        *(pf++) = csLittleEndian::Convert( csIEEEfloat::FromNative(v.y) );
        *pf = csLittleEndian::Convert( csIEEEfloat::FromNative(v.z) );
        current += 3*sizeof(uint32);
    }

    /// Add an x,y,z,w vector to the buffer
    void Add(const csVector4& v)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const csVector4) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + 4*sizeof(uint32) > bytes->GetSize())
        {
            Bug3("MsgEntry::Add(const csVector4) call for msgid=%u would overflow buffer! type = %u\n",msgid, bytes->type);
            overrun=true;
            return;
        }

        uint32 *pf = (uint32 *)(bytes->payload+current);
        *(pf++) = csLittleEndian::Convert( csIEEEfloat::FromNative(v.x) );
        *(pf++) = csLittleEndian::Convert( csIEEEfloat::FromNative(v.y) );
        *(pf++) = csLittleEndian::Convert( csIEEEfloat::FromNative(v.z) );
        *pf = csLittleEndian::Convert( csIEEEfloat::FromNative(v.w) );
        current += 4*sizeof(uint32);
    }
    
    void Add(const iSector* sector, const csStringSet* msgstrings, const csStringHashReversible* msgstringshash = NULL)
    {
        const char* sectorName = const_cast<iSector*>(sector)->QueryObject()->GetName ();
        csStringID sectorNameStrId = csInvalidStringID;
        if (msgstrings)
        {
            /// TODO: Should not need to call Contains, but as long as the request function
            //        assign new IDs we have to check first.
            if (msgstrings->Contains(sectorName))
            {
                sectorNameStrId =  const_cast<csStringSet*>(msgstrings)->Request(sectorName);
            }
            else
            {
                sectorNameStrId = csInvalidStringID;
            }
        } else if (msgstringshash)
        {
            sectorNameStrId = msgstringshash->Request(sectorName);
        }
        
        Add( (uint32_t) sectorNameStrId );
        
        if (sectorNameStrId == csInvalidStringID)
        {
            Add(sectorName);
        }
    }

    void Add(const iSector* sector);

    /// Add a processed buffer of some kind; should only be used by files and the like.
    // NOTE THIS IS NOT ENDIAN-CONVERTED:  YOUR DATA MUST ALREADY BE ENDIAN SAFE.
    void Add(const void *datastream, const uint32_t length)
    {
        if (bytes == NULL)
        {
            Bug1("MsgEntry::Add(const void *datastream,const uint32_t length) bytes=NULL!\n");
            CS_ASSERT(false);
            return;
        }

        // If the message is in overrun state, don't add anymore, it's already invalid
        if (overrun)
            return;

        // Not enough space left!  Don't overwrite the buffer.
        if (current + sizeof(uint32_t) + length > bytes->GetSize())
        {
            Bug5("MsgEntry::Add(const void *datastream,const uint32_t length) call for msgid=%u len=%u would overflow buffer of size %zu! type = %u\n",
                msgid,length, bytes->GetSize(), bytes->type);
            overrun=true;
            return;
        }

        Add(length);

        if (length!=0)
        {
            void *currentloc = (void *) (bytes->payload+current);
            memcpy(currentloc, datastream, length);
            current += length;
        }
    }

    /// Check if we are at the end of the current psMessageBytes buffer
    bool IsEmpty()
    {
        return current >= bytes->GetSize();
    }

    /// Check if we have more data in the current psMessageBytes buffer
    bool HasMore(int howMuchMore = 1)
    {
        return current+howMuchMore <= bytes->GetSize();
    }

    /// Get a null-terminated string from the current psMessageBytes buffer
    const char *GetStr()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return NULL;

        /* Verify that the string ends before the buffer does
         *  When this finishes, current is advanced to the end of the buffer or
         *  a terminating character (0x00) - whichever is reached first.
         */
        size_t position=current;
        while (current < bytes->GetSize() && bytes->payload[current]!=0x00)
        {
            current++;
        }
        
        if (current>=bytes->GetSize())
        {
            Debug3(LOG_NET,0,"Message id %u would have read beyond end of buffer %zu.\n",
                   msgid,current);
            overrun=true;
            return NULL;
        }

        // Advance 1 past the terminating character
        current++;

        // Return NULL instead of empty string(s)
        if ( bytes->payload[position] == 0 )
            return NULL;

        return bytes->payload+position;
    }

    /// Get a float from the current psMessageBytes buffer
    float GetFloat()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0.0f;

        if (current+sizeof(float) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0.0f;
        }

        uint32 *p = (uint32 *)(bytes->payload+current);
        current += sizeof(uint32);
        return csIEEEfloat::ToNative( csLittleEndian::Convert(*p) );
    }
    /// Get a signed 16bit Integer from the current psMessageBytes buffer
    int16_t GetInt16()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(int16_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        int16_t *p = (int16_t*) (bytes->payload+current);
        current += sizeof(int16_t);
        return csLittleEndian::Convert(*p);
    }
    /// Get a unsigned 16bit Integer from the curren psMessageBytes buffer
    uint16_t GetUInt16()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(uint16_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uint16_t *p = (uint16_t*) (bytes->payload+current);
        current += sizeof(uint16_t);
        return csLittleEndian::Convert(*p);
    }

    /// Get a signed 32Bit Integer from the current psMessageBytes buffer
    int32_t GetInt32()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(int32_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        int32_t *p = (int32_t*) (bytes->payload+current);
        current += sizeof(int32_t);
        return csLittleEndian::Convert(*p);
    }

    /// Get an unsigned 4byte int from the current psMessageBytes buffer
    uint32_t GetUInt32()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(uint32_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uint32_t *p = (uint32_t *)(bytes->payload+current);
        current += sizeof(uint32_t);
        return csLittleEndian::UInt32(*p);
    }

    /// Get a pointer from the current psMessageBytes buffer. Pointers must never be sent over the network!
    uintptr_t GetPointer()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(uintptr_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uintptr_t *p = (uintptr_t *)(bytes->payload+current);
        current += sizeof(uintptr_t);
        return *p;
    }

    /// Get a signed 8bit Integer from the current psMessageBytes buffer
    int8_t GetInt8()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(int8_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        int8_t *p = (int8_t*) (bytes->payload+current);
        current += sizeof(int8_t);
        return *p; // no need to convert bytes
    }
    /// Get an unsigned 8Bit Integer from the current psMessageBytes buffer
    uint8_t GetUInt8()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+sizeof(uint8_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uint8_t *p = (uint8_t *)(bytes->payload+current);
        current += sizeof(uint8_t);
        return *p; // no need to convert bytes
    }
    /// Get a bool from the current psMessageBytes buffer
    bool GetBool()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return false;

        if (current+sizeof(uint8_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return false;
        }

        return (GetUInt8() != 0);
    }

    csVector2 GetVector2()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+2*sizeof(uint32) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uint32 *p = (uint32 *)(bytes->payload+current);
        csVector2 v;
        v.x = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        v.y = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        current += 2*sizeof(uint32);
        return v;
    }

    csVector3 GetVector3()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+3*sizeof(uint32) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uint32 *p = (uint32 *)(bytes->payload+current);
        csVector3 v;
        v.x = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        v.y = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        v.z = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        current += 3*sizeof(uint32);
        return v;
    }

    csVector4 GetVector4()
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return 0;

        if (current+4*sizeof(uint32) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            overrun=true;
            return 0;
        }

        uint32 *p = (uint32 *)(bytes->payload+current);
        csVector4 v;
        v.x = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        v.y = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        v.z = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        v.w = csIEEEfloat::ToNative( csLittleEndian::Convert(*(p++)) );
        current += 4*sizeof(uint32);
        return v;
    }
    
    iSector* GetSector(const csStringSet* msgstrings, const csStringHashReversible* msgstringshash, iEngine *engine)
    {
        csString sectorName;
        csStringID sectorNameStrId;
        sectorNameStrId = GetUInt32();
        if (sectorNameStrId != csStringID(uint32_t(csInvalidStringID)))
        {
            if(msgstrings)
            {
                sectorName = msgstrings->Request(sectorNameStrId);
            }
            else if(msgstringshash)
            {
                sectorName = msgstringshash->Request(sectorNameStrId);
            }
        }
        else
        {
            sectorName = GetStr();
        }
        
        if(!sectorName.IsEmpty())
        {
            return engine->GetSectors ()->FindByName (sectorName);
        }

        return NULL;
    }
    

    iSector* GetSector();

    
    // Get a pre-converted data buffer with recorded length from the current psMessageBytes buffer.
    void * GetBufferPointerUnsafe(uint32_t& length)
    {
        // If the message is in overrun state, we know we can't read anymore
        if (overrun)
            return NULL;

        if (current+sizeof(uint32_t) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            length=0;
            overrun=true;
            return NULL;
        }

        length = GetUInt32();
        if (length==0) 
			return NULL;

        if (current + (length) > bytes->GetSize())
        {
            Debug2(LOG_NET,0,"Message id %u would have read beyond end of buffer.\n",msgid);
            length=0;
            overrun=true;
            return NULL;
        }

        void *datastream = bytes->payload+current;
        current += length;
        return datastream;  // Not safe to use this pointer after the MsgEntry goes out of scope!
    }

    void SetType(uint8_t type)
    {
        bytes->type = type;
    }
    uint8_t GetType()
    {
        return bytes->type;
    }

    int GetSequenceNumber()
    {
        return priority >> 2;
    }

    /////////////////////////////////
    // Dummy functions required by GenericQueue
    ////////////////////////////////////
    void SetPending(bool /*flag*/)
    { }
    bool GetPending()
    { return false; }

    /// Return the size of the databuffer
    size_t GetSize()
    { return bytes->GetSize();}


public:
    /** The Number of the client the Message is from/goes to */
    uint32_t clientnum;

    /** The current position for Pushing and Popping */
    size_t current;

    /** The priority this message should be handled */
    uint8_t priority;

    /** Unique identifier per machine */
    uint32_t msgid;

    /** Indicates wether a read or write overrun has occured */
    bool overrun;


    /** The message itself ie one block of memory with the
     * header and data following
     */
    psMessageBytes *bytes;
};

/** @} */

#endif



