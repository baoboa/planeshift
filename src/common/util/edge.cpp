/*
 * edge.cpp
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
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================


//=============================================================================
// Project Includes
//=============================================================================
#include "util/psutil.h"
#include "util/edge.h"
#include "util/waypoint.h"

Edge::Edge(psPath* path, psPath::Direction direction)
    :path(path),direction(direction),startWaypoint(path->GetStartWaypoint(direction)),
     endWaypoint(path->GetEndWaypoint(direction))
{
}

Edge::~Edge()
{
}


psPathPoint* Edge::GetStartPoint()
{
    return path->GetStartPoint(direction);
}

psPathPoint* Edge::GetEndPoint()
{
    return path->GetEndPoint(direction);
}

Waypoint* Edge::GetStartWaypoint()
{
    return startWaypoint;
}

Waypoint* Edge::GetEndWaypoint()
{
    return endWaypoint;
}


Edge* Edge::GetRandomEdge(const psPathNetwork::RouteFilter* routeFilter)
{

    // If there are only one way out don't bother to find if its legal
    if (endWaypoint->edges.GetSize() == 1)
    {
        return endWaypoint->edges[0];
    }
    else if (endWaypoint->edges.GetSize() == 0)
    {
        return NULL; // No point in trying to find a way out here
    }

    csArray<Edge*> edges; // List of available waypoints

    // Calculate possible edges
    for (size_t ii = 0; ii < endWaypoint->edges.GetSize(); ii++)
    {
        Edge *edge = endWaypoint->edges[ii];

        if ( ((edge->endWaypoint == startWaypoint && edge->endWaypoint->allowReturn)||
              (edge->endWaypoint != startWaypoint)) &&
             (!edge->NoWander()) &&
             (!routeFilter->Filter(edge->endWaypoint)) )
        {
            edges.Push(edge);
        }
    }

    // If no path out of is possible to go, just return.
    if (edges.GetSize() == 0)
    {
        return endWaypoint->edges[0];
    }
    

    return edges[psGetRandom((int)edges.GetSize())];
}

bool Edge::IsTeleport() const
{
    return path->teleport;
}

bool Edge::NoWander() const
{
    return path->noWander;
}

Edge::Iterator* Edge::GetIterator()
{
    if (direction == psPath::FORWARD)
    {
        return new ForwardIterator(this);
    }
    else
    {
        return new ReverseIterator(this);
    }
}

