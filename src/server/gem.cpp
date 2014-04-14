/*
* gem.cpp by Keith Fulton <keith@paqrat.com>
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
#include <iengine/engine.h>
#include <iengine/campos.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <ivideo/txtmgr.h>
#include <ivideo/texture.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/object.h>

#include <csgeom/box.h>
#include <imesh/objmodel.h>
#include <csgeom/transfrm.h>
#include <csutil/snprintf.h>
#include <csutil/hash.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <imesh/nullmesh.h>
#include <imap/loader.h>
#include <csqsqrt.h>  // CS quick square root

//=============================================================================
// Project Space Includes
//=============================================================================
#include "bulkobjects/psraceinfo.h"
#include "bulkobjects/servervitals.h"
#include "bulkobjects/psactionlocationinfo.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psnpcdialog.h"
#include "bulkobjects/pscharacter.h"
#include "bulkobjects/psquest.h"

#include "rpgrules/factions.h"

#include "util/gameevent.h"
#include "util/pserror.h"
#include "util/strutil.h"
#include "util/eventmanager.h"
#include "util/psutil.h"
#include "util/serverconsole.h"
#include "util/mathscript.h"

#include "net/npcmessages.h"
#include "net/message.h"
#include "net/msghandler.h"

#include "engine/psworld.h"
#include "engine/linmove.h"

//=============================================================================
// Local Space Includes
//=============================================================================
#include "client.h"
#include "clients.h"
#include "playergroup.h"
#include "gem.h"
#include "gemmesh.h"
#include "invitemanager.h"
#include "chatmanager.h"
#include "groupmanager.h"
#include "entitymanager.h"
#include "spawnmanager.h"
#include "usermanager.h"
#include "exchangemanager.h"
#include "events.h"
#include "psserverdr.h"
#include "psserver.h"
#include "weathermanager.h"
#include "npcmanager.h"
#include "netmanager.h"
#include "globals.h"
#include "progressionmanager.h"
#include "workmanager.h"
#include "cachemanager.h"
#include "psproxlist.h"
#include "spellmanager.h"
#include "psserverchar.h"
#include "adminmanager.h"
#include "commandmanager.h"
#include "combatmanager.h"
#include "serversongmngr.h"
#include "scripting.h"

// #define PSPROXDEBUG

#define SPEED_WALK 2.0f

/// Minimum size for the history buffer. old lines are not removed when this size is reached.
#define CHAT_HISTORY_MINIMUM_SIZE 20
/// Lifetime of a chat history line, in ticks
#define CHAT_HISTORY_LIFETIME 300000 // 5 minutes

//-----------------------------------------------------------------------------

psGemServerMeshAttach::psGemServerMeshAttach(gemObject* objectToAttach) : scfImplementationType(this)
{
    object = objectToAttach;
}


//-----------------------------------------------------------------------------

GEMSupervisor* gemObject::cel = NULL;

GEMSupervisor::GEMSupervisor(iObjectRegistry* objreg,
                             psDatabase* db,
                             EntityManager* entitymanager,
                             CacheManager* cachemanager)
{
    object_reg = objreg;
    database = db;
    npcmanager = NULL;
    entityManager = entitymanager;
    cacheManager = cachemanager;

    // Start eids at 10000. This to give nice aligned outputs for debuging.
    // Default celID scope has max of 100000 IDs so to support more than
    // 90000 enties another scope should be added to cel
    nextEID = 10000;

    Subscribe(&GEMSupervisor::HandleDamageMessage,MSGTYPE_DAMAGE_EVENT,NO_VALIDATION);
    Subscribe(&GEMSupervisor::HandleStatDRUpdateMessage,MSGTYPE_STATDRUPDATE, REQUIRE_READY_CLIENT);
    Subscribe(&GEMSupervisor::HandleStatsMessage,MSGTYPE_STATS, REQUIRE_READY_CLIENT);

    engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
}

GEMSupervisor::~GEMSupervisor()
{
    // Slow but safe method of deleting.
    size_t count = entities_by_eid.GetSize();
    while(count > 0)
    {
        csHash<gemObject*, EID>::GlobalIterator i(entities_by_eid.GetIterator());
        gemObject* obj = i.Next();
        delete obj;

        // Make sure the gemObject has deleted itself from our list otherwise
        // something has gone very wrong.
        CS_ASSERT(count > entities_by_eid.GetSize());
        count = entities_by_eid.GetSize();
        continue;
    }
}

void GEMSupervisor::HandleStatsMessage(MsgEntry* me,Client* client)
{
    if(!client->GetActor())
    {
        return;
    }
    psCharacter* psChar = client->GetActor()->GetCharacterData();

    psStatsMessage msg(client->GetClientNum(),
                       psChar->GetMaxHP().Current(),
                       psChar->GetMaxMana().Current(),
                       psChar->Inventory().MaxWeight(),
                       psChar->Inventory().GetCurrentMaxSpace());

    msg.SendMessage();
}

void GEMSupervisor::HandleDamageMessage(MsgEntry* me,Client* client)
{
    psDamageEvent evt(me);
    evt.target->BroadcastTargetStatDR(entityManager->GetClients());
}

void GEMSupervisor::HandleStatDRUpdateMessage(MsgEntry* me,Client* client)
{
    gemActor* actor = client->GetActor();
    psCharacter* psChar = actor->GetCharacterData();

    psChar->SendStatDRMessage(client->GetClientNum(), actor->GetEID(), DIRTY_VITAL_ALL);
}

EID GEMSupervisor::GetNextID()
{
    return EID(nextEID++);
}

EID GEMSupervisor::CreateEntity(gemObject* obj)
{
    EID eid = GetNextID();

    if(obj)
    {
        entities_by_eid.Put(eid, obj);
        Debug3(LOG_CELPERSIST,0,"Entity <%s> added to supervisor as %s\n", obj->GetName(), ShowID(eid));
    }

    return eid;
}

void GEMSupervisor::AddEntity(gemObject* obj, EID objEid)
{
    CS_ASSERT(!entities_by_eid.Contains(objEid));
    entities_by_eid.Put(objEid, obj);
    Debug3(LOG_CELPERSIST,0,"Entity <%s> added to supervisor as %s\n", obj->GetName(), ShowID(objEid));
}

void GEMSupervisor::AddActorEntity(gemActor* actor)
{
    actors_by_pid.Put(actor->GetPID(), actor);
    Debug3(LOG_CELPERSIST,0,"Actor added to supervisor with %s and %s.\n", ShowID(actor->GetEID()), ShowID(actor->GetPID()));
}

void GEMSupervisor::RemoveActorEntity(gemActor* actor)
{
    actors_by_pid.Delete(actor->GetPID(), actor);
    Debug3(LOG_CELPERSIST,0,"Actor <%s, %s> removed from supervisor.\n", ShowID(actor->GetEID()), ShowID(actor->GetPID()));
}

void GEMSupervisor::AddItemEntity(gemItem* item)
{
    items_by_uid.Put(item->GetItem()->GetUID(), item);
    Debug3(LOG_CELPERSIST,0,"Item added to supervisor with %s and UID:%u.\n", ShowID(item->GetEID()), item->GetItem()->GetUID());
}

void GEMSupervisor::RemoveItemEntity(gemItem* item, uint32 uid)
{
    items_by_uid.Delete(uid, item);
    Debug3(LOG_CELPERSIST,0,"Item <%s, %u> removed from supervisor.\n", ShowID(item->GetEID()), uid);
}

void GEMSupervisor::RemoveEntity(gemObject* which)
{
    if(!which)
        return;

    entities_by_eid.Delete(which->GetEID(), which);
    Debug3(LOG_CELPERSIST,0,"Entity <%s, %s> removed from supervisor.\n", which->GetName(), ShowID(which->GetEID()));

}

void GEMSupervisor::RemovePlayerFromLootables(PID playerID)
{
    csHash<gemObject*, EID>::GlobalIterator i(entities_by_eid.GetIterator());
    while(i.HasNext())
    {
        gemObject* obj = i.Next();
        gemNPC* npc = obj->GetNPCPtr();

        if(npc)
            npc->RemoveLootablePlayer(playerID);
    }
}


gemObject* GEMSupervisor::FindObject(EID id)
{
    if(!id.IsValid())
        return NULL;

    return entities_by_eid.Get(id, NULL);
}

gemObject* GEMSupervisor::FindObject(const csString &name)
{
    csHash<gemObject*, EID>::GlobalIterator i(entities_by_eid.GetIterator());

    while(i.HasNext())
    {
        gemObject* obj = i.Next();
        if(name.CompareNoCase(obj->GetName()))
            return obj;
        else if(obj->GetNPCPtr())    //Allow search for first name only with NPCs
        {
            WordArray names(obj->GetName(), false);
            if(name.CompareNoCase(names[0]))
                return obj;
        }
    }
    Debug2(LOG_CELPERSIST, 0,"No object with the name of '%s' was found.", name.GetData());
    return NULL;
}

gemActor* GEMSupervisor::FindPlayerEntity(PID player_id)
{
    return actors_by_pid.Get(player_id, NULL);
}

gemNPC* GEMSupervisor::FindNPCEntity(PID npc_id)
{
    return dynamic_cast<gemNPC*>(actors_by_pid.Get(npc_id, NULL));
}

gemNPC* GEMSupervisor::FindNPCEntity(EID eid)
{
    return dynamic_cast<gemNPC*>(entities_by_eid.Get(eid, NULL));
}

gemItem* GEMSupervisor::FindItemEntity(uint32 item_id)
{
    return items_by_uid.Get(item_id, NULL);
}

int GEMSupervisor::CountManagedNPCs(AccountID superclientID)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    int count=0;
    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        if(obj->GetSuperclientID() == superclientID)
        {
            count++;
        }
    }
    return count;
}

void GEMSupervisor::FillNPCList(MsgEntry* msg, AccountID superclientID)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        if(obj->GetSuperclientID() == superclientID)
        {
            msg->Add(obj->GetPID().Unbox());
            msg->Add(obj->GetEID().Unbox());
        }
    }
}

void GEMSupervisor::SendAllNPCStats(AccountID superclientID)
{
    csHash<gemActor*, PID>::GlobalIterator iter(actors_by_pid.GetIterator());

    while(iter.HasNext())
    {
        gemActor* actor = iter.Next();

        psserver->npcmanager->QueueStatDR(actor, DIRTY_VITAL_ALL);
    }
}

void GEMSupervisor::ActivateNPCs(AccountID superclientID)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        if(obj->GetSuperclientID() == superclientID)
        {
            // Turn off any npcs about to be managed from being temporarily impervious
            obj->GetCharacterData()->SetImperviousToAttack(obj->GetCharacterData()->GetImperviousToAttack() & ~TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later
        }
    }
}

void GEMSupervisor::StopAllNPCs(AccountID superclientID)
{
    CPrintf(CON_NOTIFY, "Shutting down entities managed by superclient %s.\n", ShowID(superclientID));

    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        if(obj->GetSuperclientID() == superclientID && obj->IsAlive())
        {
            // CPrintf(CON_DEBUG, "  Deactivating %s...\n",obj->GetName() );
            gemActor* actor = obj->GetActorPtr();
            actor->pcmove->SetVelocity(csVector3(0,0,0));
            actor->pcmove->SetAngularVelocity(csVector3(0,0,0));
            actor->pcmove->SetOnGround(true);

            csTicks timeDelay=0;
            actor->SetAction("idle",timeDelay);
            actor->SetMode(PSCHARACTER_MODE_PEACE);
            actor->GetCharacterData()->SetImperviousToAttack(actor->GetCharacterData()->GetImperviousToAttack() | TEMPORARILY_IMPERVIOUS);  // may switch this to 'hide' later

            // actor->MoveToValidPos();  // Khaki added this a year ago but it causes pathed npcs to teleport on npcclient exit.

            actor->MulticastDRUpdate();
        }
    }
}

void GEMSupervisor::UpdateAllDR()
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        if(obj->GetPID().IsValid())
        {
            obj->UpdateDR();
        }
    }
}

void GEMSupervisor::UpdateAllStats()
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        gemActor* actor = dynamic_cast<gemActor*>(obj);
        if(actor)
        {
            actor->UpdateStats();
        }
    }
}

void GEMSupervisor::GetPlayerObjects(PID playerID, csArray<gemObject*> &list)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());
    while(iter.HasNext())
    {
        gemObject* obj = iter.Next();
        gemItem* item = dynamic_cast<gemItem*>(obj);
        if(item && item->GetItem()->GetOwningCharacterID() == playerID)
        {
            list.Push(item);
        }
    }
}


void GEMSupervisor::GetAllEntityPos(csArray<psAllEntityPosMessage> &update)
{
    csHash<gemObject*, EID>::GlobalIterator iter(entities_by_eid.GetIterator());

    csTicks now = csGetTicks();

    // Loop as long as we need to send more messages.
    while(iter.HasNext())
    {
        psAllEntityPosMessage msg;
        msg.SetLength(ALLENTITYPOS_MAX_AMOUNT,0); // Set a message length limit

        // Fill the message
        int count_actual = 0;
        while(iter.HasNext())
        {
            gemObject* obj = iter.Next();
            if(obj->GetPID().IsValid())
            {
                gemActor* actor = dynamic_cast<gemActor*>(obj);
                // FIXME: Now only distribute players, this should
                //        be modify to send all actors exepct
                //        NPCs controlled by the receving superclient.
                if(actor && actor->GetClient())
                {
                    csVector3 pos,pos2;
                    float yrot;
                    InstanceID instance,oldInstance;
                    iSector* sector;
                    csTicks last;
                    obj->GetPosition(pos,yrot,sector);
                    instance = obj->GetInstance();
                    obj->GetLastSuperclientPos(pos2,oldInstance,last);

                    float dist2 = (pos.x - pos2.x) * (pos.x - pos2.x) +
                                  (pos.y - pos2.y) * (pos.y - pos2.y) +
                                  (pos.z - pos2.z) * (pos.z - pos2.z);

                    csTicks time = now - last;

                    // We need to filter some to prevent overloading the network
                    if((dist2 > 1.0) || (dist2 > .04 && time > 2000) || (instance != oldInstance))
                    {
                        count_actual++;
                        msg.Add(obj->GetEID(), pos, sector, obj->GetInstance(),
                                cacheManager->GetMsgStrings());
                        obj->SetLastSuperclientPos(pos,instance,now);
                    }
                }
            }
            if(count_actual == ALLENTITYPOS_MAX_AMOUNT) //we reached message limit so we break out of here
                break;
        }
        msg.msg->ClipToCurrentSize();  // Actual Data size
        msg.msg->Reset();
        // Now correct the first value, which is the count of following entities
        // SetLength has allready allocated the int16_t for this.
        msg.msg->Add((int16_t)count_actual);
        update.Push(msg);
    }
}

void GEMSupervisor::AttachObject(iObject* object, gemObject* gobject)
{
    csRef<psGemServerMeshAttach> attacher;
    attacher.AttachNew(new psGemServerMeshAttach(gobject));
    attacher->SetName(object->GetName());
    csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));

    object->ObjAdd(attacher_obj);
}

void GEMSupervisor::UnattachObject(iObject* object, gemObject* gobject)
{
    csRef<psGemServerMeshAttach> attacher(CS::GetChildObject<psGemServerMeshAttach>(object));
    if(attacher)
    {
        if(attacher->GetObject() == gobject)
        {
            csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));
            object->ObjRemove(attacher_obj);
        }
    }
}

gemObject* GEMSupervisor::FindAttachedObject(iObject* object)
{
    gemObject* found = 0;

    csRef<psGemServerMeshAttach> attacher(CS::GetChildObject<psGemServerMeshAttach>(object));
    if(attacher)
    {
        found = attacher->GetObject();
    }

    return found;
}

csArray<gemObject*> GEMSupervisor::FindNearbyEntities(iSector* sector, const csVector3 &pos, InstanceID instance, float radius, bool doInvisible)
{
    csArray<gemObject*> list;

    csRef<iMeshWrapperIterator> obj_it =  engine->GetNearbyMeshes(sector, pos, radius);
    while(obj_it->HasNext())
    {
        iMeshWrapper* m = obj_it->Next();
        if(!doInvisible)
        {
            bool invisible = m->GetFlags().Check(CS_ENTITY_INVISIBLE);
            if(invisible)
                continue;
        }

        gemObject* object = FindAttachedObject(m->QueryObject());

        if(object && (object->GetInstance() == instance ||
           object->GetInstance() == INSTANCE_ALL || instance == INSTANCE_ALL))
        {
            list.Push(object);
        }
    }

    return list;
}

csArray<gemObject*> GEMSupervisor::FindSectorEntities(iSector* sector, bool doInvisible)
{
    csArray<gemObject*> list;

    csRef<iMeshList> obj_it =  sector->GetMeshes();
    for(int i = 0; i < obj_it->GetCount(); i++)
    {
        iMeshWrapper* m = obj_it->Get(i);
        if(!doInvisible)
        {
            bool invisible = m->GetFlags().Check(CS_ENTITY_INVISIBLE);
            if(invisible)
                continue;
        }

        gemObject* object = FindAttachedObject(m->QueryObject());

        if(object)
        {
            list.Push(object);
        }
    }

    return list;
}

/*****************************************************************/

csString GetDefaultBehavior(const csString &dfltBehaviors, int behaviorNum)
{
    csStringArray behaviors;
    psString dflt(dfltBehaviors);

    dflt.Split(behaviors);
    if(behaviorNum < (int)behaviors.GetSize())
        return behaviors[behaviorNum];
    else
        return "";
}

gemObject::gemObject(GEMSupervisor* gemsupervisor,
                     EntityManager* entitymanager,
                     CacheManager* cachemanager,
                     const char* name,
                     const char* factname,
                     InstanceID myInstance,
                     iSector* room,
                     const csVector3 &pos,
                     float rotangle,
                     int clientnum) : factname(factname)
{
    cel = gemsupervisor;
    entityManager = entitymanager;
    cacheManager = cachemanager;

    valid    = true;
    this->name     = name;
    worldInstance = myInstance;

    proxlist = NULL;
    is_alive = false;
    alwaysWatching = false;

    eid = cel->CreateEntity(this);

    prox_distance_desired=DEF_PROX_DIST;
    prox_distance_current=DEF_PROX_DIST;

    InitMesh(name,pos,rotangle,room);

    if(!InitProximityList(DEF_PROX_DIST,clientnum))
    {
        Error1("Could not create gemObject because prox list could not be Init'd.");
        return;
    }
}

gemObject::~gemObject()
{
    // Assert on any dublicate delete
    assert(valid);
    valid = false;

    Disconnect();
    cel->RemoveEntity(this);
    delete proxlist;
    proxlist = NULL;
    delete pcmesh;
    pcmesh = NULL;
}


gemItem* gemObject::GetItemPtr()
{
    return dynamic_cast<gemItem*>(this);
}

psItem* gemObject::GetItem()
{
    gemItem* itemPtr = GetItemPtr();
    if(itemPtr)
        return itemPtr->GetItemData();

    return NULL;
}

gemActor* gemObject::GetActorPtr()
{
    return dynamic_cast<gemActor*>(this);
}

gemNPC* gemObject::GetNPCPtr()
{
    return dynamic_cast<gemNPC*>(this);
}

gemPet* gemObject::GetPetPtr()
{
    return dynamic_cast<gemPet*>(this);
}

gemActionLocation* gemObject::GetALPtr()
{
    return dynamic_cast<gemActionLocation*>(this);
}

const char* gemObject::GetName()
{
    return name;
}

void gemObject::SetName(const char* n)
{
    name = n;
}

double gemObject::GetProperty(MathEnvironment* env, const char* prop)
{
    //csString property(prop);
    //if (prop == "mesh")
    //{
    //}
    CS_ASSERT(false);
    return 0.0;
}

double gemObject::CalcFunction(MathEnvironment* env, const char* f, const double* params)
{
    CS_ASSERT(false);
    return 0.0;
}

void gemObject::Disconnect()
{
    while(receivers.GetSize() > 0)
    {
        iDeleteObjectCallback* receiver = receivers.Pop();
        receiver->DeleteObjectCallback(this);
    }
}


void gemObject::Broadcast(int clientnum, bool control)
{
    CPrintf(CON_DEBUG, "Base Object Broadcast!\n");
}

