/*
* walkpoly.cpp
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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <imesh/object.h>
#include <imesh/objmodel.h>
#include <csutil/xmltiny.h>
#include <csutil/databuf.h>
#include <iutil/object.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/consoleout.h"
#include "util/psprofile.h"
#include "util/log.h"
#include "util/psxmlparser.h"

#include "engine/linmove.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "walkpoly.h"
#include "npcmesh.h"
#include "npcclient.h"

const int VERT_COUNT      = 12;   // low vert count limits possible stretching directions
const int NUM_DIRECTIONS  = 1;
const int MAX_STEPS       = 160;
const float ANGLE_SPAN    = 0;//PI/2;
const float SEED_SIZE = 0.1F;
const float SEED_EDGE_DIST = SEED_SIZE/2+0.01;
const float MAX_COLL_Y = 2;
const float POLYCOLL_ADJUST = 0.01F;

// 0 je -z
// 90 je -x
// 180 je z
// 270 je x

int psWalkPoly::nextID = 0;

psNamedProfiles prof;

bool CheckConvexity(const csVector3 &prev, const csVector3 &changedPoint, const csVector3 &next);
bool PointIsNearWalls(psMapWalker &walker, const csVector3 &point, iSector* sector, float dist);
void EstimateY(const csVector3 &begin, const csVector3 &end, csVector3 &point);
void MoveInDirection(float angle, float length, const csVector3 &oldPos, csVector3 &newPos);

void Dump3(const char* name, const csVector3 &p)
{
    CPrintf(CON_DEBUG, "point '%s' = %f %f %f\n", name, p.x, p.y, p.z);
}
void Dump(const char* name, const csVector3 &p)
{
    CPrintf(CON_DEBUG, "point '%s': {x: %f, z: %f}\n", name, p.x, p.z);
}

template <typename T> int ListLength(csList<T> &l)
{
    int length=0;
    typename csList<T>::Iterator it(l);
    while(it.HasNext())
    {
        it.Next();
        length++;
    }
    return length;
}


bool Between(float a, float b, float c)
{
    return (a>=b && a<=c) || (a>=c && a<=b);
}

bool RBetween(float a, float b, float c)
{
    const float toler = 0.001F;
    return (a>=b-toler && a<=c+toler) || (a>=c-toler && a<=b+toler);
}

float CalcAngle(const csVector3 &start, const csVector3 &goal)
{
    float diffX = goal.x - start.x,
          diffZ = goal.z - start.z;
    float angle;

    if(diffX > 0)
    {
        if(diffZ > 0)
            angle = atanf(diffX / diffZ) + PI;
        else
            angle = atanf(-diffZ / diffX) + 1.5*PI;
    }
    else
    {
        if(diffZ > 0)
            angle = atanf(diffZ / -diffX) + 0.5*PI;
        else
            angle = atanf(-diffX / -diffZ) + 0*PI;
    }
    return angle;
}

float CalcDistance(const csVector3 &a, const csVector3 &b)
{
    return (a-b).Norm();
}

float Calc2DDistance(const csVector3 &a, const csVector3 &b)
{
    csVector3 diff = a-b;
    diff.y = 0;
    return diff.Norm();
}



bool CalcInter(ps2DLine &line1,
               ps2DLine &line2,
               csVector3 &inter)
{
    //CPrintf(CON_DEBUG, "CalcInter:\n");
    //CShift();
    //line1.Dump();
    //line2.Dump();

    if(line2.a == 0)
        inter.z = -line2.c / line2.b;
    else
    {
        float dv = line1.a / line2.a;
        float subB = (line1.b - dv * line2.b);
        if(subB == 0)
        {
            //CUnshift();
            return false; // co kdyz splyvaji, nebo jsou na stejne primce ale nedotykaji se
        }
        inter.z = -
                  (line1.c - dv * line2.c)
                  /
                  subB;
    }
    if(line1.a == 0)
        inter.x = - (line2.b * inter.z + line2.c) / line2.a;
    else
        inter.x = - (line1.b * inter.z + line1.c) / line1.a;

    //CPrintf(CON_DEBUG, "x=%f z=%f\n", inter.x, inter.z);
    //CUnshift();
    return true;
}

bool CalcInter(const csVector3 &a1, const csVector3 &b1,
               ps2DLine &line1,
               const csVector3 &a2, const csVector3 &b2,
               ps2DLine &line2,
               csVector3 &inter)
{
    return CalcInter(line1, line2, inter)
           && RBetween(inter.x, a1.x, b1.x)
           && RBetween(inter.x, a2.x, b2.x)
           && RBetween(inter.z, a1.z, b1.z)
           && RBetween(inter.z, a2.z, b2.z);
}

bool CalcInter(const csVector3 &a1, const csVector3 &b1,
               ps2DLine &line1,
               ps2DLine &line2,
               csVector3 &inter)
{

    return CalcInter(line1, line2, inter)
           && RBetween(inter.x, a1.x, b1.x)
           && RBetween(inter.z, a1.z, b1.z);
}


void pathFindTest_NormEq()
{
    csVector3 a(0, 0, 0);
    csVector3 b(-1, 0, -1);
    csVector3 c(-11, 0, -10);
    ps2DLine line;
    line.Calc(a, b);
    line.Dump();
    //Error2("convex %i\n", int(CheckConvexity(a, b, c)));
}

void pathFindTest_Inter()
{
    csVector3 a1(0, 0, 0);
    csVector3 b1(1, 0, 1);
    csVector3 a2(0, 0, 1);
    csVector3 b2(1, 0, 0);
    ps2DLine line1, line2;
    csVector3 inter;

    line1.Calc(a1, b1);
    line1.Dump();
    line2.Calc(a2, b2);
    line2.Dump();

    CalcInter(a1, b1, line1, a2, b2, line2, inter);

    Error3("inter=%f %f", inter.x, inter.z);

    /*csVector3 start(0, 0, 0);
    csVector3 goal;
    CheckDirectPath(const csVector3 & start, const csVector3 & goal)*/
}

psANode* psWalkPoly::GetFirstNode()
{
    assert(nodes.GetSize() > 0);
    return nodes[0];
}

psWalkPolyVert::psWalkPolyVert()
{
    touching = stuck = followingEdge = false;
}

psWalkPolyVert::psWalkPolyVert(const csVector3 &pos)
{
    this->pos = pos;
    touching = stuck = followingEdge = false;
}
/***********************************************************************************
*
*                            walkable poly precalculations
*
************************************************************************************/

void psWalkPoly::AddNode(psANode* node)
{
    nodes.Push(node);
}

int psWalkPoly::FindClosestEdge(const csVector3 pos)
{
    float closestDist=0;
    int closestEdge=-1;

    for(int vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        float currDist = verts[vertNum].line.CalcDist(pos);
        if(vertNum==0  ||  currDist < closestDist)
        {
            closestDist = currDist;
            closestEdge = vertNum;
        }
    }
    return closestEdge;
}

void psWalkPoly::ConnectNodeToPoly(psANode* node, bool bidirectional)
{
    int closestToNode = FindClosestEdge(node->point);

    for(size_t nodeNum=0; nodeNum < nodes.GetSize(); nodeNum++)
    {
        psANode* otherNode = nodes[nodeNum];
        if(node != otherNode)
        {
            int closestToOtherNode = FindClosestEdge(otherNode->point);

            // if they are on the same edge, their connection would lead exactly
            // on this edge, which makes CheckDirectPath problematic - it can
            // collide with several contacts
            if(closestToNode != closestToOtherNode)
            {
                float dist = CalcDistance(node->point, otherNode->point);
                node->AddEdge(otherNode, dist, 0, bidirectional);
            }
        }
    }
}

void psWalkPoly::CalcBox()
{
    box.StartBoundingBox();
    if(verts.GetSize() == 0)
        return;

    box.Set(verts[0].pos, verts[0].pos);
    for(size_t i=1; i < verts.GetSize(); i++)
        box.AddBoundingVertex(verts[i].pos);

    box.SetMin(1, box.MinY()-1);
    box.SetMax(1, box.MaxY()+1);
}

void psWalkPoly::Clear()
{
    verts.SetSize(0);
    sector = NULL;
    box.StartBoundingBox();
}

psWalkPoly* psWalkPoly::FindCont(const csVector3 &point, int edgeNum)
{
    return verts[edgeNum].FindCont(point);
}

void psWalkPoly::AddVert(const csVector3 &pos, int beforeIndex)
{
//    int next, prev;

    if(beforeIndex == -1)
        beforeIndex = (int)verts.GetSize();

    verts.Insert(beforeIndex, psWalkPolyVert(pos));

    if(verts.GetSize() >= 2)
        RecalcEdges(beforeIndex);
}

void ClipPoint(const csVector3 &point, const csVector3 &a, const csVector3 &b,
               float p1D, float a1D, float b1D,
               csVector3 &newPoint)
{
    if(p1D<a1D && p1D<b1D)
    {
        if(a1D < b1D)
            newPoint = a;
        else
            newPoint = b;
    }
    else if(p1D>a1D && p1D>b1D)
    {
        if(a1D > b1D)
            newPoint = a;
        else
            newPoint = b;
    }
    else
        newPoint = point;
}

void psWalkPoly::CalcConts(psWalkPolyMap &map)
{
    bool dbg = false;

    for(int vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        psWalkPolyVert &vert = verts[vertNum];
        vert.conts.DeleteAll();
        int endVertNum = GetNeighbourIndex(vertNum, +1);
        psWalkPolyVert &endVert = verts[endVertNum];
        if(dbg)
        {
            printf("precalculating edge:\n");
            ::Dump("a", verts[vertNum].pos);
            ::Dump("b", verts[endVertNum].pos);
        }

        csList<psWalkPoly*>::Iterator it(map.polys);
        while(it.HasNext())
        {
            if(dbg)printf("trying poly\n");
            psWalkPoly* poly2 = it.Next();
            if(poly2 != this)
                for(int k=0; k < (int)poly2->verts.GetSize(); k++)
                {
                    int endVertNum2 = poly2->GetNeighbourIndex(k, +1);
                    psWalkPolyVert &vert2 = poly2->verts[k];
                    psWalkPolyVert &endVert2 = poly2->verts[endVertNum2];
                    float dist;

                    if(Calc2DDistance(vert.pos, endVert.pos)
                            >
                            Calc2DDistance(vert2.pos, endVert2.pos)
                      )
                        dist = csMax(vert.line.CalcDist(vert2.pos), vert.line.CalcDist(endVert2.pos));
                    else
                        dist = csMax(vert2.line.CalcDist(vert.pos), vert2.line.CalcDist(endVert.pos));

                    float angle1 = CalcAngle(vert.pos, endVert.pos);
                    float angle2 = CalcAngle(endVert2.pos, vert2.pos);

                    if(dbg)
                    {
                        printf("trying edge:\n");
                        ::Dump("a", poly2->verts[k].pos);
                        ::Dump("b", poly2->verts[endVertNum2].pos);

                        /*printf("distances to second edge %f %f\n",
                            vert.line.CalcDist(poly2->verts[k].pos),
                            vert.line.CalcDist(poly2->verts[endVertNum2].pos));*/
                    }

                    if(dist < 0.05  &&  AngleDiff(angle1, angle2) <= PI/20)
                    {
                        if(dbg)printf("second edge is in the same line\n");
                        psWalkPolyCont cont;
                        float a1, b1, a2, b2;

                        if(verts[vertNum].pos.x == verts[endVertNum].pos.x)
                        {
                            a1 = verts[vertNum].pos.z;
                            b1 = verts[endVertNum].pos.z;
                            a2 = poly2->verts[k].pos.z;
                            b2 = poly2->verts[endVertNum2].pos.z;
                        }
                        else
                        {
                            a1 = verts[vertNum].pos.x;
                            b1 = verts[endVertNum].pos.x;
                            a2 = poly2->verts[k].pos.x;
                            b2 = poly2->verts[endVertNum2].pos.x;
                        }

                        ClipPoint(poly2->verts[k].pos, verts[vertNum].pos, verts[endVertNum].pos,
                                  a2, a1, b1,
                                  cont.begin);
                        ClipPoint(poly2->verts[endVertNum2].pos, verts[vertNum].pos, verts[endVertNum].pos,
                                  b2, a1, b1,
                                  cont.end);

                        if(dbg)printf("clipped contact:\n");
                        if(dbg)::Dump("a", cont.begin);
                        if(dbg)::Dump("b", cont.end);

                        if(cont.begin != cont.end)
                        {
                            if(dbg) printf("##################################################################################################adding contact\n");
                            if(dbg)printf("%f %f - %f %f (%p %p)\n", cont.begin.x, cont.begin.z, cont.end.x, cont.end.z, this, poly2);
                            cont.poly  = poly2;
                            vert.conts.Push(cont);
                        }
                    }
                }
        }
    }
}

psWalkPoly* psWalkPolyVert::FindCont(const csVector3 &point)
{
    const bool dbg = false;

    if(dbg)CPrintf(CON_DEBUG, "wpVert::FindCont this=%i\n", intptr_t(this));
    if(dbg)CShift();
    for(size_t contNum=0; contNum < conts.GetSize(); contNum++)
    {
        psWalkPolyCont &cont = conts[contNum];
        if(dbg)CPrintf(CON_DEBUG, "testing contact %f %f - %f %f (%i)\n", cont.begin.x, cont.begin.z, cont.end.x, cont.end.z, intptr_t(cont.poly));
        if(Between(point.x, cont.begin.x, cont.end.x)
                && Between(point.z, cont.begin.z, cont.end.z))
        {
            if(dbg)CPrintf(CON_DEBUG, "contact found\n");
            if(dbg)CUnshift();
            return cont.poly;
        }
    }
    if(dbg)CPrintf(CON_DEBUG, "contact not found\n");
    if(dbg)CUnshift();
    return NULL;
}


void psWalkPolyVert::CalcLineTo(const csVector3 &point)
{
    line.Calc(pos, point);
}


bool Equal2D(const csVector3 &p1, const csVector3 &p2)
{
    return p1.x==p2.x  &&  p1.z==p2.z;
}

//float CalcSlope(const csVector3 & from, const csVector3 & to)
/***********************************************************************************
*
*                            psMapWalker
*
************************************************************************************/

/** You can pass subclass of this to psMapWalker to be notified about the points
    that the walker is walking through */
class iWalkCallback
{
public:
    virtual void PositionNotify(const csVector3 &pos, iSector* sector) = 0;
    virtual ~iWalkCallback() {}
};

