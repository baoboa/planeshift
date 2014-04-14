/*
 * Author: Andrew Robberts
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef EEDIT_APP_HEADER
#define EEDIT_APP_HEADER

#include <csutil/sysfunc.h>
#include <csutil/csstring.h>
#include <csutil/ref.h>
#include <csutil/bitarray.h>
#include <csgeom/vector2.h>
#include <csgeom/vector3.h>

#include "util/genericevent.h"

struct iEngine;
struct iLoader;
struct iVFS;
struct iEvent;
struct iEventQueue;
struct iConfigManager;
struct iTextureManager;
struct iGraphics3D;
struct iGraphics2D;
struct iView;
struct iVirtualClock;
struct iMeshWrapper;
class csRandomFloatGen;

class PawsManager;
class pawsMainWidget;
class pawsWidget;
class pawsEEdit;

class EEditReporter;
class EEditToolboxManager;
class EEditInputboxManager;

class psEffectManager;
class psCal3DCallbackLoader;

class eControlManager;

/**
 * \addtogroup eedit
 * @{ */

/** The camera flags
    */
enum CAM_FLAG
{
    CAM_NONE = 0,
    
    CAM_UP,
    CAM_RIGHT,
    CAM_DOWN,
    CAM_LEFT,
    CAM_FORWARD,
    CAM_BACK,

    CAM_COUNT
};


class EEditApp
{
public:

    static const char * CONFIG_FILENAME;
    static const char * APP_NAME;
    static const char * WINDOW_CAPTION;
    static const char * KEY_DEFS_FILENAME;
    static const char * TEMP_EFFECTS_LIST;

    /**
     * Constructor
     */
    EEditApp(iObjectRegistry * obj_reg, EEditReporter * _reporter);

    /**
     * Destructor
     */
    ~EEditApp();

    /**
     * Reports a severe error in the application
     *
     * @param msg the error message
     */
    void SevereError(const char* msg);

    /**
     * Initializes some CS specific stuff, fills most of this classes global variables, and inits eedit specifics.
     *
     * @return true on success, false otherwise
     */
    bool Init();

    /**
     * Loads a single paws widget.
     *
     *  @param file the filename of the window to load
     *  @return true if succeeded, false otherwise
     */
    
    bool LoadWidget(const char * file);
    
    /**
     * Loads all the paws widgets (windows).
     *
     * @return true if all succeeded to load, false if any had an error
     */
    bool LoadWidgets();

    /**
     * Get Paws
     */
    PawsManager* GetPaws() { return paws; }


    /**
     * handles an event from the event handler
     *
     * @param event the event to handle
     */
    bool HandleEvent (iEvent &event);

    /**
     * Refreshes the list of effects.
     */
    void RefreshEffectList();

    /**
     * Effect has been changed, so lets rerender it.
     */
    bool UpdateEffect();
    
    /**
     * Reloads and renders the given effect.
     *
     * @param effectName the name of the effect to render
     * @return true on success, false on failure
     */
    bool RenderEffect(const csString &effectName);

    /**
     * Renders the current effect.
     *
     * @return true on success, false otherwise.
     */
    bool RenderCurrentEffect();
    
    /**
     * Loads the current effect.
     *
     * @return true on success, false otherwise.
     */
    bool ReloadCurrentEffect();
    
    /**
     * Toggles the pause/resume state of the effect.
     *
     *  @return true if the effect is now paused, false if it isn't
     */
    bool TogglePauseEffect();
    
    /**
     * Cancels rendering of the current effect
     *
     * @return true if the effect is no longer being rendered
     */
    bool CancelEffect();

    /**
     * Create a particle system mesh.
     */
    void CreateParticleSystem(const csString & name);

    /**
     * Toggles the visibility of the given toolbox.
     *
     * @param toolbox the ID of the toolbox to toggle
     * @return true if the toolbox is visible, false if it's invisible or not found
     */
    bool ToggleToolbox(size_t toolbox);

    /**
     * Ensures that the given toolbox is visible.
     *
     * @param toolbox The ID of the toolbox to show.
     */
    void ShowToolbox(size_t toolbox);

