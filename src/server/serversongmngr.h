/*
 * serversongmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef SERVER_SONG_MANAGER_H
#define SERVER_SONG_MANAGER_H


//====================================================================================
// Project Includes
//====================================================================================
#include <util/gameevent.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "msgmanager.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class psItem;
class MathScript;
class psCharacter;
struct ScoreStatistics;


/**
 * This event informs the song manager when the song is over.
 */
class psEndSongEvent: public psGameEvent
{
public:
    /**
     * Constructor.
     * @param charActor the player that plays this song.
     * @param songLength the length of the song in milliseconds.
     */
    psEndSongEvent(gemActor* charActor, int songLength);

    /**
     * Destructor.
     */
    virtual ~psEndSongEvent();


    //From psGameEvent
    //------------------
    virtual bool CheckTrigger();
    virtual void Trigger();

private:
    gemActor* charActor;        ///< The player that plays this song.
    csTicks startingTime;       ///< The time when the song started.
};


//--------------------------------------------------


/**
 * This class takes care of played songs and players' ranking for musical instruments
 * skills.
 */
class ServerSongManager: public MessageManager<ServerSongManager>, public Singleton<ServerSongManager>
{
public:
    ServerSongManager();    ///< Constructor.
    ~ServerSongManager();   ///< Destructor.

    bool Initialize();

    /**
     * Handles a play song request.
     * @param me the message from the client.
     * @param client the sender.
     */
    void HandlePlaySongMessage(MsgEntry* me, Client* client);

    /**
     * Handles a stop song request.
     * @param me the message from the client.
     * @param client the sender.
     */
    void HandleStopSongMessage(MsgEntry* me, Client* client);

    /**
     * Stop the song and (contrary to StopSong) update the player's mode. This is the
     * function called when the player stop manually the song or when it ends.
     * @param charActor the gemActor of the player.
     * @param isEnded true if the song has is ended, false otherwise.
     */
    void OnStopSong(gemActor* charActor, bool isEnded);

    /**
     * Takes care of the skill ranking and of sending the stop song message to the player's
     * proximity list.
     *
     * @param charActor the player that has just stopped to play his instrument.
     * @param skillRanking true to make the player gain practice points, false otherwise.
     * @note This function is called by a gemActor when it exits from the PLAY mode thus
     * StopSong does not take care of updating the player's status.
     */
    void StopSong(gemActor* charActor, bool skillRanking);

private:
    bool isProcessedSongEnded;                         ///< Flag used to keep track of the last song request. Remember to reset to false each time.
    MathScript* calcSongPar;                 ///< Keeps the script that computes the song parameters.
    MathScript* calcSongExp;                 ///< Keeps the script that computes the experience gained.
    unsigned int instrumentsCategory;                  ///< Keeps the instruments' category from server_options table.
    csHash<int, unsigned int> scoreRanks;              ///< Keeps the scores' ranks that are being played.

    /**
     * Gets the instrument currently equipped by the player.
     * @param charData the player's data.
     * @return the pointer to the instrument or null if no instrument is equipped.
     */
    psItem* GetEquippedInstrument(psCharacter* charData) const;
};

#endif // SERVER_SONG_MANAGER_H

