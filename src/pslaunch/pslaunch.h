/*
* pslaunch.h - Author: Mike Gist
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

#ifndef __PSLAUNCH_H__
#define __PSLAUNCH_H__

#include <csutil/threading/thread.h>

#include "updaterconfig.h"
#include "updaterengine.h"

#include "util/genericevent.h"

class pawsMessageTextBox;

#define APPNAME "PlaneShift Launcher"

#define LAUNCHER_CONFIG_FILENAME "/this/pslaunch.cfg"

class pawsLauncherWindowFactory;
class pawsMainWidget;
class PawsManager;
struct iSoundManager;
struct iObjectRegistry;

class psLauncherGUI
{
private:

    iObjectRegistry* object_reg;
    csRef<iVFS> vfs;
    csRef<iConfigManager> configManager;
    csRef<iGraphics3D>      g3d;
    csRef<iGraphics2D>      g2d;
    csRef<iEventQueue>      queue;
    csRef<iSoundManager>    soundManager;
    DeclareGenericEventHandler(EventHandler, psLauncherGUI, "planeshift.launcher");
    csRef<EventHandler> event_handler;

    // PAWS
    PawsManager*    paws;
    pawsMainWidget* mainWidget;
    pawsLauncherWindowFactory* launcherWidget;

    /* Info shared with other threads. */
    InfoShare *infoShare;

    /* Set to true to launch the client. */
    bool *execPSClient;
    
    /* keeps track of whether the window is visible or not. */
    bool drawScreen;

    /* Limits the frame rate either by sleeping. */
    void FrameLimit();

    /* Time when the last frame was drawn. */
    csTicks elapsed;

    /* Load the app */
    bool InitApp();

    /* Handles an event from the event handler */
    bool HandleEvent (iEvent &ev);

    /* Downloads server news */
    Downloader* downloader;

    /* File utilities */
    FileUtil* fileUtil;

    /* True if we've already been told there's an update available. */
    bool updateTold;

public:
    /* Quit the application */
    void Quit();

    void ExecClient(bool value) { *execPSClient = value; }

    psLauncherGUI(iObjectRegistry* _object_reg, InfoShare *_infoShare, bool *_execPSClient);

    Downloader* GetDownloader() { return downloader; }

    iVFS* GetVFS() { return vfs; }

    FileUtil* GetFileUtil() { return fileUtil; }

    void PerformUpdate(bool update, bool integrity);

    void PerformRepair();

    void CancelUpdater() { infoShare->SetCancelUpdater(true); }

    bool UpdateChecked() { return infoShare->GetUpdateChecked(); }

    csString GetCurrentClientVersion() { return infoShare->GetCurrentClientVersion(); }

    // Run.
    void Run();
};

#endif // __PSLAUNCH_H__
