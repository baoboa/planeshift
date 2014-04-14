/*
 * pssoundsector.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
 *           and all past and present planeshift coders
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License.
 *
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
#include <crystalspace.h>
#include "globals.h"
#include "util/log.h"

#include "pssoundsector.h"

#include "net/messages.h"

#include "manager.h"
#include "psmusic.h"
#include "psentity.h"
#include "psemitter.h"
#include "soundctrl.h"
#include "sectormngr.h"
#include "soundmanager.h"


psSoundSector::psSoundSector(iObjectRegistry* objReg)
{
    objectReg = objReg;

    active = false;

    // initializing pointers to null
    activeambient = 0;
    activemusic = 0;
}

psSoundSector::psSoundSector(csRef<iDocumentNode> sectorNode, iObjectRegistry* objReg)
{
    objectReg = objReg;

    active = false;

    // initializing pointers to null
    activeambient = 0;
    activemusic = 0;

    Load(sectorNode);
}

psSoundSector::~psSoundSector()
{
    Delete();
}

void psSoundSector::AddAmbient(csRef<iDocumentNode> Node)
{
    psMusic* ambient;

    ambient = new psMusic;

    ambient->resource         = Node->GetAttributeValue("RESOURCE");
    ambient->type             = Node->GetAttributeValueAsInt("TYPE");
    ambient->minvol           = Node->GetAttributeValueAsFloat("MINVOL");
    ambient->maxvol           = Node->GetAttributeValueAsFloat("MAXVOL");
    ambient->fadedelay        = Node->GetAttributeValueAsInt("FADEDELAY");
    ambient->timeofday        = Node->GetAttributeValueAsInt("TIME");
    ambient->timeofdayrange   = Node->GetAttributeValueAsInt("TIME_RANGE");
    ambient->loopstart        = Node->GetAttributeValueAsInt("LOOPSTART");
    ambient->loopend          = Node->GetAttributeValueAsInt("LOOPEND");
    ambient->active           = false;

    //  for wrong xmls - FIXME when youve fixed the xmls
    if(ambient->timeofday == -1)
    {
        ambient->timeofday = 0;
        ambient->timeofdayrange = 24;
    }

    ambientarray.Push(ambient);
}

void psSoundSector::UpdateAmbient(int type, SoundControl* &ctrl)
{
    psMusic* ambient;
    int timeOfDay = SoundSectorManager::GetSingleton().GetTimeOfDay();

    for(size_t i = 0; i< ambientarray.GetSize(); i++)
    {
        ambient = ambientarray[i];

        // check parameters against our world, is this the track we are searching?
        if(ambient->CheckType(type) == true
                && active == true
                && ctrl->GetToggle() == true
                && ambient->CheckTimeOfDay(timeOfDay) == true)
        {
            if(ambient->active == true)
            {
                continue;
            }

            if(ambient->Play(LOOP, ctrl))
            {
                ambient->FadeUp();
                activeambient = ambient;
            }
            else // error occured .. get rid of this ambient
            {
                DeleteAmbient(ambient);
                break;
            }
        }
        else if(ambient->active == true)
        {
            // state doesnt matter, this handle will be stopped
            ambient->FadeDownAndStop();

            if(activeambient == ambient)
            {
                activeambient = NULL;
            }
        }
    }
}

void psSoundSector::DeleteAmbient(psMusic* &ambient)
{
    ambientarray.Delete(ambient);
    delete ambient;
}

void psSoundSector::AddMusic(csRef<iDocumentNode> Node)
{
    psMusic* music;

    music = new psMusic;

    music->resource         = Node->GetAttributeValue("RESOURCE");
    music->type             = Node->GetAttributeValueAsInt("TYPE");
    music->minvol           = Node->GetAttributeValueAsFloat("MINVOL");
    music->maxvol           = Node->GetAttributeValueAsFloat("MAXVOL");
    music->fadedelay        = Node->GetAttributeValueAsInt("FADEDELAY");
    music->timeofday        = Node->GetAttributeValueAsInt("TIME");
    music->timeofdayrange   = Node->GetAttributeValueAsInt("TIME_RANGE");
    music->loopstart        = Node->GetAttributeValueAsInt("LOOPSTART");
    music->loopend          = Node->GetAttributeValueAsInt("LOOPEND");
    music->active           = false;

    //  for wrong xmls - FIXME when youve fixed the xmls
    if(music->timeofday == -1)
    {
        music->timeofday = 0;
        music->timeofdayrange = 24;
    }

    musicarray.Push(music);
}

void psSoundSector::UpdateMusic(bool loopToggle, int type,
                                SoundControl* &ctrl)
{
    psMusic* music;
    int timeOfDay = SoundSectorManager::GetSingleton().GetTimeOfDay();

    for(size_t i = 0; i< musicarray.GetSize(); i++)
    {
        music = musicarray[i];

        // check parameters against our world, is this the track we are searching?
        if(music->CheckType(type) == true
                && active == true
                && ctrl->GetToggle() == true
                && music->CheckTimeOfDay(timeOfDay) == true)
        {
            if(music->active == true)
            {
                // difficult logic. user doesnt want looping BGM
                // therefor we set this to managed and leave it active
                // so it wont be played again unless he reenabled looping or changes sectors etc ..

                if(loopToggle == false)
                {
                    // set managed as we want keep track of it
                    music->SetManaged();
                    music->DontLoop();
                    continue;
                }
                else
                {
                    // resume looping
                    music->Loop();
                    // we no longer manage it
                    music->SetUnManaged();
                    continue;
                }
            }

            //start the music only if not the current one.
            if(activemusic != music)
            {

                if(music->Play(loopToggle, ctrl))
                {
                    music->FadeUp();
                    activemusic = music;
                }
                else // error occured .. get rid of this music
                {
                    DeleteMusic(music);
                }
            }
        }
        else if(music->active == true)
        {
            // state doesnt matter, this handle will be stopped
            music->FadeDownAndStop();

            if(activemusic == music)
            {
                activemusic = NULL;
            }
        }
    }
}

void psSoundSector::DeleteMusic(psMusic* &music)
{
    musicarray.Delete(music);
    delete music;
}

void psSoundSector::AddEmitter(csRef<iDocumentNode> Node)
{
    psEmitter* emitter;

    emitter = new psEmitter;

    emitter->resource       = Node->GetAttributeValue("RESOURCE");
    emitter->minvol         = Node->GetAttributeValueAsFloat("MINVOL");
    emitter->maxvol         = Node->GetAttributeValueAsFloat("MAXVOL");
    emitter->maxrange       = Node->GetAttributeValueAsFloat("MAX_RANGE");
    emitter->minrange       = Node->GetAttributeValueAsFloat("MIN_RANGE");
    emitter->probability    = Node->GetAttributeValueAsFloat("PROBABILITY", 1.0);
    emitter->loop           = Node->GetAttributeValueAsBool("LOOP", true);
    emitter->dopplerEffect  = Node->GetAttributeValueAsBool("DOPPLER_ENABLED", true);
    emitter->fadedelay      = Node->GetAttributeValueAsInt("FADEDELAY");
    emitter->factory        = Node->GetAttributeValue("FACTORY");
    emitter->factory_prob   = Node->GetAttributeValueAsFloat("FACTORY_PROBABILITY");
    emitter->position       = csVector3(Node->GetAttributeValueAsFloat("X"),
                                        Node->GetAttributeValueAsFloat("Y"),
                                        Node->GetAttributeValueAsFloat("Z"));
    emitter->direction      = csVector3(Node->GetAttributeValueAsFloat("2X"),
                                        Node->GetAttributeValueAsFloat("2Y"),
                                        Node->GetAttributeValueAsFloat("2Z"));
    emitter->timeofday      = Node->GetAttributeValueAsInt("TIME");
    emitter->timeofdayrange = Node->GetAttributeValueAsInt("TIME_RANGE");
    emitter->active         = false;

    // adjusting the probability on the update time
    if(emitter->probability < 1.0)
    {
        emitter->probability *= SoundManager::updateTime / 1000.0f;
    }

    if(emitter->timeofday == -1)
    {
        emitter->timeofday = 0;
        emitter->timeofdayrange = 24;
    }
    emitterarray.Push(emitter);
}


//update on position change
void psSoundSector::UpdateAllEmitters(SoundControl* &ctrl)
{
    psEmitter* emitter;
    int timeOfDay = SoundSectorManager::GetSingleton().GetTimeOfDay();
    csVector3 listenerPos = SoundSystemManager::GetSingleton().GetListenerPos();

    // start/stop all emitters in range
    for(size_t i = 0; i< emitterarray.GetSize(); i++)
    {
        emitter = emitterarray[i];

        if(emitter->CheckRange(listenerPos) == true
                && active == true
                && ctrl->GetToggle() == true
                && emitter->CheckTimeOfDay(timeOfDay) == true)
        {
            if(emitter->active == true)
            {
                continue;
            }

            if(SoundManager::randomGen.Get() <= emitter->probability)
            {
                if(!emitter->Play(ctrl))
                {
                    // error occured .. emitter cant be played .. remove it
                    DeleteEmitter(emitter);
                    break;
                }
            }

        }
        else if(emitter->active != false)
        {
            emitter->Stop();
        }
    }
}

void psSoundSector::UpdateAllEntities(SoundControl* &ctrl)
{
    psEntity* entity;
    int timeOfDay;
    csVector3 listenerPos;

    listenerPos = SoundSystemManager::GetSingleton().GetListenerPos();
    timeOfDay = SoundSectorManager::GetSingleton().GetTimeOfDay();

    csHash<psEntity*, uint>::GlobalIterator tempEntityIter(tempEntities.GetIterator());
    while(tempEntityIter.HasNext())
    {
        entity = tempEntityIter.Next();

        float range;
        csVector3 rangeVec;

        rangeVec = entity->GetPosition() - listenerPos;
        range = rangeVec.Norm();

        // this update the delay and fallback state, play if it can and
        // set itself as active when appropriate. Only temporary entities
        // should be updated
        if(entity->IsTemporary())
        {
            entity->Update(timeOfDay, range, SoundManager::updateTime, ctrl, entity->GetPosition());
        }
    }

}

void psSoundSector::DeleteEmitter(psEmitter* &emitter)
{
    emitterarray.Delete(emitter);
    delete emitter;
}

void psSoundSector::AddEntityDefinition(csRef<iDocumentNode> entityNode)
{
    float minRange;
    float maxRange;
    const char* meshName;
    const char* factoryName;

    psEntity* entity;
    csRef<iDocumentNodeIterator> stateItr;

    Debug1(LOG_SOUND, 0, "psSoundSector::AddEntityDefinition START");

    // determining if this is a mesh entity or a factory entity
    meshName = entityNode->GetAttributeValue("MESH", 0);
    if(meshName == 0) // this isn't a mesh entity we need the factory
    {
        factoryName = entityNode->GetAttributeValue("FACTORY", 0);

        // this is nor a factory entity
        if(factoryName == 0)
        {
            return;
        }
    }

    // getting range parameters
    maxRange = entityNode->GetAttributeValueAsFloat("MAX_RANGE", 0);
    if(maxRange <= 0) // the entity will never play a sound
    {
        return;
    }
    minRange = entityNode->GetAttributeValueAsFloat("MIN_RANGE", 0);

    // checking if an entity with the same name is already defined
    if(meshName == 0)
    {
        entity = factories.Get(factoryName, 0);
    }
    else
    {
        entity = meshes.Get(meshName, 0);
    }

    // if it doesn't exist create it otherwise check the state
    if(entity == 0)
    {
        if(meshName == 0) // factory entity
        {
            entity = new psEntity(true, factoryName);
            factories.Put(factoryName, entity);
            Debug2(LOG_SOUND, 0, "psSoundSector::AddEntityDefinition FACTORY %s",entity->GetEntityName().GetData());
        }
        else // mesh entity
        {
            entity = new psEntity(false, meshName);
            meshes.Put(meshName, entity);
            Debug2(LOG_SOUND, 0, "psSoundSector::AddEntityDefinition MESH %s",entity->GetEntityName().GetData());
        }
    }


    // setting range
    entity->SetRange(minRange, maxRange);

    // creating all the states for this entity
    stateItr = entityNode->GetNodes("STATE");
    while(stateItr->HasNext())
    {
        entity->DefineState(stateItr->Next());
    }

    // entities have a default state as idle
    entity->SetState(psModeMessage::PEACE, true, true);
}

void psSoundSector::AddObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    psEntity* entity;

    if(mesh == 0 || meshName == 0)
    {
        return;
    }
    // retrieving the entity associated to the mesh (if any)
    entity = GetAssociatedEntity(mesh, meshName);
    if(entity == 0)
    {
        return;
    }

    // if this isn't a temporary entity, create one
    Debug4(LOG_SOUND, 0, "psSoundSector::AddObjectEntity MESH %s ID %d temp %s",entity->GetEntityName().GetData(), entity->GetMeshID(), entity->IsTemporary()?"true":"false");

    if(!entity->IsTemporary())
    {
        entity = new psEntity(entity);
        entity->SetMeshID(mesh->QueryObject()->GetID());
        tempEntities.Put(entity->GetMeshID(), entity);
    }
}

void psSoundSector::RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName)
{
    if(mesh == 0 || meshName == 0)
    {
        return;
    }
    psEntity* entity = GetAssociatedEntity(mesh, meshName);
    if(entity == 0)
    {
        return;
    }
    if(entity->IsTemporary())
    {
        Debug3(LOG_SOUND, 0, "psSoundSector::RemoveObjectEntity MESH %s ID %d",entity->GetEntityName().GetData(), entity->GetMeshID());
        DeleteEntity(entity);
    }
}

void psSoundSector::UpdateEntity(iMeshWrapper* mesh, const char* meshName, SoundControl* &ctrl)
{
    float range;
    csVector3 rangeVec;
    psEntity* entity;
    int timeOfDay;
    csVector3 listenerPos;

    if(mesh == 0 || meshName == 0)
    {
        return;
    }
    // retrieving the entity associated to the mesh (if any)
    entity = GetAssociatedEntity(mesh, meshName);
    if(entity == 0)
    {
        return;
    }

    Debug4(LOG_SOUND, 0, "psSoundSector::UpdateEntity MESH %s ID %d temp %s",entity->GetEntityName().GetData(), entity->GetMeshID(), entity->IsTemporary()?"true":"false");

    listenerPos = SoundSystemManager::GetSingleton().GetListenerPos();
    timeOfDay = SoundSectorManager::GetSingleton().GetTimeOfDay();

    rangeVec = mesh->GetMovable()->GetFullPosition() - listenerPos;
    range = rangeVec.Norm();

    // this update the delay and fallback state, play if it can and
    // set itself as active when appropriate. Only temporary entities
    // should be updated
    if(entity->IsTemporary())
    {
        csVector3 entityPosition = mesh->GetMovable()->GetFullPosition();
        entity->Update(timeOfDay, range, SoundManager::updateTime, ctrl, entityPosition);
    }
}

void psSoundSector::DeleteEntity(psEntity* &entity)
{
    if(entity->IsTemporary())
    {
        tempEntities.Delete(entity->GetMeshID(), entity);
    }
    else if(entity->IsFactoryEntity())
    {
        factories.Delete(entity->GetEntityName(), entity);
    }
    else
    {
        meshes.Delete(entity->GetEntityName(), entity);
    }

    delete entity;
}

void psSoundSector::SetEntityState(int state, iMeshWrapper* mesh, const char* actorName, bool forceChange)
{
    uint meshID;
    psEntity* entity;

    entity = GetAssociatedEntity(mesh, actorName);
    if(entity == 0)
    {
        return;
    }

    if(state!=entity->GetState())
        Debug4(LOG_SOUND, 0, "psSoundSector::SetEntityState %s state: %d meshid: %u",entity->GetEntityName().GetData(),state, mesh->QueryObject()->GetID());

    meshID = mesh->QueryObject()->GetID();

    // if the new state isn't DEFAULT_ENTITY_STATE, a new entity must be created so that
    // when it will satisfies the conditions to play (for example when the distance from
    // the player gets enough short), the entity's state will be the correct one. If the
    // new state is DEFAULT_ENTITY_STATE, the entity will be created only when conditions
    // will be satisfied.
    if(!entity->IsTemporary() && state != DEFAULT_ENTITY_STATE)
    {
        entity = new psEntity(entity);
        entity->SetMeshID(meshID);
        tempEntities.Put(meshID, entity);
    }

    // applying state change only if it's a temporary entity
    if(entity->IsTemporary())
    {
        entity->SetState(state, forceChange, true);
        return;
    }
}

void psSoundSector::Load(csRef<iDocumentNode> sectorNode)
{
    csRef<iDocumentNodeIterator> nodeIter;

    // if the sector is already defined the name is overwritten
    name = sectorNode->GetAttributeValue("NAME");

    Debug2(LOG_SOUND, 0, "Loading sound sector data for %s",name.GetData());

    nodeIter = sectorNode->GetNodes("AMBIENT");
    while(nodeIter->HasNext())
    {
        AddAmbient(nodeIter->Next());
    }

    nodeIter = sectorNode->GetNodes("BACKGROUND");
    while(nodeIter->HasNext())
    {
        AddMusic(nodeIter->Next());
    }

    nodeIter = sectorNode->GetNodes("EMITTER");
    while(nodeIter->HasNext())
    {
        AddEmitter(nodeIter->Next());
    }

    nodeIter = sectorNode->GetNodes("ENTITY");
    while(nodeIter->HasNext())
    {
        AddEntityDefinition(nodeIter->Next());
    }
}

void psSoundSector::Reload(csRef<iDocumentNode> sector)
{
    Delete();
    Load(sector);
}

void psSoundSector::Delete()
{
    for(size_t i = 0; i< musicarray.GetSize(); i++)
    {
        DeleteMusic(musicarray[i]);
    }

    for(size_t i = 0; i< ambientarray.GetSize(); i++)
    {
        DeleteAmbient(ambientarray[i]);
    }

    for(size_t i = 0; i< emitterarray.GetSize(); i++)
    {
        DeleteEmitter(emitterarray[i]);
    }

    // Deleting factories and meshes entities
    csHash<psEntity*, csString>::GlobalIterator entityIter(factories.GetIterator());
    psEntity* entity;

    while(entityIter.HasNext())
    {
        entity = entityIter.Next();
        delete entity;
    }

    entityIter = meshes.GetIterator();
    while(entityIter.HasNext())
    {
        entity = entityIter.Next();
        delete entity;
    }

    factories.DeleteAll();
    meshes.DeleteAll();

    // Deleting temporary entities
    csHash<psEntity*, uint>::GlobalIterator tempEntityIter(tempEntities.GetIterator());

    while(tempEntityIter.HasNext())
    {
        entity = tempEntityIter.Next();
        delete entity;
    }

    tempEntities.DeleteAll();
}

psEntity* psSoundSector::GetAssociatedEntity(iMeshWrapper* mesh, const char* meshName) const
{
    uint meshID;
    psEntity* entity;

    // checking if there is have a temporary entity for the mesh
    // here the common sector is not checked because it don't have temporary entities
    meshID = mesh->QueryObject()->GetID();
    entity = tempEntities.Get(meshID, 0);

    if(entity == 0)
    {
        const char* factoryName;

        // checking mesh entities
        entity = meshes.Get(meshName, 0);
        if(entity == 0)
        {
            // getting factory name
            iMeshFactoryWrapper* factory = mesh->GetFactory();
            if(factory != 0)
            {
                factoryName = factory->QueryObject()->GetName();
            }
            else
            {
                factoryName = 0;
            }

            // checking factory entities
            if(factoryName != 0)
            {
                entity = factories.Get(factoryName, 0);
            }
        }

        // if nothing has been found check in common sector
        if(entity == 0)
        {
            psSoundSector* commonSector;

            commonSector = SoundSectorManager::GetSingleton().GetCommonSector();
            entity = commonSector->meshes.Get(meshName, 0);

            if(entity == 0 && factoryName != 0)
            {
                entity = commonSector->factories.Get(factoryName, 0);
            }
        }
    }

    return entity;
}
