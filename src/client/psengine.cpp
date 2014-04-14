/*
 * psengine.cpp
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

//////////////////////////////////////////////////////////////////////////////
// OS Specific Defines
//////////////////////////////////////////////////////////////////////////////
#if defined(CS_COMPILER_GCC) && defined(CS_PLATFORM_WIN32) && defined(LoadImage)
// Somewhere in the mingw includes there is a
// define LoadImage -> LoadImageA and this
// is problematic.
#undef LoadImage
#endif

#ifdef WIN32
#include <ivideo/natwin.h> // CS_ALERT_ERROR definition
#endif

//////////////////////////////////////////////////////////////////////////////

#define CONFIGFILENAME       "/planeshift/userdata/planeshift.cfg"
#define PSAPP                "planeshift.application.client"


//////////////////////////////////////////////////////////////////////////////
// Macros
//////////////////////////////////////////////////////////////////////////////
#define PS_QUERY_PLUGIN(myref, intf, str)                    \
myref =  csQueryRegistryOrLoad<intf> (object_reg, str);      \
if (!myref)                                                  \
{                                                            \
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, PSAPP, \
       "No " str " plugin!");                                \
    return false;                                            \
}                                                            \
 
#define RegisterFactory(factoryclass)   \
    factory = new factoryclass(); \
    CS_ASSERT(factory);

//////////////////////////////////////////////////////////////////////////////

#include "psengine.h"

// CS files
#include <imap/services.h>
#include <iutil/cfgmgr.h>
#include <iutil/event.h>
#include <iutil/eventq.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/virtclk.h>
#include <iutil/vfs.h>
#include <iutil/stringarray.h>
#include <ivideo/natwin.h>
#include <csver.h>
#include <cstool/collider.h>
#include <cstool/initapp.h>
#include <csutil/event.h>

#include "globals.h"
#include "pscharcontrol.h"
#include "pscamera.h"
#include "psslotmgr.h"
#include "clientsongmngr.h"
#include "pscelclient.h"
#include "psnetmanager.h"
#include "psclientdr.h"
#include "psclientchar.h"
#include "modehandler.h"
#include "actionhandler.h"
#include "zonehandler.h"
#include "clientvitals.h"
#include "guihandler.h"
#include "autoexec.h"

#include "pscal3dcallback.h"

#include "net/connection.h"
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

#include "effects/pseffectmanager.h"

#include "psoptions.h"

#include "util/localization.h"
#include "util/pscssetup.h"
#include "util/log.h"
#include "util/strutil.h"
#include "engine/psworld.h"
#include "util/psutil.h"
#include "util/consoleout.h"
#include "entitylabels.h"
#include "chatbubbles.h"
#include "questionclient.h"
#include <ibgloader.h>
#include <iscenemanipulate.h>

/////////////////////////////////////////////////////////////////////////////
//  PAWS Includes
/////////////////////////////////////////////////////////////////////////////
#include "paws/pawsprogressbar.h"
#include "paws/pawsmenu.h"

#include "gui/pawsloading.h"
#include "gui/pawsglyphwindow.h"
#include "gui/chatwindow.h"
#include "gui/psmainwidget.h"
#include "gui/pawsconfigwindow.h"
#include "gui/pawsconfigmouse.h"
#include "gui/pawsconfigkeys.h"
#include "gui/pawsconfigcamera.h"
#include "gui/pawsconfigdetails.h"
#include "gui/pawsconfigmarriage.h"
#include "gui/pawsconfigpopup.h"
#include "gui/pawsconfigpvp.h"
#include "gui/pawsconfigchat.h"
#include "gui/pawsconfigchattabs.h"
#include "gui/pawsconfigchattabcompletion.h"
#include "gui/pawsconfigchatlogs.h"
#include "gui/pawsconfigsound.h"
#include "gui/pawsconfigshortcut.h"
#include "gui/pawsconfigactivemagic.h"
#include "gui/pawsconfigchatfont.h"
#include "gui/pawsconfigentitylabels.h"
#include "gui/pawsconfigentityinter.h"
#include "gui/pawsconfigautoexec.h"
#include "gui/pawscraftcancelwindow.h"
#include "gui/inventorywindow.h"
#include "gui/pawsitemdescriptionwindow.h"
#include "gui/pawscontainerdescwindow.h"
#include "gui/pawsinteractwindow.h"
#include "gui/pawsinfowindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/pawsglyphwindow.h"
#include "gui/pawsgroupwindow.h"
#include "gui/pawsexchangewindow.h"
#include "gui/pawsmerchantwindow.h"
#include "gui/pawsstoragewindow.h"
#include "gui/pawspetitionwindow.h"
#include "gui/pawspetitiongmwindow.h"
#include "gui/pawsspellbookwindow.h"
#include "gui/attacklist.h"
#include "gui/pawssplashwindow.h"
#include "gui/shortcutwindow.h"
#include "gui/pawsscrollmenu.h"
#include "gui/pawsdndbutton.h"
#include "gui/pawsloginwindow.h"
#include "gui/pawscharpick.h"
#include "gui/pawsloading.h"
#include "gui/pawsguildwindow.h"
#include "gui/pawslootwindow.h"
#include "gui/pawspetstatwindow.h"
#include "gui/pawsskillwindow.h"
#include "gui/pawsquestwindow.h"
#include "gui/pawsspellcancelwindow.h"
#include "gui/pawscharcreatemain.h"
#include "gui/pawscharbirth.h"
#include "gui/pawscharparents.h"
#include "gui/pawschild.h"
#include "gui/pawslife.h"
#include "gui/pawspath.h"
#include "gui/pawssummary.h"
#include "gui/pawsgmgui.h"
#include "gui/pawsmodswindow.h"
#include "gui/pawsmoney.h"
#include "gui/pawshelp.h"
#include "gui/pawsbuddy.h"
#include "gui/pawsignore.h"
#include "gui/pawsslot.h"
#include "gui/pawsactionlocationwindow.h"
#include "gui/pawsdetailwindow.h"
#include "gui/pawschardescription.h"
#include "gui/pawsquestrewardwindow.h"
#include "gui/pawscreditswindow.h"
#include "gui/pawsinventorydollview.h"
#include "gui/pawsquitinfobox.h"
#include "gui/pawsgmspawn.h"
#include "gui/pawsbookreadingwindow.h"
#include "gui/pawswritingwindow.h"
#include "gui/pawsactivemagicwindow.h"
#include "gui/pawstutorialwindow.h"
#include "gui/pawssmallinventory.h"
#include "gui/pawsconfigchatfilter.h"
#include "gui/pawsgmaction.h"
#include "gui/pawscraft.h"
#include "gui/pawsilluminationwindow.h"
#include "gui/pawsgameboard.h"
#include "gui/pawsbankwindow.h"
#include "gui/pawsconfigchatbubbles.h"
#include "gui/pawsconfigshadows.h"
#include "gui/pawsconfigtextpage.h"
#include "gui/pawsnpcdialog.h"
#include "gui/pawsconfigspellchecker.h"
#include "gui/pawsconfigtooltips.h"
#include "gui/pawsmusicwindow.h"
#include "gui/pawssheetline.h"
#include "gui/pawsscripthirednpcwindow.h"

// Needed for debugging
csString psEngine::hwRenderer = "";
csString psEngine::hwVersion = "";
csString psEngine::playerName = "";

#ifdef WIN32
#include <WinUser.h>
#ifdef SendMessage
#undef SendMessage
#endif
#include "csutil/win32/win32.h"
HWND psEngine::hwnd = 0;
#endif

#if !defined(CS_DEBUG) && defined(CS_PLATFORM_MACOSX)
// Set up the mac crash reporter in release builds. This needs to be done here
// since the mac reporter is in a different library.
#include "macosx/maccrashreport.h"

MacCrashReport* macReporter = NULL;
#endif

// ----------------------------------------------------------------------------

CS_IMPLEMENT_APPLICATION

// ----------------------------------------------------------------------------

psEngine::psEngine(iObjectRegistry* objectreg, psCSSetup* CSSetup)
{
    object_reg = objectreg;
    CS_Setup = CSSetup;
    // No no, no map loaded
    loadedMap = false;
    gameLoaded = false;
    loadstate = LS_NONE;
    loadError = false;

    charmanager = NULL;
    guiHandler = NULL;
    charController = NULL;
    mouseBinds = NULL;
    camera = NULL;
    slotManager = NULL;
    songManager = NULL;
    questionclient = NULL;
    paws = NULL;
    mainWidget = NULL;
    inventoryCache = NULL;
    loader = NULL;
    autoexec = NULL;

    loadtimeout = 10;  // Default load timeout

    drawScreen = true;

    cal3DCallbackLoader.AttachNew(new psCal3DCallbackLoader(objectreg));

    BrightnessCorrection = 0.0f;

    KFactor = 0.0f;
    targetPetitioner = "None";
    duelConfirmation = 1;  // Def
    marriageProposal = 1;  // Def

    loggedIn = false;
    numOfChars = 0;
    elapsed = 0;
    frameLimit = 0;

    muteSoundsOnFocusLoss = false;

    actorRequested = false;

    xmlparser =  csQueryRegistry<iDocumentSystem> (object_reg);
    stringset = csQueryRegistryTagInterface<iStringSet> (object_reg, "crystalspace.shared.stringset");
    nameRegistry = csEventNameRegistry::GetRegistry(object_reg);

    chatBubbles = 0;
    options = 0;
    countFPS = 0;
    timeFPS = 0;
    currFPS = 0;
    showFPS = false;

#if !defined(CS_DEBUG) && defined(CS_PLATFORM_MACOSX)
    delete macReporter;
    macReporter = NULL;
#endif
}

// ----------------------------------------------------------------------------

psEngine::~psEngine()
{
    printf("psEngine destroyed.\n");
}


void psEngine::Cleanup()
{
    if(loadstate==LS_DONE)
        QuitClient();

    delete charmanager;
    delete charController;
    delete camera;
    delete chatBubbles;
    delete autoexec;

    if(queue)
    {
        if(eventHandlerLogic)
            queue->RemoveListener(eventHandlerLogic);
        if(eventHandler2D)
            queue->RemoveListener(eventHandler2D);
        if(eventHandler3D)
            queue->RemoveListener(eventHandler3D);
        if(eventHandlerFrame)
            queue->RemoveListener(eventHandlerFrame);
    }

    delete paws; // Include delete of mainWidget

    delete questionclient;
    delete slotManager;
    delete songManager;
    delete mouseBinds;
    delete guiHandler;
    delete inventoryCache;

    // celclient needs to be destroyed before the effectmanager.
    celclient.Invalidate();
    modehandler.Invalidate();

    // Effect manager needs to be destroyed before the soundmanager.
    effectManager.Invalidate();

    delete options;
}

// ----------------------------------------------------------------------------

/**
 *
 * psEngine::Initialize
 * This function initializes all objects necessary for the game. It also creates a
 * suitable environment for the engine.
 *
 **/