class psMapWalker
{
public:
    static const float DELTA;
    //static const float WALK_Y_TOLERANCE = 0.3;
    //must be big enough to tolerate terrain elevation and setmini

    psMapWalker(psNPCClient* npcclient)
    {
        this->npcclient = npcclient;
        pcmesh = NULL;
        pcmove = NULL;
    }

    ~psMapWalker()
    {
        delete pcmesh;
        delete pcmove;
    }

    bool Init()
    {
        return InitMesh("stonebm", "/planeshift/models/stonebm/stonebm.cal3d")
               &&
               InitLinmove()
               &&
               InitCD();
    }

    bool WalkBetween(csVector3 begin,
                     csVector3 end, iSector* sector,
                     bool checkY,
                     float &realEndY,
                     iWalkCallback* callback=NULL)
    {
        csVector3 currPoint, newPoint;
        ps2DLine path, goalLine;
        float rot;
        int numSteps = 0;
        iSector* newSector;
        float speed, smoothSpeed=0.0;
        bool firstStep;
        psStopWatch watch;
        const bool dbg1 = true;
        const bool dbg2 = false;

        if(dbg1)
        {
            CPrintf(CON_DEBUG, "WalkBetween: (angle=%f)\n",CalcAngle(begin, end));
            ::Dump3("begin", begin);
            ::Dump3("end",   end);
        }

        //begin.y += 0.05; // begin a bit in the air

        path.Calc(begin, end);
        goalLine.CalcPerp(begin, end);

        SetPosition(begin, 0, sector);
        /*SettleDown();
        ::Dump3("after settle",   end);*/
        SetForwardSpeed(1);

        firstStep = true;
        currPoint = begin;
        while(true)
        {
            rot = CalcAngle(currPoint, end);
            if(dbg2)Dump3("walkability - currPoint", currPoint);
            if(dbg2)CPrintf(CON_DEBUG, "angle = %f\n", rot);

            SetRotation(rot);
            watch.Start();
            WalkABit();
            prof.AddCons("WalkABit", watch.Stop());
            GetPosition(newPoint, newSector);
            if(dbg2)Dump3("walkability - newPoint", newPoint);

            speed = CalcDistance(currPoint, newPoint) / DELTA;
            if(firstStep)
            {
                firstStep = false;
                smoothSpeed = speed;
            }
            else
                smoothSpeed = smoothSpeed*0.8 + speed*0.2;

            float dist = path.CalcDist(newPoint);
            if(dbg2)CPrintf(CON_DEBUG, "dist = %f\n", dist);
            numSteps++;

            if(dbg2)CPrintf(CON_DEBUG, "speed=%f smoothSpeed=%f\n", speed, smoothSpeed);

            // Any tolerance here can result in points that are a bit in the wall,
            // and are reachable from some angles but not from other angles
            // Because of float inaccuracy, it can happen that newPoint==end, but
            // CalcRel() still returns negative value, so we must check that.
            if(dbg2)CPrintf(CON_DEBUG, "rel=%f\n", goalLine.CalcRel(newPoint));
            if(Calc2DDistance(newPoint, end)<0.001  ||  goalLine.CalcRel(newPoint) >= 0)
                break;

            // have we just walked out of our sector ?
            if(sector != newSector)
            {
                //printf("sector cross\n");
                return false;
            }
            // have we just fallen down ?
            //if (!IsOnGround())
            if((newPoint.y - currPoint.y) * DELTA < -1)
            {
                printf("not onground\n");
                return false;
            }
            // have we just diverged from the path too much ?
            // be forgiving, because more complex terrain can cause small divergence from path
            else if(dist > 1)
            {
                if(dbg1)CPrintf(CON_DEBUG, "dist\n");
                if(dbg1)Dump3("failing point", newPoint);
                return false;
            }
            // did we just have to stop because of obstacles ?
            // some obstacles on floor can slow down, but not stop
            else if(smoothSpeed < 0.25)
            {
                if(dbg1)CPrintf(CON_DEBUG, "speed\n");
                if(dbg1)Dump3("failing point", newPoint);
                return false;
            }
            else if(numSteps > 1000)
                return false;

            if(callback != NULL)
                callback->PositionNotify(newPoint, newSector);


            currPoint = newPoint;
        }

        SettleDown(); //to calculate real y
        GetPosition(newPoint, newSector);

        //check if we reached the point on wrong floor
        if(checkY   &&   fabs(newPoint.y - end.y) >= 0.9*height/*WALK_Y_TOLERANCE*/)
        {
            if(dbg1)CPrintf(CON_DEBUG, "overYtolerance (%f vs %f)\n", newPoint.y, end.y);
            return false;
        }

        realEndY = newPoint.y;

        if(dbg1)CPrintf(CON_DEBUG, "WalkBetween success, realY=%f\n", realEndY);
        return true;
    }



protected: ////////////////////

    void SettleDown()
    {
        SetForwardSpeed(0.0001F);
        int numSteps=0;
        while(!IsOnGround()  &&  numSteps < 100)
        {
            WalkABit();
            numSteps++;
        }
    }

    void WalkABit()
    {
        pcmove->ExtrapolatePosition(DELTA);
    }

    float GetSpeed()
    {
        csVector3 vel;
        vel = pcmove->GetVelocity();
        return vel.Norm();
    }

    void GetPosition(csVector3 &pos, iSector* &sector)
    {
        float yrot;
        pcmove->GetLastPosition(pos, yrot, sector);
    }

    bool IsOnGround()
    {
        return pcmove->IsOnGround();
    }

    void SetRotation(float rot)
    {
        csVector3 pos;
        float oldRot;
        iSector* sector;

        pcmove->GetLastPosition(pos, oldRot, sector);
        pcmove->SetPosition(pos, rot, sector);
    }


    void SetPosition(const csVector3 &pos,float rotangle, iSector* sector)
    {
        pcmesh->MoveMesh(sector , pos);
        csMatrix3 matrix = (csMatrix3) csYRotMatrix3(rotangle);
        pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T(matrix);
    }

    bool InitMesh(const char* factname, const char* filename)
    {
        pcmesh = new npcMesh(npcclient->GetObjectReg(), NULL, NULL);

        if(!pcmesh->SetMesh(factname, filename))
        {
            Error3("Could not set MapWalker mesh with factname=%s and filename=%s", factname, filename);
            return false;
        }

        iMeshWrapper* mesh = pcmesh->GetMesh();
        if(!mesh)
        {
            Error2("Could not create MapWalker because could not load %s file into mesh.",factname);
            return false;
        }
        return true;
    }

    bool InitLinmove()
    {
        pcmove =  new psLinearMovement(npcclient->GetObjectReg());
        pcmove->SetSpeed(1);
        return true;
    }

    bool InitCD()
    {
        csVector3 top, bottom, offset;

        const csBox3 &box = pcmesh->GetMesh()->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();

        float width  = box.MaxX() - box.MinX();
        height = box.MaxY() - box.MinY();
        float depth  = box.MaxZ() - box.MinZ();
        float legSize;

        if(height > 0.8f)
            legSize = 0.7F;
        else
            legSize = height * 0.5f;
        top = csVector3(width,height-legSize,depth);
        bottom = csVector3(width-.1,legSize,depth-.1);
        offset = csVector3(0,0,0);

        top.x *= .7f;
        top.z *= .7f;
        bottom.x *= .7f;
        bottom.z *= .7f;
        pcmove->InitCD(top, bottom,offset, pcmesh->GetMesh());
        return true;
    }

    float SetForwardSpeed(float speed)
    {
        pcmove->SetVelocity(csVector3(0, 0, -speed));
        ////                                ^ wtf?
        return 0;
    }

    npcMesh* pcmesh;
    psLinearMovement* pcmove;
    psNPCClient* npcclient;
    float height;
};

const float psMapWalker::DELTA = 0.1F;

/***********************************************************************************
*
*                            walk cache
*
************************************************************************************/

/*class psWalkRect
{
public:
    int x, z;
    csVector bl, tr;
    bool walkable;
    csTicks lastUsed;


};

const int WR_CACHE_SIZE = 1;
const float WR_SIZE = 0.2;

class psWalkRectCache
{
public:

    psWalkRectCache(psMapWalker * walker)
    {
        this->walker = walker;
        numRects = 0;
    }

    float CalcHorizInter(const ps2DLine & line, float z)
    {
        //division by zero if fine
        return -(c+b*z) / a;
    }

    float CalcVertInter(const ps2DLine & line, float x)
    {
        //division by zero if fine
        return -(c+a*x) / b;
    }

    void GoToNextRect(const ps2DLine & line, const csBox3 & goal, csVector3 & point, psWalkRect * & rect)
    {
        float inter;
        inter = CalcVertInter(line, rect->bl.x);
        if (inter >= bl.z  &&  inter <= tr.z)
        {
            point.x = bl.x;
            point.z = inter;
            rect = GetRect(x-1, y);
            return;
        }
        ............
    }

    bool LineIsWalkable(const csVector3 & start, const csVector3 & goal)
    {
        csVector3 currPoint;
        ps2DLine line;
        psWalkRect * currRect, * goalRect;

        line.Calc(start, goal);

        currPoint = start;
        currRect = GetRect(begin);
        goalRect = GetRect(goal);
        while (currRect!=goalRect  &&  currRect->walkable)
        {
            GoToNextRect(line, goal, currPoint, currRect);
        }
    }

    void GetRectBounds(const csVector3 & point, const csVector3 & bl)//, const csVector3 & tr)
    {
        int rectX, rectZ;
        rectX = int(point.x / WR_CACHE_SIZE);
        rectZ = int(point.z / WR_CACHE_SIZE);
        b1.x = rectX * WR_CACHE_SIZE;
        b1.z = rectZ * WR_CACHE_SIZE;
        //tr.x = bl.x + WR_CACHE_SIZE;
        //tr.z = bl.z + WR_CACHE_SIZE;
    }

    psWalkRect * GetRect(const csVector3 & point)
    {
        pokud najde, zkontroluje z -
            pokud je ok, prijme toto z za sve - jen pro ucely chozeni po ctvercich !
                realne z si zjisti opravdovym walkem ? co kdyz walk nevyjde

        rects.Get
        if ()
        {
            return ...
        }
        else
        {
            return AddRect(x, y, z);
        }
    }

    psWalkRect * AddRect(float x, float y, float z)
    {
        if (numRects == WR_CACHE_SIZE)
        {
            RemoveLRU();
        }

        numRects++;
    }

    void RemoveLRU()
    {
        psWalkRect* lru = NULL;

        csList<psWalkRect*>::Iterator it(rects);
        while (it.HasNext())
        {
            psWalkRect* rect = it.Next();
            if (lru==NULL  ||  lru->lastUsed>rect->lastUsed)
                lru = rect;
        }
        delete....rects, map
        numRects++;
    }

protected:
    psMapWalker * walker;
    csList<psWalkRect*> rects;
    csHash<psWalkRect*, csVector3, ......> map;
    int numRects;
};
*/
/***********************************************************************************
*
*                            walk poly map construction
*
************************************************************************************/

// clockwise
bool CheckConvexAngle(const csVector3 &a, const csVector3 &b, const csVector3 &c)
{
    ps2DLine line;
    line.Calc(a, b);
    return line.CalcRel(c) <= 0;
}



float pow2(float x)
{
    return x*x;
}

float CalcSlope(const csVector3 &from, const csVector3 &to)
{
    float dist = sqrt(pow2(from.x-to.x) + pow2(from.y-to.y));
    if(dist != 0)
        return (to.y - from.y) / dist;
    else
        return 0;
}


void pathFindTest_Walkability(psNPCClient* npcclient)
{
    psMapWalker* w;

    //pred kominem -50.7 0 -160
    //trochu dal od komina -47 0 -165
    //vlevo od kovarny -61 0 -152
    //za  -61 0 -147
    //vpravo -56 0 -147

    w = new psMapWalker(npcclient);
    assert(w->Init());

    iSector* sector;
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());
    //sector = engine->FindSector("NPCroom");
    sector = engine->FindSector("blue");
    assert(sector);

    //pres kovarnu
    //w->WalkBetween(csVector3(-61, 0, -152), csVector3(-56, 0, -147), sector);

    //tri doprava, tri dolu
    //CheckWalkability(w, csVector3(-50, 0, -160), sector, csVector3(-47, 0, -163), sector);
    //tri doprava
    //CheckWalkability(w, csVector3(-50, 0, -160), sector, csVector3(-47, 0, -160), sector);
    //tri doprava tri nahoru
    //CheckWalkability(w, csVector3(-50, 0, -160), sector, csVector3(-47, 0, -157), sector);
    //tri doprava jeden nahoru
    //CheckWalkability(w, csVector3(-50, 0, -160), sector, csVector3(-47, 0, -159), sector);
    //tri doleva jeden nahoru
    //CheckWalkability(w, csVector3(-47, 0, -160), sector, csVector3(-50, 0, -159), sector);
    //tri doleva jeden dolu
    //CheckWalkability(w, csVector3(-47, 0, -160), sector, csVector3(-50, 0, -161), sector);

    //w->WalkBetween(csVector3(-137.873886, -11.517871, -192.646194),
    //      csVector3(-137.175446, -12.517871, -193.368301), sector);

}

/******************************************************************
*
*                       psSeed
*
*******************************************************************/
psSeed::psSeed()
{
    edgeSeed = false;
    sector = NULL;
    quality = 0;
}
psSeed::psSeed(csVector3 pos, iSector* sector, float quality)
{
    edgeSeed = false;
    this->pos     = pos;
    this->sector  = sector;
    this->quality = quality;
}
psSeed::psSeed(csVector3 edgePos, csVector3 pos, iSector* sector, float quality)
{
    edgeSeed = true;
    this->edgePos = edgePos;
    this->pos     = pos;
    this->sector  = sector;
    this->quality = quality;
}
void psSeed::SaveToString(csString &str)
{
    str += csString().Format("<seed x='%.3f' y='%.3f' z='%.3f' ex='%.3f' ey='%.3f' ez='%.3f' sector='%s' quality='%f' edge='%i'/>",
                             pos.x, pos.y, pos.z, edgePos.x, edgePos.y, edgePos.z, sector->QueryObject()->GetName(), quality, int(edgeSeed));

}
void psSeed::LoadFromXML(iDocumentNode* node, iEngine* engine)
{
    pos.x = node->GetAttributeValueAsFloat("x");
    pos.y = node->GetAttributeValueAsFloat("y");
    pos.z = node->GetAttributeValueAsFloat("z");
    edgePos.x = node->GetAttributeValueAsFloat("ex");
    edgePos.y = node->GetAttributeValueAsFloat("ey");
    edgePos.z = node->GetAttributeValueAsFloat("ez");
    sector = engine->FindSector(node->GetAttributeValue("sector"));
    quality = node->GetAttributeValueAsFloat("quality");
    edgeSeed = node->GetAttributeValueAsInt("edge")? true : false;
}
bool psSeed::CheckValidity(psMapWalker* walker)
{
    if(PointIsNearWalls(*walker, pos, sector, 0.1F))
        return false;
    if(edgeSeed)
        return walker->WalkBetween(edgePos, pos, sector, false, pos.y);
    else
        return true;
}

