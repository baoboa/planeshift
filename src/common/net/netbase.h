/*
 * netbase.h by Matze Braun <MatzeBraun@gmx.de>
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
 * This file defines some inline functions which are used in client and server
 * network functions, the client/server interface classes should be derived from
 * this base class
 */
#ifndef __NETBASE_H__
#define __NETBASE_H__

#include "net/pstypes.h"
#include "net/netinfos.h"
#include "net/netpacket.h"
#include "util/genrefqueue.h"
#include <csutil/ref.h>
#include <csutil/weakref.h>
#include <csutil/weakreferenced.h>
#include <csutil/refcount.h>
#include <csutil/strset.h>
#include <csutil/array.h>
#include "netprofile.h"

/**
 * \addtogroup common_net
 * @{ */

#define NUM_BROADCAST        0xffffffff
#define MAXQUEUESIZE        20000
#define MAXCLIENTQUEUESIZE  5000
#define MAXPACKETHISTORY    1009 // Should be a prime to improve performance of hash
                                 // This must be set carefully and ideally should be at least the size
                                 // of the input queue
#define NETAVGCOUNT 400
#define RESENDAVGCOUNT 200

const unsigned int WINDOW_MAX_SIZE = 65536; // The size of the maximum reliable window in bytes.

// The number of times the SendTo function will retry on a EAGAIN or EWOULDBLOCK
#define SENDTO_MAX_RETRIES   200
// The whole seconds that the SendTo function will block each cycle waiting for the write state to change on the socket
#define SENDTO_SELECT_TIMEOUT_SEC 0
// The microseconds (1/10^6 s) that the SendTo function will block each cycle waiting for the write state to change on the socket
#define SENDTO_SELECT_TIMEOUT_USEC 10000

/* Include platform specific socket settings */
#ifdef USE_WINSOCK
#   include "net/sockwin.h"
#endif
#ifdef USE_UNISOCK
#   include "net/sockuni.h"
#endif

// Jorrit: hack for mingw.
#ifdef SendMessage
#undef SendMessage
#endif
#ifdef SetJob
#undef SetJob
#endif
#ifdef GetJob
#undef GetJob
#endif

class MsgEntry;
class NetPacketQueueRefCount;
class csRandomGen;
class csStringHashReversible;
struct iEngine;
typedef GenericRefQueue <MsgEntry> MsgQueue;
typedef GenericRefQueue <psNetPacketEntry> NetPacketQueue;

/*
 * Microsoft sockets do not use Berkeley error codes or functions in all cases.
 * This is the workaround.
 * http://msdn.microsoft.com/en-us/library/ms737828(VS.85).aspx
 */
#ifdef CS_PLATFORM_WIN32
#undef errno
#define errno WSAGetLastError()
#else
#define WSAEWOULDBLOCK EAGAIN
#endif


struct PublishDestination
{
    uint32_t client;
    void*    object;
    float    dist;
    float    min_dist;

    PublishDestination(int client, void* object, float dist, float min_dist) : client(client), object(object), dist(dist), min_dist(min_dist) {}
};

//-----------------------------------------------------------------------------

/**
 * This class acts as a base for client/server net classes. It tries to define
 * as much common used code as possible while not trying to slow things down
 * because of the generalisation
 */
class NetBase
{
public:

    /**
     * Struct used by MessageCracker and ToString to distribute a number of access pointers.
     * Collect all in one struct instead of creating multiple arguments that
     * would be hard to maintain.
     */
    struct AccessPointers
    {
        // msgstrings are loaded by CacheManager and mantained in CacheManager.msg_strings
        // then sent from server to client at login with psMsgStringsMessage
        // and stored in <appdata>/cache/commonstrings client side
        csStringSet* msgstrings;
        csStringHashReversible* msgstringshash;
        iEngine *engine;

        /** Utility function that either request from the msgstring or from the message has
         *  depending on who that is set.
         *
         *  @return NULL if not found, or the string if found.
         */
        const char* Request(csStringID id) const;

        /** Utility function that either request from the msgstring or from the message has
         *  depending on who that is set.
         *
         *  @return csInvalidStringID if not found, or the csStringID for the string if found.
         */
        csStringID Request(const char* string) const;
    };

    
    /**
     * you can specify how much messages the ouptput queue can contain before
     * being full. 100 should be enough for the Client, but the server should
     * increase this number
     */
    NetBase(int outqueuelen=100);
    virtual ~NetBase();

    /**
     * This adds another message queue (for other threads)
     * These queues are for reading off.
     * Selection of the messages is done by a minimum and maximum ObjID
     */
    bool AddMsgQueue (MsgQueue *,objID minID=0,objID maxID=0xffffffff);

