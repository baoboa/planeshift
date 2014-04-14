/*
* npcmanager.cpp by Keith Fulton <keith@paqrat.com>
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
#include <ctype.h>
#include <csutil/sha256.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================

#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/npcmessages.h"

#include "util/eventmanager.h"
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/log.h"
#include "util/psconst.h"
#include "util/strutil.h"
#include "util/psxmlparser.h"
#include "util/mathscript.h"
#include "util/serverconsole.h"
#include "util/command.h"

#include "engine/psworld.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/servervitals.h"
#include "bulkobjects/psraceinfo.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "events.h"
#include "psserver.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "gem.h"
#include "combatmanager.h"
#include "authentserver.h"
#include "entitymanager.h"
#include "spawnmanager.h"
#include "usermanager.h"
#include "clients.h"                // Client, and ClientConnectionSet classes
#include "client.h"                 // Client, and ClientConnectionSet classes
#include "psserverchar.h"
#include "creationmanager.h"
#include "npcmanager.h"
#include "workmanager.h"
#include "weathermanager.h"
#include "adminmanager.h"
#include "netmanager.h"
#include "spellmanager.h"
#include "progressionmanager.h"
#include "scripting.h"
#include "hiremanager.h"

class psNPCManagerTick : public psGameEvent
{
protected:
    NPCManager* npcmgr;

public:
    psNPCManagerTick(int offsetticks, NPCManager* c);
    virtual void Trigger();  ///< Abstract event processing function
};

const int NPC_TICK_INTERVAL = 300;  //msec
const size_t MAX_NPC_COMMANS_MESSAGE_SIZE = 15000; // bytes

/**
 * This class is the relationship of Owner to Pet ( which includes Familiars ).
 */
class PetOwnerSession : public iDeleteObjectCallback
{
public:
    PID ownerID; ///< Character ID of the owner
    PID petID; ///< Character ID of the pet

    gemActor* owner;
    bool isActive;

    double elapsedTime; ///< create time
    double deathLockoutTime;
    double trainingLockoutTime;
    double dismissLockoutTime;
    double depletedLockoutTime;

    // Cached values
    double maxTime; // Result of GetMaxPetTime updated by CheckSession

    /**
     * Constructor.
     */
    PetOwnerSession()
    {
        owner = NULL;
        isActive = false;

        elapsedTime = 0.0;
        deathLockoutTime = 0.0;
        trainingLockoutTime = 0.0;
        dismissLockoutTime = 0.0;
        depletedLockoutTime = 0.0;

        maxTime = 0.0; // Updated by CheckSession
    };

    /**
     * Constructor.
     */
    PetOwnerSession(gemActor* owner, psCharacter* pet)
    {
        this->ownerID = owner->GetCharacterData()->GetPID();
        this->petID = pet->GetPID();

        this->owner = owner;
        this->owner->RegisterCallback(this);

        CPrintf(CON_DEBUG, "Created PetSession (%s, %s)\n", owner->GetName(), ShowID(pet->GetPID()));

        isActive = true;

        elapsedTime = pet->GetPetElapsedTime();
        deathLockoutTime = 0.0;
        trainingLockoutTime = 0.0;
        dismissLockoutTime = 0.0;
        depletedLockoutTime = 0.0;

        maxTime = 0.0; // Updated by CheckSession
    };

    virtual ~PetOwnerSession()
    {
        if(owner)
        {
            owner->UnregisterCallback(this);
        }
    };

    /**
     * Renable Time tracking for returning players.
     *
     * Reconnect happen upon summon if the client has
     * been offline.
     */
    void Reconnect(gemActor* owner)
    {
        if(owner)
        {
            if(this->owner)
            {
                owner->UnregisterCallback(this);
            }

            this->owner = owner;
            this->owner->RegisterCallback(this);
        }
    };

    /**
     * Disable time tracking for disconnected clients.
     */
    virtual void DeleteObjectCallback(iDeleteNotificationObject* object)
    {
        gemActor* sender = (gemActor*)object;

        if(sender && sender == owner)
        {
            if(owner)
            {
                // Treat disconnection clients as Dismiss. This is important so that
                // dismiss lockout time is started. That will guide if elapsed time
                // should restart or continue when reconnecting.
                if(isActive)
                {
                    Dismiss();
                }

                owner->UnregisterCallback(this);
                owner = NULL;
            }
        }
    };

    /**
     * Update time in game for this session.
     *
     * @param elapsed Number of secounds elapsed since last update.
     */
    void UpdateElapsedTime(float elapsed)
    {
        gemActor* pet = NULL;

        if(owner && isActive)
        {
            elapsedTime += elapsed;

            //Update Session status using new elapsedTime values
            CheckSession();

            // Check if this player is riding the pet.
            if(owner->IsMounted() && owner->GetMount()->GetPID() == petID)
            {
                Notify3(LOG_PETS, "Updating pet elapsed time for %s's mount to %f",owner->GetName(),elapsedTime);

                // Has the mounted pet been deactivated
                if(!isActive)
                {
                    EntityManager::GetSingleton().RemoveRideRelation(owner);
                }
            }

            size_t numPets = owner->GetClient()->GetNumPets();
            // Check to make sure this pet is still summoned
            for(size_t i = 0; i < numPets; i++)
            {
                pet = owner->GetClient()->GetPet(i);
                if(pet && pet->GetCharacterData() && pet->GetCharacterData()->GetPID() == petID)
                {
                    break;
                }
                else
                {
                    pet = NULL;
                }
            }

            if(pet)
            {
                Notify3(LOG_PETS, "Updating pet elapsed time for %s to %f",pet->GetName(),elapsedTime);

                pet->GetCharacterData()->SetPetElapsedTime(elapsedTime);

                if(!isActive)    // past Session time
                {
                    psserver->CharacterLoader.SaveCharacterData(pet->GetCharacterData(), pet, true);

                    CPrintf(CON_NOTIFY,"NPCManager Removing familiar %s from owner %s.\n",pet->GetName(),pet->GetName());
                    owner->GetClient()->SetFamiliar(NULL);
                    EntityManager::GetSingleton().RemoveActor(pet);
                    psserver->SendSystemInfo(owner->GetClientID(), "You feel your power to maintain your pet wane.");
                }
            }

        }

        // Decrease all lockout times
        deathLockoutTime -= elapsed;
        if(deathLockoutTime < 0.0f)
        {
            deathLockoutTime = 0.0f;
        }

        trainingLockoutTime -= elapsed;
        if(trainingLockoutTime < 0.0f)
        {
            trainingLockoutTime = 0.0f;
        }

        if(dismissLockoutTime > 0)
        {
            dismissLockoutTime -= elapsed;
            if(dismissLockoutTime < 0.0f)
            {
                dismissLockoutTime = 0.0f;
                DismissLockoutCompleted();
            }
        }

        depletedLockoutTime -= elapsed;
        if(depletedLockoutTime < 0.0f)
        {
            depletedLockoutTime = 0.0f;
        }
    };

    /**
     * Used to verify the session should still be valid.
     */
    bool CheckSession()
    {
        // Recalculate max time since that may depend on player skills.
        maxTime = GetMaxPetTime();

        if(elapsedTime >= maxTime)
        {
            if(owner->GetClient()->GetSecurityLevel() < GM_LEVEL_9)
            {
                Notify3(LOG_PETS, "PetSession marked invalid (%s, %s)\n", owner->GetName(), ShowID(petID));

                Depleted();

                return false;
            }
            else
            {
                return true;
            }
        }

        return true;
    }

    /**
     * Called when a pet has been killed.
     *
     * Setting death lockout time for this killed pet.
     */
    void HasBeenKilled()
    {
        deathLockoutTime = GetDeathLockoutTime();
    }

    /**
     * The pet is in killed lockout a time periode after it has been killed.
     */
    bool IsInKilledLockout() const
    {
        return deathLockoutTime > 0.0f;
    }

    /**
     * Called each time the pet receive some training.
     *
     * Start a new training lockout periode. The training lockout
     * periode is to prevent overtraining by repeating the same
     * operation with no delay. The pet will not be any better
     * by that.
     */
    void ReceivedTraining()
    {
        trainingLockoutTime = GetTrainingLockoutTime();
    }

    /**
     * The pet is in training lockout a time periode after it has received some training.
     *
     * The training lockout periode is to prevent overtraining by repeating the same
     * operation with no delay. The pet will not be any better by that.
     */
    bool IsInTrainingLockout() const
    {
        return trainingLockoutTime > 0.0f;
    }

    /**
     * Called when the pet is depleted.
     */
    void Depleted()
    {
        isActive = false;

        depletedLockoutTime = GetDepletedLockoutTime();
    }

    /**
     * The pet is in a depleted lockout for a peridode after max time has been reached.
     */
    bool IsInDepletedLockout() const
    {
        return depletedLockoutTime > 0.0f;
    }

    /**
     * Called when the pet is dismissed.
     */
    void Dismiss()
    {
        isActive = false;

        dismissLockoutTime = GetDismissLockoutTime();
    }

    bool IsInDismissLockoutTime()
    {
        return dismissLockoutTime > 0.0f;
    }

    void DismissLockoutCompleted()
    {
        Debug2(LOG_PETS, petID.Unbox(), "Dismiss lockout completed for %s", ShowID(petID));
    }


    /**
     * Called when a pet has been summoned.
     */
    void Summon()
    {
        isActive = true;

        if(!IsInDismissLockoutTime())
        {
            elapsedTime = 0.0;
        }
    }


    /**
     * Uses a MathScript to calculate the time/periode an depleted pet can't be summoned again.
     */
    double GetDepletedLockoutTime()
    {
        double lockoutTime = 10.0; // Default to 10 sec.

        MathScript* script = psserver->GetNPCManager()->GetPetDepletedLockoutTime();
        if(script && owner)
        {
            MathEnvironment env;
            env.Define("Actor", owner->GetCharacterData());
            env.Define("Skill", owner->GetCharacterData()->Skills().GetSkillRank(psserver->GetNPCManager()->GetPetSkill()).Current());
            script->Evaluate(&env);
            MathVar* timeValue = env.Lookup("DepletedLockoutTime");
            lockoutTime = timeValue->GetValue();

            Debug3(LOG_PETS, 0, "Calculated depleted lockout time for %s to %f",owner->GetName(), lockoutTime);
        }

        return lockoutTime;
    }


    /**
     * Uses a MathScript to calculate the time/periode an dismissed pet can't be summoned again.
     */
    double GetDismissLockoutTime()
    {
        double lockoutTime = 10.0; // Default to 10 sec.

        MathScript* script =
            psserver->GetNPCManager()->GetPetDismissLockoutTime();
        if(script && owner)
        {
            MathEnvironment env;
            env.Define("Actor", owner->GetCharacterData());
            env.Define("Skill", owner->GetCharacterData()->Skills().GetSkillRank(psserver->GetNPCManager()->GetPetSkill()).Current());
            script->Evaluate(&env);
            MathVar* timeValue = env.Lookup("DismissLockoutTime");
            lockoutTime = timeValue->GetValue();
            Debug3(LOG_PETS, 0, "Calculated dismiss lockout time for %s to %f",owner->GetName(), lockoutTime);
        }

        return lockoutTime;
    }

    /**
     * Uses a MathScript to calculate the maximum amount of time a Pet can remain in world.
     */
    double GetMaxPetTime()
    {
        double maxTime = 5 * 60.0; // Default to 5 min in ticks.

        MathScript* maxPetTime = psserver->GetNPCManager()->GetMaxPetTime();
        if(maxPetTime && owner)
        {
            MathEnvironment env;
            env.Define("Actor", owner->GetCharacterData());
            env.Define("Skill", owner->GetCharacterData()->Skills().GetSkillRank(psserver->GetNPCManager()->GetPetSkill()).Current());
            maxPetTime->Evaluate(&env);
            MathVar* timeValue = env.Lookup("MaxTime");
            maxTime = timeValue->GetValue();
        }

        return maxTime;
    }

    /**
     * Uses a MathScript to calculate the time/periode an killed pet can't be summoned again.
     */
    double GetDeathLockoutTime()
    {
        double lockoutTime = 10.0;  // Default to 10 sec.

        MathScript* script = psserver->GetNPCManager()->GetPetDeathLockoutTime();
        if(script && owner)
        {
            MathEnvironment env;
            env.Define("Actor", owner->GetCharacterData());
            env.Define("Skill", owner->GetCharacterData()->Skills().GetSkillRank(psserver->GetNPCManager()->GetPetSkill()).Current());
            script->Evaluate(&env);
            MathVar* timeValue = env.Lookup("DeathLockoutTime");
            lockoutTime = timeValue->GetValue();
            Debug3(LOG_PETS, 0, "Calculated death lockout time for %s to %f",owner->GetName(), lockoutTime);
        }

        return lockoutTime;
    }

    /**
     * Uses a MathScript to calculate the time/periode an npc can't be trained after receving training.
     */
    double GetTrainingLockoutTime()
    {
        double lockoutTime = 10.0;  // Default to 10 sec.

        MathScript* script = psserver->GetNPCManager()->GetPetTrainingLockoutTime();
        if(script && owner)
        {
            MathEnvironment env;
            env.Define("Actor", owner->GetCharacterData());
            env.Define("Skill", owner->GetCharacterData()->Skills().GetSkillRank(psserver->GetNPCManager()->GetPetSkill()).Current());
            script->Evaluate(&env);
            MathVar* timeValue = env.Lookup("TrainingLockoutTime");
            lockoutTime = timeValue->GetValue();
            Debug3(LOG_PETS, 0, "Calculated training lockout time for %s to %f",owner->GetName(), lockoutTime);
        }

        return lockoutTime;
    }

};

