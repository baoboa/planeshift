/*
    Crystal Space Entity Layer
    Copyright (C) 2010 by Leonardo Rodrigo Domingues
    Copyright (C) 2011 by Matthieu Kraus
  
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
  
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
  
    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef __CEL_NAVMESH__
#define __CEL_NAVMESH__

#include <cssysdef.h>
#include <csgeom/obb.h>
#include <csgeom/plane3.h>
#include <csgeom/tri.h>
#include <csgeom/vector3.h>
#include <csqsqrt.h>
#include <cstool/csapplicationframework.h>
#include <csutil/list.h>
#include <csutil/ref.h>
#include <csutil/scf_implementation.h>
#include <csutil/threadmanager.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <iengine/portalcontainer.h>
#include <iengine/sector.h>
#include <igeom/trimesh.h>
#include <imesh/objmodel.h>
#include <imesh/terrain2.h>
#include <iutil/comp.h>
#include <iutil/document.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <tools/celnavmesh.h>
#include "recastnavigation/ChunkyTriMesh.h"
#include "recastnavigation/DebugDraw.h"
#include "recastnavigation/DetourCommon.h"
#include "recastnavigation/DetourDebugDraw.h"
#include "recastnavigation/DetourNavMesh.h"
#include "recastnavigation/DetourNavMeshBuilder.h"
#include "recastnavigation/Recast.h"

CS_PLUGIN_NAMESPACE_BEGIN(celNavMesh)
{



/*
 * Recast structures
 */
static const int MAX_CONVEXVOL_PTS = 12;
struct ConvexVolume
{
  float verts[MAX_CONVEXVOL_PTS * 3];
  float hmin, hmax;
  int nverts;
  int area;
};

// These are just sample areas to use consistent values across the samples.
// The use should specify these base on his needs.
enum SamplePolyAreas
{
  SAMPLE_POLYAREA_GROUND,
  SAMPLE_POLYAREA_WATER,
  SAMPLE_POLYAREA_ROAD,
  SAMPLE_POLYAREA_DOOR,
  SAMPLE_POLYAREA_GRASS,
  SAMPLE_POLYAREA_JUMP,
};
enum SamplePolyFlags
{
  SAMPLE_POLYFLAGS_WALK = 0x01,		// Ability to walk (ground, grass, road)
  SAMPLE_POLYFLAGS_SWIM = 0x02,		// Ability to swim (water).
  SAMPLE_POLYFLAGS_DOOR = 0x04,		// Ability to move through doors.
  SAMPLE_POLYFLAGS_JUMP = 0x08,		// Ability to jump.
  SAMPLE_POLYFLAGS_ALL = 0xffff		// All abilities.
};

/**
 * CrystalSpace debug draw implementation for Recast & Detour structures.
 * When the begin() method is called, a new csSimpleRenderMesh is created. After
 * that, whenever one of the vertex() methods is called, a new vertex is added to
 * a list. Finally, when the end() method is called, the vertices are copied to
 * the csSimpleRenderMesh, and this method is added to an array. This array can be
 * later retrieved using the GetMeshes() method.
 */
class DebugDrawCS : public duDebugDraw
{
private:
  csSimpleRenderMesh* currentMesh;
  csZBufMode currentZBufMode;
  csArray<csSimpleRenderMesh*>* meshes;
  csArray<csVector3> vertices;
  csArray<csVector4> colors;
  int nVertices;
  
public:
  DebugDrawCS ();
  virtual ~DebugDrawCS ();
  csArray<csSimpleRenderMesh*>* GetMeshes ();

  // duDebugDraw
  virtual void depthMask (bool state);
	virtual void texture(bool state);
  virtual void begin (duDebugDrawPrimitives prim, float size = 1.0f);
  virtual void vertex (const float* pos, unsigned int color);
  virtual void vertex (const float x, const float y, const float z, unsigned int color);
	virtual void vertex (const float* pos, unsigned int color, const float* uv);
	virtual void vertex (const float x, const float y, const float z, unsigned int color, const float u, const float v);
  virtual void end ();
};



/**
 * Wrapper for the parameters used to build the navigation mesh.
 */
class celNavMeshParams : public scfImplementation1<celNavMeshParams, iCelNavMeshParams>
{
private:
  float agentHeight;
  float agentRadius;
  float agentMaxSlopeAngle;
  float agentMaxClimb;
  float cellSize;
  float cellHeight;
  float maxSimplificationError;
  float detailSampleDist;
  float detailSampleMaxError;
  float distanceThreshold;
  int maxEdgeLength;
  int minRegionArea;
  int mergeRegionArea;
  int maxVertsPerPoly;
  int tileSize;
  int borderSize;
  csVector3 polygonSearchBox;

public:
  celNavMeshParams ();
  celNavMeshParams (const iCelNavMeshParams* parameters);
  virtual ~celNavMeshParams ();

