/*
* npc.cpp by Keith Fulton <keith@paqrat.com>
*
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iutil/object.h>
#include <csgeom/vector3.h>
#include <iengine/movable.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "engine/linmove.h"
#include "engine/psworld.h"

#include "util/log.h"
#include "util/psdatabase.h"
#include "util/location.h"
#include "util/strutil.h"
#include "util/waypoint.h"

#include "net/clientmsghandler.h"
#include "net/npcmessages.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "networkmgr.h"
#include "npc.h"
#include "npcclient.h"
#include "gem.h"
#include "globals.h"
#include "npcmesh.h"
#include "tribe.h"
#include "npcbehave.h"


void Stat::Update(csTicks now)
{
    // No point in updating when rate is not set
    if(rate == 0.0)
    {
        lastUpdate = now;
        return;
    }

    float delta = (now - lastUpdate)/1000.0;

    value += delta*rate;

    if(value < 0.0)
    {
        value = 0.0;
    }
    else if(value > max)
    {
        value = max;
    }

    lastUpdate = now;
}

void Stat::SetValue(float value, csTicks now)
{
    this->value = value;
    lastUpdate = now;
}

void Stat::SetMax(float max)
{
    this->max = max;
    if(value > max)
    {
        value = max;
    }
}

void Stat::SetRate(float rate, csTicks now)
{
    // Update value to now.
    Update(now);
    // Set new rate
    this->rate = rate;
}







NPC::NPC(psNPCClient* npcclient, NetworkManager* networkmanager, psWorld* world, iEngine* engine, iCollideSystem* cdsys)
    : DRcounter(0),
      lastDrPosition(0),
      lastDrSector(NULL),
      lastDrTime(csGetTicks()),
      lastDrMoving(false),
      lastDrYRot(0.0),
      lastDrVel(0),
      lastDrAngVel(0),
      spawnSector(NULL),
      checked(false),
      hatelist(npcclient, engine, world),
      tick(NULL)
{
    oldbrain=NULL;
    brain=NULL;
    pid=0;
    last_update=0;
    npcActor=NULL;
    movable=NULL;

    // Set up the locates
    activeLocate = new Locate();
    activeLocate->sector=NULL;
    activeLocate->angle=0.0;
    activeLocate->wp = NULL;
    activeLocate->radius = 0.0;
    // Store the active locate in the stored locates structure.
    storedLocates.PutUnique("Active", activeLocate);

    ang_vel=vel=999;
    walkVelocity=runVelocity=0.0; // Will be cached
    scale=1.0;
    region=NULL;
    insideRegion = true; // We assume that we start inside the region, if it exists
    insideTribeHome = true; // We assume that we start inside tribe home.
    last_perception=NULL;
    alive=false;
    tribe=NULL;
    raceInfo=NULL;
    checkedSector=NULL;
    checked = false;
    checkedResult = false;
    disabled = false;
    fallCounter = 0;
    owner_id = 0;
    target_id = 0;
    this->npcclient = npcclient;
    this->networkmanager = networkmanager;
    this->world = world;
    this->cdsys = cdsys;
    this->bufferMemory = NULL;
    this->buildingSpot = NULL;
}

NPC::~NPC()
{
    delete brain;
    delete oldbrain;
    delete last_perception;
    delete bufferMemory;

    // Cleare some cached values
    region = NULL;
    activeLocate->sector = NULL;
    activeLocate->wp = NULL;
    raceInfo = NULL;

    LocateHash::GlobalIterator iter = storedLocates.GetIterator();
    while(iter.HasNext())
        delete iter.Next();

    if(tick)
    {
        tick->Remove();
    }
}

void NPC::Tick()
{
    // If NPC is disabled it should not tick
    if(disabled)
        return;

    // Ensure NPC only has one tick at a time.
    CS_ASSERT(tick == NULL);

    csTicks now = csGetTicks();

    if(npcclient->IsReady())
    {
        ScopedTimer st(200, this); // Calls the ScopedTimerCallback on timeout

        Advance(now);  // Abstract event processing function
    }

    TickPostProcess(now);

    tick = new psNPCTick(NPC_BRAIN_TICK, this);
    tick->QueueEvent();
}

void NPC::TickPostProcess(csTicks when)
{
    if(!npcActor) return;

    psLinearMovement* linmove = GetLinMove();
    bool onGround;
    csVector3 myPos,myVel,worldVel;
    float myYRot,myAngVel;
    iSector* mySector;
    linmove->GetDRData(onGround,myPos,myYRot,mySector,myVel,worldVel,myAngVel);

    float distance = npcclient->GetWorld()->Distance(myPos, mySector, lastDrPosition, lastDrSector);


    NPCDebug(this, 11, "PostProcess: Dist: %.2f YRot: %.2f -> %.2f AngVel: %.2f -> %.2f Vel: %.2f",
             distance, lastDrYRot, myYRot, lastDrAngVel, myAngVel, (myVel-lastDrVel).Norm());

    // Check if there are any change in rotatation or velocity. Thies changes will influcence the visual effect
    // so update at once.
    if((fabs(myYRot - lastDrYRot) > 0.01) || (fabs(myAngVel - lastDrAngVel) > 0.01) || ((myVel-lastDrVel).Norm() > 0.1))
    {
        npcclient->GetNetworkMgr()->QueueDRData2(this);
        // lastDrPosition, lastDrSector, lastDrTime will be updated when actually sending the Dr to server
        // in the network manager.
        lastDrMoving = true;
    }
    else if((distance > 0.1))  // Moving
    {
        // At least queue once every 3 seconds when moving
        if((when - lastDrTime) > 3000)
        {
            npcclient->GetNetworkMgr()->QueueDRData2(this);
            // lastDrPosition, lastDrSector, lastDrTime will be updated when actually sending the Dr to server
            // in the network manager.
        }
        lastDrMoving = true;
    }
    else
    {
        // Update twice when stop moving...
        if(lastDrMoving && ((when - lastDrTime) > 1000))
        {
            lastDrMoving = false;
            npcclient->GetNetworkMgr()->QueueDRData(this);
        }
    }
}



void NPC::ScopedTimerCallback(const ScopedTimer* timer)
{
    CPrintf(CON_WARNING,"Used %u time to process tick for npc: %s(%s)\n",
            timer->TimeUsed(), GetName(), ShowID(GetEID()));
    Dump();
}

csString NPC::Info(const csString &infoRequestSubCmd)
{
    csString output;

    if(infoRequestSubCmd == "all" || infoRequestSubCmd == "summary")
    {
        DumpState(output);
        output.Append("\n");
    }

    if(infoRequestSubCmd == "all")
    {
        DumpBehaviorList(output);
        output.Append("\n");
    }

    if(infoRequestSubCmd == "all")
    {
        DumpReactionList(output);
        output.Append("\n");
    }

    if(infoRequestSubCmd == "all")
    {
        DumpHateList(output);
        output.Append("\n");
    }

    if(infoRequestSubCmd == "all")
    {
        DumpDebugLog(output);
        output.Append("\n");
    }

    if(infoRequestSubCmd == "all")
    {
        DumpMemory(output);
        output.Append("\n");
    }

    if(infoRequestSubCmd == "all")
    {
        DumpControlled(output);
        output.Append("\n");
    }


    if(infoRequestSubCmd == "old")
    {
        // Position and instance
        {
            csVector3 pos;
            iSector* sector;
            float rot = 0.0;
            InstanceID instance = INSTANCE_ALL;

            if(npcActor)
            {
                psGameObject::GetPosition(npcActor,pos,rot,sector);
                instance = npcActor->GetInstance();
            }
            output.AppendFmt(" Pos: %s Rot: %.2f Inst: %d\n",
                             npcActor?toString(pos,sector).GetDataSafe():"(none)", rot, instance);
        }
        output.AppendFmt(" DR Counter: %d ", DRcounter);
        output.AppendFmt("%s\n",disabled?"Disabled ":"");

        output.AppendFmt(" Active locate: ( Pos: %s Angle: %.1f deg Radius: %.2f WP: %s )\n",
                         toString(activeLocate->pos,activeLocate->sector).GetDataSafe(),
                         activeLocate->angle*180/PI,activeLocate->radius,
                         activeLocate->wp?activeLocate->wp->GetName():"(None)");
        output.AppendFmt(" Spawn pos: %s\n", toString(spawnPosition,spawnSector).GetDataSafe());
        if(GetOwner())
        {
            output.AppendFmt(" Owner: %s\n",GetOwnerName());
        }
        if(GetRegion())
        {
            output.AppendFmt(" Region( Name: %s Inside: %s )\n", GetRegion()->GetName(), insideRegion?"Yes":"No");
        }
        if(GetTribe())
        {
            output.AppendFmt(" Tribe( Name: %s Type: %s Inside home: %s )\n",
                             GetTribe()->GetName(),GetTribeMemberType().GetDataSafe(),insideTribeHome?"Yes":"No");
        }
        if(GetTarget())
        {
            output.AppendFmt(" Target: %s\n",GetTarget()->GetName());
        }
        output.AppendFmt(" Last perception: '%s'\n",last_perception?last_perception->GetName().GetDataSafe():"(None)");
        output.AppendFmt(" Fall counter: %d\n", GetFallCounter());
        output.AppendFmt(" Brain: '%s'\n",GetBrain()->GetName());
        output.AppendFmt(" Current Behavior: '%s'",GetCurrentBehavior()?GetCurrentBehavior()->GetName():"(None)");
    }

    return output;

}

void NPC::Dump()
{
    csString output;

    DumpState(output);
    output.Append("\n");

    DumpBehaviorList(output);
    output.Append("\n");

    DumpReactionList(output);
    output.Append("\n");

    DumpHateList(output);
    output.Append("\n");

    DumpDebugLog(output);
    output.Append("\n");

    DumpMemory(output);
    output.Append("\n");

    DumpControlled(output);
    output.Append("\n");

    // Display output to Cmd Output
    csStringArray lines(output, "\n");
    for(size_t i = 0; i < lines.GetSize(); i++)
    {
        CPrintf(CON_CMDOUTPUT,"%s\n",lines[i]);
    }
}

EID NPC::GetEID()
{
    return (npcActor ? npcActor->GetEID() : 0);
}

psLinearMovement* NPC::GetLinMove()
{
    if(npcActor)
    {
        return npcActor->pcmove;
    }
    return NULL;
}

void NPC::Load(const char* name, PID pid, NPCType* type, const char* region_name, int debugging, bool disabled, EventManager* eventmanager)
{
    this->name = name;
    this->pid = pid;
    this->type = type->GetName();
    this->region_name = region_name;
    SetDebugging(debugging);
    this->disabled = disabled;
    this->brain = new NPCType(*type, this);
    origtype = type->GetName();
}

Behavior* NPC::GetCurrentBehavior()
{
    return brain->GetCurrentBehavior();
}

NPCType* NPC::GetBrain()
{
    return brain;
}

void NPC::SetBrain(NPCType* type)
{
    delete oldbrain;
    oldbrain = brain;
    this->type = type->GetName();
    this->brain = new NPCType(*type, this);

}

bool NPC::Load(iResultRow &row, csHash<NPCType*, const char*> &npctypes, EventManager* eventmanager, PID usePID)
{
    name = row["name"];
    if(usePID.IsValid())
    {
        pid = usePID;
    }
    else
    {
        pid   = row.GetInt("char_id");
        if(pid == 0)
        {
            Error2("NPC '%s' has no id attribute. Error in XML",name.GetDataSafe());
            return false;
        }
    }

    type = row["npctype"];
    if(type.Length() == 0)
    {
        Error3("NPC '%s'(%s) has no type attribute. Error in XML",name.GetDataSafe(),ShowID(pid));
        return false;
    }

    region_name = row["region"]; // optional

    NPCType* t = npctypes.Get(type, NULL);
    if(!t)
    {
        Error4("NPC '%s'(%s) type '%s' is not found. Error in XML",name.GetDataSafe(),ShowID(pid),(const char*)type);
        return false;
    }

    if(row.GetFloat("ang_vel_override") != 0.0)
    {
        ang_vel = row.GetFloat("ang_vel_override");
    }

    if(row.GetFloat("move_vel_override") != 0.0)
    {
        vel = row.GetFloat("move_vel_override");
    }

    const char* d = row["console_debug"];
    if(d && *d=='Y')
    {
        SetDebugging(5);
    }
    else
    {
        SetDebugging(0);
    }

    const char* e = row["disabled"];
    if(e && (*e=='Y' || *e=='y'))
    {
        disabled = true;
    }
    else
    {
        disabled = false;
    }

    brain = new NPCType(*t, this); // deep copy constructor
    origtype = t->GetName();

    return true; // success
}

bool NPC::InsertCopy(PID use_char_id, PID ownerPID)
{
    int r = db->Command("insert into sc_npc_definitions "
                        "(name, char_id, npctype, region, ang_vel_override, move_vel_override, console_debug, char_id_owner) values "
                        "('%s', %d,      '%s',    '%s',     %f,               %f, '%c',%d)",
                        name.GetData(), use_char_id.Unbox(), type.GetData(), region_name.GetData(), ang_vel, vel, IsDebugging() ? 'Y' : 'N', ownerPID.Unbox());

    if(r!=1)
    {
        Error3("Error in InsertCopy: %s->%s",db->GetLastQuery(),db->GetLastError());
    }
    else
    {
        Debug2(LOG_NEWCHAR, use_char_id.Unbox(), "Inserted %s", db->GetLastQuery());
    }
    return (r==1);
}

bool NPC::Delete()
{
    int r = db->Command("DELETE FROM sc_npc_definitions where char_id='%u'",pid);
    return (r == 1);
}


void NPC::SetActor(gemNPCActor* actor)
{
    npcActor = actor;

    // Initialize active location to a known ok value
    if(npcActor)
    {

        iSector* sector;
        psGameObject::GetPosition(actor,activeLocate->pos,activeLocate->angle,sector);
        movable = actor->pcmesh->GetMesh()->GetMovable();
    }
    else
    {
        movable = NULL;
    }
}

void NPC::SetAlive(bool a)
{
    alive = a;

    NPCDebug(this, 1, "** NPC %s is %s**\n", GetName(), alive?"Alive":"Dead");
}

void NPC::Advance(csTicks when)
{
    if(last_update && !disabled)
    {
        brain->Advance(when-last_update, this);

        // Check for a SetBrain() operation.
        // We can delete the old brain now that the stack is unwound and
        // we aren't using the old pointers.
        if(oldbrain)
        {
            delete oldbrain;
            oldbrain = NULL;
        }

        UpdateControlled();
    }

    last_update = when;
}

void NPC::TriggerEvent(Perception* pcpt, float maxRange,
                       csVector3* basePos, iSector* baseSector, bool sameSector)
{
    if(disabled)
    {
        NPCDebug(this, 15, "Disabled so rejecting perception #s",
                 pcpt->ToString(this).GetData());
        return;
    }

    if(maxRange > 0.0)
    {
        // This is a range based perception
        gemNPCActor* me = GetActor();
        if(!me)
        {
            NPCDebug(this, 15, "Can't do a ranged based check without an actor");
            return;
        }

        csVector3 pos;
        iSector*  sector;
        psGameObject::GetPosition(me, pos, sector);

        if(sameSector && sector != baseSector)
        {
            return;
        }

        float distance = world->Distance(pos, sector, *basePos, baseSector);

        if(distance > maxRange)
        {
            NPCDebug(this, 15,"The distance %.2f is outside range %.2f of perception %s",
                     distance, maxRange, pcpt->ToString(this).GetData());
            return;
        }
    }

    NPCDebug(this, 10,"Got event %s",pcpt->ToString(this).GetData());
    brain->FirePerception(this, pcpt);
}

void NPC::TriggerEvent(const char* pcpt)
{
    Perception perc(pcpt);
    TriggerEvent(&perc, -1, NULL, NULL, false);
}

void NPC::SetLastPerception(Perception* pcpt)
{
    if(last_perception)
        delete last_perception;
    last_perception = pcpt;
}

gemNPCActor* NPC::GetMostHated(float range, bool includeOutsideRegion, bool includeInvisible, bool includeInvincible, float* hate)
{
    iSector* sector=NULL;
    csVector3 pos;
    psGameObject::GetPosition(GetActor(), pos, sector);

    return GetMostHated(pos, sector, range, GetRegion(),
                        includeOutsideRegion, includeInvisible, includeInvincible, hate);
}


gemNPCActor* NPC::GetMostHated(csVector3 &pos, iSector* sector, float range, LocationType* region, bool includeOutsideRegion,
                               bool includeInvisible, bool includeInvincible, float* hate)
{
    gemNPCActor* hated = hatelist.GetMostHated(this, pos, sector, range, region,
                         includeOutsideRegion, includeInvisible, includeInvincible, hate);

    if(hated)
    {
        NPCDebug(this, 5, "Found most hated: %s(%s)", hated->GetName(), ShowID(hated->GetEID()));

    }
    else
    {
        NPCDebug(this, 5, "Found no hated entity");
    }

    return hated;
}

void NPC::AddToHateList(gemNPCActor* attacker, float delta)
{
    if(GetActor() && (GetActor() != attacker))
    {
        NPCDebug(this, 5, "Adding %1.2f to hatelist score for %s(%s).",
                 delta, attacker->GetName(), ShowID(attacker->GetEID()));
        hatelist.AddHate(attacker->GetEID(),delta);
        if(IsDebugging(5))
        {
            DumpHateList(this);
        }
    }
    else
    {
        NPCDebug(this, 5, "Rejecting adding of %1.2f to hatelist for self to self.", delta);
    }
}


float NPC::GetEntityHate(gemNPCActor* ent)
{
    return hatelist.GetHate(ent->GetEID());
}

void NPC::RemoveFromHateList(EID who)
{
    if(hatelist.Remove(who))
    {
        NPCDebug(this, 5, "Removed %s from hate list.", ShowID(who));
    }
}

void NPC::SetLocate(const csString &destination, const NPC::Locate &locate)
{
    // Find or create locate
    Locate* current = storedLocates.Get(destination,NULL);
    if(!current)
    {
        current = new Locate;
        storedLocates.PutUnique(destination,current);
    }

    // Copy content
    *current = locate;
}

void NPC::GetActiveLocate(csVector3 &pos, iSector* &sector, float &rot)
{
    pos    = activeLocate->pos;
    sector = activeLocate->sector;
    rot    = activeLocate->angle;
}

void NPC::GetActiveLocate(Waypoint* &wp)
{
    wp = activeLocate->wp;
}

float NPC::GetActiveLocateRadius() const
{
    return activeLocate->radius;
}

bool NPC::CopyLocate(csString source, csString destination, unsigned int flags)
{
    NPCDebug(this, 5, "Copy locate from %s to %s (%X)",source.GetDataSafe(),destination.GetDataSafe(),flags);

    Locate* sourceLocate = storedLocates.Get(source,NULL);
    if(!sourceLocate)
    {
        NPCDebug(this, 5, "Failed to copy, no source found!");
        return false;
    }

    Locate* destinationLocation = storedLocates.Get(destination,NULL);
    if(!destinationLocation)
    {
        destinationLocation = new Locate;
        storedLocates.PutUnique(destination,destinationLocation);
    }

    if(flags & LOCATION_POS)
    {
        destinationLocation->pos    = sourceLocate->pos;
    }
    if(flags & LOCATION_SECTOR)
    {
        destinationLocation->sector = sourceLocate->sector;
    }
    if(flags & LOCATION_ANGLE)
    {
        destinationLocation->angle  = sourceLocate->angle;
    }
    if(flags & LOCATION_WP)
    {
        destinationLocation->wp     = sourceLocate->wp;
    }
    if(flags & LOCATION_RADIUS)
    {
        destinationLocation->radius = sourceLocate->radius;
    }
    if(flags & LOCATION_TARGET)
    {
        destinationLocation->target = sourceLocate->target;
        if(destination == "Active")
        {
            SetTarget(destinationLocation->target);
        }
    }

    return true;
}

float NPC::GetAngularVelocity()
{
    if(ang_vel == 999)
        return brain->GetAngularVelocity(this);
    else
        return ang_vel;
}

float NPC::GetVelocity()
{
    if(vel == 999)
        return brain->GetVelocity(this);
    else
        return vel;
}

float NPC::GetWalkVelocity()
{
    // Cache value if not looked up before
    if(walkVelocity == 0.0)
    {
        walkVelocity = npcclient->GetWalkVelocity(npcActor->GetRace());
    }

    return walkVelocity;
}

float NPC::GetRunVelocity()
{
    // Cache value if not looked up before
    if(runVelocity == 0.0)
    {
        runVelocity = npcclient->GetRunVelocity(npcActor->GetRace());
    }

    return runVelocity;
}

LocationType* NPC::GetRegion()
{
    if(region)
    {
        return region;
    }
    else
    {
        region = npcclient->FindRegion(region_name);
        return region;
    }
}

void NPC::Disable(bool disable)
{
    // if not yet enabled, restart the tick
    if(disabled && !disable)
    {
        disabled = false;
        Tick();
    }

    disabled = disable;

    if(GetActor())
    {
        if(disabled)
        {
            // Stop the movement

            // Set Vel to zero again
            GetLinMove()->SetVelocity(csVector3(0,0,0));
            GetLinMove()->SetAngularVelocity(0);

            //now persist
            networkmanager->QueueDRData(this);

            // Set the npc in the none attackable state
            networkmanager->QueueTemporarilyImperviousCommand(GetActor(), true);
        }
        else
        {
            // Set the npc in the attackable state
            networkmanager->QueueTemporarilyImperviousCommand(GetActor(), false);
        }
    }
}


void NPC::DumpState(csString &output)
{
    csVector3 pos;
    iSector* sector;
    float rot = 0.0;
    InstanceID instance = INSTANCE_ALL;

    if(npcActor)
    {
        psGameObject::GetPosition(npcActor,pos,rot,sector);
        instance = npcActor->GetInstance();
    }

    output.AppendFmt("States for %s (%s)\n", name.GetData(), ShowID(pid));
    output.AppendFmt("---------------------------------------------\n");
    output.AppendFmt("Position:             %s\n",npcActor?toString(pos,sector).GetDataSafe():"(none)");
    output.AppendFmt("Rotation:             %.2f\n",rot);
    output.AppendFmt("Instance:             %d\n",instance);
    output.AppendFmt("Debugging:            %d\n",GetDebugging());
    output.AppendFmt("Debug clients:        %s\n",GetRemoteDebugClientsString().GetDataSafe());
    output.AppendFmt("DR Counter:           %d\n",DRcounter);
    output.AppendFmt("Alive:                %s\n",alive?"True":"False");
    output.AppendFmt("Disabled:             %s\n",disabled?"True":"False");
    output.AppendFmt("Checked:              %s\n",checked?"True":"False");
    output.AppendFmt("Spawn position:       %s\n",toString(spawnPosition,spawnSector).GetDataSafe());
    output.AppendFmt("Ang vel:              %.2f\n",ang_vel);
    output.AppendFmt("Vel:                  %.2f\n",vel);
    output.AppendFmt("Walk velocity:        %.2f\n",GetWalkVelocity());
    output.AppendFmt("Run velocity:         %.2f\n",GetRunVelocity());
    output.AppendFmt("HP(Value/Max/Rate):   %.1f/%.1f/%.1f\n",GetHP(),GetMaxHP(),GetHPRate());
    output.AppendFmt("Mana(V/M/R):          %.1f/%.1f/%.1f\n",GetMana(),GetMaxMana(),GetManaRate());
    output.AppendFmt("PStamina(V/M/R):      %.1f/%.1f/%.1f\n",GetPysStamina(),GetMaxPysStamina(),GetPysStaminaRate());
    output.AppendFmt("MStamina(V/M/R):      %.1f/%.1f/%.1f\n",GetMenStamina(),GetMaxMenStamina(),GetMenStaminaRate());
    output.AppendFmt("Owner:                %s\n",GetOwnerName());
    output.AppendFmt("Race:                 %s\n",GetRaceInfo()?GetRaceInfo()->name.GetDataSafe():"(None)");
    output.AppendFmt("Region:               %s\n",GetRegion()?GetRegion()->GetName():"(None)");
    output.AppendFmt("Inside region:        %s\n",insideRegion?"Yes":"No");
    output.AppendFmt("Tribe:                %s\n",GetTribe()?GetTribe()->GetName():"(None)");
    output.AppendFmt("TribeMemberType:      %s\n",GetTribeMemberType().GetDataSafe());
    output.AppendFmt("Inside tribe home:    %s\n",insideTribeHome?"Yes":"No");
    output.AppendFmt("Target:               %s\n",GetTarget()?GetTarget()->GetName():"");
    output.AppendFmt("Last perception:      %s\n",last_perception?last_perception->GetName().GetDataSafe():"(None)");
    output.AppendFmt("Fall counter:         %d\n", GetFallCounter());
    csString types;
    for(size_t m=0; m < autoMemorizeTypes.GetSize(); m++)
    {
        types.AppendFmt("%s%s",m?",":"",autoMemorizeTypes.Get(m).GetDataSafe());
    }
    output.AppendFmt("AutoMemorize:         \"%s\"\n",types.GetDataSafe());
    output.AppendFmt("Brain:                %s\n\n", brain->GetName());
    output.AppendFmt("Current Behavior:     '%s'",GetCurrentBehavior()?GetCurrentBehavior()->GetName():"(None)");

    output.AppendFmt("Locates for %s (%s)\n", name.GetData(), ShowID(pid));
    output.AppendFmt("---------------------------------------------\n");
    LocateHash::GlobalIterator iter = storedLocates.GetIterator();
    while(iter.HasNext())
    {
        csString name;
        Locate* locate = iter.Next(name);
        output.AppendFmt("%-15s Position:  %s\n",name.GetDataSafe(),toString(locate->pos,locate->sector).GetDataSafe());
        output.AppendFmt("%-15s Angle:     %.2f\n","",locate->angle);
        output.AppendFmt("%-15s Radius:    %.2f\n","",locate->radius);
        output.AppendFmt("%-15s WP:        %s\n","",locate->wp?locate->wp->GetName():"");

    }

}


void NPC::DumpBehaviorList(csString &output)
{
    output.AppendFmt("Behaviors for %s (%s)\n", name.GetData(), ShowID(pid));
    output.AppendFmt("---------------------------------------------------------\n");

    brain->DumpBehaviorList(output, this);
}

void NPC::DumpReactionList(csString &output)
{
    output.AppendFmt("Reactions for %s (%s)\n", name.GetData(), ShowID(pid));
    output.AppendFmt("---------------------------------------------------------\n");

    brain->DumpReactionList(output, this);
}

void NPC::DumpHateList(csString &output)
{
    iSector* sector=NULL;
    csVector3 pos;

    output.AppendFmt("Hate list for %s (%s)\n", name.GetData(), ShowID(pid));
    output.AppendFmt("---------------------------------------------\n");

    if(GetActor())
    {
        psGameObject::GetPosition(GetActor(),pos,sector);
        hatelist.DumpHateList(output,pos,sector);
    }
}

void NPC::DumpHateList(NPC* npc)
{
    iSector* sector=NULL;
    csVector3 pos;

    NPCDebug(npc, 5, "Hate list for %s (%s)", name.GetData(), ShowID(pid));

    if(GetActor())
    {
        psGameObject::GetPosition(GetActor(), pos, sector);
        hatelist.DumpHateList(npc, pos, sector);
    }
}

void NPC::DumpDebugLog(csString &output)
{
    output.AppendFmt("Debug log for %s (%s)\n", name.GetData(), ShowID(pid));
    output.AppendFmt("---------------------------------------------\n");
    for(size_t i = 0; i < debugLog.GetSize(); i++)
    {
        output.AppendFmt("%2zu %s\n",i,debugLog[(nextDebugLogEntry+i)%debugLog.GetSize()].GetDataSafe());
    }

}

void NPC::DumpMemory(csString &output)
{
    // Dump it's memory buffer
    output.AppendFmt("Buffers:\n");
    int index = 0;
    BufferHash::GlobalIterator iter = npcBuffer.GetIterator();
    while(iter.HasNext())
    {
        csString bufferName;
        csString value = iter.Next(bufferName);
        output.AppendFmt("String buffer(%d): %s = %s\n",index++,bufferName.GetDataSafe(),value.GetDataSafe());
    }
    output.AppendFmt("Memory buffer:\n");
    if(bufferMemory)
    {
        output.AppendFmt("Name: %s\n", bufferMemory->name.GetData());
        output.AppendFmt("Pos: x:%f y:%f z:%f\n",
                         bufferMemory->pos[0],
                         bufferMemory->pos[1],
                         bufferMemory->pos[2]);
        output.AppendFmt("Has Sector:\n");
        if(bufferMemory->GetSector())
            output.AppendFmt("Yes\n");
        else
            output.AppendFmt("No\n");
    }
    else
    {
        output.AppendFmt("Empty\n");
    }
}

void NPC::DumpControlled(csString &output)
{
    csString controlled;
    for(size_t i = 0; i < controlledActors.GetSize(); i++)
    {
        if(controlledActors[i].IsValid())
        {
            controlled.AppendFmt(" %s",controlledActors[i]->GetName());
        }
    }
    output.AppendFmt("Controlled: %s\n",controlled.GetDataSafe());
}


void NPC::ClearState()
{
    NPCDebug(this, 5, "ClearState");
    brain->ClearState(this);
    delete last_perception;
    last_perception = NULL;
    hatelist.Clear();
    SetAlive(false);
    // Enable position check next time npc is attached
    checked = false;
    disabled = false;
}

///TODO: The next 3 functions are exactly the same except a line in the code, maybe figure out a better way to handle this?

gemNPCActor* NPC::GetNearestActor(float range, csVector3 &destPosition, iSector* &destSector, float &destRange)
{
    csVector3 loc;
    iSector*  sector;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCActor*> nearlist = npcclient->FindNearbyActors(sector, loc, range);
    if(nearlist.GetSize() > 0)
    {
        gemNPCActor* nearestEnt = NULL;
        csVector3    nearestLoc;
        iSector*     nearestSector = NULL;

        float nearestRange=range;

        for(size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCActor* ent = nearlist[i];

            // Filter own NPC actor
            if(ent == GetActor())
                continue;

            csVector3 loc2;
            iSector* sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = world->Distance(loc, sector, loc2, sector2);
            if(dist < nearestRange)
            {
                nearestRange  = dist;
                nearestEnt    = ent;
                nearestLoc    = loc2;
                nearestSector = sector2;
            }
        }
        if(nearestEnt)
        {
            destPosition = nearestLoc;
            destSector   = nearestSector;
            destRange    = nearestRange;
            return nearestEnt;
        }
    }
    return NULL;
}

gemNPCActor* NPC::GetNearestNPC(float range, csVector3 &destPosition, iSector* &destSector, float &destRange)
{
    csVector3 loc;
    iSector*  sector;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCActor*> nearlist = npcclient->FindNearbyActors(sector, loc, range);
    if(nearlist.GetSize() > 0)
    {
        gemNPCActor* nearestEnt = NULL;
        csVector3    nearestLoc;
        iSector*     nearestSector = NULL;

        float nearestRange=range;

        for(size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCActor* ent = nearlist[i];

            // Filter own NPC actor, and all players
            if(ent == GetActor() || !ent->GetNPC())
                continue;

            csVector3 loc2;
            iSector* sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = world->Distance(loc, sector, loc2, sector2);
            if(dist < nearestRange)
            {
                nearestRange  = dist;
                nearestEnt    = ent;
                nearestLoc    = loc2;
                nearestSector = sector2;
            }
        }
        if(nearestEnt)
        {
            destPosition = nearestLoc;
            destSector   = nearestSector;
            destRange    = nearestRange;
            return nearestEnt;
        }
    }
    return NULL;
}

gemNPCActor* NPC::GetNearestPlayer(float range, csVector3 &destPosition, iSector* &destSector, float &destRange)
{
    csVector3 loc;
    iSector*  sector;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCActor*> nearlist = npcclient->FindNearbyActors(sector, loc, range);
    if(nearlist.GetSize() > 0)
    {
        gemNPCActor* nearestEnt = NULL;
        csVector3    nearestLoc;
        iSector*     nearestSector = NULL;

        float nearestRange=range;

        for(size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCActor* ent = nearlist[i];

            // Filter own NPC actor, and all NPCs
            if(ent == GetActor() || ent->GetNPC())
                continue;

            csVector3 loc2;
            iSector* sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = world->Distance(loc, sector, loc2, sector2);
            if(dist < nearestRange)
            {
                nearestRange  = dist;
                nearestEnt    = ent;
                nearestLoc    = loc2;
                nearestSector = sector2;
            }
        }
        if(nearestEnt)
        {
            destPosition = nearestLoc;
            destSector   = nearestSector;
            destRange    = nearestRange;
            return nearestEnt;
        }
    }
    return NULL;
}


gemNPCActor* NPC::GetNearestVisibleFriend(float range)
{
    csVector3     loc;
    iSector*      sector;
    float         min_range;
    gemNPCObject* friendEnt = NULL;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCObject*> nearlist = npcclient->FindNearbyEntities(sector,loc,range);
    if(nearlist.GetSize() > 0)
    {
        min_range=range;
        for(size_t i=0; i<nearlist.GetSize(); i++)
        {
            gemNPCObject* ent = nearlist[i];
            NPC* npcFriend = ent->GetNPC();

            if(!npcFriend || npcFriend == this)
                continue;

            csVector3 loc2, isect;
            iSector* sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = (loc2 - loc).Norm();
            if(min_range < dist)
                continue;

            // Is this friend visible?
            csIntersectingTriangle closest_tri;
            iMeshWrapper* sel = 0;

            dist = csColliderHelper::TraceBeam(npcclient->GetCollDetSys(), sector,
                                               loc + csVector3(0, 0.6f, 0), loc2 + csVector3(0, 0.6f, 0), true, closest_tri, isect, &sel);
            // Not visible
            if(dist > 0)
                continue;

            min_range = (loc2 - loc).Norm();
            friendEnt = ent;
        }
    }
    return (gemNPCActor*)friendEnt;
}

gemNPCActor* NPC::GetNearestDeadActor(float range)
{
    csVector3    loc;
    iSector*     sector;
    float        min_range;
    gemNPCActor* nearEnt = NULL;

    psGameObject::GetPosition(GetActor(), loc, sector);

    csArray<gemNPCObject*> nearlist = npcclient->FindNearbyEntities(sector,loc,range);
    if(nearlist.GetSize() > 0)
    {
        min_range=range;
        for(size_t i=0; i<nearlist.GetSize(); i++)
        {
            // Check if this is an Actor
            gemNPCActor* ent = dynamic_cast<gemNPCActor*>(nearlist[i]);
            if(!ent)
            {
                continue; // No actor
            }

            if(ent->IsAlive())
            {
                continue;
            }

            csVector3 loc2, isect;
            iSector* sector2;
            psGameObject::GetPosition(ent, loc2, sector2);

            float dist = (loc2 - loc).Norm();
            if(min_range < dist)
                continue;

            min_range = (loc2 - loc).Norm();
            nearEnt = ent;
        }
    }
    return nearEnt;
}

void NPC::AddAutoMemorize(csString types)
{
    csArray<csString> typeArray = psSplit(types,',');

    for(size_t i=0; i < typeArray.GetSize(); i++)
    {
        if(!typeArray[i].IsEmpty())
        {
            // Push at tail if not already present
            autoMemorizeTypes.PushSmart(typeArray[i]);
        }
    }

    NPCDebug(this, 10, "Added Auto Memorize Types: %s",types.GetDataSafe());
}

void NPC::RemoveAutoMemorize(csString types)
{
    csArray<csString> typeArray = psSplit(types,',');
    for(size_t i=0; i < typeArray.GetSize(); i++)
    {
        autoMemorizeTypes.Delete(typeArray[i]);
    }

    NPCDebug(this, 10, "Removed Auto Memorize Types: %s",types.GetDataSafe());
}

bool NPC::ContainAutoMemorizeType(const csString &type)
{
    return ((!type.IsEmpty()) && ((autoMemorizeTypes.Get(0) == "all") || (autoMemorizeTypes.Find(type) != csArrayItemNotFound)));
}

void NPC::LocalDebugReport(const csString &debugString)
{
    csStringArray list(debugString,"\n");
    for(size_t i = 0; i < list.GetSize(); i++)
    {
        CPrintf(CON_CMDOUTPUT, "%s (%s)> %s\n", GetName(), ShowID(pid), list[i]);
    }
}

void NPC::RemoteDebugReport(uint32_t clientNum, const csString &debugString)
{
    csStringArray list(debugString,"\n");
    for(size_t i = 0; i < list.GetSize(); i++)
    {
        networkmanager->QueueSystemInfoCommand(clientNum,"%s (%s)> %s", GetName(), ShowID(pid), list[i]);
    }
}


gemNPCObject* NPC::GetTarget()
{
    // If something is targeted, use it.
    if(target_id != 0)
    {
        // Check if visible
        gemNPCObject* obj = npcclient->FindEntityID(target_id);
        if(obj && obj->IsInvisible())
        {
            NPCDebug(this, 15, "GetTarget returning nothing, target is invisible");
            return NULL;
        }

        return obj;
    }
    else  // if not, try the last perception entity
    {
        if(GetLastPerception())
        {
            gemNPCObject* target = NULL;
            gemNPCObject* entity = GetLastPerception()->GetTarget();
            if(entity)
            {
                target = npcclient->FindEntityID(entity->GetEID());
            }
            NPCDebug(this, 16, "GetTarget returning last perception entity: %s",target ? target->GetName() : "None specified");
            return target;
        }
    }
    return NULL;
}

void NPC::SetTarget(gemNPCObject* t)
{
    if(t == NULL)
    {
        NPCDebug(this, 10, "Clearing target");
        target_id = EID(0);
    }
    else
    {
        NPCDebug(this, 10, "Setting target to: %s (%s)",t->GetName(),ShowID(t->GetEID()));
        target_id = t->GetEID();
    }
}


gemNPCObject* NPC::GetOwner()
{
    if(owner_id.IsValid())
    {
        return npcclient->FindEntityID(owner_id);
    }
    return NULL;
}

const char* NPC::GetOwnerName()
{
    if(owner_id.IsValid())
    {
        gemNPCObject* obj = npcclient->FindEntityID(owner_id);
        if(obj)
        {
            return obj->GetName();
        }
    }

    return "";
}

void NPC::SetOwner(EID owner_EID)
{
    owner_id = owner_EID;
}

void NPC::SetTribe(Tribe* new_tribe)
{
    tribe = new_tribe;
}

Tribe* NPC::GetTribe()
{
    return tribe;
}

void  NPC::SetTribeMemberType(const char* tribeMemberType)
{
    this->tribeMemberType = tribeMemberType;
}

const csString  &NPC::GetTribeMemberType() const
{
    return tribeMemberType;
}

psNPCRaceListMessage::NPCRaceInfo_t* NPC::GetRaceInfo()
{
    if(!raceInfo && npcActor)
    {
        raceInfo = npcclient->GetRaceInfo(npcActor->GetRace());
    }

    return raceInfo;
}

float NPC::GetHP()
{
    if(!npcActor) return 0.0;
    return npcActor->GetHP();
}

float NPC::GetMaxHP() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetMaxHP();
}

float NPC::GetHPRate() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetHPRate();
}

float NPC::GetMana()
{
    if(!npcActor) return 0.0;
    return npcActor->GetMana();
}

float NPC::GetMaxMana() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetMaxMana();
}

float NPC::GetManaRate() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetManaRate();
}

float NPC::GetPysStamina()
{
    if(!npcActor) return 0.0;
    return npcActor->GetPysStamina();
}

float NPC::GetMaxPysStamina() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetMaxPysStamina();
}

float NPC::GetPysStaminaRate() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetPysStaminaRate();
}

float NPC::GetMenStamina()
{
    if(!npcActor) return 0.0;
    return npcActor->GetMenStamina();
}

float NPC::GetMaxMenStamina() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetMaxMenStamina();
}

float NPC::GetMenStaminaRate() const
{
    if(!npcActor) return 0.0;
    return npcActor->GetMenStaminaRate();
}

void NPC::TakeControl(gemNPCActor* actor)
{
    controlledActors.PushSmart(csWeakRef<gemNPCActor>(actor));
}

void NPC::ReleaseControl(gemNPCActor* actor)
{
    controlledActors.Delete(csWeakRef<gemNPCActor>(actor));
}

void NPC::UpdateControlled()
{
    if(controlledActors.IsEmpty())
    {
        return;
    }

    csVector3 pos,vel;
    float yrot;
    iSector* sector;

    psGameObject::GetPosition(GetActor(), pos, yrot, sector);
    vel = GetLinMove()->GetVelocity();

    for(size_t i = 0; i < controlledActors.GetSize(); i++)
    {
        if(controlledActors[i].IsValid())
        {
            gemNPCActor* controlled = controlledActors[i];

            // TODO: Calculate some offset from controlled to controlling

            // For now use controlling position for controlled
            psGameObject::SetPosition(controlled, pos, sector);
            psGameObject::SetRotationAngle(controlled, yrot);
            controlled->pcmove->SetVelocity(vel);

            // Queue the new controlled position to the server.
            networkmanager->QueueControlCommand(GetActor(), controlled);
        }
    }
}

void NPC::CheckPosition()
{
    // We only need to check the position once

    npcMesh* pcmesh =  GetActor()->pcmesh;

    if(checked)
    {
        if(checkedPos == pcmesh->GetMesh()->GetMovable()->GetPosition() && checkedSector == pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0))
        {
            SetAlive(checkedResult);
            CPrintf(CON_NOTIFY,"Extrapolation skipped, result of: %s\n", checkedResult ? "Alive" : "Dead");
            return;
        }
    }
    if(!alive || GetLinMove()->IsPath())
        return;

    // Give the npc a jump start to make sure gravity will be applied.
    csVector3 startVel(0.0f,1.0f,0.0f);
    csVector3 vel;

    GetLinMove()->AddVelocity(startVel);
    GetLinMove()->SetOnGround(false);
    csVector3 pos(pcmesh->GetMesh()->GetMovable()->GetPosition());
    iSector* sector = pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
    // See what happens in the next 10 seconds
    int count = 100;
    while(count--)
    {
        GetLinMove()->ExtrapolatePosition(0.1f);
        vel = GetLinMove()->GetVelocity();
        // Bad starting position - npc is falling at high speed, server should automatically kill it
        if(vel.y < -50)
        {
            CPrintf(CON_ERROR,"Got bad starting location %s, killing %s (%s/%s).\n",
                    toString(pos,sector).GetDataSafe(), name.GetData(), ShowID(pid), ShowID(GetActor()->GetEID()));
            Disable();
            break;
        }
    }


    if(vel == startVel)
    {
        // Collision detection is not being applied!
        GetLinMove()->SetVelocity(csVector3(0.0f, 0.0f, 0.0f));
        psGameObject::SetPosition(GetActor(), pos);
    }
    checked = true;
    checkedPos = pos;
    checkedSector = pcmesh->GetMesh()->GetMovable()->GetSectors()->Get(0);
    checkedResult = alive;
}

void NPC::StoreSpawnPosition()
{
    psGameObject::GetPosition(GetActor(), spawnPosition, spawnSector);
}

const csVector3 &NPC::GetSpawnPosition() const
{
    return spawnPosition;
}

iSector* NPC::GetSpawnSector() const
{
    return spawnSector;
}

void NPC::SetBufferMemory(Tribe::Memory* memory)
{
    if(bufferMemory == NULL)
    {
        bufferMemory = new Tribe::Memory();
    }

    // Just copy data
    bufferMemory->name       = memory->name;
    bufferMemory->pos        = memory->pos;
    bufferMemory->sector     = memory->sector;
    bufferMemory->sectorName = memory->sectorName;
    bufferMemory->radius     = memory->radius;
}

void NPC::SetBuildingSpot(Tribe::Asset* buildingSpot)
{
    this->buildingSpot = buildingSpot;
}


/** Get the stored building spot for this NPC
 */
