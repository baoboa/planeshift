/*
  Crystal Space Entity Layer
  Copyright (C) 2001 PlaneShift Team (info@planeshift.it,
  Copyright (C) 2001-2005 by Jorrit Tyberghein

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

/*
 * This code is heavily based on pslinmove from the PlaneShift project.
 * Thanks a lot for making this!
 */

#include <cssysdef.h>

//CS includes
#include <iutil/objreg.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/evdefs.h>
#include <iutil/virtclk.h>

#include <imesh/sprite3d.h>
#include <imesh/spritecal3d.h>

#include <csutil/databuf.h>
#include <csutil/plugmgr.h>
#include <csutil/callstack.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <iengine/engine.h>
#include <iengine/sector.h>
#include <iengine/scenenode.h>
#include <imesh/object.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>
#include <ivaria/reporter.h>
#include "ivaria/mapnode.h"

#include <imesh/objmodel.h>
#include <igeom/path.h>
#include <csgeom/path.h>
#include <csgeom/math3d.h>

#include "linmove.h"
#include "colldet.h"
#include "util/strutil.h"
#include "util/log.h"


// collision detection variables
#define MAXSECTORSOCCUPIED 20

// velocity = prevVelocity + ACCEL
#define ACCEL 0.5f

#define LEGOFFSET 0

// This is the distance the CD will use to look for objects to collide with.

// Set this in order to see what meshes the player is colliding with
//#define SHOW_COLLIDER_MESH_DEBUG

#define LINMOVE_PATH_FLAG (char)0x80

/*
 * This dumps 2 lines for every cross-sector CD that fails a sector-collision
 * test. It's not that bad since this shouldn't happen that often.
 *
 *  The two cases where it should happen are:
 *  1) A mesh crosses a portal.  In this case the mesh should exist in both
 *   sectors or a copy of the mesh exist in each sector or the mesh should
 *   be "cut" into two meshes at the portal boundary. There isn't too much
 *   difference between these three from a performance perspective.
 *  2) A mesh is in a sector, but is entirely on the other side of a portal.
 *   This mesh should probably not be in the sector it's in at all. It will
 *   not be collided with, and probably wont be rendered either!
 *
 * If this message comes up, a collision did NOT occur as far as we're
 * concerned. We found a collision, but we can't reach the "space" that this
 * collision occured in, so it's ignored.
 */
//#define LINMOVE_CD_FOLLOWSEG_DEBUG

/*
 * Terminal velocity
 * ((120 miles/hour  / 3600 second/hour) * 5280 feet/mile)
 *     / 3.28 feet/meter = 53.65 m/s
 *   *2.0 for "feel" correction = 107.3m/s
 */
#define ABS_MAX_FREEFALL_VELOCITY 107.3f


/*
 * This value is a default value to define the part of the mesh used a
 * body and leg in collision detection
 */
#define BODY_LEG_FACTOR 0.6f


//----------------------------------------------------------------------------


psLinearMovement::psLinearMovement(iObjectRegistry* object_reg)
{
    colldet = 0;

    this->object_reg = object_reg;
    vc = csQueryRegistry<iVirtualClock> (object_reg);
    if(!vc)
    {
        return;
    }

    engine = csQueryRegistry<iEngine> (object_reg);
    if(!engine)
    {
        return;
    }

    velBody = angularVelocity = velWorld = 0;
    angleToReachFlag = false;
    angDelta = 0.0f;
    lastDRUpdate = 0;
    lastClientSector = NULL;
    lastClientDRUpdate = 0;

    xRot = 0.0f;
    zRot = 0.0f;
    hugGround = false;

    portalDisplaced = 0.0f;

    path = 0;
    path_speed = 0.0f;
    path_time  = 0.0f;

    offset_err = 0;
    offset_rate = 0;

    deltaLimit = 0.0f;


    ResetGravity();

    /*
    * Initialize bouding box parameters to detect if they have been
    * loaded or not
    */
    topSize.Set(0, 0, 0);

    scale = -1;
}


psLinearMovement::~psLinearMovement()
{
    delete colldet;
}


static inline bool FindIntersection(const csCollisionPair &cd, csVector3 line[2])
{
    csVector3 tri1[3];
    tri1[0] = cd.a1;
    tri1[1] = cd.b1;
    tri1[2] = cd.c1;
    csVector3 tri2[3];
    tri2[0] = cd.a2;
    tri2[1] = cd.b2;
    tri2[2] = cd.c2;
    csSegment3 isect;
    bool coplanar, ret;

    ret = csIntersect3::TriangleTriangle(tri1, tri2, isect, coplanar);
    line[0] = isect.Start();
    line[1] = isect.End();
    return ret;
}

void psLinearMovement::SetAngularVelocity(const csVector3 &angleVel)
{
    angularVelocity = angleVel;
    angleToReachFlag = false;
}

