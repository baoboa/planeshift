/*
 * psengine.h
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
#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <csutil/csstring.h>
#include <csutil/strhashr.h>
#include <csutil/randomgen.h>
#include <csutil/refarr.h>
#include <csutil/sysfunc.h>
#include <csutil/leakguard.h>
#include <csutil/eventhandlers.h>
#include <csutil/weakrefarr.h>
#include <csutil/weakreferenced.h>
#include <ivaria/profile.h>
#include <ivideo/graph3d.h>
#include <iutil/eventh.h>

#include "psnetmanager.h"
#include "isoundmngr.h"

#include "util/slots.h"

class psCelClient;
class ClientMsgHandler;
class psClientCharManager;
struct iBgLoader;
struct iConfigManager;
struct iDialogManager;
struct iDocumentSystem;
struct iEngine;
struct iEvent;
struct iEventQueue;
struct iFont;
struct iGraphics2D;
struct iGraphics3D;
struct iSceneManipulate;
struct iTextureManager;
struct iThreadReturn;
struct iVFS;
struct iVirtualClock;
class ModeHandler;
class ActionHandler;
class ZoneHandler;
class psCharController;
class psCamera;
class psEffectManager;
class psChatBubbles;
class psCal3DCallbackLoader;
class psMainWidget;
class psQuestionClient;
class psOptions;
class psCSSetup;
class psMouseBinds;
class psInventoryCache;
class PawsManager;
class Autoexec;

// Networking classes
class ClientMsgHandler;
class psNetManager;
class psSlotManager;
class ClientSongManager;

class GUIHandler;

struct DelayedLoader : public CS::Utility::WeakReferenced
{
    virtual bool CheckLoadStatus() = 0;
    virtual ~DelayedLoader() { };
};

/**
* psEngine
* This is the main class that contains all the object. This class responsible
* for the whole game functionality.
*/
class psEngine
{
public:
    enum LoadState
    {
        LS_NONE,
        LS_LOAD_SCREEN,
        LS_INIT_ENGINE,
        LS_REQUEST_WORLD,
        LS_SETTING_CHARACTERS,
        LS_CREATE_GUI,
        LS_DONE,
        LS_ERROR
    };

    /// Default constructor. It calls csApp constructor.
    psEngine(iObjectRegistry* object_reg, psCSSetup* CSSetup);

    /// Destructor
    virtual ~psEngine();

    /** Creates and loads other interfaces.  There are 2 levels for the setup.
    * @param level The level we want to load
    * At level 0:
    * <UL>
    * <LI> Various Crystal Space plugins are loaded.
    * <LI> Crash/Dump system intialized.
    * <LI> Sound manager is loaded.
    * <LI> Log system loaded.
    * <LI> PAWS windowing system and skins setup.
    * </UL>
    * At level 1:
    * <UL>
    * <LI> The network manager is setup.
    * <LI> SlotManager, ModeManager, ActionHandler, ZoneManager, CacheManager setup.
    * <LI> CEL setup.
    * <LI> Trigger to start loading models fired.
    * </UL>
    *
    * @return  True if the level was successful in loading/setting up all components.
    */
    bool Initialize(int level);

    /**
    * Clean up stuff.
    * Needs to be called before the engine is destroyed, since some objects
    * owned by the engine require the engine during their destruction.
    */
    void Cleanup();

    /// Do anything that needs to happen after the frame is rendered each time.
    void UpdatePerFrame();

    /// Wait to finish drawing the current frame.
    void FinishFrame()
    {
        g3d->FinishDraw();
        g3d->Print(NULL);
    }

    /**
    * Everything is event base for csApp based system. This method is called
    * when any event is triggered such as when a button is pushed, mouse move,
    * or keyboard pressed.
    */
    bool ProcessLogic(iEvent &Event);
    bool Process2D(iEvent &Event);
    bool Process3D(iEvent &Event);
    bool ProcessFrame(iEvent &Event);

    /// Load game calls LoadWorld and also load characters, cameras, items, etc.
    void LoadGame();

    /// check if the game has been loaded or not
    bool IsGameLoaded()
    {
        return gameLoaded;
    };