Tribe::Asset* NPC::GetBuildingSpot()
{
    return buildingSpot;
}

double NPC::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "InsideTribeHome")
    {
        return insideTribeHome?1.0:0.0;
    }
    if(property == "InsideRegion")
    {
        return insideRegion?1.0:0.0;
    }
    if(property == "Hate")
    {
        gemNPCActor* target = dynamic_cast<gemNPCActor*>(GetTarget());
        if(target)
        {
            return GetEntityHate(target);
        }
        else
        {
            return 0.0;
        }
    }
    if(property == "HasTarget")
    {
        gemNPCActor* target = dynamic_cast<gemNPCActor*>(GetTarget());
        if(target)
        {
            return 1.0;
        }
        else
        {
            return 0.0;
        }
    }
    if(property == "HP")
    {
        return GetHP();
    }
    if(property == "MaxHP")
    {
        return GetMaxHP();
    }
    if(property == "Mana")
    {
        return GetMana();
    }
    if(property == "MaxMana")
    {
        return GetMaxMana();
    }
    if(property == "PStamina")
    {
        return GetPysStamina();
    }
    if(property == "MaxPStamina")
    {
        return GetMaxPysStamina();
    }
    if(property == "MStamina")
    {
        return GetMenStamina();
    }
    if(property == "MaxMStamina")
    {
        return GetMaxMenStamina();
    }

    Error2("Requested NPC property not found '%s'", ptr);
    return 0.0;
}

