/*
 * PaladinJr.cpp - Author: Andrew Dai
 * CoAuthor: Matthieu Kraus
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#include <psconfig.h>
#include <iengine/movable.h>

#include <net/netbase.h>
#include <iutil/object.h>
#include "util/serverconsole.h"
#include "gem.h"
#include "client.h"
#include "clients.h"
#include "globals.h"
#include "psserver.h"
#include "playergroup.h"
#include "engine/linmove.h"
#include "cachemanager.h"
#include "engine/psworld.h"
#include "entitymanager.h"

#include "paladinjr.h"

/*
 * This is the maximum ms of latency that a client is presumed to have.
 */
#define MAX_ACCUMULATED_LAG 10000

void PaladinJr::Initialize(EntityManager* celbase, CacheManager* cachemanager)
{
    iConfigManager* configmanager = psserver->GetConfig();
    enforcing = configmanager->GetBool("PlaneShift.Paladin.Enforcing");
    warnCount = configmanager->GetInt("PlaneShift.Paladin.Cheat.WarningCount", 5);
    maxCount = configmanager->GetInt("PlaneShift.Paladin.Cheat.MaximumCount", 10);

    checks = NOVIOLATION;
    if(configmanager->GetBool("PlaneShift.Paladin.Check.Speed"))
    {
        checks |= SPEEDVIOLATION;
    }
    if(configmanager->GetBool("PlaneShift.Paladin.Check.Distance"))
    {
        checks |= DISTVIOLATION;
    }
    if(configmanager->GetBool("PlaneShift.Paladin.Check.Warp"))
    {
        checks |= WARPVIOLATION;
    }
    if(configmanager->GetBool("PlaneShift.Paladin.Check.CD"))
    {
        checks |= CDVIOLATION;
    }

    // if we have a check enabled - enable paladin
    enabled = (checks != NOVIOLATION);

    const csPDelArray<psCharMode> &modes = cachemanager->GetCharModes();
    const csPDelArray<psMovement> &moves = cachemanager->GetMovements();

    maxVelocity.Set(0.0f);
    csVector3 maxMod(1);

    for(size_t i = 0; i < moves.GetSize(); i++)
    {
        maxVelocity.x = csMax(maxVelocity.x, moves[i]->base_move.x);
        maxVelocity.y = csMax(maxVelocity.y, moves[i]->base_move.y);
        maxVelocity.z = csMax(maxVelocity.z, moves[i]->base_move.z);
    }
    for(size_t i = 0; i < modes.GetSize(); i++)
    {
        maxMod.x *= csMax(1.0f, modes[i]->move_mod.x);
        maxMod.y *= csMax(1.0f, modes[i]->move_mod.y);
        maxMod.z *= csMax(1.0f, modes[i]->move_mod.z);
    }
    maxVelocity.x *= maxMod.x;
    maxVelocity.y *= maxMod.y;
    maxVelocity.z *= maxMod.z;

    // Running forward while strafing on a mount
    maxSpeed = sqrtf(maxVelocity.z * maxVelocity.z + maxVelocity.x * maxVelocity.x);
    //maxSpeed = 1;

    watchTime = configmanager->GetInt("PlaneShift.Paladin.WatchTime", 30000);

    target = NULL;
    entitymanager = celbase;
}

bool PaladinJr::ValidateMovement(Client* client, gemActor* actor, psDRMessage &currUpdate)
{
    if(!enabled)
        return true;

    // Don't check GMs/Devs
    //if(client->GetSecurityLevel())
    //    return;

    // Speed check always enabled
    if(!SpeedCheck(client, actor, currUpdate))
        return false;  // DON'T USE THIS CLIENT POINTER AGAIN

    checkClient = false;

    if(target && (csGetTicks() - started > watchTime))
    {
        checked.Add(target->GetClientNum());
        target = NULL;
        started = csGetTicks();
    }

    if(checked.In(client->GetClientNum()))
    {
        if(!target && csGetTicks() - started > PALADIN_MAX_SWITCH_TIME)
        {
            // We have checked every client online
            started = csGetTicks();
            target = client;
            lastUpdate = currUpdate;
#ifdef PALADIN_DEBUG
            CPrintf(CON_DEBUG, "Now checking client %d\n", target->GetClientNum());
#endif
            checked.DeleteAll();
        }
        return true;
    }

    if(!target)
    {
        started = csGetTicks();
        target = client;
        lastUpdate = currUpdate;
#ifdef PALADIN_DEBUG
        CPrintf(CON_DEBUG, "Now checking client %d\n", target->GetClientNum());
#endif
        return true;
    }
    else if(target != client)
        return true;

    float yrot;
    iSector* sector;


    actor->SetDRData(lastUpdate);

    origPos = lastUpdate.pos;
    vel = lastUpdate.vel;
    angVel = lastUpdate.ang_vel;

    //if (vel.x == 0 && vel.z == 0)
    //{
    //    // Minimum speed to cope with client-side timing discrepancies
    //    vel.x = vel.z = -1;
    //}

    // Paladin Jr needs CD enabled on the entity.
    //kwf client->GetActor()->pcmove->UseCD(true);
    actor->pcmove->SetVelocity(vel);

    // TODO: Assuming maximum lag, need to add some kind of lag prediction here.
    // Note this ignores actual DR packet time interval because we cannot rely
    // on it so must assume a maximal value.
    //
    // Perform the extrapolation here:
    actor->pcmove->UpdateDRDelta(2000);

    // Find the extrapolated position
    actor->pcmove->GetLastPosition(predictedPos,yrot,sector);

#ifdef PALADIN_DEBUG
    CPrintf(CON_DEBUG, "Predicted: pos.x = %f, pos.y = %f, pos.z = %f\n",predictedPos.x,predictedPos.y,predictedPos.z);
#endif

    maxmove = predictedPos-origPos;

    // No longer need CD checking
    actor->pcmove->UseCD(false);

    lastUpdate = currUpdate;
    checkClient = true;
    return true;
}

