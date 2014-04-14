/*
 * sectormngr.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
 *
 * Copyright (C) 2001-2012 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "sectormngr.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iutil/cfgmgr.h>
#include <iutil/stringarray.h>

#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <util/psxmlparser.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "soundmanager.h"
#include "manager.h"
#include "psmusic.h"
#include "psemitter.h"
#include "pssoundsector.h"

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
#define DEFAULT_AREAS_PATH "/planeshift/soundlib/areas/"
#define DEFAULT_COMMON_SECTOR_NAME "common"


SoundSectorManager::SoundSectorManager()
{
    // subscribing to sound ambients and music control events
    musicSndCtrl = SoundSystemManager::GetSingleton().GetSoundControl(iSoundManager::MUSIC_SNDCTRL);
    ambientSndCtrl = SoundSystemManager::GetSingleton().GetSoundControl(iSoundManager::AMBIENT_SNDCTRL);
    musicSndCtrl->Subscribe(this);
    ambientSndCtrl->Subscribe(this);

    weather = 1; // 1 is sunshine
    timeOfDay = 0;
    combatStance = iSoundManager::PEACE;
    loopBGM = true;
    isCombatMusicOn = true;

    areSectorsLoaded = false;
    activeSector = 0;
    commonSector = 0;
}

SoundSectorManager::~SoundSectorManager()
{
    UnloadSectors();
}

bool SoundSectorManager::Initialize(iObjectRegistry* objReg)
{
    objectReg = objReg;

    return LoadSectors();
}

bool SoundSectorManager::LoadSectors()
{
    csString commonName;
    const char* areasPath;
    csRef<iVFS> vfs;
    csRef<iStringArray> files;
    csRef<iDataBuffer> expandedPath;

    // check if sectors have been already loaded
    if(areSectorsLoaded)
    {
        return true;
    }

    // loading configuration
    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager>(objectReg);
    if(configManager != 0)
    {
        areasPath = configManager->GetStr("Planeshift.Sound.AreasPath", DEFAULT_AREAS_PATH);
        commonName = configManager->GetStr("Planeshift.Sound.CommonSector", DEFAULT_COMMON_SECTOR_NAME);
    }
    else
    {
        areasPath = DEFAULT_AREAS_PATH;
        commonName = DEFAULT_COMMON_SECTOR_NAME;
    }

    // retrieving all definition files
    vfs = csQueryRegistry<iVFS>(objectReg);
    if(vfs == 0)
    {
        return false;
    }

    expandedPath = vfs->ExpandPath(areasPath);
    files = vfs->FindFiles(expandedPath->GetData());

    if(files == 0)
    {
        return false;
    }

    // the common sector is always initialized and must always be active
    commonSector = new psSoundSector(objectReg);
    commonSector->name = commonName;
    commonSector->active = true;

    // parsing files
    for(size_t i = 0; i < files->GetSize(); i++)
    {
        csString fileName = files->Get(i);
        csRef<iDocument> doc;
        csRef<iDocumentNode> rootNode;
        csRef<iDocumentNode> mapNode;
        csRef<iDocumentNodeIterator> sectorIter;

        // getting root
        doc = ParseFile(objectReg, fileName);
        if(doc == 0)
        {
            continue;
        }

        rootNode = doc->GetRoot();
        if(rootNode == 0)
        {
            continue;
        }

        mapNode = rootNode->GetNode("MAP_SOUNDS");
        if(mapNode == 0)
        {
            continue;
        }

        // parsing sectors
        sectorIter = mapNode->GetNodes("SECTOR");
        while(sectorIter->HasNext())
        {
            psSoundSector* sector;
            csRef<iDocumentNode> sectorNode = sectorIter->Next();
            csString sectorName = sectorNode->GetAttributeValue("NAME");

            // the common sector is not kept together with the others
            if(sectorName == commonName)
            {
                // common sector has been already initialized
                sector = commonSector;
            }
            else
            {
                sector = FindSector(sectorName);
            }

            // create new sector or append new definition
            if(sector == 0)
            {
                sector = new psSoundSector(sectorNode, objectReg);
                sectors.Push(sector);
            }
            else
            {
                sector->Load(sectorNode);
            }
        }
    }

    areSectorsLoaded = true;
    return true;
}

/**
 * Delete both the sectors and commonSector
 */
void SoundSectorManager::UnloadSectors()
{
    if(!areSectorsLoaded)
    {
        return;
    }

    for(size_t i = 0; i < sectors.GetSize(); i++)
    {
        delete sectors[i];
    }
    sectors.DeleteAll();

    delete commonSector;

    // updating state
    commonSector = 0;
    activeSector = 0;
    areSectorsLoaded = false;
}