  iCelNavMeshParams* Clone () const;
  virtual void SetSuggestedValues (float agentHeight, float agentRadius, float agentMaxSlopeAngle);

  virtual float GetAgentHeight () const;
  virtual void SetAgentHeight (const float height);
  virtual float GetAgentRadius () const;
  virtual void SetAgentRadius (const float radius);
  virtual float GetAgentMaxSlopeAngle () const;
  virtual void SetAgentMaxSlopeAngle (const float angle);
  virtual float GetAgentMaxClimb () const;
  virtual void SetAgentMaxClimb (const float maxClimb);
  virtual float GetCellSize () const;
  virtual void SetCellSize (const float size);
  virtual float GetCellHeight () const;
  virtual void SetCellHeight (const float height);
  virtual float GetMaxSimplificationError () const;
  virtual void SetMaxSimplificationError (const float error);
  virtual float GetDetailSampleDist () const;
  virtual void SetDetailSampleDist (const float dist);
  virtual float GetDetailSampleMaxError () const;
  virtual void SetDetailSampleMaxError (const float error);
  virtual int GetMaxEdgeLength () const;
  virtual void SetMaxEdgeLength (const int length);
  virtual int GetMinRegionArea () const;
  virtual void SetMinRegionArea (const int area);
  virtual int GetMergeRegionArea () const;
  virtual void SetMergeRegionArea (const int area);
  virtual int GetMaxVertsPerPoly () const;
  virtual void SetMaxVertsPerPoly (const int maxVerts);
  virtual int GetTileSize () const;
  virtual void SetTileSize (const int size);
  virtual int GetBorderSize () const;
  virtual void SetBorderSize (const int size);
  virtual csVector3 GetPolygonSearchBox () const;
  virtual void SetPolygonSearchBox (const csVector3 box);
};



/**
 * Path between two points which lay in the same Sector.
 */
// Based on Recast NavMeshTesterTool
class celNavMeshPath : public scfImplementation1<celNavMeshPath, iCelNavMeshPath>
{
private:
  float* path;
  int pathSize; // In nodes
  int maxPathSize;
  int currentPosition; // Path array position, not point index
  int increasePosition; // Value to be added to currentPosition to get next element
  static const int INCREASE_PATH_BY; // Increase path vector by this amount when the it gets full
  csRef<iSector> sector;
  csArray<csSimpleRenderMesh*>* debugMeshes;

public:
  celNavMeshPath (float* path, int pathSize, int maxPathSize, iSector* sector);
  virtual ~celNavMeshPath ();

  // API
  virtual iSector* GetSector () const;
  virtual void Current (csVector3& vector) const;
  virtual void Next (csVector3& vector);
  virtual void Previous (csVector3& vector);
  virtual void GetFirst (csVector3& vector) const;
  virtual void GetLast (csVector3& vector) const;
  virtual bool HasNext () const;
  virtual bool HasPrevious () const;
  virtual void Invert ();
  virtual void Restart ();
  virtual void AddNode (csVector3 node);  
  virtual void InsertNode (int pos, csVector3 node);
  virtual float Length () const;
  virtual int GetNodeCount () const;
  virtual csArray<csSimpleRenderMesh*>* GetDebugMeshes ();
};



/**
 * Polygon mesh representing the navigable areas of a Sector.
 */
class celNavMesh : public scfImplementation1<celNavMesh, iCelNavMesh>
{
private:
  struct NavMeshSetHeader
  {
    int magic;
    int version;
    int numTiles;
    dtNavMeshParams params;
  };

  struct NavMeshTileHeader
  {
    dtTileRef tileRef;
    int dataSize;
  };

  csRef<iSector> sector;
  csRef<iObjectRegistry> objectRegistry;
  csRef<iCelNavMeshPath> path;
  dtQueryFilter filter;
  dtNavMesh* detourNavMesh;
  dtNavMeshQuery* detourNavMeshQuery;
  csRef<iCelNavMeshParams> parameters;
  csArray<csSimpleRenderMesh*>* debugMeshes;
  csArray<csSimpleRenderMesh*>* agentDebugMeshes;
  float boundingMin[3];
  float boundingMax[3];
  unsigned char navMeshDrawFlags;
  static const int MAX_NODES;
  static const int NAVMESHSET_MAGIC;
  static const int NAVMESHSET_VERSION;

public:
  celNavMesh (iObjectRegistry* objectRegistry);
  virtual ~celNavMesh ();

  bool Initialize (const iCelNavMeshParams* parameters, iSector* sector, const float* boundingMin, 
                   const float* boundingMax);
  bool AddTile (unsigned char* data, int dataSize);
  bool RemoveTile (int x, int y);
  bool LoadNavMesh (iFile* file);