bool psEngine::Initialize(int level)
{
    // load basic stuff that is enough to display the splash screen
    if(level == 0)
    {
        // print out some version information
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP, APPNAME);
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
                 "This game uses Crystal Space Engine created by Jorrit and others");
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP, CS_VERSION);

        // Query for plugins
        PS_QUERY_PLUGIN(queue,        iEventQueue,    "iEventQueue");
        PS_QUERY_PLUGIN(vfs,          iVFS,           "iVFS");
        PS_QUERY_PLUGIN(engine,       iEngine,        "iEngine");
        PS_QUERY_PLUGIN(cfgmgr,       iConfigManager, "iConfigManager");
        PS_QUERY_PLUGIN(g3d,          iGraphics3D,    "iGraphics3D");
        PS_QUERY_PLUGIN(vc,           iVirtualClock,  "iVirtualClock");

        // Query for the sound plugin, if not found query the dummy one
        SoundManager = csQueryRegistryOrLoad<iSoundManager>(object_reg, "iSoundManager");
        if(!SoundManager)
        {
            PS_QUERY_PLUGIN(SoundManager, iSoundManager,  "crystalspace.planeshift.sound.dummy");
        }


        g2d = g3d->GetDriver2D();

        g2d->AllowResize(false);

        // Grab graphics info now in case we crash
        hwRenderer = g2d->GetHWRenderer();
        hwVersion = g2d->GetHWGLVersion();
#ifdef WIN32
        // Check for Windows software OpenGL renderer.
        if(hwRenderer == "GDI Generic")
        {
            printf("PlaneShift requires hardware acceleration (OpenGL). Make sure your video card drivers are up-to-date.\n");
            csRef<iWin32Assistant> assistant = csQueryRegistry<iWin32Assistant>(object_reg);
            assistant->AlertV(psEngine::hwnd, CS_ALERT_ERROR, "Warning!",  0, "PlaneShift requires hardware acceleration (OpenGL). Make sure your video card drivers are up-to-date.", 0);
        }
        csRef<iWin32Canvas> canvas = scfQueryInterface<iWin32Canvas> (g2d);
        // This does not seem to give the correct window handle.
        //hwnd = canvas->GetWindowHandle();
#endif

#if !defined(CS_DEBUG) && defined(CS_PLATFORM_MACOSX)
        // Set up the mac crash reporter now we know the graphics information.
        macReporter = new MacCrashReport(hwRenderer, hwVersion);
#endif

        // Initialize and tweak the Texture Manager
        txtmgr = g3d->GetTextureManager();
        if(!txtmgr)
        {
            return false;
        }

        //Check if sound is on or off in psclient.cfg
        csString soundPlugin;

        soundOn = true; // FIXMESOUND is this variable REALLY needed and if not how do we handle this in future?

        // Load Logger settings
        LoadLogSettings();

        // Initialize sound
        SoundManager->InitializeSectors();

        // Mount the selected gui first to allow overwriting of certain elements
        csString skinPath;
        skinPath += cfgmgr->GetStr("PlaneShift.GUI.Skin.Dir", "/planeshift/art/skins/");
        skinPath += cfgmgr->GetStr("PlaneShift.GUI.Skin.Selected", "default.zip");
        // This could be a file or a dir
        csString slash(CS_PATH_SEPARATOR);
        if(vfs->Exists(skinPath + slash))
        {
            skinPath += slash;
        }
        else if(vfs->Exists(skinPath + ".zip"))
        {
            skinPath += ".zip";
        }

        // Create the PAWS window manager
        csString skinPathBase = cfgmgr->GetStr("PlaneShift.GUI.Skin.Base","/planeshift/art/skins/base/client_base.zip");
        paws = new PawsManager(object_reg, skinPath, skinPathBase);

        options = new psOptions("/planeshift/userdata/options.cfg", vfs);

        // Default to maximum 71 fps
        // Actual fps get be up to 10 fps less so set a reasonably high limit
        maxFPS = cfgmgr->GetInt("Video.FrameLimit", 71);
        frameLimit = 1000 / maxFPS;

        mainWidget = new psMainWidget();
        paws->SetMainWidget(mainWidget);

        paws->GetMouse()->Hide(true);

        DeclareExtraFactories();

        if(!LoadSoundSettings(false))
        {
            return false;
        }

        if(! paws->LoadWidget("splash.xml"))
            return false;
        if(! paws->LoadWidget("ok.xml"))
            return false;
        if(! paws->LoadWidget("quitinfo.xml"))
            return false;

        LoadPawsWidget("Yes / No dialog","yesno.xml");

        //Load confirmation information for duels and marriages
        if(!LoadConfirmationSettings())
            return false;


        // Register our event handlers.
        event_frame = csevFrame(object_reg);
        event_canvashidden = csevCanvasHidden(object_reg,g2d);
        event_canvasexposed = csevCanvasExposed(object_reg,g2d);
        event_focusgained = csevFocusGained(object_reg);
        event_focuslost = csevFocusLost(object_reg);
        event_mouse = csevMouseEvent(object_reg);
        event_keyboard = csevKeyboardEvent(object_reg);
        event_joystick = csevJoystickEvent(object_reg);
        event_quit = csevQuit(object_reg);
