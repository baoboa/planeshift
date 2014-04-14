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

#include "cssysdef.h"
#include "cstool/initapp.h"
#include "csutil/objreg.h"
#include "csutil/event.h"
#include "csutil/cfgacc.h"
#include "csutil/inputdef.h"
#include "csutil/array.h"
#include "csutil/priorityqueue.h"
#include "cstool/mapnode.h"
#include "ivaria/mapnode.h"
#include "csgeom/math3d.h"
#include "iutil/evdefs.h"
#include "iutil/event.h"
#include "iutil/plugin.h"
#include "iutil/objreg.h"
#include "ivideo/graph3d.h"
#include "ivaria/reporter.h"
#include "ivaria/conin.h"
#include "ivaria/script.h"
#include "csutil/randomgen.h"
#include "csutil/hash.h"

#include "celgraph.h"

#include <iengine/sector.h>

//--------------------------------------------------------------------------

//SCF_IMPLEMENT_FACTORY (celEdge)

  celEdge::celEdge ()
  : scfImplementationType (this)
{
  state = true;
  weight = 0;
}

celEdge::~celEdge ()
{
}

void celEdge::SetState (bool open)
{
  state = open;
}

void celEdge::SetSuccessor (iCelNode* node)
{
  successor = node;
}

bool celEdge:: GetState ()
{
  return state;
}

iCelNode* celEdge::GetSuccessor ()
{
  return successor;
}

float celEdge::GetWeight () const
{
  return weight;
}

void celEdge::SetWeight (float weight)
{
  this->weight = weight;
}

//---------------------------------------------------------------------------

//SCF_IMPLEMENT_FACTORY (celNode)

celNode::celNode ()
  : scfImplementationType (this)
{
}

celNode::~celNode ()
{
}

size_t celNode:: AddSuccessor (iCelNode* node, bool state)
{
  csRef<iCelEdge> edge;
  edge.AttachNew(new celEdge());
  edge->SetState(state);
  edge->SetSuccessor(node);
  edge->SetWeight(csSquaredDist::PointPoint(GetPosition(), node->GetPosition()));
  return edges.Push(edge);
}

void celNode:: SetMapNode (iMapNode* node)
{
  map_node = node;
}

void celNode:: SetParent (iCelNode* par)
{
  parent = par;
}

void celNode:: SetName (const char* n)
{
  name = n;
}

void celNode:: Heuristic (float cost, iCelNode* goal)
{
  heuristic = csSquaredDist::PointPoint(GetPosition(), goal->GetPosition());
  this->cost = cost;
}

iMapNode* celNode:: GetMapNode ()
{
  return map_node;
}

csVector3 celNode:: GetPosition ()
{
  return map_node->GetPosition();
}

const char* celNode:: GetName ()
{
  return name;
}

csArray<iCelNode*> celNode:: GetSuccessors ()
{
  csArray<iCelNode*> nodes;
  size_t n = edges.GetSize();

  
  for(size_t i=0; i<n;i++){
    if(edges[i]->GetState())
      nodes.Push(edges[i]->GetSuccessor());
  }

  return nodes;
}

csArray<iCelNode*> celNode:: GetAllSuccessors ()
{
  csArray<iCelNode*> nodes;
  size_t n = edges.GetSize();

  for(size_t i=0; i<n;i++)
    nodes.Push(edges[i]->GetSuccessor());
  
  return nodes;
}

void celNode::RemoveEdge(size_t idx)
{
  edges.DeleteIndex(idx);
}

size_t celNode::AddSuccessor (iCelNode* node, bool state, float weight)
{
  csRef<iCelEdge> edge;
  edge.AttachNew(new celEdge());
  edge->SetState(state);
  edge->SetSuccessor(node);
  edge->SetWeight(weight);
  return edges.Push(edge);
}

csRefArray<iCelEdge>* celNode::GetEdges () 
{
  //csRefArray<iCelEdge> edges(this->edges);
  return &edges;
}

//---------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (celPath)

celPath::celPath (iBase* parent)
  : scfImplementationType (this, parent)
{
  cur_node = 0;
}

celPath::~celPath ()
{
}

void celPath::AddNode (iMapNode* node)
{
  nodes.Push(node);
}

void celPath::InsertNode (size_t pos, iMapNode* node)
{
  nodes.Insert(pos, node);
}

iMapNode* celPath::Next ()
{
  cur_node++;
  return nodes[cur_node];
}

