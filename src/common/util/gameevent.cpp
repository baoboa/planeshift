/*
 * gameevent.cpp
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

#include <csutil/sysfunc.h>  // csTicks def
#include <csutil/threading/atomicops.h>

#include "util/gameevent.h"
#include "util/eventmanager.h"
#include "util/consoleout.h"

//psGameEvent static variables

EventManager* psGameEvent::eventmanager = NULL; ///< eventmanager to be used for FireEvents
int psGameEvent::nextid;                        ///< id counter sequence number

psGameEvent::psGameEvent(csTicks ticks,int offsetticks, const char* newType)
{
    if(ticks)
    {
        triggerticks = ticks + offsetticks;
    }
    else
    {
        triggerticks = csGetTicks() + offsetticks;
    }

    delayticks = offsetticks;
    strncpy(type,newType, 31);
    type[31] = '\0';
    id =  CS::Threading::AtomicOperations::Increment(&nextid);
    valid = true;
}

psGameEvent::~psGameEvent()
{
}

void psGameEvent::QueueEvent()
{
    eventmanager->Push(this);
}