NPCManager::NPCManager(ClientConnectionSet* pCCS,
                       psDatabase* db,
                       EventManager* evtmgr,
                       GEMSupervisor* gemsupervisor,
                       CacheManager* cachemanager,
                       EntityManager* entitymanager)
{
    clients      = pCCS;
    database     = db;
    eventmanager = evtmgr;
    gemSupervisor = gemsupervisor;
    cacheManager = cachemanager;
    entityManager = entitymanager;

    Subscribe(&NPCManager::HandleAuthentRequest,MSGTYPE_NPCAUTHENT,REQUIRE_ANY_CLIENT);
    Subscribe(&NPCManager::HandleCommandList,MSGTYPE_NPCCOMMANDLIST,REQUIRE_ANY_CLIENT);
    Subscribe(&NPCManager::HandleConsoleCommand,MSGTYPE_NPC_COMMAND,REQUIRE_ANY_CLIENT);
    Subscribe(&NPCManager::HandleNPCReady,MSGTYPE_NPCREADY, REQUIRE_ANY_CLIENT);
    Subscribe(&NPCManager::HandleSimpleRenderMesh,MSGTYPE_SIMPLE_RENDER_MESH,REQUIRE_ANY_CLIENT);

    Subscribe(&NPCManager::HandleDamageEvent,MSGTYPE_DAMAGE_EVENT,NO_VALIDATION);
    Subscribe(&NPCManager::HandleDeathEvent,MSGTYPE_DEATH_EVENT,NO_VALIDATION);
    Subscribe(&NPCManager::HandlePetCommand,MSGTYPE_PET_COMMAND,REQUIRE_ANY_CLIENT);
    Subscribe(&NPCManager::HandlePetSkill,MSGTYPE_PET_SKILL,REQUIRE_ANY_CLIENT);

    PrepareMessage();

    psNPCManagerTick* tick = new psNPCManagerTick(NPC_TICK_INTERVAL,this);
    eventmanager->Push(tick);

    MathScriptEngine* eng = psserver->GetMathScriptEngine();
    petRangeScript = eng->FindScript("CalculateMaxPetRange");
    petReactScript = eng->FindScript("CalculatePetReact");
    petDepletedLockoutTime = eng->FindScript("CalculatePetDepletedLockoutTime");
    petDismissLockoutTime = eng->FindScript("CalculatePetDismissLockoutTime");
    maxPetTime = eng->FindScript("CalculateMaxPetTime");
    petDeathLockoutTime = eng->FindScript("CalculatePetDeathLockoutTime");
    petTrainingLockoutTime = eng->FindScript("CalculatePetTrainingLockoutTime");

    //for now keep default the first skill
    petSkill = psserver->GetCacheManager()->getOptionSafe("npcmanager:petskill","0");
}

bool NPCManager::Initialize()
{
    if(!petRangeScript)
    {
        Error1("No CalculateMaxPetRange script!");
        return false;
    }

    if(!petReactScript)
    {
        Error1("No CalculatePetReact script!");
        return false;
    }

    return true;
}


NPCManager::~NPCManager()
{
    csHash<PetOwnerSession*, PID>::GlobalIterator iter(OwnerPetList.GetIterator());
    while(iter.HasNext())
    {
        delete iter.Next();
    }
    delete outbound;
}

void NPCManager::HandleNPCReady(MsgEntry* me,Client* client)
{
    gemSupervisor->ActivateNPCs(client->GetAccountID());
    // NPC Client is now ready so add onto superclients list
    client->SetReady(true);
    superclients.Push(PublishDestination(client->GetClientNum(), client, 0, 0));

    // TODO: Consider move this to a earlier stage in the load process
    // Update the superclient with entity stats
    psserver->npcmanager->SendAllNPCStats(client);

    // Notify console user that NPC Client is up and running.
    CPrintf(CON_DEBUG, "NPC Client '%s' load completed.\n",client->GetName());
}


void NPCManager::HandleSimpleRenderMesh(MsgEntry* me, Client* client)
{
    Debug1(LOG_SUPERCLIENT, 0,"NPCManager handling Simple Render Mesh\n");
    //TODO: Only send to clients that have requested this
    psserver->GetEventManager()->Broadcast(me, NetBase::BC_EVERYONE);
}



void NPCManager::HandleDamageEvent(MsgEntry* me,Client* client)
{
    psDamageEvent evt(me);

    // NPC's need to know they were hit using a Perception
    if(evt.attacker!=NULL  &&  evt.target->GetNPCPtr())  // if npc damaged
    {
        QueueDamagePerception(evt.attacker, evt.target->GetNPCPtr(), evt.damage);
    }
}

void NPCManager::HandleDeathEvent(MsgEntry* me,Client* client)
{
    Debug1(LOG_SUPERCLIENT, 0,"NPCManager handling Death Event\n");
    psDeathEvent evt(me);

    QueueDeathPerception(evt.deadActor);
}


void NPCManager::HandleConsoleCommand(MsgEntry* me, Client* client)
{
    csString buffer;

    psServerCommandMessage msg(me);
    printf("Got command: %s\n", msg.command.GetDataSafe());

    size_t i = msg.command.FindFirst(' ');
    csString word;
    msg.command.SubString(word,0,i);
    const COMMAND* cmd = find_command(word.GetDataSafe());

    if(cmd && cmd->allowRemote)
    {
        int ret = execute_line(msg.command, &buffer);
        if(ret == -1)
            buffer = "Error executing command on the server.";
    }
    else
    {
        buffer = cmd ? "That command is not allowed to be executed remotely"
                 : "No command by that name.  Please try again.";
    }
    printf("%s\n", buffer.GetData());

    psServerCommandMessage retn(me->clientnum, buffer);
    retn.SendMessage();
}


void NPCManager::HandleAuthentRequest(MsgEntry* me,Client* notused)
{
    Client* client = clients->FindAny(me->clientnum);
    if(!client)
    {
        Error1("NPC Manager got authentication message from already connected client!");
        return;
    }

    psNPCAuthenticationMessage msg(me);

    csString status;
    status.Format("%s, %u, Received NPC Authentication Message", (const char*) msg.sUser, me->clientnum);
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

    // CHECK 1: Networking versions match
    if(!msg.NetVersionOk())
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Version mismatch.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        //psserver->RemovePlayer (me->clientnum, "You are not running the correct version of PlaneShift for this server.");
        Error2("Superclient '%s' is not running the correct version of PlaneShift for this server.",
               (const char*)msg.sUser);
        return;
    }

    // CHECK 2: Is the server ready yet to accept connections
    if(!psserver->IsReady())
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Server not ready");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);

        if(psserver->HasBeenReady())
        {
            // Locked
            Error2("Superclient '%s' authentication request rejected: Server is up but about to shutdown.\n",
                   (const char*)msg.sUser);
        }
        else
        {
            // Not ready
            // psserver->RemovePlayer(me->clientnum,"The server is up but not fully ready to go yet. Please try again in a few minutes.");

            Error2("Superclient '%s' authentication request rejected: Server not ready.\n",
                   (const char*)msg.sUser);
        }
        return;
    }

    // CHECK 3: Is the client is already logged in?
    if(msg.sUser.Length() == 0)
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Empty username.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        Error1("No username specified.\n");
        return;
    }

    msg.sUser.Downcase();
    msg.sUser.SetAt(0,toupper(msg.sUser.GetAt(0)));



    // CHECK 4: Check to see if the login is correct.

    Notify2(LOG_SUPERCLIENT, "Check Superclient Login for: '%s'\n", (const char*)msg.sUser);
    psAccountInfo* acctinfo=cacheManager->GetAccountInfoByUsername((const char*)msg.sUser);

    //check  authentication errors
    if(acctinfo==NULL)
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Username not found.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Username not found).\n",(const char*)msg.sUser);
        delete acctinfo;
        return;
    }

    csString password = CS::Utility::Checksum::SHA256::Encode(msg.sPassword).HexString();

    if(strcmp(acctinfo->password,(const char*)password))
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Bad password.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        Notify2(LOG_CONNECTIONS,"User '%s' authentication request rejected (Bad password).\n",(const char*)msg.sUser);
        delete acctinfo;
        return;
    }

    client->SetPID(0);
    client->SetSecurityLevel(acctinfo->securitylevel);

    if(acctinfo->securitylevel!= 99)
    {
        psDisconnectMessage disconnectMsg(client->GetClientNum(), 0, "Wrong security level.");
        psserver->GetEventManager()->Broadcast(disconnectMsg.msg, NetBase::BC_FINALPACKET);
        // psserver->RemovePlayer(me->clientnum, "Login id does not have superclient security rights.");

        Error3("User '%s' authentication request rejected (security level was %d).\n",
               (const char*)msg.sUser, acctinfo->securitylevel);
        delete acctinfo;
        return;
    }


    Client* existingClient = clients->FindAccount(acctinfo->accountid, client->GetClientNum());
    if(existingClient)// username already logged in
    {
        // invalid
        // psserver->RemovePlayer(me->clientnum,"You are already logged on to this server. If you were disconnected, please wait 30 seconds and try again.");
        // Diconnect existing superclient

        Error2("Superclient '%s' already logged in. Logging out existing client\n",
               (const char*)msg.sUser);
        psserver->RemovePlayer(existingClient->GetClientNum(),"Existing connection overridden by new login.");
    }

    // *********Successful superclient login here!**********

    time_t curtime = time(NULL);
    struct tm* gmttime;
    gmttime = gmtime(&curtime);
    csString buf(asctime(gmttime));
    buf.Truncate(buf.Length()-1);

    Debug3(LOG_SUPERCLIENT,0,"Superclient '%s' connected at %s.\n",(const char*)msg.sUser, (const char*)buf);

    client->SetName(msg.sUser);
    client->SetAccountID(acctinfo->accountid);
    client->SetSuperClient(true);

    csString ipAddr = client->GetIPAddress();
    //TODO:    database->UpdateLoginDate(cid,addr);

    psserver->GetAuthServer()->SendMsgStrings(me->clientnum, false);

    SendMapList(client);
    SendRaces(client);

    psserver->GetWeatherManager()->SendClientGameTime(me->clientnum);

    delete acctinfo;

    status.Format("%s, %u, %s, Superclient logged in", (const char*) msg.sUser, me->clientnum, ipAddr.GetDataSafe());
    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);
}

void NPCManager::Disconnect(Client* client)
{
    // Deactivate all the NPCs that are managed by this client
    gemSupervisor->StopAllNPCs(client->GetAccountID());

    // Disconnect the superclient
    for(size_t i=0; i< superclients.GetSize(); i++)
    {
        PublishDestination &pd = superclients[i];
        if((Client*)pd.object == client)
        {
            superclients.DeleteIndex(i);
            Debug1(LOG_SUPERCLIENT, 0,"Deleted superclient from NPCManager.\n");
            return;
        }
    }
    CPrintf(CON_DEBUG, "Attempted to delete unknown superclient.\n");
}

void NPCManager::SendMapList(Client* client)
{
    psWorld* psworld = entityManager->GetWorld();

    csString regions;
    psworld->GetAllRegionNames(regions);

    psMapListMessage list(client->GetClientNum(),regions);
    list.SendMessage();
}

void NPCManager::SendRaces(Client* client)
{
    uint32_t count = (uint32_t)cacheManager->GetRaceInfoCount();

    psNPCRaceListMessage newmsg(client->GetClientNum(),count);
    for(uint32_t i=0; i < count; i++)
    {
        psRaceInfo* ri = cacheManager->GetRaceInfoByIndex(i);
        newmsg.AddRace(ri->name,ri->walkBaseSpeed,ri->runBaseSpeed, ri->size, ri->GetScale(), i == (count-1));
    }
    newmsg.SendMessage();
}


void NPCManager::SendNPCList(Client* client)
{
    int count = gemSupervisor->CountManagedNPCs(client->GetAccountID());

    // Note, building the internal message outside the msg ctor is very bad
    // but I am doing this to avoid sending database result sets to the msg ctor.
    psNPCListMessage newmsg(client->GetClientNum(),count * 2 * sizeof(uint32_t) + sizeof(uint32_t));

    newmsg.msg->Add((uint32_t)count);

    gemSupervisor->FillNPCList(newmsg.msg,client->GetAccountID());

    if(!newmsg.msg->overrun)
    {
        newmsg.SendMessage();
    }
    else
    {
        Bug2("Overran message buffer while sending NPC List to client %u.\n",client->GetClientNum());
    }
}

void NPCManager::SendAllNPCStats(Client* client)
{
    // Ask supervisor to iterate all controlled npcs to
    // queue a StatDR perception
    gemSupervisor->SendAllNPCStats(client->GetAccountID());
}


