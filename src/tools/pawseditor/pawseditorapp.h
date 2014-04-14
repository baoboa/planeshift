/*
* pawseditorapp.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings [Borrillis] <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : main class for pawseditor
*
*/

#ifndef PAWSEDIT_APP_HEADER
#define PAWSEDIT_APP_HEADER

#include <csutil/sysfunc.h>
#include <csutil/csstring.h>
#include <csutil/ref.h>
#include <csutil/bitarray.h>
#include <csgeom/vector2.h>
#include <csgeom/vector3.h>

#include "pewidgettree.h"
#include "peskinselector.h"
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

class PawsManager;
class pawsMainWidget;
class pawsWidget;
class pawsEditor;

class psMaterialUtil;

class eControlManager;

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


class PawsEditorApp
{
public:

    static const char * CONFIG_FILENAME;
    static const char * APP_NAME;
    static const char * WINDOW_CAPTION;
    static const char * KEY_DEFS_FILENAME;

    pawsWidget *currentWidget;

    /** Constructor
     */
    PawsEditorApp(iObjectRegistry *obj_reg);

    /** Destructor
     */
    ~PawsEditorApp();

    /** Reports a severe error in the application
     *   @param msg the error message
     */
    void SevereError(const char* msg);

    /** Initializes some CS specific stuff, fills most of this classes global variables, and inits eedit specifics
     *   @return true on success, false otherwise
     */
    bool Init();

    /** Loads a single paws widget.
     *   @param file the filename of the window to load
     *   @return true if succeeded, false otherwise
     */
    
    bool LoadWidget(const char * file);
    
    /** Loads all the paws widgets (windows)
     *   @return true if all succeeded to load, false if any had an error
     */
    bool LoadWidgets();

    /**
     * Handles an event from the event handler.
     *
     * @param ev Event the event to handle
     */
    bool HandleEvent (iEvent &ev);

    /** Sets the given camera flag
     *   @param flag the editApp::CAM_FLAG to set
     *   @param val the new value of the camera flag
     */
    void SetCamFlag(int flag, bool val);

    /** Gets the value of the given camera flag
     *   @param flag the editApp::CAM_FLAG to get
     *   @return the value of the specific camera flag
     */
    bool GetCamFlag(int flag);

    /** Gets the location of the mouse pointer.
     *   @return the mouse position.
     */
    csVector2 GetMousePointer();

    /** Gets the Configuration Manager
     *   @return the configuration manager.
     */
    iConfigManager* GetConfigManager();

    /** Gets the VFS Manager
     *   @return the VFS manager.
     */
    iVFS* GetVFS();

    /** Exits the application.
     */
    void Exit();
    
    csPtr<iDocumentNode> ParseWidgetFile( const char* widgetFile );
    csPtr<iDocumentNode> ParseWidgetFile_mod( const char* widgetFile );
    void OpenWidget(const csString & text);
    void ReloadWidget();
    void OpenImageList(const csString & text);
    // Load a new skin
    void LoadSkin( const char* name );
    void ShowSkinSelector() { skinSelector->Show(); };



private:

    // Used to load a resource from the new skin
    bool LoadResources( const char* mountPath );
    
    /** Registers the custom PAWS widgets created in this application.
     */
    void RegisterFactories();

    /** Updates the geometry and misc stuff.
     */
    void Update();

    /** Takes a screenshot and saves it to a file.
     *   @param fileName the file to save the screenshot to
     */
    void TakeScreenshot(const csString & fileName);

    /**
     * Used to load the log settings
     */
    void LoadLogSettings();
    
    /// Declare our event handler
    DeclareGenericEventHandler(EventHandler,PawsEditorApp,"planeshift.pawseditor");
    csRef<EventHandler> event_handler;

    iObjectRegistry*        object_reg;
    csRef<iEngine>          engine;
    csRef<iVFS>             vfs;
    csRef<iEventQueue>      queue;
    csRef<iConfigManager>   cfgmgr;
    csRef<iTextureManager>  txtmgr;
    csRef<iGraphics3D>      g3d;
    csRef<iGraphics2D>      g2d;
    csRef<iLoader>          loader;

    // PAWS
    PawsManager      * paws;
    pawsMainWidget   * mainWidget;
    peWidgetTree     * widgetTree;
    peSkinSelector   * skinSelector;
    //peMenu             * menuWindow;
    /// keeps track of whether the window is visible or not
    bool drawScreen;
       
    // camera flags
    csBitArray camFlags;
    float camYaw;
    float camPitch;

    csString fileName;
    csString mountedPath;
    csString mountedVPath;
    csString currentSkin;

    csVector2 mousePointer;
};

#endif
