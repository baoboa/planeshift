/*
* walkpoly.h
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

#ifndef ___WALKPOLY_HEADER___
#define ___WALKPOLY_HEADER___
#if USE_WALKPOLY
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/vector3.h>
#include <csgeom/box.h>
#include <iutil/objreg.h>
#include <csutil/array.h>
#include <csutil/csstring.h>
#include <csutil/list.h>

//=============================================================================
// Local Includes
//=============================================================================
#include "pathfind.h"

/**
 * \addtogroup npcclient
 * @{ */

class psNPCClient;

void Dump3(const char* name, const csVector3 &p);
void Dump(const char* name, const csBox3 &b);
void Dump(const char* name, const csVector3 &p);

float Calc2DDistance(const csVector3 &a, const csVector3 &b);
float AngleDiff(float a1, float a2);

/** Normal equation of a line in 2D space (the y coord is ignored) */
class ps2DLine
{
public:
    float a, b, c;

    void Dump();
    void Calc(const csVector3 &p1, const csVector3 &p2);
    void CalcPerp(const csVector3 &begin, const csVector3 &end);
    float CalcDist(const csVector3 &point);
    float CalcRel(const csVector3 &point);
    void Copy(ps2DLine &dest);
};

class psWalkPoly;
class psMapWalker;

struct iSector;
class CelBase;
struct iDocumentNode;
struct iVFS;

/**************************************************************************
*   psSpatialIndex is a tree-style data structure.
*   Its purpose is fast search of the psWalkPoly that is under your feet
*   or close to you.
*   It recursively splits the space into pairs of rectangular subspaces.
*
*   Each subspace corresponds to one node of the tree.
*   Each node keeps list of psWalkPolys whose psWalkPoly::box
*   collides with the subspace of the node.
***************************************************************************/

class psSpatialIndexNode
{
    friend class psSpatialIndex;
public:
    psSpatialIndexNode();

    /** Finds terminal node containing given point
        The point MUST be located in subtree of this node */
    psSpatialIndexNode* FindNodeOfPoint(const csVector3 &p);

    /** Adds polygon to the right terminal nodes from the subtree */
    void Add(psWalkPoly* poly);

    csBox3 &GetBBox()
    {
        return bbox;
    }

    csList<psWalkPoly*>* GetPolys()
    {
        return &polys;
    };

protected:

    /** Returns distance to root (root has 0) */
    int GetTreeLevel();

    /** Splits itself into two nodes */
    void Split();

    /** Chooses and sets appropriate splitAxis and splitValue */
    void ChooseSplit();

    psSpatialIndexNode* child1, * child2;
    psSpatialIndexNode* parent;

    csBox3 bbox;        /** subspace of this node */

    float splitValue;   /** The border value of x or y or z which separates the subspaces
                            of child1 and child2 ... used only if node has children */
    char splitAxis;     /** 0=x, 1=y, 2=z  ... used only if node has children */

    csList<psWalkPoly*> polys; /** Polygons whose "box" collides with this subspace */
    int numPolys;              /** Length of the list because csList lacks that */
};

class psSpatialIndex
{
public:

    psSpatialIndex();

    /** Adds without balancing */
    void Add(psWalkPoly* poly);

    /** Rebuilds the index from scratch so that it is more balanced */
    void Rebuild();

    /** Finds terminal node containing given point.
        'hint' is an optional index node that could be the right node */
    psSpatialIndexNode* FindNodeOfPoint(const csVector3 &p, psSpatialIndexNode* hint=NULL);

    //psSpatialIndexNode * FindNodeOfPoly(psWalkPoly * poly);

    /** Finds psWalkPoly whose "box" contains given point
        'hint' is an optional index node that could be the right node */
    psWalkPoly* FindPolyOfPoint(const csVector3 &p, psSpatialIndexNode* hint=NULL);

    psANode* FindNearbyANode(const csVector3 &pos);

protected:
    psSpatialIndexNode root;
};

class psSeed
{
public:
    psSeed();
    psSeed(csVector3 pos, iSector* sector, float quality);
    psSeed(csVector3 edgePos, csVector3 pos, iSector* sector, float quality);
    void SaveToString(csString &str);
    void LoadFromXML(iDocumentNode* node, iEngine* engine);
    bool CheckValidity(psMapWalker* walker);