    /** this removes a queue */
    void RemoveMsgQueue(MsgQueue *);

    /**
     * Put a message into the outgoing queue
     */
    virtual bool SendMessage (MsgEntry* me);
    virtual bool SendMessage (MsgEntry* me,NetPacketQueueRefCount *queue);

    /**
     * Broadcast a message, DON'T USE this function, it's only for MsgHandler!
     */
    virtual void Broadcast (MsgEntry* me, int scope, int guildID) = 0;

    /**
     * Sends the given message me to all the clients in the list.
     *
     * Sends the given message me to all the clients in the list (clientlist)
     * which is of size count. This will send the message to all the clients
     * except the client which has a client number given in the variable
     * except.
     * @note (Brendon) Why is multi not const & ?
     *
     * @param me     Is the message to be sent to other clients.
     * @param multi  Is a vector of all the clients to send this message to.
     * @param except Is a client number for a client NOT to send this message
     *     to. This would usually be the client trying to send the message.
     * @param range  Is the maximum distance the client must be away to be out
     *     of "message reception range".
     */
    virtual void Multicast (MsgEntry* me, const csArray<PublishDestination>& multi, uint32_t except, float range) = 0;

    /**
     * Is this connection ready to use?
     */
    bool IsReady()  { return ready; }

    /**
     * This adds a completed message to any queues that are signed up.
     */
    bool QueueMessage(MsgEntry *me);

    /**
     * This function is the heart of NetBase - it look for new incoming packets
     * and sends packets in the outgoing queue. This is thought of being called
     * from a thread in the server or regularely (between frames?) in the client
     * It loops until both send and receive are done with all their queues or until
     * the current time has passed timeout, then returns to caller.
     */
    void ProcessNetwork (csTicks timeout);

    /** sendOut sends the next packet in the outgoing message queue */
    bool SendOut(void);

    /** this receives an Incoming Packet and analyses it */
    bool CheckIn(void);

    /**
     * Flush all messages in given queue.
     *
     * @param queue The queue to flush
     * @return True if all messages flushed.
     */
    bool Flush(MsgQueue * queue);

    /** Binds the socket to the specified address (only needed on server */
    bool Bind(const char* addr, int port);
    bool Bind(const IN_ADDR &addr, int port);

    /**
     * This class describes connections to other computers - so it contains
     * TCP/IP address, a buffer for splitted packets, msgnum and others, but not
     * things like name of the client or character type
     */
    class Connection;

    enum broadcasttype
    {
    BC_EVERYONEBUTSELF = 1,
    BC_GROUP = 2,
    BC_GUILD = 3,
    BC_SECTOR = 4,
    BC_EVERYONE = 5,
    BC_FINALPACKET = 6
    };

    csTicks GetPing(void) { return netInfos.GetAveragePingTicks();}

    psNetMsgProfiles * GetProfs() { return profs; }

    /// The timeout to use when waiting for new incoming packets.
    struct timeval timeout;

    /// Set the MsgString Hash
    void SetMsgStrings(csStringSet* msgstrings, csStringHashReversible* msgstringshash)
    {
        accessPointers.msgstrings = msgstrings;
        accessPointers.msgstringshash = msgstringshash;
    }

    /// Set the Engine
    void SetEngine(iEngine* engine) { accessPointers.engine = engine; }

    /// Get the Engine
    iEngine* GetEngine() { return accessPointers.engine; }

    /// Get the access pointers
    static AccessPointers* GetAccessPointers() { return &accessPointers; }
    
    /**
     * Log the message to LOG_MESSAGE.
     *
     * @param dir Should be R for received messages, S for sent messages and I for internal.
     * @param me  Logged message.
     */
    void LogMessages(char dir, MsgEntry* me);

    /**
     * Pars and configure user filter settings.
     *
     * @param arg User command argumets
     */
    csString LogMessageFilter(const char *arg);

    /**
     * Add a new message type to the LogMessage message filter list
     *
     * @param type The type of message to filter when loging
     */
    void AddFilterLogMessage(int type);

    /**
     * Remove a message type from the LogMessage message filter list
     *
     * @param type The type of message to not filter.
     */
    void RemoveFilterLogMessage(int type);

    /**
     * Clear all message type from the LogMessage message filter list
     *
     */
    void LogMessageFilterClear();

    /**
     * Invert the LogMessage filter
     */
    void InvertLogMessageFilter() { logmsgfiltersetting.invert = !logmsgfiltersetting.invert; }