bool SoundSectorManager::ReloadSectors()
{
    csString currentSector;

    if(!areSectorsLoaded)
    {
        return false;
    }

    currentSector = activeSector->name;
    UnloadSectors();
    LoadSectors();
    SetActiveSector(currentSector);

    return areSectorsLoaded;
}

void SoundSectorManager::Update()
{
    if(activeSector == 0)
    {
        return;
    }

    // update Emitters
    activeSector->UpdateAllEmitters(ambientSndCtrl);

    // update Entities
    activeSector->UpdateAllEntities(ambientSndCtrl);
}

void SoundSectorManager::SetLoopBGMToggle(bool toggle)
{
    loopBGM = toggle;

    if(activeSector != 0)
    {
        activeSector->UpdateMusic(loopBGM, combatStance, musicSndCtrl);
    }
}

void SoundSectorManager::SetCombatMusicToggle(bool toggle)
{
    isCombatMusicOn = toggle;

    // this updates puts the combat stance to PEACE if combat music
    // has been disabled and updates the music
    SetCombatStance(combatStance);
}

/*
 * FACTORY Emitters are converted to real EMITTERS (if not already done)
 * it also moves background and ambient music into the new sector
 * if they are not changing and are already playing
 * unused music and sounds will be unloaded
 */
bool SoundSectorManager::SetActiveSector(const char* sectorName)
{
    psSoundSector* oldSector;
    psSoundSector* newSector;

    if(!areSectorsLoaded)
    {
        return false;
    }

    // checking if the new active sector is already active
    if(activeSector != 0)
    {
        if(activeSector->name == sectorName)
        {
            return true;
        }
    }

    // if the new active sector doesn't exist fail
    newSector = FindSector(sectorName);
    if(newSector == 0)
    {
        return false;
    }

    oldSector = activeSector;
    activeSector = newSector;
    activeSector->active = true;

    /* works only on loaded sectors! */
    ConvertFactoriesToEmitter(activeSector);

    if(oldSector != NULL)
    {
        /*
        * hijack active ambient and music if they are played in the new sector
        * in the old sector there MUST be only ONE of each
        */

        TransferHandles(oldSector, activeSector);

        /* set old sector to inactive and make a update (will stop all sounds) */
        oldSector->active = false;
        UpdateSector(oldSector);

    }

    /*
    * Call Update start sound in the new sector
    */

    UpdateSector(activeSector);
    return true;
}

psSoundSector* SoundSectorManager::FindSector(const csString &sectorName) const
{
    for(size_t i = 0; i< sectors.GetSize(); i++)
    {
        if(sectorName == sectors[i]->name)
        {
            return sectors[i];
        }
    }

    return 0;
}

void SoundSectorManager::UpdateSector(psSoundSector* sector)
{
    sector->UpdateMusic(loopBGM, combatStance, musicSndCtrl);
    sector->UpdateAmbient(weather, ambientSndCtrl);
    sector->UpdateAllEmitters(ambientSndCtrl);
}

void SoundSectorManager::ConvertFactoriesToEmitter(psSoundSector* sndSector)
{
    psEmitter* newEmitter;
    psEmitter* factoryEmitter;

    iMeshList* meshes;
    iMeshWrapper* mesh;
    iMeshFactoryWrapper* factory;
    iSector* engineSector;
    csRef<iEngine> engine;


    engine = csQueryRegistry<iEngine>(objectReg);
    if(!engine)
    {
        Error1("No iEngine plugin!");
        return;
    }

    /*
    * convert emitters with factory attributes to real emitters
    * positions are random but are only generated once
    */

    for(size_t j = 0; j< sndSector->emitterarray.GetSize(); j++)
    {
        factoryEmitter = sndSector->emitterarray[j];

        if(factoryEmitter->factory.IsEmpty())
        {
            // this is not a factory emitter
            continue;
        }

        engineSector = engine->FindSector(sndSector->name, 0);
        if(engineSector == 0)
        {
            Error2("sector %s not found\n", (const char*)sndSector);
            continue;
        }

        factory = engine->GetMeshFactories()->FindByName(factoryEmitter->factory);
        if(factory == 0)
        {
            Error2("Could not find factory name %s", (const char*)factoryEmitter->factory);
            continue;
        }

        // creating an emitter for each mesh of that factory
        meshes = engineSector->GetMeshes();
        for(int k = 0; k < meshes->GetCount(); k++)
        {
            mesh = meshes->Get(k);

            if(mesh->GetFactory() == factory)
            {
                if(SoundManager::randomGen.Get() <= factoryEmitter->factory_prob)
                {
                    newEmitter = new psEmitter;

                    newEmitter->resource = csString(factoryEmitter->resource);
                    newEmitter->minvol   = factoryEmitter->minvol;
                    newEmitter->maxvol   = factoryEmitter->maxvol;
                    newEmitter->maxrange = factoryEmitter->maxrange;
                    newEmitter->position = mesh->GetMovable()->GetPosition();
                    newEmitter->active   = false;

                    sndSector->emitterarray.Push(newEmitter);
                }
            }
        }

        // the factory emitter is not needed anymore
        sndSector->DeleteEmitter(factoryEmitter);
    }
}