    /**
     * Ensures that the given toolbox is hidden.
     *
     * @param toolbox The ID of the toolbox to hide.
     */
    void HideToolbox(size_t toolbox);

    /**
     * Checks if the toolbox is visible.
     *
     * @param toolbox The ID of the toolbox to hide.
     * @return        True if the toolbox is visible.
     */
    bool IsToolboxVisible(size_t toolbox);
    
    /**
     * Grabs the toolbox manager.
     *
     * @return the toolbox manager, 0 if it doesn't exist
     */
    EEditToolboxManager * GetToolboxManager();
  
    /**
     * Grabs the effect manager.
     *
     * @return the effect manager, 0 if it doesn't exist.
     */
    psEffectManager * GetEffectManager();
    
    /**
     * Grabs the inputbox manager.
     *
     * @return the inputbox manager, 0 if it doesn't exist.
     */
    EEditInputboxManager * GetInputboxManager();
    
    /**
     * Loads a mesh based on the name or filepath given.
     *
     * @param meshFile blank, 'axis', or the path to the mesh.
     * @param name Name to use
     * @param pos  Position to use
     * @return the mesh if successful
     */
    csPtr<iMeshWrapper> LoadCustomMesh(const csString & meshFile, const csString & name, csVector3 pos);
    
    /**
     * Loads the position mesh indicator.
     *
     * @param meshFile empty string for nullmesh, "axis" for axis arrows, meshfilename for everything else.
     * @return true on success.
     */
    bool LoadPositionMesh(const csString & meshFile);

    /**
     * Loads the target mesh indicator.
     *
     * @param meshFile empty string for nullmesh, "axis" for axis arrows, meshfilename for everything else.
     * @return true on success.
     */
    bool LoadTargetMesh(const csString & meshFile);

    /**
     * Sets the animation of the position mesh.
     *
     * @param index the index of the animation to set.
     */
    void SetPositionAnim(size_t index);
    
    /**
     * Sets the animation of the target mesh.
     *
     * @param index the index of the animation to set.
     */
    void SetTargetAnim(size_t index);
    
    /**
     * Sets the position and rotation of the effect position anchor.
     *
     * @param pos the new pos of the effect's position anchor.
     * @param rot the new y rotation of the effect's position anchor.
     */
    void SetPositionData(const csVector3 & pos, float rot);

    /**
     * Sets the position and rotation of the effect target anchor.
     *
     * @param pos the new pos of the effect's target anchor.
     * @param rot the new y rotation of the effect's target anchor.
     */
    void SetTargetData(const csVector3 & pos, float rot);
    
    /**
     * Sets the given camera flag.
     *
     * @param flag the editApp::CAM_FLAG to set
     * @param val the new value of the camera flag
     */
    void SetCamFlag(int flag, bool val);

    /**
     * Gets the value of the given camera flag.
     *
     *   @param flag the editApp::CAM_FLAG to get
     *   @return the value of the specific camera flag
     */
    bool GetCamFlag(int flag);

    /**
     *Sets the current effect.
     *
     * @param name the name of the effect
     * @return true on success, false otherwise
     */
    bool SetCurrEffect(const csString & name);
    
    /**
     * Gets the name current effect.
     *
     * @return the name of the current effect
     */
    csString GetCurrEffectName();

    /**
     * Gets the name current particle system.
     *
     * @return the name of the particle system
     */
    csString GetCurrParticleSystemName();

    /**
     * Gets the ID of the current effect.
     *
     * @return the ID of the current effect.
     */
    unsigned int GetCurrEffectID() { return currEffectID; }

    /**
     * Sets the positions of the origin and target actors to defaults based on the map's camera.
     *
     * @return     True on success.
     */
    bool SetDefaultActorPositions();

    /**
     * Loads the given map file.
     *
     * @param mapFile  Path to the map to load.
     * @return         True on success.
     */
    bool LoadMap(const char * mapFile);

    /**
     * Gets the location of the mouse pointer.
     *
     *   @return the mouse position.
     */
    csVector2 GetMousePointer();

    /**
     * Exits the application.
     */
    void Exit();

