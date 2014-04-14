/*
 * eventmanager.h
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

#ifndef __EVENTMANAGER_H__
#define __EVENTMANAGER_H__

#include "util/heap.h"
#include "net/msghandler.h"

class psGameEvent;
class MsgHandler;

/**
 * \addtogroup common_util
 * @{ */

/**
 * This class handles all queueing and invoking of timed events, such as
 * combat, spells, NPC dialog responses, range weapons, or NPC respawning.
 * It maintains a queue ordered by trigger time and is polled by the engine
 * periodically to clear any queued events with trigger times less than the
 * current ticks time.
 */
class EventManager : public MsgHandler, public Singleton<EventManager>
{
protected:
    CS::Threading::Mutex mutex;
    Heap<psGameEvent> eventqueue;

    csTicks lastTick;

    /// A flag indicating the server is shutting down.
    bool stop;
    
	/// Helper function to keep a running average of the last 50 events.
	void TrackEventTimes(csTicks timeTaken,MsgEntry *msg);

public:
    EventManager();
    virtual ~EventManager();

    // Thread main loop, handling timed events and inbound messages
    virtual void Run ();

    // Called by external threads to make the Run() loop stop.
    void Stop() { stop = true; }

    /// Add new event to scheduler queue.
    void Push(psGameEvent *event);

    /// Check Event Queue for scheduled events which are due
    csTicks ProcessEventQueue();

    /// Allows sending of a message not immediately, but after a short delay
    virtual void SendMessageDelayed(MsgEntry *msg,csTicks msecDelay);
};

/** @} */

#endif