class psSeedWalkCallback :  public iWalkCallback
{
public:
    virtual ~psSeedWalkCallback()
    {
    }

    psSeedWalkCallback(psSeedSet &seeds, const csVector3 &start, const csVector3 &end, bool left)
    {
        this->seeds = &seeds;
        this->start = start;
        this->end   = end;
        numSeeds = 0;

        ps2DLine line;
        line.Calc(start, end);
        delta.x = line.a;
        delta.y = 0;
        delta.z = line.b;
        if(!left)
            delta *= -1;
        delta *= SEED_EDGE_DIST;
        delta.y += 1;
        CPrintf(CON_DEBUG, "adding seeds:\n");
        CShift();
        ::Dump3("start", start);
        ::Dump3("end",   end);
        ::Dump3("delta", delta);
        CUnshift();
    }
    void PositionNotify(const csVector3 &pos, iSector* sector)
    {
        const float SEED_STEP = 0.4F;
        float quality;

        if(CalcDistance(start, pos) / SEED_STEP >= numSeeds)
        {
            float distToStart = Calc2DDistance(start, pos);
            float distToEnd   = Calc2DDistance(end,   pos);
            /*if (distToStart < distToEnd)
                quality = distToStart / distToEnd;
            else
                quality = distToEnd   / distToStart;*/
            quality = MIN(distToStart, distToEnd);
            seeds->AddSeed(psSeed(pos, pos + delta, sector, quality));
            //::Dump3("SEEDPOS", pos);
            //::Dump3("SEEDPOS-D", pos+delta);
            numSeeds++;
        }
    }

protected:
    psSeedSet* seeds;
    csVector3 start, end;
    csVector3 delta;
    int numSeeds;
};


/******************************************************************
*
*                       psSeedSet
*
*******************************************************************/

psSeedSet::psSeedSet(iObjectRegistry* objReg)
{
    this->objReg = objReg;
}

bool psSeedSet::LoadFromString(const csString &str)
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (objReg);

    csRef<iDocument> doc = ParseString(str);
    if(doc == NULL)
    {
        CPrintf(CON_DEBUG, "failed to parse seed set\n");
        CPrintf(CON_DEBUG, "%s\n",str.GetData());
        return false;
    }

    csRef<iDocumentNode> topNode = doc->GetRoot()->GetNode("seeds");
    if(topNode == NULL)
    {
        Error1("Missing <seeds> tag");
        return false;
    }
    csRef<iDocumentNodeIterator> it = topNode->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> seedNode = it->Next();
        psSeed seed;
        seed.LoadFromXML(seedNode, engine);
        AddSeed(seed);
    }
    return true;
}

bool psSeedSet::LoadFromFile(const csString &path)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (objReg);
    csRef<iDataBuffer> buff = vfs->ReadFile(path);
    if(buff == NULL)
    {
        Error2("Could not find file: %s", path.GetData());
        return false;
    }
    return LoadFromString(buff->GetData());
}


void psSeedSet::SaveToString(csString &str)
{
    str += "<seeds>\n";
    csList<psSeed>::Iterator it(seeds);
    while(it.HasNext())
    {
        psSeed seed = it.Next();
        seed.SaveToString(str);
    }
    str += "</seeds>\n";
}

bool psSeedSet::SaveToFile(const csString &path)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (objReg);
    csString str;
    SaveToString(str);
    return vfs->WriteFile(path, str.GetData(), str.Length());
}


void psSeedSet::AddSeed(const psSeed &seed)
{
    seeds.PushBack(seed);
}

psSeed psSeedSet::PickUpBestSeed()
{
    psSeed bestSeed;
    csList<psSeed>::Iterator bestIt;
    float bestQuality;

    assert(ListLength(seeds) > 0);

    bestQuality = -1;
    csList<psSeed>::Iterator it(seeds);
    while(it.HasNext())
    {
        psSeed &seed = it.Next();
        if(seed.quality > bestQuality)
        {
            bestSeed = seed;
            bestQuality = seed.quality;
            bestIt = it;
        }
    }

    seeds.Delete(bestIt);

    return bestSeed;
}






void psWalkPoly::AddSeeds(psMapWalker* walker, psSeedSet &seeds)
{
    csVector3 start, end, middle, currPoint;

    for(int edgeNum=0; edgeNum < (int)verts.GetSize(); edgeNum++)
    {
        start  = verts[edgeNum].pos;
        end    = verts[GetNeighbourIndex(edgeNum, +1)].pos;

        float realEndY;
        psSeedWalkCallback callback(seeds, start, end, true);
        walker->WalkBetween(start, end, sector, false, realEndY, &callback);
    }
}

int mod(int a, int b)
{
    return a - a/b*b;
}

int psWalkPoly::GetNeighbourIndex(int vertNum, int offset) const
{
    int x = vertNum + offset;
    //printf("GNI=%i,%i,%i,%i\n ",x,verts.GetSize(), x % verts.GetSize(), mod(x, verts.GetSize()));
    if(x >= 0)
        return mod(x,(int)verts.GetSize());
    else
        return (int)verts.GetSize() + mod(x, (int)verts.GetSize());
}

bool psWalkPoly::CheckConvexity(int vertNum)
{
    int prev      =  GetNeighbourIndex(vertNum, -1),
        prevPrev  =  GetNeighbourIndex(vertNum, -2),
        next      =  GetNeighbourIndex(vertNum, 1),
        nextNext  =  GetNeighbourIndex(vertNum, 2);

    /*printf("cc num=%i, %i %i %i %i %i \n", vertNum, prevPrev, prev, vertNum, next, nextNext);

    ::Dump("prevprev", verts[prevPrev].pos);
    ::Dump("prev", verts[prev].pos);
    ::Dump("vertNum", verts[vertNum].pos);
    ::Dump("next", verts[next].pos);
    ::Dump("next", verts[nextNext].pos);

    printf("# %i ", (int)CheckConvexAngle(verts[prevPrev].pos,  verts[prev].pos,     verts[vertNum].pos));
    printf("# %i ", (int)CheckConvexAngle(verts[prev].pos,      verts[vertNum].pos,  verts[next].pos));
    printf("# %i ", (int)CheckConvexAngle(verts[vertNum].pos,   verts[next].pos,     verts[nextNext].pos));*/

    return CheckConvexAngle(verts[prevPrev].pos,  verts[prev].pos,     verts[vertNum].pos)
           && CheckConvexAngle(verts[prev].pos,      verts[vertNum].pos,  verts[next].pos)
           && CheckConvexAngle(verts[vertNum].pos,   verts[next].pos,     verts[nextNext].pos);
}

void pathFindTest_PolyInter()
{
    psWalkPoly p1, p2;
    psWalkPolyMap map;
//    int i;

    /*POLY VS POLY:
    -------------*/
    /*p1.AddVert(csVector3(0, 0, 2));
    p1.AddVert(csVector3(2, 0, 2));
    p1.AddVert(csVector3(2, 0, 0));
    p1.AddVert(csVector3(0, 0, 0));*/

    //get right bottom
    /*p2.AddVert(csVector3(1, 0, 3));
    p2.AddVert(csVector3(3, 0, 3));
    p2.AddVert(csVector3(3, 0, 1));
    p2.AddVert(csVector3(1, 0, 1));
    p2.RecalcAllEdges();*/

    //get all
    /*p2.AddVert(csVector3(0, 0, 2));
    p2.AddVert(csVector3(2, 0, 2));
    p2.AddVert(csVector3(2, 0, 0));
    p2.AddVert(csVector3(0, 0, 0));
    p2.RecalcAllEdges();*/

    //get right
    /*p2.AddVert(csVector3(1, 0, 3));
    p2.AddVert(csVector3(3, 0, 3));
    p2.AddVert(csVector3(3, 0, 0));
    p2.AddVert(csVector3(1, 0, 0));
    p2.RecalcAllEdges();*/

    //touching
    /*p2.AddVert(csVector3(0, 0, 2));
    p2.AddVert(csVector3(0, 0, 4));
    p2.AddVert(csVector3(2, 0, 4));
    p2.AddVert(csVector3(2, 0, 2));
    p2.RecalcAllEdges();*/

    //get middle
    /*p2.AddVert(csVector3(0.5, 0, 0.5));
    p2.AddVert(csVector3(0.5, 0, 1.5));
    p2.AddVert(csVector3(1.5, 0, 1.5));
    p2.AddVert(csVector3(1.5, 0, 0.5));
    p2.RecalcAllEdges();*/


    //real example
    p1.AddVert(csVector3(-136.468338F, 0, -194.075409F));
    p1.AddVert(csVector3(-138.757248F, 0, -192.146194F));
    p1.AddVert(csVector3(-136.969894F, 0, -193.012268F));

    p2.AddVert(csVector3(-132.585785F, 0, -198.666855F));
    p2.AddVert(csVector3(-140.539810F, 0, -188.407745F));
    p2.AddVert(csVector3(-128.833527F, 0, -189.200638F));

    p1.Dump("p1");
    p2.Dump("p2");
    p1.Intersect(p2);
    p1.Dump("result");
    p1.Dump("result");

    /*
    POLY VS LINE:
    -------------
    p1.AddVert(csVector3(0, 0, 2));
    p1.AddVert(csVector3(2, 0, 2));
    p1.AddVert(csVector3(2, 0, 0));
    p1.AddVert(csVector3(0, 0, 0));
    p1.RecalcAllEdges();*/

    //cut left half
    //p1.Cut(csVector3(1, 0, 0), csVector3(1, 0, 2));
    //cut right half
    //p1.Cut(csVector3(1, 0, 2), csVector3(1, 0, 0));
    //get right bottom
    //p1.Cut(csVector3(1, 0, -0.4), csVector3(2.4, 0, 1));
    //cut right bottom
    //p1.Cut(csVector3(2.4, 0, 1), csVector3(1, 0, -0.4));
    //cut all
    //p1.Cut(csVector3(2.4, 0, 0), csVector3(2.4, 0, 2));
    //get all
    //p1.Cut(csVector3(2.4, 0, 2), csVector3(2.4, 0, 0));
    //right edge
    //p1.Cut(csVector3(2, 0, 0), csVector3(2, 0, 2));
    //cut bottom
    //p1.Cut(csVector3(3, 0, 1), csVector3(1, 0, 1));

    fflush(0);
}

