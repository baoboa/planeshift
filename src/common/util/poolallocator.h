/*
 * poolallocator.h
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
 *
 * A template class used to allocate fixed size objects in pools.
 * 
 */

#ifndef PS_POOLALLOCATOR_H
#define PS_POOLALLOCATOR_H

// Define this to enable testing to be sure a Release()'d object came from the pool to begin with (debug mode only)
//#define POOLALLOC_DEBUG_DELETE

#include <csutil/threading/thread.h>

/**
 * \addtogroup common_util
 * @{ */

/*   Design notes:
 *
 *  This template is designed to be integrated into a class definition.
 *  Override the new and delete operators for the class to call CallFromNew() and
 *  CallFromDelete() respectively.
 *  Add a private static member variable to the class declaration like so:
 *  static PoolAllocator<myclass> mypool;
 *
 *  Add the definition (usually in the .cpp file):
 *  PoolAllocator<myclass> myclass::mypool(number of objects to allocate per "step");
 *
 *  Use new and delete as normal throughout the program.
 *
 *  DO NOT DERIVE FROM A CLASS USING A POOL ALLOCATOR UNLESS YOU DO THE FOLLOWING:
 *  1) Create a new static PoolAllocator<> member for the derived class.
 *  2) Implement the new and delete operators as above using the new PoolAllocator<> member.
 *
 *  Benefits:
 *  Pool allocation using this method is very fast.  It usually involves a test, an assignment
 *  and a return. No mutex locking is needed since this is not an attempt at being thread safe.
 *
 *  Limitations:
 *  1) Each pool operates on fixed-size objects.  These have to be at least the size of struct
 *     st_freelist_member (currently a single pointer. Which is 4 bytes on 32 bit platforms.)
 *     or the size of each object is bumped up to this size (wasting memory).
 *  2) Memory is never returned to the OS until the entire pool is destructed.  In the suggested
 *     implementation this is at program exit.
 *  3) Allocation time is not constant (it isn't on the heap either). When the pool empties more
 *     space is allocated from the heap and the pool is filled.  The larger the allocation step
 *     size (specified as a parameter to the constructor) the longer this process takes but the
 *     process will have to be done less often.
 *  4) THIS IS NOT THREADSAFE!!  To make this threadsafe the ideal solution would be atomic
 *     operations (there is a nice paper on using atomic ops to safely handle linked lists
 *     out there.)  An alternative is to mutex most calls - which further reduces the benefit
 *     over heap allocation.
 *
 *  - A pool size of 3 should be about as efficient as normal heap allocation functions
 *  
 */

#define POOLALLOC_DEFAULT_ALLOCATION_OBJECTS    1024   



template <class TEMPLATE_CLASS>
class PoolAllocator
{
    CS::Threading::Mutex mutex;

    /// A structure used as a header for each allocation "pool"
    struct st_pool_entry
    {
        struct st_pool_entry *next;
    };

    /** A structure used to track free blocks.
     *
     *  This structure uses memory inside the block itself, so the block must be
     *  at least as large as this structure.
     */
    struct st_freelist_member
    {
        struct st_freelist_member *next;
    };


    /// Pointer to the first struct st_freelist_member in the freelist.  The 'head'.
    struct st_freelist_member *freelist_head;
    /// Pointer to the first struct st_pool_entry in the list of all pools allocated.
    struct st_pool_entry *poollist_head;
    /** Contains the number of object-size blocks to allocate in each heap allocation.
     *
     *  Can also be used (along with structure sizes) to calculate the size of pools.
     *  Set by a parameter passed to the constructor.
     */
    uint32 allocation_step_itemcount;

public:
    /// PoolAllocator constructor.  Sets list heads to NULL and the per-pool object count to the passed value (or default).
    PoolAllocator(int items_per_pool=POOLALLOC_DEFAULT_ALLOCATION_OBJECTS)
    { 
        allocation_step_itemcount = items_per_pool;
        freelist_head = NULL;
        poollist_head = NULL;
    }

    /// PoolAllocator destructor.  Releases the memory assigned to the pools back to the heap and clears the pool and free lists.
    ~PoolAllocator()
    {
        struct st_pool_entry *pool=poollist_head,*nextpool;

        // Step through each pool, freeing the memory associated
        while (pool)
        {
            nextpool=pool->next;
            free(pool);
            pool=nextpool;
        }        
        poollist_head=NULL;
        freelist_head=NULL;
    }


    /// Returns a new block from the list of available blocks.  Allocates another pool if necessary.
    TEMPLATE_CLASS *CallFromNew()
    {
        CS_ASSERT_MSG("Contention detected in PoolAllocator new", mutex.TryLock());
        TEMPLATE_CLASS *objptr;

        // Allocate another pool if no free blocks are available
        if (!freelist_head)
            AllocNewPool();

        // Test for new pool allocation failure
        if (!freelist_head)
        {
            mutex.Unlock();
            return (TEMPLATE_CLASS *)NULL;
        }

        // Bump a block off the free list and return it
        objptr=(TEMPLATE_CLASS *)freelist_head;
        freelist_head=freelist_head->next;

        mutex.Unlock();
        return objptr;
    }

