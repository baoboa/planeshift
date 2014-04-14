/*
 * psaccountinfo.h
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

#ifndef __PSACCOUNTINFO_H__
#define __PSACCOUNTINFO_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <idal.h>
#include "../icachedobject.h"

//=============================================================================
// Local Includes
//=============================================================================



/** A data storage class to contain all information associated with an account.
 *
 *  An account in planeshift can have multiple characters.
 */
class psAccountInfo : public iCachedObject
{
public:
    psAccountInfo();
    ~psAccountInfo();

    bool Load(iResultRow &row);

    // iCachedObject Functions below
    virtual void ProcessCacheTimeout() {};  /// required for iCachedObject but not used here
    virtual void* RecoverObject()
    {
        return this;    /// Turn iCachedObject ptr into psAccountInfo
    }
    virtual void DeleteSelf()
    {
        delete this;    /// Delete must come from inside object to handle operator::delete overrides.
    }

    /// Each account has a unique id number associated with it
    unsigned int accountid;
    /// Each account has a unique username associated with it, chosen by the user
    csString username;
    /// The password field is plain text now but later may be a public encryption key or something more elaborate
    csString password;

    ///temporary transition variable
    csString password256;


    /// String value copied from the database containing the last login time
    csString lastlogintime;
    /// String value copied from the database containing the time of creation of the database entry
    csString createddate;
    /// String value of the last ip a connection came in from
    csString lastloginip;
    /// String containing the os the connecting machine is running
    csString os;
    /// String containing the graphics card the connecting machine is using
    csString gfxcard;
    /// String containing the graphics driver version the connecting machine is using;
    csString gfxversion;
    /// Level of spamming offenses for this account.
    int spamPoints;
    /// Number of questions answered on the help channel.
    int advisorPoints;
    /// Security level indicator for this account.
    int securitylevel;
};

#endif