void gemObject::SetAlive(bool flag, bool queue)
{
    bool changed = is_alive != flag;

    is_alive = flag;
    if(!flag)
    {
        GetCharacterData()->GetHPRate().SetBase(0);  // This keeps dead guys from regen'ing HP
        GetCharacterData()->GetManaRate().SetBase(0);  // This keeps dead guys from regen'ing Mana
        BroadcastTargetStatDR(psserver->GetNetManager()->GetConnections());
    }

    gemActor* actor = GetActorPtr();

    if(queue && changed && actor)
    {
        psserver->GetNPCManager()->QueueFlagPerception(actor);
    }
}


void gemObject::InitMesh(const char* name,
                         const csVector3 &pos,
                         const float rotangle,
                         iSector* room)
{
    csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
    nullfact = engine->FindMeshFactory("nullmesh");
    if(!nullfact)
    {
        nullfact = engine->CreateMeshFactory("crystalspace.mesh.object.null", "nullmesh", false);
        csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (nullfact->GetMeshObjectFactory());
        csBox3 bbox;
        bbox.AddBoundingVertex(csVector3(0.0f));
        nullstate->SetBoundingBox(bbox);
    }

    csRef<iMeshWrapper> mesh = engine->CreateMeshWrapper(nullfact, name);

    pcmesh = new gemMesh(psserver->GetObjectReg(), this, cel);
    pcmesh->SetMesh(mesh);

    Move(pos,rotangle,room);
}

iMeshWrapper* gemObject::GetMeshWrapper()
{
    return pcmesh->GetMesh();
}

void gemObject::Move(const csVector3 &pos,float rotangle, iSector* room)
{
    pcmesh->MoveMesh(room, rotangle, pos);
}

bool gemObject::IsNear(gemObject* obj, float radius, bool ignoreY)
{
    return RangeTo(obj, ignoreY) < radius;
}

float gemObject::RangeTo(gemObject* obj, bool ignoreY, bool ignoreInstance)
{
    // Ugly hack : if an AL got (0,0,0) as a position, bypass the check
    if((GetALPtr() && GetPosition() == csVector3(0,0,0)) || (obj->GetALPtr() && obj->GetPosition() == csVector3(0,0,0)))
        return 0.0f;

    return proxlist->RangeTo(obj, ignoreY, ignoreInstance);
}

bool gemObject::IsUpdateReq(csVector3 const &pos,csVector3 const &oldPos)
{
    return (pos-oldPos).SquaredNorm() >= DEF_UPDATE_DIST*DEF_UPDATE_DIST;
}

bool gemObject::InitProximityList(float radius,int clientnum)
{
    proxlist = new ProximityList(cel->object_reg,this, entityManager);

    proxlist->Initialize(clientnum,this); // store these for fast access later

    // A client should always subscribe to itself
    if(clientnum)
    {
        proxlist->StartMutualWatching(clientnum,this,0.0);
    }
    return true;
}

void gemObject::UpdateProxList(bool force)
{
#ifdef PSPROXDEBUG
    psString log;
#endif

    if(!force && !proxlist->CheckUpdateRequired())   // This allows updates only if moved some way away
        return;

#ifdef PSPROXDEBUG
    log.AppendFmt("Generating proxlist for %s\n", GetName());
    //proxlist->DebugDumpContents();
#endif

    // Find nearby entities
    const csVector3 &pos = GetPosition();
    iSector* sector = GetSector();

    csTicks time = csGetTicks();

    csArray<gemObject*> nearlist = cel->FindNearbyEntities(sector,pos,GetInstance(),prox_distance_current, true);

    //CPrintf(CON_SPAM, "\nUpdating proxlist for %s\n--------------------------\n",GetName());

    // Cycle through list and add any entities
    // that represent players to the proximity subscription list.
    size_t count = nearlist.GetSize();
    size_t player_count = 0;

    proxlist->ClearTouched();
    for(size_t i=0; i<count; i++)
    {
        gemObject* nearobj = nearlist[i];
        if(!nearobj)
            continue;

        // Most npcs and objects don't watch each other.
        if(!GetClientID() && !alwaysWatching && !nearobj->GetClientID() && !nearobj->AlwaysWatching())
            continue;

        float range = proxlist->RangeTo(nearobj);
#ifdef PSPROXDEBUG
        log.AppendFmt("%s is %1.2fm away from %s\n",GetName(),range,nearobj->GetName());
#endif

        if(SeesObject(nearobj, range))
        {
#ifdef PSPROXDEBUG
            log.AppendFmt(" and is seen.\n-%s (client %i) can see %s\n",GetName(),GetClientID(),nearobj->GetName());
#endif
            if(proxlist->StartWatching(nearobj, range) || force)
            {
#ifdef PSPROXDEBUG
                log.AppendFmt("-%s (client %i) started watching %s\n",GetName(),GetClientID(),nearobj->GetName());
#endif
                if(GetClientID()!=0)
                {
#ifdef PSPROXDEBUG
                    log.AppendFmt("-%s sent to client %s\n",nearobj->GetName(),GetName());
#endif
                    nearobj->Send(GetClientID(),false,false);
                    player_count++;
                }
            }
        }
        else
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s can not see %s\n",GetName(), nearobj->GetName());
#endif
        }

        if((!nearobj->GetClient() || nearobj->GetClient()->IsReady()) &&
           nearobj->SeesObject(this, range))
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s can see %s\n",nearobj->GetName(),GetName());
#endif
            // true means new addition, false means already on list
            if(nearobj->GetProxList()->StartWatching(this, range) ||
               (force &&
                // Don't send info a second time, loop above handles
                // the case when object sees itself and 'force' is true.
                nearobj != this))
            {
#ifdef PSPROXDEBUG
                log.AppendFmt("-%s started watching %s\n",nearobj->GetName(),GetName());
#endif
                if(nearobj->GetClientID()!=0)
                {
#ifdef PSPROXDEBUG
                    log.AppendFmt("Big send -%s sent to client %s\n",GetName(),nearobj->GetName());
#endif
                    Send(nearobj->GetClientID(),false,false);
                }
            }
        }
        else
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-%s can not see %s\n", nearobj->GetName(),GetName());
#endif
        }
    }

    // See how our tally did, and step down on the next update if it's too big.
    if(player_count > PROX_LIST_SHRINK_THRESHOLD)
    {
        prox_distance_current-=PROX_LIST_STEP_SIZE;
        if(prox_distance_current<2)
        {
            prox_distance_current=2;
        }
    }
    /* Step up if we're small enough
     *
     *  FIXME:  There's a chance that we can bounce back and forth between various step sizes if
     *           (PROX_LIST_SHRINK_THRESHOLD - PROX_LIST_REGROW_THRESHOLD) players happen to be in
     *          the ring between the radius defined by the two steps.  This probably isn't harmful -
     *          as long as SHRINK, REGROW and STEP are defined with some sanity it should be a rare
     *          occurance.
     */
    if(player_count < PROX_LIST_REGROW_THRESHOLD && prox_distance_current < prox_distance_desired)
    {
        prox_distance_current+=PROX_LIST_STEP_SIZE;
        if(prox_distance_current>prox_distance_desired)
        {
            prox_distance_current=prox_distance_desired;
        }
    }

    // Now remove those that should be no more connected to out object

    gemObject* obj;

    size_t debug_count = 0;
    if(GetClientID() != 0)
    {
        while(proxlist->GetUntouched_ObjectThatIWatch(obj))
        {
            debug_count++;
#ifdef PSPROXDEBUG
            log.AppendFmt("-removing %s from client %s\n",obj->GetName(),GetName());
#endif
            CS_ASSERT(obj != this);

            //               csVector3 pos; float yrot; iSector *sector;
            //               obj->GetPosition(pos,yrot,sector);
            //                CPrintf(CON_SPAM, "Removing %s at (%1.2f, %1.2f, %1.2f) in sector %s.\n",obj->GetName(),pos.x,pos.y,pos.z,sector->QueryObject()->GetName() );

            psRemoveObject remove(GetClientID(), obj->GetEID());
            remove.SendMessage();
            proxlist->EndWatching(obj);
        }
    }

    if(csGetTicks() - time > 500 || player_count > 100)
    {
        csString status;
        status.Format("Warning: Spent %u time getting untouched objects in proxlist for %s, %zu nearby entities, distance %g, location: %g %g %g %s!",
                      csGetTicks() - time, GetName(), player_count, prox_distance_current, pos.x, pos.y, pos.z, (const char*)sector->QueryObject()->GetName());
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

    while(proxlist->GetUntouched_ObjectThatWatchesMe(obj))
    {
        if(obj->GetClientID() != 0)
        {
#ifdef PSPROXDEBUG
            log.AppendFmt("-removing %s from client %s\n",GetName(),obj->GetName());
#endif
            CS_ASSERT(obj != this);
            psRemoveObject msg(obj->GetClientID(), eid);
            msg.SendMessage();
            obj->GetProxList()->EndWatching(this);
        }
    }

    if(csGetTicks() - time > 500 || player_count > 100)
    {
        csString status;
        status.Format("Warning: Spent %u time touching entities in proxlist for %s,"
                      " counted %zu nearby entities, %zu untouched objects that it watches, distance %g!",
                      csGetTicks() - time, GetName(), player_count, debug_count, prox_distance_current);
        psserver->GetLogCSV()->Write(CSV_STATUS, status);
    }

#ifdef PSPROXDEBUG
    CPrintf(CON_DEBUG, "%s\n", log.GetData());    //use CPrintf because Error1() is breaking lines
#endif
}


csArray<PublishDestination> &gemObject::GetMulticastClients()
{
    return proxlist->GetClients();
}

uint32_t  gemObject::GetClientID()
{
    return proxlist->GetClientID();
}

csArray< gemObject* >* gemObject::GetObjectsInRange(float range)
{
    csArray< gemObject* >* objectsInRange = new csArray< gemObject* >();

    // Find nearby entities
    csVector3 pos;
    iSector* sector;
    GetPosition(pos,sector);
    csArray<gemObject*> nearlist = cel->FindNearbyEntities(sector, pos, GetInstance(), range);

    size_t count = nearlist.GetSize();

    for(size_t i=0; i<count; i++)
    {
        gemObject* nearobj = nearlist[i];
        if(!nearobj)
            continue;

        objectsInRange->Push(nearobj);

        /*
        float range = proxlist->RangeTo(nearobj);
        CPrintf(CON_DEBUG, "%s is %1.2fm away from %s",GetName(),range,nearobj->GetName() );

        if (SeesObject(nearobj, range))
        {
            CPrintf(CON_DEBUG, " and is seen.\n");
        }
        else
        {
            CPrintf(CON_DEBUG, " and is not seen.\n");
        }
        */
    }

    return objectsInRange;
}

void gemObject::GetPosition(csVector3 &pos, float &yrot,iSector* &sector)
{
    yrot = GetAngle();
    GetPosition(pos, sector);
}

const csVector3 &gemObject::GetPosition()
{
    return GetMeshWrapper()->GetMovable()->GetPosition();
}

void gemObject::GetPosition(csVector3 &pos, iSector* &sector)
{
    pos = GetPosition();
    sector = GetSector();
}

float gemObject::GetAngle()
{
    csMatrix3 transf = GetMeshWrapper()->GetMovable()->GetTransform().GetT2O();
    return psWorld::Matrix2YRot(transf);
}

iSector* gemObject::GetSector()
{
    if(GetMeshWrapper()->GetMovable()->GetSectors()->GetCount())
        return GetMeshWrapper()->GetMovable()->GetSectors()->Get(0);
    return NULL;
}

float gemObject::GetVelocity()
{
    return 0.0; // Objects can't move
}


void gemObject::SendBehaviorMessage(const csString &str, gemObject* actor)
{
    Error3("gemObject %s got behavior message %s in error.",GetName(),str.GetData());
}

csString gemObject::GetDefaultBehavior(const csString &dfltBehaviors)
{
    Error3("gemObject %s got default behavior message %s in error.",GetName(),dfltBehaviors.GetData());
    return "";
}

void gemObject::Dump()
{
    csString out;
    CPrintf(CON_CMDOUTPUT ,"ProxList:\n");
    CPrintf(CON_CMDOUTPUT ,"Distance: %.2f <= Desired: %.2f \n",
            prox_distance_current,prox_distance_desired);
    GetProxList()->DebugDumpContents(out);
    CPrintf(CON_CMDOUTPUT, out.GetData());
}

//--------------------------------------------------------------------------------------
// gemActiveObject
//--------------------------------------------------------------------------------------

gemActiveObject::gemActiveObject(GEMSupervisor* gemSupervisor, EntityManager* entitymanager, CacheManager* cachemanager, const char* name,
                                 const char* factname,
                                 InstanceID myInstance,
                                 iSector* room,
                                 const csVector3 &pos,
                                 float rotangle,
                                 int clientnum)
    : gemObject(gemSupervisor,entitymanager,cachemanager,name,factname,myInstance,room,pos,rotangle,clientnum)
{
    //if entity is not set, object is not a success
//    if (entity != NULL)
//    {
//        CPrintf(CON_DEBUG, "New object: %s at %1.2f,%1.2f,%1.2f, sector %s.\n",
//            factname,pos.x,pos.y,pos.z,    sector->QueryObject()->GetName());
//    }
//    else CPrintf(CON_DEBUG, "Object %s was not created.\n", factname);

}

void gemActiveObject::Broadcast(int clientnum, bool control)
{
    CPrintf(CON_DEBUG, "Active Object Broadcast");
}

csString gemActiveObject::GetDefaultBehavior(const csString &dfltBehaviors)
{
    return ::GetDefaultBehavior(dfltBehaviors, 0);
}

void gemActiveObject::SendBehaviorMessage(const csString &msg_id, gemObject* actor)
{
    unsigned int clientnum = actor->GetClientID();

    if(msg_id == "select")
    {
        // If the player is in range of the item.
        if(RangeTo(actor) < RANGE_TO_SELECT)
        {
            int options = 0;

            psGUIInteractMessage guimsg(clientnum,options);
            guimsg.SendMessage();
        }

        return;
    }
    else if(msg_id == "context")
    {
        // If the player is in range of the item.
        if(RangeTo(actor) < RANGE_TO_SELECT && actor->IsAlive())
        {
            int options = psGUIInteractMessage::EXAMINE | psGUIInteractMessage::USE;

            if(IsConstructible()) options |= psGUIInteractMessage::CONSTRUCT;
            if(IsContainer()) options |= psGUIInteractMessage::COMBINE;
            if(IsPickupable())  options |= psGUIInteractMessage::PICKUP;
            if(IsLockable())
            {
                if(IsLocked())
                    options |= psGUIInteractMessage::UNLOCK;
                else
                    options |= psGUIInteractMessage::LOCK;
            }

            // Check if there are something to send
            if(!options)
                return;

            // Always possible to close menu
            options |= psGUIInteractMessage::CLOSE;

            psGUIInteractMessage guimsg(clientnum,options);
            guimsg.SendMessage();
        }
        return;
    }
    else if(msg_id == "pickup")
    {
        // Check if the char is dead
        if(!actor->IsAlive())
        {
            psserver->SendSystemInfo(clientnum,"You can't pick up items when you're dead.");
            return;
        }
        // Check if the item is pickupable
        if(!IsPickupable())
        {
            psserver->SendSystemInfo(clientnum,"You can't pick up %s.", GetName());
            return;
        }
        CS_ASSERT(false);
    }

    else if(msg_id == "use")
        psserver->GetWorkManager()->HandleUse(actor->GetClient());
    else if(msg_id == "combine")
        psserver->GetWorkManager()->HandleCombine(actor->GetClient());
    else if(msg_id == "unlock")
        ; //???????
    else if(msg_id == "examine")
        psserver->GetCharManager()->ViewItem(actor->GetClient(), actor->GetEID().Unbox(), PSCHARACTER_SLOT_NONE);

    return;
}

//--------------------------------------------------------------------------------------
// gemItem
//--------------------------------------------------------------------------------------

gemItem::gemItem(GEMSupervisor* gemsupervisor, CacheManager* cachemanager, EntityManager* entitymanager, psItem* item,
                 const char* factname,
                 InstanceID instance,
                 iSector* room,
                 const csVector3 &pos,
                 float xrotangle,
                 float yrotangle,
                 float zrotangle,
                 int clientnum)
    : gemActiveObject(gemsupervisor, entitymanager, cachemanager, item->GetName(),factname,instance,room,pos,yrotangle,clientnum)
{
    itemdata=item;
    matname=item->GetTextureName();
    xRot=xrotangle;
    yRot=yrotangle;
    zRot=zrotangle;
    itemType.Format("Item(%s)",itemdata->GetItemType());
    itemdata->SetGemObject(this);
    cel->AddItemEntity(this);
    tribeID = 0;
}

double gemItem::GetProperty(MathEnvironment* env, const char* prop)
{
    CS_ASSERT(itemdata);
    return itemdata->GetProperty(env, prop);
}

double gemItem::CalcFunction(MathEnvironment* env, const char* f, const double* params)
{
    CS_ASSERT(itemdata);
    return itemdata->CalcFunction(env, f, params);
}


void gemItem::Broadcast(int clientnum, bool control)
{
    int flags = 0;
    if(!IsPickupable()) flags |= psPersistItem::NOPICKUP;
    if(IsUsingCD() || GetItem()->GetSector()->GetIsColliding()) flags |= psPersistItem::COLLIDE;

    psPersistItem mesg(clientnum,
                       eid,
                       -2,
                       name,
                       factname,
                       matname,
                       GetSectorName(),
                       GetPosition(),
                       xRot,
                       yRot,
                       zRot,
                       flags,
                       cacheManager->GetMsgStrings(),
                       0  // Don't send tribeID to clients
                      );

    mesg.Multicast(GetMulticastClients(),clientnum,PROX_LIST_ANY_RANGE);
}

void gemItem::SetPosition(const csVector3 &pos,float angle, iSector* sector, InstanceID instance)
{
    Move(pos, angle, sector);
    SetInstance(instance);

    psSectorInfo* sectorInfo = NULL;
    if(sector != NULL)
        sectorInfo = cacheManager->GetSectorInfoByName(sector->QueryObject()->GetName());
    if(sectorInfo != NULL)
    {
        itemdata->SetLocationInWorld(instance, sectorInfo, pos.x, pos.y, pos.z, angle);
        itemdata->Save(false);
    }

    UpdateProxList(true);
}

void gemItem::SetRotation(float xrotangle, float yrotangle, float zrotangle)
{
    this->xRot = xrotangle;
    this->yRot = yrotangle;
    this->zRot = zrotangle;

    itemdata->SetRotationInWorld(xrotangle,yrotangle,zrotangle);
    itemdata->Save(false);
}

void gemItem::GetRotation(float &xrotangle, float &yrotangle, float &zrotangle)
{
    xrotangle = this->xRot;
    yrotangle = this->yRot;
    zrotangle = this->zRot;
}

float gemItem::GetBaseAdvertiseRange()
{
    if(itemdata==NULL)
    {
        return gemObject::GetBaseAdvertiseRange();
    }
    return itemdata->GetVisibleDistance();
}

