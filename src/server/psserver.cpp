/*
 * psserver.cpp - author: Matze Braun <matze@braunis.de>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iutil/vfs.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/cmdline.h>
#include <iutil/object.h>
#include <iutil/stringarray.h>
#include <ivaria/reporter.h>
#include <ivaria/stdrep.h>

//=============================================================================
// Library Include
//=============================================================================
#include <ibgloader.h>
#include "util/serverconsole.h"
#include "util/mathscript.h"
#include "util/psdatabase.h"
#include "util/eventmanager.h"
#include "util/log.h"
#include "util/consoleout.h"

#include "net/msghandler.h"
#include "net/messages.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psaccountinfo.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "actionmanager.h"
#include "adminmanager.h"
#include "advicemanager.h"
#include "authentserver.h"
#include "bankmanager.h"
#include "cachemanager.h"
#include "chatmanager.h"
#include "client.h"
#include "combatmanager.h"
#include "commandmanager.h"
#include "creationmanager.h"
#include "economymanager.h"
#include "entitymanager.h"
#include "exchangemanager.h"
#include "gem.h"
#include "globals.h"
#include "gmeventmanager.h"
#include "groupmanager.h"
#include "guildmanager.h"
#include "hiremanager.h"
#include "introductionmanager.h"
#include "serversongmngr.h"
#include "marriagemanager.h"
#include "minigamemanager.h"
#include "netmanager.h"
#include "npcmanager.h"
#include "playergroup.h"
#include "progressionmanager.h"
#include "psserver.h"
#include "psserverdr.h"
#include "psserverchar.h"
#include "questionmanager.h"
#include "questmanager.h"
#include "serverstatus.h"
#include "spawnmanager.h"
#include "spellmanager.h"
#include "tutorialmanager.h"
#include "usermanager.h"
#include "weathermanager.h"
#include "workmanager.h"

// Remember to bump this in server_options.sql and add to upgrade_schema.sql!
#define DATABASE_VERSION_STR "1277"


psCharacterLoader psServer::CharacterLoader;

psServer::psServer()
{
    serverconsole       = NULL;
    netmanager          = NULL;
    marriageManager     = NULL;
    entitymanager       = NULL;
    tutorialmanager     = NULL;
    database            = NULL;
    usermanager         = NULL;
    guildmanager        = NULL;
    groupmanager        = NULL;
    charmanager         = NULL;
    eventmanager        = NULL;
    charCreationManager = NULL;
    workmanager         = NULL;
    questionmanager     = NULL;
    advicemanager       = NULL;
    actionmanager       = NULL;
    minigamemanager     = NULL;
    economymanager      = NULL;
    exchangemanager     = NULL;
    spawnmanager        = NULL;
    adminmanager        = NULL;
    combatmanager       = NULL;
    weathermanager      = NULL;
    progression         = NULL;
    npcmanager          = NULL;
    spellmanager        = NULL;
    rng                 = NULL;
    questmanager        = NULL;
    gmeventManager      = NULL;
    bankmanager         = NULL;
    hiremanager         = NULL;
    intromanager        = NULL;
    songManager         = NULL;
    mathscriptengine    = NULL;
    objreg              = NULL;
    cachemanager        = NULL;
    logcsv              = NULL;
    vfs                 = NULL;
    server_quit_event   = NULL;
    unused_pid          = 0;
    MapLoaded           = false;
}

#define PS_CHECK_REF_COUNT(c) printf("%3d %s\n",c->GetRefCount(),#c)

psServer::~psServer()
{
    // Kick players from server
    if(netmanager)
    {
        ClientConnectionSet* clients = netmanager->GetConnections();

        Client* p = NULL;
        do
        {
            // this is needed to not block the RemovePlayer later...
            {
                ClientIterator i(*clients);
                p = i.HasNext() ? i.Next() : NULL;
            }

            if(p)
            {
                psAuthRejectedMessage msgb
                (p->GetClientNum(),"The server was restarted or shut down.  Please check the website or forums for more news.");

                eventmanager->Broadcast(msgb.msg, NetBase::BC_FINALPACKET);
                RemovePlayer(p->GetClientNum(),"The server was restarted or shut down.  Please check the website or forums for more news.");
            }
        }
        while(p);

        NetManager::Destroy();
    }

    delete economymanager;
    delete tutorialmanager;
    delete charmanager;
    delete entitymanager;
    delete usermanager;
    delete exchangemanager;
    delete marriageManager;
    delete spawnmanager;
    delete adminmanager;
    delete combatmanager;
    delete weathermanager;
    delete charCreationManager;
    delete workmanager;
    delete mathscriptengine;
    delete progression;
    delete npcmanager;
    delete spellmanager;
    delete minigamemanager;
    delete cachemanager;
    delete questmanager;
    delete database;
    delete logcsv;
    delete rng;
    delete gmeventManager;
    delete bankmanager;
    delete intromanager;
    delete songManager;
    delete serverconsole;
    /*
    PS_CHECK_REF_COUNT(guildmanager);
    PS_CHECK_REF_COUNT(questionmanager);
    PS_CHECK_REF_COUNT(groupmanager);
    PS_CHECK_REF_COUNT(authserver);
    PS_CHECK_REF_COUNT(vfs);
    PS_CHECK_REF_COUNT(eventmanager);
    PS_CHECK_REF_COUNT(configmanager);
    PS_CHECK_REF_COUNT(chatmanager);
    PS_CHECK_REF_COUNT(advicemanager);
    PS_CHECK_REF_COUNT(actionmanager);
    */
    guildmanager    = NULL;
    questionmanager = NULL;
    groupmanager    = NULL;
    authserver      = NULL;
    chatmanager     = NULL;
    advicemanager   = NULL;
    actionmanager   = NULL;
    minigamemanager = NULL;
}

