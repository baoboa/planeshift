/*
 * instrumentmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef INSTRMNGR_H
#define INSTRMNGR_H


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/ref.h>
#include <csutil/hash.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class csVector3;
class Instrument;
class SoundHandle;
class SoundControl;
struct iDocument;
struct iObjectRegistry;

/**
 * This class keeps and manage the musical instruments.
 */
class InstrumentManager
{
public:

    /**
     * Constructor. Creates an empty InstrumentManager.
     */
    InstrumentManager();

    /**
     * Destructor. Destroys all instruments.
     */
    ~InstrumentManager();

    /**
     * Reads the instruments definitions.
     * @param objectReg the object registry.
     * @return false if SoundSystemManager has not been correctly initialized or
     * if the instruments definitions are not syntactically correct.
     */
    bool Initialize(iObjectRegistry* objectReg);

    /**
     * Play the song in the given musical sheet with the given instruments.
     *
     * @param sndCtrl the SoundControl for this song.
     * @param pos the position of the player to play this song.
     * @param dir the direction of this sound.
     * @param songHandle an null SongHandle that will handle the song.
     * @param musicalSheet the XML musical sheet to play.
     * @param instrName the name of the instruments.
     * @return true if the song could be played, false otherwise.
     */
    bool PlaySong(SoundControl* sndCtrl, csVector3 pos, csVector3 dir, SoundHandle* &songHandle,
                csRef<iDocument> musicalSheet, const char* instrName);

private:
    csHash<Instrument*, csString> instruments;
};

#endif /* INSTRMNGR_H */