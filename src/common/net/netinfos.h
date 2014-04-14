/*
 * netinfos.h by Marc Haisenk <axl@cu-muc.de>
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
 * This serves as a layer for getting informations about the network
 */
#ifndef __NETINFOS_H__
#define __NETINFOS_H__

#include <cstypes.h>

/**
 * \addtogroup common_net
 * @{ */

/// Number of entries in the ticks queue for calculating the average
#define NETINFOS_TICKARRAYSIZE 10



/// Gives informations about the network connection
class psNetInfos
{
public:
    /// Constructor
    psNetInfos() { SetupTickArray(); }
    /// Destructor
    ~psNetInfos();
    
    /// Add a tick value to the global queue
    void    AddPingTicks(csTicks t);
    /// Compute the average ticks a ping uses
    csTicks GetAveragePingTicks();

    int     droppedPackets;

private:
    /// Fill the tick array
    void    SetupTickArray();
    /// holds the tick values needed to compute the average ping ticks
    csTicks tickArray[NETINFOS_TICKARRAYSIZE];
    /// current location in the array
    int     tickArrayLoc;
};

/** @} */

#endif