iMapNode* celPath::Previous ()
{
  return nodes[cur_node--];
}

iMapNode* celPath:: Current ()
{
  return nodes[cur_node];
}

csVector3 celPath:: CurrentPosition ()
{
  return nodes[cur_node]->GetPosition();
}

iSector* celPath:: CurrentSector ()
{
  return nodes[cur_node]->GetSector();
}

bool celPath::HasNext ()
{
  return cur_node < nodes.GetSize()-1;
}

bool celPath::HasPrevious ()
{
  return cur_node >= 0;
}

void celPath::Restart ()
{
  cur_node = 0;
}

void celPath::Clear ()
{
  nodes.DeleteAll();
  Restart();
}

void celPath::Invert ()
{
  csRef<iMapNode> temp;

  size_t size = nodes.GetSize() / 2;
  size_t j = nodes.GetSize() - 1;
  for (size_t i = 0; i < size; i++)
  {
    temp = nodes[j];
    nodes[j] = nodes[i];
    nodes[i] = temp;
    j--;
  }
}

iMapNode* celPath::GetFirst ()
{
  if(nodes.GetSize()>0)
    return nodes[0];
  return NULL;
}


iMapNode* celPath::GetLast ()
{
  if(nodes.GetSize()>0)
    return nodes[nodes.GetSize()-1];
  return NULL;
}

bool celPath::Initialize (iObjectRegistry* object_reg)
{
  celPath::object_reg = object_reg;

  return true;
}


//---------------------------------------------------------------------------

template<typename T1, typename T2>
class Comparator
{
public:
  static int Compare (T1 const &n1, T2 const &n2)
  {
    return (int)(n2->GetHeuristic()+n2->GetCost() - n1->GetHeuristic()- n1->GetCost());
  }
};

SCF_IMPLEMENT_FACTORY (celGraph)

celGraph::celGraph (iBase* parent)
  : scfImplementationType (this, parent)
{
}

celGraph::~celGraph ()
{
}

bool celGraph::Initialize (iObjectRegistry* object_reg)
{
  celGraph::object_reg = object_reg;
  
  //nodes = csRefArray<iCelNode>::csRefArray(100, 100);
  
  return true;
}

size_t celGraph:: AddNode (iCelNode* node)
{
  return nodes.Push(node);
}

void celGraph:: AddEdge (iCelNode* from, iCelNode* to, bool state)
{
  from->AddSuccessor(to, state);
}

bool celGraph:: AddEdgeByNames (const char* from, const char* to, bool state)
{
  iCelNode* f = 0;
  iCelNode* t = 0;
  bool fd = false, td = false;

  for(unsigned int i=0; i<nodes.GetSize();i++)
    {
      if(!strcmp(nodes[i]->GetName(),from))
	{
	  fd = true;
	  f = nodes[i];
	}
      if(!strcmp(nodes[i]->GetName(),to))
	{	
	  td = true;
	  t = nodes[i];
	}

      if(fd && td)
	break;
    }

  if(!f || !t)
    return false;
    

  f->AddSuccessor(t, state);
  return true;
}

iCelNode* celGraph:: GetClosest (csVector3 position)
{
  size_t n = nodes.GetSize();
  if(n == 0)
    return NULL;

  iCelNode* closest= nodes[0];
  
  float shortest = csSquaredDist::PointPoint(position, closest->GetPosition());
  float distance;

  for(size_t i=1; i<n; i++){
    distance =  csSquaredDist::PointPoint(position, nodes[i]->GetPosition());
    if(distance < shortest){
      shortest = distance;
      closest = nodes[i];
    }
  }

  return closest;
}

