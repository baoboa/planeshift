/*
* npcmesh.h by Andrew Craig <andrew@hydlaa.com>
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef NPC_MESH_HEADER
#define NPC_MESH_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include "cstypes.h"
#include "csutil/scf.h"
#include "csutil/weakref.h"

//=============================================================================
// Crystal Space Forward Definitions
//=============================================================================
struct iEngine;
struct iMeshWrapper;
struct iMeshFactoryWrapper;
struct iSector;
struct iObjectRegistry;
class csVector3;


//=============================================================================
// PlaneShift Forward Definitions
//=============================================================================
class psNPCClient;
class gemNPCObject;

/**
 * \addtogroup npcclient
 * @{ */

/** This is a helper class that defines a mesh on the server.
 *
 * It wraps around some of the loading details.
 */
class npcMesh
{
public:
    /**
     * Create a new gem Mesh.
     *
     * This will setup a new mesh and add it to the CS engine.
     *
     * @param objreg          The main Crystal Space object registry.
     * @param owner           The npcObject that is using this mesh.
     * @param super           The NPC client supervisor object.
     */
    npcMesh(iObjectRegistry* objreg, gemNPCObject* owner, psNPCClient* super);

    /**
     * Destructor.
     */
    ~npcMesh();

    /**
     * Load a mesh factory.
     *
     * This is called if it can't find the factory in the cached engine list.
     * @param fileName        The name of the file where the factory is stored.
     * @param factoryName     The name of the factory that is loading.
     *
     * @return  An iMeshFactoryWrapper that was loaded.  Null if nothing could
     *          be loaded.
     */
    iMeshFactoryWrapper* LoadMeshFactory(const char* fileName, const char* factoryName);

    /**
     * Set a mesh.
     *
     * @param factoryName     The factory name of the mesh we want to set.
     * @param fileName        The file to load if the factory is not found.
     *
     * @return true If it was successful in setting the mesh.
     */
    bool SetMesh(const char* factoryName, const char* fileName);

    /**
     * Get the Crystal Space iMeshWrapper from this.
     *
     * @return an iMeshWrapper that is being used.
     */
    iMeshWrapper* GetMesh();

    /**
     * Set a mesh.
     *
     * @param newMesh         The Crystal Space mesh wrapper to set in.
     */
    void SetMesh(iMeshWrapper* newMesh);

    /**
     * Removes the mesh from the engine.
     */
    void RemoveMesh();

    /**
     * Move a mesh.
     *
     * @param sector          The sector to move in.
     * @param position        The position to place the mesh.
     */
    void MoveMesh(iSector* sector, const csVector3 &position);

private:
    csRef<iMeshWrapper> mesh;               ///< This is the mesh we are using.
    csRef<iObjectRegistry> objectReg;       ///< CS object registry list.
    csWeakRef<iEngine> engine;              ///< Main CS engine.

    psNPCClient* gem;                       ///< Object controller.

    gemNPCObject* gemOwner;                 ///< gemObject using this mesh.
};

/** @} */

#endif
