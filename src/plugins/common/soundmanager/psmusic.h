/*
 * psmusic.h
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
 
#ifndef _PSMUSIC_H_
#define _PSMUSIC_H_

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class SoundHandle;


/**
 * This object represents a planeshift Soundtrack.
 * It maybe an ambient or a real soundtrack they are very similiar.
 * it has methods to perform the most common operations.
 */

class psMusic
{
public:
    csString       resource;            ///< sound resource
    int            type;                ///< type can be anything i use it for weather checks and similar thhings
    float          minvol;              ///< volume when fading in
    float          maxvol;              ///< volume this track is playing at
    int            fadedelay;           ///< number of milliseconds fading should use
    int            timeofday;           ///< time when this music starts playing
    int            timeofdayrange;      ///< time till this music is playing
    size_t         loopstart;           ///< frame to start at when looping
    size_t         loopend;             ///< frame when jumping back to loopstart (when looping)
    bool           active;              ///< is this soundtrack active?
    SoundHandle*   handle;              ///< handle if this soundtrack is active
    
    /**
     * Constructor
     * Sets active to false and handle to NULL
     */
    psMusic();
    /**
     * Destructor
     * Removes Handle callback if theres one
     */    
    ~psMusic();
    /**
     * Check time of day.
     * Checks if time is within this soundtracks timewindow.
     * Returns true if it is.
     * @param time <24 && >0 is resonable but can be any valid int
     */
    bool CheckTimeOfDay(int time);
    /**
     * Compares soundstracks type against a given type.
     * Returns true if they are both equal
     * @param _type type to compare against
     */
    bool CheckType(const int _type);
    /**
     * Fades this Soundtrack down and stops it.
     */
    void FadeDownAndStop();
    /**
     * Fades this Soundtrack down.
     */
    void FadeDown();
    /**
     * Fades this Soundtrack up.
     */
    void FadeUp();
    /**
     * Play this SoundTrack
     * @param loopToggle loop? maybe true or false
     * @param ctrl SoundControl for this sound
     */
    bool Play(bool loopToggle, SoundControl* &ctrl);
    /**
     * Stops this Soundtrack immediatly
     */
    void Stop();
    /**
     * Sets Autoremove to true.
     * The SoundHandle will be removed when this sound has stopped playing.
     * Warning: doesnt happen when looping!
     */
    void SetUnManaged();
    /**
     * Set Autoremove to false.
     * The SoundHandle wont be removed even if paused.
     */
    void SetManaged();
    /**
     * Enable or resume looping.
     */
    void Loop();
    /**
     * Disable looping.
     */
    void DontLoop();
    /**
     * Updates Handles Callback.
     */
    void UpdateHandleCallback();
    /**
     * Callback function for Stop.
     * SoundHandles callback will point to this and inform us if it gets destroyed.
     * It sets active to false and Handle to NULL
     */
    static void StopCallback(void* object);
};

#endif /*_PSMUSIC_H_*/