void psLinearMovement::SetAngularVelocity(const csVector3 &angleVel,
        const csVector3 &angleToReach)
{
    SetAngularVelocity(angleVel);
    angleToReachFlag = true;
    this->angleToReach = angleToReach;
}

// --------------------------------------------------------------------------

static float Matrix2YRot(const csMatrix3 &mat)
{
    csVector3 vec(0.0f, 0.0f, 1.0f);
    vec = mat * vec;

    float result = atan2(vec.x, vec.z);
    // force the result in range [0;2*pi]
    if(result < 0) result += TWO_PI;

    return result;
}

// --------------------------------------------------------------------------
//Does the actual rotation
bool psLinearMovement::RotateV(float delta)
{
    if(!mesh)
    {
        return false;
    }

    // rotation
    if(angularVelocity < SMALL_EPSILON)
        return false;

    csVector3 angle = angularVelocity * delta;
    if(angleToReachFlag)
    {
        float current_yrot = GetYRotation();
        float yrot_delta = fabs(angleToReach.y - current_yrot);
        if(fabs(angle.y) > yrot_delta)
        {
            angle.y = (angle.y / fabs(angle.y)) * yrot_delta;
            angularVelocity = 0;
            angleToReachFlag = false;
        }
    }

    iMovable* movable = mesh->GetMovable();
    csYRotMatrix3 rotMat(angle.y);
    movable->SetTransform(movable->GetTransform().GetT2O() * rotMat);
    movable->UpdateMove();
    //pcmesh->GetMesh ()->GetMovable ()->Transform (rotMat);
    return true;
}


/*
 * MAX_CD_INTERVAL is now the maximum amount of time that should pass in a
 * single step regardless of the current velocity.
 * Since acceleration is factored into the velocity each step, this
 * shouldn't be too large or you'll get unrealistic gravity
 * in some situations.
 */
#define MAX_CD_INTERVAL 1

/*
 * MIN_CD_INTERVAL is the minimum amount of time that can pass in a single
 * step through the collision detection and movement process.
 *
 * This is basically a protection against 0 time (you will never move) and
 * negative time (you will move the opposite direction until you hit an
 * obstruction.
 */
#define MIN_CD_INTERVAL 0.01


