/*
 * pscharacterlist.cpp
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
#include "util/log.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharacterlist.h"


psCharacterList::psCharacterList()
{
    memset(&characterentry,0,sizeof(characterentry));
    validSize=0;
}

psCharacterList::~psCharacterList()
{
}

const char* psCharacterList::GetCharacterName(int index)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return NULL;

    if(!characterentry[index].valid)
        return NULL;
    return characterentry[index].character_name;
}

const char* psCharacterList::GetCharacterFullName(int index)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return NULL;

    if(!characterentry[index].valid)
        return NULL;
    return characterentry[index].character_fullname;
}

void psCharacterList::SetCharacterName(int index,const char* v)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return;

    if(!characterentry[index].valid)
        return;
    strncpy(characterentry[index].character_name,v,28);
    characterentry[index].character_name[27]=0x00;
}

void psCharacterList::SetCharacterFullName(int index,const char* v,const char* w)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return;

    if(!characterentry[index].valid)
        return;
    // Set first name
    SetCharacterName(index, v);
    // Set full name
    strncpy(characterentry[index].character_fullname,v,27);
    strncat(characterentry[index].character_fullname," ",1);
    strncat(characterentry[index].character_fullname,w,27);
    characterentry[index].character_fullname[55]=0x00;
}

unsigned int psCharacterList::GetCharacterID(int index)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return 0;

    if(!characterentry[index].valid)
        return 0;
    return characterentry[index].character_id;
}

void psCharacterList::SetCharacterID(int index,unsigned int v)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return;

    if(!characterentry[index].valid)
        return;
    characterentry[index].character_id=v;
}


bool psCharacterList::GetEntryValid(int index)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return false;
    return (characterentry[index].valid);
}

void psCharacterList::SetEntryValid(int index,bool v)
{
    if(index<0 || index>=MAX_CHARACTERS_IN_LIST)
        return;
    characterentry[index].valid=v;
}

int psCharacterList::GetListLength()
{
    return MAX_CHARACTERS_IN_LIST;
}

