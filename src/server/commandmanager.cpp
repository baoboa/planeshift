/*
 * commandmanager.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "commandmanager.h"
#include "globals.h"

psCommandManager::~psCommandManager()
{
    {
        csHash<CommandGroup*, int>::GlobalIterator it(commandGroups.GetIterator());
        while(it.HasNext())
        {
            CommandGroup* group = it.Next();
            delete group;
        }
    }
    {
        csHash<CommandList*, csString>::GlobalIterator it(commandLists.GetIterator());
        while(it.HasNext())
        {
            CommandList* list = it.Next();
            delete list;
        }
    }
}

bool psCommandManager::LoadFromDatabase()
{
    unsigned int currentrow;
    Result result(db->Select("SELECT * from command_groups"));

    if(!result.IsValid())
        return false;

    for(currentrow=0; currentrow< result.Count(); currentrow++)
    {

        CommandGroup* group = new CommandGroup;
        group->id = result[currentrow].GetUInt32("id");
        group->name = result[currentrow]["group_name"];

        Result commands(db->Select("SELECT * from command_group_assignment where group_member=%d", group->id));
        if(!commands.IsValid())
        {
            delete group;
            return false;
        }


        unsigned int commandCount;
        for(commandCount = 0; commandCount < commands.Count(); commandCount++)
        {
            group->commands.Push(commands[commandCount]["command_name"]);
        }

        commandGroups.Put(group->id, group);
    }

    // Organize the other list.
    csHash<CommandGroup*, int>::GlobalIterator iter = commandGroups.GetIterator();
    while(iter.HasNext())
    {
        CommandGroup* g = iter.Next();

        for(size_t z = 0; z < g->commands.GetSize(); z++)
        {
            csString command = g->commands[z];
            CommandList* list = commandLists.Get(command, NULL);
            // If null then not in list yet so we add
            if(list == NULL)
            {
                list = new CommandList;
                list->command = command;

                commandLists.Put(command, list);
            }

            list->groupMembers.Push(g->id);
        }
    }

    return true;
}

bool psCommandManager::GroupExists(int securityLevel)
{
    // Force every security level to legal range
    if(securityLevel > 30) securityLevel = 30;

    return commandGroups.Contains(securityLevel);
}

bool psCommandManager::Validate(int securityLevel, const char* command)
{
    if(!command)
        return false;

    // Force every security level to legal range
    if(securityLevel > 30) securityLevel = 30;

    CommandList* list = commandLists.Get(command, NULL);
    if(list)
        return list->groupMembers.Find(securityLevel) != csArrayItemNotFound;
    else
        return false;
}

bool psCommandManager::Validate(int securityLevel, const char* command, csString &error)
{
    if(!command)
    {
        error = "No command found";
        return false;
    }

    // Force every security level to legal range
    if(securityLevel > 30) securityLevel = 30;

    CommandList* list = commandLists.Get(command, NULL);
    if(list)
    {
        size_t find = list->groupMembers.Find(securityLevel);
        if(find == csArrayItemNotFound)
        {
            CommandGroup* grp = commandGroups.Get(securityLevel, NULL);
            csString groupName = "Unknown";
            if(grp)
                groupName = grp->name;

            error.Format("You do not have access to '%s' in your group '%s'", command, groupName.GetData());
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        error.Format("%s not found in any group!", command);
        return false;
    }
}

void psCommandManager::BuildXML(int securityLevel, csString &dest, bool subscribe)
{
    CommandGroup* grp = commandGroups.Get(securityLevel, NULL);
    if(!grp)
        return;

    dest.AppendFmt("<subscribe value='%s' />", subscribe ? "true":"false");

    for(size_t z = 0; z < grp->commands.GetSize(); z++)
    {
        csString cmd(grp->commands[z].GetData());
        if(cmd.GetAt(0) == '/')   // Make a list of the commands, but leave out the specials
            dest.AppendFmt("<command name='%s' />", cmd.GetData());
    }
}