int psLinearMovement::MoveSprite(float delta)
{
    int ret = PS_MOVE_SUCCEED;

    csReversibleTransform fulltransf = mesh->GetMovable()
                                       ->GetFullTransform();
    //  const csMatrix3& transf = fulltransf.GetT2O ();

    // Calculate the total velocity (body and world) in OBJECT space.
    csVector3 bodyVel(fulltransf.Other2ThisRelative(velWorld) + velBody);

    float local_max_interval =
        csMin(csMin((bodyVel.y==0.0f) ? MAX_CD_INTERVAL : ABS(intervalSize.y/bodyVel.y),
                    (bodyVel.x==0.0f) ? MAX_CD_INTERVAL : ABS(intervalSize.x/bodyVel.x)
                   ),(bodyVel.z==0.0f) ? MAX_CD_INTERVAL : ABS(intervalSize.z/bodyVel.z)
             );

    // Err on the side of safety (95% error margin)
    local_max_interval *= 0.95f;

    //printf("local_max_interval=%f, bodyVel is %1.2f, %1.2f, %1.2f\n",local_max_interval, bodyVel.x, bodyVel.y, bodyVel.z);
    //printf("velWorld is %1.2f, %1.2f, %1.2f\n", velWorld.x, velWorld.y, velWorld.z);
    //printf("velBody is %1.2f, %1.2f, %1.2f\n", velBody.x, velBody.y, velBody.z);

    // Sanity check on time interval here.  Something is messing it up. -KWF
    //local_max_interval = csMax(local_max_interval, 0.1F);

    if(colldet)
    {
        while(delta > local_max_interval)
        {

            csVector3 oldpos(mesh->GetMovable()->GetFullTransform().GetOrigin());

            // Perform brutal optimisation here for falling with no obstacles
            if(velWorld.y < -20.0f && mesh->GetMovable()->GetSectors()->GetCount() > 0)
            {
                csVector3 worldVel(fulltransf.This2OtherRelative(velBody) + velWorld);
                bool hit = false;

                // We check for other meshes at the start and end of the box with radius * 2 to be on the safe side
                {
                    csRef<iMeshWrapperIterator> objectIter = engine->GetNearbyMeshes(mesh->GetMovable()->GetSectors()->Get(0),
                            oldpos + boundingBox.GetCenter(), boundingBox.GetSize().Norm() * 2);
                    if(objectIter->HasNext())
                        hit = true;
                }
                if(!hit)
                {
                    csRef<iMeshWrapperIterator> objectIter = engine->GetNearbyMeshes(mesh->GetMovable()->GetSectors()->Get(0),
                            oldpos + worldVel * delta + boundingBox.GetCenter(), boundingBox.GetSize().Norm() * 2);
                    if(objectIter->HasNext())
                        hit = true;
                }
                if(!hit)
                {
                    csOrthoTransform transform_newpos(csMatrix3(), oldpos + worldVel * delta);
                    csBox3 fullBox(fulltransf.This2OtherRelative(boundingBox.Min()), fulltransf.This2OtherRelative(boundingBox.Max()));
                    csBox3 newBox(transform_newpos.This2OtherRelative(boundingBox.Min()), transform_newpos.This2OtherRelative(boundingBox.Max()));


                    fullBox.AddBoundingBox(newBox);
                    csRef<iMeshWrapperIterator> objectIter = engine->GetNearbyMeshes(mesh->GetMovable()->GetSectors()->Get(0), fullBox);
                    if(objectIter->HasNext())
                        hit = true;
                }
                if(!hit)
                    local_max_interval = delta;
            }

            ret = MoveV(local_max_interval);

            csVector3 displacement(fulltransf.GetOrigin() - oldpos);

            displacement = fulltransf.Other2ThisRelative(displacement);

            // Check the invariants still hold otherwise we may jump walls
            if(!(fabs(displacement.x) <= intervalSize.x))
            {
                printf("X (%g) out of bounds when performing CD > %g!\n", fabs(displacement.x), intervalSize.x);
                CS_ASSERT(false);
            }
            if(!(fabs(displacement.z) <= intervalSize.z))
            {
                printf("Z (%g) out of bounds when performing CD > %g!\n", fabs(displacement.y), intervalSize.y);
                CS_ASSERT(false);
            }
            if(!(fabs(displacement.y) <= intervalSize.y))
            {
                printf("Y (%g) out of bounds when performing CD > %g!\n", fabs(displacement.z), intervalSize.z);
                CS_ASSERT(false);
            }

            RotateV(local_max_interval);

            // We must update the transform after every rotation!
            fulltransf = mesh->GetMovable()->GetFullTransform();

            if(ret == PS_MOVE_FAIL)
                return ret;

            // The velocity may have changed by now
            bodyVel = fulltransf.Other2ThisRelative(velWorld) + velBody;

            delta -= local_max_interval;
            local_max_interval = csMin(csMin((bodyVel.y==0.0f)
                                             ? MAX_CD_INTERVAL
                                             : ABS(intervalSize.y/bodyVel.y), (bodyVel.x==0.0f)
                                             ? MAX_CD_INTERVAL
                                             : ABS(intervalSize.x/bodyVel.x)), (bodyVel.z==0.0f)
                                       ? MAX_CD_INTERVAL
                                       : ABS(intervalSize.z/bodyVel.z));
            // Err on the side of safety (95% error margin)
            local_max_interval *= 0.95f;

            // Sanity check on time interval here.  Something is messing it up. -KWF
            // local_max_interval = csMax(local_max_interval, 0.1F);
        }
    }

    if(!colldet || delta)
    {
        ret = MoveV(delta);
        RotateV(delta);
    }

    return ret;
}

// Apply the gradual offset correction from SetSoftDRUpdate to the mesh position
void psLinearMovement::OffsetSprite(float delta)
{
    if(offset_err.IsZero()) return;    // no offset correction to perform

    iMovable* movable = mesh->GetMovable();
    csVector3 oldpos = movable->GetPosition();
    csVector3 newpos;

    csVector3 del_offset = offset_rate;
    del_offset *= delta;

    // Check for NaN conditions:
    if(del_offset.x != del_offset.x) del_offset.x = 0.0f;
    if(del_offset.y != del_offset.y) del_offset.y = 0.0f;
    if(del_offset.z != del_offset.z) del_offset.z = 0.0f;

    // Calculate error correction for this time interval
    if((del_offset.x > offset_err.x && del_offset.x > 0.0f) ||
            (del_offset.x < offset_err.x && del_offset.x < 0.0f))
    {
        del_offset.x = offset_err.x;
        offset_rate.x = 0.0f;
    }
    if((del_offset.y > offset_err.y && del_offset.y > 0.0f) ||
            (del_offset.y < offset_err.y && del_offset.y < 0.0f))
    {
        del_offset.y = offset_err.y;
        offset_rate.y = 0.0f;
    }
    if((del_offset.z > offset_err.z && del_offset.z > 0.0f) ||
            (del_offset.z < offset_err.z && del_offset.z < 0.0f))
    {
        del_offset.z = offset_err.z;
        offset_rate.z = 0.0f;
    }
    offset_err -= del_offset;

    newpos = oldpos + del_offset;

    movable->GetTransform().SetOrigin(newpos);
}

