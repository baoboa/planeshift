/*
 * netinfos.cpp by Marc Haisenk <axl@cu-muc.de>
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
#ifndef __NETINFOS_CPP__
#define __NETINFOS_CPP__

#include <psconfig.h>

#include "netinfos.h"

psNetInfos::~psNetInfos()
{
}

void psNetInfos::SetupTickArray()
{
    for (int i=0; i < NETINFOS_TICKARRAYSIZE; i++)
    tickArray[i] = 0;
    
    tickArrayLoc = 0;
}

void psNetInfos::AddPingTicks(csTicks t)
{
    // implemention of a "ring array"
    tickArrayLoc = (tickArrayLoc + 1) % NETINFOS_TICKARRAYSIZE;
    tickArray[tickArrayLoc] = t;
}

csTicks psNetInfos::GetAveragePingTicks()
{
    
    csTicks sum  = 0;
    csTicks tmp  = 0;
    int     num  = 0;

    // iterate through the array and add all values that aren't 0
    for (int i=0; i < NETINFOS_TICKARRAYSIZE; i++)
    {
    tmp=tickArray[i];
    if (tmp) 
    {
        num++;
        sum+=tmp;
    }
    }

    // if all values are 0 then avoid dividing by 0 ;-)    
    if (!num) return 0;
    // else return the sum of values divided by the number of values
    return sum/num;
}

#endif

