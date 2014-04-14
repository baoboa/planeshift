/*
 * psaccountinfo.cpp
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
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "psaccountinfo.h"


psAccountInfo::psAccountInfo()
{
    accountid=0;
    spamPoints=0;
    advisorPoints=0;
    securitylevel=0;
}

psAccountInfo::~psAccountInfo()
{
}

bool psAccountInfo::Load(iResultRow &row)
{
    accountid = row.GetUInt32("id");
    username  = row["username"];
    password  = row["password"];
    password256  = row["password256"];

    createddate = row["created_date"];
    lastlogintime = row["last_login"];
    lastloginip = row["last_login_ip"];
    os = row["operating_system"];
    gfxcard = row["graphics_card"];
    gfxversion = row["graphics_version"];

    spamPoints    = row.GetInt("spam_points");
    advisorPoints = row.GetInt("advisor_points");

    securitylevel = row.GetInt("security_level");

    return true;
}