// Do the actual move
int psLinearMovement::MoveV(float delta)
{
    if(velBody < SMALL_EPSILON && velWorld < SMALL_EPSILON && (!colldet || colldet->IsOnGround()))
        return PS_MOVE_DONTMOVE;  // didn't move anywhere

    int ret = PS_MOVE_SUCCEED;
    iMovable* movable = mesh->GetMovable();
    if(movable->GetSectors()->GetCount() <= 0)
        return PS_MOVE_DONTMOVE;  // didn't move anywhere

    csMatrix3 mat;

    // To test collision detection we use absolute position and transformation
    // (this is relevant if we are anchored). Later on we will correct that.
    csReversibleTransform fulltransf = movable->GetFullTransform();
    mat = fulltransf.GetT2O();

    csVector3 worldVel(fulltransf.This2OtherRelative(velBody) + velWorld);
    csVector3 oldpos(fulltransf.GetOrigin());
    csVector3 newpos(worldVel*delta + oldpos);
    csVector3 bufpos = newpos;
    float dist = (newpos - oldpos).Norm();

    // @@@ Magodra: In some cases the newpos seams to be invalid. Not sure about
    //              the reason, but the FollowSegment function will not work later
    //              in this function. So check for that condidtion, give warning,
    //              and halt the movement.
    if(CS::IsNaN(newpos.x) || CS::IsNaN(newpos.y) || CS::IsNaN(newpos.z))
    {
        printf("From old position %s ",toString(oldpos,movable->GetSectors()->Get(0)).GetDataSafe());
        StackTrace("LinearMovement to a NAN position.");
        return PS_MOVE_DONTMOVE;  // didn't move anywhere
    }

    // Check for collisions and adjust position
    if(colldet)
    {
        if(!colldet->AdjustForCollisions(oldpos, newpos, worldVel,
                                         delta, movable))
        {
            ret = PS_MOVE_FAIL;
            newpos = oldpos;
        }
        else
        {
            // @@@ Magodra 20140101 Seams that we get a lot of false
            // partials due to slopes that count for more than 1/10 of the
            // distance. Add a new 2d test to verify partial horizontal movement
            // before returning partial.

            // check if we collided, did move less than 9/10 of the distance
            //            if((newpos - bufpos).Norm() > dist/10.0)
            //{
            csVector3 newpos2d(newpos.x,0,newpos.z);
            csVector3 bufpos2d(bufpos.x,0,bufpos.z);
            if ((newpos2d - bufpos2d).Norm() > dist/10.0)
            {
                ret = PS_MOVE_PARTIAL;
            }
            //}
        }
    }

    csVector3 origNewpos = newpos;
    bool mirror = false;

    // Update position to account for portals
    iSector* new_sector = movable->GetSectors()->Get(0);
    iSector* old_sector = new_sector;

    // @@@ Jorrit: had to do this add!
    // We need to measure slightly above the position of the actor or else
    // we won't really cross a portal.
    float height5 = (bottomSize.y + topSize.y) / 10.0f;
    newpos.y += height5;
    csMatrix3 id;
    csOrthoTransform transform_oldpos(id, oldpos +
                                      csVector3(0.0f, height5, 0.0f));

    new_sector = new_sector->FollowSegment(transform_oldpos, newpos, mirror,
                                           PS_LINMOVE_FOLLOW_ONLY_PORTALS);
    newpos.y -= height5;
    if(new_sector != old_sector)
        movable->SetSector(new_sector);

    portalDisplaced += newpos - origNewpos;
    if(!IsOnGround())
    {
        //printf("Applying gravity: velY: %g.\n", velWorld.y);
        // gravity! move down!
        velWorld.y  -= gravity * delta;
        /*
         * Terminal velocity
         *   ((120 miles/hour  / 3600 second/hour) * 5280 feet/mile)
         *   / 3.28 feet/meter = 53.65 m/s
         */
        // The body velocity is figured in here too.
        if(velWorld.y < 0)
        {

            if(fulltransf.This2OtherRelative(velBody).y
                    + velWorld.y < -(ABS_MAX_FREEFALL_VELOCITY))
                velWorld.y = -(ABS_MAX_FREEFALL_VELOCITY)
                             - fulltransf.This2OtherRelative(velBody).y;
            if(velWorld.y > 0)
            {
                // printf("Reset other y %g\n", fulltransf.This2OtherRelative (velBody).y);
                velWorld.y = 0;
            }
        }
    }
    else
    {
        if(velWorld.y < 0)
        {
            velWorld.y = 0;
        }

        if(hugGround)
            HugGround(newpos, new_sector);
    }

    // Move to the new position. If we have an anchor we have to convert
    // the new position from absolute to relative.
    movable->GetTransform().SetOrigin(newpos);
    movable->GetTransform().SetT2O(
        movable->GetTransform().GetT2O() * transform_oldpos.GetT2O());

    if(colldet)
    {
        // Part 4: Add us to all nearby sectors.
        mesh->PlaceMesh();
    }

    movable->UpdateMove();

    return ret;
}

