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

#ifndef __CEL_HPF__
#define __CEL_HPF__

#include <cssysdef.h>
#include <csgeom/math3d.h>
#include <csgeom/vector3.h>
#include <cstool/mapnode.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/scf_implementation.h>
#include <iengine/sector.h>
#include <iutil/comp.h>
#include <iutil/document.h>
#include <iutil/objreg.h>
#include <ivaria/mapnode.h>
#include <tools/celgraph.h>
#include <tools/celnavmesh.h>
#include <tools/celhpf.h>
#include "celnavmesh.h"

CS_PLUGIN_NAMESPACE_BEGIN(celNavMesh)
{


/**
 * Path between two map nodes.
 */
class celHPath : public scfImplementation1<celHPath, iCelHPath>
{
private:
  csRef<iCelPath> hlPath; // High level path
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >& navMeshes;
  csArray<csRef<iCelNavMeshPath> > llPaths; // Low level paths
  size_t currentllPosition; // Current position for low level paths array
  csRef<iMapNode> currentNode;
  csPtrKey<iSector> currentSector;
  csRef<iMapNode> firstNode; // Optimization for celHPath::GetFirst
  csRef<iMapNode> lastNode; // Optimization for celHPath::GetLast
  csArray<csSimpleRenderMesh*>* debugMeshes;
  bool reverse;
  float length;
  float advanced;

  virtual bool HasNextInternal (bool reverse) const;
  virtual iMapNode* NextInternal (bool reverse);

public:
  celHPath (csHash<csRef<iCelNavMesh>, csPtrKey<iSector> >& navMeshes);
  virtual ~celHPath ();

  void Initialize(iCelPath* highLevelPath);

  // API
  virtual bool HasNext () const;
  virtual bool HasPrevious () const;
  virtual iMapNode* Next ();
  virtual iMapNode* Previous ();
  virtual iMapNode* Current ();
  virtual iMapNode* GetFirst ();
  virtual iMapNode* GetLast ();
  virtual void Invert ();
  virtual void Restart ();
  virtual float GetDistance () const;
  virtual csArray<csSimpleRenderMesh*>* GetDebugMeshes ();
};



/**
 * Hierarchical navigation mesh representing the navigable areas of a Map.
 */
class celHNavStruct : public scfImplementation1<celHNavStruct, iCelHNavStruct>
{
private:
  csRef<iObjectRegistry> objectRegistry;
  csRef<iCelNavMeshParams> parameters;
  csHash<csRef<iCelNavMesh>, csPtrKey<iSector> > navMeshes;
  csRef<iCelGraph> hlGraph; // High level graph
  csRef<celHPath> path;
  csArray<csSimpleRenderMesh*>* debugMeshes;

  // Helpers for the SaveToFile method
  void SaveParameters (iDocumentNode* node);
  void SaveNavMeshes (iDocumentNode* node, iVFS* vfs);
  void SaveHighLevelGraph (iDocumentNode* node1, iDocumentNode* node2);

public:
  celHNavStruct (const iCelNavMeshParams* params, iObjectRegistry* objectRegistry);
  virtual ~celHNavStruct ();

  void AddNavMesh(iCelNavMesh* navMesh);
  bool BuildHighLevelGraph();
  void SetHighLevelGraph(iCelGraph* graph);

  // API
  virtual iCelHPath* ShortestPath (const csVector3& from, iSector* fromSector, const csVector3& goal,
                                   iSector* goalSector);
  virtual iCelHPath* ShortestPath (iMapNode* from, iMapNode* goal);
  virtual bool Update (const csBox3& boundingBox, iSector* sector = 0);
  virtual bool Update (const csOBB& boundingBox, iSector* sector = 0);
  virtual bool SaveToFile (iVFS* vfs, const char* directory);
  virtual const iCelNavMeshParams* GetNavMeshParams () const;
  virtual csArray<csSimpleRenderMesh*>* GetDebugMeshes (iSector* sector = 0);
  virtual csArray<csSimpleRenderMesh*>* GetAgentDebugMeshes (const csVector3& pos, int red, int green, 
                                                           int blue, int alpha);
  virtual void ResetAgentDebugMeshes ();
};



/**
 * Hierarchical navigation mesh creator.
 */
class celHNavStructBuilder : public scfImplementation2<celHNavStructBuilder, iCelHNavStructBuilder, iComponent>
{
private:
  csRef<iObjectRegistry> objectRegistry;
  csRef<iCelNavMeshParams> parameters;
  csRefArray<iSector> sectors;
  csHash<csRef<iCelNavMeshBuilder>, csPtrKey<iSector> > builders;
  csRef<celHNavStruct> navStruct;

  bool InstantiateNavMeshBuilders();

  // Helpers for LoadFromFile()
  bool ParseParameters (iDocumentNode* node, iCelNavMeshParams* params);
  bool ParseMeshes (iDocumentNode* node, csHash<csRef<iSector>, const char*>& sectors, 
                    celHNavStruct* navStruct, iVFS* vfs, iCelNavMeshParams* params);
  bool ParseGraph (iDocumentNode* node, iCelGraph* graph, csHash<csRef<iSector>, const char*> sectors);  

public:
  celHNavStructBuilder (iBase* parent);
  virtual ~celHNavStructBuilder ();
  virtual bool Initialize (iObjectRegistry*);

  // API
  virtual bool SetSectors (csRefArray<iSector>* sectorList);
  virtual iCelHNavStruct* BuildHNavStruct ();
  virtual iCelHNavStruct* LoadHNavStruct (iVFS* vfs, const char* directory);
  virtual const iCelNavMeshParams* GetNavMeshParams () const;
  virtual void SetNavMeshParams (const iCelNavMeshParams* parameters);
};

}
CS_PLUGIN_NAMESPACE_END(celNavMesh)

#endif // __CEL_HPF__