#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
        event_selectionnotify = csevSelectionNotify(object_reg);
#endif
        eventHandlerLogic = csPtr<LogicEventHandler> (new LogicEventHandler(this));
        eventHandler2D = csPtr<EventHandler2D> (new EventHandler2D(this));
        eventHandler3D = csPtr<EventHandler3D> (new EventHandler3D(this));
        eventHandlerFrame = csPtr<FrameEventHandler> (new FrameEventHandler(this));

        csEventID esublog[] =
        {
            event_frame,
            event_canvashidden,
            event_canvasexposed,
            event_focusgained,
            event_focuslost,
            event_mouse,
            event_keyboard,
            event_joystick,
            event_quit,
#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
            event_selectionnotify,
#endif
            CS_EVENTLIST_END
        };

        queue->RegisterListener(eventHandlerLogic, esublog);
        queue->RegisterListener(eventHandler2D, event_frame);
        queue->RegisterListener(eventHandler3D, event_frame);
        queue->RegisterListener(eventHandlerFrame, event_frame);

        // Inform debug that everything initialized successfully
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
                 "psEngine initialized.");

        // Set up font for messages that bypass paws.
        iFontServer* fntsvr = g2d->GetFontServer();
        if(fntsvr)
        {
            font = fntsvr->LoadFont(CSFONT_LARGE);
        }

        return true;
    }
    else if(level == 1)
    {
        backgroundWorldLoading = /*false; // Hardcoded disable due to major issues.*/ psengine->GetConfig()->GetBool("PlaneShift.Loading.BackgroundWorldLoading");
        loader = csQueryRegistry<iBgLoader>(object_reg);
        scenemanipulator = scfQueryInterface<iSceneManipulate>(loader);

        // Check VFS mounts
        csString test_path;
        csRef<iDataBuffer> realPath;

        test_path = "/planeshift/materials/";
        realPath = vfs->GetRealPath(test_path);

        if(!realPath.IsValid())
        {
            Error2("Bad virtual path %s.", test_path.GetData());
            PS_PAUSEEXIT(1);
        }
        test_path = "/planeshift/meshes/";
        realPath = vfs->GetRealPath(test_path);

        if(!realPath.IsValid())
        {
            Error2("Bad virtual path %s.", test_path.GetData());
            PS_PAUSEEXIT(1);
        }
        test_path = "/planeshift/world/";
        realPath = vfs->GetRealPath(test_path);

        if(!realPath.IsValid())
        {
            Error2("Bad virtual path %s.", test_path.GetData());
            PS_PAUSEEXIT(1);
        }

        // Set progress bar.
        pawsProgressBar* progress = (pawsProgressBar*)paws->FindWidget("SplashProgress");
        meshes = vfs->FindFiles("/planeshift/meshes/");
        maps = vfs->FindFiles("/planeshift/world/");
        progress->SetTotalValue(2+meshes->GetSize()+maps->GetSize());
        progress->SetCurrentValue(0.0f);

        // Start to fill the loader cache.
        precaches.Push(loader->PrecacheData("/planeshift/materials/materials.cslib"));
        lastLoadingCount = 1;

        // Initialize Networking
        if(!netmanager)
        {
            netmanager.AttachNew(new psNetManager);

            if(!netmanager->Initialize(object_reg))
            {
                lasterror = "Couldn't init Network Manager.";
                return false;
            }
            psMessageCracker::msghandler = netmanager->GetMsgHandler();
        }

        inventoryCache = new psInventoryCache();
        guiHandler = new GUIHandler();
        celclient.AttachNew(new psCelClient());
        slotManager = new psSlotManager();
        songManager = new ClientSongManager();
        modehandler.AttachNew(new ModeHandler(celclient,netmanager->GetMsgHandler(),object_reg));
        actionhandler.AttachNew(new ActionHandler(netmanager->GetMsgHandler(), object_reg));
        zonehandler.AttachNew(new ZoneHandler(netmanager->GetMsgHandler(), celclient));
        questionclient = new psQuestionClient(GetMsgHandler(), object_reg);

        if(!zonehandler->IsValid())
        {
            lasterror = "Couldn't init Zone Handler.";
            Error2("FATAL ERROR: %s", lasterror.GetData());
            PS_PAUSEEXIT(1);
        }
        if(!celclient->Initialize(object_reg, GetMsgHandler()))
        {
            lasterror = "Couldn't init Cel Manager.";
            Error2("FATAL ERROR: %s",lasterror.GetData());
            PS_PAUSEEXIT(1);
        }


        if(!modehandler->Initialize())
        {
            lasterror = "ModeHandler failed init.";
            Error2("FATAL ERROR: %s",lasterror.GetData());
            PS_PAUSEEXIT(1);
        }

        BrightnessCorrection = options->GetOption("engine","brightness", 0.0f);
        UpdateLights();

        // Init the main widget
        mainWidget->SetupMain();

        if(!camera)
        {
            camera = new psCamera();
        }

        if(!charmanager)
        {
            charmanager = new psClientCharManager(object_reg);

            if(!charmanager->Initialize(GetMsgHandler(), celclient))
            {
                lasterror = "Couldn't init Character Manager.";
                return false;
            }
        }
        if(!autoexec)
        {
            autoexec = new Autoexec();
        }
    }
    else if(level == 2)
    {
        for(size_t i=0; i<precaches.GetSize(); ++i)
        {
            if(precaches[i]->IsFinished())
            {
                precaches.DeleteIndex(i);
                --i;
            }
        }

        pawsProgressBar* progress = (pawsProgressBar*)paws->FindWidget("SplashProgress");
        progress->SetCurrentValue(progress->GetCurrentValue()+(lastLoadingCount-precaches.GetSize()));
        lastLoadingCount = precaches.GetSize();

        if(lastLoadingCount != 0)
            return false;

        // Continue to fill the loader cache.
        for(size_t j=0; j<meshes->GetSize(); ++j)
        {
            precaches.Push(loader->PrecacheData(meshes->Get(j)));
        }

        lastLoadingCount = meshes->GetSize();
    }
    else if(level == 3)
    {
        for(size_t i=0; i<precaches.GetSize(); ++i)
        {
            if(precaches[i]->IsFinished())
            {
                precaches.DeleteIndex(i);
                --i;
            }
        }

        pawsProgressBar* progress = (pawsProgressBar*)paws->FindWidget("SplashProgress");
        progress->SetCurrentValue(progress->GetCurrentValue()+(lastLoadingCount-precaches.GetSize()));
        lastLoadingCount = precaches.GetSize();

        if(lastLoadingCount != 0)
            return false;

        // Continue to fill the loader cache.
        for(size_t j=0; j<maps->GetSize(); ++j)
        {
            precaches.Push(loader->PrecacheData(maps->Get(j)));
        }

        lastLoadingCount = maps->GetSize();
    }
    else if(level == 4)
    {
        for(size_t i=0; i<precaches.GetSize(); ++i)
        {
            if(precaches[i]->IsFinished())
            {
                precaches.DeleteIndex(i);
                --i;
            }
        }

        pawsProgressBar* progress = (pawsProgressBar*)paws->FindWidget("SplashProgress");
        progress->SetCurrentValue(progress->GetCurrentValue()+(lastLoadingCount-precaches.GetSize()));
        lastLoadingCount = precaches.GetSize();

        if(lastLoadingCount != 0)
            return false;

        // Load effects now, before we have actors
        if(!effectManager)
        {
            effectManager.AttachNew(new psEffectManager(object_reg));
        }

        if(!effectManager->LoadFromDirectory("/this/data/effects", true, camera->GetView()))
        {
            FatalError("Failed to load effects!");
            return false;
        }

        if(!paws->LoadWidget("charpick.xml"))
        {
            FatalError("Failed to load character picker!");
            return false;
        }

        meshes.Invalidate();
        maps.Invalidate();
        loader->ClearTemporaryData();
    }
    else if(level == 5)
    {
        return paws->LoadObjectViews();
    }

    return true;
}

