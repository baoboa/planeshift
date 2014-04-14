/*
 * gem.h - author Keith Fulton <keith@paqrat.com>
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
 * This is the cel access class for PS.
 *
 * TODO: This API really needs to be refactored into a set of factories.
 */

#ifndef __GEM_H__
#define __GEM_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/mesh.h>
#include <iengine/sector.h>
#include <iutil/vfs.h>
#include <csutil/csobject.h>
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/weakreferenced.h>

//=============================================================================
// Project Space Includes
//=============================================================================
#include "bulkobjects/activespell.h"
#include "bulkobjects/buffable.h"
#include "bulkobjects/pscharacter.h"

#include "util/gameevent.h"
#include "util/consoleout.h"

#include "net/npcmessages.h"  // required for psNPCCommandsMessage::PerceptionType

//=============================================================================
// Local Space Includes
//=============================================================================
#include "msgmanager.h"
#include "deathcallback.h"

struct iMeshWrapper;

class ProximityList;
class ServerCharManager;
class EntityManager;
class gemObject;
class PlayerGroup;
class psDatabase;
class psItem;
class csMatrix3;
class NPCManager;
class psGlyphList;
class ProgressionManager;
class psNPCDialog;
class psAllEntityPosMessage;
class psActionLocation;
class psSpellCastEvent;
class MathScript;
class gemItem;
class gemActor;
class gemNPC;
class gemPet;
class gemActionLocation;
class ClientConnectionSet;
class PublishVector;
class psLinearMovement;
class gemMesh;

/**
 * \addtogroup server
 * @{ */

#define BUFF_INDICATOR          "+"
#define DEBUFF_INDICATOR        "-"

#define UNSTICK_TIME 15000


//-----------------------------------------------------------------------------

/**
 * Helper class to attach a PlaneShift gem object to a particular mesh.
 */
class psGemServerMeshAttach : public scfImplementationExt1<psGemServerMeshAttach,
    csObject,
    scfFakeInterface<psGemServerMeshAttach> >
{
public:
    SCF_INTERFACE(psGemServerMeshAttach, 0, 0, 1);

    /**
     * Setup this helper with the object we want to attach with.
     *
     * @param object  The gemObject we want to attach to a mesh.
     */
    psGemServerMeshAttach(gemObject* object);

    /**
     * Get the gemObject that the mesh has attached.
     */
    gemObject* GetObject()
    {
        return object;
    }

private:
    gemObject* object;          ///< The object that is attached to a iMeshWrapper object.
};

/**
* This class holds the refs to the core factories, etc in CEL.
*/
class GEMSupervisor : public MessageManager<GEMSupervisor>
{
    EntityManager* entityManager;
    CacheManager* cacheManager;
public:
    iObjectRegistry*        object_reg;                     ///< The Crystal Space Object Registry.
    psDatabase*             database;                       ///< The main PlaneShift database object.
    NPCManager*             npcmanager;                     ///< NPC controller.

    /**
     * Create a new singleton of the GEM Supervisor object.
     *
     * @param objreg The Crystal Space Object Registry.
     * @param db The main PlaneShift database.
     * @param entitymanager The EntityManager.
     * @param cachemanager The CacheManager.
     */
    GEMSupervisor(iObjectRegistry* objreg, psDatabase* db, EntityManager* entitymanager, CacheManager* cachemanager);

    /**
     * Destroy this singleton.
     *
     * This will only be ever called on a server shutdown.
     */
    virtual ~GEMSupervisor();

    /**
     * Get a hash of all the current entities on the server.
     *
     * @return a csHash of all the gemObjects.
     */
    csHash<gemObject*, EID> &GetAllGEMS()
    {
        return entities_by_eid;
    }

    /** @name Search functions
     */
    ///@{
    /**
     * Find an entity ID (EID) for an item.
     *
     * @param item The psItem that we want to find the entity ID for.
     * @return the EID of that item if it was found. 0 if no id could be found.
     */
    EID        FindItemID(psItem* item);

    gemObject* FindObject(EID cel_id);

    gemObject* FindObject(const csString &name);

    gemActor*  FindPlayerEntity(PID player_id);

    gemNPC*    FindNPCEntity(PID npc_id);

    gemNPC*    FindNPCEntity(EID eid);

    gemItem*   FindItemEntity(uint32 item_id);
    ///@}

    EID  CreateEntity(gemObject* obj);
    void AddEntity(gemObject* obj, EID objEid); ///< Ugly function, used for gemAL
    void RemoveEntity(gemObject* which);
    void AddActorEntity(gemActor* actor);
    void RemoveActorEntity(gemActor* actor);
    void AddItemEntity(gemItem* item);
    void RemoveItemEntity(gemItem* item, uint32 uid);

    void RemovePlayerFromLootables(PID playerID);

    void UpdateAllDR();
    void UpdateAllStats();

    void GetAllEntityPos(csArray<psAllEntityPosMessage> &msgs);
    int  CountManagedNPCs(AccountID superclientID);
    void FillNPCList(MsgEntry* msg, AccountID superclientID);
    void SendAllNPCStats(AccountID superclientID);
    void ActivateNPCs(AccountID superclientID);
    void StopAllNPCs(AccountID superclientID);

    /**
     * Gets a list of all the 'live' entities that this player has ownership of.
     *
     * This can be things like items in containers or work items.
     *
     * @param playerID The character owner ID we are looking for.
     * @param list The populated list of items that are active in game.
     */
    void GetPlayerObjects(PID playerID, csArray<gemObject*> &list);

    /**
     * Teleport a player to a location.
     *
     * @param object     The player to move
     * @param x          X Location to move to
     * @param y          Y Location to move to
     * @param z          Z Location to move to
     * @param rot        The rotation to use.
     * @param sectorname The sector name to move to.
     */
    void Teleport(gemObject* object, float x, float y, float z, float rot, const char* sectorname);

    void HandleStatsMessage(MsgEntry* me, Client* client);
    void HandleDamageMessage(MsgEntry* me, Client* client);
    void HandleStatDRUpdateMessage(MsgEntry* me, Client* client);

    /**
     * Attach a server gemObject to a Crystal Space object.
     *
     * In most cases this will be a mesh wrapper.
     *
     * @param object The Crystal Space object we want to attach our gemObject to.
     * @param gobject The PlaneShift object that we want to attach.
     */
    void AttachObject(iObject* object, gemObject* gobject);


    /**
     * Unattach a gemObject from a Crystal Space object.
     *
     * In most cases the Crystal Space object is a meshwrapper.
     *
     * @param object The Crystal Space object we want to unattach our client object from.
     * @param gobject The gem object we want to unattach.
      */
    void UnattachObject(iObject* object, gemObject* gobject);


    /**
     * See if there is a gemObject attached to a given Crystal Space object.
     *
     * @param object The Cyrstal Space object we want to see if there is an object attached to.
     * @return A gemObject if it exists that is attached to the Crystal Space object.
     */
    gemObject* FindAttachedObject(iObject* object);


