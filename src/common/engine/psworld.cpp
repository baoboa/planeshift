/*
 * psworld.cpp - author Matze Braun <MatzeBraun@gmx.de> and
 *              Keith Fulton <keith@paqrat.com>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <cstool/collider.h>
#include <csutil/scfstringarray.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <iengine/portalcontainer.h>
#include <imesh/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <ibgloader.h>
#include "util/psconst.h"
#include "util/consoleout.h"
#include "util/log.h"
#include "util/strutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psworld.h"

psWorld::psWorld()
{
    regions.AttachNew(new scfStringArray());
}

psWorld::~psWorld()
{
    transarray.Empty();
}

bool psWorld::Initialize(iObjectRegistry* objectReg)
{
    object_reg = objectReg;
    engine = csQueryRegistry<iEngine>(object_reg);
    loader = csQueryRegistry<iBgLoader>(object_reg);

    return true;
}

bool psWorld::NewRegion(const char* mapfile)
{
    regions->Push(mapfile);
    if(!loader->LoadZones(regions))
    {
        // Oh no, failed so don't try again!
        regions->Pop();
        return false;
    }

    // This must be rebuilt when the sector list changes
    BuildWarpCache();

    return true;
}

void psWorld::GetAllRegionNames(csString &str)
{
    str.Clear();
    for(unsigned i=0; i < regions->GetSize(); i++)
    {
        str.Append(regions->Get(i));
        str.Append("|");
    }
}

void psWorld::BuildWarpCache()
{
    int sectorCount = engine->GetSectors()->GetCount();
    Debug2(LOG_STARTUP,0,"Building warp cache for %d sectors...",sectorCount);

    /// Clear existing entries
    transarray.Empty();

    transarray.SetSize(sectorCount);

    for(int i=0; i<sectorCount; i++)
    {
        const csSet<csPtrKey<iMeshWrapper> > &portals = engine->GetSectors()->Get(i)->GetPortalMeshes();
        Debug3(LOG_STARTUP,0," %zu portal meshes for %s",portals.GetSize(),
               engine->GetSectors()->Get(i)->QueryObject()->GetName());

        csSet<csPtrKey<iMeshWrapper> >::GlobalIterator it = portals.GetIterator();
        while(it.HasNext())
        {
            iMeshWrapper* portal_mesh = it.Next();
            iPortalContainer* pc = portal_mesh->GetPortalContainer();
            for(int j = 0 ; j < pc->GetPortalCount() ; j++)
            {
                iPortal* portal = pc->GetPortal(j);
                if(portal->CompleteSector(0))
                {
                    if(engine->GetSectors()->Find(portal->GetSector()) != -1)
                    {
                        const csReversibleTransform warp = portal->GetWarp();

                        // Check the values and issue warning for strange values.
                        csVector3 v_o2t = warp.GetO2TTranslation();
                        if(fabs(v_o2t.x) > 1.0e+10 || fabs(v_o2t.y) > 1.0e+10 || fabs(v_o2t.z) > 1.0e+10)
                        {
                            Error3("Warpspace with very large o2t translation:  %s: %s",
                                   portal->GetName(),warp.Description().GetData());
                        }

                        // Apply the warp to the chache
                        transarray[i].Set(portal->GetSector(), warp);
                    }
                }
            }
        }
    }
}

void psWorld::DumpWarpCache()
{
    for(size_t i=0; i<transarray.GetSize(); i++)
    {
        csHash<csReversibleTransform*, csPtrKey<iSector> >::GlobalIterator it =  transarray[i].GetIterator();
        iSector* fromSector = engine->GetSectors()->Get((int)i);
        CPrintf(CON_CMDOUTPUT,"%s\n",fromSector->QueryObject()->GetName());
        while(it.HasNext())
        {
            csPtrKey<iSector>  sector;
            csReversibleTransform* rt = it.Next(sector);
            CPrintf(CON_CMDOUTPUT,"  %-20s : %s\n",sector->QueryObject()->GetName(),toString(*rt).GetData());

        }
    }
}


bool psWorld::WarpSpace(const iSector* from, const iSector* to, csVector3 &pos)
{
    if(from == to)
        return true; // No need to transform, pos ok.

    int i = engine->GetSectors()->Find((iSector*)from);
    if(i == -1)
    {
        return false; // Didn't find transformation, pos not ok.
    }

    csReversibleTransform* transform = transarray[i].Get((iSector*)to);
    if(transform)
    {
        pos = *transform * pos;
        return true; // Position transformed, pos ok.
    }

    return false; // Didn't find transformation, pos not ok.
}

bool psWorld::WarpSpaceRelative(const iSector* from, const iSector* to, csVector3 &pos)
{
    if(from == to)
        return true; // No need to transform, pos ok.

    int i = engine->GetSectors()->Find((iSector*)from);
    if(i == -1)
    {
        return false; // Didn't find transformation, pos not ok.
    }

    csReversibleTransform* transform = transarray[i].Get((iSector*)to);
    if(transform)
    {
        pos = transform->Other2ThisRelative(pos);
        return true; // Position transformed, pos ok.
    }

    return false; // Didn't find transformation, pos not ok.
}

bool psWorld::Connected(const iSector* from, const iSector* to)
{
    if(from == to)
        return true;

    int i = engine->GetSectors()->Find((iSector*)from);
    if(i == -1)
        return false;

    if(transarray.GetSize() == 0)
        return true;

    csReversibleTransform* transform = transarray[i].Get((iSector*)to);
    if(transform)
        return true;

    return false;
}

float psWorld::Distance(const csVector3 &from_pos, const iSector* from_sector, csVector3 to_pos, const iSector* to_sector)
{
    if(from_sector == to_sector)
    {
        return (from_pos - to_pos).Norm();
    }
    else
    {
        if(WarpSpace(to_sector, from_sector, to_pos))
        {
            return (from_pos - to_pos).Norm();
        }
        else
        {
            return INFINITY_DISTANCE; // No transformation found, so just set larg distance.
        }

    }
}

float psWorld::Distance2(const csVector3 &from_pos, const iSector* from_sector, csVector3 to_pos, const iSector* to_sector)
{
    if(from_sector == to_sector)
    {
        float dx = from_pos.x - to_pos.x;
        float dz = from_pos.z - to_pos.z;
        return sqrt(dx*dx + dz*dz);
    }
    else
    {
        if(WarpSpace(to_sector, from_sector, to_pos))
        {
            float dx = from_pos.x - to_pos.x;
            float dz = from_pos.z - to_pos.z;
            return sqrt(dx*dx + dz*dz);
        }
        else
        {
            return INFINITY_DISTANCE; // No transformation found, so just set larg distance.
        }
    }
}


float psWorld::Distance(iMeshWrapper* ent1, iMeshWrapper* ent2)
{
    csVector3 pos1,pos2;
    iSector* sector1,*sector2;


    GetPosition(ent1,pos1,NULL,sector1);
    GetPosition(ent2,pos2,NULL,sector2);

    return Distance(pos1,sector1,pos2,sector2);
}



void psWorld::GetPosition(iMeshWrapper* pcmesh, csVector3 &pos, float* yrot,iSector* &sector)
{
    pos = pcmesh->GetMovable()->GetPosition();

    // rotation
    if(yrot)
    {
        csMatrix3 transf = pcmesh->GetMovable()->GetTransform().GetT2O();
        *yrot = Matrix2YRot(transf);
    }

    // Sector
    if(pcmesh->GetMovable()->GetSectors()->GetCount())
    {
        sector = pcmesh->GetMovable()->GetSectors()->Get(0);
    }
    else
    {
        sector = NULL;
    }
}


float psWorld::Matrix2YRot(const csMatrix3 &mat)
{
    csVector3 vec(0,0,1);
    vec = mat * vec;

    float result = atan2(vec.x, vec.z);
    // force the result in range [0;2*pi]
    if(result < 0) result += TWO_PI;

    return result;
}

csVector3 psWorld::Matrix2Rot(const csMatrix3 &mat)
{
    csVector3 rot(0);

    // obtain yaw
    csVector3 vec(0,1,0);
    vec = mat * vec;
    rot.z = acos(vec.y);

    // obtain roll
    rot.y = atan2(vec.z/sin(rot.z), -vec.x/sin(rot.z));

    // obtain pitch
    vec = mat * csVector3(0,0,1);
    vec = csZRotMatrix3(-rot.z) * csYRotMatrix3(-rot.y) * vec; // reverse roll and yaw here to ease our job
    rot.x = atan2(vec.x, vec.z);

    // force results in [0;2*PI] range
    rot.x = rot.x < 0 ? rot.x + TWO_PI : rot.x;
    rot.y = rot.y < 0 ? rot.y + TWO_PI : rot.y;
    rot.z = rot.z < 0 ? rot.z + TWO_PI : rot.z;
    return rot;
}

