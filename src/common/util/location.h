/*
 * location.h
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __LOCATION_H__
#define __LOCATION_H__

#include <csutil/csstring.h>
#include <csgeom/vector3.h>
#include <csgeom/box.h>
#include <csutil/parray.h>
#include <csutil/list.h>
#include <iengine/sector.h>
#include <csutil/weakref.h>

#include "util/psconst.h"
#include "util/psdatabase.h"

struct iEngine;
class LocationType;
class psWorld;
class iEffectIDAllocator;

/**
 * \addtogroup common_util
 * @{ */

/**
 * A Location is a named place on the map, located
 * dynamically by NPCs as scripted.
 *
 * @note Location dosn't support multi sector locations yes.
 */
class Location
{
public:
    int                 id;                     ///< The ID of this location, from the DB.
    csString            name;                   ///< The name of this location.
    csVector3           pos;                    ///< The positon of this location
    float               rot_angle;              ///< Some location has an angle.
    float               radius;                 ///< The radius of this locaiton.
    csArray<Location*>  locs;                   ///< A number of points for regions. Including self for first location in region.
    int                 id_prev_loc_in_region;  ///< Prev database ID for a region.
    csString            sectorName;             ///< The sector where this location is located.

    csBox2              boundingBox;            ///< Bounding box used for boundary checks.
    csWeakRef<iSector>  sector;                 ///< Cached sector
    LocationType*       type;                   ///< Points back to location type
    uint32_t            effectID;               ///< When displayed in a client this is the effect id
    Location*           region;                 ///< Pointer to first location in a region.

    /** Constructor
     */
    Location();

    /** Constructor
     */
    Location(LocationType* locationType, const char* locationName, const csVector3 &pos, iSector* sector, float radius, float rot_angle, const csString &flags);

    /** Destructor
     */
    ~Location();

    /** Get the DB ID.
     */
    int GetID() const
    {
        return id;
    }

    /** Set DB ID.
     */
    void SetID(int id)
    {
        this->id = id;
    }

    /** Return the position of this location.
     */
    const csVector3 &GetPosition() const
    {
        return pos;
    }

    /** Get flags.
     * @note No flags implemented yet.
     */
    csString GetFlags() const
    {
        return csString();
    }

    /** Set flags.
     * @note No flags implemented yet.
     */
    bool SetFlags(const csString &flags)
    {
        return false;
    }

    /** Set flag.
     * @note No flags implemented yet.
     */
    bool SetFlag(const csString &flag, bool enable)
    {
        return false;
    }

    /** Get the Radius.
     */
    float GetRadius() const
    {
        return radius;
    }

    /** Set Radius and recalculate the boudning box.
     */
    bool SetRadius(iDataConnection* db, float radius);

    /** Set Radius and recalculate the boudning box.
     */
    void SetRadius(float radius);

    /** Get the rotation angle
     */
    float GetRotationAngle() const
    {
        return rot_angle;
    }

    /** Get the type name of this location.
     */
    const char* GetTypeName() const;

    /** Load a location from the DB
     */
    bool Load(iResultRow &row, iEngine* engine, iDataConnection* db);

    /** Create or update an entry for this location in the DB.
     * @note Will update if there is a id different from -1.
     */
    bool CreateUpdate(iDataConnection* db);

    /** Import a location from an XML document.
     */
    bool Import(iDocumentNode* node, iDataConnection* db, int typeID);

    /** Query if this location is a region.
     * @return return true when this location is a region.
     *
     * @note Region is a location with multiple points.
     */
    bool IsRegion()
    {
        return locs.GetSize() != 0;
    }

    /** Query if this location is a circle
     */
    bool IsCircle()
    {
        return locs.GetSize() == 0;
    }

    /** Return cached sector or find the sector and cache it from engine.
     */
    iSector*            GetSector(iEngine* engine);

    /** Return cached sector or find the sector and cache it from engine.
     */
    iSector*            GetSector(iEngine* engine) const;

    /** Return the bounding box for this location
     *
     * @return Bounding box of the location
     */
    const csBox2 &GetBoundingBox() const;

    /** Function to calculate the bounding box for a location.
     *
     * This function should be called after the location has been
     * loaded or modified.
     */
    void CalculateBoundingBox();

    /** Check if a point is within bounds of this location.
     */
    bool CheckWithinBounds(iEngine* engine,const csVector3 &pos,const iSector* sector);

    /** Get a random position in the location
     *
     * Will return the position found. Do not relay on the
     * state of the parameters if operation failes.
     *
     * @param engine Used to find the sector
     * @param pos    The found position is returned here.
     * @param sector The found sector is returned here.
     *
     * @return True if position is found.
     */
    bool GetRandomPosition(iEngine* engine,csVector3 &pos,iSector* &sector);

    /** retrive a sector ID.
     */
    static int GetSectorID(iDataConnection* db, const char* name);

    /** retrive the name of this location.
     */
    const char* GetName() const
    {
        return name.GetDataSafe();
    }

    /** Set the name.
     */
    void SetName(const csString &name)
    {
        this->name = name;
    }

    /** Return the effect ID for this location or assign a new ID
        @param allocator
     */
    uint32_t GetEffectID(iEffectIDAllocator* allocator);

    /** Adjust the postion of this location.
     */
    bool Adjust(iDataConnection* db, const csVector3 &pos, iSector* sector);

    /** Adjust the postion of this location.
     */
    bool Adjust(iDataConnection* db, const csVector3 &pos, iSector* sector, float rot_angle);

    /** Adjust the postion of this location.
     */
    bool Adjust(const csVector3 &pos, iSector* sector);

