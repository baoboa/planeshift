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
#include <imesh/nullmesh.h>

#include "effects/pseffectanchorbasic.h"

#include "util/pscssetup.h"
#include "util/log.h"

psEffectAnchorBasic::psEffectAnchorBasic()
    :psEffectAnchor()
{
}

psEffectAnchorBasic::~psEffectAnchorBasic()
{
}

bool psEffectAnchorBasic::Load(iDocumentNode* node)
{
    // get the attributes
    name.Clear();
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect relative anchor with no name.\n");
        return false;
    }

    if(!psEffectAnchor::Load(node))
        return false;

    return true;
}

bool psEffectAnchorBasic::Create(const csVector3 &offset, iMeshWrapper* /*posAttach*/, bool rotateWithMesh)
{
    static unsigned long nextUniqueID = 0;
    csString anchorID = "effect_anchor_basic_";
    anchorID += nextUniqueID++;

    objBasePos = offset;
    objOffset = csVector3(0,0,0);
    this->rotateWithMesh = rotateWithMesh;

    mesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", anchorID);
    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (mesh->GetMeshObject());
    if(!state)
    {
        Error1("No NullMeshState.");
        return false;
    }
    state->SetRadius(1.0);

    isReady = true;

    return true;
}

bool psEffectAnchorBasic::Update(csTicks elapsed)
{
    if(!psEffectAnchor::Update(elapsed))
        return false;

    // do stuff here

    return true;
}

psEffectAnchor* psEffectAnchorBasic::Clone() const
{
    psEffectAnchorBasic* newObj = new psEffectAnchorBasic();
    CloneBase(newObj);

    // basic anchor specific

    return newObj;
}
