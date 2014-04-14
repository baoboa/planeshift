/*
* dummysndmngr.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef _DUMMYSNDMNGR_H_
#define _DUMMYSNDMNGR_H_

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iutil/comp.h>
#include <csgeom/vector3.h>
#include <iutil/document.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <isoundmngr.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
struct iObjectRegistry;
class DummySoundControl;

/**
 * This is a dummy implementation of iSoundManager.
 * @see iSoundManager
 */
class DummySoundManager: public scfImplementation2<DummySoundManager, iSoundManager, iComponent>
{
public:
    DummySoundManager(iBase* parent);
    virtual ~DummySoundManager();

    //From iComponent
    virtual bool Initialize(iObjectRegistry* objReg);

    //From iSoundManager
    //Sectors managing
    virtual bool InitializeSectors();
    virtual bool LoadActiveSector(const char* sectorName);
    virtual bool ReloadSectors();

    //SoundControls and SoundQueue managing
    virtual iSoundControl* GetSndCtrl(SndCtrlID sndCtrlID);
    virtual bool AddSndQueue(int queueID, SndCtrlID sndCtrlID);
    virtual void RemoveSndQueue(int queueID);
    virtual bool PushQueueItem(int queueID, const char* fileName);

    //State
    virtual bool IsSoundActive(SndCtrlID sndCtrlID);
    virtual void SetCombatStance(int newCombatStance);
    virtual int GetCombatStance() const;
    virtual void SetPlayerMovement(csVector3 playerPosition, csVector3 playerVelocity);
    virtual csVector3 GetPosition() const;
    virtual void SetTimeOfDay(int newTimeOfDay);
    virtual void SetWeather(int newWeather);
    virtual void SetEntityState(int state, iMeshWrapper* mesh, const char* meshName, bool forceChange);
    virtual void AddObjectEntity(iMeshWrapper* mesh, const char* meshName);
    virtual void RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName);
    virtual void UpdateObjectEntity(iMeshWrapper* mesh, const char* meshName);

    //Toggles
    virtual void SetLoopBGMToggle(bool toggle);
    virtual bool IsLoopBGMToggleOn();
    virtual void SetCombatMusicToggle(bool toggle);
    virtual bool IsCombatMusicToggleOn();
    virtual void SetListenerOnCameraToggle(bool toggle);
    virtual bool IsListenerOnCameraToggleOn();
    virtual void SetChatToggle(bool toggle);
    virtual bool IsChatToggleOn();

    //Play sounds
    virtual bool IsSoundValid(uint soundID) const;
    virtual uint PlaySound(const char* fileName, bool loop, SndCtrlID sndCtrlID);
    virtual uint PlaySound(const char* fileName, bool loop, SndCtrlID sndCtrlID, csVector3 pos, csVector3 dir, float minDist, float maxDist);
    virtual uint PlaySong(csRef<iDocument> musicalSheet, const char* instrument,
        SndCtrlID sndCtrlID, csVector3 pos, csVector3 dir);
    virtual bool StopSound(uint soundID);
    virtual bool SetSoundSource(uint soundID, csVector3 position);

    //Updating function
    virtual void Update();
    virtual void UpdateListener(iView* view);
    

private:
    DummySoundControl* defaultSndCtrl;    ///< default sound control to return
    csVector3          position;          ///< current playerposition
    int                combat;            ///< current stance
    bool               loopToggle;        ///< loobBGM toggle
    bool               combatToggle;      ///< toggle for combatmusic
    bool               listenerToggle;    ///< toggle for listener switch between player and camera position
    bool               chatToggle;        ///< toggle for chatsounds
};    

#endif // __DUMMYSNDMNGR_H_
