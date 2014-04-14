/*
 * deathcallback.h - author Michael Cummings <cummings.michael@gmail.com>
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

#ifndef __DEATHCALLBACK_H__
#define __DEATHCALLBACK_H__

class iDeathCallback;


class iDeathNotificationObject
{
public:
    virtual void RegisterCallback(iDeathCallback* receiver) = 0;
    virtual void UnregisterCallback(iDeathCallback* receiver) = 0;
    virtual ~iDeathNotificationObject() {}
};


/**
* This class generically allows objects to be notified
* when a gemActor dies.  psGEMEvent uses this heavily
* to make sure that timed events for an actor are not
* run when obsolete, but other classes may use this
* too as appropriate.
*/
class iDeathCallback
{
public:
    virtual void DeathCallback(iDeathNotificationObject* object)=0;
    virtual ~iDeathCallback() {}
};



#endif
