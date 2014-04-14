/*
 * manager.h
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef _SOUND_MANAGER_H_
#define _SOUND_MANAGER_H_


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <iutil/objreg.h>
#include <csgeom/vector3.h>
#include <csutil/hash.h>
#include <iutil/timer.h>
#include <csutil/timer.h>
#include <csutil/randomgen.h>

//====================================================================================
// Project Includes
//====================================================================================
#include "util/singleton.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class SoundSystem;
class SoundDataCache;
class SoundHandle;
class SoundControl;

enum
{
    LOOP        = true,
    DONT_LOOP   = false
};


#define VOLUME_ZERO 0.0f
#define VOLUME_MIN  0.0f
#define VOLUME_NORM 1.0f
#define VOLUME_MAX  2.0f

#define DEFAULT_SNDSYS_UPDATE_TIME 100
#define DEFAULT_SPEED_OF_SOUND 331
#define DEFAULT_DOPPLER_FACTOR 0.0
#define DEFAULT_SOUNDLIB_PATH "/planeshift/art/soundlib.xml"

/**
 * This Manager Object is used to play all sounds.
 * It keeps track of Data/Sounds and does fading, sets volume and removes
 * sounds which are no longer needed.
 * You may use the Play*Sound methods to play a sound. You need a @see SoundControl
 * and a empty @see SoundHandle pointer to play a unmanaged sound. 
 * You may discard the Handle or use it the manage your sound.
 * SoundControl and SoundHandle have many Interface functions which help you todo what
 * ever you want. Access to the underlaying sources is also granted. 
 */

class SoundSystemManager: public Singleton<SoundSystemManager>
{
public:
    bool                   Initialised;       ///< is initialized ?
    SoundControl*          mainSndCtrl;
    csRef<iEventTimer>     eventTimer;        ///< timer event used by all the sound handle to play after a delay

    /**
     * Constructor.
     */
    SoundSystemManager();

    /**
     * Destructor will remove everything this SoundManager created.
     * It will stop and delete all Handles, removes @see SoundDataCache and @see SoundSystem.
     * All Pointers to objects within this Manager becoming invalid (handles SoundControls etc).
     * Make Sure that you dont use them after destruction.
     */
    ~SoundSystemManager();

    /**
     * Initializes this SoundSystemManager.
     * Also initializes a @see SoundSystem and a @see SoundDataCache object.
     * Loads soundlib if presend and set Initialised to true or false (based on success)
     * @param objectReg the object registry.
     * @return true if initialization is successful, false otherwise.
     */
    bool Initialize(iObjectRegistry* objectReg);

    /**
     * Check if the given handle ID exists.
     * @param handleID the handle ID to check.
     */
    bool IsHandleValid(uint handleID) const;

    /**
     * Checks all SoundHandles alters Volume, does fading, applies the Doppler effect and removes
     * them if Unmanaged (Autoremove true).
     */
    void UpdateSound();

    /**
     * Plays a 2D sound (Cannot be converted to 3D).
     * @param name name of the resource you want to play @see SoundDataCache for details
     * @param loop LOOP or DONT_LOOP
     * @param loopstart startframe when looping
     * @param loopend when reach it will jump to startframe
     * @param volume_preset volume for this sound, all volume calculations are based upon this
     * @param sndCtrl SoundControl to control this sound
     * @param handle a Handle you have to supply. If the handle is null a new one is created. You may discard
     * it if you dont want to manage this sound.
     * @return true if the handle could be played, false otherwise.
     */
    bool Play2DSound(const char* name, bool loop, size_t loopstart,
                     size_t loopend, float volume_preset,
                     SoundControl* &sndCtrl, SoundHandle* &handle);

    /**
     * Plays a 3D sound.
     * @param name name of the resource you want to play @see SoundDataCache for details
     * @param loop LOOP or DONT_LOOP
     * @param loopstart startframe when looping
     * @param loopend when reach it will jump to startframe
     * @param volume_preset volume for this sound, all volume calculations are based upon this
     * @param pos Sound 3d position of the sound, keep type3d in mind because it can be ABSOLUTE or RELATIVE
     * @param dir direction this sound is emitting to
     * @param mindist distance when maxvolume should be reached
     * @param maxdist distance when minvolume is applied
     * @param rad radiation of the directional cone. Set it to 0 if you dont want a directional sound.
     * @param handle a Handle you have to supply. If the handle is null a new one is created. You may discard
     * it if you dont want to manage this sound.
     * @param dopplerEffect true to apply the doppler effect to this sound, false otherwise.
     * @return true if the handle could be played, false otherwise.
     * */
    bool Play3DSound(const char* name, bool loop, size_t loopstart,
                     size_t loopend, float volume_preset,
                     SoundControl* &sndCtrl, csVector3 pos, csVector3 dir,
                     float mindist, float maxdist, float rad, int type3d,
                     SoundHandle* &handle, bool dopplerEffect = true);

