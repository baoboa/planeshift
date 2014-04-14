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

#include <cssysdef.h>
#include <iutil/objreg.h>
#include <csgeom/vector3.h>
#include <csutil/parray.h>

class SoundSystem;
class SoundData;
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

/**
 * This Manager Object is used to play all sounds.
 * It keeps track of Data/Sounds and does fading, sets volume and removes
 * sounds which are no longer needed.
 * You may use the Play*Sound methods to play a sound. You need a @see SoundControl
 * and a empty @see SoundHandle pointer to play a unmanaged sound. 
 * You may discard the Handle or use it the manage your sound.
 * SoundControl and SoundHandle have many Interface functions which help you todo what
 * ever you want. Access to the underlaying sources is also granted. 
 *     
 * TODO merge Play* functions
 */

class SoundSystemManager
{
    public:
    bool             Initialised;                   ///< is initialized ?
    SoundControl    *mainSndCtrl;                   ///< sound control for this manager
    SoundControl    *guiSndCtrl;                    ///< sound control for paws / gui
    SoundControl    *effectSndCtrl;                 ///< sound control for effects / actions

    /**
     * Constructor initializes this SoundSystemManager.
     * Also initializes a @see SoundSystem and a @see SoundData object.
     * Loads soundlib if presend and set Initialised to true or false (based on success)
     * You can always assume that this Interface works. BUT Play*Sound functions
     * will return false if its not Initialised. Be prepared to handle such Error conditions
     * because they may do that at any time (when something goes wrong).
     */
    SoundSystemManager (iObjectRegistry* objectReg);
    /**
     * Destructor will remove everything this SoundManager created.
     * It will stop and delete all Handles, removes @see SoundData and @see SoundSystem.
     * All Pointers to objects within this Manager becoming invalid (handles SoundControls etc).
     * Make Sure that you dont use them after destruction.
     */
    ~SoundSystemManager ();

    /**
     * Checks all SoundHandles alters Volume, does fading and removes them if Unmanaged (Autoremove true).
     */
    void UpdateSound ();

    /**
     * Plays a 2D sound (Cannot be converted to 3D).
     * @param name name of the resource you want to play @see SoundData for details
     * @param loop LOOP or DONT_LOOP
     * @param loopstart startframe when looping
     * @param loopend when reach it will jump to startframe
     * @param volume_preset volume for this sound, all volume calculations are based upon this
     * @param sndCtrl SoundControl to control this sound
     * @param handle a Handle you have to supply. You may discard it if you dont want to manage this sound.
     */
    bool Play2DSound (const char *name, bool loop, size_t loopstart,
                      size_t loopend, float volume_preset,
                      SoundControl* &sndCtrl, SoundHandle * &handle);

    /**
     * Plays a 3D sound.
     * @param name name of the resource you want to play @see SoundData for details
     * @param loop LOOP or DONT_LOOP
     * @param loopstart startframe when looping
     * @param loopend when reach it will jump to startframe
     * @param volume_preset volume for this sound, all volume calculations are based upon this
     * @param sndCtrl the sound controller
     * @param pos Sound 3d position of the sound, keep type3d in mind because it can be ABSOLUTE or RELATIVE
     * @param dir direction this sound is emitting to
     * @param mindist distance when maxvolume should be reached
     * @param maxdist distance when minvolume is applied
     * @param rad radiation of the directional cone. Set it to 0 if you dont want a directional sound.
     * @param type3d Either ABSOLUTE or RELATIVE.
     * @param handle a Handle you have to supply. You may discard it if you dont want to manage this sound.
     * */
    bool Play3DSound (const char *name, bool loop, size_t loopstart,
                      size_t loopend, float volume_preset,
                      SoundControl* &sndCtrl, csVector3 pos, csVector3 dir,
                      float mindist, float maxdist, float rad, int type3d,
                      SoundHandle * &handle);

    /**
     * Stops and removes a SoundHandle.
     * Use with caution. It doesnt care if its still in use.
     * @param handle A valid soundhandle
     */
    void StopSound (SoundHandle *handle);

    /**
     * Update listener position.
     * @param v viewpoint or for that matter hearpoint
     * @param f front
     * @param t top 
     */
    void UpdateListener (csVector3 v, csVector3 f, csVector3 t);

    /**
     * Updates this SoundManager and everything its driving.
     * Consider this as its Mainloop. It does ten updates per second.
     * It calls UpdateSound and updates SoundData.
     */
    void Update ();
    /**
     * Returns a NEW SoundControl.
     * It creates a new SoundControl and returns it. Also keeps
     * a internal list of all existing SoundControls. But atm there
     * is no way to recover a lost SoundControl. 
     */
    SoundControl* GetSoundControl ();

    SoundSystem* GetSoundSystem() { return soundSystem; }
    SoundData* GetSoundData() { return soundData; }

    private:
    SoundSystem         *soundSystem;
    SoundData           *soundData;
    csPDelArray<SoundControl> soundController;  ///< array which contains all SoundControls

    csPDelArray<SoundHandle> soundHandles;      ///< array which contains all SoundHandles

    csTicks                 SndTime;            ///< current csticks
    csTicks                 LastUpdateTime;     ///< when the last update happened
};

#endif /*_SOUND_MANAGER_H_*/