void NPCManager::HandleCommandList(MsgEntry* me,Client* client)
{
    psNPCCommandsMessage list(me);

    if(!list.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Could not parse psNPCCommandsMessage from client %u.\n",me->clientnum);
        return;
    }

    csTicks begin = csGetTicks();
    int count[24] = {0};
    int times[24] = {0};

    int cmd = list.msg->GetInt8();
    while(cmd != list.CMD_TERMINATOR && !list.msg->overrun)
    {
        csTicks cmdBegin = csGetTicks();
        count[cmd]++;
        switch(cmd)
        {
            case psNPCCommandsMessage::CMD_ASSESS:
            {
                // Extract the data
                EID actorEID = EID(list.msg->GetUInt32());
                EID targetEID = EID(list.msg->GetUInt32());
                csString physical = list.msg->GetStr();
                csString magical = list.msg->GetStr();
                csString overall = list.msg->GetStr();

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Error2("Received incomplete CMD_ASSESS from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC* actor = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(actorEID));
                if(!actor)
                {
                    Error2("Illegal %s from superclient!\n", ShowID(actorEID));
                    break;
                }

                gemActor* target = dynamic_cast<gemActor*>(gemSupervisor->FindObject(targetEID));
                if(!target)
                {
                    Error2("Illegal %s from superclient!\n", ShowID(actorEID));
                    break;
                }

                // Ask user manager about an assessment between the to entities
                int theirPhysicalLevel,theirMagicalLevel,physicalDiff,magicalDiff,overallDiff;
                psserver->GetUserManager()->CalculateComparativeDifference(actor->GetCharacterData(), target->GetCharacterData(),
                        theirPhysicalLevel, theirMagicalLevel,
                        physicalDiff, magicalDiff, overallDiff);

                // Character's overall comparison.
                static const char* const ComparePhrases[] =
                {
                    "extremely weaker",
                    "much weaker",
                    "weaker",
                    "equal",
                    "stronger",
                    "much stronger",
                    "extremely stronger"
                };

                QueueAssessPerception(actorEID,targetEID,physical,ComparePhrases[physicalDiff],
                                      magical,ComparePhrases[magicalDiff],overall,ComparePhrases[overallDiff]);

                break;
            }
            case psNPCCommandsMessage::CMD_DRDATA:
            {
                // extract the data
                uint32_t len = 0;
                void* data = list.msg->GetBufferPointerUnsafe(len);

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Error2("Received incomplete CMD_DRDATA from NPC client %u.\n",me->clientnum);
                    break;
                }

                psDRMessage drmsg(data,len,psserver->GetNetManager()->GetAccessPointers()); // alternate method of cracking

                // copy the DR data into an iDataBuffer
                csRef<iDataBuffer> databuf;
                databuf.AttachNew(new csDataBuffer(len));
                memcpy(databuf->GetData(), data, len);
                // find the entity and Set the DR data for it
                gemActor* actor = dynamic_cast<gemActor*>(gemSupervisor->FindObject(drmsg.entityid));

                if(!actor)
                {
                    Error2("Illegal %s from superclient!\n", ShowID(drmsg.entityid));

                }
                else if(!actor->IsAlive())
                {
                    Debug3(LOG_SUPERCLIENT, actor->GetPID().Unbox(), "Ignoring DR data for dead npc %s(%s).\n",
                           actor->GetName(), ShowID(drmsg.entityid));
                }
                else
                {
                    // Go ahead and update the server version
                    actor->SetDRData(drmsg);

                    // Now multicast to other clients
                    actor->UpdateProxList();
                    actor->MulticastDRUpdate();

                    if(drmsg.vel.y < -20 || drmsg.pos.y < -1000)                   //NPC has fallen down
                    {
                        // First print out what happend
                        CPrintf(CON_DEBUG, "Received bad DR data from NPC %s(%s %s), killing NPC.\n",
                                actor->GetName(), ShowID(drmsg.entityid), ShowID(actor->GetPID()));
                        csVector3 pos;
                        float yrot;
                        iSector* sector;
                        actor->GetPosition(pos,yrot,sector);
                        CPrintf(CON_DEBUG, "Pos: %s\n",toString(pos,sector).GetData());

                        // Than kill the NPC
                        actor->Kill(NULL);
                        break;
                    }
                }
                break;
            }
            case psNPCCommandsMessage::CMD_ATTACK:
            {
                EID attacker_id = EID(list.msg->GetUInt32());
                EID target_id = EID(list.msg->GetUInt32());
                uint32_t stanceCSID = list.msg->GetUInt32(); // Common String ID
                csString stance = cacheManager->FindCommonString(stanceCSID);

                Debug4(LOG_SUPERCLIENT, attacker_id.Unbox(), "-->Got %s attack cmd for entity %s to %s\n", stance.GetDataSafe(), ShowID(attacker_id), ShowID(target_id));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "Received incomplete CMD_ATTACK from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* attacker = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(attacker_id));
                if(attacker && attacker->IsAlive())
                {
                    gemActor* target = dynamic_cast<gemActor*>(gemSupervisor->FindObject(target_id));
                    if(!target)
                    {
                        attacker->SetTarget(target);
                        if(attacker->GetMode() == PSCHARACTER_MODE_COMBAT)
                        {
                            psserver->combatmanager->StopAttack(attacker);
                        }

                        if(target_id==0)
                        {
                            Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s has stopped attacking.\n", attacker->GetName());
                        }
                        else      // entity may have been removed since this msg was queued
                        {
                            Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "Couldn't find entity %s to attack.\n", ShowID(target_id));
                        }
                    }
                    else
                    {
                        attacker->SetTarget(target);
                        if(attacker->GetMode() == PSCHARACTER_MODE_COMBAT)
                        {
                            psserver->combatmanager->StopAttack(attacker);
                        }

                        if(!target->GetClient() || !target->GetActorPtr()->GetInvincibility())
                        {
                            if(psserver->combatmanager->AttackSomeone(attacker,target,
                                    CombatManager::GetStance(cacheManager, stance)))
                            {
                                Debug3(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s is now attacking %s.\n",
                                       attacker->GetName(), target->GetName());
                            }
                            else
                            {
                                Debug3(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s failed to start attacking %s.\n",
                                       attacker->GetName(), target->GetName());

                                QueueFailedToAttackPerception(attacker, target);
                            }
                        }
                        else
                        {
                            Debug3(LOG_SUPERCLIENT, attacker_id.Unbox(), "%s cannot attack GM [%s].\n", attacker->GetName(), target->GetName());
                        }
                    }

                }
                else
                {
                    Debug2(LOG_SUPERCLIENT, attacker_id.Unbox(), "No entity %s or not alive", ShowID(attacker_id));
                }
                break;
            }
            case psNPCCommandsMessage::CMD_CAST:
            {
                EID attackerEID = EID(list.msg->GetUInt32());
                EID targetEID = EID(list.msg->GetUInt32());
                csString spell = list.msg->GetStr();
                float kFactor = list.msg->GetFloat();

                Debug5(LOG_SUPERCLIENT, attackerEID.Unbox(), "-->Got cast %s with k %.1f from attacker entity %s to target %s\n", spell.GetDataSafe(), kFactor, ShowID(attackerEID), ShowID(targetEID));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, attackerEID.Unbox(), "Received incomplete CMD_CAST from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* attacker = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(attackerEID));
                if(!attacker || !attacker->IsAlive())
                {
                    Debug2(LOG_SUPERCLIENT, attackerEID.Unbox(), "No entity %s or not alive", ShowID(attackerEID));
                    break;
                }

                gemObject* target   = gemSupervisor->FindObject(targetEID);
                attacker->SetTargetObject(target);

                psserver->GetSpellManager()->Cast(attacker, spell, kFactor, NULL);

                break;
            }
            case psNPCCommandsMessage::CMD_DELETE_NPC:
            {
                PID npcPID = PID(list.msg->GetUInt32());

                Debug2(LOG_SUPERCLIENT, npcPID.Unbox(), "-->Got delete npc %s\n",ShowID(npcPID));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, npcPID.Unbox(), "Received incomplete CMD_DELETE_NPC from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* npc = gemSupervisor->FindNPCEntity(npcPID);
                if(!npc)
                {
                    Debug2(LOG_SUPERCLIENT, npcPID.Unbox(), "No entity %s", ShowID(npcPID));
                    break;
                }

                // Delete npc from world and DB.
                if (!EntityManager::GetSingleton().DeleteActor(npc))
                {
                    Error2("Failed to remove NPC %s!!!",ShowID(npcPID));
                }

                break;
            }
            case psNPCCommandsMessage::CMD_SCRIPT:
            {
                EID  npcEID = EID(list.msg->GetUInt32()); // NPC
                EID  targetEID = EID(list.msg->GetUInt32()); // Target
                csString scriptName = list.msg->GetStr();

                Debug4(LOG_SUPERCLIENT, npcEID.Unbox(), "-->Got script cmd for entity %s target %s to run %s\n",
                       ShowID(npcEID), ShowID(targetEID), scriptName.GetDataSafe());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_SCRIPT from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemObject* target = dynamic_cast<gemObject*>(gemSupervisor->FindObject(targetEID));

                gemNPC* npc = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(npcEID));
                if(npc)
                {
                    ProgressionScript* progScript = NULL;

                    progScript = psserver->GetProgressionManager()->FindScript(scriptName.GetDataSafe());
                    if(!progScript)
                    {
                        Error2("Faild to find script %s",scriptName.GetDataSafe());
                        break;
                    }

                    MathEnvironment env;
                    env.Define("Actor",   npc);
                    env.Define("Target",  target);
                    progScript->Run(&env);

                }
                else
                {
                    Error1("NPC Client try to run script with no existing NPC.");
                }

                break;
            }
            case psNPCCommandsMessage::CMD_SIT:
            {
                EID  npcId = EID(list.msg->GetUInt32()); // NPC
                EID  targetId = EID(list.msg->GetUInt32()); // Target
                bool sit = list.msg->GetBool();

                Debug3(LOG_SUPERCLIENT, npcId.Unbox(), "-->Got sit cmd for entity %s to %s\n", ShowID(npcId), sit?"sit":"stand");

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_SIT from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemObject* target = dynamic_cast<gemObject*>(gemSupervisor->FindObject(targetId));

                gemNPC* npc = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(npcId));
                if(npc)
                {
                    npc->SetTargetObject(target);
                    if(sit)
                    {
                        npc->Sit();
                    }
                    else
                    {
                        npc->Stand();
                    }
                }
                else
                {
                    Error1("NPC Client try to sit/stand with no existing npc");
                }

                break;
            }
            case psNPCCommandsMessage::CMD_SPAWN:
            {
                EID spawner_id = EID(list.msg->GetUInt32()); // Mother
                EID spawned_id = EID(list.msg->GetUInt32()); // Father
                csString tribeMemberType = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT, spawner_id.Unbox(), "-->Got spawn cmd for entity %s to %s\n", ShowID(spawner_id), ShowID(spawned_id));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_SPAWN from NPC client %u.\n",me->clientnum);
                    break;
                }
                gemNPC* spawner = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(spawner_id));

                if(spawner)
                {
                    gemNPC* spawned = entityManager->CloneNPC(spawner->GetCharacterData());
                    QueueSpawnedPerception(spawned, spawner, tribeMemberType);
                }
                else
                {
                    Error1("NPC Client try to clone non existing npc");
                }
                break;
            }
            case psNPCCommandsMessage::CMD_SPAWN_BUILDING:
            {
                EID           spawner_id = EID(list.msg->GetUInt32()); // Mother
                csVector3     where = list.msg->GetVector3();
                csString      sectorName = list.msg->GetStr();
                csString      buildingName = list.msg->GetStr();
                int           tribeID = list.msg->GetInt16();
                bool          pickupable = list.msg->GetBool();

                InstanceID    instance = DEFAULT_INSTANCE;
                psSectorInfo* sector = cacheManager->GetSectorInfoByName(sectorName);

                Debug4(LOG_SUPERCLIENT, spawner_id.Unbox(), "-->Got spawn building cmd at %s in %s for %s\n", toString(where).GetDataSafe(), sectorName.GetDataSafe(), buildingName.GetDataSafe());

                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_SPAWN_BUILDING from NPC client %u.\n", me->clientnum);
                    break;
                }

                // Now issue the spawn building command
                // Create the logic of the item
                psItemStats* itemstats = cacheManager->GetBasicItemStatsByName(buildingName);
                if(!itemstats)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Can not find itemstats for building %s.", buildingName.GetDataSafe());
                    break;
                }
                psItem* requiredItem = new psItem();
                requiredItem->SetBaseStats(itemstats);
                requiredItem->SetLocationInWorld(instance, sector, where[0], where[1], where[2], 0);
                requiredItem->SetIsPickupable(pickupable);
                requiredItem->SetLoaded();
                requiredItem->Save(false);

                // Create the gem entity
                (void) entityManager->CreateItem(requiredItem, false, tribeID);
                break;
            }
            case psNPCCommandsMessage::CMD_UNBUILD:
            {
                EID unbuilderEID = EID(list.msg->GetUInt32());
                EID buildingEID  = EID(list.msg->GetUInt32());

                Debug3(LOG_SUPERCLIENT, unbuilderEID.Unbox(), "-->Got unbuild cmd for unbilder %s to unbuild %s\n", ShowID(unbuilderEID), ShowID(buildingEID));

                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_UNBUILD from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemItem* building = dynamic_cast<gemItem*>(gemSupervisor->FindObject(buildingEID));

                if(building)
                {
                    psItem* item = building->GetItem();
                    if(item)
                    {
                        entityManager->RemoveActor(building); // building is deleted in Remove Actor
                        item->Destroy();                      // Remove item from DB.
                    }
                }

                break;
            }
            case psNPCCommandsMessage::CMD_BUSY:
            {
                EID entityEID = EID(list.msg->GetUInt32());
                bool busy = list.msg->GetBool();

                Debug2(LOG_SUPERCLIENT,me->clientnum,"-->Got busy cmd: for entity %s\n", ShowID(entityEID));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_BUSY from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC* entity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entityEID));
                if(!entity)
                {
                    Error1("Couldn't find entity for CMD_BUSY.");
                    break;
                }

                entity->SetBusy(busy);

                break;
            }
            case psNPCCommandsMessage::CMD_TALK:
            {
                EID speakerId = EID(list.msg->GetUInt32());
                EID targetId = EID(list.msg->GetUInt32());
                psNPCCommandsMessage::PerceptionTalkType talkType = (psNPCCommandsMessage::PerceptionTalkType)list.msg->GetUInt32();
                bool publicTalk = list.msg->GetBool();
                const char* text = list.msg->GetStr();

                Debug3(LOG_SUPERCLIENT,me->clientnum,"-->Got talk cmd: %s for entity %s\n", text, ShowID(speakerId));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_TALK from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemNPC* speaker = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(speakerId));
                if(!speaker)
                {
                    Error1("Couldn't find speaker for CMD_TALK.");
                    break;
                }

                Client* who = NULL; // Who to talk/action to
                gemActor* target = dynamic_cast<gemActor*>(gemSupervisor->FindObject(targetId));
                if(target)
                {
                    who = target->GetClient();
                }

                csTicks timeDelay=0; // Not used for broadcast

                switch(talkType)
                {
                    case psNPCCommandsMessage::TALK_SAY:
                        speaker->Say(text, who, publicTalk, timeDelay);
                        break;
                    case psNPCCommandsMessage::TALK_ME:
                        speaker->ActionCommand(false, false, text, who, publicTalk, timeDelay);
                        break;
                    case psNPCCommandsMessage::TALK_MY:
                        speaker->ActionCommand(true, false, text, who, publicTalk, timeDelay);
                        break;
                    case psNPCCommandsMessage::TALK_NARRATE:
                        speaker->ActionCommand(false, true, text, who, publicTalk, timeDelay);
                        break;
                    default:
                        Error2("Unkown talk type %u received from NPC Client",talkType);
                        break;
                }

                break;
            }
            case psNPCCommandsMessage::CMD_VISIBILITY:
            {
                EID entity_id = list.msg->GetUInt32();
                bool status = list.msg->GetBool();
                Debug3(LOG_SUPERCLIENT, me->clientnum, "-->Got visibility cmd: %s for entity %s\n", status ? "yes" : "no", ShowID(entity_id));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_VISIBILITY from NPC client %u.\n",me->clientnum);
                    break;
                }
                gemNPC* entity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                if(entity) entity->SetVisibility(status);
                break;
            }
            case psNPCCommandsMessage::CMD_PICKUP:
            {
                EID entity_id = list.msg->GetUInt32();
                EID item_id = list.msg->GetUInt32();
                int count = list.msg->GetInt16();
                Debug4(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got pickup cmd: Entity %s to pickup %d of %s\n", ShowID(entity_id), count, ShowID(item_id));

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_PICKUP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* gEntity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                psCharacter* chardata = NULL;
                if(gEntity) chardata = gEntity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT,0,"Couldn't find character data.\n");
                    break;
                }

                gemItem* gItem = dynamic_cast<gemItem*>(gemSupervisor->FindObject(item_id));
                psItem* item = NULL;
                if(gItem) item = gItem->GetItem();
                if(!item)
                {
                    Debug1(LOG_SUPERCLIENT,0,"Couldn't find item data.\n");
                    break;
                }

                // If the entity is in range of the item AND the item may be picked up, and check for dead user
                // TODO: add support to handle some items to be pickupable for npcclient even if not for players.
                //       can use the pickupable weak for this.
                if(gEntity->IsAlive() && (gItem->RangeTo(gEntity) < RANGE_TO_SELECT) && gItem->IsPickupable())
                {

                    // TODO: Include support for splitting of a stack
                    //       into count items.

                    // Cache values from item, because item might be deleted in Add
                    csString qname = item->GetQuantityName();

                    if(chardata && chardata->Inventory().Add(item))
                    {
                        // Ownership for the psItem is now the character's
                        // inventory so don't delete it when we destroy
                        // the gemItem.
                        gItem->ClearItemData();
                        entityManager->RemoveActor(gItem);  // Destroy this
                    }
                    else
                    {
                        // TODO: Handle of pickup of partial stacks.
                    }
                }

                break;
            }
            case psNPCCommandsMessage::CMD_EMOTE:
            {
                EID  npcId = EID(list.msg->GetUInt32()); // NPC
                EID  targetId = EID(list.msg->GetUInt32()); // Target
                csString cmd = list.msg->GetStr();

                Debug3(LOG_SUPERCLIENT, npcId.Unbox(), "-->Got emote cmd for entity %s with %s\n", ShowID(npcId), cmd.GetData());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT,me->clientnum,"Received incomplete CMD_EMOTE from NPC client %u.\n",me->clientnum);
                    break;
                }

                gemObject* target = dynamic_cast<gemObject*>(gemSupervisor->FindObject(targetId));

                gemNPC* npc = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(npcId));
                if(npc)
                {
                    npc->SetTargetObject(target);
                    psserver->usermanager->CheckForEmote(cmd, true, npc);
                }
                else
                {
                    Error1("NPC Client try to emote with no existing npc");
                }

                break;
            }
            case psNPCCommandsMessage::CMD_EQUIP:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString item = list.msg->GetStr();
                csString slot = list.msg->GetStr();
                int count = list.msg->GetInt16();
                Debug6(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got equip cmd from %u: Entity %s to equip %d %s in %s\n",
                       me->clientnum, ShowID(entity_id), count, item.GetData(), slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_EQUIP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* gEntity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                psCharacter* chardata = NULL;
                if(gEntity) chardata = gEntity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)cacheManager->slotNameHash.GetID(slot);
                if(slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find slot %s.\n", slot.GetData());
                    break;
                }

                // Create the item
                psItemStats* baseStats = cacheManager->GetBasicItemStatsByName(item);
                if(!baseStats)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find base for item %s.\n", item.GetData());
                    break;
                }

                // See if the char already has this item
                size_t index = chardata->Inventory().FindItemStatIndex(baseStats);
                if(index != SIZET_NOT_FOUND)
                {
                    psItem* existingItem = chardata->Inventory().GetInventoryIndexItem(index);
                    if(!chardata->Inventory().EquipItem(existingItem, (INVENTORY_SLOT_NUMBER) slotID))
                        Error3("Could not equip %s in slot %u for npc, but it is in inventory.\n",existingItem->GetName(),slotID);
                }
                else
                {
                    // Make a permanent new item
                    psItem* newItem = baseStats->InstantiateBasicItem(true);
                    if(!newItem)
                    {
                        Debug2(LOG_SUPERCLIENT,0,"Couldn't create item %s.\n",item.GetData());
                        break;
                    }
                    newItem->SetStackCount(count);

                    if(chardata->Inventory().Add(newItem,false,false))   // Item must be in inv before equipping it
                    {
                        if(!chardata->Inventory().EquipItem(newItem, (INVENTORY_SLOT_NUMBER) slotID))
                            Error3("Could not equip %s in slot %u for npc, but it is in inventory.\n",newItem->GetName(),slotID);
                    }
                    else
                    {
                        Error2("Adding new item %s to inventory failed.", item.GetData());
                        delete newItem;
                    }
                }
                break;
            }
            case psNPCCommandsMessage::CMD_DEQUIP:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString slot = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got dequip cmd: Entity %s to dequip from %s\n",
                       ShowID(entity_id), slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_DEQUIP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* gEntity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                psCharacter* chardata = NULL;
                if(gEntity) chardata = gEntity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)cacheManager->slotNameHash.GetID(slot);
                if(slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find slot %s.\n", slot.GetData());
                    break;
                }

                Debug3(LOG_SUPERCLIENT, entity_id.Unbox(), "Removing item in slot %d from %s.\n",slotID,chardata->GetCharName());

                psItem* oldItem = chardata->Inventory().RemoveItem(NULL,(INVENTORY_SLOT_NUMBER)slotID);
                if(oldItem)
                    delete oldItem;

                break;
            }
            case psNPCCommandsMessage::CMD_WORK:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString type = list.msg->GetStr();
                csString resource = list.msg->GetStr();
                Debug4(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got work cmd: Entity %s to %s for %s\n",
                       ShowID(entity_id), type.GetData(), resource.GetData());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_WORK from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* gEntity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                psCharacter* chardata = NULL;
                if(gEntity) chardata = gEntity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                psserver->GetWorkManager()->HandleProduction(gEntity,type,resource);
                break;
            }

            case psNPCCommandsMessage::CMD_CONTROL:
            {
                // Extract the data
                EID controllingEntity = EID(list.msg->GetUInt32());
                // extract the DR data
                uint32_t len = 0;
                void* data = list.msg->GetBufferPointerUnsafe(len);

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, controllingEntity.Unbox(), "Received incomplete CMD_CONTROL from NPC client %u.\n", me->clientnum);
                    break;
                }

                psDRMessage drmsg(data,len,psserver->GetNetManager()->GetAccessPointers()); // alternate method of cracking

                Debug5(LOG_SUPERCLIENT, controllingEntity.Unbox(), "-->Got control Controlling EID: %u Controlled EID: %u Pos: %s yRot: %.1f\n",
                       controllingEntity.Unbox(), drmsg.entityid.Unbox(),toString(drmsg.pos,drmsg.sector).GetDataSafe(),drmsg.yrot);

                gemActor* controlled = dynamic_cast<gemActor*>(gemSupervisor->FindObject(drmsg.entityid));
                if(!controlled)
                {
                    Debug1(LOG_SUPERCLIENT, controllingEntity.Unbox(), "Couldn't find controlled entity.\n");
                    break;
                }

                // TODO: Allow for breake free

                controlled->SetDRData(drmsg);
                if(controlled->GetClient())
                    controlled->GetClient()->SetCheatMask(MOVE_CHEAT, true); // Tell paladin one of these is OK.

                controlled->UpdateProxList();
                controlled->MulticastDRUpdate();
                controlled->ForcePositionUpdate();
                controlled->BroadcastTargetStatDR(entityManager->GetClients());

                break;
            }

            case psNPCCommandsMessage::CMD_LOOT:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                EID target_id = EID(list.msg->GetUInt32());
                csString type = list.msg->GetStr();
                Debug4(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got loot cmd: Entity %s to loot from EID: %s for %s\n",
                       ShowID(entity_id), ShowID(target_id), type.GetData());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_LOOT from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemActor* actor  = dynamic_cast<gemActor*>(gemSupervisor->FindObject(entity_id));
                gemActor* target = dynamic_cast<gemActor*>(gemSupervisor->FindObject(target_id));

                if(actor)
                {
                    actor->SetTargetObject(target);

                    if(psserver->GetUserManager()->CheckTargetLootable(actor, NULL))
                    {
                        psserver->GetUserManager()->LootMoney(actor, NULL);
                        psserver->GetUserManager()->LootItems(actor, NULL, type);
                    }
                }
                else
                    Error1("NPC Client try to loot with no existing npc");

                break;
            }

            case psNPCCommandsMessage::CMD_DROP:
            {
                EID entity_id = list.msg->GetUInt32();
                csString slot = list.msg->GetStr();
                Debug3(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got drop cmd: Entity %s to drop from %s\n",
                       ShowID(entity_id), slot.GetData());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_DROP from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* gEntity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                psCharacter* chardata = NULL;
                if(gEntity) chardata = gEntity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                INVENTORY_SLOT_NUMBER slotID = (INVENTORY_SLOT_NUMBER)cacheManager->slotNameHash.GetID(slot);
                if(slotID == PSCHARACTER_SLOT_NONE)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find slot %s.\n", slot.GetData());
                    break;
                }

                psItem* oldItem = chardata->Inventory().GetInventoryItem((INVENTORY_SLOT_NUMBER)slotID);
                if(oldItem)
                {
                    oldItem = chardata->Inventory().RemoveItemID(oldItem->GetUID());
                    chardata->DropItem(oldItem);
                }

                break;
            }

            case psNPCCommandsMessage::CMD_TRANSFER:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                csString item = list.msg->GetStr();
                int count = list.msg->GetInt8();
                csString target = list.msg->GetStr();

                Debug4(LOG_SUPERCLIENT, entity_id.Unbox(), "-->Got transfer cmd: Entity %s to transfer '%s' to %s\n",
                       ShowID(entity_id), item.GetDataSafe(), target.GetDataSafe());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, entity_id.Unbox(), "Received incomplete CMD_TRANSFER from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemNPC* gEntity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));
                psCharacter* chardata = NULL;
                if(gEntity) chardata = gEntity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                if(item.IsEmpty())
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Transfere with empty item not possible.\n");
                    break;
                }


                psItemStats* itemstats = cacheManager->GetBasicItemStatsByName(item);
                if(!itemstats)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(),  "Invalid item name\n");
                    break;
                }

                size_t slot = chardata->Inventory().FindItemStatIndex(itemstats);
                if(slot == SIZET_NOT_FOUND)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(),  "Item not found\n");
                    break;
                }


                psItem* transferItem = chardata->Inventory().RemoveItemIndex(slot,count);
                if(!transferItem)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(),  "Item could not be removed\n");
                    break;
                }

                count = transferItem->GetStackCount();

                // TODO: Check the target, for now assume tribe. Tribe dosn't held items in server so delete them and notify npcclient
                QueueTransferPerception(gEntity,transferItem,target);
                delete transferItem;

                break;
            }
            case psNPCCommandsMessage::CMD_RESURRECT:
            {
                csVector3 where;
                PID playerID = PID(list.msg->GetUInt32());
                float rot = list.msg->GetFloat();
                where = list.msg->GetVector3();
                iSector* sector = list.msg->GetSector(psserver->GetCacheManager()->GetMsgStrings(),0,
                                                      entityManager->GetEngine());

                Debug5(LOG_SUPERCLIENT, playerID.Unbox(),
                       "-->Got resurrect cmd: %s Rot: %.2f Where: %s Sector: %s\n",
                       ShowID(playerID), rot, where.Description().GetDataSafe(),
                       sector->QueryObject()->GetName());

                // Make sure we haven't run past the end of the buffer
                if(list.msg->overrun)
                {
                    Debug2(LOG_SUPERCLIENT, playerID.Unbox(), "Received incomplete CMD_RESURRECT from NPC client %u.\n", me->clientnum);
                    break;
                }

                gemActor* actor = gemSupervisor->FindPlayerEntity(playerID);
                if(actor && actor->IsAlive())
                {
                    Debug2(LOG_SUPERCLIENT, playerID.Unbox(), "Ignoring received CMD_RESURRECT from NPC %s.\n", ShowID(playerID));
                    break;
                }

                psCharacter* chardata=psServer::CharacterLoader.LoadCharacterData(playerID, false);
                if(chardata==NULL)
                {
                    Error2("Character %s to be respawned does not have character data to be loaded!\n", ShowID(playerID));
                    break;
                }

                psserver->GetSpawnManager()->Respawn(chardata, INSTANCE_ALL, where, rot, sector->QueryObject()->GetName());

                break;
            }

            case psNPCCommandsMessage::CMD_SEQUENCE:
            {
                csString name = list.msg->GetStr();
                int cmd = list.msg->GetUInt8();
                int count = list.msg->GetInt32();

                psSequenceMessage msg(0,name,cmd,count);
                psserver->GetEventManager()->Broadcast(msg.msg,NetBase::BC_EVERYONE);

                break;
            }

            case psNPCCommandsMessage::CMD_TEMPORARILY_IMPERVIOUS:
            {
                EID entity_id = EID(list.msg->GetUInt32());
                int impervious = list.msg->GetBool();

                gemNPC* entity = dynamic_cast<gemNPC*>(gemSupervisor->FindObject(entity_id));

                psCharacter* chardata = NULL;
                if(entity) chardata = entity->GetCharacterData();
                if(!chardata)
                {
                    Debug1(LOG_SUPERCLIENT, entity_id.Unbox(), "Couldn't find character data.\n");
                    break;
                }

                if(impervious)
                {
                    chardata->SetImperviousToAttack(chardata->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);
                }
                else
                {
                    chardata->SetImperviousToAttack(chardata->GetImperviousToAttack() & ~TEMPORARILY_IMPERVIOUS);
                }

                break;
            }
            case psNPCCommandsMessage::CMD_INFO_REPLY:
            {
                // Extract the data
                uint32_t clientnum = list.msg->GetUInt32();
                csString reply = list.msg->GetStr();

                // Check if client is still online
                Client* client = psserver->GetConnections()->Find(clientnum);
                if(!client)
                {
                    Debug3(LOG_SUPERCLIENT, clientnum, "Couldn't find client %d for info reply: %s.",
                           clientnum,reply.GetDataSafe());
                    break;
                }
                if(reply.Length() < MAXSYSTEMMSGSIZE)
                {
                    psserver->SendSystemInfo(client->GetClientNum(),reply);
                }
                else
                {
                    csStringArray list(reply,"\n");
                    for(size_t i = 0; i < list.GetSize(); i++)
                    {
                        psserver->SendSystemInfo(client->GetClientNum(),list[i]);
                    }
                }

                break;
            }


        }
        times[cmd] += csGetTicks() - cmdBegin;
        cmd = list.msg->GetInt8();
    }
    begin = csGetTicks() - begin;
    if(begin > 500)
    {
        int total = 0;
        for(int i = 0; i < 24; i++)
            total += count[i];

        csString status;
        status.Format("NPCManager::HandleCommandList() took %d time. %d commands. Counts: ", begin, total);

        for(int i = 0; i < 24; i++)
            if(times[i] > 100)
                status.AppendFmt("%d: %d# x%d", i, count[i], times[i]);

        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }
    if(list.msg->overrun)
    {
        Debug2(LOG_NET,me->clientnum,"Received unterminated or unparsable psNPCCommandsMessage from client %u.\n",me->clientnum);
    }
}

