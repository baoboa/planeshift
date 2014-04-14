/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef EEDIT_TOOLBOX_HEADER
#define EEDIT_TOOLBOX_HEADER


/** A base class for all eedit toolbox windows.
 */
class EEditToolbox
{
public:
    enum TOOLBOX_TYPE
    {
        T_POSITION = 0,
        T_TARGET,
        T_CAMERA,
        T_RENDER,
        T_LOAD_EFFECT,
        T_EDIT_EFFECT,
        T_LOAD_MAP,
        T_ERROR,
        T_FPS,
        T_SHORTCUTS,
	T_PARTICLES,

        T_COUNT
    };
    
    EEditToolbox();
    virtual ~EEditToolbox();

    /** Updates the toobox.
     *   @param elapsed the time elapsed in milliseconds.
     */
    virtual void Update(unsigned int elapsed);

    /** Gets the toolbox type.
     *   @return the toolbox type
     */
    virtual size_t GetType() const = 0;

    /** Gets the name of the toolbox.
     *   @return the toolbox name
     */
    virtual const char * GetName() const = 0;
    
private:

};

#endif 