  // API
  virtual iCelNavMeshPath* ShortestPath (const csVector3& from, const csVector3& goal, int maxPathSize = 32);
  virtual bool Update (const csBox3& boundingBox);
  virtual bool Update (const csOBB& boundingBox);
  virtual iSector* GetSector () const;
  virtual void SetSector (iSector* sector);
  virtual iCelNavMeshParams* GetParameters () const;
  virtual csBox3 GetBoundingBox() const;
  virtual csArray<csPoly3D> QueryPolygons(const csBox3& box) const;
  virtual bool SaveToFile (iFile* file) const;
  virtual csArray<csSimpleRenderMesh*>* GetDebugMeshes ();
  virtual csArray<csSimpleRenderMesh*>* GetAgentDebugMeshes (const csVector3& pos);
  virtual csArray<csSimpleRenderMesh*>* GetAgentDebugMeshes (const csVector3& pos, int red, int green, 
                                                           int blue, int alpha);  
  virtual void ResetAgentDebugMeshes ();
};



/**
 * Navigation mesh creator.
 */
class celNavMeshBuilder : public ThreadedCallable<celNavMeshBuilder>,
                          public scfImplementation2<celNavMeshBuilder, iCelNavMeshBuilder, iComponent>
{
private:
  // Crystal space & CEL
  csRef<iObjectRegistry> objectRegistry;
  csRef<iSector> currentSector;
  csRef<iStringSet> strings;

  // Recast & Detour
  rcChunkyTriMesh* chunkyTriMesh;
  
  // Tile specific
  unsigned char* triangleAreas;
  rcContext dummy;
  rcHeightfield* solid;
  rcCompactHeightfield* chf;
  rcContourSet* cSet;
  rcPolyMesh* pMesh;
  rcPolyMeshDetail* dMesh;
  
  // Off-Mesh connections.
  static const int MAX_OFFMESH_CONNECTIONS = 256;
  float offMeshConVerts[MAX_OFFMESH_CONNECTIONS * 3 * 2];
  float offMeshConRads[MAX_OFFMESH_CONNECTIONS];
  unsigned char offMeshConDirs[MAX_OFFMESH_CONNECTIONS];
  unsigned char offMeshConAreas[MAX_OFFMESH_CONNECTIONS];
  unsigned short offMeshConFlags[MAX_OFFMESH_CONNECTIONS];
  int numberOfOffMeshCon;

  // Convex Volumes
  static const int MAX_VOLUMES = 256;
  ConvexVolume volumes[MAX_VOLUMES];
  int numberOfVolumes;

  csRef<iCelNavMeshParams> parameters;
  csRef<celNavMesh> navMesh;

  // Others
  int numberOfVertices;
  float* triangleVertices;
  int numberOfTriangles;
  int* triangleIndices;
  float boundingMin[3];
  float boundingMax[3];

  void CleanUpSectorData ();
  void CleanUpTileData ();
  bool GetSectorData ();  
  unsigned char* BuildTile(const int tx, const int ty, const float* bmin, const float* bmax, 
                           const rcConfig& tileConfig, int& dataSize);
  iObjectRegistry* GetObjectRegistry() const { return objectRegistry; }

  // helper function to check whether an object has to be clipped
  // stores whether the object has to be clipped in result and returns true if it's visible
  bool CheckClipping(const csPlane3& clipPlane, const csBox3& bbox, bool& result);

  // helper function to clip a convex polygon to a plane and triangulate it apllying an object to world transform
  void SplitPolygon(int indexOffset, int numVerts, csVector3* poly, csArray<csVector3>& vertices, csArray<csTriangle>& triangles,
                    const csReversibleTransform& t, csPlane3& clipPlane, bool clipPolygon);

  // helper function to add a bul of triangles to the sector data

public:
  celNavMeshBuilder (iBase* parent);
  virtual ~celNavMeshBuilder ();
  virtual bool Initialize (iObjectRegistry*);
  bool UpdateNavMesh (celNavMesh* navMesh, const csBox3& boundingBox);

  // API
  virtual bool SetSector (iSector* sector);
  THREADED_CALLABLE_DECL(celNavMeshBuilder,BuildNavMesh,csThreadReturn,THREADEDL,false,false);
  virtual iCelNavMesh* LoadNavMesh (iFile* file);
  virtual const iCelNavMeshParams* GetNavMeshParams () const;
  virtual void SetNavMeshParams (const iCelNavMeshParams* parameters);
  virtual iSector* GetSector () const;

};

}
CS_PLUGIN_NAMESPACE_END(celNavMesh)

#endif // __CEL_NAVMESH__
