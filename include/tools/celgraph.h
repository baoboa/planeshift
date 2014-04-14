/*
    Crystal Space Entity Layer
    Copyright (C) 2006 by Jorrit Tyberghein
  
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

#ifndef __CEL_CELGRAPH__
#define __CEL_CELGRAPH__

#include "cstypes.h"
#include "csutil/scf.h"
#include "csutil/flags.h"
#include "csutil/stringarray.h"
#include <ivaria/mapnode.h>

struct iCelNode;

/**
 * Interface for CEL Edge.
 */
struct iCelEdge : public virtual iBase
{
  SCF_INTERFACE (iCelEdge, 1, 0, 1);

  /**
   * Sets if the edge is open or closed.
   */  
  virtual void SetState (bool open) = 0;

  /**
   * Sets successor node.
   */  
  virtual void SetSuccessor (iCelNode* node) = 0;
  
  /**
   * Get state.
   */  
  virtual bool GetState () = 0;

  /**
   * Get end-side Node.
   */
  virtual iCelNode* GetSuccessor () = 0;

  /// Get weight
  virtual float GetWeight () const = 0;

  /// Set weight
  virtual void SetWeight (float weight) = 0;
};


/**
 * Interface for CEL Node.
 */
struct iCelNode : public virtual iBase
{
  SCF_INTERFACE (iCelNode, 1, 0, 3);

  /**
   * Adds a successor to this node (This will create a new edge).
   */
  virtual size_t AddSuccessor (iCelNode* node, bool state) = 0;
  
  /**
   * Sets mapnode of this node.
   */
  virtual void SetMapNode (iMapNode* node) = 0;

  /**
   * Sets parent to this node.
   */
  virtual void SetParent (iCelNode* par)= 0;
  
  /**
   * Calculates euclidean distance between this node and goal.
   */
  virtual void SetName (const char* par)= 0;
  
  /**
   * Calculates euclidean distance between this node and goal.
   */
  virtual void Heuristic (float cost, iCelNode* goal)= 0;

  /**
   * Get MapNode of this node.
   */
  virtual iMapNode* GetMapNode () = 0;

  /**
   * Get position of this node.
   */
  virtual csVector3 GetPosition () = 0;

  /**
   * Get name of this node.
   */
  virtual const char* GetName () = 0;
  
  /**
   * Get Parent to this node.
   */
  virtual iCelNode* GetParent () = 0;

  /**
   * Get reachable successors to this node.
   */
  virtual csArray<iCelNode*> GetSuccessors () = 0;

  /**
   * Get all successors to this node.
   */
  virtual csArray<iCelNode*> GetAllSuccessors () = 0;

  /**
   * Get stored heuristic.
   */
  virtual float  GetHeuristic () = 0;

  /**
   * Get stored cost.
   */
  virtual float  GetCost () = 0;

  /**
   * Get Number of Edges.
   */
   virtual size_t GetEdgeCount () = 0;

  /**
   * Get a node by index.
   */
  virtual iCelEdge* GetEdge (size_t idx) = 0;

  /**
   * Remove an edge.
   */
  virtual void RemoveEdge (size_t idx) = 0;

  /**
   * Adds a successor to this node, using a weight different then the euclidean
   * distance between the two nodes (This will create a new edge).
   */
  virtual size_t AddSuccessor (iCelNode* node, bool state, float weight) = 0;

  /**
   * Get edges.
   */
  virtual csRefArray<iCelEdge>* GetEdges () = 0;
};


/**
 * Interface for CEL Path.
 */
struct iCelPath : public virtual iBase
{
  SCF_INTERFACE (iCelPath, 2, 0, 1);

  /**
   * Query the underlying iObject
   */
  virtual iObject* QueryObject () = 0;

  /**
   * Adds a new node at the end of the path.
   */
  virtual void AddNode (iMapNode* node) = 0;
  
  /**
   * Adds a new node in position pos.
   */
  virtual void InsertNode (size_t pos, iMapNode* node) = 0;
  
  /**
   * Get next node in path.
   */
  virtual iMapNode* Next () = 0;
  
  /**
   * Get previous node in path.
   */
  virtual iMapNode* Previous () = 0;
  
  /**
   * Get current node in path.
   */
  virtual iMapNode* Current () = 0;
  
  /**
   * Get currents node position.
   */
  virtual csVector3 CurrentPosition () = 0;

  /**
   * Get currents node sector.
   */
  virtual iSector* CurrentSector () = 0;
  
  /**
   * Checks if there are more nodes ahead in the path.
   */
  virtual bool HasNext () = 0;

  /**
   * Checks if there are more nodes back in the path.
   */
  virtual bool HasPrevious () = 0;

  /**
   * Restarts path
   */
  virtual void Restart () = 0;

  /**
   * Clears path
   */
  virtual void Clear () = 0;

  /**
   * Get First node in path.
   */
  virtual iMapNode* GetFirst () = 0;
  
  /**
   * Get last node in path.
   */
  virtual iMapNode* GetLast () = 0;
  
  /**
   * Invert nodes in the path.
   */
  virtual void Invert () = 0;
  
  /**
   * Get number of nodes in the path.
   */
  virtual size_t GetNodeCount () = 0;
};


/**
 * Interface for the CEL Graph.
 */
struct iCelGraph : public virtual iBase
{
  SCF_INTERFACE (iCelGraph, 1, 0, 4);

  /**
   * Query the underlying iObject
   */
  virtual iObject* QueryObject () = 0;

  /**
   * Create a node for this graph. The node will be added to the graph.
   */
  virtual iCelNode* CreateNode (const char* name, csVector3 &pos) = 0;

  /**
   * Adds a node to the graph.
   */
  virtual size_t AddNode (iCelNode* node) = 0;

  /**
   * Adds an edge to the graph.
   */
  virtual void AddEdge (iCelNode* from, iCelNode* to, bool state) = 0;

  /**
   * Adds an edge to the graph.
   */
  virtual bool AddEdgeByNames (const char* from, const char* to, bool state) = 0;

  /**
   * Gets the closest node to position.
   */
  virtual iCelNode* GetClosest (csVector3 position) = 0; 

  /**
   * Gets the shortest path from node from to node to.
   */
  virtual bool ShortestPath (iCelNode* from, iCelNode* goal, iCelPath* path) = 0;

  /**
   * Gets the shortest path from node from to node to.
   */
  virtual iCelNode* RandomPath (iCelNode* from, int distance, iCelPath* path) = 0;

  /**
   * Get Number of Nodes
   */
   virtual size_t GetNodeCount () = 0;

  /**
   * Get a node by index
   */
  virtual iCelNode* GetNode (size_t idx) = 0;

  /**
   * Removes a node from the graph.
   */
  virtual void RemoveNode (size_t idx) = 0;

  /**
   * Removes an edge from the graph.
   */
  virtual void RemoveEdge (iCelNode* from, size_t idx) = 0;

  /**
   * Adds an edge to the graph, using a weight different then the euclidean
   * distance between the two nodes.
   */
  virtual size_t AddEdge (iCelNode* from, iCelNode* to, bool state, float weight) = 0;

  /**
   * Creates an empty node and adds it to the graph.
   */
  virtual iCelNode* CreateEmptyNode (size_t& index) = 0;

  /**
   * Gets the shortest path from node from to node to.
   */
  virtual bool ShortestPath2 (iCelNode* from, iCelNode* goal, iCelPath* path) = 0;
  
};

#endif //__CEL_CELGRAPH__

