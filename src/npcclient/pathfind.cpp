/*
* pathfind.cpp
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
#ifdef USE_WALKPOLY

#include <psconfig.h>
#include <math.h>
#include <csutil/databuf.h>
#include <iutil/object.h>
#include <iengine/engine.h>
#include <iengine/sector.h>

#include "pathfind.h"
#include "walkpoly.h"
#include "npcclient.h"
#include "util/log.h"
#include "util/psxmlparser.h"
#include "util/consoleout.h"


/**********************************************
* Notes
***********************************************
* Hierarchy of maps
* -----------------
*
* 0 = walkable poly map
* 1 = basic A* graph
* 2 - clusters of level 1 nodes
* 3 - clusters of level 2 nodes
* n - etc.
*
* allows us to choose different trade-off between precision of a map and complexity of a map
* in different situations
*
* to keep size of search space and thus searching time down for longer paths, we choose
* higher level map that will give us much shorter searching times at the cost
* of lower path quality
*
* one path can be constructed with several serches of different distances and thus different levels
* of precision
*
*
* A* OPEN list
* ------------
* Some suggest to use priority queues instead of simple linked lists because
* they have O(log(n)) retrieval of the most promising node.
* The cons is that they have O(log(n)) for all operations, and linked list
* has small O(1) times on insertion and deletion. Overall priority queues
* are O(log(n)) and lists O(n) so queues scale better with number of A* nodes,
* but linked lists are more efficient up to some unknown value of n.
* Our scalability is solved by hierarchical A* so A* should be running on fairly
* small graphs, so we don't need queues, I guess. I thought I learned about this idea
* in a gamasutra article but I can't find it now.
* The linked list could be optimized by splitting it to several lists
* grouping nodes by quality, we would search in the best list only.
*
* Fibonacci heaps have O(1) insert and delete and O(log(n)) search,
* but with big constants.
*
* Nonadmissible heuristic
* -----------------------
* In our case the relative importance of speed of calculation and quality
* of result is more on the speed side, than e.g. in some real-time strategy game
* where the units must not look stupid. If some beasts won't find the best
* route in dungeons, nobody will notice or care. Because of this, we could use some
* heuristic that is not "admissible" i.e. it won't produce optimal results,
* and it will lead to examination of less A* nodes that when using euclidean distance
* as heuristic. It might be simply euclidean distance multiplied by some coefficient
* (higher than 1) or something smarter.
*
* This coefficient can be determined for each map by simulating searches
* with different coefficient values and comparing the quality of their results.
* Then we choose the coefficient that gives us good enough quality
* (e.g. lengths of its paths are 130% of lengths of optimal paths, on average)
* Or we could adjust the coefficient dynamically according to CPU load.




we can't mark some verts as stuck because they can become free thanks to movement of neighbours
*/


#define MAX_EXPANDS 9999    // find better value in future

int psANode::nextID = 0;

float CalcDistance(const csVector3 &a, const csVector3 &b);
float rndf(float min, float max);


void Dump(const char* name, const csBox3 &b)
{
    CPrintf(CON_DEBUG, "bbox '%s':\n", name);
    CShift();
    Dump3("b1", b.Min());
    Dump3("b2", b.Max());
    CUnshift();
}




float CalcDistance(psANode* node1, psANode* node2)
{
    return CalcDistance(node1->point, node2->point);
}


/***********************************************************************************
*
*                            basic A* graph construction
*
************************************************************************************/

void pathFindTest_AMapConstruction(psNPCClient* npcclient)
{
    psAMap Amap(npcclient->GetObjectReg());
    psWalkPolyMap map(npcclient->GetObjectReg());
    map.LoadFromFile("/this/pfgen.xml");
    //map.CalcConts();
    map.BuildBasicAMap(Amap);
    Amap.SaveToFile("/this/pfagen.xml");

    Amap.PruneSuperfluous();
    Amap.SaveToFile("/this/pfagen.xml");
}

psAMap::psAMap()
{
    objReg = NULL;
    vfs = NULL;
}

psAMap::psAMap(iObjectRegistry* objReg)
{
    this->objReg = objReg;
    vfs =  csQueryRegistry<iVFS> (objReg);
}

psANode::~psANode()
{
    DeleteConnectionsFromNeighbours();
    if(poly1!=NULL)
        poly1->DeleteNode(this);
    if(poly2!=NULL)
        poly2->DeleteNode(this);
    if(cluster!=NULL)
        cluster->DeleteNode(this);
}

void psAMap::PruneSuperfluous()
{
    csList<psANode*>::Iterator it, next;
    bool prunedSomething;

    //super ugly

    //na posledni node se vykasle

    do
    {
        prunedSomething = false;
        it = nodes;
        if(it.HasNext())
            it.Next();

        while(it.HasCurrent())
        {
            psANode* node = *it;

            if(it.HasNext())
            {
                next = it;
                next.Next();
            }
            else
                next = csList<psANode*>::Iterator();

            if(node->ShouldBePruned(*this))
            {
                delete node;
                nodes.Delete(it);
                prunedSomething = true;
            }

            it = next;
        }
    }
    while(prunedSomething);
}

psWalkPoly* psANode::GetSomePoly()
{
    if(poly1 != NULL)
        return poly1;
    return poly2;
}

