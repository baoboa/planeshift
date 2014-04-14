/*
 * solid.cpp by Andrew Craig <andrew@hydlaa.com>
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

#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include "cstool/collider.h"
#include "iutil/objreg.h"
#include "ivaria/collider.h"

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "solid.h"


psSolid::psSolid(iObjectRegistry* object)
{
    objectReg    = object;
    noCollider   = false;
    colliderWrap = 0;
}


psSolid::~psSolid()
{
    if(colliderWrap && colliderWrap->GetObjectParent())
    {
        colliderWrap->GetObjectParent()->ObjRemove(static_cast<iObject*>(colliderWrap));
    }
}

void psSolid::SetMesh(iMeshWrapper* pcmesh)
{
    mesh = pcmesh;
    colliderWrap = 0;
    noCollider = false;
}


void psSolid::Setup()
{
    colliderWrap = 0;
    noCollider = false;
    GetCollider();
}


iCollider* psSolid::GetCollider()
{
    if(colliderWrap)
    {
        return colliderWrap->GetCollider();
    }

    if(noCollider)
    {
        return 0;
    }

    if(mesh)
    {
        csRef<iCollideSystem> collideSystem = csQueryRegistry<iCollideSystem>(objectReg);

        colliderWrap = csColliderHelper::InitializeCollisionWrapper(collideSystem, mesh);
        if(colliderWrap)
        {
            return colliderWrap->GetCollider();
        }
    }

    noCollider = true;
    return 0;
}
