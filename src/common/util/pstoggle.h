/*
 * pstoggle.h
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
 

#ifndef PSTOGGLE_H_
#define PSTOGGLE_H_

/**
 * \addtogroup common_util
 * @{ */

/**
 * Simple Toggle with callback functionality.
 * Calls the callback function on toggle state change.
 * Dont forget to remove the callback if you destroy the object it points to.
 */

class psToggle
{
    public:
    /**
     * Constructor - default state is true.
     */
    psToggle();
    /**
     * Destructor
     */
    ~psToggle();

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
    /**
     * Set Toggle.
     * @param newstate true or false
     */
    void SetToggle(bool newstate);
    /**
     * Get Toggle
     * returns state
     */
    bool GetToggle();
    /**
     * Set Toggle to true.
     */
    void Activate();
    /**
     * Set Toggle to false.
     */
    void Deactivate();

    private:
    bool        state;                      ///< contains togglestate
    void (*callbackobject);                 ///< callback object if set
    void (*callbackfunction) (void *);      ///< callback function if set
    bool hasCallback;                       ///< has a callback
    /**
     * Makes the callback if set.
     * Calls callbackfunction of callbackobject.
     */
    void Callback();
};

/** @} */

#endif /*PSTOGGLE_H_*/
