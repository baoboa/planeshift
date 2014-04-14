/*
* location.cpp - author: Keith Fulton <keith@paqrat.com>
*
* Copyright (C) 2004,2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <iutil/objreg.h>
#include <iutil/object.h>
#include <csutil/csobject.h>
#include <iutil/vfs.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <iengine/engine.h>

#include "location.h"
#include "util/consoleout.h"
#include "util/log.h"
#include "util/psstring.h"
#include "util/psutil.h"
#include "util/pspath.h"
#include "engine/psworld.h"
#include "util/strutil.h"

/*------------------------------------------------------------------*/

Location::Location()
    :type(NULL),effectID(0),region(NULL)
{

}

Location::Location(LocationType* locationType, const char* locationName, const csVector3 &pos, iSector* sector, float radius, float rot_angle, const csString &flags)
    :id(-1),name(locationName),pos(pos),rot_angle(rot_angle),radius(radius),
     id_prev_loc_in_region(-1),sector(sector),
     type(locationType),effectID(0),region(NULL)
{
    sectorName = sector->QueryObject()->GetName();
    type->AddLocation(this);
}


Location::~Location()
{
    if(type)
    {
        type->RemoveLocation(this);
    }

    while(locs.GetSize())
    {
        Location* loc = locs.Pop();
        // First location in a region is the location that holds the locs array.
        // The first location is in the locs as well so don't delete self yet.
        if(loc != this)
        {
            delete loc;
        }
    }
}

bool Location::SetRadius(iDataConnection* db, float radius)
{
    SetRadius(radius);

    return CreateUpdate(db);
}

void Location::SetRadius(float radius)
{
    this->radius = radius;

    CalculateBoundingBox();
}

const char* Location::GetTypeName() const
{
    return type->GetName();
}

iSector* Location::GetSector(iEngine* engine)
{
    // Return cached value
    if(sector) return sector;

    // Store cache value
    sector = engine->FindSector(sectorName.GetDataSafe());
    return sector;
}

iSector* Location::GetSector(iEngine* engine) const
{
    // Return cached value
    if(sector) return sector;

    // Just return the looked up value
    return engine->FindSector(sectorName.GetDataSafe());
}

const csBox2 &Location::GetBoundingBox() const
{
    return boundingBox;
}

void Location::CalculateBoundingBox()
{
    if(IsCircle())
    {
        boundingBox.AddBoundingVertex(pos.x-radius,pos.z-radius);
        boundingBox.AddBoundingVertex(pos.x+radius,pos.z+radius);
    }
    else
    {
        for(size_t i=0; i< locs.GetSize(); i++)
        {
            boundingBox.AddBoundingVertex(locs[i]->pos.x,locs[i]->pos.z);
        }
    }
}



int Location::GetSectorID(iDataConnection* db,const char* name)
{
    // Load all with same master location type
    Result rs(db->Select("select id from sectors where name='%s'",name));

    if(!rs.IsValid())
    {
        Error2("Could not find sector id from db: %s",db->GetLastError());
        return -1;
    }
    return rs[0].GetInt("id");
}

uint32_t Location::GetEffectID(iEffectIDAllocator* allocator)
{
    if(effectID <= 0)
    {
        effectID = allocator->GetEffectID();
    }

    return effectID;
}

bool Location::Adjust(iDataConnection* db, const csVector3 &pos, iSector* sector)
{
    if(!Adjust(pos,sector))
    {
        return false;
    }

    db->CommandPump("UPDATE sc_locations SET x=%.2f,y=%.2f,z=%.2f,angle=%.2f WHERE id=%d",
                    pos.x,pos.y,pos.z,rot_angle,id);

    return true;
}

bool Location::Adjust(iDataConnection* db, const csVector3 &pos, iSector* sector, float rot_angle)
{
    if(!Adjust(pos,sector,rot_angle))
    {
        return false;
    }

    db->CommandPump("UPDATE sc_locations SET x=%.2f,y=%.2f,z=%.2f,angle=%.2f WHERE id=%d",
                    pos.x,pos.y,pos.z,rot_angle,id);

    return true;
}

bool Location::Adjust(const csVector3 &pos, iSector* sector)
{
    this->pos = pos;
    this->sector = sector;

    return true;
}

bool Location::Adjust(const csVector3 &pos, iSector* sector, float rot_angle)
{
    this->pos = pos;
    this->sector = sector;
    this->rot_angle = rot_angle;
    
    return true;
}


