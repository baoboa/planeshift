/*
 * pscache.h
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Very simple base class for a cache mechanism, developed specifically for 
 * a client-side cache that requires synchronising from the server.
 */

#ifndef PS_CACHE
#define PS_CACHE

/**
 * \addtogroup common_util
 * @{ */

/**
 * psCache
 *
 * The psCache class implements the common cache mechanism.
 */
class psCache
{
    public:
        enum CACHE_STATUS
        {
            INVALID,
            VALID
        };

        psCache();
        ~psCache();

        /** Return cache status.
         *  @return CACHE_STATUS The cache status returns.
         */
        CACHE_STATUS GetCacheStatus (void) { return cacheStatus; }

        /** Set cache status.
         *  @param newStatus: new cache status.
         */
        void SetCacheStatus (CACHE_STATUS newStatus) { cacheStatus = newStatus; }

    protected:
        CACHE_STATUS cacheStatus;
};

/** @} */

#endif