    /**
     * Create a list of all nearby gem objects.
     *
     * @param sector The sector to check in.
     * @param pos The starting position
     * @param instance The instance ID for the starting point
     * @param radius The distance around the starting point to check.
     * @param doInvisible If true check invisible meshes otherwise ignore them.
     *
     * @return A csArray<> of all the objects in the given radius.
     */
    csArray<gemObject*> FindNearbyEntities(iSector* sector, const csVector3 &pos, InstanceID instance, float radius, bool doInvisible = false);

    /**
     * Create a list of all gem objects in a sector.
     *
     * @param sector The sector to check in.
     * @param doInvisible If true check invisible meshes otherwise ignore them.
     *
     * @return A csArray<> of all the objects in the given radius.
     */
    csArray<gemObject*> FindSectorEntities(iSector* sector, bool doInvisible = false);

protected:
    /**
     * Get the next ID for an object.
     *
     * @return The next ID available that can be assigned to an object.
     */
    EID GetNextID();

    csHash<gemObject*, EID> entities_by_eid; ///< A list of all the entities stored by EID (entity/gem ID).
    csHash<gemItem*, uint32> items_by_uid;   ///< A list of all the items stored by UID (psItem ID).
    csHash<gemActor*,  PID> actors_by_pid;   ///< A list of all the actors stored by PID (player/character ID).

    uint32              nextEID;             ///< The next ID available for an object.


    csRef<iEngine> engine;                   ///< Stored here to save expensive csQueryRegistry calls
};

//-----------------------------------------------------------------------------

/**
* A gemObject is any solid, graphical object visible in PS with normal physics
* and normal collision detection.
*/
class gemObject : public iDeleteNotificationObject, public CS::Utility::WeakReferenced, public iScriptableVar
{

public:
    virtual ~gemObject();

    EID GetEID()
    {
        return eid;
    }

    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    virtual const char* ToString()
    {
        return name.GetData();
    }
    ///@}

    /**
     * Called when a client disconnects.
     */
    virtual void Disconnect();

    virtual bool IsValid(void)
    {
        return eid.IsValid();
    }

    /**
     * Returns whether the object is alive.
     */
    bool IsAlive() const
    {
        return is_alive;
    }

    /**
     * Set needed stats for alive or dead for object
     *
     * When dead the mana and HP rate are set to 0.
     *
     * @param flag  True if alive
     * @param queue Set to false to prevent queuing to superclients
     */
    void SetAlive(bool flag, bool queue = true);

    uint32_t GetClientID();

    virtual const char* GetObjectType()
    {
        return "Object";
    }

    gemItem* GetItemPtr();
    gemActor* GetActorPtr();
    gemNPC* GetNPCPtr();
    gemPet* GetPetPtr();
    gemActionLocation* GetALPtr();

    psItem* GetItem();
    virtual psCharacter* GetCharacterData()
    {
        return NULL;
    }

    virtual Client* GetClient() const
    {
        return NULL;
    }

    const char* GetName();
    void SetName(const char* n);

    void SetInstance(InstanceID newInstance)
    {
        worldInstance = newInstance;
    }
    InstanceID  GetInstance()
    {
        return worldInstance;
    }

    void RegisterCallback(iDeleteObjectCallback* receiver)
    {
        receivers.Push(receiver);
    }
    void UnregisterCallback(iDeleteObjectCallback* receiver)
    {
        receivers.Delete(receiver);
    }

    /** @name Mesh related functions
     *
     */
    ///@{

    /**
     *
     */
    iMeshWrapper* GetMeshWrapper();

    /**
     *
     */
    csString GetMesh()
    {
        return factname;
    }

    /**
     *
     */
    void Move(const csVector3 &pos,float rotangle,iSector* room);

    /**
     *
     */
    bool IsNear(gemObject* obj,float radius, bool ignoreY = false);

    /**
     *
     */
    const csVector3 &GetPosition();

    /**
     *
     */
    void GetPosition(csVector3 &pos, float &yrot,iSector* &sector);

    /**
     *
     */
    void GetPosition(csVector3 &pos, iSector* &sector);

    /**
     * Get rotation of entity as returned by psWorld::Matrix2YRot.
     *
     * @return rotation about y axis
     * @see psWorld::Matrix2Yrot
     */
    float GetAngle();

    /**
     *
     */
    iSector* GetSector();

    /**
     * Get the z velocity of the actor.
     */
    virtual float GetVelocity();

    /**
     *
     */
    const char* GetSectorName()
    {
        return GetSector() ? GetSector()->QueryObject()->GetName() : "(null)";
    }

    /**
     *
     */
    csArray<gemObject*>* GetObjectsInRange(float range);
    ///@}

    /** @name Proxlist related functions.
     */
    ///@{

    /**
     *
     */
    ProximityList* GetProxList()
    {
        return proxlist;
    };

    /**
     *
     */
    csArray<PublishDestination> &GetMulticastClients();

    /**
     * Generates proxlist if needed (or forced).
     *
     * Then removes entities of nearby objects at clients, if needed.
     *
     * @param force Force an update if set to true.
     */
    void UpdateProxList(bool force = false);

    /**
     *
     */
    void RemoveFromAllProx();

    /**
     *
     */
    void SetAlwaysWatching(bool w)
    {
        alwaysWatching = w;
    }

    /**
     *
     */
    bool AlwaysWatching()
    {
        return alwaysWatching;
    }
    ///@}

    float RangeTo(gemObject* obj, bool ignoreY = false, bool ignoreInstance = false);

    virtual bool IsUpdateReq(csVector3 const &pos,csVector3 const &oldPos);

    /**
     * This value indicates the range that this entity would become visible
     * to other entities if no other modifiers were taken into consideration.
     */
    virtual float GetBaseAdvertiseRange()
    {
        return DEF_PROX_DIST;
    };

    virtual void SendBehaviorMessage(const csString &str, gemObject* obj);
    virtual csString GetDefaultBehavior(const csString &dfltBehaviors);

    /**
     * Dump debug information.
     *
     * Used in the com_print function.
     */
    virtual void Dump();


    /** @name Networking functions
     */
    ///@{

    /**
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     */
    virtual void Broadcast(int clientnum, bool control);

    /**
     * Send this object to the given client.
     *
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     * @param to_superclients Send to super clients
     * @param allEntities Buffer the message instead of sending it.
     */
    virtual bool Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities=NULL)
    {
        return true;
    }

    /**
     * Distribute this object to all members of a group.
     *
     * @param me The MsgEntry to send to all group members.
     */
    virtual void SendGroupMessage(MsgEntry* me) { };
    ///@}

    /** @name Overridden functions in child classes
     */
    ///@{
    virtual PID GetPID()
    {
        return 0;
    }
    virtual int GetGuildID()
    {
        return 0;
    }
    virtual psGuildInfo* GetGuild()
    {
        return 0;
    }
    virtual bool UpdateDR()
    {
        return false;
    }
    virtual void BroadcastTargetStatDR(ClientConnectionSet* clients) { }
    virtual void SendTargetStatDR(Client* client) { }
    virtual psNPCDialog* GetNPCDialogPtr()
    {
        return 0;
    }
    virtual void GetLastSuperclientPos(csVector3 &pos, InstanceID &instance, csTicks &last) const { }
    virtual void SetLastSuperclientPos(const csVector3 &pos, InstanceID instance, const csTicks &now) { }
    virtual void AddLootablePlayer(PID playerID) { }
    virtual void RemoveLootablePlayer(PID playerID) { }
    virtual bool IsLootablePlayer(PID playerID)
    {
        return false;
    }
    virtual Client* GetRandomLootClient(int range)
    {
        return NULL;
    }
    virtual AccountID GetSuperclientID()
    {
        return 0;
    }
    virtual void SetSuperclientID(AccountID id) { }

    virtual bool GetVisibility()
    {
        return true;
    }
    virtual bool SeesObject(gemObject* object, float range)
    {
        return false;
    }

    virtual gemObject* GetOwner()
    {
        return NULL;
    }

    /**
     * Returns if the object has killsteal protection.
     *
     * Rules about killsteal protection should be implmented here.
     * Generic objects don't have killsteal protection
     *
     * @return A boolean indicating if this gemObject must have killsteal protection.
     */
    virtual bool HasKillStealProtection()
    {
        return false;
    }
    ///@}