void psEngine::LoadLogSettings()
{
    int count=0;
    for(int i=0; i< MAX_FLAGS; i++)
    {
        if(pslog::GetName(i))
        {
            pslog::SetFlag(pslog::GetName(i),cfgmgr->GetBool(pslog::GetSettingName(i)), 0);
            if((cfgmgr->GetBool(pslog::GetSettingName(i))))
            {
                count++;
            }

        }
    }
    if(count==0)
    {
        CPrintf(CON_CMDOUTPUT,"All LOGS are off.\n");
    }

}

void psEngine::DeclareExtraFactories()
{
    Debug1(LOG_PAWS,0,"psEngine::DeclareExtraFactories.");

    pawsWidgetFactory* factory;

    RegisterFactory(pawsInventoryDollViewFactory);
    RegisterFactory(pawsGlyphSlotFactory);
    RegisterFactory(pawsInfoWindowFactory);
    RegisterFactory(pawsSplashWindowFactory);
    RegisterFactory(pawsLoadWindowFactory);
    RegisterFactory(pawsChatWindowFactory);
    RegisterFactory(pawsInventoryWindowFactory);
    RegisterFactory(pawsItemDescriptionWindowFactory);
    RegisterFactory(pawsContainerDescWindowFactory);
    RegisterFactory(pawsInteractWindowFactory);
    RegisterFactory(pawsControlWindowFactory);
    RegisterFactory(pawsGroupWindowFactory);
    RegisterFactory(pawsExchangeWindowFactory);
    RegisterFactory(pawsSpellBookWindowFactory);
    RegisterFactory(pawsAttackBookWindowFactory);
    RegisterFactory(pawsGlyphWindowFactory);
    RegisterFactory(pawsMerchantWindowFactory);
    RegisterFactory(pawsStorageWindowFactory);
    RegisterFactory(pawsConfigWindowFactory);
    RegisterFactory(pawsConfigKeysFactory);
    RegisterFactory(pawsConfigPopupFactory);
    RegisterFactory(pawsConfigPvPFactory);
    RegisterFactory(pawsConfigMarriageFactory);
    RegisterFactory(pawsFingeringWindowFactory);
    RegisterFactory(pawsConfigDetailsFactory);
    RegisterFactory(pawsConfigMouseFactory);
    RegisterFactory(pawsConfigCameraFactory);
    RegisterFactory(pawsConfigChatFactory);
    RegisterFactory(pawsConfigSoundFactory);
    RegisterFactory(pawsConfigShortcutFactory);
    RegisterFactory(pawsConfigActiveMagicFactory);
    RegisterFactory(pawsConfigChatFontFactory);
    RegisterFactory(pawsConfigEntityLabelsFactory);
    RegisterFactory(pawsConfigEntityInteractionFactory);
    RegisterFactory(pawsConfigAutoexecFactory);
    RegisterFactory(pawsPetitionWindowFactory);
    RegisterFactory(pawsPetitionGMWindowFactory);
    RegisterFactory(pawsShortcutWindowFactory);
    RegisterFactory(pawsScrollMenuFactory);
    RegisterFactory(pawsDnDButtonFactory);
    RegisterFactory(pawsLoginWindowFactory);
    RegisterFactory(pawsCharacterPickerWindowFactory);
    RegisterFactory(pawsGuildWindowFactory);
    RegisterFactory(pawsLootWindowFactory);
    RegisterFactory(pawsCreationMainFactory);
    RegisterFactory(pawsCharBirthFactory);
    RegisterFactory(pawsCharParentsFactory);
    RegisterFactory(pawsChildhoodWindowFactory);
    RegisterFactory(pawsLifeEventWindowFactory);
    RegisterFactory(pawsPathWindowFactory);
    RegisterFactory(pawsSummaryWindowFactory);
    RegisterFactory(pawsSkillWindowFactory);
    RegisterFactory(pawsQuestListWindowFactory);
    RegisterFactory(pawsSpellCancelWindowFactory);
    RegisterFactory(pawsGmGUIWindowFactory);
    RegisterFactory(pawsMoneyFactory);
    RegisterFactory(pawsHelpFactory);
    RegisterFactory(pawsBuddyWindowFactory);
    RegisterFactory(pawsIgnoreWindowFactory);
    RegisterFactory(pawsSlotFactory);
    RegisterFactory(pawsActionLocationWindowFactory);
    RegisterFactory(pawsDetailWindowFactory);
    RegisterFactory(pawsCharDescriptionFactory);
    RegisterFactory(pawsQuestRewardWindowFactory);
    RegisterFactory(pawsCreditsWindowFactory);
    RegisterFactory(pawsQuitInfoBoxFactory);
    RegisterFactory(pawsGMSpawnWindowFactory);
    RegisterFactory(pawsModsWindowFactory);
    RegisterFactory(pawsSkillIndicatorFactory);
    RegisterFactory(pawsBookReadingWindowFactory);
    RegisterFactory(pawsWritingWindowFactory);
    RegisterFactory(pawsActiveMagicWindowFactory);
    RegisterFactory(pawsSmallInventoryWindowFactory);
    RegisterFactory(pawsConfigChatFilterFactory);
    RegisterFactory(pawsConfigChatTabsFactory);
    RegisterFactory(pawsConfigChatTabCompletionFactory);
    RegisterFactory(pawsConfigChatLogsFactory);
    RegisterFactory(pawsGMActionWindowFactory);
    RegisterFactory(pawsCraftWindowFactory);
    RegisterFactory(pawsPetStatWindowFactory);
    RegisterFactory(pawsTutorialWindowFactory);
    RegisterFactory(pawsTutorialNotifyWindowFactory);
    RegisterFactory(pawsSketchWindowFactory);
    RegisterFactory(pawsGameBoardFactory);
    RegisterFactory(pawsGameTileFactory);
    RegisterFactory(pawsBankWindowFactory);
    RegisterFactory(pawsConfigChatBubblesFactory);
    RegisterFactory(pawsConfigShadowsFactory);
    RegisterFactory(pawsConfigTextPageFactory);
    RegisterFactory(pawsNpcDialogWindowFactory);
    RegisterFactory(pawsCraftCancelWindowFactory);
    RegisterFactory(pawsConfigSpellCheckerFactory);
    RegisterFactory(pawsConfigTooltipsFactory);
    RegisterFactory(pawsMusicWindowFactory);
    RegisterFactory(pawsSheetLineFactory);
    RegisterFactory(pawsScriptHiredNPCWindowFactory);}

