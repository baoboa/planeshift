/*
* waypoint.cpp
*
* Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <iutil/objreg.h>
#include <iutil/object.h>
#include <csutil/csobject.h>
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iengine/engine.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/waypoint.h"
#include "util/edge.h"
#include "util/location.h"
#include "util/log.h"
#include "util/psstring.h"
#include "util/strutil.h"
#include "util/psutil.h"

//-----------------------------------------------------------------------------
WaypointAlias::WaypointAlias(Waypoint* wp, int id, const csString& alias, float rotationAngle)
    :wp(wp),id(id),alias(alias),rotationAngle(rotationAngle)
{
}

bool WaypointAlias::CreateUpdate(iDataConnection* db)
{
    const char * fields[] = 
        {
            "wp_id",
            "alias",
            "rotation_angle"};

    psStringArray values;
    values.FormatPush("%d",wp->GetID());
    values.FormatPush("%s",alias.GetDataSafe());
    values.FormatPush("%.1f",rotationAngle*180.0/PI);

    if (id == -1)
    {
        id = db->GenericInsertWithID("sc_waypoint_aliases",fields,values);
        if (id == 0)
        {
            id = -1;
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_waypoint_aliases","id",idStr,fields,values);    
    }

    return true;
}

bool WaypointAlias::SetRotationAngle(iDataConnection* db, float rotationAngle)
{
    SetRotationAngle(rotationAngle);

    return CreateUpdate(db);
}

//-----------------------------------------------------------------------------

Waypoint::Waypoint()
    :effectID(0)
{
    distance = 0.0;
    pi = NULL;
    loc.id = -1;
}

Waypoint::Waypoint(const char* name)
    :effectID(0)
{
    distance = 0.0;
    pi = NULL;
    loc.id = -1;
    loc.name = name;
}

Waypoint::Waypoint(csString& name, 
                   csVector3& pos, csString& sectorName,
                   float radius, csString& flags)
    :effectID(0)
{
    distance = 0.0;
    pi = NULL;
    loc.id = -1;
    loc.name = name;
    loc.pos = pos;
    loc.sectorName = sectorName;
    loc.sector = NULL;
    loc.radius = radius;
    loc.rot_angle = 0.0;
    
    SetFlags(flags);
}

bool Waypoint::Load(iDocumentNode *node, iEngine * engine)
{
    loc.id = 0;
    loc.name = node->GetAttributeValue("name");
    loc.pos.x = node->GetAttributeValueAsFloat("x");
    loc.pos.y = node->GetAttributeValueAsFloat("y");
    loc.pos.z = node->GetAttributeValueAsFloat("z");
    loc.sectorName = node->GetAttributeValue("sector");
    
    if ( loc.sectorName.Length() > 0 )
        loc.sector = engine->FindSector(loc.sectorName);

    loc.radius = node->GetAttributeValueAsFloat("radius");
    loc.rot_angle = 0.0;
    allowReturn = node->GetAttributeValueAsBool("allow_return",false); // by default don't allow return to previous point

    return (loc.name.Length()>0);
}

bool Waypoint::Import(iDocumentNode *node, iEngine * engine, iDataConnection *db)
{
    loc.name = node->GetAttributeValue("name");
    loc.pos.x = node->GetAttributeValueAsFloat("x");
    loc.pos.y = node->GetAttributeValueAsFloat("y");
    loc.pos.z = node->GetAttributeValueAsFloat("z");
    loc.sectorName = node->GetAttributeValue("sector");
    
    if ( loc.sectorName.Length() > 0 )
        loc.sector = engine->FindSector(loc.sectorName);

    loc.radius = node->GetAttributeValueAsFloat("radius");
    loc.rot_angle = 0.0;
    allowReturn = node->GetAttributeValueAsBool("allow_return",false); // by default don't allow return to previous point


    const char * fields[] = 
        {"name","x","y","z","loc_sector_id","radius","flags"};
    psStringArray values;
    values.Push(loc.name);
    values.FormatPush("%.2f",loc.pos.x);
    values.FormatPush("%.2f",loc.pos.y);
    values.FormatPush("%.2f",loc.pos.z);
    values.FormatPush("%d",loc.GetSectorID(db,loc.sectorName));
    values.FormatPush("%.2f",loc.radius);
    values.Push(GetFlags());

    if (loc.id == -1)
    {
        loc.id = db->GenericInsertWithID("sc_waypoints",fields,values);
        if (loc.id == 0)
        {
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",loc.id);
        return db->GenericUpdateWithID("sc_waypoints","id",idStr,fields,values);    
    }

    return true;
}

bool Waypoint::Load(iResultRow& row, iEngine *engine)
{
    loc.id         = row.GetInt("id");
    loc.name       = row["name"];
    loc.pos.x      = row.GetFloat("x");
    loc.pos.y      = row.GetFloat("y");
    loc.pos.z      = row.GetFloat("z");
    loc.sectorName = row["sector"];
    loc.sector     = engine->FindSector(loc.sectorName);
    loc.radius     = row.GetFloat("radius");
    loc.rot_angle  = 0.0;
    group          = row["wp_group"];

    SetFlags(row["flags"]);

    return true;
}

void Waypoint::AddLink(psPath * path, Waypoint * wp, psPath::Direction direction, float distance)
{
    links.Push(wp);
    edges.Push(new Edge(path, direction));
    paths.Push(path);
    dists.Push(distance);
}

void Waypoint::RemoveLink(psPath * path)
{
    size_t index = paths.Find(path);
    if (index != csArrayItemNotFound)
    {
        links.DeleteIndexFast(index);
        edges.DeleteIndexFast(index);
        paths.DeleteIndexFast(index);
        dists.DeleteIndexFast(index);
    }
}

Edge* Waypoint::GetRandomEdge(const psPathNetwork::RouteFilter* routeFilter)
{
    // If there are only one way out don't bother to find if its legal
    if (edges.GetSize() == 1)
    {
        return edges[0];
    }
    else if (edges.GetSize() == 0)
    {
        return NULL; // No point in trying to find a way out here
    }

    csArray<Edge*> candidateEdges; // List of available waypoints

    // Calculate possible edges
    for (size_t ii = 0; ii < edges.GetSize(); ii++)
    {
        Edge *edge = edges[ii];

        if ( (!edge->NoWander()) && (!routeFilter->Filter(edge->GetEndWaypoint())) )
        {
            candidateEdges.Push(edge);
        }
    }

    // If no path out of is possible to go, just return.
    if (candidateEdges.GetSize() == 0)
    {
        return edges[0];
    }
    

    return candidateEdges[psGetRandom((int)candidateEdges.GetSize())];
}


WaypointAlias* Waypoint::AddAlias(int id, csString aliasName, float rotationAngle)
{
    WaypointAlias* alias = new WaypointAlias(this, id, aliasName, rotationAngle);
    aliases.Push( alias );
    return alias;
}

void Waypoint::RemoveAlias(csString aliasName)
{
    // Check for aliases
    for (size_t i = 0; i < aliases.GetSize(); i++)
    {
        if (aliasName.CompareNoCase(aliases[i]->alias))
        {
            aliases.DeleteIndexFast(i);
            return;
        }
    }
}

void Waypoint::SetFlags(const csString & flagstr)
{
    allowReturn  = isFlagSet(flagstr,"ALLOW_RETURN");
    underground  = isFlagSet(flagstr,"UNDERGROUND");
    underwater   = isFlagSet(flagstr,"UNDERWATER");
    priv         = isFlagSet(flagstr,"PRIVATE");
    pub          = isFlagSet(flagstr,"PUBLIC");
    city         = isFlagSet(flagstr,"CITY");
    indoor       = isFlagSet(flagstr,"INDOOR");
    path         = isFlagSet(flagstr,"PATH");
    road         = isFlagSet(flagstr,"ROAD");
    ground       = isFlagSet(flagstr,"GROUND");
}

bool Waypoint::SetFlag(iDataConnection * db, const csString &flagstr, bool enable)
{
    if (!SetFlag(flagstr, enable))
    {
        return false;
    }

    int result = db->CommandPump("UPDATE sc_waypoints SET flags='%s' WHERE id=%d",
                                 GetFlags().GetDataSafe(), loc.id);
    if (result != 1)
    {
        Error2("Sql failed: %s\n",db->GetLastError());
        return false;
    }
    
    return true;
}

bool Waypoint::SetFlag(const csString &flagstr, bool enable)
{
    if (isFlagSet(flagstr,"ALLOW_RETURN"))
    {
        allowReturn = enable;
    }
    else if (isFlagSet(flagstr,"UNDERGROUND"))
    {
        underground = enable;
    }
    else if (isFlagSet(flagstr,"UNDERWATER"))
    {
        underwater = enable;
    }
    else if (isFlagSet(flagstr,"PRIVATE"))
    {
        priv = enable;
    }
    else if (isFlagSet(flagstr,"PUBLIC"))
    {
        pub = enable;
    }
    else if (isFlagSet(flagstr,"CITY"))
    {
        city = enable;
    }
    else if (isFlagSet(flagstr,"INDOOR"))
    {
        indoor = enable;
    }
    else if (isFlagSet(flagstr,"PATH"))
    {
        path = enable;
    }
    else if (isFlagSet(flagstr,"ROAD"))
    {
        road = enable;
    }
    else if (isFlagSet(flagstr,"GROUND"))
    {
        ground = enable;
    }
    else
    {
        return false;
    }
    return true;
}

csString Waypoint::GetFlags() const
{
    csString flagStr;
    bool added = false;
    if (allowReturn)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("ALLOW_RETURN");
        added = true;
    }
    if (underground)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("UNDERGROUND");
        added = true;
    }
    if (underwater)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("UNDERWATER");
        added = true;
    }
    if (priv)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("PRIVATE");
        added = true;
    }
    if (pub)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("PUBLIC");
        added = true;
    }
    if (city)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("CITY");
        added = true;
    }
    if (indoor)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("INDOOR");
        added = true;
    }
    if (path)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("PATH");
        added = true;
    }
    if (road)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("ROAD");
        added = true;
    }
    if (ground)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("GROUND");
        added = true;
    }
    return flagStr;
}

bool Waypoint::SetRadius(iDataConnection * db, float radius)
{
    int res =db->Command("update sc_waypoints set radius='%f' where id=%d",
                         radius, GetID());
    if (res != 1)
    {
        return false;
    } 
   
    return SetRadius( radius );
}

bool Waypoint::SetRadius(float radius)
{
    loc.radius = radius;

    return true;
}

void Waypoint::RecalculateEdges(psWorld * world, iEngine *engine)
{
    // Recalculate radius for each of the path points connected
    for (size_t ii = 0; ii < edges.GetSize(); ii++)
    {
        Edge *edge = edges[ii];

        edge->GetPath()->Precalculate(world, engine, true);
    }
}



bool Waypoint::Rename(iDataConnection * db,const char* name)
{
    int res =db->Command("update sc_waypoints set name='%s' where id=%d",
                         name,GetID());
    if (res != 1)
    {
        return false;
    } 
   
    Rename(name);

    return true;
}

void Waypoint::Rename(const char* name)
{
    loc.name = name;
}

csString Waypoint::GetAliases()
{
    csString str;
    
    for (size_t i = 0; i < aliases.GetSize(); i++)
    {
        if (i != 0) str.Append(", ");
        str.AppendFmt("%s(%.1f)",aliases[i]->GetName(),aliases[i]->GetRotationAngle()*180.0/PI);
    }
    return str;
}

const WaypointAlias* Waypoint::FindAlias(const csString& aliasName) const
{
    // Check for aliases
    for (size_t i = 0; i < aliases.GetSize(); i++)
    {
        if (aliasName.CompareNoCase(aliases[i]->alias))
        {
            return aliases[i];
        }
    }
    return NULL;
}

WaypointAlias* Waypoint::FindAlias(const csString& aliasName)
{
    // Check for aliases
    for (size_t i = 0; i < aliases.GetSize(); i++)
    {
        if (aliasName.CompareNoCase(aliases[i]->alias))
        {
            return aliases[i];
        }
    }
    return NULL;
}

    
bool Waypoint::SetRotationAngle(const csString& aliasName, float rotationAngle)
{
    WaypointAlias* alias = FindAlias(aliasName);
    if (alias)
    {
        alias->SetRotationAngle(rotationAngle);
    }
    else
    {
        return false;
    }
    return true;
}



int Waypoint::Create(iDataConnection *db)
{
    const char *fieldnames[]=
        {
            "name",
            "wp_group",
            "x",
            "y",
            "z",
            "radius",
            "flags",
            "loc_sector_id"
        };

    psStringArray values;
    values.FormatPush("%s", loc.name.GetDataSafe());
    values.FormatPush("%s", group.GetDataSafe());
    values.FormatPush("%10.2f",loc.pos.x);
    values.FormatPush("%10.2f",loc.pos.y);
    values.FormatPush("%10.2f",loc.pos.z);
    values.FormatPush("%10.2f",loc.radius);
    values.FormatPush("%s",GetFlags().GetDataSafe());
    values.FormatPush("0");

    loc.id = db->GenericInsertWithID("sc_waypoints",fieldnames,values);

    if (loc.id==0)
    {
        Error2("Failed to create new WP Error %s",db->GetLastError());
        return -1;
    }

    psString cmd;
    cmd.Format("update sc_waypoints set loc_sector_id=(select id from sectors where name='%s') where id=%d",
               loc.sectorName.GetDataSafe(),loc.id);
    db->Command(cmd);
    
    return loc.id;
}

WaypointAlias* Waypoint::CreateAlias(iDataConnection * db, csString aliasName, float rotationAngle)
{
    WaypointAlias* alias = AddAlias(-1, aliasName, rotationAngle);

    if (alias->CreateUpdate(db))
    {
        return alias;
    }
    
    RemoveAlias(aliasName);

    return NULL;
}

bool Waypoint::RemoveAlias(iDataConnection * db, csString alias)
{
    int res =db->Command("delete from sc_waypoint_aliases where wp_id='%d' and alias='%s'",
                         GetID(),alias.GetDataSafe());
    if (res != 1)
    {
        return false;
    }
    
    RemoveAlias(alias);
    return true;
}

void Waypoint::SetID(int id)
{
    loc.id = id;
}


bool Waypoint::Adjust(iDataConnection * db, csVector3 & pos, csString sector)
{
    int result = db->CommandPump("UPDATE sc_waypoints SET x=%.2f,y=%.2f,z=%.2f,"
                                 "loc_sector_id=(select id from sectors where name='%s') WHERE id=%d",
                                 pos.x,pos.y,pos.z,sector.GetDataSafe(),loc.id);

    Adjust(pos, sector);
    return (result == 1);
}

void Waypoint::Adjust(csVector3 & pos, csString sector)
{
    loc.pos = pos;
    loc.sectorName = sector;
    loc.sector = NULL;

    for (size_t i=0; i<edges.GetSize(); i++)
    {
        psPathPoint * pp = edges[i]->GetStartPoint();
        pp->Adjust(pos,sector);
    }
}


void Waypoint::Adjust(csVector3 & pos, iSector* sector)
{
    Adjust(pos, sector->QueryObject()->GetName());
    loc.sector = sector;
}


bool Waypoint::CheckWithin(iEngine * engine, const csVector3& pos, const iSector* sector)
{
    if (sector == loc.GetSector(engine))
    {
        float range = (loc.pos - pos).Norm();
        if (range <= loc.radius)
        {
            return true;
        }
    }
    
    return false;
}

uint32_t Waypoint::GetEffectID(iEffectIDAllocator* allocator)
{
    if (effectID <= 0)
    {
        effectID = allocator->GetEffectID();
    }
    
    return effectID;
}

