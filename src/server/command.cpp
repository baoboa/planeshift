/*
 * command.cpp
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
 * The commands, the user can type in in the console are defined here
 * if you write a new one, don't forget to add it to the list at the end of
 * the file.
 *
 * Author: Matthias Braun <MatzeBraun@gmx.de>
 */

#include <psconfig.h>

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <ctype.h>

#include <iutil/object.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <csutil/csstring.h>
#include <csutil/md5.h>
#include <iutil/stringarray.h>
#include <iengine/collection.h>
#include <iengine/engine.h>

#include "util/command.h"
#include "util/serverconsole.h"
#include "util/eventmanager.h"
#include "net/messages.h"
#include "globals.h"
#include "psserver.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "netmanager.h"
#include "util/strutil.h"
#include "gem.h"
#include "invitemanager.h"
#include "entitymanager.h"
#include "util/psdatabase.h"
#include "spawnmanager.h"
#include "actionmanager.h"
#include "psproxlist.h"
#include "adminmanager.h"
#include "groupmanager.h"
#include "progressionmanager.h"
#include "combatmanager.h"
#include "weathermanager.h"
#include "rpgrules/factions.h"
#include "util/dbprofile.h"
#include "economymanager.h"
#include "questmanager.h"
#include "chatmanager.h"
#include "engine/psworld.h"
#include "bulkobjects/dictionary.h"
#include "bulkobjects/psnpcdialog.h"

#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psnpcloader.h"
#include "bulkobjects/psaccountinfo.h"
#include "bulkobjects/pstrainerinfo.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psattack.h"
#include "bulkobjects/pssectorinfo.h"
#include "bulkobjects/psraceinfo.h"

int com_lock(const char*)
{
    EntityManager::GetSingleton().SetReady(false);
    return 0;
}

int com_ready(const char*)
{
    if(psserver->IsMapLoaded())
    {
        EntityManager::GetSingleton().SetReady(true);
        CPrintf(CON_CMDOUTPUT, "Server is now ready\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT, "Failed to switch server to ready! No map loaded\n");
    }

    return 0;
}

/// Shuts down the server and exit program
int com_quit(const char*  arg)
{
    if(strncasecmp(arg,"stop", 4) == 0) //if the user passed 'stop' we will abort the shut down process
    {
        psserver->QuitServer(-1, NULL);
    }
    else
    {
        psserver->QuitServer(atoi(arg), NULL);
    }
    return 0;
}

/* Print out help for ARG, or for all of the commands if ARG is
not present. */
int com_help(const char* arg)
{
    register int i;
    int printed = 0;

    CPrintf(CON_CMDOUTPUT ,"\n");
    for(i = 0; commands[i].name; i++)
    {
        if(!*arg || (strcmp(arg, commands[i].name) == 0))
        {
            if(strlen(commands[i].name) < 8)
                CPrintf(CON_CMDOUTPUT ,"%s\t\t%s.\n", commands[i].name, commands[i].doc);
            else
                CPrintf(CON_CMDOUTPUT ,"%s\t%s.\n", commands[i].name, commands[i].doc);
            printed++;
        }
    }

    if(!printed)
    {
        CPrintf(CON_CMDOUTPUT ,"No commands match `%s'.  Possibilities are:\n", arg);

        for(i = 0; commands[i].name; i++)
        {
            /* Print in six columns. */
            if(printed == 6)
            {
                printed = 0;
                CPrintf(CON_CMDOUTPUT ,"\n");
            }

            CPrintf(CON_CMDOUTPUT ,"%s\t", commands[i].name);
            printed++;
        }

        if(printed)
            CPrintf(CON_CMDOUTPUT ,"\n");
    }

    CPrintf(CON_CMDOUTPUT ,"\n");

    return (0);
}

static inline csString PS_GetClientStatus(Client* client)
{
    csString status;
    if(client->IsReady())
    {
        if(client->GetConnection()->heartbeat == 0)
        {
            status = "ok";
        }
        else
        {
            status.Format("connection problem(%d)",
                          client->GetConnection()->heartbeat);
        }
    }
    else
    {
        if(client->GetActor())
        {
            status = "loading";
        }
        else
        {
            status = "connecting";
        }
    }

    return status;
}

int com_settime(const char* arg)
{
    if(!arg || strlen(arg) == 0)
    {
        CPrintf(CON_CMDOUTPUT,"Please provide a time to use\n");
        return 0;
    }

    int hour = atoi(arg);
    int minute = 0;

    if(hour > 23 || hour < 0)
    {
        CPrintf(CON_CMDOUTPUT, "Select a time between 0-23\n");
        return 0;
    }

    psserver->GetWeatherManager()->SetGameTime(hour,minute);
    CPrintf(CON_CMDOUTPUT, "Current Game Hour set to: %d:%02d\n", hour,minute);

    return 0;
}

int com_showtime(const char*)
{
    CPrintf(CON_CMDOUTPUT,"Game time is %d:%02d %d-%d-%d\n",
            psserver->GetWeatherManager()->GetGameTODHour(),
            psserver->GetWeatherManager()->GetGameTODMinute(),
            psserver->GetWeatherManager()->GetGameTODYear(),
            psserver->GetWeatherManager()->GetGameTODMonth(),
            psserver->GetWeatherManager()->GetGameTODDay());
    return 0;
}


int com_setmaxout(const char* arg)
{
    if(!arg || strlen(arg) == 0)
    {
        CPrintf(CON_CMDOUTPUT, "Use one of the following values to control output on standard output:\n");
        CPrintf(CON_CMDOUTPUT, "  0: no output at all\n");
        CPrintf(CON_CMDOUTPUT, "  1: only output of server commands\n");
        CPrintf(CON_CMDOUTPUT, "  2: 1 + errors\n");
        CPrintf(CON_CMDOUTPUT, "  3: 2 + warnings\n");
        CPrintf(CON_CMDOUTPUT, "  4: 3 + notifications\n");
        CPrintf(CON_CMDOUTPUT, "  5: 4 + debug messages\n");
        CPrintf(CON_CMDOUTPUT, "  6: 5 + spam\n");
        return 0;
    }
    int msg = CON_SPAM;;
    sscanf(arg, "%d", &msg);
    ConsoleOut::SetMaximumOutputClassStdout((ConsoleOutMsgClass)msg);
    return 0;
}

int com_setmaxfile(const char* arg)
{
    if(!arg || strlen(arg) == 0)
    {
        CPrintf(CON_CMDOUTPUT, "Use one of the following values to control output on output file:\n");
        CPrintf(CON_CMDOUTPUT, "  0: no output at all\n");
        CPrintf(CON_CMDOUTPUT, "  1: only output of server commands\n");
        CPrintf(CON_CMDOUTPUT, "  2: 1 + errors\n");
        CPrintf(CON_CMDOUTPUT, "  3: 2 + warnings\n");
        CPrintf(CON_CMDOUTPUT, "  4: 3 + notifications\n");
        CPrintf(CON_CMDOUTPUT, "  5: 4 + debug messages\n");
        CPrintf(CON_CMDOUTPUT, "  6: 5 + spam\n");
        return 0;
    }
    int msg = CON_SPAM;;
    sscanf(arg, "%d", &msg);
    ConsoleOut::SetMaximumOutputClassFile((ConsoleOutMsgClass)msg);
    return 0;
}

int com_netprofile(const char*)
{
    psNetMsgProfiles* profs = psserver->GetNetManager()->GetProfs();
    csString dumpstr = profs->Dump();
    csRef<iFile> file = psserver->vfs->Open("/this/netprofile.txt",VFS_FILE_WRITE);
    file->Write(dumpstr, dumpstr.Length());
    CPrintf(CON_CMDOUTPUT, "Net profile dumped to netprofile.txt\n");
    profs->Reset();
    return 0;
}

int com_dbprofile(const char*)
{
    csString dumpstr = db->DumpProfile();
    csRef<iFile> file = psserver->vfs->Open("/this/dbprofile.txt",VFS_FILE_WRITE);
    file->Write(dumpstr, dumpstr.Length());
    CPrintf(CON_CMDOUTPUT, "DB profile dumped to dbprofile.txt\n");
    db->ResetProfile();
    return 0;
}

int com_queue(const char* player)
{
    int playernum = atoi(player);
    if(playernum<=0)
    {
        CPrintf(CON_CMDOUTPUT ,"Please Specify the number of the client!\n");
        CPrintf(CON_CMDOUTPUT ,"  (you can use 'status' to see a list of clients and their"
                "numbers.)\n");
        return 0;
    }

    uint32_t cnum = (uint32_t) playernum;
    Client* client = psserver->GetNetManager()->GetConnections()->Find(cnum);
    if(!client)
    {
        CPrintf(CON_CMDOUTPUT ,COL_RED "Client with number %u not found!\n" COL_NORMAL,
                cnum);
        return 0;
    }

    CPrintf(CON_CMDOUTPUT, "OutQueue size for client %d is %d\n", playernum, client->outqueue->Count());
    return 0;

}

/** print out server status */
int com_status(const char*)
{
    bool ready = psserver->IsReady();
    bool hasBeenReady = psserver->HasBeenReady();

    CPrintf(CON_CMDOUTPUT ,"Server           : " COL_CYAN "%s\n" COL_NORMAL,
            (ready ^ hasBeenReady) ? "is shutting down" : "is running.");
    CPrintf(CON_CMDOUTPUT ,"World            : " COL_CYAN "%s\n" COL_NORMAL,
            hasBeenReady ? "loaded and running." : "not loaded.");
    CPrintf(CON_CMDOUTPUT ,"Connection Count : " COL_CYAN "%d\n" COL_NORMAL,
            psserver->GetNetManager()->GetConnections()->Count());
    CPrintf(CON_CMDOUTPUT ,COL_GREEN "%-5s %-7s %-25s %14s %10s %9s %s %s %s %s\n" COL_NORMAL,"EID","PID","Name","CNum","Ready","Time con.", "RTT", "Window filled", "Est. packet loss", "Packets sent");

    ClientConnectionSet* clients = psserver->GetNetManager()->GetConnections();
    if(psserver->GetNetManager()->GetConnections()->Count() == 0)
    {
        CPrintf(CON_CMDOUTPUT ,"  *** List Empty ***\n");
        return 0;
    }

    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* client = i.Next();
        csString clientStatus;
        clientStatus.Format("%5d %7d %-25s %14d %10s",
                            client->GetActor() ? client->GetActor()->GetEID().Unbox() : 0,
                            client->GetPID().Unbox(),
                            client->GetName(),
                            client->GetClientNum(),
                            (const char*) PS_GetClientStatus(client));

        psCharacter* character = client->GetCharacterData();
        //if the client lacks a character (eg: npcclient) show for now 0:0:0 as time
        //so the table doesn't get screwed up
        unsigned int time = character? character->GetOnlineTimeThisSession() : 0;
        unsigned int hour,min,sec;
        sec = time%60;
        time = time/60;
        min = time%60;
        hour = time/60;
        clientStatus.AppendFmt(" %3u:%02u:%02u",hour,min,sec);

        if(client->GetConnection())
        {
            clientStatus.AppendFmt(" %4u %4u %4.2f %4u", client->GetConnection()->RTO,
                                   client->GetConnection()->window, client->GetConnection()->sends > 0 ?  100.0 * client->GetConnection()->resends/client->GetConnection()->sends : 0.0, client->GetConnection()->sends);
        }
        clientStatus.Append('\n');
        CPrintf(CON_CMDOUTPUT ,clientStatus.GetData());
    }

    return 0;
}

