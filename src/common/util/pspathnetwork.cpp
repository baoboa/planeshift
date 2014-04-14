/*
* pspathnetwork.cpp
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

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iengine/engine.h>
#include <iengine/sector.h>

//====================================================================================
// Project Includes
//====================================================================================
#include "util/psconst.h"
#include "util/psutil.h"
#include "util/strutil.h"
#include "util/log.h"
#include "util/psdatabase.h"
#include "util/psstring.h"
#include "engine/psworld.h"
#include "util/waypoint.h"
#include "util/consoleout.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "pspathnetwork.h"

bool psPathNetwork::Load(iEngine *engine, iDataConnection *db,psWorld * world)
{
    // First initialize pointers to some importent classes
    this->engine = engine;
    this->db = db;
    this->world = world;
    

    Result rs(db->Select("select wp.*,s.name as sector from sc_waypoints wp, sectors s where wp.loc_sector_id = s.id"));

    if (!rs.IsValid())
    {
        Error2("Could not load waypoints from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs.Count(); i++)
    {
        Waypoint *wp = new Waypoint();

        if (wp->Load(rs[i],engine))
        {
            Result rs2(db->Select("select id,alias,rotation_angle from sc_waypoint_aliases where wp_id=%d",wp->GetID()));
            for (int j=0; j<(int)rs2.Count(); j++)
            {
                wp->AddAlias(rs2[j].GetInt(0),rs2[j][1],rs2[j].GetFloat(2)*PI/180.0);
            }

            waypoints.Push(wp);

            // Push in groups
            if (strcmp(wp->GetGroup(),"")!=0)
            {
                AddWaypointToGroup(wp->GetGroup(),wp);
            }
            
        }
        else
        {
            Error2("Could not load waypoint: %s",db->GetLastError() );            
            delete wp;
            return false;
        }
        
    }

    Result rs1(db->Select("select * from sc_waypoint_links"));

    if (!rs1.IsValid())
    {
        Error2("Could not load waypoint links from db: %s",db->GetLastError() );
        return false;
    }
    for (int i=0; i<(int)rs1.Count(); i++)
    {
        Waypoint * wp1 = FindWaypoint(rs1[i].GetInt("wp1"));
        Waypoint * wp2 = FindWaypoint(rs1[i].GetInt("wp2"));
        if(!wp1 || !wp2)
        {
        	if(!wp1)
        	    Error2("Could not find waypoint wp1 link with id %d",rs1[i].GetInt("wp1") );
        	if(!wp2)
        	    Error2("Could not find waypoint wp2 link with id %d",rs1[i].GetInt("wp2") );
        	return false;
        }
        psString flagStr(rs1[i]["flags"]);

        int pathId = rs1[i].GetInt("id");
        
        csString pathType = rs1[i]["type"];
        
        psPath * path;
        if (strcasecmp(pathType,"linear") == 0)
        {
            path = new psLinearPath(pathId,rs1[i]["name"],flagStr);
        }
        else
        {
            path = new psLinearPath(pathId,rs1[i]["name"],flagStr); // For now
        }

        path->SetStart(wp1);
        if (!path->Load(db,engine))
        {
            Error1("Failed to load path");
            return false;
        }
        path->SetEnd(wp2);
        paths.Push(path);

        float dist = path->GetLength(world,engine);
                                    
        wp1->AddLink(path,wp2,psPath::FORWARD,dist);
        if (!path->oneWay)
        {
            wp2->AddLink(path,wp1,psPath::REVERSE,dist); // bi-directional link is implied
        }
    }
    
    
    return true;
}

size_t psPathNetwork::AddWaypointToGroup(csString group, Waypoint * wp)
{
    size_t index;
    
    index = waypointGroupNames.PushSmart(group);
    if (index >= waypointGroups.GetSize())
    {
        csList<Waypoint*> list;
        waypointGroups.Push(list);
    }

    waypointGroups[index].PushBack(wp);
    
    return index;
}


Waypoint *psPathNetwork::FindWaypoint(int id)
{
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    while (iter.HasNext())
    {
        wp = iter.Next();

        if (wp->loc.id == id)
        {
            return wp;
        }
    }
    
    return NULL;
}

Waypoint *psPathNetwork::FindWaypoint(const char * name, WaypointAlias** alias)
{
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    while (iter.HasNext())
    {
        wp = iter.Next();

        // Check name
        if (strcasecmp(wp->GetName(),name)==0)
        {
            if (alias)
            {
                *alias = NULL;
            }
            
            return wp;
        }
        // Check for aliases
        for (size_t i = 0; i < wp->aliases.GetSize(); i++)
        {
            if (strcasecmp(wp->aliases[i]->alias,name)==0)
            {
                if (alias)
                {
                    *alias = wp->aliases[i];
                }

                return wp; // Found name in aliases
            }
        }
    }
    
    return NULL;
}


Waypoint *psPathNetwork::FindWaypoint(const csVector3& v, iSector *sector)
{
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());

    while (iter.HasNext())
    {
        Waypoint *wp = iter.Next();

        float dist = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (dist < wp->loc.radius)
        {
            return wp;
        }
    }

    return NULL;
}

Waypoint *psPathNetwork::FindNearestWaypoint(const csVector3& v,iSector *sector, float range, float * found_range)
{
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    float min_range = range;

    Waypoint *min_wp = NULL;

    while (iter.HasNext())
    {
        wp = iter.Next();

        float dist2 = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (min_range < 0 || dist2 < min_range)
        {
            min_range = dist2;
            min_wp = wp;
        }
    }
    if (min_wp && found_range) *found_range = min_range;

    return min_wp;
}

Waypoint *psPathNetwork::FindRandomWaypoint(const csVector3& v,iSector *sector, float range, float * found_range)
{
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    csArray<Waypoint*> nearby;
    csArray<float> dist;
 
    while (iter.HasNext())
    {
        wp = iter.Next();

        float dist2 = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (range < 0 || dist2 < range)
        {
            nearby.Push(wp);
            dist.Push(dist2);
        }
    }

    if (nearby.GetSize()>0)  // found one or more closer than range
    {
        size_t pick = psGetRandom((uint32)nearby.GetSize());
        
        if (found_range) *found_range = sqrt(dist[pick]);
        
        return nearby[pick];
    }


    return NULL;
}

Waypoint *psPathNetwork::FindNearestWaypoint(int group, const csVector3& v,iSector *sector, float range, float * found_range)
{
    csList<Waypoint*>::Iterator iter(waypointGroups[group]);
    Waypoint *wp;

    float min_range = range;

    Waypoint *min_wp = NULL;

    while (iter.HasNext())
    {
        wp = iter.Next();

        float dist2 = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (min_range < 0 || dist2 < min_range)
        {
            min_range = dist2;
            min_wp = wp;
        }
    }
    if (min_wp && found_range) *found_range = min_range;

    return min_wp;
}

Waypoint *psPathNetwork::FindRandomWaypoint(int group, const csVector3& v,iSector *sector, float range, float * found_range)
{
    csList<Waypoint*>::Iterator iter(waypointGroups[group]);
    Waypoint *wp;

    csArray<Waypoint*> nearby;
    csArray<float> dist;
 
    while (iter.HasNext())
    {
        wp = iter.Next();

        float dist2 = world->Distance(v,sector,wp->loc.pos,wp->GetSector(engine));
        
        if (range < 0 || dist2 < range)
        {
            nearby.Push(wp);
            dist.Push(dist2);
        }
    }

    if (nearby.GetSize()>0)  // found one or more closer than range
    {
        size_t pick = psGetRandom((uint32)nearby.GetSize());
        
        if (found_range) *found_range = sqrt(dist[pick]);
        
        return nearby[pick];
    }


    return NULL;
}

int psPathNetwork::FindWaypointGroup(const char * groupName)
{
    for (size_t i=0;i< waypointGroupNames.GetSize();i++)
    {
        if (strcasecmp(groupName,waypointGroupNames[i].GetDataSafe())==0)
        {
            return (int)i; // Found matching group name
        }
    }
    return -1;
}

psPathPoint* psPathNetwork::FindPathPoint(int id)
{
    for (size_t p = 0; p < paths.GetSize(); p++)
    {
        psPathPoint* point = paths[p]->FindPoint(id);
        if (point) return point;
    }

    return NULL;
}

psPathPoint* psPathNetwork::FindPoint(const psPath* path, const csVector3& pos, iSector* sector, float range, int& index)
{
    float dist,tmpFract;

    dist = path->Distance(world,engine,pos,sector,&index,&tmpFract);
    if (dist < 0.0)
    {
        return NULL;
    }
    
    return path->points[index];
}


psPath *psPathNetwork::FindNearestPath(const csVector3& v,iSector *sector, float range, float * found_range, int * index, float * fraction)
{
    psPath * found = NULL;
    int idx = -1;
    int tmpIdx;
    float fract = 0.0, tmpFract;
 
    for (size_t p = 0; p < paths.GetSize(); p++)
    {
        float dist2 = paths[p]->Distance(world,engine,v,sector,&tmpIdx,&tmpFract);
                    
        if (dist2 >= 0.0 && (range < 0 || dist2 < range))
        {
            found = paths[p];
            range = dist2;
            idx = tmpIdx;
            fract = tmpFract;
        }
    }
    if (found)
    {
        if (found_range)
        {
            *found_range = range;
        }
        if (index)
        {
            *index = idx;
        }
        if (fraction)
        {
            *fraction = fract;
        }
    }
    return found;
}


psPathPoint* psPathNetwork::FindNearestPoint(const psPath* path, const csVector3& v, const iSector *sector, float range)
{
    int tmpIdx;
    
    float dist2 = path->DistancePoint(world,engine,v,sector,&tmpIdx);

    if (dist2 >= 0.0)
    {
        return path->points[tmpIdx];
    }
    
    return NULL;
}


psPath *psPathNetwork::FindNearestPoint(const csVector3& v,iSector *sector, float range, float * found_range, int * index)
{
    psPath * found = NULL;
    int idx = -1;
    int tmpIdx;
 
    for (size_t p = 0; p < paths.GetSize(); p++)
    {
        float dist2 = paths[p]->DistancePoint(world,engine,v,sector,&tmpIdx);
                    
        if (dist2 >= 0.0 && (range < 0 || dist2 < range))
        {
            found = paths[p];
            range = dist2;
            idx = tmpIdx;
        }
    }
    if (found)
    {
        if (found_range)
        {
            *found_range = range;
        }
        if (index)
        {
            *index = idx;
        }
    }
    return found;
}

csList<Waypoint*> psPathNetwork::FindWaypointRoute(Waypoint * start, Waypoint * end, const psPathNetwork::RouteFilter* routeFilter)
{
    csList<Waypoint*> waypoint_list;
    csList<Waypoint*> priority; // Should have been a priority queue
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;
    
    // Using Dijkstra's algorithm to find shortest way

    // Initialize
    while (iter.HasNext())
    {
        wp = iter.Next();

        // Filter the waypoints with exception of the start and end point.
        if ((wp != start) && (wp != end) && routeFilter->Filter(wp))
        {
            wp->excluded = true;
            continue; // No need to think more about this waypoint
        }
        else    
        {
            wp->excluded = false;
        }

        wp->distance = INFINITY_DISTANCE;
        wp->pi = NULL;
        
        // Insert WP into priority queue
        priority.PushBack(wp);
    }
    start->distance = 0;

    while (!priority.IsEmpty())
    {
        Waypoint *wp_u = NULL;
        Waypoint *pri_wp = NULL;

        // Extract min from priority queue
        csList<Waypoint*>::Iterator pri(priority);
        csList<Waypoint*>::Iterator pri_loc;
        while (pri.HasNext())
        {
            pri_wp = pri.Next();

            if (!wp_u || pri_wp->distance < wp_u->distance)
            {
                wp_u = pri_wp;
                pri_loc = pri;
            }
        }
        priority.Delete(pri_loc);
        size_t v;
        for (v = 0; v < wp_u->links.GetSize(); v++)
        {
            Waypoint * wp_v = wp_u->links[v];

            // Is the target waypoint excluded, in that case continue on.
            if (wp_v->excluded)
            {
                continue;
            }

            // Relax
            if (wp_v->distance > wp_u->distance + wp_u->dists[v])
            {
                wp_v->distance = wp_u->distance + wp_u->dists[v];
                wp_v->pi = wp_u;
            }
        }
        // if wp == end, we should be done!!!!!!
    }

    wp = end;

    if (end->pi)
    {
        wp = end;
        while (wp)
        {
            waypoint_list.PushFront(wp);
            wp = wp->pi;
        }
    }

    return waypoint_list;
}


csList<Edge*> psPathNetwork::FindEdgeRoute(Waypoint * start, Waypoint * end, const psPathNetwork::RouteFilter* routeFilter)
{
    csList<Edge*> edges;

    csList<Waypoint*> waypoints = FindWaypointRoute(start, end, routeFilter);
    csList<Waypoint*>::Iterator iter(waypoints);
    Waypoint* last = NULL;
    while (iter.HasNext())
    {
        Waypoint* wp = iter.Next();
        if (last)
        {
            Edge* edge = FindEdge(last,wp);
            edges.PushBack(edge);
        }

        last = wp;
    }
    return edges;
}


void psPathNetwork::ListWaypoints(const char * pattern)
{
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;
    CPrintf(CON_CMDOUTPUT, "Waypoints\n");
    CPrintf(CON_CMDOUTPUT, "%9s %-30s %-45s %-6s %-6s %-30s\n", "WP", "Name", "Position","Radius","Dist","PI");
    while (iter.HasNext())
    {
        wp = iter.Next();

        if (!pattern || strstr(wp->GetName(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%9d %-30s %-45s %6.2f %6.1f %-30s" ,
                    wp->loc.id,wp->GetName(),toString(wp->loc.pos,wp->loc.sector).GetDataSafe(),
                    wp->loc.radius,wp->distance,
                    (wp->pi?wp->pi->GetName():""));

            for (size_t i = 0; i < wp->links.GetSize(); i++)
            {
                CPrintf(CON_CMDOUTPUT," %s%s(%d,%.1f)",(wp->edges[i]->NoWander()?"#":""),
                        wp->links[i]->GetName(),wp->links[i]->GetID(),wp->dists[i]);
            }

            CPrintf(CON_CMDOUTPUT,"\n");
        }
    }

    CPrintf(CON_CMDOUTPUT, "Waypoint groups\n");

    for (size_t i = 0; i < waypointGroupNames.GetSize(); i++)
    {
        CPrintf(CON_CMDOUTPUT,"%s\n  ",waypointGroupNames[i].GetDataSafe());
        csList<Waypoint*>::Iterator iter(waypointGroups[i]);
        bool first = true;
        while (iter.HasNext())
        {
           Waypoint *wp = iter.Next();
           CPrintf(CON_CMDOUTPUT,"%s%s(%d)",first?"":", ",wp->GetName(),wp->GetID());
           first = false;
        }
        CPrintf(CON_CMDOUTPUT,"\n");
    }
    
}



size_t psPathNetwork::FindPointsInSector(iSector *sector, csList<psPathPoint*>& list)
{
    size_t count = 0;
    for (size_t p = 0; p < paths.GetSize(); p++)
    {
       count += paths[p]->FindPointsInSector( engine, sector, list );
    }

    return count;
}

size_t psPathNetwork::FindWaypointsInSector(iSector *sector, csList<Waypoint*>& list)
{
    size_t count = 0;
    csPDelArray<Waypoint>::Iterator iter(waypoints.GetIterator());
    Waypoint *wp;

    // Initialize
    while (iter.HasNext())
    {
        wp = iter.Next();

	if ( wp->GetSector(engine) == sector )
        {
           list.PushBack(wp);
           count++;
        }
    }
    return count;
}

void psPathNetwork::ListPaths(const char* /*name*/)
{
    csPDelArray<psPath>::Iterator iter(paths.GetIterator());
    psPath *path;

    while (iter.HasNext())
    {
        path = iter.Next();

        CPrintf(CON_CMDOUTPUT,"%9d %6d %6d %-30s\n",path->id,(path->start?path->start->loc.id:0),
                (path->end?path->end->loc.id:0),path->name.GetDataSafe());

        for (int i = 0; i < path->GetNumPoints();i++)
        {
            psPathPoint * pp = path->points[i];
            CPrintf(CON_CMDOUTPUT, "%9d (%9.3f,%9.3f,%9.3f, %s) %6.3f %6.3f\n" ,
                    pp->id,pp->pos.x,pp->pos.y,pp->pos.z,
                    pp->sectorName.GetDataSafe(),pp->startDistance[psPath::FORWARD],pp->startDistance[psPath::REVERSE]);
        }
    }
}

