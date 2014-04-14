/*
 * genericevent.h
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef _PS_GENERIC_EVENT_HANDLER_
#define _PS_GENERIC_EVENT_HANDLER_

#include <csutil/scf_implementation.h>
#include <iutil/eventh.h>

/**
 * \addtogroup common_util
 * @{ */

/** Declares a generic event handler for a class.
 *  Use this within the body of the parent class you wish to receive events.
 *
 * @param handlerName The name to give to this new class declaration
 * @param parentType The class name of the parent class for this
 * @param eventName The event name to use for this handler
 */
#define DeclareGenericEventHandler(handlerName,parentType,eventName)                \
class handlerName : public scfImplementation1<handlerName,iEventHandler>            \
{                                                                                   \
private:                                                                            \
    parentType* parent;                                                             \
                                                                                    \
public:                                                                             \
    handlerName(parentType* p)                                                      \
        : scfImplementationType(this), parent(p) {}                                 \
                                                                                    \
    virtual ~handlerName() {}                                                       \
                                                                                    \
    virtual bool HandleEvent(iEvent& event)                                         \
    {                                                                               \
        return parent->HandleEvent(event);                                          \
    }                                                                               \
                                                                                    \
    CS_EVENTHANDLER_NAMES(eventName);                                               \
    CS_EVENTHANDLER_NIL_CONSTRAINTS;                                                \
}

/** @} */

#endif