void psWalkPoly::Cut(const csVector3 &cut1, const csVector3 &cut2)
{
    csVector3 firstInter, secondInter;
    int firstToDelete=0, lastToDelete=0;
    int numInter;
    ps2DLine cutLine;
    csVector3 inter;
    int vertNum;
    bool clockWise = false;
    const bool dbg = false;

    if(dbg)CPrintf(CON_DEBUG, "poly.Cut:\n");
    if(dbg)::Dump("cut1", cut1);
    if(dbg)::Dump("cut2", cut2);

    if(verts.GetSize() == 0)
        return;

    cutLine.Calc(cut1, cut2);

    numInter = 0;
    for(vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        int vertNum2 = GetNeighbourIndex(vertNum, 1);
        csVector3 &a = verts[vertNum].pos,
                   & b = verts[vertNum2].pos;

        if(dbg)CPrintf(CON_DEBUG, "try %i %i\n", vertNum, vertNum2);
        if(dbg)::Dump("a", a);
        if(dbg)::Dump("b", b);

        if(CalcInter(a, b, verts[vertNum].line,
                     cutLine, inter))
        {
            ::EstimateY(a, b, inter);

            csVector3 diff = b - a;
            float distAtoB = Calc2DDistance(a, b);
            float distAtoInter = Calc2DDistance(a, inter);
            inter.y = a.y + diff.y / distAtoB * distAtoInter;


            float relA, relB;
            relA = cutLine.CalcRel(a);
            relB = cutLine.CalcRel(b);

            if(dbg)CPrintf(CON_DEBUG, "inter: %f %f rel: %f %f\n", inter.x, inter.z, relA, relB);

            if(relA!=0  &&  relB!=0)
            {
                numInter++;
                if(numInter == 1)
                {
                    firstInter = inter;
                    if(relA < 0)
                    {
                        firstToDelete = vertNum2;
                        clockWise = true;
                    }
                    else
                    {
                        firstToDelete = vertNum;
                        clockWise = false;
                    }
                }
                else
                {
                    if(relA < 0)
                        lastToDelete = vertNum2;
                    else
                        lastToDelete = vertNum;
                    secondInter = inter;
                }
            }
        }
    }

    if(numInter < 2)
    {
        float rel=0;
        for(vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
        {
            rel = cutLine.CalcRel(verts[vertNum].pos);
            if(rel != 0) break;
        }

        if(vertNum<(int)verts.GetSize())
            if(dbg)CPrintf(CON_DEBUG, "vertNum=i\n", vertNum);

        if(rel > 0)
        {
            if(dbg)CPrintf(CON_DEBUG, "cut all\n");
            verts.SetSize(0);
        }
        else if(dbg)CPrintf(CON_DEBUG, "preserve all\n");
    }
    else
    {
        int end;
        vertNum = firstToDelete;
        if(clockWise)
            end = GetNeighbourIndex(lastToDelete, 1);
        else
            end = GetNeighbourIndex(lastToDelete, -1);
        if(dbg)CPrintf(CON_DEBUG, "cutting range %i %i end%i cw%i\n", firstToDelete, lastToDelete, end, int(clockWise));

        while(vertNum != end)
        {
            if(dbg)CPrintf(CON_DEBUG, "cutting num=%i end=%i\n", vertNum, end);
            verts.DeleteIndex(vertNum);
            if(firstToDelete > vertNum)
                firstToDelete--;
            if(lastToDelete > vertNum)
                lastToDelete--;
            if(end > vertNum)
                end--;
            if(clockWise)
                vertNum = GetNeighbourIndex(vertNum, 0);
            else
                vertNum = GetNeighbourIndex(vertNum, -1);
        }

        if(clockWise)
        {
            AddVert(secondInter, firstToDelete);
            AddVert(firstInter, firstToDelete);
        }
        else
        {
            AddVert(firstInter, firstToDelete);
            AddVert(secondInter, firstToDelete);
        }
        if(dbg)CPrintf(CON_DEBUG, "poly was cut");
    }
}

void psWalkPoly::ClearInfo()
{
    for(size_t vertNum = 0; vertNum < verts.GetSize(); vertNum++)
        verts[vertNum].info.Clear();
}

void psWalkPoly::Dump(const char* name)
{
    CPrintf(CON_DEBUG, "dumping poly '%s':\n", name);
    for(int vertNum = 0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        ::Dump("", verts[vertNum].pos);
        if(vertNum < (int)verts.GetSize()-1) CPrintf(CON_DEBUG, ",");
        //printf("info: %s\n", verts[vertNum].info.GetData());
        //verts[vertNum].line.Dump();
    }
}

void psWalkPoly::DumpJS(const char* name)
{
    CPrintf(CON_DEBUG, "dumping poly '%s':\n", name);
    CShift();
    DumpPureJS();
    CUnshift();
}

void psWalkPoly::DumpPolyJS(const char* name)
{
    CPrintf(CON_DEBUG, "//poly '%s':\n", name);
    CShift();
    DumpPureJS();
    CPrintf(CON_DEBUG, "\n");
    CUnshift();
}

void psWalkPoly::DumpPureJS()
{
    /*CPrintf(CON_DEBUG, "[");
    for (int vertNum = 0; vertNum < verts.GetSize(); vertNum++)
    {
        CPrintf(CON_DEBUG, "{x: %f, z: %f}", verts[vertNum].pos.x, verts[vertNum].pos.z);
        if (vertNum < verts.GetSize()-1)
            CPrintf(CON_DEBUG, ",\n");
        else
            CPrintf(CON_DEBUG, "]\n");
    }*/
    csString s;
    SaveToString(s);
    CPrintf(CON_DEBUG, "%s\n", s.GetData());
}

void psWalkPoly::Intersect(const psWalkPoly &other)
{
    int vertNum = 0;
    while(vertNum<(int)other.verts.GetSize()  &&  verts.GetSize()>0)
    {
        Cut(other.verts[vertNum].pos, other.verts[other.GetNeighbourIndex(vertNum, +1)].pos);
        vertNum++;
    }
}

csVector3 psWalkPoly::GetClosestCollisionPoint(csList<psWalkPoly*> &collisions, ps2DLine &line)
{
    csVector3 closestVert;
    float closestDist;
    psWalkPoly* collision;


    closestDist = 1000;
    csList<psWalkPoly*>::Iterator it(collisions);
    while(it.HasNext())
    {
        collision = it.Next();
        for(size_t vertNum = 0; vertNum < collision->verts.GetSize(); vertNum++)
        {
            float dist = line.CalcDist(collision->verts[vertNum].pos);
            if(dist < closestDist)
            {
                closestDist = dist;
                closestVert = collision->verts[vertNum].pos;
            }
        }
    }
    return closestVert;
}


void psWalkPoly::MovePointInside(csVector3 &point, const csVector3 &goal)
{
//    float angle;
    csVector3 first, second;
    ps2DLine goalLine;
    float rel;
    int vertNum;
    int numFound = 0;

    goalLine.Calc(point, goal);

    for(vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        csVector3 inter;
        if(CalcInter(verts[vertNum].pos, verts[GetNeighbourIndex(vertNum, +1)].pos,
                     verts[vertNum].line, goalLine, inter))
        {
            if(numFound == 0)
            {
                numFound++;
                first = inter;
            }
            else
            {
                numFound++;
                second = inter;
                break;
            }
        }
    }

    if(numFound != 2)
    {
        CPrintf(CON_DEBUG, "MovePointInside: problem: numFound=%i\n", numFound);
        return;
    }

    ::Dump3("first", first);
    ::Dump3("second", second);

    ps2DLine perp;
    csVector3 diff = second-first;
    csVector3 u = diff.Unit();

    ::Dump3("diff", diff);

    perp.CalcPerp(first, second);
    rel = perp.CalcRel(point);

    CPrintf(CON_DEBUG, "rel=%f mag=%f Frel=%f Srel=%f 2dd=%f\n", rel, diff.Norm(), perp.CalcRel(first), perp.CalcRel(second), Calc2DDistance(first, second));

    if(rel > -0.001)
    {
        point = second - diff * 0.01;
    }
    else if(rel < -Calc2DDistance(first, second) + 0.001)
    {
        point = first + diff * 0.99;
    }

    /*for (vertNum=0; vertNum < verts.GetSize(); vertNum++)
        if (verts[vertNum].line.CalcRel(point) > 0)
        {
            if (CalcInter(verts[vertNum].pos, verts[GetNeighbourIndex(vertNum, +1)].pos,
                      verts[vertNum].line, goalLine, onBorder))
                break;
        }

    if (vertNum < verts.GetSize())
    {
        onBorder.y = point.y;
        //::Dump3("MPI inter", onBorder);
    }
    else
    {
        onBorder = GetClosestPoint(point, angle);
        onBorder.y = point.y;
        //::Dump3("MPI closest", onBorder);
    }

    // neni zaruceno ze bude opravdu uvnitr, muze prestrelit
    // najdeme 2 pruseciky a dame mezi

    //::Dump3("MPI adjust", (onBorder-point).Unit() * 0.01);

    if (onBorder != point)
        point = onBorder + (onBorder-point).Unit() * 0.01;*/
}

bool psWalkPoly::CollidesWithPolys(psWalkPolyMap &map, csList<psWalkPoly*>* polys)
{
    psWalkPoly collision;
    const bool dbg = false;

    csList<psWalkPoly*>::Iterator it(*polys);
    while(it.HasNext())
    {
        psWalkPoly* poly = it.Next();
        if(poly != this)
        {
            collision = *this;
            if(dbg)collision.DumpJS("current poly");
            if(dbg)poly->DumpJS("checked for collision");

            //!!!!
            collision.RecalcAllEdges();
            poly->RecalcAllEdges();

            collision.Intersect(*poly);
            if(collision.verts.GetSize()>0
                    &&  fabs((float)(collision.EstimateY(collision.verts[0].pos)
                                     - poly->EstimateY(collision.verts[0].pos) <= MAX_COLL_Y)))
            {
                if(dbg)collision.DumpJS("collides with some poly");
                //this->DumpJS("our poly is");
                //poly->DumpJS("colliding with");
                return true;
            }
        }
    }
    return false;
}

csVector3 collVertCmpPos;
int collVertCmp(const void* voidA, const void* voidB)
{
    csVector3* a = (csVector3*)voidA;
    csVector3* b = (csVector3*)voidB;
    float distA = Calc2DDistance(*a, collVertCmpPos);
    float distB = Calc2DDistance(*b, collVertCmpPos);

    if(distA < distB)
        return -1;
    else if(distA == distB)
        return 0;
    else
        return 1;
}

void SortInterVerts(const csVector3 &oldPos, csArray<csVector3> &collVerts)
{
    size_t vertNum;
    csVector3* verts;

    verts = new csVector3[collVerts.GetSize()];
    for(vertNum = 0; vertNum < collVerts.GetSize(); vertNum++)
        verts[vertNum] = collVerts[vertNum];

    collVertCmpPos = oldPos;
    qsort(verts, collVerts.GetSize(), sizeof(verts[0]), collVertCmp);

    for(vertNum = 0; vertNum < collVerts.GetSize(); vertNum++)
        collVerts[vertNum] = verts[vertNum];
    delete [] verts;
}

float NormalizeAngle(float a)
{
    //printf("fm %f=%f\n", a, fmod(a, TWO_PI));
    a = fmod(a, TWO_PI);
    if(a < 0)
        a = TWO_PI + a;
    return a;
}

float AngleDiff(float a1, float a2)
{
    float na1 = NormalizeAngle(a1),
          na2 = NormalizeAngle(a2);
    float diff = fabs(na1 - na2);
    //printf("%f %f, %f %f %f\n", a1, a2, na1, na2, diff);
    if(diff > PI)
        return TWO_PI - diff;
    else
        return diff;
}

bool psWalkPoly::HandlePolyCollisions(psWalkPolyMap &map, int vertNum, const csVector3 &oldPos)
{
    psWalkPoly collision;
    csVector3 &vert = verts[vertNum].pos;
    csVector3 proposedPos = vert;
    csArray<csVector3> collVerts;
    csArray<float> collAngles;
    size_t collVertNum;
    const bool dbg = true;

    if(dbg)CPrintf(CON_DEBUG, "HandlePolyCollisions num=%i \n", vertNum);
    if(dbg)CShift();

    psSpatialIndexNode* node = map.index.FindNodeOfPoint(vert);
    if(!node)
        return true;

    csList<psWalkPoly*>* polys = node->GetPolys();
    csList<psWalkPoly*>::Iterator it(*polys);
    while(it.HasNext())
    {
        psWalkPoly* poly = it.Next();
        if(poly != this)
        {
            collision = *this;
            //if(dbg)collision.DumpPolyJS("our poly");
            //if(dbg)poly->DumpPolyJS("checking against");

            //!!!!
            collision.RecalcAllEdges();
            poly->RecalcAllEdges();

            collision.Intersect(*poly);
            if(collision.verts.GetSize() > 0
                    && fabs(collision.EstimateY(collision.verts[0].pos)
                            - poly->EstimateY(collision.verts[0].pos)) <= MAX_COLL_Y)
            {
                if(dbg)CPrintf(CON_DEBUG, "collides with %i\n", poly->id);
                //if(dbg)collision.DumpPolyJS("their intersection is");
                //this->DumpJS("our poly is");
                //poly->DumpJS("colliding with");



                csVector3 closest;
                float angle;
                closest = poly->GetClosestPoint(oldPos, angle);
                collVerts.Push(closest);
                collAngles.Push(angle);
                //for (collVertNum = 0; collVertNum < collision.verts.GetSize(); collVertNum++)
                //  collVerts.Push(collision.verts[collVertNum].pos);
            }
            else if(dbg)CPrintf(CON_DEBUG, "no intersection with %i\n", poly->id);
        }
    }

    if(collVerts.GetSize() == 0)
    {
        if(dbg)CUnshift();
        return true;
    }

    SortInterVerts(proposedPos, collVerts);
    collVertNum = 0;
    bool found = false;
    while(collVertNum<collVerts.GetSize()  &&  !found)
    {
        csVector3 closest = collVerts[collVertNum];

        if(dbg)CPrintf(CON_DEBUG, "trying a collVert:\n");
        if(dbg)::Dump("collVert", closest);

        csVector3 prev = verts[GetNeighbourIndex(vertNum, -1)].pos;
        csVector3 next = verts[GetNeighbourIndex(vertNum, +1)].pos;
        ps2DLine line;
        line.Calc(prev, next);

        vert = closest;
        vert.x -= line.a * POLYCOLL_ADJUST;
        vert.z -= line.b * POLYCOLL_ADJUST;
        if(dbg)::Dump("after adjust", vert);

        RecalcEdges(vertNum);
        found = ! CollidesWithPolys(map, polys);
        if(found)
        {
            float edgeAngle = collAngles[collVertNum];
            csVector3 a, b;

            MoveInDirection(edgeAngle,    1, closest, a);
            MoveInDirection(edgeAngle+PI, 1, closest, b);
            //::Dump3("a", a);            ::Dump3("b", b);

            float newAngle;
            if(line.CalcRel(a) > line.CalcRel(b))
                newAngle = edgeAngle;
            else
                newAngle = NormalizeAngle(edgeAngle + PI);

            //CPrintf(CON_DEBUG, "a=%f n=%f %f %f\n", edgeAngle, newAngle, line.CalcRel(a), line.CalcRel(b));

            if(!verts[vertNum].followingEdge)
            {
                if(dbg)CPrintf(CON_DEBUG, "angle %f--->%f\n", verts[vertNum].angle, newAngle);
                verts[vertNum].angle = newAngle;
                verts[vertNum].followingEdge = true;
            }
            else if(dbg)CPrintf(CON_DEBUG, "already following another\n");
            ;//????
        }
        if(dbg)CPrintf(CON_DEBUG, "collVert validity: %i\n", int(found));

        if(!found)
            collVertNum++;
    }

    verts[vertNum].touching = true;

    if(dbg)CUnshift();
    return found;
}

void MoveNearlyTo(const csVector3 &goal, csVector3 &point)
{
    csVector3 diff = goal - point;
    point += diff * 0.9;
}

float psWalkPoly::GetNarrowness()
{
    float min, max;
    GetShape(min, max);
    return max / min;
}

void psWalkPoly::GetShape(float &min, float &max)
{
    csVector3 polyMean(0,0,0);
    int vertNum;

    for(vertNum=0; vertNum<(int)verts.GetSize(); vertNum++)
        polyMean += verts[vertNum].pos;
    polyMean /= verts.GetSize();

    for(vertNum=0; vertNum<(int)verts.GetSize(); vertNum++)
    {
        csVector3 edgeMean;
        edgeMean = (verts[vertNum].pos + verts[GetNeighbourIndex(vertNum, +1)].pos) / 2;

        float dist = Calc2DDistance(edgeMean, polyMean);
        if(vertNum == 0)
        {
            min = max = dist;
        }
        else
        {
            min = MIN(min, dist);
            max = MAX(max, dist);
        }
    }
}

bool psWalkPoly::TouchesPoly(psWalkPoly* poly)
{
    for(size_t vertNum=0; vertNum<verts.GetSize(); vertNum++)
    {
        csArray<psWalkPolyCont> &conts = verts[vertNum].conts;
        for(size_t contNum=0; contNum < conts.GetSize(); contNum++)
            if(conts[contNum].poly == poly)
                return true;
    }
    return false;
}

bool psWalkPoly::NeighboursTouchEachOther()
{
    csArray<psWalkPoly*> neighbours;
    size_t neighNum1, neighNum2;

    for(size_t vertNum=0; vertNum<verts.GetSize(); vertNum++)
    {
        csArray<psWalkPolyCont> &conts = verts[vertNum].conts;
        for(size_t contNum=0; contNum < conts.GetSize(); contNum++)
            neighbours.Push(conts[contNum].poly);
    }

    for(neighNum1=0; neighNum1 < neighbours.GetSize(); neighNum1++)
        for(neighNum2=neighNum1+1; neighNum2 < neighbours.GetSize(); neighNum2++)
            if(!neighbours[neighNum1]->TouchesPoly(neighbours[neighNum2]))
                return false;
    return true;
}

void MoveToABit(const csVector3 &goal, csVector3 &point, float bit)
{
    point += (goal-point) * bit;
}

const float MAX_NARROWNESS = 4;


csVector3 MoveTo(const csVector3 &pos, const csVector3 &goal, float len)
{
    csVector3 a;

    a = pos + (goal-pos).Unit() * len;
    if(CalcDistance(pos, goal) > CalcDistance(pos, a))
        return a;
    else
        return goal;
}

// returns true if it moved at least a bit
bool psWalkPoly::HandleProblems(psMapWalker &walker, psWalkPolyMap &map, int vertNum, const csVector3 &oldPos)
{
    const bool dbg = true;
    bool res;
    psStopWatch watch;

    if(dbg)CPrintf(CON_DEBUG, "HandleProblems vertnum=%i\n", vertNum);
    if(dbg)CShift();

    if(dbg)DumpPolyJS("poly before handling");

    //verts[vertNum].touching = false;

    if(!CheckConvexity(vertNum))
    {
        if(dbg)CPrintf(CON_DEBUG, "fixing not convex\n");
        if(dbg)::Dump("old", verts[vertNum].pos);

        int prev, next;
        ps2DLine goalLine;
        csVector3 inter;

        prev = GetNeighbourIndex(vertNum, -2);
        if(verts[prev].line.CalcRel(verts[vertNum].pos) > 0)
        {
            goalLine.Calc(oldPos, verts[vertNum].pos);
            CalcInter(verts[prev].line, goalLine, inter);

            verts[vertNum].pos.x = inter.x;
            verts[vertNum].pos.z = inter.z;
            MoveToABit(oldPos, verts[vertNum].pos, 0.1F);
        }

        next = GetNeighbourIndex(vertNum, +1);
        if(verts[next].line.CalcRel(verts[vertNum].pos) > 0)
        {
            goalLine.Calc(oldPos, verts[vertNum].pos);
            CalcInter(verts[next].line, goalLine, inter);

            verts[vertNum].pos.x = inter.x;
            verts[vertNum].pos.z = inter.z;
            MoveToABit(oldPos, verts[vertNum].pos, 0.1F);
        }

        RecalcEdges(vertNum);
        if(dbg)::Dump("fixed", verts[vertNum].pos);
    }

    // too small causes fragmentation
    // narrowness checks probably just cause harm
    /*if (GetNarrowness() > MAX_NARROWNESS)
    {
        if(dbg)CPrintf(CON_DEBUG, "too narrow\n");
        verts[vertNum].info += "narrow ";
        if(dbg)CUnshift();
        return false;
    }*/

    // convexity must be checked before HandlePolyCollisions, because it requires
    // convex polygons to work correctly

    watch.Start();
    res = CheckConvexity(vertNum);
    prof.AddCons("checkconvex", watch.Stop());
    if(!res)
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: not convex\n");
        verts[vertNum].info += "notconvex ";
        if(dbg)CUnshift();
        return false;
    }

    watch.Start();
    res = HandlePolyCollisions(map, vertNum, oldPos);
    prof.AddCons("handlepolycollisions", watch.Stop());
    if(!res)
    {
        verts[vertNum].info += "polycoll ";
        if(dbg)CPrintf(CON_DEBUG, "problem: polycoll\n");
        if(dbg)CUnshift();
        return false;
    }

    if(dbg)DumpPolyJS("poly after polycol");

    // convexity must be checked after HandlePolyCollisions too, because it could have changed
    // the polygon
    watch.Start();
    res = CheckConvexity(vertNum);
    prof.AddCons("checkconvex", watch.Stop());
    if(!res)
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: not convex\n");
        verts[vertNum].info += "notconvex ";
        if(dbg)CUnshift();
        return false;
    }

    watch.Start();
    csVector3 prev, next;
    csVector3 &curr = verts[vertNum].pos;

    if(PointIsNearWalls(walker, curr, sector, 0.05F))
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: point near walls\n");
        verts[vertNum].info += "nearwalls ";
        if(dbg)CUnshift();
        prof.AddCons("walk", watch.Stop());
        verts[vertNum].touching = true;
        return false;
    }

    prev = verts[GetNeighbourIndex(vertNum, -1)].pos;
    next = verts[GetNeighbourIndex(vertNum, +1)].pos;
    float tmp;

    //MoveTo(oldPos, prev, 2)
    if(!walker.WalkBetween(curr, prev, sector, true, tmp))
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: can't walk to prev\n");
        verts[vertNum].info += "walkprev ";
        if(dbg)CUnshift();
        prof.AddCons("walk", watch.Stop());
        verts[vertNum].touching = true;
        return false;
    }
    if(!walker.WalkBetween(curr, next, sector, true, tmp))
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: can't walk to next\n");
        verts[vertNum].info += "walkprev ";
        if(dbg)CUnshift();
        prof.AddCons("walk", watch.Stop());
        verts[vertNum].touching = true;
        return false;
    }

    if(!walker.WalkBetween(prev, curr, sector, true, curr.y))
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: can't walk from prev\n");
        verts[vertNum].info += "walkprev ";
        if(dbg)CUnshift();
        prof.AddCons("walk", watch.Stop());
        verts[vertNum].touching = true;
        return false;
    }
    if(!walker.WalkBetween(next, curr, sector, true, curr.y))
    {
        if(dbg)CPrintf(CON_DEBUG, "problem: can't walk from next\n");
        verts[vertNum].info += "walknext ";
        if(dbg)CUnshift();
        prof.AddCons("walk", watch.Stop());
        verts[vertNum].touching = true;
        return false;
    }
    prof.AddCons("walk", watch.Stop());

    verts[vertNum].info += "noprob ";
    if(dbg)CPrintf(CON_DEBUG, "problem: noprob\n");
    if(dbg)CUnshift();
    return true;
}



