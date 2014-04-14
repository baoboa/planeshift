/*
 * psemitter.h
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
 

#ifndef _PSEMITTER_H_
#define _PSEMITTER_H_

/**
 * This object represents a planeshift soundEmitter.
 * Everything is in this object public and you can modify it at any time.
 * Make sure that you call Stop and Play after doing that.
 * Keep in mind that you have to be carefull when changing the SoundHandle
 * and its "active" status.
 */

class psEmitter
{
public:
    csString       resource;            ///< name of the resource
    float          minvol;              ///< vol at maxrange
    float          maxvol;              ///< vol at minrange
    int            fadedelay;           ///< how long fading should take in milliseconds
    float          minrange;            ///< range when it should reach maxvol
    float          maxrange;            ///< range when it should have minvol
    float          probability;         ///< probability that this emitter play the sound
    csString       factory;             ///< name of the factory this emitter should attach to
    float          factory_prob;        ///< probability that this emitter attaches to the factory
    bool           loop;                ///< true if the sound if the sound has to loop
    csVector3      position;            ///< position of the emitter
    csVector3      direction;           ///< direction were emitting to
    bool           active;              ///< is this emitter active?
    bool           dopplerEffect;       ///< true if the doppler effect must be applied for this emitter
    int            timeofday;           ///< time when this emitter starts playing
    int            timeofdayrange;      ///< time when this emitter stops
    SoundHandle*   handle;              ///< @see SoundHandle if it is active 
    
    /**
     * Constructor
     */
    psEmitter();
    /**
     * Destructor
     * Removes our Callback from the Handle if its still playing.
     */
    ~psEmitter();
    /**
     * Copy Constructor
     * copies resource volume range and time parameters
     * everything else remains uninitialized! 
     * @param emitter a existing emitter
     */
    psEmitter(psEmitter* const &emitter);
    /**
     * Check range to the given position.
     * Calculates the distance to the given position and returns 
     * true if this emitter is in range.
     * @param listenerPos position used for calculation
     */
    bool CheckRange(csVector3 listenerPos);
    /**
     * Check time of day.
     * Checks if time is within this emitters timewindow.
     * Returns true if it is.
     * @param time <24 && >0 is resonable but can be any valid int
     */
    bool CheckTimeOfDay(int time);
    /**
     * Play this emitter.
     * Makes this emitter play - you need to provide a SoundControl.
     * @param ctrl SoundControl for this emitter
     */
    bool Play(SoundControl* &ctrl);
    /**
     * Stops this emitter.
     */
    void Stop();
    /**
     * Callback function for Stop.
     * SoundHandles callback will point to this and inform us if it gets destroyed.
     * It sets active to false and Handle to NULL
     */
    static void StopCallback(void* object);
};

#endif /*_PSEMITTER_H_*/