bool psServer::Initialize(iObjectRegistry* object_reg)
{
    // Disable threaded loading.
    // This doesn't work correctly with psserver because the console isn't running in the main thread.
    csRef<iThreadManager> tman = csQueryRegistry<iThreadManager>(object_reg);
    tman->SetAlwaysRunNow(true);

    //Map isn't loaded yet
    MapLoaded=false;

    objreg = object_reg;

    // Start logging asap
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

    rng = new csRandomGen();


    vfs =  csQueryRegistry<iVFS> (objreg);
    configmanager =  csQueryRegistry<iConfigManager> (object_reg);

    if(!configmanager || !vfs)
    {
        Error1("Couldn't find Configmanager!");
        return false;
    }

    // Which plugin is loaded here is specified in psserver.cfg file.

    // Apperantly we need to do this for it to work correctly
    configmanager->Load("psserver.cfg");

    // Load the log settings
    LoadLogSettings();

    // Initialise the CSV logger
    logcsv = new LogCSV(configmanager, vfs);

    // Start Database

    database = new psDatabase(object_reg);

    csString db_host, db_user, db_pass, db_name;
    unsigned int db_port;

    db_host = configmanager->GetStr("PlaneShift.Database.host", "localhost");
    db_user = configmanager->GetStr("PlaneShift.Database.userid", "planeshift");
    db_pass = configmanager->GetStr("PlaneShift.Database.password", "planeshift");
    db_name = configmanager->GetStr("PlaneShift.Database.name", "planeshift");
    db_port = configmanager->GetInt("PlaneShift.Database.port");

    Debug5(LOG_STARTUP,0,COL_BLUE "Database Host: '%s' User: '%s' Databasename: '%s' Port: %d" COL_NORMAL,
           (const char*) db_host, (const char*) db_user, (const char*) db_name, db_port);

    if(!database->Initialize(db_host, db_port, db_user, db_pass, db_name))
    {
        Error2("Could not create database or connect to it: %s",(const char*) database->GetLastError());
        delete database;
        database = NULL;
        return false;
    }

    csString db_version;
    if(! GetServerOption("db_version", db_version))
    {
        CPrintf(CON_ERROR, "Couldn't determine database version.  Error was %s.\n", db->GetLastError());
        db = NULL;
        return false;
    }

    if(strcmp(db_version, DATABASE_VERSION_STR))
    {
        CPrintf(CON_ERROR, "Database version mismatch: we have '%s' while we are looking for '%s'. Recreate your database using create_all.sql\n", (const char*)db_version, DATABASE_VERSION_STR);
        db = NULL;
        return false;
    }


    Debug1(LOG_STARTUP,0,"Started Database");

    Debug1(LOG_STARTUP,0,"Filling loader cache");

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

    Debug1(LOG_STARTUP,0,"Loader cache filled");

    // MathScript Engine
    mathscriptengine = new MathScriptEngine(db,"math_script");

    cachemanager = new CacheManager();

    //Loads the standard motd message from db
    Result result(db->Select("SELECT option_value FROM server_options WHERE option_name = 'standard_motd'"));
    if(result.IsValid()  &&  result.Count()>0)
    {
        motd = result[0][0];
    }
    else
    {
        motd.Clear();
    }

    entitymanager = new EntityManager;

    // Needed by PreloadSpells
    progression = new ProgressionManager(GetConnections(), cachemanager);

    // Initialize DB settings cache
    if(!cachemanager->PreloadAll(entitymanager))
    {
        CPrintf(CON_ERROR, "Could not initialize database cache.\n");
        delete database;
        database = NULL;
        return false;
    }

    Debug1(LOG_STARTUP,0,"Preloaded mesh names, texture names, part names, image names, race info, sector info, traits, item categories, item stats, ways and spells.");

    if(!CharacterLoader.Initialize())
    {
        CPrintf(CON_ERROR, "Could not initialize Character Loader.\n");
        cachemanager->UnloadAll();
        delete database;
        database = NULL;
        return false;
    }

    // Start Network Thread

    netmanager = NetManager::Create(cachemanager, MSGTYPE_PREAUTHENTICATE,MSGTYPE_NPCAUTHENT);
    if(!netmanager)
    {
        return false;
    }

    csString serveraddr =
        configmanager->GetStr("PlaneShift.Server.Addr", "0.0.0.0");
    int port =
        configmanager->GetInt("PlaneShift.Server.Port", 1243);
    Debug3(LOG_STARTUP,0,COL_BLUE "Listening on '%s' Port %d." COL_NORMAL,
           (const char*) serveraddr, port);
    if(!netmanager->Bind(serveraddr, port))
    {
        Error1("Failed to bind");
        delete netmanager;
        netmanager = NULL;
        return false;
    }
    Debug1(LOG_STARTUP,0,"Started Network Thread");


    // Start Event Manager

    // Create the MAIN GAME thread. The eventhandler handles
    // both messages and events. For backward compablility
    // we still store two points. But they are one object
    // and one thread.
    eventmanager.AttachNew(new EventManager);
    if(!eventmanager)
        return false;

    // This gives access to msghandler to all message types
    psMessageCracker::msghandler = eventmanager;

    if(!eventmanager->Initialize(netmanager, 1000))
        return false;

    Debug1(LOG_STARTUP,0,"Started Event Manager Thread");

    if(!progression->Initialize(object_reg))
    {
        Error1("Failed to start progression manager!");
        return false;
    }

    Debug1(LOG_STARTUP,0,"Started Progression Manager");

    // Init Bank Manager.
    bankmanager = new BankManager();

    usermanager = new UserManager(GetConnections(), cachemanager, bankmanager, entitymanager);
    Debug1(LOG_STARTUP,0,"Started User Manager");

    // Load emotes
    if(!usermanager->LoadEmotes("/planeshift/data/emotes.xml", vfs))
    {
        CPrintf(CON_ERROR, "Could not load emotes from emotes.xml");
        return false;
    }

    // Set up wiring for entitymanager
    GEMSupervisor* gem = new GEMSupervisor(object_reg,database, entitymanager, cachemanager);
    psServerDR* psserverdr = new psServerDR(cachemanager, entitymanager);
    if(!entitymanager->Initialize(object_reg, GetConnections(), usermanager, gem, psserverdr, cachemanager))
    {
        Error1("Failed to initialise CEL!");
        delete entitymanager;
        entitymanager = NULL;
        return false;
    }
    entitymanager->SetReady(false);
    usermanager->Initialize(entitymanager->GetGEM());
    netmanager->SetEngine(entitymanager->GetEngine());
    Debug1(LOG_STARTUP,0,"Started CEL");

    // Start Combat Manager
    combatmanager = new CombatManager(cachemanager, entitymanager);
    if(!combatmanager->InitializePVP())
    {
        return false;
    }
    Debug1(LOG_STARTUP,0,"Started Combat Manager");

    // Start Spell Manager
    spellmanager = new SpellManager(GetConnections(), object_reg, cachemanager);
    Debug1(LOG_STARTUP,0,"Started Spell Manager");

    // Start Weather Manager
    weathermanager = new WeatherManager(cachemanager);
    weathermanager->Initialize();
    Debug1(LOG_STARTUP,0,"Started Weather Manager");

    marriageManager = new psMarriageManager();

    questmanager = new QuestManager(cachemanager);

    if(!questmanager->Initialize())
        return false;

    chatmanager.AttachNew(new ChatManager);
    Debug1(LOG_STARTUP,0,"Started Chat Manager");

    guildmanager.AttachNew(new GuildManager(GetConnections(), chatmanager));
    Debug1(LOG_STARTUP,0,"Started Guild Manager");

    questionmanager.AttachNew(new QuestionManager());
    Debug1(LOG_STARTUP,0,"Started Question Manager");

    advicemanager.AttachNew(new AdviceManager(database));
    Debug1(LOG_STARTUP,0,"Started Advice Manager");

    groupmanager.AttachNew(new GroupManager(GetConnections(), chatmanager));
    Debug1(LOG_STARTUP,0,"Started Group Manager");

    charmanager = new ServerCharManager(cachemanager, entitymanager->GetGEM());
    if(!charmanager->Initialize())
        return false;
    Debug1(LOG_STARTUP,0,"Started Character Manager");

    spawnmanager = new SpawnManager(database, cachemanager, entitymanager, entitymanager->GetGEM());
    Debug1(LOG_STARTUP,0,"Started NPC Spawn Manager");

    adminmanager = new AdminManager;
    Debug1(LOG_STARTUP,0,"Started Admin Manager");

    tutorialmanager = new TutorialManager(GetConnections());

    actionmanager.AttachNew(new ActionManager(database));
    Debug1(LOG_STARTUP,0,"Started Action Manager");

    authserver.AttachNew(new AuthenticationServer(GetConnections(), usermanager, guildmanager));
    Debug1(LOG_STARTUP,0,"Started Authentication Server");

    exchangemanager = new ExchangeManager(GetConnections());
    Debug1(LOG_STARTUP,0,"Started Exchange Manager");

    npcmanager = new NPCManager(GetConnections(), database, eventmanager, entitymanager->GetGEM(), cachemanager, entitymanager);
    if(!npcmanager->Initialize())
    {
        Error1("Failed to start npc manager!");
        return false;

    }
    Debug1(LOG_STARTUP,0,"Started NPC Superclient Manager");

    // Start work manager
    workmanager = new WorkManager(cachemanager, entitymanager);
    Debug1(LOG_STARTUP,0,"Started Work Manager");

    // Start economy manager
    economymanager = new EconomyManager();
    Debug1(LOG_STARTUP,0,"Started Economy Manager");
    // Start droping
    economymanager->ScheduleDrop(1 * 60 * 60 * 1000,true); // 1 hour

    // Start minigame manager
    minigamemanager = new MiniGameManager();
    if(!minigamemanager->Initialise())
    {
        Error1("Failed to load minigame data");
        return false;
    }
    Debug1(LOG_STARTUP, 0, "Started Minigame Manager");

    charCreationManager = new CharCreationManager(gem, cachemanager, entitymanager);
    if(!charCreationManager->Initialize())
    {
        Error1("Failed to load character creation data");
        return false;
    }
    Debug1(LOG_STARTUP,0, "Started Character Creation Manager");

    gmeventManager = new GMEventManager();
    if(!gmeventManager->Initialise())
    {
        Error1("Failed to load GM Events Manager");
        return false;
    }

    intromanager = new IntroductionManager();
    Debug1(LOG_STARTUP,0, "Started Introduction Manager");

    songManager = new ServerSongManager();
    if(!songManager->Initialize())
    {
        Error1("Failed to load Song Manager");
        return false;
    }
    Debug1(LOG_STARTUP,0, "Started Song Manager");

    // Init Hire Manager
    hiremanager = new HireManager();
    if (!hiremanager->Initialize())
    {
        Error1("Failed to start hire manager!");
        return false;
    }

    if(!ServerStatus::Initialize(object_reg))
    {
        CPrintf(CON_WARNING, "Warning: Couldn't initialize server status reporter.\n");
    }
    else
    {
        Debug1(LOG_STARTUP,0,"Server status reporter initialized.");
    }

    weathermanager->StartGameTime();
    return true;
}

