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

#include <psconfig.h>
#include "slots.h"

SlotNameHash::SlotNameHash()
{
    // Construct the hash map with our name->enum ID.
    slotNames.Register ("righthand",     PSCHARACTER_SLOT_RIGHTHAND );
    slotNames.Register ("lefthand",      PSCHARACTER_SLOT_LEFTHAND );
    slotNames.Register ("bothhands",     PSCHARACTER_SLOT_BOTHHANDS );
    slotNames.Register ("helm",          PSCHARACTER_SLOT_HELM );
    slotNames.Register ("rightfinger",   PSCHARACTER_SLOT_RIGHTFINGER );
    slotNames.Register ("leftfinger",    PSCHARACTER_SLOT_LEFTFINGER );
    slotNames.Register ("neck",          PSCHARACTER_SLOT_NECK );
    slotNames.Register ("back",          PSCHARACTER_SLOT_BACK );
    slotNames.Register ("arms",          PSCHARACTER_SLOT_ARMS );
    slotNames.Register ("gloves",        PSCHARACTER_SLOT_GLOVES );
    slotNames.Register ("boots",         PSCHARACTER_SLOT_BOOTS );
    slotNames.Register ("legs",          PSCHARACTER_SLOT_LEGS );
    slotNames.Register ("belt",          PSCHARACTER_SLOT_BELT );
    slotNames.Register ("bracers",       PSCHARACTER_SLOT_BRACERS );
    slotNames.Register ("torso",         PSCHARACTER_SLOT_TORSO );
    slotNames.Register ("mind",          PSCHARACTER_SLOT_MIND );

    AddSecondaryName("gloves",      "hands");
    AddSecondaryName("boots",       "feet");
    AddSecondaryName("helm",        "head");
}

int SlotNameHash::GetID( const csString& name )
{
    csString newName( name );
    return slotNames.Request( newName );
}

const char* SlotNameHash::GetName( int id )
{
    return slotNames.Request( id );
}

const char* SlotNameHash::GetSecondaryName( int id )
{
    const char* name = GetName(id);
    if(!name)
        return NULL;

    for(size_t i = 0; i < secondaryNames.GetSize();i++)
    {
        PrimaryToSecondary* curr = secondaryNames.Get(i);
        if(curr->primary == name)
        {
            return curr->secondary;
        }
    }

    return name;
}

void SlotNameHash::AddSecondaryName(const char* primary, const char* secondary)
{
    PrimaryToSecondary* curr = new PrimaryToSecondary;
    curr->primary = primary;
    curr->secondary = secondary;

    secondaryNames.Push(curr);
}