int com_say(const char* text)
{
    csString outtext = "Server Admin: ";
    outtext += text;

    psSystemMessage newmsg(0, MSG_INFO_SERVER, outtext);
    psserver->GetEventManager()->Broadcast(newmsg.msg);
    CPrintf(CON_CMDOUTPUT, "%s\n", (const char*) outtext);

    return 0;
}

int com_sayGossip(const char* text)
{
    psChatMessage newMsg(0, 0, "Server Admin", 0, text, CHAT_CHANNEL, false, 1);
    psserver->GetChatManager()->SendServerChannelMessage(newMsg, 1);

    CPrintf(CON_CMDOUTPUT, "%s\n", text);

    return 0;
}

int com_kick(const char* player)
{
    int playernum = atoi(player);
    if(playernum<=0)
    {
        CPrintf(CON_CMDOUTPUT ,"Please Specify the number of the client!\n");
        CPrintf(CON_CMDOUTPUT ,"  (you can use 'status' to see a list of clients and their"
                "numbers.)\n");
        return 0;
    }

    uint32_t cnum = (uint32_t) playernum;
    Client* client = psserver->GetNetManager()->GetConnections()->Find(cnum);
    if(!client)
    {
        CPrintf(CON_CMDOUTPUT ,COL_RED "Client with number %u not found!\n" COL_NORMAL,
                cnum);
        return 0;
    }

    psserver->RemovePlayer(cnum,"You were kicked from the server by a GM.");
    return 0;
}

int com_delete(const char* name)
{
    if(strlen(name) == 0)
    {
        CPrintf(CON_CMDOUTPUT ,"Usuage:\nPS Server: delete charactername\n");
        return 0;
    }

    //Check to see if this player needs to be kicked first
    Client* client = psserver->GetNetManager()->GetConnections()->Find(name);
    if(client)
    {
        psserver->RemovePlayer(client->GetClientNum(),"Your character is being deleted so you are being kicked.");
    }

    PID characteruid=psserver->CharacterLoader.FindCharacterID(name);
    if(!characteruid.IsValid())
    {
        CPrintf(CON_CMDOUTPUT ,"Character <%s> was not found in the database.\n", name);
        return 0;
    }

    csString error;
    if(!psserver->CharacterLoader.DeleteCharacterData(characteruid,error))
    {
        if(!error.Length())
            CPrintf(CON_CMDOUTPUT ,"Character <%s> was not found in the database.\n", name);
        else
            CPrintf(CON_CMDOUTPUT ,"Character <%s> error: %s .\n", name,error.GetData());
        return 0;
    }


    CPrintf(CON_CMDOUTPUT ,"Character <%s> was removed from the database.\n", name);

    return 0;
}


int com_set(const char* args)
{
    iConfigManager* cfgmgr = psserver->GetConfig();

    if(!strcmp(args, ""))
    {
        CPrintf(CON_CMDOUTPUT ,"Please give the name of the var you want to change:\n");

        csRef<iConfigIterator> ci = cfgmgr->Enumerate("PlaneShift.Server.User.");
        if(!ci)
        {
            return 0;
        }

        while(ci->HasNext())
        {
            ci->Next();

            if(ci->GetKey(true))
            {
                if(ci->GetComment())
                {
                    CPrintf(CON_CMDOUTPUT ,"%s",ci->GetComment());
                }

                CPrintf(CON_CMDOUTPUT ,COL_GREEN "%s = '%s'\n" COL_NORMAL,
                        ci->GetKey(true), ci->GetStr());
            }
        }
    }
    else
    {
        WordArray words(args);
        csString a1 = "PlaneShift.Server.User.";
        a1 += words[0];
        csString a2 = words[1];
        if(a2.IsEmpty())
        {
            CPrintf(CON_CMDOUTPUT ,COL_GREEN " %s = '%s'\n" COL_NORMAL,
                    (const char*) a1, cfgmgr->GetStr(a1, ""));
        }
        else
        {
            cfgmgr->SetStr(a1, a2);
            CPrintf(CON_CMDOUTPUT ,COL_GREEN " SET %s = '%s'\n" COL_NORMAL,
                    (const char*) a1, cfgmgr->GetStr(a1, ""));
        }
    }

    return 0;
}

int com_maplist(const char*)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());
    if(!vfs)
        return 0;

    csRef<iDataBuffer> xpath = vfs->ExpandPath("/planeshift/world/");
    const char* dir = **xpath;
    csRef<iStringArray> files = vfs->FindFiles(dir);

    if(!files)
        return 0;
    for(size_t i=0; i < files->GetSize(); i++)
    {
        char* name = csStrNew(files->Get(i));
        char* onlyname = name + strlen("/planeshift/world/");
        onlyname[strlen(onlyname)] = '\0';
        CPrintf(CON_CMDOUTPUT ,"%s\n",onlyname);
        delete [] name;
    }
    return 0;
}

int com_dumpwarpspace(const char*)
{
    EntityManager::GetSingleton().GetWorld()->DumpWarpCache();
    return 0;
}


int com_loadmap(const char* mapname)
{
    if(!strcmp(mapname, ""))
    {
        CPrintf(CON_CMDOUTPUT ,COL_RED "Please specify any zip file in your /art/world"
                " directory:\n" COL_NORMAL);
        com_maplist(NULL);
        return 0;
    }

    if(!psserver->LoadMap(mapname))
    {
        CPrintf(CON_CMDOUTPUT ,"Couldn't load map %s!\n", mapname);
        return 0;
    }

    return 0;
}

int com_spawn(const char* sector = 0)
{
    // After world is loaded, repop NPCs--only the first time.
    static bool already_spawned = false;
    psSectorInfo* sectorinfo = NULL;
    if(sector)
        sectorinfo = psserver->GetCacheManager()->GetSectorInfoByName(sector);

    if(!already_spawned)
    {
        psserver->GetSpawnManager()->RepopulateLive(sectorinfo);
        psserver->GetSpawnManager()->RepopulateItems(sectorinfo);
        psserver->GetActionManager()->RepopulateActionLocations(sectorinfo);
        psserver->GetSpawnManager()->LoadHuntLocations(sectorinfo); // Start spawning
        already_spawned = true;
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ,"Already respawned everything.\n");
    }
    return 0;
}

int com_rain(const char* arg)
{
    const char*  syntax = "rain <sector> <drops> <fade> <duration>";
    if(strlen(arg) == 0)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify sector and number of drops (minimum 2000 drops for lightning, 0 drops to stop rain) and duration (0 for default duration).\nSyntax: %s\n",syntax);
        return 0;
    }
    WordArray words(arg);
    csString sector(words[0]);
    int drops = atoi(words[1]);
    int fade = atoi(words[2]);
    int length = atoi(words[3]);
    psSectorInfo* sectorinfo = psserver->GetCacheManager()->GetSectorInfoByName(sector.GetData());
    if(!sectorinfo)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not find that sector.\nSyntax: %s\n",syntax);
        return 0;
    }
    if(drops < 0 || (length <= 0 && drops > 0))
    {
        CPrintf(CON_CMDOUTPUT ,"Drops must be >= 0. Length must be > 0 if drops > 0.\nSyntax: %s\n",syntax);
        return 0;
    }
    if(drops == 0)
    {
        CPrintf(CON_CMDOUTPUT ,"Stopping rain in sector %s.\n", sector.GetData());
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ,"Starting rain in sector %s with %d drops for %d ticks with fade %d.\n",
                sector.GetData(), drops, length, fade);
    }

    psserver->GetWeatherManager()->QueueNextEvent(0, psWeatherMessage::RAIN, drops, length, fade,
            sector.GetData(), sectorinfo);
    return 0;
}

int com_dict(const char* arg)
{
    WordArray words(arg);
    if(words.GetCount()>1)
    {
        csString fullname = words[0]+" "+words[1];
        dict->Print(fullname.GetDataSafe());
    }
    else
        dict->Print(words[0].GetDataSafe());
    return 0;
}

int com_filtermsg(const char* arg)
{
    CPrintf(CON_CMDOUTPUT ,"%s\n",psserver->GetNetManager()->LogMessageFilter(arg).GetDataSafe());

    return 0;
}

int com_loadnpc(const char* npcName)
{

    //TODO Rewrite
    CPrintf(CON_CMDOUTPUT, "Disabled at the moment");



//    AdminManager * adminmgr = psserver->GetAdminManager();
//
//    csString name = npcName;
//
    // Call this thread save function to initiate loading of NPC.
//    adminmgr->AdminCreateNewNPC(name);

    return 0;
}

int com_loadquest(const char* stringId)
{
    int id = atoi(stringId);
    CPrintf(CON_CMDOUTPUT, "Reloading quest id %d\n", id);

    if(!psserver->GetCacheManager()->UnloadQuest(id))
        CPrintf(CON_CMDOUTPUT, "Could not remove quest %d\n", id);
    else
        CPrintf(CON_CMDOUTPUT, "Existing quest removed.\n");

    if(!psserver->GetCacheManager()->LoadQuest(id))
        CPrintf(CON_CMDOUTPUT, "Could not load quest %d\n", id);
    else
        CPrintf(CON_CMDOUTPUT, "Quest %d loaded.\n", id);
    return 0;
}