float CalcLengthOfEdges(const csVector3 &a, const csVector3 &b, const csVector3 &c)
{
    return Calc2DDistance(a, b) + Calc2DDistance(b, c);
}
float CalcTriangleArea(const csVector3 &a, const csVector3 &b, const csVector3 &c)
{
    ps2DLine base;
    base.Calc(a, b);
    float area = Calc2DDistance(a,b) * base.CalcDist(c) / 2;

    /*CPrintf(CON_DEBUG, "CalcTriangleArea res=%f\n", area);
    CShift();
    ::Dump("a", a);
    ::Dump("b", b);
    ::Dump("c", c);
    CUnshift();*/
    return area;
}

void MoveInDirection(float angle, float length, const csVector3 &oldPos, csVector3 &newPos)
{
    newPos.x = oldPos.x - sinf(angle) * length;
    newPos.y = oldPos.y;
    newPos.z = oldPos.z - cosf(angle) * length;
}

bool psWalkPoly::StretchVert(psMapWalker &walker, psWalkPolyMap &map, int vertNum, const csVector3 &newPos, float stepSize)
{

    csVector3 oldPos;
    int prevVertNum, nextVertNum;
    csVector3 prevVert, nextVert;
    float oldTriangleArea;
    psStopWatch watch;
    const bool dbg = true;

    if(verts.GetSize() < 3)
        return false;

    watch.Start();

    if(dbg)CPrintf(CON_DEBUG, "StretchVert num=%i angle=%f\n", vertNum, verts[vertNum].angle);
    if(dbg)CShift();

    if(dbg)DumpPolyJS("poly before movement");

    prevVertNum = GetNeighbourIndex(vertNum, -1);
    prevVert = verts[prevVertNum].pos;
    nextVertNum = GetNeighbourIndex(vertNum, +1);
    nextVert = verts[nextVertNum].pos;

    oldTriangleArea = CalcTriangleArea(prevVert, verts[vertNum].pos, nextVert);
    oldPos = verts[vertNum].pos;

    if(dbg)::Dump("dir oldPos", oldPos);

    csVector3 prevPrev = verts[GetNeighbourIndex(vertNum, -2)].pos;
    csVector3 nextNext = verts[GetNeighbourIndex(vertNum, +2)].pos;
    //newPos = oldPos + ((prevVert-prevPrev) + (nextVert-nextNext)).Unit() * stepSize;
    //newPos = oldPos + ((oldPos-prevVert).Unit() + (oldPos-nextVert).Unit()).Unit() * stepSize;

    //if(dbg)CPrintf(CON_DEBUG, "trying dir angle=%f\n", dirs[dirNum].angle);
    if(dbg)::Dump("dir newPos", newPos);
    verts[vertNum].pos = newPos;
    RecalcEdges(vertNum);
    if(HandleProblems(walker, map, vertNum, oldPos))
    {
        float newTriangle = CalcTriangleArea(prevVert, verts[vertNum].pos, nextVert);
        if(newTriangle > oldTriangleArea)
        {
            CalcBox();
            if(dbg)CPrintf(CON_DEBUG, "good direction found, area=%f\n", GetArea());
            if(dbg)CUnshift();
            prof.AddCons("StretchVert - ok", watch.Stop());
            return true;
        }
        else if(dbg)CPrintf(CON_DEBUG, "direction would decrease area: %f ---> %f\n", oldTriangleArea, newTriangle);
    }

    // no good direction found ...
    if(dbg)CPrintf(CON_DEBUG, "NO good direction found\n");
    verts[vertNum].pos = oldPos;
    verts[vertNum].stuck = true;
    RecalcEdges(vertNum);

    if(dbg)CUnshift();
    prof.AddCons("StretchVert - fail", watch.Stop());
    return false;
}

void psWalkPoly::GlueToNeighbours(psMapWalker &walker, psWalkPolyMap &map)
{
    csVector3 vertPos;
    bool first;

    CPrintf(CON_DEBUG, "GlueToNeighbours id=%i\n", id);
    CShift();

    for(int vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        vertPos = verts[vertNum].pos;
        psSpatialIndexNode* node = map.index.FindNodeOfPoint(vertPos);
        if(node != NULL)
        {
            csList<psWalkPoly*>* polys = node->GetPolys();
            csList<psWalkPoly*>::Iterator it(*polys);
            csVector3 closestSoFar;

            first = true;
            while(it.HasNext())
            {
                psWalkPoly* poly = it.Next();
                if(poly != this)
                {
                    float angle;
                    csVector3 closestOfPoly = poly->GetClosestPoint(vertPos, angle);
                    if(first  ||  Calc2DDistance(vertPos, closestOfPoly) < Calc2DDistance(vertPos, closestSoFar))
                        closestSoFar = closestOfPoly;
                    first = false;
                }
            }

            if(!first  &&  Calc2DDistance(vertPos, closestSoFar) < 0.5)
            {
                verts[vertNum].pos = closestSoFar - (closestSoFar - vertPos).Unit() * POLYCOLL_ADJUST;
                RecalcEdges(vertNum);

                CPrintf(CON_DEBUG, "trying to glue vertnum=%i\n", vertNum);
                ::Dump3("closest", closestSoFar);
                ::Dump3("new position", verts[vertNum].pos);

                if(HandleProblems(walker, map, vertNum, vertPos))
                    CPrintf(CON_DEBUG, "success\n");
                else
                {
                    verts[vertNum] = vertPos;   //rollback
                    RecalcEdges(vertNum);
                    CPrintf(CON_DEBUG, "failed\n");
                }
            }
        }
    }
    CUnshift();
}

csVector3 psWalkPoly::GetClosestPoint(const csVector3 &point, float &angle)
{
    csVector3 closest, candidate;
    int closestVertNum = 0;
    ps2DLine line, perp, pointLine;

    pointLine.a = pointLine.b = pointLine.c = 0;

    //CPrintf(CON_DEBUG, "GetClosestPoint\n");
    //::Dump3("point", point);
    for(int vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        //CPrintf(CON_DEBUG, "closest: vertnum=%i\n", vertNum);
        int next = GetNeighbourIndex(vertNum, +1);
        csVector3 &A = verts[vertNum].pos,
                   & B = verts[next].pos;

        //::Dump3("A", A);
        //::Dump3("B", B);

        perp.CalcPerp(A, B);
        if(perp.CalcRel(point) >= 0)
        {
            //CPrintf(CON_DEBUG, "B\n");
            candidate = B;
            goto comparison;    // bite me
        }

        perp.CalcPerp(B, A);
        if(perp.CalcRel(point) >= 0)
        {
            //CPrintf(CON_DEBUG, "A\n");
            candidate = A;
            goto comparison;
        }

        //CPrintf(CON_DEBUG, "edge\n");
        line.Calc(B, A);
        pointLine.Calc(point, csVector3(point.x + pointLine.a, point.y, point.z + pointLine.b));
        //::Dump3("second", csVector3(point.x + pointLine.a, point.y, point.z + pointLine.b));
        //line.Dump();
        //pointLine.Dump();
        CalcInter(line, pointLine, candidate);

comparison:
        //::Dump3("candidate", candidate);
        if(vertNum==0  ||  Calc2DDistance(point, candidate) < Calc2DDistance(point, closest))
        {
            closest = candidate;
            closestVertNum = vertNum;
        }
    }
    angle = CalcAngle(verts[closestVertNum].pos,
                      verts[GetNeighbourIndex(closestVertNum, +1)].pos);
    return closest;
}

float psWalkPoly::EstimateY(const csVector3 &point)
{
    csArray<float> dists, coeffs;
    size_t vertNum;
    float coeffSume=0;
    //float maxDist=0;
    float y=0;

    //I am sure there is better way how to calculate this


    dists.SetSize(verts.GetSize());
    coeffs.SetSize(verts.GetSize());

    for(vertNum=0; vertNum < verts.GetSize(); vertNum++)
    {
        dists[vertNum] = Calc2DDistance(verts[vertNum].pos, point);
        //maxDist = MAX(maxDist, dists[vertNum]);
    }

    for(vertNum=0; vertNum < verts.GetSize(); vertNum++)
    {
        if(dists[vertNum] == 0)
            return verts[vertNum].pos.y;
        coeffs[vertNum] = 1 / dists[vertNum];
        coeffSume += coeffs[vertNum];
    }

    for(vertNum=0; vertNum < verts.GetSize(); vertNum++)
        y += verts[vertNum].pos.y * coeffs[vertNum] / coeffSume;

    return y;
}

void psWalkPoly::RecalcAllEdges()
{
    for(int vertNum = 0; vertNum < (int)verts.GetSize(); vertNum++)
        RecalcEdges(vertNum);
}

void psWalkPoly::RecalcEdges(int vertNum)
{
    int prev, next;

    assert(verts.GetSize() >= 2);

    prev = GetNeighbourIndex(vertNum, -1);
    next = GetNeighbourIndex(vertNum, +1);

    verts[vertNum].CalcLineTo(verts[next].pos);
    verts[prev].CalcLineTo(verts[vertNum].pos);
}