double NPC::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    csString function(functionName);

    Error2("Requested NPC function not found '%s'", functionName);
    return 0.0;
}

const char* NPC::ToString()
{
    return "NPC";
}

csString NPC::GetBuffer(const csString &bufferName)
{
    csString value = npcBuffer.Get(bufferName,"");

    NPCDebug(this, 6, "Get Buffer(%s) return: '%s'",bufferName.GetDataSafe(),value.GetDataSafe());

    return value;
}

void NPC::SetBuffer(const csString &bufferName, const csString &value)
{
    NPCDebug(this, 6, "Set Buffer(%s,%s)",bufferName.GetDataSafe(),value.GetDataSafe());

    npcBuffer.PutUnique(bufferName,value);
}

void NPC::ReplaceBuffers(csString &result)
{
    // Only replace if there is something to replace
    if(result.Find("$NBUFFER[") == ((size_t)-1)) return;

    BufferHash::GlobalIterator iter = npcBuffer.GetIterator();
    while(iter.HasNext())
    {
        csString bufferName;
        csString value = iter.Next(bufferName);
        csString replace("$NBUFFER[");
        replace += bufferName;
        replace += "]";

        result.ReplaceAll(replace,value);
    }
}

void NPC::ReplaceLocations(csString &result)
{
    size_t startPos,endPos;

    // Only replace if there is something to replace
    startPos = result.Find("$LOCATION[");
    while(startPos != ((size_t)-1))
    {
        endPos = result.FindFirst(']',startPos);
        if(endPos == ((size_t)-1)) return;  // There should always be a ] after $LOCATION

        csString locationString = result.Slice(startPos+10,endPos-(startPos+10));

        csArray<csString> strArr = psSplit(locationString,'.');
        if(strArr.GetSize() != 2) return;  // Should always be a location and an attribute.

        NPC::Locate* location = storedLocates.Get(strArr[0],NULL);
        if(!location)
        {
            Error4("NPC %s(%s) Failed to find location %s in replace locations",
                   GetName(),ShowID(GetEID()),strArr[0].GetDataSafe());
            return; // Failed to find location
        }

        csString replace;


        if(strArr[1].CompareNoCase("targetEID"))
        {
            if(location->target.IsValid())
            {
                replace = ShowID(location->target->GetEID());
            }
        }
        else if(strArr[1].CompareNoCase("targetName"))
        {
            if(location->target.IsValid())
            {
                replace = location->target->GetName();
            }
        }
        else
        {
            Error5("NPC %s(%s) Failed to find find attribute %s for location %s in replace locations",
                   GetName(),ShowID(GetEID()),strArr[1].GetDataSafe(),strArr[0].GetDataSafe());
            return; // Not implemented or unkown attribute.
        }

        result.DeleteAt(startPos,endPos-startPos+1);
        result.Insert(startPos,replace);

        startPos = result.Find("$LOCATION[");
    }
}

