/*
 * handle.h
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

#ifndef _SOUND_HANDLE_H_
#define _SOUND_HANDLE_H_


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <crystalspace.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "soundctrl.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class SoundSystemManager;


enum
{
    FADE_DOWN   =   0,
    FADE_UP     =   1,
    FADE_STOP   =   -1
};

class SoundHandle: public scfImplementation1<SoundHandle, iTimerEvent>, public iSoundControlListener
{
public:
    csString                                name;           ///< name of the resource or the file - not unique
    SoundControl*                           sndCtrl;        ///< @see SoundControl

    csRef<iSndSysData>                      snddata;        ///< pointer to sound data
    csRef<iSndSysStream>                    sndstream;      ///< sound stream
    csRef<iSndSysSource>                    sndsource;      ///< sndsource if 2D
    csRef<iSndSysSource3D>                  sndsource3d;    ///< sndsource if 3D
    csRef<iSndSysSource3DDirectionalSimple> sndsourcedir;   ///< additional source if 3D and directional

    SoundHandle();                                          ///< constructor 
    virtual ~SoundHandle();                                 ///< destructor

    /**
     * Does fading calculation for this Handle
     * @param volume volume to add / substract
     * @param time time within this must be done
     * @param direction FADE_DOWN / FADE_UP or FADE_STOP
     */
    void Fade(float volume, int time, int direction);

    /**
     * Perform a fading step if it is needed.
     */
    void FadeStep();

    /**
     * Initialize this Handle.
     * Done within this because its not failsave. Returns true or false.
     * @param resname name of the resource
     * @param loop LOOP or DONT_LOOP
     * @param volume_preset volume which all calculation are based upon
     * @param type 3d type: can be CS_SND3D_DISABLE=0. CS_SND3D_RELATIVE=1 or CS_SND3D_ABSOLUTE=2
     * @param ctrl SoundControl which controls this Handle
     * @param dopplerEffect true to apply the doppler effect to this sound, false otherwise.
     */
    virtual bool Init(const char* resname, bool loop,
              float volume_preset, int type,
              SoundControl* &ctrl, bool dopplerEffect);

    /**
     * Unpause the sound after the given delay has elapsed.
     */
    virtual bool Perform(iTimerEvent* /*ev*/);

    /**
     * Converts this Handle to a 3D Handle.
     * Note: doesnt work when type is CS_SND3D_DISABLE
     * Also created directional sources if rad > 0
     * @param mindist distance when volume reaches max
     * @param maxdist distance when volume is a min
     * @param pos 3d position of this sound
     * @param dir direction this sound is emitting to (if rad > 0)
     * @param rad radiation of the directional cone
     */
    void ConvertTo3D(float mindist, float maxdist,
                     csVector3 pos, csVector3 dir,
                     float rad);

    /**
     * Detects if the source of this handle is 3D.
     * @return true if the source is 3D, false otherwise.
     */
    bool Is3D();

    /**
     * Check if the doppler effect is enabled for this sound.
     * @return true if the doppler effect is enabled, false otherwise.
     */
    bool IsDopplerEffectEnabled();

    /**
     * Check if the listener is within the maximum distance allowed.
     * @param listenerPos the listener position.
     * @return true if the listener is within the maximum distance from the object.
     */
    bool IsWithinMaximumDistance(csVector3 listenerPos) const;

    /**
     * Gets the ID of this handle.
     * @return the ID of this handle.
     */
    uint GetID() const { return id; }

    /**
     * Sets the ID of this handle.
     * @param id the new ID of this handle.
     */
    void SetID(uint identifier) { id = identifier; }

    /**
     * Gets the position of the source.
     * @note: always call SoundHandle::Is3D() before calling this method.
     *
     * @return the position of the source.
     */
    csVector3 GetSourcePosition();

    /**
     * Whether to remove this Sound/Handle on pause.
     * True means it will be removed  when Sound is paused (Unamanged Sound).
     * False means it will stay and leak if you dont take care (Managed Sound).
     * @param toggle true or false
     */
    void SetAutoRemove(bool toggle);

    /**
     * Returns state of AutoRemove
     */
    bool GetAutoRemove();

    /**
     * Unpause the sound stream of this header after the given delay.
     * @param delay the delay in milliseconds.
     */
    void UnpauseAfterDelay(unsigned int delay);

    /**
     * Sets a Callback to a object function.
     * @param object pointer to the object
     * @param function pointer to a static void function within the object
     */
    void SetCallback(void (*object), void (*function) (void *));
    /**
     * Remove the callback.
     */
    void RemoveCallback();

    // From iSoundControlListener
    //----------------------------
    virtual void OnSoundChange(SoundControl* sndCtrl);

protected:
    bool dopplerEffect;                ///< true if the doppler effect is enabled for this sound
    float currentVolume;               ///< the volume all calculations are based upon.

private:
    uint id;                           ///< id of this handle
    int  fadeSteps;                    ///< number of steps to complete the fade, >0 if fading up, <0 if fading down.
    bool fadeStop;                     ///< true if the sound must be paused after fading down.
    float maxDistance;                 ///< maximum distance at which this sound is heard (if the sound is 3D)
    bool hasCallback;                  ///< true of theres a callback set, false of not
    void (*callbackobject);            ///< pointer to the callback object
    void (*callbackfunction) (void *); ///< pointer to the callback function

    bool autoremove;                   ///< remove this handle when pause?
    bool delayActive;                  ///< true if the handle is waiting for the end of delay to unpause

    /**
     * Makes the callback if set.
     * Calls callbackfunction of callbackobject.
     */
    void Callback();

};

#endif /*_SOUND_HANDLE_H_*/
