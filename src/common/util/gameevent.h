/*
 * gameevent.h
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

#ifndef __GAMEEVENT_H__
#define __GAMEEVENT_H__

#include <csutil/csstring.h>

class EventManager;

/**
 * \addtogroup common_util
 * @{ */

/**
 * All scheduled events must inherit from this class.  These events are
 * queued by the EventManager and are passed to the various subscribed
 * handlers at the appropriate time.
 */
class psGameEvent
{
    char type[32];          ///< The type of this GameEvent, used for debugging
public:
    static EventManager* eventmanager;

    csTicks triggerticks;   ///< ticks value when event should be triggered.
    csTicks delayticks;     ///< delay before the event starts
    int id;                 ///< id value combined with ticks ensures uniqueness for tree
    static int nextid;      ///< id counter sequence
    bool valid;             ///< Set this to false if the trigger should not be fired.

    /**
     * Construct a new game event. Set ticks to 0 if the event should
     * be fired at offset ticks from current time.
     */
    psGameEvent(csTicks ticks,int offsetticks, const char* newType);

    virtual ~psGameEvent();

    /**
     * Publish the game event to the local program.
     */
    void QueueEvent();

    /**
     * Called right before a Trigger is called.
     *
     * Default implementation use the valid flag to determin if the Trigger
     * should be calle.Could be overridden to allow for other conditions.
     *
     * This function allows psGEMEvents to disconnect themselves
     */
    virtual bool CheckTrigger()
    {
        return valid;
    };

    /**
     * Abstract event processing function
     *
     * This functino have to be overridden and will be called if
     * CheckTrigger is ok at the time given in the constructor.
     */
    virtual void Trigger()=0;

    /**
     * Set the valid flag.
     *
     * Setting of valid to false will if the CheckTrigger isn't overridden
     * cause the Trigger not to be called.
     *
     * @param valid The new value of the valid flag.
     */
    virtual void SetValid(bool valid)
    {
        this->valid = valid;
    }

    /**
     * Return the valid flag.
     *
     * @return The valid flag.
     */
    virtual bool IsValid()
    {
        return valid;
    }

    /**
     * Return the type that this event where created with.
     * Used for debugging.
     */
    const char* GetType()
    {
        return type;
    }

    /**
     * Return a string with information about the event.
     * Used for debugging to be able to identify things like
     * way did this event take so long to execute.
     *
     * TODO: Make this function abstract. But for now return "".
     */
    //TODO:    virtual csString ToString() const = 0;
    virtual csString ToString() const
    {
        return "Not implemented";
    }

    bool operator==(const psGameEvent &other) const
    {
        return (triggerticks==other.triggerticks &&
                id==other.id);
    };

    bool operator<(const psGameEvent &other) const
    {
        if(triggerticks<other.triggerticks)
            return true;
        if(triggerticks>other.triggerticks)
            return false;
        if(id<other.id)
            return true;
        return false;
    };
    bool operator>(const psGameEvent &other) const
    {
        if(triggerticks>other.triggerticks)
            return true;
        if(triggerticks<other.triggerticks)
            return false;
        if(id>other.id)
            return true;
        return false;
    };
};

/** @} */

#endif