void psLinearMovement::HugGround(const csVector3 &pos, iSector* sector)
{
    csVector3 start, end;
    csIntersectingTriangle closest_tri;
    csVector3 isect[4];
    csPlane3 plane;
    bool hit[4];

    // Set minimum base dimensions of 0.5x0.5 for good aesthetics
    float legsXlimit = csMax(bottomSize.x / 2, 0.5f);
    float legsZlimit = csMax(bottomSize.z / 2, 0.5f);

    start.y = pos.y + shift.y + 0.01;

    // Assuming the bounding box is axis-aligned: (Lower-left point)
    start.x = pos.x - legsXlimit;
    start.z = pos.z - legsZlimit;

    end = start;
    end.y -= 5;

    hit[0] = csColliderHelper::TraceBeam(cdsys, sector, start, end,
                                         false, closest_tri, isect[0]) != -1;

    // Assuming the bounding box is axis-aligned: (Upper-left point)
    start.x = pos.x - legsXlimit;
    start.z = pos.z + legsZlimit;

    end = start;
    end.y -= 5;

    hit[1] = csColliderHelper::TraceBeam(cdsys, sector, start, end,
                                         false, closest_tri, isect[1]) != -1;

    // Assuming the bounding box is axis-aligned: (Upper-right point)
    start.x = pos.x + legsXlimit;
    start.z = pos.z + legsZlimit;

    end = start;
    end.y -= 5;

    hit[2] = csColliderHelper::TraceBeam(cdsys, sector, start, end,
                                         false, closest_tri, isect[2]) != -1;

    // Assuming the bounding box is axis-aligned: (Lower-right point)
    start.x = pos.x + legsXlimit;
    start.z = pos.z - legsZlimit;

    end = start;
    end.y -= 5;

    hit[3] = csColliderHelper::TraceBeam(cdsys, sector, start, end,
                                         false, closest_tri, isect[3]) != -1;

    //printf("Isect (%f %f %f %f)\n",hit[0] ? isect[0].y : -999, hit[1] ? isect[1].y : -999, hit[2] ? isect[2].y: -999, hit[3] ? isect[3].y: -999);

    int notHit = 0;
    int lowest = -1;
    for(int i = 0; i < 4 && notHit <= 1; i++)
    {
        if(!hit[i])
        {
            notHit++;
            lowest = i;
            continue;
        }
        if(notHit == 0)
        {
            if(lowest == -1)
                lowest = i;
            else if(isect[lowest].y > isect[i].y)
                lowest = i;
        }
    }
    if(notHit <= 1)
    {
        switch(lowest)
        {
            case 0:
                plane.Set(isect[1], isect[2], isect[3]);
                break;
            case 1:
                plane.Set(isect[0], isect[2], isect[3]);
                break;
            case 2:
                plane.Set(isect[0], isect[1], isect[3]);
                break;
            case 3:
                plane.Set(isect[0], isect[1], isect[2]);
                break;
        }
        csVector3 normal = plane.GetNormal().Unit();

        float newxRot = atan2(normal.z, normal.y);
        float newzRot = -atan2(normal.x, normal.y);
        csMatrix3 rotMat = csZRotMatrix3(newzRot) * csXRotMatrix3(newxRot - xRot)
                           * csZRotMatrix3(-zRot);
        mesh->GetMovable()->Transform(rotMat);
        xRot = newxRot;
        zRot = newzRot;
    }
}

void psLinearMovement::UpdateDRDelta(csTicks ticksdelta)
{
    float delta = ticksdelta;
    delta /= 1000;
    ExtrapolatePosition(delta);
    // lastDRUpdate += ticksdelta;  The way this fn is used, it should not update this var
}


void psLinearMovement::UpdateDR(csTicks ticks)
{
    if(lastDRUpdate)  // first time through gives huge deltas without this
    {
        float delta = ticks - lastDRUpdate;
        delta /= 1000;
        ExtrapolatePosition(delta);
    }
    lastDRUpdate = ticks;
}

void psLinearMovement::UpdateDR()
{
    csTicks time = csGetTicks();
    if(lastDRUpdate)
    {
        float delta = time - lastDRUpdate;
        delta /= 1000;
        ExtrapolatePosition(delta);
    }
    lastDRUpdate = time;
}

csTicks psLinearMovement::TimeDiff()
{
    return csGetTicks() - lastDRUpdate;
}