bool psANode::ShouldBePruned(psAMap &map)
{
    size_t neighNum1, neighNum2;
    psAPath path;

    if(poly1 != NULL   &&   poly1->GetNumNodes() == 1)
    {
        CPrintf(CON_DEBUG, "cannot prune node %i (because poly1 has no more nodes)\n", id);
        return false;
    }
    if(poly2 != NULL   &&   poly2->GetNumNodes() == 1)
    {
        CPrintf(CON_DEBUG, "cannot prune node %i (because poly2 has no more nodes)\n", id);
        return false;
    }
    if(edges.GetSize() == 0)
        return true;

    csArray<psAEdge> &edges0 = edges[0];
    if(edges0.GetSize() == 0)
        return true;

    DeleteConnectionsFromNeighbours();
    for(neighNum1 = 0; neighNum1 < edges0.GetSize(); neighNum1++)
        for(neighNum2 = neighNum1+1; neighNum2 < edges0.GetSize(); neighNum2++)
        {
            if(!map.RunA(edges0[neighNum1].neighbour, edges0[neighNum2].neighbour, 0, 10, path)
                    ||
                    path.CalcCost() > (edges0[neighNum1].cost+edges0[neighNum2].cost)*5)
            {
                RestoreConnectionsFromNeighbours();
                CPrintf(CON_DEBUG, "cannot prune node %i (because of %i %i, cost=%f)\n", id, edges0[neighNum1].neighbour->id, edges0[neighNum2].neighbour->id, path.CalcCost());
                return false;
            }
        }
    RestoreConnectionsFromNeighbours();
    CPrintf(CON_DEBUG, "can prune node %i\n", id);
    return true;
}

float psANode::GetEdgeCost(psANode* neighbour)
{
    assert(edges.GetSize() > 0);

    csArray<psAEdge> &edges0 = edges[0];

    for(size_t edgeNum=0; edgeNum < edges0.GetSize(); edgeNum++)
        if(edges0[edgeNum].neighbour == neighbour)
            return edges0[edgeNum].cost;

    assert("neighbour not found"==0);
    return 0;
}

void psANode::DeleteEdges(psANode* neighbour)
{
    //CPrintf(CON_DEBUG, "DeleteEdges %i %i\n", id, neighbour->id);

    for(size_t level=0; level < edges.GetSize(); level++)
    {
        size_t neighNum = 0;
        csArray<psAEdge> &levEdges = edges[level];
        while(neighNum < levEdges.GetSize())
        {
            if(levEdges[neighNum].neighbour == neighbour)
                levEdges.DeleteIndex(neighNum);
            else
                neighNum ++;
        }
    }
}

void psANode::DeleteConnectionsFromNeighbours()
{
    if(edges.GetSize() == 0)
        return;

    csArray<psAEdge> &edges0 = edges[0];

    for(size_t neighNum = 0; neighNum < edges0.GetSize(); neighNum++)
        edges0[neighNum].neighbour->DeleteEdges(this);
}

void psANode::RestoreConnectionsFromNeighbours()
{
    if(edges.GetSize() == 0)
        return;

    csArray<psAEdge> &edges0 = edges[0];

    for(size_t neighNum = 0; neighNum < edges0.GetSize(); neighNum++)
    {
        float dist = CalcDistance(this, edges0[neighNum].neighbour);
        edges0[neighNum].neighbour->AddEdge(this, dist, 0, false);
    }
}

void psANode::LoadBasicsFromXML(iDocumentNode* node, psWalkPolyMap* wpMap, iEngine* engine)
{
    id = node->GetAttributeValueAsInt("id");
    point.x = node->GetAttributeValueAsFloat("x");
    point.y = node->GetAttributeValueAsFloat("y");
    point.z = node->GetAttributeValueAsFloat("z");
    sector = engine->FindSector(node->GetAttributeValue("sector"));

    if(wpMap != NULL)
    {
        poly1 = wpMap->FindPoly(node->GetAttributeValueAsInt("poly1"));
        poly2 = wpMap->FindPoly(node->GetAttributeValueAsInt("poly2"));
        if(poly1 != NULL)
            poly1->AddNode(this);
        if(poly2 != NULL)
            poly2->AddNode(this);
    }
}

void psANode::LoadEdgesFromXML(iDocumentNode* node, psAMap &map)
{
    csRef<iDocumentNodeIterator> it = node->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> edgeNode = it->Next();
        psANode* neighbour = map.FindNode(edgeNode->GetAttributeValueAsInt("id"));
        assert(neighbour);
        AddEdge(neighbour, edgeNode->GetAttributeValueAsFloat("cost"), edgeNode->GetAttributeValueAsInt("level"), false);
    }
}

bool psAMap::LoadFromString(const csString &str, psWalkPolyMap* wpMap)
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (objReg);

    csRef<iDocument> doc = ParseString(str);
    if(doc == NULL)
    {
        CPrintf(CON_DEBUG, "failed to parse A* map\n");
        CPrintf(CON_DEBUG, "%s\n",str.GetData());
        return false;
    }

    csRef<iDocumentNode> mapNode = doc->GetRoot()->GetNode("map");
    if(mapNode == NULL)
    {
        Error1("Missing <map> tag");
        return false;
    }

    csRef<iDocumentNodeIterator> it = mapNode->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> nodeNode = it->Next();
        psANode* node = new psANode;
        node->LoadBasicsFromXML(nodeNode, wpMap, engine);
        AddNode(node);
    }

    it = mapNode->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> nodeNode = it->Next();
        psANode* node = FindNode(nodeNode->GetAttributeValueAsInt("id"));
        assert(node);
        node->LoadEdgesFromXML(nodeNode, *this);
    }
    return true;
}

