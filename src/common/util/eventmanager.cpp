/*
* eventmanager.cpp
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

#include "gameevent.h"
#include "util/consoleout.h"

#include "net/messages.h"
#include "eventmanager.h"

// Number of recent events to use when calculating moving average
#define EVENT_AVERAGETIME_COUNT 50

/*---------------------------------------------------------------------------*/

EventManager::EventManager()
{
    // Setting up the static pointer in psGameEvent. Used so
    // that an event can be fired without needing to look up 
    // the event manager first.
    lastTick = 0;
    stop = false;
    psGameEvent::eventmanager = this;
}

EventManager::~EventManager()
{
    // Clean up the event queue
    while (eventqueue.Length())
    {
        delete eventqueue.DeleteMin();
    }
}

void EventManager::Push(psGameEvent *event)
{
    CS::Threading::MutexScopedLock lock(mutex);

    // This inserts the event into the priority queue, sorted by timestamp
    eventqueue.Insert(event);

    /*check if events are inserted late*/
    if (event->triggerticks < lastTick)
    {
        // yelp and adjust lastTick to avoid wrong warning
        CPrintf(
                CON_DEBUG,
                "Event %d scheduled at %d is being inserted late. Last processed event was scheduled for %d.\n",
                event->id, event->triggerticks, lastTick);
        lastTick = event->triggerticks;
    }
}

// Process events at least every 250 tick
#define PROCESS_EVENT   250

csTicks EventManager::ProcessEventQueue()
{
    csTicks now = csGetTicks();

    static int lastid;

    psGameEvent *event = NULL;
    int events = 0;
    int count = 0;

    while (true)
    {

        {
            CS::Threading::MutexScopedLock lock(mutex);

            event = eventqueue.FindMin();

            if (!event || event->triggerticks > now)
            {
                // Empty event queue or not time for event yet
                break;
            }
            eventqueue.DeleteMin();

            /*check if events arrive in order*/
            if (event->triggerticks < lastTick)
            {
                /* this should not happen at all */
                CPrintf(
                        CON_DEBUG,
                        "Event %s:%s (%d) scheduled at %d is being processed out of order at time %d! Last processed event was scheduled for %d.\n",
                        event->GetType(), 
                        event->ToString().GetDataSafe(),
                        event->id, event->triggerticks, now, lastTick);
                CS_ASSERT_MSG("Event trigger time is inconsistent", false);
            }
            else
            {
                lastTick = event->triggerticks;
            }

        }
        

        events++;
        csTicks start = csGetTicks();

        if (event->CheckTrigger())
        {
            event->Trigger();
        }

        csTicks timeTaken = csGetTicks() - start;

        if(timeTaken > 1000)
        {
            csString status;
            status.Format("Event type %s:%s has taken %u time to process\n", event->GetType(), 
                          event->ToString().GetDataSafe(),timeTaken);
            CPrintf(CON_WARNING, "%s\n", status.GetData());
            if(LogCSV::GetSingletonPtr())
                LogCSV::GetSingleton().Write(CSV_STATUS, status);
        }

        if (lastid == event->id)
        {
            CPrintf(CON_DEBUG, "Event %d is being processed more than once at time %d!\n",event->id,event->triggerticks);
        }
        lastid    = event->id;

        count++;
//        if (count % 100 == 0)
//        {
//            CPrintf(CON_DEBUG, "Went through event loop 100 times in one timeslice for %d events.  This means we either have duplicate events, "
//                "bugs in event generation or bugs in deleting events from the event tree. "
//                "Last event: %s:%s took %u time\n", events, event->GetType(), event->ToString().GetDataSafe(), timeTaken);
//        }

        delete event;
    }

    if (event)
    {
        // We have a event so report when we would like to be
        // called again
        return csMin(event->triggerticks, PROCESS_EVENT + now);
    }

    return PROCESS_EVENT; // Process events at least every PROCESS_EVENT ticks
}

void EventManager::TrackEventTimes(csTicks timeTaken,MsgEntry *msg)
{
	static bool filled = false;
	static csTicks eventtimes[EVENT_AVERAGETIME_COUNT];
	static short index = 0;
	static csTicks eventtimesTotal = 0;
	
	csString status;

	if(!filled)
		eventtimesTotal += timeTaken;
	else
		eventtimesTotal = timeTaken + eventtimesTotal - eventtimes[index];

	eventtimes[index] = timeTaken;

	// Done this way to prevent a division operator
	if(filled && timeTaken > 500 && (timeTaken * EVENT_AVERAGETIME_COUNT > 2 * eventtimesTotal || eventtimesTotal > EVENT_AVERAGETIME_COUNT * 1000))
	{
		status.Format("Message type %s has taken %u time to process, average time of events is %u", (const char *) GetMsgTypeName(msg->GetType()), timeTaken, eventtimesTotal / EVENT_AVERAGETIME_COUNT);
		CPrintf(CON_WARNING, "%s\n", status.GetData());
		if(LogCSV::GetSingletonPtr())
			LogCSV::GetSingleton().Write(CSV_STATUS, status);
	}

	index++;

	// Rollover
	if(index == EVENT_AVERAGETIME_COUNT)
	{
		index = 0;
		filled = true;
	}
}

// This is the MAIN GAME thread. Every message and event are handled from
// this thread. This eliminate need for synchronization of access to
// game data.
void EventManager::Run ()
{
    csRef<MsgEntry> msg = 0;

    csTicks nextEvent = csGetTicks() + PROCESS_EVENT;

    while (!stop)
    {
        csTicks now = csGetTicks();
        int timeout = nextEvent - now;

        if (timeout > 0)
        {
            msg = queue->GetWait(timeout);
        }

        if (msg)
        {
            csTicks start = csGetTicks();

            Publish(msg);

            csTicks timeTaken = csGetTicks() - start;

            // Ignore messages that take no time to process.
            if(timeTaken)
            {
				TrackEventTimes(timeTaken,msg);
            }

            // don't forget to release the packet
            msg = NULL;
        }
        else if (now >= nextEvent)
        {
            nextEvent = ProcessEventQueue();
        }
    }
    CPrintf(CON_CMDOUTPUT, "Event thread stopped!\n");
}


class psDelayedMessageEvent : public psGameEvent
{
public:
    psDelayedMessageEvent(csTicks msecDelay, MsgEntry *me, EventManager *myparent)
        : psGameEvent(0,msecDelay,"psDelayedMessageEvent")
    {
        myMsg = me;
        myParent = myparent;
    }
    virtual void Trigger()
    {
        myParent->SendMessage(myMsg);
    }
    virtual csString ToString() const
    {
        csString str;
        str.Format("Delayed message of type : %d", myMsg->GetType());
        return str;
    }
    

private:
    csRef<MsgEntry> myMsg;
    EventManager *myParent;
};


void EventManager::SendMessageDelayed(MsgEntry *me,csTicks msecDelay)
{
    psDelayedMessageEvent *msg = new psDelayedMessageEvent(msecDelay,me,this);
    Push(msg);
}