psWalkPoly::psWalkPoly()
{
    id = nextID ++;
    sector = NULL;
    supl = false;
}

psWalkPoly::~psWalkPoly()
{
    for(size_t nodeNum=0; nodeNum < nodes.GetSize(); nodeNum++)
    {
        psANode* node = nodes[nodeNum];
        if(node->poly1 == this)
            node->poly1 = NULL;
        if(node->poly2 == this)
            node->poly2 = NULL;
    }
}

void psWalkPoly::DeleteContsWith(psWalkPoly* poly)
{
    for(size_t vertNum=0; vertNum<verts.GetSize(); vertNum++)
    {
        csArray<psWalkPolyCont> &conts = verts[vertNum].conts;
        size_t contNum=0;
        while(contNum < conts.GetSize())
            if(conts[contNum].poly == poly)
            {
                //CPrintf(CON_DEBUG, "deleting contact with %i from %i\n", poly->id, id);
                conts.DeleteIndex(contNum);
            }
            else
                contNum++;
    }
}

void psWalkPoly::SetSector(iSector* sector)
{
    this->sector = sector;
}

///TODO: Use csRandomGen for this
float rndf(float min, float max)
{
    return (float(rand()) / RAND_MAX) * (max-min) + min;
}

///TODO: Use csRandomGen for this
int rnd(int min, int max)
{
    return (int)rndf(min, max+1);
}

csVector3 Unit2D(const csVector3 &begin, const csVector3 &end)
{
    return (end-begin) / Calc2DDistance(begin, end);
}

int psWalkPoly::Stretch(psMapWalker &walker, psWalkPolyMap &map, float stepSize)
{
    int numMovedThisStep=0;
    int begin = csGetTicks();
    csVector3 newPos;

    CPrintf(CON_DEBUG, "stretch id=%i time=%i stepsize=%f\n", id, csGetTicks(), stepSize);
    ClearInfo();

    for(int vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
    {
        MoveInDirection(verts[vertNum].angle, stepSize, verts[vertNum].pos, newPos);
        if(StretchVert(walker, map, vertNum, newPos, stepSize))
            numMovedThisStep++;
    }


    //CPrintf(CON_DEBUG, "//poly after a Stretch\n");
    //DumpPolyJS();
    CPrintf(CON_DEBUG, "area after Stretch=%f numverts=%i time=%i\n", GetArea(), verts.GetSize(), csGetTicks()-begin);
    return numMovedThisStep;
}

void psWalkPoly::SetMini(psMapWalker &walker, const csVector3 &pos, int numVerts)
{
    float angleStep = TWO_PI / numVerts;
    float angle = 0;
    csVector3 vertPos;

    for(int vertNum=0; vertNum < numVerts; vertNum++)
    {
        MoveInDirection(angle, SEED_SIZE/2, pos, vertPos);

        // calculate the real y of new vertex
        //if (walker.WalkBetween(pos, vertPos, sector, false, vertPos.y))
        //  AddVert(vertPos);

        vertPos.y += 0.1F;
        AddVert(vertPos);

        angle += angleStep;
    }

    CalcBox();
}

bool psWalkPoly::IsInLine()
{
    return false;

    /*if (verts.GetSize() < 2)
        return false;

    ps2DLine & line = verts[0].line;
    for (int vertNum = 2; vertNum < verts.GetSize(); vertNum++)
        if (line.CalcDist(verts[vertNum].pos) > 0.1)
            return false;*/
    //for (int vertNum = 2; vertNum < verts.GetSize(); vertNum++)
    return true;
}

psWalkPoly* psWalkPoly::GetNearNeighbour(const csVector3 &pos, float maxDist)
{
    csVector3 closest;
    float angle;    //unused

    for(size_t vertNum=0; vertNum < verts.GetSize(); vertNum++)
    {
        csArray<psWalkPolyCont> &conts = verts[vertNum].conts;
        for(size_t contNum=0; contNum < conts.GetSize(); contNum++)
        {
            closest = conts[contNum].poly->GetClosestPoint(pos, angle);
            if(Calc2DDistance(pos, closest) <= maxDist)
                return conts[contNum].poly;
        }
    }
    return NULL;
}

bool psWalkPoly::NeighbourSupsMe(psWalkPoly* neighbour)
{
    csVector3 vert, closest;
    float angle;    //unused

    for(size_t vertNum=0; vertNum < verts.GetSize(); vertNum++)
    {
        vert = verts[vertNum].pos;
        closest = neighbour->GetClosestPoint(vert, angle);
        if(Calc2DDistance(vert, closest) > 1)
            return false;
    }
    return true;
}

bool psWalkPoly::ReachableFromNeighbours()
{
    csList<psWalkPoly*> neighbours;
    size_t vertNum;

    for(vertNum=0; vertNum < verts.GetSize(); vertNum++)
    {
        csArray<psWalkPolyCont> &conts = verts[vertNum].conts;
        for(size_t contNum=0; contNum < conts.GetSize(); contNum++)
            neighbours.PushBack(conts[contNum].poly);
    }

    csList<psWalkPoly*>::Iterator it(neighbours);
    while(it.HasNext())
    {
        psWalkPoly* neighbour = it.Next();
        if(NeighbourSupsMe(neighbour))
            return true;
    }
    return false;
}

float psWalkPoly::GetArea()
{
    float area = 0;
    int vertNum2;

    if(verts.GetSize() <= 2)
        return 0;

    for(int vertNum1=1; vertNum1 < (int)verts.GetSize()-1; vertNum1++)
    {
        vertNum2 = GetNeighbourIndex(vertNum1, +1);
        area += CalcTriangleArea(verts[0].pos, verts[vertNum1].pos, verts[vertNum2].pos);
    }
    return area;
}

float psWalkPoly::GetBoxSize()
{
    float size = 0;
    float dx = box.MaxX() - box.MinX();
    float dz = box.MaxZ() - box.MinZ();
    size = dx * dz;
    return size;
}

float psWalkPoly::GetLengthOfEdges()
{
    float size = 0;
    for(int vertNum=0; vertNum < (int)verts.GetSize(); vertNum++)
        size += Calc2DDistance(verts[vertNum].pos, verts[GetNeighbourIndex(vertNum, +1)].pos);
    return size;
}

float psWalkPoly::GetTriangleArea(int vertNum)
{
    assert(verts.GetSize() >= 3);
    int prev = GetNeighbourIndex(vertNum, -1),
        next = GetNeighbourIndex(vertNum, +1);
    return CalcTriangleArea(verts[prev].pos, verts[vertNum].pos, verts[next].pos);
}

bool PointIsNearWalls(psMapWalker &walker, const csVector3 &point, iSector* sector, float dist)
{
    csVector3 a, b, c, d;
    float realEndY;

    a = point;
    a.y += 1;

    b = a;
    c = a;
    d = a;

    a.x -= dist;
    a.z += dist;
    b.x += dist;
    b.z += dist;
    c.x += dist;
    c.z -= dist;
    d.x -= dist;
    d.z -= dist;

    /*CPrintf(CON_DEBUG, "IsNearWalls %i %i %i %i\n",
        int(walker.WalkBetween(point, a, sector, realEndY)),
        int(walker.WalkBetween(point, b, sector, realEndY)),
        int(walker.WalkBetween(point, c, sector, realEndY)),
        int(walker.WalkBetween(point, d, sector, realEndY)));*/

    return !walker.WalkBetween(point, a, sector, false, realEndY)
           ||
           !walker.WalkBetween(point, b, sector, false, realEndY)
           ||
           !walker.WalkBetween(point, c, sector, false, realEndY)
           ||
           !walker.WalkBetween(point, d, sector, false, realEndY);
}

bool psWalkPoly::IsNearWalls(psMapWalker &walker, int vertNum)
{
    return PointIsNearWalls(walker, verts[vertNum].pos, sector, 0.1F);
}

bool psWalkPoly::PointIsIn(const csVector3 &p, float maxRel)
{
    for(size_t vertNum=0; vertNum < verts.GetSize(); vertNum++)
        if(verts[vertNum].line.CalcRel(p) > maxRel)
            return false;
    return true;
}

int psWalkPoly::GetNumConts()
{
    int numConts=0;
    for(int vertNum = 0; vertNum < (int)verts.GetSize(); vertNum++)
        numConts += (int)verts[vertNum].conts.GetSize();
    return numConts;
}

void psWalkPoly::DeleteNode(psANode* node)
{
    for(size_t nodeNum=0; nodeNum < nodes.GetSize(); nodeNum++)
    {
        if(nodes[nodeNum] == node)
        {
            nodes.DeleteIndex(nodeNum);
            return;
        }
    }
}

void psWalkPoly::PruneUnboundVerts(psMapWalker &walker)
{
    int vertNum = 0;
    float oldArea = GetArea();

    CPrintf(CON_DEBUG, "PruneUnboundVerts\n");
    CShift();

    while(vertNum<(int)verts.GetSize()  &&  verts.GetSize()>3)
    {
        CPrintf(CON_DEBUG, "vertnum=%i touching=%i\n", vertNum, int(verts[vertNum].touching));
        if(!verts[vertNum].touching  &&  !IsNearWalls(walker, vertNum) && GetTriangleArea(vertNum)<0.2*oldArea)
        {
            CPrintf(CON_DEBUG, "pruned unbound %i\n",vertNum);
            verts.DeleteIndex(vertNum);
            RecalcAllEdges();
        }
        else
        {
            CPrintf(CON_DEBUG, "leaving alone\n");
            vertNum++;
        }
    }
    CUnshift();
}

void psWalkPoly::PruneInLineVerts()
{
    int vertNum=0;
    int next;
    int nextNext;
    ps2DLine line;

    while(vertNum<(int)verts.GetSize()  &&  verts.GetSize()>2)
    {
        //if (!verts[vertNum].touching)
        {
            nextNext = GetNeighbourIndex(vertNum, +2);
            while(nextNext != vertNum)
            {
                next = GetNeighbourIndex(vertNum, +1);
                line.Calc(verts[vertNum].pos, verts[nextNext].pos);
                //float dist = (verts[vertNum].pos - verts[nextNext].pos).Norm();
                //the tolerance cannot be dependent on length of edge
                //(as a wrong way to protect small polys from prunning), because that would
                //give advantages to shorter edges
                if(line.CalcDist(verts[next].pos) < 0.001)
                    //if (line.CalcDist(verts[next].pos) < 0.001*dist)
                {
                    CPrintf(CON_DEBUG, "pruned inline %i\n",next);
                    verts.DeleteIndex(next);
                    if(next < vertNum)  // fix index if it changed
                        vertNum--;
                    if(next < nextNext)  // fix index if it changed
                        nextNext--;
                    if(verts.GetSize()>2)
                    {
                        RecalcEdges(GetNeighbourIndex(next, -1));
                        RecalcEdges(GetNeighbourIndex(next, 0));
                    }
                }
                else
                    break;
                nextNext = GetNeighbourIndex(nextNext, +1);
            }
        }
        vertNum++;
    }
}

void pathFindTest_Convex()
{
    /*
        csVector3(-131.928680, 0, -198.448929),
            csVector3(-131.878708, 0, -197.535568),
            csVector3(-131.878693, 0, -197.535507))));
    */

    Error2("convex: %i", int(CheckConvexAngle(
                                 csVector3(-131.928680F, 0, -198.448929F),//0
                                 csVector3(-131.878708F, 0, -197.535568F),//1
                                 csVector3(-131.878693F, 0, -197.535507F) //2
                             )));

    Error2("convex: %i", int(CheckConvexAngle(
                                 csVector3(-131.878693F, 0, -197.535507F),//2
                                 csVector3(-131.928680F, 0, -198.448929F),//0
                                 csVector3(-131.878708F, 0, -197.535568F) //1
                             )));

    Error2("convex: %i", int(CheckConvexAngle(
                                 csVector3(-131.878708F, 0, -197.535568F),//1
                                 csVector3(-131.878693F, 0, -197.535507F),//2
                                 csVector3(-131.928680F, 0, -198.448929F) //0
                             )));
}

void pathFindTest_PruneWP(psNPCClient* npcclient)
{
    psWalkPolyMap map(npcclient->GetObjectReg());
    map.LoadFromFile("/this/pfgen.xml");
    map.PrunePolys();
    map.SaveToFile("/this/pfgen.xml");
}

void pathFindTest_Stretch(psNPCClient* npcclient)
{
    //printf("%f %f %f %f\n", NormalizeAngle(1), NormalizeAngle(7), NormalizeAngle(-1), NormalizeAngle(-7));
    //printf("%f %f %f\n", AngleDiff(1,-1), AngleDiff(1,7), AngleDiff(-1,-7));

    psWalkPolyMap map(npcclient->GetObjectReg());
    psWalkPoly p1, p2;
    iSector* sector;
    psSeedSet seeds(npcclient->GetObjectReg());
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (npcclient->GetObjectReg());


    //map.LoadFromFile("/this/pfgen.xml");
    //seeds.LoadFromFile("/this/seeds.xml");

    /*csVector3 p, n, c;
    p = csVector3(-181.917236, 0, -130.545532);
    n = csVector3(-181.662994, 0, -131.034805);
    c = csVector3(-181.777802, 0, -130.678299);
    ::Dump("c-p", c-p);
    ::Dump("c-n", c-n);
    printf("c-p U %f\n", (c-p).Norm());
    printf("c-n U %f\n", (c-n).Norm());
    ::Dump("c-p U", (c-p).Unit());
    ::Dump("c-n U", (c-n).Unit());*/

    /*psMapWalker * walker = new psMapWalker(cel);
    assert(walker->Init());*/


    /*p1.SetMini(csVector3(-134, -11, -193), VERT_COUNT);
    p1.Dump("mini poly");
    p1.RecalcAllEdges();
    p1.SetSector(sector);
    map.AddPoly(&p1);*/

    /*p2.AddVert(csVector3(-140, -11, -188));
    p2.AddVert(csVector3(-136, -11, -188));
    p2.AddVert(csVector3(-136, -11, -198));
    p2.AddVert(csVector3(-140, -11, -198));
    p2.SetSector(sector);
    map.AddPoly(&p2);*/

    //for (int i=0; i < 10; i++) p1.StretchVert(*walker, map, 6);
    //p1.StretchVert(*walker, map, 1);
    //for (int i=0; i < 10; i++)  p1.Stretch(*walker, map);
    //p1.Stretch(*walker, map);

    //middle of stair way
    //sector = engine->FindSector("blue");
    //seeds.AddSeed(psSeed(csVector3(-134, -12, -193), sector, 1));
    //map.Generate(cel, seeds, sector);


    //turn under the stairs
    //map.Generate(cel, csVector3(-102, -17, -195), sector);

    //krizovatka u mrize
    sector = engine->FindSector("blue");
    seeds.AddSeed(psSeed(csVector3(-180, -5.4F, -192), sector, 1));
    map.Generate(npcclient, seeds, sector);

    //nad krizovatkou u mrize
    //map.Generate(cel, csVector3(-180, -5.4, -170), sector);

    //nad nad krizovatkou u mrize
    //map.Generate(cel, csVector3(-180, -5.4, -140), sector);

    //zatacka daleko nad krizovatkou u mrize
    //map.Generate(cel, csVector3(-182.1, -5.4, -94), sector);

    //p1.Dump("final stretched poly");
    //while (p1.StretchVert(*walker, map, 0)) {  }


    //stred areny
    /*sector = engine->FindSector("cntr");
    seeds.AddSeed(psSeed(csVector3(0, -7, 0), sector, 1));
    map.Generate(cel, seeds, sector);*/

    //map.Generate(cel, csVector3(9.6, -3.4, 8.7), sector);

    //bojiste
    //map.Generate(cel, csVector3(42, 0, 43), sector);

    //chodba u bojiste
    //sector = engine->FindSector("upper");
    //map.Generate(cel, csVector3(34, 0, 9.4), sector);

    //uprostred schodu vedoucich nahoru z centra
    //sector = engine->FindSector("cntr");
    //map.Generate(cel, csVector3(-3.35, -4.77, -7.6), sector);

    //zahnuta chodba uplne dole
    //sector = engine->FindSector("trans1");
    //map.Generate(cel, csVector3(49, -17.43, -22.2), sector);




    //pred zacatkem schodu v taverne
    //sector = engine->FindSector("Akk-Central");
    //seeds.AddSeed(psSeed(csVector3(-42, -6.9, -21), sector, 1));
    //map.Generate(cel, seeds, sector);

    //chodby mezi domy (nad stany)
    //sector = engine->FindSector("Akk-East");
    //map.Generate(cel, csVector3(79.8, -0.18, -32), sector);

    //mezi bednami
    //sector = engine->FindSector("Akk-East");
    //map.Generate(cel, csVector3(71, 1.8, 47), sector);

    //mezi stany
    //sector = engine->FindSector("Akk-East");
    //map.Generate(cel, csVector3(-28, -4.5, -53), sector);

    //mezi branou a tavernou
    //sector = engine->FindSector("Akk-Central");
    //map.Generate(cel, csVector3(-42.8, -7, -44), sector);

    map.SaveToFile("/this/pfgen.xml");
}


void psWalkPolyMap::Generate(psNPCClient* npcclient, psSeedSet &seeds, iSector* sector)
{
    psWalkPoly* poly;
    int numMovedThisStep;
    int numSteps;
    int numStuckRounds, numSuccessRounds;
    size_t vertNum;
    psMapWalker* walker;
    psStopWatch watch, watch2;
    psSeed seed;
    bool seedIsValid;
    const float MAX_AREA = 1000;


    walker = new psMapWalker(npcclient);
    assert(walker->Init());

    watch.Start();

    while(true)
    {
        watch2.Start();
        seedIsValid = false;
        while(!seeds.IsEmpty()  &&  !seedIsValid)
        {
            seed = seeds.PickUpBestSeed();
            ::Dump3("picked up seed", seed.pos);
            seedIsValid = seed.CheckValidity(walker);
        }
        CPrintf(CON_DEBUG, "seed pick time %i\n", watch2.Stop());

        // ============== LOOP EXIT ===============
        if(!seedIsValid)
            break;


        poly = new psWalkPoly;
        poly->SetSector(sector);
        watch2.Start();
        poly->SetMini(*walker, seed.pos, VERT_COUNT);
        CPrintf(CON_DEBUG, "poly mini time %i\n", watch2.Stop());

        for(vertNum=0; vertNum < poly->verts.GetSize(); vertNum++)
            poly->verts[vertNum].angle = 2*PI/VERT_COUNT * vertNum;


        CShift();
        numSteps = 0;
        numStuckRounds = 0;
        numSuccessRounds = 0;
        do
        {
            float stepSize = MIN(0.3, sqrt(poly->GetArea()) * 0.05);

            poly->PruneInLineVerts();

            numMovedThisStep = poly->Stretch(*walker, *this, stepSize);
            if(numMovedThisStep == 0)
                numStuckRounds++;
            else
            {
                numStuckRounds = 0;
                numSuccessRounds ++;
            }

            numSteps++;
        }
        while(numStuckRounds<=2  &&  poly->GetArea()<MAX_AREA &&  numSteps<MAX_STEPS);
        CUnshift();

        if(poly->GetArea()>=MAX_AREA)
            CPrintf(CON_DEBUG, "too big\n");
        if(numSteps==MAX_STEPS)
            CPrintf(CON_DEBUG, "too many steps\n");
        if(numStuckRounds>2)
            CPrintf(CON_DEBUG, "all vertices were stuck for a few rounds\n");

        poly->DumpPolyJS("stretched poly");
        poly->PruneUnboundVerts(*walker);
        poly->DumpPolyJS("prunned unbound");
        poly->PruneInLineVerts();
        poly->DumpPolyJS("prunned unimportant");
        //poly->GlueToNeighbours(*walker, *this);
        //poly->DumpPolyJS("glued");

        //if (/*!poly->IsInLine()*/  poly->GetArea()>=2)
        if(numSuccessRounds > 0)
        {
            CPrintf(CON_DEBUG, "poly was accepted (id=%i)\n", poly->id);
            AddPoly(poly);
            poly->AddSeeds(walker, seeds);

            SaveToFile("/this/pfgen.xml");
            seeds.SaveToFile("/this/seeds.xml");

            //if (ListLength(polys) == 5) break;
        }
        else
        {
            CPrintf(CON_DEBUG, "poly was refused\n");
            delete poly;
        }
    }

    prof.AddCons("generate", watch.Stop());
    printf("%s", prof.Dump("msec", "WalkPoly generation profile").GetData());

    printf("ran out of seeds\n");
    DumpPolys();
    fflush(0);

    delete walker;
}

void psWalkPoly::LoadFromXML(iDocumentNode* node, iEngine* engine)
{
    id = node->GetAttributeValueAsInt("id");
    sector = engine->FindSector(node->GetAttributeValue("sector"));

    csRef<iDocumentNodeIterator> it = node->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> vertNode = it->Next();
        AddVert(csVector3(vertNode->GetAttributeValueAsFloat("x"),
                          vertNode->GetAttributeValueAsFloat("y"),
                          vertNode->GetAttributeValueAsFloat("z")));
    }
    RecalcAllEdges();
}

