/*
  Crystal Space Entity Layer
  Copyright (C) 2001 PlaneShift Team (info@planeshift.it,
  Copyright (C) 2001-2003 by Jorrit Tyberghein

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
 * This code is heavily based on pslinmove from the PlaneShift project.
 * Thanks a lot for making this!
 */

#ifndef __CEL_PF_COLL_DET_FACT__
#define __CEL_PF_COLL_DET_FACT__

//CS Includes
#include <cstypes.h>
#include <iutil/comp.h>
#include <iutil/eventh.h>
#include <csutil/scf.h>
#include <csutil/stringarray.h>
#include <csutil/sysfunc.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>


struct iObjectRegistry;
struct iVirtualClock;
struct iCollider;
struct iCollideSystem;
struct iSector;
struct iPath;

class csReversibleTransform;
class csObject;
class csColliderWrapper;


/**
 * Collision Detection (CD) related class.
 * This class handles CD for linearmovement PC.
 */
class psCollisionDetection
{
public:
    psCollisionDetection(iObjectRegistry* object_reg);
    virtual ~psCollisionDetection();

    virtual bool Init(const csVector3 &body, const csVector3 &legs, const csVector3 &shift, iMeshWrapper* mesh);

    virtual bool IsOnGround() const;
    virtual void SetOnGround(bool flag);
    virtual void UseCD(bool flag);

    virtual bool AdjustForCollisions(csVector3 &oldpos,
                                     csVector3 &newpos,
                                     csVector3 &vel,
                                     float delta,
                                     iMovable* movable);

protected:
    csRef<iMeshWrapper> mesh;

    csColliderActor colliderActor;

    bool useCD;           ///< Enable collision detection flag.

    //Collision vars
    csVector3 shift;
    csVector3 topSize;
    csVector3 bottomSize;

    // Try to find the collider for an object. Can return 0 if there is none.
    // This function uses csColliderWrapper if present. If not it will try
    // if there is a pcsolid to get a collider from.
    iCollider* FindCollider(iObject* object);

};

#endif