bool gemItem::Send(int clientnum, bool , bool to_superclients, psPersistAllEntities* allEntities)
{
    int flags = 0;
    if(!IsPickupable()) flags |= psPersistItem::NOPICKUP;
    if(IsUsingCD() || GetItem()->GetSector()->GetIsColliding()) flags |= psPersistItem::COLLIDE;

    psPersistItem mesg(clientnum,
                       eid,
                       -2,
                       name,
                       factname,
                       matname,
                       GetSectorName(),
                       GetPosition(),
                       xRot,
                       yRot,
                       zRot,
                       flags,
                       cacheManager->GetMsgStrings(),
                       (to_superclients || allEntities)?GetTribeID():0, // Don't send tribe ids to clients
                       (to_superclients || allEntities)?GetItem()->GetUID():0);

    if(clientnum && !to_superclients)
    {
        mesg.SendMessage();
    }

    if(to_superclients)
    {
        Debug1(LOG_SUPERCLIENT, clientnum, "Sending gemItem to superclients.\n");

        if(clientnum == 0)  // Send to all superclients
        {
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }

    if(allEntities)
    {
        return allEntities->AddEntityMessage(mesg.msg);
    }

    return true;
}

//Here we check the flag to see if we can pick up this item
bool gemItem::IsPickupable()
{
    return !itemdata->GetIsNoPickup();
}

bool gemItem::IsPickupableStrong()
{
    return !itemdata->GetIsNoPickupStrong();
}

bool gemItem::IsPickupableWeak()
{
    return !itemdata->GetIsNoPickupWeak();
}

bool gemItem::IsLockable()
{
    return itemdata->GetIsLockable();
}

bool gemItem::IsLocked()
{
    return itemdata->GetIsLocked();
}

bool gemItem::IsConstructible()
{
    return itemdata->GetIsConstructible();
}

bool gemItem::IsUsingCD()
{
    return itemdata->GetIsCD();
}

bool gemItem::IsSecurityLocked()
{
    return itemdata->GetIsSecurityLocked();
}

bool gemItem::IsContainer()
{
    return itemdata->GetIsContainer();
}

bool gemItem::GetCanTransform()
{
    return itemdata->GetCanTransform();
}

bool gemItem::GetVisibility()
{
    /* This function is called after itemdata might be deleted so never
       include use of itemdata in this function. */
    return true;
}

void gemItem::SendBehaviorMessage(const csString &msg_id, gemObject* actor)
{
    unsigned int clientnum = actor->GetClientID();
    if(msg_id == "pickup")
    {
        // Check if the char is dead
        if(!actor->IsAlive())
        {
            psserver->SendSystemInfo(clientnum,"You can't pick up items when you're dead.");
            return;
        }
        // Check if the item is pickupable
        if(!IsPickupable())
        {
            psserver->SendSystemInfo(clientnum,"You can't pick up %s.", GetName());
            return;
        }

        // Check if the item is in range
        if(!(RangeTo(actor) < RANGE_TO_SELECT))
        {
            if(!psserver->CheckAccess(actor->GetClient(), "pickup override", false))
            {
                psserver->SendSystemInfo(clientnum,"You're too far away to pick up %s.", GetName());
                return;
            }
        }

        psItem* item = GetItemData();

        // Check if item is guarded
        if(!psserver->CheckAccess(actor->GetClient(), "pickup override", false))
        {
            PID guardCharacterID = item->GetGuardingCharacterID();
            gemActor* guardActor = cel->FindPlayerEntity(guardCharacterID);
            if(guardCharacterID.IsValid() &&
                    guardCharacterID != actor->GetCharacterData()->GetPID() &&
                    guardActor &&
                    guardActor->InsideGuardedArea(item->GetGemObject()) &&
                    (guardActor->GetInstance() == this->GetInstance()))
            {
                psserver->SendSystemInfo(clientnum,"You notice that the item is being guarded by %s",
                                         guardActor->GetCharacterData()->GetCharFullName());
                return;
            }
        }

        // Cache values from item, because item might be deleted by Add
        csString qname = item->GetQuantityName();
        gemContainer* container = dynamic_cast<gemContainer*>(this);

        uint32 origUID = item->GetUID();
        csString itemName = item->GetName();
        unsigned short origStackCount = item->GetStackCount();

        gemActor* gActor = actor->GetActorPtr();
        psCharacter* chardata = NULL;
        if(gActor) chardata = gActor->GetCharacterData();

        if(chardata && chardata->Inventory().Add(item, false, true, PSCHARACTER_SLOT_NONE, container))  // Test = false, Stack = true
        {
            // We passed the item to the character's inventory so don't
            // dereference the pointer or delete it.
            itemdata = NULL;
            if(container)
            {
                container->CleareWithoutDelete();
            }


            Client* client = actor->GetClient();
            if(!client)
            {
                Debug2(LOG_ANY,clientnum,"User action from unknown client!  Clientnum:%d\n",clientnum);
                return;
            }
            client->SetTargetObject(NULL);
            // item is deleted if we pickup money
            if(item)
            {
                item->ScheduleRespawn();
                psPickupEvent evt(
                    chardata->GetPID(),
                    chardata->GetCharName(),
                    item->GetUID(),
                    item->GetName(),
                    item->GetStackCount(),
                    (int)item->GetCurrentStats()->GetQuality(),
                    0
                );
                evt.FireEvent();
            }
            else
            {
                psPickupEvent evt(
                    chardata->GetPID(),
                    chardata->GetCharName(),
                    origUID,
                    itemName,
                    origStackCount,
                    0,
                    0
                );
                evt.FireEvent();
            }

            psSystemMessage newmsg(clientnum, MSG_INFO_BASE, "%s picked up %s", actor->GetName(), qname.GetData());
            newmsg.Multicast(actor->GetMulticastClients(),0,RANGE_TO_SELECT);

            psserver->GetCharManager()->UpdateItemViews(clientnum);

            if(GetItemData())
            {
                cel->RemoveItemEntity(this, origUID);
            }
            entityManager->RemoveActor(this);  // Destroy this
        }
        else
        {

            /* TODO: Include support for partial pickup of stacks

            // Check if part of a stack where picked up
            if (item && count > item->GetStackCount())
            {
                count = count - item->GetStackCount();
                qname = psItem::GetQuantityName(item->GetName(),count);

                psSystemMessage newmsg(client, MSG_INFO, "%s picked up %s", actor->GetName(), qname.GetData() );
                newmsg.Multicast(actor->GetMulticastClients(),0,RANGE_TO_SELECT);

                psserver->GetCharManager()->UpdateItemViews(clientnum);

                csString buf;
                buf.Format("%s, %s, %s, \"%s\", %d, %d", actor->GetName(), "World", "Pickup", GetName(), 0, 0);
                psserver->GetLogCSV()->Write(CSV_EXCHANGES, buf);

            }
            */

            // Assume inventory is full so tell player about that to.
            psserver->SendSystemInfo(clientnum, "You can't carry anymore %s",GetName());
        }
    }
    // Command for taking everything in a container.
    else if(msg_id == "takeall")
    {
        SendBehaviorMessageTakeAll(msg_id, actor, true);
    }
    else if(msg_id == "takestackall")
    {
        SendBehaviorMessageTakeAll(msg_id, actor, false);
    }
    else
    {
        gemActiveObject::SendBehaviorMessage(msg_id, actor);
    }

}

// Implement SendBehaviorMessage for takeall and takestackall
void gemItem::SendBehaviorMessageTakeAll(const csString &msg_id, gemObject* obj, bool precise)
{
    Client* fromClient = obj->GetClient();
    if(!fromClient)
    {
        printf("Command /%s can't find valid Client - something's wrong", msg_id.GetData());
    }

    //printf("Attempting to do MoveAllFromContainer requested by client number %d\n", fromClient->GetClientNum());
    psCharacter* chr = obj->GetCharacterData();
    gemActor* actor = chr->GetActor();

    // Checks the current target of the actor, trying to take all items from it.
    gemObject* targetObject = actor->GetTargetObject();

    gemContainer* worldContainer = NULL;
    psItem* currentItem = NULL;
    psItem* newItem = NULL;

    // Check the client is actually targetting something
    if(!targetObject)
    {
        psserver->SendSystemError(fromClient->GetClientNum(),
                                  "You need to target a container to %s.", msg_id.GetData());
        return;
    }

    // Check the target is valid
    if(!targetObject->IsValid())
    {
        psserver->SendSystemError(fromClient->GetClientNum(),
                                  "You need to target a container to %s.", msg_id.GetData());
        return;
    }

    worldContainer = dynamic_cast<gemContainer*>(targetObject);

    // Check the casting was successful, suggesting the target of client might just be a container.
    if(!worldContainer)
    {
        psserver->SendSystemError(fromClient->GetClientNum(),
                                  "You need to target a container to %s.", msg_id.GetData());
        return;
    }

    // Checks if the target is a container
    if(!worldContainer->IsContainer())
    {
        psserver->SendSystemError(fromClient->GetClientNum(),
                                  "Can not put item into another item %s.",worldContainer->GetItem()->GetName());
        return;
    }

    // Check client is in range
    if(fromClient->GetActor()->RangeTo(worldContainer, true, true) > RANGE_TO_USE)
    {
        psserver->SendSystemError(fromClient->GetClientNum(),
                                  "You are not in range to use %s.",worldContainer->GetItem()->GetName());
        return;
    }

    //printf("Checks done, proceeding to take all items..\n");

    // Iterate through items in container, take what can be taken.
    gemContainer::psContainerIterator i(worldContainer);
    while(i.HasNext())
    {
        currentItem = i.Next();

        // Check the client can remove the current item
        if(!worldContainer->CanTake(fromClient, currentItem))
            continue;

        // Check the current item can be added to character inventory
        if(chr->Inventory().Add(currentItem, true))
        {
            newItem = i.RemoveCurrent(fromClient);

            // Check if removing was successful
            if(!newItem)
            {
                printf("/takeall: Removing %s by %d unsuccessful", currentItem->GetName(), fromClient->GetClientNum());
                continue;
            }

            if(precise)
            {
                chr->Inventory().Add(newItem, false);
            }
            else
            {
                chr->Inventory().Add(newItem, false, true, PSCHARACTER_SLOT_NONE, 0, false);    // call Add with precise == false
            }
        }
    }

    // Update character's inventory view
    psserver->GetCharManager()->UpdateItemViews(fromClient->GetClientNum());
}

//--------------------------------------------------------------------------------------
// gemContainer
//--------------------------------------------------------------------------------------

gemContainer::gemContainer(GEMSupervisor* gemsupervisor, CacheManager* cachemanager,
                           EntityManager* entitymanager, psItem* item,
                           const char* factname,
                           InstanceID myInstance,
                           iSector* room,
                           const csVector3 &pos,
                           float xrotangle,
                           float yrotangle,
                           float zrotangle,
                           int clientnum)
    : gemItem(gemsupervisor,cachemanager,entitymanager,item,factname,myInstance,room,pos,xrotangle,yrotangle,zrotangle,clientnum)
{
}

gemContainer::~gemContainer()
{
    while(itemlist.GetSize())
    {
        delete itemlist.Pop();
    }
}

void gemContainer::CleareWithoutDelete()
{
    itemlist.Empty();
}


bool gemContainer::CanAdd(unsigned short amountToAdd, psItem* item, int slot, csString &reason)
{
    if(!item)
    {
        reason = ""; // No reason since this is an internal error
        return false;
    }

    if(slot >= SlotCount())
    {
        reason = ""; // No reason since this is an internal error
        return false;
    }

    PID guardID = GetItem()->GetGuardingCharacterID();
    gemActor* guardingActor = cel->FindPlayerEntity(guardID);

    // Test if container is guarded by someone else who is near
    if(guardID.IsValid() && (guardID != item->GetOwningCharacterID() &&
                             guardID != item->GetGuardingCharacterID())
            && guardingActor && guardingActor->InsideGuardedArea(this))
    {
        reason = " because container is guarded";
        return false;
    }

    /* We often want to see if a partial stack could be added, but we want to
     * check before actually doing the splitting.  So, we take an extra parameter
     * and fake the stack count before calling AddToContainer with the test flag;
     * we put it back when we're done. */

    float currentSize = 0;
    gemContainer::psContainerIterator iter(this);
    while(iter.HasNext())
    {
        psItem* child = iter.Next();
        currentSize += child->GetTotalStackSize();
    }
    if(item->GetItemSize()*amountToAdd + currentSize > itemdata->GetContainerMaxSize())
    {
        reason = " because the item is to large to fitt";
        return false;
    }

    unsigned short savedCount = item->GetStackCount();
    item->SetStackCount(amountToAdd);
    bool result = AddToContainer(item, NULL, slot, true);
    item->SetStackCount(savedCount);
    return result;
}

bool gemContainer::CanTake(Client* client, psItem* item)
{
    if(!client || !item)
        return false;

    // Allow developers to take anything
    if(client->GetSecurityLevel() >= GM_DEVELOPER)
        return true;

    /* \note
     * The check if the item is NPCOwned or NoPickup only makes
     * sense if it is not possible to have NPCOwned items in a not
     * NPCOwned container, or NoPickup items in a PickUpable container
     */
    //not allowed to take npcowned or nopickup items in a container
    if(item->GetIsNpcOwned() || item->GetIsNoPickup())
        return false;

    psItem* containeritem = GetItem();
    CS_ASSERT(containeritem);

    // Check for npc-owned or locked container
    if(client->GetSecurityLevel() < GM_LEVEL_2)
    {
        if(containeritem->GetIsNpcOwned())
        {
            return false;
        }
        if(containeritem->GetIsLocked())
        {
            psserver->SendSystemError(client->GetClientNum(), "You cannot take an item from a locked container!");
            return false;
        }
    }

    // Allow if the item is pickupable and either: public, guarded by the character, or the guarding character is offline
    PID guard = item->GetGuardingCharacterID();
    gemActor* guardingActor = cel->FindPlayerEntity(guard);

    if((!guard.IsValid() || guard == client->GetCharacterData()->GetPID() || !guardingActor) ||
            (!guardingActor->InsideGuardedArea(this)))
    {
        return true;
    }

    return false;
}

bool gemContainer::AddToContainer(psItem* item, Client* fromClient, int slot, bool test)
{
    // Slot changing and item creation in craft is done at the very end of event.
    // A new item is created from scratch and replaces the previous item stack and all.
    // Auto-transform containers will interupt event if item is changed.
    // Checks in craft code to make sure original item is what is expected before the item is replaced.

    // If slot number not specified put into first open slot
    if(slot == PSCHARACTER_SLOT_NONE)
    {
        for(int i = 0; i < SlotCount(); i++)
        {
            if(FindItemInSlot(i) == NULL)
            {
                slot = i;
                break;
            }
        }
    }
    if(slot < 0 || slot >= SlotCount())
        return false;

    // printf("Adding %s to GEM container %s in slot %d.\n", item->GetName(), GetName(), slot);

    // If the destination slot is occupied, try and look for an empty slot.
    if(FindItemInSlot(slot))
        return AddToContainer(item, fromClient, PSCHARACTER_SLOT_NONE, test);

    if(test)
        return true;

    // If the gemContainer we are dropping the item into is not pickupable then we
    // guard the item placed inside.  Otherwise the item becomes public.
    if(fromClient)
    {
        /* put the player behind the client as guard */
        item->SetGuardingCharacterID(fromClient->GetCharacterData()->GetPID());
    }
    if(item->GetOwningCharacterID().IsValid())
    {
        /* item comes from a player () */
        item->SetGuardingCharacterID(item->GetOwningCharacterID());
    }

    if(!GetItem()->GetIsNoPickup() && item->GetOwningCharacterID().IsValid())
        GetItem()->SetGuardingCharacterID(item->GetOwningCharacterID());

    item->SetOwningCharacter(NULL);
    item->SetContainerID(GetItem()->GetUID());
    item->SetLocInParent((INVENTORY_SLOT_NUMBER)(slot));
    item->Save(false);
    itemlist.PushSmart(item);

    item->UpdateView(fromClient, eid, false);
    return true;
}

bool gemContainer::RemoveFromContainer(psItem* item,Client* fromClient)
{
    // printf("Removing %s from container now.\n", item->GetName() );
    if(itemlist.Delete(item))
    {
        item->UpdateView(fromClient, eid, true);
        return true;
    }
    else
        return false;
}

psItem* gemContainer::RemoveFromContainer(psItem* itemStack, int fromSlot, Client* fromClient, int stackCount)
{
    // printf("Removing %s from container now.\n", itemStack->GetName() );

    psItem* item = NULL;
    bool clear = false;

    // If the stack count is the taken amount then we can delete the entire stack from the itemlist
    if(itemStack->GetStackCount() == stackCount)
    {
        item = itemStack;
        itemlist.Delete(itemStack);
        clear = true;   // Clear out the slot on the client
    }
    else
    {
        // Split out the stack for the required amount.
        item = itemStack->SplitStack(stackCount);

        // Save the lowered value
        itemStack->Save(false);
    }

    // Send out messages about the change in the item stack.
    itemStack->UpdateView(fromClient, eid, clear);
    return item;
}


psItem* gemContainer::FindItemInSlot(int slot, int stackCount)
{
    if(slot < 0 || slot >= SlotCount())
    {
        Error3("FindItemSlot Slot index %d is outside [0..%d]",slot,SlotCount());
        return NULL;
    }
    psContainerIterator it(this);
    while(it.HasNext())
    {
        // Check for specific slot
        //  These are small arrays so no need to hash index
        psItem* item = it.Next();
        if(item == NULL)
        {
            Error1("Null item in container list.");
            return NULL;
        }

        // Return the item in parrent location slot
        if(item->GetLocInParent() == slot)
        {
            // If the item is here and the stack count we want is available.
            if(stackCount == -1 || item->GetStackCount() >= stackCount)
            {
                return item;
            }
            else
            {
                return NULL;
            }
        }
    }
    return NULL;
}

gemContainer::psContainerIterator::psContainerIterator(gemContainer* containerItem)
{
    UseContainerItem(containerItem);
}

bool gemContainer::psContainerIterator::HasNext()
{
    if(current + 1 < container->CountItems())
        return true;
    return false;
}

psItem* gemContainer::psContainerIterator::Next()
{
    if(!HasNext())
        return NULL;

    current++;
    return container->GetIndexItem(current);
}

psItem* gemContainer::psContainerIterator::RemoveCurrent(Client* fromClient)
{
    if(current < container->CountItems())
    {
        psItem* item = container->GetIndexItem(current);
        container->RemoveFromContainer(item,fromClient);
        psserver->GetWorkManager()->StopWork(fromClient, item);
        current--; // This adjusts so that the next "Next()" call actually returns the next item and doesn't skip one.
        return item;
    }
    return NULL;
}

void gemContainer::psContainerIterator::UseContainerItem(gemContainer* containerItem)
{
    container=containerItem;
    current = SIZET_NOT_FOUND;
}

//--------------------------------------------------------------------------------------
// gemActionLocation
//--------------------------------------------------------------------------------------

gemActionLocation::gemActionLocation(GEMSupervisor* gemSupervisor,EntityManager* entitymanager,CacheManager* cachemanager,psActionLocation* action, iSector* isec, int clientnum)
    : gemActiveObject(gemSupervisor,entitymanager,cachemanager,action->name,
                      action->meshname,
                      action->GetInstance(),
                      isec,
                      action->position,
                      0.0f,
                      clientnum)
{
    this->action = action;
    action->SetGemObject(this);

    // action locations use the AL ID as their EID for some reason
    // ugly hack, to put the AL in the entity hash
    cel->RemoveEntity(this);
    eid = action->id;
    cel->AddEntity(this, eid);

    this->prox_distance_desired = 0.0F;
    this->prox_distance_current = 0.0F;

    visible = true;

    SetName(name);


    csRef< iEngine > engine =  csQueryRegistry<iEngine> (psserver->GetObjectReg());
    csRef< iMeshWrapper > nullmesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "pos_anchor", isec, action->position);
    if(!nullmesh)
    {
        Error1("Could not create gemActionLocation because crystalspace.mesh.onbject.null could not be created.");
        return ;
    }
    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (nullmesh->GetMeshObject());
    if(!state)
    {
        Error1("No NullMeshState.");
        return ;
    }
    state->SetRadius(1.0);

    pcmesh->SetMesh(nullmesh);

    Move(action->GetPosition(), 0, isec);
}

void gemActionLocation::Broadcast(int clientnum, bool control)
{
    psPersistActionLocation mesg(
        clientnum,
        eid,
        -2,
        name,
        GetSectorName(),
        action->meshname
    );

    mesg.Multicast(GetMulticastClients(),clientnum,PROX_LIST_ANY_RANGE);

}

float gemActionLocation::GetBaseAdvertiseRange()
{
    /*if ( action == NULL )
        return gemObject::GetBaseAdvertiseRange();
    return action->radius;*/

    // todo - check if action location has proximity item
    //     if so check itemdata->GetVisibleDistance();
    return 10000.0f;
}

