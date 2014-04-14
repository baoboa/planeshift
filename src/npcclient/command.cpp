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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>
#include <iutil/vfs.h>
#include <iengine/engine.h>
#include <csutil/csstring.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/command.h"
#include "util/serverconsole.h"
#include "util/log.h"
#include "util/location.h"
#include "util/strutil.h"
#include "util/eventmanager.h"

#include "engine/psworld.h"

#include "net/connection.h"

#include "npcclient.h"
#include "npc.h"
#include "networkmgr.h"
#include "globals.h"


/* shut down the server and exit program */
int com_quit(const char*)
{
    npcclient->Disconnect();

    npcclient->GetEventManager()->Stop();

    return 0;
}

int com_disable(const char* arg)
{
    npcclient->EnableDisableNPCs(arg,false);
    return 0;
}

int com_enable(const char* arg)
{
    npcclient->EnableDisableNPCs(arg,true);
    return 0;
}

int com_locationtest(const char* line)
{
    WordArray words(line,false);

    if(words.GetCount() < 5)
    {
        CPrintf(CON_CMDOUTPUT,"Syntax: loctest loc sector x y z\n");
        return 0;
    }

    LocationType* loc = npcclient->FindRegion(words[0]);
    if(!loc)
    {
        CPrintf(CON_CMDOUTPUT,"Location '%s' not found\n",words[0].GetDataSafe());
        return 0;
    }

    iSector* sector = npcclient->GetEngine()->FindSector(words[1]);
    if(!sector)
    {
        CPrintf(CON_CMDOUTPUT,"Sector '%s' not found\n",words[1].GetDataSafe());
        return 0;
    }

    float x = atof(words[2]);
    float y = atof(words[3]);
    float z = atof(words[4]);

    csVector3 pos(x,y,z);


    if(loc->CheckWithinBounds(npcclient->GetEngine(),pos,sector))
    {
        CPrintf(CON_CMDOUTPUT,"Within loc\n");
    }
    else
    {
        CPrintf(CON_CMDOUTPUT,"Outside loc\n");
    }

    return 0;
}

/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int com_help(const char* arg)
{
    register int i;
    int printed = 0;

    CPrintf(CON_CMDOUTPUT, "\n");
    for(i = 0; commands[i].name; i++)
    {
        if(!*arg || (strcmp(arg, commands[i].name) == 0))
        {
            CPrintf(CON_CMDOUTPUT, "%-12s %s.\n", commands[i].name, commands[i].doc);
            printed++;
        }
    }

    if(!printed)
    {
        CPrintf(CON_CMDOUTPUT, "No commands match `%s'.  Possibilities are:\n", arg);

        for(i = 0; commands[i].name; i++)
        {
            /* Print in six columns. */
            if(printed == 6)
            {
                printed = 0;
                CPrintf(CON_CMDOUTPUT, "\n");
            }

            CPrintf(CON_CMDOUTPUT, "%s\t", commands[i].name);
            printed++;
        }

        if(printed)
            CPrintf(CON_CMDOUTPUT, "\n");
    }

    CPrintf(CON_CMDOUTPUT, "\n");

    return (0);
}

/* List entity given. */
int com_list(const char* arg)
{
    WordArray words(arg,false);

    if(words.GetCount() == 0 || strncasecmp(words[0],"help",1) == 0)
    {
        CPrintf(CON_CMDOUTPUT,"Syntax: list [char|ent|help|loc|npc|path|race|reactions|recipe|tribe|warpspace|waypoint] <pattern|EID>\n");
        CPrintf(CON_CMDOUTPUT,"Sub commands:\n");
        CPrintf(CON_CMDOUTPUT,"  char|npc [summary|stats|<pattern>]\n");
        return 0;
    }

    // Compare all strings up to the point that is needed to uniq identify them.
    if(strncasecmp(words[0],"char",1) == 0)
    {
        npcclient->ListAllEntities(words[1],true);
    }
    else if(strncasecmp(words[0],"ent",1) == 0)
    {
        npcclient->ListAllEntities(words[1],false);
    }
    else if(strncasecmp(words[0],"loc",1) == 0)
    {
        npcclient->ListLocations(words[1]);
    }
    else if(strncasecmp(words[0],"npc",1) == 0)
    {
        npcclient->ListAllNPCs(words[1]);
    }
    else if(strncasecmp(words[0],"path",1) == 0)
    {
        npcclient->ListPaths(words[1]);
    }
    else if(strncasecmp(words[0],"race",2) == 0)
    {
        if(!npcclient->DumpRace(words[1]))
        {
            CPrintf(CON_CMDOUTPUT, "No Race with id '%s' found.\n", words[1].GetDataSafe());
        }
    }
    else if(strncasecmp(words[0],"reactions",3) == 0)
    {
        npcclient->ListReactions(words[1]);
    }
    else if(strncasecmp(words[0],"recipe",3) == 0)
    {
        npcclient->ListTribeRecipes(words[1]);
    }
    else if(strncasecmp(words[0],"tribe",1) == 0)
    {
        npcclient->ListTribes(words[1]);
    }
    else if(strncasecmp(words[0],"warpspace",3) == 0)
    {
        npcclient->GetWorld()->DumpWarpCache();
    }
    else if((strncasecmp(words[0],"waypoint",3) == 0) || strncasecmp(words[0],"wp",2) == 0)
    {
        npcclient->ListWaypoints(words[1]);
    }
    else
    {
        CPrintf(CON_CMDOUTPUT,"%s not a know entity\n",words[0].GetDataSafe());
    }

    return (0);
}


