/*
 * pspathnetwork.h
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
#ifndef __PSPATHNETWORK_H__
#define __PSPATHNETWORK_H__

#include <csutil/array.h>
#include <csutil/list.h>

#include <idal.h>

#include "util/pspath.h"

class Edge;
class Waypoint;
class WaypointAlias;
class psPath;
class psWorld;

/**
 * \addtogroup common_util
 * @{ */

/**
 * A network of Waypoint and psPath objects.
 */
class psPathNetwork
{
public:

    /**
     * Template class for implementing waypoint filtering when calculating routes.
     */
    class RouteFilter
    {
      public:
       /**
        * Called to check if a waypoint should be filters.
        */
        virtual bool Filter(const Waypoint* wp) const = 0;
    };


    csPDelArray<Waypoint> waypoints;
    csPDelArray<psPath> paths;
    
    csArray<csString> waypointGroupNames;
    csArray< csList<Waypoint*> > waypointGroups;

    csWeakRef<iEngine> engine;
    csWeakRef<iDataConnection> db;
    psWorld * world;
    
    
    /**
     * Load all waypoins and paths from db
     */
    bool Load(iEngine *engine, iDataConnection *db, psWorld * world);

    /**
     * Add a new waypoint to the given group.
     *
     */
    size_t AddWaypointToGroup(csString group, Waypoint * wp);

    /**
     * Find waypoint by id
     */
    Waypoint *FindWaypoint(int id);

    /**
     * Find waypoint by name
     *
     * @param name The name of the waypoint to find
     * @param alias If not null pointer wil be set if the name is of one of the aliases for the WP found.
     */
    Waypoint *FindWaypoint(const char * name, WaypointAlias** alias = NULL);
    
    /**
     * Find waypoint at a given point in the world.
     *
     * Return the first waypoint where the position
     * is within the radius of the waypoint.
     */
    Waypoint *FindWaypoint(const csVector3& v, iSector *sector);

    /**
     * Find waypoint nearest to a point in the world
     *
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only waypoints within range from waypoint, -1 if range dosn't matter
     * @param[out] found_range The range the waypoint was found at.
     * @return The found waypoint or NULL if no wapoint was found.
     */
    Waypoint *FindNearestWaypoint(const csVector3& v,iSector *sector, float range, float * found_range = NULL);

    /**
     * Find random waypoint within a given range to a point in the world.
     *
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only waypoints within range from waypoint, -1 if range dosn't matter
     * @param[out] found_range The range the waypoint was found at.
     * @return The found waypoint or NULL if no wapoint was found.
     */
    Waypoint *FindRandomWaypoint(const csVector3& v, iSector *sector, float range, float * found_range = NULL);

    /**
     * Find waypoint nearest to a point in the world in the given group.
     *
     * @param group            The group to search in.
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only waypoints within range from waypoint, -1 if range dosn't matter
     * @param[out] found_range The range the waypoint was found at.
     * @return The found waypoint or NULL if no wapoint was found.
     */
    Waypoint *FindNearestWaypoint(int group, const csVector3& v,iSector *sector, float range, float * found_range = NULL);

    /**
     * Find random waypoint within a given range to a point in the world.
     *
     * @param group            The group to search in.
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only waypoints within range from waypoint, -1 if range dosn't matter
     * @param[out] found_range The range the waypoint was found at.
     * @return The found waypoint or NULL if no wapoint was found.
     */
    Waypoint *FindRandomWaypoint(int group, const csVector3& v, iSector *sector, float range, float * found_range = NULL);

    /**
     * Find the index for the given group name, return -1 if no group is found.
     */
    int FindWaypointGroup(const char * groupName);

    /**
     * Find path point by id
     */
    psPathPoint* FindPathPoint(int id);

    /**
     * Find the point nearest to the path.
     *
     * @param path             The path containging the point to search for.
     * @param pos              The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only points within range from given position, -1 if range dosn't matter
     * @param[out] index       Return the index of the path point at the start of this segment.
     */
    psPathPoint* FindPoint(const psPath* path, const csVector3& pos, iSector* sector, float range, int& index);
    
    /**
     * Find the path nearest to a point in the world.
     *
     * @param path             The path containging the point to search for.
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only points within range from given position, -1 if range dosn't matter
     */
    psPathPoint* FindNearestPoint(const psPath* path, const csVector3& v, const iSector *sector, float range);

    /**
     * Find the path nearest to a point in the world.
     *
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only paths within range from given position, -1 if range dosn't matter
     * @param[out] found_range The range the path was found at.
     * @param[out] index       Return the index of the path found.
     * @param[out] fraction    Return the fraction of the path where the positions is closesed.
     * @return The found path or NULL if no path was found.
     */
    psPath *FindNearestPath(const csVector3& v, iSector *sector, float range, float * found_range = NULL, int * index = NULL, float * fraction = NULL);
    
    /**
     * Find the point nearest to a point in the world.
     *
     * @param v                The position to start searching from.
     * @param sector           The sector to search in.
     * @param range            Find only points within range from given position, -1 if range dosn't matter
     * @param[out] found_range The range the point was found at.
     * @param[out] index       Return the index of the point found.
     * @return The found path or NULL if no path was found.
     */
    psPath *FindNearestPoint(const csVector3& v, iSector *sector, float range, float * found_range = NULL, int * index = NULL);
    
    /**
     * Find the shortest route between waypoint start and stop.
     */
    csList<Waypoint*> FindWaypointRoute(Waypoint * start, Waypoint * end, const RouteFilter* routeFilter);

    /**
     * Find the shortest route between waypoint start and stop.
     */
    csList<Edge*> FindEdgeRoute(Waypoint * start, Waypoint * end, const RouteFilter* routeFilter);

    /**
     * Get a list of points in a sector
     */
    size_t FindPointsInSector(iSector *sector, csList<psPathPoint*>& list);

    /**
     * Get a list of waypoints in a sector
     */
    size_t FindWaypointsInSector(iSector *sector, csList<Waypoint*>& list);

    
    /**
     * List all waypoints matching pattern to console.
     */
    void ListWaypoints(const char *pattern);
    
    /**
     * List all paths matching pattern to console.
     */
    void ListPaths(const char *pattern);

    /**
     * Find the named path.
     */
    psPath* FindPath(const char *name);

    /**
     * Find the path.
     */
    psPath* FindPath(int id);
    
    /**
     * Find a given edge from starting waypoint wp1 to end waypoint wp2.
     */
    Edge* FindEdge(const Waypoint * wp1, const Waypoint * wp2);


    /**
     * Create a new waypoint and insert in db.
     */
    Waypoint* CreateWaypoint( iDataConnection *db, csString& name, csVector3& pos, csString& sectorName, float radius, csString& flags);

    /**
     * Create a new waypoint
     */
    Waypoint* CreateWaypoint( csString& name, csVector3& pos, csString& sectorName, float radius, csString& flags);
    
    /**
     * Create a new path/connection/link between two waypoints
     */
    psPath* CreatePath(iDataConnection *db, const csString& name, Waypoint* wp1, Waypoint* wp2, const csString& flags);

    /**
     * Create a new path/connection/link between two waypoints from an external created path object
     */
    psPath* CreatePath(iDataConnection *db, psPath * path);

    /**
     * Create a new path/connection/link between two waypoints from an external created path object
     */
    psPath* CreatePath(psPath * path);
    
    /**
     * Get next unique number for waypoint checking.
     */
    int GetNextWaypointCheck();

    /**
     * Delete the given path from the db.
     */
    bool Delete(psPath * path);
};

/** @} */

#endif
