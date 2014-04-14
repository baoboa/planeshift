/*
 * pspath.h
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
#ifndef __PSPATH_H__
#define __PSPATH_H__

#include <csutil/weakref.h>
#include <csutil/parray.h>
#include <csutil/list.h>
#include <igeom/path.h>
#include <csgeom/vector3.h>

#include <idal.h>
#include <psstdint.h>

class Waypoint;
class psPathAnchor;
class psWorld;
struct iMovable;
struct iSector;
struct iEngine;
class Location;
class psString;
class psPath;

/**
 * \addtogroup common_util
 * @{ */

/**
 * Helper interface to allocate effect IDs
 */
class iEffectIDAllocator
{
public:
    virtual uint32_t GetEffectID() = 0;
};


/**
   Represents a point on a path between two waypoints.
 */
class psPathPoint
{
    friend class psPathNetwork;
    friend class psPath;
    friend class psLinearPath;
    friend class psPathAnchor;
public:
    psPathPoint(psPath* parentPath);

    /// Load the point from the db row
    bool Load(iResultRow& row, iEngine *engine);
    
    /// Establish this path point in the db
    bool Create(iDataConnection * db, int pathID);

    /// Remove this path point in the db
    bool Remove(iDataConnection * db);

    /// Update this path point in the db
    bool UpdatePrevPointId(iDataConnection * db, int prevPointId);
    
    /// Adjust the point position
    bool Adjust(iDataConnection * db, csVector3 & pos, csString sector);
    
    /** Adjust the point position
     */
    void Adjust(csVector3 & pos, csString sector);
    
    /** Adjust the point position
     */
    void Adjust(csVector3 & pos, iSector* sector);

    /// Set the previous point id
    void SetPrevious(int previous) { prevPointId = previous; }

    /// Get the previous point id
    int GetPreviousPointId() const { return prevPointId; }

    int GetID() const { return id; }
    void SetID(int id) { this->id = id; }

    const csVector3& GetPosition() const { return pos; }
    iSector * GetSector(iEngine *engine);
    iSector * GetSector(iEngine *engine) const;
    float GetRadius();
    float GetRadius() const;

    void SetWaypoint(Waypoint* waypoint);

    Waypoint* GetWaypoint();

    /** Return the db id of the point
    */
    int GetID();

    /** Return the name of this path point.
     *
     * Will only have a name if this is a waypoint as well.*/
    csString GetName();

    /** Return the effect ID for this pathpoint or assign a new ID
        @param allocator 
     */
    uint32_t GetEffectID(iEffectIDAllocator* allocator);

    /** Return the parent path for this path point
     */
    psPath* GetPath() const;

    /** Return the index of this point in the path
     */
    int GetPathIndex() const;
    
private:
    ///////////////////
    // Database data
    int                    id;
    int                    prevPointId;
    csVector3              pos;
    csString               sectorName;       /**< Should really only be the pointer, but
                                                  since sector might not be available
                                                  when loaded we use the name for now.**/

    ///////////////////
    // Internal data
    float                  radius;           ///< The radious of ths path point. Extrapolated
                                             ///< from start to end waypoint radiuses.
    csWeakRef<iSector>     sector;           ///< Cached sector
    float                  startDistance[2]; ///< Start distance for FORWARD and REVERS    
    Waypoint*              waypoint;         ///< Pointer to the waypoint if this point is on a wp.
    uint32_t               effectID;         ///< When displayed in a client this is the effect id.
    psPath*                path;             ///< Pointer to the path that this point is a part of.
};

class psPath
{
    friend class psPathAnchor;
public:
    typedef enum {
        FORWARD,
        REVERSE
    } Direction;

    typedef csPDelArray<psPathPoint> PathPointArray;
            
    int                id;
    csString           name;
    Waypoint*          start;   ///< This path start waypoint
    Waypoint*          end;     ///< This path end waypoint
    PathPointArray     points;  ///< The points this path consists of including start wp and end wp

    /// Flags
    bool                   oneWay;
    bool                   noWander;
    bool                   teleport; ///< Teleport between the waypoints
    
    bool                   precalculationValid;
    float                  totalDistance;
    
    psPath(csString name, Waypoint * wp1, Waypoint * wp2, psString flagStr);
    psPath(int pathID, csString name, psString flagStr);
    
    virtual ~psPath();

    /// Load the path from the db
    bool Load(iDataConnection * db, iEngine *engine);

    /// Create a path in the db
    bool Create(iDataConnection *db);

    /// Adjust a point in the path
    bool Adjust(iDataConnection * db, int index, csVector3 & pos, csString sector);

    /// Add a new point to the path
    psPathPoint* AddPoint(Location * loc, bool first = false);

    /// Add a new point to the path and update db
    psPathPoint* AddPoint(iDataConnection *db, const csVector3& pos, const char * sectorName, bool first = false);
    
    /// Add a new point to the path
    psPathPoint* AddPoint(const csVector3& pos, float radius, const char * sectorName, bool first = false);

    /// Insert a new point to the path and update db
    psPathPoint* InsertPoint(iDataConnection *db, int index, const csVector3& pos, const char * sectorName);

    /// Remova a point from the path and update db
    bool RemovePoint(iDataConnection *db, int index);

    /// Remova a point from the path
    bool RemovePoint(int index);
    
    /// Remova a point from the path and update db
    bool RemovePoint(iDataConnection *db, psPathPoint* point);

    /// Update the indexes, after insert and removal of points.
    bool UpdatePrevPointIndexes(iDataConnection* db);
    
    /// Set the start of the path
    void SetStart(Waypoint * wp);

    /// Set the end of the path
    void SetEnd(Waypoint * wp);

