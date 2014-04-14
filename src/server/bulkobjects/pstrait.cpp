/*
 * pstrait.cpp
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
#include "pstrait.h"

const char* psTrait::locationString[] = {"NONE","FACE","HAIR_STYLE","BEARD_STYLE","HAIR_COLOR","SKIN_TONE","ITEM", "EYE_COLOR"};

psTrait::psTrait():
    uid(0),next_trait_uid(0),next_trait(NULL),raceID(0),race(0),gender(PSCHARACTER_GENDER_NONE),
    location(PSTRAIT_LOCATION_NONE),cstr_id_mesh(0),cstr_id_material(0),cstr_id_texture(0),
    onlyNPC(false)
{
}

psTrait::~psTrait()
{
}

const char* psTrait::GetLocationString() const
{
    return locationString[(int)location];
}

csString psTrait::ToXML(bool compact) const
{
    csString str;
    csString genderStr;
    switch(gender)
    {
        case PSCHARACTER_GENDER_NONE:
            genderStr = "N";
            break;
        case PSCHARACTER_GENDER_MALE:
            genderStr += "M";
            break;
        case PSCHARACTER_GENDER_FEMALE:
            genderStr = "F";
            break;
        default:
            genderStr = "M";
            break;
    }
    if(compact)
    {
        str.Format("<trait id=\"%u\" next=\"%u\""
                   " loc=\"%s\" mesh=\"%u\""
                   " mat=\"%u\" tex=\"%u\" shader=\"%s\"/>",
                   uid,next_trait_uid,
                   GetLocationString(),cstr_id_mesh,
                   cstr_id_material,cstr_id_texture, shaderVar.GetData());
    }
    else
    {
        str.Format("<trait id=\"%u\" next=\"%u\" race=\"%d\" gender=\"%s\""
                   " loc=\"%s\" name=\"%s\" mesh=\"%u\""
                   " mat=\"%u\" tex=\"%u\" shader=\"%s\"/>",
                   uid,next_trait_uid,race,genderStr.GetData(),
                   GetLocationString(),name.GetData(),cstr_id_mesh,
                   cstr_id_material,cstr_id_texture, shaderVar.GetData());
    }
    return str;
}

