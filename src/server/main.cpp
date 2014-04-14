/*
 * main.cpp
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

#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>

#include "globals.h"
#include "psserver.h"
#include <idal.h>

#include "util/serverconsole.h"
#include "util/log.h"
#include "util/pscssetup.h"
#include "net/messages.h"


// ----------------------------------------------------------------
// this is needed for the engine to construct OS specific stuff
// such as building WinMain() calling function in windows
CS_IMPLEMENT_APPLICATION

// ----------------------------------------------------------------

// global vars
psServer* psserver;
extern iDataConnection* db;

// ----------------------------------------------------------------
#ifdef APPNAME
#undef APPNAME
#endif

#define APPNAME "PlaneShift Azure Spirit - Server"

#include "util/strutil.h"


int main(int argc, char** argv)
{
    psCSSetup* CSSetup = new psCSSetup(argc, argv, "/this/psserver.cfg", 0);

    iObjectRegistry* object_reg = CSSetup->InitCS();

    pslog::Initialize(object_reg);

    // Start the server
    psserver = new psServer;

    if(!psserver->Initialize(object_reg))
    {
        CPrintf(CON_ERROR, COL_RED "Error while initializing server!\n" COL_NORMAL);

        // Cleanup before exit
        delete psserver;
        delete CSSetup;
        csInitializer::DestroyApplication(object_reg);

        PS_PAUSEEXIT(1);
    }

    psserver->MainLoop();

    delete psserver;

    // Free registered message factories.
    psfUnRegisterMsgFactories();

    // Save Configuration
    csRef<iConfigManager> cfgmgr= csQueryRegistry<iConfigManager> (object_reg);
    if(cfgmgr)
    {
        cfgmgr->Save();
    }

    cfgmgr = NULL;

    delete CSSetup;

    csInitializer::DestroyApplication(object_reg);

    printf("Main thread stopped!\n");
    return 0;
}