//-----------------------------------------------------------------------------

/**
 * These functions receive and process all events that occur when the game runs.
 * An event is classified by CS as anything that occurs, whether be it mouse move,
 * mouse click, messages, etc.
 */

bool psEngine::ProcessLogic(iEvent &ev)
{
    lastEvent = &ev;

    if(ev.Name == event_frame)
    {
        if(gameLoaded)
        {
            modehandler->PreProcess();
        }

        // If any objects or actors are enqueued to be created, create the next one this frame.
        if(celclient)
        {
            celclient->CheckEntityQueues();
        }

        UpdatePerFrame();
    }
    if(ev.Name == event_canvashidden)
    {
        drawScreen = false;
        if(soundOn)
        {
            MuteAllSounds();
        }
    }
    else if(ev.Name == event_canvasexposed)
    {
        drawScreen = true;
        if(soundOn)
        {
            UnmuteAllSounds();
        }
    }
    else if(ev.Name == event_focusgained)
    {
        if(GetMuteSoundsOnFocusLoss() && soundOn)
        {
            UnmuteAllSounds();
        }
    }
    else if(ev.Name == event_focuslost)
    {
        if(GetMuteSoundsOnFocusLoss() && soundOn)
        {
            MuteAllSounds();
        }
    }
    else if(ev.Name == event_quit)
    {
        // Disconnect and quit, if this event wasn't made here
        QuitClient();
    }
    else
    {
        bool handled = false;
        if(paws->HandleEvent(ev))
        {
            handled = true;
        }

        if(slotManager && !handled && slotManager->HandleEvent(ev))
        {
            handled = true;
        }

        if(charController)
        {
            if(!handled && charController->HandleEvent(ev))
            {
                handled = true;
            }

            if(handled && paws->GetFocusOverridesControls())
            {
                charController->GetMovementManager()->StopControlledMovement();
            }
        }

        return handled;
    }

    return false;
}

bool psEngine::Process3D(iEvent &ev)
{
    lastEvent = &ev;

    for(size_t i=0; i<delayedLoaders.GetSize(); ++i)
    {
        csWeakRef<DelayedLoader> dl = delayedLoaders.Get(i);
        if(!dl.IsValid())
        {
            delayedLoaders.DeleteIndex(i--);
            continue;
        }

        if(dl->CheckLoadStatus())
            break;
    }

    // Loading the game
    if(loadstate != LS_DONE)
    {
        LoadGame();
    }

    if(drawScreen)
    {
        // FPS limits
        FrameLimit();

        paws->Draw3D();
        if(camera && loadstate == LS_DONE)
        {
            camera->Draw();
            return true;
        }
    }
    else
    {
        // Sleep for a bit but don't draw anything if minimized or world not loaded.
        FrameLimit();
        return true;
    }

    return false;
}

bool psEngine::Process2D(iEvent &ev)
{
    lastEvent = &ev;

    if(drawScreen)
    {
        g3d->BeginDraw(CSDRAW_2DGRAPHICS);
        if(effectManager)
            effectManager->Render2D(g3d, g2d);
        paws->Draw();
        if(showFPS)
        {
            csString fpsDisplay;
            fpsDisplay.Format("%.2f", getFPS());
            g2d->Write(font, 5, 5, g2d->FindRGB(255, 255, 255), -1, fpsDisplay);
        }
    }

    return true;
}

bool psEngine::ProcessFrame(iEvent &ev)
{
    lastEvent = &ev;

    if(drawScreen)
        FinishFrame();

    // We need to call this after drawing was finished so
    // LoadingScreen had a chance to be displayed when loading
    // maps in-game
    if(zonehandler)
        zonehandler->OnDrawingFinished();

    return true;
}

// ----------------------------------------------------------------------------

const csHandlerID* psEngine::LogicEventHandler::GenericPrec(csRef<iEventHandlerRegistry> &handler_reg,
        csRef<iEventNameRegistry> &name_reg,
        csEventID e) const
{
    if(e != csevFrame(name_reg))
        return 0;
    static csHandlerID precConstraint[2] =
    {
        handler_reg->GetGenericID("planeshift.clientmsghandler"),
        CS_HANDLERLIST_END
    };
    return precConstraint;
}

const csHandlerID* psEngine::LogicEventHandler::GenericSucc(csRef<iEventHandlerRegistry> &handler_reg,
        csRef<iEventNameRegistry> &name_reg,
        csEventID e) const
{
    if(e != csevFrame(name_reg))
        return 0;
    static csHandlerID succConstraint[6] =
    {
        FrameSignpost_Logic3D::StaticID(handler_reg),
        FrameSignpost_3D2D::StaticID(handler_reg),
        FrameSignpost_2DConsole::StaticID(handler_reg),
        FrameSignpost_ConsoleDebug::StaticID(handler_reg),
        FrameSignpost_DebugFrame::StaticID(handler_reg),
        CS_HANDLERLIST_END
    };
    return succConstraint;
}

// ----------------------------------------------------------------------------

inline void psEngine::FrameLimit()
{
    csTicks sleeptime;

    // Find the time taken since we left this function
    csTicks elapsedTime = csGetTicks() - elapsed;

    // If we're loading then do special handling.
    if(zonehandler.IsValid() && zonehandler->IsLoading())
    {
        sleeptime = 1000/30;
    }
    // Define sleeptimes
    else if(!camera)
    {
        // Get the window
        static pawsWidget* credits = NULL;
        if(!credits)
            credits = paws->FindWidget("CreditsWindow", false);

        if(credits && credits->IsVisible())
            sleeptime = 1000/60;
        else
            sleeptime = 1000/30;
    }
    else
    {
        sleeptime = frameLimit;
    }

    // Here we sacrifice drawing AND loading time
    if(elapsedTime < sleeptime)
        csSleep(sleeptime - elapsedTime);

    timeFPS -= elapsed;
    elapsed = csGetTicks();

    timeFPS += elapsed;
    countFPS++;
    if(timeFPS > 500)
    {
        currFPS = 1000.0 * countFPS / timeFPS;
        timeFPS = 0;
        countFPS = 0;
    }
}

void psEngine::MuteAllSounds(void)
{
    SoundManager->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->Mute();
}

void psEngine::UnmuteAllSounds(void)
{
    SoundManager->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->Unmute();
}

// ----------------------------------------------------------------------------

const char* psEngine::FindCommonString(unsigned int cstr_id)
{
    csStringHashReversible* strings = GetMsgStrings();
    if(!strings)
        return "";

    return strings->Request(cstr_id);
}

csStringID psEngine::FindCommonStringId(const char* str)
{
    csStringHashReversible* strings = GetMsgStrings();
    if(!strings)
        return csInvalidStringID;

    return strings->Request(str);
}

csStringHashReversible* psEngine::GetMsgStrings()
{
    if(!celclient || !celclient->GetClientDR())
        return NULL;

    return celclient->GetClientDR()->GetMsgStrings();
}

// ----------------------------------------------------------------------------

