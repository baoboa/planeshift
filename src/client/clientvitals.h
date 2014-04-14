/*
 * clientvitals.h
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

#ifndef CLIENT_VITALS_HEADER
#define CLIENT_VITALS_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "rpgrules/vitals.h"

//=============================================================================
// Local Includes
//=============================================================================

class psStatDRMessage;

/// A character vital (such as HP or Mana) - client side
struct Vital
{
    Vital() : value(0.0), drRate(0.0) {}

    float value;
    float drRate;
};

/**
 * Handles the incoming vital data from the server to update it's
 * local values.
 */
class psClientVitals : public psVitalManager<Vital>
{
public:
    psClientVitals();

     /**
      * Handles new Vital data from the server.
      *
      * @param msg The Vital message from the server with the correct
      *            values for this Vital.
      * @param labelname The label to be appended to published values
      */
    void HandleDRData(psStatDRMessage& msg, const char *labelname );

     /**
      * Reset virtuals on death.
      *
      * @param labelname The label to be appended to published values
      */
    void HandleDeath( const char *labelname );

    /**
     * Predicts the new values of the various Vitals.
     *
     * This helps limit the lag time in getting new values for these Vitals.
     *
     * @param now  The current ticks to use from last valid data from the server.
     * @param labelname The label to be appended to published values
     */
    void Predict( csTicks now, const char *labelname );

private:
    /// version indicator to ignore out of order packets
    unsigned char counter;
    bool counterSet;
};


#endif