    /// Places a block back on the list of available blocks. Does NOT release memory back to the heap - ever.
    void CallFromDelete(TEMPLATE_CLASS *obj)
    {
     CS_ASSERT_MSG("Contention detected in PoolAllocator delete", mutex.TryLock());

        // NULL pointers are OK - just like delete.
        if (!obj)
        {
            mutex.Unlock();
            return;
        }
#ifdef POOLALLOC_DEBUG_DELETE
        // Throw an assert if this block didn't come from this PoolAllocator
        CS_ASSERT(ObjectCameFromPool(obj));
#endif
        // Clear memory to be on the safe side
        memset(obj, 0xDD, sizeof(TEMPLATE_CLASS));

        // Add this entry to the head of the free list
        ((struct st_freelist_member *)obj)->next=freelist_head;
        freelist_head=((struct st_freelist_member *)obj);
        mutex.Unlock();
    }

    bool PointerCameFromPool(void *ptr)
    {
        return (ObjectCameFromPool((TEMPLATE_CLASS *)ptr)!=NULL);
    }


private:
    /// Internal function.  Checks if a given block came from one of the pools associated with this PoolAllocator
    struct st_pool_entry *ObjectCameFromPool(TEMPLATE_CLASS *obj)
    {
        int entrysize;
        struct st_pool_entry *testentry=poollist_head;

        // Determine the size of blocks
        entrysize=sizeof(TEMPLATE_CLASS);
        if (entrysize<sizeof(struct st_freelist_member))
            entrysize=sizeof(struct st_freelist_member);

        // Walk through each pool
        while (testentry)
        {
            /*  Each pool consists of:
             *  1 struct st_pool_entry  (4 bytes)
             *  (allocation_step_itemcount) blocks of (entrysize) bytes
             *
             *  We know that if an object came from the pool it must satisfy the following tests:
             *  1) Its pointer falls after the pool pointer and prior to the pool pointer + the pool size
             *  2) Its offset from the pool pointer + header must be divisible by the block size
             */
            if ((uint8 *)obj>(uint8 *)testentry &&
                (uint8 *)obj<(uint8 *)testentry + sizeof(st_pool_entry)+entrysize*allocation_step_itemcount)
            {
                if ((unsigned long)((uint8 *)obj-((uint8 *)testentry + sizeof(st_pool_entry))) % (unsigned long)entrysize)
                {
                    // This pointer is within a pool, but does not fall on an object boundary!
                    return NULL;
                }
                return testentry;
            }
            testentry=testentry->next;
        }
        return NULL;
    }


    /// Internal function.  Allocates and prepares another pool of blocks for use.
    void AllocNewPool()
    {
        // Different allocation methods depending on the size of the class
        if (sizeof(TEMPLATE_CLASS)<sizeof(struct st_freelist_member))
        {
            // Using the sizeof struct st_freelist_member as the size of blocks
            uint8 *buffer;
            struct st_freelist_member *workmember;
            struct st_pool_entry *pool;
            unsigned int i;

            // Allocate an appropriately sized block from the heap.  Include space for the pool header.
            buffer=(uint8 *)calloc(1,sizeof(st_pool_entry)+sizeof(st_freelist_member)*allocation_step_itemcount);

            if (!buffer)
                return;

            // Set the pointer to the pool header
            pool=(st_pool_entry *)buffer;

            // Set the pointer to the first entry
            workmember=(st_freelist_member *)(buffer+sizeof(st_pool_entry));
            for (i=0;i<allocation_step_itemcount-1;i++)
            {
                // Set each entry (except the last) to point to the next extry
                workmember->next=workmember+1;
                workmember++;
            }

            // Last entry should point to the head of the freelist
            workmember->next=freelist_head;
            // New freelist head should point to the first member
            freelist_head=(st_freelist_member *)(buffer+sizeof(st_pool_entry));

            // Add this pool to the list of pools
            pool->next=poollist_head;
            poollist_head=pool;
        }
        else
        {
            uint8 *buffer;
            TEMPLATE_CLASS *workclass;
            struct st_pool_entry *pool;
            unsigned int i;

            buffer=(uint8 *)malloc(sizeof(st_pool_entry)+sizeof(TEMPLATE_CLASS)*allocation_step_itemcount);

            if (!buffer)
                return;

            // Junk memory to be on the safe side
            memset(buffer, 0, sizeof(st_pool_entry));
            memset(buffer+sizeof(st_pool_entry), 0xDD, sizeof(TEMPLATE_CLASS)*allocation_step_itemcount);

            // Set the pointer to the pool header
            pool=(st_pool_entry *)buffer;

            // Set the pointer to the first entry
            workclass=(TEMPLATE_CLASS *)(buffer+sizeof(st_pool_entry));
            for (i=0;i<allocation_step_itemcount-1;i++)
            {
                // Set each entry (except the last) to point to the next extry
                ((st_freelist_member *)workclass)->next=(st_freelist_member *)(workclass+1);
                workclass++;
            }

            // Last entry should point to the head of the freelist
            ((st_freelist_member *)workclass)->next=freelist_head;

            // New freelist head should point to the first member
            freelist_head=(st_freelist_member *)(buffer+sizeof(st_pool_entry));

            // Add this pool to the list of pools
            pool->next=poollist_head;
            poollist_head=pool;
        }

    }



};

/** @} */

#endif
