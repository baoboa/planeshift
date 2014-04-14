/*
 * commandmanager.h
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

#ifndef __COMMANDMANAGER_H__
#define __COMMANDMANAGER_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/hash.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================


/** Defines a command group.
  */
struct CommandGroup
{
    csString name; ///< The name of the group from the database.
    unsigned int id;        ///< Unique ID code from database.
    csArray<csString> commands;  ///< List of all the commands in this group.
};


/** Defines a command listing
  */
struct CommandList
{
    csString command;        ///< The command string
    csArray<int> groupMembers;  ///< List of all groups the command is in.
};


/** This class handles the permission system such as Game Masters and
    Role play masters.

    This system uses 2 database tables.  One that defines the groups
    and their names ( and their parent group for inherriting commands ).

    The second is a table that assigns commands to different groups.  On
    load it will create two lists.  One based on group number and one based on
    command.

    That way when a player logs in we can get a list of commands based on their
    group and send them a subscribe message.  We can also then do level checking
    when we receive a command from a client.
*/
class psCommandManager
{
public:
    virtual ~psCommandManager();

    bool LoadFromDatabase();
    bool GroupExists(int securityLevel);
    bool Validate(int securityLevel, const char* command);
    bool Validate(int securityLevel, const char* command, csString &error);
    void BuildXML(int securityLevel, csString &dest, bool subscribe = true);

private:

    csHash<CommandGroup*, int>     commandGroups;
    csHash<CommandList*, csString> commandLists;
};

#endif
