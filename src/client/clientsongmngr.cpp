/*
 * clientsongmngr.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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


#include <psconfig.h>
#include "clientsongmngr.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iengine/mesh.h>
#include <iengine/movable.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <music/musicutil.h>
#include <gui/pawsmusicwindow.h>
#include <paws/pawsmanager.h>
#include <net/clientmsghandler.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "globals.h"
#include "pscelclient.h"


ClientSongManager::ClientSongManager()
{
    mainSongID = NO_SONG;

    // Subscribing
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_PLAY_SONG);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_STOP_SONG);
}

ClientSongManager::~ClientSongManager()
{
    // Unsubscribing
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_PLAY_SONG);
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_STOP_SONG);
}

void ClientSongManager::PlayMainPlayerSong(uint32_t itemID, const csString &musicalSheet)
{
    // updating state
    sheet = musicalSheet;
    mainSongID = (uint)PENDING;

    // request to server
    psMusicalSheetMessage musicalSheetMessage(0, itemID, true, true, "", sheet);
    musicalSheetMessage.SendMessage();
}

void ClientSongManager::StopMainPlayerSong(bool notifyServer)
{
    // sending message
    if(notifyServer)
    {
        psStopSongMessage stopMessage;
        stopMessage.SendMessage();
    }

    // stopping song
    StopSong(mainSongID);
    mainSongID = NO_SONG;
    sheet.Empty();

    // updating listeners
    TriggerListeners();
}

void ClientSongManager::Update()
{
    uint soundID;                       // song ID for the sound manager
    uint32 songID;                      // song ID for the server
    csArray<uint32> songsToDelete;      // ended songs

    iSoundManager* sndMngr = psengine->GetSoundManager();

    // checking main player song
    if(mainSongID != NO_SONG
       && mainSongID != (uint)PENDING
       && !sndMngr->IsSoundValid(mainSongID))
    {
        StopMainPlayerSong(false);
    }

    // detecting ended songs
    csHash<uint, uint32>::GlobalIterator songIter(songMap.GetIterator());
    while(songIter.HasNext())
    {
        soundID = songIter.Next(songID);

        if(!sndMngr->IsSoundValid(soundID))
        {
            songsToDelete.Push(songID);
            StopSong(soundID);
        }
    }

    // deleting ended songs
    for(size_t i = 0; i < songsToDelete.GetSize(); i++)
    {
        songMap.DeleteAll(songsToDelete[i]);
    }
}

void ClientSongManager::Subscribe(iSongManagerListener* listener)
{
    listeners.PushSmart(listener);
}

void ClientSongManager::Unsubscribe(iSongManagerListener* listener)
{
    listeners.Delete(listener);
}

void ClientSongManager::HandleMessage(MsgEntry* message)
{
    uint8_t msgType = message->GetType();

    // Playing
    if(msgType == MSGTYPE_PLAY_SONG)
    {
        uint songHandleID;
        csVector3 playerPos;
        iSoundManager* sndMngr;

        psPlaySongMessage playMsg(message);

        // getting player's position
        playerPos = psengine->GetCelClient()->FindObject(playMsg.songID)->GetMesh()->GetMovable()->GetFullPosition();

        // if sounds are not active the song will still be heard by players around
        sndMngr = psengine->GetSoundManager();
        if(sndMngr->IsSoundActive(iSoundManager::INSTRUMENT_SNDCTRL))
        {
            // playing
            if(playMsg.toPlayer)
            {
                songHandleID = PlaySong(sheet, playMsg.instrName, playerPos);
            }
            else
            {
                // decompressing score
                csString uncompressedScore;
                psMusic::ZDecompressSong(playMsg.musicalScore, uncompressedScore);

                songHandleID = PlaySong(uncompressedScore, playMsg.instrName, playerPos);
            }

            // handling instrument not defined
            if(songHandleID == 0)
            {
                // stopping song, informing server and player
                if(playMsg.toPlayer)
                {
                    // noticing server
                    StopMainPlayerSong(true);

                    // noticing user
                    psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("You cannot play this song!"));
                    msg.FireEvent();
                }

                return;
            }

            // saving song ID
            if(playMsg.toPlayer)
            {
                mainSongID = songHandleID;
            }
            else
            {
                songMap.Put(playMsg.songID, songHandleID);
            }
        }
        else
        {
            mainSongID = NO_SONG;
        }
    }

    // Stopping
    else if(msgType == MSGTYPE_STOP_SONG)
    {
        psStopSongMessage stopMsg(message);

        if(stopMsg.toPlayer)
        {
            csString errorStr;

            if(mainSongID == (uint)PENDING) // no instrument equipped, invalid MusicXML or low skill
            {
                // updating mainSongId
                mainSongID = NO_SONG;

                // updating listeners
                TriggerListeners();
            }
            else if(mainSongID == NO_SONG) // sound are deactivated or song has ended
            {
                TriggerListeners();
            }
            else // player's mode has changed
            {
                StopMainPlayerSong(false);
            }

            // noticing user
            switch(stopMsg.errorCode)
            {
            case psStopSongMessage::ILLEGAL_SCORE:
                errorStr = "Illegal musical score!";
                break;
            case psStopSongMessage::NO_INSTRUMENT:
                errorStr = "You do not have an equipped musical instrument!";
                break;
            }

            if(!errorStr.IsEmpty())
            {
                psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate(errorStr));
                msg.FireEvent();
            }
        }
        else if(songMap.Contains(stopMsg.songID))
        {
            StopSong(songMap.Get(stopMsg.songID, 0));
            songMap.DeleteAll(stopMsg.songID);
        }
    }
}

uint ClientSongManager::PlaySong(const char* musicalSheet, const char* instrName, csVector3 playerPos)
{
    csRef<iDocument> sheetDoc;
    csRef<iDocumentSystem> docSys;
    iSoundManager* sndMngr = psengine->GetSoundManager();

    // creating document
    docSys = csQueryRegistry<iDocumentSystem>(psengine->GetObjectRegistry());
    sheetDoc = docSys->CreateDocument();
    sheetDoc->Parse(musicalSheet, true);

    return sndMngr->PlaySong(sheetDoc, instrName, iSoundManager::INSTRUMENT_SNDCTRL, playerPos, 0);
}

void ClientSongManager::StopSong(uint songID)
{
    psengine->GetSoundManager()->StopSound(songID);
}

void ClientSongManager::TriggerListeners()
{
    for(size_t i = 0; i < listeners.GetSize(); i++)
    {
        listeners[i]->OnMainPlayerSongStop();
    }
}
