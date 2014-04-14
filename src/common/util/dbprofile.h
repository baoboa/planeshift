/*
 * dbprofile.h by Ondrej Hurt
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

#ifndef __DBPROFILE_H__
#define __DBPROFILE_H__

#include <csutil/parray.h>
#include "util/psprofile.h"

class psString;

/**
 * \addtogroup common_util
 * @{ */

/**  Statistics of time consumed by SQL statements */
class psDBProfiles : public psNamedProfiles
{
public:

    virtual void AddSQLTime(const csString & sql, csTicks time);
    csString Dump();
    
protected:

    void StripConstantsFromSQL(psString & sql);

};

/** @} */

#endif