protected:
    gemObject(GEMSupervisor* gemsupervisor, EntityManager* entitymanager, CacheManager* cachemanager, const char* name, const char* factname, InstanceID myinstance, iSector* room,
              const csVector3 &pos, float rotangle, int clientnum);

    bool valid;                                 ///< Is object fully loaded

    gemMesh* pcmesh;                            ///< link to mesh class
    ProximityList* proxlist;                    ///< Proximity List for this object
    csString name;                              ///< Name of this object, used mostly for debugging
    static GEMSupervisor* cel;                  ///< Static ptr back to main collection of all objects
    EntityManager* entityManager;
    CacheManager* cacheManager;
    InstanceID worldInstance;                   ///< Only objects which match instances can see each other
    bool is_alive;                              ///< Flag indicating whether object is alive or not
    csString factname;                          ///< Name of CS Mesh Factory used to create this object
    csString matname;                           ///< Name of base material used by this object.
    EID eid;                                    ///< Entity ID (unique identifier for object)
    csRef<iMeshFactoryWrapper> nullfact;        ///< Null factory for our mesh instances.
    bool alwaysWatching;                           ///< True if this object always watches (proxlists) regardless of owner.

    csArray<iDeleteObjectCallback*> receivers;  ///< List of objects which are to be notified when this object is deleted.

    float prox_distance_desired;                ///< What is the maximum range of proxlist we want
    float prox_distance_current;                ///< What is the current actual range for proxlists (they adjust when the # of objects gets too high)

    bool InitProximityList(float radius,int clientnum);

    void InitMesh(const char* name, const csVector3 &pos, const float rotangle, iSector* room);
};

//-----------------------------------------------------------------------------

/**
 * Any PS Object with which a player may have interaction (i.e. clickable).
 */
class gemActiveObject : public gemObject
{
public:
    gemActiveObject(GEMSupervisor* gemSupervisor, EntityManager* entitymanager,CacheManager* cachemanager, const char* name,
                    const char* factname,
                    InstanceID myInstance,
                    iSector* room,
                    const csVector3 &pos,
                    float rotangle,
                    int clientnum);

    virtual const char* GetObjectType()
    {
        return "Active object";
    }

    /**
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     */
    virtual void Broadcast(int clientnum, bool control);

    /**
     * Send this object to the given client.
     *
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     * @param to_superclients Send to super clients
     * @param allEntities Buffer the message instead of sending it.
     */
    virtual bool Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities=NULL)
    {
        return true;
    }

    virtual void SendBehaviorMessage(const csString &str, gemObject* obj);
    virtual csString GetDefaultBehavior(const csString &dfltBehaviors);

    virtual bool IsPickupable()
    {
        return false;
    }
    virtual bool IsPickupableWeak()
    {
        return false;
    }
    virtual bool IsPickupableStrong()
    {
        return false;
    }
    virtual bool IsLockable()
    {
        return false;
    }
    virtual bool IsLocked()
    {
        return false;
    }
    virtual bool IsConstructible()
    {
        return false;
    }
    virtual bool IsSecutityLocked()
    {
        return false;
    }
    virtual bool IsContainer()
    {
        return false;
    }
};

//-----------------------------------------------------------------------------

class gemItem : public gemActiveObject
{
protected:
    psItem* itemdata;
    csString itemType;
    float xRot;
    float yRot;
    float zRot;
    uint32_t tribeID; ///< Holds the id of the tribe owning this item (0 if is not tribe-owned)

public:
    gemItem(GEMSupervisor* gemsupervisor,
            CacheManager* cachemanager,
            EntityManager* entitymanager,
            psItem* item,
            const char* factname,
            InstanceID myInstance,
            iSector* room,
            const csVector3 &pos,
            float xrotangle,
            float yrotangle,
            float zrotangle,
            int clientnum);

    virtual ~gemItem()
    {
        delete itemdata;
    }

    void SetTribeID(uint32_t id)
    {
        tribeID = id;
    }
    uint32_t  GetTribeID()
    {
        return tribeID;
    }

    virtual const char* GetObjectType()
    {
        return itemType.GetData();
    }
    psItem* GetItemData()
    {
        return itemdata;
    }
    void ClearItemData()
    {
        itemdata = NULL;
    }


    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    ///@}

    virtual float GetBaseAdvertiseRange();

    /**
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     */
    virtual void Broadcast(int clientnum, bool control);

    /**
     * Send this object to the given client.
     *
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     * @param to_superclients Send to super clients
     * @param allEntities Buffer the message instead of sending it.
     */
    virtual bool Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities=NULL);

    /**
     * Set position of item in world.
     *
     * @param pos The coordinates of the object in the sector
     * @param angle The y rotation angle
     * @param sector The sector in which the object is
     * @param instance The instance the object is in
     *
     * @return Set the position of the item in given sector, instance and position. Also sets the item y rotation
     */
    virtual void SetPosition(const csVector3 &pos,float angle, iSector* sector, InstanceID instance);

    /**
     * Set the x, y and z axis rotations for the item.
     *
     * @param xrotangle the variable used to set the x rotation of the item
     * @param yrotangle the variable used to set the x rotation of the item
     * @param zrotangle the variable used to set the z rotation of the item
     */
    virtual void SetRotation(float xrotangle, float yrotangle, float zrotangle);

    /**
     * Get the x,y and z axis rotations for the item.
     *
     * @param xrotangle the variable in which the x rotation will be stored
     * @param yrotangle the variable in which the y rotation will be stored
     * @param zrotangle the variable in which the z rotation will be stored
     */
    virtual void GetRotation(float &xrotangle, float &yrotangle, float &zrotangle);

    /**
     * Gets if the object can be picked up from anyone.
     *
     * @return TRUE if the object can be picked up from anyone.
     */
    virtual bool IsPickupable();

    /**
     * Gets if the object can be picked up from anyone according to weak rules.
     *
     * This is a weaker version which allows more overriding of the not
     * pickupable status.
     *
     * @return TRUE if the object can be picked up from anyone according to weak rules.
     */
    virtual bool IsPickupableWeak();

    /**
     * Gets if the object can be picked up from anyone according to strong rules.
     *
     * This is a stronger version.
     *
     * @return TRUE if the object can be picked up from anyone according to strong rules.
     */
    virtual bool IsPickupableStrong();

    virtual bool IsLockable();
    virtual bool IsLocked();
    virtual bool IsConstructible();
    virtual bool IsSecurityLocked();
    virtual bool IsContainer();
    virtual bool IsUsingCD();

    virtual bool GetCanTransform();
    virtual bool GetVisibility();

    virtual void SendBehaviorMessage(const csString &str, gemObject* obj);