Location* Location::Insert(iDataConnection* db, csVector3 &pos, iSector* sector)
{
    Location* location = new Location(type, name, pos, sector, radius, rot_angle, GetFlags());
    location->id_prev_loc_in_region = GetID();

    // Check if this location is in a region, if not convert this locaiton into a region.
    if(!region)
    {
        locs.Push(this);   // First location is in the locs as well.
        region = this;
    }

    // Create DB entry
    location->CreateUpdate(db);

    // Update all the pointers and stuff.
    location->region = region;
    size_t index = region->locs.Find(this);
    Location* next = region->locs[(index+1)%region->locs.GetSize()];
    next->id_prev_loc_in_region = location->GetID();
    next->CreateUpdate(db);

    if(index+1 >= region->locs.GetSize())
    {
        region->locs.Push(location);
    }
    else
    {
        region->locs.Insert((index+1)%region->locs.GetSize(),location);
    }

    return location;
}

Location* Location::Insert(int id, csVector3 &pos, iSector* sector)
{
    Location* location = new Location(type, name, pos, sector, radius, rot_angle, GetFlags());
    location->SetID(id);
    location->id_prev_loc_in_region = GetID();

    // Check if this location is in a region, if not convert this locaiton into a region.
    if(!region)
    {
        locs.Push(this);   // First location is in the locs as well.
        region = this;
    }

    // Update all the pointers and stuff.
    location->region = region;
    size_t index = region->locs.Find(this);
    Location* next = region->locs[(index+1)%region->locs.GetSize()];
    next->id_prev_loc_in_region = location->GetID();

    if(index+1 >= region->locs.GetSize())
    {
        region->locs.Push(location);
    }
    else
    {
        region->locs.Insert((index+1)%region->locs.GetSize(),location);
    }

    return location;
}


bool Location::Load(iResultRow &row, iEngine* engine, iDataConnection* /*db*/)
{
    id         = row.GetInt("id");
    name       = row["name"];
    pos.x      = row.GetFloat("x");
    pos.y      = row.GetFloat("y");
    pos.z      = row.GetFloat("z");
    rot_angle  = row.GetFloat("angle");;
    radius     = row.GetFloat("radius");
    sectorName = row["sector"];
    // Cache sector if engine is available.
    if(engine)
    {
        sector     = engine->FindSector(sectorName);
    }
    else
    {
        sector = NULL;
    }

    id_prev_loc_in_region = row.GetInt("id_prev_loc_in_region");

    return true;
}

