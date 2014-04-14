/*
* updater.cpp - Author: Mike Gist
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/syspath.h>

#include <csutil/syspath.h>
#include "updaterconfig.h"
#include "updaterengine.h"
#include "updater.h"

iObjectRegistry* psUpdater::object_reg = NULL;

psUpdater::psUpdater(int argc, char* argv[])
{
    object_reg = csInitializer::CreateEnvironment (argc, argv);
    if(!object_reg)
    {
        printf("Object Reg failed to Init!\n");
        exit(1);
    }
    // Request needed plugins.
    csInitializer::RequestPlugins(object_reg, CS_REQUEST_VFS, CS_REQUEST_END);

    // Mount the VFS paths.
    csRef<iVFS> vfs = csQueryRegistry<iVFS>(object_reg);
    if (!vfs->Mount ("/planeshift/", "$^"))
    {
        printf("Failed to mount /planeshift!\n");
        exit(1);
    }

    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (object_reg);
    csString configPath = csGetPlatformConfigPath("PlaneShift");
    configPath.ReplaceAll("/.crystalspace/", "/.");
    configPath = configManager->GetStr("PlaneShift.UserConfigPath", configPath);
    FileUtil fileUtil(vfs);
    csRef<FileStat> filestat = fileUtil.StatFile(configPath);
    if (!filestat.IsValid() && CS::Platform::CreateDirectory(configPath) < 0)
    {
        printf("Could not create required %s directory!\n", configPath.GetData());
        exit(1);
    }

    if (!vfs->Mount("/planeshift/userdata", configPath + "$/"))
    {
        printf("Could not mount %s as /planeshift/userdata!\n", configPath.GetData());
        exit(1);
    }
}

psUpdater::~psUpdater()
{
    csInitializer::DestroyApplication(object_reg);
}

void psUpdater::RunUpdate(UpdaterEngine* engine) const
{
    // Check if the updater is disabled because of third-party support.
    if(!engine->GetConfig()->IsUpdateEnabled())
        return;

    // Check if we're already in the middle of a self-update.
    if(engine->GetConfig()->IsSelfUpdating())
    {
        // Continue the self update, passing the update stage.
        if(engine->SelfUpdate(engine->GetConfig()->IsSelfUpdating()))
            return;
    }

    // Check for a mirror switch.
    if(engine->GetConfig()->SwitchMirror())
    {
        printf("Switching updater mirror!\n");
        if(!engine->SwitchMirror())
            return;
    }

    // Check if we want to do an integrity check instead of an update.
    if(engine->GetConfig()->CheckForIntegrity())
    {
        printf("Checking the integrity of the install:\n");
        engine->CheckIntegrity();
        return;
    }

    // Begin update checking!
    engine->CheckForUpdates();
    return;
}