int com_importnpc(const char* filename)
{
    psNPCLoader npcloader;

    csString file;
    file.Format("/this/%s", filename);

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());

    csString newDir(file);
    newDir.Append("/");

    csRef<iStringArray> filenames = vfs->FindFiles(newDir);
    if(filenames->GetSize() > 0)
    {
        csString currentFile;
        size_t failed = 0;
        size_t count = filenames->GetSize();

        while(filenames->GetSize() > 0)
        {

            currentFile = filenames->Pop();

            // skip non xml files
            if(currentFile.Find(".xml") == (size_t)-1)
                continue;

            if(npcloader.LoadFromFile(currentFile))
                CPrintf(CON_CMDOUTPUT ,"Succesfully imported NPC from file %s.\n", currentFile.GetData());
            else
            {
                failed++;
                CPrintf(CON_WARNING ,"Failed to import NPC from file %s.\n\n", currentFile.GetData());
            }
        }
        CPrintf(CON_CMDOUTPUT, "Successfully imported %u NPCs.\n", count - failed);
        return 0;
    };


    if(npcloader.LoadFromFile(file))
    {
        CPrintf(CON_CMDOUTPUT ,"Succesfully imported NPC.\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ,"Failed to import NPC.\n");
    }

    return 0;
}


int com_exportnpc(const char* args)
{
    WordArray words(args);
    csArray<int> npcids;
    bool all = false;

    if(words.GetCount() == 1 && !strcasecmp(words.Get(0),"all"))
        all = true;

    if(words.GetCount()!=2 && !all)
    {
        CPrintf(CON_CMDOUTPUT ,"Usage: /exportnpc <id> <filename>\n");
        CPrintf(CON_CMDOUTPUT ,"       /exportnpc all\n");
        return -1;
    }

    psNPCLoader npcloader;

    if(all)
    {
        Result result(db->Select("SELECT id from characters where npc_master_id !=0"));
        if(!result.IsValid())
        {
            CPrintf(CON_ERROR, "Cannot load character ids from database.\n");
            CPrintf(CON_ERROR, db->GetLastError());
            return -1;
        }
        if(result.Count() == 0)
        {
            CPrintf(CON_ERROR, "No NPCs to export.\n");
            return -1;
        }

        csString filename;
        int npcid;
        unsigned long failed = 0;

        for(unsigned long i=0; i<result.Count(); i++)
        {
            npcid = result[i].GetInt(0);
            filename.Format("npc-id%i.xml", npcid);
            if(!npcloader.SaveToFile(result[i].GetInt(0), filename))
            {
                failed++;
                CPrintf(CON_WARNING, "Failed to export npc %i\n\n", npcid);
            }
        }
        CPrintf(CON_CMDOUTPUT, "Successfully exported %u NPCs.\n", result.Count() - failed);
        return 0;
    }

    int id = atoi(words.Get(0));
    csString fileName = words.Get(1);

    if(npcloader.SaveToFile(id, fileName))
    {
        CPrintf(CON_CMDOUTPUT ,"Succesfully exported NPC.\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ,"Failed to export NPC.\n");
    }

    return 0;
}


int com_importdialogs(const char* filename)
{
    csString file;

    if(filename==NULL || filename[0] == '\0')
    {
        CPrintf(CON_WARNING ,"Please speficy a filename, pattern or 'all' (for all files in /this/).\n\n");
        return 0;
    }

    printf("-%s-",filename);

    if(!strcmp(filename,"all"))
        file="/this/";
    else
        file.Format("/this/%s", filename);

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());

    csString newDir(file);
    newDir.Append("/");

    csRef<iStringArray> filenames = vfs->FindFiles(newDir);
    if(filenames->GetSize() > 0)
    {
        csString currentFile;
        size_t failed = 0;
        size_t count = filenames->GetSize();
        psNPCLoader npcloader;

        while(filenames->GetSize() > 0)
        {
            currentFile = filenames->Pop();

            // skip non xml files
            if(currentFile.Find(".xml") == (size_t)-1)
                continue;

            if(npcloader.LoadDialogsFromFile(currentFile))
                CPrintf(CON_CMDOUTPUT ,"Succesfully imported dialogs from file %s.\n", currentFile.GetData());
            else
            {
                failed++;
                CPrintf(CON_WARNING ,"Failed to import dialogs from file %s.\n\n", currentFile.GetData());
            }
        }
        CPrintf(CON_CMDOUTPUT, "Successfully imported %u dialogs.\n", count - failed);
        return 0;
    };

    psNPCLoader npcloader;
    if(npcloader.LoadDialogsFromFile(file))
    {
        CPrintf(CON_CMDOUTPUT ,"Succesfully imported NPC dialogs.\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ,"Failed to import NPC dialogs.\n");
    }

    return 0;
}


int com_exportdialogs(const char* args)
{
    bool allquests = false;
    WordArray words(args);

    if(words.GetCount() == 1 && !strcasecmp(words.Get(0),"allquests"))
        allquests = true;

    if(words.GetCount()!=3 && !allquests)
    {
        CPrintf(CON_CMDOUTPUT ,"Usage: /exportdialogs area <areaname> <filename>\n");
        //CPrintf(CON_CMDOUTPUT ,"       /exportdialogs queststep <questid> <filename>\n");
        CPrintf(CON_CMDOUTPUT ,"       /exportdialogs quest <questid> <filename>\n");
        CPrintf(CON_CMDOUTPUT ,"       /exportdialogs allquests\n");
        return -1;
    }

    if(allquests)
    {
        Result questids(db->Select("SELECT id FROM quests WHERE master_quest_id = 0"));
        if(questids.Count() == 0)
        {
            CPrintf(CON_ERROR, "No complete quests found.\n");
            return -1;
        }

        psNPCLoader npcloader;
        csString filename;
        csString areaname("");
        int questid;
        unsigned long failed = 0;

        for(unsigned long i=0; i<questids.Count(); i++)
        {
            questid = questids[i].GetInt(0);
            filename.Format("quest-id%i.xml",questid);
            if(!npcloader.SaveDialogsToFile(areaname, filename, questid, true))
            {
                failed++;
                CPrintf(CON_WARNING, "Failed to export quest %i\n\n", questid);
            }
        }
        CPrintf(CON_CMDOUTPUT, "Successfully exported %u quests.\n", questids.Count() - failed);
        return 0;
    }

    csString type = words.Get(0);
    csString areaname = words.Get(1);
    csString filename = words.Get(2);
    bool quest = type.CompareNoCase("quest");

    int questid = -1;

    if(quest)
    {
        questid = atoi(areaname.GetData());
        areaname.Clear();
        Result masterQuest(db->Select("SELECT master_quest_id FROM quests WHERE id = '%i'", questid));
        if(masterQuest.Count() < 1)
        {
            CPrintf(CON_CMDOUTPUT, "No quest with that id found in the quests table.\n");
            return -1;
        }
        if(masterQuest[0].GetInt(0) != 0)
        {
            CPrintf(CON_CMDOUTPUT, "This quest is not a complete quest.\n");
            return -1;
        }
    }
    else if(!type.CompareNoCase("area"))
    {
        CPrintf(CON_CMDOUTPUT ,"Usage: /exportdialogs area <areaname> <filename>\n");
        //CPrintf(CON_CMDOUTPUT ,"       /exportdialogs queststep <questid> <filename>\n");
        CPrintf(CON_CMDOUTPUT ,"       /exportdialogs quest <questid> <filename>\n");
        return -1;
    }

    psNPCLoader npcloader;
    // trick to allow export of areas with spaces, just use $ instead, example Pet$Groffel$1
    areaname.ReplaceAll("$"," ");
    if(npcloader.SaveDialogsToFile(areaname, filename, questid, quest))
    {
        CPrintf(CON_CMDOUTPUT ,"Succesfully exported NPC dialogs\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT ,"Failed to export NPC dialogs\n");
    }

    return 0;
}


int com_newacct(const char* arg)
{
    csStringArray words(arg,"/");
    if(words.GetSize() < 2)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify username/password[/securitylevel].\n");
        return 0;
    }

    csString username = words[0];
    csString password = words[1];
    csString levelStr = words[2];

    int level = strtoul(levelStr.GetDataSafe(), NULL, 0); //aquires the level

    psAccountInfo accountinfo;

    accountinfo.username = username;
    accountinfo.password = csMD5::Encode(password).HexString();
    accountinfo.securitylevel = level;

    if(psserver->GetCacheManager()->NewAccountInfo(&accountinfo)==0)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not create account.\n");
        return 0;
    }
    CPrintf(CON_CMDOUTPUT ,"Account created successfully.\n");

    return 0;
}

/*****
int com_newguild(char *nameleader)
{
    char *leadername;
    if (!nameleader)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify guildname/leader.\n");
        return 0;
    }
    char *slash = strchr(nameleader,'/');
    if (!slash)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify guildname/leader with a slash (/) between them.\n");
        return 0;
    }
    *slash = 0;
    leadername=slash+1;

    unsigned int leaderuid=psserver->CharacterLoader.FindCharacterID(leadername);
    if (leaderuid==0)
    {
        CPrintf(CON_CMDOUTPUT ,"That leader name was not found.\n");
        return 0;
    }


    int rc = psserver->GetDatabase()->CreateGuild(nameleader,leaderuid);

    switch(rc)
    {
    case 0: CPrintf(CON_CMDOUTPUT ,"Guild created successfully.\n");
        break;
    case 1: CPrintf(CON_CMDOUTPUT ,"That guildname already exists.\n");
        break;
    case 2: CPrintf(CON_CMDOUTPUT ,"That leader name was not found.\n");
        break;
    default: CPrintf(CON_CMDOUTPUT ,"SQL Error: %s\n",psserver->GetDatabase()->GetLastError());
        break;
    }

    return 0;
}

int com_joinguild(char *guild_member)
{
    char *membername;
    if (!guild_member)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify guildname/newmember.\n");
        return 0;
    }
    char *slash = strchr(guild_member,'/');
    if (!slash)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify guildname/newmember with a slash (/) between them.\n");
        return 0;
    }
    *slash = 0;
    membername=slash+1;

    int guild = psserver->GetDatabase()->GetGuildID(guild_member);
    unsigned int memberuid = psserver->CharacterLoader.FindCharacterID(membername);

    if (guild==-1 || memberuid==0)
    {
        CPrintf(CON_CMDOUTPUT ,"guild or member name is not found.\n");
        return 0;
    }

    int rc = psserver->GetDatabase()->JoinGuild(guild,memberuid);

    switch(rc)
    {
    case 0:  CPrintf(CON_CMDOUTPUT ,"Guild joined successfully.\n");
        break;
    case -2: CPrintf(CON_CMDOUTPUT ,"Character is already a member of another guild.\n");
        break;
    default: CPrintf(CON_CMDOUTPUT ,"SQL Error: %s\n",psserver->GetDatabase()->GetLastError());
        break;
    }

    return 0;
}


int com_quitguild(char *name)
{
    if (!name)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify member who is leaving.\n");
        return 0;
    }

        unsigned int memberuid=psserver->CharacterLoader.FindCharacterID(name);

    if (memberuid==0)
    {
        CPrintf(CON_CMDOUTPUT ,"Character name is not found.  Sorry.\n");
        return 0;
    }

    int rc = psserver->GetDatabase()->LeaveGuild(memberuid);

    switch(rc)
    {
    case 0:  CPrintf(CON_CMDOUTPUT ,"Guild left successfully.\n");
        break;
    case -3: CPrintf(CON_CMDOUTPUT ," is not a member of a guild right now.\n");
        break;
    default: CPrintf(CON_CMDOUTPUT ,"SQL Error: %s\n",psserver->GetDatabase()->GetLastError());
        break;
    }

    return 0;
}
******************************/

