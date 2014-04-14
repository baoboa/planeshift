/*
* status.h
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef NPC_STATUS_HEADER
#define NCP_STATUS_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/vfs.h>
#include <csutil/threading/thread.h>
#include <csutil/csstring.h>

/**
 * \addtogroup npcclient
 * @{ */

struct iObjectRegistry;

/** This class is used to record the status of the npcclient to display it on a website so
    people can see the status of it.
*/
class NPCStatus
{
public:
    /** Reads config files, starts periodical status generator */
    static bool Initialize(iObjectRegistry* objreg);

    /** Has the generator run in a while */
    static void ScheduleNextRun();

    /// Interval in milliseconds to generate a report file.
    static csTicks reportRate;

    /// File that it should log to.
    static csString reportFile;

    static unsigned int count;
};

/** @} */

#endif