bool psAMap::LoadFromFile(const csString &path, psWalkPolyMap* wpMap)
{
    csRef<iDataBuffer> buff = vfs->ReadFile(path);
    if(buff == NULL)
    {
        Error2("Could not find file: %s", path.GetData());
        return false;
    }
    return LoadFromString(buff->GetData(), wpMap);
}


void psANode::SaveToString(csString &str)
{
    csString xml;

    str += csString().Format("<node id='%i' x='%f' y='%f' z='%f' poly1='%i' poly2='%i' sector='%s'>",
                             id, point.x, point.y, point.z,
                             poly1?poly1->GetID():0, poly2?poly2->GetID():0,
                             sector->QueryObject()->GetName());
    for(size_t levelNum=0; levelNum < edges.GetSize(); levelNum++)
    {
        csArray<psAEdge> &levEdges = edges[levelNum];
        for(size_t edgeNum=0; edgeNum < levEdges.GetSize(); edgeNum++)
            str += csString().Format("<edge level='%zu' id='%i' cost='%f'/>",
                                     levelNum, levEdges[edgeNum].neighbour->id, levEdges[edgeNum].cost);
    }
    str += "</node>";
}

void psAMap::SaveToString(csString &str)
{
    csList<psANode*>::Iterator it(nodes);

    str += "<map>\n";
    while(it.HasNext())
    {
        psANode* node = it.Next();
        node->SaveToString(str);
    }
    str += "</map>\n";
}

bool psAMap::SaveToFile(const csString &path)
{
    csString str;
    SaveToString(str);
    return vfs->WriteFile(path, str.GetData(), str.Length());
}


void psANode::DumpJS()
{
    if(edges.GetSize() == 0)
        return;
    csArray<psAEdge> &e = edges[0];
    for(size_t neighNum=0; neighNum < e.GetSize(); neighNum++)
    {
        psANode* neigh = e[neighNum].neighbour;
        CPrintf(CON_DEBUG, "drawLine({begin: {x: %f, z: %f}, end: {x: %f, z: %f}}, 'black', '', '')\n", point.x, point.z, neigh->point.x, neigh->point.z);
    }
}

void psAMap::DumpJS()
{
    csList <psANode*>::Iterator it(nodes);
    while(it.HasNext())
    {
        psANode* node = it.Next();
        node->DumpJS();
    }
}





void psAMap::AddNode(psANode* node)
{
    nodes.PushBack(node);
}

void psWalkPolyMap::BuildBasicAMap(psAMap &AMap)
{
    size_t vertNum;
    csHash<bool, csPtrKey<psWalkPoly> > doneConts;

    /* For each walkable polygon, create A* nodes in middles of contacts with neighbour polygons
        (if they were not created already) and connect them to all A* nodes
        from the current poly, and all the A* nodes from the poly that it contacts with. */
    csList<psWalkPoly*>::Iterator it(polys);
    while(it.HasNext())
    {
        psWalkPoly* poly = it.Next();
        for(vertNum = 0; vertNum < poly->verts.GetSize(); vertNum++)
        {
            psWalkPolyVert &vert = poly->verts[vertNum];
            csArray<psWalkPolyCont> &conts = vert.conts;

            for(size_t contNum = 0; contNum < conts.GetSize(); contNum++)
            {
                psWalkPolyCont &cont = conts[contNum];
                if(!doneConts.Get(cont.poly, false))
                {
                    psANode* mean = new psANode((cont.begin+cont.end)/2, poly->GetSector());
                    mean->poly1 = poly;
                    mean->poly2 = cont.poly;
                    poly->AddNode(mean);
                    cont.poly->AddNode(mean);
                    poly->ConnectNodeToPoly(mean, true);
                    cont.poly->ConnectNodeToPoly(mean, true);
                    AMap.AddNode(mean);

                    doneConts.Put(poly, true);
                }
            }
        }
    }
}

/*void psWalkPolyMap::BuildBasicAMap(psAMap & AMap)
{
    int vertNum;
    csHash<bool, psWalkPoly*> doneConts;

    // For each walkable polygon,
    //     create a psANode for each of its vertices,
    //     then fully interconnect them
    csList<psWalkPoly*>::Iterator it(polys);
    while (it.HasNext())
    {
        psWalkPoly * poly = it.Next();
        for (vertNum = 0; vertNum < poly->verts.GetSize(); vertNum++)
        {
            psANode * node = new psANode(poly->verts[vertNum].pos);
            poly->AddNode(node);
            poly->ConnectNodeToPoly(node, false);
            AMap.AddNode(node);
        }
    }

    // For each walkable polygon,
    //     create A* nodes in middle of contacts, and interconnect them
    //     with all vertices of both polygons
    it = polys;
    while (it.HasNext())
    {
        psWalkPoly * poly = it.Next();
        for (vertNum = 0; vertNum < poly->verts.GetSize(); vertNum++)
        {
            psWalkPolyVert & vert = poly->verts[vertNum];
            int nextVertNum = poly->GetNeighbourIndex(vertNum, +1);
            psWalkPolyVert & nextVert = poly->verts[nextVertNum];
            csArray<psWalkPolyCont> & conts = vert.conts;

            for (int contNum = 0; contNum < conts.GetSize(); contNum++)
            {
                psWalkPolyCont & cont = conts[contNum];
                if (!doneConts.Get(cont.poly, false))
                {
                    psANode * mean = new psANode((cont.begin+cont.end)/2);
                    poly->AddNode(mean);
                    cont.poly->AddNode(mean);
                    poly->ConnectNodeToPoly(mean, true);
                    cont.poly->ConnectNodeToPoly(mean, true);
                    AMap.AddNode(mean);

                    doneConts.Put(poly, true);
                }
            }
        }
    }
}*/