bool celGraph::ShortestPath (iCelNode* from, iCelNode* goal, iCelPath* path)
{
  path->Clear();

  CS::Utility::PriorityQueue<iCelNode*, csArray<iCelNode*>, Comparator<iCelNode*, iCelNode*> > queue;
  csHash<iCelNode*, uint> hash;
  csHashComputer<float> computer;
  csArray<iCelNode*> array;

  from->Heuristic(0, goal);
  queue.Insert(from);

  //Check if the list is empty

  while(!queue.IsEmpty())
  {
    //Choose the node with less cost+h
    iCelNode* current = queue.Pop();

    //Check if we have arrived to our goal
    if(current == goal)
    {
      if(current == from)
      {
        path->InsertNode(0, current->GetMapNode());
        return true;
      }
      while(true)
      {
        if(current == from)
        {
          return true;
        }
        path->InsertNode(0, current->GetMapNode());
        current = current->GetParent();
      }
    }

    // Get edges
    csRefArray<iCelEdge>* edges = current->GetEdges();
    size_t size = edges->GetSize();
    for (size_t i = 0; i < size; i++)
    {
      iCelNode* suc = edges->Get(i)->GetSuccessor();
      // Check if this Node is already in the queue
      array = hash.GetAll(computer.ComputeHash(suc->GetPosition().x + suc->GetPosition().y));
      csArray<iCelNode*> :: Iterator it = array.GetIterator();
      bool in = false;

      while(it.HasNext())
      {
        iCelNode* cur = it.Next();
        if(cur == suc)
        {
          in = true;
          break;
        }
      }

      if(in)
      {
        continue;
      }

      suc->SetParent(current);
      suc->Heuristic(current->GetCost() + edges->Get(i)->GetWeight(), goal);
      queue.Insert(suc);
      hash.Put(computer.ComputeHash(suc->GetPosition().x + suc->GetPosition().y), suc);
    }
  }
  //goal is unreachable from here
  return false;
}

bool celGraph::ShortestPath2 (iCelNode* from, iCelNode* goal, iCelPath* path)
{
  CS::Utility::PriorityQueue<iCelNode*, csArray<iCelNode*>, Comparator<iCelNode*, iCelNode*> > queue;
  csHash<iCelNode*, csPtrKey<iCelNode> > openSet;
  csHash<iCelNode*, csPtrKey<iCelNode> > closedSet;

  path->Clear();
  from->Heuristic(0, goal);
  queue.Insert(from);

  while (!queue.IsEmpty())
  {
    // Get the node with the least estimated cost
    iCelNode* current = queue.Pop();

    // Found path
    if (current == goal)
    {
      while (current != from)
      {
        path->AddNode(current->GetMapNode());
        current = current->GetParent();
      }
      path->AddNode(from->GetMapNode());
      path->Invert();
      return true;
    }

    closedSet.Put(current, current);

    // Get successors
    csRefArray<iCelEdge>* edges = current->GetEdges();
    size_t size = edges->GetSize();
    for (size_t i = 0; i < size; i++)
    {
      iCelNode* successor = edges->Get(i)->GetSuccessor();

      if (closedSet.Contains(successor))
      {
        continue;
      }

      float cost = current->GetCost() + edges->Get(i)->GetWeight();
      if (!openSet.Contains(successor))
      {
        successor->SetParent(current);
        successor->Heuristic(cost, goal);
        queue.Insert(successor);
        openSet.Put(successor, successor);
      }
      else
      {
        if (cost < successor->GetCost())
        {
          successor->SetParent(current);
          successor->Heuristic(cost, goal);
        }
      }
    }
  }
  // Goal is unreachable from here
  return false;
}

iCelNode* celGraph::RandomPath (iCelNode* from, int distance, iCelPath* path)
{
  csRandomGen random;
  random.Initialize();
  int i=0;

  iCelNode* current = from;
  while(true){
    path->AddNode(current->GetMapNode());
    csArray<iCelNode*> succ = current->GetSuccessors();
    if(i==distance)
      return current;
    
    if(succ.GetSize() == 0)
      return current;
    
    current = succ[random.Get((uint32)succ.GetSize())];
    i++;
  }
}

iCelNode* celGraph::CreateNode (const char *name, csVector3 &pos)
{
  csRef<iMapNode> n;
  n.AttachNew(new csMapNode("n0"));
  n->SetPosition(pos);

  csRef<iCelNode> newnode;
  newnode.AttachNew(new celNode());
  newnode->SetName(name);
  newnode->SetMapNode(n);
  AddNode(newnode);
  return newnode;
}

void celGraph::RemoveNode (size_t idx)
{
  nodes.DeleteIndex(idx);
}

void celGraph::RemoveEdge (iCelNode* from, size_t idx)
{
  from->RemoveEdge(idx);
}

size_t celGraph::AddEdge (iCelNode* from, iCelNode* to, bool state, float weight)
{
  return from->AddSuccessor(to, state, weight);
}

iCelNode* celGraph::CreateEmptyNode (size_t& index)
{
  csRef<iCelNode> newnode;
  newnode.AttachNew(new celNode());
  index = AddNode(newnode);
  return newnode;
}