    /// Precalculate values needed for anchors
    virtual void Precalculate(psWorld * world, iEngine *engine, bool forceUpdate = false);

    /// Calculate distance from point to path
    virtual float Distance(psWorld * world, iEngine *engine,const csVector3& pos, const iSector* sector, int * index = NULL, float * fraction = NULL) const;

    /// Calculate distance from point to path points
    virtual float DistancePoint(psWorld * world, iEngine *engine,const csVector3& pos, const iSector* sector, int * index = NULL, bool include_ends = false) const;

    /// Get the start point
    psPathPoint* GetStartPoint(Direction direction);
    
    /// Get the end point
    psPathPoint* GetEndPoint(Direction direction);

    /// Get the start waypoint
    Waypoint* GetStartWaypoint(Direction direction);
    
    /// Get the end waypoint
    Waypoint* GetEndWaypoint(Direction direction);
    
    /// Get the end point position
    csVector3 GetEndPos(Direction direction);
    
    /// Get the end rotation
    float GetEndRot(Direction direction);
    
    /// Get the end sector
    iSector* GetEndSector(iEngine * engine, Direction direction);

    
    /// Return a path anchor to this path
    virtual psPathAnchor* CreatePathAnchor();

    /// Get number of points in path
    virtual int GetNumPoints () const { return (int)points.GetSize(); }

    /// Get name of the path
    virtual const char* GetName() const { return name.GetDataSafe(); }

    /// Get ID of the path
    int GetID() const { return id; }

    /// Rename the path and update the db
    bool Rename(iDataConnection * db,const char* name);
    
    /// Rename the path
    void Rename(const char* name);

    /// Get the length of one segment of the path.
    virtual float GetLength(psWorld * world, iEngine *engine, int index);

    /// Get the total length of all path segments.
    virtual float GetLength(psWorld * world, iEngine *engine);

    /// Utility function to calcualte angle to point between to points
    float CalculateIncidentAngle(csVector3& pos, csVector3& dest);
    
    /// Return a string of flags
    csString GetFlags() const;

    /// Set the flags from a string
    void SetFlags(const psString & flagStr);

    /// Set the flags from a string and update the db.
    bool SetFlag(iDataConnection * db, const csString &flagstr, bool enable);

    /// Set the flags from a string
    bool SetFlag(const csString &flagstr, bool enable);

    /// Get all points in the given sector for this path
    size_t FindPointsInSector(iEngine * engine, iSector *sector, csList<psPathPoint*>& list);

    /// Get Path Point by id
    psPathPoint* FindPoint(int id);

    /// Get Path Point index by id
    int FindPointIndex(int id) const;

    /// Get Path Point index by point
    int FindPointIndex(const psPathPoint* point) const;
    
    // Get Path Point by index
    psPathPoint* GetPoint(int index);

    // Get Path Point by index
    const psPathPoint* GetPoint(int index) const;
    
protected:
    /// Do the actual precalculate work
    virtual void PrecalculatePath(psWorld * world, iEngine *engine) = 0;

    /// Get the interpolated position.
    virtual void GetInterpolatedPosition (int index, float fraction, csVector3& pos) = 0;

    /// Get the interpolated up vector.
    virtual void GetInterpolatedUp (int index, float fraction, csVector3& up) = 0;
    
    /// Get the interpolated forward vector.
    virtual void GetInterpolatedForward (int index, float fraction, csVector3& forward) = 0;
};

class psLinearPath: public psPath
{
public:
    psLinearPath(csString name, Waypoint * wp1, Waypoint * wp2, psString flagStr);
    psLinearPath(int pathID, csString name, psString flagStr);
    virtual ~psLinearPath(){};

protected:
    /// Do the actual precalculate work
    virtual void PrecalculatePath(psWorld * world, iEngine *engine);

    /// Get the interpolated position.
    virtual void GetInterpolatedPosition (int index, float fraction, csVector3& pos);

    /// Get the interpolated up vector.
    virtual void GetInterpolatedUp (int index, float fraction, csVector3& up);
    
    /// Get the interpolated forward vector.
    virtual void GetInterpolatedForward (int index, float fraction, csVector3& forward);
    
private:
    csArray<float> dx;
    csArray<float> dy;
    csArray<float> dz;
};

// Warning: This is BROKEN and does not work across sectors.
class psPathAnchor
{
public:
    psPathAnchor(psPath * path);
    virtual ~psPathAnchor() {}

    /// Calculate internal values for this path given some distance value.
    virtual bool CalculateAtDistance(psWorld * world, iEngine *engine, float distance, psPath::Direction direction);

    /// Get the interpolated position.
    virtual void GetInterpolatedPosition (csVector3& pos);

    /// Get the interpolated up vector.
    virtual void GetInterpolatedUp (csVector3& up);
    
    /// Get the interpolated forward vector.
    virtual void GetInterpolatedForward (csVector3& forward);

    /// Extrapolate the movable delta distance along the path
    /// Warning: Use ExtrapolatePosition with CD off.
    virtual bool Extrapolate(psWorld * world, iEngine *engine, float delta, psPath::Direction direction, iMovable* movable);


    /// Get the current distance this anchor has moved along the path
    float GetDistance(){ return pathDistance; }
    /// Get the index to the current path point
    int GetCurrentAtIndex() { return currentAtIndex; }
    /// Get the current direction
    psPath::Direction GetCurrentAtDirection() { return currentAtDirection; }
    /// Get the current fraction of the current path segment
    float GetCurrentAtFraction(){ return currentAtFraction; }

private:
    psPath * path;
    // Internal non reentrant data leagal after calcuateAt operation
    int                    currentAtIndex;
    psPath::Direction      currentAtDirection;
    float                  currentAtFraction;

    float                  pathDistance;    
};

/** @} */

#endif