private:
    /**
     * Used to handle the take all messages with containers.
     *
     * @param str     The id of the message takestackall or takeall
     * @param obj     The actor doing the action.
     * @param precise Whathever if we should stack the items only if they are equal or
     *                only if they are of the same type.
     */
    virtual void SendBehaviorMessageTakeAll(const csString &str, gemObject* obj, bool precise);
};

//-----------------------------------------------------------------------------

/**
 * gemContainers are the public containers in the world for crafting, like
 * forges or ovens.
 *
 * Regular containers in inventory, like sacks, are simulated
 * by psCharInventory.
 */
class gemContainer : public gemItem
{
protected:
    csArray<psItem*> itemlist;
    bool AddToContainer(psItem* item, Client* fromClient,int slot, bool test);

public:
    gemContainer(GEMSupervisor* gemSupervisor, CacheManager* cachemanager,
                 EntityManager* entitymanager,
                 psItem* item,
                 const char* factname,
                 InstanceID myInstance,
                 iSector* room,
                 const csVector3 &pos,
                 float xrotangle,
                 float yrotangle,
                 float zrotangle,
                 int clientnum);

    ~gemContainer();

    /**
     * Clear list of items without delting them.
     *
     * Used when item is moved from world to inventory.
     */
    void CleareWithoutDelete();

    /**
     * Check if a item can be added to a container.
     */
    bool CanAdd(unsigned short amountToAdd, psItem* item, int slot, csString &reason);
    bool AddToContainer(psItem* item,Client* fromClient, int slot=-1)
    {
        return AddToContainer(item, fromClient, slot, false);
    }
    bool RemoveFromContainer(psItem* item,Client* fromClient);

    /**
     * Checks if client is allowed to remove an item from the container.
     *
     * @param client A pointer to the client viewing the container
     * @param item  Item being viewed or taken
     *
     * @return boolean indicating if client is allowed to take the item
     */
    bool CanTake(Client* client, psItem* item);

    /**
     * Remove an item from the container.
     *
     * @param itemStack  A pointer to the complete stack of the items we are looking at.
     * @param fromSlot   Where in the container the items are removed from.
     * @param fromClient The client that is removing the items.
     * @param stackCount The amount of items we want to remove.
     *
     * @return An item pointer that is the removed items from container.  If itemStack == the item returned
     *         then the entire stack has been removed.  Otherwise it will be a new item instance.
     */
    psItem* RemoveFromContainer(psItem* itemStack, int fromSlot, Client* fromClient, int stackCount);

    psItem* FindItemInSlot(int slot, int stackCount = -1);
    int SlotCount()
    {
        return GetItemData()->GetContainerMaxSlots();
    }
    size_t CountItems()
    {
        return itemlist.GetSize();
    }
    psItem* GetIndexItem(size_t i)
    {
        return itemlist[i];
    }

    class psContainerIterator;

    class psContainerIterator
    {
        size_t current;
        gemContainer* container;

    public:

        psContainerIterator(gemContainer* containerItem);
        bool HasNext();
        psItem* Next();
        psItem* RemoveCurrent(Client* fromClient);
        void UseContainerItem(gemContainer* containerItem);
    };
};

//-----------------------------------------------------------------------------

class gemActionLocation : public gemActiveObject
{
private:
    psActionLocation* action;
    bool visible;

public:
    gemActionLocation(GEMSupervisor* gemSupervisor,EntityManager* entitymanager,CacheManager* cachemanager,
                      psActionLocation* action, iSector* isec, int clientnum);

    virtual const char* GetObjectType()
    {
        return "ActionLocation";
    }
    virtual psActionLocation* GetAction()
    {
        return action;
    }

    virtual float GetBaseAdvertiseRange();
    virtual bool SeesObject(gemObject* object, float range);

    /**
     * @param clientnum The client that initiated this update.
     * @param control   Set to true when sent to the controlling client.
     */
    virtual void Broadcast(int clientnum, bool control);

    /**
     * Send this object to the given client.
     *
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     * @param to_superclients Send to super clients
     * @param allEntities Buffer the message instead of sending it.
     */
    virtual bool Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities=NULL);

    virtual void SendBehaviorMessage(const csString &str, gemObject* obj);

    virtual bool GetVisibility()
    {
        return visible;
    };
    virtual void SetVisibility(bool vis)
    {
        visible = vis;
    };
};

//-----------------------------------------------------------------------------

/**
 * A record in a gemActor's attacker history.
 *
 * This is abstract; entries are
 * classified as either normal damage or DOT.
 */
class AttackerHistory
{
public:
    virtual ~AttackerHistory() {}

    csWeakRef<gemActor> Attacker() const
    {
        return attacker_ref;
    }
    csTicks TimeOfAttack() const
    {
        return timeOfAttack;
    }
    virtual float Damage() const = 0; // Always positive.
protected:
    AttackerHistory(gemActor* attacker) : attacker_ref(attacker)
    {
        timeOfAttack = csGetTicks();
    }

    csWeakRef<gemActor> attacker_ref;
    csTicks timeOfAttack;
};

/**
 * An AttackerHistory entry for a regular, one-time damaging attack.
 */
class DamageHistory : public AttackerHistory
{
public:
    DamageHistory(gemActor* attacker, float dmg) : AttackerHistory(attacker), damage(dmg)
    {
        CS_ASSERT(damage > 0);
    }
    virtual ~DamageHistory() {}
    virtual float Damage() const
    {
        return damage;
    }
protected:
    float damage;
};

/**
 * An AttackerHistory entry for a DOT (damage over time) attack.
 */
class DOTHistory : public AttackerHistory
{
public:
    DOTHistory(gemActor* attacker, float hpRate, csTicks duration) : AttackerHistory(attacker), hpRate(hpRate), duration(duration)
    {
        CS_ASSERT(hpRate < 0);
    }
    virtual ~DOTHistory() {}
    virtual float Damage() const
    {
        csTicks elapsed = csGetTicks() - timeOfAttack;
        return -hpRate * csMin(duration, elapsed);
    }
protected:
    float hpRate;
    csTicks duration;
};

//-----------------------------------------------------------------------------
class FrozenBuffable : public ClampedPositiveBuffable<int>
{
public:
    void Initialize(gemActor* actor)
    {
        this->actor = actor;
    }

protected:
    gemActor* actor;
    virtual void OnChange();
};

//-----------------------------------------------------------------------------

/**
 * Any semi-autonomous object, either a player or an NPC.
 */
class gemActor :  public gemObject, public iDeathNotificationObject
{
protected:
    psCharacter* psChar;
    PID pid;                   ///< Player ID (also known as character ID or PID)
    csRef<PlayerGroup> group;

    psCharacter* mount;

    csVector3 top, bottom, offset;
    csVector3 last_production_pos;

    csWeakRef<Client> clientRef;

