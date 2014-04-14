/*
 * autoexec.cpp - Author: Fabian Stock (Aiwendil)
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#define AUTOEXEC_FILE "/planeshift/userdata/options/autoexec.xml"
#define DEFAULT_AUTOEXEC_FILE "/planeshift/data/options/autoexec_def.xml"
#define AUTOEXECROOTNODE "autoexec"
#define AUTOEXECOPTIONSNODE "options"
#define AUTOEXECCOMMANDSNODE "commands"

#include "globals.h"
#include <psconfig.h>

// Library Includes
#include "paws/pawsmanager.h"
#include "net/cmdhandler.h"


#include "autoexec.h"

Autoexec::Autoexec() : enabled(false)
{
      csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());

    // Check if there have been created a custom file
    // else use the default file.
    csString fileName = AUTOEXEC_FILE;
    if (!vfs->Exists(fileName))
    {
      fileName = DEFAULT_AUTOEXEC_FILE;
    }
    LoadCommands(fileName);
}

Autoexec::~Autoexec()
{
    SaveCommands();
}

void Autoexec::execute()
{
    // first execute the general commands not assigned to a char name
    for (size_t i = 0; i < cmds.GetSize(); i++)
    {
        if (!cmds[i].name)
        {
          if (cmds[i].cmd && cmds[i].cmd != "")
          {
            psengine->GetCmdHandler()->Execute(cmds[i].cmd, false);
            //printf("executing general commands: %s\n", cmds[i].cmd.GetData());
          }
        }
    }
    // now the commands for the current char
    csString charname = psengine->GetMainPlayerName();
    for (size_t i = 0; i < cmds.GetSize(); i++)
    {
        if (cmds[i].name == charname)
        {
            if (cmds[i].cmd && cmds[i].cmd != "")
            {
                psengine->GetCmdHandler()->Execute(cmds[i].cmd, false);
                //printf("executing commands for %s : %s\n", cmds[i].name.GetData(), cmds[i].cmd.GetData());
                return; // No need to look for a second char name match
            }
        }
    }
}

csString Autoexec::getCommands(csString name)
{
    for (size_t i = 0; i < cmds.GetSize(); i++)
    {
       if (cmds[i].name == name)
       {
           return cmds[i].cmd;
       }
    }
    return "";
}

void Autoexec::LoadCommands(const char * fileName)
{
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDocumentNode> root, autoexecNode, optionsNode;

    csRef<iDocument> doc = psengine->GetXMLParser()->CreateDocument();
    csRef<iDataBuffer> buf (vfs->ReadFile (fileName));
    if (!buf || !buf->GetSize ())
    {
        return ;
    }
    const char* error = doc->Parse( buf );
    if ( error )
    {
        Error2("Error loading shortcut window commands: %s\n", error);
        return ;
    }

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("%s has no XML root", fileName);
    }
    autoexecNode = root->GetNode(AUTOEXECROOTNODE);
    if (autoexecNode == NULL)
    {
        Error2("%s has no <autoexec> tag", fileName);
    }
    optionsNode = autoexecNode->GetNode(AUTOEXECOPTIONSNODE);
    if (optionsNode != NULL)
    {
        enabled = optionsNode->GetAttributeValueAsBool("enable", false);
    }
    else
    {
        Error2("%s has no <options> tag", fileName);
    }
    csRef<iDocumentNodeIterator> oNodes = autoexecNode->GetNodes(AUTOEXECCOMMANDSNODE);
    while (oNodes->HasNext())
    {
        csRef<iDocumentNode> commands = oNodes->Next();
        csString charname = commands->GetAttributeValue("charname");
        csString cmds = commands->GetAttributeValue("execute");
        addCommand(charname, cmds);
    }
}

void Autoexec::SaveCommands()
{
    // no need to save if there are no autoexec cammands
    if (cmds.GetSize() == 0)
    {
        return;
    }
    // Save the autoexec commands
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot ();
    csRef<iDocumentNode> mainNode = root->CreateNodeBefore(CS_NODE_ELEMENT);
    mainNode->SetValue(AUTOEXECROOTNODE);

    csRef<iDocumentNode> optionsNode = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
    optionsNode->SetValue(AUTOEXECOPTIONSNODE);
    optionsNode->SetAttributeAsInt("enable", enabled);

    for (size_t i = 0; i < cmds.GetSize(); i++)
    {
        // only save if there is anything to execute.
        if (cmds[i].cmd && cmds[i].cmd != "")
        {
            csRef<iDocumentNode> commandsNode = mainNode->CreateNodeBefore(CS_NODE_ELEMENT);
            commandsNode->SetValue(AUTOEXECCOMMANDSNODE);
            //check if the commands are attached to a char name
            if (cmds[i].name)
            {
                commandsNode->SetAttribute("charname", cmds[i].name);
            }
            commandsNode->SetAttribute("execute", cmds[i].cmd);
        }
    }

    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    doc->Write(vfs, AUTOEXEC_FILE);
}

void Autoexec::LoadDefault()
{
    LoadCommands(DEFAULT_AUTOEXEC_FILE);
}


void Autoexec::addCommand(csString name, csString cmd)
{
    // add only the latest commands for a name, discard any previous commands for the same name.
    for (size_t i = 0; i < cmds.GetSize(); i++)
    {
        if (cmds[i].name == name)
        {
            cmds[i].cmd = cmd;
            return;
        }
    }
    // no autoexec cammands for this name yet, so create some.
    AutoexecCommand addCommand;
    addCommand.name = name;
    addCommand.cmd = cmd;
    cmds.Push(addCommand);
}

