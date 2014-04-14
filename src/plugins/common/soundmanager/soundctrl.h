/*
* soundctrl.h
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


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/set.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <isoundctrl.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class SoundControl;


/**
 * Interface to implement to handle SoundControl's events.
 */
struct iSoundControlListener
{
    /**
     * This function is called everytime the volume or the toggle change.
     * @param sndCtrl the SoundControl that has been changed.
     */
    virtual void OnSoundChange(SoundControl* sndCtrl) = 0;
};


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

class SoundControl: public iSoundControl
{
public:

    /**
    * Constructor.
    * Defaults are unmuted, enabled and volume is set to 1.0f.
    */ 
    SoundControl(int ID);

    /**
    * Destructor
    */
    virtual ~SoundControl();

    /**
    * Subscribe to SoundControl's events.
    * @param listener the subscriber.
    */ 
    void Subscribe(iSoundControlListener* listener);

    /**
    * Unsubscribe the listener.
    * @param listener the object to remove.
    */
    void Unsubscribe(iSoundControlListener* listener);

    /**
    * Returns this Volume ID.
    */
    virtual int GetID() const;

    /**
     * Dampen volume over time to configured value
     * @param damp This value is used to describe if the function should dampen or open the sound.
     * @return true when the volume has reached the dampened or full value.
     */
    virtual void VolumeDampening(float damp);

    /**
     * Retrieve if volume is dampened
     * @return true when the volume has reached the dampened or full value.
     */
    virtual bool IsDampened() const;

    /**
    * Returns current Volume as float.
    */
    virtual float GetVolume() const;

    /**
    * Sets volume to the given float.
    * @param vol Volume as float
    */
    virtual void SetVolume(float vol);

    /**
    * Unmute this.
    */
    virtual void Unmute();

    /**
    * Mute this
    */
    virtual void Mute();

    /**
    * Get current Toggle state.
    * Returns isEnabled 
    */
    virtual bool GetToggle() const;

    /**
    * Sets Toggle.
    * @param value true or false
    */           
    virtual void SetToggle(bool value);

    /**
    * deactivates Toggle.
    * Sets isEnabled to false
    */
    virtual void DeactivateToggle();

    /**
    * activates Toggle.
    * Sets isEnabled to true
    */
    virtual void ActivateToggle(); 

private:
    int     id;                         ///< id of this control
    bool    isEnabled;                  ///< is it enabled? true or false
    bool    isMuted;                    ///< is it muted? true or false
    bool    isDampened;                 ///< is it enabled? true or false
    float   volume;                     ///< current volume as float
    float   volumeDamp;            		///< current dampening of volume as float

    csSet<iSoundControlListener*> listeners;   ///< listeners to SoundControl events.

    /**
    * Call OnSoundChange() on listeners.
    */
    void CallListeners();

};

#endif /*_SOUND_CONTROL_H_*/