void NPCManager::AddEntity(gemObject* obj)
{
    obj->Send(0,false,true);
}


void NPCManager::RemoveEntity(MsgEntry* me)
{
    psserver->GetEventManager()->Multicast(me, superclients, 0, PROX_LIST_ANY_RANGE);
}

void NPCManager::UpdateWorldPositions()
{
    if(superclients.GetSize())
    {
        gemSupervisor->UpdateAllDR();

        csArray<psAllEntityPosMessage> msgs;
        gemSupervisor->GetAllEntityPos(msgs);

        for(size_t i = 0; i < msgs.GetSize(); i++)
        {
            msgs.Get(i).Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
        }
    }
}

bool NPCManager::CanPetHearYou(int clientnum, Client* owner, gemNPC* pet, const char* type)
{
    MathEnvironment env;
    env.Define("Skill", owner->GetCharacterData()->GetSkillRank(GetPetSkill()).Current());

    petRangeScript->Evaluate(&env);
    MathVar* varMaxRange = env.Lookup("MaxRange");
    float max_range = varMaxRange->GetValue();

    if(DoLogDebug(LOG_NPC))
    {
        CPrintf(CON_DEBUG, "Variables for CalculateMaxPetRange:\n");
        env.DumpAllVars();
    }

    if(pet->GetInstance() != owner->GetActor()->GetInstance() ||
            owner->GetActor()->RangeTo(pet, false) >= max_range)
    {
        psserver->SendSystemInfo(clientnum, "Your %s is too far away to hear you", type);
        return false;
    }

    return true;
}