bool Location::CreateUpdate(iDataConnection* db)
{
    const char* fields[] =
    {
        "type_id",
        "id_prev_loc_in_region",
        "name",
        "x",
        "y",
        "z",
        "angle",
        "radius",
        "flags",
        "loc_sector_id"
    };

    psStringArray values;
    values.FormatPush("%d",type->GetID());
    values.FormatPush("%d",id_prev_loc_in_region);
    values.Push(name);
    values.FormatPush("%.2f",pos.x);
    values.FormatPush("%.2f",pos.y);
    values.FormatPush("%.2f",pos.z);
    values.FormatPush("%.2f",rot_angle);
    values.FormatPush("%.2f",radius);
    csString flagStr;
    values.Push(flagStr.GetDataSafe());
    values.FormatPush("%d",GetSectorID(db,sectorName));

    if(id == -1)
    {
        id = db->GenericInsertWithID("sc_locations",fields,values);
        if(id == 0)
        {
            id = -1;
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_locations","id",idStr,fields,values);
    }

    return false;
}

bool Location::Import(iDocumentNode* node, iDataConnection* db,int typeID)
{
    name       = node->GetAttributeValue("name");
    pos.x      = node->GetAttributeValueAsFloat("x");
    pos.y      = node->GetAttributeValueAsFloat("y");
    pos.z      = node->GetAttributeValueAsFloat("z");
    rot_angle  = node->GetAttributeValueAsFloat("angle");
    radius     = node->GetAttributeValueAsFloat("radius");
    sectorName = node->GetAttributeValue("sector");
    id_prev_loc_in_region = 0; // Not suppored for import.


    const char* fields[] =
    {"type_id","id_prev_loc_in_region","name","x","y","z","angle","radius","flags","loc_sector_id"};
    psStringArray values;
    values.FormatPush("%d",typeID);
    values.FormatPush("%d",id_prev_loc_in_region);
    values.Push(name);
    values.FormatPush("%.2f",pos.x);
    values.FormatPush("%.2f",pos.y);
    values.FormatPush("%.2f",pos.z);
    values.FormatPush("%.2f",rot_angle);
    values.FormatPush("%.2f",radius);
    csString flagStr;
    values.Push(flagStr);
    values.FormatPush("%d",GetSectorID(db,sectorName));

    if(id == -1)
    {
        id = db->GenericInsertWithID("sc_locations",fields,values);
        if(id == 0)
        {
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_locations","id",idStr,fields,values);
    }

    return true;
}

bool Location::CheckWithinBounds(iEngine* engine,const csVector3 &p,const iSector* sector)
{
    if(!IsRegion())
        return false;

    if(GetSector(engine) != sector)
        return false;

    // Thanks to http://astronomy.swin.edu.au/~pbourke/geometry/insidepoly/
    // for this example code.
    int counter = 0;
    size_t i,N=locs.GetSize();
    float xinters;
    csVector3 p1,p2;

    p1 = locs[0]->pos;
    for(i=1; i<=N; i++)
    {
        p2 = locs[i % N]->pos;
        if(p.z > csMin(p1.z,p2.z))
        {
            if(p.z <= csMax(p1.z,p2.z))
            {
                if(p.x <= csMax(p1.x,p2.x))
                {
                    if(p1.z != p2.z)
                    {
                        xinters = (p.z-p1.z)*(p2.x-p1.x)/(p2.z-p1.z)+p1.x;
                        if(p1.x == p2.x || p.x <= xinters)
                            counter++;
                    }
                }
            }
        }
        p1 = p2;
    }

    return (counter % 2 != 0);
}

bool Location::GetRandomPosition(iEngine* engine,csVector3 &pos,iSector* &sector)
{
    csVector3 randomPos;
    iSector*  randomSector;

    // TODO: Hack, for now just get the y value and sector from the first point.
    randomPos.y = locs[0]->pos.y;
    randomSector = locs[0]->GetSector(engine);

    do
    {
        randomPos.x = boundingBox.MinX() + psGetRandom()*(boundingBox.MaxX() - boundingBox.MinX());
        randomPos.z = boundingBox.MinY() + psGetRandom()*(boundingBox.MaxY() - boundingBox.MinY());

    }
    while(!CheckWithinBounds(engine,randomPos,randomSector));

    pos = randomPos;
    sector = randomSector;

    return true;
}

csString Location::ToString() const
{
    csString result;
    
    result.AppendFmt("%s - %.2f",toString(pos,sector).GetData(),rot_angle);

    return result;
}

/*------------------------------------------------------------------*/
LocationType::LocationType()
    :id(-1)
{
}

LocationType::LocationType(int id, const csString &name)
    :id(id),name(name)
{
}


LocationType::~LocationType()
{
    while(locs.GetSize())
    {
        Location* loc = locs.Pop();  //removes reference
        //now delete the location (a polygon). Since this will delete all points
        //on the polygon, and the first is a reference to loc, make sure to
        //delete that reference first:
        loc->locs.DeleteIndex(0);
        // now delete the polygon
        delete loc;
    }
}

bool LocationType::CreateUpdate(iDataConnection* db)
{
    const char* fields[] =
    {
        "name"
    };

    psStringArray values;
    values.Push(name);

    if(id == -1)
    {
        id = db->GenericInsertWithID("sc_location_type",fields,values);
        if(id == 0)
        {
            id = -1;
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_location_type","id",idStr,fields,values);
    }

    return false;
}



bool LocationType::Load(iDocumentNode* node)
{
    name = node->GetAttributeValue("name");
    if(!name.Length())
    {
        CPrintf(CON_ERROR, "Location Types must all have name attributes.\n");
        return false;
    }

    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a widget so read it's factory to create it.
        if(strcmp(node->GetValue(), "loc") == 0)
        {
            Location* newloc = new Location;
            newloc->pos.x      = node->GetAttributeValueAsFloat("x");
            newloc->pos.y      = node->GetAttributeValueAsFloat("y");
            newloc->pos.z      = node->GetAttributeValueAsFloat("z");
            newloc->rot_angle  = node->GetAttributeValueAsFloat("angle");
            newloc->radius     = node->GetAttributeValueAsFloat("radius");
            newloc->sectorName = node->GetAttributeValue("sector");
            newloc->type = this;
            locs.Push(newloc);
        }
    }
    return true;
}

void LocationType::AddLocation(Location* location)
{
    locs.Push(location);
}

void LocationType::RemoveLocation(Location* location)
{
    locs.Delete(location);
}



bool LocationType::Import(iDocumentNode* node, iDataConnection* db)
{
    name = node->GetAttributeValue("name");
    if(!name.Length())
    {
        CPrintf(CON_ERROR, "Location Types must all have name attributes.\n");
        return false;
    }

    const char* fields[] =
    {"name"};
    psStringArray values;
    values.Push(name);

    if(id == -1)
    {
        id = db->GenericInsertWithID("sc_location_type",fields,values);
        if(id == 0)
        {
            return false;
        }
    }
    else
    {
        csString idStr;
        idStr.Format("%d",id);
        return db->GenericUpdateWithID("sc_location_type","id",idStr,fields,values);
    }


    return true;
}


bool LocationType::Load(iResultRow &row, iEngine* engine, iDataConnection* db)
{
    id   = row.GetInt("id");
    name = row["name"];
    if(!name.Length())
    {
        CPrintf(CON_ERROR, "Location Types must all have name attributes.\n");
        return false;
    }

    // Load all with same master location type
    Result rs(db->Select("select loc.*,s.name as sector from sc_locations loc, sectors s where loc.loc_sector_id = s.id and type_id = %d and id_prev_loc_in_region <= 0",id)); // Only load locations, regions to be loaded later

    if(!rs.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError());
        return false;
    }
    for(int i=0; i<(int)rs.Count(); i++)
    {
        Location* newloc = new Location;
        newloc->Load(rs[i],engine,db);
        newloc->type = this;
        locs.Push(newloc);
    }

    // Load region with same master location type
    Result rs2(db->Select("select loc.*,s.name as sector from sc_locations loc, sectors s where loc.loc_sector_id = s.id and type_id = %d and id_prev_loc_in_region > 0",id)); // Only load regions, locations has been loaded allready

    csArray<Location*> tmpLocs;

    if(!rs2.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError());
        return false;
    }
    for(int i=0; i<(int)rs2.Count(); i++)
    {
        Location* newloc = new Location;
        newloc->Load(rs2[i],engine,db);

        newloc->type = this;
        tmpLocs.Push(newloc);
    }
    while(tmpLocs.GetSize())
    {
        Location* curr, *first;
        curr = first = tmpLocs.Pop();
        bool   found;
        first->type = this;
        first->locs.Push(first);
        first->region = first;
        do
        {
            found = false;
            for(size_t i= 0; i<tmpLocs.GetSize(); i++)
            {
                if(curr->id == tmpLocs[i]->id_prev_loc_in_region)
                {
                    curr = tmpLocs[i];
                    tmpLocs.DeleteIndex(i);
                    curr->type = this;
                    first->locs.Push(curr);
                    curr->region = first;
                    found = true;
                    break;
                }
            }

        }
        while(found);

        //when not a closed loop of at least 3 points, delete this
        //polygon, but continue with rest.
        if(first->locs.GetSize() <= 2)
        {
            Error1("Only two locs for region defined!");
            //delete all locations in 'polygon'. When deleting first,
            //it will recursively delete its polygon locations, in this
            //case including itself. So remove that reference first
            first->locs.DeleteIndex(0);
            delete first;
        }
        else if(curr->id != first->id_prev_loc_in_region)
        {
            Error1("First and last loc not connected!");
            //delete all locations in 'polygon'. When deleting first,
            //it will recursively delete its polygon locations, in this
            //case including itself. So remove that reference first
            first->locs.DeleteIndex(0);
            delete first;
        }
        else
        {
            first->type = this;
            locs.Push(first);
            first->CalculateBoundingBox();
        }
    }

    return true;
}