bool PaladinJr::CheckCollDetection(Client* client, gemActor* actor)
{
    if(!enabled || !checkClient || !(checks & CDVIOLATION) || client->GetSecurityLevel())
        return true;

    csVector3 pos;
    float yrot;
    iSector* sector;
    csVector3 posChange;


    actor->GetPosition(pos,yrot,sector);
    posChange = pos-origPos;

//#ifdef PALADIN_DEBUG
//    CPrintf(CON_DEBUG, "Actual: pos.x = %f, pos.y = %f, pos.z = %f\nDifference = %f\n",pos.x,pos.y,pos.z, (predictedPos - pos).Norm());
//        CPrintf(CON_DEBUG, "Actual displacement: x = %f y = %f z = %f\n", posChange.x, posChange.y, posChange.z);
//        CPrintf(CON_DEBUG, "Maximum extrapolated displacement: x = %f y = %f z = %f\n",maxmove.x, maxmove.y, maxmove.z);
//#endif

    // TODO:
    // Height checking disabled for now because jump data is not sent.
    if(fabs(posChange.x) > fabs(maxmove.x) || fabs(posChange.z) > fabs(maxmove.z))
    {
#ifdef PALADIN_DEBUG
        CPrintf(CON_DEBUG, "CD violation registered for client %s.\n", client->GetName());
        CPrintf(CON_DEBUG, "Details:\n");
        CPrintf(CON_DEBUG, "Original position: x = %f y = %f z = %f\n", origPos.x, origPos.y, origPos.z);
        CPrintf(CON_DEBUG, "Actual displacement: x = %f y = %f z = %f\n", posChange.x, posChange.y, posChange.z);
        CPrintf(CON_DEBUG, "Maximum extrapolated displacement: x = %f y = %f z = %f\n",maxmove.x, maxmove.y, maxmove.z);
        CPrintf(CON_DEBUG, "Previous velocity: x = %f y = %f z = %f\n", vel.x, vel.y, vel.z);
#endif

        csString buf;
        buf.Format("%s, %s, %s, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %s\n",
                   client->GetName(), "CD violation", sector->QueryObject()->GetName(),origPos.x, origPos.y, origPos.z,
                   maxmove.x, maxmove.y, maxmove.z, posChange.x, posChange.y, posChange.z, vel.x, vel.y, vel.z,
                   angVel.x, angVel.y, angVel.z, PALADIN_VERSION);
        psserver->GetLogCSV()->Write(CSV_PALADIN, buf);
    }

    return true;
}

