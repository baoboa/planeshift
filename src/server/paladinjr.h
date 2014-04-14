/*
 * paladinjr.h - Author: Andrew Dai
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PALADIN_H__
#define __PALADIN_H__
//#define PALADIN_DEBUG

#include <iutil/cfgmgr.h>

#ifdef PALADIN_DEBUG
#define PALADIN_MAX_SWITCH_TIME 0
#else
#define PALADIN_MAX_SWITCH_TIME 5000
#endif

#define PALADIN_VERSION "0.13"

class PaladinJr
{
public:

    /// Extrapolate the current position from last DR packet
    bool ValidateMovement(Client* client, gemActor* actor, psDRMessage &drmsg);

    /** Compare extrapolated displacement with new displacement from new DR packet
     * Should be called after actor has been updated with newest recieved DR packet
     */
    bool CheckCollDetection(Client* client, gemActor* actor);

    void Initialize(EntityManager* celbase, CacheManager* cachemanager);

    ~PaladinJr()
    {  }

    bool IsEnabled()
    {
        return enabled;
    }

private:

    enum // possible cheat checks as bitmask
    {
        NOVIOLATION    = 0x0,
        WARPVIOLATION  = 0x1,
        SPEEDVIOLATION = 0x2,
        DISTVIOLATION  = 0x4,
        CDVIOLATION    = 0x8
    };

    bool enabled;
    bool enforcing;
    int checks; ///< checks to perform
    unsigned int watchTime;
    unsigned int warnCount; ///< warn each x detects
    unsigned int maxCount; ///< kick after x detects

    bool SpeedCheck(Client* client, gemActor* actor, psDRMessage &currUpdate);


    EntityManager*         entitymanager;

    /// Already checked list of clientnums
    csSet<uint32_t> checked;

    /// Currently checking client
    Client* target;

    /// Time at which we started to check target client
    csTicks started;

    /// Predicted, extrapolated, position based on last recieved DR packet
    csVector3 predictedPos;

    /// Position before extrapolation
    csVector3 origPos;
    csVector3 vel;
    csVector3 angVel;

    /// Maximum displacement vector
    csVector3 maxmove;

    /// Check this client?
    bool checkClient;

    /// Data from last DR update stored here
    psDRMessage lastUpdate;

    /// Maximum absolute velocities
    csVector3 maxVelocity;

    /// Maximum speed
    float maxSpeed;
};


#endif