    /**
     * Pause a sound and set the autoremove. The handle is picked up in the next update.
     * @param handleID the ID of the handle to stop.
     * @return true if the handle exists, false otherwise.
     */
    bool StopSound(uint handleID);

    /**
     * Set the position of the sound source of the handle with the given ID.
     * @param handleID the ID of the handle to stop.
     * @param position the new position of the sound's source.
     * @return true if the handle exists, false otherwise.
     */
    bool SetSoundSource(uint handleID, csVector3 position);

    /**
     * Sets the current player's position.
     * @param pos the new player's position.
     */
    void SetPlayerPosition(csVector3& pos);

    /**
     * Gets the current player's position.
     * @return the player's position.
     */
    csVector3& GetPlayerPosition();

    /**
     * Sets the player velocity.
     * @param vel the player's velocity.
     */
    void SetPlayerVelocity(csVector3 vel);

    /**
     * Update listener position.
     * @param v viewpoint or for that matter hearpoint
     * @param f front
     * @param t top 
     */
    void UpdateListener(csVector3 v, csVector3 f, csVector3 t);

    /**
     * Gets the listener position.
     * @return the listener position.
     */
    csVector3 GetListenerPos() const;

    /**
     * Updates this SoundManager and everything its driving.
     * Consider this as its Mainloop. It does ten updates per second.
     * It calls UpdateSound and updates SoundDataCache.
     */
    void Update();

    /**
     * Returns a NEW SoundControl.
     * It creates a new SoundControl and returns it. Also keeps
     * a internal list of all existing SoundControls. But atm there
     * is no way to recover a lost SoundControl. 
     */
    SoundControl* AddSoundControl(uint sndCtrlID);
    void RemoveSoundControl(uint sndCtrlID);
    SoundControl* GetSoundControl(uint sndCtrlID) const;

    SoundSystem* GetSoundSystem() const { return soundSystem; }
    SoundDataCache* GetSoundDataCache() const { return soundDataCache; }

private:
    SoundSystem*                   soundSystem;
    SoundDataCache*                soundDataCache;
    csHash<SoundHandle*, uint>     soundHandles;       ///< hash which contains all SoundHandles by id
    csHash<SoundControl*, uint>    soundControllers;   ///< hash which contains all SoundControls by id

    uint                           updateTime;         ///< update throttle of the sound system in milliseconds
    csTicks                        SndTime;            ///< current csticks
    csTicks                        LastUpdateTime;     ///< when the last update happened

    csVector3                      playerPosition;     ///< current player's position
    csVector3                      playerVelocity;     ///< the main player's speed

    uint                           speedOfSound;       ///< the speed of sound for doppler effect
    float                          dopplerFactor;      ///< the doppler factor that multiplies the change of frequency

    /**
     * Finds a unique ID > 0 for a SoundHandle.
     * @return the unique ID for the SoundHandle.
     */
    uint FindHandleID();

    /**
     * Stops and removes a SoundHandle.
     * Use with caution. It doesn't care if it's still in use.
     * @param handleID the id of the handle
     */
    void RemoveHandle(uint handleID);

    /**
     * Changes the play rate percent for the stream of the given handle according to
     * the Doppler effect. It always considers the source not moving.
     * @note: always use SoundHandle::Is3D() before calling this method.
     *
     * @param handle the SoundHandle with the stream to changes.
     */
    void ChangePlayRate(SoundHandle* handle);

    /**
     * Create a SoundHandle if the given one is null and it initializes it.
     *
     * @param name name of the resource you want to play @see SoundDataCache for details
     * @param loop LOOP or DONT_LOOP
     * @param loopstart startframe when looping
     * @param loopend when reach it will jump to startframe
     * @param volume_preset volume for this sound, all volume calculations are based upon this
     * @param sndCtrl SoundControl to control this sound
     * @param handle the SoundHandle that will be initialized. After the method's call
     * it is a null pointer if it could not be created.
     * @param dopplerEffect true to apply the doppler effect to this sound, false otherwise.
     * @return true if the handle has been initialized, false otherwise.
     */
    bool InitSoundHandle(const char* name, bool loop, size_t loopstart,
                     size_t loopend, float volume_preset, int type3d,
                     SoundControl* &sndCtrl, SoundHandle* &handle, bool dopplerEffect);
};

#endif /*_SOUND_MANAGER_H_*/
