/*
 * entitymanager.h
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
#ifndef __EntityManager_H__
#define __EntityManager_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/vector3.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"
#include "util/psconst.h"
#include "util/singleton.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

struct iSector;
struct iEngine;

class ClientConnectionSet;
class psServerDR;
class psDatabase;
class psWorld;
class psItem;
class GEMSupervisor;
class UserManager;
class gemItem;
class gemNPC;
class gemActor;
class psActionLocation;
class psCharacter;
class psSectorInfo;
class psMovementInfoMessage;


struct psAffinityAttribute
{
public:
    csString Attribute;
    csString Category;
};

struct psFamiliarType
{
public:
    PID Id;
    csString Name;
    csString Type;
    csString Lifecycle;
    csString AttackTool;
    csString AttackType;
    csString MagicalAffinity;
};

/// Manages CEL entities on the server
class EntityManager : public MessageManager<EntityManager>, public Singleton<EntityManager>
{
public:
    EntityManager();
    virtual ~EntityManager();

    bool Initialize(iObjectRegistry* object_reg,
                    ClientConnectionSet* clients,
                    UserManager* usermanager,
                    GEMSupervisor* gemsupervisor,
                    psServerDR* psserverdr,
                    CacheManager* cachemanager);

    void HandleUserAction(MsgEntry* me, Client* client);
    void HandleWorld(MsgEntry* me, Client* client);
    void HandleActor(MsgEntry* me, Client* client);
    void HandleAllRequest(MsgEntry* me, Client* client);
    void SendMovementInfo(MsgEntry* me, Client* client);

    bool LoadMap(const char* mapname);

    iSector* FindSector(const char* name);

    bool CreatePlayer(Client* client);
    bool DeletePlayer(Client* client);

    PID CopyNPCFromDatabase(PID master_id, float x, float y, float z, float angle, const csString &sector, InstanceID instance, const char* firstName = NULL, const char* lastName = NULL);
    EID CreateNPC(PID npcID, bool updateProxList = true, bool alwaysWatching = false);
    EID CreateNPC(psCharacter* chardata, bool updateProxList = true, bool alwaysWatching = false);
    EID CreateNPC(psCharacter* chardata, InstanceID instance, csVector3 pos, iSector* sector, float yrot, bool updateProxList = true, bool alwaysWatching = false);

    /** Create a new familiar NPC.
     */
    gemNPC* CreateFamiliar(gemActor* owner, PID masterPID);
    
    /** Create a new hired NPC.
     */
    gemNPC* CreateHiredNPC(gemActor* owner, PID masterPID, const csString& name);
    
    /** Clone a NPC.
     *  Used by the tribe system to generate new mebers.
     */
    gemNPC* CloneNPC(psCharacter* chardata);

    bool CreateActionLocation(psActionLocation* instance, bool transient);

    gemItem* CreateItem(psItem* iteminstance, bool transient, int tribeID = 0);
    gemItem* MoveItemToWorld(psItem*       keyItem,
                             InstanceID  instance,
                             psSectorInfo* sectorinfo,
                             float         loc_x,
                             float         loc_y,
                             float         loc_z,
                             float         loc_xrot,
                             float         loc_yrot,
                             float         loc_zrot,
                             psCharacter*  owner,
                             bool          transient);

    bool RemoveActor(gemObject* actor);

    /** Delete an actor from the world.
     *
     *  Entity is removed from world and deleted from DB. If the actor
     *  is a NPC a command is sent to NPC Clients to delte any records
     *  there as well.
     */
    bool DeleteActor(gemObject* actor);

    bool AddRideRelation(gemActor* rider, gemActor* mount);
    void RemoveRideRelation(gemActor* rider);

    void SetReady(bool flag);
    bool IsReady()
    {
        return ready;
    }
    bool HasBeenReady()
    {
        return hasBeenReady;
    }
    GEMSupervisor* GetGEM()
    {
        return gem;
    }
    iEngine* GetEngine()
    {
        return engine;
    }

    ClientConnectionSet* GetClients()
    {
        return clients;
    };
    psWorld* GetWorld()
    {
        return gameWorld;
    }

protected:
    csHash<psAffinityAttribute*> affinityAttributeList;
    csHash<psFamiliarType*, PID> familiarTypeList;

    bool SendActorList(Client* client);


    void CreateMovementInfoMsg();
    void LoadFamiliarTypes();
    void LoadFamiliarAffinityAttributes();
    PID GetMasterFamiliarID(psCharacter* charData);
    int CalculateFamiliarAffinity(psCharacter* chardata, size_t type, size_t lifecycle, size_t attacktool, size_t attacktype);


    bool ready;
    bool hasBeenReady;
    psServerDR* serverdr;
    ClientConnectionSet* clients;
    psDatabase* database;
    UserManager* usermanager;
    CacheManager* cacheManager;
    GEMSupervisor* gem;
    iEngine* engine;
    psWorld* gameWorld;

    psMovementInfoMessage* moveinfomsg;
};

class psEntityEvent : public psGameEvent
{
public:
    enum EventType
    {
        DESTROY = 0
    };

    psEntityEvent(EntityManager* entitymanager, EventType type, gemObject* object) :
        psGameEvent(0, 0, "psEntityEvent"), type(type), object(object)
    {
        this->entitymanager = entitymanager;
    }

    virtual void Trigger()
    {
        switch(type)
        {
            case DESTROY:
            {
                entitymanager->RemoveActor(object);
                break;
            }
        }
    }

private:
    EventType type;
    gemObject* object;
    EntityManager* entitymanager;
};

#endif