/*

    prunning

    mozna je nutne nejdriv zjistit koho chci vyhodit a udelat to az na konci,
        nez vyhazovat rovnou ? ? ?
    nebo spis mozna udelat tolik kol vyhazovani, dokud se neco vyhazuje
        protoze node mohla prezit jen diky svemu napojeni na jinou node, ktera ale byla
            pozdeji vyhozena
*/


/***********************************************************************************
*
*                            A* hierarchy construction
*
************************************************************************************/

template <typename T> typename csList<T>::Iterator FindInList(csList<T> &lst, T &x)
{
    typename csList<T>::Iterator it(lst);

    while(it.HasNext())
    {
        if(it.Next() == x)
            return it;
    }
#ifdef CS_COMPILER_MSVC
    return csList<T>::Iterator();
#elif !CS_COMPILER_MSVC
    return typename csList<T>::Iterator();
#endif
}

void psACluster::DeleteNode(psANode* node)
{
    csList<psANode*>::Iterator it;

    it = FindInList(parts, node);
    assert(it);
    parts.Delete(it);

    it = FindInList(exits, node);
    assert(it);
    exits.Delete(it);
}

int psACluster::CalcFreeBorderSize(int level)
{
    /*    int size=0;

        csList<psANode*>::Iterator it(parts);
        while (it.HasNext())

        for (size_t exitNum=0; exitNum < exits.GetSize(); exitNum++)
        {
            psANode * node = it.Next();
            csArray<psAEdge> & levEdges = edges[level];
            int neighNum=0;

            while (neighNum<levEdges.GetSize()  &&  IsClustered(neigh))
                neighNum++;

            if (neighNum<levEdges.GetSize())
                size++;
        }
        return size;*/
    return 0;
}

void psACluster::GrowClusterFrom(psANode* start, int level, csList <psANode*> &unassigned)
{
    /*    const int MIN_CLUSTER_SIZE = 10,
                  MAX_CLUSTER_SIZE = 20;

        csList<psANode*> open;  // nodes of our cluster that need expanding
        int minFreeBorderSize;
        int bestClusterSize;


        open.PushBack(start);
        csList<psANode*>::Iterator it;
        it = FindInList(unassigned, start);
        unassigned.Delete(it);
        AddNode(start);

        int clusterSize = 0;
        while (!open.IsEmpty()  &&  clusterSize<MAX_CLUSTER_SIZE)
        {
            // expand cluster by one node
            psANode * expanded = open.Front();
            open.PopFront();
            clusterSize++;

            for (int edgeNum=0; edgeNum < expanded->edges[level].GetSize(); edgeNum++)
            {
                psAEdge * edge = & expanded->edges[level][edgeNum];
                psANode * neighbour = edge->neighbour;

                if (!Contains(neighbour))
                {
                    csList<psANode*>::Iterator it;
                    it = FindInList(unassigned, neighbour);
                    unassigned.Delete(it);

                    open.PushBack(neighbour);
                    AddNode(neighbour);
                }
            }

            // record the best cluster size so far
            if (clusterSize >= MIN_CLUSTER_SIZE)
            {
                int currFreeBorderSize = CalcFreeBorderSize();
                if (clusterSize==MIN_CLUSTER_SIZE  ||  currFreeBorderSize<minFreeBorderSize)
                {
                    minFreeBorderSize   = currFreeBorderSize;
                    bestClusterSize = clusterSize;
                }
            }
        }

        // perhaps remove some nodes to get smaller size of free cluster border
        if (clusterSize >= MIN_CLUSTER_SIZE)
        {
            int excessSize = clusterSize - bestClusterSize;
            for (int i=0; i < excessSize; i++)
            {
                psANode * node = parts.Last();
                parts.PopBack();
                unassigned.PushBack(node);
            }
        }*/
}

/*void psACluster::PickExits()
{

}*/

/*void psAMap::BuildHierarchy()
{
    int level = 0;
    csArray <psANode*> unassigned;

    nextlevel = nodes;

    while (nextLevel.GetSize() > MAX_NODES_PER_LEVEL)
    {
        level++;
        unassigned = nextLevel;

        while (unassigned.GetSize() > 0)
        {
            psACluster * c = CreateClusterFrom(unassigned[0], unassigned);
            for (int exitNum=0; exitNum < c->exits.GetSize(); exitNum++)
                nextLevel.Push(c->exits[exitNum]);
        }
    }
}*/

/***********************************************************************************
*
*                            the A* algorithm
*
************************************************************************************/
psAEdge::psAEdge(psANode* neighbour, float cost)
{
    this->neighbour = neighbour;
    this->cost      = cost;
}

psANode::psANode()
{
    id = nextID++;
    poly1 = poly2 = NULL;
    sector = NULL;
    cluster = NULL;
    ARunNum = 0;
}

