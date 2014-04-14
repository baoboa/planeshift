/*
 * pscssetup.cpp - Authored by Elliot Paquette, cetel@earthlink.net
 *
 * The code for class reg. was adapted from Andrew Craig's 
 * pawsmainwidget.cpp.
 *
 * This is an adaptation of Matze Braun's mountcfg and *.inc files
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


#include <iutil/vfs.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/plugin.h>
#include <iutil/objreg.h>

#include <ivaria/reporter.h>
#include <ivaria/stdrep.h>
#include <ivideo/natwin.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <cstool/initapp.h>

#include <csutil/util.h>
#include <csutil/cmdhelp.h>
#include <csutil/scf.h>
#include <csutil/xmltiny.h>
#include <csutil/csstring.h>
#include <csutil/csshlib.h>
#include <csutil/databuf.h>
#include <iutil/stringarray.h>

#include <igraphic/imageio.h>

#include "util/fileutil.h"
#include "util/log.h"
#include "util/consoleout.h"
#include "pscssetup.h"

iObjectRegistry* psCSSetup::object_reg = NULL;

psCSSetup::psCSSetup(int _argc, char** _argv, 
                    const char* _engineConfigfile, const char* _userConfigfile)
{
  argc=_argc;
  argv=_argv;
  engineConfigfile=_engineConfigfile;
  userConfigfile=_userConfigfile;

  object_reg = csInitializer::CreateEnvironment (argc, argv);
  if (!object_reg) PS_PAUSEEXIT(1);

}


psCSSetup::~psCSSetup()
{
    csRef<iStringArray> mounts = vfs->GetMounts();
    const size_t count = mounts->GetSize();
    for(size_t i = 0; i < count; i++)
        vfs->Unmount( mounts->Get(i), NULL );
}

bool psCSSetup::AddWindowInformations(const char *Info)
{
    return InitCSWindow((csString)APPNAME + " [" + Info + "]");
}

bool psCSSetup::InitCSWindow(const char *name)
{
    csRef<iGraphics3D> g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
        return false;

    if (!g3d->GetDriver2D())
        return false;
    
    iNativeWindow *nw = g3d->GetDriver2D()->GetNativeWindow();
    if (nw)
        nw->SetTitle(name);
    return true;
}

bool psCSSetup::SetIcon(const char* ImageFileLocation)
{
    //add a nice icon
    csRef<iImage> Image;
    csRef<iDataBuffer> buf(vfs->ReadFile(ImageFileLocation, false));
    if (!buf.IsValid())
    {
        Error2( "Could not open window icon: >%s<", ImageFileLocation);
        return false;
    }
    csRef<iImageIO> imageLoader =  csQueryRegistry<iImageIO >(object_reg);
    Image = imageLoader->Load(buf, CS_IMGFMT_TRUECOLOR | CS_IMGFMT_ALPHA);
    return SetIcon(Image);
}

bool psCSSetup::SetIcon(iImage* Image)
{
    csRef<iGraphics3D> g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d || !g3d->GetDriver2D()) return false;
    iNativeWindow *nw = g3d->GetDriver2D()->GetNativeWindow();
    if (!nw) return false;
    nw->SetIcon(Image);
    return true;
}

iObjectRegistry* psCSSetup::InitCS(iReporterListener * customReporter)
{ 
    if (!object_reg)
        PS_PAUSEEXIT(1);

    if ( !csInitializer::SetupConfigManager(object_reg, engineConfigfile))
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, "psclient",
               "csInitializer::SetupConfigManager failed!\n"
               "Is your CRYSTAL environment variable set?");
        PS_PAUSEEXIT(1);
    }

    vfs  =  csQueryRegistry<iVFS> (object_reg);
    configManager =  csQueryRegistry<iConfigManager> (object_reg);

    // Mount user dir.
    MountUserData();

    //disable fullscreen default on linux. It's not well supported
    #ifndef CS_PLATFORM_WIN32
    #ifndef CS_PLATFORM_MACOSX
    configManager->SetBool("Video.FullScreen", false);
    #endif
    #endif
  
    if (userConfigfile != NULL)
    {
        cfg = configManager->AddDomain(userConfigfile,vfs,iConfigManager::ConfigPriorityUserApp);
        configManager->SetDynamicDomain(cfg);
    }

    bool pluginRequest;
    if (customReporter)
    {
        pluginRequest = csInitializer::RequestPlugins(object_reg,
            CS_REQUEST_REPORTER,
            CS_REQUEST_END);

        csRef<iReporter> reporter =  csQueryRegistry<iReporter> (object_reg);
        if (reporter)
            reporter->AddReporterListener(customReporter);
    }
    else
    {
        pluginRequest = csInitializer::RequestPlugins(object_reg,
            CS_REQUEST_REPORTER,
            CS_REQUEST_REPORTERLISTENER,
            CS_REQUEST_END);
    }
    if (!pluginRequest)
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, 
               "psclient", "Failed to initialize plugins!");
        return 0;
    }
  
    // Check for commandline help.
    if (csCommandLineHelper::CheckHelp (object_reg))
    {
        csCommandLineHelper::Help (object_reg);
        PS_PAUSEEXIT(1);
    }

    // Initialize Graphics Window
    if (!InitCSWindow(APPNAME)) 
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
            "psclient", "No 3d driver (iGraphics3D) plugin!");
        PS_PAUSEEXIT(1);
    }

    if (!csInitializer::OpenApplication (object_reg))
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, 
                "psclient",
                "csInitializer::OpenApplication failed!\n"
                "Is your CRYSTAL environment var set?");
        csInitializer::DestroyApplication (object_reg);
        PS_PAUSEEXIT(1);
    }

    // Reset the window name because with Xwindows OpenApplication changes it
    InitCSWindow(APPNAME);

    SetIcon((const char*)"/planeshift/support/icons/psicon.png");

    // tweak reporter plugin to report everything...
    // is there a command line switch or something to do this which I've missed?
    if (customReporter == 0)
    {
        csRef<iStandardReporterListener> reporter = 
             csQueryRegistry<iStandardReporterListener> (object_reg);
        if (reporter) 
        {
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_BUG, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_ERROR, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_NOTIFY, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_DEBUG, true);
            reporter->ShowMessageID(CS_REPORTER_SEVERITY_WARNING, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_NOTIFY, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_ERROR, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_WARNING, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_BUG, true, false, false, false, true);
            reporter->SetMessageDestination(CS_REPORTER_SEVERITY_DEBUG, true, false, false, false, true);
        }
    }

    return object_reg;
}

void psCSSetup::MountUserData()
{
    // Mount the per-user configuration directory (platform specific)
    // - On Linux, use ~/.PlaneShift instead of ~/.crystalspace/PlaneShift.
    csString configPath = csGetPlatformConfigPath("PlaneShift");
    configPath.ReplaceAll("/.crystalspace/", "/.");

    // Alternatively, you can set it in psclient.cfg.
    configPath = configManager->GetStr("PlaneShift.UserConfigPath", configPath);

    printf("Your configuration files are in... %s\n", configPath.GetData());

    // Create the mount point if it doesn't exist...die if we can't.
    FileUtil fileUtil(vfs);
    csRef<FileStat> filestat = fileUtil.StatFile(configPath);
    if (!filestat.IsValid() && CS::Platform::CreateDirectory(configPath) < 0)
    {
        printf("Could not create required %s directory!\n", configPath.GetData());
        PS_PAUSEEXIT(1);
    }

    if (!vfs->Mount("/planeshift/userdata", configPath + "$/"))
    {
        printf("Could not mount %s as /planeshift/userdata!\n", configPath.GetData());
        PS_PAUSEEXIT(1);
    }

    vfs->Unmount("/shadercache/user", NULL);

    if (!vfs->Mount("/shadercache/user", configPath + "$/shadercache"))
    {
        printf("Could not mount %s as /shadercache/user!\n", configPath.GetData() );
        PS_PAUSEEXIT(1);
    }
}
