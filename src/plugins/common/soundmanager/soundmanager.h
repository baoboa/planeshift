/*
* soundmanager.h, Author: Andrea Rizzi <88whacko@gmail.com>
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

#ifndef _SOUNDMANAGER_H_
#define _SOUNDMANAGER_H_

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iutil/comp.h>
#include <iutil/eventh.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <isoundmngr.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "soundctrl.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class SoundQueue;
class csRandomGen;
class InstrumentManager;
class SoundSectorManager;
class SoundSystemManager;


/**
 * Implement iSoundManager.
 * @see iSoundManager
 */
class SoundManager: public scfImplementation3<SoundManager, iSoundManager, iComponent, iEventHandler>
{
public:
    // TODO this should be moved inside a ConfigManager
    static uint updateTime;                 ///< update throttle in milliseconds.
    static csRandomGen randomGen;           ///< random number generator.

    SoundManager(iBase* parent);
    virtual ~SoundManager();

    //From iComponent
    virtual bool Initialize(iObjectRegistry* objReg);

    //From iEventHandler
    virtual bool HandleEvent(iEvent &e);
    CS_EVENTHANDLER_NAMES("crystalspace.planeshift.sound")
    virtual const csHandlerID* GenericPrec(csRef<iEventHandlerRegistry> &ehr,
            csRef<iEventNameRegistry> &enr, csEventID id) const;
    virtual const csHandlerID* GenericSucc(csRef<iEventHandlerRegistry> &ehr,
            csRef<iEventNameRegistry> &enr, csEventID id) const { return 0; }
    CS_EVENTHANDLER_DEFAULT_INSTANCE_CONSTRAINTS

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
    csRef<iObjectRegistry>      objectReg;         ///< object registry
    SoundSystemManager*         sndSysMgr;         ///< the sound system manager used to play sounds
    InstrumentManager*          instrMgr;          ///< the instruments manager
    SoundSectorManager*         sectorMgr;         ///< the sound sectors manager.

    csHash<SoundQueue*, int>    soundQueues;       ///< all the SoundQueues created by ID
    bool                        listenerOnCamera;  ///< toggle for listener switch between player and camera position.
    bool                        chatToggle;        ///< toggle for chat sounds.

    csTicks                     sndTime;           ///< current csticks
    csTicks                     lastUpdateTime;    ///< csticks when the last update happend

    float						volumeDampPercent; ///< configured percent of dampening.
    csArray<SndCtrlID>          dampenCtrls;       ///< The controls to dampened.

    csEventID                   evSystemOpen;      ///< ID of the 'Open' event fired on system startup

    /**
     * Initialize the SoundSystemManager and components that depends on its initialization.
     * Since the sound system listener is initialized on system open event, this function
     * must be called after that.
     */
    void InitSoundSystem();

};    

#endif // __SOUNDMANAGER_H__