int com_setmaxout(const char* arg)
{
    if(!arg || strlen(arg) == 0)
    {
        CPrintf(CON_CMDOUTPUT, "Use one of the following values to control output on standard output:\n");
        CPrintf(CON_CMDOUTPUT, "  1: only output of server commands\n");
        CPrintf(CON_CMDOUTPUT, "  2: 1 + bug\n");
        CPrintf(CON_CMDOUTPUT, "  3: 2 + errors\n");
        CPrintf(CON_CMDOUTPUT, "  4: 3 + warnings\n");
        CPrintf(CON_CMDOUTPUT, "  5: 4 + notifications\n");
        CPrintf(CON_CMDOUTPUT, "  6: 5 + debug messages\n");
        CPrintf(CON_CMDOUTPUT, "  7: 6 + spam\n");
        CPrintf(CON_CMDOUTPUT, "Current level: %d\n",ConsoleOut::GetMaximumOutputClassStdout());
        return 0;
    }
    int msg = CON_SPAM;;
    sscanf(arg, "%d", &msg);
    if(msg < CON_CMDOUTPUT) msg = CON_CMDOUTPUT;
    if(msg > CON_SPAM) msg = CON_SPAM;
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
        CPrintf(CON_CMDOUTPUT, "  2: 1 + bug\n");
        CPrintf(CON_CMDOUTPUT, "  3: 2 + errors\n");
        CPrintf(CON_CMDOUTPUT, "  4: 3 + warnings\n");
        CPrintf(CON_CMDOUTPUT, "  5: 4 + notifications\n");
        CPrintf(CON_CMDOUTPUT, "  6: 5 + debug messages\n");
        CPrintf(CON_CMDOUTPUT, "  7: 6 + spam\n");
        CPrintf(CON_CMDOUTPUT, "Current level: %d\n",ConsoleOut::GetMaximumOutputClassFile());
        return 0;
    }
    int msg = CON_SPAM;;
    sscanf(arg, "%d", &msg);
    if(msg < CON_NONE) msg = CON_NONE;
    if(msg > CON_SPAM) msg = CON_SPAM;
    ConsoleOut::SetMaximumOutputClassFile((ConsoleOutMsgClass)msg);
    return 0;
}

int com_showlogs(const char* line)
{
    pslog::DisplayFlags(*line?line:NULL);
    return 0;
}

int com_print(const char* line)
{
    if(!npcclient->DumpNPC(line))
    {
        CPrintf(CON_CMDOUTPUT, "No NPC with id '%s' found.\n", line);
    }

    return 0;
}

int com_info(const char* line)
{
    if(!npcclient->InfoNPC(line))
    {
        CPrintf(CON_CMDOUTPUT, "No NPC with id '%s' found.\n", line);
    }

    return 0;
}

int com_debugnpc(const char* line)
{
    WordArray words(line);

    if(!*line)
    {
        CPrintf(CON_CMDOUTPUT, "Please specify: <npc_id> [<log_level>]\n");
        return 0;
    }


    unsigned int id = atoi(words[0]);
    NPC* npc = npcclient->FindNPCByPID(id);
    if(!npc)
    {
        CPrintf(CON_CMDOUTPUT, "No NPC with id '%s' found.\n", words[0].GetDataSafe());
        return 0;
    }

    if(words.GetCount() >= 2)
    {
        int level = atoi(words[1]);
        npc->SetDebugging(level);
        CPrintf(CON_CMDOUTPUT, "Debugging for NPC %s set to %d.\n", npc->GetName(),level);
    }
    else
    {
        if(npc->SwitchDebugging())
        {
            CPrintf(CON_CMDOUTPUT, "Debugging for NPC %s switched ON.\n", npc->GetName());
        }
        else
        {
            CPrintf(CON_CMDOUTPUT, "Debugging for NPC %s switched OFF.\n", npc->GetName());
        }
    }

    return 0;
}

