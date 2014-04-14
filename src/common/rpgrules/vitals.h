/*
 * vitals.h
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

#ifndef PS_VITAL_HEADER
#define PS_VITAL_HEADER

#define HP_REGEN_RATE  0.2F
#define MANA_REGEN_RATE  0.2F


/** The vitals that the client is concerned about. Used as params into
  * the vitals manager.
  */
enum PS_VITALS
{
    VITAL_HITPOINTS,
    VITAL_MANA,
    VITAL_PYSSTAMINA,
    VITAL_MENSTAMINA,
    VITAL_COUNT   // Count used to build a array of vitals, not a legal vital name
};

/** Used by the server to tell which fields are incoming */
enum PS_DIRTY_VITALS
{
    DIRTY_VITAL_HP              = 0x0001,
    DIRTY_VITAL_HP_MAX          = 0x0002,
    DIRTY_VITAL_HP_RATE         = 0x0004,
    DIRTY_VITAL_MANA            = 0x0008,
    DIRTY_VITAL_MANA_MAX        = 0x0010,
    DIRTY_VITAL_MANA_RATE       = 0x0020,
    DIRTY_VITAL_PYSSTAMINA      = 0x0040,
    DIRTY_VITAL_PYSSTAMINA_MAX  = 0x0080,
    DIRTY_VITAL_PYSSTAMINA_RATE = 0x0100,
    DIRTY_VITAL_MENSTAMINA      = 0x0200,
    DIRTY_VITAL_MENSTAMINA_MAX  = 0x0400,
    DIRTY_VITAL_MENSTAMINA_RATE = 0x0800,
    DIRTY_VITAL_EXPERIENCE      = 0x1000,
    DIRTY_VITAL_PROGRESSION     = 0x2000,
    DIRTY_VITAL_ALL = DIRTY_VITAL_HP | DIRTY_VITAL_HP_MAX | DIRTY_VITAL_HP_RATE |
                      DIRTY_VITAL_MANA | DIRTY_VITAL_MANA_MAX |DIRTY_VITAL_MANA_RATE |
                      DIRTY_VITAL_PYSSTAMINA | DIRTY_VITAL_PYSSTAMINA_MAX | DIRTY_VITAL_PYSSTAMINA_RATE |
                      DIRTY_VITAL_MENSTAMINA | DIRTY_VITAL_MENSTAMINA_MAX | DIRTY_VITAL_MENSTAMINA_RATE |
                      DIRTY_VITAL_EXPERIENCE |
                      DIRTY_VITAL_PROGRESSION
};

/** Manages a set of Vitals and does the predictions and updates on them
  *   when new data comes from the server.
  */
template <typename Vital>
class psVitalManager
{
public:
    psVitalManager()
    {
        experiencePoints  = 0;
        progressionPoints = 0;
        lastDRUpdate = 0;
    }
    ~psVitalManager() {}
    
    void SetVitals(const psVitalManager & newVitalMgr)
    {
        for(int i = 0; i < VITAL_COUNT; i++)
        {
            vitals[i] = newVitalMgr.vitals[i];
        }
    }

    /// Reset to the "original" vitals (for use when killing NPCs).
    void ResetVitals()
    {
        for (int i = 0; i < VITAL_COUNT; i++)
            vitals[i] = origVitals[i];
    }

    /// Saves the current vitals as the "original".
    void SetOrigVitals()
    {
        for (int i = 0; i < VITAL_COUNT; i++)
            origVitals[i] = vitals[i];
    }

    /** @brief Get a reference to a particular vital.
      *
      * @param vital @see PS_VITALS
      * @return The vital reference.
      */
    Vital & GetVital(int vital)
    {
        CS_ASSERT(vital >= 0 && vital < VITAL_COUNT);
        return vitals[vital];
    }

    // Quick accessors
    float GetHP()
    {
        return vitals[VITAL_HITPOINTS].value;
    }

    float GetMana()
    {
        return vitals[VITAL_MANA].value;
    }

    float GetPStamina()
    {
        return vitals[VITAL_PYSSTAMINA].value;
    }

    float GetMStamina()
    {
        return vitals[VITAL_MENSTAMINA].value;
    }

    /** Get players experience points. */
    unsigned int GetExp() { return experiencePoints; }

    /** Gets a players current progression points.*/
    unsigned int GetPP()  { return progressionPoints; }

protected:
    csTicks lastDRUpdate;

    /// A list of player vitals
    Vital vitals[VITAL_COUNT];
    Vital origVitals[VITAL_COUNT]; // saved copy for quick restore

    /// Players current experience points
    unsigned int experiencePoints;

    /// Players progression Points
    unsigned int progressionPoints;
};
#endif
