/*
 * psraceinfo.h
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



#ifndef __PSRACEINFO_H__
#define __PSRACEINFO_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "psitem.h"
#include "pscharacter.h"  // for gender 

enum PSRACEINFO_STAMINA
{
    PSRACEINFO_STAMINA_PHYSICAL_STILL = 0,
    PSRACEINFO_STAMINA_PHYSICAL_WALK,
    PSRACEINFO_STAMINA_MENTAL_STILL,
    PSRACEINFO_STAMINA_MENTAL_WALK
};

struct psRaceStartingLocation
{
    float x,y,z,yrot;
    float range; ///< the range in which a random position will be choosen.
    const char* sector_name;
};

struct psRaceInfo
{
protected:
    unsigned short attributes[PSITEMSTATS_STAT_COUNT];

public:
    psRaceInfo();
    ~psRaceInfo();
    bool Load(iResultRow &row);

    bool LoadBaseSpeeds(iObjectRegistry* object_reg);

    unsigned int uid;
    unsigned int race;
    csString name;
    csString sex;

    PSCHARACTER_GENDER gender;

    csString mesh_name;
    csString base_texture_name;
    csVector3 size;
    int initialCP;
    uint32 natural_armor_id;
    uint32 natural_weapon_id;
    float runMinSpeed,runBaseSpeed,runMaxSpeed;
    float walkMinSpeed,walkBaseSpeed,walkMaxSpeed;

    float GetBaseAttribute(PSITEMSTATS_STAT attrib);

private:

    csString helmGroup;         ///< The name of the helm group race is in.
    csString BracerGroup;       ///< The name of the bracer group race is in.
    csString BeltGroup;         ///< The name of the belt group race is in.
    csString CloakGroup;        ///< The name of the cloak group race is in.

    csString MounterAnim;       ///< The name of the anim the mounter will use when mounting this race.

    float speedModifier;        ///< The speed modifier of this race compared to default speeds

    float scale;                ///< The scale override of this race

    void  SetBaseAttribute(PSITEMSTATS_STAT attrib, float val);
public:

    float baseRegen[4];

    csArray<psRaceStartingLocation> startingLocations;

    void GetStartingLocation(float &x,float &y, float &z,float &rot,float &range,const char* &sectorname);
    void GetSize(csVector3 &size)
    {
        size = this->size;
    };

    /** Gets the name of this race.
     *  @return The race name.
     */
    const char* GetName()
    {
        return name.GetDataSafe();
    }

    const char* GetHonorific();
    const char* GetObjectPronoun();
    const char* GetPossessive();

    csString ReadableRaceGender();

    const char* GetGender()
    {
        return sex;
    }
    const char* GetRace()
    {
        return name;
    }

    const char* GetMeshName()
    {
        return mesh_name.GetDataSafe();
    }

    /** Gets the name of the texture associated to this race, if any.
     *  @return The name of the texture.
     */
    const char* GetTextureName()
    {
        return base_texture_name.GetDataSafe();
    }

    const char* GetHelmGroup()
    {
        return helmGroup.GetDataSafe();
    }
    const char* GetBracerGroup()
    {
        return BracerGroup.GetDataSafe();
    }
    const char* GetBeltGroup()
    {
        return BeltGroup.GetDataSafe();
    }
    const char* GetCloakGroup()
    {
        return CloakGroup.GetDataSafe();
    }

    const char* GetMounterAnim()
    {
        return MounterAnim.GetDataSafe();
    }

    /** Returns the natural armor stat id used for this race when it's not equipping an armor.
     *  @return The itemstats id of the natural armor.
     */
    uint32 GetNaturalArmorID()
    {
        return natural_armor_id;
    }

    /** Returns the natural weapon stat id used for this race when it's not equipping a weapon.
     *  @return The itemstats id of the natural weapon.
     */
    uint32 GetNaturalWeaponID()
    {
        return natural_weapon_id;
    }

    float GetSpeedModifier()
    {
        return speedModifier;
    }

    /** Gets the scale override, if any, of this race.
     *  @return A float which contains the scale override for this race.
     */
    float GetScale()
    {
        return scale;
    }

    /** Gets the id of the race (regardless of sex).
     *  @return An int rappresenting the whole race.
     */
    int GetRaceID()
    {
        return race;
    }

    /** Gets the id of the specific race (considering also its sex, and alternative versions),
     *  @return An int rappresenting the specific instance of the race (as a specific sex).
     */
    int GetUID()
    {
        return uid;
    }


};



#endif