int com_addinv(const char* line)
{
    bool temploaded=false;

    if(!line)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify Character Name and Basic Item Template Name and optionally a stack count.\n");
        return 0;
    }

    WordArray words(line);

    csString charactername = words[0];
    csString item   = words[1];
    //int stack_count = words.GetInt(2);

    if(!charactername.Length() || !item.Length())
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify Character Name and Basic Item Template ID and optionally a stack count.\n");
        return 0;
    }

    // Get the UID of this character based on the provided name.  This ensures the name is accurate.
    PID characteruid = psserver->CharacterLoader.FindCharacterID(charactername.GetData());
    if(!characteruid.IsValid())
    {
        CPrintf(CON_CMDOUTPUT ,"Character name not found.\n");
        return 0;
    }

    // Get the ItemStats based on the name provided.
    psItemStats* itemstats=psserver->GetCacheManager()->GetBasicItemStatsByID(atoi(item.GetData()));
    if(itemstats==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"No Basic Item Template with that id was found.\n");
        return 0;
    }

    // If the character is online, update the stats live.  Otherwise we need to load the character data to add this item to
    //  an appropriate inventory slot.
    psCharacter* chardata=NULL;
    Client* client = psserver->GetNetManager()->GetConnections()->Find(charactername.GetData());
    if(!client)
    {
        // Character is not online
        chardata=psserver->CharacterLoader.LoadCharacterData(characteruid,false);
        temploaded=true;
    }
    else
        chardata=client->GetCharacterData();

    if(chardata==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not get character data for specified character.\n");
        return 0;
    }


    psItem* iteminstance = itemstats->InstantiateBasicItem();
    if(iteminstance==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not instantiate item based on basic properties.\n");
        return 0;
    }

    iteminstance->SetLoaded();  // Item is fully created
    if(!chardata->Inventory().Add(iteminstance, false, false))
    {
        CPrintf(CON_CMDOUTPUT ,"The item did not fit into the character's inventory.\n");
        psserver->GetCacheManager()->RemoveInstance(iteminstance);
        return 0;
    }
   
    // If we temporarily loaded the character for this add, unload them now
    if(temploaded)
        delete chardata;


    CPrintf(CON_CMDOUTPUT ,"Added item '%s' to character '%s'.\n",item.GetData(),charactername.GetData());

    return 0;
}

csString get_item_modifiers(psItem* item)
{
    csString modifiers;
    modifiers = "(Modifiers:";
    bool use_comma = false;
    int i;
    for(i = 0 ; i < PSITEM_MAX_MODIFIERS ; i++)
    {
        psItemStats* stats = item->GetModifier(i);
        if(stats)
        {
            if(use_comma) modifiers += ", ";
            use_comma = true;
            modifiers += stats->GetName();
        }
    }
    if(!use_comma) modifiers += "none)";
    else modifiers += ")";
    return modifiers;
}

csString get_item_stats(psItem* item)
{
    csString stats;
    stats.Format("Qual:%g Guild:%u Crafter:%u DecayResist:%g SumWeight:%g Uni:%c",
                 item->GetItemQuality(),
                 item->GetGuildID(),
                 item->GetCrafterID().Unbox(),
                 item->GetDecayResistance(),
                 0.0, ////// TODO: Hardcoded zero weight
                 //////////////////////////item->GetSumWeight(),
                 item->GetIsUnique() ? 'Y' : 'N');
    return stats;
}

static void indent(int depth)
{
    for(int i=0; i<depth; i++) CPrintf(CON_CMDOUTPUT ,"    ");
}

void show_itemstat_stats(const char* prefix,psItemStats* itemstats,int depth)
{
    indent(depth);
    PSITEM_FLAGS flags = itemstats->GetFlags();
    csString flags_string;
    if(flags & PSITEMSTATS_FLAG_IS_A_MELEE_WEAPON) flags_string += "MELEE ";
    if(flags & PSITEMSTATS_FLAG_IS_A_RANGED_WEAPON) flags_string += "RANGED ";
    if(flags & PSITEMSTATS_FLAG_IS_A_SHIELD) flags_string += "SHIELD ";
    if(flags & PSITEMSTATS_FLAG_IS_AMMO) flags_string += "AMMO ";
    if(flags & PSITEMSTATS_FLAG_IS_A_CONTAINER) flags_string += "CONTAINER ";
    if(flags & PSITEMSTATS_FLAG_IS_A_TRAP) flags_string += "TRAP ";
    if(flags & PSITEMSTATS_FLAG_IS_CONSTRUCTIBLE) flags_string += "CONSTRUCTIBLE ";
    if(flags & PSITEMSTATS_FLAG_USES_AMMO) flags_string += "USESAMMO ";
    if(flags & PSITEMSTATS_FLAG_IS_STACKABLE) flags_string += "STACKABLE ";
    if(flags & PSITEMSTATS_FLAG_IS_GLYPH) flags_string += "GLYPH ";
    if(flags & PSITEMSTATS_FLAG_CAN_TRANSFORM) flags_string += "TRANSFORM ";
    if(flags & PSITEMSTATS_FLAG_NOPICKUP) flags_string += "NOPICKUP ";
    if(flags & PSITEMSTATS_FLAG_TRIA) flags_string += "TRIA ";
    if(flags & PSITEMSTATS_FLAG_HEXA) flags_string += "HEXA ";
    if(flags & PSITEMSTATS_FLAG_OCTA) flags_string += "OCTA ";
    if(flags & PSITEMSTATS_FLAG_CIRCLE) flags_string += "CIRCLE ";
    if(flags & PSITEMSTATS_FLAG_CONSUMABLE) flags_string += "CONSUMABLE ";
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"%s Flags: %s\n", prefix, flags_string.GetData());
    }
}

void show_item_stats(psItem* item,int depth)
{
    depth++;
    if(item->GetDescription())
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"Desc:%s\n", item->GetDescription());
    }

    indent(depth);
    CPrintf(CON_CMDOUTPUT ,"Weight:%g Size:%g ContainerMaxSize:%d VisDistance:%g DecayResist:%g\n",
            item->GetWeight(),
            item->GetItemSize(),
            item->GetContainerMaxSize(),
            item->GetVisibleDistance(),
            item->GetDecayResistance());

    if(item->GetMeshName())
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"MeshName:%s\n", item->GetMeshName());
    }
    if(item->GetTextureName())
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"TextureName:%s\n", item->GetTextureName());
    }
    if(item->GetPartName())
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"PartName:%s\n", item->GetPartName());
    }
    if(item->GetImageName())
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"ImageName:%s\n", item->GetImageName());
    }

    psMoney price = item->GetPrice();
    if(price.GetTotal() > 0)
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"Price Circles:%d Octas:%d Hexas:%d Trias:%d Total:%d\n",
                price.GetCircles(), price.GetOctas(), price.GetHexas(), price.GetTrias(),
                price.GetTotal());
    }

    PSITEM_FLAGS flags = item->GetFlags();
    csString flags_string;
    if(flags & PSITEM_FLAG_UNIQUE_ITEM) flags_string += "UNIQUE ";
    if(flags & PSITEM_FLAG_PURIFIED) flags_string += "PURIFIED ";
    if(flags & PSITEM_FLAG_PURIFYING) flags_string += "PURIFYING ";
    if(flags & PSITEM_FLAG_LOCKED) flags_string += "LOCKED ";
    if(flags & PSITEM_FLAG_LOCKABLE) flags_string += "LOCKABLE ";
    if(flags & PSITEM_FLAG_SECURITYLOCK) flags_string += "SECURITYLOCK ";
    if(flags & PSITEM_FLAG_UNPICKABLE) flags_string += "UNPICKABLE ";
    if(flags & PSITEM_FLAG_KEY) flags_string += "KEY ";
    if(flags & PSITEM_FLAG_MASTERKEY) flags_string += "MASTERKEY ";
    if(flags & PSITEM_FLAG_SETTINGITEM) flags_string += "SETTINGITEM ";
    if(flags_string.Length() > 0)
    {
        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"Flags: %s\n", flags_string.GetData());
    }

    //psItemStats* basestats = item->GetBaseStats();
    //if (basestats)
    //show_itemstat_stats("Base",basestats,depth);
    psItemStats* curstats = item->GetCurrentStats();
    if(curstats)
        show_itemstat_stats("Current",curstats,depth);

    PSITEMSTATS_ARMORTYPE armortype = item->GetArmorType();
    if(armortype != PSITEMSTATS_ARMORTYPE_NONE)
    {
        indent(depth);
        switch(armortype)
        {
            case PSITEMSTATS_ARMORTYPE_LIGHT:
                CPrintf(CON_CMDOUTPUT ,"Armor:Light");
                break;
            case PSITEMSTATS_ARMORTYPE_MEDIUM:
                CPrintf(CON_CMDOUTPUT ,"Armor:Medium");
                break;
            case PSITEMSTATS_ARMORTYPE_HEAVY:
                CPrintf(CON_CMDOUTPUT ,"Armor:Heavy");
                break;
            default:
                ;
        }
        CPrintf(CON_CMDOUTPUT ," Hardness:%g\n",
                item->GetHardness());
    }
    csString weapontype = item->GetWeaponType()->name;
    if (!weapontype.IsEmpty())
    {

        indent(depth);
        CPrintf(CON_CMDOUTPUT ,"WType:%s", weapontype.GetData()); 

        CPrintf(CON_CMDOUTPUT ," Latency:%g Penetration:%g untgtblock:%g tgtblock=%g cntblock=%g\n",
                item->GetLatency(),
                item->GetPenetration(),
                item->GetUntargetedBlockValue(),
                item->GetTargetedBlockValue(),
                item->GetCounterBlockValue());
    }
    PSITEMSTATS_AMMOTYPE ammotype = item->GetAmmoType();
    if(ammotype != PSITEMSTATS_AMMOTYPE_NONE)
    {
        indent(depth);
        switch(ammotype)
        {
            case PSITEMSTATS_AMMOTYPE_ARROWS:
                CPrintf(CON_CMDOUTPUT ,"Ammo:Arrows\n");
                break;
            case PSITEMSTATS_AMMOTYPE_BOLTS:
                CPrintf(CON_CMDOUTPUT ,"Ammo:Bolts\n");
                break;
            case PSITEMSTATS_AMMOTYPE_ROCKS:
                CPrintf(CON_CMDOUTPUT ,"Ammo:Rocks\n");
                break;
            default:
                ;
        }
    }
}

