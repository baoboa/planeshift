/*
 *
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
#ifndef PS_HEADER_SLOTS
#define PS_HEADER_SLOTS

#include <csutil/csstring.h>
#include <csutil/strhashr.h>
#include <csutil/parray.h>

/**
 * \addtogroup common_util
 * @{ */

/** Holds a list of the possible socket identifiers that items can be attached to.
 */
enum INVENTORY_SLOT_NUMBER
{
    PSCHARACTER_SLOT_STORAGE = -2,
    PSCHARACTER_SLOT_NONE = -1,
    PSCHARACTER_SLOT_RIGHTHAND = 0,
    PSCHARACTER_SLOT_LEFTHAND = 1,
    PSCHARACTER_SLOT_BOTHHANDS = 2,
    PSCHARACTER_SLOT_RIGHTFINGER = 3,
    PSCHARACTER_SLOT_LEFTFINGER = 4,
    PSCHARACTER_SLOT_HELM = 5,
    PSCHARACTER_SLOT_NECK = 6,
    PSCHARACTER_SLOT_BACK = 7,
    PSCHARACTER_SLOT_ARMS = 8,
    PSCHARACTER_SLOT_GLOVES = 9,
    PSCHARACTER_SLOT_BOOTS = 10,
    PSCHARACTER_SLOT_LEGS = 11,
    PSCHARACTER_SLOT_BELT = 12,
    PSCHARACTER_SLOT_BRACERS = 13,
    PSCHARACTER_SLOT_TORSO = 14,
    PSCHARACTER_SLOT_MIND = 15,
    PSCHARACTER_SLOT_BULK1 = 16,
    PSCHARACTER_SLOT_BULK2 = 17,
    PSCHARACTER_SLOT_BULK3 = 18,
    PSCHARACTER_SLOT_BULK4 = 19,
    PSCHARACTER_SLOT_BULK5 = 20,
    PSCHARACTER_SLOT_BULK6 = 21,
    PSCHARACTER_SLOT_BULK7 = 22,
    PSCHARACTER_SLOT_BULK8 = 23,
    PSCHARACTER_SLOT_BULK9 = 24,
    PSCHARACTER_SLOT_BULK10 = 25,
    PSCHARACTER_SLOT_BULK11 = 26,
    PSCHARACTER_SLOT_BULK12 = 27,
    PSCHARACTER_SLOT_BULK13 = 28,
    PSCHARACTER_SLOT_BULK14 = 29,
    PSCHARACTER_SLOT_BULK15 = 30,
    PSCHARACTER_SLOT_BULK16 = 31,
    PSCHARACTER_SLOT_BULK17 = 32,
    PSCHARACTER_SLOT_BULK18 = 33,
    PSCHARACTER_SLOT_BULK19 = 34,
    PSCHARACTER_SLOT_BULK20 = 35,
    PSCHARACTER_SLOT_BULK21 = 36,
    PSCHARACTER_SLOT_BULK22 = 37,
    PSCHARACTER_SLOT_BULK23 = 38,
    PSCHARACTER_SLOT_BULK24 = 39,
    PSCHARACTER_SLOT_BULK25 = 40,
    PSCHARACTER_SLOT_BULK26 = 41,
    PSCHARACTER_SLOT_BULK27 = 42,
    PSCHARACTER_SLOT_BULK28 = 43,
    PSCHARACTER_SLOT_BULK29 = 44,
    PSCHARACTER_SLOT_BULK30 = 45,
    PSCHARACTER_SLOT_BULK31 = 46,
    PSCHARACTER_SLOT_BULK32 = 47,
    PSCHARACTER_SLOT_BULK_END = 48 // not a valid number
};

/** A hash map class that stores a name->ID of sockets. This way we can easily
  * work with numbers and then convert to string if we need to.  This is shared
  * class across both the client and server since both may need this.
  */
class SlotNameHash
{
public:
    SlotNameHash();
    /// Get the enum ID from a string name.
    int GetID( const csString& name );

    /// Get the name from an enum
    const char* GetName( int id );

    /// Get the secondary, non exact name
    const char* GetSecondaryName( int id );
    void AddSecondaryName(const char* primary, const char* secondary);

private:

    struct PrimaryToSecondary
    {
        csString primary,secondary;
    };

    csStringHashReversible slotNames;
    csPDelArray<PrimaryToSecondary> secondaryNames;
};

/** @} */

#endif
