/*
 * npcclient.h
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
#ifndef __NPCCLIENT_H__
#define __NPCCLIENT_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/hash.h>
#include <csutil/ref.h>
#include <csutil/list.h>
#include <iutil/vfs.h>

struct iObjectRegistry;
struct iDocumentNode;
struct iConfigManager;
struct iCollideSystem;

//=============================================================================
// Library Includes
//=============================================================================
#include "net/pstypes.h"
#include "net/npcmessages.h"

#include "util/psconst.h"
#include "util/serverconsole.h"
#include "util/pspath.h"
#include "util/pspathnetwork.h"
#include "util/mathscript.h"

#include "tools/celhpf.h"

/**
 * \addtogroup npcclient
 * @{ */

class  psDatabase;
class  MsgHandler;
class  psNetConnection;
class  EventManager;
class  RecipeManager;
class  NetworkManager;
class  NPCType;
class  NPC;
class  gemNPCActor;
class  gemNPCObject;
class  gemNPCItem;
struct iVFS;
class  Perception;
class  Reaction;
class  psWorld;
class  Location;
class  LocationType;
class  LocationManager;
class  Waypoint;
//class  psPFMaps;
class  Tribe;
class  psPath;
class  psPathNetwork;
struct iCelHNavStruct;

/**
 * The main NPC Client class holding references to important superclient objects
 */
class psNPCClient : public iCommandCatcher, public iScriptableVar
{
public:

    /** Keep information about deferred NPCs during loading.
     */
    struct DeferredNPC
    {
        PID      id;
        csString name;
    };


    /**
     * Constructor.
     *
     * Create the NPC Client application class.
     */
    psNPCClient();

    /**
     * Cleans up all allocated memory and removes all the players from the world.
     */
    ~psNPCClient();

    /**
     * Initialize the superclient.
     * Starts a thread for the Network, Message Handler. Also
     * binds the status socket for the network.
     *
     * @param object_reg The object registry
     * @param host       The host that keeps the DB.
     * @param user       The user for the DB.
     * @param pass       The password for the DB.
     * @param port       The port for the DB.
     * @return True if the initialization went ok.
     */
    bool Initialize(iObjectRegistry* object_reg, const char* host, const char* user, const char* pass, int port);

    /**
     * Just make iObjectRegistry available to other classes.
     */
    iObjectRegistry* GetObjectReg()
    {
        return objreg;
    }

    /**
     * Make a tribe pointer available - for the network manager
     */
    Tribe* GetTribe(int id);

    /**
     * Load and fork off a new thread for the Server Console. Then start
     * EventManager's main loop, processing all network and server events.
     */
    void MainLoop();

    /**
     * Catch a command from the console.
     *
     * @param cmd The command.
     */
    virtual void CatchCommand(const char* cmd);

    /**
     * Used to load the log settings
     */
    void LoadLogSettings();

    /**
     * Used to save the log settings
     */
    void SaveLogSettings();

    /**
     * To be called when the the load process is completed.
     */
    void LoadCompleted();

    /**
     * If a world is ready and loaded this returns true.
     */
    bool IsReady();

    /**
    * This does 1 AI calc per Tribe then returns.
    */
    void Tick();

    /**
     * Load a map into a region.
     *
     * This is a copy of code from entitymanager->CreateRoom().
     *
     * @param mapfile A file with maps to load.
     */
    bool LoadMap(const char* mapfile);

    /** Find a region with a given name.
     * This function handles the searching for the specified region name
     * so that other functions can refer to the region directly.
     */
    LocationType* FindRegion(const char* regname);

    /** Find a location with a given name.
     * This function handles the searching for the specified location type
     * so that other functions can refer to the location type directly.
     */
    LocationType* FindLocation(const char* locname);

    /**
     * This function handles the searching for the specified object
     * type and basically does the work for the \<locate\> script command.
     */
    Location* FindLocation(const char* loctype, const char* name);

    /**
     * This function handles the searching for the specified object
     * type and basically does the work for the \<locate\> script command.
     */
    Location* FindNearestLocation(const char* loctype, csVector3 &pos, iSector* sector, float range = -1, float* found_range = NULL);