    /** Adjust the postion of this location.
     */
    bool Adjust(const csVector3 &pos, iSector* sector, float rot_angle);
    
    /** Insert a new point in a region after this location.
     */
    Location* Insert(iDataConnection* db, csVector3 &pos, iSector* sector);

    /** Insert a new point in a region after this location.
     */
    Location* Insert(int id, csVector3 &pos, iSector* sector);

    /** Create text representation.
     */
    csString ToString() const;
};

/**
 * This stores a vector of positions listing a set of
 * points defining a common type of location, such as
 * a list of burning fires or guard stations--whatever
 * the NPCs need.
 */
class LocationType
{
public:
    int                   id;   ///< The DB ID of this location type.
    csString              name; ///< The name of this location type.
    csArray<Location*>    locs; ///< All the location of this location type.

    /** Constructor
     */
    LocationType();

    /** Constructor
     */
    LocationType(int id, const csString &name);

    /** Destructor
     */
    ~LocationType();

    /** Create or update an entry for this location type in the DB.
     * @note Will update if there is a id different from -1.
     */
    bool CreateUpdate(iDataConnection* db);

    /** Load a location type from an XML file.
     */
    bool Load(iDocumentNode* node);
    bool Import(iDocumentNode* node, iDataConnection* db);
    bool ImportLocations(iDocumentNode* node, iDataConnection* db);

    /** Load a location type from DB.
     */
    bool Load(iResultRow &row, iEngine* engine, iDataConnection* db);

    /** Add a new location to this location type.
     */
    void AddLocation(Location* location);

    /** Remove a location from this location type.
     */
    void RemoveLocation(Location* location);

    /** Check if a point is within any of the locaitons of this location type.
     */
    bool CheckWithinBounds(iEngine* engine,const csVector3 &pos,const iSector* sector);

    /** Get a random position in the location
     *
     * Will return the position found. Do not relay on the
     * state of the parameters if operation failes.
     *
     * @param engine   Used to find the sector
     * @param pos      The found position is returned here.
     * @param sector   The found sector is returned here.
     * @param inSector If set, only search for random pos in this sector.
     *
     * @return True if position is found.
     */
    bool GetRandomPosition(iEngine* engine,csVector3 &pos,iSector* &sector, const iSector* inSector);

    /** retrive the ID of this location type.
     */
    int GetID() const
    {
        return id;
    }

    /** retrive the name of this location type.
     */
    const char* GetName() const
    {
        return name.GetDataSafe();
    }
};

/**
 * Manager that manage all locations and location types.
 */
class LocationManager
{
public:
    /** Constructor
     */
    LocationManager();

    /** Destructor
     */
    virtual ~LocationManager();

    /** Load all locations from DB.
     */
    bool Load(iEngine* engine, iDataConnection* db);

    /** Retrive the number of locations in this manager.
     */
    int GetNumberOfLocations() const;

    /** Retrive one location from this manager by index.
     */
    Location* GetLocation(int index);

    /** Find a region by name.
     */
    LocationType* FindRegion(const char* regname);

    /** Find a location type by name
     */
    LocationType* FindLocation(const char* typeName);

    /** Find a location of a specfic location type by nam.
     */
    Location* FindLocation(const char* typeName, const char* name);

    /** Find a location of a specfic location type by nam.
     */
    Location* FindLocation(LocationType* type, const char* name);

    /** Find a location of a specfic location type by nam.
     */
    Location* FindLocation(int id);

    /** Find all location in given sector.
     */
    size_t FindLocationsInSector(iEngine* engine, iSector* sector, csList<Location*> &list);

    /** Find the neares location to a point.
     */
    Location* FindNearestLocation(psWorld* world, csVector3 &pos, iSector* sector, float range, float* found_range);

    /** Find the neares location to a point of a given location type.
     */
    Location* FindNearestLocation(psWorld* world, const char* typeName, csVector3 &pos, iSector* sector, float range, float* found_range);

    /** Find a random location within a given max range.
     */
    Location* FindRandomLocation(psWorld* world, const char* typeName, csVector3 &pos, iSector* sector, float range, float* found_range);

    /** Get a iterator to all Location Types stored in the Location Manager.
     */
    csHash<LocationType*, csString>::GlobalIterator GetIterator();

    /** Create a new location. And add it to the DB.
     */
    Location* CreateLocation(iDataConnection* db, LocationType* locationType, const char* locationName,
                             const csVector3 &pos, iSector* sector, float radius, float rot_angle,
                             const csString &flags);

    /** Create a new location
     */
    Location* CreateLocation(const char* locationTypeName, const char* locationName,
                             const csVector3 &pos, iSector* sector, float radius, float rot_angle,
                             const csString &flags);

    /** Create a new location
     */
    Location* CreateLocation(LocationType* locationType, const char* locationName,
                             const csVector3 &pos, iSector* sector, float radius, float rot_angle,
                             const csString &flags);

    /** Create a new location type
     */
    LocationType* CreateLocationType(iDataConnection* db, const csString &locationName);

    /** Create a new location type
     */
    LocationType* CreateLocationType(const csString &locationName);

    /** Create a new location type
     */
    LocationType* CreateLocationType(int id, const csString &locationName);

    /** Remove a location type
     */
    bool RemoveLocationType(iDataConnection* db, const csString &locationName);

    /** Remove a location type
     */
    bool RemoveLocationType(const csString &locationName);

private:
    csHash<LocationType*, csString> loctypes;          ///< Hash on all location types, hashed on the type.
    csArray<Location*>              all_locations;     ///< Quick access array to all locations.
};

/** @} */

#endif

