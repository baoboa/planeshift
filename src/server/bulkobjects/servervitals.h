/*
 * servervitals.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef SERVER_VITALS_HEADER
#define SERVER_VITALS_HEADER
#include "psstdint.h"

//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "rpgrules/vitals.h"
#include "../playergroup.h"

//=============================================================================
// Local Includes
//=============================================================================

class MsgEntry;
class psCharacter;

/// Buffables for vitals, which automatically update the dirty flag as necessary.
class VitalBuffable : public Buffable<float>
{
public:
    virtual ~VitalBuffable() { }

    void Initialize(unsigned int* sDirty, int dirtyF)
    {
        statsDirty = sDirty;
        dirtyFlag = dirtyF;
    }

protected:
    virtual void OnChange()
    {
        *statsDirty |= dirtyFlag;
    }

    int dirtyFlag; ///< The bit value we should set when this becomes dirty.
    unsigned int* statsDirty; ///< Pointer to the psServerVitals dirty bitfield.
};

/// A character vital (such as HP or Mana) - server side.
struct Vital
{
    Vital() : value(0.0) {}

    float value;
    VitalBuffable drRate; ///< Amount added to this vital each second
    VitalBuffable max;
};

//----------------------------------------------------------------------------

/** Server side of the character vitals manager.  Does a lot more accessing
  * of the data to set particular things.  Also does construction of data to
  * send to a client.
  */
class psServerVitals : public psVitalManager<Vital>
{
public:
    psServerVitals(psCharacter* character);

    /** Handles new Vital data construction for the server.
     */
    bool SendStatDRMessage(uint32_t clientnum, EID eid, unsigned int flags, csRef<PlayerGroup> group = NULL);

    bool Update(csTicks now);

    void SetExp(unsigned int exp);
    void SetPP(unsigned int pp);

    void SetVital(int vitalName, int dirtyFlag, float value);
    void AdjustVital(int vitalName, int dirtyFlag, float delta);

    /** Return the dirty flags for vitals.
     */
    unsigned int GetStatsDirtyFlags() const;

    /** Set all vitals dirty.
     */
    void SetAllStatsDirty();

    /** Cleare the dirty flags for vitals.
     */
    void ClearStatsDirtyFlags(unsigned int dirtyFlags);

private:
    /// Clamps the vital's current value to be in the interval [0, max].
    void ClampVital(int vital);

    /** @brief Adjust a field in a vital statistic.
     *
     *  @param vitalName One of the enums @see PS_VITALS
     *  @param dirtyFlag What became dirty in this adjustment. @see  PS_DIRTY_VITAL.
     *
     *  @return The new value of the field in the vital stat.
     */
    Vital &DirtyVital(int vitalName, int dirtyFlag);

    ///  @see  PS_DIRTY_VITALS
    unsigned int statsDirty;
    unsigned char version;
    psCharacter* character;  ///< the character whose vitals we manage
};

#endif

