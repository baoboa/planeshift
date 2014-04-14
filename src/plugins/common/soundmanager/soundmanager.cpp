/*
* soundmanager.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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


//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>

#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/cfgmgr.h>
#include <iutil/eventq.h>
#include <iutil/event.h>

#include <csutil/eventnames.h>

//====================================================================================
// Project Includes
//====================================================================================
#include "util/psstring.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "soundmanager.h"

#include "queue.h"
#include "handle.h"
#include "manager.h"
#include "soundctrl.h"
#include "sectormngr.h"
#include "instrumentmngr.h"

#define DEFAULT_SECTOR_UPDATE_TIME 50
#define DEFAULT_DAMPENING_PERCENT 0.1f
#define DEFAULT_DAMPENING_CONTROLS "music"


SCF_IMPLEMENT_FACTORY(SoundManager)

csRandomGen SoundManager::randomGen;

uint SoundManager::updateTime = DEFAULT_SECTOR_UPDATE_TIME;

SoundManager::SoundManager(iBase* parent): scfImplementationType(this, parent)
{
    // sound system manager and sound controls can be initialized here
    // remember that the main SoundControl is initialized automatically
    sndSysMgr = new SoundSystemManager();
    for(uint id = MAIN_SNDCTRL + 1; id < COUNT_SNDCTRL; id++)
    {
        sndSysMgr->AddSoundControl(id);
    }
    AddSndQueue(VOICE_QUEUE, VOICE_SNDCTRL);

    // create managers so we don't have to deal with null pointers
    // remember that SoundSectorManager needs music and ambient sound
    // controls to be already initialized when created
    instrMgr = new InstrumentManager();
    sectorMgr = new SoundSectorManager();
}


SoundManager::~SoundManager()
{
    delete sectorMgr;
    delete instrMgr;
    delete sndSysMgr;
    // Note: sndSysMgr should take care of SoundControls .. we can ignore them

    // Deleting SoundQueues
    csHash<SoundQueue*, int>::GlobalIterator queueIter(soundQueues.GetIterator());
    SoundQueue* sq;

    while(queueIter.HasNext())
    {
        sq = queueIter.Next();
        delete sq;
    }

    soundQueues.DeleteAll();
}

//-----------------
// FROM iComponent 
//-----------------

bool SoundManager::Initialize(iObjectRegistry* objReg)
{
    const char* volumeDampControls;

    objectReg = objReg;

    lastUpdateTime = csGetTicks();

    // configuration
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        SoundManager::updateTime = configManager->GetInt("Planeshift.Sound.UpdateTime", DEFAULT_SECTOR_UPDATE_TIME);

        volumeDampPercent = configManager->GetFloat("PlaneShift.Sound.DampeningPercent", DEFAULT_DAMPENING_PERCENT);
        volumeDampControls = configManager->GetStr("PlaneShift.Sound.DampeningControls", DEFAULT_DAMPENING_CONTROLS);
    }
    else
    {
        volumeDampPercent = DEFAULT_DAMPENING_PERCENT;
        volumeDampControls = DEFAULT_DAMPENING_CONTROLS;
    }

	/*
	 * Prepare an array of the sound controls to dampened when voice is queued.
	 * The configuration string is delimited by | and each control get referenced
	 * by the word lowercase. eg. "music" for the music sound control.
	 */
    psString strDampControls(volumeDampControls);
    csStringArray dampControls;

    strDampControls.Split(dampControls,'|');

    if(strDampControls.Find("ambient") != csArrayItemNotFound)
    {
        dampenCtrls.Push(AMBIENT_SNDCTRL);
    }
    if(strDampControls.Find("music") != csArrayItemNotFound)
    {
        dampenCtrls.Push(MUSIC_SNDCTRL);
    }
    if(strDampControls.Find("action") != csArrayItemNotFound)
    {
        dampenCtrls.Push(ACTION_SNDCTRL);
    }
    if(strDampControls.Find("effect") != csArrayItemNotFound)
    {
        dampenCtrls.Push(EFFECT_SNDCTRL);
    }
    if(strDampControls.Find("gui") != csArrayItemNotFound)
    {
        dampenCtrls.Push(GUI_SNDCTRL);
    }
    if(strDampControls.Find("instrument") != csArrayItemNotFound)
    {
        dampenCtrls.Push(INSTRUMENT_SNDCTRL);
    }

    // Registering for event callbacks
    csRef<iEventQueue> queue = csQueryRegistry<iEventQueue>(objectReg);
    evSystemOpen = csevSystemOpen(objectReg);
    if (queue != 0)
    {
        queue->RegisterListener(this, evSystemOpen);
    }

    //TODO: NEED TO INITIALIZE ALL VARIABLES!
    return true;
}

