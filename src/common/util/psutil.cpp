/*
 * Author: Anders Reggestad
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <csutil/sysfunc.h>
#include <csutil/randomgen.h>

#include "psutil.h"
#include "util/consoleout.h"

void GetTimeOfDay(csString& string)
{
    time_t curr=time(0);
    tm* gmtm = gmtime(&curr);
    
    string.Format("%d-%02d-%02d %02d:%02d:%02d",
                        gmtm->tm_year+1900,
                        gmtm->tm_mon+1,
                        gmtm->tm_mday,
                        gmtm->tm_hour,
                        gmtm->tm_min,
                        gmtm->tm_sec);
}                        


ScopedTimer::ScopedTimer(csTicks limit, const char * format, ... )
    :start(csGetTicks()),timeUsed(0),limit(limit),callback(NULL)
{ 
    char str[1024];
    va_list args;

    va_start(args, format);
    vsnprintf(str,1023, format, args);
    str[1023]=0x00;
    va_end(args);

    comment = str;
}

ScopedTimer::ScopedTimer(csTicks limit, ScopedTimerCB* callback )
    :start(csGetTicks()),limit(limit),callback(callback)
{
}


ScopedTimer::~ScopedTimer()
{
    timeUsed = csGetTicks() - start;
    if (timeUsed > limit)
    {
        if (callback)
        {
            callback->ScopedTimerCallback(this);
        }
        else
        {
            CPrintf(CON_DEBUG,"Took %d time to process %s\n",timeUsed,comment.GetDataSafe());
        }
    }
}

csTicks ScopedTimer::TimeUsed() const
{
    return timeUsed;
}


csRandomGen psrandomGen;

float psGetRandom()
{ 
    return psrandomGen.Get();
}

uint32 psGetRandom(uint32 limit)
{
    return psrandomGen.Get(limit);
}