    /**
     * Toggle the global receive LogMessage filter
     */
    void ToggleReceiveMessageFilter() { logmsgfiltersetting.receive = !logmsgfiltersetting.receive; }

    /**
     * Toggle the global send LogMessage filter
     */
    void ToggleSendMessageFilter() { logmsgfiltersetting.send = !logmsgfiltersetting.send; }

    /**
     * Set the filter hex messages flag.
     */
    void SetLogMessageFilterHex(bool filterhex) { logmsgfiltersetting.filterhex = filterhex; }

    /**
     * Dump the current filter settings.
     */
    void LogMessageFilterDump();

    /** return a random ID that can be used for messages */
    uint32_t GetRandomID();

protected:
    /**
     * Check if the given message type should be logged or not.
     * @param type The message type to check for filtering
     * @param dir  The direction 'R' or 'S'
     *
     */
    bool FilterLogMessage(int type,char dir);

    /* the following protected stuff is thought of being used in the inherited
     * network classes
     */

    /** Wrapper to encapsulate the sendto call and provide for retry if the buffer is full.
     *
     *
     */
    int SendTo (LPSOCKADDR_IN addr, const void *data, unsigned int size)
    {
        struct timeval timeout;
        int sentbytes;
        int retries=0;
        fd_set wfds;

        #ifdef DEBUG
            if (!addr || !data)
            Error1("wrong args");
        #endif

        // #define ENDIAN_DEBUG

        #ifdef ENDIAN_DEBUG
            FILE *f = fopen("hexdump.txt","a");
            int i;
            int len;
            char c;
            const char *ptr;
            ptr = (const char *)data;
            len = ((40<size) ? 40 : size);
            fprintf(f,"\n------------\nSending %d bytes:\n",len);
            for ( i = 0 ; i < len ; i++ ) /* Print the hex dump for len chars. */
                fprintf(f,"%02x " ,(*(ptr+i)&0xff));
            for ( i = len ; i < 16 ; i++ ) /* Pad out the dump field to the ASCII */
                fprintf(f, "   " ); /* field. */
            fprintf(f, " " );
            for ( i = 0 ; i < len ; i++ ) /* Now print the ASCII field. */
            {
                 c=0x7f&(*(ptr+i)); /* mask out bit 7 */
                 if (!(isprint(c))) /* If not printable */
                     fprintf(f, "." ); /* print a dot */
                 else {
                     fprintf(f, "%c" ,c);
                 } /* else display it */
            }
            fprintf(f,"\n-----------------\n");
            fclose(f);
        #endif

        // Try and send the data, if we fail we wait for the status to change
        sentbytes=SOCK_SENDTO(mysocket, data, size, 0, (LPSOCKADDR) addr, sizeof (SOCKADDR_IN) );


        /* Call select() to wait until precisely the time of the change, but have a timeout.
         *  It's possible that the buffer will free between the sendto call and the select
         *  leaving the select hanging forever if not for the timeout value.
         */
        while (retries++ < SENDTO_MAX_RETRIES && sentbytes==-1 && (errno==EAGAIN || errno==WSAEWOULDBLOCK))
        {
            printf("In while loop on EAGAIN... retry #%d.\n", retries);

            // Clear the file descriptor set
            FD_ZERO(&wfds);
            // Set the socket's FD in this set
            FD_SET(mysocket,&wfds);

            // Zero out the timeout value to start
            memset(&timeout,0,sizeof(struct timeval));
            timeout.tv_sec=SENDTO_SELECT_TIMEOUT_SEC;
            timeout.tv_usec=SENDTO_SELECT_TIMEOUT_USEC;

            /* Wait for the descriptor to change.  Note that it's possible that the status
             * has already cleared, so we'll try to send again after this anyway.
             */
            SOCK_SELECT(mysocket+1,NULL,&wfds,NULL,&timeout);

            // Try and send again.
            sentbytes=SOCK_SENDTO(mysocket, data, size, 0, (LPSOCKADDR) addr, sizeof (SOCKADDR_IN) );
        }

        if (sentbytes>0)
        {
            totaltransferout += size;
            totalcountout++;
        }
        else
        {
            Error2("NetBase::SendTo() gave up trying to send a packet with errno=%d.",errno);
        }

        return sentbytes;
    }

