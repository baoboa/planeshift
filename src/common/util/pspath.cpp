
/*
* pspath.cpp
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/movable.h>
#include <iutil/object.h>
#include <csgeom/math3d.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psconst.h"
#include "util/pspath.h"
#include "util/location.h"
#include "util/log.h"
#include "util/psdatabase.h"
#include "util/waypoint.h"
#include "util/strutil.h"
#include "util/psstring.h"
#include "engine/psworld.h"

psPathPoint::psPathPoint(psPath* parentPath):
    id(-1),prevPointId(0),radius(0.0),waypoint(NULL),effectID(0),path(parentPath)
{
}

bool psPathPoint::Load(iResultRow& row, iEngine *engine)
{
    id          = row.GetInt("id");
    prevPointId = row.GetInt("prev_point");
    pos.x       = row.GetFloat("x");
    pos.y       = row.GetFloat("y");
    pos.z       = row.GetFloat("z");
    sectorName  = row["sector_name"];
    sector      = engine->FindSector(sectorName);

    return true;
}

iSector* psPathPoint::GetSector(iEngine * engine)
{
    // Return cached value
    if (sector) return sector;
    
    // Update cached value
    sector = engine->FindSector(sectorName.GetDataSafe());
    return sector;
}

iSector* psPathPoint::GetSector(iEngine * engine) const
{
    // Return cached value
    if (sector) return sector;

    // Just return the looked up value
    return engine->FindSector(sectorName.GetDataSafe());
}

float psPathPoint::GetRadius()
{
    if (waypoint)
    {
        radius = waypoint->GetRadius();
    }

    return radius;
}


float psPathPoint::GetRadius() const
{
    psPathPoint* noneConst = const_cast<psPathPoint*>(this);
    return noneConst->GetRadius();
}



bool psPathPoint::Create(iDataConnection * db, int pathID)
{
    const char *fieldnames[]=
        {
            "path_id",
            "prev_point",
            "x",
            "y",
            "z"
        };

    psStringArray values;
    values.FormatPush("%u", pathID );
    values.FormatPush("%u", prevPointId );
    values.FormatPush("%10.2f",pos.x);
    values.FormatPush("%10.2f",pos.y);
    values.FormatPush("%10.2f",pos.z);
    
    id = db->GenericInsertWithID("sc_path_points",fieldnames,values);
    if (id==0)
    {
        Error2("Failed to create new Path Point Error %s",db->GetLastError());
        return false;
    }
    
    db->Command("update sc_path_points set loc_sector_id="
                "(select id from sectors where name='%s') where id=%d",
                sectorName.GetDataSafe(),id);

    return true;
    
}

bool psPathPoint::Remove(iDataConnection * db)
{
    int result = db->CommandPump("DELETE from sc_path_points WHERE id=%d",id);

    return (result == 1);
}


bool psPathPoint::UpdatePrevPointId(iDataConnection * db, int prevPointId)
{
    if (this->prevPointId != prevPointId)
    {
        SetPrevious(prevPointId);
        
        int result = db->CommandPump("UPDATE sc_path_points SET prev_point=%d WHERE id=%d",
                                 this->prevPointId,id);

        return (result == 1);
    }

    return true;
}



bool psPathPoint::Adjust(iDataConnection * db, csVector3 & pos, csString sector)
{
    int result = db->CommandPump("UPDATE sc_path_points SET x=%.2f,y=%.2f,z=%.2f,"
                                 "loc_sector_id=(select id from sectors where name='%s') WHERE id=%d",
                                 pos.x,pos.y,pos.z,sector.GetDataSafe(),id);

    Adjust(pos,sector);

    return (result == 1);

}

void psPathPoint::Adjust(csVector3 & pos, csString sector)
{
    this->pos = pos;
    this->sectorName = sector;
    this->sector = NULL;
}

void psPathPoint::Adjust(csVector3 & pos, iSector* sector)
{
    Adjust(pos, sector->QueryObject()->GetName());
    this->sector = sector;
}


void psPathPoint::SetWaypoint(Waypoint* waypoint)
{
    this->waypoint = waypoint;
}

Waypoint* psPathPoint::GetWaypoint()
{
    return waypoint;
}

int psPathPoint::GetID()
{
    return id;
}

csString psPathPoint::GetName()
{
    csString name("PP"); // Default to just PathPoint (PP).

    if (waypoint)
    {
        name = waypoint->GetName();
    }

    return name;
}

uint32_t psPathPoint::GetEffectID(iEffectIDAllocator* allocator)
{
    if (effectID <= 0)
    {
        effectID = allocator->GetEffectID();
    }
    
    return effectID;
}

psPath* psPathPoint::GetPath() const
{
    return path;
}

int psPathPoint::GetPathIndex() const
{
    return path->FindPointIndex(this);
}


//---------------------------------------------------------------------------

psPath::psPath(csString name, Waypoint * wp1, Waypoint * wp2, psString flagStr)
    :id(-1),name(name),start(NULL),end(NULL),oneWay(false),noWander(false),precalculationValid(false)
{
    SetFlags(flagStr);
    SetStart(wp1);
    SetEnd(wp2);
}

psPath::psPath(int pathID, csString name, psString flagStr)
    :id(pathID),name(name),start(NULL),end(NULL),oneWay(false),noWander(false),precalculationValid(false)
{
    SetFlags(flagStr);
}

psPath::~psPath()
{
    if (start)
    {
        start->RemoveLink(this);
        start = NULL;
    }
    if (end)
    {
        end->RemoveLink(this);
        end = NULL;
    }
    for (size_t i = 0; i < points.GetSize(); i++)
    {
        psPathPoint * pp = points[i];
        points[i] = NULL;
        delete pp;
    }
    points.DeleteAll();
}

psPathPoint* psPath::AddPoint(Location * loc, bool first)
{
    return AddPoint(loc->pos,loc->radius,loc->sectorName,first);
}

psPathPoint* psPath::AddPoint(iDataConnection * db, const csVector3& pos, const char * sectorName, bool first)
{
    psPathPoint * pp = AddPoint(pos, 0.0, sectorName, first);
    if (id != -1)
    {
        pp->Create(db,id);
    }

    return pp;
}

psPathPoint* psPath::AddPoint(const csVector3& pos, float radius, const char * sectorName, bool first)
{
    psPathPoint * pp = new psPathPoint(this);

    pp->id = -1;
    pp->pos = pos;
    pp->radius = radius;
    pp->sectorName = sectorName;

    if (first)
    {
        if (start)
        {
            points.Insert(1,pp); // Start waypoint occupies index 0
        }
        else
        {
            points.Insert(0,pp);
        }
    }
    else
    {
        if (end)
        {
            points.Insert(points.GetSize()-1,pp); // End waypoint occupies last index
        }
        else
        {
            points.Push(pp);
        }
    }
    // Update points
    for (size_t i = 2; i < points.GetSize()-1; i++)
    {
        points[i]->prevPointId = points[i-1]->GetID();
    }

    // Path isn't valid anymore
    precalculationValid = false;
    
    return pp;
}

psPathPoint* psPath::InsertPoint(iDataConnection *db, int index, const csVector3& pos, const char * sectorName)
{
    psPathPoint * pp = new psPathPoint(this);

    pp->id = -1;
    pp->pos = pos;
    pp->radius = 0.0;
    pp->sectorName = sectorName;

    if (!pp->Create(db,id))
    {
        delete pp;
        return NULL;
    }

    points.Insert(index,pp);

    if (!UpdatePrevPointIndexes(db))
    {        
        return NULL;
    }
    

    // Path isn't valid anymore
    precalculationValid = false;
    
    return pp;
}

bool psPath::RemovePoint(iDataConnection *db, psPathPoint* point)
{
    size_t pos = points.Find(point);
    if (pos == ((size_t)(-1)))
    {
        return false;
    }

    return RemovePoint(db, pos);
}


bool psPath::RemovePoint(iDataConnection *db, int index)
{
    psPathPoint* point = points.Extract(index);
    point->Remove(db);
    delete point;

    if (!UpdatePrevPointIndexes(db))
    {
        return false;
    }
    
    return true;
}

bool psPath::RemovePoint(int index)
{
    psPathPoint* point = points.Extract(index);
    delete point;

    return true;
}

    
bool psPath::UpdatePrevPointIndexes(iDataConnection* db)
{
    // Update points prev ids
    for (size_t i = 1; i < points.GetSize()-1; i++)
    {
        if (!points[i]->UpdatePrevPointId(db, i > 1 ? points[i-1]->GetID():0))
        {
            return false;
        }
    }
    return true;
}


void psPath::SetStart(Waypoint * wp)
{
    psPathPoint* pathPoint = AddPoint(wp->loc.pos, wp->loc.radius, wp->loc.sectorName, true); //Add start wp as a point
    pathPoint->SetWaypoint(wp);

    start = wp; // AddPoint use start so set after call to AddPoint
}

void psPath::SetEnd(Waypoint * wp)
{
    psPathPoint* pathPoint = AddPoint(wp->loc.pos, wp->loc.radius, wp->loc.sectorName, false); // Add end wp as a point
    pathPoint->SetWaypoint(wp);

    end = wp; // AddPoint use end so set after call to AddPoint
}


void psPath::Precalculate(psWorld * world, iEngine *engine, bool forceUpdate)
{
    if (precalculationValid && !forceUpdate) return;

    PrecalculatePath(world,engine);
}

float DistancePointLine(const csVector3 &p, const csVector3 &l1, const csVector3 &l2, float & t)
{
    t = - (l1-p)*(l2-l1)/ csSquaredDist::PointPoint(l2,l1);
    return sqrt(csSquaredDist::PointLine(p,l1,l2));
}

float psPath::Distance(psWorld * world, iEngine *engine,const csVector3& pos, const iSector* sector, int * index, float * fraction) const
{
    float dist = -1.0;
    int idx = -1;
    float fract = 0.0;
    for (size_t i = 0; i < points.GetSize()-1; i++)
    {
        csVector3 l1(points[i]->pos);
        csVector3 l2(points[i+1]->pos);
        
        // If sectors are not connected, return a very big value.
        if (!world->WarpSpace(points[i]->GetSector(engine),sector,l1) ||
            !world->WarpSpace(points[i+1]->GetSector(engine),sector,l2))
        {
            dist = INFINITY_DISTANCE;
            break;
        }

        float t = 0.0;
        float d = DistancePointLine(pos,l1,l2,t);
        
        if ((t >= 0.0 && t <= 1.0) && (dist < 0 || d < dist))
        {
            dist = d;
            idx = (int)i;
            fract = t;
        }
    }
    if (dist >= 0.0)
    {
        if (index)
        {
            *index = idx;
        }
        if (fraction)
        {
            *fraction = fract;
        }
    }
    return dist;
}

float psPath::DistancePoint(psWorld * world, iEngine *engine,const csVector3& pos,const iSector* sector, int * index, bool include_ends) const
{
    float dist = -1.0;
    int idx = -1;
    size_t start = 0;
    size_t end = points.GetSize();
    if (!include_ends)
    {
        start++;
        if (end > 0) end--;
    }
    
    for (size_t i = start; i < end; i++)
    {
        float d = world->Distance(pos, sector, points[i]->pos, points[i]->GetSector(engine));
        
        if (dist < 0 || d < dist)
        {
            dist = d;
            idx = (int)i;
        }
    }
    if (dist >= 0.0)
    {
        if (index)
        {
            *index = idx;
        }
    }
    return dist;
}

psPathPoint* psPath::GetStartPoint(Direction direction)
{
    if (direction == FORWARD)
    {
        return points[0];
    }
    else
    {
        return points[points.GetSize()-1];
    }
}

psPathPoint* psPath::GetEndPoint(Direction direction)
{
    if (direction == FORWARD)
    {
        return points[points.GetSize()-1];
    }
    else
    {
        return points[0];
    }
}

Waypoint* psPath::GetStartWaypoint(Direction direction)
{
    if (direction == FORWARD)
    {
        return start;
    }
    else
    {
        return end;
    }
}
    
Waypoint* psPath::GetEndWaypoint(Direction direction)
{
    if (direction == FORWARD)
    {
        return end;
    }
    else
    {
        return start;
    }
}


csVector3 psPath::GetEndPos(Direction direction)
{
    return GetEndPoint(direction)->pos;
}

float psPath::GetEndRot(Direction direction)
{
    if (direction == FORWARD)
    {
        return CalculateIncidentAngle(points[points.GetSize()-2]->pos,points[points.GetSize()-1]->pos);
    }
    else
    {
        return CalculateIncidentAngle(points[1]->pos,points[0]->pos);
    }    
}

iSector* psPath::GetEndSector(iEngine * engine, Direction direction)
{
    return GetEndPoint(direction)->GetSector(engine);
}

psPathAnchor* psPath::CreatePathAnchor()
{
    return new psPathAnchor(this);
}

bool psPath::Rename(iDataConnection * db,const char* name)
{
    int res =db->Command("update sc_waypoint_links set name='%s' where id=%d",
                         name,GetID());
    if (res != 1)
    {
        return false;
    } 
   
    Rename(name);

    return true;
}

void psPath::Rename(const char* name)
{
    this->name = name;
}

float psPath::GetLength(psWorld * world, iEngine *engine)
{ 
    Precalculate(world,engine);
    return totalDistance;
}

float psPath::GetLength(psWorld * world, iEngine *engine, int index)
{ 
    Precalculate(world,engine);

    return points[index+1]->startDistance[FORWARD] - points[index]->startDistance[FORWARD];
}


bool psPath::Load(iDataConnection * db, iEngine *engine)
{
    Result rs1(db->Select("select pp.*,s.name AS sector_name from sc_path_points pp, sectors s where pp.loc_sector_id = s.id and path_id='%d'",id));
 
    if (!rs1.IsValid())
    {
        Error2("Could not load path points from db: %s",db->GetLastError() );
        return false;
    }
    if (rs1.Count() == 0)
    {
        // No path points for this path
        return true;
    }
    csArray<psPathPoint*>  tempPoints;
    for (int i=0; i<(int)rs1.Count(); i++)
    {
        psPathPoint * pp = new psPathPoint(this);
        
        pp->Load(rs1[i],engine);

        tempPoints.Push(pp);
    }

    int currPoint = 0, prevPoint=0;
    size_t limit = tempPoints.GetSize()*tempPoints.GetSize()+2;  // Just add 2 to make it work for 1 point paths to
    while (tempPoints.GetSize() && limit)
    {
        if (tempPoints[currPoint]->prevPointId == prevPoint)
        {
            psPathPoint * pp = tempPoints[currPoint];
            tempPoints.DeleteIndexFast(currPoint);
            
            points.Push(pp);
            prevPoint = pp->id;
            currPoint--;
        }
        currPoint++;

        if (currPoint >= (int)tempPoints.GetSize())
        {
            currPoint = 0;
        }

        limit--;
    }
    if (limit == 0)
    {
        Error2("Could not assemble path %d",id);
        return false;
    }
    
    return true;
}

bool psPath::Create(iDataConnection * db)
{
    const char *fieldnames[]=
        {
            "name",
            "type",
            "wp1",
            "wp2",
            "flags"
        };
    
    psStringArray values;
    values.FormatPush("%s", name.GetDataSafe());
    values.FormatPush("%s", "LINEAR" );
    values.FormatPush("%d", start->GetID());
    values.FormatPush("%d", end->GetID());
    values.FormatPush("%s", GetFlags().GetDataSafe());
            
    id = db->GenericInsertWithID("sc_waypoint_links",fieldnames,values);

    if (id==0)
    {
        Error2("Failed to create new WP Link Error %s",db->GetLastError());
        return false;
    }

    // First and last point is a waypoint so no need to create
    // those.
    for (size_t i = 1; i < points.GetSize()-1; i++)
    {
        if (i > 1)
        {
            points[i]->SetPrevious(points[i-1]->GetID());
        }
        points[i]->Create(db, id);
    }

    return true;
}

bool psPath::Adjust(iDataConnection * db, int index, csVector3 & pos, csString sector)
{
    // First and last point is a placeholder for the waypoint only update the position.
    if (index == 0 || index == ((int)points.GetSize()-1))
    {
        points[index]->Adjust(pos,sector);
        return true;
    }
    
    return points[index]->Adjust(db,pos,sector);
}


float psPath::CalculateIncidentAngle(csVector3& pos, csVector3& dest)
{
    csVector3 diff = dest-pos;  // Get vector from player to desired position

    if (!diff.x)
        diff.x = .000001F; // div/0 protect

    float angle = atan2(-diff.x,-diff.z);

    return angle;
}

csString psPath::GetFlags() const
{
    csString flagStr;
    bool added = false;
    if (oneWay)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("ONEWAY");
        added = true;
    }
    if (noWander)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("NO_WANDER");
        added = true;
    }
    if (teleport)
    {
        if (added) flagStr.Append(", ");
        flagStr.Append("TELEPORT");
        added = true;
    }

    return flagStr;
}

void psPath::SetFlags(const psString& flagStr)
{
    oneWay    = isFlagSet(flagStr,"ONEWAY");
    noWander  = isFlagSet(flagStr,"NO_WANDER");
    teleport  = isFlagSet(flagStr,"TELEPORT");
}

bool psPath::SetFlag(iDataConnection * db, const csString &flagstr, bool enable)
{
    if (!SetFlag(flagstr,enable))
    {
        return false;
    }

    int result = db->CommandPump("UPDATE sc_waypoint_links SET flags='%s' WHERE id=%d",
                                 GetFlags().GetDataSafe(), id);
    if (result != 1)
    {
        Error2("Sql failed: %s\n",db->GetLastError());
        return false;
    }
    
    return true;
}

bool psPath::SetFlag(const csString &flagstr, bool enable)
{
    if (isFlagSet(flagstr,"ONEWAY"))
    {
        oneWay = enable;
    }
    else if (isFlagSet(flagstr,"NO_WANDER"))
    {
        noWander = enable;
    }
    else if (isFlagSet(flagstr,"TELEPORT"))
    {
        teleport = enable;
    }
    else
    {
        return false;
    }

    return true;
}


size_t psPath::FindPointsInSector(iEngine * engine, iSector *sector, csList<psPathPoint*>& list)
{
    size_t count = 0;
    for (size_t i = 0; i < points.GetSize(); i++)
    {
        psPathPoint* point = points[i];
        if (point->GetSector(engine) == sector)
        {
            list.PushBack(point);
            count++;
        }
    }
    return count;
}

psPathPoint* psPath::FindPoint(int id)
{
    for (size_t i = 0; i < points.GetSize(); i++)
    {
        psPathPoint* point = points[i];
        if (point->GetID() == id)
        {
            return point;
        }
    }
    return NULL;
}

int psPath::FindPointIndex(int id) const
{
    for (size_t i = 0; i < points.GetSize(); i++)
    {
        const psPathPoint* point = points[i];
        if (point->GetID() == id)
        {
            return i;
        }
    }
    return -1;
}

int psPath::FindPointIndex(const psPathPoint* point) const
{
    for (size_t i = 0; i < points.GetSize(); i++)
    {
        if (points[i] == point)
        {
            return i;
        }
    }
    return -1;
}

psPathPoint* psPath::GetPoint(int index)
{
    return points[index];
}

const psPathPoint* psPath::GetPoint(int index) const
{
    return points[index];
}






//---------------------------------------------------------------------------

psLinearPath::psLinearPath(csString name, Waypoint * wp1, Waypoint * wp2, const psString flagStr)
    :psPath(name,wp1,wp2, flagStr)
{
}

psLinearPath::psLinearPath(int pathID, csString name, psString flagStr)
    :psPath(pathID,name,flagStr)
{
}

void psLinearPath::PrecalculatePath(psWorld * world, iEngine *engine)
{
    totalDistance = 0;

    // Sett a fixed high distance for teleports
    if (teleport)
    {
        totalDistance = 1000.0;
        precalculationValid = true;
        return;
    }

    for (size_t ii=0;ii<points.GetSize()-1;ii++)
    {
        csVector3 pos1(points[ii]->pos),pos2(points[ii+1]->pos);

	iSector* sectorP1 = points[ii]->GetSector(engine);
	iSector* sectorP2 = points[ii+1]->GetSector(engine);

        if( !sectorP1 || !sectorP2 )
        {
            precalculationValid = false;
            return;
        }
        if (!world->WarpSpace(sectorP2,sectorP1,pos2))
        {
            Error6("In path \'%s\', sector %s of points %lu and"
                   " sector %s of point %lu are not connected by a portal!",
                   name.GetDataSafe(),
                   sectorP1->QueryObject()->GetName(), (unsigned long)ii,
                   sectorP2->QueryObject()->GetName(), (unsigned long)ii+1);
            precalculationValid = false;
            return;
        }
        
        float dist = (pos2-pos1).Norm();

        points[ii]->startDistance[FORWARD] = totalDistance;

        totalDistance += dist;
        
        dx.Push( pos2.x - pos1.x);
        dy.Push( pos2.y - pos1.y);
        dz.Push( pos2.z - pos1.z);        
    }    
    points[points.GetSize()-1]->startDistance[FORWARD] = totalDistance;
    
    float r1=points[0]->GetRadius();
    float r2=points[points.GetSize()-1]->GetRadius();

    for (size_t ii=0;ii<points.GetSize();ii++)
    {
        points[ii]->startDistance[REVERSE] =  totalDistance - points[ii]->startDistance[FORWARD];
        points[ii]->radius = r1 + (r2-r1)*points[ii]->startDistance[FORWARD]/totalDistance;
    }
    precalculationValid = true;
}

void psLinearPath::GetInterpolatedPosition (int index, float fraction, csVector3& pos)
{
    psPathPoint * pp = points[index];
    pos.x = pp->pos.x + dx[index]* fraction;
    pos.y = pp->pos.y + dy[index]* fraction;
    pos.z = pp->pos.z + dz[index]* fraction;
}

void psLinearPath::GetInterpolatedUp (int /*index*/, float /*fraction*/, csVector3& up)
{
    up = csVector3(0,1,0);
}
    
