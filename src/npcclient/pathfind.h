/*
* pathfind.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef ___PATHFIND_HEADER___
#define ___PATHFIND_HEADER___

#ifdef USE_WALKPOLY

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/vector3.h>
#include <csgeom/box.h>
#include <csutil/hash.h>
#include <iutil/objreg.h>
#include <csutil/array.h>
#include <csutil/csstring.h>
#include <csutil/list.h>

/**
 * \addtogroup npcclient
 * @{ */

class psANode;
class psWalkPoly;
class psWalkPolyMap;
class psAMap;
struct iVFS;
struct iSector;
struct iCollection;
struct iEngine;
struct iDocumentNode;

/***************************************************************************//**
* Keeps pathfinding maps for one region.
*******************************************************************************/
class psPFMap
{
public:
    csString name;
    iCollection* region;
    psWalkPolyMap* wpMap;
    psAMap* aMap;
};

/***************************************************************************//**
* Keeps pathfinding maps for all regions.
*******************************************************************************/
class psPFMaps
{
public:
    psPFMaps(iObjectRegistry* objReg);
    psPFMap* GetRegionBySector(const csString &sectorName);
    psPFMap* GetRegionBySector(iSector* sector);
    psPFMap* FindRegionByName(const csString &regionName);

protected:
    iCollection* GetCSRegionOfSector(const csString &sectorName);

    csList<psPFMap*> regions;

    /// sector name ---> psPFMap
    csHash<psPFMap*, csString> regionMap;

    iObjectRegistry* objReg;
    csRef<iEngine> engine;
};

/***************************************************************************//**
*  Edge from one A* node to another A* node.
*  Each connection in A* graph is bidirectional, so each edge
*  in this graph is kept as two psAEdges (from two different directions)
*******************************************************************************/
class psAEdge
{
public:
    psAEdge(psANode* neighbour, float cost);
    psANode* neighbour;
    float cost;    /**< How hard is it for a NPC to walk along this edge - influenced
                       by length and terrain properties */
};

/* Empty subclass that unites psACluster and psAHierarchyNode
class psAHierarchyNode
{
};*/

/*************************************************************//**
* Cluster is group of A* nodes or group of smaller clusters.
* Keeps list of all exits from cluster.
*****************************************************************/
class psACluster
{
public:
    bool Contains(psANode* node);
    bool AddNode(psANode* node);
    void DeleteNode(psANode* node);

    void GrowClusterFrom(psANode* start, int level, csList <psANode*> &unassigned);

protected:

    int CalcFreeBorderSize(int level);

    csList <psANode*> parts;
    csList <psANode*> exits;
};

/*******************************************************************************//**
* All A* nodes are split into three groups during run of the A* algorithm:
*    unknown - we haven't discovered these nodes yet in this search
*    open    - we discovered these nodes already, but maybe not all their neighbours
*    closed  - we discovered these nodes and all their neighbours already
***********************************************************************************/
enum psAState {AState_open, AState_closed, AState_unknown};


/************************************************************************//**
* Describes one A* node and keeps temporary state for the current run
* of A* algorithm. This temporary state is valid for one run of A* only.
* We don't clean it before each A* run, rather we keep number of the A*
* run where it was used. Before we process this node in the current A*, we check
* if its state is valid for the current A* run - if not, THEN we clear it.
****************************************************************************/

class psANode
{
public:
    psANode();
    ~psANode();
    psANode(const csVector3 &point);
    psANode(const csVector3 &point, iSector* sector);

    /** This must be called before we work with this node - it checks
        if our node needs to be inited and does the initialization if needed */
    void EnsureNodeIsInited(int currARunNum);

    void SetBestPrev(psANode* bestPrev, float cost);
    void CalcHeur(psANode* goal);
    void AddEdge(psANode* neighbour, float cost, int level, bool bidirectional=true);
    void DeleteEdges(psANode* neighbour);

    void ConnectToPoly(psWalkPoly* poly, bool bidirectional);

    bool ShouldBePruned(psAMap &map);

    void SaveToString(csString &str);

    //wpMap can be NULL
    void LoadBasicsFromXML(iDocumentNode* node, psWalkPolyMap* wpMap, iEngine* engine);
    void LoadEdgesFromXML(iDocumentNode* node, psAMap &map);

    void DumpJS();

    psWalkPoly* GetSomePoly();

    //works on 0th level only !
    void DeleteConnectionsFromNeighbours();
    void RestoreConnectionsFromNeighbours();

    //on 0th level
    float GetEdgeCost(psANode* neighbour);

    csVector3 point;
    iSector* sector;
    csArray<csArray<psAEdge> > edges;
    psACluster* cluster;

    int id;
    psWalkPoly* poly1, * poly2;
    static int nextID;

    /* The following variables are temporary A* state valid for one A* run only */
    int ARunNum;          /**< number of the A* run */
    psAState state;
    psANode* prevInOpen, * nextInOpen;   /**  */
    psANode* bestPrev;
    float bestCost;
    float heur;
    float total;
};

class psAPath
{
public:
    psAPath();
    psAPath(psPFMaps* regions);

    void SetMaps(psPFMaps* maps);

    void AddNodeToFront(psANode* node);

    // resets
    void SetDest(const csVector3 &dest);
    csVector3 GetDest()
    {
        return dest;
    }

    void CalcLocalDest(const csVector3 &currPoint, iSector* currSector, csVector3 &localDest);

    void Dump();
    void ResetNodes();

    ///calculates cost from first to last node, ignores 'dest'
    float CalcCost();

protected:

    csList<psANode*>::Iterator FindDirectNode(const csVector3 &currPoint);
    void InsertSubpath(psAPath &subpath);
    csString GetRegionNameOfSector(const csString &sectorName);
    psANode* GetNthNode(int n);
    void RandomizeFirst(const csVector3 &currPoint);

    bool CalcLocalDestFromLocalPath(const csVector3 &currPoint, csVector3 &localDest);

    psANode* FindNearbyANode(const csVector3 &pos, iSector* sector);

    csVector3 dest;
    csList<psANode*> nodes;
    psPFMaps* maps;
};

/** Keeper of all our A* info */
class psAMap
{
public:
    psAMap(iObjectRegistry* objReg);
    psAMap();

    void SetObjReg(iObjectRegistry* objReg)
    {
        this->objReg=objReg;
    }
    void AddNode(psANode* node);

    void PruneSuperfluous();

    /** Builds hierachy of A* nodes - it clusters the nodes,
        then clusters their clusters... etc. */
    void BuildHierarchy();

    /** Returns path between two nodes on given level of hierarchy.
        All nodes in the returned path will be on this level of hierarchy.
        Returns true when path was found */
    bool RunA(psANode* start, psANode* goal, int level, int maxExpands, psAPath &path);

    void DumpJS();

    bool LoadFromString(const csString &str, psWalkPolyMap* wpMap);
    bool LoadFromFile(const csString &path, psWalkPolyMap* wpMap);
    void SaveToString(csString &str);
    bool SaveToFile(const csString &path);

    psANode* FindNode(int id);

protected:
    csList <psANode*> nodes;
    csList <psACluster*> clusters;
    iObjectRegistry* objReg;
    csRef<iVFS> vfs;
};

/** @} */

#endif

#endif