void psServer::MainLoop()
{
    // Start the server console.
    serverconsole = new ServerConsole(objreg, "psserver", "PS Server");

    csString status("Server initialized");
    logcsv->Write(CSV_STATUS, status);

    // Enter the real main loop - handling events and messages.
    eventmanager->Run();

    status = "Server shutdown";
    logcsv->Write(CSV_STATUS, status);

    // Save log settings
    SaveLogSettings();
}

void psServer::RemovePlayer(uint32_t clientnum,const char* reason)
{
    Client* client = netmanager->GetConnections()->FindAny(clientnum);
    if(!client)
    {
        CPrintf(CON_WARNING, "Tried to remove non-existent client: %d\n", clientnum);
        return;
    }

    csString ipAddr = client->GetIPAddress();

    csString status;
    status.Format("%s, %u, Client (%s) removed", ipAddr.GetDataSafe(), client->GetClientNum(), client->GetName());

    psserver->GetLogCSV()->Write(CSV_AUTHENT, status);

    Notify3(LOG_CONNECTIONS, "Remove player '%s' (%d)", client->GetName(),client->GetClientNum());

    client->Disconnect();

    chatmanager->RemoveAllChannels(client);

    authserver->SendDisconnect(client,reason);

    entitymanager->DeletePlayer(client);

    if(client->IsSuperClient())
    {
        npcmanager->Disconnect(client);
    }

    netmanager->GetConnections()->MarkDelete(client);
}