//-----------------------------------------------------------------------------

HateList::~HateList()
{
    Clear();
}

void HateList::AddHate(EID entity_id, float delta)
{
    HateListEntry* h = hatelist.Get(entity_id, 0);
    if(!h)
    {
        h = new HateListEntry;
        h->entity_id   = entity_id;
        h->hate_amount = delta;
        hatelist.Put(entity_id,h);
    }
    else
    {
        h->hate_amount += delta;
    }
}

gemNPCActor* HateList::GetMostHated(NPC* npc, csVector3 &pos, iSector* sector, float range, LocationType* region,
                                    bool includeOutsideRegion, bool includeInvisible, bool includeInvincible, float* hate)
{
    gemNPCObject* mostHated = NULL;
    float mostHateAmount=0.0;

    csArray<gemNPCObject*> list = npcclient->FindNearbyEntities(sector,pos,range);
    for(size_t i=0; i<list.GetSize(); i++)
    {
        HateListEntry* h = hatelist.Get(list[i]->GetEID(),0);
        if(h)
        {
            gemNPCObject* obj = npcclient->FindEntityID(list[i]->GetEID());

            if(!obj) continue;

            // Skipp if invisible or invincible
            if(obj->IsInvisible() && !includeInvisible) continue;
            if(obj->IsInvincible() && !includeInvincible) continue;

            if(!mostHated || h->hate_amount > mostHateAmount)
            {
                csVector3 objPos;
                iSector* objSector;
                psGameObject::GetPosition(obj, objPos, objSector);

                // Don't include if a region is defined and obj not within region.
                if(!includeOutsideRegion && region && !region->CheckWithinBounds(engine,objPos,objSector))
                {
                    NPCDebug(npc, 10, "Skipping %s(%s) since outside region %s",obj->GetName(),ShowID(obj->GetEID()),region->GetName());
                    continue;
                }

                mostHated = list[i];
                mostHateAmount = h->hate_amount;
            }
        }
    }
    if(hate)
    {
        *hate = mostHateAmount;
    }
    return (gemNPCActor*)mostHated;
}