    /**
     * small inliner for receiving packets... This just
     * encapsulates the lowlevel socket funcs
     */
    int RecvFrom (LPSOCKADDR_IN addr, socklen_t *socklen, void *buf,
        unsigned int maxsize)
    {
        #ifdef DEBUG
        if (!addr || !buf)
            Error1("wrong args");
        #endif
        fd_set set;

        /* Initialize the file descriptor set. */
        FD_ZERO (&set);
        FD_SET (mysocket, &set);
#ifndef CS_PLATFORM_WIN32
        if(pipe_fd[0] > 0)
            FD_SET (pipe_fd[0], &set);
#endif

        // Backup the timeval struct in case select changes it as on Linux
        struct timeval prevTimeout = timeout;

        /* select returns 0 if timeout, 1 if input available, -1 if error. */
        if (SOCK_SELECT(csMax(mysocket, pipe_fd[0]) + 1, &set, NULL, NULL, &timeout) < 1)
        {
            timeout = prevTimeout;
            return 0;
        }

#ifndef CS_PLATFORM_WIN32
        if(FD_ISSET(pipe_fd[0], &set))
        {
            char throwaway[32];
            if(read(pipe_fd[0], throwaway, 32) == -1)
            {
               Error1("Read failed!");
            }
        }
#endif

        timeout = prevTimeout;

        if(!FD_ISSET(mysocket, &set))
            return 0;

        int err = SOCK_RECVFROM (mysocket, buf, maxsize, 0,
            (LPSOCKADDR) addr, socklen);
        if (err>=0)
        {
            totaltransferin += err;
            totalcountin++;
        }
        return err;
    }


    /**
     * some helper functions... the getConnBy functions should be reimplemented
     * in the client/server classes.
     */
    int GetIPByName (LPSOCKADDR_IN addr, const char *name);

    virtual Connection *GetConnByIP (LPSOCKADDR_IN addr) = 0;

    virtual Connection *GetConnByNum (uint32_t clientnum) = 0;

    /**
     * This function is called when we receive packets from an unknown client.
     * This is useful for the server to handle clients that are just connecting
     */
    virtual bool HandleUnknownClient (LPSOCKADDR_IN addr, MsgEntry *data) = 0;

    /**
     * This initialises the socket lib and creates a listening UDP socket, if
     * you're the client you should set port to zero, so a random free port is
     * user
     */
    bool Init(bool autobind = true);

    void Close(bool force = true);

    /**
     * This takes incoming packets and examines them for priority.
     * If pkt is ACK, it finds the awaiting ack pkt and removes it.
     */
    bool HandleAck(psNetPacketEntry* pkt, Connection* connection, LPSOCKADDR_IN addr);

    /**
     * This cycles through set of pkts awaiting ack and resends old ones.
     * This function must be called periodically by an outside agent, such
     * as NetManager.
     */
    void CheckResendPkts(void);

    /**
     * This takes incoming packets and rebuilds psMessages from them. If/when a
     * complete message is reassembled, it calls HandleCompletedMessage().
     */
    bool BuildMessage(psNetPacketEntry* pkt,Connection* &connection,LPSOCKADDR_IN addr);

    /**
     * This checks the list of packets waiting to be assembled into complete messages.
     *  It forms a list of up to 10 (may be changed, check code) packets which are older
     *  than 10 seconds of age.  These packets are removed from the list.
     *  This should usually be called with the same frequency as CheckResendPkts() though
     *  their functionality is not related.
     */
    void CheckFragmentTimeouts(void);

    /**
     * This adds the incoming packet to the pending packets tree, and builds
     * the psMessageBytes struct and MsgEntry struct if complete.
     */
    csPtr<MsgEntry> CheckCompleteMessage(uint32_t client,uint32_t id);

    /**
     * This receives only fully reassembled messages and adds to appropriate
     * queues.
     * If pkt is HIGH priority, it creates relevant ack pkts and queues it to send back.
     */
    void HandleCompletedMessage(MsgEntry *me,
                Connection* &connection,
                LPSOCKADDR_IN addr,
                psNetPacketEntry* pkt);

    /**
     * This tries to drop packets that received doubled
     */
    bool CheckDoublePackets (Connection* connection, psNetPacketEntry* pkt);


    /**
     * This attempts to merge as many packets as possible into one before
     * sending.  It empties the passed queue.
     */
    bool SendMergedPackets(NetPacketQueue *q);

    /**
     * This does the sending and puts the packet in "awaiting ack" if necessary.
     */
    bool SendSinglePacket(psNetPacketEntry* pkt);

    /**
     * Send packet to the clientnum given by clientnum in psNetPacketEntry
     */
    bool SendFinalPacket(psNetPacketEntry* pkt);

    /**
     * This only sends out a packet
     */
    bool SendFinalPacket(psNetPacketEntry* pkt, LPSOCKADDR_IN addr);

