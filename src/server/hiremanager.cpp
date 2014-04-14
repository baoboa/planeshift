/*
 * hiremanager.h  creator <andersr@pvv.org>
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
#include <util/psdatabase.h>
#include <util/location.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "hiremanager.h"
#include "hiresession.h"
#include "entitymanager.h"
#include "bulkobjects/pscharacterloader.h"
#include "client.h"
#include "adminmanager.h"
#include "npcmanager.h"
#include "questmanager.h"

HireManager::HireManager()
{
    Subscribe(&HireManager::HandleScriptMessage, MSGTYPE_HIRED_NPC_SCRIPT, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
}


HireManager::~HireManager()
{
    // Free all allocated sessions
    while(!hires.IsEmpty())
    {
        HireSession* session = hires.Last();
        delete session;
        hires.PopBack();
    }
}

bool HireManager::Initialize()
{
    if(!Load())
    {
        Error1("Failed to load hire sessions");
        return false;
    }

    return true; // Server OK to continue to load.
}

bool HireManager::Load()
{
    Result result(db->Select("SELECT * FROM npc_hired_npcs"));
    if(!result.IsValid())
    {
        return false;
    }

    // Get location manager to be able to resolve the work location of new sessions.
    LocationManager* locations = psserver->GetAdminManager()->GetLocationManager();
    if(!locations)
    {
        return false;
    }

    for(size_t i = 0; i < result.Count(); i++)
    {
        HireSession* session = new HireSession();
        if(!session->Load(result[i]))
        {
            Error2("Failed to load npc_hired_npcs nr %zu",i);
            delete session;
            return false;
        }

        // Resolve the work location.
        if (session->GetWorkLocationId() != 0)
        {
            Location* location = locations->FindLocation(session->GetWorkLocationId());
            if(!location)
            {
                Error2("Failed to resolve location %d",session->GetWorkLocationId());
                delete session;
                return false;
            }
            session->SetWorkLocation(location);
        }
        
        hires.PushBack(session);

        Debug3(LOG_HIRE, session->GetOwnerPID().Unbox(), "Loaded hire session between owner %s and npc %s",
               ShowID(session->GetOwnerPID()),ShowID(session->GetHiredPID()));
    }
    return true;
}

bool HireManager::StartHire(gemActor* owner)
{
    HireSession* session = NULL;

    // Check if a pending hire exists allready for this owner.
    session = GetPendingHire(owner);
    if(session)
    {
        return false; // Can only handle one pending hire at a time
    }

    // Create a new session
    session = CreateHireSession(owner);

    return (session != NULL); // True if a session was created
}

bool HireManager::SetHireType(gemActor* owner, const csString &name, const csString &npcType)
{
    HireSession* session = GetPendingHire(owner);
    if(!session)
    {
        // Indirect create a new session if no session is found.
        session = CreateHireSession(owner);
        if(!session)
        {
            return false; // Actor was not allowed to create a new session.
        }
    }

    // Update session object
    session->SetHireType(name, npcType);

    return true;
}

bool HireManager::SetHireMasterPID(gemActor* owner, PID masterPID)
{
    HireSession* session = GetPendingHire(owner);
    if(!session)
    {
        // Indirect create a new session if no session is found.
        session = CreateHireSession(owner);
        if(!session)
        {
            return false; // Actor was not allowed to create a new session.
        }
    }

    // Update session object
    session->SetMasterPID(masterPID);

    return true;
}

gemActor* HireManager::ConfirmHire(gemActor* owner)
{
    HireSession* session = GetPendingHire(owner);
    if(!session)
    {
        return NULL; // No session to confirm found.
    }

    // Are all the prerequesites configured?
    if(!session->VerifyPendingHireConfigured())
    {
        return NULL;
    }

    //Spawn hired NPC
    gemNPC* hiredNPC = EntityManager::GetSingleton().CreateHiredNPC(owner,
                       session->GetMasterPID(),
                       session->GetHireTypeName());
    if(!hiredNPC)
    {
        Error3("Failed to create npc for hire session for owner %s and hired_npc %s",
               ShowID(session->GetOwnerPID()), ShowID(session->GetHiredPID()));
        // TODO: What to do here!!
        // Delete session??
        return NULL;
    }

    session->SetHiredNPC(hiredNPC);
    RemovePendingHire(owner);

    if(!session->Save(true))  // This is a new session.
    {
        Error3("Failed to save hire session for owner %s and hired_npc %s",
               ShowID(session->GetOwnerPID()), ShowID(session->GetHiredPID()))
    }

    return hiredNPC;
}

bool HireManager::HandleScriptMessageRequest(uint32_t clientnum, gemActor* owner, gemNPC* hiredNPC)
{
    HireSession* session = GetSessionByPIDs(owner->GetPID(), hiredNPC->GetPID());
    if(!session)
    {
        return false;
    }

    // Display Script Dialoge.
    psHiredNPCScriptMessage msg(clientnum, psHiredNPCScriptMessage::REQUEST_REPLY, hiredNPC->GetEID(),
                                session->GetWorkLocationString().GetDataSafe(),
                                true,
                                session->GetScript().GetDataSafe());
    msg.SendMessage();

    return true;
}

bool HireManager::ReleaseHire(gemActor* owner, gemNPC* hiredNPC)
{
    HireSession* session = GetSessionByPIDs(owner->GetPID(), hiredNPC->GetPID());
    if(!session)
    {
        return false;
    }

    if(!EntityManager::GetSingleton().DeleteActor(hiredNPC))
    {
        Error1("Failed to delete hired NPC!!!");
    }

    // Remove hire record from DB.
    session->Delete();

    hires.Delete(session);

    delete session;

    return true;
}

bool HireManager::AllowedToHire(gemActor* owner)
{
    // TODO: Include check to verify if actor satisfy the requirements to
    //       hire a NPC. This should be a math script.
    return true;
}

bool HireManager::AddHiredNPC(gemNPC* hiredNPC)
{
    Debug2(LOG_HIRE, hiredNPC->GetPID().Unbox(), "Checking hired for %s", ShowID(hiredNPC->GetPID()));

    // A Hired NPC will only be hired by one NPC so geting the one session is ok.
    HireSession* session = GetSessionByHirePID(hiredNPC->GetPID());
    if(!session)
    {
        return false; // This NPC was not among the hired NPCs.
    }
    Debug2(LOG_HIRE, hiredNPC->GetPID().Unbox(), "Found hired NPC %s", ShowID(hiredNPC->GetPID()));

    session->SetHiredNPC(hiredNPC);

    // Connect the owner of the hire to this NPC.
    hiredNPC->SetOwner(session->GetOwner());
    hiredNPC->GetCharacterData()->SetHired(true);

    // Apply the script.
    if(session->ShouldLoad() && (session->GetScript().Length() > 0))
    {
        int line = session->ApplyScript();
        if(line!=0)
        {
            Error2("Failed to parse hired npc dialog with error %s",
                   psserver->GetQuestManager()->LastError());
            return false;
        }
    }

    return true;
}

bool HireManager::AddOwner(gemActor* owner)
{
    Debug2(LOG_HIRE, owner->GetPID().Unbox(), "Checking owner for %s", ShowID(owner->GetPID()));

    csList<HireSession*>::Iterator iter(hires);

    // Need to loop thrugh all sessions since one player might hire multiple NPCs
    // if allowed by hire requirements.
    while(iter.HasNext())
    {
        HireSession* session = iter.Next();
        if(session->GetOwnerPID() == owner->GetPID())
        {
            Debug2(LOG_HIRE, owner->GetPID().Unbox(), "Found owner %s", ShowID(owner->GetPID()));

            session->SetOwner(owner);
            if(session->GetHiredNPC())
            {
                session->GetHiredNPC()->SetOwner(owner);
            }
        }
    }

    return true;
}


HireSession* HireManager::CreateHireSession(gemActor* owner)
{
    HireSession* session = NULL;

    if(AllowedToHire(owner))
    {
        // Create a new session
        session = new HireSession(owner);

        // Store the session in the manager
        hires.PushBack(session);
        pendingHires.PutUnique(owner->GetPID(),session);

        Debug2(LOG_HIRE, owner->GetPID().Unbox(), "New hire session created for %s", ShowID(owner->GetPID()));
    }

    return session;
}

HireSession* HireManager::GetPendingHire(gemActor* owner)
{
    return pendingHires.Get(owner->GetPID(),NULL);
}

HireSession* HireManager::GetSessionByHirePID(PID hiredPID)
{
    csList<HireSession*>::Iterator iter(hires);

    while(iter.HasNext())
    {
        HireSession* session = iter.Next();
        if(session->GetHiredPID() == hiredPID)
        {
            return session;
        }
    }
    return NULL;
}

HireSession* HireManager::GetSessionByPIDs(PID ownerPID, PID hiredPID)
{
    csList<HireSession*>::Iterator iter(hires);

    while(iter.HasNext())
    {
        HireSession* session = iter.Next();
        if(session->GetOwnerPID() == ownerPID && session->GetHiredPID() == hiredPID)
        {
            return session;
        }
    }
    return NULL;
}

void HireManager::RemovePendingHire(gemActor* owner)
{
    pendingHires.DeleteAll(owner->GetPID());
}

void HireManager::HandleScriptMessage(MsgEntry* me, Client* client)
{
    psHiredNPCScriptMessage msg(me);
    if(!msg.valid)
    {
        Error1("Received not valid psHiredNPCScriptMessage.");
        return;
    }

    // Resolv the hired NPC from the script message.
    gemNPC* hiredNPC = EntityManager::GetSingleton().GetGEM()->FindNPCEntity(msg.hiredEID);
    if(!hiredNPC)
    {
        return;
    }

    switch(msg.command)
    {
        case psHiredNPCScriptMessage::REQUEST:
        {
            HandleScriptMessageRequest(client->GetClientNum(), client->GetActor(), hiredNPC);
        }
        break;
        case psHiredNPCScriptMessage::VERIFY:
        {
            csString errorMessage;

            bool valid = ValidateScript(client->GetPID(), hiredNPC->GetPID(), msg.script, errorMessage);
            if(!valid)
            {
                psserver->SendSystemError(client->GetClientNum(), "Failed to verify script.");
            }

            psHiredNPCScriptMessage reply(client->GetClientNum(), psHiredNPCScriptMessage::VERIFY_REPLY,
                                          msg.hiredEID, valid, errorMessage);
            reply.SendMessage();

        }
        break;
        case psHiredNPCScriptMessage::COMMIT:
        {
            HandleScriptMessageCommit(client->GetClientNum(), client->GetPID(), hiredNPC->GetPID());
        }
        break;
        case psHiredNPCScriptMessage::CANCEL:
        {
            CancelScript(client->GetPID(), hiredNPC->GetPID());
        }
        break;
        case psHiredNPCScriptMessage::WORK_LOCATION:
        {
            WorkLocation(client->GetActor(), hiredNPC);
        }
        break;
        case psHiredNPCScriptMessage::CHECK_WORK_LOCATION_RESULT:
        {
            CheckWorkLocationResult(hiredNPC, msg.choice, msg.errorMessage);
        }
        break;
    }
}


bool HireManager::ValidateScript(PID ownerPID, PID hiredPID, const csString &script, csString &errorMessage)
{
    HireSession* session = GetSessionByPIDs(ownerPID, hiredPID);
    if(!session)
    {
        errorMessage = "Error: No session found.";
        return false;
    }

    // Check work location
    if(session->GetTempWorkLocation() == NULL)
    {
        session->SetVerified(false);
        errorMessage = "No work location defined.";
        return false; // No work location defined.
    }

    if(!session->GetTempWorkLocationValid())
    {
        session->SetVerified(false);
        errorMessage = "Work location isn't valid.";
        return false; // Not a valid work postion.
    }

    if(script.IsEmpty())
    {
        session->SetVerified(false);
        errorMessage = "No script defined.";
        return false; // Empty script not valid.
    }

    //TODO: Add validation of script. For now accept.

    // Store the verified script. Client need to confirm to activate it.
    session->SetVerifiedScript(script);

    session->SetVerified(true);

    errorMessage = "Success.";
    return true;
}

bool HireManager::HandleScriptMessageCommit(uint32_t clientnum, PID ownerPID, PID hiredPID)
{
    HireSession* session = GetSessionByPIDs(ownerPID, hiredPID);
    if(!session)
    {
        return false;
    }

    gemNPC* hiredNPC = session->GetHiredNPC();

    if(session->IsVerified())
    {
        // Move the verified script over to be the current script.
        session->SetScript(session->GetVerifiedScript());
        Location* tempLocation = session->GetTempWorkLocation();

        // Create and set the new work location
        Location* location = CreateUpdateLocation("work",
                             tempLocation->GetName(),
                             tempLocation->GetSector(EntityManager::GetSingleton().GetEngine()),
                             tempLocation->GetPosition(),
                             tempLocation->GetRotationAngle());
        session->SetWorkLocation(location);

        // Save the updated hire record.
        session->Save();

        // Apply the new script to the server
        int line = session->ApplyScript();
        if(line != 0) // Null is success
        {
            psHiredNPCScriptMessage msg(clientnum, psHiredNPCScriptMessage::COMMIT_REPLY, hiredNPC->GetEID(),
                                        false, psserver->GetQuestManager()->LastError());
            msg.SendMessage();
            return false;
        }
    }
    else
    {
        psHiredNPCScriptMessage msg(clientnum, psHiredNPCScriptMessage::COMMIT_REPLY, hiredNPC->GetEID(),
                                    false, "Error: No session.");
        msg.SendMessage();

        return false;
    }

    // Display Script Dialoge.
    psHiredNPCScriptMessage msg(clientnum, psHiredNPCScriptMessage::COMMIT_REPLY, hiredNPC->GetEID(),
                                true, "Successfull commit");
    msg.SendMessage();

    return true;
}


bool HireManager::CancelScript(PID ownerPID, PID hiredPID)
{
    HireSession* session = GetSessionByPIDs(ownerPID, hiredPID);
    if(!session)
    {
        return false;
    }

    // Cleare the verified script store.
    session->SetVerifiedScript("");

    return true;
}

bool HireManager::WorkLocation(gemActor* owner, gemNPC* hiredNPC)
{
    HireSession* session = GetSessionByPIDs(owner->GetPID(), hiredNPC->GetPID());
    if(!session)
    {
        return false;
    }

    csVector3 myPos;
    float myRot = 0.0;
    iSector* mySector = 0;
    csString mySectorName;

    owner->GetPosition(myPos, myRot, mySector);

    Location* location = CreateUpdateLocation("temp_work", hiredNPC->GetName(), mySector, myPos, myRot);
    if(location)
    {
        // Update session with this temporary location.
        session->SetTempWorkLocation(location);

        // Verify that it is possible to navigate to the location.
        psserver->npcmanager->CheckWorkLocation(hiredNPC, location);

        // Update client with new position text.
        psHiredNPCScriptMessage msg(owner->GetClient()->GetClientNum(),
                                    psHiredNPCScriptMessage::WORK_LOCATION_UPDATE,
                                    hiredNPC->GetEID(), location->ToString());
        msg.SendMessage();
    }
    else
    {
        psHiredNPCScriptMessage msg(owner->GetClient()->GetClientNum(),
                                    psHiredNPCScriptMessage::WORK_LOCATION_UPDATE,
                                    hiredNPC->GetEID(), "(Failed)");
        msg.SendMessage();
    }


    return true;
}

Location* HireManager::CreateUpdateLocation(const char* type, const char* name,
        iSector* sector, const csVector3 &position, float angle)
{
    // Create or update an temporary location that isn't saved to db.
    LocationManager* locations = psserver->GetAdminManager()->GetLocationManager();
    if(!locations)
    {
        return NULL;
    }

    LocationType* locationType = locations->FindLocation(type);
    if(!locationType)
    {
        locationType = locations->CreateLocationType(db, type);
        if(!locationType)
        {
            return NULL;
        }
        psserver->npcmanager->LocationTypeAdd(locationType);
    }

    Location* location = locations->FindLocation(locationType, name);
    if(!location)
    {
        location = locations->CreateLocation(db, locationType, name,
                                             position, sector, 1.0f, angle, "");
        if(location)
        {
            psserver->npcmanager->LocationCreated(location);
        }
    }
    else
    {
        if(location->Adjust(db, position, sector, angle))
        {
            psserver->npcmanager->LocationAdjusted(location);
        }
    }

    return location;
}


bool HireManager::CheckWorkLocationResult(gemNPC* hiredNPC, bool valid, const char* errorMessage)
{
    HireSession* session = GetSessionByHirePID(hiredNPC->GetPID());
    if(!session)
    {
        Error2("Faild to find session for hired NPC %s",ShowID(hiredNPC->GetPID()));
        return false;
    }

    session->SetTempWorkLocationValid(valid);

    gemActor* owner = session->GetOwner();
    if(!owner)
    {
        Error2("Session has no owner for hired NPC %s",ShowID(hiredNPC->GetPID()));
        return false;
    }

    if(owner->GetClient())
    {
        psHiredNPCScriptMessage msg(owner->GetClient()->GetClientNum(),
                                    psHiredNPCScriptMessage::WORK_LOCATION_RESULT,
                                    hiredNPC->GetEID(), valid, errorMessage);
        msg.SendMessage();
    }


    return true;
}
