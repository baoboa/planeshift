/*
 * songhandle.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef SONGHANDLE_H
#define SONGHANDLE_H

//====================================================================================
// Local Includes
//====================================================================================
#include "handle.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class Instrument;


class SongHandle: public SoundHandle
{
public:
    /**
     * Constructor.
     * @param musicalSheet the XML musical sheet.
     * @param instrument the instrument that the player use to play the song.
     */
    SongHandle(csRef<iDocument> musicalSheet, Instrument* instrument);

    /**
     * Destructor.
     */
    virtual ~SongHandle();


    // From SoundHandle
    //------------------

    virtual bool Init(const char* resname, bool loop, float volume_preset,
              int type, SoundControl* &ctrl, bool dopplerEffect);

private:
    Instrument* instrument;         ///< instrument of the song
    csRef<iDocument> sheet;         ///< musical sheet of the song
};

#endif /* SONGHANDLE_H */