int psLinearMovement::ExtrapolatePosition(float delta)
{
    if(path)
    {
        path_time += delta;
        path->CalculateAtTime(path_time);
        csVector3 pos, look, up;

        path->GetInterpolatedPosition(pos);
        path->GetInterpolatedUp(up);
        path->GetInterpolatedForward(look);

        mesh->GetMovable()->GetTransform().SetOrigin(pos);
        mesh->GetMovable()->GetTransform().LookAt(
            look.Unit(), up.Unit());
        mesh->GetMovable()->UpdateMove();

        csRef<iSprite3DState> spstate =
            scfQueryInterface<iSprite3DState> (mesh->GetMeshObject());

        if(spstate && strcmp(path_actions[path->GetCurrentIndex()],
                             spstate->GetCurAction()->GetName()))
        {
            spstate->SetAction(path_actions[path->GetCurrentIndex()]);
        }
        if(path_time>path->GetTime(path->Length() - 1))
        {
            path = 0;
            path_time = 0;
        }
        return PS_MOVE_SUCCEED;
    }
    else
    {
        return MoveSprite(delta);
    }
}

void psLinearMovement::TickEveryFrame()
{
    if(!mesh)
    {
        return;
    }

    csTicks elapsed_time = vc->GetElapsedTicks();
    if(!elapsed_time)
        return;

    float delta = elapsed_time / 1000.0f;
    // Compensate for offset
    OffsetSprite(delta);
    if(fabsf(deltaLimit) > SMALL_EPSILON)
        delta = csMin(delta, deltaLimit);

    // Adjust the properties.
    ExtrapolatePosition(delta);
}



void psLinearMovement::GetCDDimensions(csVector3 &body, csVector3 &legs,
                                       csVector3 &shift)
{
    body = topSize;
    legs = bottomSize;
    shift = psLinearMovement::shift;
}


bool psLinearMovement::InitCD(const csVector3 &body, const csVector3 &legs,
                              const csVector3 &shift, iMeshWrapper* meshWrap)
{
    mesh = meshWrap;

    topSize = body;
    bottomSize = legs;

    if(bottomSize.x * bottomSize.y > (0.8f * 1.4f + 0.1f))
        hugGround = true;

    intervalSize.x = csMin(topSize.x, bottomSize.x);
    intervalSize.y = csMin(topSize.y, bottomSize.y);
    intervalSize.z = csMin(topSize.z, bottomSize.z);

    float maxX = csMax(body.x, legs.x)+shift.x;
    float maxZ = csMax(body.z, legs.z)+shift.z;


    float bX2 = body.x / 2.0f;
    float bZ2 = body.z / 2.0f;
    float bYbottom = legs.y;
    float bYtop = legs.y + body.y;

    csBox3 top(csVector3(-bX2, bYbottom, -bZ2) + shift,
               csVector3(bX2, bYtop, bZ2) + shift);

    float lX2 = legs.x / 2.0f;
    float lZ2 = legs.z / 2.0f;

    csBox3 bot(csVector3(-lX2, 0, -lZ2) + shift,
               csVector3(lX2, 0 + legs.y, lZ2) + shift);

    boundingBox.Set(csVector3(-maxX / 2.0f, 0, -maxZ / 2.0f) + shift,
                    csVector3(maxX / 2.0f, bYtop, maxZ / 2.0f) + shift);

    psLinearMovement::shift = shift;

    cdsys = csQueryRegistry<iCollideSystem> (object_reg);

    if(colldet)
        delete colldet;

    colldet = new psCollisionDetection(object_reg);
    return colldet->Init(topSize, bottomSize, shift, mesh);
}

bool psLinearMovement::IsOnGround() const
{
    if(colldet)
        return colldet->IsOnGround();

    return true;
}

/**
 * WARNING:  At present time this function does not work correctly!
 * <p>
 * The underlying function csEngine::GetNearbySectors () is not implemented.
 * Instead it returns only your current sector.
 */
int psLinearMovement::FindSectors(const csVector3 &pos, float radius,
                                  iSector** sectors)
{
    int numsector = 0;

    csRef<iSectorIterator> sectorit =
        engine->GetNearbySectors(GetSector(), pos, radius);

    iSector* sector;
    while(sectorit->HasNext())
    {
        sector = sectorit->Next();
        sectors[numsector++] = sector;
        if(numsector >= MAXSECTORSOCCUPIED)
            break;
    }

    return numsector;
}

void psLinearMovement::GetDRData(bool &on_ground,
                                 csVector3 &pos, float &yrot, iSector* &sector, csVector3 &vel,
                                 csVector3 &worldVel, float &ang_vel)
{
    on_ground = IsOnGround();
    GetLastPosition(pos, yrot, sector);
    vel = velBody;
    ang_vel = angularVelocity.y;
    worldVel = this->velWorld;
}


iSector* psLinearMovement::GetSector() const
{
    if(mesh->GetMovable()->GetSectors()->GetCount() > 0)
    {
        return mesh->GetMovable()->GetSectors()->Get(0);
    }
    else
    {
        printf("psLinearMovement::GetSector() tried to get a sector where none were available");
        return NULL;
    }
}

