/*
 * hiresession.cpp  creator <andersr@pvv.org>
 *
 * Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//====================================================================================
// Crystal Space Includes
//====================================================================================

//====================================================================================
// Project Includes
//====================================================================================
#include <util/location.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "hiresession.h"
#include "gem.h"
#include "questmanager.h"

HireSession::HireSession():
    guild(false),
    workLocationID(0),
    verified(false),
    tempWorkLocation(NULL),
    tempWorkLocationValid(false),
    workLocation(NULL),
    shouldLoad(true)
{
}

HireSession::HireSession(gemActor* owner):
    guild(false),
    workLocationID(0),
    verified(false),
    tempWorkLocation(NULL),
    tempWorkLocationValid(false),
    workLocation(NULL),
    shouldLoad(false) // Should not be loaded when created from new owner
{
    SetOwner(owner);
}

HireSession::~HireSession()
{
}

bool HireSession::Load(iResultRow &row)
{
    ownerPID = PID(row.GetInt("owner_id"));
    hiredPID = PID(row.GetInt("hired_npc_id"));
    guild = row["guild"][0] == 'Y';
    workLocationID = row.GetInt("work_location_id");
    script = row["script"];

    return true;
}

bool HireSession::Save(bool newSession)
{
    unsigned long result = 0;

    if(newSession)
    {
        result = db->CommandPump("INSERT INTO npc_hired_npcs"
                                 " (owner_id,hired_npc_id,guild,work_location_id,script)"
                                 " VALUES ('%u','%u','%s','%u','%s')",
                                 ownerPID.Unbox(), hiredPID.Unbox(), guild?"Y":"N", workLocationID,
                                 script.GetDataSafe());
    }
    else
    {
        result = db->CommandPump("UPDATE npc_hired_npcs SET guild='%s', work_location_id='%u', script='%s'"
                                 " WHERE owner_id=%u AND hired_npc_id=%u",
                                 guild?"Y":"N", workLocationID,script.GetDataSafe(),
                                 ownerPID.Unbox(), hiredPID.Unbox());
    }

    if(result==QUERY_FAILED)
        return false;

    return true;
}

bool HireSession::Delete()
{
    unsigned long result = 0;

    result = db->CommandPump("DELETE FROM npc_hired_npcs WHERE owner_id=%u AND hired_npc_id=%u",
                             ownerPID.Unbox(), hiredPID.Unbox());

    if(result==QUERY_FAILED)
        return false;

    return true;
}


void HireSession::SetHireType(const csString &name, const csString &npcType)
{
    hireTypeName = name;
    hireTypeNPCType = npcType;
}

const csString &HireSession::GetHireTypeName() const
{
    return hireTypeName;
}

const csString &HireSession::GetHireTypeNPCType() const
{
    return hireTypeNPCType;
}

void HireSession::SetMasterPID(PID masterPID)
{
    this->masterPID = masterPID;
}

PID HireSession::GetMasterPID() const
{
    return masterPID;
}

void HireSession::SetHiredNPC(gemNPC* hiredNPC)
{
    hiredPID = hiredNPC->GetPID();
    this->hiredNPC = hiredNPC;
}

gemNPC* HireSession::GetHiredNPC() const
{
    return hiredNPC;
}

void HireSession::SetOwner(gemActor* owner)
{
    ownerPID = owner->GetCharacterData()->GetPID();
    this->owner = owner;
}

gemActor* HireSession::GetOwner() const
{
    return owner;
}

bool HireSession::VerifyPendingHireConfigured()
{
    return (masterPID.IsValid() && !hireTypeName.IsEmpty() && !hireTypeNPCType.IsEmpty());
}

PID HireSession::GetOwnerPID()
{
    return ownerPID;
}

PID HireSession::GetHiredPID()
{
    return hiredPID;
}

const csString &HireSession::GetScript() const
{
    return script;
}

void HireSession::SetScript(const csString &newScript)
{
    script = newScript;
}

int HireSession::GetWorkLocationId()
{
    return workLocationID;
}

Location* HireSession::GetWorkLocation()
{
    return workLocation;
}

void HireSession::SetWorkLocation(Location* location)
{
    workLocationID = location->GetID();
    workLocation = location;
}

bool HireSession::IsVerified()
{
    return verified;
}

void HireSession::SetVerified(bool state)
{
    verified = state;
}

const csString &HireSession::GetVerifiedScript() const
{
    return verifiedScript;
}

void HireSession::SetVerifiedScript(const csString &newVerifiedScript)
{
    verifiedScript = newVerifiedScript;
}

csString HireSession::GetWorkLocationString()
{
    csString workLocationString;

    if(!workLocation)
    {
        workLocationString = "(Undefined)";
    }
    else
    {
        workLocationString = workLocation->ToString();
    }

    return workLocationString;
}

csString HireSession::GetTempWorkLocationString()
{
    csString workLocationString;

    if(!tempWorkLocation)
    {
        workLocationString = "(Undefined)";
    }
    else
    {
        workLocationString = tempWorkLocation->ToString();
    }

    return workLocationString;
}

Location* HireSession::GetTempWorkLocation()
{
    return tempWorkLocation;
}

void HireSession::SetTempWorkLocation(Location* location)
{
    tempWorkLocation = location;
    tempWorkLocationValid = false;
}

void HireSession::SetTempWorkLocationValid(bool valid)
{
    tempWorkLocationValid = valid;
}

bool HireSession::GetTempWorkLocationValid()
{
    return tempWorkLocationValid;
}



int HireSession::ApplyScript()
{
    /*
    csString parsed(script);

    csString npcReplacement(hiredNPC->GetName());
    npcReplacement += ":";

    parsed.ReplaceAll("NPC:",npcReplacement);
    return psserver->GetQuestManager()->ParseQuestScript(-1, parsed);
    */
    return psserver->GetQuestManager()->ParseCustomScript(-1, hiredNPC->GetName(), script);
}


bool HireSession::ShouldLoad()
{
    bool result = shouldLoad;
    shouldLoad = false;
    return result;

}
