/*
 * clientsongmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef CLIENT_SONG_MANAGER_H
#define CLIENT_SONG_MANAGER_H

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <csutil/ref.h>
#include <csutil/hash.h>
#include <iutil/document.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <net/cmdbase.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class MsgEntry;
class csString;
class csVector3;


/**
 * This interface implemets a listener to client song events.
 */
class iSongManagerListener
{
public:
    /**
     * This is called when the main player's song is stopped.
     */
    virtual void OnMainPlayerSongStop() = 0;
};

/**
 * This class connect the GUI and the server side of the instruments system to the
 * sound plugin. It keeps and manage all the songs that are being played.
 */
class ClientSongManager: public psClientNetSubscriber
{
public:
    /**
     * Constructor.
     */
    ClientSongManager();

    /**
     * Destructor.
     */
    ~ClientSongManager();

    /**
     * Makes the main player play the song in the given musical sheet and inform
     * the server about it.
     *
     * @param itemID the ID of the item that contains the musical sheet.
     * @param musicalSheet the musical sheet into XML format.
     */
    void PlayMainPlayerSong(uint32_t itemID, const csString &musicalSheet);

    /**
     * Stops the song played by the main player.
     * @param notifyServer true if the server must be notified about the stop.
     */
    void StopMainPlayerSong(bool notifyServer);

    /**
     * This method handle the ended songs.
     */
    void Update();

    /**
     * Subscribe the given iSongManagerListener to this manager.
     * @param listener the listener.
     */
    void Subscribe(iSongManagerListener* listener);

    /**
     * Unsubscribe the given iSongManagerListener.
     * @param listener the listener.
     */
    void Unsubscribe(iSongManagerListener* listener);


    // From psClientNetSubscriber
    //----------------------------
    virtual void HandleMessage(MsgEntry* message);

private:
    /**
     * This enum contains values for mainSongID.
     */
    enum
    {
        PENDING = -1,
        NO_SONG = 0
    };

    uint mainSongID;                            ///< ID of the song of the main player. 0 if he is not playing.
    csString sheet;                             ///< The last played musical sheet.
    csHash<uint, uint32> songMap;               ///< Map from the server songs' IDs to the sound manager's ones.
    csArray<iSongManagerListener*> listeners;   ///< Listeners to this manager's events.

    /**
     * Plays the given song. It does not inform the server.
     *
     * @param musicalSheet the musical sheet into XML format.
     * @param instrName the name of the instrument used to play.
     * @param playerPos the position of the player.
     * @return the song handle's ID.
     */
    uint PlaySong(const char* musicalSheet, const char* instrName, csVector3 playerPos);

    /**
     * Stops the song with the given ID.
     * @param songID the ID of the song to stop.
     */
    void StopSong(uint songID);

    /**
     * Calls OnMainPlayerSongStop on all the listeners.
     */
    void TriggerListeners();
};

#endif // CLIENT_SONG_MANAGER_H