    uint8_t DRcounter;  ///< increments in loop to prevent out of order packet overwrites of better data
    uint8_t forceDRcounter; ///< sequence number for forced position updates
    csTicks lastDR;
    csVector3 lastV;

    /** Production Start Pos is used to record the place where people started digging. */
    csVector3 productionStartPos;

    csVector3 lastSentSuperclientPos;
    unsigned int lastSentSuperclientInstance;
    csTicks      lastSentSuperclientTick;

    csArray<iDeathCallback*> deathReceivers;  ///< List of objects which are to be notified when this actor dies.

    struct DRstate
    {
        csVector3 pos;
        iSector* sector;
        float yrot;
        InstanceID instance;
    } valid_location;

    DRstate newvalid_location;
    DRstate last_location;
    DRstate prev_teleport_location;

    // used by /report command.
    // for details on current /report implementation
    // check PS#2789.

    /// Info: Stores a chat history element.
    struct ChatHistoryEntry
    {
        /// Info: Time this line was said.
        time_t _time;

        /// Info: Actual text. (Preformated depending on chat type)
        csString _line;

        /// Info: Constructor. When no time is given, current time is used.
        ChatHistoryEntry(const char* szLine, time_t t = 0);

        /** Prepends a string representation of the time to this chat line
          * so it can be written to a log file. The resulting line is
          * written to 'line' argument (reference).
          * Note: this function also applies \n to the line end.
          */
        void GetLogLine(csString &line) const;
    };

    /// unsigned int activeReports
    /// Info: Total /report commands filed against this
    /// player that are still active (logging).
    unsigned int activeReports;

    /// csArray<ChatHistoryEntry> chatHistory
    /// Info: Chat history for this player.
    /// A chat line stays in history for CHAT_HISTORY_LIFETIME (defined in gem.cpp).
    csArray<ChatHistoryEntry> chatHistory;

    /// csRef<iFile> logging_chat_file
    /// Info: log file handle.
    csRef<iFile> logging_chat_file;

    /**
     * Determines the right size and height for the collider for the sprite
     * and sets the actual position of the sprite.
     */
    bool InitLinMove(const csVector3 &pos,float angle, iSector* sector);
    bool InitCharData(Client* c);

    /// What commands the actor can access. Normally only for a player controlled object.
    int securityLevel;
    int masqueradeLevel;

    bool isFalling;           ///< is this object falling down ?
    csVector3 fallStartPos;   ///< the position where the fall began
    iSector* fallStartSector; ///< the sector where the fall began
    csTicks fallStartTime;

    bool invincible;          ///< cannot be attacked
    bool visible;             ///< is visible to clients ?
    bool viewAllObjects;      ///< can view invisible objects?

    csWeakRef<gemObject> targetObject; ///< Is the current targeted object for this actor.

    csPDelArray<AttackerHistory> dmgHistory;
    csPDelArray<ProgressionScript> onAttackScripts, onDefenseScripts, onNearlyDeadScripts, onMovementScripts;

    csArray<ActiveSpell*> activeSpells;

    virtual void ApplyStaminaCalculations(const csVector3 &velocity, float times);

    /// Set initial attributes for GMs
    void SetGMDefaults();

    uint8_t movementMode; ///< Actor movement mode from DB table movement_modes
    bool isAllowedToMove; ///< Is a movement lockout in effect?
    bool atRest;          ///< Is this character stationary or moving?
    FrozenBuffable isFrozen;  ///< Whether the actor is frozen or not.

    PSCHARACTER_MODE player_mode;
    Stance combat_stance;

    psSpellCastGameEvent* spellCasting; ///< Hold a pointer to the game event
    ///< for the spell currently cast.
    psWorkGameEvent* workEvent;

    /// Needed to force the effect of a psForcePositionMessage since
    /// sectors are tied to instances and force position messages may be lost on crash, loading
    /// failure, etc.
    /// This ensures that a DR message containing the new sector is received at least once.
    iSector* forcedSector;

    bool CanSwitchMode(PSCHARACTER_MODE from, PSCHARACTER_MODE to);

    /**
     * active magic message seq
     */
    uint32_t activeMagic_seq;


public:
    psLinearMovement* pcmove;

    gemActor(GEMSupervisor* gemsupervisor,
             CacheManager* cachemanager,
             EntityManager* entitymanager,
             psCharacter* chardata, const char* factname,
             InstanceID myInstance,iSector* room,const csVector3 &pos,float rotangle,int clientnum);

    virtual ~gemActor();

    virtual const char* GetObjectType()
    {
        return "Actor";
    }
    virtual psCharacter* GetCharacterData()
    {
        return psChar;
    }
    virtual Client* GetClient() const;

    virtual PID GetPID()
    {
        return pid;
    }

    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    ///@}

    bool SetupCharData();

    void SetTextureParts(const char* parts);
    void SetEquipment(const char* equip);

    psCharacter* GetMount() const
    {
        return mount;
    }

    void SetMount(psCharacter* newMount)
    {
        mount = newMount;
    }

    bool IsMounted()
    {
        return (mount != NULL);
    }


    PSCHARACTER_MODE GetMode()
    {
        return player_mode;
    }
    const char* GetModeStr(); ///< Return a string name of the mode
    void SetMode(PSCHARACTER_MODE newmode, uint32_t extraData = 0);
    const Stance &GetCombatStance()
    {
        return combat_stance;
    }
    virtual void SetCombatStance(const Stance &stance);

    void SetSpellCasting(psSpellCastGameEvent* event)
    {
        spellCasting = event;
    }
    bool IsSpellCasting()
    {
        return spellCasting != NULL;
    }
    void InterruptSpellCasting()
    {
        if(spellCasting) spellCasting->Interrupt();
    }

    /**
     * Assign trade work event so it can be accessed.
     */
    void SetTradeWork(psWorkGameEvent* event)
    {
        workEvent = event;
    }

    /**
     * Return trade work event so it can be stopped.
     */
    psWorkGameEvent* GetTradeWork()
    {
        return workEvent;
    }

    bool IsAllowedToMove()
    {
        return isAllowedToMove;    ///< Covers sitting, death, and out-of-stamina
    }
    void SetAllowedToMove(bool newvalue);

    /// Check whether the actor is frozen.
    void SetFrozen(bool flag)
    {
        isFrozen.SetBase(flag ? 1 : 0);
    }
    bool IsFrozen()
    {
        return (isFrozen.Current() > 0);
    }
    FrozenBuffable &GetBuffableFrozen()
    {
        return isFrozen;
    }

    /**
     * Get the target type in regards to attacks
     */
    int GetTargetType(gemObject* target);

    /**
     * Checks if our actor can attack given target.
     * If not, send him some informative text message
     */
    bool IsAllowedToAttack(gemObject* target, csString &msg);


    /**
     * Make the character sit.
     */
    void Sit();

    /**
     * Makes the character stand up.
     */
    void Stand();

    /**
     * Call this to ask the actor to set allowed to disconnect in the connected client.
     */
    void SetAllowedToDisconnect(bool allowed);

    void SetSecurityLevel(int level);
    void SetMasqueradeLevel(int level);
    int GetSecurityLevel()
    {
        return(securityLevel);
    }
    int GetMasqueradeLevel()
    {
        return(masqueradeLevel);
    }