    bool edgeSeed;
    float quality;
    csVector3 pos, edgePos;
    iSector* sector;
};

class psSeedSet
{
public:
    psSeedSet(iObjectRegistry* objReg);
    psSeed PickUpBestSeed();
    void AddSeed(const psSeed &seed);
    bool IsEmpty()
    {
        return seeds.IsEmpty();
    }

    bool LoadFromString(const csString &str);
    bool LoadFromFile(const csString &path);

    void SaveToString(csString &str);
    bool SaveToFile(const csString &path);

protected:
    iObjectRegistry* objReg;
    csList<psSeed> seeds;
};

/*****************************************************************
* The areas of map that are without major obstacles are denoted
* by set of polygons that cover these areas. This is used
* to precisely calculate very short paths. Other paths
* are calculated using A*, and later fine-tuned with this map.
******************************************************************/

/** This describes a contact of edge of one polygon with edge of another polygon */
class psWalkPolyCont
{
public:
    csVector3 begin, end;   ///< what part of the edge
    psWalkPoly* poly;       ///< the polygon that touches us
};

/** Vertex of psWalkPoly, also containing info about edge to the next vertex.
    Contains list of contacts with neighbour psWalkPolys */
class psWalkPolyVert
{
public:
    csVector3 pos;
    ps2DLine line;      ///< normal equation of edge from this vertex to the following one
    csArray<psWalkPolyCont> conts;
    csString info;      ///< debug only

    bool touching, stuck;
    bool followingEdge;

    float angle;

    psWalkPolyVert();
    psWalkPolyVert(const csVector3 &pos);

    /** Finds neighbour polygon that contacts with this edge at given point.
        Return NULL = there is no contact at this point */
    psWalkPoly* FindCont(const csVector3 &point);

    void CalcLineTo(const csVector3 &point);
};

/** This just keeps all polygons that we know and their index */
class psWalkPolyMap
{
    friend class psWalkPoly;
public:
    psWalkPolyMap(iObjectRegistry* objReg);
    psWalkPolyMap();

    void SetObjReg(iObjectRegistry* objReg)
    {
        this->objReg=objReg;
    }

    /** Adds new polygon to map */
    void AddPoly(psWalkPoly*);

    /** Checks if you can go directly between two points without problems */
    bool CheckDirectPath(csVector3 start, csVector3 goal);

    void Generate(psNPCClient* npcclient, psSeedSet &seeds, iSector* sector);

    void CalcConts();

    csString DumpPolys();

    void BuildBasicAMap(psAMap &AMap);

    void LoadFromXML(iDocumentNode* node);
    bool LoadFromString(const csString &str);
    bool LoadFromFile(const csString &path);

    void SaveToString(csString &str);
    bool SaveToFile(const csString &path);

    psANode* FindNearbyANode(const csVector3 &pos);

    psWalkPoly* FindPoly(int id);

    void PrunePolys();

    void DeleteContsWith(psWalkPoly* poly);

protected:

    csList<psWalkPoly*> polys;
    psSpatialIndex index;

    csHash<psWalkPoly*> polyByID;

    iObjectRegistry* objReg;
    csRef<iVFS> vfs;
};

/********************************************************************
 * A convex polygon that denotes area without major obstacles .
 * It is not necessarily a real polygon, because its vertices
 * do not have to be on the same 3D plane (just "roughly").
 * The walkable area within the polygon borders is rarely flat, too.
 ********************************************************************/

class psWalkPoly
{
    friend class psWalkPolyMap;
public:
    psWalkPoly();
    ~psWalkPoly();

    /** Adds a new vertex to polygon
    - vertices must be added CLOCKWISE ! */
    void AddVert(const csVector3 &pos, int beforeIndex=-1);
    size_t GetNumVerts()
    {
        return verts.GetSize();
    }

    void AddNode(psANode* node);
    void DeleteNode(psANode* node);

    void SetSector(iSector* sector);


    /** Moves 'point' (which must be within our polygon) towards 'goal' until it reaches
        another polygon or dead end.
        Returns true, if it reached another polygon, false on dead end */
    bool MoveToNextPoly(const csVector3 &goal, csVector3 &point, psWalkPoly* &nextPoly);

