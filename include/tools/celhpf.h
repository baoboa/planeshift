/*
    Crystal Space Entity Layer
    Copyright (C) 2010 by Leonardo Rodrigo Domingues
  
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

#ifndef __CEL_HPFAPI__
#define __CEL_HPFAPI__

#include <csutil/scf.h>
#include <csutil/list.h>
#include <tools/celnavmesh.h>

#include "iutil/vfs.h"

class csBox3;
class csOBB;
class csString;
class csVector3;
struct csSimpleRenderMesh;
struct iCelNavMeshParams;
struct iCelPath;
struct iSector;
struct iSectorList;
struct iMapNode;



/**
 * Hierarchical path between two points.
 */
struct iCelHPath : public virtual iBase
{
  SCF_INTERFACE (iCelHPath, 1, 0, 0);

  /// Check if the path can be transversed forward from the current position.
  virtual bool HasNext () const = 0;

  /// Check if the path can be transversed backward from the current position.
  virtual bool HasPrevious () const = 0;

  /// Get next node.
  virtual iMapNode* Next () = 0;
  
  /// Get previous node.
  virtual iMapNode* Previous () = 0;
    
  /// Get current node.
  virtual iMapNode* Current () = 0;

  /// Get first node.
  virtual iMapNode* GetFirst () = 0;
  
  /// Get last node.
  virtual iMapNode* GetLast () = 0;
  
  /// Invert path.
  virtual void Invert () = 0;

  /// Restart path.  
  virtual void Restart () = 0;

  /// Distance to goal.
  virtual float GetDistance () const = 0;
  
  /**
   * Render path.
   */
  virtual csArray<csSimpleRenderMesh*>* GetDebugMeshes () = 0;
};



/**
 * Hierarchical navigation structure representing the navigable areas of a Map.
 */
struct iCelHNavStruct : public virtual iBase
{
  SCF_INTERFACE (iCelHNavStruct, 1, 1, 0);

  /**
   * Find the shortest path between two points.
   * \param from Origin coordinates.
   * \param fromSector Origin sector.
   * \param goal Destination coordinates.
   * \param goalSector Destination sector.
   * \return Pointer to the shortest path between the two points, or 0 in case something went wrong.
   * \remarks The path returned by this method will be rendered useless if the originating iCelHNavStruct
   *          is destroyed.
   */
  virtual iCelHPath* ShortestPath (const csVector3& from, iSector* fromSector, const csVector3& goal,
    iSector* goalSector) = 0;

  /**
   * Find the shortest path between two points.
   * \param from Origin of the path.
   * \param goal Destination of the path.
   * \return Pointer to the shortest path between the two points, or 0 in case something went wrong.
   * \remarks The path returned by this method will be rendered useless if the originating iCelHNavStruct
   *          is destroyed.
   */
  virtual iCelHPath* ShortestPath (iMapNode* from, iMapNode* goal) = 0;
  
  /**
   * Update the tiles of the hierarchical navigation structure that intersect with an axis aligned bounding box.
   * \param boundingBox Bounding box representing the area to be updated.
   * \param sector Only update tiles from this sector, if specified.
   * \return True in case everything went right and false otherwise.
   * \remarks A sector should be specified whenever possible.
   */
  virtual bool Update (const csBox3& boundingBox, iSector* sector = 0) = 0;

  /**
   * Update the tiles of the hierarchical navigation structure that intersect with an oriented bounding box.
   * \param boundingBox Bounding box representing the area to be updated.
   * \param sector Only update tiles from this sector, if specified.
   * \return True in case everything went right and false otherwise.
   * \remarks A sector should be specified whenever possible.
   */
  virtual bool Update (const csOBB& boundingBox, iSector* sector = 0) = 0;

  /**
   * Save this structure to a file.
   * \param vfs Pointer to the virtual file system. The file will be saved the file at the current directory 
   *            of this file system.
   * \param file File name.
   * \return True in case everything went right and false otherwise.
   */
  virtual bool SaveToFile (iVFS* vfs, const char* directory) = 0;

  /**
   * Get an object representation of the navigation mesh parameters.
   * \return Pointer to navigation mesh parameters object.
   */
  virtual const iCelNavMeshParams* GetNavMeshParams () const = 0;

  /**
   * Render navigation structure.
   */
  virtual csArray<csSimpleRenderMesh*>* GetDebugMeshes (iSector* sector = 0) = 0;

  /**
   * Render proxy agent of the specified color.
   * Adding to any previously got since construction or reset.
   */
  virtual csArray<csSimpleRenderMesh*>* GetAgentDebugMeshes (const csVector3& pos, int red, int green,
                                                           int blue, int alpha) = 0;

  /**
   * Clear all previous proxy agents.
   */
  virtual void ResetAgentDebugMeshes () = 0;
};



/**
 * Hierarchical navigation struct creator.
 */
struct iCelHNavStructBuilder : public virtual iBase
{
  SCF_INTERFACE (iCelHNavStructBuilder, 1, 0, 0);

  /**
   * Set the Sectors used to build the navigation structure.
   * \param sectorList List containing the sectors for which navmeshes will be built.
   * \return True in case everything went right and false otherwise.
   * \remarks You should call iCelHNavStructBuilder::SetNavMeshParams() before this method.
   */
  virtual bool SetSectors(csRefArray<iSector>* sectorList) = 0;

  /**
   * Build a hierarchical navigation structure using the current configurations.
   * \return Pointer to the navigation mesh, or 0 if something went wrong.
   */
  virtual iCelHNavStruct* BuildHNavStruct () = 0;

  /**
   * Load a hierarchical navigation structure from a file.
   * \param vfs Pointer to the virtual file system. The file will be loaded from the current directory 
   *            of this file system.
   * \param directory Directory name (vfs path).
   * \return Pointer to the navigation mesh, or 0 if something went wrong.
   */
  virtual iCelHNavStruct* LoadHNavStruct (iVFS* vfs, const char* directory) = 0;

  /**
   * Get an object representation of the navigation mesh parameters.
   * \return Pointer to navigation mesh parameters object.
   */
  virtual const iCelNavMeshParams* GetNavMeshParams () const = 0;

  /**
   * Set navigation mesh parameters.
   * \remarks Should be called before iCelHNavStructBuilder::SetSectors().
   */
  virtual void SetNavMeshParams (const iCelNavMeshParams* parameters) = 0;
};

#endif // __CEL_HPFAPI__
