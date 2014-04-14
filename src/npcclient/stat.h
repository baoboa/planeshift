/*
 * stat.h
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

#ifndef PS_STAT_HEADER
#define PS_STAT_HEADER

#include <cstypes.h>

/**
 * Represents a stat for the NPC. Used to extrapolated stat values
 * as they regenerate.
 */
class Stat
{
public:
    /**
     * Constructor
     */
    Stat() : value(0.0), max(0.0), rate(0.0), lastUpdate(0) {}

    /**
     * Update the stat to now.
     */
    void Update(csTicks now);

    /**
     * Set the stat value.
     */
    void SetValue(float value, csTicks now);

    /**
     * Set the maximum stat value.
     */
    void SetMax(float max);

    /**
     * Set the regeneration value for this stat.
     */
    void SetRate(float rate, csTicks now);

    /**
     * Return the current value of the stat.
     *
     * Call Update before to get the extrapolated value.
     */
    float GetValue() const
    {
        return value;
    }

    /**
     * Get the max stat value
     */
    float GetMax() const
    {
        return max;
    }

    /**
     * Get the regeneration rate.
     */
    float GetRate() const
    {
        return rate;
    }
private:
    float   value;         ///< Value of the stat
    float   max;           ///< Maximum value of the stat
    float   rate;          ///< Regeneration rate for the stat
    csTicks lastUpdate;    ///< Last time this stat was updated
};

#endif