bool NPCManager::WillPetReact(int clientnum, Client* owner, gemNPC* pet, const char* type, int level)
{
    MathEnvironment env;
    env.Define("Skill", owner->GetCharacterData()->GetSkillRank(GetPetSkill()).Current());
    env.Define("Level", level);
    petReactScript->Evaluate(&env);
    MathVar* varReact = env.Lookup("React");
    float react = varReact->GetValue();

    if(DoLogDebug(LOG_NPC))
    {
        CPrintf(CON_DEBUG, "Variables for CalculatePetReact:\n");
        env.DumpAllVars();
    }

    if(react > 0.0)
    {
        return true;
    }

    psserver->SendSystemInfo(clientnum, "Your %s does not react to your command.", type);
    return false;
}

void NPCManager::HandlePetCommand(MsgEntry* me,Client* client)
{
    psPETCommandMessage msg(me);
    gemNPC* pet = NULL;
    psCharacter* chardata = NULL;
    csString firstName, lastName;
    csString prevFirstName, prevLastName;
    csString fullName, prevFullName;
    const char* typeStr = "familiar";
    PID familiarID;

    Client* owner = clients->FindAny(me->clientnum);
    if(!owner)
    {
        Error2("invalid client object from psPETCommandMessage from client %u.\n",me->clientnum);
        return;
    }

    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Could not parse psPETCommandMessage from client %u.\n",me->clientnum);
        return;
    }

    WordArray optionWords(msg.options);

    // All pet commands operate on the currently summoned pet except
    // for the SUMMON command.
    if(msg.command == psPETCommandMessage::CMD_SUMMON)
    {
        if(owner->GetFamiliar() ||
                (owner->GetActor()->GetMount() &&
                 owner->GetActor()->GetMount()->GetOwnerID() == owner->GetPID()))
        {
            psserver->SendSystemInfo(me->clientnum,
                                     "Your familiar has already been summoned.");
            return;
        }

        if(!owner->GetCharacterData()->CanSummonFamiliar())
        {
            psserver->SendSystemInfo(me->clientnum,
                                     "You need to equip the item your familiar is bound to.");
            return;
        }

        char* end = NULL;
        size_t targetID = strtoul(optionWords[0].IsEmpty() ?
                                  msg.target.GetDataSafe() : optionWords[0].GetDataSafe(), &end, 0);

        // Operator did give a name, let's see if we find the named pet.
        if(optionWords[0].Length() != 0 && (end == NULL || *end))
        {
            chardata = owner->GetCharacterData();
            size_t numPets = chardata->GetNumFamiliars();
            for(size_t i = 0; i < numPets; i++)
            {
                PID pid = chardata->GetFamiliarID(i);

                Result result(db->Select("SELECT name FROM characters WHERE id = %u", pid.Unbox()));
                if(result.IsValid() && result.Count())
                {
                    if(optionWords[0].CompareNoCase(result[0]["name"]))
                    {
                        familiarID = pid;
                        break;
                    }
                }
            }
            if(!familiarID.IsValid())
            {
                psserver->SendSystemInfo(me->clientnum, "You do not have a pet named '%s'.", optionWords[0].GetData());
                return;
            }
        }
        else
        {
            familiarID = owner->GetCharacterData()->GetFamiliarID(targetID);
            if(!familiarID.IsValid())
            {
                psserver->SendSystemInfo(me->clientnum, "You have no familiar to command.");
                return;
            }
        }
    }
    else if(!msg.target.IsEmpty())
    {
        // Client has specified a specific NPC to command.
        size_t inx = strtoul(msg.target.GetData(), NULL, 0);
        pet = dynamic_cast <gemNPC*>(owner->GetPet(inx));
        if(!pet)
        {
            // There is no separate gemNPC while mounted (actor is the player).
            // Instead of saying there is no familiar to command, return a more
            // useful message.
            psserver->SendSystemInfo(me->clientnum,
                                     "You have no pet numbered '%zu' to command.", inx);
            return;
        }
        switch(msg.command)
        {
            // Level 0 commands, will allways react
            case psPETCommandMessage::CMD_TARGET :
            {
                if(CanPetHearYou(me->clientnum, owner, pet, typeStr))
                {
                    if(optionWords.GetCount() == 0)
                    {
                        psserver->SendSystemInfo(me->clientnum, "You must specify a name for your pet to target.");
                        return;
                    }

                    firstName = optionWords.Get(0);
                    if(optionWords.GetCount() > 1)
                    {
                        lastName = optionWords.GetTail(1);
                    }
                    gemObject* target = psserver->GetAdminManager()->FindObjectByString(firstName,owner->GetActor());

                    firstName = NormalizeCharacterName(firstName);

                    if(firstName == "Me")
                    {
                        firstName = owner->GetName();
                    }
                    lastName = NormalizeCharacterName(lastName);

                    if(target)
                    {
                        pet->SetTarget(target);
                        psserver->SendSystemInfo(me->clientnum, "%s has successfully targeted %s." , pet->GetName(), target->GetName());
                    }
                    else
                    {
                        psserver->SendSystemInfo(me->clientnum, "Cannot find '%s' to target.", firstName.GetData());
                    }
                }
            }
            break;
            // Level 1 commands
            case psPETCommandMessage::CMD_FOLLOW :
            case psPETCommandMessage::CMD_STAY :
            {
                if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                        WillPetReact(me->clientnum, owner, pet, typeStr, 1))
                {
                    // If no target, then target owner
                    if(!pet->GetTarget())
                    {
                        pet->SetTarget(owner->GetActor());
                    }
                    QueueOwnerCmdPerception(owner->GetActor(), pet, (psPETCommandMessage::PetCommand_t)msg.command);
                }
            }
            break;
            // Level 2 commands
            case psPETCommandMessage::CMD_GUARD :
            case psPETCommandMessage::CMD_RUN :
            case psPETCommandMessage::CMD_WALK :
            {
                if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                        WillPetReact(me->clientnum, owner, pet, typeStr, 2))
                {
                    QueueOwnerCmdPerception(owner->GetActor(), pet, (psPETCommandMessage::PetCommand_t)msg.command);
                }
            }
            break;
            // Level 3 commands
            case psPETCommandMessage::CMD_ASSIST :
            {
                if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                        WillPetReact(me->clientnum, owner, pet, typeStr, 3))
                {
                    QueueOwnerCmdPerception(owner->GetActor(), pet, (psPETCommandMessage::PetCommand_t)msg.command);
                }
            }
            break;
            // Level 4 commands
            case psPETCommandMessage::CMD_ATTACK :
            {
                if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                        WillPetReact(me->clientnum, owner, pet, typeStr, 4))
                {
                    gemActor* lastAttacker = NULL;
                    gemObject* trg = pet->GetTarget();
                    if(trg != NULL)
                    {
                        gemActor* targetActor = trg->GetActorPtr();
                        /* We check if the owner can attack the other entity in order to not allow players
                         * to override permissions and at the same time allowing pet<->player, pet<->pet
                         * when in pvp. We allow gm to do anything they want (can attack everything including
                         * their own pet just like how it happens with players
                         */
                        csString msg; // Not used
                        if(targetActor == NULL ||
                                !owner->GetActor()->IsAllowedToAttack(trg, msg) ||
                                (trg == pet && !owner->IsGM()))
                        {
                            psserver->SendSystemInfo(me->clientnum,"Your pet refuses.");
                        }
                        else if(targetActor && !targetActor->CanBeAttackedBy(pet,lastAttacker))
                        {
                            csString tmp;
                            if(lastAttacker)
                            {
                                tmp.Format("You must be grouped with %s for your pet to attack %s.",
                                           lastAttacker->GetName(), trg->GetName());
                            }
                            else
                            {
                                tmp.Format("Your pet is not allowed to attack right now.");
                            }
                            psserver->SendSystemInfo(me->clientnum,tmp.GetDataSafe());
                        }
                        else
                        {
                            Stance stance = CombatManager::GetStance(cacheManager, "Aggressive");
                            if(optionWords.GetCount() != 0)
                            {
                                stance.stance_id = optionWords.GetInt(0);
                            }
                            QueueOwnerCmdPerception(owner->GetActor(), pet, psPETCommandMessage::CMD_ATTACK);
                        }
                    }
                    else
                    {
                        psserver->SendSystemInfo(me->clientnum, "Your pet needs a target to attack.");
                    }
                }
            }
            break;
            case psPETCommandMessage::CMD_STOPATTACK :
            {
                if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                        WillPetReact(me->clientnum, owner, pet, typeStr, 4))
                {
                    QueueOwnerCmdPerception(owner->GetActor(), pet, (psPETCommandMessage::PetCommand_t)msg.command);
                }
            }
            break;
        }
        return;
    }
    else
    {
        pet = dynamic_cast <gemNPC*>(owner->GetFamiliar());
        if(!pet)
        {
            // There is no separate gemNPC while mounted (actor is the player).
            // Instead of saying there is no familiar to command, return a more
            // useful message.
            psserver->SendSystemInfo(me->clientnum,
                                     owner->GetActor()->GetMount() ?
                                     "You can't command your familiar while mounted." :
                                     "You have no summoned familiar to command.");
            return;
        }
    }

    switch(msg.command)
    {
        // Level 0 commands, will allways react
        case psPETCommandMessage::CMD_DISMISS :
        {
            if(pet->IsValid())
            {
                DismissPet(pet, owner);
            }
            else
            {
                psserver->SendSystemInfo(me->clientnum, "Your pet has already returned to the netherworld.");
                return;
            }
        }
        break;
        case psPETCommandMessage::CMD_NAME :
        {
            if(!CanPetHearYou(me->clientnum, owner, pet, typeStr))
            {
                return;
            }

            if(optionWords.GetCount() == 0)
            {
                psserver->SendSystemInfo(me->clientnum, "You must specify a new name for your pet.");
                return;
            }

            firstName = optionWords.Get(0);
            if(firstName.Length() > MAX_PLAYER_NAME_LENGTH)
            {
                psserver->SendSystemError(me->clientnum, "First name is too long!");
                return;
            }
            firstName = NormalizeCharacterName(firstName);
            if(!CharCreationManager::FilterName(firstName))
            {
                psserver->SendSystemError(me->clientnum, "The name %s is invalid!", firstName.GetData());
                return;
            }
            if(optionWords.GetCount() > 1)
            {
                lastName = optionWords.GetTail(1);
                if(lastName.Length() > MAX_PLAYER_NAME_LENGTH)
                {
                    psserver->SendSystemError(me->clientnum, "Last name is too long!");
                    return;
                }

                lastName = NormalizeCharacterName(lastName);
                if(!CharCreationManager::FilterName(lastName))
                {
                    psserver->SendSystemError(me->clientnum, "The last name %s is invalid!", lastName.GetData());
                    return;
                }
            }
            else
            {
                //we need this to be initialized or we won't be able to set it correctly
                lastName = "";
                lastName.Clear();
            }

            if(psserver->GetCharManager()->IsBanned(firstName))
            {
                psserver->SendSystemError(me->clientnum, "The name %s is invalid!", firstName.GetData());
                return;
            }

            if(psserver->GetCharManager()->IsBanned(lastName))
            {
                psserver->SendSystemError(me->clientnum, "The last name %s is invalid!", lastName.GetData());
                return;
            }

            chardata = pet->GetCharacterData();
            prevFirstName = chardata->GetCharName();
            prevLastName = chardata->GetCharLastName();
            if(firstName == prevFirstName && lastName == prevLastName)
            {
                // no changes needed
                psserver->SendSystemError(me->clientnum, "Your %s is already known with that name!", typeStr);
                return;
            }

            if(firstName != prevFirstName && !CharCreationManager::IsUnique(firstName))
            {
                psserver->SendSystemError(me->clientnum, "The name %s is not unique!",
                                          firstName.GetDataSafe());
                return;
            }

            prevFullName = chardata->GetCharFullName();
            chardata->SetFullName(firstName, lastName);
            fullName = chardata->GetCharFullName();
            pet->SetName(fullName);

            psServer::CharacterLoader.SaveCharacterData(chardata, pet, true);

            if(owner->GetFamiliar())
            {
                psUpdateObjectNameMessage newNameMsg(0,pet->GetEID(),pet->GetCharacterData()->GetCharFullName());
                psserver->GetEventManager()->Broadcast(newNameMsg.msg,NetBase::BC_EVERYONE);
            }
            else
            {
                entityManager->RemoveActor(pet);
            }

            psserver->SendSystemInfo(me->clientnum,
                                     "Your pet %s is now known as %s",
                                     prevFullName.GetData(),
                                     fullName.GetData());
        }
        break;
        case psPETCommandMessage::CMD_SUMMON :
        {
            // Attach to Familiar
            PetOwnerSession* session = NULL;

            session = OwnerPetList.Get(familiarID, NULL);

            psCharacter* petdata = psserver->CharacterLoader.LoadCharacterData(familiarID, false);
            // Check for an existing session
            if(!session)
            {
                // Create session if one doesn't exist
                session = CreatePetOwnerSession(owner->GetActor(), petdata);
            }

            if(!session)
            {
                Error2("Error while creating pet session for Character '%s'\n", owner->GetCharacterData()->GetCharName());
                return;
            }

            if(session->owner != owner->GetActor())
            {
                session->Reconnect(owner->GetActor());
            }

            // Check time in game for pet
            if(session->IsInDepletedLockout())
            {
                psserver->SendSystemInfo(me->clientnum,"The power of the ring of familiar is currently depleted, it will take more time to summon a pet again.");
                return;
            }

            if(session->IsInKilledLockout())
            {
                if(familiarID.IsValid())
                {
                    psserver->SendSystemInfo(me->clientnum, "Your pet is hiding in the netherworld.");
                }
                else
                {
                    psserver->SendSystemInfo(me->clientnum, "Your familiar is avoiding you.");
                }
                return;
            }


            // Check if there has been suffisient time since last summon.
            // Will continue on the last elapsed time since dismiss lockout
            // wasn't completed. Give a warning to the user.
            if(session->IsInDismissLockoutTime())
            {
                psserver->SendSystemInfo(me->clientnum,"Your pet was just dismissed, but manage with an effort to return.");
            }

            {
                session->Summon();

                iSector* targetSector;
                csVector3 targetPoint;
                float yRot = 0.0;
                InstanceID instance;
                owner->GetActor()->GetPosition(targetPoint,yRot,targetSector);
                instance = owner->GetActor()->GetInstance();
                psSectorInfo* sectorInfo = cacheManager->GetSectorInfoByName(targetSector->QueryObject()->GetName());
                petdata->SetLocationInWorld(instance,sectorInfo,targetPoint.x,targetPoint.y,targetPoint.z,yRot);

                entityManager->CreateNPC(petdata);
                pet = gemSupervisor->FindNPCEntity(familiarID);
                if(pet == NULL)
                {
                    Error2("Error while creating Familiar NPC for Character '%s'\n", owner->GetCharacterData()->GetCharName());
                    psserver->SendSystemError(owner->GetClientNum(), "Could not find Familiar GEM and set its location.");
                    return; // If all we didn't do was load the familiar
                }
                if(!pet->IsValid())
                {
                    Error2("No valid Familiar NPC for Character '%s'\n", owner->GetCharacterData()->GetCharName());
                    psserver->SendSystemError(owner->GetClientNum(), "Could not find valid Familiar");
                    return; // If all we didn't do was load the familiar
                }

                owner->SetFamiliar(pet);
                // Send OwnerActionLogon Perception
                pet->SetOwner(owner->GetActor());
                if(!session->IsInTrainingLockout())
                {
                    owner->GetCharacterData()->Skills().AddSkillPractice(GetPetSkill(), 1);
                    session->ReceivedTraining();
                }
                // Have the pet auto follow when summoned
                // If no target, then target owner
                if(!pet->GetTarget())
                {
                    pet->SetTarget(owner->GetActor());
                }
                QueueOwnerCmdPerception(owner->GetActor(), pet, psPETCommandMessage::CMD_FOLLOW);
            }
        }
        break;
        case psPETCommandMessage::CMD_TARGET :
        {
            if(CanPetHearYou(me->clientnum, owner, pet, typeStr))
            {
                if(optionWords.GetCount() == 0)
                {
                    psserver->SendSystemInfo(me->clientnum, "You must specify a name for your pet to target.");
                    return;
                }

                firstName = optionWords.Get(0);
                if(optionWords.GetCount() > 1)
                {
                    lastName = optionWords.GetTail(1);
                }
                gemObject* target = psserver->GetAdminManager()->FindObjectByString(firstName,owner->GetActor());

                firstName = NormalizeCharacterName(firstName);

                if(firstName == "Me")
                {
                    firstName = owner->GetName();
                }
                lastName = NormalizeCharacterName(lastName);

                if(target)
                {
                    pet->SetTarget(target);
                    psserver->SendSystemInfo(me->clientnum, "%s has successfully targeted %s." , pet->GetName(), target->GetName());
                }
                else
                {
                    psserver->SendSystemInfo(me->clientnum, "Cannot find '%s' to target.", firstName.GetData());
                }
            }
        }
        break;
        // Level 1 commands
        case psPETCommandMessage::CMD_FOLLOW :
        case psPETCommandMessage::CMD_STAY :
        {
            PetOwnerSession* session = OwnerPetList.Get(pet->GetCharacterData()->GetPID(), NULL);
            if(!session)
            {
                CPrintf(CON_NOTIFY, "Cannot locate PetSession for owner %s.\n", pet->GetName(), owner->GetName());
                return;
            }

            if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                    WillPetReact(me->clientnum, owner, pet, typeStr, 1))
            {
                // If no target, then target owner
                if(!pet->GetTarget())
                {
                    pet->SetTarget(owner->GetActor());
                }
                QueueOwnerCmdPerception(owner->GetActor(), pet, (psPETCommandMessage::PetCommand_t)msg.command);
                if(!session->IsInTrainingLockout())
                {
                    owner->GetCharacterData()->Skills().AddSkillPractice(GetPetSkill(), 1);
                    session->ReceivedTraining();
                }
            }
        }
        break;
        // Level 2 commands
        case psPETCommandMessage::CMD_GUARD :
        case psPETCommandMessage::CMD_RUN :
        case psPETCommandMessage::CMD_WALK :
        {
            if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                    WillPetReact(me->clientnum, owner, pet, typeStr, 2))
            {
                QueueOwnerCmdPerception(owner->GetActor(), pet, (psPETCommandMessage::PetCommand_t)msg.command);
            }
        }
        break;
        // Level 3 commands
        case psPETCommandMessage::CMD_ASSIST :
        {
            if(CanPetHearYou(me->clientnum, owner, pet, typeStr) &&
                    WillPetReact(me->clientnum, owner, pet, typeStr, 3))
            {
                QueueOwnerCmdPerception(owner->GetActor(), pet, psPETCommandMessage::CMD_ASSIST);
            }
        }
        break;
        // Level 4 commands
        case psPETCommandMessage::CMD_ATTACK :
        {
            PetOwnerSession* session = OwnerPetList.Get(pet->GetCharacterData()->GetPID(), NULL);
            if(!session)
            {
                CPrintf(CON_NOTIFY, "Cannot locate PetSession for owner %s.\n", pet->GetName(), owner->GetName());
                return;
            }

            if(CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 4))
            {
                gemActor* lastAttacker = NULL;
                gemObject* trg = pet->GetTarget();
                if(trg != NULL)
                {
                    gemActor* targetActor = trg->GetActorPtr();
                    /* We check if the owner can attack the other entity in order to not allow players
                     * to override permissions and at the same time allowing pet<->player, pet<->pet
                     * when in pvp. We allow gm to do anything they want (can attack everything including
                     * their own pet just like how it happens with players
                     */
                    csString msg; // Not used
                    if(targetActor == NULL || !owner->GetActor()->IsAllowedToAttack(trg, msg) ||
                            (trg == pet && !owner->IsGM()))

                    {
                        psserver->SendSystemInfo(me->clientnum,"Your familiar refuses.");
                    }
                    else if(targetActor && !targetActor->CanBeAttackedBy(pet,lastAttacker))
                    {
                        csString tmp;
                        if(lastAttacker)
                        {
                            tmp.Format("You must be grouped with %s for your pet to attack %s.",
                                       lastAttacker->GetName(), trg->GetName());
                        }
                        else
                        {
                            tmp.Format("Your pet is not allowed to attack right now.");
                        }
                        psserver->SendSystemInfo(me->clientnum,tmp.GetDataSafe());
                    }
                    else
                    {
                        Stance stance = CombatManager::GetStance(cacheManager, "Aggressive");
                        if(optionWords.GetCount() != 0)
                        {
                            stance.stance_id = optionWords.GetInt(0);
                        }
                        QueueOwnerCmdPerception(owner->GetActor(), pet, psPETCommandMessage::CMD_ATTACK);
                        if(!session->IsInTrainingLockout())
                        {
                            owner->GetCharacterData()->Skills().AddSkillPractice(GetPetSkill(), 1);
                            session->ReceivedTraining();
                        }
                    }
                }
                else
                {
                    psserver->SendSystemInfo(me->clientnum, "Your %s needs a target to attack.", typeStr);
                }
            }
        }
        break;
        case psPETCommandMessage::CMD_STOPATTACK :
        {
            PetOwnerSession* session = OwnerPetList.Get(pet->GetCharacterData()->GetPID(), NULL);
            if(!session)
            {
                CPrintf(CON_NOTIFY, "Cannot locate PetSession for owner %s.\n", pet->GetName(), owner->GetName());
                return;
            }

            if(CanPetHearYou(me->clientnum, owner, pet, typeStr) && WillPetReact(me->clientnum, owner, pet, typeStr, 4))
            {
                QueueOwnerCmdPerception(owner->GetActor(), pet, psPETCommandMessage::CMD_STOPATTACK);
                if(!session->IsInTrainingLockout())
                {
                    owner->GetCharacterData()->Skills().AddSkillPractice(GetPetSkill(), 1);
                    session->ReceivedTraining();
                }
            }
        }
        break;
    }
}