void psWalkPoly::SaveToString(csString &str)
{
    str += csString().Format("<poly id='%i' sector='%s'>\n", id, sector->QueryObject()->GetName());
    for(size_t vertNum=0; vertNum < verts.GetSize(); vertNum++)
    {
        csVector3 &pos = verts[vertNum].pos;
        str += csString().Format("<p x='%.3f' y='%.3f' z='%.3f'/>", pos.x, pos.y, pos.z);
    }
    str += "</poly>\n";
}

void psWalkPolyMap::DeleteContsWith(psWalkPoly* poly)
{
    csList<psWalkPoly*>::Iterator it(polys);
    while(it.HasNext())
        it.Next()->DeleteContsWith(poly);
}

psANode* psWalkPolyMap::FindNearbyANode(const csVector3 &pos)
{
    return index.FindNearbyANode(pos);
}

psWalkPolyMap::psWalkPolyMap()
{
    objReg = NULL;
    vfs = NULL;
}

psWalkPolyMap::psWalkPolyMap(iObjectRegistry* objReg)
{
    this->objReg = objReg;
    vfs =  csQueryRegistry<iVFS> (objReg);
}

void psWalkPolyMap::LoadFromXML(iDocumentNode* node)
{
    csRef<iEngine> engine =  csQueryRegistry<iEngine> (objReg);

    csRef<iDocumentNodeIterator> it = node->GetNodes();
    while(it->HasNext())
    {
        csRef<iDocumentNode> polyNode = it->Next();
        psWalkPoly* poly = new psWalkPoly;
        poly->LoadFromXML(polyNode, engine);

        // preserve uniqueness of ids
        if(psWalkPoly::nextID <= poly->id)
            psWalkPoly::nextID = poly->id + 1;
        AddPoly(poly);
    }
    CalcConts();
}

void psWalkPolyMap::SaveToString(csString &str)
{
    csList<psWalkPoly*>::Iterator it(polys);

    str += "<map>\n";
    while(it.HasNext())
    {
        psWalkPoly* poly = it.Next();
        poly->SaveToString(str);
    }
    str += "</map>\n";
}

bool psWalkPolyMap::SaveToFile(const csString &path)
{
    csString str;
    SaveToString(str);
    return vfs->WriteFile(path, str.GetData(), str.Length());
}

bool psWalkPolyMap::LoadFromFile(const csString &path)
{
//csRef<iDataBuffer> buff2 = vfs->ReadFile("/planeshift/world2/_README.txt");
//csRef<iDataBuffer> buff2 = vfs->ReadFile("/this/art/world/sewers_wpmap.xml");
//csRef<iDataBuffer> buff2 = vfs->ReadFile("/this/report.xml");
//assert(buff2);

    csRef<iDocument> doc = ParseFile(objReg, path);
    if(doc == NULL)
        return false;
    csRef<iDocumentNode> mapNode = doc->GetRoot()->GetNode("map");
    if(mapNode == NULL)
    {
        Error1("Missing <map> tag");
        return false;
    }
    LoadFromXML(mapNode);
    return true;
}

bool psWalkPolyMap::LoadFromString(const csString &str)
{
    csRef<iDocument> doc = ParseString(str);
    if(doc == NULL)
        return false;
    csRef<iDocumentNode> mapNode = doc->GetRoot()->GetNode("map");
    if(mapNode == NULL)
    {
        Error1("Missing <map> tag");
        return false;
    }
    LoadFromXML(mapNode);
    return true;
}

void psWalkPolyMap::PrunePolys()
{
    csList<psWalkPoly*>::Iterator it, next;

    it = polys;
    if(it.HasNext())
        it.Next();
    while(it.HasCurrent())
    {
        psWalkPoly* poly = *it;

        if(it.HasNext())
        {
            next = it;
            next.Next();
        }
        else
            next = csList<psWalkPoly*>::Iterator();

        if(!poly->supl)
        {
            CPrintf(CON_DEBUG, "%i are =%f\n", poly->id, poly->GetArea());
            if(poly->GetArea() < 0.02)
            {
                CPrintf(CON_DEBUG, "prunning poly id=%i\n", poly->id);
                DeleteContsWith(poly);
                delete poly;
                polys.Delete(it);
            }

            if(poly->ReachableFromNeighbours())
            {
                CPrintf(CON_DEBUG, "prunning reachable poly id=%i\n", poly->id);
                DeleteContsWith(poly);
                delete poly;
                polys.Delete(it);
            }
        }

        it = next;
    }
}

void psWalkPolyMap::CalcConts()
{
    csList<psWalkPoly*>::Iterator it(polys);

    while(it.HasNext())
    {
        psWalkPoly* poly = it.Next();
        poly->CalcConts(*this);
    }
}

void pathFindTest_Save(psNPCClient* npcclient)
{
    psWalkPoly p1, p2;
    psWalkPolyMap map(npcclient->GetObjectReg());

    p1.AddVert(csVector3(0, 0, 2.123F));
    p1.AddVert(csVector3(2, 0, 2));
    p1.AddVert(csVector3(2, 0, 0));
    p1.AddVert(csVector3(0, 0, 0));

    p2.AddVert(csVector3(1, 0, 3));
    p2.AddVert(csVector3(3, 0, 3));
    p2.AddVert(csVector3(3, 0, 1));
    p2.AddVert(csVector3(1, 0, 1));

    map.AddPoly(&p1);
    map.AddPoly(&p2);

    map.SaveToFile("/this/pf.xml");

    psWalkPolyMap map2(npcclient->GetObjectReg());
    map2.LoadFromFile("/this/pf.xml");
    map2.DumpPolys();
}

/***********************************************************************************
*
*                            detection of direct walkability
*
************************************************************************************/

void EstimateY(const csVector3 &begin, const csVector3 &end, csVector3 &point)
{
    float distToBegin = Calc2DDistance(begin, point);
    float slope = (end.y - begin.y) / Calc2DDistance(begin, end);
    point.y = begin.y + slope * distToBegin;
}

bool psWalkPoly::MoveToBorder(const csVector3 &goal, csVector3 &point, int &edgeNum)
{
    csVector3 inter;
    ps2DLine goalLine;
    const bool dbg = true;

    if(dbg)
    {
        CPrintf(CON_DEBUG, "MoveToBorder id=%i\n", id);
        CShift();
        ::Dump("point", point);
        ::Dump("goal",  goal);
    }

    goalLine.Calc(point, goal);
    if(dbg)goalLine.Dump();

    for(edgeNum=0; edgeNum < (int)verts.GetSize(); edgeNum++)
    {
        if(dbg)
        {
            CPrintf(CON_DEBUG, "testing edge %i\n", edgeNum);
            CShift();
            ::Dump("a", verts[edgeNum].pos);
            ::Dump("b", verts[GetNeighbourIndex(edgeNum, +1)].pos);
            CUnshift();
        }

        if(dbg)verts[edgeNum].line.Dump();
        if(CalcInter(verts[edgeNum].pos,
                     verts[GetNeighbourIndex(edgeNum, +1)].pos,
                     verts[edgeNum].line,
                     point, goal, goalLine,
                     inter)
          )//&& verts[edgeNum].line.CalcDist(point) > 0.0001)
            break;
    }

    // this happens when currPoint is not within our polygon
    if((size_t)edgeNum == verts.GetSize())
    {
        if(dbg)
        {
            CPrintf(CON_DEBUG, "no intersection found: point is out of poly\n");
            ::Dump("point is", point);
            DumpPolyJS("poly is");
            CUnshift();
        }
        return false;
    }

    ::EstimateY(verts[edgeNum].pos, verts[GetNeighbourIndex(edgeNum, +1)].pos, point);

    if(dbg)CPrintf(CON_DEBUG, "found edge inter edge=%i x=%f y=%f z=%f\n", edgeNum, inter.x, inter.y, inter.z);
    point = inter;
    if(dbg)CUnshift();
    return true;
}