csString com_showinv_itemextra(bool moreiteminfo, psItem* currentitem)
{
    if(moreiteminfo)
        return get_item_modifiers(currentitem) + " " + get_item_stats(currentitem);
    else
        return csString("");
}

void com_showinv_item(bool moreiteminfo, psItem* currentitem, const char* slotname, unsigned int slotnumber)
{
//    psItem *workingset[5];
//    unsigned int positionset[5];
    csString output;

    if(currentitem!=NULL)
    {
        if(moreiteminfo)
        {
            output.AppendFmt("%s\tCount: %d\tID:%u Instance ID:%u \n",currentitem->GetName(),
                             currentitem->GetStackCount(),currentitem->GetBaseStats()->GetUID(), currentitem->GetUID());
        }
        else
        {
            output.AppendFmt("%s\tCount: %d\t", currentitem->GetName(),currentitem->GetStackCount());
        }
        csString siextra = com_showinv_itemextra(moreiteminfo,currentitem);
        if(slotnumber != (unsigned int)-1)
        {
            output.AppendFmt("(%s%u) Count:%u %s\n",slotname,slotnumber,currentitem->GetStackCount(), siextra.GetData());
            //   CPrintf(CON_CMDOUTPUT ,"%s (%s%u) %s\n",currentitem->GetName(),slotname,slotnumber,
            //   siextra.GetData());
        }
        else
        {
            output.AppendFmt("(%s) Extra: %s\n",slotname, siextra.GetData());
            //   CPrintf(CON_CMDOUTPUT ,"%s (%s) %s\n",currentitem->GetName(),slotname,
            //   siextra.GetData());
        }
        CPrintf(CON_CMDOUTPUT, output);
        if(moreiteminfo)
            show_item_stats(currentitem,0);

        /*******************************************************************************************
        if (currentitem->GetIsContainer())
        {
            workingset[0]=currentitem;
            positionset[0]=0;
            depth=1;
            while (depth>1 || positionset[depth-1]<currentitem->GetContainerMaxSlots())
            {
                currentitem=workingset[depth-1]->GetItemInSlot(positionset[depth-1]);
                if (currentitem!=NULL)
                {
                    for (int i=0;i<depth;i++)
                        CPrintf(CON_CMDOUTPUT ,"   -");
            csString siextra = com_showinv_itemextra(moreiteminfo,currentitem);
                    CPrintf(CON_CMDOUTPUT ,"%s %s\n",currentitem->GetName(), siextra.GetData());
            if (moreiteminfo)
            show_item_stats(currentitem,depth);

                    // Check recursion
                    if (currentitem->GetIsContainer())
                    {
                        if (depth>=5)
                        {
                            for (int i=0;i<depth;i++)
                                CPrintf(CON_CMDOUTPUT ,"   -");
                            CPrintf(CON_CMDOUTPUT ,"Items deeper than max depth of 5.\n");
                        }
                        else
                        {
                            workingset[depth]=currentitem;
                            positionset[depth]=0;
                            depth++;
                            continue;
                        }
                    }
                }
                // Next item at this level
                positionset[depth-1]++;

                // Drop to previous level
                while (depth>1 && positionset[depth-1]>=currentitem->GetContainerMaxSlots())
                {
                    depth--;
                    positionset[depth-1]++;
                }
            }
        }
        *********************************************************/
    }
}

int com_showinv(const char* line, bool moreiteminfo)
{
    bool temploaded=false;

    if(!line)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify a character name.\n");
        return 0;
    }

    // If the character is online use the active stats.  Otherwise we need to load the character data.
    psCharacter* chardata=NULL;
    PID characteruid = psserver->CharacterLoader.FindCharacterID(line,false);
    if(!characteruid.IsValid())
    {
        CPrintf(CON_CMDOUTPUT ,"Character name is not found.\n");
        return 0;
    }

    gemObject* obj = psserver->entitymanager->GetGEM()->FindPlayerEntity(characteruid);
    if(!obj)
    {
        obj = psserver->entitymanager->GetGEM()->FindNPCEntity(characteruid);
    }
    // If the character is online use the active stats.  Otherwise we need to load the character data.
    if(obj)
    {
        chardata = obj->GetCharacterData();
    }
    else
    {
        // Character is not online
        CPrintf(CON_CMDOUTPUT,"Loading inventory from database because character is not found online.\n");
        chardata=psserver->CharacterLoader.LoadCharacterData(characteruid,true);
        temploaded=true;
    }

    if(chardata==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not get character data for specified character.\n");
        return 0;
    }

    /*  Iterating through a character's items is not standard enough to have an iterator yet.
     *   So we implement the logic here.
     */
    psItem* currentitem;
    unsigned int charslot;

///KWF    CPrintf(CON_CMDOUTPUT ,"Weight: %.2f MaxWeight: %.2f Capacity: %.2f MaxCapacity: %.2f\n",
///            chardata->Inventory().Weight(),chardata->Inventory().MaxWeight(),
///            chardata->Inventory().Capacity(), chardata->Inventory().MaxCapacity());

    if(moreiteminfo)
        CPrintf(CON_CMDOUTPUT ,"%d trias, %d hexas, %d octas, %d circles\n",
                chardata->Money().GetTrias(),
                chardata->Money().GetHexas(),
                chardata->Money().GetOctas(),
                chardata->Money().GetCircles());

    CPrintf(CON_CMDOUTPUT ,"Total money: %d\n", chardata->Money().GetTotal());

    // Inventory indexes start at 1.  0 is reserved for the "NULL" item.
    for(charslot=1; charslot<chardata->Inventory().GetInventoryIndexCount(); charslot++)
    {
        currentitem=chardata->Inventory().GetInventoryIndexItem(charslot);
        const char* name = psserver->GetCacheManager()->slotNameHash.GetName(currentitem->GetLocInParent());
        char buff[20];
        if(!name)
        {
            cs_snprintf(buff,19,"Bulk %d",currentitem->GetLocInParent(true));
        }
        com_showinv_item(moreiteminfo,currentitem, name ? name : buff,(unsigned int)-1);
    }

    if(temploaded)
        delete chardata;

    return 0;
}

int com_showinv(const char* line)
{
    return com_showinv(line, false);
}

int com_showinvf(const char* line)
{
    return com_showinv(line, true);
}

int com_bulkdelete(const char* line)
{
    if(!line)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify a source file.\n");
        return 0;
    }
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());

    if(!vfs->Exists(line))
    {
        CPrintf(CON_CMDOUTPUT ,"The specified file doesn't exist.\n");
        return 0;
    }
    if(line[strlen(line)-1] == '/')
    {
        CPrintf(CON_CMDOUTPUT ,"The specified file doesn't exist.\n");
        return 0;
    }

    csRef<iDataBuffer> filedataBuf = vfs->ReadFile(line);
    const char* filedata = *(*filedataBuf);

    int totalCount = 0, failCount = 0;

    while(*filedata != 0)
    {
        while(*filedata == '\n')
            ++filedata;

        //Extract a line
        char fline[100];
        size_t len = strcspn(filedata, "\n");
        memcpy(fline, filedata, len);
        fline[len] = '\0';
        filedata += len;

        int characteruid_int = atoi(fline);
        if(characteruid_int < 1)
            continue;

        ++totalCount;

        PID characteruid(characteruid_int);

        csString error;
        if(!psserver->CharacterLoader.DeleteCharacterData(characteruid,error))
        {
            if(!error.Length())
                CPrintf(CON_CMDOUTPUT ,"Character <%u> was not found in the database.\n", characteruid.Unbox());
            else
                CPrintf(CON_CMDOUTPUT ,"Character <%u> error: %s .\n", characteruid.Unbox(),error.GetData());
            ++failCount;
        }
    }

    CPrintf(CON_CMDOUTPUT, "Found %u characters, deleted %u successfully, while %u failed.\n", totalCount, (totalCount - failCount), failCount);

    return 0;
}

int com_exec(const char* line)
{
    if(!line)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify a script file.\n");
        return 0;
    }
    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psserver->GetObjectReg());

    if(!vfs->Exists(line))
    {
        CPrintf(CON_CMDOUTPUT ,"The specified file doesn't exist.\n");
        return 0;
    }
    if(line[strlen(line)-1] == '/')
    {
        CPrintf(CON_CMDOUTPUT ,"The specified file doesn't exist.\n");
        return 0;
    }

    csRef<iDataBuffer> script = vfs->ReadFile(line);
    ServerConsole::ExecuteScript(*(*script));

    return 0;
}

int com_print(const char* line)
{
    if(!line || !atoi(line))
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify an entity #.\n");
        return 0;
    }
    EID eid(strtoul(line, NULL, 10));
    gemObject* obj;

    obj = psserver->entitymanager->GetGEM()->FindObject(eid);

    if(obj)
    {
        obj->Dump();

        gemNPC* npc = obj->GetNPCPtr();
        if(npc)
        {
            CPrintf(CON_CMDOUTPUT ,"--------- NPC Information -------\n");
            psNPCDialog* npcdlg = npc->GetNPCDialogPtr();

            if(npcdlg)
            {
                npcdlg->DumpDialog();
            }


        }


        return 0;
    }

    CPrintf(CON_CMDOUTPUT ,"Entity %s was not found.\n", ShowID(eid));
    return 0;
}


int com_entlist(const char* pattern)
{
    csHash<gemObject*, EID> &gems = psserver->entitymanager->GetGEM()->GetAllGEMS();
    csHash<gemObject*, EID>::GlobalIterator i(gems.GetIterator());
    gemObject* obj;

    CPrintf(CON_CMDOUTPUT ,"%-5s %-7s %-15s %-20s Position\n","EID","PID","Type","Name");
    while(i.HasNext())
    {
        obj = i.Next();
        if(!obj)
        {
            continue;
        }

        if(!pattern || strstr(obj->GetName(),pattern) || atoi(pattern) == (int)obj->GetEID().Unbox())
        {
            csVector3 pos;
            float     rot;
            iSector*  sector;
            obj->GetPosition(pos,rot,sector);
            const char* sector_name =
                (sector) ? sector->QueryObject()->GetName():"(null)";

            CPrintf(CON_CMDOUTPUT ,"%5d %7d %-15s %-20s (%9.3f,%9.3f,%9.3f, %s)\n",
                    obj->GetEID().Unbox(),
                    obj->GetPID().Unbox(),
                    obj->GetObjectType(),
                    obj->GetName(),
                    pos.x,pos.y,pos.z,sector_name);
        }
    }

    return 0;
}