    // Last Production Pos is used to require people to move around while /digging
    void SetLastProductionPos(csVector3 &pos)
    {
        last_production_pos = pos;
    }
    void GetLastProductionPos(csVector3 &pos)
    {
        pos = last_production_pos;
    }

    /**
     * Returns the place where the player last started digging.
     *
     * @return The location where the player last started digging.
     */
    const csVector3 &GetProductionStartPos(void) const
    {
        return productionStartPos;
    }

    /**
     * Sets the place where the player started digging.
     *
     * @param pos The location where the player started digging.
     */
    void SetProductionStartPos(const csVector3 &pos)
    {
        productionStartPos = pos;
    }

    // To be used for the /report command.

    /**
     * Adds an active report (file logging session) to this actor.
     *
     * @param[in] reporter The client actor issuing /report
     * @return true on success, or false on error (failure to start logging probably)
     */
    bool AddChatReport(gemActor* reporter);

    /**
     * Removes an active report (file logging session) for this actor.
     *
     * This function is invoked automatically after a specific amount of time
     * from the report activation.
     */
    void RemoveChatReport();

    /**
     * Returns /report file logging status.
     */
    bool IsLoggingChat() const
    {
        return activeReports > 0;
    }

    /**
     * Adds the chat message to the history and optionally to the log file.
     *
     * @param[in] who The name of the character who sent this message
     * @param[in] msg The chat message
     * @return Returns true if the message was written to the log file
     */
    bool LogChatMessage(const char* who, const psChatMessage &msg);

    /**
     * @brief Saves a system message to this actor's chat history and logs it to
     * a file, if there are active reports.
     *
     * @return Returns true if the line was written to the log file
     */
    bool LogSystemMessage(const char* szLine);

    /**
     * @brief Saves a line to this actor's chat history and logs it to
     * a file, if there are active reports.
     *
     * @return Returns true if the line was written to the log file
     */
    bool LogLine(const char* szLine);


    void UpdateStats();
    void ProcessStamina();
    void ProcessStamina(const csVector3 &velocity, bool force=false);

    void Teleport(const char* sec, const csVector3 &pos, float yrot, InstanceID instance, int32_t loadDelay = 0, csString background = "", csVector2 point1 = 0, csVector2 point2 = 0, csString widget = "");
    void Teleport(iSector* sector, const csVector3 &pos, float yrot, InstanceID instance, int32_t loadDelay = 0, csString background = "", csVector2 point1 = 0, csVector2 point2 = 0, csString widget = "");
    void Teleport(iSector* sector, const csVector3 &pos, float yrot, int32_t loadDelay = 0, csString background = "", csVector2 point1 = 0, csVector2 point2 = 0, csString widget = "");

    void SetPosition(const csVector3 &pos,float angle, iSector* sector);
    void SetInstance(InstanceID worldInstance);

    void UpdateValidLocation(const csVector3 &pos, float yrot, iSector* sector, InstanceID instance, bool force = false);

    bool SetDRData(psDRMessage &drmsg);
    void MulticastDRUpdate();
    virtual void ForcePositionUpdate(int32_t loadDelay = 0, csString background = "", csVector2 point1 = 0, csVector2 point2 = 0, csString widget = "");

    using gemObject::RegisterCallback;
    using gemObject::UnregisterCallback;
    void RegisterCallback(iDeathCallback* receiver)
    {
        deathReceivers.Push(receiver);
    }
    void UnregisterCallback(iDeathCallback* receiver)
    {
        deathReceivers.Delete(receiver);
    }
    void HandleDeath();

    float GetRelativeFaction(gemActor* speaker);

    csPtr<PlayerGroup> GetGroup();
    void SetGroup(PlayerGroup* group);
    bool InGroup() const;
    bool IsGroupedWith(gemActor* other, bool IncludePets = false) const;
    int GetGroupID();
    void RemoveFromGroup();

    bool IsMyPet(gemActor* other) const;

    const char* GetFirstName()
    {
        return psChar->GetCharName();
    }

    const char* GetGuildName();
    psGuildInfo* GetGuild()
    {
        return psChar->GetGuild();
    }
    psGuildLevel* GetGuildLevel()
    {
        return psChar->GetGuildLevel();
    }

    /**
     * Gets the psguildmember structure refereed to this actor, if in a guild.
     *
     * @return psGuildMember: A pointer to the psGuildMember structure of this actor
     */
    psGuildMember* GetGuildMembership()
    {
        return psChar->GetGuildMembership();
    }

    void DoDamage(gemActor* attacker, float damage);
    void AddAttackerHistory(gemActor* attacker, float damage); // direct damage version
    void AddAttackerHistory(gemActor* attacker, float hpRate, csTicks duration); // DoT version
    void RemoveAttackerHistory(gemActor* attacker);
    bool CanBeAttackedBy(gemActor* attacker, gemActor* &lastAttacker) const;

    /**
     * Checks the attacker history to see if the attacker has actually attacked the entity.
     *
     *  Used to exclude from loot those who didn't actually partecipate in the attack even if
     *  they are grouped.
     *
     *  @param attacker A pointer to the actor we are checking if it has attacked.
     *  @return TRUE if the actor has attacked the entity.
     */
    virtual bool HasBeenAttackedBy(gemActor* attacker);

    void Kill(gemActor* attacker)
    {
        DoDamage(attacker, psChar->GetHP());
    }
    void Defeat();
    void Resurrect();

    virtual bool UpdateDR();
    virtual void GetLastSuperclientPos(csVector3 &pos, InstanceID &instance, csTicks &last) const;
    virtual void SetLastSuperclientPos(const csVector3 &pos, InstanceID instance, const csTicks &now);

    virtual void BroadcastTargetStatDR(ClientConnectionSet* clients);
    virtual void SendTargetStatDR(Client* client);
    virtual void SendGroupStats();

    void SetAction(const char* anim,csTicks &timeDelay);

    virtual void Broadcast(int clientnum, bool control);