    // References to plugins
    iObjectRegistry*      GetObjectRegistry()
    {
        return object_reg;
    }
    iEventNameRegistry*   GetEventNameRegistry()
    {
        return nameRegistry;
    }
    iEngine*              GetEngine()
    {
        return engine;
    }
    iGraphics3D*          GetG3D()
    {
        return g3d;
    }
    iGraphics2D*          GetG2D()
    {
        return g2d;
    }
    iTextureManager*      GetTextureManager()
    {
        return txtmgr;
    }
    iVFS*                 GetVFS()
    {
        return vfs;
    }
    iVirtualClock*        GetVirtualClock()
    {
        return vc;
    }
    iDocumentSystem*      GetXMLParser()
    {
        return xmlparser;
    }
    iConfigManager*       GetConfig()
    {
        return cfgmgr;    ///< config file
    }
    iBgLoader*            GetLoader()
    {
        return loader;
    }
    iSceneManipulate*     GetSceneManipulator()
    {
        return scenemanipulator;
    }
    iSoundManager*        GetSoundManager()
    {
        return SoundManager;
    }

    csRandomGen &GetRandomGen()
    {
        return random;
    }
    float GetRandom()
    {
        return random.Get();
    }

    ClientMsgHandler*    GetMsgHandler()
    {
        return netmanager->GetMsgHandler();
    }
    CmdHandler*            GetCmdHandler()
    {
        return netmanager->GetCmdHandler();
    }
    psSlotManager*         GetSlotManager()
    {
        return slotManager;
    }
    ClientSongManager*     GetSongManager()
    {
        return songManager;
    }
    psClientCharManager*   GetCharManager()
    {
        return charmanager;
    }
    psCelClient*           GetCelClient()
    {
        return celclient;
    };
    psMainWidget*          GetMainWidget()
    {
        return mainWidget;
    }
    psEffectManager*       GetEffectManager()
    {
        return effectManager;
    }
    psChatBubbles*         GetChatBubbles()
    {
        return chatBubbles;
    }
    psOptions*             GetOptions()
    {
        return options;
    }
    ModeHandler*           GetModeHandler()
    {
        return modehandler;
    }
    ActionHandler*         GetActionHandler()
    {
        return actionhandler;
    }
    psCharController*      GetCharControl()
    {
        return charController;
    }
    psMouseBinds*          GetMouseBinds();
    psCamera*              GetPSCamera()
    {
        return camera;
    }
    psNetManager*          GetNetManager()
    {
        return netmanager;
    }
    psCSSetup*             GetCSSetup()
    {
        return CS_Setup;
    }
    ZoneHandler*           GetZoneHandler()
    {
        return zonehandler;
    }
    Autoexec*              GetAutoexec()
    {
        return autoexec;
    }

    /**
     * Access the player's petitioner target.
     */
    void SetTargetPetitioner(const char* pet)
    {
        targetPetitioner = pet;
    }
    
    const char* GetTargetPetitioner()
    {
        return targetPetitioner;
    }

    /**
     * Quits client.
     */
    void QuitClient();

    /**
     * Formally disconnects a client and allows time for an info box to be shown.
     */
    void Disconnect();

    /**
     * Tell the engine to start the load proceedure.
     */
    void StartLoad()
    {
        loadstate = LS_LOAD_SCREEN;
    }
    LoadState loadstate;

    size_t GetTime();

    /**
     * Get the current brightness correlation value.
     */
    float GetBrightnessCorrection()
    {
        return BrightnessCorrection;
    }
    
    /**
     * Set the current brightness correlation value.
     */
    void SetBrightnessCorrection(float B);

    /**
     * Adjust brightness up and inform user.
     */
    void AdjustBrightnessCorrectionUp();

    /**
     * Adjust brightness down and inform user.
     */
    void AdjustBrightnessCorrectionDown();

    /**
     * Reset brightness and inform user.
     */
    void ResetBrightnessCorrection();
    
    void UpdateLights();

    float GetKFactor()
    {
        return KFactor;
    }
    void SetKFactor(float K)
    {
        KFactor = K;
    }

    iEvent* GetLastEvent()
    {
        return lastEvent;
    }

    // Functions to set and get frameLimit converted into a value of FPS for users
    void setLimitFPS(int);
    int getLimitFPS()
    {
        return maxFPS;
    }
    float getFPS()
    {
        return currFPS;
    }

    bool toggleFPS()
    {
        showFPS = !showFPS;
        return showFPS;
    }

