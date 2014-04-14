/*
 * cachemanager.h
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __CACHEDOBJECT_H__
#define __CACHEDOBJECT_H__

struct iCachedObject
{
    /** This function is called by the generic cache
    *  if the specified ticks go by and the cache
    *  object is not already removed.  After this
    *  function is called, the object is removed
    *  from the generic cache and deleted.
    */
    virtual void ProcessCacheTimeout() = 0;
    virtual void* RecoverObject() = 0;
    virtual void DeleteSelf() = 0;
    virtual ~iCachedObject() {}
};


#endif