void psLinearPath::GetInterpolatedForward (int index, float /*fraction*/, csVector3& forward)
{
    forward.x = dx[index];
    forward.y = dy[index];
    forward.z = dz[index];
}

//---------------------------------------------------------------------------

psPathAnchor::psPathAnchor(psPath * path)
    :path(path),pathDistance(0.0)
{
}

bool psPathAnchor::CalculateAtDistance(psWorld * world, iEngine *engine, float distance, psPath::Direction direction)
{
    float start = 0.0 ,end = 0.0; // Start and end distance of currentAtIndex

    // Check if we are at the end of the path. GetLength will indirect precalculate the path
    if (distance < 0.0 || distance > path->GetLength(world,engine))
    {
        return false;
    }
    
    if (direction == psPath::FORWARD)
    {
        // First find the current index.
        for (currentAtIndex = 0; currentAtIndex < (int)path->points.GetSize() -1 ; currentAtIndex++)
        {
            start = path->points[currentAtIndex]->startDistance[direction];
            end   = path->points[currentAtIndex+1]->startDistance[direction];
            if (distance >= start && distance <= end) break; // Found segment
        }
        currentAtFraction = (distance - start) / (end - start);
    }
    else
    {
        // First find the current index.
        for (currentAtIndex = 0; currentAtIndex < (int)path->points.GetSize() -1 ; currentAtIndex++)
        {
            start = path->points[currentAtIndex+1]->startDistance[direction];
            end   = path->points[currentAtIndex]->startDistance[direction];
            if (distance >= start && distance <= end) break; // Found segment
        }
        currentAtFraction = (end - distance) / (end - start);
    }

    currentAtDirection = direction;
    
    return true;
}