    /**
     * Sets the duel confirm type and updates the confirmation settings XML.
     *
     * @brief Sets duel confirm type
     * @param confirmType Updates \ref duelConfirmation with this value
     */
    inline void SetDuelConfirm(int confirmType)
    {
        duelConfirmation = confirmType;
        WriteConfirmationSettings();
    }
    /**
     * @brief Simply returns the duel confirmation setting
     * @return \ref duelConfirmation
     */
    inline int GetDuelConfirm() const
    {
        return duelConfirmation;
    }
    /**
     * Sets if marriage proposals should be ignored and updates the confirmation settings XML.
     * @brief Sets if marriage proposals should be ignored
     * @param ignore Updates \ref marriageProposal with this value
     */
    inline void SetMarriageProposal(bool ignore)
    {
        marriageProposal = ignore;
        WriteConfirmationSettings();
    }
    /**
     * @brief Simply returns if marriage proposals should be read or automatically ignored
     * @return \ref marriageProposal
     */
    inline bool GetMarriageProposal() const
    {
        return marriageProposal;
    }
    
    /**
     * Confirmation settings for marriage proposals and duels are written to /planeshift/userdata/options/confirmation.xml
     * @brief Writes confirmation settings for duels and marriages
     */
    void WriteConfirmationSettings();
    
    /**
     * Confirmation settings for marriage proposals and duels are read from /planeshift/userdata/options/confirmation.xml
     * @brief Loads confimration settings for duels and marriages
     * @return True on success, False otherwise
     */
    bool LoadConfirmationSettings();

    /**
     * Loads and applies the sound settings.
     *
     * @param forceDef True if you want to load the default settings
     * @return true if it was successfully done
     */
    bool LoadSoundSettings(bool forceDef);

    /// Checks if the client has loaded its map
    inline bool HasLoadedMap() const
    {
        return loadedMap;
    }

    /// Sets the HasLoadedMap value
    void SetLoadedMap(bool value)
    {
        loadedMap = value;
    }

    /// Checks if the client had any errors during the loading state
    bool LoadingError()
    {
        return loadError;
    }

    /// Sets the LoadError value
    void LoadError(bool value)
    {
        loadError = value;
    }

    /**
     * Loads a widget.
     *
     * @param title Title to show in the logs if an error occurs
     * @param filename XML file
     * @return Append this to a var and check it after all loading is done
     */
    bool LoadPawsWidget(const char* title, const char* filename);

    /**
     * Loads custom paws widgets that are specified as a list in an xml file.
     *
     * @param filename The xml file that holds the list of custom widgets.
     */
    bool LoadCustomPawsWidgets(const char* filename);

    SlotNameHash slotName;

    void AddLoadingWindowMsg(const csString &msg);

    /**
     * Shortcut to pawsQuitInfoBox.
     *
     * @param msg Message to display before quiting
     */
    void FatalError(const char* msg);

    /// Logged in?
    bool IsLoggedIn()
    {
        return loggedIn;
    }
    void SetLoggedIn(bool v)
    {
        loggedIn = v;
    }

    /**
     * Set the number of characters this player has.
     *
     * Used to wait for full loading of characters in selection screen.
     */
    void SetNumChars(int chars)
    {
        numOfChars = chars;
    }

    /// Get the number of characters this player should have.
    int GetNumChars()
    {
        return numOfChars;
    }

    /// Get the main character's name
    const char* GetMainPlayerName();

    /// Update the window title with the current informations
    bool UpdateWindowTitleInformations();

    /// Force the next frame to get drawn. Used when updating LoadWindow.
    void ForceRefresh()
    {
        elapsed = 0;
    }

    /// Set Guild Name here for use with Entity labels
    void SetGuildName(const char* name)
    {
        guildname = name;
    }

    /// Get GuildName
    const char* GetGuildName()
    {
        return guildname;
    }

    /** Gets whether sounds should be muted when the application loses focus.
    * @return true if sounds should be muted, false otherwise
    */
    bool GetMuteSoundsOnFocusLoss(void) const
    {
        return muteSoundsOnFocusLoss;
    }
    /** Sets whether sounds should be muted when the application loses focus.
    * @param value true if sounds should be muted, false otherwise
    */
    void SetMuteSoundsOnFocusLoss(bool value)
    {
        muteSoundsOnFocusLoss = value;
    }

    /// Mute all sounds.
    void MuteAllSounds(void);
    /// Unmute all sounds.
    void UnmuteAllSounds(void);

    /** FindCommonString
    * @param cstr_id The id of the common string to find.
    */
    const char* FindCommonString(unsigned int cstr_id);

    /**
    * FindCommonStringId
    * @param[in] str The string we are looking for
    * @return The id of the common string or csInvalidStringID if not found
    */
    csStringID FindCommonStringId(const char* str);