    /**
     * This function handles the searching for the specified object
     * type and basically does the work for the \<locate\> script command.
     */
    Location* FindRandomLocation(const char* loctype, csVector3 &pos, iSector* sector, float range = -1, float* found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindNearestWaypoint(const csVector3 &v, iSector* sector, float range = -1, float*  found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindRandomWaypoint(const csVector3 &v, iSector* sector, float range = -1, float*  found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one with the given group.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindNearestWaypoint(const char* group, csVector3 &v, iSector* sector, float range = -1, float*  found_range = NULL);

    /**
     * This iterates over all waypoints and finds the nearest one with the given group.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindRandomWaypoint(const char* group, csVector3 &v, iSector* sector, float range = -1, float*  found_range = NULL);

    /**
     * This iterates over all waypoints and finds the one the entity is at.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindWaypoint(gemNPCObject* entity);

    /** Return the nearest waypoint to the entity.
     */
    Waypoint* FindNearestWaypoint(gemNPCObject* entity, float range, float* found_range);


    /**
     * This iterates over all waypoints and finds the one with the given id.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindWaypoint(int id);

    /**
     * This iterates over all waypoints and finds the one with the given name.
     * There is probably a more efficient structure to accomplish this.
     */
    Waypoint* FindWaypoint(const char* name, WaypointAlias** alias);

    /**
     * Find the shortest route between waypoint start and stop.
     */
    csList<Waypoint*> FindWaypointRoute(Waypoint*  start, Waypoint*  end, const psPathNetwork::RouteFilter* filter);

    /**
     * Find the shortest route between waypoint start and stop.
     */
    csList<Edge*> FindEdgeRoute(Waypoint*  start, Waypoint*  end, const psPathNetwork::RouteFilter* filter);

    /**
     * This function handles the searching for the specified NPCType name
     * so that other NPCTypes can subclass an existing one.
     */
    NPCType* FindNPCType(const char* npctype_name);

    /**
     * Add a race info to the race info list
     */
    void AddRaceInfo(const psNPCRaceListMessage::NPCRaceInfo_t &raceInfo);

    /**
     * Get pointer to a race info to the given race name.
     */
    psNPCRaceListMessage::NPCRaceInfo_t* GetRaceInfo(const char* name);

    /**
     * Dump all race infos
     */
    bool DumpRace(const char* pattern);

    /**
     * Find the walk velocity for a given race
     */
    float GetWalkVelocity(csString &race);

    /**
     * Find the run velocity for a given race
     */
    float GetRunVelocity(csString &race);

    /**
     * Returns the network handler so script ops can invoke the network.
     */
    NetworkManager* GetNetworkMgr()
    {
        return network;
    }

    /**
     * Returns the network handler so script ops can invoke the network.
     */
    psNetConnection* GetNetConnection()
    {
        return connection;
    }

    /**
     * Register a reaction for an NPC with the client. Used to speed up FirePerception.
     */
    void RegisterReaction(NPC* npc, Reaction* reaction);

    /**
     * Sends a perception to all npcs.
     *
     * If macRange is greather than 0.0 only npcs within that range
     * of the base position will be triggered.
     *
     * \sa NPC::TriggerEvent Tribe::TriggerEvent
     *
     * @param pcpt            The perception
     * @param maxRange        If greater than 0.0 then max range apply
     * @param basePos         The base position for range checks.
     * @param baseSector      The base sector for range checks.
     * @param sameSector      Only trigger if in same sector
     */
    void TriggerEvent(Perception* pcpt, float maxRange=-1.0,
                      csVector3* basePos=NULL, iSector* baseSector=NULL,
                      bool sameSector=false);

    /**
     * Returns the Event Manger.
     *
     * @return Returns a ptr to the event manager.
     */
    EventManager* GetEventManager()
    {
        return eventmanager;
    }

    /**
     * Returns the Math Scripting Engine.
     *
     * @return Returns a ptr to the math script engine.
     */
    MathScriptEngine*  GetMathScriptEngine()
    {
        return mathScriptEngine;
    }

    /**
     * Disconnect from server nicely, before quitting.
     */
    void Disconnect();

    /**
     * SetEntityPos finds the given ID entity, and updates
     * its position in mesh and linmove.
     *
     * @param id       The EID of the entity.
     * @param pos      The new position.
     * @param sector   The new sector.
     * @param instance The new instance.
     * @param force    Applies the entity reposition also on npc. Useful with teleport slide and other operations.
     */
    void SetEntityPos(EID id, csVector3 &pos, iSector* sector, InstanceID instance, bool force = false);

    /**
     * Find the NPC* attached to the entity with the specified character ID
     *
     * @param character_id  The PID of the NPC to find.
     * @return The NPC or NULL if no NPC with this PID was found.
     */
    NPC* FindNPCByPID(PID character_id);

    /**
     * Find the NPC* attached to the entity with the specified EID
     *
     * @param entid           The EID of the NPC to find.
     * @return The NPC or NULL if no NPC with this EID was found.
     */
    NPC* FindNPC(EID entid);

    /**
     * Find a given path.
     *
     * @param name            The name of the path.
     * @return The path or NULL if no path was found.
     */
    psPath* FindPath(const char* name);

    /**
     * Enable or disable NPCs
     *
     * Alter the Disabled state of NPCs.
     *
     * @param pattern Used to mach NPC(s) to enable or disable.
     * @param enable  Set to true if the NPC should be enabled.
     */
    void EnableDisableNPCs(const char* pattern, bool enable);

    /**
     * List all NPCs matching pattern to console.
     */
    void ListAllNPCs(const char* pattern);

    /**
     * List the amount of npc currently loaded.
     */
    size_t GetNpcListAmount()
    {
        return npcs.GetSize();
    }

    /**
     * Retrive the current tick counter
     */
    unsigned int GetTickCounter() const
    {
        return tick_counter;
    }

    /**
     * Find one npc and print its current state info.
     */
    bool DumpNPC(const char* pattern);

    /**
     * Find one npc and print its current info.
     */
    bool InfoNPC(const char* pattern);

    /**
     * List all known entities on superclient.
     */
    void ListAllEntities(const char* pattern, bool onlyCharacters = false);

    /**
     * List all tribes matching pattern to console.
     */
    void ListTribes(const char* pattern);

    /**
     * List all the active recipes in a tribe
     */
    void ListTribeRecipes(const char* tribeID);

    /**
     * List all the registered reactions
     */
    void ListReactions(const char* pattern);

    /**
     * List all waypoints matching pattern to console.
     */
    void ListWaypoints(const char* pattern);

    /**
     * List all paths matching pattern to console.
     */
    void ListPaths(const char* pattern);

    /**
     * List all locations matching pattern to console.
     */
    void ListLocations(const char* pattern);

    /**
     * Special handling for death notifications from the server
     * is required, to stop behavior scripting, delete hate lists,
     * etc.
     */
    void HandleDeath(NPC* who);

    void Add(gemNPCObject* object);
    void Remove(gemNPCObject* object);
    void RemoveAll();

    /** Remove NPC from NPC Client.
     */
    void Remove(NPC* npc);

    gemNPCObject* FindCharacterID(PID pid);
    gemNPCObject* FindEntityID(EID eid);

    /**
     * Find named entity.
     *
     * Return first entity that macht the given name.
     */
    gemNPCObject* FindEntityByName(const char* name);

    /**
     * Loop through every tribe and check if this npc is a member.
     *
     * @return false if tribe member and failed to attach, otherwise true
     */
    bool CheckAttachTribes(NPC* npc);

    void AttachNPC(gemNPCActor* actor, uint8_t DRcounter, EID ownerEID, PID masterID);

    iCollideSystem* GetCollDetSys()
    {
        return cdsys;
    }

    NPC* ReadSingleNPC(PID char_id, PID master_id = 0);

    /**
     * Clones a master npc to a new npc whith the passed PID.
     *
     * Used to inheredit behaviours from other npc.
     *
     * @note this is used as last shore scenario. This way the npc even
     *       if mastered can override the entry of it's master.
     *       The master PID comes from psserver.
     *
     * @param char_id A valid PID of the character which is taking it's
     *                attributes from the master npc.
     * @param master_id A valid PID of the master npc from which we will
     *                  copy the attributes.
     * @return A pointer to the newly created NPC. NULL if the arguments
     *         where invalid or the master wasn't found.
     */
    NPC* ReadMasteredNPC(PID char_id, PID master_id);

    /**
     * Update with time from server in order to start timed events.
     */
    void UpdateTime(int minute, int hour, int day, int month, int year);

    iCelHNavStruct*  GetNavStruct()
    {
        return navStruct;
    }

    iCelHPath* ShortestPath(NPC* npc, const csVector3 &from, iSector* fromSector, const csVector3 &goal, iSector* goalSector);

    psWorld*  GetWorld()
    {
        return world;
    }

    iEngine*  GetEngine()
    {
        return engine;
    }

    iVFS*   GetVFS()
    {
        return vfs;
    }

    /** @name Game time
     * Functions to retrive the game time.
     */
    ///@{

    /**
     * Get the minute of the game time of day.
     */
    int GetGameTODMinute()
    {
        return gameMinute;
    }

    /**
     * Get the hour of the game time of day.
     */
    int GetGameTODHour()
    {
        return gameHour;
    }

    /**
     * Get the day of the game time of day.
     */
    int GetGameTODDay()
    {
        return gameDay;
    }

    /**
     * Get the month of the game time of day.
     */
    int GetGameTODMonth()
    {
        return gameMonth;
    }

    /**
     * Get the year of the game time of day.
     */
    int GetGameTODYear()
    {
        return gameYear;
    }
    ///@}

    /**
     * Attach a server gemObject to a Crystal Space object.
     *
     * In most cases this will be a mesh wrapper.
     *
     * @param object The Crystal Space object we want to attach our gemObject to.
     * @param gobject The PlaneShift object that we want to attach.
     */
    void AttachObject(iObject* object, gemNPCObject* gobject);

    /**
     * Unattach a gemObject from a Crystal Space object.
     *
     * In most cases the Crystal Space object is a meshwrapper.
     *
     * @param object The Crystal Space object we want to unattach our object from.
     * @param gobject The gem object we want to unattach.
     */
    void UnattachObject(iObject* object, gemNPCObject* gobject);

    /**
     * See if there is a gemObject attached to a given Crystal Space object.
     *
     * @param object The Cyrstal Space object we want to see if there is an object attached to.
     *
     * @return A gemNPCObject if it exists that is attached to the Crystal Space object.
     */
    gemNPCObject* FindAttachedObject(iObject* object);


    /**
     * Create a list of all nearby gem objects.
     *
     * @param sector The sector to check in.
     * @param pos The starting position
     * @param radius The distance around the starting point to check.
     * @param doInvisible If true check invisible meshes otherwise ignore them.
     *
     * @return A csArray<> of all the objects in the given radius.
     */
    csArray<gemNPCObject*> FindNearbyEntities(iSector* sector, const csVector3 &pos, float radius, bool doInvisible = false);

    /**
     * Create a list of all nearby gem actors.
     *
     * @param sector The sector to check in.
     * @param pos The starting position
     * @param radius The distance around the starting point to check.
     * @param doInvisible If true check invisible meshes otherwise ignore them.
     *
     * @return A csArray<> of all the objects in the given radius.
     */
    csArray<gemNPCActor*> FindNearbyActors(iSector* sector, const csVector3 &pos, float radius, bool doInvisible = false);

    /**
     * Load and return the root node of an xml file.
     *
     * @param xmlfile Name of the xml file to find the root node of.
     */
    csPtr<iDocumentNode> GetRootNode(const char* xmlfile);

    /**
     * Add an NPCType
     *
     * Upon loading a tribe, the Recipe Manager creates a NPCType and
     * uses this method to Put the assembled npctype into the npctypes map.
     *
     * @param newType String containing the npctype in xml format.
     * @return        False if any error is encountered, true otherwise.
     */
    bool AddNPCType(csString newType);

    /**
     * Decode a npc type from the XML definition.
     *
     * @param root The document node that containts a NPC type.
     */
    bool LoadNPCTypes(iDocumentNode* root);

    /**
     * Load NPC Types from DB.
     */
    bool LoadNPCTypes();

    /**
     * Fire perception
     *
     * Method used by the console command 'fireperc'.
     * It fires a perception on the given npc.
     *
     * @param NPCId      NPC on which to fire perc
     * @param perception The perception to fire
     * @return           True if npc is found and perception is sent. False otherwise (doh!)
     */
    bool FirePerception(int NPCId, const char* perception);

    /**
     * made public for the networkmanager to call it
     */
    bool LoadPathNetwork();

    /**
     * Return the path network of the npcclient
     */
    psPathNetwork* GetPathNetwork()
    {
        return pathNetwork;
    }

    /**
     * Return the location manager of the npcclient
     */
    LocationManager* GetLocationManager()
    {
        return locationManager;
    }

    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    virtual const char* ToString();
    ///@}

protected:
    /**
     * Read NPCs from the DB.
     */
    bool ReadNPCsFromDatabase();

    /**
     * Load locations from the DB.
     */
    bool LoadLocations();

    /**
     * Load Tribes from the DB.
     */
    bool LoadTribes();

    /**
     * Find all items that are close to NPC's and percept the
     * NPC.
     */
    void PerceptProximityItems();

    /**
     * Find all locations that are close to NPC's and percept the
     * NPC.
     */
    void PerceptProximityLocations();

    /**
     * Find all actors that are in a tribe home and percept tribe and actor if it is an NPC.
     */
    void PerceptProximityTribeHome();

public:
    static psNPCClient*             npcclient;
protected:
    psNetConnection*                connection;
    iObjectRegistry*                objreg;
    csRef<iEngine>                  engine;
    csRef<iConfigManager >          configmanager;
    csRef<MsgHandler>               msghandler;
    ServerConsole*                  serverconsole;
    EventManager*                   eventmanager;
    RecipeManager*                  recipemanager;
    NetworkManager*                 network;
    psDatabase*                     database;
    csRef<iVFS>                     vfs;

    csHash<NPCType*, const char*>   npctypes;
    LocationManager*                locationManager;         ///< The manager for location.
    MathScriptEngine*               mathScriptEngine;
    psPathNetwork*                  pathNetwork;
    csRef<iCelHNavStruct>           navStruct;
    csPDelArray<NPC>                npcs;
    csArray<DeferredNPC>            npcsDeferred;
    csPDelArray<Tribe>              tribes;
    csHash<gemNPCObject*, EID>      all_gem_objects_by_eid;
    csHash<gemNPCObject*, PID>      all_gem_objects_by_pid;
    csPDelArray<gemNPCObject>       all_gem_objects;
    csArray<gemNPCItem*>            all_gem_items;
    csArray<gemNPCActor*>           all_gem_actors;

    csHash<psNPCRaceListMessage::NPCRaceInfo_t,csString>     raceInfos; ///< Information about all the races.

    csHash<NPC*,csString>           allReactions;     ///< Hash of all registered reactions.
    csArray<csString>               notUsedReactions; ///< List of not matched reactions.

    csRef<iCollideSystem>           cdsys;

    psWorld*                        world;

    //    psPFMaps*                       PFMaps;

    /// Counter used to start events at every nth client tick
    unsigned int                    tick_counter;

    /// Counters used to distribute load regarding perception generation
    int                             current_long_range_perception_index;
    int                             current_long_range_perception_loc_index;
    int                             current_tribe_home_perception_index;

    // Game Time
    int                             gameMinute;
    int                             gameHour;
    int                             gameDay;
    int                             gameMonth;
    int                             gameYear;
    csTicks                         gameTimeUpdated;

};

/** @} */

#endif

