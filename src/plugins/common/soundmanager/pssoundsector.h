/*
 * pssoundsector.h
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

#ifndef _PSSOUNDSECTOR_H_
#define _PSSOUNDSECTOR_H_

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <csutil/hash.h>
#include <csutil/csstring.h>
#include <csgeom/vector3.h>

//------------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------------
class psMusic;
class psEntity;
class psEmitter;
class SoundControl;
class GEMClientActor;
struct iMeshWrapper;
struct iDocumentNode;

/**
 * work in progress - Looks like a mishap. I hope find a object where i
 * can merge this into
 */
class psSoundSector
{
public:
    csString                     name;               ///< name of this sector
    bool                         active;             ///< is this sector active?
    psMusic*                     activeambient;      ///< active ambient music
    psMusic*                     activemusic;        ///< active music
    csArray<psMusic*>            ambientarray;       ///< array of available ambients
    csArray<psMusic*>            musicarray;         ///< array of available musics
    csArray<psEmitter*>          emitterarray;       ///< array of emitters
    csHash<psEntity*, csString>  factories;          ///< hash of factory entities
    csHash<psEntity*, csString>  meshes;             ///< hash of mesh entities
    csHash<psEntity*, uint>      tempEntities;       ///< hash of all the temporary psEntites (i.e. associated to a specific mesh)

    /**
     * Create an empty psSoundSector with no musics, ambients, emitters and entities.
     * @param objectReg the object registry.
     */
    psSoundSector(iObjectRegistry* objectReg);

    /**
     * Creates a sound sector and loads its definition. This is exactly identical to
     * calling the normal constructor and then Load().
     * @param objectReg the object registry.
     * @param sectorNode iDocumentNode that contains the sector's definition.
     */
    psSoundSector(csRef<iDocumentNode> sectorNode, iObjectRegistry* objectReg);

    /**
     * Destructor.
     * Cleans all arrays
     */
    ~psSoundSector();
    void AddAmbient(csRef<iDocumentNode> Node);
    void UpdateAmbient(int type, SoundControl* &ctrl);
    void DeleteAmbient(psMusic* &ambient);
    void AddMusic(csRef<iDocumentNode> Node);
    void UpdateMusic(bool loopToggle, int type, SoundControl* &ctrl);
    void DeleteMusic(psMusic* &music);

    /**
     * Adds one emitter from the current array of known emitters
     * @param ctrl The sound control to be used to handle the update.
     */
    void AddEmitter(csRef<iDocumentNode> Node);

    /**
     * Start/stops all emitters based on distance and time of the day
     * @param ctrl The sound control to be used to handle the update.
     */
    void UpdateAllEmitters(SoundControl* &ctrl);

    /**
     * Update all entities times, and based on their status generate a sound.
     * @param ctrl The sound control to be used to handle the update.
     */
    void UpdateAllEntities(SoundControl* &ctrl);

    /**
     * Deletes one emitter from the current array of known emitters
     * @param ctrl The sound control to be used to handle the update.
     */
    void DeleteEmitter(psEmitter* &emitter);
    /**
     * Loads an ENTITY entry from an xml file and generates an entity sound definition
     * @param entityNode xml node to load
     */
    void AddEntityDefinition(csRef<iDocumentNode> entityNode);

    /**
     * Updates an object entity managed from the sector.
     * @param mesh The mesh associated to the entity.
     * @param meshName the name associated to the entity.
     * @param ctrl The sound control to be used to handle the update.
     */
    void UpdateEntity(iMeshWrapper* mesh, const char* meshName, SoundControl* &ctrl);
    void DeleteEntity(psEntity* &entity);

    /**
     * Adds an object entity to be managed from the sector.
     * @param mesh The mesh associated to the entity.
     * @param meshName the name associated to the entity.
     */
    void AddObjectEntity(iMeshWrapper* mesh, const char* meshName);

    /**
     * Removes an object entity managed from the sector.
     * @param mesh The mesh associated to the entity.
     * @param meshName the name associated to the entity.
     */
    void RemoveObjectEntity(iMeshWrapper* mesh, const char* meshName);



    /**
     * Sets the new state for the entity associated to the given mesh. If
     * it is already playing a sound, it is stopped.
     *
     * @param state the new state >= 0 for the entity. For negative value
     * the function is not defined.
     * @param mesh the mesh associated to the entity.
     * @param actorName the name associated to the entity.
     * @param forceChange if it is false the entity does not change its
     * state if the new one is not defined. If it is true the entity stops
     * play any sound until a new valid state is defined.
     */
    void SetEntityState(int state, iMeshWrapper* mesh, const char* actorName, bool forceChange);

    /**
     * Loads the sector's definition from the given node. This does not delete
     * the previous data so, if called twice, the new definition is "appended"
     * to the old one.
     * @param sectorNode the node containing the sector's definition.
     */
    void Load(csRef<iDocumentNode> sectorNode);
    void Reload(csRef<iDocumentNode> sector);
    void Delete();

private:
    csRef<iObjectRegistry> objectReg;

    /**
     * Helper function that retrieve the entity associated to a mesh. It checks
     * in temporary, mesh and factory entities in both this and common sector
     * with the priority temporary > this->meshes > this->factories > common->meshes >
     * > common->factories.
     * @param mesh the mesh associated to the entity.
     * @param meshName the name associated to the entity.
     * @return the entity associated to the mesh or 0 if it cannot be found.
     */
    psEntity* GetAssociatedEntity(iMeshWrapper* mesh, const char* meshName) const;
};

#endif /*_PSSOUNDSECTOR_H_*/