int com_charlist(const char*)
{
    csHash<gemObject*, EID> &gems = psserver->entitymanager->GetGEM()->GetAllGEMS();
    csHash<gemObject*, EID>::GlobalIterator i(gems.GetIterator());
    gemObject* obj;

    CPrintf(CON_CMDOUTPUT ,"%-9s %-5s %-9s  %-9s %-10s %-20s\n","PID","EID","CNUM","SCNUM","Type","Name");
    while(i.HasNext())
    {
        obj = i.Next();
        gemActor* actor = dynamic_cast<gemActor*>(obj);
        if(actor)
        {
            CPrintf(CON_CMDOUTPUT ,"%9u %5u %9u %9u %-10s %-20s\n",
                    actor->GetCharacterData()->GetPID().Unbox(),
                    actor->GetEID().Unbox(),
                    actor->GetClientID(),
                    actor->GetSuperclientID().Unbox(),
                    actor->GetObjectType(),
                    actor->GetName());
        }
    }

    return 0;
}

int com_racelist(const char* pattern)
{
    uint32_t count = (uint32_t)psserver->GetCacheManager()->GetRaceInfoCount();

    CPrintf(CON_CMDOUTPUT,"%-20s %10s %10s %-20s\n","Race","WalkSpeed","RunSpeed","Size");
    for(uint32_t i=0; i < count; i++)
    {
        psRaceInfo* ri = psserver->GetCacheManager()->GetRaceInfoByIndex(i);

        CPrintf(CON_CMDOUTPUT,"%-20s %10.2f %10.2f %20s\n",ri->name.GetDataSafe(),ri->walkBaseSpeed,ri->runBaseSpeed,
                toString(ri->size).GetDataSafe());
    }
    return 0;
}


int com_factions(const char*)
{

    csHash<gemObject*, EID> &gems = psserver->entitymanager->GetGEM()->GetAllGEMS();
    csHash<gemObject*, EID>::GlobalIterator itr(gems.GetIterator());
    gemObject* obj;

    csArray<gemActor*> actors;
    size_t i;
    size_t j;
    while(itr.HasNext())
    {
        obj = itr.Next();

        if(obj && obj->GetActorPtr())
        {
            gemActor* actor = obj->GetActorPtr();
            if(actor)
                actors.Push(actor);
        }
    }

    size_t num = actors.GetSize();
    {
        csString output;
        output.AppendFmt("                     ");
        for(i = 0; i < num; i++)
        {
            output.AppendFmt("%*s ", (int)csMax((size_t)7, strlen(actors[i]->GetName())), actors[i]->GetName());
        }
        CPrintf(CON_CMDOUTPUT ,"%s\n",output.GetDataSafe());
    }

    for(i = 0; i < num; i++)
    {
        csString output;

        output.AppendFmt("%20s ",actors[i]->GetName());
        for(j = 0; j < num; j++)
        {
            output.AppendFmt("%*.2f ", (int)csMax((size_t)7, strlen(actors[j]->GetName())), actors[i]->GetRelativeFaction(actors[j]));
        }
        CPrintf(CON_CMDOUTPUT ,"%s\n",output.GetDataSafe());
    }

    csHash<Faction*,int> factions_by_id = psserver->GetCacheManager()->GetFactionHash();
    csHash<Faction*, int>::GlobalIterator iter = factions_by_id.GetIterator();
    {
        csString output;
        output.AppendFmt("                     ");
        while(iter.HasNext())
        {
            Faction* faction = iter.Next();
            output.AppendFmt("%15s ",faction->name.GetData());
        }
        CPrintf(CON_CMDOUTPUT ,"%s\n",output.GetDataSafe());
    }

    for(j = 0; j < num; j++)
    {
        csString output;
        output.AppendFmt("%20s",actors[j]->GetName());
        csHash<Faction*, int>::GlobalIterator iter = factions_by_id.GetIterator();
        while(iter.HasNext())
        {
            Faction* faction = iter.Next();
            FactionSet* factionSet = actors[j]->GetCharacterData()->GetFactions();
            int standing = 0;
            float weight = 0.0;
            factionSet->GetFactionStanding(faction->id,standing,weight);
            output.AppendFmt(" %7d %7.2f",standing,weight);
        }
        CPrintf(CON_CMDOUTPUT ,"%s\n",output.GetDataSafe());
    }


    return 0;
}


int com_sectors(const char*)
{
    csRef<iEngine> engine = csQueryRegistry<iEngine> (psserver->GetObjectReg());
    csRef<iSectorList> sectorList = engine->GetSectors();
    csRef<iCollectionArray> collections = engine->GetCollections();
    for(int i = 0; i < sectorList->GetCount(); i++)
    {
        iSector* sector = sectorList->Get(i);
        csString sectorName = sector->QueryObject()->GetName();
        psSectorInfo* si = psserver->GetCacheManager()->GetSectorInfoByName(sectorName);


        CPrintf(CON_CMDOUTPUT ,"%4i %4u %s",i,si?si->uid:0,sectorName.GetDataSafe());

        for(size_t r = 0; r < collections->GetSize(); r++)
        {
            if(collections->Get(r)->FindSector(sector->QueryObject()->GetName()))
            {

                CPrintf(CON_CMDOUTPUT ," %s", collections->Get(r)->QueryObject()->GetName());
            }

        }
        CPrintf(CON_CMDOUTPUT ,"\n");

    }
    return 0;
}

int com_showlogs(const char* line)
{
    pslog::DisplayFlags(*line?line:NULL);
    return 0;
}

int com_setlog(const char* line)
{
    if(!*line)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify: <log> <true/false> <filter_id>\n");
        CPrintf(CON_CMDOUTPUT, "            or: all <true/false> \n");
        return 0;
    }
    WordArray words(line);
    csString log(words[0]);
    csString flagword(words[1]);
    csString filter(words[2]);

    bool flag;
    if(flagword.IsEmpty() || tolower(flagword.GetAt(0)) == 't' ||
            tolower(flagword.GetAt(0)) == 'y' || flagword.GetAt(0) == '1')
    {
        flag=true;
    }
    else
    {
        flag=false;
    }

    uint32 filter_id=0;
    if(filter && !filter.IsEmpty())
    {
        filter_id=atoi(filter.GetDataSafe());
    }

    pslog::SetFlag(log, flag, filter_id);

    return 0;
}

int com_adjuststat(const char* line)
{
    CPrintf(CON_CMDOUTPUT, "No longer implemented, sorry.");
#if 0
    if(!line || !strcmp(line,"") || !strcmp((const char*)line,"help"))
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify: PlayerName StatValue AdjustMent.\n"
                "Where StatValue can be one of following:\n"
                "HP: HP HP_max HP_rate\n"
                "Mana: Mana: Mana_max Mana_rate\n"
                "Physical Stamina: Pstamina(Psta) Pstamina(Psta)_max Pstamina(Psta)_rate\n"
                "Mental Stamina: Mstamina(Msta) Mstamina(Msta)_max Mstamina(Msta)_rate");
        return 0;
    }

    WordArray words(line);
    csString charname(words[0]);
    csString statValue(words[1]);
    float adjust = atof(words[2]);
    int clientnum = atoi(words[0]);

    csHash<gemObject*, EID> &gems = psserver->entitymanager->GetGEM()->GetAllGEMS();
    csHash<gemObject*, EID>::GlobalIterator i(gems.GetIterator());
    gemActor* actor = NULL;
    bool found = false;

    if(clientnum != 0)
    {
        Client* client = psserver->GetNetManager()->GetClient(clientnum);
        if(!client)
        {
            CPrintf(CON_CMDOUTPUT ,"Couldn't find client %d!\n",clientnum);
            return 0;
        }
        actor = client->GetActor();
        if(!actor)
        {
            CPrintf(CON_CMDOUTPUT ,"Found client, but not an actor!\n");
            return 0;
        }

        found = true;

    }
    else
    {
        while(i.HasNext())
        {
            gemObject* obj = i.Next();

            actor = dynamic_cast<gemActor*>(obj);
            if(
                actor &&
                actor->GetCharacterData() &&
                !strcmp(actor->GetCharacterData()->GetCharName(),charname.GetData())
            )
            {
                found = true;
                break;
            }
        }
    }

    // Fail safe
    if(!actor)
        return 0;

    psCharacter* charData = actor->GetCharacterData();
    if(charData)
    {
        statValue.Downcase();
        float newValue = 0.0;
        if(statValue == "hp" || statValue=="hitpoints")
        {
            newValue = charData->AdjustHitPoints(adjust);
        }
        else if(statValue == "hp_max" || statValue=="hitpoints_max")
        {
            newValue = charData->AdjustHitPointsMax(adjust);
        }
        else if(statValue == "hp_rate" || statValue=="hitpoints_rate")
        {
            newValue = charData->AdjustHitPointsRate(adjust);
        }
        else if(statValue == "mana")
        {
            newValue = charData->AdjustMana(adjust);
        }
        else if(statValue == "mana_max")
        {
            newValue = charData->AdjustManaMax(adjust);
        }
        else if(statValue == "mana_rate")
        {
            newValue = charData->AdjustManaRate(adjust);
        }
        else if(statValue == "pstamina" || statValue == "psta")
        {
            newValue = charData->AdjustStamina(adjust,true);
        }
        else if(statValue == "pstamina_max" || statValue == "psta_max")
        {
            newValue = charData->AdjustStaminaMax(adjust,true);
        }
        else if(statValue == "pstamina_rate" || statValue == "psta_rate")
        {
            newValue = charData->AdjustStaminaRate(adjust,true);
        }
        else if(statValue == "mstamina" || statValue == "msta")
        {
            newValue = charData->AdjustStamina(adjust,false);
        }
        else if(statValue == "mstamina_max" || statValue == "msta_max")
        {
            newValue = charData->AdjustStaminaMax(adjust,false);
        }
        else if(statValue == "mstamina_rate" || statValue == "msta_rate")
        {
            newValue = charData->AdjustStaminaRate(adjust,false);
        }
        else
        {
            CPrintf(CON_CMDOUTPUT ,"Unkown statValue %s\n",statValue.GetData());
            return 0;
        }
        CPrintf(CON_CMDOUTPUT ,"Adjusted %s: %s with %.2f to %.2f\n",
                charname.GetData(),statValue.GetData(),adjust,newValue);
        return 0;
    }
#endif
    return 0;
}