psPath   *psPathNetwork::FindPath(const char *name)
{
    csPDelArray<psPath>::Iterator iter(paths.GetIterator());
    psPath *path;

    while (iter.HasNext())
    {
        path = iter.Next();
        if (strcmp ( path->GetName(),name) == 0)
        {
            return path;
        }
    }

    return NULL;
}

psPath* psPathNetwork::FindPath(int id)
{
    csPDelArray<psPath>::Iterator iter(paths.GetIterator());
    psPath *path;

    while (iter.HasNext())
    {
        path = iter.Next();
        if ( path->GetID() == id )
        {
            return path;
        }
    }

    return NULL;
}

Edge* psPathNetwork::FindEdge(const Waypoint * wp1, const Waypoint * wp2)
{
    // Is there a link between wp1 and wp2?
    const size_t index = wp1->links.Find(const_cast<Waypoint*>(wp2));
    if (index != csArrayItemNotFound)
    {
        // Get chached values
        return wp1->edges[index];
    }

    return NULL;
}

Waypoint* psPathNetwork::CreateWaypoint(iDataConnection *db, csString& name, 
                                        csVector3& pos, csString& sectorName,
                                        float radius, csString& flags)
{

    Waypoint *wp = CreateWaypoint(name,pos,sectorName,radius,flags);

    if (!wp->Create(db))
    {
        delete wp;
        return NULL;
    }

    return wp;
}

