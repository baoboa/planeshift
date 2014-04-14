/*
 * stringarray.h
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PS_STRINGARRAY_H
#define PS_STRINGARRAY_H

#include <stdarg.h>
#include <csutil/stringarray.h>

/**
 * \addtogroup common_util
 * @{ */

/**
 * A slightly improved version of csStringArray, sporting the handy FormatPush
 * method.
 */
class psStringArray : public csStringArray
{
public:
    /**
     * Push a printf-style string onto the list (makes copy of string after
     * formatting).
     */
    size_t FormatPush(char const * fmt, ...)
    {
        csString str;
        va_list args;
        va_start(args, fmt);
        str.FormatV(fmt, args);
        va_end(args);
        return Push(str);
    }
};

/** @} */

#endif // PS_STRINGARRAY_H