bool psWalkPoly::MoveToNextPoly(const csVector3 &goal, csVector3 &point, psWalkPoly* &nextPoly)
{
    int edgeNum;

    if(!MoveToBorder(goal, point, edgeNum))
        return false;

    nextPoly = FindCont(point, edgeNum);
    if(nextPoly == NULL)
        return false;

    csVector3 dir = goal-point;
    dir.Normalize();
    point += 0.05 * dir;

    if(!nextPoly->PointIsIn(point, -0.001F))
        nextPoly->MovePointInside(point, goal);

    return true;
}


void pathFindTest_CheckDirect(psNPCClient* npcclient)
{
    csVector3 goal;
    csVector3 currPoint;
    psWalkPolyMap map(npcclient->GetObjectReg());
//    int edgeNum;
//    int i;

    map.LoadFromFile("/this/pfgen.xml");
    map.CalcConts();

    currPoint.Set(-120, -15, -198.5);
    //goal.Set(-180, -5, -185);
    goal.Set(-135, -16, -192);

    map.CheckDirectPath(currPoint, goal);
}

psWalkPoly* psWalkPolyMap::FindPoly(int id)
{
    return polyByID.Get(id, NULL);
}

void psWalkPolyMap::AddPoly(psWalkPoly* poly)
{
    polys.PushBack(poly);
    index.Add(poly);
    polyByID.Put(poly->id, poly);
}

csString psWalkPolyMap::DumpPolys()
{
    csList<psWalkPoly*>::Iterator it(polys);
    while(it.HasNext())
    {
        it.Next()->DumpPureJS();
    }
    return "";
}

bool psWalkPolyMap::CheckDirectPath(csVector3 start, csVector3 goal)
{
    psWalkPoly* goalPoly;
    psWalkPoly* currPoly;
    csVector3 currPoint;
    bool found = false;
    const bool dbg = true;

    if(dbg)CPrintf(CON_DEBUG, "CheckDirectPath\n");
    if(dbg)CShift();

    ::Dump3("start", start);
    ::Dump3("goal", goal);

    currPoint = start;

    currPoly = index.FindPolyOfPoint(start);
    if(!currPoly)
    {
        if(dbg)CUnshift();
        return false;
    }

    goalPoly = index.FindPolyOfPoint(goal);
    if(!goalPoly)
    {
        if(dbg)CUnshift();
        return false;
    }

    if(dbg)CPrintf(CON_DEBUG, "currPoly = %i\n", currPoly->id);
    if(dbg)CPrintf(CON_DEBUG, "goalPoly = %i\n", goalPoly->id);

    if(!currPoly->PointIsIn(currPoint, -0.001F))
    {
        currPoly->MovePointInside(currPoint, -0.001F);
    }
    if(!goalPoly->PointIsIn(goal, -0.001F))
    {
        goalPoly->MovePointInside(goal, start);
    }

    ::Dump3("start2", start);
    ::Dump3("goal2", goal);

    while(currPoly != NULL)
    {
        found = currPoly == goalPoly
                ||
                Calc2DDistance(currPoint, goal)<=0.01;

        if(found)
            break;

        if(!currPoly->MoveToNextPoly(goal, currPoint, currPoly))
        {
            if(dbg)CPrintf(CON_DEBUG, "no next poly - path is not direct\n");
            if(dbg)CUnshift();
            return false;
        }

        if(dbg)::Dump("moved to next poly", currPoint);
    }

    if(dbg)Dump("final point", currPoint);
    if(dbg)CUnshift();
    return found;
}


/***********************************************************************************
*
*                            spatial index
*
************************************************************************************/


#define MAX_POLY_COUNT_OF_NODE   10

psSpatialIndexNode::psSpatialIndexNode()
{
    parent = child1 = child2 = NULL;
    numPolys    =  0;
    splitAxis   =  ' ';
    splitValue  =  0;
}


void pathFindTest_index()
{
    psSpatialIndex idx;
    const int np = 60;
    int i;
    psWalkPoly* p[np];
    psWalkPolyMap map;

    for(i=0; i < np; i++)
        p[i] = new psWalkPoly();

    for(i=0; i < np; i++)
        p[i]->AddVert(csVector3(rnd(-10, 10), rnd(-10, 10), rnd(-10, 10)));

    /*p[0]->AddVert(csVector3(1, 1, 1));
    p[1]->AddVert(csVector3(1, 1, -1));
    p[2]->AddVert(csVector3(1, -1, -1));
    p[3]->AddVert(csVector3(-1, -1, -1));
    p[4]->AddVert(csVector3(-1, 1, 1));
    p[5]->AddVert(csVector3(-1, -1, 1));*/

    for(i=0; i < np; i++)
    {
        p[i]->RecalcAllEdges();
        idx.Add(p[i]);
    }

    for(i=0; i < 100; i++)
    {
        csVector3 p(rnd(-20, 20), rnd(-20, 20), rnd(-20, 20));
        psSpatialIndexNode* node = idx.FindNodeOfPoint(p);
        if(node->GetBBox().In(p))
            printf("right,");
        else
            printf("wrong,");
    }
    printf("\n");

    /*psSpatialIndexNode * node;
    node = idx.FindNodeOfPoint(csVector3(1, 1, 1));
    Dump("1,1,1", node->bbox);
    node = idx.FindNodeOfPoint(csVector3(1, 1, -1));
    Dump("1,1,-1", node->bbox);
    node = idx.FindNodeOfPoint(csVector3(1, -1, -1));
    Dump("1,-1,-1", node->bbox);
    node = idx.FindNodeOfPoint(csVector3(-1, -1, -1));
    Dump("-1,-1,-1", node->bbox);
    node = idx.FindNodeOfPoint(csVector3(-1, 1, 1));
    Dump("-1,1,1", node->bbox);
    node = idx.FindNodeOfPoint(csVector3(-1, -1, 1));
    Dump("-1,-1,1", node->bbox);*/
}

/*void psSpatialIndex::Rebuild()
{

}
*/

int psSpatialIndexNode::GetTreeLevel()
{
    if(parent == NULL)
        return 0;
    else
        return parent->GetTreeLevel()+1;
}

void psSpatialIndexNode::Split()
{
    int axisIdx=0;

    //printf("indexnode::split\n");
    //Dump("parent", bbox);

    ChooseSplit();

    // set children bboxes
    child1 = new psSpatialIndexNode();
    child1->bbox = bbox;
    child1->parent = this;
    child2 = new psSpatialIndexNode();
    child2->bbox = bbox;
    child1->parent = this;
    switch(splitAxis)
    {
        case 'x':
            axisIdx = 0;
            break;
        case 'y':
            axisIdx = 1;
            break;
        case 'z':
            axisIdx = 2;
            break;
    }
    child1->bbox.SetMax(axisIdx, splitValue);
    child2->bbox.SetMin(axisIdx, splitValue);

    //Dump("child1", child1->bbox);
    //Dump("child2", child2->bbox);

    // move polys to one or both children
    csList<psWalkPoly*>::Iterator it(polys);
    while(it.HasNext())
    {
        psWalkPoly* poly = it.Next();
        if(child1->bbox.Overlap(poly->GetBox()))
            child1->Add(poly);
        if(child2->bbox.Overlap(poly->GetBox()))
            child2->Add(poly);
    }
    polys.DeleteAll();
    numPolys = 0;
}

void psSpatialIndexNode::ChooseSplit()
{
    csBox3 contBBox;
    psWalkPoly* poly;

    csList<psWalkPoly*>::Iterator it(polys);
    if(!it.HasNext()) return;

    poly = it.Next();
    contBBox = poly->GetBox();

    while(it.HasNext())
    {
        poly = it.Next();
        contBBox += poly->GetBox();
    }

    contBBox *= bbox;
    //Dump("real node bbox", contBBox);

    float dx = contBBox.MaxX() - contBBox.MinX();
    float dy = contBBox.MaxY() - contBBox.MinY();
    float dz = contBBox.MaxZ() - contBBox.MinZ();

    if(dx > dy)
    {
        if(dx > dz)
        {
            splitAxis = 'x';
            splitValue = (contBBox.MaxX() + contBBox.MinX()) / 2;
        }
        else
        {
            splitAxis = 'z';
            splitValue = (contBBox.MaxZ() + contBBox.MinZ()) / 2;
        }
    }
    else
    {
        if(dy > dz)
        {
            splitAxis = 'y';
            splitValue = (contBBox.MaxY() + contBBox.MinY()) / 2;
        }
        else
        {
            splitAxis = 'z';
            splitValue = (contBBox.MaxZ() + contBBox.MinZ()) / 2;
        }
    }
}

psSpatialIndexNode* psSpatialIndexNode::FindNodeOfPoint(const csVector3 &p)
{
    //printf("indexnode::find\n");
    if(child1 == NULL)
    {
        //printf("find in term\n");
        assert(bbox.In(p));
        return this;
    }
    //printf("find in nonterm\n");
    //Dump("child1", child1->bbox);
    if(child1->bbox.In(p))
        return child1->FindNodeOfPoint(p);
    //Dump("child2", child2->bbox);
    assert(child2->bbox.In(p));
    return child2->FindNodeOfPoint(p);
    assert(false);
}


void psSpatialIndexNode::Add(psWalkPoly* poly)
{
    //if (GetTreeLevel() == 10) assert(0);

    poly->CalcBox();

    //printf("indexnode::add\n");
    //Dump("poly box", poly->GetBox());
    if(child1 == NULL)
    {
        //printf("adding to term\n");
        polys.PushBack(poly);
        numPolys++;
        if(numPolys > MAX_POLY_COUNT_OF_NODE)  // this does not lower the poly count, when fed with evil data
            Split();
    }
    else
    {
        //printf("adding to nonterm\n");
        if(child1->bbox.Overlap(poly->GetBox()))
            child1->Add(poly);
        if(child2->bbox.Overlap(poly->GetBox()))
            child2->Add(poly);
    }
}

psSpatialIndex::psSpatialIndex()
{
    root.bbox.Set(-CS_BOUNDINGBOX_MAXVALUE, -CS_BOUNDINGBOX_MAXVALUE, -CS_BOUNDINGBOX_MAXVALUE,
                  CS_BOUNDINGBOX_MAXVALUE,  CS_BOUNDINGBOX_MAXVALUE,  CS_BOUNDINGBOX_MAXVALUE);
}

void psSpatialIndex::Add(psWalkPoly* poly)
{
    //printf("index::add\n");
    root.Add(poly);
}

psSpatialIndexNode* psSpatialIndex::FindNodeOfPoint(const csVector3 &p, psSpatialIndexNode* hint)
{
    //printf("index::find\n");
    // if the hint is terminal and contains our point, we have the solution
    if(hint != NULL   &&   hint->child1 == NULL   &&   hint->bbox.In(p))
        return hint;
    else
        return root.FindNodeOfPoint(p);
}

psANode* psSpatialIndex::FindNearbyANode(const csVector3 &pos)
{
    psWalkPoly* poly = FindPolyOfPoint(pos);
    if(poly == NULL)
        return NULL;
    else
        return poly->GetFirstNode();
}

/*psSpatialIndexNode * psSpatialIndex::FindNodeOfPoly(psWalkPoly * poly)
{
    psWalkPoly * node = FindPolyOfPoint(poly->verts[0].pos);
}*/

psWalkPoly* psSpatialIndex::FindPolyOfPoint(const csVector3 &p, psSpatialIndexNode* hint)
{
    /// quick&dirty:

    csVector3 closest;
    psWalkPoly* bestPoly=NULL;
    float dist, bestDist=0.0;
    float angle;
    psWalkPoly* poly;

    //CPrintf(CON_DEBUG, "index::pfop:\n");
    //CShift();
    //Dump3("searched point", p);

    psSpatialIndexNode* node = FindNodeOfPoint(p, hint);
    assert(node);
    csList<psWalkPoly*>::Iterator it(node->polys);
    while(it.HasNext())
    {
        poly = it.Next();
        //CPrintf(CON_DEBUG, "index::pfop checking:\n");
        //Dump("poly", poly->GetBox());
        if(poly->PointIsIn(p))
        {
            dist = fabs(p.y - poly->EstimateY(p));
            if(bestPoly==NULL  ||  dist<bestDist)
            {
                bestPoly = poly;
                bestDist = dist;
            }
        }
    }

    if(bestPoly != NULL)
        return bestPoly;



    bestPoly = NULL;
    it = node->polys;
    while(it.HasNext())
    {
        poly = it.Next();

        closest = poly->GetClosestPoint(p, angle);
        // we must ignore y, because closest.y is just rough estimate
        dist = Calc2DDistance(closest, p);

        //CPrintf(CON_DEBUG, "index::pfop id=%i dist=%f\n", poly->GetID(), dist);
        //::Dump3("closest", closest);

        if(dist<1  && (bestPoly==NULL  ||  dist<bestDist))
        {
            bestPoly = poly;
            bestDist = dist;
        }
    }

    //CUnshift();
    return bestPoly;
}

/***********************************************************************************
*
*                         ps2DLine
*
************************************************************************************/

void ps2DLine::Dump()
{
    CPrintf(CON_DEBUG, "Norm eq: a=%f b=%f c=%f\n", a, b, c);
}
//1.2.2001
void ps2DLine::CalcPerp(const csVector3 &begin, const csVector3 &end)
{
    csVector3 diff = end - begin;
    float norm2D = diff.x*diff.x + diff.z*diff.z;
    a = diff.x / norm2D;
    b = diff.z / norm2D;
    c = - (a*end.x + b*end.z);
}

float ps2DLine::CalcRel(const csVector3 &point)
{
    return a*point.x + b*point.z + c;
}

float ps2DLine::CalcDist(const csVector3 &point)
{
    return fabs(CalcRel(point));
}

/** Normal vector will point "left" from the direction vector.
    Normalized for fast distance calculations */
void ps2DLine::Calc(const csVector3 &p1, const csVector3 &p2)
{
    float sX, sZ;

    sX = p2.x-p1.x;
    sZ = p2.z-p1.z;

    a = -sZ;
    b = sX;

    c = - (a*p1.x + b*p1.z);

    float magn = sqrt(a*a + b*b);
    a /= magn;
    b /= magn;
    c /= magn;
}

void ps2DLine::Copy(ps2DLine &dest)
{
    dest.a = a;
    dest.b = b;
    dest.c = c;
}

#endif