inline void psEngine::UpdatePerFrame()
{
    if(!celclient)
        return;

    // Must be in PostProcess for accurate DR updates
    if(celclient->GetClientDR())
        celclient->GetClientDR()->CheckDeadReckoningUpdate();

    if(celclient->GetMainPlayer())
    {
        celclient->GetClientDR()->CheckSectorCrossing(celclient->GetMainPlayer());
        celclient->PruneEntities();    // Prune CD-intensive entities by disabling CD

        // Update Stats for Player
        celclient->GetMainPlayer()->GetVitalMgr()->Predict(csGetTicks(),"Self");

        // Update stats for Target if Target is there and has stats
        GEMClientObject* target = charmanager->GetTarget();
        if(target && target->GetType() != -2)
        {
            GEMClientActor* actor = dynamic_cast<GEMClientActor*>(target);
            if(actor)
                actor->GetVitalMgr()->Predict(csGetTicks(),"Target");
        }
    }

    if(celclient)
        celclient->Update(HasLoadedMap());

    if(effectManager)
        effectManager->Update();

    if(songManager)
        songManager->Update();

    // Update the sound system
    if(GetSoundStatus()
            && loadstate == LS_DONE)
    {
        GEMClientActor* mainPlayer = celclient->GetMainPlayer();
        csVector3 vel = mainPlayer->GetVelocity();
        vel = mainPlayer->GetMesh()->GetMovable()->GetTransform().GetT2O() * vel;

        SoundManager->UpdateListener(camera->GetView());
        SoundManager->SetPlayerMovement(mainPlayer->Pos(), vel);
    }

    /* sound is updated EVERY FRAME, doesnt matter if 2D or 3D
     * it will slow itself down to save cputime
     */

    SoundManager->Update();
}

// ----------------------------------------------------------------------------

void psEngine::QuitClient()
{
    // Only run through the shut down procedure once
    static bool alreadyQuitting = false;
    if(alreadyQuitting)
    {
        return;
    }

    alreadyQuitting = true;

    loadstate = LS_NONE;

    csRef<iConfigManager> cfg(csQueryRegistry<iConfigManager> (object_reg));
    if(cfg)
    {
        cfg->Save();
    }

    Disconnect();

    queue->GetEventOutlet()->Broadcast(event_quit);
}

void psEngine::Disconnect()
{
    if (netmanager)
        netmanager->Disconnect();
}

//----------------------------------------------------------------------------

const char* psEngine::GetMainPlayerName()
{
    return GetCelClient()->GetMainPlayerName();
}

// ----------------------------------------------------------------------------

bool psEngine::UpdateWindowTitleInformations()
{
    return CS_Setup->AddWindowInformations(GetMainPlayerName());
}

// ----------------------------------------------------------------------------

void psEngine::AddLoadingWindowMsg(const csString &msg)
{
    pawsLoadWindow* window = static_cast <pawsLoadWindow*>(paws->FindWidget("LoadWindow"));
    if(window != NULL)
    {
        window->AddText(paws->Translate(msg));
        ForceRefresh();
    }
}

/*
 * When loading this is called several times whenever there is a spare moment
 * in the event queue.  When it is called it does one of the cases and breaks out.
 */
void psEngine::LoadGame()
{
    switch(loadstate)
    {
        case LS_ERROR:
        case LS_NONE:
            return;

        case LS_LOAD_SCREEN:
        {
            paws->LoadWidget("loadwindow.xml");

            pawsLoadWindow* window = dynamic_cast <pawsLoadWindow*>(paws->FindWidget("LoadWindow"));
            if(!window)
            {
                FatalError("Widget: LoadWindow could not be found or is not a pawsLoadWindow widget.");
                return;
            }

            AddLoadingWindowMsg("Loading...");
            ForceRefresh();

            // Request MOTD
            psMOTDRequestMessage motdRe;
            GetMsgHandler()->SendMessage(motdRe.msg);

            pawsWidget* cpw = paws->FindWidget("CharPickerWindow");
            cpw->DeleteYourself();

            loadstate = LS_INIT_ENGINE;
            break;
        }

        case LS_INIT_ENGINE:
        {
            if(charController == NULL)
            {
                charController = new psCharController(nameRegistry);
                if(!charController->Initialize())
                {
                    FatalError("Couldn't initialize the character controller!");
                    return;
                }
            }

            // load chat bubbles
            if(!chatBubbles)
            {
                chatBubbles = new psChatBubbles();
                chatBubbles->Initialize(this);
            }

            loadstate = LS_REQUEST_WORLD;
            break;
        }

        case LS_REQUEST_WORLD:
        {
            if(!charController->IsReady())
                return;  // Wait for character modes

            celclient->RequestServerWorld();

            loadtimeout = csGetTicks() + cfgmgr->GetInt("PlaneShift.Client.User.Persisttimeout", 60) * 1000;

            AddLoadingWindowMsg("Requesting connection");
            ForceRefresh();

            loadstate = LS_SETTING_CHARACTERS;
            break;
        }

        case LS_SETTING_CHARACTERS:
        {
            // Wait for the map to be loaded
            if(!HasLoadedMap())
                return;

            // Request the actor.
            if(!actorRequested)
            {
                celclient->RequestActor();
                actorRequested = true;
            }

            if(!celclient->IsReady())
            {
                if(celclient->GetRequestStatus() != 0 && csGetTicks() > loadtimeout)
                {
                    csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
                             "PSLoader: timeout!");

                    FatalError("Timeout waiting for the world to load");
                }

                // We don't have the main actor or world yet
                return;
            }

            // Set controlled actor and map controls
            charController->GetMovementManager()->SetActor(NULL);

            // Init camera with controlled actor
            camera->InitializeView(celclient->GetMainPlayer());

            // Get stats
            psStatsMessage statmsg;
            GetMsgHandler()->SendMessage(statmsg.msg);

            AddLoadingWindowMsg("Getting entities");
            celclient->GetMainPlayer()->SendDRUpdate(0, celclient->GetClientDR()->GetMsgStrings());

            AddLoadingWindowMsg("Loading GUI");
            ForceRefresh();

            loadstate = LS_CREATE_GUI;
            break;
        }

        case LS_CREATE_GUI:
        {
            // Must be first!!!
            if(!paws->LoadWidget("control.xml"))
            {
                GetMainWidget()->LockPlayer();
                FatalError("The toolbar couldn't be loaded\nPlease check your logs");
                return;
            }



            LoadPawsWidget( "Attack Window",           "attackwindow.xml" );
            LoadPawsWidget("Status window",           "infowindow.xml");
            LoadPawsWidget("Ignore window",           "ignorewindow.xml");
            LoadPawsWidget("Communications window",   "chat.xml");
            LoadPawsWidget("Inventory window",        "inventory.xml");
            LoadPawsWidget("Item description window", "itemdesc.xml");
            LoadPawsWidget("Container description window","containerdesc.xml");
            LoadPawsWidget("Book Reading window", "readbook.xml");
            LoadPawsWidget("Interact menu",           "interact.xml");
            LoadPawsWidget("Group status window",     "group.xml");
            LoadPawsWidget("Exchange window",         "exchange.xml");
            LoadPawsWidget("Glyph window",            "glyph.xml");
            LoadPawsWidget("Merchant window",         "merchant.xml");
            LoadPawsWidget("Storage window",          "storage.xml");
            LoadPawsWidget("Petition window",         "petition.xml");
            LoadPawsWidget("Petititon GM window",     "petitiongm.xml");
            LoadPawsWidget("Spellbook window",        "spellwindow.xml");
            LoadPawsWidget("Shortcut window",         "shortcutwindow.xml");
            LoadPawsWidget("GM GUI window",           "gmguiwindow.xml");
            LoadPawsWidget("Control panel",           "configwindow.xml");
            LoadPawsWidget("Fingering window",        "fingering.xml");
            LoadPawsWidget("Guild information window","guildwindow.xml");
            LoadPawsWidget("Loot window",             "loot.xml");
            LoadPawsWidget("Skills window",           "skillwindow.xml");
            LoadPawsWidget("PetStats window",         "petstatwindow.xml");
            LoadPawsWidget("Quest notebook window",   "questnotebook.xml");
            LoadPawsWidget("Spell cast status window","spellcancelwindow.xml");
            LoadPawsWidget("Help window",             "helpwindow.xml");
            LoadPawsWidget("Buddy window",            "buddy.xml");
            LoadPawsWidget("Action Location window",  "actionlocation.xml");
            LoadPawsWidget("Details window",          "detailwindow.xml");
            LoadPawsWidget("Character description window","chardescwindow.xml");
            LoadPawsWidget("Quest reward window",     "questrewardwindow.xml");
            LoadPawsWidget("GM Spawn interface",      "gmspawn.xml");
            LoadPawsWidget("GM Spawn modifier",       "modswindow.xml");
            LoadPawsWidget("Active Magic window",     "activemagicwindow.xml");
            LoadPawsWidget("Small Inventory Window",  "smallinventory.xml");
            LoadPawsWidget("GM Action Location Edit", "gmaddeditaction.xml");
            LoadPawsWidget("Crafting",                "craft.xml");
            LoadPawsWidget("Tutorial",                "tutorial.xml");
            LoadPawsWidget("Sketch",                  "illumination.xml");
            LoadPawsWidget("GameBoard",               "gameboard.xml");
            LoadPawsWidget("Writing window",          "bookwriting.xml");
            LoadPawsWidget("NPC dialog window",       "dialog.xml");
            LoadPawsWidget("Craft status window",     "craftcancelwindow.xml");
            LoadPawsWidget("Music window",            "musicwindow.xml");
            LoadPawsWidget("Script Hired NPC window", "scripthirednpcwindow.xml");

            LoadCustomPawsWidgets("/data/gui/customwidgetslist.xml");

            HideWindow("DescriptionEdit");
            HideWindow("ItemDescWindow");
            HideWindow("BookReadingWindow");
            HideWindow("ContainerDescWindow");
            HideWindow("GroupWindow");
            HideWindow("InteractWindow");
            HideWindow("ExchangeWindow");
            HideWindow("MerchantWindow");
            HideWindow("StorageWindow");
            HideWindow("ShortcutEdit");
            HideWindow("FingeringWindow");
            HideWindow("QuestEdit");
            HideWindow("LootWindow");
            HideWindow("ActionLocationWindow");
            HideWindow("DetailWindow");
            HideWindow("QuestRewardWindow");
            HideWindow("SpellCancelWindow");
            HideWindow("GMSpawnWindow");
            HideWindow("ModsWindow");
            HideWindow("AddEditActionWindow");
            HideWindow("GmGUI");
            HideWindow("PetStatWindow");
            HideWindow("WritingWindow");
            HideWindow("CraftCancelWindow");
            HideWindow("ScriptHiredNPCWindow");
            HideWindow("NPCDialogWindow");


            paws->GetMouse()->ChangeImage("Skins Normal Mouse Pointer");

            // If we had any problems, show them
            if(wdgProblems.GetSize() > 0)
            {
                printf("Following widgets failed to load:\n");

                // Loop through the array
                for(size_t i = 0; i < wdgProblems.GetSize(); i++)
                {
                    csString str = wdgProblems.Get(i);
                    if(str.Length() > 0)
                        printf("%s\n",str.GetData());
                }

                GetMainWidget()->LockPlayer();
                FatalError("One or more widgets failed to load\nPlease check your logs");
                return;
            }

            psClientStatusMessage statusmsg(true);
            statusmsg.SendMessage();

            // load the mouse options if not loaded already. The GetMouseBinds
            // function will load them if they are requested before this code
            // is executed.
            if(!mouseBinds)
            {
                mouseBinds = GetMouseBinds();
            }

            // run autoexec commands for this char
            if(autoexec->GetEnabled())
            {
                autoexec->execute();
            }

            // Set the focus to the main widget
            paws->SetCurrentFocusedWidget(GetMainWidget());

            // Enable render2texture (disabled until fully working).
            //paws->UseR2T(true);

            loadstate = LS_DONE;
            break;
        }


        default:
            loadstate = LS_NONE;
    }

    csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, PSAPP,
             "PSLoader: step %d: success", loadstate);

    if(loadstate==LS_DONE)
    {
        gameLoaded = true;
        paws->FindWidget("LoadWindow")->Hide();
    }
}

