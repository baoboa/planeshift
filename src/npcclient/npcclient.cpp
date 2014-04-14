/*
* npcclient.cpp - author: Keith Fulton <keith@paqrat.com>
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
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/cmdline.h>
#include <iutil/object.h>
#include <iutil/stringarray.h>
#include <csutil/csobject.h>
#include <ivaria/reporter.h>
#include <iutil/vfs.h>
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>
#include <iengine/mesh.h>
#include <iengine/engine.h>

//=============================================================================
// Project Includes
//=============================================================================
#include <ibgloader.h>
#include "util/serverconsole.h"
#include "util/psdatabase.h"
#include "util/eventmanager.h"
#include "util/location.h"
#include "util/waypoint.h"
#include "util/psstring.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "util/pspathnetwork.h"
#include <tools/celhpf.h>
#include "util/mathscript.h"

#include "net/connection.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcclient.h"
#include "pathfind.h"
#include "networkmgr.h"
#include "npcbehave.h"
#include "npc.h"
#include "perceptions.h"
#include "gem.h"
#include "recipe.h"
#include "tribe.h"
#include "status.h"

bool running;

class psNPCClientTick : public psGameEvent
{
protected:
    static psNPCClient* client;

public:
    psNPCClientTick(int offsetticks, psNPCClient* c);
    virtual void Trigger();  // Abstract event processing function
    virtual csString ToString() const;
};

psNPCClient* psNPCClient::npcclient = NULL;

psNPCClient::psNPCClient() : serverconsole(NULL)
{
    npcclient     = this;  // Static pointer to self
    connection    = NULL;
    objreg        = NULL;
    locationManager = NULL;
    mathScriptEngine = NULL;
    world         = NULL;
    //    PFMaps       = NULL;
    pathNetwork   = NULL;
    eventmanager  = NULL;
    recipemanager = NULL;
    running       = true;
    database      = NULL;
    network       = NULL;
    tick_counter  = 0;
    current_long_range_perception_index = 0;
    current_long_range_perception_loc_index = 0;
    current_tribe_home_perception_index = 0;
    gameMinute = gameHour = gameDay = gameMonth = gameYear = 0;
    gameTimeUpdated = 0;
}

psNPCClient::~psNPCClient()
{
    csHash<NPCType*, const char*>::GlobalIterator npcTypeIter(npctypes.GetIterator());
    while(npcTypeIter.HasNext())
        delete npcTypeIter.Next();
    npctypes.Empty();

    delete locationManager;
    delete mathScriptEngine;

    running = false;
    delete connection;
    delete network;
    delete serverconsole;
    delete database;


    delete pathNetwork;
    //    delete PFMaps;
    delete world;

    // Delete Recipe Manager
    delete recipemanager;

    gemNPCObject::FiniMesh();
}

bool psNPCClient::Initialize(iObjectRegistry* object_reg,const char* _host, const char* _user, const char* _pass, int _port)
{
    objreg = object_reg;

    configmanager =  csQueryRegistry<iConfigManager> (object_reg);
    if(!configmanager)
    {
        CPrintf(CON_ERROR, "Couldn't find Configmanager!\n");
        return false;
    }

    engine =  csQueryRegistry<iEngine> (object_reg);
    if(!engine)
    {
        CPrintf(CON_ERROR, "Couldn't find Engine!\n");
        return false;
    }


    // Load the log settings
    LoadLogSettings();

    // Start Database

    database = new psDatabase(object_reg);

    csString db_host, db_user, db_pass, db_name;
    unsigned int db_port;

    db_host = configmanager->GetStr("PlaneShift.Database.npchost", "localhost");
    db_user = configmanager->GetStr("PlaneShift.Database.npcuserid", "planeshift");
    db_pass = configmanager->GetStr("PlaneShift.Database.npcpassword", "planeshift");
    db_name = configmanager->GetStr("PlaneShift.Database.npcname", "planeshift");
    db_port = configmanager->GetInt("PlaneShift.Database.npcport", 0);

    Debug4(LOG_STARTUP,0,COL_BLUE "Database Host: '%s' User: '%s' Databasename: '%s'\n" COL_NORMAL,
           (const char*) db_host, (const char*) db_user, (const char*) db_name);

    if(!database->Initialize(db_host, db_port, db_user, db_pass, db_name))
    {
        Error2("Could not create database or connect to it: %s\n", database->GetLastError());
        delete database;
        return false;
    }

    csString user,pass,host;
    int port;

    host = _host ? _host : configmanager->GetStr("PlaneShift.NPCClient.host", "localhost");
    user = _user ? _user : configmanager->GetStr("PlaneShift.NPCClient.userid", "superclient");
    pass = _pass ? _pass : configmanager->GetStr("PlaneShift.NPCClient.password", "planeshift");
    port = _port ? _port : configmanager->GetInt("PlaneShift.NPCClient.port", 13331);

    CPrintf(CON_DEBUG, "Initialize Network Thread...\n");
    connection = new psNetConnection(500); // 500 elements in queue
    if(!connection->Initialize(object_reg))
    {
        CPrintf(CON_ERROR, "Network thread initialization failed!\n");
        delete connection;
        return false;
    }

    mathScriptEngine = new MathScriptEngine(db,"math_script");
    eventmanager = new EventManager;
    recipemanager = new RecipeManager(this, eventmanager);
    msghandler   = eventmanager;
    psMessageCracker::msghandler = eventmanager;

    // Start up network
    if(!msghandler->Initialize(connection))
    {
        return false;                // Attach to incoming messages.
    }
    network = new NetworkManager(msghandler,connection, engine);

    vfs =  csQueryRegistry<iVFS> (object_reg);
    if(!vfs)
    {
        CPrintf(CON_ERROR, "could not open VFS\n");
        exit(1);
    }

    world = new psWorld();
    if(!world)
    {
        CPrintf(CON_ERROR, "could not create world\n");
        exit(1);
    }
    world->Initialize(object_reg);

    if(!LoadNPCTypes())
    {
        CPrintf(CON_ERROR, "Couldn't load the npctypes\n");
        exit(1);
    }

    if(!recipemanager->LoadRecipes())
    {
        CPrintf(CON_ERROR, "Couldn't load the recipes table\n");
        exit(1);
    }

    if(!LoadTribes())
    {
        CPrintf(CON_ERROR, "Couldn't load the tribes table\n");
        exit(1);
    }

    if(!ReadNPCsFromDatabase())
    {
        CPrintf(CON_ERROR, "Couldn't load the npcs from db\n");
        exit(1);
    }

    if(!LoadLocations())
    {
        CPrintf(CON_ERROR, "Couldn't load the sc_location_type table\n");
        exit(1);
    }

    cdsys =  csQueryRegistry<iCollideSystem> (objreg);

    //    PFMaps = new psPFMaps(objreg);

    CPrintf(CON_CMDOUTPUT,"Filling loader cache\n");

    csRef<iBgLoader> loader = csQueryRegistry<iBgLoader>(object_reg);
    csRef<iThreadManager> threadManager = csQueryRegistry<iThreadManager>(object_reg);

    // load materials
    loader->PrecacheDataWait("/planeshift/materials/materials.cslib");

    csRefArray<iThreadReturn> precaches;

    // load meshes
    csRef<iStringArray> meshes = vfs->FindFiles("/planeshift/meshes/");
    for(size_t j=0; j<meshes->GetSize(); ++j)
    {
        precaches.Push(loader->PrecacheData(meshes->Get(j)));
    }
    threadManager->Wait(precaches);
    precaches.Empty();
    meshes->Empty();

    // load maps
    csRef<iStringArray> maps = vfs->FindFiles("/planeshift/world/");
    for(size_t j=0; j<maps->GetSize(); ++j)
    {
        precaches.Push(loader->PrecacheData(maps->Get(j)));
    }
    threadManager->Wait(precaches);
    precaches.Empty();
    maps->Empty();

    // clear up data that is only required parse time
    loader->ClearTemporaryData();

    CPrintf(CON_CMDOUTPUT,"Loader cache filled\n");

    CPrintf(CON_DEBUG, "Connecting to Host: '%s' User: '%s' Password: '%s' Port %d...\n",
            (const char*) host, (const char*) user, (const char*) pass, port);
    if(!connection->Connect(host,port))
    {
        CPrintf(CON_ERROR, "Couldn't resolve hostname %s on port %d.\n",(const char*)host,port);
        exit(1);
    }


    // Starts the logon process
    network->Authenticate(host,port,user,pass);

    NPCStatus::Initialize(objreg);

    return true;
}

void psNPCClient::MainLoop()
{
    csRef<iCommandLineParser> cmdline =
        csQueryRegistry<iCommandLineParser> (objreg);

    if(cmdline)
    {
        const char* ofile = cmdline->GetOption("output");
        if(ofile != NULL)
        {
            ConsoleOut::SetOutputFile(ofile, false);
        }
        else
        {
            const char* afile = cmdline->GetOption("append");
            if(afile != NULL)
            {
                ConsoleOut::SetOutputFile(afile, true);
            }
        }
    }
    ConsoleOut::SetMaximumOutputClassStdout(CON_SPAM);
    ConsoleOut::SetMaximumOutputClassFile(CON_SPAM);

    // Start the server console (and handle -run=file).
    serverconsole = new ServerConsole(objreg, "psnpcclient", "NPC Client");
    serverconsole->SetCommandCatcher(this);

    // Enter the real main loop - handling events and messages.
    eventmanager->Run();

    // Save log settings
    SaveLogSettings();
}

void psNPCClient::Disconnect()
{
    network->Disconnect();
}

void psNPCClient::LoadLogSettings()
{
    int count=0;
    for(int i=0; i< MAX_FLAGS; i++)
    {
        if(pslog::GetName(i))
        {
            pslog::SetFlag(pslog::GetName(i),configmanager->GetBool(pslog::GetSettingName(i)),0);
            if(configmanager->GetBool(pslog::GetSettingName(i)))
                count++;
        }
    }
    if(count==0)
    {
        CPrintf(CON_CMDOUTPUT,"All LOGS are off.\n");
    }
}

void psNPCClient::SaveLogSettings()
{
    for(int i=0; i< MAX_FLAGS; i++)
    {
        if(pslog::GetName(i))
        {
            configmanager->SetBool(pslog::GetSettingName(i),pslog::GetValue(pslog::GetName(i)));
        }
    }

    configmanager->Save();
}


csPtr<iDocumentNode> psNPCClient::GetRootNode(const char* xmlfile)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDataBuffer> buff = vfs->ReadFile(xmlfile);

    if(!buff || !buff->GetSize())
    {
        return NULL;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(buff);
    if(error)
    {
        Error3("%s in %s", error, xmlfile);
        return NULL;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", xmlfile);
        return NULL;
    }
    return csPtr<iDocumentNode>(root);
}

Tribe* psNPCClient::GetTribe(int id)
{
    for(size_t i=0; i<tribes.GetSize(); i++)
    {
        if(tribes[i]->GetID() == id)
        {
            return tribes[i];
        }
    }
    return NULL;
}

bool psNPCClient::AddNPCType(csString newType)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument>       doc = xml->CreateDocument();
    const char*            error = doc->Parse(newType);
    if(error)
    {
        Error2("Error parsing assembled xml: %s.", error);
        return false;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    csRef<iDocumentNode> node = root->GetNode("npctype");
    if(!node)
    {
        Error1("Error. No root node on xml.");
        return false;
    }

    NPCType* npctype = new NPCType();
    if(npctype->Load(node))
    {
        npctypes.Put(npctype->GetName(), npctype);
        return true;
    }
    else
    {
        delete npctype;
        return false;
    }
}

bool psNPCClient::LoadNPCTypes(iDocumentNode* root)
{

    csRef<iDocumentNode> topNode = root->GetNode("npctypes");
    if(!topNode)
    {
        Error1("No <npctypes> tag");
        return false;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a NPC so load it.
        if(strcmp(node->GetValue(), "npctype") == 0)
        {
            NPCType* npctype = new NPCType();
            if(npctype->Load(node))
            {
                npctypes.Put(npctype->GetName(), npctype);
            }
            else
            {
                delete npctype;
                return false;
            }
        }
    }

    return true;
}

bool psNPCClient::LoadNPCTypes()
{
    // First clear the npctypes in case load npc types is used as a reload.
    csHash<NPCType*, const char*>::GlobalIterator npcTypeIter(npctypes.GetIterator());
    while(npcTypeIter.HasNext())
        delete npcTypeIter.Next();
    npctypes.Empty();


    csArray<unsigned long> postponedNPCTypeID;
    Result rs(db->Select("SELECT * from sc_npctypes ORDER BY id"));

    if(!rs.IsValid() || !(rs.Count() >= 1))
    {
        Error2("Could not load npctypes from db: %s Loading from file",db->GetLastError());
        csRef<iDocumentNode> root = GetRootNode("/this/data/npcbehave.xml");

        if(!root.IsValid() || !LoadNPCTypes(root))
        {
            CPrintf(CON_ERROR, "Couldn't load the npctypes table\n");
            return false;
        }
        return true;
    }


    for(unsigned long i = 0; i < rs.Count(); i++)
    {
        NPCType* npctype = new NPCType();
        if(npctype->Load(rs[i]))
        {
            npctypes.Put(npctype->GetName(), npctype);
        }
        else
        {
            delete npctype;
            //we keep these for a more deep inspection
            postponedNPCTypeID.Push(i);
        }
    }

    size_t lastSize = 0;
    //try reloading all the failures till the size of failed ones changes and it's > 0
    //as soon as one iteration didn't load anything we will exit with failure
    while(postponedNPCTypeID.GetSize() != 0 && lastSize != postponedNPCTypeID.GetSize())
    {
        lastSize = postponedNPCTypeID.GetSize(); //keep the starting amount of items
        for(size_t y = 0; y < postponedNPCTypeID.GetSize(); y++)
        {
            //maybe we should reorder them by dependency? the issue with that is that
            //we would need to know more about the items inner working.
            NPCType* npctype = new NPCType();
            if(npctype->Load(rs[postponedNPCTypeID.Get(y)]))
            {
                npctypes.Put(npctype->GetName(), npctype);
                postponedNPCTypeID.DeleteIndex(y);
                y--;
            }
            else
            {
                delete npctype;
            }
        }
    }

    if(postponedNPCTypeID.IsEmpty())
    {
        CPrintf(CON_WARNING, "Loaded %d NPC Types.\n", rs.Count());
    }
    else
        CPrintf(CON_WARNING, "Tried to load %d NPC Types, but %d failed.\n", rs.Count(), postponedNPCTypeID.GetSize());

    //success only if the array is empty at this point, else failure.
    return postponedNPCTypeID.IsEmpty();
}

bool psNPCClient::FirePerception(int NPCId, const char* perception)
{
    for(size_t i=0; i<npcs.GetSize(); i++)
    {

        if((int)npcs[i]->GetPID().Unbox() == NPCId)
        {
            npcs[i]->TriggerEvent(perception);
            return true;
        }

    }
    return false;
}

void psNPCClient::Add(gemNPCObject* object)
{
    EID eid = object->GetEID();

    all_gem_objects.Push(object);

    gemNPCItem* item = dynamic_cast<gemNPCItem*>(object);
    if(item)
    {
        all_gem_items.Push(item);

        // Is this a tribe Item with tribe ID?
        if(item->GetTribeID() != 0)
        {
            Tribe* tribe = GetTribe(item->GetTribeID());
            if(tribe)
            {
                tribe->HandlePersistItem(item);
            }
        }
        else
        {
            // Is this an asset without tribe ID?
            for(size_t i=0; i<tribes.GetSize(); i++)
            {
                Tribe::Asset* asset = tribes[i]->GetAsset(item->GetUID());
                if(asset)
                {
                    asset->item = item;
                }
            }
        }
    }

    gemNPCActor* actor = dynamic_cast<gemNPCActor*>(object);
    if(actor)
    {
        all_gem_actors.Push(actor);
    }

    all_gem_objects_by_eid.Put(eid, object);
    if(object->GetPID().IsValid())
    {
        all_gem_objects_by_pid.Put(object->GetPID(), object);
    }

    Notify2(LOG_CELPERSIST,"Added gemNPCObject(%s)\n", ShowID(eid));
}

void psNPCClient::Remove(gemNPCObject* object)
{
    NPC* npc = object->GetNPC();
    if(npc)
    {
        NPCDebug(npc, 5, "Removing entity");
    }

    EID EID = object->GetEID();

    // Remove entity from all hated lists.
    for(size_t x=0; x<npcs.GetSize(); x++)
    {
        npcs[x]->RemoveFromHateList(EID);
    }

    all_gem_objects_by_eid.DeleteAll(EID);
    if(object->GetPID().IsValid())
    {
        all_gem_objects_by_pid.DeleteAll(object->GetPID());
    }

    gemNPCItem* item = dynamic_cast<gemNPCItem*>(object);
    if(item)
    {
        size_t n = all_gem_items.Find(item);
        if(n != csArrayItemNotFound)
        {
            all_gem_items.DeleteIndexFast(n);
        }

        Tribe::Asset* asset = NULL;
        Tribe* tribe = NULL;
        // Is this a tribe Item with tribe ID?
        if(item->GetTribeID() != 0)
        {
            tribe = GetTribe(item->GetTribeID());
            if(tribe)
            {
                asset = tribe->GetAsset(item->GetUID());
            }
        }
        else
        {
            // Is this an asset without tribe ID?
            for(size_t i=0; i<tribes.GetSize(); i++)
            {
                tribe = tribes[i];
                asset = tribe->GetAsset(item->GetUID());
                if(asset) break;
            }
        }
        if(asset && tribe)
        {
            tribe->RemoveAsset(asset);
        }

    }

    gemNPCActor* actor = dynamic_cast<gemNPCActor*>(object);
    if(actor)
    {
        size_t n = all_gem_actors.Find(actor);
        if(n != csArrayItemNotFound)
        {
            all_gem_actors.DeleteIndexFast(n);
        }
    }

    size_t n = all_gem_objects.Find(object);
    if(n != csArrayItemNotFound)
    {
        all_gem_objects.DeleteIndexFast(n);
    }

    Notify2(LOG_CELPERSIST,"removed gemNPCObject(%s)\n", ShowID(EID));
}

void psNPCClient::RemoveAll()
{
//    size_t i;
//    for (i=0; i<all_gem_objects.GetSize(); i++)
//    {
//        UnattachNPC(all_gem_objects[i]->GetEntity(),FindAttachedNPC(all_gem_objects[i]->GetEntity()));
//    }

    all_gem_items.DeleteAll();
    all_gem_actors.DeleteAll();
    all_gem_objects_by_eid.DeleteAll();
    all_gem_objects_by_pid.DeleteAll();
    all_gem_objects.DeleteAll();
}

void psNPCClient::Remove(NPC* npc)
{
    npcs.Delete(npc);
}


gemNPCObject* psNPCClient::FindEntityID(EID EID)
{
    return all_gem_objects_by_eid.Get(EID, 0);
}

gemNPCObject* psNPCClient::FindEntityByName(const char* name)
{
    for(size_t i=0; i < all_gem_objects.GetSize(); i++)
    {
        gemNPCObject* object = all_gem_objects[i];

        if(strcmp(object->GetName(),name) == 0)
        {
            return object;
        }
    }
    return NULL;
}

gemNPCObject* psNPCClient::FindCharacterID(PID PID)
{
    return all_gem_objects_by_pid.Get(PID, 0);
}

NPC* psNPCClient::ReadSingleNPC(PID char_id, PID master_id)
{
    Result result(db->Select("SELECT * FROM sc_npc_definitions WHERE char_id=%u",
                             master_id.IsValid() ? master_id.Unbox() : char_id.Unbox()));
    if(!result.IsValid() || !result.Count())
        return NULL;

    NPC* newnpc = new NPC(this, network, world, engine, cdsys);

    if(newnpc->Load(result[0],npctypes, eventmanager, master_id.IsValid() ? char_id : 0))
    {
        newnpc->Tick();
        npcs.Push(newnpc);
        return newnpc;
    }
    else
    {
        delete newnpc;
        return NULL;
    }
}

NPC* psNPCClient::ReadMasteredNPC(PID char_id, PID master_id)
{
    if(!char_id.IsValid() || !master_id.IsValid())
        return NULL;

    NPC* npc = FindNPCByPID(master_id);
    if(!npc) //the npc wasn't found in the loaded npc.
    {
        //try loading the master if missing.
        npc = ReadSingleNPC(master_id);
        if(!npc) //if not found bail out
            return NULL;
    }

    //create a new npc
    NPC* newnpc = new NPC(this, network, world, engine, cdsys);
    //copy the data from the master npc
    newnpc->Load(npc->GetName(), char_id, npc->GetBrain(), npc->GetRegionName(),
                 npc->IsDebugging(), npc->IsDisabled(), eventmanager);
    newnpc->Tick();
    npcs.Push(newnpc);
    return newnpc;
}

// iScriptableVar interface for MathScripts
double psNPCClient::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "gameYear")
    {
        return gameYear;
    }
    if(property == "gameMonth")
    {
        return gameMonth;
    }
    if(property == "gameHour")
    {
        return gameHour;
    }
    if(property == "gameMinute")
    {
        return gameMinute;
    }
    Error2("Requested psNPCClient property not found '%s'", ptr);
    return 0.0;
}

// iScriptableVar interface for MathScripts
double psNPCClient::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    csString function(functionName);

    Error2("Requested psNPCClient function not found '%s'", functionName);
    return 0.0;
}

const char* psNPCClient::ToString()
{
    return "NPCClient";
}


bool psNPCClient::ReadNPCsFromDatabase()
{
    Result rs(db->Select("select * from sc_npc_definitions"));

    if(!rs.IsValid())
    {
        Error2("Could not load npcs from db: %s",db->GetLastError());
        return false;
    }
    int loadedNPCs = 0;
    for(unsigned long i=0; i<rs.Count(); i++)
    {
        csString name = rs[i]["name"];
        // Familiars are not loaded until neeeded.
        if(name.StartsWith("FAMILIAR:"))
        {
            DeferredNPC defNPC;
            defNPC.id = rs[i].GetInt("char_id");
            defNPC.name = name;
            npcsDeferred.Push(defNPC);
            continue;
        }

        // creates a new NPC in NPCClient only
        NPC* npc = new NPC(this, network, world, engine, cdsys);
        // loads the NPC brain
        if(npc->Load(rs[i],npctypes, eventmanager, 0))
        {
            npc->Tick();
            npcs.Push(npc);

            // check if NPC is part of a tribe
            if(!CheckAttachTribes(npc))
            {
                return false;
            }
            else if(npc->GetTribe())
            {
                // Overwrite brain with tribe brain
                int id = npc->GetTribe()->GetID();
                csString typeName = "tribe_";
                typeName.Append(id);
                NPCType* brain = npctypes.Get(typeName.GetData(), NULL);
                if(!brain)
                {
                    Error3("Failed to load tribe brain for NPC %s(%s)",npc->GetName(),ShowID(npc->GetPID()));
                    return false;
                }
                npc->SetBrain(brain);
                loadedNPCs++;
                // this NPC is not part of a tribe
            }
            else
            {
                // disable the NPC by default, we will activate only the ones the server sends us
                npc->Disable();
            }
        }
        else
        {
            delete npc;
            return false;
        }
    }

    CPrintf(CON_WARNING, "Loaded %d NPCs from sc_npc_definitions.\n", loadedNPCs);
    return true;
}

bool psNPCClient::LoadPathNetwork()
{
    csRef<iCelHNavStructBuilder> builder = csQueryRegistry<iCelHNavStructBuilder>(objreg);
    if(!builder.IsValid())
    {
        Error1("Couldn't find builder");
        return false;
    }
    csString navmesh = configmanager->GetStr("PlaneShift.NPCClient.NavMesh","/planeshift/navmesh");
    navStruct = builder->LoadHNavStruct(vfs, navmesh);

    if(!navStruct.IsValid())
    {
        Error1("Navigation mesh is not valid, check /planeshift/navmesh");
        return false;
    }

    pathNetwork = new psPathNetwork();
    return pathNetwork->Load(engine, db, world);
}

bool psNPCClient::LoadLocations()
{
    locationManager = new LocationManager;

    if(!locationManager->Load(engine, db))
    {
        Error1("Could not load locations.");
        return false;
    }

    return true;
}


bool psNPCClient::LoadTribes()
{
    Result rs(db->Select("SELECT t.*, s.name AS home_sector_name FROM tribes t, sectors s WHERE s.id = t.home_sector_id"));

    if(!rs.IsValid())
    {
        Error2("Could not load tribes from db: %s",db->GetLastError());
        return false;
    }
    for(int i=0; i<(int)rs.Count(); i++)
    {
        Tribe* tribe = new Tribe(eventmanager, recipemanager);

        if(tribe->Load(rs[i]))
        {
            tribes.Push(tribe);

            {
                // Start Load Members scope
                Result rs2(db->Select("select * from tribe_members where tribe_id=%d",tribe->GetID()));
                if(!rs2.IsValid())
                {
                    Error2("Could not load tribe members from db: %s",db->GetLastError());
                    return false;
                }
                for(int j=0; j<(int)rs2.Count(); j++)
                {
                    tribe->LoadMember(rs2[j]);
                }
            } // End Load Members scope

            {
                // Start Load Memories scope
                Result rs2(db->Select("select m.*,s.name AS sector_name from sc_tribe_memories m, sectors s WHERE s.id = m.sector_id and tribe_id=%d",tribe->GetID()));
                if(!rs2.IsValid())
                {
                    Error2("Could not load tribe members from db: %s",db->GetLastError());
                    return false;
                }
                for(int j=0; j<(int)rs2.Count(); j++)
                {
                    tribe->LoadMemory(rs2[j]);
                }
            } // End Load Memories scope

            {
                // Start Load Resources scope
                Result rs2(db->Select("select * from sc_tribe_resources WHERE tribe_id=%d",tribe->GetID()));
                if(!rs2.IsValid())
                {
                    Error2("Could not load tribe resources from db: %s",db->GetLastError());
                    return false;
                }
                for(int j=0; j<(int)rs2.Count(); j++)
                {
                    tribe->LoadResource(rs2[j]);
                }
            } // End Load Resources scope

            {
                // Start Load Knowledge scope
                Result rs2(db->Select("select * from sc_tribe_knowledge WHERE tribe_id=%d", tribe->GetID()));
                if(!rs2.IsValid())
                {
                    Error2("Could not load tribe knowledge from db: %s", db->GetLastError());
                    return false;
                }
                for(size_t i=0; i<rs2.Count(); i++)
                {
                    tribe->AddKnowledge(rs2[i]["knowledge"]);
                }
            } // End Load Knowledge scope

            {
                // Start Load Assets scope
                Result rs2(db->Select("select a.*,s.name AS sector_name from sc_tribe_assets a LEFT JOIN sectors s on s.id = a.sector_id WHERE tribe_id=%d", tribe->GetID()));
                if(!rs2.IsValid())
                {
                    Error2("Could not load tribe assets from db: %s", db->GetLastError());
                    return false;
                }
                for(size_t i=0; i<rs2.Count(); i++)
                {
                    tribe->LoadAsset(rs2[i]);
                }
            } // End Load Assets scope

            // Enroll at Recipe Manager
            if(!recipemanager->AddTribe(tribe))
            {
                return false;
            }
        }
        else
        {
            delete tribe;
            return false;
        }
    }

    CPrintf(CON_WARNING, "Loaded %d Tribes.\n", rs.Count());

    return true;
}

bool psNPCClient::CheckAttachTribes(NPC* npc)
{
    // Check if this NPC is part of a tribe
    for(size_t j=0; j<tribes.GetSize(); j++)
    {
        // Check if npc is part of the tribe and if so attach the npc
        if(!tribes[j]->CheckAttach(npc))
        {
            return false;
        }
    }
    return true;
}


void psNPCClient::AttachNPC(gemNPCActor* actor, uint8_t DRcounter, EID ownerEID, PID masterID)
{
    if(!actor) return;

    NPC* npc = NULL;

    // Check based on characterID
    npc = FindNPCByPID(actor->GetPID());
    if(!npc)
    {
        npc = ReadSingleNPC(actor->GetPID()); //try reloading the tables if we didn't find it
        if(!npc) //still not found. we do a last check
        {
            if(masterID.IsValid()) //Probably it's mastered. Try loading the master for this
            {
                npc = ReadMasteredNPC(actor->GetPID(), masterID); //loads the master npc data and assign it to this
            }
            if(!npc) //last chance if false good bye
            {
                Error3("NPC %s(%s) was not found in scripted npcs for this npcclient.\n",
                       actor->GetName(), ShowID(actor->GetPID()));
                return;
            }
        }
    }

    npc->SetOwner(ownerEID);

    actor->AttachNPC(npc);

    if(DRcounter != (uint8_t) -1)
    {
        npc->SetDRCounter(DRcounter);
    }

    CheckAttachTribes(npc);

    NPCDebug(npc, 5, "We are now managing NPC <%s, %s, %s>.\n", actor->GetName(), ShowID(actor->GetPID()), ShowID(actor->GetEID()));

    // Test if this actor is in a valid starting position.
    npc->CheckPosition();

    // Store the spawn position
    npc->StoreSpawnPosition();

    // Report final correct starting position.
    GetNetworkMgr()->QueueDRData(npc);
}



NPC* psNPCClient::FindNPCByPID(PID character_id)
{
    for(size_t x=0; x<npcs.GetSize(); x++)
    {
        if(npcs[x]->GetPID() == character_id)
            return npcs[x];
    }

    for(size_t j=0; j<npcsDeferred.GetSize(); j++)
    {
        if(npcsDeferred[j].id == character_id)
        {
            NPC* npc = ReadSingleNPC(character_id);
            if(npc)
            {
                // Read single npc added the new npc to the npcs list. Now remove
                // from the deferred list.
                npcsDeferred.DeleteIndexFast(j);
                return npc;
            }
        }
    }

    return NULL;
}

NPC* psNPCClient::FindNPC(EID EID)
{
    gemNPCObject* obj = all_gem_objects_by_eid.Get(EID, 0);
    if(!obj) return NULL;

    return obj->GetNPC();
}

psPath* psNPCClient::FindPath(const char* name)
{
    return pathNetwork->FindPath(name);
}

void psNPCClient::RegisterReaction(NPC* npc, Reaction* reaction)
{
    allReactions.Put(reaction->GetEventType(), npc);
}

void psNPCClient::TriggerEvent(Perception* pcpt, float maxRange,
                               csVector3* basePos, iSector* baseSector,
                               bool sameSector)
{
    bool foundUser = false;

    // Only trigger NPCs that have this percpetion type registered as a reaction.
    csHash<NPC*,csString>::Iterator iter(allReactions.GetIterator(pcpt->GetName()));
    while(iter.HasNext())
    {
        NPC* npc = iter.Next();

        // skip disabled NPCs
        if(npc->IsDisabled())
            continue;

        npc->TriggerEvent(pcpt, maxRange, basePos, baseSector, sameSector);
        foundUser = true;
    }
    if(!foundUser)
    {
        notUsedReactions.PushSmart(pcpt->GetName());
    }
}

void psNPCClient::SetEntityPos(EID eid, csVector3 &pos, iSector* sector, InstanceID instance, bool force)
{

    gemNPCObject* obj = FindEntityID(eid);
    if(obj)
    {
        if(!force && obj->GetNPC())
        {
            // Skip updating NPC
            return;
        }

        obj->SetPosition(pos,sector,&instance);
    }
    else
    {
        CPrintf(CON_DEBUG, "SetEntityPos: Entity %s not found!\n", ShowID(eid));
    }
}

bool psNPCClient::IsReady()
{
    return network->IsReady();
}

void psNPCClient::LoadCompleted()
{
    // Client is loaded.

    // This starts the NPC AI processing loop.
    psNPCClientTick* tick = new psNPCClientTick(255,this);
    tick->QueueEvent();

    // Notify console user that NPC Client is up and running.
    CPrintf(CON_NOTIFY, "NPC Client load completed.\n");
}

void psNPCClient::Tick()
{
    // Fire a new tick for the common AI processing loop
    psNPCClientTick* tick = new psNPCClientTick(250,this);
    tick->QueueEvent();

    tick_counter++;

    ScopedTimer st_tick(250, "tick for tick_counter %d.",tick_counter);

    csTicks when = csGetTicks();

    // Advance tribes
    for(size_t j=0; j<tribes.GetSize(); j++)
    {
        ScopedTimer st(250, tribes[j]); // Calls ScopedTimerCallback on the tribe object

        tribes[j]->Advance(when,eventmanager);
    }

    // Percept proximity items every 4th tick
    if(tick_counter % 4 == 0)
    {
        ScopedTimer st(200, "tick for percept proximity items");

        PerceptProximityItems();
    }

    // Percept proximity locations every 4th tick
    // Tribes uses this to memorize locations as a tribe
    // member pass by
    if(tick_counter % 4 == 2)
    {
        ScopedTimer st(200, "tick for percept proximity locations");

        PerceptProximityLocations();
    }

    // Percept proximity tribe homes
    // Tribes use this event to attack stranger in their camp
    if(tick_counter % 4 == 3)
    {
        ScopedTimer st(200, "tick for percept proximity tribe homes");

        PerceptProximityTribeHome();
    }


    // Send all queued npc commands to the server
    network->SendAllCommands(true); // Final
}




bool psNPCClient::LoadMap(const char* mapfile)
{
    return world->NewRegion(mapfile);
}

LocationType* psNPCClient::FindRegion(const char* regname)
{
    return locationManager->FindRegion(regname);
}

LocationType* psNPCClient::FindLocation(const char* locname)
{
    return locationManager->FindLocation(locname);
}


NPCType* psNPCClient::FindNPCType(const char* npctype_name)
{

    return npctypes.Get(npctype_name, NULL);
}

void psNPCClient::AddRaceInfo(const psNPCRaceListMessage::NPCRaceInfo_t &raceInfo)
{
    raceInfos.PutUnique(raceInfo.name,raceInfo);
}

psNPCRaceListMessage::NPCRaceInfo_t* psNPCClient::GetRaceInfo(const char* name)
{
    return raceInfos.GetElementPointer(name);
}


float psNPCClient::GetWalkVelocity(csString &race)
{
    psNPCRaceListMessage::NPCRaceInfo_t* ri = raceInfos.GetElementPointer(race);
    if(ri)
    {
        return ri->walkSpeed;
    }

    return 0.0;
}

float psNPCClient::GetRunVelocity(csString &race)
{
    psNPCRaceListMessage::NPCRaceInfo_t* ri = raceInfos.GetElementPointer(race);
    if(ri)
    {
        return ri->runSpeed;
    }

    return 0.0;
}

Location* psNPCClient::FindLocation(const char* loctype, const char* name)
{
    return locationManager->FindLocation(loctype, name);
}

Location* psNPCClient::FindNearestLocation(const char* loctype, csVector3 &pos, iSector* sector, float range, float* found_range)
{
    return locationManager->FindNearestLocation(world, loctype, pos, sector, range, found_range);
}

Location* psNPCClient::FindRandomLocation(const char* loctype, csVector3 &pos, iSector* sector, float range, float* found_range)
{
    return locationManager->FindRandomLocation(world, loctype, pos, sector, range, found_range);
}

Waypoint* psNPCClient::FindNearestWaypoint(const csVector3 &v,iSector* sector, float range, float* found_range)
{
    return pathNetwork->FindNearestWaypoint(v, sector, range, found_range);
}

Waypoint* psNPCClient::FindRandomWaypoint(const csVector3 &v,iSector* sector, float range, float* found_range)
{
    return pathNetwork->FindRandomWaypoint(v, sector, range, found_range);
}

Waypoint* psNPCClient::FindWaypoint(gemNPCObject* entity)
{
    csVector3 position;
    iSector*  sector;

    psGameObject::GetPosition(entity, position, sector);

    return pathNetwork->FindWaypoint(position, sector);
}

Waypoint* psNPCClient::FindNearestWaypoint(gemNPCObject* entity, float range, float* found_range)
{
    csVector3 position;
    iSector*  sector;

    psGameObject::GetPosition(entity, position, sector);

    return pathNetwork->FindNearestWaypoint(position, sector, range, found_range);
}

Waypoint* psNPCClient::FindNearestWaypoint(const char* group, csVector3 &v,iSector* sector, float range, float* found_range)
{
    int groupIndex = pathNetwork->FindWaypointGroup(group);
    if(groupIndex == -1)
    {
        return NULL;
    }
    return pathNetwork->FindNearestWaypoint(groupIndex, v, sector, range, found_range);
}

Waypoint* psNPCClient::FindRandomWaypoint(const char* group, csVector3 &v,iSector* sector, float range, float* found_range)
{
    int groupIndex = pathNetwork->FindWaypointGroup(group);
    if(groupIndex == -1)
    {
        return NULL;
    }
    return pathNetwork->FindRandomWaypoint(groupIndex, v, sector, range, found_range);
}

Waypoint* psNPCClient::FindWaypoint(int id)
{
    return pathNetwork->FindWaypoint(id);
}


Waypoint* psNPCClient::FindWaypoint(const char* name, WaypointAlias** alias)
{
    return pathNetwork->FindWaypoint(name, alias);
}

csList<Waypoint*> psNPCClient::FindWaypointRoute(Waypoint* start, Waypoint* end, const psPathNetwork::RouteFilter* filter)
{
    return pathNetwork->FindWaypointRoute(start, end, filter);
}

csList<Edge*> psNPCClient::FindEdgeRoute(Waypoint* start, Waypoint* end, const psPathNetwork::RouteFilter* filter)
{
    return pathNetwork->FindEdgeRoute(start, end, filter);
}

void psNPCClient::EnableDisableNPCs(const char* pattern, bool enable)
{
    // First check if pattern is number
    EID eid = atoi(pattern);

    if(eid != 0)
    {
        NPC* npc = FindNPC(eid);
        if(npc)
        {
            npc->Disable(!enable);
        }
    }
    else
    {
        // Mach by pattern

        for(size_t i=0; i<npcs.GetSize(); i++)
        {
            if((strcmp(pattern,"all")==0) ||
                    (strstr(npcs[i]->GetName(),pattern)))
            {
                npcs[i]->Disable(!enable);
            }
        }
    }
}



void psNPCClient::ListAllNPCs(const char* pattern)
{
    if(pattern && strcasecmp(pattern, "summary")==0)
    {
        int disabled = 0;
        int alive = 0;
        int entity = 0;
        int behaviour = 0;
        int brain = 0;
        for(size_t i = 0; i < npcs.GetSize(); i++)
        {
            if(npcs[i]->IsAlive())
                alive++;
            if(npcs[i]->IsDisabled())
                disabled++;
            if(npcs[i]->GetActor())
                entity++;
            if(npcs[i]->GetCurrentBehavior())
                behaviour++;
            if(npcs[i]->GetBrain())
                brain++;
        }
        CPrintf(CON_CMDOUTPUT, "NPC summary for %d NPCs: %d disabled, %d alive, %d with entities, %d with current behaviour, %d with brain\n",
                npcs.GetSize(), disabled, alive, entity, behaviour, brain);

        return; // No point continue since no npc should be named summary :)
    }
    else if(pattern && strcasecmp(pattern,"stats") == 0)
    {
        CPrintf(CON_CMDOUTPUT, "%-7s %-5s %-30s %-17s %-17s %-17s %-17s\n",
                "NPC ID", "EID", "Name",
                "HP", "Mana","PStamina","MStamina");
        for(size_t i = 0; i < npcs.GetSize(); i++)
        {
            CPrintf(CON_CMDOUTPUT, "%-7u %-5d %-30s %5.1f/%5.1f/%5.1f %5.1f/%5.1f/%5.1f %5.1f/%5.1f/%5.1f %5.1f/%5.1f/%5.1f \n" ,
                    npcs[i]->GetPID().Unbox(),
                    npcs[i]->GetActor() ? npcs[i]->GetActor()->GetEID().Unbox() : 0,
                    npcs[i]->GetName(),
                    npcs[i]->GetHP(),npcs[i]->GetMaxHP(),npcs[i]->GetHPRate(),
                    npcs[i]->GetMana(),npcs[i]->GetMaxMana(),npcs[i]->GetManaRate(),
                    npcs[i]->GetPysStamina(),npcs[i]->GetMaxPysStamina(),npcs[i]->GetPysStaminaRate(),
                    npcs[i]->GetMenStamina(),npcs[i]->GetMaxMenStamina(),npcs[i]->GetMenStaminaRate()
                   );
        }

        return;
    }

    // Default behavior

    CPrintf(CON_CMDOUTPUT, "%-7s %-5s %-30s %-6s %-6s %-20s %-20s %-4s %-3s %-8s\n",
            "NPC ID", "EID", "Name", "Entity", "Status", "Brain","Behaviour","Step","Dbg","Disabled");
    for(size_t i = 0; i < npcs.GetSize(); i++)
    {
        if(!pattern || strstr(npcs[i]->GetName(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%-7u %-5d %-30s %-6s %-6s %-20s %-20s %4d %-3s %-8s\n" ,
                    npcs[i]->GetPID().Unbox(),
                    npcs[i]->GetActor() ? npcs[i]->GetActor()->GetEID().Unbox() : 0,
                    npcs[i]->GetName(),
                    (npcs[i]->GetActor()?"Entity":"None  "),
                    (npcs[i]->IsAlive()?"Alive":"Dead"),
                    (npcs[i]->GetBrain()?npcs[i]->GetBrain()->GetName():"(None)"),
                    (npcs[i]->GetCurrentBehavior()?npcs[i]->GetCurrentBehavior()->GetName():"(None)"),
                    (npcs[i]->GetCurrentBehavior()?npcs[i]->GetCurrentBehavior()->GetCurrentStep():0),
                    (npcs[i]->IsDebugging()?"Yes":"No"),
                    (npcs[i]->IsDisabled()?"Disabled":"Active")
                   );
        }
    }
}

bool psNPCClient::DumpRace(const char* pattern)
{
    csHash<psNPCRaceListMessage::NPCRaceInfo_t,csString>::GlobalIterator it(raceInfos.GetIterator());

    CPrintf(CON_CMDOUTPUT,"%-20s %10s %10s %-20s\n","Race","WalkSpeed","RunSpeed","Size");

    while(it.HasNext())
    {
        const psNPCRaceListMessage::NPCRaceInfo_t &ri = it.Next();
        CPrintf(CON_CMDOUTPUT,"%-20s %10.2f %10.2f %20s\n",ri.name.GetDataSafe(),ri.walkSpeed,ri.runSpeed,
                toString(ri.size).GetDataSafe());
    }

    return true;
}


bool psNPCClient::DumpNPC(const char* pattern)
{
    unsigned int id = atoi(pattern);
    for(size_t i=0; i<npcs.GetSize(); i++)
    {
        if(npcs[i]->GetPID() == id)
        {
            npcs[i]->Dump();
            return true;
        }
    }
    return false;
}

bool psNPCClient::InfoNPC(const char* pattern)
{
    unsigned int id = atoi(pattern);
    for(size_t i=0; i<npcs.GetSize(); i++)
    {
        NPC* npc = npcs[i];
        if(npc->GetPID() == id)
        {
            csString info = npc->Info("all");
            CPrintf(CON_CMDOUTPUT, "Info for %s(%s)\n%s\n",
                    npc->GetName(), ShowID(npc->GetPID()), info.GetDataSafe());
            return true;
        }
    }
    return false;
}


void psNPCClient::ListAllEntities(const char* pattern, bool onlyCharacters)
{
    if(onlyCharacters)
    {

        if(pattern && (strcasecmp(pattern, "summary")==0))
        {
            int numChars = 0;
            int alive = 0;

            for(size_t i=0; i < all_gem_objects.GetSize(); i++)
            {
                gemNPCActor* actor = dynamic_cast<gemNPCActor*>(all_gem_objects[i]);
                if(!actor || actor->GetNPC())
                    continue;

                numChars++;

                if(actor->IsAlive())
                    alive++;
            }
            CPrintf(CON_CMDOUTPUT, "Actor summary for %d actors: %d alive\n",
                    numChars, alive);

            return; // No point continue since no actor should be named summary :)
        }
        else if(pattern && strcasecmp(pattern,"stats") == 0)
        {
            CPrintf(CON_CMDOUTPUT, "%-7s %-5s %-30s %-17s %-17s %-17s %-17s\n",
                    "PID", "EID", "Name",
                    "HP", "Mana","PStamina","MStamina");
            for(size_t i=0; i < all_gem_objects.GetSize(); i++)
            {
                gemNPCActor* actor = dynamic_cast<gemNPCActor*>(all_gem_objects[i]);
                if(!actor || actor->GetNPC())
                    continue;

                CPrintf(CON_CMDOUTPUT, "%-7u %-5d %-30s %5.1f/%5.1f/%5.1f %5.1f/%5.1f/%5.1f %5.1f/%5.1f/%5.1f %5.1f/%5.1f/%5.1f \n" ,
                        actor->GetPID().Unbox(),
                        actor->GetEID().Unbox(),
                        actor->GetName(),
                        actor->GetHP(),actor->GetMaxHP(),actor->GetHPRate(),
                        actor->GetMana(),actor->GetMaxMana(),actor->GetManaRate(),
                        actor->GetPysStamina(),actor->GetMaxPysStamina(),actor->GetPysStaminaRate(),
                        actor->GetMenStamina(),actor->GetMaxMenStamina(),actor->GetMenStaminaRate()
                       );
            }

            return;
        }

        // Default behavior

        CPrintf(CON_CMDOUTPUT, "%-9s %-5s %-10s %-30s %-3s %-3s %-5s\n" ,
                "Player ID", "EID","Type","Name","Vis","Inv","Alive");
        for(size_t i=0; i < all_gem_objects.GetSize(); i++)
        {
            gemNPCActor* actor = dynamic_cast<gemNPCActor*>(all_gem_objects[i]);
            if(!actor || actor->GetNPC())
                continue;

            if(!pattern || strstr(actor->GetName(),pattern) || atoi(pattern) == (int)actor->GetEID().Unbox())
            {
                CPrintf(CON_CMDOUTPUT, "%-9d %-5d %-10s %-30s %-3s %-3s %-5s\n",
                        actor->GetPID().Unbox(),
                        actor->GetEID().Unbox(),
                        actor->GetObjectType(),
                        actor->GetName(),
                        (actor->IsVisible()?"Yes":"No"),
                        (actor->IsInvincible()?"Yes":"No"),
                        (actor->IsAlive()?"Yes":"No"));
            }

        }
        return;
    }

    CPrintf(CON_CMDOUTPUT, "%-9s %-5s %-10s %-30s %-3s %-3s %-4s Position\n",
            "Player ID","EID","Type","Name","Vis","Inv","Pick");
    for(size_t i=0; i < all_gem_objects.GetSize(); i++)
    {
        gemNPCObject* obj = all_gem_objects[i];
        csVector3 pos;
        iSector* sector;
        psGameObject::GetPosition(obj, pos, sector);

        if(!pattern || strstr(obj->GetName(),pattern) || atoi(pattern) == (int)obj->GetEID().Unbox())
        {
            CPrintf(CON_CMDOUTPUT, "%-9d %5d %-10s %-30s %-3s %-3s %-4s %s %d\n",
                    obj->GetPID().Unbox(),
                    obj->GetEID().Unbox(),
                    obj->GetObjectType(),
                    obj->GetName(),
                    (obj->IsVisible()?"Yes":"No"),
                    (obj->IsInvincible()?"Yes":"No"),
                    (obj->IsPickable()?"Yes":"No"),
                    toString(pos,sector).GetData(),
                    obj->GetInstance());
        }
    }

}

void psNPCClient::ListTribes(const char* pattern)
{
    if(tribes.GetSize() == 0)
    {
        CPrintf(CON_CMDOUTPUT, "No tribes defined\n");
        return;
    }


    csVector3 pos;
    iSector*  sector;
    float     radius;
    // Write basic details about tribes
    if(!pattern || strlen(pattern) == 0)
    {
        CPrintf(CON_CMDOUTPUT, "\n%9s %-30s %-7s %-35s %5s\n",
                "Tribe id", "Name", "MCount", "Location","Radius");

        for(size_t i=0; i<tribes.GetSize(); i++)
        {
            tribes[i]->GetHome(pos,radius,sector);
            CPrintf(CON_CMDOUTPUT, "%9d %-30s %-7d %35s %5f\n",
                    tribes[i]->GetID(),
                    tribes[i]->GetName(),
                    tribes[i]->GetMemberCount(),
                    toString(pos,sector).GetDataSafe(),
                    radius);
        }
    }
    else
    {
        int tribeID = atoi(pattern);
        printf("%d\n", tribeID);
        for(size_t i=0; i<tribes.GetSize(); i++)
        {
            // Find the ID
            if(tribes[i]->GetID() == tribeID)
            {
                tribes[i]->GetHome(pos,radius,sector);
                // Print basic tribe data
                CPrintf(CON_CMDOUTPUT, "Tribe ID: %d\nTribe Name: %s\nLocation: %s\nMain Resource: %s (nick:%s)\nCanGrow: %s \nShouldGrow: %s \n",
                        tribes[i]->GetID(),
                        tribes[i]->GetName(),
                        toString(pos,sector).GetDataSafe(),
                        tribes[i]->GetNeededResource(),
                        tribes[i]->GetNeededResourceNick(),
                        (tribes[i]->CanGrow()?"Yes":"No"),
                        (tribes[i]->ShouldGrow()?"Yes":"No"));

                // Print Tribe Members
                CPrintf(CON_CMDOUTPUT,"Members:\n");
                CPrintf(CON_CMDOUTPUT, "%-6s %-6s %-30s %-3s %-6s %-15s %-15s %-20s %-20s\n",
                        "NPC ID", "EID", "Name", "Ent", "Status", "Brain","Behaviour","Owner","TribeMemberType");
                for(size_t j = 0; j < tribes[i]->GetMemberCount(); j++)
                {
                    NPC* npc = tribes[i]->GetMember(j);
                    CPrintf(CON_CMDOUTPUT, "%6u %6d %-30s %-3s %-6s %-15s %-15s %-20s %-20s\n" ,
                            npc->GetPID().Unbox(),
                            npc->GetActor() ? npc->GetActor()->GetEID().Unbox() : 0,
                            npc->GetName(),
                            (npc->GetActor()?"Yes":" - "),
                            (npc->IsAlive()?"Alive":"Dead"),
                            (npc->GetBrain()?npc->GetBrain()->GetName():""),
                            (npc->GetCurrentBehavior()?npc->GetCurrentBehavior()->GetName():""),
                            npc->GetOwnerName(),
                            npc->GetTribeMemberType().GetDataSafe()
                           );
                }

                // Print Resources
                CPrintf(CON_CMDOUTPUT,"Resources:\n");
                CPrintf(CON_CMDOUTPUT,"%7s %-20s %7s\n","ID","Name","Amount");
                for(size_t r = 0; r < tribes[i]->GetResourceCount(); r++)
                {
                    CPrintf(CON_CMDOUTPUT,"%7d %-20s %d\n",
                            tribes[i]->GetResource(r).id,
                            tribes[i]->GetResource(r).name.GetData(),
                            tribes[i]->GetResource(r).amount);
                }

                // Print Memories
                CPrintf(CON_CMDOUTPUT,"Memories:\n");
                CPrintf(CON_CMDOUTPUT,"%7s %-25s Position                Radius  %-20s  %-20s\n","ID","Name","Sector","Private to NPC");
                csList<Tribe::Memory*>::Iterator it = tribes[i]->GetMemoryIterator();
                while(it.HasNext())
                {
                    Tribe::Memory* memory = it.Next();
                    csString name;
                    if(memory->npc)
                    {
                        name.Format("%s(%u)", memory->npc->GetName(), memory->npc->GetPID().Unbox());
                    }
                    CPrintf(CON_CMDOUTPUT,"%7d %-25s %7.1f %7.1f %7.1f %7.1f %-20s %-20s\n",
                            memory->id,
                            memory->name.GetDataSafe(),
                            memory->pos.x,memory->pos.y,memory->pos.z,memory->radius,
                            (memory->GetSector()?memory->GetSector()->QueryObject()->GetName():""),
                            name.GetDataSafe());
                }

                // Print Recipe List
                tribes[i]->DumpRecipesToConsole();

                // Print Knowledge
                tribes[i]->DumpKnowledge();

                // Print Assets
                tribes[i]->DumpAssets();

                // Print Recipe Buffers
                tribes[i]->DumpBuffers();

                return; // The tribe has been dumped
            }
        }
    }

}

void psNPCClient::ListTribeRecipes(const char* tribeID)
{
    int tribeid = atoi(tribeID);
    for(size_t i=0; i<tribes.GetSize(); i++)
    {
        if(tribeid == tribes[i]->GetID())
        {
            tribes[i]->DumpRecipesToConsole();
            return;
        }
    }
    CPrintf(CON_CMDOUTPUT, "No Tribe with id '%d' found.\n", tribeID);
}

void psNPCClient::ListReactions(const char* pattern)
{
    csArray<NPC*> lnpcs;
    csArray<csString> reactions;

    // Extract uniq lists of NPCs and reactions
    {
        csHash<NPC*,csString>::GlobalIterator iter(allReactions.GetIterator());
        while(iter.HasNext())
        {
            csTuple2<NPC*,csString> item = iter.NextTuple();
            lnpcs.PushSmart(item.first);
            reactions.PushSmart(item.second);
        }
    }

    CPrintf(CON_CMDOUTPUT, "Registered reactions (Per NPC):\n");
    csArray<NPC*>::Iterator npcIter = lnpcs.GetIterator();
    while(npcIter.HasNext())
    {
        NPC* npc = npcIter.Next();
        CPrintf(CON_CMDOUTPUT, "NPC: %s(%s):\n",npc->GetName(),ShowID(npc->GetEID()));
        csString result;
        csString delim = "";
        csHash<NPC*,csString>::GlobalIterator iter(allReactions.GetIterator());
        while(iter.HasNext())
        {
            csTuple2<NPC*,csString> item = iter.NextTuple();
            if(item.first == npc)
            {
                csString reaction = item.second;
                if(result.Length() > 70)
                {
                    CPrintf(CON_CMDOUTPUT,"%s\n",result.GetDataSafe());
                    delim = "";
                    result = "";
                }
                result.AppendFmt("%s%s",delim.GetData(),reaction.GetDataSafe());
                delim = ", ";
            }
        }
        CPrintf(CON_CMDOUTPUT,"%s\n",result.GetDataSafe());
    }
    CPrintf(CON_CMDOUTPUT, "Registered reactions (Per Reaction):\n");
    csArray<csString>::Iterator reactionIter = reactions.GetIterator();
    while(reactionIter.HasNext())
    {
        csString reaction = reactionIter.Next();
        CPrintf(CON_CMDOUTPUT, "Reaction: \"%s\"\n",reaction.GetDataSafe());
        csString result;
        csString delim = "";
        csHash<NPC*,csString>::Iterator iter(allReactions.GetIterator(reaction));
        while(iter.HasNext())
        {
            NPC* npc = iter.Next();
            if(result.Length() > 70)
            {
                CPrintf(CON_CMDOUTPUT,"%s\n",result.GetDataSafe());
                delim = "";
                result = "";
            }
            result.AppendFmt("%s%s(%s)",delim.GetData(),npc->GetName(),ShowID(npc->GetEID()));
            delim = ", ";
        }
        CPrintf(CON_CMDOUTPUT,"%s\n",result.GetDataSafe());
    }

    CPrintf(CON_CMDOUTPUT, "Not used reactions:\n");
    csArray<csString>::Iterator notUsedIter(notUsedReactions.GetIterator());
    while(notUsedIter.HasNext())
    {
        csString type = notUsedIter.Next();
        CPrintf(CON_CMDOUTPUT, "%s\n",type.GetDataSafe());
    }

}



void psNPCClient::ListWaypoints(const char* pattern)
{
    pathNetwork->ListWaypoints(pattern);
}

void psNPCClient::ListPaths(const char* pattern)
{
    pathNetwork->ListPaths(pattern);
}

void psNPCClient::ListLocations(const char* pattern)
{
    csHash<LocationType*, csString>::GlobalIterator iter(locationManager->GetIterator());
    LocationType* loc;

    CPrintf(CON_CMDOUTPUT, "%9s %9s %-30s %-10s %10s\n", "Type id", "Loc id", "Name", "Region","");
    while(iter.HasNext())
    {
        loc = iter.Next();
        if(!pattern || strstr(loc->name.GetDataSafe(),pattern))
        {
            CPrintf(CON_CMDOUTPUT, "%9d %9s %-30s %-10s\n" ,
                    loc->id,"",loc->name.GetDataSafe(),"","");

            for(size_t i = 0; i < loc->locs.GetSize(); i++)
            {
                if(loc->locs[i]->IsRegion())
                {
                    CPrintf(CON_CMDOUTPUT, "%9s %9d %-30s %-10s\n" ,
                            "",loc->locs[i]->id,loc->locs[i]->name.GetDataSafe(),
                            (loc->locs[i]->IsRegion()?"True":"False"));
                    for(size_t j = 0; j < loc->locs[i]->locs.GetSize(); j++)
                    {
                        CPrintf(CON_CMDOUTPUT, "%9s %9s %-30s %-10s (%9.3f,%9.3f,%9.3f, %s) %9.3f\n" ,
                                "","","","",
                                loc->locs[i]->locs[j]->pos.x,loc->locs[i]->locs[j]->pos.y,loc->locs[i]->locs[j]->pos.z,
                                loc->locs[i]->locs[j]->sectorName.GetDataSafe(),
                                loc->locs[i]->locs[j]->radius);
                    }
                }
                else
                {
                    CPrintf(CON_CMDOUTPUT, "%9s %9d %-30s %-10s (%9.3f,%9.3f,%9.3f, %s) %9.3f  %9.3f\n" ,
                            "",loc->locs[i]->id,loc->locs[i]->name.GetDataSafe(),
                            (loc->locs[i]->IsRegion()?"True":"False"),
                            loc->locs[i]->pos.x,loc->locs[i]->pos.y,loc->locs[i]->pos.z,
                            loc->locs[i]->sectorName.GetDataSafe(),
                            loc->locs[i]->radius,loc->locs[i]->rot_angle);
                }
            }
        }
    }
}


void psNPCClient::HandleDeath(NPC* who)
{
    who->GetBrain()->Interrupt(who);
    who->SetAlive(false);

    // If enity update entity as well
    gemNPCActor* entity = who->GetActor();
    if(entity)
    {
        entity->SetAlive(false);
    }

    if(who->GetTribe())
    {
        who->GetTribe()->HandleDeath(who);
    }
}

void psNPCClient::PerceptProximityItems()
{
    //
    // Note: The follwing method could skeep checking a item for
    // an iteraction. This because fast delete is used to remove
    // items from list and than a item on the end might be moved right
    // into the space just checked.
    //

    int size = (int)all_gem_items.GetSize();
    if(!size) return;  // Nothing to do if no items

    int check_count = 50; // We only check 50 items each time. This number has to be tuned
    if(check_count > size)
    {
        // If not more than check_count items don't check them more than once :)
        check_count = size;
    }

    csList<NPC*> npcList;
    {
        csHash<NPC*,csString>::Iterator iter(allReactions.GetIterator("item sensed"));
        while(iter.HasNext())
        {
            NPC* npc = iter.Next();
            npcList.PushBack(npc);
        }
    }
    {
        csHash<NPC*,csString>::Iterator iter(allReactions.GetIterator("item adjacent"));
        while(iter.HasNext())
        {
            NPC* npc = iter.Next();
            npcList.PushBack(npc);
        }
    }
    {
        csHash<NPC*,csString>::Iterator iter(allReactions.GetIterator("item nearby"));
        while(iter.HasNext())
        {
            NPC* npc = iter.Next();
            npcList.PushBack(npc);
        }
    }

    while(check_count--)
    {
        current_long_range_perception_index++;
        if(current_long_range_perception_index >= (int)all_gem_items.GetSize())
        {
            current_long_range_perception_index = 0;
        }

        gemNPCItem* item = all_gem_items[current_long_range_perception_index];

        if(item && item->IsPickable())
        {
            iSector* item_sector;
            csVector3 item_pos;
            psGameObject::GetPosition(item,item_pos,item_sector);

            // Use bounding boxes to check within perception range. This
            // is faster than using the distance.
            csBox3 bboxLong;
            bboxLong.AddBoundingVertex(item_pos-csVector3(LONG_RANGE_PERCEPTION));
            bboxLong.AddBoundingVertexSmart(item_pos+csVector3(LONG_RANGE_PERCEPTION));
            csBox3 bboxShort;
            bboxShort.AddBoundingVertex(item_pos-csVector3(SHORT_RANGE_PERCEPTION));
            bboxShort.AddBoundingVertexSmart(item_pos+csVector3(SHORT_RANGE_PERCEPTION));
            csBox3 bboxPersonal;
            bboxPersonal.AddBoundingVertex(item_pos-csVector3(PERSONAL_RANGE_PERCEPTION));
            bboxPersonal.AddBoundingVertexSmart(item_pos+csVector3(PERSONAL_RANGE_PERCEPTION));

            csList<NPC*>::Iterator iter(npcList);
            while(iter.HasNext())
            {
                NPC* npc = iter.Next();

                // skip disabled NPCs
                if(npc->IsDisabled())
                    continue;

                if(npc->GetActor() == NULL)
                    continue;

                iSector* npc_sector;
                csVector3 npc_pos;
                psGameObject::GetPosition(npc->GetActor(), npc_pos, npc_sector);

                // Only percept for items in same sector, NPC will probably not see a item
                // in other sectors.
                if(npc_sector != item_sector)
                {
                    continue;
                }

                if(npc_pos < bboxLong)
                {
                    if(npc_pos < bboxShort)
                    {
                        if(npc_pos < bboxPersonal)
                        {
                            ItemPerception pcpt_adjacent("item adjacent", item);
                            npc->TriggerEvent(&pcpt_adjacent);
                            continue;
                        }
                        ItemPerception pcpt_nearby("item nearby", item);
                        npc->TriggerEvent(&pcpt_nearby);
                        continue;
                    }
                    ItemPerception pcpt_sensed("item sensed", item);
                    npc->TriggerEvent(&pcpt_sensed);
                    continue;
                }
            }
        }
    }
}

void psNPCClient::PerceptProximityLocations()
{

    int size = locationManager->GetNumberOfLocations();

    if(!size) return;  // Nothing to do if no items

    int check_count = 50; // We only check 50 items each time. This number has to be tuned
    if(check_count > size)
    {
        // If not more than check_count items don't check them more than once :)
        check_count = size;
    }

    while(check_count--)
    {
        current_long_range_perception_loc_index++;
        if(current_long_range_perception_loc_index >= size)
        {
            current_long_range_perception_loc_index = 0;
        }

        Location* location = locationManager->GetLocation(current_long_range_perception_loc_index);

        LocationPerception pcpt_sensed("location sensed", location->type->name, location, engine);

        // Now trigger every NPC within the sector of the location.
        TriggerEvent(&pcpt_sensed, location->radius + LONG_RANGE_PERCEPTION,
                     &location->pos, location->GetSector(engine), true); // Broadcast only in sector
    }
}

void psNPCClient::PerceptProximityTribeHome()
{
    int size = (int)all_gem_actors.GetSize();

    if(!size) return;  // Nothing to do if no items

    int check_count = 50; // We only check 50 items each time. This number has to be tuned
    if(check_count > size)
    {
        // If not more than check_count items don't check them more than once :)
        check_count = size;
    }

    while(check_count--)
    {
        current_tribe_home_perception_index++;
        if(current_tribe_home_perception_index >= size)
        {
            current_tribe_home_perception_index = 0;
        }

        gemNPCActor* actor = all_gem_actors[current_tribe_home_perception_index];
        bool within = false;

        // Now we have an actor, check that actor for each tribe.
        for(size_t i=0; i<tribes.GetSize(); i++)
        {
            Tribe* tribe = tribes[i];

            csVector3 position;
            iSector* sector;
            psGameObject::GetPosition(actor, position, sector);

            if(tribe->CheckWithinBoundsTribeHome(NULL, position, sector))
            {
                within = true;
                if(actor->SetWithinTribe(tribe))
                {
                    NPC* npc = actor->GetNPC();
                    if(npc)
                    {
                        // Percept the NPC that it has entered a tribe home
                        Perception pcpt_entering("tribe_home:entering");
                        npc->TriggerEvent(&pcpt_entering);
                    }

                    // Percept all members of the tribe that an actor has entered the tribe home
                    Perception pcpt_entered("tribe_home:actor_entered");
                    tribe->TriggerEvent(&pcpt_entered);
                }
            }
        }
        if(!within)
        {
            Tribe* oldTribe = NULL;

            if(actor->SetWithinTribe(NULL,&oldTribe))
            {
                NPC* npc = actor->GetNPC();
                if(npc)
                {
                    // Percept the NPC that it has entered a tribe home
                    Perception pcpt_living("tribe_home:living");
                    npc->TriggerEvent(&pcpt_living);
                }

                // Percept all members of the tribe that an actor has entered the tribe home
                Perception pcpt_left("tribe_home:actor_left");
                oldTribe->TriggerEvent(&pcpt_left);
            }
        }

    }

}

void psNPCClient::UpdateTime(int minute, int hour, int day, int month, int year)
{
    gameHour = hour;
    gameMinute = minute;
    gameDay = day;
    gameMonth = month;
    gameYear = year;

    gameTimeUpdated = csGetTicks();

    TimePerception pcpt(gameHour,gameMinute,gameYear,gameMonth,gameDay);
    TriggerEvent(&pcpt); // Broadcast

    Notify6(LOG_WEATHER,"The time is now %d:%02d %d-%d-%d.",
            gameHour,gameMinute,gameYear,gameMonth,gameDay);
}

iCelHPath* psNPCClient::ShortestPath(NPC* npc, const csVector3 &from, iSector* fromSector, const csVector3 &goal, iSector* goalSector)
{
    if(npc->IsDebugging(5))
    {
        // Seems like the getdebugmeshes destroy the path, so get a separate path for debug...
        iCelHPath* path = GetNavStruct()->ShortestPath(from,fromSector,goal,goalSector);
        if(path)
        {
            csArray<csSimpleRenderMesh*>* list = path->GetDebugMeshes();
            csArray<csSimpleRenderMesh*>::Iterator countIter = list->GetIterator();
            uint16_t count = 0;
            while(countIter.HasNext())
            {
                count++;
                countIter.Next();
            }

            csArray<csSimpleRenderMesh*>::Iterator iter = list->GetIterator();
            uint16_t index = 0;
            while(iter.HasNext())
            {
                csSimpleRenderMesh* &simpleRenderMesh = iter.Next();
                psSimpleRenderMeshMessage msg(0, connection->GetAccessPointers(), "NPC Path", index, count, fromSector, *simpleRenderMesh);
                msghandler->SendMessage(msg.msg);
                index++;
            }
        }

    }

    iCelHPath* path = GetNavStruct()->ShortestPath(from,fromSector,goal,goalSector);
    if(!path)
    {
        NPCDebug(npc, 5, "Failed to find path");
    }

    return path;
}



void psNPCClient::CatchCommand(const char* cmd)
{
    printf("Caught command: %s\n",cmd);
    network->SendConsoleCommand(cmd+1);
}

void psNPCClient::AttachObject(iObject* object, gemNPCObject* gobject)
{
    csRef<psNpcMeshAttach> attacher;
    attacher.AttachNew(new psNpcMeshAttach(gobject));
    attacher->SetName(object->GetName());
    csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));

    object->ObjAdd(attacher_obj);
}

void psNPCClient::UnattachObject(iObject* object, gemNPCObject* gobject)
{
    csRef<psNpcMeshAttach> attacher(CS::GetChildObject<psNpcMeshAttach>(object));
    if(attacher)
    {
        if(attacher->GetObject() == gobject)
        {
            csRef<iObject> attacher_obj(scfQueryInterface<iObject>(attacher));
            object->ObjRemove(attacher_obj);
        }
    }
}

gemNPCObject* psNPCClient::FindAttachedObject(iObject* object)
{
    gemNPCObject* found = 0;

    csRef<psNpcMeshAttach> attacher(CS::GetChildObject<psNpcMeshAttach>(object));
    if(attacher)
    {
        found = attacher->GetObject();
    }

    return found;
}

csArray<gemNPCObject*> psNPCClient::FindNearbyEntities(iSector* sector, const csVector3 &pos, float radius, bool doInvisible)
{
    csArray<gemNPCObject*> list;

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

        gemNPCObject* object = FindAttachedObject(m->QueryObject());

        if(object)
        {
            list.Push(object);
        }
    }

    return list;
}

csArray<gemNPCActor*> psNPCClient::FindNearbyActors(iSector* sector, const csVector3 &pos, float radius, bool doInvisible)
{
    csArray<gemNPCActor*> list;

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

        gemNPCActor* actor = dynamic_cast<gemNPCActor*>(FindAttachedObject(m->QueryObject()));

        if(actor)
        {
            list.Push(actor);
        }
    }

    return list;
}

/*------------------------------------------------------------------*/

psNPCClient* psNPCClientTick::client = NULL;

psNPCClientTick::psNPCClientTick(int offsetticks, psNPCClient* c)
    : psGameEvent(0,offsetticks,"psNPCClientTick")
{
    client = c;
}

void psNPCClientTick::Trigger()
{
    if(running)
        client->Tick();
}

csString psNPCClientTick::ToString() const
{
    return "NPC Client tick";
}
