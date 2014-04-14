/*
 * psentity.h
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


#ifndef _PSENTITY_H_
#define _PSENTITY_H_


#define DEFAULT_ENTITY_STATE 0
#define UNDEFINED_ENTITY_STATE -1

/**
 * This object represents a planeshift entity sound. It can be a mesh entity, if it is
 * associated to a mesh object, or a factory entity if it is associated to a mesh factory.
 *
 * @see psSoundSector psEntity main user.
 */
class psEntity
{
public:
    /**
     * Create a new psEntity. The initial state is set to DEFAULT_ENTITY_STATE.
     * @param isFactoryEntity true if this is associated to a factory, false if this is
     * associated to a mesh.
     * @param entityName the name of the factory or the mesh.
     */
    psEntity(bool isFactoryEntity, const char* entityName);

    /**
     * Destructor. When destroyed the sounds is not stopped and it is removed
     * by the SoundSystemManager when it is done.
     */
    ~psEntity();

    /**
     * Copy Constructor. It copies everthing but not the handle that is set to 0.
     *
     * @note the data of each possible state is copied by reference to save time
     * so any change on that data will affect also its clone.
     */
    psEntity(psEntity* const &entity);

    /**
     * Return true if this is a factory entity.
     * @return true if this is a factory entity, false otherwise.
     */
    bool IsFactoryEntity() const
    {
        return isFactoryEntity;
    }

    /**
     * Gets the entity name associated to this object. Use IsFactoryEntity() to check
     * if this name is a mesh name or a factory name.
     * @return the entity name.
     */
    const csString &GetEntityName() const
    {
        return entityName;
    }

    /**
     * Sets this object as a mesh entity.
     * @param meshName the name of the mesh associated to this object.
     */
    void SetAsMeshEntity(const char* meshName);

    /**
     * Sets this object as a factory entity.
     * @param factoryName the name of the factory associated to this object.
     */
    void SetAsFactoryEntity(const char* factoryName);

    /**
     * Gets the maximum distance at which this entity can be heard.
     * @return the maximum distance at which this entity can be heard.
     */
    float GetMaxRange() const
    {
        return maxRange;
    }

    /**
     * Sets the range for this entity.
     * @param minRange maximum distance at which this entity is heard at maximum volume.
     * @param maxRange maximum distance at which this entity can be heard.
     */
    void SetRange(float minRange, float maxRange);

    /**
     * Get the ID of the mesh associated to this entity.
     * @return the ID of the mesh.
     */
    uint GetMeshID() const;

    /**
     * Set the ID of the mesh associated to this entity.
     * @param id the new id of this entity.
     */
    void SetMeshID(uint id);

    /**
     * Returns the 3d position of this entity.
     */
    csVector3 GetPosition()
    {
        return position;
    };

    /**
     * Create a new state from an XML <state> node.
     * @param stateNode the state node.
     * @return true if the state could be created, false otherwise.
     */
    bool DefineState(csRef<iDocumentNode> stateNode);

    /**
     * Check if this entity is active.
     * @return true if this is active, false otherwise.
     */
    bool IsActive() const
    {
        return isActive;
    }

    /**
     * Activate or deactivate this entity.
     * @param toggle true to activate this entity, false to deactivate it.
     */
    void SetActive(bool toggle)
    {
        isActive = toggle;
    }

    /**
     * Used to determine if this is a temporary entity associated to a specific mesh
     * object or if it is a factory/mesh entity that is not associated to any mesh.
     * @return true if this entity is temporary, false otherwise.
     */
    bool IsTemporary() const;

    /**
     * Check if the sound associated to this entity is still working.
     * @return true if it is still playing, false otherwise.
     */
    bool IsPlaying() const;

    /**
     * Checks if all condition for the sound to play are satisfied in
     * the current state.
     *
     * @param time <24 && >0 is resonable but can be any valid int.
     * @param range the distance to test.
     * @return true if the dalayed is done, the entity is not in an
     * undefined state, the given time is within this entity's time
     * window and if the distance is between the minimum and maximum
     * range. False otherwise.
     */
    bool CanPlay(int time, float range) const;

    /**
     * Gets the state of the entity
     */
    int GetState()
    {
        return state;
    };

    /**
     * Set the new state for the entity. If the given state is undefined for
     * this entity the change of state is not forced, the state does not change.
     * On the other hand if the change is forced the entity's state becomes
     * UNDEFINED_ENTITY_STATE. In this state psEntity cannot play anything.
     *
     * @param state the new state for this entity.
     * @param forceChange true to force the state change, false otherwise.
     * @param setReady when set to true this set the entity in the condition to
     * play the new state sounds by stopping any eventual playing sound and
     * resetting the delay.
     */
    void SetState(int state, bool forceChange, bool setReady);

    /**
     * Force this entity to play the sound associated to its current state.
     * You need to supply a SoundControl and the position for this sound.
     *
     * @param ctrl the SoundControl to control this sound.
     * @param entityposition position of this entity.
     * @return true if the sound is played, false if it is not or if this
     * entity is already playing a sound.
     */
    bool Play(SoundControl* &ctrl, csVector3 entityPosition);

    /**
     * If this entity is playing, this method forces this to stop the sound.
     */
    void Stop();

    /**
     * Update the entity's activation status, play the sound associated with
     * the current state if all condition are satisfied, update the delay
     * and fallback in the new state if necessary.
     * @param time the time of the day, <24 && >0 is resonable but can be any
     * valid int.
     * @param distance the distance from the listener.
     * @param ctrl the SoundControl to play the sound with.
     * @param entityPosition the position of the player.
     */
    void Update(int time, float distance, int interval, SoundControl* &ctrl,
                csVector3 entityPosition);

private:
    /**
     * Keeps all the parameters of an entity state.
     */
    struct EntityState
    {
        csStringArray resources;    ///< resource names of the sounds associated to the state.
        float volume;               ///< volume of the sound.
        float probability;          ///< how high is the probability that this entity makes this sound.
        int delay;                  ///< number of milliseconds till played again.
        int timeOfDayStart;         ///< time when this entity starts playing.
        int timeOfDayEnd;           ///< time when this entity stops.

        int fallbackState;          ///< the stateID that is activated after this one.
        float fallbackProbability;  ///< the probability per second that the fallbackState is activated.

        int references;             ///< how many psEntity point to this EntityState.
    };

    bool isActive;                      ///< true if this psEntity is active

    bool isFactoryEntity;               ///< true if this is a factory entity, false if it's a mesh entity.
    csString entityName;                ///< name of the entity associated to this object.

    int state;                         ///< current state of this entity. A negative value means that this entity is in an undefined state.
    int when;                           ///< counter to keep track when it has been played - zero means i may play at any time (in ms)
    uint id;                            ///< the id of the mesh object whose sound is controlled by this entity.
    float minRange;                     ///< minimum distance at which this entity can be heard
    float maxRange;                     ///< maximum distance at which this entity can be heard
    csVector3      position;            ///< position of the entity

    SoundHandle* handle;                ///< pointer to the SoundHandle if playing
    csHash<EntityState*, uint> states;  ///< entity states hash mapped by their ID.

    /**
     * The Callback gets called if the SoundHandle is destroyed.
     * It sets handle to NULL.
     */
    static void StopCallback(void* object);
};

#endif /*_PSENTITY_H_*/