// --------------------------------------------------------------------------

void psLinearMovement::SetPathAction(int which, const char* action)
{
    path_actions.Put(which,action);
}

//#define DRDBG(X) printf ("DR: [ %f ] : %s\n", delta, X);
#define DRDBG(x)


float psLinearMovement::GetYRotation() const
{
    // user will get a warning and a nothing if theres no mesh
    if(!mesh)  return 0.0;
    const csMatrix3 &transf = mesh->GetMovable()
                              ->GetTransform().GetT2O();
    return Matrix2YRot(transf);
}

void psLinearMovement::SetYRotation(float yrot)
{
    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3(yrot);
    mesh->GetMovable()->GetTransform().SetO2T(matrix);

    mesh->GetMovable()->UpdateMove();
}


const csVector3 psLinearMovement::GetPosition() const
{
    // user will get a warning and a nothing if theres no mesh
    if(!mesh)  return csVector3(0);
    return mesh->GetMovable()->GetPosition();
}

const csVector3 psLinearMovement::GetFullPosition() const
{
    // user will get a warning and a nothing if theres no mesh
    if(!mesh)  return csVector3(0);
    return mesh->GetMovable()->GetFullPosition();
}

void psLinearMovement::GetLastPosition(csVector3 &pos, float &yrot,
                                       iSector* &sector) const
{
    if(!mesh)  return;

    // Position
    pos = GetPosition();

    // rotation
    yrot = GetYRotation();

    // Sector
    sector = GetSector();
}

void psLinearMovement::GetLastClientPosition(csVector3 &pos, float &yrot,
        iSector* &sector)
{
    if(!mesh)  return;

    // Position
    pos = lastClientPosition;

    // rotation
    yrot = lastClientYrot;

    // Sector
    sector = lastClientSector;
}

void psLinearMovement::GetLastFullPosition(csVector3 &pos, float &yrot,
        iSector* &sector)
{
    if(!mesh)  return;

    // Position
    pos = GetFullPosition();

    // rotation
    yrot = GetYRotation();

    // Sector
    sector = GetSector();
}

void psLinearMovement::SetFullPosition(const csVector3 &pos, float yrot,
                                       const iSector* sector)
{
    // Position
    csVector3 newpos;

    if(!sector)
    {
        StackTrace("Setting position without sector");
    }


    newpos = pos;
    mesh->GetMovable()->SetPosition((iSector*)sector, newpos);

    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3(yrot);
    // @@@ Not correct if anchor is transformed!!!
    mesh->GetMovable()->GetTransform().SetO2T(matrix);

    // Sector
    mesh->GetMovable()->UpdateMove();
}

void psLinearMovement::SetFullPosition(const char* center_name, float yrot,
                                       iSector* sector)
{
    csRef<iMapNode> mapnode = CS::GetNamedChildObject<iMapNode>(sector->QueryObject(), center_name);
    if(mapnode)
    {
        SetFullPosition(mapnode->GetPosition(), yrot, sector);
    }
}

void psLinearMovement::SetPosition(const csVector3 &pos, float yrot,
                                   const iSector* sector)
{
    if(!sector)
    {
        StackTrace("Setting position without sector");
    }

    Debug4(LOG_CELPERSIST,0,"DEBUG: psLinearMovement::SetPosition %s current transform: %s scale %f \n", mesh->QueryObject()->GetName(), mesh->GetMovable()->GetTransform().Description().GetData(),scale);

    // Position and Sector
    mesh->GetMovable()->SetPosition((iSector*)sector,pos);

    // at first loading scale may not be yet set
    if(scale>0)
    {
        // Rotation and scale
        csMatrix3 rotMatrix = (csMatrix3) csYRotMatrix3(yrot);
        csMatrix3 scaleMatrix = csMatrix3(1/scale,0,0, 0,1/scale,0, 0,0,1/scale);
        mesh->GetMovable()->GetTransform().SetO2T(scaleMatrix*rotMatrix);
    }
    else
    {
        // Rotation only
        csMatrix3 rotMatrix = (csMatrix3) csYRotMatrix3(yrot);
        mesh->GetMovable()->GetTransform().SetO2T(rotMatrix);
    }

    mesh->GetMovable()->UpdateMove();
}

void psLinearMovement::SetPosition(const char* center_name, float yrot,
                                   iSector* sector)
{
    csRef<iMapNode> mapnode = CS::GetNamedChildObject<iMapNode>(sector->QueryObject(), center_name);
    if(mapnode)
    {
        SetPosition(mapnode->GetPosition(), yrot, sector);
    }
}

