/*
 * pstrait.h
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

#ifndef __PSTRAIT_H__
#define __PSTRAIT_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/charmessages.h"

//=============================================================================
// Local Includes
//=============================================================================

struct psTrait
{
    psTrait();
    ~psTrait();
    static const char* locationString[];
    const char* GetLocationString() const;
    csString ToXML(bool compact = false) const;
    unsigned int uid;
    unsigned int next_trait_uid;
    psTrait* next_trait;
    unsigned int raceID; // The DB uid for the race_info table
    unsigned int race;   // The internal race ID
    PSCHARACTER_GENDER gender;
    PSTRAIT_LOCATION location;
    csString name;
    unsigned int cstr_id_mesh;
    unsigned int cstr_id_material;
    unsigned int cstr_id_texture;
    bool onlyNPC;
    csString shaderVar;      ///< The variable for the shader for this trait.
};


#endif