    /// Get the message strings/common string table
    csStringHashReversible* GetMsgStrings();

    ///Get the status of the sound plugin, if available or not.
    bool GetSoundStatus()
    {
        return soundOn;
    }

    /// get the inventory cache
    psInventoryCache* GetInventoryCache(void)
    {
        return inventoryCache;
    }

    /// Whether or not we're background loading.
    bool BackgroundWorldLoading()
    {
        return backgroundWorldLoading;
    }

    void RegisterDelayedLoader(DelayedLoader* obj)
    {
        delayedLoaders.PushSmart(obj);
    }
    void UnregisterDelayedLoader(DelayedLoader* obj)
    {
        delayedLoaders.Delete(obj);
    }

    static csString hwRenderer;
    static csString hwVersion;
    static csString playerName;
#ifdef WIN32
    static HWND hwnd;
#endif

private:
    /// Load the log report settings from the config file.
    void LoadLogSettings();

    /// queries all needed plugins
    bool QueryPlugins();

    /// This adds more factories to the standard PAWS ones.
    void DeclareExtraFactories();

    void HideWindow(const csString &widgetName);

    /// Limits the frame rate either by returning false or sleeping
    void FrameLimit();

    /* plugins we're using... */
    csRef<iObjectRegistry>    object_reg;     ///< The Object Registry
    csRef<iEventNameRegistry> nameRegistry;   ///< The name registry.
    csRef<iEngine>            engine;         ///< Engine plug-in handle.
    csRef<iConfigManager>     cfgmgr;         ///< Config Manager
    csRef<iTextureManager>    txtmgr;         ///< Texture Manager
    csRef<iVFS>               vfs;            ///< Virtual File System
    csRef<iGraphics2D>        g2d;            ///< 2d canvas
    csRef<iGraphics3D>        g3d;            ///< 3d canvas
    csRef<iEventQueue>        queue;          ///< Event Queue
    csRef<iVirtualClock>      vc;             ///< Clock
    csRef<iDocumentSystem>    xmlparser;      ///< XML Parser
    csRef<iSoundManager>      SoundManager;   ///< planehift soundmanager
    csRef<iStringSet>         stringset;
    csRandomGen               random;

    csRef<psNetManager>       netmanager;   ///< Network manager
    csRef<psCelClient>        celclient;    ///< CEL client
    csRef<ModeHandler>        modehandler;  ///< Handling background audio sent from server, etc.
    csRef<ActionHandler>      actionhandler;
    csRef<ZoneHandler>        zonehandler;  ///< Region/map file memory manager.
    csRef<psCal3DCallbackLoader> cal3DCallbackLoader;
    psClientCharManager*      charmanager;  ///< Holds the charactermanager
    GUIHandler*               guiHandler;
    psCharController*         charController;
    psMouseBinds*             mouseBinds;
    psCamera*                 camera;
    csRef<psEffectManager>    effectManager;
    psChatBubbles*            chatBubbles;
    psOptions*                options;
    psSlotManager*            slotManager;
    ClientSongManager*        songManager;
    psQuestionClient*         questionclient;
    PawsManager*              paws;          ///< Hold the ps AWS manager
    psMainWidget*             mainWidget;    ///< Hold the ps overridden version of the desktop
    psInventoryCache*	      inventoryCache;///< inventory cache for client
    psCSSetup*                CS_Setup;
    csRef<iBgLoader>          loader;
    csRef<iSceneManipulate>   scenemanipulator;
    Autoexec*                 autoexec;

    /* status, misc. vars */
    bool gameLoaded; ///< determines if the game is loaded or not
    bool modalMenu;
    bool loggedIn;

    csTicks loadtimeout;

    csString lasterror;

    csString guildname;

    /// Used to stop rendering when the window is minimized
    bool drawScreen;

    // For splash progress.
    csRefArray<iThreadReturn> precaches;
    csRef<iStringArray> meshes;
    csRef<iStringArray> maps;
    size_t lastLoadingCount;

    // True if we've requested the main actor.
    bool actorRequested;

private:

    csString targetPetitioner;

    float BrightnessCorrection;

    float KFactor;           ///< Hold the K factor used for casting spells.

    iEvent* lastEvent;

    /**
     * @brief How duel requests should be handled<br/>
     * 0: Never accept duels and silently reply with "no"<br/>
     * 1: Prompt user for duel acceptance <i>(default)</i><br/>
     * 2: Always accept duels and silenty reply with "yes"
     */
    int duelConfirmation;
    bool marriageProposal; ///< How marriage proposals should be handled.<br>True: Prompt user<br>False: Silently ignore