bool gemActionLocation::SeesObject(gemObject* object, float range)
{
    if(worldInstance == object->GetInstance())
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool gemActionLocation::Send(int clientnum, bool , bool to_superclients, psPersistAllEntities* allEntities)
{
    psPersistActionLocation mesg(
        clientnum,
        eid,
        -2,
        name.GetData(),
        GetSectorName(),
        action->meshname
    );


    if(clientnum && !to_superclients)
    {
        mesg.SendMessage();
    }

    /* TODO: Include if superclient need action locations
    if (to_superclients)
    {
        if (clientnum == 0) // Send to all superclients
        {
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }

    if (allEntities)
        allEntities->AddEntityMessage(mesg.msg);

    */
    return true;
}

void gemActionLocation::SendBehaviorMessage(const csString &str, gemObject* actor)
{
    gemObject* item = action->GetRealItem();
    if (item)
    {
        gemActor* myactor = (gemActor*)actor;
        myactor->SetTargetObject(item);
        item->SendBehaviorMessage(str, actor);
        myactor->SetTargetObject(this);
    }
    else
        gemActiveObject::SendBehaviorMessage(str, actor);
}

//--------------------------------------------------------------------------------------
// FrozenBuffable
//--------------------------------------------------------------------------------------

void FrozenBuffable::OnChange()
{
    actor->SetAllowedToMove(!(Current() > 0));
}

//--------------------------------------------------------------------------------------
// gemActor
//--------------------------------------------------------------------------------------

gemActor::gemActor(GEMSupervisor* gemsupervisor, CacheManager* cachemanager, EntityManager* entitymanager, psCharacter* chardata,
                   const char* factname,
                   InstanceID myInstance,
                   iSector* room,
                   const csVector3 &pos,
                   float rotangle,
                   int clientnum) :
    gemObject(gemsupervisor,entitymanager,cachemanager,chardata->GetCharFullName(),factname,myInstance,room,pos,rotangle,clientnum),
    psChar(chardata), mount(NULL), DRcounter(0), forceDRcounter(0), lastDR(0), lastV(0), lastSentSuperclientPos(0, 0, 0),
    lastSentSuperclientInstance(-1), activeReports(0), isFalling(false), invincible(false), visible(true), viewAllObjects(false),
    movementMode(0), isAllowedToMove(true), atRest(true), player_mode(PSCHARACTER_MODE_PEACE), spellCasting(NULL), workEvent(NULL),
    activeMagic_seq(0), pcmove(NULL), nevertired(false), infinitemana(false), instantcast(false), safefall(false), givekillexp(false),
    attackable(false)
{
    forcedSector = NULL;
    entityManager = entitymanager;

    pid = chardata->GetPID();

    cel->AddActorEntity(this);
    isFrozen.Initialize(this);
    SetFrozen(false);

    // Store a safe reference to the client
    clientRef = psserver->GetConnections()->FindAny(clientnum);
    if(clientnum && !clientRef)
    {
        Error3("Failed to find client %d for %s!", clientnum, ShowID(pid));
        return;
    }

    SetAlive(true, false); // Set alive but don't queue

    if(!InitLinMove(pos,rotangle,room))
    {
        Error1("Could not initialize LinMove prop class, so actor not created.");
        return;
    }

    combat_stance = CombatManager::GetStance(cachemanager,"None");
    chardata->SetActor(this);

    if(!InitCharData(clientRef))
    {
        Error1("Could not init char data.  Actor not created.");
        return;
    }

    if(psChar->IsStatue())
        player_mode = PSCHARACTER_MODE_STATUE;

    Debug6(LOG_NPC,GetEID().Unbox(),"Successfully created actor %s at %1.2f,%1.2f,%1.2f in sector %s.\n",
           factname,pos.x,pos.y,pos.z,GetSectorName());

    SetPrevTeleportLocation(pos, rotangle, GetSector(), myInstance);

    // Set the initial valid location to be the spot the actor was created at.
    UpdateValidLocation(pos, rotangle, GetSector(), myInstance, true);

    GetCharacterData()->SetStaminaRegenerationStill();
}

gemActor::~gemActor()
{
    if(activeReports > 0)
    {
        LogLine(csString().Format("-- %s has quit --", GetFirstName()).GetData());
        activeReports = 1; // force RemoveChatReport() to stop logging.
        RemoveChatReport();
    }

    // Disconnect while pointers are still valid
    Disconnect();

    cel->RemoveActorEntity(this);

    while(!activeSpells.IsEmpty())
    {
        delete activeSpells.Pop();
    }

    if(psChar)
    {
        // Not deleting here anymore, but pushing to the global cache

        // delete psChar;
        // psChar = NULL;
        psChar->SetActor(NULL); // clear so cached object doesn't attempt to retain this
        cacheManager->AddToCache(psChar, cacheManager->MakeCacheName("char",psChar->GetPID().Unbox()),120);
        psChar = NULL;
    }

    delete pcmove;
}

double gemActor::GetProperty(MathEnvironment* env, const char* prop)
{
    if(!strcasecmp(prop, "stamina_drain_p"))
        return combat_stance.stamina_drain_P;
    if(!strcasecmp(prop, "stamina_drain_m"))
        return combat_stance.stamina_drain_M;
    if(!strcasecmp(prop, "attack_speed_mod"))
        return combat_stance.attack_speed_mod;
    if(!strcasecmp(prop, "attack_damage_mod"))
        return combat_stance.attack_damage_mod;
    if(!strcasecmp(prop, "defense_avoid_mod"))
        return combat_stance.defense_avoid_mod;
    if(!strcasecmp(prop, "defense_absorb_mod"))
        return combat_stance.defense_absorb_mod;
    if(!strcasecmp(prop, "combatstance"))
    {
        // Backwards compatibility.
        return combat_stance.stance_id;
    }
    if(!strcasecmp(prop, "isadvisorbanned"))
    {
        return (double)(GetClient() ? GetClient()->IsAdvisorBanned() : true);
    }
    if(!strcasecmp(prop, "advisorpoints"))
    {
        return (double)(GetClient() ? GetClient()->GetAdvisorPoints() : 0);
    }

    CS_ASSERT(psChar);
    return psChar->GetProperty(env, prop);
}

double gemActor::CalcFunction(MathEnvironment* env, const char* f, const double* params)
{
    csString func(f);
    if(func == "ActiveSpellCount")
    {
        return ActiveSpellCount(env->GetString(params[0]));
    }
    else if(func == "RangeTo")
    {
        // Ultimately this should probably go in gemObject::CalcFunction, but
        // we'd need to check both gemObject and psCharacter, and that's a bit
        // of a pain with the current interface.
        return RangeTo((gemObject*)(intptr_t)params[0]);
    }
    else if(func.StartsWith("SendSystem"))
    {
        uint32 clientID = GetClientID();
        if(!clientID)
        {
            return 0;
        }

        // MathScript specific string formatting
        size_t argc = static_cast<size_t>(params[1]);
        const double* args = argc ? &params[2] : NULL;
        csString format = env->GetString(params[0]);
        Debug2(LOG_SCRIPT, pid.Unbox(),"got format string '%s'", format.GetData());
        csString string = MathScriptEngine::FormatMessage(format,argc,args);

        if(func == "SendSystemInfo")
        {
            psserver->SendSystemInfo(clientID,string);
        }
        else if(func == "SendSystemBaseInfo")
        {
            psserver->SendSystemInfo(clientID,string);
        }
        else if(func == "SendSystemResult")
        {
            psserver->SendSystemResult(clientID,string);
        }
        else if(func == "SendSystemOK")
        {
            psserver->SendSystemOK(clientID,string);
        }
        else if(func == "SendSystemError")
        {
            psserver->SendSystemError(clientID,string);
        }
        else
        {
            return -1;
        }

        return 1;
    }
    else if(func == "InterruptSpellCasting")
    {
        InterruptSpellCasting();
        return 1;
    }
    CS_ASSERT(psChar);
    return psChar->CalcFunction(env, f, params);
}

Client* gemActor::GetClient() const
{
    return clientRef.IsValid() ? clientRef : NULL ;
}

bool gemActor::MoveToSpawnPos(int32_t delay, csString background, csVector2 point1, csVector2 point2, csString widget)
{
    csVector3 startingPos;
    float startingYrot;
    iSector* startingSector;

    //Get the spawn position for this actor, taking in account the range.
    if(!GetSpawnPos(startingPos,startingYrot,startingSector, true))
        return false;

    pcmove->SetOnGround(false);
    Teleport(startingSector, startingPos, startingYrot, DEFAULT_INSTANCE, delay, background, point1, point2, widget);

    psGenericEvent evt(GetClientID(), psGenericEvent::SPAWN_MOVE);
    evt.FireEvent();

    return true;
}

bool gemActor::GetSpawnPos(csVector3 &pos, float &yrot, iSector* &sector, bool useRange)
{
    float x, y, z, range;
    const char* sectorname;

    psRaceInfo* raceinfo = psChar->GetRaceInfo();
    if(!raceinfo)
        return false;


    raceinfo->GetStartingLocation(x,y,z,yrot,range,sectorname);

    if(!sectorname)
    {
        x = -19;
        y = -4;
        z = -144;
        yrot = 3.85f;
        range = 0;
        sectorname = "hydlaa_plaza";
    }
    //change the x and y according to the range if requested, used only for teleport by the engine vs by a gm
    if(useRange && range > 0)
    {
        x = psserver->GetRandomRange(x, range);
        z = psserver->GetRandomRange(z, range);
    }
    pos = csVector3(x, y, z);

    sector = entityManager->FindSector(sectorname);
    if(!sector)
        return false;
    else
        return true;
}

void gemActor::SetAllowedToMove(bool newvalue)
{
    if(isAllowedToMove == newvalue)
        return;

    isAllowedToMove = newvalue;

    // The server will ignore any attempts to move,
    // and we'll request the client not to send any.
    psMoveLockMessage msg(GetClientID(),!isAllowedToMove);
    msg.SendMessage();

    // @todo: Notify super clients...
}

int gemActor::GetTargetType(gemObject* target)
{
    if(!target)
    {
        return TARGET_NONE; /* No Target */
    }

    gemActor* targetActor = target->GetActorPtr();

    if(targetActor == NULL)
    {
        return TARGET_ITEM; /* Item */
    }

    if(!target->IsAlive())
    {
        return TARGET_DEAD;
    }

    Client* attackerClient = GetClient();

    if(attackerClient && attackerClient->IsGM())
    {
        // GMs can interpret targets as either friends or foe...even self.
        // This allows them to attack or cast spells on anyone.
        return TARGET_SELF | TARGET_FRIEND | TARGET_FOE;
    }

    if(this == target)
    {
        return TARGET_SELF; /* Self */
    }

    if(GetCharacterData()->IsNPC() && target->GetCharacterData()->IsNPC())
    {
        // NPCs can interpret targets as either friends or foe...even self.
        // This allows them to attack or cast spells on any other NPC.
        return TARGET_SELF | TARGET_FRIEND | TARGET_FOE;
    }


    if(target->GetCharacterData()->GetImperviousToAttack())
    {
        return TARGET_FRIEND; /* Impervious NPC */
    }

    // Is target a NPC?
    Client* targetclient = target->GetClient();
    if(!targetclient)
    {
        if(target->GetCharacterData()->IsPet())
        {
            csString msg;

            // Pet's target type depends on its owner's (enable when they can defend themselves)
            gemObject* owner = psserver->entitymanager->GetGEM()->FindPlayerEntity(target->GetCharacterData()->GetOwnerID());
            if(!owner || !IsAllowedToAttack(owner,msg))
                return TARGET_FRIEND;
        }
        return TARGET_FOE; /* Foe */
    }

    if(targetActor->GetInvincibility())
        return TARGET_FRIEND; /* Invincible GM */

    if(targetActor->attackable)
        return TARGET_FOE; /* attackable GM */

    // Only clients are in duels
    if(attackerClient && targetclient)
    {
        // Challenged to a duel?
        if(attackerClient->IsDuelClient(target->GetClientID())
                || targetclient->IsDuelClient(GetClientID()))
        {
            return TARGET_FOE; /* Attackable player */
        }
    }


    // In PvP region?
    csVector3 attackerpos, targetpos;
    float yrot;
    iSector* attackersector, *targetsector;
    GetPosition(attackerpos, yrot, attackersector);
    target->GetPosition(targetpos, yrot, targetsector);

    if(psserver->GetCombatManager()->InPVPRegion(attackerpos,attackersector)
            && psserver->GetCombatManager()->InPVPRegion(targetpos,targetsector))
    {
        return TARGET_FOE; /* Attackable player */
    }

    // Is this a player who has hit you and run out of a PVP area?
    for(size_t i=0; i< GetDamageHistoryCount(); i++)
    {
        gemActor* attacker = GetDamageHistory((int) i)->Attacker();
        // If the target has ever hit you, you can attack them back.  Logging out clears this.
        if(attacker && attacker == target)
            return TARGET_FOE;
    }

    // Declared war?
    psGuildInfo* attackguild = GetGuild();
    psGuildInfo* targetguild = target->GetGuild();
    if(attackguild && targetguild &&
            targetguild->IsGuildWarActive(attackguild))
    {
        return TARGET_FOE; /* Attackable player */
    }

    if(InGroup() && targetActor->InGroup())
    {
        csRef<PlayerGroup> AttackerGroup = GetGroup();
        csRef<PlayerGroup> TargetGroup = targetActor->GetGroup();
        if(AttackerGroup->IsInDuelWith(TargetGroup))
            return TARGET_FOE;
    }

    return TARGET_FRIEND; /* Friend */
}


bool gemActor::IsAllowedToAttack(gemObject* target, csString &msg)
{
    msg = "";

    int type = GetTargetType(target);

    if(type == TARGET_NONE)
        msg = "You must select a target to attack.";
    else if(type & TARGET_ITEM)
        msg = "You can't attack an inanimate object.";
    else if(type & TARGET_DEAD)
        msg.Format("%s is already dead.",target->GetName());
    else if(type & TARGET_FOE)
    {
        gemActor* foe = target->GetActorPtr();
        CS_ASSERT(foe != NULL); // Since this is a foe it should have a actor.

        gemActor* lastAttacker = NULL;
        if(target->HasKillStealProtection() && !foe->CanBeAttackedBy(this, lastAttacker))
        {
            if(lastAttacker)
            {
                msg.Format("You must be grouped with %s to attack %s.",
                           lastAttacker->GetName(), foe->GetName());
            }
            else
            {
                msg = "You are not allowed to attack right now.";
            }
        }
    }
    else if(type & TARGET_FRIEND)
        msg.Format("You cannot attack %s.",target->GetName());
    else if(type & TARGET_SELF)
        msg = "You cannot attack yourself.";

    if(!msg.IsEmpty())
    {
        return false;
    }
    return true;
}

void gemActor::Sit()
{
    if(GetMode() == PSCHARACTER_MODE_PEACE
            && AtRest() && !IsFalling())
    {
        SetMode(PSCHARACTER_MODE_SIT);
        psserver->GetUserManager()->Emote("%s takes a seat.", "%s takes a seat by %s.", "sit", this);
    }
    else
    {
        Debug2(LOG_SUPERCLIENT, 0, "%s Failed to sit.\n",ShowID(GetEID()));
    }

}

void gemActor::Stand()
{
    if(GetMode() == PSCHARACTER_MODE_SIT)
    {
        SetMode(PSCHARACTER_MODE_PEACE);
        psUserActionMessage anim(GetClientID(), GetEID(), "stand up");
        anim.Multicast(GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
        psserver->GetUserManager()->Emote("%s stands up.", "%s stands up.", "stand", this);
    }
    else if(GetMode() == PSCHARACTER_MODE_OVERWEIGHT)
    {
        psserver->SendSystemError(GetClientID(), "You can't stand up because you're overloaded!");
    }
}

void gemActor::SetAllowedToDisconnect(bool allowed)
{
    Client* client = GetClient();
    if(client)
    {
        client->SetAllowedToDisconnect(allowed);
    }
}



bool gemActor::MoveToValidPos(bool force)
{
    // Don't allow /unstick within 15 seconds of starting a fall...that way,
    // people can't jump off a cliff and use it to save themselves.
    if(!force && (!isFalling || (csGetTicks() - fallStartTime < UNSTICK_TIME)))
        return false;

    isFalling = false;
    pcmove->SetOnGround(false);

    Teleport(valid_location.sector, valid_location.pos, valid_location.yrot, valid_location.instance);
    return true;
}

void gemActor::GetValidPos(csVector3 &pos, float &yrot, iSector* &sector, InstanceID &instance)
{
    pos = valid_location.pos;
    yrot = valid_location.yrot;
    sector = valid_location.sector;
    instance = valid_location.instance;
}

void gemActor::SetVisibility(bool visible)
{
    this->visible = visible;
    UpdateProxList(true);
    /* Queue Visibility and Invincibility flags to the superclient.
       The superclient should than select whether to react on the actor.
    */
    psserver->GetNPCManager()->QueueFlagPerception(this);
}

void gemActor::SetInvincibility(bool invincible)
{
    this->invincible = invincible;
    /* Queue Visibility and Invincibility flags to the superclient.
       The superclient should than select whether to react on the actor.
    */
    psserver->GetNPCManager()->QueueFlagPerception(this);
}

void gemActor::SetSecurityLevel(int level)
{
    securityLevel = level;
    masqueradeLevel = level; // override any current masquerading setting
}

void gemActor::SetMasqueradeLevel(int level)
{
    masqueradeLevel = level;
    UpdateProxList(true);
    /* Queue Visibility and Invincibility flags to the superclient.
       The superclient should than select whether to react on the actor.
    */
    psserver->GetNPCManager()->QueueFlagPerception(this);
}

bool gemActor::SeesObject(gemObject* object, float range)
{
    bool res = (worldInstance == object->GetInstance() || object->GetInstance() == INSTANCE_ALL || GetInstance() == INSTANCE_ALL)
               &&
               (GetBaseAdvertiseRange() >= range)
               &&
               (object==this  ||  object->GetVisibility()  ||  GetViewAllObjects());

#ifdef PSPROXDEBUG
    psString log;
    log.AppendFmt("%s object: %s %s in Instance %d vs %s %s in %d and baseadvertise %.2f vs range %.2f\n",
                  res?"Sees":"Dosn't see", name.GetData(), GetViewAllObjects()?"sees all":"see visible",worldInstance,
                  object->GetVisibility()?"Visible":"Invisible",object->GetName(), object->GetInstance(),
                  GetBaseAdvertiseRange(),range);
    CPrintf(CON_DEBUG, "%s\n", log.GetData());    //use CPrintf because Error1() is breaking lines
#endif
    return res;
}

void gemActor::SetViewAllObjects(bool v)
{
    viewAllObjects = v;
    UpdateProxList(true);
}

void gemActor::StopMoving(bool worldVel)
{
    // stop this actor from moving
    csVector3 zeros(0.0f, 0.0f, 0.0f);
    pcmove->SetVelocity(zeros);
    pcmove->SetAngularVelocity(zeros);
    if(worldVel)
        pcmove->ClearWorldVelocity();
}

csPtr<PlayerGroup> gemActor::GetGroup()
{
    return csPtr<PlayerGroup>(group);
}

void gemActor::Defeat()
{
    // Auto-unlock after a few seconds if not explicitly killed first
    psSpareDefeatedEvent* evt = new psSpareDefeatedEvent(this);
    psserver->GetEventManager()->Push(evt);

    // Interrupt spells currently cast. This has to be done
    // before setting character mode as the interrupting may
    // set the character back to PEACE.
    InterruptSpellCasting();

    // Set the character mode to defeated.
    SetMode(PSCHARACTER_MODE_DEFEATED);

    // Cancel any active spells which are marked as doing HP damage.
    // This prevents DoT from killing the player.  We leave the rest
    // since there may be long standing debuffs like the DR curse.
    CancelActiveSpellsWhichDamage();
    if(psChar->GetHP() < 1) // make sure we won't die instantly
    {
        psChar->SetHitPoints(1);
    }
}

void gemActor::Resurrect()
{
    // Note that this does not handle NPCs...see SpawnManager::Respawn.

    Debug2(LOG_COMBAT, pid.Unbox(), "Resurrect event triggered for %s.\n", GetName());

    pcmove->SetOnGround(false);

    // Set the mode before teleporting so people in the vicinity of the destination
    // get the right mode (peace) in the psPersistActor message they receive.
    // Note that characters are still movelocked when this is reset.
    SetMode(PSCHARACTER_MODE_PEACE);

    // Check if the current player is in npcroom or tutorial. In that case we
    // teleport back to npcroom or tutorial on resurrect. Otherwise we teleport
    // to DR.
    iSector* sector = pcmove->GetSector();

    //get the sector info to check if this sector has a special death sector defined
    psSectorInfo* sectorInfo = NULL;
    if(sector)
        sectorInfo = cacheManager->GetSectorInfoByName(sector->QueryObject()->GetName());

    //if the sector was found and there is text in the sector try teleporting there.
    if(sectorInfo && sectorInfo->GetDeathSector().Length())
    {
        Teleport(sectorInfo->GetDeathSector(), sectorInfo->GetDeathCord(), sectorInfo->GetDeathRot(), DEFAULT_INSTANCE);
    }
    else
    {
        float x,y,z,yrot;
        optionEntry* deathentry = cacheManager->getOptionSafe("death","");
        if(psChar->GetTotalOnlineTime() > (unsigned int)(deathentry->getOptionSafe("avoidtime", "0")->getValueAsInt()))
        {
            csString sectorName = deathentry->getOptionSafe("sectorname", "DR01")->getValue();
            x = deathentry->getOptionSafe("sectorx", "-29.2")->getValueAsDouble();
            y = deathentry->getOptionSafe("sectory", "-119.0")->getValueAsDouble();
            z = deathentry->getOptionSafe("sectorz", "28.2")->getValueAsDouble();
            yrot = deathentry->getOptionSafe("sectoryrot", "0.0")->getValueAsDouble();
            Teleport(sectorName, csVector3(x, y, z), yrot, DEFAULT_INSTANCE);
        }
        else
        {
            csString message = deathentry->getOptionSafe("avoidtext", "")->getValue();
            MoveToSpawnPos();
            if(message.Length())
                psserver->SendSystemInfo(GetClientID(), message.GetData());
        }
    }

    //reset mana only in sectors restoring it, to prevent exploits using /die in death maps or for
    //other reasoning.
    if(!sectorInfo || sectorInfo->GetDeathRestoreMana())
        psChar->SetMana(psChar->GetMaxMana().Base());

    //reset HP only in sectors restoring it, to prevent exploits using /die in death maps or for
    //other reasoning, in the other cases set only the minimum amount to keep the player alive and not
    //make him loop die.
    if(!sectorInfo || sectorInfo->GetDeathRestoreHP())
        psChar->SetHitPoints(psChar->GetMaxHP().Base());
    else
        psChar->SetHitPoints(1);

    psChar->SetStamina(psChar->GetMaxPStamina().Base(), true);
    psChar->SetStamina(psChar->GetMaxMStamina().Base(), false);
    psChar->GetHPRate().SetBase(HP_REGEN_RATE);
    psChar->GetManaRate().SetBase(MANA_REGEN_RATE);

    // Just to force sending of all stats. Not only the one that was changed due to adjustment abow.
    psChar->SetAllStatsDirty();

    // Publish stats to all liseners
    SendGroupStats();
}

void gemActor::InvokeAttackScripts(gemActor* defender, psItem* weapon)
{
    MathEnvironment env;
    env.Define("Attacker", this);
    env.Define("Defender", defender);
    env.Define("Weapon",   weapon);

    for(size_t i = 0; i < onAttackScripts.GetSize(); i++)
    {
        onAttackScripts[i]->Run(&env);
    }
}

void gemActor::InvokeDefenseScripts(gemActor* attacker, psItem* weapon)
{
    MathEnvironment env;
    env.Define("Attacker", attacker);
    env.Define("Defender", this);
    env.Define("Weapon",   weapon);

    for(size_t i = 0; i < onDefenseScripts.GetSize(); i++)
    {
        onDefenseScripts[i]->Run(&env);
    }
}

void gemActor::InvokeNearlyDeadScripts(gemActor* attacker, psItem* weapon)
{
    MathEnvironment env;
    env.Define("Attacker", attacker);
    env.Define("Defender", this);
    env.Define("Weapon",   weapon);

    for(size_t i = 0; i < onNearlyDeadScripts.GetSize(); i++)
    {
        onNearlyDeadScripts[i]->Run(&env);
    }
}

void gemActor::InvokeMovementScripts()
{
    csVector3 pos = GetMeshWrapper()->GetMovable()->GetPosition();
    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("LocX", pos.x);
    env.Define("LocY", pos.y);
    env.Define("LocZ", pos.z);

    for(size_t i = 0; i < onMovementScripts.GetSize(); i++)
    {
        onMovementScripts[i]->Run(&env);
    }
}

void gemActor::DoDamage(gemActor* attacker, float damage)
{
    // Handle trivial "already dead" case
    if(!IsAlive())
    {
        if(attacker && attacker->GetClientID())
            psserver->SendSystemInfo(attacker->GetClientID(),"It's already dead.");
        return;
    }

    MathEnvironment env;
    env.Define("Actor", this);
    env.Define("Damage", damage);
    (void) psserver->GetCacheManager()->GetDoDamage()->Evaluate(&env);

    if(GetCharacterData()->GetHP() - damage < 0)
    {
        damage = GetCharacterData()->GetHP();
    }

    // Add dmg to history
    AddAttackerHistory(attacker, damage);

    psChar->AdjustHitPoints(-damage);
    float hp = psChar->GetHP();

    if(damage != 0.0)
    {
        if(attacker != NULL)
        {
            Debug5(LOG_COMBAT, attacker->GetPID().Unbox(), "%s do damage %1.2f on %s to new hp %1.2f", attacker->GetName(), damage, GetName(), hp);
        }
        else
        {
            Debug4(LOG_COMBAT,0,"damage %1.2f on %s to new hp %1.2f", damage,GetName(),hp);
        }

        /// Allow rest of server to process this damage
        psDamageEvent evt(attacker, this, damage);
        evt.FireEvent();
    }

    // Death takes some special work
    if(fabs(hp) < 0.0005f || GetMode() == PSCHARACTER_MODE_DEFEATED)   // check for dead
    {
        // for now, if the actor is on a mount, he's dropped off it
        if(GetMount())
        {
            entityManager->RemoveRideRelation(this);
        }

        // If no explicit killer, look for the last person to cast a DoT spell.
        if(!attacker && dmgHistory.GetSize() > 0)
        {
            for(int i = (int) dmgHistory.GetSize() - 1; i >= 0; i--)
            {
                if(dynamic_cast<DOTHistory*>(dmgHistory[i]))
                {
                    attacker = dmgHistory[i]->Attacker();
                    break;
                }
            }
        }
        // If in a duel, don't immediately kill...defeat.
        if(attacker && GetMode() != PSCHARACTER_MODE_DEFEATED && GetClient() && GetClient()->IsDuelClient(attacker->GetClientID()))
        {
            Defeat();
            psserver->SendSystemError(GetClientID(), "You've been defeated by %s!", attacker->GetName());
            GetClient()->AnnounceToDuelClients(attacker, "defeated");
        }
        else
        {
            psDeathEvent death(this,attacker);
            death.FireEvent();

            if(GetMode() == PSCHARACTER_MODE_DEFEATED && attacker)
            {
                psserver->SendSystemError(GetClientID(), "You've been slain by %s!", attacker->GetName());
                GetClient()->AnnounceToDuelClients(attacker, "slain");
            }

            StopMoving(true);

            SetMode(PSCHARACTER_MODE_DEAD);
        }

        // if damage due to spell then spell is ending anyway, so no need to force
        // 'stop attack.'
        if(attacker && attacker->GetMode() == PSCHARACTER_MODE_COMBAT &&
                (!attacker->GetClient() || attacker->GetClient()->GetTargetObject() == this))
        {
            psserver->combatmanager->StopAttack(attacker);
        }

        MulticastDRUpdate();
    }

    // Update group stats and it's own
    SendGroupStats();
}

void gemActor::AddAttackerHistory(gemActor* attacker, float damage)
{
    if(!attacker || damage <= 0)
        return;

    dmgHistory.Push(new DamageHistory(attacker, damage));
}

void gemActor::AddAttackerHistory(gemActor* attacker, float hpRate, csTicks duration)
{
    // only record health drain, not health gain
    if(!attacker || hpRate >= 0)
        return;

    dmgHistory.Push(new DOTHistory(attacker, hpRate, duration));
}

void gemActor::RemoveAttackerHistory(gemActor* attacker)
{
    if(attacker && dmgHistory.GetSize() > 0)
    {
        // Count backwards to avoid trouble with shifting indexes
        for(size_t i = dmgHistory.GetSize() - 1; i != (size_t) -1; i--)
        {
            if(dmgHistory[i]->Attacker() == attacker)
            {
                dmgHistory.DeleteIndex(i);
            }
        }
    }
}

bool gemActor::CanBeAttackedBy(gemActor* attacker, gemActor* &lastAttacker) const
{
    lastAttacker = NULL;
    csTicks lasttime = csGetTicks();

    for(int i = (int)GetDamageHistoryCount(); i>0; i--)
    {
        const AttackerHistory* lasthit = GetDamageHistory(i-1);

        // any 15 second gap is enough to make us stop looking
        if((lasttime - lasthit->TimeOfAttack()) > 15000)
        {
            return true;
        }
        lasttime = lasthit->TimeOfAttack();

        if(lasthit->Attacker().IsValid())
        {
            lastAttacker = lasthit->Attacker();
        }
        else
        {
            continue;  // ignore disconnects
        }

        // If someone else, except for attacker pet, hit first and attacker not grouped with them,
        // attacker locked out
        if(lastAttacker != attacker && !attacker->IsGroupedWith(lastAttacker, true) &&
                !attacker->IsMyPet(lastAttacker))
        {
            return false;
        }
    }

    return true;
}

bool gemActor::HasBeenAttackedBy(gemActor* attacker)
{
    for(int i = (int)GetDamageHistoryCount(); i>0; i--)
    {
        const AttackerHistory* hit = GetDamageHistory(i-1);

        //check if we have found our attacker in the history
        if(attacker == hit->Attacker())
        {
            return true;
        }
    }
    return false;
}


void gemActor::UpdateStats()
{
    if(psChar)
    {
        if(psChar->UpdateStatDRData(csGetTicks()))
        {
            // There are dirty stats that need to be published
            SendGroupStats();
        }
    }
}

void gemActor::HandleDeath()
{
    // Cancel the appropriate ActiveSpells
    CancelActiveSpellsForDeath();

    // Notifiy recievers that this actor has died.
    // Recievers will have to register again if they
    // want to be continually notified of deaths
    while(deathReceivers.GetSize() > 0)
    {
        iDeathCallback* receiver = deathReceivers.Pop();
        receiver->DeathCallback(this);
    }

    psChar->SetAllStatsDirty();
    SendGroupStats();
}

void gemActor::SendGroupStats()
{
    // First we update super clients. Updating stats to clients (Targeted entities)
    // will cause the stats dirty flag to be cleared so we do this update first.
    unsigned int statsDirtyFlags = GetCharacterData()->GetStatsDirtyFlags();
    psserver->GetNPCManager()->QueueStatDR(this, statsDirtyFlags);

    if(InGroup() || GetClientID())
    {
        psChar->SendStatDRMessage(GetClientID(), eid, 0, InGroup() ? GetGroup() : NULL);
    }

    BroadcastTargetStatDR(entityManager->GetClients());
}

bool gemActor::Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities)
{
    csRef<PlayerGroup> group = this->GetGroup();
    csString texparts;
    csString equipmentParts;

    psChar->MakeTextureString(texparts);
    psChar->MakeEquipmentString(equipmentParts);

    csString guildName;
    guildName.Clear();
    if(psChar->GetGuild() && !psChar->GetGuild()->IsSecret())
    {
        guildName = psChar->GetGuild()->GetName();
    }

    uint32_t groupID = 0;
    if(group)
    {
        groupID = group->GetGroupID();
    }

    uint32_t flags = 0;

    // The following flags will be percepted to the NPC client in the FlagPerception message
    // upon change.
    if(!GetVisibility())    flags |= psPersistActor::INVISIBLE;
    if(GetInvincibility())  flags |= psPersistActor::INVINCIBLE;
    if(IsAlive())           flags |= psPersistActor::IS_ALIVE;

    Client* targetClient = psserver->GetConnections()->Find(clientnum);
    if(targetClient && targetClient->GetCharacterData())
    {
        if(((GetClient()->GetSecurityLevel() >= GM_LEVEL_0) &&
                GetCharacterData()->GetGuild()) ||
                (targetClient->GetSecurityLevel() >= GM_LEVEL_0) ||
                targetClient->GetCharacterData()->Knows(psChar))
        {
            flags |= psPersistActor::NAMEKNOWN;
        }
    }

    psPersistActor mesg(clientnum,
                        securityLevel,
                        masqueradeLevel,
                        control,
                        name,
                        guildName,
                        psChar->GetRaceInfo()->GetMeshName(),
                        psChar->GetRaceInfo()->GetTextureName(),
                        psChar->GetRaceInfo()->GetName(),
                        GetMount() ? GetMount()->GetRaceInfo()->mesh_name : "null",
                        GetMount() ? GetMount()->GetRaceInfo()->GetMounterAnim() : "null",
                        psChar->GetRaceInfo()->gender,
                        psChar->GetScale(),
                        GetMount() ? GetMount()->GetRaceInfo()->GetScale() : 1,
                        psChar->GetHelmGroup(),
                        psChar->GetBracerGroup(),
                        psChar->GetBeltGroup(),
                        psChar->GetCloakGroup(),
                        top, bottom,offset,
                        texparts,
                        equipmentParts,
                        DRcounter,
                        eid,
                        cacheManager->GetMsgStrings(),
                        pcmove,
                        movementMode,
                        GetMode(),
                        (to_superclients || allEntities) ? pid : 0, // playerID should not be distributed to clients
                        groupID,
                        0, // ownerEID
                        flags,
                        0, // masterID
                        (to_superclients || allEntities)
                       );

    // Only send to client
    if(clientnum && !to_superclients)
    {
        mesg.SendMessage();
    }

    if(to_superclients)
    {
        Debug1(LOG_SUPERCLIENT, clientnum, "Sending gemActor to superclients.\n");

        mesg.SetInstance(GetInstance());
        if(clientnum == 0)  // Send to all superclients
        {
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }

    if(allEntities)
    {
        return allEntities->AddEntityMessage(mesg.msg);
    }

    return true;
}

void gemActor::Broadcast(int clientnum, bool control)
{
    csArray<PublishDestination> &dest = GetMulticastClients();
    for(unsigned long i = 0 ; i < dest.GetSize(); i++)
    {
        if(dest[i].dist < PROX_LIST_ANY_RANGE)
            Send(dest[i].client, control, false);
    }

    // Update the labels of people in your secret guild
    if(psChar->GetGuild() && psChar->GetGuild()->IsSecret())
    {
        psUpdatePlayerGuildMessage update(
            clientnum,
            eid,
            psChar->GetGuild()->GetName());

        psserver->GetEventManager()->Broadcast(update.msg,NetBase::BC_GUILD,psChar->GetGuild()->GetID());
    }
}

gemObject* gemActor::FindNearbyActorName(const char* name)
{
    return proxlist->FindObjectName(name);
}

const char* gemActor::GetGuildName()
{
    if(GetGuild())
        return GetGuild()->GetName();
    else
        return NULL;
}

bool gemActor::AddChatReport(gemActor* reporter)
{
    activeReports++;

    csString cssBuffer("");
    csString cssTempLine("");

    if(activeReports == 1)
    {
        // this is the first report. Let's open the log file
        // and write all buffered history in it...
        csRef<iVFS> vfs =  csQueryRegistry<iVFS> (cel->object_reg);
        if(!vfs)
        {
            activeReports = 0;
            Error1("Query for vfs plugin failed!");
            return false;
        }

        // One file per user
        csString cssFilename = csString().Format("/this/logs/report_chat_%s.log", GetFirstName());

        logging_chat_file = vfs->Open(cssFilename.GetData(), VFS_FILE_APPEND);
        if(!logging_chat_file)
        {
            Error1("Error, could not open log file to append.");
            activeReports = 0;
            return false;
        }

        Notify2(LOG_ANY, "Logging of chat messages for '%s' started\n", ShowID(pid));

        const csVector3 &pos = GetPosition();
        iSector* sector = GetSector();

        int degrees = (int)(GetAngle() * 180.0f / PI);

        csString sectorName = sector ? sector->QueryObject()->GetName() : "(null)";

        // Start with general information
        cssBuffer = "================================================================\n";
        cssBuffer.AppendFmt("Starting to log player %s (%s).\n", GetName(), ShowID(pid));
        cssBuffer.AppendFmt("Player has %d security level.\n", securityLevel);
        cssBuffer.AppendFmt("At position (%1.2f, %1.2f, %1.2f) angle: %d in sector: %s, instance: %d.\n",
                            pos.x, pos.y, pos.z, degrees, sectorName.GetData(), worldInstance);
        cssBuffer.AppendFmt("Total time connected is %1.1f hours.\n", (GetCharacterData()->GetTimeConnected() / 3600.0f));
        cssBuffer.AppendFmt("================================================================\n");
        // Write existing chat history
        size_t length = chatHistory.GetSize();
        for(size_t i=0; i<length; i++)
        {
            chatHistory[i].GetLogLine(cssTempLine);
            cssBuffer += cssTempLine;
        }
        chatHistory.DeleteRange(0, length-1); //we don't want to relog again these.
    }

    // Add /report line
    ChatHistoryEntry cheReport(csString().Format("-- At this point player got reported by %s --", reporter->GetName()).GetData());
    cheReport.GetLogLine(cssTempLine);
    cssBuffer += cssTempLine;

    // Write the data to the file
    logging_chat_file->Write(cssBuffer.GetData(), cssBuffer.Length());
    logging_chat_file->Flush(); // flush

    return true; // been there, done that
}

void gemActor::RemoveChatReport()
{
    activeReports--;

    if(activeReports == 0)
    {
        // This is the last /report for this client that expires,
        // so we terminate logging and close the file.
        Notify2(LOG_ANY, "Logging of chat messages for '%s' stopped -\n", ShowID(pid));

        csString cssBuffer("================================================================\n\n");
        logging_chat_file->Write(cssBuffer.GetData(), cssBuffer.Length());
        logging_chat_file->Flush();

        // This should close the file.
        logging_chat_file = 0;
    }
}

bool gemActor::LogChatMessage(const char* who, const psChatMessage &msg)
{
    csString cssLine("");

    // Format according to chat type
    switch(msg.iChatType)
    {
        case CHAT_SHOUT:
            cssLine.Format("%s shouts: %s", who, msg.sText.GetData());
            break;
        case CHAT_SAY:
            cssLine.Format("%s says: %s", who, msg.sText.GetData());
            break;
        case CHAT_CHANNEL:
            cssLine.Format("Channel %d> %s: %s", msg.channelID, who, msg.sText.GetData());
            break;
        case CHAT_TELL:
            cssLine.Format("%s tells %s: %s", who, msg.sPerson.GetData(), msg.sText.GetData());
            break;
        case CHAT_GUILD:
            cssLine.Format("GuildChat from %s: %s", who, msg.sText.GetData());
            break;
        case CHAT_ALLIANCE:
            cssLine.Format("AllianceChat from %s: %s", who, msg.sText.GetData());
            break;
        case CHAT_GROUP:
            cssLine.Format("GroupChat from %s: %s", who, msg.sText.GetData());
            break;
        case CHAT_AUCTION:
            cssLine.Format("Auction from %s: %s", who, msg.sText.GetData());
            break;
        default:
            return false; // We do not log any other chat types.
    }
    return LogLine(cssLine.GetData());
}

bool gemActor::LogSystemMessage(const char* szLine)
{
    return LogLine(csString().Format("> %s", szLine).GetData());
}

bool gemActor::LogLine(const char* szLine)
{
    // Add to chat history
    ChatHistoryEntry cheNew(szLine);

    if(!IsLoggingChat())  // Check if we're logging. If not, store the message in the history and bail out.
    {
        chatHistory.Push(cheNew);

        // Maintain history
        time_t check = time(0) - CHAT_HISTORY_LIFETIME;
        // We delete lines older than current time + CHAT_HISTORY_LIFETIME
        // if the history size is not lowest than the minimum
        while((chatHistory.GetSize()>CHAT_HISTORY_MINIMUM_SIZE) &&
                chatHistory.Get(0)._time < check)
        {
            chatHistory.DeleteIndex(0);
        }
        return false;
    }

    csString cssLine("");
    cheNew.GetLogLine(cssLine); // Get a log-writtable line
    logging_chat_file->Write(cssLine.GetData(), cssLine.Length()); // Write to the file
    return true; // The line was written to a file, so return true.
}

/**
* Determines the right size and height for the collider for the sprite
* and sets the actual position of the sprite.
*/
bool gemActor::InitLinMove(const csVector3 &pos,
                           float angle, iSector* sector)
{
    pcmove = new psLinearMovement(psserver->GetObjectReg());

    // Check if we're using sizes from the db.
    csVector3 size;
    psRaceInfo* raceinfo = psChar->GetRaceInfo();
    raceinfo->GetSize(size);
    float width = size.x;
    float height = size.y;
    float depth = size.z;

    // Add a fudge factor to the depth to allow for feet
    // sticking forward while running
    depth *= 1.33F;

    // Now check if the size within limits.
    // Can't be to small or the ExtrapolatePosition in NPCClient
    // will take to long to process.
    if(width < 0.2)
    {
        Warning4(LOG_ANY, "Raceinfo width %.2f is too small for %s(%s). Using default.", width, GetName(), ShowID(pid));
        width = 0.8F;
    }
    if(depth < 0.2*1.33)
    {
        Warning4(LOG_ANY, "Raceinfo depth %.2f is too small for %s(%s). Using default.", depth, GetName(), ShowID(pid));
        depth = 0.6F*1.33F;
    }
    if(height < 0.2F)
    {
        Warning4(LOG_ANY, "Raceinfo height %.2f is too small for %s(%s). Using default.", height, GetName(), ShowID(pid));
        height = 1.4F;
    }

    float legSize;
    if(height > 0.8F)
    {
        legSize = 0.7F;
    }
    else
    {
        legSize = height * 0.5f;
    }

    top = csVector3(width,height-legSize,depth);
    bottom = csVector3(width-0.1,legSize,depth-0.1);
    offset = csVector3(0,0,0);
    top.x *= .7f;
    top.z *= .7f;
    bottom.x *= .7f;
    bottom.z *= .7f;

    pcmove->InitCD(top, bottom,offset, GetMeshWrapper());
    // Disable CD checking because server does not need it.
    pcmove->UseCD(false);

    SetPosition(pos,angle,sector);

    return true;  // right now this func never fail, but might later.
}

bool gemActor::SetupCharData()
{
    //dummy function
    return true;  // right now this func never fail, but might later.
}


bool gemActor::InitCharData(Client* c)
{
    if(!c)
    {
        // NPC
        SetSecurityLevel(-1);
    }
    else
    {
        SetSecurityLevel(c->GetSecurityLevel());
        SetGMDefaults();
    }

    return SetupCharData();

}

void gemActor::SetTextureParts(const char* parts)
{
}

void gemActor::SetEquipment(const char* equip)
{
}

const char* gemActor::GetModeStr()
{
    static const char* strs[] = {"unknown","doing nothing","in combat","spell casting","working","dead","sitting","carrying too much","exhausted", "defeated", "statued", "playing an instrument" };
    return strs[player_mode];
}

bool gemActor::CanSwitchMode(PSCHARACTER_MODE from, PSCHARACTER_MODE to)
{
    switch(to)
    {
        case PSCHARACTER_MODE_DEFEATED:
            return from != PSCHARACTER_MODE_DEAD;
        case PSCHARACTER_MODE_OVERWEIGHT:
            return (from != PSCHARACTER_MODE_DEAD &&
                    from != PSCHARACTER_MODE_DEFEATED);
        case PSCHARACTER_MODE_EXHAUSTED:
        case PSCHARACTER_MODE_COMBAT:
        case PSCHARACTER_MODE_SPELL_CASTING:
        case PSCHARACTER_MODE_WORK:
        case PSCHARACTER_MODE_PLAY:
            return (from != PSCHARACTER_MODE_OVERWEIGHT &&
                    from != PSCHARACTER_MODE_DEAD &&
                    from != PSCHARACTER_MODE_DEFEATED);
        case PSCHARACTER_MODE_SIT:
        case PSCHARACTER_MODE_PEACE:
        case PSCHARACTER_MODE_DEAD:
        case PSCHARACTER_MODE_STATUE:
        default:
            return true;
    }
}

void gemActor::SetMode(PSCHARACTER_MODE newmode, uint32_t extraData)
{
    // Assume combat messages need to be sent anyway, because the stance may have changed.
    // TODO: integrate this with extraData and remove this workaround?
    if(player_mode == newmode && newmode != PSCHARACTER_MODE_COMBAT)
        return;

    if(newmode < PSCHARACTER_MODE_PEACE || newmode >= PSCHARACTER_MODE_COUNT)
    {
        Error3("Unhandled mode: %d switching to %d", player_mode, newmode);
        return;
    }

    if(!CanSwitchMode(player_mode, newmode))
        return;

    // Force mode to be OVERWEIGHT if encumbered and the proposed mode isn't
    // more important.
    if(!psChar->Inventory().HasEnoughUnusedWeight(0) && CanSwitchMode(newmode, PSCHARACTER_MODE_OVERWEIGHT))
        newmode = PSCHARACTER_MODE_OVERWEIGHT;

    if(newmode != PSCHARACTER_MODE_COMBAT)
    {
        SetCombatStance(CombatManager::GetStance(cacheManager,"None"));
    }

    if(newmode == PSCHARACTER_MODE_COMBAT)
        extraData = combat_stance.stance_id;

    psModeMessage msg(GetClientID(), GetEID(), (uint8_t) newmode, extraData);
    msg.Multicast(GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);

    bool isFrozen = IsFrozen();
    SetAllowedToMove(newmode != PSCHARACTER_MODE_DEAD &&
                     newmode != PSCHARACTER_MODE_DEFEATED &&
                     newmode != PSCHARACTER_MODE_SIT &&
                     newmode != PSCHARACTER_MODE_EXHAUSTED &&
                     newmode != PSCHARACTER_MODE_OVERWEIGHT &&
                     newmode != PSCHARACTER_MODE_STATUE &&
                     newmode != PSCHARACTER_MODE_PLAY &&
                     !isFrozen);

    SetAlive(newmode != PSCHARACTER_MODE_DEAD);

    SetAllowedToDisconnect(newmode != PSCHARACTER_MODE_SPELL_CASTING &&
                           newmode != PSCHARACTER_MODE_COMBAT &&
                           newmode != PSCHARACTER_MODE_DEFEATED);

    // cancel ongoing work
    if(player_mode == PSCHARACTER_MODE_WORK && workEvent)
    {
        workEvent->Interrupt();
        workEvent = NULL;
    }

    // stop playing current song
    if(player_mode == PSCHARACTER_MODE_PLAY)
    {
        psserver->GetSongManager()->StopSong(this, true);
    }

    switch(newmode)
    {
        case PSCHARACTER_MODE_EXHAUSTED:
        case PSCHARACTER_MODE_COMBAT:
            psChar->SetStaminaRegenerationStill();  // start stamina regen
            break;

        case PSCHARACTER_MODE_SIT:
        case PSCHARACTER_MODE_OVERWEIGHT:
            psChar->SetStaminaRegenerationSitting();
            break;

        case PSCHARACTER_MODE_DEAD:
        case PSCHARACTER_MODE_STATUE:
            psChar->SetStaminaRegenerationNone();  // no stamina regen while dead
            break;
        case PSCHARACTER_MODE_DEFEATED:
            psChar->GetHPRate().SetBase(HP_REGEN_RATE);
            psChar->SetStaminaRegenerationNone();
            break;

        case PSCHARACTER_MODE_PLAY:
            psChar->StartSong();
            break;
        case PSCHARACTER_MODE_WORK:
            break;  // work manager sets it's own rates
        default:
            break;
    }

    player_mode = newmode;

    if(newmode == PSCHARACTER_MODE_PEACE || newmode == PSCHARACTER_MODE_SPELL_CASTING)
        ProcessStamina();
}

void gemActor::SetCombatStance(const Stance &stance)
{
    CS_ASSERT(stance.stance_id < cacheManager->stances.GetSize());

    if(combat_stance.stance_id == stance.stance_id)
        return;

    combat_stance = stance;
    Debug3(LOG_COMBAT, pid.Unbox(), "Setting stance to %s for %s", stance.stance_name.GetData(), GetName());
}

// This function should only be run on initial load
void gemActor::SetGMDefaults()
{
    if(cacheManager->GetCommandManager()->Validate(securityLevel, "default invincible"))
    {
        invincible = true;
        safefall = true;
        nevertired = true;
        infinitemana = true;
    }

    if(cacheManager->GetCommandManager()->Validate(securityLevel, "default invisible"))
    {
        visible = false;
    }

    if(cacheManager->GetCommandManager()->Validate(securityLevel, "default viewall"))
    {
        viewAllObjects = true;
    }

    if(cacheManager->GetCommandManager()->Validate(securityLevel, "default infinite inventory"))
    {
        SetFiniteInventory(false);
    }

    questtester = false;  // Always off by default
}

void gemActor::SetDefaults(bool player)
{
    if(player)
    {
        if(invincible)
        {
            SetInvincibility(false);
        }
        safefall = false;
        nevertired = false;
        infinitemana = false;
        if(!GetVisibility())
        {
            SetVisibility(true);
        }
        if(GetViewAllObjects())
        {
            SetViewAllObjects(false);
        }
        if(!GetFiniteInventory())
        {
            SetFiniteInventory(true);
        }
        questtester = false;
        instantcast = false;
        givekillexp = true;
        attackable = true;
        GetClient()->SetBuddyListHide(false);
    }
    else
    {
        if(cacheManager->GetCommandManager()->Validate(securityLevel, "default invincible"))
        {
            if(invincible)
            {
                SetInvincibility(true);
            }
            safefall = true;
            nevertired = true;
            infinitemana = true;
        }
        if(cacheManager->GetCommandManager()->Validate(securityLevel, "default invisible"))
        {
            if(GetVisibility())
            {
                SetVisibility(false);
            }
            if(!GetViewAllObjects())
            {
                SetViewAllObjects(true);
            }
        }
        if(cacheManager->GetCommandManager()->Validate(securityLevel, "default infinite inventory"))
        {
            if(GetFiniteInventory())
            {
                SetFiniteInventory(false);
            }
        }
        questtester = false;
        instantcast = false;
        givekillexp = false;
        attackable = false;
        GetClient()->SetBuddyListHide(true);
    }
}


void gemActor::SetInstance(InstanceID worldInstance)
{
    this->worldInstance = worldInstance;
}

void gemActor::Teleport(const char* sectorName, const csVector3 &pos, float yrot, InstanceID instance, int32_t loadDelay, csString background, csVector2 point1, csVector2 point2, csString widget)
{
    csRef<iEngine> engine = csQueryRegistry<iEngine>(psserver->GetObjectReg());
    iSector* sector = engine->GetSectors()->FindByName(sectorName);
    if(!sector)
    {
        Bug2("Sector %s is not found!", sectorName);
        return;
    }
    Teleport(sector, pos, yrot, instance, loadDelay, background, point1, point2, widget);
}

void gemActor::Teleport(iSector* sector, const csVector3 &pos, float yrot, InstanceID instance, int32_t loadDelay, csString background, csVector2 point1, csVector2 point2, csString widget)
{
    SetInstance(instance);
    Teleport(sector, pos, yrot, loadDelay, background, point1, point2, widget);
}

void gemActor::Teleport(iSector* sector, const csVector3 &pos, float yrot, int32_t loadDelay, csString background, csVector2 point1, csVector2 point2, csString widget)
{
    StopMoving();
    SetPosition(pos, yrot, sector);

    if(GetClient())
        GetClient()->SetCheatMask(MOVE_CHEAT, true); // Tell paladin one of these is OK.

    UpdateProxList();
    MulticastDRUpdate();
    ForcePositionUpdate(loadDelay, background, point1, point2, widget);
    BroadcastTargetStatDR(entityManager->GetClients()); //we need to update the stats too
}

void gemActor::SetPosition(const csVector3 &pos,float angle, iSector* sector)
{
    // Verify the location first. CS cannot handle positions greater than 100000.
    if(fabs(pos.x) > 100000 || fabs(pos.y) > 100000 || fabs(pos.z) > 100000)
    {
        MoveToSpawnPos();
        Error5("Attempted to set position of actor pid %d to %g %g %g", pid.Unbox(), pos.x, pos.y, pos.z);
        return;
    }

    Move(pos, angle, sector);

    psSectorInfo* sectorInfo = NULL;
    if(sector != NULL)
        sectorInfo = cacheManager->GetSectorInfoByName(sector->QueryObject()->GetName());
    if(sectorInfo != NULL)
    {
        psChar->SetLocationInWorld(worldInstance, sectorInfo, pos.x, pos.y, pos.z, angle);
        UpdateValidLocation(pos, angle, sector, worldInstance, true);
    }
}

//#define STAMINA_PROCESS_DEBUG

void gemActor::ProcessStamina()
{
    csVector3 vel = pcmove->GetVelocity();
    ProcessStamina(vel, true);
}

void gemActor::ProcessStamina(const csVector3 &velocity, bool force)
{
    // GM flag
    if(nevertired)
        return;

    csTicks now = csGetTicks();

    if(lastDR == 0)
        lastDR = now;

    csTicks elapsed = now - lastDR;

    // Update once a second, or on any updates with upward motion
    if(elapsed > 1000 || velocity.y > SMALL_EPSILON || force)
    {
        lastDR += elapsed;

        float times = 1.0f;

        /* If we're slow or forced, we need to multiply the stamina with the secs passed.
         * DR updates are sent when speed changes, so it should be quite accurate.
         */
        times = float(elapsed) / 1000.0f;

        atRest = velocity.IsZero();

#ifdef STAMINA_PROCESS_DEBUG
        printf("Processing stamina with %u elapsed since last\n", elapsed);
#endif

        ApplyStaminaCalculations(velocity,times);
    }
}

void gemActor::ApplyStaminaCalculations(const csVector3 &v, float times)
{
    csVector3 thisV = lastV;
    lastV=v;

    if(atRest)
    {
#ifdef STAMINA_PROCESS_DEBUG
        printf("Not moving\n");
#endif

        if(GetMode() == PSCHARACTER_MODE_PEACE || GetMode() == PSCHARACTER_MODE_SPELL_CASTING)
            psChar->SetStaminaRegenerationStill();
        else if(GetMode() == PSCHARACTER_MODE_SIT)
            psChar->SetStaminaRegenerationSitting();
    }
    else // Moving
    {
        // unwork stuff
        if(GetMode() == PSCHARACTER_MODE_WORK)
        {
            SetMode(PSCHARACTER_MODE_PEACE);
            psserver->SendSystemInfo(GetClientID(),"You stop working.");
        }

#ifdef STAMINA_PROCESS_DEBUG
        printf("Moving @");
#endif

        // Compute stuff
        /*
                  ^
             v  / |
              /   | y
            /a    |
           o------>
              xz
        */
        double Speed; ///< magnitude of the velocity vector
        double Angle; ///< angle between velocity vector and X-Z plane

        double XZvel = csQsqrt(thisV.x*thisV.x + thisV.z*thisV.z);
        if(thisV.y > EPSILON)
        {
            if(XZvel > EPSILON)
            {
                Speed = csQsqrt(thisV.y*thisV.y + XZvel*XZvel);
                Angle = atan(thisV.y / XZvel);
            }
            else // straight up
            {
                Speed = thisV.y;
                Angle = HALF_PI;
            }
        }
        else // flat ground
        {
            Speed = XZvel;
            Angle = 0.0f;
        }

#ifdef STAMINA_PROCESS_DEBUG
        printf(" %f %f =>", float(Speed), float(Angle));
#endif

        // Stuff goes in
        MathEnvironment env;
        env.Define("Speed",       Speed);                                       // How fast you're moving
        env.Define("AscentAngle", Angle);                                       // How steep your climb is
        env.Define("Weight",      psChar->Inventory().GetCurrentTotalWeight()); // How much you're carrying
        env.Define("MaxWeight",   psChar->Inventory().MaxWeight());             // How much you can carry
        env.Define("MaxStamina",  psChar->GetMaxPStamina().Current());          // Max stamina of the character

        // Do stuff with stuff
        (void) psserver->GetCacheManager()->GetStaminaMove()->Evaluate(&env);

        // Stuff comes out
        MathVar* drain = env.Lookup("Drain");
        float value = drain->GetValue();
        //value *= times;

#ifdef STAMINA_PROCESS_DEBUG
        printf(" %f\n", value);
#endif

        // Apply stuff
        if(GetMode() == PSCHARACTER_MODE_PEACE || GetMode() == PSCHARACTER_MODE_SPELL_CASTING)
        {
            psChar->SetStaminaRegenerationWalk();
            VitalBuffable &pRate = psChar->GetPStaminaRate();
            pRate.SetBase(pRate.Base()-value);
        }
        else  // Another regen in place
        {
            psChar->AdjustStamina(-value*times,true);
        }
    }

    // Check stuff
    if(psChar->GetStamina(true) < 0.05f)
    {
#ifdef STAMINA_PROCESS_DEBUG
        printf("Stopping\n");
#endif

        SetMode(PSCHARACTER_MODE_EXHAUSTED);
        psChar->SetStaminaRegenerationStill();
        psserver->SendSystemResult(GetClientID(),"You're too exhausted to move");
    }
    else if(GetMode() == PSCHARACTER_MODE_EXHAUSTED)
    {
        SetMode(PSCHARACTER_MODE_PEACE);
    }
}

bool gemActor::SetDRData(psDRMessage &drmsg)
{
    if(!drmsg.IsNewerThan(DRcounter))
    {
        return false; // don't do the rest of this if this msg is out of date
    }
    // If this DR came from a client which has not accepted the force position message at least once then we must
    // ignore it as it must be out of date
    if(forcedSector)
    {
        if(drmsg.sector != forcedSector)
        {
            return false;
        }
        else
        {
            forcedSector = NULL;  // Reset the forced sector after we received the correct sector at least once.
        }
    }
    pcmove->SetDRData(drmsg.on_ground,drmsg.pos,drmsg.yrot,drmsg.sector,drmsg.vel,drmsg.worldVel,drmsg.ang_vel);
    DRcounter = drmsg.counter;


    // Apply stamina only on PCs
    if(GetClientID())
        ProcessStamina(drmsg.vel + drmsg.worldVel);

    if(drmsg.sector != NULL)
    {
        UpdateValidLocation(drmsg.pos, drmsg.yrot, drmsg.sector, worldInstance);
        psSectorInfo* sectorInfo = cacheManager->GetSectorInfoByName(drmsg.sector->QueryObject()->GetName());
        if(sectorInfo != NULL)
        {
            psChar->SetLocationInWorld(worldInstance,sectorInfo, drmsg.pos.x, drmsg.pos.y, drmsg.pos.z, drmsg.yrot);

            if(IsSpellCasting() && drmsg.vel.SquaredNorm() > 13.0f)
            {
                InterruptSpellCasting();
            }
        }
    }

    InvokeMovementScripts();
    return true;
}

void gemActor::UpdateValidLocation(const csVector3 &pos, float yrot, iSector* sector, InstanceID instance, bool force)
{
    // 10m hops
    if(force || (!isFalling && (pos - newvalid_location.pos).SquaredNorm() > 100.0f))
    {
        valid_location = newvalid_location;
        newvalid_location.pos = pos;
        newvalid_location.yrot = yrot;
        newvalid_location.sector = sector;
        newvalid_location.instance = instance;
        if(force)
            valid_location = newvalid_location;
    }

    last_location.pos = pos;
    last_location.yrot = yrot;
    last_location.sector = sector;
    last_location.instance = instance;
}

void gemActor::MoveToLastPos()
{
    pcmove->SetOnGround(false);
    Teleport(last_location.sector, last_location.pos, last_location.yrot, last_location.instance);
}

void gemActor::GetLastLocation(csVector3 &pos, float &yrot, iSector* &sector, InstanceID &instance)
{
    pos = last_location.pos;
    yrot = last_location.yrot;
    sector = last_location.sector;
    instance = last_location.instance;
}

void gemActor::SetPrevTeleportLocation(const csVector3 &pos, float yrot, iSector* sector, InstanceID instance)
{
    prev_teleport_location.pos = pos;
    prev_teleport_location.yrot = yrot;
    prev_teleport_location.sector = sector;
    prev_teleport_location.instance = instance;
}

void gemActor::GetPrevTeleportLocation(csVector3 &pos, float &yrot, iSector* &sector, InstanceID &instance)
{
    pos = prev_teleport_location.pos;
    yrot = prev_teleport_location.yrot;
    sector = prev_teleport_location.sector;
    instance = prev_teleport_location.instance;
}

void gemActor::MulticastDRUpdate()
{
    bool on_ground;
    float yrot,ang_vel;
    csVector3 pos,vel,worldVel;
    iSector* sector;
    pcmove->GetDRData(on_ground,pos,yrot,sector,vel,worldVel,ang_vel);
    psDRMessage drmsg(0, eid, on_ground, movementMode, DRcounter,
                      pos,yrot,sector, "", vel,worldVel,ang_vel,
                      psserver->GetNetManager()->GetAccessPointers());
    drmsg.Multicast(GetMulticastClients(),0,PROX_LIST_ANY_RANGE);
}

void gemActor::ForcePositionUpdate(int32_t loadDelay, csString background, csVector2 point1, csVector2 point2, csString widget)
{
    uint32_t clientnum = GetClientID();
    forcedSector = GetSector();

    psForcePositionMessage msg(clientnum, ++forceDRcounter, GetPosition(), GetAngle(), GetSector(), GetVelocity(),
                               cacheManager->GetMsgStrings(), loadDelay, background, point1, point2, widget);
    msg.SendMessage();
}

bool gemActor::UpdateDR()
{
    bool on_ground = pcmove->IsOnGround();
    // We do not want gravity enforced since server is not using CD.
    pcmove->SetOnGround(true);
    pcmove->UpdateDR();
    pcmove->SetOnGround(on_ground);
    return true;
}

void gemActor::GetLastSuperclientPos(csVector3 &pos, InstanceID &instance, csTicks &last) const
{
    pos = lastSentSuperclientPos;
    instance = lastSentSuperclientInstance;
    last = lastSentSuperclientTick;
}

void gemActor::SetLastSuperclientPos(const csVector3 &pos, InstanceID instance, const csTicks &now)
{
    lastSentSuperclientPos = pos;
    lastSentSuperclientInstance = instance;
    lastSentSuperclientTick = now;
}


float gemActor::GetRelativeFaction(gemActor* speaker)
{
    return GetCharacterData()->GetFactions()->FindWeightedDiff(speaker->GetCharacterData()->GetFactions());
}

void gemActor::SetGroup(PlayerGroup* group)
{
    PlayerGroup* oldGroup = this->group;
    this->group = group;

    int groupID = 0;
    bool self = false;
    if(group)
    {
        groupID = group->GetGroupID();
    }

    // Update group clients for group removal
    if(oldGroup != NULL)
    {
        for(int i = 0; i < (int)oldGroup->GetMemberCount(); i++)
        {
            gemActor* memb = oldGroup->GetMember(i);
            if(!memb)
                continue;

            if(memb == this)
                self = true; // No need to send after

            // Send group update
            psUpdatePlayerGroupMessage update(memb->GetClientID(), eid, groupID);

            // CPrintf(CON_DEBUG,"Sending oldgroup update to client %d (%s)\n",memb->GetClientID(),memb->GetName());

            update.SendMessage();
        }
    }

    // Send this actor to the new group to update their entity labels
    if(group != NULL)
    {
        for(int i = 0; i < (int)group->GetMemberCount(); i++)
        {
            gemActor* memb = group->GetMember(i);
            if(!memb)
                continue;

            if(memb == this && self) // already sent to ourselves
                continue;

            // Send us to the rest of the group
            psUpdatePlayerGroupMessage update1(memb->GetClientID(), eid, groupID);

            // Send the rest of the group to us (To make sure we got all the actors)
            psUpdatePlayerGroupMessage update2(GetClientID(),
                                               memb->GetEID(),
                                               groupID);

            // CPrintf(CON_DEBUG,"Sending group update to client %d (%s)\n",memb->GetClientID(),memb->GetName());
            update1.SendMessage();
            update2.SendMessage();
        }
    }

    // Finaly send the message if it hasn't been
    if(!self)
    {
        psUpdatePlayerGroupMessage update(GetClientID(), eid, groupID);
        update.SendMessage();
    }
}

int gemActor::GetGroupID()
{
    csRef<PlayerGroup> group = GetGroup();
    if(group)
    {
        return group->GetGroupID();
    }

    return 0;
}

bool gemActor::InGroup() const
{
    return group.IsValid();
}

bool gemActor::IsGroupedWith(gemActor* other, bool IncludePets) const
{
    return group.IsValid() && group->HasMember(other, IncludePets);
}

void gemActor::RemoveFromGroup()
{
    if(group.IsValid())
    {
        group->Remove(this);
    }
}

bool gemActor::IsMyPet(gemActor* other) const
{
    Client* client = GetClient();

    if(!client) return false;  // Can't own a pet if no client

    return GetClient()->IsMyPet(other);
}



void gemActor::SendGroupMessage(MsgEntry* me)
{
    if(group.IsValid())
    {
        group->Broadcast(me);
    }
    else
    {
        psserver->GetEventManager()->SendMessage(me);
    }
}


void gemActor::SendTargetStatDR(Client* client)
{
    psChar->SendStatDRMessage(client->GetClientNum(), eid, DIRTY_VITAL_ALL);
}

void gemActor::BroadcastTargetStatDR(ClientConnectionSet* clients)
{

    // Send statDR to all clients that have this client targeted
    csArray<PublishDestination> &c = GetMulticastClients();
    for(size_t i=0; i < c.GetSize(); i++)
    {
        Client* cl = clients->Find(c[i].client);
        if(!cl)   // not ready yet
            continue;
        if((gemActor*)cl->GetTargetObject() == this)   // client has this actor targeted
        {
            SendTargetStatDR(cl);
        }
    }

    // Send statDR to player's client
    Client* actorClient = clients->FindPlayer(pid);
    if(actorClient != NULL)
    {
        psChar->SendStatDRMessage(actorClient->GetClientNum(), eid, 0);
    }
}

csString gemActor::GetDefaultBehavior(const csString &dfltBehaviors)
{
    return ::GetDefaultBehavior(dfltBehaviors, 1);
}

void gemActor::SendBehaviorMessage(const csString &msg_id, gemObject* actor)
{
    if(msg_id == "context")
    {
        // If the player is in range of the item.
        if(RangeTo(actor) < RANGE_TO_SELECT && actor->IsAlive())
        {
            if(actor->GetClientID() == 0)
                return;
            if(!actor->GetCharacterData())
                return;

            int options = 0;

            options |= psGUIInteractMessage::PLAYERDESC;

            // Get the actor who is targetting. The target is *this.
            gemActor* activeActor = dynamic_cast<gemActor*>(actor);
            if(activeActor && activeActor->GetClientID() != 0)
            {
                if(activeActor->GetMount() && this == actor)
                    options |= psGUIInteractMessage::UNMOUNT;

                if((activeActor->GetMode() == PSCHARACTER_MODE_PEACE || activeActor->GetMode() == PSCHARACTER_MODE_SIT) && this != activeActor)
                    options |= psGUIInteractMessage::EXCHANGE;

                // Can we attack this player?
                csString msg; // Not used
                if(IsAlive() && activeActor->IsAlive() && activeActor->IsAllowedToAttack(this,msg))
                    options |= psGUIInteractMessage::ATTACK;

                /*Options for a wedding or a divorce, in order to show the proper button.
                 *If the character is married with the target, the only option is to divorce.*/

                if(activeActor->GetCharacterData()->GetIsMarried()
                        && this != activeActor
                        && !strcmp(activeActor->GetCharacterData()->GetSpouseName(), GetCharacterData()->GetActor()->GetName()))
                {
                    options |= psGUIInteractMessage::DIVORCE;
                }
                //If the target is not married, then we can ask to marry him/her. Otherwise, no other option is valid.
                //However the target must be of the opposite sex, except in the Kran case.
                else if(!activeActor->GetCharacterData()->GetIsMarried() &&
                        !GetCharacterData()->GetIsMarried() && this != activeActor)
                {
                    if((GetCharacterData()->GetRaceInfo()->gender == PSCHARACTER_GENDER_NONE &&
                            activeActor->GetCharacterData()->GetRaceInfo()->gender == PSCHARACTER_GENDER_NONE) ||
                            (GetCharacterData()->GetRaceInfo()->gender !=
                             activeActor->GetCharacterData()->GetRaceInfo()->gender &&
                             GetCharacterData()->GetRaceInfo()->gender != PSCHARACTER_GENDER_NONE
                             && activeActor->GetCharacterData()->GetRaceInfo()->gender != PSCHARACTER_GENDER_NONE))
                    {
                        options |= psGUIInteractMessage::MARRIAGE;
                    }
                }

                // Introduce yourself
                if(IsAlive() && GetCharacterData()->Knows(activeActor->GetCharacterData()))
                {
                    options |= psGUIInteractMessage::INTRODUCE;
                }
            }

            if(!options)
                return;

            // Always possible to close menu
            options |= psGUIInteractMessage::CLOSE;

            unsigned int client = actor->GetClientID();
            psGUIInteractMessage guimsg(client,options);
            guimsg.SendMessage();
        }
    }
    else if(msg_id == "playerdesc")
        psserver->usermanager->SendCharacterDescription(actor->GetClient(),
                this, false, false, "behaviorMsg");
    else if(msg_id == "exchange")
        psserver->exchangemanager->StartExchange(actor->GetClient(), true);
    else if(msg_id == "attack")
        psserver->usermanager->Attack(CombatManager::GetStance(cacheManager,"Normal"), actor->GetClient());
}

void gemActor::SetAction(const char* anim,csTicks &timeDelay)
{
    if(!anim)
        return;
    psOverrideActionMessage action(0, eid, anim);
    action.Multicast(GetMulticastClients(),-1,0);
}

float gemActor::FallEnded(const csVector3 &pos, iSector* sector)
{
    isFalling = false;

    psWorld* psworld = entityManager->GetWorld();

    // Convert fallStartPos into coordinate system of fall end sector.
    if(!psworld->WarpSpace(fallStartSector,sector,fallStartPos))
    {
        // If we can't warp space, someone probably teleported between two
        // sectors without portals to one another.

        // Error3("Don't know how to calculate damage for fall between %s and %s", fallStartSector->QueryObject()->GetName(),sector->QueryObject()->GetName());
        return 0.0; // Don't give any fall damage if we don't know how to transform
    }

    return fallStartPos.y - pos.y;
}

void gemActor::FallBegan(const csVector3 &pos, iSector* sector)
{
    isFalling = true;

    this->fallStartPos = pos;
    this->fallStartSector = sector;
    this->fallStartTime = csGetTicks();
}

void gemActor::AttachScript(ProgressionScript* script, int type)
{
    CS_ASSERT(script);
    switch(type)
    {
        case ATTACK:
            onAttackScripts.Push(script);
            break;
        case DEFENSE:
            onDefenseScripts.Push(script);
            break;
        case NEARLYDEAD:
            onNearlyDeadScripts.Push(script);
            break;
        case MOVE:
            onMovementScripts.Push(script);
            break;
    };
}

void gemActor::DetachScript(ProgressionScript* script, int type)
{
    switch(type)
    {
        case ATTACK:
            onAttackScripts.Delete(script);
            break;
        case DEFENSE:
            onDefenseScripts.Delete(script);
            break;
        case NEARLYDEAD:
            onNearlyDeadScripts.Delete(script);
            break;
        case MOVE:
            onMovementScripts.Delete(script);
            break;
    };
}

void gemActor::AddActiveSpell(ActiveSpell* asp)
{
    if(!asp)
    {
        return;
    }
    activeSpells.Push(asp);

    if( asp->GetImage().IsEmpty() )
    {
        psSpell*   lspell = psserver->GetCacheManager()->GetSpellByName(asp->Name());
        csString   image;
        if(lspell)
        {
            image = lspell->GetImage();
        }
        asp->SetImage(image);
    }

    SendActiveSpells();
}

void gemActor::SendActiveSpells()
{
    psGUIActiveMagicMessage outgoing(GetClientID(), activeSpells, GetActiveMagicSequence());  // <---add message index tracking!
    outgoing.SendMessage();
}


bool gemActor::RemoveActiveSpell(ActiveSpell* asp)
{
    if(activeSpells.Delete(asp))
    {
        SendActiveSpells();
        return true;
    }
    return false;
}

ActiveSpell* gemActor::FindActiveSpell(const csString &name, SPELL_TYPE type)
{
    for(size_t i = 0; i < activeSpells.GetSize(); i++)
    {
        if(activeSpells[i]->Name() == name && activeSpells[i]->Type() == type)
            return activeSpells[i];
    }
    return NULL;
}

int gemActor::ActiveSpellCount(const csString &name)
{
    int count = 0;
    for(size_t i = 0; i < activeSpells.GetSize(); i++)
    {
        if(activeSpells[i]->Name() == name)
            ++count;
    }
    return count;
}

void gemActor::CancelActiveSpellsForDeath()
{
    // Cancel any spells marked to expire on death.
    for(size_t i = activeSpells.GetSize() - 1; i != (size_t) -1; i--)
    {
        ActiveSpell* asp = activeSpells[i];
        if(asp->CancelOnDeath() && asp->Cancel())
            delete asp;
    }
}

void gemActor::CancelActiveSpellsWhichDamage()
{
    // Cancel any spells marked to expire on death.
    for(size_t i = activeSpells.GetSize() - 1; i != (size_t) -1; i--)
    {
        ActiveSpell* asp = activeSpells[i];
        if(asp->DamagesHP() && asp->Cancel())
            delete asp;
    }
}

int gemActor::FindAnimIndex(const char* name)
{
    return cacheManager->GetMsgStrings()->Request(name);
}

bool gemActor::SetMesh(const char* meshname)
{
    if(pcmesh->GetMesh())
    {
        if(cacheManager->GetRaceInfoByMeshName(meshname) != NULL)
        {
            UpdateProxList(true);
            return true;
        }
    }
    else
    {
        // Get current position to give to the newly set mesh
        csVector3 pos;
        float angle;
        iSector* sector;
        GetPosition(pos,angle,sector);

        csRef<iEngine> engine = csQueryRegistry<iEngine>(psserver->GetObjectReg());
        csRef<iMeshWrapper> newmesh = engine->CreateMeshWrapper(meshname);
        pcmesh->SetMesh(newmesh);

        if(pcmove)
        {
            delete pcmove;
            pcmove = NULL;
        }
        InitLinMove(pos, angle, sector);

        SetPosition(pos,angle,sector);
        MulticastDRUpdate();

        if(pcmesh->GetMesh())
        {
            UpdateProxList(true);
            return true;
        }
    }
    return false;
}

/** Get the z velocity of the actor.
 */
float gemActor::GetVelocity()
{
    csVector3 vel = pcmove->GetVelocity();
    return vel.z;
}

bool gemActor::InsideGuardedArea(gemObject* object)
{
    // Is inside own guard range?
    if(RangeTo(object) <= RANGE_TO_GUARD)
    {
        return true;
    }
    else
    {
        // Check if inside familiar/pet range?
        // TODO: Add some skill check here?
        Client* client = GetClient();
        if(client)
        {
            gemActor* familiar = client->GetFamiliar();
            if(familiar && (familiar->RangeTo(object) <= RANGE_TO_GUARD))
            {
                return true;
            }
            else
            {
                size_t num = client->GetNumPets();
                for(size_t n = 0; n < num; n++)
                {
                    gemActor* pet = client->GetPet(n);
                    if(pet && (pet->RangeTo(object) <= RANGE_TO_GUARD))
                    {
                        return true;
                    }
                }
            }

        }
    }
    return false;
}


/* gemActor::ChatHistoryEntry function implementations */

/// ChatHistoryEntry(const char*, time_t = 0)
/// Info: Constructor. When no time is given, current time is used.
gemActor::ChatHistoryEntry::ChatHistoryEntry(const char* szLine, time_t t)
    : _time(t?t:time(0)), _line(szLine) {}

/// void GetLogLine(csString&) const
/// Info: Prepends a string representation of the time to this chat line
/// so it can be written to a log file. The resulting line is
/// written to 'line' argument (reference).
/// Note: this function also applies \n to the line end.
void gemActor::ChatHistoryEntry::GetLogLine(csString &line) const
{
    tm* gmtm = gmtime(&_time);
    csString cssTime = csString().Format("%d-%02d-%02d %02d:%02d:%02d",
                                         gmtm->tm_year+1900, gmtm->tm_mon+1, gmtm->tm_mday,
                                         gmtm->tm_hour, gmtm->tm_min, gmtm->tm_sec);
    line.Format("[%s] %s\n", cssTime.GetData(), _line.GetData());
}


//--------------------------------------------------------------------------------------

gemNPC::gemNPC(GEMSupervisor* gemsupervisor, CacheManager* cachemanager,
               EntityManager* entitymanager,
               psCharacter* chardata,
               const char* factname,
               InstanceID instance,
               iSector* room,
               const csVector3 &pos,
               float rotangle,
               int clientnum)
    : gemActor(gemsupervisor,cachemanager,entitymanager,chardata,factname,instance,room,pos,rotangle,clientnum)
{
    npcdialog = NULL;
    superClientID = 0;
    nextVeryShortRangeAvail = 0; /// When can npc respond to very short range prox trigger again
    nextShortRangeAvail = 0;     /// When can npc respond to short range prox trigger again
    nextLongRangeAvail = 0;      /// When can npc respond to long range prox trigger again
    speakers = 0;
    busy = false;

    //if( !GetEntity() )
    //{
    //    Error3("Error in GemNPC %s constructor File: %s", factname, filename);
    //    return;
    //}

    pcmove->SetOnGround(true);

    combat_stance = CombatManager::GetStance(cachemanager,"Normal");

    if(chardata->GetOwnerID().IsValid())
    {
        SetOwner(cel->FindPlayerEntity(chardata->GetOwnerID()));
    }
    Debug3(LOG_NPC,GetEID().Unbox(), "Created npc firstname:%s, lastname:%s\n",chardata->GetCharName(), chardata->GetCharLastName());
}

gemNPC::~gemNPC()
{
    Disconnect();
    delete npcdialog;
    npcdialog = NULL;
}

void gemNPC::SetCombatStance(const Stance &stance)
{
    // NPCs don't have stances yet
    // Stance never changes from PSCHARACTER_STANCE_NORMAL for NPCs
}

void gemNPC::SetPosition(const csVector3 &pos,float angle, iSector* sector)
{
    gemActor::SetPosition(pos,angle,sector);
    UpdateProxList(true);
}

void gemNPC::SetupDialog(PID npcID, PID masterNpcID, bool force)
{
    if(!force)
        force = dict->FindKnowledgeArea(name);  // present in Quest Script but not in the queried table

    if(force || db->SelectSingleNumber("SELECT count(*) FROM npc_knowledge_areas WHERE player_id=%d", npcID.Unbox()) > 0
            || (masterNpcID != 0 && npcID != masterNpcID && db->SelectSingleNumber("SELECT count(*) FROM npc_knowledge_areas WHERE player_id=%d", masterNpcID.Unbox()) > 0))
    {
        npcdialog = new psNPCDialog(this);
        if(npcdialog->Initialize(db,npcID))
        {
            //we load the inherited KA after because this makes them essentially of **lower priority** as the array
            //in psnpcdialog, even if it supports ordering, is never ordered, and relies on mysql ordering.
            if(masterNpcID != 0 && npcID != masterNpcID)
                npcdialog->LoadKnowledgeAreas(masterNpcID);
        }
        else
        {
            Error2("Failed to initialize NPC dialog for %s\n", ShowID(npcID));
        }
        if(force)
        {
            csString newArea(name);
            newArea.Downcase();
            newArea.Trim();
            npcdialog->AddKnowledgeArea(newArea);
        }
    }
}

void gemNPC::ReactToPlayerApproach(psNPCCommandsMessage::PerceptionType type,gemActor* player)
{
    // printf("%s received Player approach message %d about player %s.\n",
    //        GetName(), type, player->GetName() );
    csTicks* which_avail = NULL;  // points to 1 of 3 times in gemNPC

    if(npcdialog)
    {
        csString trigger;
        csTicks now = csGetTicks();
        switch(type)
        {
            case psNPCCommandsMessage::PCPT_LONGRANGEPLAYER:

                if(now > nextLongRangeAvail)
                {
                    trigger = "!longrange";
                    which_avail = &nextLongRangeAvail;
                    nextLongRangeAvail = now + (psserver->GetRandom(30) + 10) * 1000;
                }
                break;

            case psNPCCommandsMessage::PCPT_SHORTRANGEPLAYER:

                if(now > nextShortRangeAvail)
                {
                    trigger = "!shortrange";
                    which_avail = &nextShortRangeAvail;
                    nextShortRangeAvail = now + (psserver->GetRandom(30) + 10) * 1000;
                }
                break;

            case psNPCCommandsMessage::PCPT_VERYSHORTRANGEPLAYER:

                if(now > nextVeryShortRangeAvail)
                {
                    trigger = "!veryshortrange";
                    which_avail = &nextVeryShortRangeAvail;
                    nextVeryShortRangeAvail = now + (psserver->GetRandom(30) + 10) * 1000;
                }
                break;

            default:

                trigger = "!anyrange";
                which_avail = &now;
                break;
        }

        if(which_avail)
        {
            NpcResponse* resp = npcdialog->FindXMLResponse(player->GetClient(),trigger);
            if(resp)
            {
                resp->ExecuteScript(player, this);
            }
            else // not found, so don't try again to find it later
            {
                *which_avail += 3600 * 1000;  // only recheck once an hour if not found the first time
            }
        }
    }
}

void gemNPC::ShowPopupMenu(Client* client)
{
    RegisterSpeaker(client);

    NpcResponse* resp = NULL;
    NpcDialogMenu menu;

    csArray<QuestAssignment*> &quests = client->GetCharacterData()->GetQuestMgr().GetAssignedQuests();

    // Merge current spot in active quests first
    for(size_t i=0; i < quests.GetSize(); i++)
    {
        quests[i]->GetQuest();
        // If the quest is completed or the last response was not from this NPC, then skip
        if(quests[i]->last_response_from_npc_pid != pid || quests[i]->status == 'C')
        {
            // printf("Skipping completed or irrelevant quest: %s\n", q->GetName() );
            continue;
        }
        // printf("Checking quest %lu: %s.  ", (unsigned long) i, q->GetName() );
        int last_response = quests[i]->last_response;
        // printf("Got last response %d\n", last_response);

        if(last_response != -1)  // within a quest step
        {
            resp = dict->FindResponse(last_response);
            if(resp) //TODO: this happens needs some investigation on the reason
                menu.Add(resp->menu);
            else
                Error4("BUG: resp was null! last_response %d in %s (%c)", last_response, quests[i]->GetQuest()->GetName(), quests[i]->status);
        }
        else
        {
            // printf("Got last_response==-1 for quest %lu.\n", (unsigned long) i);
        }
    }

    // Also offer default choices in case a new quest should be started
    NpcDialogMenu* npcmenu = dict->FindMenu(name);
    if(npcmenu)
    {
        menu.Add(npcmenu);
    }

    if(menu.triggers.GetSize())
    {
        menu.ShowMenu(client,0, this);
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "This NPC has no quest for you, but might have other things to say.");
    }
}

void gemNPC::SetOwner(gemObject* newOwner)
{
    if(newOwner)
    {
        owner = newOwner;
        GetCharacterData()->SetOwnerID(newOwner->GetCharacterData()->GetPID());
    }
    else
    {
        owner = NULL;
    }
}

csString gemNPC::GetDefaultBehavior(const csString &dfltBehaviors)
{
    int behNum;
    if(psChar->IsMerchant())
        behNum = 2;
    else if(IsAlive() && GetCharacterData()->GetImperviousToAttack() && GetNPCDialogPtr() != NULL)
    {
        behNum = 6;
        //temporary code to support old clients. when netbumping move this to a more appropriate number.
        //and remove the two lines under this
        if(::GetDefaultBehavior(dfltBehaviors, behNum) == "")
            return csString("talk");
    }
    else if(IsAlive())
        behNum = 3;
    else
        behNum = 4;

    return ::GetDefaultBehavior(dfltBehaviors, behNum);
}

void gemNPC::SendBehaviorMessage(const csString &msg_id, gemObject* obj)
{
    gemActor* actor = obj->GetActorPtr();
    CS_ASSERT(actor);
    unsigned int client = actor->GetClientID();
    if(msg_id == "select")
    {
        // If the player is in range of the item.
        if(RangeTo(actor) < RANGE_TO_SELECT)
        {
            int options = 0;

            psGUIInteractMessage guimsg(client,options);
            guimsg.SendMessage();
        }

        return;
    }
    else if(msg_id == "context")
    {
        // If the player is in range of the item and if player is alive.
        if(RangeTo(actor) <= RANGE_TO_SELECT && actor->IsAlive())
        {
            int options = 0;

            // Pet?
            if(psChar->IsPet())
            {
                // Mine?
                if(psChar->GetOwnerID() == actor->GetCharacterData()->GetPID())
                {
                    options |= psGUIInteractMessage::VIEWSTATS;
                    options |= psGUIInteractMessage::DISMISS;

                    // If we are in a peaceful mode we can possibly do some trading.
                    if(actor->GetMode() == PSCHARACTER_MODE_PEACE)
                        options |= psGUIInteractMessage::GIVE;

                    //If we are alive then we can talk with an NPC
                    if(IsAlive())
                    {
                        options |= psGUIInteractMessage::NPCTALK;
                        if(psChar->IsMount() &&
                                psChar->GetOwnerID() == actor->GetCharacterData()->GetPID())
                        {
                            if(actor->GetMount())
                                options |= psGUIInteractMessage::UNMOUNT;
                            else
                                options |= psGUIInteractMessage::MOUNT;
                        }
                    }
                }
                else
                    options |= psGUIInteractMessage::PLAYERDESC;
            }
            else if(psChar->IsMount())
            {
                // Mine? for now bypass
                if(true)  //(psChar->GetOwnerID() == actor->GetCharacterData()->GetPID())
                {
                    //bypass viewstat for now as mounts aren't owned
                    //options |= psGUIInteractMessage::VIEWSTATS;
                    options |= psGUIInteractMessage::PLAYERDESC;
                    if(actor->GetMount())
                        options |= psGUIInteractMessage::UNMOUNT;
                    else
                        options |= psGUIInteractMessage::MOUNT;

                    // If we are in a peaceful mode we can possibly do some trading.
                    if(actor->GetMode() == PSCHARACTER_MODE_PEACE)
                        options |= psGUIInteractMessage::GIVE;

                    //If we are alive then we can talk with an NPC
                    if(IsAlive())
                        options |= psGUIInteractMessage::NPCTALK;
                }
                else
                    options |= psGUIInteractMessage::PLAYERDESC;
            }
            else // Normal NPCs
            {
                options |= psGUIInteractMessage::PLAYERDESC;

                // Loot a dead character
                if(!IsAlive())
                {
                    options |= psGUIInteractMessage::LOOT;
                }

                // If we are in a peaceful mode we can possibly do some trading.
                if(ExchangeManager::ExchangeCheck(actor->GetClient(), this))
                {
                    options |= psGUIInteractMessage::GIVE; // Exchange
                }

                // Bank with a banker
                if((actor->GetMode() == PSCHARACTER_MODE_PEACE) && psChar->IsBanker() && IsAlive())
                {
                    options |= psGUIInteractMessage::BANK;
                }

                // Trade with a merchant
                if(ServerCharManager::TradingCheck(actor->GetClient(), this))
                {
                    options |= psGUIInteractMessage::BUYSELL; // Traide
                }

                // If we are in a peaceful mode we can possibly do some training.
                if((actor->GetMode() == PSCHARACTER_MODE_PEACE) && psChar->IsTrainer() && IsAlive())
                {
                    options |= psGUIInteractMessage::TRAIN;
                }

                // If we are alive then we can talk with an NPC
                if(IsAlive())
                {
                    options |= psGUIInteractMessage::NPCTALK;
                }

                // Can we attack this NPC?
                csString msg; // Not used
                if(IsAlive() && actor->IsAllowedToAttack(this,msg))
                {
                    options |= psGUIInteractMessage::ATTACK;
                }

                if(IsAlive() && psChar->IsStorage())
                {
                    options |= psGUIInteractMessage::STORAGE;
                }
            }

            // Check if there is something to send
            if(!options)
                return;

            // Always possible to close menu
            options |= psGUIInteractMessage::CLOSE;

            psGUIInteractMessage guimsg(client,options);
            guimsg.SendMessage();
        }
    }
    else if(msg_id == "buysell")
        psserver->GetCharManager()->BeginTrading(actor->GetClient(), this, "sell");
    else if(msg_id == "give")
        psserver->exchangemanager->StartExchange(actor->GetClient(), false);
    else if(msg_id == "playerdesc")
        psserver->usermanager->SendCharacterDescription(actor->GetClient(),
                this, false, false, "behaviorMsg");
    else if(msg_id == "attack")
        psserver->usermanager->Attack(CombatManager::GetStance(cacheManager,"Normal"), actor->GetClient());
    else if(msg_id == "loot")
        psserver->usermanager->Loot(actor->GetClient());
    else if(msg_id == "talk")
        ShowPopupMenu(actor->GetClient());
}

void gemNPC::AddLootablePlayer(PID playerID)
{
    lootablePlayers.Push(playerID);
}

void gemNPC::RemoveLootablePlayer(PID playerID)
{
    for(size_t i = 0; i < lootablePlayers.GetSize(); i++)
    {
        if(lootablePlayers[i] == playerID)
            lootablePlayers[i] = -1; // Should not be possible to find a player with this id.
        // Fast and client is removed from table;
    }
}


bool gemNPC::IsLootablePlayer(PID playerID)
{
    for(size_t i = 0; i < lootablePlayers.GetSize(); i++)
    {
        if(lootablePlayers[i] == playerID)
            return true;
    }
    return false;
}

Client* gemNPC::GetRandomLootClient(int range)
{
    if(lootablePlayers.GetSize() == 0)
        return NULL;

    csArray<PID> temp;
    int which = psserver->rng->Get((int)lootablePlayers.GetSize());
    int first = which;

    do
    {
        if(lootablePlayers[which] != -1)
            temp.Push(lootablePlayers[which]);

        ++which;
        if(which == (int)lootablePlayers.GetSize())
            which = 0;
    }
    while(which != first);

    Client* found;

    for(size_t i = 0; i < temp.GetSize(); i++)
    {
        found = EntityManager::GetSingleton().GetClients()->FindPlayer(temp[i]);

        if(found && found->GetActor() &&
                found->GetActor()->RangeTo(this) < range)
        {
            return found;
        }
    }

    return NULL;
}

void gemNPC::Say(const char* sayText, Client* who, bool sayPublic, csTicks &timeDelay)
{
    int chtype = CHAT_NPC; // Default type is NPC.

    if(!sayText)
    {
        return; // Nothing to say so return
    }

    if(who && !sayPublic)  // specific client specified means use /tell not /say
    {
        Notify2(LOG_NPC,"Private NPC Response: %s\n", sayText);

        // Some NPC responses are now in the form of private tells.
        psChatMessage newMsg(who->GetClientNum(), eid, GetName(), 0, sayText, chtype, false);

        // first response gets 1 second delay to simulate NPC thinking
        // subsequent ones add to the current time delay, and send delayed
        if(timeDelay==0)
            timeDelay = (csTicks)(1000);
        psserver->GetEventManager()->SendMessageDelayed(newMsg.msg,timeDelay);

        timeDelay += (csTicks)(2000 + 50*strlen(sayText));
    }
    else
    {
        Notify2(LOG_NPC,"Public NPC Response: %s\n", sayText);
        // Some NPC responses are now in the form of public /says.
        psChatMessage newMsg(0, eid, GetName(), 0, sayText, chtype, false);
        newMsg.Multicast(GetMulticastClients(), 0, CHAT_SAY_RANGE);
    }

    if(who)
    {
        // This perception allows the superclient to know about the dialog with a specific person.
        psserver->GetNPCManager()->QueueTalkPerception(who->GetActor(), this);
    }
}

void gemNPC::ActionCommand(bool actionMy, bool actionNarrate, const char* actText, Client* who, bool actionPublic, csTicks &timeDelay)
{
    int chtype = CHAT_NPC_ME; // Default type is ME.

    if(actionMy)
    {
        chtype = CHAT_NPC_MY;
    }
    else if(actionNarrate)
    {
        chtype = CHAT_NPC_NARRATE;
    }

    if(who && !actionPublic)
    {
        Notify2(LOG_NPC,"Private NPC action: %s\n", actText);

        // Some NPC actions are now in the form of private actions.
        psChatMessage msg(who->GetClientNum(), eid, GetName(), 0, actText, chtype, false);

        // first response gets 1 second delay to simulate NPC thinking
        // subsequent ones add to the current time delay, and send delayed
        if(timeDelay==0)
            timeDelay = (csTicks)(1000);
        psserver->GetEventManager()->SendMessageDelayed(msg.msg,timeDelay);

        timeDelay += (csTicks)(1000 + 30*strlen(actText));
    }
    else
    {
        Notify2(LOG_NPC,"Public NPC action: %s\n", actText);
        // Some NPC actions are now in the form of public /me, or /mys
        psChatMessage msg(0, eid, GetName(), 0, actText, chtype, false);
        msg.Multicast(GetMulticastClients(), 0, CHAT_SAY_RANGE);
    }
}

void gemNPC::AddBadText(const char* playerSaid, const char* trigger)
{
    for(size_t i=0; i< badText.GetSize(); i++)
    {
        if(badText[i]->said == playerSaid)
        {
            badText[i]->count++;
            badText[i]->when = csGetTicks();
            return;
        }
    }
    DialogCounter* dlg = new DialogCounter;
    dlg->count   = 1;
    dlg->said    = playerSaid;
    dlg->trigger = trigger;
    dlg->when    = csGetTicks();
    badText.Push(dlg);
    badText.Sort(&gemNPC::DialogCounter::Compare);
    if(badText.GetSize() > 64)
        delete badText.Pop();  // never let >64 items in
}

void gemNPC::GetBadText(size_t first,size_t last, csStringArray &saidArray, csStringArray &trigArray)
{
    saidArray.Empty();
    trigArray.Empty();

    for(size_t i=first-1; i<last; i++)
    {
        if(i>=badText.GetSize())
            continue;

        saidArray.Push(badText[i]->said);
        csString temp = badText[i]->trigger;
        temp.AppendFmt("(%d)", badText[i]->count);
        trigArray.Push(temp);
    }
}

bool gemNPC::Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities)
{
    csString texparts;
    csString equipmentParts;
    EID ownerEID;

    psChar->MakeTextureString(texparts);
    psChar->MakeEquipmentString(equipmentParts);

    csString guildName;
    guildName.Clear();
    if(psChar->GetGuild() && !psChar->GetGuild()->IsSecret())
        guildName = psChar->GetGuild()->GetName();

    if(this->owner.IsValid())
    {
        ownerEID = owner->GetEID();
    }

    uint32_t flags = psPersistActor::NPC;

    if(!GetVisibility())    flags |= psPersistActor::INVISIBLE;
    if(GetInvincibility())  flags |= psPersistActor::INVINCIBLE;
    if(IsAlive())           flags |= psPersistActor::IS_ALIVE;

    Client* targetClient = psserver->GetConnections()->Find(clientnum);
    if(targetClient && targetClient->GetCharacterData())
    {
        flags |= psPersistActor::NAMEKNOWN;

        // Enable to enable introductions between player and NPCs
        /*if ((targetClient->GetSecurityLevel() >= GM_LEVEL_0) ||
              targetClient->GetCharacterData()->Knows(psChar))
                                                             flags |= psPersistActor::NAMEKNOWN;*/
    }

    psPersistActor mesg(
        clientnum,
        securityLevel,
        masqueradeLevel,
        control,
        name,
        guildName,
        psChar->GetRaceInfo()->GetMeshName(),
        psChar->GetRaceInfo()->GetTextureName(),
        psChar->GetRaceInfo()->GetName(),
        GetMount() ? GetMount()->GetRaceInfo()->mesh_name : "null",
        GetMount() ? GetMount()->GetRaceInfo()->GetMounterAnim() : "null",
        psChar->GetRaceInfo()->gender,
        psChar->GetScale(),
        GetMount() ? GetMount()->GetRaceInfo()->GetScale() : 1,
        psChar->GetHelmGroup(),
        psChar->GetBracerGroup(),
        psChar->GetBeltGroup(),
        psChar->GetCloakGroup(),
        top, bottom,offset,
        texparts,
        equipmentParts,
        DRcounter,
        eid,
        cacheManager->GetMsgStrings(),
        pcmove,
        movementMode,
        GetMode(),
        (to_superclients || allEntities) ? pid : 0, // playerID should not be distributed to clients
        0, // groupID
        ownerEID,
        flags,
        (to_superclients || allEntities) ? psChar->GetMasterNPCID() : 0,
        (to_superclients || allEntities)
    );

    if(clientnum && !to_superclients)
    {
        mesg.SendMessage();
    }

    if(to_superclients)
    {
        mesg.SetInstance(GetInstance());
        if(clientnum == 0)  // Send to all superclients
        {
            Debug1(LOG_SUPERCLIENT, clientnum, "Sending gemNPC to superclients.\n");
            mesg.Multicast(psserver->GetNPCManager()->GetSuperClients(),0,PROX_LIST_ANY_RANGE);
        }
        else
        {
            mesg.SendMessage();
        }
    }
    if(allEntities)
    {
        return allEntities->AddEntityMessage(mesg.msg);
    }

    return true;
}

