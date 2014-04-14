/*
  Crystal Space Entity Layer
  Copyright (C) 2001 PlaneShift Team <info@planeshift.it>
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

#include <cssysdef.h>

//CS includes
#include <iutil/objreg.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/evdefs.h>
#include <iutil/virtclk.h>

#include <imesh/sprite3d.h>

#include <csutil/databuf.h>
#include <csutil/plugmgr.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>

#include <imesh/terrain.h>
#include <imesh/objmodel.h>


#include "colldet.h"


//----------------------------------------------------------------------------

psCollisionDetection::psCollisionDetection(iObjectRegistry* object_reg)
{
    csRef<iCollideSystem> cdsys = csQueryRegistry<iCollideSystem> (object_reg);
    if(!cdsys)
    {
        return;
    }
    colliderActor.SetCollideSystem(cdsys);
    colliderActor.SetGravity(19.2f);

    csRef<iEngine> engine = csQueryRegistry<iEngine> (object_reg);
    if(!engine)
    {
        return;
    }
    colliderActor.SetEngine(engine);

    mesh = 0;
}

psCollisionDetection::~psCollisionDetection()
{

}



bool psCollisionDetection::AdjustForCollisions(csVector3 &oldpos,
        csVector3 &newpos,
        csVector3 &vel,
        float delta,
        iMovable* /*movable*/)
{
    if(!useCD)
    {
        return true;
    }

    return colliderActor.AdjustForCollisions(oldpos, newpos, vel, delta);
}



bool psCollisionDetection::Init(const csVector3 &body, const csVector3 &legs, const csVector3 &_shift, iMeshWrapper* meshWrap)
{
    mesh = meshWrap;

    if(!mesh)
    {
        return false;
    }

    topSize = body;
    bottomSize = legs;
    psCollisionDetection::shift = _shift;
    colliderActor.InitializeColliders(mesh, legs, body, shift);
    useCD = true;
    return true;
}

iCollider* psCollisionDetection::FindCollider(iObject* object)
{
    csColliderWrapper* wrap = csColliderWrapper::GetColliderWrapper(object);
    if(wrap)
    {
        return wrap->GetCollider();
    }
    return 0;
}

bool psCollisionDetection::IsOnGround() const
{
    return colliderActor.IsOnGround();
}

void psCollisionDetection::SetOnGround(bool flag)
{
    colliderActor.SetOnGround(flag);
}

void psCollisionDetection::UseCD(bool flag)
{
    useCD = flag;
}