bool HateList::Remove(EID entity_id)
{
    HateListEntry* h = hatelist.Get(entity_id, 0);
    if(h)
        delete h;
    return hatelist.DeleteAll(entity_id);
}

void HateList::Clear()
{
    csHash<HateListEntry*, EID>::GlobalIterator iter = hatelist.GetIterator();
    while(iter.HasNext())
    {
        delete iter.Next();
    }
    hatelist.DeleteAll();
}

float HateList::GetHate(EID ent)
{
    HateListEntry* h = hatelist.Get(ent, 0);
    if(h)
        return h->hate_amount;
    else
        return 0;
}

void HateList::DumpHateList(csString &output, const csVector3 &myPos, iSector* mySector)
{
    csHash<HateListEntry*, EID>::GlobalIterator iter = hatelist.GetIterator();

    output.AppendFmt("%6s %-20s %5s %-40s %5s %s\n",
                     "Entity","Name", "Hated","Pos","Range","Flags");

    while(iter.HasNext())
    {
        HateListEntry* h = (HateListEntry*)iter.Next();
        csVector3 pos(9.9f,9.9f,9.9f);
        gemNPCObject* obj = npcclient->FindEntityID(h->entity_id);
        csString sectorName;

        if(obj)
        {
            iSector* sector;
            psGameObject::GetPosition(obj,pos,sector);
            if(sector)
            {
                sectorName = sector->QueryObject()->GetName();
            }
            output.AppendFmt("%6d %-20s %5.1f %-40s %5.1f%s%s",
                             h->entity_id.Unbox(), obj->GetName(), h->hate_amount, toString(pos, sector).GetDataSafe(),
                             world->Distance(pos,sector,myPos,mySector), obj->IsInvisible()?" Invisible":"", obj->IsInvincible()?" Invincible":"");
            output.AppendFmt("\n");
        }
        else
        {
            // This is an error situation. Should not hate something that isn't online.
            output.AppendFmt("Entity: %u Hated: %.1f\n", h->entity_id.Unbox(), h->hate_amount);
        }

    }
    output.AppendFmt("\n");
}