bool LocationType::CheckWithinBounds(iEngine* engine, const csVector3 &p,const iSector* sector)
{
    for(size_t i = 0; i < locs.GetSize(); i++)
    {
        if(locs[i]->CheckWithinBounds(engine,p,sector)) return true;
    }

    return false;
}

bool LocationType::GetRandomPosition(iEngine* engine,csVector3 &pos,iSector* &sector, const iSector* inSector)
{
    for(size_t i = 0; i < locs.GetSize(); i++)
    {
        // If search is limited to inSector than procede until secor is found.
        if (inSector && (locs[i]->GetSector(engine) != inSector)) continue;

        if(locs[i]->GetRandomPosition(engine,pos,sector)) return true;
    }

    return false;
}

/*------------------------------------------------------------------*/

LocationManager::LocationManager()
{

}

LocationManager::~LocationManager()
{
    csHash<LocationType*, csString>::GlobalIterator iter(loctypes.GetIterator());
    while(iter.HasNext())
        delete iter.Next();
    loctypes.Empty();
}

bool LocationManager::Load(iEngine* engine, iDataConnection* db)
{


    Result rs(db->Select("select * from sc_location_type"));

    if(!rs.IsValid())
    {
        Error2("Could not load locations from db: %s",db->GetLastError());
        return false;
    }
    for(int i=0; i<(int)rs.Count(); i++)
    {
        LocationType* loctype = new LocationType();

        if(loctype->Load(rs[i],engine,db))
        {
            loctypes.Put(loctype->name, loctype);
            CPrintf(CON_DEBUG, "Added location type '%s'(%d)\n",loctype->name.GetDataSafe(),loctype->id);
        }
        else
        {
            Error2("Could not load location: %s",db->GetLastError());
            delete loctype;
            return false;
        }

    }
    CPrintf(CON_WARNING, "Loaded %d locations \n",rs.Count());

    // Create a cache of all the locations.
    csHash<LocationType*, csString>::GlobalIterator iter(loctypes.GetIterator());
    LocationType* loc;
    while(iter.HasNext())
    {
        loc = iter.Next();
        for(size_t i = 0; i < loc->locs.GetSize(); i++)
        {
            all_locations.Push(loc->locs[i]);
        }
    }

    return true;
}

