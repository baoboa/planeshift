/*
 * serverstatus.h by Matze Braun <matze@braunis.de>
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
#ifndef __SERVERSTATUS_H__
#define __SERVERSTATUS_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/vfs.h>
#include <csutil/threading/thread.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================



#ifdef USE_WINSOCK
#   include "net/sockwin.h"
#endif
#ifdef USE_UNISOCK
#   include "net/sockuni.h"
#endif

struct iObjectRegistry;
class psServer;

/** This class generates logs at a particular interval that has information that
 *  can be displayed later on a website.
 *
 *  At the moment it overwrites logs but can be easily changed to either create a
 *  unique log file for each report ( such as daily ).
 *
 * Log Format:
 *  &lt;server_report time="Time stamp for report" number="number of this report" &gt;
 *  &lt;player name="player name"
 *             guild="guild name"
 *             title="guild rank"
 *             security="security level"
 *             secret="yes|no" /&gt;
 *  &lt;server_report&gt;
 */

class ServerStatus
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

    static unsigned int mob_deathcount;
    static unsigned int mob_birthcount;
    static unsigned int player_deathcount;

    static unsigned int sold_items;
    static unsigned int sold_value;
};

#endif