    /**
     * Send this object to the given client.
     *
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     * @param to_superclients Send to super clients
     * @param allEntities Buffer the message instead of sending it.
     */
    virtual bool Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities=NULL);

    /**
     * Distribute this object to all members of a group.
     *
     * @param me The MsgEntry to send to all group members.
     */
    virtual void SendGroupMessage(MsgEntry* me);

    /**
     * Used by chat manager to identify an npc you are near if you talk to one.
     */
    gemObject* FindNearbyActorName(const char* name);

    virtual void SendBehaviorMessage(const csString &str, gemObject* obj);
    virtual csString GetDefaultBehavior(const csString &dfltBehaviors);

    /**
     * Called when the object began falling - 'fallBeginning' tells where the fall started
     * displaceY is the displacement that needs to be added to due to passing through
     * warping portals portalSector is the final sector of the player after passing through the warping
     * portals.
     */
    void FallBegan(const csVector3 &pos, iSector* sector);

    /**
     * Called when the object has fallen down - sets its falling status to false.
     */
    float FallEnded(const csVector3 &pos, iSector* sector);

    /**
     * Checks if the object is falling.
     */
    bool IsFalling()
    {
        return isFalling;
    }

    csTicks GetFallStartTime()
    {
        return fallStartTime;
    }

    bool AtRest() const
    {
        return atRest;
    }

    virtual bool GetVisibility()
    {
        return visible;
    }
    virtual void SetVisibility(bool visible);
    virtual bool SeesObject(gemObject* object, float range);

    virtual bool GetInvincibility()
    {
        return invincible;
    }
    virtual void SetInvincibility(bool invincible);

    /**
     * Flag to determine of this player can see all objects.
     */
    bool GetViewAllObjects()
    {
        return viewAllObjects;
    }
    void SetViewAllObjects(bool v);

    void StopMoving(bool worldVel = false);

    /**
     * Moves player to his spawn position.
     *
     * @param delay The time to wait in the load screen even if the new position was already loaded.
     * @param background The background to use when the load screen is shown or if it's forced.
     * @param point1 Start point for animation
     * @param point2 Destination point for animation
     * @param widget The widget which will replace the normal loading one, if any is defined.
     * @return TRUE in case of success. FALSE if the character lacks a spawn position.
     */
    bool MoveToSpawnPos(int32_t delay = 0, csString background = "", csVector2 point1 = 0, csVector2 point2 = 0, csString widget = "");

    /**
     * Gets the player spawn position according to his race.
     *
     * @todo Implement the support for player specific spawn pos, not only race specific.
     *
     * @param pos A csVector where to store the player position.
     * @param yrot A float where to store the rotation of the player.
     * @param sector An iSector where to store the sector of the position the player.
     * @param useRange A boolean which says if we have to apply random range calculation in the positions.
     * @return FALSE If the race or the destination sector of the player can't be found.
     */
    bool GetSpawnPos(csVector3 &pos, float &yrot, iSector* &sector, bool useRange = false);

    /**
     * Restores actor to his last valid position.
     *
     * @param force Force an update if set to true.
     */
    bool MoveToValidPos(bool force = false);

    void GetValidPos(csVector3 &pos, float &yrot, iSector* &sector, InstanceID &instance);

    /**
     * Get the last reported location this actor was at.
     */
    void GetLastLocation(csVector3 &pos, float &yrot, iSector* &sector, InstanceID &instance);

    /**
     * Moves player to his last reported location.
     */
    void MoveToLastPos();

    /// Record the location of this actor before he was teleported
    void SetPrevTeleportLocation(const csVector3 &pos, float yrot, iSector* sector, InstanceID instance);
    /// Get the location of the player before he was teleported
    void GetPrevTeleportLocation(csVector3 &pos, float &yrot, iSector* &sector, InstanceID &instance);

    AttackerHistory* GetDamageHistory(int pos) const
    {
        return dmgHistory.Get(pos);
    }
    size_t GetDamageHistoryCount() const
    {
        return dmgHistory.GetSize();
    }
    void ClearDamageHistory()
    {
        dmgHistory.Empty();
    }

    void AttachScript(ProgressionScript* script, int type);
    void DetachScript(ProgressionScript* script, int type);
    void InvokeAttackScripts(gemActor* defender, psItem* weapon);
    void InvokeDefenseScripts(gemActor* attacker, psItem* weapon);
    void InvokeNearlyDeadScripts(gemActor* defender, psItem* weapon);
    void InvokeMovementScripts();

    void AddActiveSpell(ActiveSpell* asp);
    void SendActiveSpells();
    bool RemoveActiveSpell(ActiveSpell* asp);
    ActiveSpell* FindActiveSpell(const csString &name, SPELL_TYPE type);
    int ActiveSpellCount(const csString &name);
    csArray<ActiveSpell*> &GetActiveSpells()
    {
        return activeSpells;
    }
    void CancelActiveSpellsForDeath();
    void CancelActiveSpellsWhichDamage();
    int FindAnimIndex(const char* name);


    /**
     * Set attributes back to default.
     *
     * GMs use this commands to set attributes back to the default
     * for either a player or a GM.
     *
     * @param player Set to true for players. False for GMs.
     */
    void SetDefaults(bool player);


    /** @name GM/debug abilities
     * Turn on and of features used for GMs/debuging.
     */
    ///@{
    /** These flags are for GM/debug abilities */
    bool nevertired;        ///< infinite stamina
    bool infinitemana;      ///< infinite mana
    bool instantcast;       ///< cast spells instantly
    bool safefall;          ///< no fall damage
    bool questtester;       ///< no quest lockouts
    bool givekillexp;       ///< give exp if killed
    bool attackable;        ///< This actor can be attacked by anyone without limitations or /challenge
    ///@}

    /**
     * @note don't use this directly
     */
    bool SetMesh(const char* meshname);

    bool GetFiniteInventory()
    {
        return GetCharacterData()->Inventory().GetDoRestrictions();
    }
    void SetFiniteInventory(bool v)
    {
        GetCharacterData()->Inventory().SetDoRestrictions(v);
    }

    // Target information
    void SetTargetObject(gemObject* object)
    {
        targetObject = object;
    }
    gemObject* GetTargetObject() const
    {
        return targetObject;
    }

    /**
     * Get the z velocity of the actor.
     */
    virtual float GetVelocity();

    /**
     * Check if object is inside guarded area.
     *
     * Guarded are is a circle of RANGE_TO_GUARD around player and pet.
     */
    virtual bool InsideGuardedArea(gemObject* object);

    /**
     * Get the activeMagic sequence, updating to the next value;
     */
    uint32_t GetActiveMagicSequence()
    {
        activeMagic_seq++;
        return activeMagic_seq;
    }
};

//-----------------------------------------------------------------------------
class NpcDialogMenu;

class gemNPC : public gemActor
{
protected:
    psNPCDialog* npcdialog;
    AccountID superClientID;
    csWeakRef<gemObject>  target;
    csWeakRef<gemObject>  owner;

    csTicks nextVeryShortRangeAvail; ///< When can npc respond to very short range prox trigger again
    csTicks nextShortRangeAvail;     ///< When can npc respond to short range prox trigger again
    csTicks nextLongRangeAvail;      ///< When can npc respond to long range prox trigger again

    /// Array of client id's allowed to loot this char
    csArray<PID> lootablePlayers;

    struct DialogCounter
    {
        csString said;
        csString trigger;
        int      count;
        csTicks  when;
        static int Compare(DialogCounter* const &first,  DialogCounter* const &second)
        {
            if(first->count != second->count)
                return first->count - second->count;
            return first->when - second->when;
//            if (first.count != second.count)
//                return first.count - second.count;
//            return first.when - second.when;
        }
    };

    csPDelArray<DialogCounter> badText;

    int speakers;

    bool busy; ///< Indicator from the NPC client to state the if the NPC is busy

public:
    gemNPC(GEMSupervisor* gemSupervisor,
           CacheManager* cachemanager,
           EntityManager* entityManager,
           psCharacter* chardata, const char* factname,
           InstanceID myInstance,iSector* room,const csVector3 &pos,float rotangle,int clientnum);

    virtual ~gemNPC();

    virtual void SetCombatStance(const Stance &stance);

    virtual const char* GetObjectType()
    {
        return "NPC";
    }
    virtual psNPCDialog* GetNPCDialogPtr()
    {
        return npcdialog;
    }

    virtual AccountID GetSuperclientID()
    {
        return superClientID;
    }
    virtual void SetSuperclientID(AccountID id)
    {
        superClientID = id;
    }

