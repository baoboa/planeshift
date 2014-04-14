/*
 * psentity.cpp
 *
 * Copyright (C) 2001-2010 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Andrea Rizzi <88whacko@gmail.com>
 *           Saul Leite <leite@engineer.com>
 *           Mathias 'AgY' Voeroes <agy@operswithoutlife.net>
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

#include "pssound.h"
#include "soundmanager.h"

#include "util/log.h"


psEntity::psEntity(bool isFactory, const char* name)
{
    isFactoryEntity = isFactory;
    entityName = name;

    isActive = false;
    when = 0;
    state = DEFAULT_ENTITY_STATE;
    id = 0;
    handle = 0;
    position = csVector3(0);

    minRange = 0.0f;
    maxRange = 0.0f;
}

psEntity::psEntity(psEntity* const &entity)
{
    isFactoryEntity = entity->isFactoryEntity;
    entityName = entity->entityName;

    Debug2(LOG_SOUND, 0, "psEntity::psEntity %s",entityName.GetData());

    states = csHash<EntityState*, uint>(entity->states);

    // updating states references
    csHash<EntityState*, uint>::GlobalIterator statIter(states.GetIterator());
    EntityState* entityState;

    while(statIter.HasNext())
    {
        entityState = statIter.Next();
        entityState->references++;
    }

    isActive = entity->isActive;
    when = entity->when;
    state = entity->state;
    id = entity->id;
    handle = 0;
    position = entity->position;

    minRange = entity->minRange;
    maxRange = entity->maxRange;
}

psEntity::~psEntity()
{
    if(IsPlaying())
    {
        handle->RemoveCallback();
    }

    // removing state parameters but only if there are no more references
    csHash<EntityState*, uint>::GlobalIterator statIter(states.GetIterator());
    EntityState* entityState;

    while(statIter.HasNext())
    {
        entityState = statIter.Next();

        if(entityState->references == 1)
        {
            delete entityState;
        }
        else
        {
            entityState->references--;
        }
    }

    states.DeleteAll();
}

void psEntity::SetAsMeshEntity(const char* meshName)
{
    if(meshName == 0)
    {
        return;
    }

    isFactoryEntity = false;
    entityName = meshName;
}

void psEntity::SetAsFactoryEntity(const char* factoryName)
{
    if(factoryName == 0)
    {
        return;
    }

    isFactoryEntity = true;
    entityName = factoryName;
}

void psEntity::SetRange(float minDistance, float maxDistance)
{
    minRange = minDistance;
    maxRange = maxDistance;
}

uint psEntity::GetMeshID() const
{
    return id;
}

void psEntity::SetMeshID(uint identifier)
{
    id = identifier;
}

bool psEntity::DefineState(csRef<iDocumentNode> stateNode)
{

    int stateID;

    EntityState* entityState;
    csRef<iDocumentNodeIterator> resourceItr;

    // checking if the state ID is legal
    stateID = stateNode->GetAttributeValueAsInt("ID", -1);
    Debug4(LOG_SOUND, 0, "psEntity::DefineState %s state %d meshid %u",entityName.GetData(),stateID,id);
    if(stateID < 0)
    {
        return false;
    }

    // initializing the EntityState
    entityState = states.Get(stateID, 0);

    if(entityState != 0) // already defined
    {
        return false;
    }

    entityState = new EntityState();

    // initializing resources
    resourceItr = stateNode->GetNodes("RESOURCE");
    while(resourceItr->HasNext())
    {
        const char* resourceName = resourceItr->Next()->GetAttributeValue("NAME", 0);
        if(resourceName == 0)
        {
            continue;
        }
        entityState->resources.PushSmart(resourceName);
    }

    // checking if there is at least one resource
    if(entityState->resources.IsEmpty())
    {
        delete entityState;
        return false;
    }

    // setting all the other parameters
    entityState->probability = stateNode->GetAttributeValueAsFloat("PROBABILITY", 1.0);
    entityState->volume = stateNode->GetAttributeValueAsFloat("VOLUME", VOLUME_NORM);
    entityState->delay = stateNode->GetAttributeValueAsInt("DELAY", 0);
    entityState->timeOfDayStart = stateNode->GetAttributeValueAsInt("TIME_START", 0);
    entityState->timeOfDayEnd = stateNode->GetAttributeValueAsInt("TIME_END", 24);
    entityState->fallbackState = stateNode->GetAttributeValueAsInt("FALLBACK_STATE", UNDEFINED_ENTITY_STATE);
    entityState->fallbackProbability = stateNode->GetAttributeValueAsFloat("FALLBACK_PROBABILITY", 0.0);
    entityState->references = 1;

    // adjusting the probabilities on the update time
    if(entityState->probability < 1.0)
    {
        entityState->probability *= SoundManager::updateTime / 1000.0;
    }
    if(entityState->fallbackProbability < 1.0)
    {
        entityState->fallbackProbability *= SoundManager::updateTime / 1000.0;
    }

    // in XML delay is given in seconds, converting delay into milliseconds
    entityState->delay = entityState->delay * 1000;

    states.Put(stateID, entityState);

    return true;
}

bool psEntity::IsTemporary() const
{
    return (id != 0);
}

bool psEntity::IsPlaying() const
{
    return (handle != 0);
}

bool psEntity::CanPlay(int time, float range) const
{
    EntityState* entityState;

    // checking if it is in the undefined state
    entityState = states.Get(state, 0);
    if(entityState == 0)
    {
        Debug3(LOG_SOUND, 0, "psEntity::CanPlay %s meshid: %u undefined state.", entityName.GetData(), GetMeshID());
        return false;
    }

    // checking time, range and delay
    if(range < minRange || range > maxRange)
    {
        Debug6(LOG_SOUND, 0, "psEntity::CanPlay %s meshid: %u range %f %f %f", entityName.GetData(),GetMeshID(), minRange, range, maxRange);
        return false;
    }
    else if(time < entityState->timeOfDayStart || entityState->timeOfDayEnd < time)
    {
        Debug6(LOG_SOUND, 0, "psEntity::CanPlay %s meshid: %u time of day %d %d %d", entityName.GetData(),GetMeshID(), entityState->timeOfDayStart,time,entityState->timeOfDayEnd);
        return false;
    }
    else if(when <= 0)
    {
        Debug4(LOG_SOUND, 0, "psEntity::CanPlay TRUE %s meshid: %u when <0 : %d", entityName.GetData(),GetMeshID(), when);
        return true;
    }
    Debug4(LOG_SOUND, 0, "psEntity::CanPlay %s meshid: %u when : %d", entityName.GetData(),GetMeshID(), when);

    return false;
}

void psEntity::SetState(int newState, bool forceChange, bool setReady)
{
    EntityState* entityState;

    // check if it's already in this state or if it's defined
    if(state == newState)
    {
        return;
    }

    Debug3(LOG_SOUND, 0, "psEntity::SetState entity: %s state: %u", entityName.GetData(), newState);

    if(setReady)
    {
        Stop();
        when = 0; // resetting delay
    }

    // setting state
    entityState = states.Get(newState, 0);
    if(entityState == 0)
    {
        if(forceChange)
        {
            state = UNDEFINED_ENTITY_STATE;
        }

        return;
    }

    state = newState;
}

bool psEntity::Play(SoundControl* &ctrl, csVector3 entityPosition)
{
    EntityState* entityState;

    // checking if a sound is still playing
    if(IsPlaying())
    {
        return false;
    }

    // checking if the state is defined
    entityState = states.Get(state, 0);
    if(entityState == 0)
    {
        return false;
    }

    // picking up randomly among resources
    if(!entityState->resources.IsEmpty())
    {
        int resourceNumber = SoundManager::randomGen.Get() * entityState->resources.GetSize();

        Debug4(LOG_SOUND, 0, "psEntity::Play() %s PLAYS %s meshid: %u",entityName.GetData(),entityState->resources[resourceNumber],GetMeshID());

        if(SoundSystemManager::GetSingleton().Play3DSound(entityState->resources[resourceNumber], DONT_LOOP, 0, 0,
                entityState->volume, ctrl, entityPosition, 0, minRange, maxRange,
                VOLUME_ZERO, CS_SND3D_ABSOLUTE, handle))
        {
            when = entityState->delay;
            handle->SetCallback(this, &StopCallback);
            return true;
        }
    }

    return false;
}

void psEntity::Stop()
{
    if(IsPlaying())
    {
        handle->RemoveCallback();
        SoundSystemManager::GetSingleton().StopSound(handle->GetID());
        handle = 0;
    }
}

void psEntity::Update(int time, float distance, int interval, SoundControl* &ctrl,
                      csVector3 entityPosition)
{
    EntityState* currentState = states.Get(state, 0);

    position = entityPosition;
    if(IsPlaying())
    {
        isActive = true;
    }
    else if(when > 0)   // surely !IsPlaying() is true
    {
        // reducing delay time
        isActive = true;
        when -= interval;
    }
    else if(currentState != 0 && CanPlay(time, distance))
    {
        float prob;

        isActive = true;
        prob = SoundManager::randomGen.Get();
        if(prob <= currentState->probability)
        {
            Play(ctrl, entityPosition);
        }
    }
    else if(state != DEFAULT_ENTITY_STATE)
    {
        // if the state is different than DEFAULT_ENTITY_STATE this information
        // should be kept
        isActive = true;
    }
    else
    {
        isActive = false;
    }

    // the state goes to fallback even if the sound is still playing otherwise
    // it will never fallback when entity->probability == 1
    if(currentState != 0)
    {
        float prob = SoundManager::randomGen.Get();
        if(prob <= currentState->fallbackProbability)
        {
            // If the entity is still playing the sound is not stopped so that
            // states that change to a fallback state with probability 1.0 won't
            // play a new sound until the previous one has been done.
            SetState(currentState->fallbackState, true, false);
        }
    }
}

void psEntity::StopCallback(void* object)
{
    psEntity* which = (psEntity*) object;
    which->handle = 0;
}