psANode::psANode(const csVector3 &point)
{
    id = nextID++;
    this->point = point;
    poly1 = poly2 = NULL;
    sector = NULL;
    cluster = NULL;
    ARunNum = 0;
}

psANode::psANode(const csVector3 &point, iSector* sector)
{
    id = nextID++;
    this->point = point;
    this->sector = sector;
    poly1 = poly2 = NULL;
    cluster = NULL;
    ARunNum = 0;
}

void psANode::AddEdge(psANode* neighbour, float cost, int level, bool bidirectional)
{
    if((int)edges.GetSize() <= level)
        edges.SetSize(level+1, csArray<psAEdge>());

    size_t neighNum;
    csArray<psAEdge> &levEdges = edges[level];
    for(neighNum=0; neighNum < levEdges.GetSize(); neighNum++)
        if(levEdges[neighNum].neighbour == neighbour)
            break;

    if(neighNum == levEdges.GetSize())
    {
        levEdges.Push(psAEdge(neighbour, cost));
    }

    if(bidirectional)
        neighbour->AddEdge(this, cost, level, false);
}

void psANode::SetBestPrev(psANode* bestPrev, float cost)
{
    this->bestPrev = bestPrev;
    this->bestCost = cost;
}

psANode* FindMostPromising(psANode* firstOpen)
{
    psANode* bestSoFar, * current;

    bestSoFar = firstOpen;
    current = firstOpen;
    while(current != NULL)
    {
        if(current->total < bestSoFar->total)
        {
            bestSoFar = current;
        }
        current = current->nextInOpen;
    }
    return bestSoFar;
}

void psANode::EnsureNodeIsInited(int currARunNum)
{
    if(ARunNum == currARunNum)
        return;

    ARunNum = currARunNum;
    state = AState_unknown;
    prevInOpen = nextInOpen = NULL;
    bestPrev = NULL;
    bestCost = 0;
    heur = 0;
    total = 0;
}

psANode* InsertIntoOpen(psANode* firstOpen, psANode* node)
{
    node->state = AState_open;

    node->prevInOpen = NULL;
    node->nextInOpen = firstOpen;
    if(firstOpen != NULL)
        firstOpen->prevInOpen = node;
    return node;
}

psANode* RemoveFromOpen(psANode* firstOpen, psANode* node)
{
    node->state = AState_closed;

    if(node->prevInOpen == NULL)
        firstOpen = node->nextInOpen;
    else
        node->prevInOpen->nextInOpen = node->nextInOpen;
    if(node->nextInOpen != NULL)
        node->nextInOpen->prevInOpen = node->prevInOpen;
    node->prevInOpen = node->nextInOpen = NULL;
    return firstOpen;
}

void ConstructPath(psANode* goal, psAPath &path)
{
    psANode* node;

    path.ResetNodes();
    node = goal;
    while(node != NULL)
    {
        path.AddNodeToFront(node);
        node = node->bestPrev;
    }
}

void psANode::CalcHeur(psANode* goal)
{
    heur = CalcDistance(point, goal->point);
    total = bestCost + heur;
}

bool psAMap::RunA(psANode* start, psANode* goal, int level, int maxExpands, psAPath &path)
{
    static int ARunNum = 0;
    psANode* firstOpen = NULL;
    psANode* expanded = NULL;
    psAEdge* edge;
    int numExpands;
    const bool dbg = false;

    ARunNum++;
    numExpands = 0;

    if(dbg)CPrintf(CON_DEBUG, "RunA start=%i goal=%i\n", start->id, goal->id);
    if(dbg)CShift();

    start->EnsureNodeIsInited(ARunNum);
    firstOpen = InsertIntoOpen(firstOpen, start);
    start->CalcHeur(goal);

    while(firstOpen != NULL)
    {
        expanded = FindMostPromising(firstOpen);
        numExpands ++;
        if(dbg)CPrintf(CON_DEBUG, "expanding=%i\n", expanded->id);
        if(expanded == goal   ||   numExpands == maxExpands)
            break;

        if(dbg)CShift();
        firstOpen = RemoveFromOpen(firstOpen, expanded);

        for(size_t edgeNum = 0; edgeNum < expanded->edges[level].GetSize(); edgeNum++)
        {
            edge = &expanded->edges[level][edgeNum];
            psANode* neighbour = edge->neighbour;

            // existuje skoro nulova pst ze nedojde k initu a stane se buhvi co
            neighbour->EnsureNodeIsInited(ARunNum);

            float newCost = expanded->bestCost + edge->cost;
            if(neighbour->state == AState_unknown)
            {
                if(dbg)CPrintf(CON_DEBUG, "unknown=%i\n", neighbour->id);
                firstOpen = InsertIntoOpen(firstOpen, neighbour);
                neighbour->SetBestPrev(expanded, newCost);
                neighbour->CalcHeur(goal);
            }
            else if(neighbour->state == AState_open)
            {
                if(dbg)CPrintf(CON_DEBUG, "known=%i\n", neighbour->id);
                if(newCost < neighbour->bestCost)
                {
                    if(dbg)CPrintf(CON_DEBUG, "updating\n");
                    neighbour->total -= neighbour->bestCost - newCost;
                    neighbour->SetBestPrev(expanded, newCost);
                }
            }
        }
        if(dbg)CUnshift();
    }

    if(expanded == goal)
    {
        ConstructPath(goal, path);
        if(dbg)CUnshift();
        return true;
    }
    else
    {
        if(dbg)CUnshift();
        return false;
    }
}

