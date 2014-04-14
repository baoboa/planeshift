/*
  * psserverdr.cpp by Matze Braun <MatzeBraun@gmx.de>
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/databuff.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <iutil/object.h>


//=============================================================================
// Project Includes
//=============================================================================
#include "net/message.h"
#include "net/msghandler.h"

#include "engine/linmove.h"

#include "util/serverconsole.h"
#include "util/log.h"
#include "util/psconst.h"
#include "util/mathscript.h"
#include "util/eventmanager.h"
#include "bulkobjects/pssectorinfo.h"


//=============================================================================
// Local Includes
//=============================================================================
#include "psserverdr.h"
#include "client.h"
#include "clients.h"
#include "tutorialmanager.h"
#include "events.h"
#include "psserver.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "gem.h"
#include "entitymanager.h"
#include "progressionmanager.h"
#include "adminmanager.h"
#include "paladinjr.h"
#include "psproxlist.h"
#include "globals.h"
#include "scripting.h"
#include "netmanager.h"

psServerDR::psServerDR(CacheManager* cachemanager, EntityManager* entitymanager)
{
    cacheManager = cachemanager;
    entityManager = entitymanager;
    paladin = NULL;
    calc_damage = psserver->GetMathScriptEngine()->FindScript("Calculate Fall Damage");
}

psServerDR::~psServerDR()
{
    delete paladin;
}

bool psServerDR::Initialize()
{
    Subscribe(&psServerDR::HandleDeadReckoning, MSGTYPE_DEAD_RECKONING, REQUIRE_READY_CLIENT);

    //load the script
    if(!calc_damage)
    {
        Error1("Failed to find MathScript >Calculate Fall Damage<!");
        return false;
    }

    paladin = new PaladinJr;
    paladin->Initialize(entityManager, cacheManager);

    return true;
}

void psServerDR::SendPersist()
{
    // no server side actions yet

    return;
}

void psServerDR::HandleFallDamage(gemActor* actor,int clientnum, const csVector3 &pos, iSector* sector)
{
    float fallHeight = actor->FallEnded(pos,sector);

    MathEnvironment env;
    env.Define("FallHeight", fallHeight);
    env.Define("Damage", 0.0); // Make sure damage is defined.

    calc_damage->Evaluate(&env);
    MathVar* var_fall_dmg = env.Lookup("Damage");
    float damage = var_fall_dmg->GetValue();
    Debug4(LOG_LINMOVE,actor->GetClientID(), "%s fell %.2fm for damage %.2f",
           actor->GetName(), fallHeight, damage);

    if(damage > 0)
    {
        bool died = (damage > actor->GetCharacterData()->GetHP());

        actor->DoDamage(NULL, damage);
        if(died)
        {
            //Client died
            psserver->SendSystemInfo(clientnum, "You fell down and died.");
        }
    }
}

void psServerDR::ResetPos(gemActor* actor)
{
    psserver->SendSystemInfo(actor->GetClient()->GetClientNum(), "Received out of bounds positional data, resetting your position.");

    iSector* targetSector;
    csVector3 targetPoint;
    csString targetSectorName;
    float yRot = 0;
    actor->GetPosition(targetPoint, yRot, targetSector);
    targetSectorName = targetSector->QueryObject()->GetObjectParent()->GetName();

    csString status;
    status.Format("Received out of bounds position for %s. Resetting to sector %s.", actor->GetName(), (const char*) targetSectorName);
    if(LogCSV::GetSingletonPtr())
        LogCSV::GetSingleton().Write(CSV_STATUS, status);

    psserver->GetAdminManager()->GetStartOfMap(actor->GetClient()->GetClientNum(), targetSectorName, targetSector, targetPoint);
    actor->pcmove->SetOnGround(true);
    actor->Teleport(targetSector, targetPoint, 0);
    actor->FallEnded(targetPoint, targetSector);
}

void psServerDR::HandleDeadReckoning(MsgEntry* me,Client* client)
{
    psDRMessage drmsg(me,psserver->GetNetManager()->GetAccessPointers());
    if(!drmsg.valid)
    {
        Error2("Received unparsable psDRMessage from client %u.\n",me->clientnum);
        return;
    }

    gemActor* actor = client->GetActor();

    if(actor == NULL)
    {
        Error1("Received DR data for NULL actor.");
        return;
    }

    if(actor->IsFrozen() || !actor->IsAllowedToMove())    // Is this movement allowed?
    {
        if(drmsg.worldVel.y > 0)
        {
            client->CountDetectedCheat();  // This DR data may be an exploit but may also be valid from lag.
            actor->pcmove->AddVelocity(csVector3(0,-1,0));
            actor->UpdateDR();
            actor->MulticastDRUpdate();
            return;
        }
    }

    if(drmsg.sector == NULL)
    {
        Error2("Client sent the server DR message with unknown sector \"%s\" !", drmsg.sectorName.GetData());
        psserver->SendSystemInfo(me->clientnum,
                                 "Received unknown sector \"%s\" - moving you to a valid position.",
                                 drmsg.sectorName.GetData());
        /* FIXME: Strangely, when logging on, the client ends up putting the
         * actor in SectorWhereWeKeepEntitiesResidingInUnloadedMaps, and send
         * a DR packet with (null) sector.  It then relies on the server to
         * bail it out with MoveToValidPos, which at this stage is the login
         * position the server tried to set in the first place. */
        actor->MoveToValidPos(true);
        return;
    }

    // These values must be sane or the proxlist will die.
    // The != test tests for NaN because if it is, the proxlist will mysteriously die (found out the hard way)
    if(drmsg.pos.x != drmsg.pos.x || drmsg.pos.y != drmsg.pos.y || drmsg.pos.z != drmsg.pos.z ||
            fabs(drmsg.pos.x) > 100000 || fabs(drmsg.pos.y) > 1000 || fabs(drmsg.pos.z) > 100000)
    {
        ResetPos(actor);
        return;
    }
    else if(drmsg.vel.x != drmsg.vel.x || drmsg.vel.y != drmsg.vel.y || drmsg.vel.z != drmsg.vel.z ||
            fabs(drmsg.vel.x) > 1000 || fabs(drmsg.vel.y) > 1000 || fabs(drmsg.vel.z) > 1000)
    {
        ResetPos(actor);
        return;
    }
    else if(drmsg.worldVel.x != drmsg.worldVel.x || drmsg.worldVel.y != drmsg.worldVel.y || drmsg.worldVel.z != drmsg.worldVel.z ||
            fabs(drmsg.worldVel.x) > 1000 || fabs(drmsg.worldVel.y) > 1000 || fabs(drmsg.worldVel.z) > 1000)
    {
        ResetPos(actor);
        return;
    }

    if(!paladin->ValidateMovement(client, actor, drmsg))
        // client has been disconnected or message was rejected
        return;

    // Go ahead and update the server version
    if(!actor->SetDRData(drmsg))  // out of date message if returns false
        return;

    // Check for Movement Tutorial Required.
    // Usually we don't want to check but DR msgs are so frequent,
    // perf hit is unacceptable otherwise.
    if(actor->GetCharacterData()->NeedsHelpEvent(TutorialManager::MOVEMENT))
    {
        if(!drmsg.vel.IsZero() || drmsg.ang_vel)
        {
            psMovementEvent evt(client->GetClientNum());
            evt.FireEvent();
        }
    }

    // Update falling status
    if(actor->pcmove->IsOnGround())
    {
        if(actor->IsFalling())
        {
            iSector* sector = actor->GetMeshWrapper()->GetMovable()->GetSectors()->Get(0);

            // GM flag
            if(!actor->safefall)
            {
                HandleFallDamage(actor,me->clientnum, drmsg.pos,sector);
            }
            else
            {
                actor->FallEnded(drmsg.pos,sector);
            }
        }
    }
    else if(!actor->IsFalling())
    {
        iSector* sector = actor->GetMeshWrapper()->GetMovable()->GetSectors()->Get(0);
        actor->FallBegan(drmsg.pos, sector);
    }

    //csTicks time = csGetTicks();
    actor->UpdateProxList();
    /*
    if (csGetTicks() - time > 500)
    {
        csString status;
        status.Format("Warning: Spent %u time updating proxlist for %s!", csGetTicks() - time, actor->GetName());
        psString out;
        actor->GetProxList()->DebugDumpContents(out);
        out.ReplaceAllSubString("\n", " ");

        status.Append(out);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    */

    // Now multicast to other clients
    psserver->GetEventManager()->Multicast(me,
                                           actor->GetMulticastClients(),
                                           me->clientnum,PROX_LIST_ANY_RANGE);

    paladin->CheckCollDetection(client, actor);

    // Swap lines for easy   Penalty testing. Old code use db to do this.
    //if (strcmp(drmsg.sector->QueryObject()->GetName(), "NPCroom1") == 0)
    psSectorInfo* sectorInfo = NULL;
    //get the sector info of this sector to check it's caracteristics
    if(drmsg.sector != NULL)
        sectorInfo = cacheManager->GetSectorInfoByName(drmsg.sector->QueryObject()->GetName());

    //is this a sector which teleports on entrance? (used for portals)
    if(sectorInfo && sectorInfo->GetIsTeleporting())
    {
        actor->pcmove->SetOnGround(false);

        //get the sector defined for teleport for later usage
        csString newSectorName = sectorInfo->GetTeleportingSector();

        //if the sector for teleport has data teleport to it
        if(newSectorName.Length())
            actor->Teleport(newSectorName, sectorInfo->GetTeleportingCord(), sectorInfo->GetTeleportingRot(), DEFAULT_INSTANCE);
        else //else just go to default (spawn)
            actor->MoveToSpawnPos();

        actor->StopMoving(true);

        //check if this sector triggered teleport has to warrant a death penalty
        if(sectorInfo->GetHasPenalty())
        {
            // should probably load this on startup.
            static ProgressionScript* death_penalty = NULL;
            if(!death_penalty)
                death_penalty = psserver->GetProgressionManager()->FindScript("death_penalty");

            if(death_penalty)
            {
                MathEnvironment env;
                env.Define("Actor", actor);
                death_penalty->Run(&env);
            }
        }
    }
}