//--------------------
// FROM iEventhandler 
//--------------------

/*
 * To get the listener (and so to initialize SoundSystemManager and
 * SoundSystem) iSndSysRenderer must receive the evSystemOpen.
 */
bool SoundManager::HandleEvent(iEvent &e)
{
    if (e.Name == evSystemOpen) 
    {
        InitSoundSystem();
    }

    return false;
}

const csHandlerID* SoundManager::GenericPrec(csRef<iEventHandlerRegistry> &ehr,
            csRef<iEventNameRegistry> &/*enr*/, csEventID id) const
{
    if(id == evSystemOpen)
    {
        static csHandlerID precConstraint[2]; // TODO not thread safe

        precConstraint[0] = ehr->GetGenericID("crystalspace.sndsys.renderer");
        precConstraint[1] = CS_HANDLERLIST_END;
        return precConstraint;
    }

    return 0;
}

//--------------------
// FROM iSoundManager 
//--------------------

bool SoundManager::InitializeSectors()
{
    // there's no reason to initialize sectors if the sound system
    // has not been initialized
    if(sndSysMgr->Initialised == false)
    {
        return false;
    }

    return sectorMgr->Initialize(objectReg);
}

bool SoundManager::LoadActiveSector(const char* sectorName)
{
    return sectorMgr->SetActiveSector(sectorName);
}

bool SoundManager::ReloadSectors()
{
    return sectorMgr->ReloadSectors();
}

iSoundControl* SoundManager::GetSndCtrl(SndCtrlID sndCtrlID)
{
    if(sndCtrlID == MAIN_SNDCTRL)
    {
        return sndSysMgr->mainSndCtrl;
    }
    else
    {
        return sndSysMgr->GetSoundControl(sndCtrlID);
    }
}

bool SoundManager::AddSndQueue(int queueID, SndCtrlID sndCtrlID)
{
    SoundQueue* sndQueue;
    SoundControl* sndCtrl;

    // checking if a queue with the same ID already exists
    if(soundQueues.Get(queueID, 0) != 0)
    {
        return false;
    }

    // we don't want to play a sound with the main sound control
    sndCtrl = sndSysMgr->GetSoundControl(sndCtrlID);
    if(sndCtrl == 0)
    {
        return false;
    }

    sndQueue = new SoundQueue(sndCtrl, VOLUME_NORM);
    soundQueues.Put(queueID, sndQueue);

    return true;
}


void SoundManager::RemoveSndQueue(int queueID)
{
    soundQueues.DeleteAll(queueID);
    return;
}


bool SoundManager::PushQueueItem(int queueID, const char* fileName)
{
    SoundQueue* sq = soundQueues.Get(queueID, 0);

    if(sq == 0)
    {
        return false;
    }

    sq->AddItem(fileName);
    return true;
}


bool SoundManager::IsSoundActive(SndCtrlID sndCtrlID)
{
    SoundControl* sndCtrl;

    if(sndSysMgr == 0)
    {
        return false;
    }

    sndCtrl = static_cast<SoundControl*>(GetSndCtrl(sndCtrlID));
    if(sndCtrl == 0)
    {
        return false;
    }

    return sndCtrl->GetToggle() && sndSysMgr->mainSndCtrl->GetToggle() && sndSysMgr->Initialised;
}