int com_liststats(const char* line)
{
    if(!line)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify: PlayerName");
        return 0;
    }
    PID characteruid = psserver->CharacterLoader.FindCharacterID(line, false);
    if(!characteruid.IsValid())
    {
        CPrintf(CON_CMDOUTPUT ,"Character name is not found.\n");
        return 0;
    }

    // If the character is online use the active stats.  Otherwise we need to load the character data.
    Client* client = psserver->GetNetManager()->GetConnections()->Find(line);

    // When the instance of this struct is removed as soon as function
    // exits it will delete the temporary chardata.
    struct AutoRemove
    {
        bool temploaded;
        psCharacter* chardata;
        AutoRemove() : temploaded(false), chardata(NULL)
        {
        }
        ~AutoRemove()
        {
            if(temploaded)
            {
                delete chardata;
            }
        }
    };
    AutoRemove chardata_keeper;

    if(!client)
    {
        // Character is not online
        chardata_keeper.chardata=psserver->CharacterLoader.LoadCharacterData(characteruid,true);
        chardata_keeper.temploaded=true;
    }
    else
        chardata_keeper.chardata=client->GetCharacterData();

    if(chardata_keeper.chardata==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not get character data for specified character.\n");
        return 0;
    }

    psCharacter* charData = chardata_keeper.chardata;
    CPrintf(CON_CMDOUTPUT ,"\nStats for player %s\n",charData->GetCharName());
    CPrintf(CON_CMDOUTPUT     ,"Stat            Current     Max           Rate\n");
    {
        // Only show these stats if character is really loaded.
        CPrintf(CON_CMDOUTPUT ,"HP              %7.1f %7.1f(%7.1f) %7.1f(%7.1f)\n", charData->GetHP(),
                charData->GetMaxHP().Base(), charData->GetMaxHP().Current(), charData->GetHPRate().Base(), charData->GetHPRate().Current());
        CPrintf(CON_CMDOUTPUT ,"Mana            %7.1f %7.1f(%7.1f) %7.1f(%7.1f)\n", charData->GetMana(),
                charData->GetMaxMana().Base(), charData->GetMaxMana().Current(), charData->GetManaRate().Base(), charData->GetManaRate().Current());
        CPrintf(CON_CMDOUTPUT ,"Physical Stamina%7.1f %7.1f(%7.1f) %7.1f(%7.1f)\n", charData->GetStamina(true),
                charData->GetMaxPStamina().Base(), charData->GetMaxPStamina().Current(), charData->GetPStaminaRate().Base(), charData->GetPStaminaRate().Current());
        CPrintf(CON_CMDOUTPUT ,"Mental Stamina  %7.1f %7.1f(%7.1f) %7.1f(%7.1f)\n",charData->GetStamina(false),
                charData->GetMaxMStamina().Base(), charData->GetMaxMStamina().Current(), charData->GetMStaminaRate().Base(), charData->GetMStaminaRate().Current());
    }

    MathEnvironment skillVal;
    MathEnvironment baseSkillVal;
    charData->GetSkillValues(&skillVal);
    charData->GetSkillBaseValues(&baseSkillVal);

    CPrintf(CON_CMDOUTPUT ,"Stat        Base        Buffed\n");
    {
        CPrintf(CON_CMDOUTPUT ,"STR        %7.1f\t%7.1f\n", baseSkillVal.Lookup("STR")->GetValue(), skillVal.Lookup("STR")->GetValue());
        CPrintf(CON_CMDOUTPUT ,"AGI        %7.1f\t%7.1f\n", baseSkillVal.Lookup("AGI")->GetValue(), skillVal.Lookup("AGI")->GetValue());
        CPrintf(CON_CMDOUTPUT ,"END        %7.1f\t%7.1f\n", baseSkillVal.Lookup("END")->GetValue(), skillVal.Lookup("END")->GetValue());
        CPrintf(CON_CMDOUTPUT ,"INT        %7.1f\t%7.1f\n", baseSkillVal.Lookup("INT")->GetValue(), skillVal.Lookup("INT")->GetValue());
        CPrintf(CON_CMDOUTPUT ,"WIL        %7.1f\t%7.1f\n", baseSkillVal.Lookup("WIL")->GetValue(), skillVal.Lookup("WIL")->GetValue());
        CPrintf(CON_CMDOUTPUT ,"CHA        %7.1f\t%7.1f\n", baseSkillVal.Lookup("CHA")->GetValue(), skillVal.Lookup("CHA")->GetValue());
    }

    CPrintf(CON_CMDOUTPUT ,"Experience points(W)  %7u\n",charData->GetExperiencePoints());
    CPrintf(CON_CMDOUTPUT ,"Progression points(X) %7u\n",charData->GetProgressionPoints());
    CPrintf(CON_CMDOUTPUT ,"%-20s %12s %12s %12s\n","Skill","Practice(Z)","Knowledge(Y)","Rank(R)");
    for(unsigned int skillID = 0; skillID < psserver->GetCacheManager()->GetSkillAmount(); skillID++)
    {
        psSkillInfo* info = psserver->GetCacheManager()->GetSkillByID(skillID);
        if(!info)
        {
            Error2("Can't find skill %d",skillID);
            continue;
        }

        unsigned int z = charData->Skills().GetSkillPractice(info->id);
        unsigned int y = charData->Skills().GetSkillKnowledge(info->id);
        unsigned int rank = charData->Skills().GetSkillRank(info->id).Current();


        if(z == 0 && y == 0 && rank == 0)
            continue;

        CPrintf(CON_CMDOUTPUT ,"%-20s %12u %12u %12u \n",info->name.GetData(),
                z, y, rank);
    }
    return 0;
}


int com_progress(const char* line)
{
    csStringArray words(line,",");

    if(words.GetSize() < 2)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify: <player>, <event>\n");

        return 0;
    }

    const char* event = words[1];
    if(!event)
    {
        CPrintf(CON_CMDOUTPUT ,"Please specify: <player>, <event>\n");
        return 0;
    }
    const char* charname = words[0];

    // Convert to int, if possible
    int clientnum = atoi(charname);

    csHash<gemObject*, EID> &gems = psserver->entitymanager->GetGEM()->GetAllGEMS();
    csHash<gemObject*, EID>::GlobalIterator i(gems.GetIterator());
    gemObject* obj;
    gemActor* actor = NULL;
    bool found = false;

    if(clientnum == 0)
    {
        while(i.HasNext())
        {
            obj = i.Next();

            if(!strcasecmp(obj->GetName(),charname))
            {
                actor = dynamic_cast<gemActor*>(obj);
                found = true;
                break;
            }
        }
    }
    else
    {
        Client* client = psserver->GetNetManager()->GetClient(clientnum);
        if(!client)
        {
            CPrintf(CON_CMDOUTPUT ,"Player %d not found!\n",clientnum);
            return 0;
        }

        actor = client->GetActor();
        if(!actor)
        {
            CPrintf(CON_CMDOUTPUT ,"Player %s found, but without actor object!\n",client->GetName());
            return 0;
        }

        found = true;
    }

    // Did we find the player?
    if(!found)
    {
        CPrintf(CON_CMDOUTPUT ,"Player %s not found!\n",charname);
        return 0;
    }


    if(actor!=NULL)
    {
        // Find script
        ProgressionScript* script = psserver->GetProgressionManager()->FindScript(event);
        if(!script)
        {
            CPrintf(CON_CMDOUTPUT, "Progression script \"%s\" not found.", event);
            return 0;
        }

        // We don't know what the script expects the actor to be called, so define...basically everything.
        MathEnvironment env;
        env.Define("Actor",  actor);
        env.Define("Caster", actor);
        env.Define("NPC",    actor);
        env.Define("Target", actor);
        script->Run(&env);
        CPrintf(CON_CMDOUTPUT, "Script Executed\n");
        return 0;
    }

    return 0;
}


/** Kills a player right away
 */
int com_kill(const char* player)
{
    int clientNum = atoi(player);
    Client* client = psserver->GetNetManager()->GetConnections()->Find(clientNum);
    if(!client)
    {
        CPrintf(CON_CMDOUTPUT ,"Client %d not found!\n",clientNum);
        return 0;
    }
    EID eid = client->GetActor()->GetEID();
    gemActor* object = (gemActor*)psserver->entitymanager->GetGEM()->FindObject(eid);
    object->Kill(NULL);
    return 0;
}

/** Kills a npc right away
 */
int com_killnpc(const char* input)
{
    EID eid = atoi(input);
    gemActor* object = dynamic_cast<gemActor*>(psserver->entitymanager->GetGEM()->FindObject(eid));
    if(!object)
    {
        CPrintf(CON_CMDOUTPUT, "NPC with %s not found!\n", ShowID(eid));
        return 0;
    }
    object->Kill(NULL);
    return 0;
}

int com_motd(const char* str)
{
    if(!strcmp(str,""))
    {
        CPrintf(CON_CMDOUTPUT ,"MOTD: %s\n",psserver->GetMOTD());
    }
    else
    {
        psserver->SetMOTD((const char*)str);
    }
    return 0;
}

int com_questreward(const char* str)
{
    csString cmd(str);
    WordArray words(cmd);

    csString charactername = words[0];
    csString item   = words[1];

    if(charactername.IsEmpty() || item.IsEmpty())
    {
        CPrintf(CON_CMDOUTPUT ,"Both char name and item number should be specified.\n");
        return 0;
    }

    PID characteruid = psserver->CharacterLoader.FindCharacterID(charactername.GetData());
    if(!characteruid.IsValid())
    {
        CPrintf(CON_CMDOUTPUT ,"Character name not found.\n");
        return 0;
    }

    struct QuestRewardItem reward;

    // Get the ItemStats based on the name provided.
    reward.itemstat = psserver->GetCacheManager()->GetBasicItemStatsByID(atoi(item.GetData()));
    if(reward.itemstat==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"No Basic Item Template with that id was found.\n");
        return 0;
    }

    reward.count = 1;
    reward.quality = 0.0;
    if(words.GetCount() > 2)
    {
        reward.count = atoi(words[2]);
    }
    if(words.GetCount() > 3)
    {
        reward.quality = atof(words[3]);
    }

    // If the character is online, update the stats live.  Otherwise we need to load the character data to add this item to
    //  an appropriate inventory slot.
    Client* client = psserver->GetNetManager()->GetConnections()->Find(charactername.GetData());
    csArray<QuestRewardItem> items;
    items.Push(reward);

    if(!client)
    {
        // Character is not online
        return 0;
    }

    psCharacter* chardata=client->GetCharacterData();
    if(chardata==NULL)
    {
        CPrintf(CON_CMDOUTPUT ,"Could not get character data for specified character.\n");
        return 0;
    }

    csTicks timeDelay=0;
    psserver->questmanager->OfferRewardsToPlayer(client, items, timeDelay);
    return 0;
}


