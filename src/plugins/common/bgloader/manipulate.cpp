/*
 *  loader.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008-10 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cssysdef.h>
#include <cstool/collider.h>
#include <cstool/enginetools.h>
#include <cstool/vfsdirchange.h>
#include <csutil/scanstr.h>
#include <csutil/scfstringarray.h>
#include <iengine/camera.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <imesh/object.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivaria/engseq.h>
#include <ivideo/graph2d.h>
#include <ivideo/material.h>

#include "util/psconst.h"
#include "loader.h"

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
    iMeshWrapper* BgLoader::CreateAndSelectMesh(const char* factName,
                                                const char* matName,
                                                iCamera* camera,
                                                const csVector2& pos)
    {
        // Check that requested mesh is valid.
        csRef<MeshFact> meshfact = parserData.factories.Get(factName);

        if(!meshfact.IsValid())
            return 0;

        // Get WS position.
        csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);

        // If there's no hit then we can't create.
        if(result.mesh == 0)
            return 0;

        // Load meshfactory.
        if(!meshfact->Load(true))
        {
            return 0;
        }
        selectedFactory = meshfact;

        csRef<iMeshFactoryWrapper> factory = meshfact->GetObject();

        // Update stored position.
        previousPosition = pos;

        // Create new mesh.
        if(selectedMesh && resetHitbeam)
        {
            selectedMesh->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
        }
        resetHitbeam = false;
        selectedMesh = factory->CreateMeshWrapper();
        selectedMesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
        selectedMesh->GetMovable()->SetPosition(camera->GetSector(), result.isect);
        selectedMesh->GetMovable()->UpdateMove();

        if(matName != NULL)
        {
            // Check that requested material is valid.
            csRef<Material> material = parserData.materials.Get(matName);

            if(material.IsValid())
            {
                if(!material->Load(true))
                {
                    engine->RemoveObject(selectedMesh);
                    selectedMesh.Invalidate();

                    selectedFactory->Unload();
                    selectedFactory.Invalidate();

                    return 0;
                }
                selectedMaterial = material;
                csRef<iMaterialWrapper> wrapper = material->GetObject();
                selectedMesh->GetMeshObject()->SetMaterialWrapper(wrapper);
            }
        }

        const csReversibleTransform& trans = selectedMesh->GetMovable()->GetTransform();

        // save original translation
        origTrans = trans.GetO2TTranslation();
        origRot = 0;

        // make it rotate around the center
        csBox3 bbox = selectedMesh->GetFactory()->GetMeshObjectFactory()->
                        GetObjectModel()->GetObjectBoundingBox();
        rotBase = (bbox.GetCenter() / trans) - trans.GetOrigin();

        return selectedMesh;
    }

    iMeshWrapper* BgLoader::SelectMesh(iCamera* camera, const csVector2& pos)
    {
        // Get WS position.
        csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);

        // Reset flags.
        if(selectedMesh && resetHitbeam)
        {
            selectedMesh->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
        }

        // Get new selected mesh.
        selectedMesh = result.mesh;
        if(selectedMesh)
        {
            resetHitbeam = !selectedMesh->GetFlags().Check(CS_ENTITY_NOHITBEAM);
            selectedMesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
        }

        // Update stored position.
        previousPosition = pos;

        return selectedMesh;
    }

    bool BgLoader::TranslateSelected(bool vertical, iCamera* camera, const csVector2& pos)
    {
        if(selectedMesh.IsValid())
        {
            csVector3 diff = 0;
            if(vertical)
            {
                float d = 5 * float(previousPosition.y - pos.y) / g2d->GetWidth();
                diff = csVector3(0.0f, d, 0.0f);
            }
            else
            {
                // Get WS position.
                csScreenTargetResult result = csEngineTools::FindScreenTarget(pos, 1000, camera);
                if(result.mesh)
                {
                    diff = result.isect - origTrans;
                }
            }

            previousPosition = pos;
            if(diff != 0)
            {
                origTrans += diff;
                selectedMesh->GetMovable()->GetTransform().Translate(diff);
                return true;
            }
        }

        return false;
    }

    void BgLoader::RotateSelected(const csVector2& pos)
    {
        if(selectedMesh.IsValid())
        {
            float factor_h = 6 * PI * ((float)previousPosition.x - pos.x) / g2d->GetHeight();
            float factor_v = 6 * PI * ((float)previousPosition.y - pos.y) / g2d->GetHeight();
            origRot += factor_h*currRot_h + factor_v*currRot_v;

            csYRotMatrix3 pitch(origRot.x);
            csYRotMatrix3 roll(origRot.y);
            csZRotMatrix3 yaw(origRot.z);

            csReversibleTransform trans(roll*yaw, rotBase);
            trans *= csReversibleTransform(pitch, -rotBase+origTrans);

            selectedMesh->GetMovable()->SetTransform(trans);
            previousPosition = pos;
        }
    }

    void BgLoader::SetRotation(int flags_h, int flags_v)
    {
        currRot_h.x = (flags_h & PS_MANIPULATE_PITCH) ? 1 : 0;
        currRot_h.y = (flags_h & PS_MANIPULATE_ROLL ) ? 1 : 0;
        currRot_h.z = (flags_h & PS_MANIPULATE_YAW  ) ? 1 : 0;
        currRot_v.x = (flags_v & PS_MANIPULATE_PITCH) ? 1 : 0;
        currRot_v.y = (flags_v & PS_MANIPULATE_ROLL ) ? 1 : 0;
        currRot_v.z = (flags_v & PS_MANIPULATE_YAW  ) ? 1 : 0;
    }

    void BgLoader::RemoveSelected()
    {
        if(selectedMesh.IsValid())
        {
            selectedMesh->GetMovable()->SetSector(0);
            selectedMesh.Invalidate();
        }

        if(selectedMaterial.IsValid())
        {
            selectedMaterial->Unload();
            selectedMaterial.Invalidate();
        }

        if(selectedFactory.IsValid())
        {
            selectedFactory->Unload();
            selectedFactory.Invalidate();
        }
    }

    void BgLoader::GetPosition(csVector3 & pos, csVector3 & rot, const csVector2& /*screenPos*/)
    {
        if(selectedMesh.IsValid())
        {
           rot = origRot;
           pos = origTrans;
        }
        else
        {
           rot = 0;
           pos = 0;
        }
    }
}
CS_PLUGIN_NAMESPACE_END(bgLoader)
