/*
* isoundctrl.h, Author: Andrea Rizzi <88whacko@gmail.com>
*
* Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#ifndef _ISOUNDCONTROL_H_
#define _ISOUNDCONTROL_H_


/** This interface defines the sound controller used by the application.
 *
 * The API allows the user to control the volume and the state of a group
 * of sounds. The user can define a different SoundControl for each type
 * of sound is needed to be controlled differently. For example one can
 * use a SoundControl to manage voices and another one for the GUI's sounds.
 *
 * @see iSoundManager creates SoundControls.
 */
struct iSoundControl
{
    /**
     * Get the SoundControl's ID.
     * @return the Soundcontrol's ID.
     */
    virtual int GetID() const = 0;

    /**
     * Dampen volume over time to configured value
     * @param damp This value is used to describe if the function should dampen or open the sound.
     */
    virtual void VolumeDampening(float damp) = 0;

    /**
     * Retrieve if volume is dampened
     * @return true when the volume has reached the dampened or full value.
     */
    virtual bool IsDampened() const = 0;

    /**
     * Get the current volume of the sounds controlled by this SoundControl.
     * @return the volume as a float.
     */
    virtual float GetVolume() const = 0;

    /**
     * Set the volume of the sounds controlled by this SoundControl.
     * @param vol the volume to be set.
     */
    virtual void SetVolume(float vol) = 0;

    /**
     * Unmute sounds controlled by this SoundControl.
     */
    virtual void Unmute() = 0;

    /**
     * Mute sounds controlled by this SoundControl.
     */
    virtual void Mute() = 0;

    /**
     * Get the current Toggle state.
     * @return true if sounds controlled by this SoundControl are activated,
     * false otherwise.
     */
    virtual bool GetToggle() const = 0;

    /**
     * Set the current Toggle state.
     * @param toggle true to activate the sounds controlled by this SoundControl,
     * false otherwise.
     */
    virtual void SetToggle(bool toggle) = 0;

    /**
     * Deactivate sounds controlled by this SoundControl.
     */
    virtual void DeactivateToggle() = 0;

    /**
     * Activate sounds controlled by this SoundControl.
     */
    virtual void ActivateToggle() = 0;
};

#endif // _ISOUNDCONTROL_H_