int com_transactions(const char* str)
{
    csString cmd(str);

    if(!cmd.Length())
    {
        CPrintf(CON_CMDOUTPUT,"Transaction actions:\n");
        CPrintf(CON_CMDOUTPUT,"DUMP <TIMESTAMP FROM> <TIMESTAMP TO> - Dumps the transaction history to the console\n");
        CPrintf(CON_CMDOUTPUT,"ERASE - Empties the transaction history\n");
        return 0;
    }

    cmd.Upcase();
    WordArray words(cmd);
    EconomyManager* economy = psserver->GetEconomyManager();

    if(words[0] == "DUMP")
    {
        for(unsigned int i = 0; i< economy->GetTotalTransactions(); i++)
        {
            TransactionEntity* trans = economy->GetTransaction(i);
            if(trans)
            {
                // Dump it
                CPrintf(
                    CON_CMDOUTPUT,
                    "%s transaction for %d %d (Quality %d) (%s => %s) with price %u/ (%d)\n",
                    trans->moneyIn?"Selling":" Buying",
                    trans->count,
                    trans->item,
                    trans->quality,
                    ShowID(trans->from),
                    ShowID(trans->to),
                    trans->price,
                    trans->stamp);
            }
        }
    }
    else if(words[0] == "ERASE")
    {
        economy->ClearTransactions();
        CPrintf(CON_CMDOUTPUT,"Cleared transactions\n");
    }
    else
        CPrintf(CON_CMDOUTPUT,"Unknown action\n");

    return 0;
}

int com_allocations(const char* str)
{
    CS::Debug::DumpAllocateMemoryBlocks();
    CPrintf(CON_CMDOUTPUT,"Dumped.\n");

    return 0;
}

int com_lschannel(const char*)
{
    CPrintf(CON_CMDOUTPUT, "%s", psserver->GetChatManager()->channelsToString().GetDataSafe());

    return 0;
}

int com_randomloot(const char* loot)
{
    if(strlen(loot) == 0)
    {
        CPrintf(CON_CMDOUTPUT, "Error. Syntax = randomloot \"<item name>\" <#modifiers: 0-3>\n");
        return 0;
    }

    WordArray words(loot, false);
    csString baseItemName = words[0];
    int numModifiers = atoi(words[1]);

    if(numModifiers < 0 || numModifiers > 3)
    {
        numModifiers = 0;
        CPrintf(CON_CMDOUTPUT, "Number of modifiers out of range 0-3. Default = 0\n");
    }

    LootEntrySet* testLootEntrySet =
        new LootEntrySet(1, psserver->GetSpawnManager()->GetLootRandomizer());
    LootEntry* testEntry = new LootEntry;
    if(testLootEntrySet && testEntry)
    {
        // get the base item stats
        testEntry->item = psserver->GetCacheManager()->GetBasicItemStatsByName(baseItemName);
        if(testEntry->item)
        {
            testEntry->probability = 1.0;   // want a dead cert for testing!
            testEntry->min_money = 0;       // ignore money
            testEntry->max_money = 0;
            testEntry->randomize = true;
            testEntry->min_item = 1;
            testEntry->max_item = 1;
            testEntry->randomizeProbability = 1.0;

            // add loot entry into set
            testLootEntrySet->AddLootEntry(testEntry);

            // generate loot from base item
            testLootEntrySet->CreateLoot(NULL, numModifiers);
        }
        else
        {
            CPrintf(CON_CMDOUTPUT, "\'%s\' not found.\n",
                    baseItemName.GetDataSafe());
            delete testEntry;
        }

        delete testLootEntrySet;
    }
    else
    {
        CPrintf(CON_CMDOUTPUT, "Could not create LootEntrySet/LootEntry instance.\n");

        delete testLootEntrySet;
        delete testEntry;
    }

    return 0;
}

int com_list(const char* arg)
{
    WordArray words(arg,false);

    if(words.GetCount() == 0)
    {
        CPrintf(CON_CMDOUTPUT,"Syntax: list [char|ent|path|race|warpspace|waypoint] <pattern|EID>\n");
        return 0;
    }

    // Compare all strings up to the point that is needed to uniq identify them.
    if(strncasecmp(words[0],"char",1) == 0)
    {
        com_charlist(words[1]);
    }
    else if(strncasecmp(words[0],"ent",1) == 0)
    {
        com_entlist(words[1]);
    }
    else if(strncasecmp(words[0],"path",1) == 0)
    {
        psserver->GetAdminManager()->GetPathNetwork()->ListPaths(words[1]);
    }
    else if(strncasecmp(words[0],"race",1) == 0)
    {
        com_racelist(words[1]);
    }
    else if(strncasecmp(words[0],"warpspace",3) == 0)
    {
        EntityManager::GetSingleton().GetWorld()->DumpWarpCache();
    }
    else if((strncasecmp(words[0],"waypoint",3) == 0) || strncasecmp(words[0],"wp",2) == 0)
    {
        psserver->GetAdminManager()->GetPathNetwork()->ListWaypoints(words[1]);
    }

    return 0;
}


/** List of commands available at the console.
 *
 * Add new commands that should be avalble at the console here.
 * 1st parameter is the name.
 * 2nd parameter is allowRemote, that when true would allow this command to be executed
 *               from a remote console.
 * 3rd parameter is the function pointer to the callback function that will be called
 *               when the user acctivate the function.
 * 4th parameter is a description of the command.
 *
 * Make sure the last entry contain 0 for all entries to terminate the list.
 */
const COMMAND commands[] =
{

    // Server commands
    { "-- Server commands",  true, NULL, "------------------------------------------------" },
    { "dbprofile",  true, com_dbprofile, "shows database profile info" },
    { "exec",      true, com_exec,      "Executes a script file" },
    { "help",      true, com_help,      "Show help information" },
    { "kick",      true, com_kick,      "Kick player from the server"},
    { "queue",     true, com_queue,      "Get the size of a player queue"},
    { "loadmap",   true, com_loadmap,   "Loads a map into the server"},
    { "lock",      false, com_lock,      "Tells server to stop accepting connections"},
    { "maplist",   true, com_maplist,   "List all mounted maps"},
    { "dumpwarpspace",   true, com_dumpwarpspace,   "Dump the warp space table"},
    { "netprofile", true, com_netprofile, "shows network profile info" },
    { "quit",      true, com_quit,      "[minutes] Makes the server exit immediately or after the specified amount of minutes"},
    { "ready",     false, com_ready,     "Tells server to start accepting connections"},
    { "sectors",   true, com_sectors,   "Display all sectors" },
    { "set",       true, com_set,       "Sets a server variable"},
    { "setlog",    true, com_setlog,    "Set server log" },
    { "setmaxfile",false, com_setmaxfile,"Set maximum message class for output file"},
    { "setmaxout", false, com_setmaxout, "Set maximum message class for standard output"},
    { "settime",   true, com_settime,    "Sets the current server hour using a 24 hour clock" },
    { "showtime",  true, com_showtime,   "Show the current time" },
    { "showlogs",  true, com_showlogs,  "Show server logs" },
    { "spawn",     false, com_spawn,     "Loads npcs, items, action locations, hunt locations in the server"},
    { "status",    true, com_status,    "Show server status"},
    { "transactions", false, com_transactions, "Performs an action on the transaction history (run without parameters for options)" },
    { "dumpallocations", true, com_allocations, "Dump all allocations to allocations.txt if CS extensive memdebug is enabled" },

    // npc commands
    { "-- NPC commands",  true, NULL, "------------------------------------------------" },
    { "exportdialogs", true, com_exportdialogs, "Loads NPC dialogs from the DB and stores them in a XML file"},
    { "exportnpc", true, com_exportnpc, "Loads NPC data from the DB and stores it in a XML file"},
    { "importdialogs", true, com_importdialogs, "Loads NPC dialogs from a XML file or a directory and inserts them into the DB"},
    { "importnpc", true, com_importnpc, "Loads NPC data from a XML file or a directory and inserts it into the DB"},
    { "loadnpc",   true, com_loadnpc,   "Loads/Reloads an NPC from the DB into the world"},
    { "loadquest", true, com_loadquest, "Loads/Reloads a quest from the DB into the world"},
    { "newacct",   true, com_newacct,   "Create a new account: newacct <user/passwd[/security level]>" },
//  { "newguild",  com_newguild,  "Create a new guild: newguild <name/leader>" },
//  { "joinguild", com_joinguild, "Attach player to guild: joinguild <guild/player>" },
//  { "quitguild", com_quitguild, "Detach player from guild: quitguild <player>" },

    // player-entities commands

    { "-- Player commands",  true, NULL, "------------------------------------------------" },
    { "addinv",    true, com_addinv,    "Add an item to a player's inventory" },
    { "adjuststat",true, com_adjuststat,"Adjust player stat [HP|HP_max|HP_rate|Mana|Mana_max|Mana_rate|Stamina|Stamina_max|Stamina_rate]" },
    { "bulkdelete",true, com_bulkdelete,"Delete a list of characters specified by character ID's in the given file" },
    { "charlist",  true, com_charlist,  "List all known characters" },
    { "delete",    false, com_delete,    "Delete a player from the database"},
    { "dict",      true, com_dict,      "Dump the NPC dictionary"},
    { "kill",      true, com_kill,      "kill <playerID> Kills a player" },
    { "killnpc",   true, com_killnpc,   "killnpc <eid> Kills a npc" },
    { "progress",  true, com_progress,  "progress <player>,<event/script>" },
    { "questreward", true, com_questreward, "Preforms the same action as when a player gets a quest reward" },
    { "say",       true, com_say,       "Tell something to all players connected"},
    { "gossipsay", true, com_sayGossip, "Tell something to all players in the main public channel"},
    { "showinv",   true, com_showinv,   "Show items in a player's inventory" },
    { "showinvf",  true, com_showinvf,  "Show items in a player's inventory (more item information)" },

    // various commands
    { "-- Various commands",  true, NULL, "------------------------------------------------" },
    { "list",      true, com_list,      "List entites ( list [char|ent|path|waypoint] <filter> )" },
    { "entlist",   true, com_entlist,   "List all known entities" },
    { "factions",  true, com_factions,  "Display factions" },
    { "filtermsg", true, com_filtermsg, "Add or remove messages from the LOG_MESSAGE log"},
    { "liststats", true, com_liststats, "List player stats" },
    { "motd",      true, com_motd,      "motd <msg> Sets the MOTD" },
    { "print",     true, com_print,     "Displays debug data about a specified entity" },
    { "rain",      true, com_rain,      "Forces it to start or stop raining in a sector"},
    { "lschannel", true, com_lschannel, "Lists all the channels in the server"},
    { "randomloot",false,com_randomloot,"Generates random loot"},
    { 0, 0, 0, 0 }
};