void psLinearMovement::SetDRData(bool on_ground,
                                 csVector3 &pos, float yrot, iSector* sector, csVector3 &vel,
                                 csVector3 &worldVel, float ang_vel)
{
    if(colldet)
    {
        colldet->SetOnGround(on_ground);
    }
    SetPosition(pos,yrot,sector);
    SetVelocity(vel);
    ClearWorldVelocity();
    AddVelocity(worldVel);
    csVector3 rot(0.0f, ang_vel, 0.0f);
    SetAngularVelocity(rot);
    lastDRUpdate = csGetTicks();
    lastClientDRUpdate = lastDRUpdate;
    lastClientPosition = pos;
    lastClientSector = sector;
    lastClientYrot = yrot;
}

void psLinearMovement::SetSoftDRData(bool on_ground,
                                     csVector3 &pos, float yrot, iSector* sector, csVector3 &vel,
                                     csVector3 &worldVel, float ang_vel)
{
    if(colldet)
        colldet->SetOnGround(on_ground);

    csVector3 cur_pos;
    float cur_rot;
    iSector* cur_sect;
    GetLastPosition(cur_pos, cur_rot, cur_sect);
    if(cur_sect == sector)
    {
        offset_err = pos - cur_pos;
        // Check for NaN conditions:
        if(offset_err.x != offset_err.x) offset_err.x = 0.0f;
        if(offset_err.y != offset_err.y) offset_err.y = 0.0f;
        if(offset_err.z != offset_err.z) offset_err.z = 0.0f;
        offset_rate = offset_err;
        SetPosition(cur_pos, yrot, sector);
    }
    else
    {
        offset_rate = offset_err = csVector3(0.0f, 0.0f ,0.0f);
        SetPosition(pos, yrot, sector);
    }

    SetVelocity(vel);
    ClearWorldVelocity();
    AddVelocity(worldVel);
    csVector3 rot(0.0f, ang_vel, 0.0f);
    SetAngularVelocity(rot);
    lastDRUpdate = csGetTicks();
}


void psLinearMovement::SetVelocity(const csVector3 &vel)
{
    // Y movement here is NOT lift and gravity effects. It IS for
    // jumping & jetpacks.
    velBody = vel;
}

void psLinearMovement::AddVelocity(const csVector3 &vel)
{
    // -107 is terminal velocity
    if(vel.Norm() > 110)
        printf("Garbage data in AddVel!\n");

    // Y movement here can be used for lift and gravity effects.
    velWorld += vel;
}

void psLinearMovement::ClearWorldVelocity()
{
    // Y movement here can be used for lift and gravity effects.
    velWorld = 0.0f;
}

void psLinearMovement::GetVelocity(csVector3 &v) const
{
    v = GetVelocity();
}

const csVector3 psLinearMovement::GetVelocity() const
{
    csVector3 velworld = mesh->GetMovable()->GetTransform().Other2ThisRelative(velWorld);

    // Return the composite of the object and world velocity
    // in the OBJECT coordinate system.
    return velworld + velBody;
}

const csVector3 psLinearMovement::GetAngularVelocity() const
{
    return angularVelocity;
}

void psLinearMovement::GetAngularVelocity(csVector3 &v) const
{
    v = angularVelocity;
}

void psLinearMovement::SetOnGround(bool onground)
{
    if(colldet)
    {
        colldet->SetOnGround(onground);
    }
}

void psLinearMovement::SetHugGround(bool hugGround)
{
    this->hugGround = hugGround;
}

void psLinearMovement::SetGravity(float grav)
{
    gravity = grav;
}

float psLinearMovement::GetGravity()
{
    return gravity;
}

void psLinearMovement::ResetGravity()
{
    gravity = 19.6f;
}


bool psLinearMovement::IsPath() const
{
    return (path != 0);
}

void psLinearMovement::SetPath(iPath* newpath)
{
    path = newpath;
    path_sent = false;
}

void psLinearMovement::SetPathTime(float timeval)
{
    path_time = timeval;
}

void psLinearMovement::SetPathSpeed(float speed)
{
    path_speed = speed;
}

void psLinearMovement::SetPathSector(const char* sectorname)
{
    path_sector = sectorname;
}

void psLinearMovement::SetDeltaLimit(float deltaLimit)
{
    this->deltaLimit = deltaLimit;
}

csVector3 psLinearMovement::GetPortalDisplacement()
{
    return portalDisplaced;
}

void psLinearMovement::ClearPortalDisplacement()
{
    portalDisplaced = 0.0f;
}

void psLinearMovement::UseCD(bool cd)
{
    if(colldet)
    {
        colldet->UseCD(cd);
    }
}

void psLinearMovement::StackTrace(const char* error)
{
    printf("%s\n",error);
    csCallStack* stack = csCallStackHelper::CreateCallStack();
    if(stack)
    {
        stack->Print();
        stack->Free();
    }
}