    iObjectRegistry *       GetObjectRegistry() const;
    csRef<iEngine>          GetEngine() const;
    csRef<iVFS>             GetVFS() const;
    csRef<iEventQueue>      GetEventQueue() const;
    csRef<iConfigManager>   GetConfigManager() const;
    csRef<iTextureManager>  GetTextureManager() const;
    csRef<iGraphics3D>      GetGraphics3D() const;
    csRef<iGraphics2D>      GetGraphics2D() const;
    csRef<iVirtualClock>    GetVirtualClock() const;
    csRef<iLoader>          GetLoader() const;
    EEditReporter *         GetReporter() const;

    const char * GetConfigString(const char * key, const char * defaultValue) const;
    void         SetConfigString(const char * key, const char * value);

    float GetConfigFloat(const char * key, float defaultValue) const;
    void  SetConfigFloat(const char * key, float value);

    bool GetConfigBool(const char * key, bool defaultValue) const;
    void SetConfigBool(const char * key, bool value);

    /**
     * Executes a named command.
     *
     * @param cmd     The name of the command to execute.
     */
    void ExecuteCommand(const char * cmd);

private:

    /**
     * Creates an effect name<->path mapping from an effectslist file.
     *
     * @param path        The path to search.
     * @param clear       If true, this will replace the existing path list, otherwise it appends it.
     * @return            True on success, false otherwise.
     */
    bool CreateEffectPathList(const csString & path, bool clear=true);

    /**
     * Appends to the current effect name<->path mapping from an effect file.
     *
     * @param effectFile the location of the effect file.
     * @param sourcePath the source file that loads this effect.
     */
    bool AppendEffectPathList(const csString & effectFile, const csString & sourcePath);
    
    /**
     * Creates the shortcuts for the shortcut window.
     */
    void CreateShortcuts();

    /**
     * Registers the custom PAWS widgets created in this application.
     */
    void RegisterFactories();

    /**
     * Updates the geometry and misc stuff.
     */
    void Update();

    /**
     * Takes a screenshot and saves it to a file.
     *
     * @param fileName the file to save the screenshot to
     */
    void TakeScreenshot(const csString & fileName);
    
    /// Declare our event handler
    DeclareGenericEventHandler(EventHandler,EEditApp,"planeshift.eedit");
    csRef<EventHandler> event_handler;

    EEditReporter *         reporter;

    iObjectRegistry*        object_reg;
    csRef<iEngine>          engine;
    csRef<iVFS>             vfs;
    csRef<iEventQueue>      queue;
    csRef<iConfigManager>   cfgmgr;
    csRef<iTextureManager>  txtmgr;
    csRef<iGraphics3D>      g3d;
    csRef<iGraphics2D>      g2d;
    csRef<iVirtualClock>    vc;
    csRef<iLoader>          loader;

    // PAWS
    PawsManager    * paws;
    pawsMainWidget * mainWidget;

    pawsEEdit            * editWindow;

    csString currEffectName;
    csString currEffectLoc;
    
    /// keeps track of whether the window is visible or not
    bool drawScreen;
   
    EEditToolboxManager * toolboxManager;
    EEditInputboxManager * inputboxManager;
    
    // effect stuff
    csRef<psEffectManager> effectManager;
    unsigned int currEffectID;
    bool effectLoaded;

    /// The current particle mesh that we are editing.
    csRef<iMeshWrapper> particleSystem;

    /// the mesh that the effect's position will be anchored to
    csRef<iMeshWrapper> pos_anchor;
    
    /// the mesh that the effect's target will be anchored to
    csRef<iMeshWrapper> tar_anchor;
    
    /// true if the effects shouldn't be updated, false otherwise
    bool effectPaused;

    // camera flags
    csBitArray camFlags;
    float camYaw;
    float camPitch;

    eControlManager * controlManager;

    struct EffectPath
    {
        EffectPath(const char * newName, const char * newPath) : name(newName), path(newPath) {}
        
        csString name;
        csString path;
    };
    csArray<EffectPath> effectPaths;

    csVector2 mousePointer;

    csRef<psCal3DCallbackLoader> cal3DCallbackLoader;
    
    csRandomFloatGen * rand;

};

/** @} */

#endif
