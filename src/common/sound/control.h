/*
 * control.h
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


#ifndef _SOUND_CONTROL_H_
#define _SOUND_CONTROL_H_

/**
 * A Volume and Sound control class.
 * With this class you can control the volume and the overall state
 * of a unlimited number of SoundsHandles. The whole class is pretty straith forward and selfdescribing.
 * It also provides a callback functionality which calls back whenever something changes.
 * You can use it by using SetCallback, please make sure that you call RemoveCallback
 * before removing whether this Callback is set to.
 
 * Each SoundHandle must have one SoundControl associated. This is done during
 * SoundHandle creation @see SystemSoundManager or @see SoundHandle::Init for details.
 */

class SoundControl
{
    public:
    int     id;                         ///< id of this control
    /**
     * Sets internal callbackpointers.
     * Those Callbacks are called whenever a control changes.
     * E.g. someone changes Volume or pushes a Toggle
     * @param object pointer to a instance
     * @param function pointer to a static void function within that instance
     */ 
    void SetCallback(void (*object), void (*function) (void *));
    /**
     * Removes Callback
     */
    void RemoveCallback();
    /**
     * Constructor.
     * Defaults are unmuted, enabled and volume is set to 1.0f.
     */ 
    SoundControl ();
    /**
     * Destructor
     */
    ~SoundControl ();
    /**
     * Returns current Volume as float.
     */
    float GetVolume ();
    /**
     * Sets volume to the given float.
     * @param vol Volume as float
     */
    void SetVolume (float vol);
    /**
      * Unmute this.
      */
    void Unmute ();
    /**
     * Mute this
     */
    void Mute ();

    /**
     * Get current Toggle state.
     * Returns isEnabled 
     */
    bool GetToggle ();
    /**
     * Sets Toggle.
     * @param value true or false
     */           
    void SetToggle (bool value);
    /**
     * deactivates Toggle.
     * Sets isEnabled to false
     */
    void DeactivateToggle ();
    /**
     * activates Toggle.
     * Sets isEnabled to true
     */
    void ActivateToggle (); 

    private:
    bool    isEnabled;                  ///< is it enabled? true or false
    float   volume;                     ///< current volume as float
    bool    isMuted;                    ///< is it muted? true or false

    void (*callbackobject);             ///< pointer to a callback object (if set)
    void (*callbackfunction) (void *);  ///< pointer to a callback function (if set)
    bool hasCallback;                   ///< true if theres a Callback set, false if not

    /**
     * Will call the Callback if set.
     * Checks @see hasCallback and call the Callback if true. 
     */
    void Callback ();

    /**
     * Returns this Volume ID.
     * Not used atm
     */
    int GetID ();
};

#endif /*_SOUND_CONTROL_H_*/