void NPCManager::DismissPet(gemNPC* pet, Client* owner)
{
    PetOwnerSession* session = OwnerPetList.Get(pet->GetCharacterData()->GetPID(), NULL);

    // Check for an existing session
    if(!session)
    {
        CPrintf(CON_NOTIFY, "Cannot locate PetSession for owner %s.\n", pet->GetName(), owner->GetName());
    }
    else
    {
        session->Dismiss();
    }

    psserver->CharacterLoader.SaveCharacterData(pet->GetCharacterData(), pet, true);

    CPrintf(CON_NOTIFY, "NPCManager Removing familiar %s from owner %s.\n", pet->GetName(), owner->GetName());
    owner->SetFamiliar(NULL);
    entityManager->RemoveActor(pet);
}

void NPCManager::PrepareMessage()
{
    outbound = new psNPCCommandsMessage(0,MAX_NPC_COMMANS_MESSAGE_SIZE);
    cmd_count = 0;
}

void NPCManager::CheckSendPerceptionQueue(size_t expectedAddSize)
{
    const size_t TERMINATE_MSG_SIZE = sizeof(uint8_t);

    if(outbound->msg->GetSize()+expectedAddSize+TERMINATE_MSG_SIZE > MAX_NPC_COMMANS_MESSAGE_SIZE)
    {
        SendAllCommands(false); //as this happens before an npctick we don't create a new one
    }
}

/**
 */
void NPCManager::QueueAssessPerception(EID entityEID, EID targetEID, const csString &physicalAssessmentPerception,
                                       const csString &physicalAssessmentDifferencePerception,
                                       const csString &magicalAssessmentPerception,
                                       const csString &magicalAssessmentDifferencePerception,
                                       const csString &overallAssessmentPerception,
                                       const csString &overallAssessmentDifferencePerception)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+2*sizeof(uint32_t)+(physicalAssessmentPerception.Length()+1)+
                             (physicalAssessmentDifferencePerception.Length()+1)+
                             (magicalAssessmentPerception.Length()+1)+
                             (magicalAssessmentDifferencePerception.Length()+1)+
                             (overallAssessmentPerception.Length()+1)+
                             (overallAssessmentDifferencePerception.Length()+1));

    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_ASSESS);
    outbound->msg->Add(entityEID.Unbox());
    outbound->msg->Add(targetEID.Unbox());
    outbound->msg->Add(physicalAssessmentPerception);
    outbound->msg->Add(physicalAssessmentDifferencePerception);
    outbound->msg->Add(magicalAssessmentPerception);
    outbound->msg->Add(magicalAssessmentDifferencePerception);
    outbound->msg->Add(overallAssessmentPerception);
    outbound->msg->Add(overallAssessmentDifferencePerception);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueAssessPerception put message in overrun state!\n");
        Error1("NPCManager::QueueAssessPerception put message in overrun state!\n");
    }

    Debug3(LOG_NPC, entityEID.Unbox(), "Added assess perception: Entity: %s Target. %s\n",
           ShowID(entityEID),ShowID(targetEID));
}


/**
 * Talking sends the speaker, the target of the speech, and
 * the worst faction score between the two to the superclient.
 */
void NPCManager::QueueTalkPerception(gemActor* speaker,gemNPC* target)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(int16_t)+sizeof(uint32_t)*2);
    float faction = target->GetRelativeFaction(speaker);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_TALK);
    outbound->msg->Add(speaker->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add((int16_t) faction);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueTalkPerception put message in overrun state!\n");
        Error1("NPCManager::QueueTalkPerception put message in overrun state!\n");
    }

    Debug4(LOG_NPC, speaker->GetEID().Unbox(), "Added perception: %s spoke to %s with %1.1f faction standing.\n",
           speaker->GetName(),
           target->GetName(),
           faction);
}

/**
 * The initial attack perception sends group info to the superclient
 * so that the npc can populate his hate list.  Then future damage
 * perceptions can influence this hate list to help the mob decide
 * who to attack back.  The superclient can also use this perception
 * to "cheat" and not wait for first damage to attack back--although
 * we are currently not using it that way.
 */
void NPCManager::QueueAttackPerception(gemActor* attacker,gemNPC* target)
{
    if(attacker->InGroup())
    {
        csRef<PlayerGroup> g = attacker->GetGroup();
        CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+sizeof(int8_t)+
                                 (sizeof(uint32_t)+sizeof(int8_t))*g->GetMemberCount());
        outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_GROUPATTACK);
        outbound->msg->Add(target->GetEID().Unbox());
        outbound->msg->Add((int8_t) g->GetMemberCount());
        for(int i=0; i<(int)g->GetMemberCount(); i++)
        {
            outbound->msg->Add(g->GetMember(i)->GetEID().Unbox());
            outbound->msg->Add((int8_t) g->GetMember(i)->GetCharacterData()->Skills().GetBestSkillSlot(true));
        }

        cmd_count++;

        if(outbound->msg->overrun)
        {
            CS_ASSERT(!"NPCManager::QueueAttackPerception group put message in overrun state!\n");
            Error1("NPCManager::QueueAttackPerception group put message in overrun state!\n");
        }

        Debug3(LOG_NPC, attacker->GetEID().Unbox(), "Added perception: %s's group is attacking %s.\n",
               attacker->GetName(),
               target->GetName());
    }

    // lone gunman
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_ATTACK);
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add(attacker->GetEID().Unbox());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueAttackPerception lone gunman put message in overrun state!\n");
        Error1("NPCManager::QueueAttackPerception lone gunman put message in overrun state!\n");
    }

    Debug3(LOG_NPC, attacker->GetEID().Unbox(), "Added perception: %s is attacking %s.\n",
           attacker->GetName(),
           target->GetName());
}

/**
 * Each instance of damage to an NPC is sent here.  The NPC is expected
 * to use this information on his hate list.
 */
void NPCManager::QueueDamagePerception(gemActor* attacker,gemNPC* target,float dmg)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+MSG_SIZEOF_FLOAT*3);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_DMG);
    outbound->msg->Add(attacker->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add((float) dmg);
    outbound->msg->Add((float) target->GetCharacterData()->GetHP());
    outbound->msg->Add((float) target->GetCharacterData()->GetMaxHP().Current());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueDamagePerception put message in overrun state!\n");
        Error1("NPCManager::QueueDamagePerception put message in overrun state!\n");
    }

    Debug4(LOG_NPC, attacker->GetEID().Unbox(), "Added perception: %s hit %s for %1.1f dmg.\n",
           attacker->GetName(),
           target->GetName(),
           dmg);
}