psANode* psAMap::FindNode(int id)
{
    csList <psANode*>::Iterator it(nodes);

    while(it.HasNext())
    {
        psANode* node = it.Next();
        if(node->id == id)
            return node;
    }
    return NULL;
}

/***********************************************************************************
*
*                            the hierarchical A* algorithm
*
************************************************************************************/

/*bool RunHierarchicalA(psANode * start, psANode * goal, int level, int ARunNum, psApath & path)
{
    findfinal
    foreachexit
        euclid
    return start + finallevel path
}*/


void pathFindTest_AStarWithArtificialMap()
{
    psAPath path;

    /*prima cara
    psANode * n0 = new psANode(csVector3(0,0,0), "0");
    psANode * n1 = new psANode(csVector3(0,0,1), "1");
    psANode * ng = new psANode(csVector3(0,0,2), "2");
    n0->AddEdge(n1, 1, 0);
    n1->AddEdge(ng, 1, 0);
    RunA(n0, ng, 0, path);
    Dump(path);*/

    /*slibna cesta ktera se ukaze slepou
    psANode * n0 = new psANode(csVector3(0,0,0), "0");
    psANode * n1 = new psANode(csVector3(-1,0,0), "1");
    psANode * n2 = new psANode(csVector3(-1,0,1), "2");
    psANode * n3 = new psANode(csVector3(-1,0,2), "3");
    psANode * n4 = new psANode(csVector3(0,0,1), "4");
    psANode * ng = new psANode(csVector3(0,0,2), "g");
    n0->AddEdge(n1, 1, 0);
    n1->AddEdge(n2, 1, 0);
    n2->AddEdge(n3, 1, 0);
    n3->AddEdge(ng, 1, 0);
    n0->AddEdge(n4, 1, 0);
    RunA(n0, ng, 0, path);
    Dump(path);*/
}

void pathFindTest_AStarWithRealMap(psNPCClient* cel)
{
    psAPath path;
    psAMap AMap(cel->GetObjectReg());
    psWalkPolyMap PMap(cel->GetObjectReg());

    /*PMap.LoadFromFile("/this/pf10.xml");
    PMap.CalcConts();
    PMap.BuildBasicAMap(AMap);*/

    AMap.LoadFromFile("/this/pfa20.xml", NULL);

    psANode* start = AMap.FindNode(0);
    psANode* end   = AMap.FindNode(38);
    assert(start&&end);

    AMap.RunA(start, end, 0, 999, path);
    path.Dump();
}

void pathFindTest_PathCalc(psNPCClient* npcclient)
{
    psPFMaps maps(npcclient->GetObjectReg());
    psAPath path(&maps);
    csVector3 localDest;

    //psAMap AMap(cel->GetObjectReg());
    //psWalkPolyMap PMap(cel->GetObjectReg());

    //PMap.LoadFromFile("/this/pfgen.xml");
    //AMap.LoadFromFile("/this/pfagen.xml", &PMap);

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());

    path.SetDest(csVector3(-239, -4.75F, -91.43F));

    path.CalcLocalDest(csVector3(-102.65F, -16.8F, -197.99F), engine->FindSector("blue"), localDest);

    path.Dump();
    ::Dump3("localDest", localDest);
}


/***********************************************************************************
*
*                            psPFMaps
*
************************************************************************************/


psPFMaps::psPFMaps(iObjectRegistry* objReg)
{
    this->objReg = objReg;
    engine =  csQueryRegistry<iEngine> (objReg);
}

iCollection* psPFMaps::GetCSRegionOfSector(const csString &sectorName)
{
    csRef<iCollectionArray> list = engine->GetCollections();
    for(size_t regionNum = 0; regionNum < list->GetSize(); regionNum++)
    {
        iCollection* r = list->Get(regionNum);
        if(r->FindSector(sectorName) != NULL)
            return r;
    }
    return NULL;
}

psPFMap* psPFMaps::FindRegionByName(const csString &regionName)
{
    csList<psPFMap*>::Iterator it(regions);

    while(it.HasNext())
    {
        psPFMap* region = it.Next();
        if(region->name == regionName)
            return region;
    }
    return NULL;
}

psPFMap* psPFMaps::GetRegionBySector(iSector* sector)
{
    return GetRegionBySector(sector->QueryObject()->GetName());
}

psPFMap* psPFMaps::GetRegionBySector(const csString &sectorName)
{
    psPFMap* region;
    iCollection* CSregion;
    csString regionName;
    const bool dbg = false;

    if(dbg)CPrintf(CON_DEBUG, "GRBS %s\n", sectorName.GetData());

    region = regionMap.Get(sectorName.GetData(), NULL);
    if(region != NULL)
        return region;

    if(dbg)CPrintf(CON_DEBUG, "not found in regionMap\n");

    CSregion = GetCSRegionOfSector(sectorName);
    if(CSregion == NULL)
        return NULL;

    regionName = CSregion->QueryObject()->GetName();
    region = FindRegionByName(regionName);
    if(region != NULL)
    {
        regionMap.Put(sectorName.GetData(), region);
        return region;
    }

    if(dbg)CPrintf(CON_DEBUG, "not found in region list\n");

    region = new psPFMap;
    region->name = regionName;
    region->region = CSregion;
    region->wpMap = new psWalkPolyMap(objReg);
    if(!region->wpMap->LoadFromFile("/planeshift/pfmaps/" + regionName + "_wpmap.xml"))
    {
        CPrintf(CON_DEBUG, "failed to load WalkPoly map\n");
        return NULL;
    }
    region->aMap = new psAMap(objReg);
    if(!region->aMap->LoadFromFile("/planeshift/pfmaps/" + regionName + "_amap.xml", region->wpMap))
    {
        CPrintf(CON_DEBUG, "failed to load A* map\n");
        return NULL;
    }
    regions.PushBack(region);
    regionMap.Put(sectorName.GetData(), region);
    return region;
}