int com_debugtribe(const char* line)
{
    WordArray words(line);

    if(!*line)
    {
        CPrintf(CON_CMDOUTPUT, "Please specify: <tribe_id> [<log_level>]\n");
        return 0;
    }


    unsigned int id = atoi(words[0]);
    Tribe* tribe = npcclient->GetTribe(id);
    if(!tribe)
    {
        CPrintf(CON_CMDOUTPUT, "No Tribe with id '%s' found.\n", words[0].GetDataSafe());
        return 0;
    }

    if(words.GetCount() >= 2)
    {
        int level = atoi(words[1]);
        tribe->SetDebugging(level);
        CPrintf(CON_CMDOUTPUT, "Debugging for Tribe %s set to %d.\n", tribe->GetName(), level);
    }
    else
    {
        if(tribe->SwitchDebugging())
        {
            CPrintf(CON_CMDOUTPUT, "Debugging for Tribe %s switched ON.\n", tribe->GetName());
        }
        else
        {
            CPrintf(CON_CMDOUTPUT, "Debugging for Tribe %s switched OFF.\n", tribe->GetName());
        }
    }

    return 0;
}

int com_setbuffer(const char* line)
{
    WordArray words(line,false);

    if(!*line || words.GetCount() != 3)
    {
        CPrintf(CON_CMDOUTPUT, "Please specify: <npc_id> <buffer_name> <buffer_value>\n");
        return 0;
    }


    unsigned int id = atoi(words[0]);
    NPC* npc = npcclient->FindNPCByPID(id);
    if(!npc)
    {
        CPrintf(CON_CMDOUTPUT, "No NPC with id '%s' found.\n", words[0].GetDataSafe());
        return 0;
    }

    npc->SetBuffer(words[1],words[2]);

    CPrintf(CON_CMDOUTPUT, "Setting buffer %s to %s for NPC %s.\n", words[1].GetDataSafe(), words[2].GetDataSafe(), npc->GetName());

    return 0;
}

int com_setlog(const char* line)
{
    if(!*line)
    {
        CPrintf(CON_CMDOUTPUT, "Please specify: <log> <true/false> <filter_id>\n");
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

    npcclient->SaveLogSettings();

    return 0;
}

int com_filtermsg(const char* arg)
{

    CPrintf(CON_CMDOUTPUT ,"%s\n",npcclient->GetNetConnection()->LogMessageFilter(arg).GetDataSafe());

    return 0;
}

int com_fireperc(const char* arg)
{
    csStringArray arguments;
    arguments.SplitString(arg, " ");
    if(npcclient->FirePerception(atoi(arguments.Get(0)), arguments.Get(1)))
    {
        CPrintf(CON_CMDOUTPUT, "Perception %s fired on NPC %s.\n", arguments.Get(1), arguments.Get(0));
    }
    else
    {
        CPrintf(CON_CMDOUTPUT, "Could not fire perception. NPC ID: %s does not exist.\n", arguments.Get(0));
    }
    return 0;
}

int com_showtime(const char*)
{
    CPrintf(CON_CMDOUTPUT,"Game time is %d:%02d %d-%d-%d\n",
            npcclient->GetGameTODHour(),
            npcclient->GetGameTODMinute(),
            npcclient->GetGameTODYear(),
            npcclient->GetGameTODMonth(),
            npcclient->GetGameTODDay());
    csTicks now = csGetTicks();
    CPrintf(CON_CMDOUTPUT,"Tick is %u. Remaining %.5f days before csTicks rollover.\n",now,(float)(UINT_MAX-now)/(1000.0*60.0*60.0*24.0));
    return 0;
}


int com_status(const char*)
{
    CPrintf(CON_CMDOUTPUT,"Amount of loaded npcs: %d\n", npcclient->GetNpcListAmount());
    CPrintf(CON_CMDOUTPUT,"Tick counter         : %d\n", npcclient->GetTickCounter());
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
    { "debugnpc",     false, com_debugnpc,     "Switches the debug mode on 1 NPC"},
    { "debugtribe",   false, com_debugtribe,   "Switches the debug mode on 1 Tribe"},
    { "disable",      false, com_disable,      "Disable a enabled NPC. [all | pattern | EID]"},
    { "enable",       false, com_enable,       "Enable a disabled NPC. [all | pattern | EID]"},
    { "filtermsg",    true,  com_filtermsg,    "Add or remove messages from the LOG_MESSAGE log"},
    { "fireperc",     false, com_fireperc,     "Fire the given perception on the given npc. (fireperc [npcPID] [perception])"},
    { "help",         false, com_help,         "Show help information" },
    { "info",         false, com_info,         "Short print for 1 NPC"},
    { "list",         false, com_list,         "List entities ( list [char|ent|loc|npc|path|race|recipe|tribe|warpspace|waypoint] <filter> )" },
    { "print",        false, com_print,        "List all behaviors/hate of 1 NPC"},
    { "quit",         true,  com_quit,         "Makes the npc client exit"},
    { "setbuffer",    false, com_setbuffer,    "Set a npc buffer"},
    { "setlog",       false, com_setlog,       "Set server log" },
    { "setmaxfile",   false, com_setmaxfile,   "Set maximum message class for output file"},
    { "setmaxout",    false, com_setmaxout,    "Set maximum message class for standard output"},
    { "showlogs",     false, com_showlogs,     "Show server logs" },
    { "showtime",     false, com_showtime,     "Show the current game time"},
    { "status",       false, com_status,       "Give some general statistics on the npcclient"},
    { 0, 0, 0, 0 }
};