bool PaladinJr::SpeedCheck(Client* client, gemActor* actor, psDRMessage &currUpdate)
{
    csVector3 oldpos;
    // Dummy variables
    float yrot;
    iSector* sector;
    psWorld* world = entitymanager->GetWorld();
    int violation = NOVIOLATION;

    actor->pcmove->GetLastClientPosition(oldpos, yrot, sector);

    // If no previous observations then we have nothing to check against.
    if(!sector)
        return true;

    // define cheating variables
    float dist = 0.0;
    float reported_distance = 0.0;
    float max_noncheat_distance = 0.0;
    float lag_distance = 0.0;
    csTicks timedelta = 0;
    csVector3 vel;

    // check for warpviolation
    if(sector != currUpdate.sector && !world->WarpSpace(sector, currUpdate.sector, oldpos))
    {
        if(checks & WARPVIOLATION)
        {
            violation = WARPVIOLATION;
        }
        else
        {
            // we don't do warp checking and crossed a sector
            // skip this round
            return true;
        }
    }

    if(checks & SPEEDVIOLATION)
    {
        // we don't use the absolute value of the vertical
        // speed in order to let falls go through
        if(fabs(currUpdate.vel.x) <= maxVelocity.x &&
                currUpdate.vel.y  <= maxVelocity.y &&
                fabs(currUpdate.vel.z) <= maxVelocity.z)
        {
            violation |= SPEEDVIOLATION;
        }
    }

    // distance check is skipped on warp violation as it would be wrong
    if(checks & DISTVIOLATION && !(violation & WARPVIOLATION))
    {
        dist = (currUpdate.pos-oldpos).Norm();
        timedelta = actor->pcmove->ClientTimeDiff();

        // We use the last reported vel, not the new vel, to calculate how far he should have gone since the last DR update
        vel = actor->pcmove->GetVelocity();
        vel.y = 0; // ignore vertical velocity
        reported_distance = vel.Norm()*timedelta/1000;

        Debug4(LOG_CHEAT, client->GetClientNum(),"Player went %1.3fm in %u ticks when %1.3fm was allowed.\n",dist, timedelta, reported_distance);

        max_noncheat_distance = maxSpeed*timedelta/1000;
        lag_distance          = maxSpeed*client->accumulatedLag/1000;

        if(dist < max_noncheat_distance + lag_distance)
        {
            if(dist == 0)
            {
                // player is stationary - reset accumulated lag
                NetBase::Connection* connection = client->GetConnection();
                client->accumulatedLag = connection->estRTT + connection->devRTT;
            }
            else if(fabs(dist-reported_distance) < dist/20)
            {
                // ignore jitter caused differences
                Debug1(LOG_CHEAT, client->GetClientNum(),"Ignoring lag jitter.");
            }
            else
            {
                // adjust accumulated lag
                float lag = (reported_distance - dist) * 1000.f/maxSpeed + client->accumulatedLag;

                // cap to meaningful values
                lag = lag < 0 ? 0 : lag > MAX_ACCUMULATED_LAG ? MAX_ACCUMULATED_LAG : lag;

                client->accumulatedLag = (csTicks)lag;

                Debug2(LOG_CHEAT, client->GetClientNum(),"Accumulated lag: %u\n",client->accumulatedLag);
            }
        }
        else
        {
            violation |= DISTVIOLATION;
        }
    }

    if(violation != NOVIOLATION)
    {
        if(client->GetCheatMask(MOVE_CHEAT))
        {
            //printf("Server has pre-authorized this apparent speed violation.\n");
            client->SetCheatMask(MOVE_CHEAT, false);  // now clear the Get Out of Jail Free card
            return true;  // not cheating
        }

        Debug6(LOG_CHEAT, client->GetClientNum(),"Went %1.2f in %u ticks when %1.2f was expected plus %1.2f allowed lag distance (%1.2f)\n", dist, timedelta, max_noncheat_distance, lag_distance, max_noncheat_distance+lag_distance);
        //printf("Z Vel is %1.2f\n", currUpdate.vel.z);
        //printf("MaxSpeed is %1.2f\n", maxSpeed);

        // Report cheater
        csVector3 angVel;
        csString buf;
        csString type;
        csString sectorName(sector->QueryObject()->GetName());

        // Player has probably been warped
        if(violation & WARPVIOLATION)
        {
            sectorName.Append(" to ");
            sectorName.Append(currUpdate.sectorName);
            type = "Warp Violation";
        }

        if(violation & SPEEDVIOLATION)
        {
            if(!type.IsEmpty())
                type += "|";
            type += "Speed Violation (Hack confirmed)";
        }

        if(violation & DISTVIOLATION)
        {
            if(!type.IsEmpty())
                type += "|";
            type += "Distance Violation";
        }

        if(enforcing)
        {
            actor->ForcePositionUpdate();
        }

        actor->pcmove->GetAngularVelocity(angVel);
        buf.Format("%s, %s, %s, %.3f %.3f %.3f, %.3f 0 %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %.3f %.3f %.3f, %s\n",
                   client->GetName(), type.GetData(), sectorName.GetData(),oldpos.x, oldpos.y, oldpos.z,
                   max_noncheat_distance, max_noncheat_distance,
                   currUpdate.pos.x - oldpos.x, currUpdate.pos.y - oldpos.y, currUpdate.pos.z - oldpos.z,
                   vel.x, vel.y, vel.z, angVel.x, angVel.y, angVel.z, PALADIN_VERSION);

        psserver->GetLogCSV()->Write(CSV_PALADIN, buf);

        Debug5(LOG_CHEAT, client->GetClientNum(),"Player %s traversed %1.2fm in %u msec with an accumulated lag allowance of %u ms. Cheat detected!\n",
               client->GetName(),dist,timedelta,client->accumulatedLag);

        client->CountDetectedCheat();
        //printf("Client has %d detected cheats now.\n", client->GetDetectedCheatCount());
        if(client->GetDetectedCheatCount() % warnCount == 0)
        {
            psserver->SendSystemError(client->GetClientNum(),"You have been flagged as using speed hacks.  You will be disconnected if you continue.");
        }
        if(client->GetDetectedCheatCount() >= maxCount)
        {
            //printf("Disconnecting a cheating client.\n");
            psserver->RemovePlayer(client->GetClientNum(),"Paladin has kicked you from the server for cheating.");
            return false;
        }

        return !enforcing;
    }
    else
    {
        return true;
    }
}

