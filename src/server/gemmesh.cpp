/*
* gemmesh.h by Andrew Craig <andrew@hydlaa.com>
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
#include "iengine/engine.h"
#include "iengine/mesh.h"
#include "iengine/movable.h"
#include "iutil/vfs.h"
#include "iutil/objreg.h"
#include "imap/loader.h"

//=============================================================================
// PlaneShift Forward Definitions
//=============================================================================
#include "gemmesh.h"
#include "gem.h"

gemMesh::gemMesh(iObjectRegistry* objreg, gemObject* owner, GEMSupervisor* super)
{
    objectReg = objreg;
    gemOwner = owner;
    gem = super;

    engine = csQueryRegistry<iEngine>(objectReg);
}

gemMesh::~gemMesh()
{
    RemoveMesh();
}


iMeshFactoryWrapper* gemMesh::LoadMeshFactory(const char* fileName, const char* factoryName)
{
    csRef<iVFS> vfs = csQueryRegistry<iVFS>(objectReg);

    csRef<iThreadedLoader> loader = csQueryRegistry<iThreadedLoader>(objectReg);

    csRef<iThreadReturn> ret = loader->LoadFileWait(vfs->GetCwd(), fileName);

    csRef<iMeshFactoryWrapper> imesh_fact = 0;

    if(ret->WasSuccessful())
    {
        if(!ret->GetResultRefPtr().IsValid())
        {
            imesh_fact = engine->FindMeshFactory(factoryName);
        }
        else
        {
            imesh_fact = scfQueryInterface<iMeshFactoryWrapper>(ret->GetResultRefPtr());
        }
    }

    return imesh_fact;
}


bool gemMesh::SetMesh(const char* factoryName, const char* fileName)
{
    bool result = false;
    RemoveMesh();

    csRef<iMeshFactoryWrapper> mesh_fact = engine->GetMeshFactories()->FindByName(factoryName);

    if(!mesh_fact)
    {
        mesh_fact = LoadMeshFactory(fileName, factoryName);
    }

    if(mesh_fact)
    {
        mesh = engine->CreateMeshWrapper(mesh_fact ,factoryName);
        gem->AttachObject(mesh->QueryObject(), gemOwner);
        result = true;
    }

    return result;
}

iMeshWrapper* gemMesh::GetMesh()
{
    return mesh;
}

void gemMesh::SetMesh(iMeshWrapper* newMesh)
{
    RemoveMesh();
    mesh = newMesh;

    if(newMesh)
    {
        gem->AttachObject(newMesh->QueryObject(), gemOwner);
    }
}

void gemMesh::RemoveMesh()
{
    if(mesh)
    {
        gem->UnattachObject(mesh->QueryObject(), gemOwner);
        engine->RemoveObject(mesh);
        mesh = 0;
    }
}

void gemMesh::MoveMesh(iSector* sector, const float yrot, const csVector3 &position)
{
    if(mesh)
    {
        if(sector)
        {
            mesh->GetMovable()->SetSector(sector);
        }

        mesh->GetMovable()->SetPosition(position);
    }

    csMatrix3 matrix = (csMatrix3) csYRotMatrix3(yrot);
    mesh->GetMovable()->GetTransform().SetO2T(matrix);

    mesh->GetMovable()->UpdateMove();
}