void NPCManager::QueueDeathPerception(gemObject* who)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_DEATH);
    outbound->msg->Add(who->GetEID().Unbox());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueDeathPerception put message in overrun state!\n");
        Error1("NPCManager::QueueDeathPerception put message in overrun state!\n");
    }

    Debug2(LOG_NPC, who->GetEID().Unbox(), "Added perception: %s death.\n", who->GetName());
}

void NPCManager::QueueSpellPerception(gemActor* caster, gemObject* target,const char* spell_cat_name,
                                      uint32_t spell_category, float severity)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+sizeof(uint32_t)+sizeof(int8_t));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_SPELL);
    outbound->msg->Add(caster->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());
    outbound->msg->Add((uint32_t) spell_category);
    outbound->msg->Add((int8_t)(severity * 10));

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueSpellPerception put message in overrun state!\n");
        Error1("NPCManager::QueueSpellPerception put message in overrun state!\n");
    }

    Debug4(LOG_NPC, caster->GetEID().Unbox(), "Added perception: %s cast a %s spell on %s.\n", caster->GetName(), spell_cat_name, target->GetName());
}

void NPCManager::QueueStatDR(gemActor* npc, unsigned int statsDirtyFlags)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+sizeof(uint16_t)+MSG_SIZEOF_FLOAT*12);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_STAT_DR);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add((uint16_t) statsDirtyFlags);

    if(statsDirtyFlags & DIRTY_VITAL_HP)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetHP());
    }
    if(statsDirtyFlags & DIRTY_VITAL_HP_MAX)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetMaxHP().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_HP_RATE)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetHPRate().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_MANA)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetMana());
    }
    if(statsDirtyFlags & DIRTY_VITAL_MANA_MAX)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetMaxMana().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_MANA_RATE)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetManaRate().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetStamina(true));
    }
    if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA_MAX)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetMaxPStamina().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_PYSSTAMINA_RATE)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetPStaminaRate().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetStamina(false));
    }
    if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA_MAX)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetMaxMStamina().Current());
    }
    if(statsDirtyFlags & DIRTY_VITAL_MENSTAMINA_RATE)
    {
        outbound->msg->Add((float) npc->GetCharacterData()->GetMStaminaRate().Current());
    }

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueStatDR put message in overrun state!\n");
        Error1("NPCManager::QueueStatDR put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added perception: StatDR for %s\n",npc->GetName());
}


void NPCManager::QueueEnemyPerception(psNPCCommandsMessage::PerceptionType type,
                                      gemActor* npc, gemActor* player,
                                      float relative_faction)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+sizeof(float));
    outbound->msg->Add((int8_t) type);
    outbound->msg->Add(npc->GetEID().Unbox());   // Only entity IDs are passed to npcclient
    outbound->msg->Add(player->GetEID().Unbox());
    outbound->msg->Add((float) relative_faction);
    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueEnemyPerception put message in overrun state!\n");
        Error1("NPCManager::QueueEnemyPerception put message in overrun state!\n");
    }

    Debug5(LOG_NPC, player->GetEID().Unbox(), "Added perception: Entity %s within range of entity %s, type %d, faction %.0f.\n", ShowID(player->GetEID()), ShowID(npc->GetEID()), type, relative_faction);

    gemNPC* myNPC = dynamic_cast<gemNPC*>(npc);
    if(!myNPC)
        return;  // Illegal to not pass actual npc object to this function

    myNPC->ReactToPlayerApproach(type, player);
}

/**
 * The client /pet stay command cause the OwnerCmd perception to be sent to
 * the superclient.
 */
void NPCManager::QueueOwnerCmdPerception(gemActor* owner, gemNPC* pet, psPETCommandMessage::PetCommand_t command)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+sizeof(uint32_t)*3);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_OWNER_CMD);
    outbound->msg->Add((uint32_t) command);
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add(pet->GetEID().Unbox());
    outbound->msg->Add((uint32_t)(pet->GetTarget() ? pet->GetTarget()->GetEID().Unbox() : 0));

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueOwnerCmdPerception put message in overrun state!\n");
        Error1("NPCManager::QueueOwnerCmdPerception put message in overrun state!\n");
    }

    Debug4(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s has told %s to %d.\n",
           owner->GetName(),
           pet->GetName(), (int)command);
}

void NPCManager::QueueInventoryPerception(gemActor* owner, psItem* itemdata, bool inserted)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+strlen(itemdata->GetName())+1+
                             sizeof(bool)+sizeof(int16_t));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_INVENTORY);
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add((char*) itemdata->GetName());
    outbound->msg->Add((bool) inserted);
    outbound->msg->Add((int16_t) itemdata->GetStackCount());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueInventoryPerception put message in overrun state!\n");
        Error1("NPCManager::QueueInventoryPerception put message in overrun state!\n");
    }

    Debug7(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) has %s %d %s %s inventory.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           (inserted?"added":"removed"),
           itemdata->GetStackCount(),
           itemdata->GetName(),
           (inserted?"to":"from"));
}

void NPCManager::QueueFlagPerception(gemActor* owner)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+sizeof(uint32_t));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_FLAG);
    outbound->msg->Add(owner->GetEID().Unbox());

    uint32_t flags = 0;

    if(!owner->GetVisibility())   flags |= psNPCCommandsMessage::INVISIBLE;
    if(owner->GetInvincibility()) flags |= psNPCCommandsMessage::INVINCIBLE;
    if(owner->IsAlive())          flags |= psNPCCommandsMessage::IS_ALIVE;

    outbound->msg->Add(flags);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueFlagPerception put message in overrun state!\n");
        Error1("NPCManager::QueueFlagPerception put message in overrun state!\n");
    }

    Debug4(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) flags 0x%X.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           flags);

}

void NPCManager::QueueNPCCmdPerception(gemActor* owner, const csString &cmd)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+cmd.Length()+1);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_NPCCMD);
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add(cmd);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueNPCCmdPerception put message in overrun state!\n");
        Error1("NPCManager::QueueNPCCmdPerception put message in overrun state!\n");
    }

    Debug4(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) npc cmd %s.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           cmd.GetData());

}

void NPCManager::QueueTransferPerception(gemActor* owner, psItem* itemdata, csString target)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+strlen(itemdata->GetName())+1+
                             sizeof(int8_t)+target.Length()+1);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_TRANSFER);
    outbound->msg->Add(owner->GetEID().Unbox());
    outbound->msg->Add((char*) itemdata->GetName());
    outbound->msg->Add((int8_t) itemdata->GetStackCount());
    outbound->msg->Add((char*) target.GetDataSafe());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueTransferPerception put message in overrun state!\n");
        Error1("NPCManager::QueueTransferPerception put message in overrun state!\n");
    }

    Debug6(LOG_NPC, owner->GetEID().Unbox(), "Added perception: %s(%s) has transfered %d %s to %s.\n",
           owner->GetName(),
           ShowID(owner->GetEID()),
           itemdata->GetStackCount(),
           itemdata->GetName(),
           target.GetDataSafe());
}

void NPCManager::QueueSpawnedPerception(gemNPC* spawned, gemNPC* spawner, const csString &tribeMemberType)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*4);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_SPAWNED);
    outbound->msg->Add(spawned->GetCharacterData()->GetPID().Unbox());
    outbound->msg->Add(spawned->GetEID().Unbox());
    outbound->msg->Add(spawner->GetEID().Unbox());
    outbound->msg->Add(tribeMemberType);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueSpawnedPerception put message in overrun state!\n");
        Error1("NPCManager::QueueSpawnedPerception put message in overrun state!\n");
    }

    Debug3(LOG_NPC, spawner->GetEID().Unbox(), "Added spawn perception: %s from %s.\n", ShowID(spawned->GetEID()), ShowID(spawner->GetEID()));
}

void NPCManager::QueueTeleportPerception(gemNPC* npc, csVector3 &pos, float yrot, iSector* sector, InstanceID instance)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*3+sizeof(float)*4+strlen(sector->QueryObject()->GetName()));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_TELEPORT);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(pos);
    outbound->msg->Add(yrot);
    outbound->msg->Add(sector, psserver->GetCacheManager()->GetMsgStrings());
    outbound->msg->Add(instance);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueTeleportPerception put message in overrun state!\n");
        Error1("NPCManager::QueueTeleportPerception put message in overrun state!\n");
    }

    Debug3(LOG_NPC, npc->GetEID().Unbox(), "Added teleport perception for %s to %s.\n", ShowID(npc->GetEID()), toString(pos,sector).GetDataSafe());
}

void NPCManager::QueueInfoRequestPerception(gemNPC* npc, Client* client, const char* infoRequestSubCmd)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+(strlen(infoRequestSubCmd)+1));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_INFO_REQUEST);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(client->GetClientNum());
    outbound->msg->Add(infoRequestSubCmd);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueInfoRequestPerception put message in overrun state!\n");
        Error1("NPCManager::QueueInfoRequestPerception put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added Info Request perception for %s.\n", ShowID(npc->GetEID()));
}

void NPCManager::QueueChangeOwnerPerception(gemNPC* npc, EID owner)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2);
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_CHANGE_OWNER);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(owner.Unbox());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueChangeOwnerPerception put message in overrun state!\n");
        Error1("NPCManager::QueueChangeOwnerPerception put message in overrun state!\n");
    }

    Debug3(LOG_NPC, npc->GetEID().Unbox(), "Change owner of %s to %s.\n", ShowID(npc->GetEID()), ShowID(owner));
}

void NPCManager::QueueFailedToAttackPerception(gemNPC* attacker, gemObject* target)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+2*sizeof(uint32_t));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_FAILED_TO_ATTACK);
    outbound->msg->Add(attacker->GetEID().Unbox());
    outbound->msg->Add(target->GetEID().Unbox());

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueFailedToAttackPerception put message in overrun state!\n");
        Error1("NPCManager::QueueFailedToAttackPerception put message in overrun state!\n");
    }

    Debug2(LOG_NPC, attacker->GetEID().Unbox(), "Added Failed to Attack perception for %s.\n", ShowID(attacker->GetEID()));
}

void NPCManager::QueuePerceptPerception(gemNPC* npc, csString perception, csString type)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+(perception.Length()+1)+(type.Length()+1));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_PERCEPT);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(perception);
    outbound->msg->Add(type);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueuePerceptPerception put message in overrun state!\n");
        Error1("NPCManager::QueuePerceptPerception put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added Percept perception for %s.\n", ShowID(npc->GetEID()));
}

void NPCManager::QueueSpokenToPerception(gemNPC* npc, bool spokenTo)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)+sizeof(bool));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_SPOKEN_TO);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(spokenTo);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::QueueSpokenToPerception put message in overrun state!\n");
        Error1("NPCManager::QueueSpokenToPerception put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added SpokenTo perception for %s.\n", ShowID(npc->GetEID()));
}




void NPCManager::ChangeNPCBrain(gemNPC* npc, Client* client, const char* brainName)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+(strlen(brainName)+1));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_CHANGE_BRAIN);
    outbound->msg->Add(npc->GetEID().Unbox());
    if (client)
    {
        outbound->msg->Add((uint32_t)client->GetClientNum());
    }
    else
    {
        outbound->msg->Add((uint32_t)0);
    }
    
    outbound->msg->Add(brainName);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::ChangeNPCBrain put message in overrun state!\n");
        Error1("NPCManager::ChangeNPCBrain put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added Brain Change perception for %s.\n", ShowID(npc->GetEID()));
}

void NPCManager::DebugNPC(gemNPC* npc, Client* client, uint8_t debugLevel)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+sizeof(uint8));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_DEBUG_NPC);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(client->GetClientNum());
    outbound->msg->Add(debugLevel);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::DebugNPC put message in overrun state!\n");
        Error1("NPCManager::DebugNPC put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added NPC Debug Level perception for %s.\n", ShowID(npc->GetEID()));
}

void NPCManager::DebugTribe(gemNPC* npc, Client* client, uint8_t debugLevel)
{
    CheckSendPerceptionQueue(sizeof(int8_t)+sizeof(uint32_t)*2+sizeof(uint8));
    outbound->msg->Add((int8_t) psNPCCommandsMessage::PCPT_DEBUG_TRIBE);
    outbound->msg->Add(npc->GetEID().Unbox());
    outbound->msg->Add(client->GetClientNum());
    outbound->msg->Add(debugLevel);

    cmd_count++;

    if(outbound->msg->overrun)
    {
        CS_ASSERT(!"NPCManager::DebugTribe put message in overrun state!\n");
        Error1("NPCManager::DebugTribe put message in overrun state!\n");
    }

    Debug2(LOG_NPC, npc->GetEID().Unbox(), "Added Tribe Debug Level perception for %s.\n", ShowID(npc->GetEID()));
}



void NPCManager::SendAllCommands(bool createNewTick)
{
    if(cmd_count)
    {
        outbound->msg->Add((int8_t) psNPCCommandsMessage::CMD_TERMINATOR);
        if(outbound->msg->overrun)
        {
            Bug1("Failed to terminate and send psNPCCommandsMessage.  Would have overrun buffer.\n");
        }
        else
        {
            outbound->msg->ClipToCurrentSize();

            // CPrintf(CON_DEBUG, "Sending %d bytes to superclients...\n",outbound->msg->data->GetTotalSize() );

            outbound->Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
        }
        delete outbound;
        outbound=NULL;
        PrepareMessage();
    }

    if(createNewTick)
    {
        psNPCManagerTick* tick = new psNPCManagerTick(NPC_TICK_INTERVAL,this);
        eventmanager->Push(tick);
    }
}

void NPCManager::NewNPCNotify(PID player_id, PID master_id, PID owner_id)
{
    Debug4(LOG_NPC, 0, "New NPC(%s) with master(%s) and owner(%s) sent to superclients.\n",
           ShowID(player_id), ShowID(master_id), ShowID(owner_id));

    psNewNPCCreatedMessage msg(0, player_id, master_id, owner_id);
    msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
}

void NPCManager::DeletedNPCNotify(PID player_id)
{
    Debug2(LOG_NPC, 0, "Deleted NPC(%s) sent to superclients.", ShowID(player_id));

    psNPCDeletedMessage msg(0, player_id);
    msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
}


void NPCManager::WorkDoneNotify(EID npcEID, csString reward, csString nick)
{
    Debug3(LOG_NPC, npcEID.Unbox(), "NPC(%s) got item %s.\n", ShowID(npcEID), reward.GetData());
    psNPCWorkDoneMessage msg(0, npcEID, reward.GetData(), nick.GetData());
    msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
}

