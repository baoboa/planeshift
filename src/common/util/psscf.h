/*
 * psscf.h    Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2001-2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef PSSCF_H
#define PSSCF_H

/**
 * \addtogroup common_util
 * @{ */

/* Note from Keith:  I would like to make this a hard assert()
   once we get all existing errors/leaks fixed, so we find them
   sooner from now on. For now it's just a printf.
   
   Use like CHECK_FINAL_DECREF(rainmesh,"Rain object"); just
   before your DecRef or setting a csRef to NULL to delete the
   object.  It should help us check if what we think is being
   freed is actually being freed.
*/

#define CHECK_FINAL_DECREF(obj,what)  \
    if (obj->GetRefCount() != 1)      \
    {                                 \
        printf("\n***Object is NOT being freed properly in %s:%d.  RefCount of %s is %d instead of 1.\n\n", \
           __FILE__,__LINE__,what, obj->GetRefCount() ); \
    }                               \

#define CHECK_FINAL_DECREF_CONFIRM(obj,what)  \
    CHECK_FINAL_DECREF(obj,what)              \
    else                    \
    {                        \
    printf("\n***Object %s is being deleted correctly in %s:%d.\n\n",what,__FILE__,__LINE__);  \
    }                        \


/** @} */

#endif