    void AddSeeds(psMapWalker* walker, psSeedSet &seeds);

    /** Checks if polygon is still convex after we moved vertex 'vertNum' */
    bool CheckConvexity(int vertNum);

    void Clear();

    //ignores y
    void Cut(const csVector3 &cut1, const csVector3 &cut2);
    //ignores y
    void Intersect(const psWalkPoly &other);

    float EstimateY(const csVector3 &point);

    void Dump(const char* name);
    void DumpJS(const char* name);
    void DumpPureJS();
    void DumpPolyJS(const char* name);

    int Stretch(psMapWalker &walker, psWalkPolyMap &map, float stepSize);
    void GlueToNeighbours(psMapWalker &walker, psWalkPolyMap &map);

    float GetBoxSize();
    float GetArea();
    float GetTriangleArea(int vertNum);
    float GetLengthOfEdges();
    void GetShape(float &min, float &max);
    float GetNarrowness();

    void PruneUnboundVerts(psMapWalker &walker);
    void PruneInLineVerts();

    bool IsNearWalls(psMapWalker &walker, int vertNum);

    /** returns true if it moved at least a bit */
    bool StretchVert(psMapWalker &walker, psWalkPolyMap &map, int vertNum, const csVector3 &newPos, float stepSize);

    void SetMini(psMapWalker &walker, const csVector3 &pos, int numVerts);

    /** Calculates box of polygon  */
    void CalcBox();
    csBox3 &GetBox()
    {
        return box;
    }

    bool IsInLine();

    void ClearInfo();

    bool CollidesWithPolys(psWalkPolyMap &map, csList<psWalkPoly*>* polys);

    void SaveToString(csString &str);
    void LoadFromXML(iDocumentNode* node, iEngine* engine);

    /** Populates the 'conts' array  */
    void CalcConts(psWalkPolyMap &map);

    void RecalcAllEdges();

    void ConnectNodeToPoly(psANode* node, bool bidirectional);

    bool TouchesPoly(psWalkPoly* poly);
    bool NeighboursTouchEachOther();

    psANode* GetFirstNode();

    int GetID()
    {
        return id;
    }
    iSector* GetSector()
    {
        return sector;
    }

    bool PointIsIn(const csVector3 &p, float maxRel=0);

    csVector3 GetClosestPoint(const csVector3 &point, float &angle);

    int GetNumConts();

    size_t GetNumNodes()
    {
        return nodes.GetSize();
    }

    bool ReachableFromNeighbours();

    psWalkPoly* GetNearNeighbour(const csVector3 &pos, float maxDist);

    void DeleteContsWith(psWalkPoly* poly);

    bool NeighbourSupsMe(psWalkPoly* neighbour);

    void MovePointInside(csVector3 &point, const csVector3 &goal);

protected:

    void RecalcEdges(int vertNum);

    int FindClosestEdge(const csVector3 pos);

    /** Finds contact of this polygon with another polygon that is
        at given point and edge */
    psWalkPoly* FindCont(const csVector3 &point, int edgeNum);

    /** Moves 'point' to the closest point in the direction of 'goal' that is within
        our polygon (some point on polygon border).
        'edgeNum' is number of the edge where the new point is */
    bool MoveToBorder(const csVector3 &goal, csVector3 &point, int &edgeNum);


    /** Populates the 'edges' array according to 'verts' */
    void CalcEdges();


    int GetNeighbourIndex(int vertNum, int offset) const;

    bool HandleProblems(psMapWalker &walker, psWalkPolyMap &map, int vertNum, const csVector3 &oldPos);
    csVector3 GetClosestCollisionPoint(csList<psWalkPoly*> &collisions, ps2DLine &line);
    bool HandlePolyCollisions(psWalkPolyMap &map, int vertNum, const csVector3 &oldPos);

    int id;
    static int nextID;
    bool supl;

    csArray<psWalkPolyVert> verts;       /** vertices of polygon */
    iSector* sector;
    csBox3 box;       /** box around polygon - taller than the real poly */

    csArray<psANode*> nodes;
};

/** @} */

#endif

#endif

