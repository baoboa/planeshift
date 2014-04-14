/*
* dummysndmngr.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <cssysdef.h>
#include <iutil/plugin.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "dummysndmngr.h"
#include "dummysndctrl.h"


SCF_IMPLEMENT_FACTORY(DummySoundManager)


DummySoundManager::DummySoundManager(iBase* parent): scfImplementationType(this, parent),combat(0),loopToggle(false),combatToggle(false),listenerToggle(false),chatToggle(false)
{
    defaultSndCtrl = new DummySoundControl(0);
    return;
}

DummySoundManager::~DummySoundManager()
{
    delete defaultSndCtrl;
    return;
}

bool DummySoundManager::Initialize(iObjectRegistry* objReg)
{
    return true;
}

bool DummySoundManager::InitializeSectors()
{
    return true;
}

bool DummySoundManager::LoadActiveSector(const char* sectorName)
{
    return true;
}

bool DummySoundManager::ReloadSectors()
{
    return true;
}

iSoundControl* DummySoundManager::GetSndCtrl(SndCtrlID sndCtrlID)
{
    return defaultSndCtrl;
}

bool DummySoundManager::AddSndQueue(int queueID, SndCtrlID sndCtrlID)
{
    return true;
}

void DummySoundManager::RemoveSndQueue(int queueID)
{
    return;
}

bool DummySoundManager::PushQueueItem(int queueID, const char* fileName)
{
    return true;
}

bool DummySoundManager::IsSoundActive(SndCtrlID sndCtrlID)
{
    return false;
}

void DummySoundManager::SetCombatStance(int newCombatStance)
{
    combat = newCombatStance;
}

int DummySoundManager::GetCombatStance() const
{
    return combat;
}

void DummySoundManager::SetPlayerMovement(csVector3 playerPos, csVector3 playerVelocity)
{
    position = playerPos;
}


csVector3 DummySoundManager::GetPosition() const
{
    return position;
}

void DummySoundManager::SetTimeOfDay(int newTimeOfDay)
{
    return;
}

void DummySoundManager::SetWeather(int newWeather)
{
    return;
}

void DummySoundManager::SetEntityState(int state, iMeshWrapper* mesh, const char* meshName, bool forceChange)
{
    return;
}

void DummySoundManager::AddObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    return;
}

void DummySoundManager::RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    return;
}

void DummySoundManager::UpdateObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    return;
}

void DummySoundManager::SetLoopBGMToggle(bool toggle)
{
    loopToggle = toggle;
}

bool DummySoundManager::IsLoopBGMToggleOn()
{
    return loopToggle;
}

void DummySoundManager::SetCombatMusicToggle(bool toggle)
{
    combatToggle = toggle;
}

bool DummySoundManager::IsCombatMusicToggleOn()
{
    return combatToggle;
}

void DummySoundManager::SetListenerOnCameraToggle(bool toggle)
{
    listenerToggle = toggle;
}

bool DummySoundManager::IsListenerOnCameraToggleOn()
{
    return listenerToggle;
}

void DummySoundManager::SetChatToggle(bool toggle)
{
    chatToggle = toggle;
}

bool DummySoundManager::IsChatToggleOn()
{
    return chatToggle;
}

bool DummySoundManager::IsSoundValid(uint soundID) const
{
    return false;
}

uint DummySoundManager::PlaySound(const char* fileName, bool loop, SndCtrlID sndCtrlID)
{
    return 0;
}

uint DummySoundManager::PlaySound(const char* fileName, bool loop, SndCtrlID sndCtrlID, csVector3 pos, csVector3 dir, float minDist, float maxDist)
{
    return 0;
}

uint DummySoundManager::PlaySong(csRef<iDocument> musicalSheet, const char* instrument,
        SndCtrlID sndCtrlID, csVector3 pos, csVector3 dir)
{
    return 0;
}

bool DummySoundManager::StopSound(uint soundID)
{
    return true;
}

bool DummySoundManager::SetSoundSource(uint soundID, csVector3 position)
{
    return true;
}

void DummySoundManager::Update()
{
    return;
}

void DummySoundManager::UpdateListener(iView* view)
{
    return;
}