void psEngine::HideWindow(const csString &widgetName)
{
    pawsWidget* wnd = paws->FindWidget(widgetName);
    if(wnd != NULL)
        wnd->Hide();
}

psMouseBinds* psEngine::GetMouseBinds()
{
    // If not loaded load the mouse binds. They should normaly be loaded
    // from the load GUI step, but this function have been observerd called
    // before.
    if(!mouseBinds)
    {
        mouseBinds = new psMouseBinds();

        csString fileName = "/planeshift/userdata/options/mouse.xml";
        if(!vfs->Exists(fileName))
        {
            fileName = "/planeshift/data/options/mouse_def.xml";
        }

        if(!mouseBinds->LoadFromFile(object_reg, fileName))
            Error1("Failed to load mouse options");
    }
    return mouseBinds;
}

size_t psEngine::GetTime()
{
    return modehandler->GetTime();
}

void psEngine::SetBrightnessCorrection(float B)    
{
    BrightnessCorrection = B;

    options->SetOption("engine","brightness", BrightnessCorrection);
}

void psEngine::AdjustBrightnessCorrectionUp()
{
    csString sysMsg;

    float brightnessCorrection = psengine->GetBrightnessCorrection();

    if(brightnessCorrection < 3.0f)
    {
        brightnessCorrection += 0.1f;
    }
    if (brightnessCorrection > 3.0)
    {
        brightnessCorrection = 3.0;
    }

    psengine->SetBrightnessCorrection(brightnessCorrection);
    psengine->UpdateLights();

    sysMsg.Format("Brightness correction: %0.1f", brightnessCorrection);
    psSystemMessage ackMsg(0, MSG_OK, sysMsg);
    ackMsg.FireEvent();
}

void psEngine::AdjustBrightnessCorrectionDown()
{
    csString sysMsg;

    float brightnessCorrection = psengine->GetBrightnessCorrection();

    if(brightnessCorrection > -1.0f)
    {
        brightnessCorrection -= 0.1f;
    }
    if (brightnessCorrection < -1.0f)
    {
        brightnessCorrection = -1.0f;
    }

    psengine->SetBrightnessCorrection(brightnessCorrection);
    psengine->UpdateLights();

    sysMsg.Format("Brightness correction: %0.1f", brightnessCorrection);
    psSystemMessage ackMsg(0, MSG_OK, sysMsg);
    ackMsg.FireEvent();
}

void psEngine::ResetBrightnessCorrection()
{
    psengine->SetBrightnessCorrection(0.0f);
    psengine->UpdateLights();
 
 
    psSystemMessage ackmsg(0, MSG_OK, "Brightness Reset!");
    ackmsg.FireEvent();
}


void psEngine::UpdateLights()
{
    modehandler->UpdateLights();
}

void psEngine::WriteConfirmationSettings()
{
    csString xml;
    xml = "<Confirmation>\n";
    xml += "    <PvP value=\"";
    xml.Append(duelConfirmation);
    xml += "\"/>\n";
    xml += "    <Marriage value=\"";
    xml.Append(marriageProposal);
    xml += "\"/>\n";
    xml += "</Confirmation>\n";

    vfs->WriteFile("/planeshift/userdata/options/confirmation.xml", xml.GetData(), xml.Length());
}

