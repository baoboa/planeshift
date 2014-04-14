/*
 * factions.h
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
#ifndef __FACTIONS_H__
#define __FACTIONS_H__

#include <csutil/hash.h>
#include <csutil/csstring.h>

/** This struct stores the values and text used for the dynamically
 *  generated life events based on factions
 */
struct FactionLifeEvent
{
    int value;                  ///< Value from which this life event is attribuited
    csString event_description; ///< The text of this life event
    
    bool operator== (FactionLifeEvent OtherEvt) const { return value == OtherEvt.value; }
    bool operator<  (FactionLifeEvent OtherEvt) const { return value <  OtherEvt.value; }
};

/** An ingame faction group.
 *
 */
struct Faction
{
    csString name;
    csString description;
    int      id;
    float    weight;
    csArray<FactionLifeEvent> PositiveFactionEvents; ///< Stores the Positive faction values life events
    csArray<FactionLifeEvent> NegativeFactionEvents; ///< Stores the Negative faction values life events
};


/** This struct stores the particular score of a particular player to a
 *   particular faction.
 *
 */
struct FactionStanding
{
    Faction *faction;
    int score;
    bool dirty;
};


/**
 * This class is a set of faction structures. It's designed for storing,
 * updating and saving many faction scores per player in a compact way.
 */
class FactionSet
{
protected:
    /// A list of all the standings with each faction
    csHash<FactionStanding*, int>   factionstandings;

    /// A list of all the factions in this set
    csHash<Faction*, int>*   factions_by_id;

public:
    /** Construct the faction set based on the comma delimited text.
      * @param csv_list The list of comma delimited factions.  Faction,score.
      * @param factionset The global list of factions.
      */
    FactionSet(const char *csv_list,csHash<Faction*, int, CS::Memory::AllocatorMalloc> &factionset);

    ~FactionSet();

    bool GetFactionStanding(int factionID,int& standing, float& weight);

    /** Updates a faction standing.
      * If the factionID is not in the current list of standings a new standing
      * is added.
      *
      * @param factionID The ID of the faction to update.
      * @param delta The amount to change the faction score by.  If the faction is
      *              not in the current list this will be the starting score.
      * @param setDirty declares if the variable should be set dirty when updated.
      * @param overwrite Sets if the delta should overwrite the current value (true)
      *        or just be added to the current value (false)
      * 
      */
    void UpdateFactionStanding(int factionID, int delta, bool setDirty = true, bool overwrite = false);

    /** Create a comma delimited string based on the current faction standings.
      * @param csv The destination for the constructed string.
      */
    void GetFactionListCSV(csString& csv);

    float FindWeightedDiff(FactionSet* other);

    csHash<FactionStanding*, int>&  GetStandings() { return factionstandings; }

    /**
     * Check given faction.
     */
    bool CheckFaction(Faction * faction, int value);

    /**
     * Just get the number.
     */
    int GetFaction(Faction *faction);
};



#endif