void psPathAnchor::GetInterpolatedPosition (csVector3& pos)
{
    path->GetInterpolatedPosition(currentAtIndex,currentAtFraction,pos);
}

void psPathAnchor::GetInterpolatedUp (csVector3& up)
{
    path->GetInterpolatedUp(currentAtIndex,currentAtFraction,up);
}

void psPathAnchor::GetInterpolatedForward (csVector3& forward)
{
    path->GetInterpolatedForward(currentAtIndex,currentAtFraction,forward);

    if (currentAtDirection != psPath::REVERSE)
    {
        forward = -forward;
    }
}

bool psPathAnchor::Extrapolate(psWorld * world, iEngine *engine, float delta, psPath::Direction direction, iMovable * movable)
{
    pathDistance += delta;

    if (!CalculateAtDistance (world, engine, pathDistance, direction))
    {
        return false; // At end of path
    }
    

    csVector3 pos, look, up;
    
    GetInterpolatedPosition (pos);
    GetInterpolatedUp (up);
    GetInterpolatedForward (look);
    
    // Check whether we should do anything and if transformations are needed apply them
    iSector* sector = movable->GetSectors()->Get(0);
    if (path->points[currentAtIndex]->GetSector(engine) == sector)
    {
        // do nothing
    }
    else if (path->points[currentAtIndex+1]->GetSector(engine) == sector)
    {
        if (!world->WarpSpace(path->points[currentAtIndex]->GetSector(engine), path->points[currentAtIndex+1]->GetSector(engine), pos) ||
            !world->WarpSpaceRelative(path->points[currentAtIndex]->GetSector(engine), path->points[currentAtIndex+1]->GetSector(engine), look) ||
            !world->WarpSpaceRelative(path->points[currentAtIndex]->GetSector(engine), path->points[currentAtIndex+1]->GetSector(engine), up))
        {
            Error1("transformation failed");
            return true;
        }
    }
    else
    {
        Error4("psPatchAnchor: expected sector %s or %s - got %s", path->points[currentAtIndex]->GetSector(engine)->QueryObject()->GetName(),
            path->points[currentAtIndex+1]->GetSector(engine)->QueryObject()->GetName(), sector->QueryObject()->GetName());
        return true;
    }

    // Set Position
    movable->GetTransform().SetOrigin(pos);

    // Set rotation.
    movable->GetTransform().LookAt(look.Unit (), up.Unit ());
    movable->UpdateMove ();

    return true;
}


