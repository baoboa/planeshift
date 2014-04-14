/*
 * sectormngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2012 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef SND_SECTOR_MNGR_H
#define SND_SECTOR_MNGR_H


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>

//====================================================================================
// Project Includes
//====================================================================================
#include "util/singleton.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "soundctrl.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class csString;
class SoundControl;
class psSoundSector;
struct iMeshWrapper;
struct iObjectRegistry;

/**
 * This class handles every sound defined in a sector. It basically controls all
 * sounds that comes from the 3D world.
 */
class SoundSectorManager: public iSoundControlListener,  public Singleton<SoundSectorManager>
{
public:

    /**
     * Constructor.
     */
    SoundSectorManager();

    /**
     * Destructor.
     */
    virtual ~SoundSectorManager();

    /**
     * Load all sound sectors defined in XML files stored in the areas folder. Its     * path is defined in the option Planeshift.Sound.AreasPath of the configuration
     * file.
     * @param objectReg the object registry.
     * @return true if sectors could be loaded, false otherwise.
     */
    bool Initialize(iObjectRegistry* objectReg);

    /**
     * Unload all sound sectors.
     */
    void UnloadSectors();

    /**
     * Reload all sound sectors from XML definition. If SoundSectorManager has not
     * been already initialized nothing happens.
     * @return true if sectors could be loaded again, false otherwise.
     */
    bool ReloadSectors();

    /**
     * Update the active sector. This is called every x milliseconds by SoundManager::Update()
     * It updates Emitters and Entities
     */
    void Update();

    /**
     * Checks if the background music loop is allowed.
     * @return true if the background music loop is allowed, false otherwise.
     */
    bool IsLoopBGMOn() const
    {
        return loopBGM;
    }

    /**
     * Toggles the background music.
     * @param toggle true to make the background music to loop.
     */
    void SetLoopBGMToggle(bool toggle);

    /**
     * Checks if the combat music is activated.
     * @return true if the combat music is activated, false otherwise.
     */
    bool IsCombatMusicOn() const
    {
        return isCombatMusicOn;
    }

    /**
     * Toggles the combat music.
     * @param toggle true to activate the combat music.
     */
    void SetCombatMusicToggle(bool toggle);

    /**
     * Gets the common sector.
     * @return the common sector.
     */
    psSoundSector* GetCommonSector()
    {
        return commonSector;
    }

    /**
     * Sets the current active sector to the one with the given name and deactivate
     * the previous one (if any). If no sound sector with that name is found, nothing
     * change.
     * @return true if the given sector exists, false otherwise.
     */
    bool SetActiveSector(const char* sectorName);

    /**
     * Gets the current combat stance.
     * @return the current combat stance.
     */
    int GetCombatStance() const
    {
        return combatStance;
    }

    /**
     * Sets the current combat stance.
     * @param newCombatStance the new combat stance.
     */
    void SetCombatStance(int newCombatStance);

    /**
     * Gets the time of the day.
     * @return the time of the day.
     */
    int GetTimeOfDay() const
    {
        return timeOfDay;
    }

    /**
     * Sets the current time of the day.
     * @param newTimeOfDay the new time of the day.
     */
    void SetTimeOfDay(int newTimeOfDay);

    /**
     * Sets the current weather.
     * @param newWeather the new weather.
     */
    void SetWeather(int newWeather);

    /**
     * Sets the new state for the entity associated to the given mesh. If
     * it is already playing a sound, it is stopped.
     *
     * @param state the new state >= 0 for the entity. For negative value
     * the function is not defined.
     * @param mesh the mesh associated to the entity.
     * @param meshName the name associated to the entity.
     * @param forceChange if it is false the entity does not change its
     * state if the new one is not defined. If it is true the entity stops
     * play any sound until a new valid state is defined.
     */
    void SetEntityState(int state, iMeshWrapper* mesh, const char* meshName, bool forceChange);

    /**
     * Adds an object entity to be managed from the current sector.
     * @param mesh The mesh associated to the entity.
     * @param meshName the name associated to the entity.
     */
    void AddObjectEntity(iMeshWrapper* mesh, const char* meshName);

    /**
     * Removes an object entity managed from the current sector.
     * @param mesh The mesh associated to the entity.
     * @param meshName the name associated to the entity.
     */
    void RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName);

    /**
     * Updates an object entity managed from the current sector.
     * @param mesh The mesh associated to the entity.
     * @param meshName the name associated to the entity.
     */
    void UpdateObjectEntity(iMeshWrapper* mesh, const char* meshName);


    // From iSoundControlListener
    //-----------------------------
    virtual void OnSoundChange(SoundControl* sndCtrl);

private:
    csRef<iObjectRegistry> objectReg;  ///< object registry.

    int weather;                       ///< current weather state (from weather.h).
    int timeOfDay;                     ///< time of the day.
    int combatStance;                  ///< current combat stance.

    bool loopBGM;                      ///< true if background music should loop.
    bool isCombatMusicOn;              ///< true if combat music is active.

    SoundControl* musicSndCtrl;        ///< cache of SoundControl for music.
    SoundControl* ambientSndCtrl;      ///< cache of SoundControl for ambient sounds.

    bool areSectorsLoaded;             ///< true if the sectors are already loaded.
    psSoundSector* activeSector;       ///< current active sector.
    psSoundSector* commonSector;       ///< sector that keeps features common to all sectors.
    csArray<psSoundSector*> sectors;   ///< array which contains all sectors.

    /**
     * Load all sectors from the XML definition; areasPath and commonName must
     * have been initialized before calling this method. If a sector is defined
     * twice, the new definition is "appended" to the old one so it is possible
     * to split a sector definition in more than a file.
     * @return true if the loading was successful, false otherwise.
     */
    bool LoadSectors();

    /**
    * Find sector by name.
    * @param sectorName name of the sector to search.
    * @return the sector or 0 if it could not be found.
    */
    psSoundSector* FindSector(const csString &sectorName) const;

    /**
     * Update a whole sector.
     * @param sector SoundSector to update.
     */
    void UpdateSector(psSoundSector* sector);

    /**
     * Converts the sector's factory emitters to real emitters. After
     * conversion the factory emitters are removed.
     * @param sector the sound sector that needs the conversion.
     */
    void ConvertFactoriesToEmitter(psSoundSector* sector);

    /**
     * Transfers handle from a psSoundSector to a another psSoundSector
     * Moves SoundHandle and takes care that everything remains valid.
     * Good example on what is possible with all those interfaces.
     * @param oldSector sector to move from
     * @param newSector sector to move to
     */
    void TransferHandles(psSoundSector* oldSector, psSoundSector* newSector);

};

#endif /* SND_SECTOR_MNGR_H */