void SoundManager::SetCombatStance(int newCombatStance)
{
    sectorMgr->SetCombatStance(newCombatStance);
}

/*
* FIXME Remove GetCombatStance when you fix the victory effect
*/
int SoundManager::GetCombatStance() const
{
    return sectorMgr->GetCombatStance();
}


void SoundManager::SetPlayerMovement(csVector3 playerPos, csVector3 playerVelocity)
{
    sndSysMgr->SetPlayerVelocity(playerVelocity);
    sndSysMgr->SetPlayerPosition(playerPos);
}


csVector3 SoundManager::GetPosition() const
{
    return sndSysMgr->GetPlayerPosition();
}

/*
* TODO Set functions which trigger updates
*/
void SoundManager::SetTimeOfDay(int newTimeOfDay)
{
    sectorMgr->SetTimeOfDay(newTimeOfDay);
}

void SoundManager::SetWeather(int newWeather)
{
    sectorMgr->SetWeather(newWeather);
}

void SoundManager::SetEntityState(int state, iMeshWrapper* mesh, const char* meshName, bool forceChange)
{
    sectorMgr->SetEntityState(state, mesh, meshName, forceChange);
}

void SoundManager::AddObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    sectorMgr->AddObjectEntity(mesh, meshName);
}

void SoundManager::RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    sectorMgr->RemoveObjectEntity(mesh, meshName);
}

void SoundManager::UpdateObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    sectorMgr->UpdateObjectEntity(mesh, meshName);
}

void SoundManager::SetLoopBGMToggle(bool toggle)
{
    sectorMgr->SetLoopBGMToggle(toggle);
}

bool SoundManager::IsLoopBGMToggleOn()
{
    return sectorMgr->IsLoopBGMOn();
}

void SoundManager::SetCombatMusicToggle(bool toggle)
{
    sectorMgr->SetCombatMusicToggle(toggle);
}

bool SoundManager::IsCombatMusicToggleOn()
{
    return sectorMgr->IsCombatMusicOn();
}


void SoundManager::SetListenerOnCameraToggle(bool toggle)
{
    listenerOnCamera = toggle;
}


bool SoundManager::IsListenerOnCameraToggleOn()
{
    return listenerOnCamera;
}


void SoundManager::SetChatToggle(bool toggle)
{
    chatToggle = toggle;
}


bool SoundManager::IsChatToggleOn()
{
    return chatToggle;
}

bool SoundManager::IsSoundValid(uint soundID) const
{
    return sndSysMgr->IsHandleValid(soundID);
}

uint SoundManager::PlaySound(const char* fileName, bool loop, SndCtrlID sndCtrlID)
{
    SoundHandle* handle = 0;

    // we don't want to play a sound with the main sound control
    SoundControl* sndCtrl = sndSysMgr->GetSoundControl(sndCtrlID);

    if(sndSysMgr->Play2DSound(fileName, loop, 0, 0, VOLUME_NORM,
        sndCtrl, handle))
    {
        return handle->GetID();
    }
    else
    {
        return 0;
    }
}


uint SoundManager::PlaySound(const char* fileName, bool loop, SndCtrlID sndCtrlID, csVector3 pos, csVector3 dir, float minDist, float maxDist)
{
    SoundHandle* handle = 0;

    // we don't want to play a sound with the main sound control
    SoundControl* sndCtrl = sndSysMgr->GetSoundControl(sndCtrlID);

    if(sndSysMgr->Play3DSound(fileName, loop, 0, 0, VOLUME_NORM,
        sndCtrl, pos, dir, minDist, maxDist,
        0, CS_SND3D_ABSOLUTE, handle))
    {
        return handle->GetID();
    }
    else
    {
        return 0;
    }
}

uint SoundManager::PlaySong(csRef<iDocument> musicalSheet, const char* instrument,
              SndCtrlID sndCtrlID, csVector3 pos, csVector3 dir)
{
    SoundHandle* handle = 0;

    // we don't want to play a sound with the main sound control
    SoundControl* sndCtrl = sndSysMgr->GetSoundControl(sndCtrlID);

    if(instrMgr->PlaySong(sndCtrl, pos, dir, handle, musicalSheet, instrument))
    {
        return handle->GetID();
    }
    else
    {
        return 0;
    }
}


