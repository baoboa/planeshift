/*
 * solid.h by Andrew Craig <andrew@hydlaa.com>
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

#ifndef SOLID_HEADER
#define SOLID_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include "csutil/scf.h"
#include "csutil/weakref.h"
#include <iengine/mesh.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================



struct iObjectRegistry;


/** Collider creation class.
  * For any objects that want to collide with other things use this class and
  * pass in the mesh to be used.
  */
class psSolid
{
public:
    /** Create a blank collider.
      * @param objectReg The Crystal Space object registry needed to create the
      *                   collider wrappers.
      */
    psSolid(iObjectRegistry* objectReg);

    ~psSolid();

    /** Set the mesh that we want to use as the colliding one.
      * @param mesh The CEL pcmesh object that has the mesh we want to use as the
      *             collding one.
      */
    void SetMesh(iMeshWrapper* mesh);

    /** Complete the setup of the class and create the collider wrappers.
     */
    void Setup();

    /** Get the collider object this class creates.
      * @return The created collider object.
      */
    iCollider* GetCollider();

private:
    csRef<iObjectRegistry> objectReg;               ///< Needed to create the collider wrapper.
    csWeakRef<iMeshWrapper> mesh;                        ///< The mesh we are using to collide with.
    csRef<csColliderWrapper> colliderWrap;          ///< The actual collider wrapper.

    bool noCollider;                                ///< true if we have a collider wrapper, false otherwise.
};

#endif