void psServer::MutePlayer(uint32_t clientnum,const char* reason)
{
    Client* client = netmanager->GetConnections()->Find(clientnum);
    if(!client)
    {
        CPrintf(CON_WARNING, "Tried to mute non-existent client: %d\n", clientnum);
        return;
    }

    CPrintf(CON_DEBUG, "Mute player '%s' (%d)\n", client->GetName(),
            client->GetClientNum());

    client->SetMute(true);

    psserver->SendSystemInfo(client->GetClientNum(),reason);
}

void psServer::UnmutePlayer(uint32_t clientnum,const char* reason)
{
    Client* client = netmanager->GetConnections()->Find(clientnum);
    if(!client)
    {
        CPrintf(CON_WARNING, "Tried to unmute non-existent client: %d\n", clientnum);
        return;
    }

    CPrintf(CON_DEBUG, "Unmute player '%s' (%d)\n", client->GetName(),
            client->GetClientNum());

    client->SetMute(false);
    psserver->SendSystemInfo(client->GetClientNum(),reason);
}

bool psServer::LoadMap(const char* mapname)
{
    if(entitymanager->LoadMap(mapname))
    {
        MapLoaded=true;
    }
    return MapLoaded;
}

bool psServer::IsReady()
{
    if(!entitymanager)
        return false;

    return entitymanager->IsReady();
}