    bool loadedMap;
    bool loadError; ///< If something happend during loading, it will show
    csArray<csString> wdgProblems; ///< Widget load errors

    /// Stores the number of chars available to this player. Used in loading.
    int numOfChars;

    /// Time at which last frame was drawn.
    csTicks elapsed;

    /// msec to wait between drawing frames
    csTicks frameLimit;

    /// Maximum number of frames per second to draw.
    int maxFPS;

    float currFPS;
    unsigned int countFPS;
    unsigned int timeFPS;

    bool showFPS;

    csRef<iFont> font;

    /// Whether sounds should be muted when the application loses focus.
    bool muteSoundsOnFocusLoss;

    /// Define if the sound is on or off from psclient.cfg.
    bool soundOn;

    bool backgroundWorldLoading;

    // Event ID cache
    csEventID event_frame;
    csEventID event_canvashidden;
    csEventID event_canvasexposed;
    csEventID event_focusgained;
    csEventID event_focuslost;
    csEventID event_mouse;
    csEventID event_keyboard;
    csEventID event_joystick;
    csEventID event_quit;

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    csEventID event_selectionnotify;
#endif

    /**
    * Embedded iEventHandler interface that handles frame events in the
    * logic phase.
    */
    class LogicEventHandler : public scfImplementation1<LogicEventHandler, iEventHandler>
    {
    public:
        LogicEventHandler(psEngine* parent) : scfImplementationType(this), parent(parent)
        {
        }

        virtual ~LogicEventHandler()
        {
        }

        virtual bool HandleEvent(iEvent &ev)
        {
            return parent->ProcessLogic(ev);
        }

        virtual const csHandlerID* GenericPrec(csRef<iEventHandlerRegistry> &,
                                               csRef<iEventNameRegistry> &, csEventID) const;
        virtual const csHandlerID* GenericSucc(csRef<iEventHandlerRegistry> &,
                                               csRef<iEventNameRegistry> &, csEventID) const;

        CS_EVENTHANDLER_NAMES("planeshift.client.frame.logic")
        CS_EVENTHANDLER_DEFAULT_INSTANCE_CONSTRAINTS

    private:
        psEngine* parent;
    };

    /**
    * Embedded iEventHandler interface that handles frame events in the
    * 3D phase.
    */
    class EventHandler3D : public scfImplementation1<EventHandler3D, iEventHandler>
    {
    public:
        EventHandler3D(psEngine* parent) : scfImplementationType(this), parent(parent)
        {
        }

        virtual ~EventHandler3D()
        {
        }

        virtual bool HandleEvent(iEvent &ev)
        {
            return parent->Process3D(ev);
        }

        CS_EVENTHANDLER_PHASE_3D("planeshift.client.frame.3d");

    private:
        psEngine* parent;
    };

    /**
    * Embedded iEventHandler interface that handles frame events in the
    * 2D phase.
    */
    class EventHandler2D : public scfImplementation1<EventHandler2D, iEventHandler>
    {
    public:
        EventHandler2D(psEngine* parent) : scfImplementationType(this), parent(parent)
        {
        }

        virtual ~EventHandler2D()
        {
        }

        virtual bool HandleEvent(iEvent &ev)
        {
            return parent->Process2D(ev);
        }

        CS_EVENTHANDLER_PHASE_2D("planeshift.client.frame.2d");

    private:
        psEngine* parent;
    };

    /**
    * Embedded iEventHandler interface that handles frame events in the
    * Frame phase.
    */
    class FrameEventHandler : public scfImplementation1<FrameEventHandler, iEventHandler>
    {
    public:
        FrameEventHandler(psEngine* parent) : scfImplementationType(this), parent(parent)
        {
        }

        virtual ~FrameEventHandler()
        {
        }

        virtual bool HandleEvent(iEvent &ev)
        {
            return parent->ProcessFrame(ev);
        }

        CS_EVENTHANDLER_PHASE_FRAME("planeshift.client.frame");

    private:
        psEngine* parent;
    };

    csRef<LogicEventHandler> eventHandlerLogic;
    csRef<EventHandler3D> eventHandler3D;
    csRef<EventHandler2D> eventHandler2D;
    csRef<FrameEventHandler> eventHandlerFrame;

    csWeakRefArray<DelayedLoader> delayedLoaders;
};

#endif