bool psEngine::LoadConfirmationSettings()
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, mainNode, optionNode;

    csString fileName = "/planeshift/userdata/options/confirmation.xml";
    if(!vfs->Exists(fileName))
    {
        fileName = "/planeshift/data/options/confirmation_def.xml";
    }

    doc = ParseFile(object_reg, fileName);
    if(doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("confirmation.xml has no XML root");
        return false;
    }
    mainNode = root->GetNode("Confirmation");
    if(mainNode == NULL)
    {
        Error1("confirmation.xml has no <Confirmation> tag");
        return false;
    }

    optionNode = mainNode->GetNode("PvP");
    if(optionNode != NULL)
    {
        duelConfirmation = optionNode->GetAttributeValueAsInt("value");
    }
    optionNode = mainNode->GetNode("Marriage");
    if(optionNode != NULL)
    {
        marriageProposal = optionNode->GetAttributeValueAsInt("value") != 0;
    }

    return true;
}

bool psEngine::LoadSoundSettings(bool forceDef)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, mainNode, optionNode;

    csString fileName;
    if(!forceDef)
        fileName = "/planeshift/userdata/options/sound.xml";

    if(forceDef || !vfs->Exists(fileName))
    {
        fileName = "/planeshift/data/options/sound_def.xml";
    }

    doc = ParseFile(object_reg, fileName);
    if(doc == NULL)
    {
        Error2("Failed to parse file %s", fileName.GetData());
        return false;
    }
    root = doc->GetRoot();
    if(root == NULL)
    {
        Error2("%s has no XML root",fileName.GetData());
        return false;
    }
    mainNode = root->GetNode("sound");
    if(mainNode == NULL)
    {
        Error2("%s has no <sound> tag",fileName.GetData());
        return false;
    }

    // load and apply the settings
    optionNode = mainNode->GetNode("ambient");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("actions");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("effects");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("music");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("gui");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("voices");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on",true));

    optionNode = mainNode->GetNode("instruments");
    if(optionNode != NULL)
        SoundManager->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->SetToggle(optionNode->GetAttributeValueAsBool("on", true));

    optionNode = mainNode->GetNode("volume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::MAIN_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("musicvolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::MUSIC_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("ambientvolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::AMBIENT_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("actionsvolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::ACTION_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("effectsvolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::EFFECT_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("guivolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::GUI_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("voicesvolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::VOICE_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("instrumentsvolume");
    if(optionNode != NULL)
    {
        int volume = optionNode->GetAttributeValueAsInt("value", 100);
        SoundManager->GetSndCtrl(iSoundManager::INSTRUMENT_SNDCTRL)->SetVolume((float)volume / 100);
    }

    optionNode = mainNode->GetNode("muteonfocusloss");
    if(optionNode != NULL)
        SetMuteSoundsOnFocusLoss(optionNode->GetAttributeValueAsBool("on", false));

    optionNode = mainNode->GetNode("loopbgm");
    if(optionNode)
        SoundManager->SetLoopBGMToggle(optionNode->GetAttributeValueAsBool("on", false));

    optionNode = mainNode->GetNode("combatmusic");
    if(optionNode)
        SoundManager->SetCombatMusicToggle(optionNode->GetAttributeValueAsBool("on", true));

    optionNode = mainNode->GetNode("chatsound");
    if(optionNode)
        SoundManager->SetChatToggle(optionNode->GetAttributeValueAsBool("on", true));

    optionNode = mainNode->GetNode("usecamerapos");
    if(optionNode)
        SoundManager->SetListenerOnCameraToggle(optionNode->GetAttributeValueAsBool("on", true));
    return true;
}

bool psEngine::LoadPawsWidget(const char* title, const char* filename)
{
    bool loaded = paws->LoadWidget(filename);

    if(!loaded)
    {
        wdgProblems.Push(title);
    }

    return loaded;
}

bool psEngine::LoadCustomPawsWidgets(const char* filename)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root;
    const char* error;

    csRef<iDataBuffer> buff = vfs->ReadFile(filename);
    if(buff == NULL)
    {
        Error2("Could not find file: %s", filename);
        return false;
    }
    doc = xmlparser->CreateDocument();
    assert(doc);
    error = doc->Parse(buff);
    if(error)
    {
        Error3("Parse error in %s: %s", filename, error);
        return false;
    }
    if(doc == NULL)
        return false;

    root = doc->GetRoot();
    if(root == NULL)
    {
        Error2("No root in XML %s", filename);
        return false;
    }

    csRef<iDocumentNode> customWidgetsNode = root->GetNode("custom_widgets");
    if(!customWidgetsNode)
    {
        Error2("No custom_widgets node in %s", filename);
        return false;
    }

    csRef<iDocumentNodeIterator> nodes = customWidgetsNode->GetNodes("widget");
    while(nodes->HasNext())
    {
        csRef<iDocumentNode> widgetNode = nodes->Next();

        csString name = widgetNode->GetAttributeValue("name");
        csString file = widgetNode->GetAttributeValue("file");

        LoadPawsWidget(name, file);
    }
    return true;
}

void psEngine::FatalError(const char* msg)
{
    loadstate = LS_ERROR;
    Bug2("%s\n", msg);

    pawsQuitInfoBox* quitinfo = (pawsQuitInfoBox*)(paws->FindWidget("QuitInfoWindow"));
    if(quitinfo)
    {
        quitinfo->SetBox(msg);
        quitinfo->Show();
    }
    else
    {
        // Force crash to give dump here
        int* crash = NULL;
        *crash=0;
        QuitClient();
    }
}

void psEngine::setLimitFPS(int a)
{
    maxFPS = a;
    frameLimit = (1000 / maxFPS);
    cfgmgr->SetInt("Video.frameLimit", maxFPS);
    cfgmgr->Save();// need to save this every time
}

// ----------------------------------------------------------------------------

/**
 * pureCallHandler()
 * Handles all Pure Virtual Function calls for MSVC.
 */
#ifdef CS_COMPILER_MSVC
void pureCallHandler()
{
    CS::Debug::AssertMessage("false", __FILE__, __LINE__, "Pure Virtual Function Call!\n");
}
#endif

/**
 * main()
 * Main function. Like any other applications, this is the entry point of the
 * program. The SysSystemDriver is created and initialized here. It also
 * creates psEngine and initialize it. Then it calls the main loop.
 **/

psEngine* psengine;

int main(int argc, char* argv[])
{
#ifdef CS_COMPILER_MSVC
    _set_purecall_handler(pureCallHandler);
#endif
    psCSSetup* CSSetup = new psCSSetup(argc, argv, "/this/psclient.cfg", CONFIGFILENAME);
    iObjectRegistry* object_reg = CSSetup->InitCS();

    pslog::Initialize(object_reg);
    pslog::disp_flag[LOG_LOAD] = true;

    // Create our application object
    psengine = new psEngine(object_reg, CSSetup);


    // Initialize engine
    if(!psengine->Initialize(0))
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_ERROR,
                 PSAPP, "Failed to init app!");
        PS_PAUSEEXIT(1);
    }

    // start the main event loop
    csDefaultRunLoop(object_reg);

    psengine->Cleanup();


    Notify1(LOG_ANY,"Removing engine...");

    // remove engine before destroying Application
    delete psengine;
    psengine = NULL;

    // Free registered message factories.
    psfUnRegisterMsgFactories();

    delete CSSetup;

    Notify1(LOG_ANY,"Destroying application...");

    csInitializer::DestroyApplication(object_reg);

    return 0;
}