/***********************************************************************************
*
*                            psAPath
*
************************************************************************************/

const int MAX_STEPS_OF_DIRECT = 3;

psAPath::psAPath()
{
    this->maps = NULL;
}

psAPath::psAPath(psPFMaps* maps)
{
    this->maps = maps;
}

void psAPath::SetMaps(psPFMaps* maps)
{
    this->maps = maps;
}

psANode* psAPath::GetNthNode(int n)
{
    int currNodeNum = -1;
    psANode* currNode = NULL;
    csList<psANode*>::Iterator it(nodes);

    while(it.HasNext() && currNodeNum!=n)
    {
        currNode = it.Next();
        currNodeNum++;
    }

    if(currNodeNum == n)
        return currNode;
    else
        return NULL;
}

csList<psANode*>::Iterator psAPath::FindDirectNode(const csVector3 &currPoint)
{
    csList<psANode*>::Iterator firstNotDir(nodes);
    csList<psANode*>::Iterator lastDir;
//    psWalkPolyMap * wpMap;
    int numSeenNodes = 0;
    const bool dbg = true;

    if(dbg)
    {
        CPrintf(CON_DEBUG, "FindDirectNode\n");
        CShift();
        Dump();
        Dump3("currPoint", currPoint);
    }

    while(firstNotDir.HasNext()  && numSeenNodes<MAX_STEPS_OF_DIRECT)
    {
        psANode* node = firstNotDir.Next();
        numSeenNodes++;

        if(dbg)Dump3("trying", node->point);

        psPFMap* region = maps->GetRegionBySector(node->sector);
        if(region == NULL)
        {
            if(dbg)CUnshift();
            return csList<psANode*>::Iterator();
        }
        if(!region->wpMap->CheckDirectPath(currPoint, node->point))
            break;
        if(dbg)CPrintf(CON_DEBUG, "is direct\n");
        lastDir = firstNotDir;
    }

    if(dbg)CUnshift();
    return lastDir;
}


csVector3 RandomizePoint(const csVector3 &point, float max)
{
    csVector3 p = point;
    p.x += rndf(-max, +max);
    p.z += rndf(-max, +max);
    return p;
}

void psAPath::RandomizeFirst(const csVector3 &currPoint)
{
    psANode* firstNode, * secondNode;
    csVector3 firstPoint, secondPoint;
    csVector3 rndPoint;

    firstNode = GetNthNode(0);
    if(firstNode == NULL)
        return;
    firstPoint = firstNode->point;

    psPFMap* region = maps->GetRegionBySector(firstNode->sector);
    if(region == NULL)
        return;

    secondNode = GetNthNode(1);
    if(secondNode == NULL)
        secondPoint = dest;
    else
        secondPoint = secondNode->point;

    const int MAX_TRIALS = 3;
    int numTrials = 0;
    do
    {
        rndPoint = RandomizePoint(firstNode->point, 1);
        numTrials ++;
    }
    while(region->wpMap->CheckDirectPath(currPoint, rndPoint)
            && region->wpMap->CheckDirectPath(rndPoint, secondPoint)
            && numTrials < MAX_TRIALS);

    if(numTrials < MAX_TRIALS)
        firstNode->point = rndPoint;
}

bool psAPath::CalcLocalDestFromLocalPath(const csVector3 &currPoint, csVector3 &localDest)
{
    const bool dbg = false;
    csList<psANode*>::Iterator directNode;

    directNode = FindDirectNode(currPoint);
    if(!directNode.HasCurrent())
        return false;

    if(dbg)CPrintf(CON_DEBUG, "direct node %i\n", (*directNode)->id);

    csList<psANode*>::Iterator it(nodes), next;
    it.Next();
    //if (dbg)CPrintf(CON_DEBUG, "exa %i\n", (*it)->id);
    while((*it)->id != (*directNode)->id)
    {
        next = it;
        next.Next();
        if(dbg)CPrintf(CON_DEBUG, "deleting before direct %i\n", (*it)->id);
        nodes.Delete(it);
        it = next;
    }

    //RandomizeFirst(currPoint);

    localDest = nodes.Front()->point;
    return true;
}

psANode* psAPath::FindNearbyANode(const csVector3 &pos, iSector* sector)
{
    psPFMap* r = maps->GetRegionBySector(sector);
    if(r == NULL)
        return NULL;
    return r->wpMap->FindNearbyANode(pos);
}

void psAPath::InsertSubpath(psAPath &subpath)
{
    bool wasEmpty = nodes.IsEmpty();
    csList<psANode*>::Iterator first;

    if(!wasEmpty)
    {
        first = nodes;
        first.Next(); // move to first
    }

    csList<psANode*>::Iterator it(subpath.nodes);
    while(it.HasNext())
        if(wasEmpty)
            nodes.PushBack(it.Next());
        else
            nodes.InsertBefore(first, it.Next());
}

