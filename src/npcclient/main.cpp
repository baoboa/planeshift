#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>

#include "globals.h"
#include "util/serverconsole.h"
#include "util/pscssetup.h"
#include "npcclient.h"
#include "npcgui.h"
//#include "net/netpacket.h"
#include "util/log.h"

// global vars
psNPCClient* npcclient;

CS_IMPLEMENT_APPLICATION

// ----------------------------------------------------------------

#ifdef APPNAME
#undef APPNAME
#endif
#define APPNAME "PlaneShift NPC Client"

#include "util/strutil.h"
#include <iutil/cmdline.h>


int main(int argc, char** argv)
{

    psCSSetup* CSSetup = new psCSSetup(argc, argv, "/this/npcclient.cfg",0);
    iObjectRegistry* object_reg = CSSetup->InitCS();

    pslog::Initialize(object_reg);

    csRef<iCommandLineParser> clp = csQueryRegistry<iCommandLineParser>(object_reg);

    // Start the server
    npcclient = new psNPCClient;
    csString host = clp->GetOption("host");
    csString user = clp->GetOption("user");
    csString pass = clp->GetOption("pass");
    csString portStr = clp->GetOption("port");
    int port = atoi(portStr.GetDataSafe());

    if(!npcclient->Initialize(object_reg,host,user,pass,port))
    {
        CPrintf(CON_ERROR, COL_RED "error while initializing NpcClient!\n" COL_NORMAL);
        return 1;
    }

    // Check whether to init debug gui.
    NpcGui* gui = 0;
    if(clp->GetBoolOption("gui"))
    {
        gui = new NpcGui(object_reg, npcclient);
        CS_ASSERT_MSG("GUI failed to initialise!", gui->Initialise());
    }

    npcclient->MainLoop();

    delete gui;
    delete npcclient;

    // Save Configuration
    csRef<iConfigManager> cfgmgr= csQueryRegistry<iConfigManager> (object_reg);
    if(cfgmgr)
        cfgmgr->Save();
    cfgmgr = NULL;

    delete CSSetup;

    csInitializer::DestroyApplication(object_reg);

    return 0;
}