bool SoundManager::StopSound(uint soundID)
{
    if(soundID == 0)
    {
        return false;
    }

    return sndSysMgr->StopSound(soundID);
}


bool SoundManager::SetSoundSource(uint soundID, csVector3 position)
{
    if(soundID == 0)
    {
        return false;
    }

    return sndSysMgr->SetSoundSource(soundID, position);
}

/*
* Updates things that cant be triggered by
* a event (yet). E.g. Check if theres a Voice to play (TODO: callback)
* or if there are Emitters or Entities in or out of range that
* need a update
*
* Im not sure if those can ever be event based
* sure we know that theres something moving
* but ten to twenty times per second is enough
* not sure how often it would be called if
* executed by engine
* 
* FIXME csticks
*
*/
void SoundManager::Update()
{

    sndTime = csGetTicks();

    // twenty times per second
    if(lastUpdateTime + SoundManager::updateTime <= sndTime)
    {
    	/*
    	 * Prepare an boolean with the value for if we are to dampen the volume or if we
    	 * want the full volume. This value is true when an voice is loaded or currently
    	 * playing.
    	 */
    	bool voiceLoaded = (soundQueues.Get(VOICE_QUEUE, 0)->GetSize() > 0);

    	/*
    	 * Start with a ready value true and check if the volume dampening is in the correct
    	 * mode. If a voice is loaded the volume should be dampened. If not it should be full.
    	 * The voice will not play until the change been made and the volume is at a level
    	 * that we are ready to present the speech to the player.
    	 * The damp variable will get the percent to dampen to or 100% when we go to full.
    	 */
    	bool ready = true;
        for (size_t i = 0; i< dampenCtrls.GetSize(); i++)
        {
    		if(GetSndCtrl(dampenCtrls[i])->IsDampened() == !voiceLoaded)
    		{
				ready = false;
				float damp = voiceLoaded ? volumeDampPercent : 1.0f;
				GetSndCtrl(dampenCtrls[i])->VolumeDampening(damp);
			}
		}



    	if(ready)
    	{
			// Making queues work
			csHash<SoundQueue*, int>::GlobalIterator queueIter(soundQueues.GetIterator());
			SoundQueue* sq;

			while(queueIter.HasNext())
			{
				sq = queueIter.Next();
				sq->Work();
			}

			// Updating sectors if needed
            sectorMgr->Update();
    	}
        lastUpdateTime = csGetTicks();
    }

    // dont forget to Update our SoundSystemManager
    // remember that it has a own throttle
    sndSysMgr->Update();
}

void SoundManager::InitSoundSystem()
{
    // Initializing sound system. Remember that InstrumentManager must be
    // initialized after SoundSystemManager or it won't define any instrument.
    // SoundSectorManager's initialization is optional and it's done by
    // InitializeSectors()
    sndSysMgr->Initialize(objectReg);
    instrMgr->Initialize(objectReg);
}

/*
* were always using camera height and rotation
* position can be players position or cameras position
* updated every frame? ..
*/
void SoundManager::UpdateListener(iView* view)
{
    csVector3 hearpoint;
    csMatrix3 matrix;
    csVector3 front;
    csVector3 top;

    /* TODO wrong way todo this */
    if(sndSysMgr->Initialised == false)
    {
        return;
    }

    if(!listenerOnCamera)
    {
        hearpoint = sndSysMgr->GetPlayerPosition();
    }
    else
    {
        // take position/direction from view->GetCamera()
        hearpoint = view->GetPerspectiveCamera()->GetCamera()
            ->GetTransform().GetOrigin();
    }

    matrix = view->GetPerspectiveCamera()->GetCamera()
        ->GetTransform().GetT2O();
    front = matrix.Col3();
    top   = matrix.Col2();

    sndSysMgr->UpdateListener(hearpoint, front, top);

}

