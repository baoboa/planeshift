/*
 * serverpinger.h - Author: Ondrej Hurt
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

#ifndef SERVER_PINGER_H
#define SERVER_PINGER_H

#include <net/connection.h>

/**
 * \addtogroup common_net
 * @{ */

/// Class psServerPinger takes care about sending pings to a server in Login screen.
class psServerPinger
{
public:

    enum SERVERSTATUS { INIT,
                        FAILED,
                        READY, 
                        WAIT,
                        FULL,
                        LOCKED};
    
    psServerPinger(const csString & serverName, const csString& description, const csString & address, int port, iObjectRegistry * objReg);
    ~psServerPinger();

    /** Initialize the connection.
     * @return success of the initialization */
    bool Initialize();

    /** Opens connection to server.
     * @return success of the connection */
    bool Connect();

    /** Sends server disconnection message */
    void Disconnect();

    /** Call this periodically */
    void DoYourWork();

    /** Returns currently measured ping to server is miliseconds. Returns -1 if ping timed out */
    int GetPing() { return ping; }

    /** Returns last received flags from server */
    int GetFlags() { return flags; }

    SERVERSTATUS GetStatus();
    
    float GetLoss() {
        if(lost == sent - 1 )
            return 1;
        else if (lost == 0)
            return 0;
        else
            return (float)lost/sent;
    }

    csString GetName()       { return name;        }
    csString GetDescription(){ return description; }
    csString GetAddress()    { return address;     }
    int      GetPort()       { return port;        }

protected:
    csString name;      ///< server info
    csString description;
    csString address;
    int port;

    psNetConnection * connection;
    MsgQueue * queue;

    int ping;           ///< last measured ping (-1 means timeout, 9999 means wait)
    int lastPingTime;   ///< the time when we sent our last ping to server
    unsigned int sent;  ///< the number of ping messages sent
    unsigned int lost;  ///< the number of ping messages lost
    bool waiting;       ///< are we waiting for ping response from server ?
    unsigned int pingID;///< unique identifier of ping message - enables us to ignore ping responses that come after timeout
    unsigned int flags; ///< Last flags returned from server. Should only be used if ping != -1 && ping != 9999
    iObjectRegistry * objReg;
};

/** @} */

#endif
