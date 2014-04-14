/*
* dummysndctrl.h, Author: Andrea Rizzi <88whacko@gmail.com>
*
* Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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


#ifndef _DUMMYSNDCTRL_H_
#define _DUMMYSNDCTRL_H_

//====================================================================================
// Project Includes
//====================================================================================
#include <isoundctrl.h>


/**
* This is just a dummy implementation of iSoundControl.
*/

class DummySoundControl: public iSoundControl
{
public:

    DummySoundControl(int ID);

    virtual ~DummySoundControl();

    virtual int GetID() const;

    virtual void VolumeDampening(float damp);

    virtual bool IsDampened() const;

    virtual float GetVolume() const;

    virtual void SetVolume(float vol);

    virtual void Unmute();

    virtual void Mute();

    virtual bool GetToggle() const;

    virtual void SetToggle(bool value);

    virtual void DeactivateToggle();

    virtual void ActivateToggle();

private:
    int id;         ///< the SoundControl's id
    float volume;   ///< the SoundControl's volume
    bool isEnabled; ///< the SoundControl's toggle
    bool dampening; ///< the dampening status

};

#endif /*_DUMMYSNDCTRL_H_*/