bool psServer::HasBeenReady()
{
    if(!entitymanager)
        return false;

    return entitymanager->HasBeenReady();
}

bool psServer::IsFull(size_t numclients, Client* client)
{
    unsigned int maxclients = GetConfig()->GetInt("PlaneShift.Server.User.connectionlimit", 20);

    if(client)
    {
        return numclients > maxclients &&
               !cachemanager->GetCommandManager()->Validate(client->GetSecurityLevel(), "always login");
    }
    else
    {
        return numclients > maxclients;
    }
}


void psServer::SendSystemInfo(int clientnum, const char* fmt, ...)
{
    if(clientnum == 0 || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    csString cssLine = csString().FormatV(fmt, args);
    va_end(args);

    // As the message fmt was already parsed there is no need to do it again.
    psSystemMessageSafe newmsg(clientnum ,MSG_INFO, cssLine.GetData());

    if(newmsg.valid)
    {
        // Save to chat history (PS#2789)
        if(Client* cl = GetConnections()->Find(clientnum))
            cl->GetActor()->LogSystemMessage(cssLine.GetData());
        eventmanager->SendMessage(newmsg.msg);
    }
    else
    {
        Bug2("Could not create valid psSystemMessage for client %u.",clientnum);
    }
}

void psServer::SendSystemBaseInfo(int clientnum, const char* fmt, ...)
{
    if(clientnum == 0 || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    csString cssLine = csString().FormatV(fmt, args);
    va_end(args);

    // As the message fmt was already parsed there is no need to do it again.
    psSystemMessageSafe newmsg(clientnum ,MSG_INFO_BASE, cssLine.GetData());

    if(newmsg.valid)
    {
        // Save to chat history (PS#2789)
        if(Client* cl = GetConnections()->Find(clientnum))
            cl->GetActor()->LogSystemMessage(cssLine.GetData());
        eventmanager->SendMessage(newmsg.msg);
    }
    else
    {
        Bug2("Could not create valid psSystemMessage for client %u.",clientnum);
    }
}

void psServer::SendSystemResult(int clientnum, const char* fmt, ...)
{
    if(clientnum == 0 || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    csString cssLine = csString().FormatV(fmt, args);
    va_end(args);

    // As the message fmt was already parsed there is no need to do it again.
    psSystemMessageSafe newmsg(clientnum ,MSG_RESULT, cssLine.GetData());

    if(newmsg.valid)
    {
        // Save to chat history (PS#2789)
        if(Client* cl = GetConnections()->Find(clientnum))
            cl->GetActor()->LogSystemMessage(cssLine.GetData());
        eventmanager->SendMessage(newmsg.msg);
    }
    else
    {
        Bug2("Could not create valid psSystemMessage for client %u.",clientnum);
    }
}

void psServer::SendSystemOK(int clientnum, const char* fmt, ...)
{
    if(clientnum == 0 || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    csString cssLine = csString().FormatV(fmt, args);
    va_end(args);

    // As the message fmt was already parsed there is no need to do it again.
    psSystemMessageSafe newmsg(clientnum ,MSG_OK, cssLine.GetData());

    if(newmsg.valid)
    {
        // Save to chat history (PS#2789)
        if(Client* cl = GetConnections()->Find(clientnum))
            cl->GetActor()->LogSystemMessage(cssLine.GetData());
        eventmanager->SendMessage(newmsg.msg);
    }
    else
    {
        Bug2("Could not create valid psSystemMessage for client %u.",clientnum);
    }
}

void psServer::SendSystemError(int clientnum, const char* fmt, ...)
{
    if(clientnum == 0 || fmt == NULL)
        return;

    va_list args;
    va_start(args, fmt);
    csString cssLine = csString().FormatV(fmt, args);
    va_end(args);

    // As the message fmt was already parsed there is no need to do it again.
    psSystemMessageSafe newmsg(clientnum ,MSG_ERROR, cssLine.GetData());

    if(newmsg.valid)
    {
        // Save to chat history (PS#2789)
        if(Client* cl = GetConnections()->Find(clientnum))
            cl->GetActor()->LogSystemMessage(cssLine.GetData());
        eventmanager->SendMessage(newmsg.msg);
    }
    else
    {
        Bug2("Could not create valid psSystemMessage for client %u.",clientnum);
    }
}

void psServer::LoadLogSettings()
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

    csString debugFile =  configmanager->GetStr("PlaneShift.DebugFile");
    if(debugFile.Length() > 0)
    {
        csRef<iStandardReporterListener> reporter =  csQueryRegistry<iStandardReporterListener > (objreg);


        reporter->SetMessageDestination(CS_REPORTER_SEVERITY_DEBUG, true, false ,false, false, true, false);
        reporter->SetMessageDestination(CS_REPORTER_SEVERITY_ERROR, true, false ,false, false, true, false);
        reporter->SetMessageDestination(CS_REPORTER_SEVERITY_BUG, true, false ,false, false, true, false);

        time_t curr=time(0);
        tm* gmtm = gmtime(&curr);

        csString timeStr;
        timeStr.Format("-%d-%02d-%02d-%02d:%02d:%02d",
                       gmtm->tm_year+1900,
                       gmtm->tm_mon+1,
                       gmtm->tm_mday,
                       gmtm->tm_hour,
                       gmtm->tm_min,
                       gmtm->tm_sec);

        debugFile.Append(timeStr);
        reporter->SetDebugFile(debugFile, true);

        Debug2(LOG_STARTUP,0,"PlaneShift Server Log Opened............. %s", timeStr.GetData());

    }
}

void psServer::SaveLogSettings()
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

ClientConnectionSet* psServer::GetConnections()
{
    return netmanager->GetConnections();
}

/*-----------------Buddy List Management Functions-------------------------*/

bool psServer::AddBuddy(PID self, PID buddy)
{
    int rows = db->Command("INSERT INTO character_relationships (character_id, related_id, relationship_type) VALUES (%u, %u, 'buddy')",
                           self.Unbox(), buddy.Unbox());

    if(rows != 1)
    {
        database->SetLastError(database->GetLastSQLError());
        return false;
    }

    return true;
}

bool psServer::RemoveBuddy(PID self, PID buddy)
{
    int rows = db->Command("DELETE FROM character_relationships WHERE character_id=%d AND related_id=%d AND relationship_type='buddy'",
                           self.Unbox(), buddy.Unbox());

    if(rows != 1)
    {
        database->SetLastError(database->GetLastSQLError());
        return false;
    }

    return true;
}

void psServer::UpdateDialog(const char* area, const char* trigger,
                            const char* response, int num)
{
    csString escTrigger;
    csString escArea;
    csString escResp;

    db->Escape(escTrigger, trigger);
    db->Escape(escArea, area);
    db->Escape(escResp, response);

    // Find the response id:
    int id = db->SelectSingleNumber("SELECT response_id FROM npc_triggers "
                                    "WHERE trigger=\"%s\" AND area=\"%s\"",
                                    escTrigger.GetData(), escArea.GetData());


    db->Command("UPDATE npc_responses SET response%d=\"%s\" WHERE id=%d",
                num, escResp.GetData(), id);
}



void psServer::UpdateDialog(const char* area, const char* trigger,
                            const char* prohim, const char* proher,
                            const char* proit,     const char* prothem)
{
    csString escTrigger;
    csString escArea;
    csString escHim,escHer,escIt,escThem;

    db->Escape(escTrigger, trigger);
    db->Escape(escArea, area);
    db->Escape(escHim, prohim);
    db->Escape(escHer, proher);
    db->Escape(escIt, proit);
    db->Escape(escThem, prothem);


    // Find the response id:
    int id = db->SelectSingleNumber("SELECT response_id FROM npc_triggers "
                                    "WHERE trigger=\"%s\" AND area=\"%s\"",
                                    escTrigger.GetData(), escArea.GetData());


    db->Command("UPDATE npc_responses SET "
                "pronoun_him=\"%s\", "
                "pronoun_her=\"%s\", "
                "pronoun_it=\"%s\", "
                "pronoun_them=\"%s\" "
                "WHERE id=%d",
                escHim.GetData(), escHer.GetData(),
                escIt.GetData(), escThem.GetData(), id);
}

iResultSet* psServer::GetAllTriggersInArea(csString data)
{
    iResultSet* rs;

    csString escape;
    db->Escape(escape, data);
    rs = db->Select("SELECT trigger FROM npc_triggers "
                    "WHERE area='%s'", escape.GetData());
    if(!rs)
    {
        Error2("db ERROR: %s", db->GetLastError());
        Error2("LAST QUERY: %s", db->GetLastQuery());
        return NULL;
    }

    return rs;
}

iResultSet* psServer::GetAllResponses(csString &trigger)
{
    iResultSet* rs;
    csString escTrigger;
    db->Escape(escTrigger, trigger);


    // Get the response ID:
    int id = db->SelectSingleNumber("SELECT response_id FROM npc_triggers "
                                    "WHERE trigger=\"%s\"",
                                    escTrigger.GetData());

    if(!id)
    {
        Error2("db ERROR: %s", db->GetLastError());
        Error2("LAST QUERY: %s", db->GetLastQuery());
        return NULL;
    }


    rs = db->Select("SELECT * FROM npc_responses "
                    "WHERE id=%d",id);
    if(!rs)
    {
        Error2("db ERROR: %s", db->GetLastError());
        Error2("LAST QUERY: %s", db->GetLastQuery());
        return NULL;
    }

    return rs;
}


iResultSet* psServer::GetSuperclientNPCs(int superclientID)
{
    iResultSet* rs;

    rs = db->Select("SELECT id"
                    "  FROM characters"
                    " WHERE account_id='%d'",
                    superclientID);
    if(!rs)
    {
        database->SetLastError(database->GetLastSQLError());
    }

    return rs;
}


bool psServer::GetServerOption(const char* option_name,csString &value)
{
    csString escape;
    db->Escape(escape, option_name);
    Result result(db->Select("select option_value from server_options where "
                             "option_name='%s'", escape.GetData()));

    if(!result.IsValid())
    {
        csString temp;
        temp.Format("Couldn't execute query.\nCommand was "
                    "<%s>.\nError returned was <%s>\n",
                    db->GetLastQuery(),db->GetLastError());
        database->SetLastError(temp);
        return false;
    }

    if(result.Count() == 1)
    {
        value = result[0]["option_value"];
        return true;
    }

    return false;
}

bool psServer::SetServerOption(const char* option_name,const csString &value)
{
    csString escape, dummy;
    bool bExists = GetServerOption(option_name, dummy);
    unsigned long result;

    db->Escape(escape, option_name);

    if(bExists)
        result = db->Command("update server_options set option_value='%s' where option_name='%s'", value.GetData(), option_name);
    else
        result = db->Command("insert into server_options(option_name, option_value) values('%s','%s')", option_name, value.GetData());

    return result==1;
}

bool psServer::CheckAccess(Client* client, const char* command, bool returnError)
{
    if(returnError)
    {
        bool gotAccess;
        csString errorMessage;
        gotAccess = cachemanager->GetCommandManager()->Validate(client->GetSecurityLevel(), command, errorMessage);
        if(gotAccess)
            return true;

        SendSystemError(client->GetClientNum(), errorMessage);
        return false;
    }

    return cachemanager->GetCommandManager()->Validate(client->GetSecurityLevel(), command);
}

class psQuitEvent : public psGameEvent
{
public:
    psQuitEvent(EntityManager* entitymanager, csTicks msecDelay, psQuitEvent* quit_event, csString message,
                bool server_lock, bool server_shutdown)
        : psGameEvent(0,msecDelay,"psDelayedQuitEvent")
    {
        message_quit_event = quit_event;
        mytext = message;
        trigger_server_lock = server_lock;
        trigger_server_shutdown = server_shutdown;
        entityManager = entitymanager;
    }
    virtual void Trigger()
    {
        psSystemMessage newmsg(0, MSG_INFO_SERVER, mytext);
        psserver->GetEventManager()->Broadcast(newmsg.msg);
        CPrintf(CON_CMDOUTPUT, "%s\n", mytext.GetDataSafe());
        if(trigger_server_lock) //This is triggering the server lock
            entityManager->SetReady(false);
        if(trigger_server_shutdown) //This is triggering the server shut down
            psserver->GetEventManager()->Stop();
    }
    virtual bool CheckTrigger()
    {
        //If this is the event triggering the server shut down pass it's validity, else check that event if
        //it's still valid
        return message_quit_event == NULL ? valid : message_quit_event->CheckTrigger();
    }
    void Invalidate()
    {
        valid = false; //This is used to invalidate the server shut down event
    }
private:
    csString mytext; ///< Keeps the message which will be sent to clients
    bool trigger_server_lock; ///< If true this is the event locking the server
    bool trigger_server_shutdown; ///< If true this is the event which will shut down the server
    psQuitEvent* message_quit_event; ///< Stores a link to the master event which will shut down the server
    EntityManager* entityManager;
};

/// Shuts down the server and exit program
void psServer::QuitServer(int time, Client* client) //-1 for stop, 0 for now > 0 for timed shutdown
{
    if(time == -1) //if the user passed 'stop' (-1) we will abort the shut down process
    {
        if(server_quit_event == NULL) //if there is no shutdown event active avoid showing anything
        {
            csString error_msg = "The server is not restarting right now";
            CPrintf(CON_CMDOUTPUT, "%s\n", error_msg.GetDataSafe());
            if(client)
                SendSystemInfo(client->GetClientNum(),error_msg.GetDataSafe());
            return;
        }
        server_quit_event->Invalidate(); //there is a quit event let's make it invalid
        if(IsMapLoaded() && HasBeenReady())  //remake the server available if it was locked in the process
            entitymanager->SetReady(true);
        server_quit_event = NULL;  //we don't need it anymore so let's clean ourselves of it
        //Let the user know about the new situation about the server
        csString abort_msg = "Server Admin: The server is no longer restarting.";
        psSystemMessage newmsg(0, MSG_INFO_SERVER, abort_msg);
        GetEventManager()->Broadcast(newmsg.msg);
        CPrintf(CON_CMDOUTPUT, "%s\n", abort_msg.GetDataSafe());
    }
    else
    {
        if(server_quit_event == NULL) //check that there isn't a quit event already running...
        {
            uint quit_delay = time; //if the user passed a number we will read it
            if(quit_delay)  //we have an argument > 0 so let's put an event for server shut down
            {
                if(quit_delay > 1440) //We limit the maximum quit delay to 24 hours
                {
                    csString error_msg = "The specified quit time is too high. Try with a lower value.";
                    CPrintf(CON_CMDOUTPUT, "%s\n", error_msg.GetDataSafe());
                    if(client)
                        SendSystemInfo(client->GetClientNum(),error_msg.GetDataSafe());
                    return;
                }
                //we got less than 5 minutes for shut down so let's lock the server immediately
                if(quit_delay < 5) entitymanager->SetReady(false);

                //generates the messages to alert the user and allocates them in the queque
                for(uint i = 3; i > 0; i--) //i = 3 sets up the 0 seconds message and so is the event triggering
                {
                    //shutdown
                    csString outtext = "Server Admin: The server will shut down in ";
                    outtext += 60-(i*20);
                    outtext += " seconds.";
                    psQuitEvent* Quit_event = new psQuitEvent(entitymanager, ((quit_delay-1)*60+i*20)*1000, server_quit_event,
                            outtext, false, i == 3);
                    GetEventManager()->Push(Quit_event);
                    if(!server_quit_event) server_quit_event = Quit_event;
                }
                csString outtext = "Server Admin: The server will shut down in 1 minute.";
                psQuitEvent* Quit_event = new psQuitEvent(entitymanager, (quit_delay-1)*60*1000, server_quit_event, outtext,
                        false, false);
                GetEventManager()->Push(Quit_event);

                if(quit_delay == 1) return; //if the time we had was 1 minute no reason to go on
                uint quit_time = (quit_delay < 5)? quit_delay : 5; //manage the period 1<x<5 minutes
                while(1)
                {
                    csString outtext = "Server Admin: The server will shut down in ";
                    outtext += quit_time;
                    outtext += " minutes.";
                    psQuitEvent* Quit_event = new psQuitEvent(entitymanager, (quit_delay-quit_time)*60*1000, server_quit_event,
                            outtext, quit_time == 5, false);
                    GetEventManager()->Push(Quit_event);
                    if(quit_time == quit_delay)
                    {
                        break;
                    } //we have got to the first message saying the server
                    //will be shut down let's go out of the loop
                    else if(quit_time+5 > quit_delay)
                    {
                        quit_time = quit_delay;
                    } //we have reached the second message
                    //saying the server will shut down
                    //so manage the case of not multiple
                    //of 5 minutes shut down times
                    else
                    {
                        quit_time +=5;
                    } //we have still a long way so let's go to the next 5 minutes message
                }
            }
            else //we have no arguments or the argument passed is zero let's quit immediately
            {
                GetEventManager()->Stop();
            }
        }
        else //we have found a quit event so we will inform the user about that
        {
            uint planned_shutdown = (server_quit_event->triggerticks-csGetTicks())/1000; //gets the seconds
            //to the event
            uint minutes = planned_shutdown/60; //get the minutes to the event
            uint seconds = planned_shutdown%60; //get the seconds to the event when the minutes are subtracted
            csString quitInfo = "The server is already shutting down in ";
            if(minutes) //if we don't have minutes (so they are zero) skip them
                quitInfo += minutes;
            quitInfo += " minutes ";
            if(seconds) //if we don't have seconds (so they are zero) skip them
                quitInfo += seconds;
            quitInfo += " seconds";
            CPrintf(CON_CMDOUTPUT, "%s\n", quitInfo.GetDataSafe()); //send the message to the server console
            if(client)
                SendSystemInfo(client->GetClientNum(),quitInfo.GetDataSafe());
        }
    }
    return;
}