void psAPath::CalcLocalDest(const csVector3 &currPoint, iSector* currSector, csVector3 &localDest)
{
    const bool dbg = false;

    if(dbg)CPrintf(CON_DEBUG, "CalcLocalDest\n");
    if(dbg)Dump3("currPoint", currPoint);
    if(dbg)Dump3("dest", dest);

    localDest = dest;  // if we fail to determine the local destination, then we just go blindly to the final goal by default
    return;

    psPFMap* r = maps->GetRegionBySector(currSector);
    if(r == NULL)
    {
        if(dbg)CPrintf(CON_DEBUG, "region not found\n");
        return;
    }

    if(r->wpMap->CheckDirectPath(currPoint, dest))
    {
        //todo: check of larger distances/different regions
        if(dbg)CPrintf(CON_DEBUG, "dest is direct\n");
        nodes.DeleteAll();
        localDest = dest;
    }
    else if(nodes.IsEmpty())
    {
        if(dbg)CPrintf(CON_DEBUG, "path is empty\n");

        psANode* startNode, * destNode;

        startNode = FindNearbyANode(currPoint, currSector);
        if(startNode == NULL) return;

        destNode = FindNearbyANode(dest, currSector);
        if(destNode == NULL) return;

        r->aMap->RunA(startNode, destNode, 0, MAX_EXPANDS, *this);

        //if (dbg)CPrintf(CON_DEBUG, "A* result:\n");
        //if (dbg)Dump();

        CalcLocalDestFromLocalPath(currPoint, localDest);
    }
    else if(CalcLocalDestFromLocalPath(currPoint, localDest))
    {
        if(dbg)CPrintf(CON_DEBUG, "the beginning of path is direct\n");
    }
    else
    {
        if(dbg)CPrintf(CON_DEBUG, "first point of path is NOT direct\n");

        psANode* startNode;

        startNode = FindNearbyANode(currPoint, currSector);
        if(startNode == NULL) return;

        psAPath subpath;
        r->aMap->RunA(startNode, nodes.Front(), 0, MAX_EXPANDS, subpath);

        if(dbg)CPrintf(CON_DEBUG, "A* result:\n");
        if(dbg)subpath.Dump();

        subpath.nodes.PopBack();
        InsertSubpath(subpath);

        CalcLocalDestFromLocalPath(currPoint, localDest);
    }

    if(dbg)Dump3("returning", localDest);
}

float psAPath::CalcCost()
{
    csList<psANode*>::Iterator it = nodes;
    psANode* previous;
    float cost;

    cost = 0;
    previous = NULL;

    while(it.HasNext())
    {
        psANode* current = it.Next();
        if(previous != NULL)
            cost += previous->GetEdgeCost(current);
        previous = current;
    }

    return cost;
}

void psAPath::Dump()
{
    CPrintf(CON_DEBUG, "Path=\n");
    CShift();
    csList<psANode*>::Iterator it(nodes);
    while(it.HasNext())
    {
        psANode* node = it.Next();
        CPrintf(CON_DEBUG, "node=%i\n", node->id);
        CShift();
        ::Dump3("", node->point);
        CUnshift();
    }
    CUnshift();
}

void psAPath::AddNodeToFront(psANode* node)
{
    nodes.PushFront(node);
}

void psAPath::ResetNodes()
{
    nodes.DeleteAll();
}

void psAPath::SetDest(const csVector3 &dest)
{
    ResetNodes();
    this->dest = dest;
}

//////////////////////////////////////////////////////

void pathFindTest_APathGetDest(psNPCClient* npcclient)
{
    psPFMaps* maps = new psPFMaps(npcclient->GetObjectReg());
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());
    psAPath path(maps);
    csVector3 localDest;

    path.SetDest(csVector3(-180, -4, -135));
    //path.CalcLocalDest(csVector3(-180, -4, -150), engine->FindSector("blue"), locDest);
    //path.CalcLocalDest(csVector3(-180, -4, -195), engine->FindSector("blue"), locDest);

    path.CalcLocalDest(csVector3(-170, -4, -195), engine->FindSector("blue"), localDest);
    path.CalcLocalDest(csVector3(-179, -4, -184), engine->FindSector("blue"), localDest);

    ::Dump3("localDest", localDest);
}

void pathFindTest_CheckDirect(psNPCClient* npcclient);
void pathFindTest_Inter();
void pathFindTest_PolyInter();
void pathFindTest_NormEq();
void pathFindTest_Walkability(psNPCClient* npcclient);
void pathFindTest_Stretch(psNPCClient* cel);
void pathFindTest_Convex();
void pathFindTest_Save(psNPCClient* cel);
void pathFindTest_PruneWP(psNPCClient* cel);
void pathFindTest_PathCalc(psNPCClient* cel);

void pathFindTest(psNPCClient* cel)
{
    //pathFindTest_CheckDirect(cel);
    //pathFindTest_NormEq();
    //pathFindTest_Walkability(cel);
    //pathFindTest_PolyInter();
    //pathFindTest_Stretch(cel);
    //pathFindTest_Convex();
    //pathFindTest_Save(cel);

    //pathFindTest_AStarWithRealMap(cel);

    pathFindTest_Stretch(cel);
    //pathFindTest_AMapConstruction(cel);

    //pathFindTest_PathCalc(cel);

    //pathFindTest_PruneWP(cel);

    //pathFindTest_APathGetDest(cel);
}

#endif