/*
* Transfer ambient/music Handles from oldSector to newSector if needed
* if loopBGM is false then the handle might be invalid
* this is no problem because we dont touch it
* all we do is copy the address
*/
void SoundSectorManager::TransferHandles(psSoundSector* oldSector,
        psSoundSector* newSector)
{
    for(size_t j = 0; j< newSector->musicarray.GetSize(); j++)
    {
        if(oldSector->activemusic == NULL)
        {
            break;
        }

        if(csStrCaseCmp(newSector->musicarray[j]->resource,
                        oldSector->activemusic->resource) == 0)
        {
            /* yay active resource with the same name - steal the handle*/
            newSector->musicarray[j]->handle = oldSector->activemusic->handle;
            /* set handle to NULL */
            oldSector->activemusic->handle = NULL;
            newSector->activemusic = newSector->musicarray[j];
            /* set the sound to active */
            newSector->musicarray[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldSector->activemusic->active = false;
            /* set it to NULL to avoid problems */
            oldSector->activemusic = NULL;
            /* update callback */
            newSector->activemusic->UpdateHandleCallback();
        }
    }

    for(size_t j = 0; j< activeSector->ambientarray.GetSize(); j++)
    {
        if(oldSector->activeambient == NULL)
        {
            break;
        }

        if(csStrCaseCmp(newSector->ambientarray[j]->resource,
                        oldSector->activeambient->resource) == 0)
        {
            /* yay active resource with the same name - steal the handle*/
            newSector->ambientarray[j]->handle = oldSector->activeambient->handle;
            /* set handle to NULL */
            oldSector->activeambient->handle = NULL;
            newSector->activeambient = newSector->ambientarray[j];
            /* set the sound to active */
            newSector->ambientarray[j]->active = true;
            /* set it to inactive to prevent damage on the handle */
            oldSector->activeambient->active = false;
            /* set it to NULL to avoid problems */
            oldSector->activeambient = NULL;
            /* update callback */
            newSector->activeambient->UpdateHandleCallback();
        }
    }
    //TODO: we should transfer also entities to the new sector.
}

void SoundSectorManager::SetCombatStance(int newCombatStance)
{
    if(isCombatMusicOn)
    {
        combatStance = newCombatStance;
    }
    else
    {
        combatStance = iSoundManager::PEACE;
    }

    if(activeSector != 0)
    {
        activeSector->UpdateMusic(loopBGM, combatStance, musicSndCtrl);
    }
}

void SoundSectorManager::SetTimeOfDay(int newTimeOfDay)
{
    timeOfDay = newTimeOfDay;

    if(activeSector != 0)
    {
        // ambient and music are not touched at every update
        activeSector->UpdateAmbient(weather, ambientSndCtrl);
        activeSector->UpdateMusic(loopBGM, combatStance, musicSndCtrl);
    }
}

void SoundSectorManager::SetWeather(int newWeather)
{
    // Engine calls SetWeather every frame, update is
    // only called if weather is changing
    if(weather != newWeather)
    {
        weather = newWeather;

        if(activeSector != 0)
        {
            activeSector->UpdateAmbient(weather, ambientSndCtrl);
        }
    }
}

void SoundSectorManager::SetEntityState(int state, iMeshWrapper* mesh, const char* meshName, bool forceChange)
{
    if(activeSector != 0)
    {
        activeSector->SetEntityState(state, mesh, meshName, forceChange);
    }
}

void SoundSectorManager::AddObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    if(activeSector != 0)
    {
        activeSector->AddObjectEntity(mesh, meshName);
        activeSector->UpdateEntity(mesh, meshName, ambientSndCtrl);
    }
}

void SoundSectorManager::RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    if(activeSector != 0)
    {
        activeSector->RemoveObjectEntity(mesh, meshName);
    }
}

void SoundSectorManager::UpdateObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    if(activeSector != 0)
    {
        activeSector->UpdateEntity(mesh, meshName, ambientSndCtrl);
    }
}

//----------------------------
// FROM iSoundControlListener
//----------------------------

void SoundSectorManager::OnSoundChange(SoundControl* sndCtrl)
{
    if(activeSector == 0)
    {
        return;
    }

    if(sndCtrl == musicSndCtrl)
    {
        activeSector->UpdateMusic(loopBGM, combatStance, musicSndCtrl);
    }
    else if(sndCtrl == ambientSndCtrl)
    {
        activeSector->UpdateAmbient(weather, ambientSndCtrl);
    }
}