void gemNPC::Broadcast(int clientnum, bool control)
{
    csArray<PublishDestination> &dest = GetMulticastClients();
    for(unsigned long i = 0 ; i < dest.GetSize(); i++)
    {
        if(dest[i].dist < PROX_LIST_ANY_RANGE)
            Send(dest[i].client, control, false);
    }

    csArray<PublishDestination> &destSuper = psserver->GetNPCManager()->GetSuperClients();
    for(unsigned long i = 0 ; i < destSuper.GetSize(); i++)
    {
        if(destSuper[i].dist < PROX_LIST_ANY_RANGE)
            Send(destSuper[i].client, control, true);
    }

}

void gemNPC::SendGroupStats()
{
    unsigned int statsDirtyFlags = GetCharacterData()->GetStatsDirtyFlags();

    // Distribute PET stats to owner client
    if(GetCharacterData()->IsPet())
    {
        // Get Client ID of Owner
        gemObject* owner = GetOwner();
        if(owner && owner->GetClientID())
        {
            GetCharacterData()->SendStatDRMessage(owner->GetClientID(), eid, statsDirtyFlags);
        }
    }

    // Now call the actor function to push stat updates to groups
    // and clients that target this entity.
    gemActor::SendGroupStats();


    // Now that every thing is updated make sure no stats are dirty
    // NPCs that are not targeted will not have a StatDRMessage sent
    // and need these stats dirty flags to be cleared or they will
    // be updated each second.
    GetCharacterData()->ClearStatsDirtyFlags(statsDirtyFlags);
}