int LocationManager::GetNumberOfLocations() const
{
    return all_locations.GetSize();
}

Location* LocationManager::GetLocation(int index)
{
    return all_locations[index];
}

LocationType* LocationManager::FindRegion(const char* regname)
{
    if(!regname)
        return NULL;

    LocationType* found = loctypes.Get(regname, NULL);
    if(found && found->locs[0] && found->locs[0]->IsRegion())
    {
        return found;
    }
    return NULL;
}

LocationType* LocationManager::FindLocation(const char* typeName)
{
    if(!typeName)
        return NULL;

    LocationType* found = loctypes.Get(typeName, NULL);
    return found;
}

Location* LocationManager::FindLocation(const char* typeName, const char* name)
{
    LocationType* found = loctypes.Get(typeName, NULL);
    if(found)
    {
        return FindLocation(found, name);
    }
    return NULL;
}

Location* LocationManager::FindLocation(LocationType* type, const char* name)
{
    for(size_t i=0; i<type->locs.GetSize(); i++)
    {
        if(strcasecmp(type->locs[i]->name,name) == 0)
        {
            return type->locs[i];
        }
    }
    return NULL;
}

Location* LocationManager::FindLocation(int id)
{
    for(size_t i=0; i<all_locations.GetSize(); i++)
    {
        Location* location = all_locations[i];

        if(location->GetID() == id)
        {
            return location;
        }

        if(location->IsRegion())
        {
            for(size_t j=0; j<location->locs.GetSize(); j++)
            {
                Location* location2 = location->locs[j];

                if(location2->GetID() == id)
                {
                    return location2;
                }
            }
        }


    }
    return NULL;
}



Location* LocationManager::FindNearestLocation(psWorld* world, csVector3 &pos, iSector* sector, float range, float* found_range)
{
    float min_range = range;

    Location* min_location = NULL;

    for(size_t i=0; i<all_locations.GetSize(); i++)
    {
        Location* location = all_locations[i];

        float dist2 = world->Distance(pos,sector,location->pos,location->GetSector(world->GetEngine()));

        if(min_range < 0 || dist2 < min_range)
        {
            min_range = dist2;
            min_location = location;
        }
    }
    if(min_location && found_range) *found_range = min_range;

    return min_location;
}

size_t LocationManager::FindLocationsInSector(iEngine* engine, iSector* sector, csList<Location*> &list)
{
    size_t count = 0;
    for(size_t i=0; i<all_locations.GetSize(); i++)
    {
        Location* location = all_locations[i];

        if(location->GetSector(engine) == sector)
        {
            list.PushBack(location);
            count++;
        }
    }
    return count;
}

