/*
 * heap.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Andrew Dai
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Creation Date: 10/Feb 2005
 * Description : This is an implementation of a min heap. (a priority queue)
 *
 */

#ifndef __HEAP_H__
#define __HEAP_H__

#include <csutil/array.h>

/**
 * \addtogroup common_util
 * @{ */

template <class T>
class Heap : protected csArray<T*>
{
    // Work around gcc 3.4 braindamage.
    using csArray<T*>::Get;
    using csArray<T*>::Top;

public:
    Heap(int ilimit = 0)
        : csArray<T*>(ilimit)
    {
    }

    size_t Length () const
    {
        return csArray<T*>::GetSize();
    }

    void Insert(T* what)
    {
        size_t i;

        // Make room for new element
        this->SetSize(Length() + 1);

        // This special case is required as we are not using a sentinel
        if(Length() > 1)
            for( i = Length()-1; i != 0 && *Get((i-1)/2) > *what; i = (i-1)/2)
                this->Put(i, this->Get((i-1)/2));
        else
            i = 0;

        this->Put(i, what);
    }

    T* DeleteMin()
    {
        size_t i, child;
        T* minElement = this->Get(0);
        T* lastElement = this->Top();

        CS_ASSERT(Length());
        // Shrink the array by 1
        this->Truncate(Length() - 1);

        if(!Length())
            return minElement;

        for(i = 0; i * 2 + 1 < Length(); i = child)
        {
            // Find the smaller child
            child = i * 2 + 1;
            if( child != Length()-1 && *this->Get(child + 1) < *this->Get(child) )
                child++;

            // Move it down one level
            if( *lastElement > *this->Get(child))
                this->Put(i, this->Get(child));
            else
                break;
        }
        this->Put(i, lastElement);
        return minElement;
    }

    T* FindMin()
    {
        if(Length())
            return this->Get(0);
        else
            return NULL;
    }
};

/** @} */

#endif

