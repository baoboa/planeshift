/*
 * edge.h
 *
 * Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __EDGE_H__
#define __EDGE_H__

#include "util/pspath.h"
#include "util/pspathnetwork.h"

// Forward declaration of types
class psPathPoint;

/**
 * \addtogroup common_util
 * @{ */

/** Represents the Edge in a PathNetwork.
 *
 *  Points to the psPath that holds the actual path and keep
 *  track of the direction of the path for this edge.
 *  The edge is unidirectional, so for a bidirectional connection
 *  between two waypoints on edge for each waypoint should be created.
 */
class Edge
{
private:

    psPath*            path;          ///< The path for this edge
    psPath::Direction  direction;     ///< The direction this edge go along the path

    Waypoint*          startWaypoint; ///< Cached waypoint for the start of the path
    Waypoint*          endWaypoint;   ///< Cached waypoint for the start of the path

public:

    /** Create a new Edge for a path in the given direction
     */
    Edge(psPath* path, psPath::Direction direction);
    

    /** Destructor
     */
    virtual ~Edge();


    /** Return the first point in this edge
     */
    psPathPoint* GetStartPoint();

    /** Return the end point in this edge
     */
    psPathPoint* GetEndPoint();

    /** Return the path
     */
    psPath* GetPath() { return path; }

    /** Return the direction of path this edge is going
     */
    psPath::Direction GetDirection() { return direction; }
    
    /** Return the first waypoint in the edge
     */
    Waypoint* GetStartWaypoint();

    /** Return the last waypoint in the edge
     */
    Waypoint* GetEndWaypoint();

    /** Get a random edge from this.
     */
    Edge* GetRandomEdge(const psPathNetwork::RouteFilter* routeFilter);

    /** Check if this edge a teleport edge.
     */
    bool IsTeleport() const;

    /** Check if wander is allowed.
     */
    bool NoWander() const;

    /** Iterator base class for Forward and Reverse Iterators
     */
    class Iterator
    {
      public:
        /** Constructor */
        Iterator(){};

        /** Destructor */
        virtual ~Iterator(){};

        /** Check if there is more elements.
         */
        virtual bool HasNext() = 0;

        /** Return next element.
         *
         *  Return the next element. Call HasNext
         *  before calling this to see if there
         *  are more elements.
         */
        virtual psPathPoint* Next() = 0;
    };

    /** Return the iterator for this edge.
     *
     *  Return a pointer to an newly allocated
     *  Forward or Reverce Iterator. User should
     *  delete the iterator when done.
     */
    Iterator* GetIterator();

protected:

    /** Implementation of Iterator for FORWARD direction.
     */
    class ForwardIterator: public Iterator
    {
      private:
        psPath::PathPointArray::Iterator pointIterator;
      public:

        ForwardIterator(Edge* edge):
            pointIterator(edge->path->points.GetIterator())
        {
        }

        virtual bool HasNext()
        {
            return pointIterator.HasNext();
        }

        virtual psPathPoint* Next()
        {
            return pointIterator.Next();
        }
    };

    /** Implementation of Iterator for REVERSE direction.
     */
    class ReverseIterator: public Iterator
    {
      private:
        psPath::PathPointArray::ReverseIterator pointIterator;
      public:

        ReverseIterator(Edge* edge):
            pointIterator(edge->path->points.GetReverseIterator())
        {
        }

        virtual bool HasNext()
        {
            return pointIterator.HasNext();
        }

        virtual psPathPoint* Next()
        {
            return pointIterator.Next();
        }
    };

};

/** @} */

#endif