Location* LocationManager::FindNearestLocation(psWorld* world, const char* typeName, csVector3 &pos, iSector* sector, float range, float* found_range)
{
    LocationType* found = loctypes.Get(typeName, NULL);
    if(found)
    {
        float min_range = range;

        int   min_i = -1;

        for(size_t i=0; i<found->locs.GetSize(); i++)
        {
            float dist2 = world->Distance(pos,sector,found->locs[i]->pos,found->locs[i]->GetSector(world->GetEngine()));

            if(min_range < 0 || dist2 < min_range)
            {
                min_range = dist2;
                min_i = (int)i;
            }
        }
        if(min_i > -1)   // found closest one
        {
            if(found_range) *found_range = min_range;

            return found->locs[(size_t)min_i];
        }
    }
    return NULL;
}

Location* LocationManager::FindRandomLocation(psWorld* world, const char* typeName, csVector3 &pos, iSector* sector, float range, float* found_range)
{
    csArray<Location*> nearby;
    csArray<float> dist;

    LocationType* found = loctypes.Get(typeName, NULL);
    if(found)
    {
        for(size_t i=0; i<found->locs.GetSize(); i++)
        {
            float dist2 = world->Distance(pos,sector,found->locs[i]->pos,found->locs[i]->GetSector(world->GetEngine()));

            if(range < 0 || dist2 < range)
            {
                nearby.Push(found->locs[i]);
                dist.Push(dist2);
            }
        }

        if(nearby.GetSize()>0)   // found one or more closer than range
        {
            size_t pick = psGetRandom((uint32)nearby.GetSize());

            if(found_range) *found_range = sqrt(dist[pick]);

            return nearby[pick];
        }
    }
    return NULL;
}

csHash<LocationType*, csString>::GlobalIterator LocationManager::GetIterator()
{
    return loctypes.GetIterator();
}

Location* LocationManager::CreateLocation(iDataConnection* db, LocationType* locationType, const char* locationName, const csVector3 &pos, iSector* sector, float radius, float rot_angle, const csString &flags)
{
    Location* location = CreateLocation(locationType, locationName, pos, sector, radius, rot_angle, flags);

    if(!location->CreateUpdate(db))
    {
        delete location;
        return NULL;
    }

    return location;
}

Location* LocationManager::CreateLocation(const char* locationTypeName, const char* locationName, const csVector3 &pos, iSector* sector, float radius, float rot_angle, const csString &flags)
{
    LocationType* locationType = FindLocation(locationTypeName);
    if(!locationType)
    {
        return NULL;
    }

    return CreateLocation(locationType, locationName, pos, sector, radius, rot_angle, flags);
}

Location* LocationManager::CreateLocation(LocationType* locationType, const char* locationName, const csVector3 &pos, iSector* sector, float radius, float rot_angle, const csString &flags)
{
    Location* location = new Location(locationType, locationName, pos, sector, radius, rot_angle, flags);

    all_locations.Push(location);

    return location;
}

LocationType* LocationManager::CreateLocationType(iDataConnection* db, const csString &locationName)
{
    LocationType* locationType = new LocationType(-1,locationName);

    if(locationType->CreateUpdate(db))
    {
        loctypes.Put(locationName, locationType);
        return locationType;
    }

    delete locationType;
    return NULL;
}

LocationType* LocationManager::CreateLocationType(const csString &locationName)
{
    LocationType* locationType = new LocationType(-1,locationName);

    loctypes.Put(locationName, locationType);
    return locationType;
}

LocationType* LocationManager::CreateLocationType(int id, const csString &locationName)
{
    LocationType* locationType = new LocationType(id,locationName);
    loctypes.Put(locationName, locationType);
    return locationType;
}


bool LocationManager::RemoveLocationType(iDataConnection* db, const csString &locationName)
{
    int res =db->Command("delete from sc_location_type where name='%s'",
                         locationName.GetDataSafe());
    if(res != 1)
    {
        return false;
    }

    RemoveLocationType(locationName);

    return true;
}


bool LocationManager::RemoveLocationType(const csString &locationName)
{
    LocationType* locationType = FindLocation(locationName);
    if(locationType)
    {

        loctypes.Delete(locationType->GetName(),locationType);

        delete locationType;
        return true;
    }

    return false;
}



/*------------------------------------------------------------------*/
