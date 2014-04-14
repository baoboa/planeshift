/*    Crystal Space Entity Layer
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

#ifndef __CEL_TOOLS_CELGRAPH__
#define __CEL_TOOLS_CELGRAPH__

#include "csutil/csobject.h"
#include "csutil/util.h"
#include "csutil/hash.h"
#include "csutil/redblacktree.h"
#include "csutil/weakrefarr.h"
#include "csutil/stringarray.h"
#include "csutil/eventhandlers.h"
#include "iutil/comp.h"
#include "iutil/eventh.h"
#include "iutil/eventq.h"
#include "ivaria/conin.h"
#include "ivaria/conout.h"

#include "tools/celgraph.h"

#include <stdio.h>
#include <string.h>

/**
 * This is a mapnode Graph for CEL.
 */
class celEdge : public scfImplementation1<
		   celEdge, iCelEdge>
{
 private:
  csRef<iCelNode> successor;
  bool state;
  float weight;

 public:
  celEdge ();
  virtual ~celEdge ();
  virtual void SetState (bool open);
  virtual void SetSuccessor (iCelNode* node);
  virtual bool GetState ();
  virtual iCelNode* GetSuccessor ();
  virtual float GetWeight () const;
  virtual void SetWeight (float weight);
};

class celNode : public scfImplementation1<
		   celNode, iCelNode>
{
 private:
  csRefArray<iCelEdge> edges;
  csRef<iMapNode> map_node;
  csRef<iCelNode> parent;
  float heuristic;
  float cost;
  csString name;

 public:
  celNode ();
  virtual ~celNode ();
  virtual size_t AddSuccessor (iCelNode* node, bool state);
  virtual void SetMapNode (iMapNode* node);
  virtual void SetParent (iCelNode* par);
  virtual void SetName (const char* n);
  virtual void Heuristic (float cost, iCelNode* goal);
  virtual iMapNode* GetMapNode ();
  virtual csVector3 GetPosition ();
  virtual const char* GetName ();
  virtual iCelNode* GetParent () {return parent;}
  virtual csArray<iCelNode*> GetSuccessors ();
  virtual csArray<iCelNode*> GetAllSuccessors ();
  virtual float GetHeuristic () {return heuristic;};
  virtual float GetCost () {return cost;};
  virtual size_t GetEdgeCount ()
  { return edges.GetSize(); }
  virtual iCelEdge* GetEdge (size_t idx)
  { return edges.Get(idx); }
  virtual void RemoveEdge (size_t idx);
  virtual size_t AddSuccessor (iCelNode* node, bool state, float weight);
  virtual csRefArray<iCelEdge>* GetEdges ();
};

/**
 * This is a mapnode Path for CEL.
 */ 

class celPath : public scfImplementationExt2<
celPath, csObject, iCelPath, iComponent>
{
 private:
  iObjectRegistry* object_reg;
  size_t cur_node;
  csRefArray<iMapNode> nodes;
  
 public:
  celPath (iBase* parent);
  virtual ~celPath ();
  virtual iObject* QueryObject () { return this; }
  virtual void AddNode (iMapNode* node);
  virtual void InsertNode (size_t pos, iMapNode* node);
  virtual iMapNode* Next ();
  virtual iMapNode* Previous ();
  virtual iMapNode* Current ();
  virtual csVector3 CurrentPosition ();
  virtual iSector* CurrentSector ();
  virtual bool HasNext ();
  virtual bool HasPrevious ();
  virtual void Restart ();
  virtual void Clear ();
  virtual bool Initialize (iObjectRegistry* object_reg);
  virtual iMapNode* GetFirst ();
  virtual iMapNode* GetLast ();
  virtual void Invert ();
  virtual size_t GetNodeCount ()
  { return nodes.GetSize(); }
};

/**
 * This is a mapnode Graph for CEL.
 */
class celGraph : public scfImplementationExt2<
  celGraph, csObject, iCelGraph, iComponent>
{
private:
  iObjectRegistry* object_reg;
   csRefArray <iCelNode> nodes;

public: 
  celGraph (iBase* parent);
  virtual ~celGraph ();
  virtual iObject* QueryObject () { return this; }
  virtual bool Initialize (iObjectRegistry* object_reg);
  virtual iCelNode* CreateNode (const char *name, csVector3 &pos);
  virtual size_t AddNode (iCelNode* node);
  virtual void AddEdge (iCelNode* from, iCelNode* to, bool state);
  virtual bool AddEdgeByNames (const char* from, const char* to, bool state);
  virtual iCelNode* GetClosest (csVector3 position);
  virtual bool ShortestPath (iCelNode* from, iCelNode* goal, iCelPath* path);
  virtual iCelNode* RandomPath (iCelNode* from, int distance, iCelPath* path);
  virtual size_t GetNodeCount ()
  { return nodes.GetSize(); }
  virtual iCelNode* GetNode (size_t idx)
  { return nodes.Get(idx); }
  virtual void RemoveNode (size_t idx);
  virtual void RemoveEdge (iCelNode* from, size_t idx);
  virtual size_t AddEdge (iCelNode* from, iCelNode* to, bool state, float weight);
  virtual iCelNode* CreateEmptyNode (size_t& index);
  virtual bool ShortestPath2 (iCelNode* from, iCelNode* goal, iCelPath* path);
};

#endif //__CEL_TOOLS_CELGRAPH__ 