void gemNPC::ForcePositionUpdate()
{
    psAllEntityPosMessage msg;
    //we send just one position update
    msg.SetLength(1,0);
    iSector* sector = GetSector();
    csVector3 position = GetPosition();
    csTicks now = csGetTicks();
    //we add the data and flag this position update as forced
    msg.Add(GetEID(), position, sector, GetInstance(),
            cacheManager->GetMsgStrings(),true);
    SetLastSuperclientPos(GetPosition(),GetInstance(),now);
    //send this to all npcclients
    msg.Multicast(psserver->GetNPCManager()->GetSuperClients(),-1,PROX_LIST_ANY_RANGE);
}


class psCheckSpeakerEvent : public psGameEvent
{
public:
    psCheckSpeakerEvent(EID eid)
        : psGameEvent(0, 65000,"psCheckSpeakerEvent"), eid(eid)
    {

    }

    virtual void Trigger()
    {
        gemNPC* npc = psserver->entitymanager->GetGEM()->FindNPCEntity(eid);
        if(npc)
        {
            npc->CheckSpeakers();
        }
    }

private:

    EID eid;
};

void gemNPC::RegisterSpeaker(Client* client)
{
    bool first = speakers <= 0;

    speakers++;

    if(first)
    {
        psserver->GetNPCManager()->QueueSpokenToPerception(this, true);
    }

    psCheckSpeakerEvent* event = new psCheckSpeakerEvent(GetEID());
    event->QueueEvent();
}


void gemNPC::CheckSpeakers()
{
    bool last = (speakers == 1);

    speakers--;

    if(speakers <= 0)
    {
        speakers = 0;
    }

    if(last)
    {
        psserver->GetNPCManager()->QueueSpokenToPerception(this, false);
    }
}

void gemNPC::SetBusy(bool busy)
{
    this->busy = busy;
}

bool gemNPC::IsBusy() const
{
    return busy;
}