    /** Outgoing message queue */
    csRef<NetPacketQueueRefCount> NetworkQueue;

    /** weak referenced list of outbound queues with waiting data so disconnected clients won't receive packets*/
    GenericRefQueue<NetPacketQueueRefCount, csWeakRef > senders;

    /** Incoming message queue vector */
    csArray<MsgQueue*> inqueues;

    /** Packets Awaiting Ack pool */
    csHash<csRef<psNetPacketEntry>, PacketKey> awaitingack;

    /** System Socket lib initialized? */
    static int socklibrefcount;

    /** is the connection ready? */
    bool ready;

    /** total bytes transferred by this object */
    long totaltransferin, totaltransferout;
    /** total packages transferred by this object */
    long totalcountin, totalcountout;

    /** Moving averages */
    typedef struct {
        unsigned int senders;
        unsigned int messages;
        csTicks time;
    } SendQueueStats_t;

    SendQueueStats_t sendStats[NETAVGCOUNT];
    csTicks lastSendReport;
    unsigned int avgIndex;

    size_t resends[RESENDAVGCOUNT];
    unsigned int resendIndex;

    psNetMsgProfiles * profs;

private:
    /** my socket */
    SOCKET mysocket;

    /** a pipe to wake up from the select call when data is ready to be written */
    SOCKET pipe_fd[2];

    /** tree holding the outgoing packets */
    csHash<csRef<psNetPacketEntry> , PacketKey> packets;

    /** for generating random values (unfortunately the msvc rand() function
     * is not good at all
     */
    csRandomGen* randomgen;

    /** mutex used for the random generator */
    CS::Threading::Mutex mutex;

    /** network information layer */
    psNetInfos netInfos;

    /** Access Pointers for MessageCrackers */
    static AccessPointers accessPointers;

    /** LogMessage filter setting */
    csArray<int> logmessagefilter;

    typedef struct {
        bool invert;
        bool filterhex;
        bool receive;
        bool send;
    } LogMsgFilterSetting_t;

    LogMsgFilterSetting_t logmsgfiltersetting;

    char* input_buffer;
};


//-----------------------------------------------------------------------------


/** This class holds data for a connection */
class NetBase::Connection
{
    uint32_t sequence;
public:
    csHash<uint32_t> packethistoryhash;
    /** buffer for split up packets, allocated when needed */
    void *buf;
    /** The INet Adress of the client */
    SOCKADDR_IN addr;
    /** The adress if provided, usually in clients, else an empty csstring */
    csString nameAddr;
    /** The Number of the last incoming packet */
    int pcknumin;
    /** The Number of the last outgoing packet */
    int pcknumout;
    /** Is this already a valid connection? */
    bool valid;
    /** the client num */
    uint32_t clientnum;
    /** last time packet was received from this connection */
    csTicks lastRecvPacketTime;
    /** number of attempts to keep alive connection without ack response */
    int heartbeat;
    /** Estimated RTT */
    float estRTT;
    /** RTT deviance */
    float devRTT;
    /** timeout */
    csTicks RTO;
    /** Number of reliable sends */
    uint32_t sends;
    /** Number of resends */
    uint32_t resends;
    
    // Reliable transmission window size
    uint32_t window;

    /** keeps track of received packets to drop doubled packets */
    uint32_t packethistoryid[MAXPACKETHISTORY];
    uint32_t packethistoryoffset[MAXPACKETHISTORY];
    int historypos;

    Connection(uint32_t num=0);
    ~Connection();
    bool isValid() const
    { return valid; }

    uint32_t GetNextPacketID() {return sequence++;}
    
    /// Check if the reliable transmission window is full
    bool IsWindowFull() {return window > WINDOW_MAX_SIZE; }
    /// Add to window when reliable data is in transit
    void AddToWindow(uint32_t bytes) {window += bytes; }
    /// Remove from transmission window when an ack is received
    void RemoveFromWindow(uint32_t bytes) { if(bytes > window) abort(); window -= bytes;}
};


//-----------------------------------------------------------------------------


class NetPacketQueueRefCount : public NetPacketQueue, public csSyncRefCount, public CS::Utility::WeakReferenced
{
private:
    bool pending;

public:
    NetPacketQueueRefCount(int qlen)
    : NetPacketQueue(qlen)
    { pending=false; }
    virtual ~NetPacketQueueRefCount()
    {}

    /// This flag ensures the same object is not queued twice.
    /// Doesn't have to mutexed here because these are already called from within a queue mutex.
    void SetPending(bool flag)
    {
        pending = flag;
    }
    bool GetPending()
    {
        return pending;
    }
};

/** @} */

#endif

