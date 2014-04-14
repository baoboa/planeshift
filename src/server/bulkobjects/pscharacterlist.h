/*
 * pscharacterlist.h
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

#ifndef __PSCHARACTERLIST_H__
#define __PSCHARACTERLIST_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "../icachedobject.h"

//=============================================================================
// Local Includes
//=============================================================================


#define MAX_CHARACTERS_IN_LIST 10

class psCharacterList : public iCachedObject
{
public:
    psCharacterList();
    ~psCharacterList();

    const char* GetCharacterName(int index);
    const char* GetCharacterFullName(int index);
    void SetCharacterName(int index,const char* v);
    void SetCharacterFullName(int index,const char* v,const char* w);

    unsigned int GetCharacterID(int index);
    void SetCharacterID(int index,unsigned int v);

    bool GetEntryValid(int index);
    void SetEntryValid(int index,bool v);

    int GetListLength();
    /** Get the number of valid characters in list.
     */
    int GetValidCount()
    {
        return validSize;
    }

    /** Set the number of valid characters in this list.
      */
    void SetValidCount(int size)
    {
        validSize = size;
    }

    // iCachedObject Functions below
    virtual void ProcessCacheTimeout() {};  /// required for iCachedObject but not used here
    virtual void* RecoverObject()
    {
        return this;    /// Turn iCachedObject ptr into psCharacterList
    }
    virtual void DeleteSelf()
    {
        delete this;    /// Delete must come from inside object to handle operator::delete overrides.
    }

private:

    struct psCharacterListEntry
    {
        bool valid;
        unsigned int character_id;
        char character_name[28];
        char character_fullname[56];
    };

    psCharacterListEntry characterentry[MAX_CHARACTERS_IN_LIST];
    int validSize;
};


#endif


