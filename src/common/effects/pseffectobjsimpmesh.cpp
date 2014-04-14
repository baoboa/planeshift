/*
* Author: Andrew Robberts
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <imap/loader.h>
#include <imesh/objmodel.h>
#include <csutil/flags.h>

#include "effects/pseffectobjsimpmesh.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/pscssetup.h"
#include "util/log.h"

psEffectObjSimpMesh::psEffectObjSimpMesh(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
}

psEffectObjSimpMesh::~psEffectObjSimpMesh()
{
    if(mesh.IsValid())
    {
        engine->RemoveObject(mesh);
    }
}

bool psEffectObjSimpMesh::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{

    // get the attributes
    name.Clear();
    materialName.Clear();
    fileName.Clear();
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
        else if(attrName == "material")
            materialName = attr->GetValue();
        else if(attrName == "file")
            fileName = csString("/this/art/effects/") + attr->GetValue();
        else if(attrName == "mesh")
            meshName = attr->GetValue();
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psCSSetup::object_reg);
    assert(vfs);

    if(!vfs->Exists(fileName))
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj without specifying an existing mesh file.\n");
        return false;
    }

    if(!psEffectObj::Load(node, ldr_context))
        return false;

    return PostSetup();
}

bool psEffectObjSimpMesh::Render(const csVector3 &up)
{
    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_simpmesh_";
    effectID += nextUniqueID++;

    iMeshWrapper* templateMesh = engine->FindMeshObject(meshName);
    if(!templateMesh)
        return false;

//    csRef<iMeshObject> newObj = templateMesh->GetMeshObject()->Clone();
//    if (!newObj)
//        newObj = templateMesh->GetMeshObject();

    if(mesh.IsValid())
    {
        engine->RemoveObject(mesh);
    }
    mesh = engine->CreateMeshWrapper(templateMesh->GetMeshObject(), effectID);

    // mesh has been loaded before hand
    if(!mesh)
        return false;

    // do the up vector
    objUp = up;
    csReversibleTransform rt;
    rt.LookAt(up, csVector3(1,2,0));
    matUp = rt.GetT2O();
    matBase = matUp;

    // common flags
    mesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    mesh->SetZBufMode(zFunc);
    mesh->SetRenderPriority(priority);

    // disable culling
    csStringID viscull_id = globalStringSet->Request("viscull");
    mesh->GetMeshObject()->GetObjectModel()->SetTriangleData(viscull_id, 0);

    return true;
}

bool psEffectObjSimpMesh::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    if(keyFrames->GetSize() == 0)
        return true;

    // do stuff here

    return true;
}

psEffectObj* psEffectObjSimpMesh::Clone() const
{
    psEffectObjSimpMesh* newObj = new psEffectObjSimpMesh(view, renderer2d);
    CloneBase(newObj);

    // simp mesh specific
    newObj->fileName = fileName;
    newObj->meshName = meshName;

    return newObj;
}

bool psEffectObjSimpMesh::PostSetup()
{
    csRef<iLoader> loader = csQueryRegistry<iLoader> (psCSSetup::object_reg);
    loader->LoadLibraryFile(fileName, effectsCollection);

    return true;
}

