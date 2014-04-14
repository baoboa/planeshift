/*
 * factions.cpp
 *      written by Keith Fulton <keith@paqrat.com>
 *  
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <csutil/hash.h>
#include "factions.h"

FactionSet::FactionSet(const char *csv_list,csHash<Faction*, int> &factionset)
{
    factions_by_id = &factionset;

    if ( !csv_list )
        return;
    csString temp(csv_list);
    if ( temp == "(null)" )
        return;
    
                
    char *buff = temp.Detach();
    strcpy(buff,csv_list);
               
    char *ptr = strtok(buff,",");
    while (ptr)
    {
        FactionStanding *fs = new FactionStanding;
        fs->faction = factions_by_id->Get(atoi(ptr),0);
        CS_ASSERT(fs->faction != NULL);
        ptr = strtok(NULL,",");
        if (!ptr)
        {
            delete fs;
            break;
        }
        fs->score = atoi(ptr);
        fs->dirty = false;
        factionstandings.Put(fs->faction->id,fs);
        ptr = strtok(NULL,",");
    }
    delete[] buff;
}

FactionSet::~FactionSet()
{
    // Safely delete the FactionStanding objects
    csHash<FactionStanding*, int>::GlobalIterator iter(factionstandings.GetIterator());
    while(iter.HasNext())
    {
        FactionStanding* standing = iter.Next();
        delete standing;
    }
}

bool FactionSet::GetFactionStanding(int factionID,int& standing, float& weight)
{
    FactionStanding *fs = factionstandings.Get(factionID,0);
    if (fs)
    {
        standing = fs->score;
        weight   = fs->faction->weight;
        return true;
    }
    return false;
}

void FactionSet::UpdateFactionStanding(int factionID, int delta, bool setDirty, bool overwrite)
{
    FactionStanding *fs = factionstandings.Get(factionID,0);
    
    if (fs)
    {
        //if overwrite is true we set the value directly
        if(overwrite)
            fs->score = delta;
        //if overwrite is false in place we sum the delta
        else
            fs->score += delta;

        if(setDirty)
            fs->dirty = true;
    }
    else
    {
        fs = new FactionStanding;        
        fs->faction = factions_by_id->Get(factionID,0);
        fs->score = delta;
        //sets the dirty flag depending if this should setDirty or not
        //so it's possible to use this function even for db loading
        fs->dirty = setDirty;
        factionstandings.Put(fs->faction->id,fs);
    }
}

void FactionSet::GetFactionListCSV(csString& csv)
{
    csHash<FactionStanding*, int>::GlobalIterator iter = factionstandings.GetIterator();
    
    if (iter.HasNext())
    {
        FactionStanding *fs = iter.Next();
        csv.AppendFmt("%d,%d", fs->faction->id, fs->score);
    }
    while (iter.HasNext())
    {
        FactionStanding *fs = iter.Next();
        csv.AppendFmt(",%d,%d", fs->faction->id, fs->score);
    }
}

float FactionSet::FindWeightedDiff(FactionSet *other)
{
    csHash<FactionStanding*, int>::GlobalIterator iter = factionstandings.GetIterator();
    float total_standing = 0, total_weight = 0;

    while (iter.HasNext())
    {
        FactionStanding *my_fs;
        int standing;
        float weight;
        my_fs = iter.Next();
        if (!other->GetFactionStanding(my_fs->faction->id,standing,weight))
        {
            standing = 0;
            weight = my_fs->faction->weight;
        }
        total_standing += (standing - my_fs->score) * weight;
        total_weight   += weight;
    }
    if ( total_weight == 0 )
        return 0.0;
    return total_standing/total_weight;  // weighted average
}

bool FactionSet::CheckFaction(Faction * faction, int value)
{
    int standing;
    float weight;

    if (GetFactionStanding(faction->id,standing,weight))
    {
        return (value > standing);
    }
    else
    {
        return false;
    }
}

int FactionSet::GetFaction(Faction* faction)
{
    int standing = 0;
    float weight;
    bool worked = GetFactionStanding(faction->id,standing,weight);
    CS_ASSERT(worked);
    (void)worked; // supress unused var warning

    return standing;
}