void HateList::DumpHateList(NPC* npc, const csVector3 &myPos, iSector* mySector)
{
    csHash<HateListEntry*, EID>::GlobalIterator iter = hatelist.GetIterator();

    NPCDebug(npc, 5, "%6s %-20s %5s %-40s %5s %s",
             "Entity","Name", "Hated","Pos","Range","Flags");

    while(iter.HasNext())
    {
        HateListEntry* h = (HateListEntry*)iter.Next();
        csVector3 pos(9.9f,9.9f,9.9f);
        gemNPCObject* obj = npcclient->FindEntityID(h->entity_id);
        csString sectorName;

        if(obj)
        {
            iSector* sector;
            psGameObject::GetPosition(obj,pos,sector);
            if(sector)
            {
                sectorName = sector->QueryObject()->GetName();
            }
            NPCDebug(npc, 5, "%6d %-20s %5.1f %-40s %5.1f%s%s",
                     h->entity_id.Unbox(), obj->GetName(), h->hate_amount, toString(pos, sector).GetDataSafe(),
                     world->Distance(pos,sector,myPos,mySector), obj->IsInvisible()?" Invisible":"", obj->IsInvincible()?" Invincible":"");
        }
        else
        {
            // This is an error situation. Should not hate something that isn't online.
            NPCDebug(npc, 5, "Entity: %u Hated: %.1f", h->entity_id.Unbox(), h->hate_amount);
        }

    }
}
