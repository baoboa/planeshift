/*
 * singleton.h - Author: Andrew Dai
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
 */

#ifndef __SINGLETON_H__
#define __SINGLETON_H__

/**
 * \addtogroup common_util
 * @{ */

template <typename T> class Singleton
{
    static T* ptrSingleton;
    
public:
    Singleton(T* ptr)
    {
        CS_ASSERT(ptrSingleton == 0);
        ptrSingleton = ptr;
    }

    // Use this constructor only when the derived class is only deriving from Singleton
    Singleton(void)
    {
        CS_ASSERT(ptrSingleton == 0);
        ptrSingleton = (T*) (this);
    }

    ~Singleton()
    {
        CS_ASSERT(ptrSingleton != 0);
        ptrSingleton = NULL;
    }
    static T& GetSingleton(void)
    {
        return *ptrSingleton;
    }
    static T* GetSingletonPtr(void)
    {
        return ptrSingleton;
    }
};
template <typename T> T* Singleton<T>::ptrSingleton = 0;

/** @} */

#endif