    void SetupDialog(PID npcID, PID masterNpcID, bool force=false);
    void ReactToPlayerApproach(psNPCCommandsMessage::PerceptionType type, gemActor* player);

    virtual void ApplyStaminaCalculations(const csVector3 &velocity, float times) { } // NPCs usually have 0 stamina.
    // Before this fix, this caused a major long term bug where NPCs would give up attacking after a few hits and expending all stamina.

    virtual void AddLootablePlayer(PID playerID);
    virtual void RemoveLootablePlayer(PID playerID);
    bool IsLootablePlayer(PID playerID);
    const csArray<PID> &GetLootablePlayers() const
    {
        return lootablePlayers;
    }
    virtual Client* GetRandomLootClient(int range);

    /**
     * Used to allow a NPC to communicate by saying things to its environment.
     *
     * Use this to publish chat messages of type SAY for NPCs.
     * They can either be of private to the client who or broadcasted.
     *
     * @param sayText       The text to send.
     * @param who           Who to send to.
     * @param sayPublic     True if it should be a public say, otherwise it will only be sent to the destination client given by who.
     * @param timeDelay     Used to lay out a sequence of responses in time depended on the length of the sayText.
     */
    void Say(const char* sayText, Client* who, bool sayPublic, csTicks &timeDelay);

    /**
     * Used to allow a NPC to communicate through action to its environment.
     *
     * Use this to publish chat messages of type ME,MY,NARRATE for NPCs.
     * They can either be of private to the client who or broadcasted.
     * Use one of the actionMy or actionNarrate to override the default type
     * of ME.
     *
     * @param actionMy      If true a /my will be used
     * @param actionNarrate If true a narrate will be used if not actionMy is set.
     * @param actText       The text to send.
     * @param who           Who to send to.
     * @param actionPublic  True if it should be a public action, otherwise it will only be sent to the destination client given by who.
     * @param timeDelay     Used to lay out a sequence of responses in time depended on the length of the actText.
     */
    void ActionCommand(bool actionMy, bool actionNarrate, const char* actText, Client* who, bool actionPublic, csTicks &timeDelay);

    void AddBadText(const char* playerSaid,const char* trigger);
    void GetBadText(size_t first,size_t last, csStringArray &saidArray, csStringArray &trigArray);

    /**
     * Send this object to the given client.
     *
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     * @param to_superclients Send to super clients
     * @param allEntities Buffer the message instead of sending it.
     */
    virtual bool Send(int clientnum, bool control, bool to_superclients, psPersistAllEntities* allEntities=NULL);

    /**
     * @param clientnum The client that initiated this update.
     * @param control  Set to true when sent to the controlling client.
     */
    virtual void Broadcast(int clientnum, bool control);

    virtual void SendBehaviorMessage(const csString &str, gemObject* obj);
    virtual csString GetDefaultBehavior(const csString &dfltBehaviors);
    void ShowPopupMenu(Client* client);

    virtual void SetTarget(gemObject* newTarget)
    {
        target = newTarget;
    }
    virtual gemObject* GetTarget()
    {
        return this->target;
    }

    virtual void SetOwner(gemObject* owner);

    virtual gemObject* GetOwner()
    {
        return this->owner;
    }

    virtual void SetPosition(const csVector3 &pos, float angle, iSector* sector);


    /**
     * Returns if the npc has killsteal protection.
     *
     * Rules about killsteal protection should be implemented here.
     * Generic NPCs have killsteal protection, pet don't have it.
     *
     * @return A boolean indicating if this gemNPC must have killsteal protection.
     */
    virtual bool HasKillStealProtection()
    {
        return !GetCharacterData()->IsPet();
    }

    virtual void SendGroupStats();
    virtual void ForcePositionUpdate();

    /**
     * Register clients that speak to a npc
     */
    void RegisterSpeaker(Client* client);

    /**
     * Check speakers to see if not spoken to anymore.
     */
    void CheckSpeakers();

    /**
     * To set the busy state of the NPC
     *
     * NPC operation \<busy /\> and \<idle /\> set and cleare this flag.
     * @see BusyOperation
     */
    void SetBusy(bool busy);

    /**
     * True if the NPC client sent the busy command.
     *
     * NPC operation \<busy /\> and \<idle /\> set and cleare this flag.
     * @see BusyOperation
     */
    bool IsBusy() const;
};

//-----------------------------------------------------------------------------

class gemPet : public gemNPC
{
public:

    gemPet(GEMSupervisor* gemsupervisor,
           CacheManager* cachemanager,
           EntityManager* entitymanager,
           psCharacter* chardata, const char* factname, InstanceID instance, iSector* room,
           const csVector3 &pos,float rotangle,int clientnum,uint32 id) : gemNPC(gemsupervisor, cachemanager, entitymanager, chardata,factname,instance,room,pos,rotangle,clientnum)
    {
        this->persistanceLevel = "Temporary";
    };

    virtual const char* GetObjectType()
    {
        return "PET";
    }


    void SetPersistanceLevel(const char* level)
    {
        this->persistanceLevel = level;
    };
    const char* SetPersistanceLevel(void)
    {
        return persistanceLevel.GetData();
    };
    bool IsFamiliar(void)
    {
        return this->persistanceLevel.CompareNoCase("Permanent");
    };

private:
    csString persistanceLevel;
};

//-----------------------------------------------------------------------------

/**
 * This class automatically implements timed events which depend
 * on the existence and validity of a gemObject of any kind.
 *
 * It will make sure this event is cancelled and skipped when the
 * gemObject is deleted before the event is fired.
 */
class psGEMEvent : public psGameEvent, public iDeleteObjectCallback
{
public:
    csWeakRef<gemObject> dependency;

    psGEMEvent(csTicks ticks,int offsetticks,gemObject* depends, const char* newType)
        : psGameEvent(ticks,offsetticks,newType)
    {
        dependency = NULL;

        // Register for disconnect events
        if(depends)
        {
            dependency = depends;
            depends->RegisterCallback(this);
        }
    }

    virtual ~psGEMEvent()
    {
        // If DeleteObjectCallback() has not been called normal operation
        // this object have to unregister to prevent the
        // object from calling DeleteObjectCallback() later when destroyed.
        if(dependency.IsValid())
        {
            dependency->UnregisterCallback(this);
            dependency = NULL;
        }
    }

    virtual void DeleteObjectCallback(iDeleteNotificationObject* object)
    {
        SetValid(false); // Prevent the Trigger from beeing called.

        if(dependency.IsValid())
        {
            dependency->UnregisterCallback(this);
            dependency = NULL;
        }
    }
};

//-----------------------------------------------------------------------------

class psResurrectEvent : public psGameEvent // psGEMEvent
{
protected:
    csWeakRef<gemObject> who;

public:
    psResurrectEvent(csTicks ticks,int offsetticks,gemActor* actor)
        : psGameEvent(ticks,offsetticks,"psResurrectEvent")
    {
        who = actor;
    }

    void Trigger()
    {
        if(who.IsValid())
        {
            gemActor* actor = dynamic_cast<gemActor*>((gemObject*) who);
            actor->Resurrect();
        }
    }
};

/** @} */

#endif