Waypoint* psPathNetwork::CreateWaypoint(csString& name, 
                                        csVector3& pos, csString& sectorName,
                                        float radius, csString& flags)
{

    Waypoint *wp = new Waypoint(name,pos,sectorName,radius,flags);

    waypoints.Push(wp);

    return wp;
}

psPath* psPathNetwork::CreatePath(iDataConnection *db, const csString& name, Waypoint* wp1, Waypoint* wp2,
                                  const csString& flags)
{
    psPath * path = new psLinearPath(name,wp1,wp2,flags);

    return CreatePath(db, path);
}

psPath* psPathNetwork::CreatePath(psPath * path)
{
    paths.Push(path);

    float dist = path->GetLength(world,engine); 
    
    path->start->AddLink(path,path->end,psPath::FORWARD,dist);
    if (!path->oneWay)
    {
        path->end->AddLink(path,path->start,psPath::REVERSE,dist); // bi-directional link is implied
    }

    return path;
}

psPath* psPathNetwork::CreatePath(iDataConnection *db, psPath * path)
{
    if (!path->Create(db))
    {
        delete path;
        return NULL;
    }

    return CreatePath( path );
}


int psPathNetwork::GetNextWaypointCheck()
{
    static int check = 0;
    return ++check;
}

bool psPathNetwork::Delete(psPath * path)
{
    // First remove from db.
    db->CommandPump("delete from sc_path_points where path_id=%d",path->GetID());
    db->CommandPump("delete from sc_waypoint_links where id=%d",path->GetID());

    // Delete the object
    Waypoint * start = path->start;
    Waypoint * end = path->end;

    start->RemoveLink(path);
    path->start = NULL;

    end->RemoveLink(path);
    path->end = NULL;

    paths.Extract(paths.Find(path));

    delete path;

    // Now delete any waypoints that dosn't have any links anymore.
    if (start->links.GetSize() == 0)
    {
        db->CommandPump("delete from sc_waypoints where id=%d",start->GetID());
        db->CommandPump("delete from sc_waypoint_aliases where wp_id=%d",start->GetID());

        size_t index = waypoints.Find(start);
        if (index != csArrayItemNotFound)
        {
            // This will delete the start
            waypoints.DeleteIndexFast(index);
            start = NULL;
        }
    }

    if (end->links.GetSize() == 0)
    {
        db->CommandPump("delete from sc_waypoints where id=%d",end->GetID());
        db->CommandPump("delete from sc_waypoint_aliases where wp_id=%d",end->GetID());

        size_t index = waypoints.Find(end);
        if (index != csArrayItemNotFound)
        {
            // This will delete the end
            waypoints.DeleteIndexFast(index);
            end = NULL;
        }
    }
    

    return true;
}
