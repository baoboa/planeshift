/*
* genqueue.h by Matze Braun <MatzeBraun@gmx.de>
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

#ifndef __GENQUEUE_H__
#define __GENQUEUE_H__

#include <csutil/ref.h>
#include <csutil/threading/mutex.h>
#include <csutil/threading/condition.h>

#include "util/pserror.h"

/**
 * \addtogroup common_util
 * @{ */

/**
* A queue of smart pointers with locking facilties
* for multi-threading. The objects in the queue must
* implement reference counting.
*/
template <class queuetype, template <class T> class refType = csRef >
class GenericRefQueue
{
public:
    GenericRefQueue(unsigned int maxsize = 500)
    {
        /* we make the buffer 1 typ bigger, so we can avoid one check and one
         variable when testing if buffer is full */
        qsize = maxsize;
        qbuffer = new refType<queuetype>[qsize + 1]();
        qstart = qend = 0;
    }

    ~GenericRefQueue()
    {
        delete[] qbuffer;
    }
    
    /** like above, but waits to add the next message, if the queue is full
     *  be careful with this. It's easy to deadlock! */
    bool AddWait(queuetype* msg, csTicks timeout = 0)
    {
        // is there's a space in the queue left just add it
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        while(true)
        {
            bool added = Add(msg);
            if (added)
            {
                return true;
            }
            Error1("Queue full! Waiting.\n");
            
            // Wait release mutex before waiting so that it is possible to
            // add new messages.
            if (!datacondition.Wait(mutex, timeout))
            {
                // Timed out waiting for new message
                return false;
            }
        }
    }
        
    /** This adds a message to the queue and waits if it is full */
    bool Add(queuetype* msg)
    {
        unsigned int tqend;

        CS::Threading::RecursiveMutexScopedLock lock(mutex);

        if (!msg->GetPending())
        {
            tqend = (qend + 1) % qsize;
            // check if queue is full
            if (tqend == qstart)
            {
                Interrupt();
                return false;
            }
            // check are we having a refcount race (in which msg would already be destroyed)
			CS_ASSERT(msg->GetRefCount() > 0);
            // add Message to queue
            qbuffer[qend]=msg;
            qend=tqend;

            msg->SetPending(true);

            Interrupt();
        }
        return true;
    }
    
    // Peeks at the next message from the queue but does not remove it.
    csPtr<queuetype> Peek()
	{
        CS::Threading::RecursiveMutexScopedLock lock(mutex);

        csRef<queuetype> ptr;
        
        unsigned int qpointer = qstart;
        
        // if this is a weakref queue we should skip over null entries
        while(!ptr.IsValid())
        {
            // check if queue is empty
            if (qpointer == qend)
            {
                return 0;
            }

            // removes Message from queue
            ptr = qbuffer[qpointer];
            
            qpointer = (qpointer + 1) % qsize;
        }

        return csPtr<queuetype>(ptr);
	}

    /**
    * This gets the next message from the queue, it is then removed from
    * the queue. Note: It returns a pointer to the message, so a null
    * pointer indicates an error
    */
    csPtr<queuetype> Get()
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);

        csRef<queuetype> ptr;
        
        // if this is a weakref queue we should skip over null entries
        while(!ptr.IsValid())
        {
            // check if queue is empty
            if (qstart == qend)
            {
                Interrupt();
                return 0;
            }

            // removes Message from queue
            ptr = qbuffer[qstart];
            qbuffer[qstart] = 0;
            
            qstart = (qstart + 1) % qsize;
        }

        ptr->SetPending(false);
        Interrupt();

        return csPtr<queuetype>(ptr);
    }

    /** like above, but waits for the next message, if the queue is empty */
    csPtr<queuetype> GetWait(csTicks timeout)
    {
        // is there's a message in the queue left just return it
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        while(true)
        {
            csRef<queuetype> temp = Get();
            if (temp)
            {
                return csPtr<queuetype> (temp);
            }

            // Wait release mutex before waiting so that it is possible to
            // add new messages.
            if (!datacondition.Wait(mutex, timeout))
            {
                // Timed out waiting for new message
                return 0;
            }
        }
        CS_ASSERT(false);
        return 0;
    }

    /**
    * This function interrupt the queue if it is waiting.
    */
    void Interrupt()
    {
        datacondition.NotifyOne();
    }

    /**
     * Number of items in the queue.
     */
    unsigned int Count()
    {
        CS::Threading::RecursiveMutexScopedLock lock(mutex);
        if(qend < qstart)
            return qend + qsize - qstart;
        else
            return qend - qstart;
    }
    
    bool IsFull()
    {
    	CS::Threading::RecursiveMutexScopedLock lock(mutex);
    	return ((qend + 1) % qsize == qstart);
    }
protected:

    refType<queuetype>* qbuffer;
    unsigned int qstart, qend, qsize;
    CS::Threading::RecursiveMutex mutex;
    CS::Threading::Condition datacondition;
};

/** @} */

#endif
