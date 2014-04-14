/*
 * pslaunch.cpp - Author: Mike Gist
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

#ifndef CS_PLATFORM_WIN32
#include <sys/wait.h>
#endif

#include <csutil/syspath.h>
#include <iutil/eventq.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>
#include <csutil/syspath.h>
#include <curl/curl.h>

#include "download.h"
#include "globals.h"
#include "pslaunch.h"
#include "pawslauncherwindow.h"
#include "updater.h"
#include "isoundmngr.h"

#include "paws/pawsbutton.h"
#include "paws/pawsmainwidget.h"
#include "paws/pawsmanager.h"
#include "paws/pawsokbox.h"
#include "paws/pawstextbox.h"
#include "util/log.h"

CS_IMPLEMENT_APPLICATION

psLauncherGUI* psLaunchGUI;

void UploadDump( const char * file, const char *uploadargs );

using namespace CS::Threading;

psLauncherGUI::psLauncherGUI(iObjectRegistry* _object_reg, InfoShare *_infoShare, bool *_execPSClient):
    mainWidget(NULL),launcherWidget(NULL)
{
    object_reg = _object_reg;
    infoShare = _infoShare;
    execPSClient = _execPSClient;

    drawScreen = true;
    elapsed = 0;
    psLaunchGUI = this;
    fileUtil = NULL;
    downloader = NULL;
    paws = NULL;
    updateTold = false;
}

void psLauncherGUI::Run()
{
    if(InitApp())
        csDefaultRunLoop(object_reg);

    delete paws;
    paws = NULL;
    delete downloader;
    downloader = NULL;
    delete fileUtil;
    fileUtil = NULL;

    csInitializer::CloseApplication(object_reg);
}

bool psLauncherGUI::InitApp()
{
    pslog::Initialize(object_reg);

    vfs = csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        printf("vfs failed to Init!\n");
        return false;
    }

    configManager = csQueryRegistry<iConfigManager> (object_reg);
    if (!configManager)
    {
        printf("configManager failed to Init!\n");
        return false;
    }

    queue = csQueryRegistry<iEventQueue> (object_reg);
    if (!queue)
    {
        printf("No iEventQueue plugin!\n");
        return false;
    }

    g3d = csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
    {
        printf("iGraphics3D failed to Init!\n");
        return false;
    }

    g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        printf("GetDriver2D failed to Init!\n");
        return false;
    }

    // Set the window title
    iNativeWindow *nw = g2d->GetNativeWindow();
    if (nw)
        nw->SetTitle(APPNAME);

    // Initialise Sound
    soundManager = csQueryRegistryOrLoad<iSoundManager>(object_reg, "iSoundManager");
    if(!soundManager)
    {
        // if the main sound manager is not found load the dummy plugin
        soundManager = csQueryRegistryOrLoad<iSoundManager>(object_reg, "crystalspace.planeshift.sound.dummy");
        if(!soundManager)
        {
            printf("Could not load iSoundManager!\n");
            return false;
        }
    }

    // Initialise downloader.
    downloader = new Downloader(vfs);

    // Initialise file utilities.
    fileUtil = new FileUtil(vfs);

    if(!csInitializer::OpenApplication(object_reg))
    {
        printf("Error initialising app (CRYSTAL not set?)\n");
        return false;
    }

    g2d->AllowResize(false);

    // paws initialization
    csString skinPath;
    skinPath = configManager->GetStr("PlaneShift.GUI.Skin", "/planeshift/art/pslaunch.zip");
    paws = new PawsManager(object_reg, skinPath);
    if (!paws)
    {
        printf("Failed to init PAWS!\n");
        return false;
    }

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    // Register factory
    launcherWidget = new pawsLauncherWindowFactory();

    // Load widgets
    if (!paws->LoadWidget("data/gui/pslaunch.xml"))
    {
        printf("Warning: Loading 'data/gui/pslaunch.xml' failed!");
        return false;
    }

    pawsWidget* launcher = paws->FindWidget("Launcher");
    launcher->SetBackgroundAlpha(0);

    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    // Register our event handler
    event_handler.AttachNew(new EventHandler (this));
    csEventID esub[] = 
    {
        csevFrame (object_reg),
        csevMouseEvent (object_reg),
        csevKeyboardEvent (object_reg),
        csevQuit (object_reg),
        CS_EVENTLIST_END
    };
    queue->RegisterListener(event_handler, esub);

    return true;
}

bool psLauncherGUI::HandleEvent (iEvent &ev)
{
    if (ev.Name == csevQuit (object_reg))
    {
        if(!infoShare->GetExitGUI())
        {
            infoShare->SetCancelUpdater(true);
        }
        return false;
    }

    if(infoShare->GetExitGUI())
    {
        Quit();
        return false;
    }

    //if the update was disabled we just don't show the window
    //so the user cannot update using this.
    if(!configManager->GetBool("Update.Enable", true))
    {
        updateTold = true;
        infoShare->SetOutOfSync(false);
    }

    if(infoShare->GetOutOfSync())
    {
        pawsOkBox* notify = (pawsOkBox*)paws->FindWidget("Notify");
        notify->SetText(
                "Local config and server config are incompatible!\n\n"
                "To resolve this, run a repair.");
        notify->Show();
        infoShare->SetOutOfSync(false);
    }

    if(infoShare->GetCheckIntegrity())
    {
        pawsMessageTextBox* updateProgressOutput = (pawsMessageTextBox*)paws->FindWidget("UpdaterOutput");
        while(!infoShare->ConsoleIsEmpty())
        {
            csString message = infoShare->ConsolePop();
            if(message.FindFirst("\n") == 0)
            {
                updateProgressOutput->AddMessage(message);
            }
            else if(message.FindFirst("\r") != (size_t) -1)
            {
                updateProgressOutput->ReplaceLastMessage(message);
            }
            else
            {
                updateProgressOutput->AppendLastMessage(message);
            }
        }
        if(infoShare->GetUpdateNeeded())
        {
            pawsButton* yes = (pawsButton*)paws->FindWidget("UpdaterYesButton");
            pawsButton* no = (pawsButton*)paws->FindWidget("UpdaterNoButton");
            yes->Show();
            no->Show();
        }
    }
    else if(infoShare->GetPerformUpdate())
    {
        if(!infoShare->GetUpdateNeeded())
        {
            infoShare->SetPerformUpdate(false);
            infoShare->Sync();
        }

        pawsMessageTextBox* updateProgressOutput = (pawsMessageTextBox*)paws->FindWidget("UpdaterOutput");
        while(!infoShare->ConsoleIsEmpty())
        {
            csString message = infoShare->ConsolePop();
            if(message.FindFirst("\n") == 0)
            {
                updateProgressOutput->AddMessage(message);
            }
            else if(message.FindFirst("\r") != (size_t)-1)
            {
                updateProgressOutput->ReplaceLastMessage(message);
            }
            else
            {
                updateProgressOutput->AppendLastMessage(message);
            }
        }
    }
    else if(paws->FindWidget("LauncherUpdater")->IsVisible())
    {
        paws->FindWidget("UpdaterOkButton")->Show();
        paws->FindWidget("UpdaterCancelButton")->Hide();
    }
    else if(!updateTold)
    {
        if(infoShare->GetUpdateAdminNeeded())
        {
            pawsOkBox* notify = (pawsOkBox*)paws->FindWidget("Notify");
            notify->SetText("An update is available but you don't have the correct permissions to continue!\n\n"
                    "Please restart the program as an admin.");
            notify->Show();
            updateTold = true;
        }
        else if(infoShare->GetUpdateNeeded())
        {
            paws->FindWidget("UpdateAvailable")->Show();
            updateTold = true;
        }
    }

    if (paws->HandleEvent(ev))
        return true;

    if (ev.Name == csevFrame (object_reg))
    {
        if (drawScreen)
        {
            FrameLimit();
            g3d->BeginDraw(CSDRAW_2DGRAPHICS);
            paws->Draw();
            soundManager->Update();
        }
        else
        {
            csSleep(150);
        }

        g3d->FinishDraw ();
        g3d->Print (NULL);
        return true;
    }
    else if (ev.Name == csevCanvasHidden (object_reg, g2d))
    {
        drawScreen = false;
    }
    else if (ev.Name == csevCanvasExposed (object_reg, g2d))
    {
        drawScreen = true;
    }
    return false;
}

void psLauncherGUI::FrameLimit()
{
    csTicks sleeptime;
    csTicks elapsedTime = csGetTicks() - elapsed;

    // Define sleeptime to limit fps to around 45
    sleeptime = 22; // 1000 / 45

    // Here we sacrifice drawing time
    if(elapsedTime < sleeptime)
        csSleep(sleeptime - elapsedTime);

    elapsed = csGetTicks();
}

void psLauncherGUI::Quit()
{
    queue->GetEventOutlet()->Broadcast(csevQuit (object_reg));
    infoShare->SetExitGUI(true);
}

void psLauncherGUI::PerformUpdate(bool update, bool integrity)
{
    infoShare->SetPerformUpdate(update || integrity);
    infoShare->SetUpdateNeeded(update);
    infoShare->Sync();

    if(update && !infoShare->GetExitGUI())
    {
        paws->FindWidget("LauncherMain")->Hide();
        paws->FindWidget("LauncherUpdater")->Show();
        paws->FindWidget("LauncherUpdater")->OnGainFocus();
        paws->FindWidget("UpdaterCancelButton")->Show();
    }
}

void psLauncherGUI::PerformRepair()
{
    infoShare->EmptyConsole();
    if(infoShare->GetCheckIntegrity())
    {
        csSleep(500);
    }
    infoShare->SetCheckIntegrity(true);
}

int main(int argc, char* argv[])
{
    // Select between GUI and console mode.
    bool console = false;
    bool help = false;
    bool uploaddump = false;
    csString  dumpUpload("");
    csString  dumpUploadArgs("");

    for(int i=0; i<argc; i++)
    {
        csString s(argv[i]);
        if(s.CompareNoCase("--console") || s.CompareNoCase("-console") ||
           s.CompareNoCase("--switch") || s.CompareNoCase("-switch") ||
           s.CompareNoCase("--repair") || s.CompareNoCase("-repair") ||
           s.CompareNoCase("--uploaddump") || s.CompareNoCase("-uploaddump"))
        {
            console = true;
        }
        else if(s.CompareNoCase("--help") || s.CompareNoCase("-help"))
        {
            help = true;
        }
        else if(s.StartsWith("--uploaddump",true) || s.StartsWith("-uploaddump",true))
        {
            dumpUpload=s.Slice( s.FindFirst( '=', 0 )+1);
            uploaddump = true;
        }
        else if(s.StartsWith("--args",true) || s.StartsWith("-args",true))
        {
            dumpUploadArgs=s.Slice( s.FindFirst( '=', 0 )+1);
        }
    }

    // Must be done when no other threads are running!
    curl_global_init(CURL_GLOBAL_ALL);

    // Convert args to an array of csString.
    csStringArray args;
    for(int i=0; i<argc; i++)
    {
        args.Push(argv[i]);
    }

    if(help)
    {
        printf("PlaneShift Updater Version %1.2f for %s.\n"
               "Launcher and updater for Planeshift\n\n"
               "pslaunch [--help] [--console] [--repair] [--switch]\n\n"
               "--help      Displays this help dialog\n"
               "--console   Run updater without the GUI\n"
               "--switch    Switch active updater mirror\n"
               "--repair        Check for any problems and prompt to repair them\n"
               "--uploaddump    Send a Planeshift dumpfile to the dev team for analysis\n",
               UPDATER_VERSION, (new Config())->GetPlatform());
    }
    else if (uploaddump)
    {
        printf ("PSUpdater --uploaddump executing...\n");
        // Set up CS (DONE ONLY TO GET VFS)
        psUpdater* updater = new psUpdater(argc, argv);

        // Initialize updater engine. (DONE ONLY TO GET VFS)
        UpdaterEngine* engine = new UpdaterEngine(args, updater->GetObjectRegistry(), "pslaunch");

        // read parameters from file (linux only at the moment)
        csRef<iVFS> vfs = engine->GetVFS();
        csRef<iConfigFile> configFile;
        configFile.AttachNew(new csConfigFile("/planeshift/userdata/crash/crash.params",vfs));
        configFile->Load("/planeshift/userdata/crash/crash.params",vfs);

        // based on : http://www.fifi.org/doc/libcurl-ssl-dev/examples/postit2.c
        CURL* curl = curl_easy_init();
        struct curl_httppost* post = NULL;
        struct curl_httppost* last = NULL;
        struct curl_slist *headerlist=NULL;
        //char buf[] = "Expect:";

        /* upload to this place */
        curl_easy_setopt(curl, CURLOPT_URL,"http://194.116.72.94/crash-reports/submit");

        /* Add simple file section */
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "upload_file_minidump",
                     CURLFORM_FILE, dumpUpload.GetData(), CURLFORM_END);

        curl_formadd(&post, &last, CURLFORM_COPYNAME, "name",
                     CURLFORM_COPYCONTENTS, "upload_file_minidump",
                     CURLFORM_END);

        curl_formadd(&post, &last, CURLFORM_COPYNAME, "filename",
                     CURLFORM_COPYCONTENTS, dumpUpload.GetData(),
                     CURLFORM_END);

        curl_formadd(&post, &last, CURLFORM_COPYNAME, "ProductName",
                     CURLFORM_COPYCONTENTS, "PlaneShift",
                     CURLFORM_END);

        curl_formadd(&post, &last, CURLFORM_COPYNAME, "ReleaseChannel",
                     CURLFORM_COPYCONTENTS, "release",
                     CURLFORM_END);

        curl_formadd(&post, &last, CURLFORM_COPYNAME, "Version",
                     CURLFORM_COPYCONTENTS, configFile->GetStr("Version"),
                     CURLFORM_END);

        curl_formadd(&post, &last, CURLFORM_COPYNAME, "PlayerName",
                     CURLFORM_COPYCONTENTS, configFile->GetStr("PlayerName"),
                     CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "Renderer",
                     CURLFORM_COPYCONTENTS, configFile->GetStr("Renderer"),
                     CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "RendererVersion",
                     CURLFORM_COPYCONTENTS, configFile->GetStr("RendererVersion"),
                     CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "CrashTime",
                     CURLFORM_COPYCONTENTS, configFile->GetStr("CrashTime"),
                     CURLFORM_END);
        curl_formadd(&post, &last, CURLFORM_COPYNAME, "Processor",
                     CURLFORM_COPYCONTENTS, configFile->GetStr("Processor"),
                     CURLFORM_END);

        /* enable verbose for easier tracing */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        /*
         * Initalize custom header list
         * (stating that Expect: 100-continue is not wanted)
         */
        headerlist = curl_slist_append(headerlist, "Content-Disposition: form-data; name=\"example\"");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

        /* Set the form info */
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

        // Perform the upload
        CURLcode res = curl_easy_perform(curl);

        /* Check for errors */
        if(res != CURLE_OK)
        {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));

        }
        else
        {
          double speed_upload, total_time;
          /* now extract transfer info */
          curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
          curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

          fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",
                  speed_upload, total_time);
          //delete the dumpfile if the upload was successful
          unlink( dumpUpload.GetData() );
        }
        /* always cleanup */
        curl_easy_cleanup(curl);

    }
    else if(console)
    {
        // Set up CS
        psUpdater* updater = new psUpdater(argc, argv);

        // Initialize updater engine.
        UpdaterEngine* engine = new UpdaterEngine(args, updater->GetObjectRegistry(), "pslaunch");

        printf("\nPlaneShift Updater Version %1.2f for %s.\n\n", UPDATER_VERSION, engine->GetConfig()->GetCurrentConfig()->GetPlatform());

        // Run the update process!
        updater->RunUpdate(engine);

        // Maybe this fixes a bug.
        fflush(stdout);

        if(!engine->GetConfig()->IsSelfUpdating())
        {
#if defined(WIN32) && defined(NDEBUG)
            printf("\nUpdater finished!\n");
#else
            printf("\nUpdater finished, press enter to exit.\n");
            getchar();
#endif
        }

        // Terminate updater!
        delete engine;
        delete updater;
        engine = NULL;
        updater = NULL;
    }
    else
    {
        bool exitApp = false;
        while(!exitApp)
        {
            // Set up CS
            iObjectRegistry* object_reg = csInitializer::CreateEnvironment (argc, argv);

            if(!object_reg)
            {
                printf("Object Reg failed to Init!\n");
                exit(1);
            }

            // Request needed plugins for updater.
            csInitializer::SetupConfigManager(object_reg, LAUNCHER_CONFIG_FILENAME);
            csInitializer::RequestPlugins(object_reg, CS_REQUEST_VFS, CS_REQUEST_END);

            // Mount the VFS paths.
            csRef<iVFS> vfs = csQueryRegistry<iVFS>(object_reg);
            if (!vfs->Mount ("/planeshift/", "$^"))
            {
                printf("Failed to mount /planeshift!\n");
                return false;
            }

            // Set config path.
            csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (object_reg);
            csString configPath = csGetPlatformConfigPath("PlaneShift");
            configPath.ReplaceAll("/.crystalspace/", "/.");
            configPath = configManager->GetStr("PlaneShift.UserConfigPath", configPath);

            // Check that the path exists, else attempt to create it.
            FileUtil* fileUtil = new FileUtil(vfs);
            csRef<FileStat> filestat = fileUtil->StatFile(configPath);
            if (!filestat.IsValid() && CS::Platform::CreateDirectory(configPath) < 0)
            {
                printf("Could not create required %s directory!\n", configPath.GetData());
                return false;
            }
            filestat.Invalidate();
            delete fileUtil;
            fileUtil = NULL;

            // Mount config path.
            if (!vfs->Mount("/planeshift/userdata", configPath + "$/"))
            {
                printf("Could not mount %s as /planeshift/userdata!\n", configPath.GetData());
                return false;
            }

            // Init thread communication structure.
            InfoShare *infoShare = new InfoShare();
            infoShare->SetPerformUpdate(false);
            infoShare->SetUpdateNeeded(false);

            // Initialize updater engine.
            csRef<UpdaterEngine> engine;
            engine.AttachNew(new UpdaterEngine(args, object_reg, "pslaunch", infoShare));

            // If we're self updating, continue self update.
            if(engine->GetConfig()->IsSelfUpdating())
            {
                exitApp = engine->SelfUpdate(engine->GetConfig()->IsSelfUpdating());
            }

            // Set to true by GUI if we have to launch the client.
            bool execPSClient = false;

            // If we don't have to exit the app, create updater thread and run the GUI.
            if(!exitApp)
            {
                // Start up updater.
                csRef<Thread> updaterThread;
                updaterThread.AttachNew(new Thread(engine));
                updaterThread->Start();

                // Request needed plugins for GUI.
                csInitializer::RequestPlugins(object_reg,
                        CS_REQUEST_FONTSERVER,
                        CS_REQUEST_IMAGELOADER,
                        CS_REQUEST_OPENGL3D,
                        CS_REQUEST_ENGINE,
                        CS_REQUEST_LEVELLOADER,
                        CS_REQUEST_END);

                infoShare->SetCurrentClientVersion(engine->GetCurrentClientVersion());

                // Start GUI.
                psLauncherGUI* gui = new psLauncherGUI(object_reg, infoShare, &execPSClient);
                gui->Run();

                // Free GUI.
                delete gui;
            }

            // Clean up everything else.
            engine.Invalidate();
            delete infoShare;
            configManager.Invalidate();
            vfs.Invalidate();
            csInitializer::DestroyApplication(object_reg);

            if (execPSClient)
            {
                // Execute psclient process.

#ifdef CS_PLATFORM_WIN32

                // Info for CreateProcess.
                STARTUPINFO siStartupInfo;
                DWORD dwExitCode;
                PROCESS_INFORMATION piProcessInfo;
                memset(&siStartupInfo, 0, sizeof(siStartupInfo));
                memset(&piProcessInfo, 0, sizeof(piProcessInfo));
                siStartupInfo.cb = sizeof(siStartupInfo);

                csString commandLine = "psclient.exe";

                for(int i=1; i<argc; ++i)
                {
                    commandLine.AppendFmt(" %s", argv[i]);
                }

                if(CreateProcess(NULL, (LPSTR)commandLine.GetData(), 0, 0, false,
                        CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo))
                {
                    GetExitCodeProcess(piProcessInfo.hProcess, &dwExitCode);
                    while (dwExitCode == STILL_ACTIVE)
                    {
                        csSleep(1000);
                        GetExitCodeProcess(piProcessInfo.hProcess, &dwExitCode);
                    }
                    exitApp = dwExitCode ? 0 : !0;
                    CloseHandle(piProcessInfo.hProcess);
                    CloseHandle(piProcessInfo.hThread);
                }
                else
                    printf("Failed to launch psclient!\n");
#else
                if(fork() == 0)
                {
#ifdef CS_PLATFORM_MACOSX
                    char** nargv = new char*[argc+2];
                    char* name = "/usr/bin/open";
                    char* psc = "psclient.app";
                    nargv[0] = name;
                    nargv[1] = psc;
                    for(int i=2; i<argc+1; ++i)
                    {
                        nargv[i] = argv[i-1];
                    }
                    nargv[argc+1] = (char*)0;
                    execv("/usr/bin/open", nargv);
                    delete nargv;
#else
                    char** nargv = new char*[argc+1];
                    char* name = const_cast<char*>("./psclient");
                    nargv[0] = name;
                    for(int i=1; i<argc; ++i)
                    {
                        nargv[i] = argv[i];
                    }
                    nargv[argc] = (char*)0;
                    execv("./psclient", nargv);
                    delete nargv;
#endif
                }
                else
                {
                    int status;
                    wait(&status);
                    exitApp = (status == 0);
                }
#endif
            }
            else
            {
                exitApp = true;
            }
        }
    }

    return 0;
}