void NPCManager::ControlNPC(gemNPC* npc)
{
    Client* superclient = clients->FindAccount(npc->GetSuperclientID());
    if(superclient)
    {
        npc->GetCharacterData()->SetImperviousToAttack(npc->GetCharacterData()->GetImperviousToAttack() & ~TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
    }
    else
    {
        npc->GetCharacterData()->SetImperviousToAttack(npc->GetCharacterData()->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
    }
}

void NPCManager::CheckWorkLocation(gemNPC* npc, Location* location)
{
    Client* superclient = clients->FindAccount(npc->GetSuperclientID());
    if(superclient)
    {
        // Send message to super client
        psHiredNPCScriptMessage msg(0, psHiredNPCScriptMessage::CHECK_WORK_LOCATION, npc->GetEID(),
                                    location->GetTypeName(), location->GetName());
        // TODO: Send only to one superclient.. Since we run with only one we can multicast to that for now.
        msg.Multicast(superclients,-1,PROX_LIST_ANY_RANGE);
    }
    else
    {
        // No superclient found online, deam the location invalid.
        psserver->GetHireManager()->CheckWorkLocationResult(npc, false, "Error: No superclient online.");
    }
}


PetOwnerSession* NPCManager::CreatePetOwnerSession(gemActor* owner, psCharacter* petData)
{
    if(owner && petData)
    {
        PetOwnerSession* session = new PetOwnerSession(owner, petData);
        if(session)
        {
            OwnerPetList.Put(session->petID, session);
            return session;
        }
    }
    return NULL;
}


void NPCManager::RemovePetOwnerSession(PetOwnerSession* session)
{
    if(session)
    {
        OwnerPetList.DeleteAll(session->petID);
        delete session;
    }
}

void NPCManager::UpdatePetTime()
{
    PetOwnerSession* session;

    // Loop through all Sessions
    csHash< PetOwnerSession*, PID >::GlobalIterator loop(OwnerPetList.GetIterator());
    while(loop.HasNext())      // Increase Pet Time in Game by NPC_TICK_INTERVAL
    {
        session = loop.Next();
        session->UpdateElapsedTime(NPC_TICK_INTERVAL/1000.0); // Convert from ticks to seconds
    }
}

void NPCManager::LocationAdjusted(Location* location)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager ajusting location (%s)%d\n", location->GetName(), location->GetID());

    psLocationMessage msg(psLocationMessage::LOCATION_ADJUSTED, location);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::LocationCreated(Location* location)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager created location (%s)%d\n", location->GetName(), location->GetID());

    psLocationMessage msg(psLocationMessage::LOCATION_CREATED, location);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::LocationInserted(Location* location)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager inserted location (%s)%d\n", location->GetName(), location->GetID());

    psLocationMessage msg(psLocationMessage::LOCATION_INSERTED, location);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::LocationRadius(Location* location)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager changed radius of location (%s)%d\n", location->GetName(), location->GetID());

    psLocationMessage msg(psLocationMessage::LOCATION_RADIUS, location);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::LocationTypeAdd(LocationType* locationType)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager added new location type (%s)%d\n", locationType->GetName(), locationType->GetID());

    psLocationMessage msg(psLocationMessage::LOCATION_TYPE_ADD, locationType);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::LocationTypeRemove(const csString &locationTypeName)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager removed location type (%s)\n", locationTypeName.GetDataSafe());

    psLocationMessage msg(psLocationMessage::LOCATION_TYPE_REMOVE, locationTypeName);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointAdjusted(Waypoint* wp)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager ajusting waypoint %d\n", wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_ADJUSTED, wp);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::PathPointAdjusted(psPathPoint* point)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager ajusting pathpoint %d\n", point->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::POINT_ADJUSTED, point);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointAddAlias(const Waypoint* wp, const WaypointAlias* alias)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager added alias %s to waypoint %d\n",
           alias->GetName(), wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_ALIAS_ADD, wp, alias);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointAliasRotation(const Waypoint* wp, const WaypointAlias* alias)
{
    Debug4(LOG_SUPERCLIENT, 0, "NPCManager changed rotation of alias %s of waypoint %d to %.1f\n",
           alias->GetName(), wp->GetID(),alias->GetRotationAngle()*180.0/PI);

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_ALIAS_ROTATION_ANGLE, wp, alias);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointRemoveAlias(const Waypoint* wp, const csString &alias)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager removed alias %s from waypoint %d\n",
           alias.GetDataSafe(), wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_ALIAS_REMOVE, wp, alias);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointSetFlag(const Waypoint* wp, const csString &flag, bool enable)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager Set flag %s from waypoint %d\n",
           flag.GetDataSafe(), wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_SET_FLAG, wp, flag, enable);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::PathSetFlag(const psPath* path, const csString &flag, bool enable)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager Set flag %s from path %d\n",
           flag.GetDataSafe(), path->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::PATH_SET_FLAG, path, flag, enable);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointRadius(const Waypoint* wp)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager Set radius %.2f on waypoint %d\n",
           wp->GetRadius(), wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_RADIUS, wp);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointRename(const Waypoint* wp)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager Set new name %s on waypoint %d\n",
           wp->GetName(), wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_RENAME, wp);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::PathRename(const psPath* path)
{
    Debug3(LOG_SUPERCLIENT, 0, "NPCManager Set new name %s on path %d\n",
           path->GetName(), path->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::PATH_RENAME, path);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::WaypointCreate(const Waypoint* wp)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager create waypoint %d\n",
           wp->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::WAYPOINT_CREATE, wp);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::PathCreate(const psPath* path)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager create path %d\n",
           path->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::PATH_CREATE, path);
    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);

    // Add all the points
    // First and last is waypoints
    for(int i = 1; i < path->GetNumPoints()-1; i++)
    {
        psPathNetworkMessage msg(psPathNetworkMessage::PATH_ADD_POINT, path, path->GetPoint(i));

        msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
    }

}

void NPCManager::AddPoint(const psPath* path, const psPathPoint* point)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager add point to path %d\n",
           path->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::PATH_ADD_POINT, path, point);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

void NPCManager::RemovePoint(const psPath* path, int pointId)
{
    Debug2(LOG_SUPERCLIENT, 0, "NPCManager remove point from path %d\n",
           path->GetID());

    psPathNetworkMessage msg(psPathNetworkMessage::PATH_REMOVE_POINT, path, pointId);

    msg.Multicast(superclients, -1, PROX_LIST_ANY_RANGE);
}

/*------------------------------------------------------------------*/

psNPCManagerTick::psNPCManagerTick(int offsetticks, NPCManager* c)
    : psGameEvent(0,offsetticks,"psNPCManagerTick")
{
    npcmgr = c;
}

void psNPCManagerTick::Trigger()
{
    static int counter=0;

    if(!(counter%3))
        npcmgr->UpdateWorldPositions();

    npcmgr->SendAllCommands(true); // Flag to create new tick event
    npcmgr->UpdatePetTime();
    counter++;

}

/*-----------------------------------------------------------------*/
/* Pet Skills Handler                                              */
/*-----------------------------------------------------------------*/
void NPCManager::HandlePetSkill(MsgEntry* me,Client* client)
{
    psPetSkillMessage msg(me);
    if(!msg.valid)
    {
        Debug2(LOG_NET, me->clientnum, "Received unparsable psPetSkillMessage from client %u.\n", me->clientnum);
        return;
    }
    // Client* client = clients->FindAny( me->clientnum );

    if(!client->GetFamiliar())
        return;

    //    CPrintf(CON_DEBUG, "ProgressionManager::HandleSkill(%d,%s)\n",msg.command, (const char*)msg.commandData);
    switch(msg.command)
    {
        case psPetSkillMessage::REQUEST:
        {
            // Send all petStats to seed client
            psCharacter* chr = client->GetFamiliar()->GetCharacterData();
            chr->SendStatDRMessage(me->clientnum, client->GetFamiliar()->GetEID(), DIRTY_VITAL_ALL);

            SendPetSkillList(client);
            break;
        }
        case psPetSkillMessage::SKILL_SELECTED:
        {
            csRef<iDocumentSystem> xml;
            xml.AttachNew(new csTinyDocumentSystem);

            CS_ASSERT(xml);

            csRef<iDocument> invList  = xml->CreateDocument();

            const char* error = invList->Parse(msg.commandData);
            if(error)
            {
                Error2("Error in XML: %s", error);
                return;
            }

            csRef<iDocumentNode> root = invList->GetRoot();
            if(!root)
            {
                Error1("No XML root");
                return;
            }
            csRef<iDocumentNode> topNode = root->GetNode("S");
            if(!topNode)
            {
                Error1("No <S> tag");
                return;
            }

            csString skillName = topNode->GetAttributeValue("NAME");

            psSkillInfo* info = cacheManager->GetSkillByName(skillName);

            csString buff;
            if(info)
            {
                csString escpxml = EscpXML(info->description);
                buff.Format("<DESCRIPTION DESC=\"%s\" CAT=\"%d\"/>", escpxml.GetData(), info->category);
            }
            //TODO: this code needs some checking. before it attempted to get info->category. which would be a null pointer deference.
            else
            {
                buff.Format("<DESCRIPTION DESC=\"\" CAT=\"\"/>");
            }

            psCharacter* chr = client->GetFamiliar()->GetCharacterData();
            MathEnvironment skillVal;
            chr->GetSkillValues(&skillVal);

            psPetSkillMessage newmsg(client->GetClientNum(),
                                     psPetSkillMessage::DESCRIPTION,
                                     buff,
                                     (unsigned int)skillVal.Lookup("STR")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("END")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("AGI")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("INT")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("WIL")->GetRoundValue(),
                                     (unsigned int)skillVal.Lookup("CHA")->GetRoundValue(),
                                     (unsigned int)(chr->GetHP()),
                                     (unsigned int)(chr->GetMana()),
                                     (unsigned int)(chr->GetStamina(true)),
                                     (unsigned int)(chr->GetStamina(false)),
                                     (unsigned int)(chr->GetMaxHP().Current()),
                                     (unsigned int)(chr->GetMaxMana().Current()),
                                     (unsigned int)(chr->GetMaxPStamina().Current()),
                                     (unsigned int)(chr->GetMaxMStamina().Current()),
                                     true,
                                     PSSKILL_NONE);

            if(newmsg.valid)
            {
                eventmanager->SendMessage(newmsg.msg);
            }
            else
            {
                Bug2("Could not create valid psPetSkillMessage for client %u.\n",client->GetClientNum());
            }
            break;
        }

        case psPetSkillMessage::QUIT:
        {
            //client->GetCharacterData()->SetTrainer(NULL);
            break;
        }
    }
}

void NPCManager::SendPetSkillList(Client* client, bool forceOpen, PSSKILL focus)
{
    csString buff;
    psCharacter* character = client->GetFamiliar()->GetCharacterData();

    buff.Format("<L X=\"%u\" >", character->GetProgressionPoints());

    int realID = -1;
    bool found = false;

    for(unsigned int skillID = 0; skillID < cacheManager->GetSkillAmount(); skillID++)
    {
        psSkillInfo* info = cacheManager->GetSkillByID(skillID);
        if(!info)
        {
            Error2("Can't find skill %d",skillID);
            return;
        }

        Skill &charSkill = character->Skills().Get((PSSKILL) skillID);

        csString escpxml = EscpXML(info->name);

        buff.AppendFmt("<SKILL NAME=\"%s\" R=\"%i\" Y=\"%i\" YC=\"%i\" Z=\"%i\" ZC=\"%i\" CAT=\"%d\"/>",
                       escpxml.GetData(), charSkill.rank.Base(),
                       charSkill.y, charSkill.yCost, charSkill.z, charSkill.zCost, info->category);
    }
    buff.Append("</L>");

    psCharacter* chr = client->GetFamiliar()->GetCharacterData();

    MathEnvironment skillVal;
    chr->GetSkillValues(&skillVal);

    psPetSkillMessage newmsg(client->GetClientNum(),
                             psPetSkillMessage::SKILL_LIST,
                             buff,
                             (unsigned int)skillVal.Lookup("STR")->GetRoundValue(),
                             (unsigned int)skillVal.Lookup("END")->GetRoundValue(),
                             (unsigned int)skillVal.Lookup("AGI")->GetRoundValue(),
                             (unsigned int)skillVal.Lookup("INT")->GetRoundValue(),
                             (unsigned int)skillVal.Lookup("WIL")->GetRoundValue(),
                             (unsigned int)skillVal.Lookup("CHA")->GetRoundValue(),
                             (unsigned int)chr->GetHP(),
                             (unsigned int)chr->GetMana(),
                             (unsigned int)chr->GetStamina(true),
                             (unsigned int)chr->GetStamina(false),
                             (unsigned int)chr->GetMaxHP().Current(),
                             (unsigned int)chr->GetMaxMana().Current(),
                             (unsigned int)chr->GetMaxPStamina().Current(),
                             (unsigned int)chr->GetMaxMStamina().Current(),
                             forceOpen,
                             found?realID:-1
                            );

    CPrintf(CON_DEBUG, "Sending psPetSkillMessage w/ stats to %d, Valid: ",int(client->GetClientNum()));
    if(newmsg.valid)
    {
        CPrintf(CON_DEBUG, "Yes\n");
        eventmanager->SendMessage(newmsg.msg);

    }
    else
    {
        CPrintf(CON_DEBUG, "No\n");
        Bug2("Could not create valid psPetSkillMessage for client %u.\n",client->GetClientNum());
    }
}


void NPCManager::PetHasBeenKilled(gemNPC*  pet)
{
    PetOwnerSession* session = OwnerPetList.Get(pet->GetCharacterData()->GetPID(), NULL);

    if(session)
    {
        session->HasBeenKilled();
    }

}

void NPCManager::PetInfo(Client* client, psCharacter* pet)
{
    PetOwnerSession* session = OwnerPetList.Get(pet->GetPID(), NULL);
    if(!session)
    {
        CPrintf(CON_NOTIFY, "Cannot locate PetSession for %s.\n", ShowID(pet->GetPID()));
        return;
    }

    psserver->SendSystemInfo(client->GetClientNum(),
                             "PET: active: %s elapsed: %.2f max time: %.2f Lockout's: death: %.2f training: %.2f dismiss: %.2f depleted: %.2f",
                             session->isActive?"yes":"no",
                             session->elapsedTime,
                             session->maxTime,
                             session->deathLockoutTime,
                             session->trainingLockoutTime,
                             session->dismissLockoutTime,
                             session->depletedLockoutTime);
}
