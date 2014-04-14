/*
 * pawsmanager.h - Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 *
 * pawsmanager.h: interface for the PawsManager class.
 *
 *----------------------------------------------------------------------------
 */

#ifndef PAWS_MANAGER_HEADER
#define PAWS_MANAGER_HEADER

#include <csutil/ref.h>
#include <csutil/parray.h>
#include <csutil/hash.h>
#include <csutil/csstring.h>
#include <csgeom/vector3.h>

#include <iutil/vfs.h>
#include <iutil/document.h>

#include "util/mathscript.h"
#include "util/log.h"

#include "pawsmouse.h"
#include "psmousebinds.h"
#include "pawsstyles.h"

struct iObjectRegistry;
struct iGraphics2D;
struct iGraphics3D;
struct iXWindow;
struct iEvent;
class psLocalization;

struct iSoundManager;

class pawsWidget;
class pawsWidgetFactory;

class pawsMainWidget;
class pawsTextureManager;
class pawsPrefManager;

struct iPAWSSubscriber;
struct PAWSData;
struct PAWSSubscription;

/**
 * \addtogroup common_paws
 * @{ */

typedef csHash<PAWSSubscription*,csString> PAWSSubscriptionsHash;

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)

/// SelectionNotify event(Should be moved into CS).
#define csevSelectionNotify(reg) (csEventNameRegistry::GetID((reg), "crystalspace.xwindow.clipboard.selection.notify"))

#endif

#define CONFIG_TOOLTIPS_FILE_NAME       "/planeshift/userdata/options/tooltips.xml"
#define CONFIG_TOOLTIPS_FILE_NAME_DEF   "/planeshift/data/options/tooltips_def.xml"

/**
 * Main PlaneShift Window manager.
 */
class PawsManager : public Singleton<PawsManager>
{
public:

    PawsManager(iObjectRegistry* objectReg, const char* skin, const char* skinBase = NULL);

    virtual ~PawsManager();

    /// Establish main widget.
    void SetMainWidget(pawsMainWidget* widg);

    /** @brief Process mouse and keyboard events.
     *  @param event iEvent to process.
     */
    bool HandleEvent(iEvent &event);

    /** Draw the main widget and the mouse last.
     */
    void Draw();

    /** Draw 3D parts of the widgets
     */
    void Draw3D();

    /// Returns the 2D renderer.
    iGraphics2D* GetGraphics2D()
    {
        return graphics2D;
    }

    /// Returns the 3D renderer.
    iGraphics3D* GetGraphics3D()
    {
        return graphics3D;
    }

    /// Returns the object registry.
    iObjectRegistry* GetObjectRegistry()
    {
        return objectReg;
    }

    /// Get the event name registry.
    iEventNameRegistry* GetEventNameRegistry()
    {
        return nameRegistry;
    }

    /// Returns the texture manager.
    pawsTextureManager* GetTextureManager()
    {
        return textureManager;
    }

    void UseR2T(bool r2t)
    {
        render2texture = r2t;
    }

    bool UsingR2T() const
    {
        return render2texture;
    }

    /// Loads a skin and loades unregistered resources
    bool LoadSkinDefinition(const char* zip);

    /** @brief Add a new factory to the list that the manager knows about.
     *
     *  Each widget type must have it's own factory so the manager can
     *  build it based on it's name.
     *  @param factory The widget factory to add to the list.
     */
    void RegisterWidgetFactory(pawsWidgetFactory* factory);

    /** @brief Loads a widget definition file.
     *
     *  This loads a widget from the specified XML file and adds it to the main widget.
     *
     *  @param widgetFile The standard path of the widget to load.
     *  @see psLocalization::FindLocalizedFile()
     *  @return True if the widget was loaded properly.
     */
    bool LoadWidget(const char* widgetFile);

    /** @brief Loads a widget definition from a string.
     *
     *  This loads a widget from an XML string and returns it to the caller.
     *
     *  @param widgetDefinition The xml, either from a file or constructed on the fly.
     *  @see psLocalization::FindLocalizedFile()
     *  @return True if the widget was loaded properly.
     */
    pawsWidget* LoadWidgetFromString(const char* widgetDefinition);


    /** @brief Loads a widget from given XML node.
     *  @return NULL on failure.
     */
    pawsWidget* LoadWidget(iDocumentNode* widgetNode);

    /** @brief Loads widgets from a definition file without assigning a parent.
     *
     *  This loads multiple widgets from XML and places pointers to the
     *  loaded widgets into the array loadedWidgets.
     *
     *  @param widgetFile The standard path of the widget to load.
     *  @param loadedWidgets An array which will be cleared and then filled
     *  with any widgets loaded.
     *  @return False if an error occured.  Note that some widgets may be
     *  loaded even if an error occurs.
     *  @see psLocalization::FindLocalizedFile()
     */
    bool LoadChildWidgets(const char* widgetFile, csArray<pawsWidget*> &loadedWidgets);


    /** @brief Create a new widget.
     *
     *  This creates a new widget based on the factory that is passed in.
     *  @param factoryName The name of the factory that is used to create a
     *  widget.
     *  @return A new instance of a widget if the factory was found. NULL
     *  if the widget could not be found.
     */
    pawsWidget* CreateWidget(const char* factoryName);

    /** @brief Create a new widget using copy constructor.
    *
    *  This creates a new widget based on the factory that is passed in.
    *  @param factoryName The name of the factory that is used to create a
    *  widget.
    *  @param origin The original widget that will be used by copy constructor.
    *  @return A new instance of a widget if the factory was found. NULL
    *  if the widget could not be found.
    */
    pawsWidget* CreateWidget(const char* factoryName, const pawsWidget* origin);

    /// Adds an object view to the array.
    void AddObjectView(pawsWidget* widget)
    {
        objectViews.Push(widget);
    }

    bool LoadObjectViews();

    /// Removes an object view from the array.
    void RemoveObjectView(pawsWidget* widget)
    {
        objectViews.Delete(widget);
    }

    /// Returns the widget that is focused.
    pawsWidget* GetCurrentFocusedWidget()
    {
        return currentFocusedWidget;
    }

    /// Returns true if the current focused widget needs to override all controls
    bool GetFocusOverridesControls()
    {
        return focusOverridesControls;
    }

    /// Returns modal widget
    pawsWidget* GetModalWidget()
    {
        return modalWidget;
    }

    /** @brief Give this widget focus.
     *  @param widget The widget to focus.
     */
    void SetCurrentFocusedWidget(pawsWidget* widget);

    /** @brief Make this widget modal.
     *  @param widget The modal widget.
     */
    void SetModalWidget(pawsWidget* widget);

    /** pawsWidget destructor calls this so PawsManager can NULLify all
     * its links to the widget.
     */
    void OnWidgetDeleted(pawsWidget* widget);

    /** Remove focus and mouseover effect from widget if widget is hidden */
    void OnWidgetHidden(pawsWidget* widget);

    /** @brief Let the window manager know that a widget is being moved.
     *  @param moving The widget that is currently moving.
     */
    void MovingWidget(pawsWidget* moving);

    /** @brief Let the manager know that a widget is being resized.
     *  @param widget The widget that is being resized.
     *  @param flags The resize flags that this widget should be resized with.
     */
    void ResizingWidget(pawsWidget* widget, int flags);

    /// Returns the prefrence manager.
    pawsPrefManager* GetPrefs()
    {
        return prefs;
    }

    /// Returns the mouse.
    pawsMouse* GetMouse()
    {
        return mouse;
    }

    /// Returns the resize button image.
    iPawsImage* GetResizeImage()
    {
        return resizeImg;
    }

    /**
     * Locate a widget by name.
     *
     * @param name The name of the widget.
     * @param complain If true it will show errors if the widget wasn't found.
     */
    pawsWidget* FindWidget(const char* name, bool complain=true);

    /**
     * Remove the named widget from the mainwidget
     *
     * @param widgetName The name of the widget to look for.
     * @param complain If true it will show errors if the widget wasn't found.
     * @return a boolean stating if the widget was found (and so removed)
     */
    bool RemoveWidget(const char* widgetName, bool complain);

    /// Returns the main widget.
    pawsMainWidget* GetMainWidget()
    {
        return mainWidget;
    }

    /// Returns the psLocalization object:
    psLocalization* GetLocalization()
    {
        return localization;
    }

    /// A shortcut - translation without need to call GetLocalization().
    csString Translate(const csString &orig);

    /** @brief Gets the widget that is being drag'n'dropped over screen with the mouse.
     *
     *  @return The widget being dragged.
     *  @remarks Ownership of the widget goes to psPawsManager. This means that
     *  this widget must not be already owned (e.g. be child of another widget).
     *  Parameter can be NULL.
     */
    pawsWidget* GetDragDropWidget();

    /** @brief Sets the widget that is being drag'n'dropped over screen with the mouse.
     *
     * @param dragDropWidget The widget to drag.
     * @remarks Ownership of the widget goes to psPawsManager. This means that
     * this widget must not be already owned (e.g. be child of another widget).
     */
    void SetDragDropWidget(pawsWidget* dragDropWidget);

    /** @brief Gets the factor the font should be adjusted by for proper fontsize based on resolution.
     *  @return the factor for the fontsize.
     */
    float GetFontFactor()
    {
        return fontFactor;
    };

    /** @brief Creates a warning box with the supplied text.
     *  @param message The warning.
     *  @param notify The widget which recevies event notifications ( i.e. Button Pressed ).
     *  @param modal If the widet should be a modal one or not.
     */
    void CreateWarningBox(const char* message, pawsWidget* notify = NULL, bool modal = true);

    /**
     * Creates a YesNo box with the supplied text.
     *
     * @param message The warning.
     * @param notify The Widget which recives event notifications (i.e. Button Pressed).
     * @param modal The box shold be modal.
     * @param translate Whathever the message should be translated or not.
     */
    void CreateYesNoBox(const char* message, pawsWidget* notify = NULL, bool modal = true, bool translate = true);

    /** @brief Applies PAWS style to XML node.
     *  @see pawsstyles.h
     */
    bool ApplyStyle(const char* name, iDocumentNode* target);

    /*                          ToolTips Functions/Variables
    ------------------------------------------------------------------------*/
    int  TooltipsColors[3];      ///< Array of tooltip colors
    bool ToolTipEnable;          ///< Sets if the tooltip should be enable
    bool ToolTipEnableBgColor;   ///< Sets if the tooltip should have a bgcolor

    /// Loads the Tooltips config-file
    bool LoadTooltips(const char* fileName);
    /// returns the full path to the tooltips.xml in skin.zip
    csString getToolTipSkinPath();
    /// returns tooltips enabled/disabled
    bool getToolTipEnable()
    {
        return ToolTipEnable;
    };
    /// changes tooltips enabled/disabled
    void setToolTipEnable(bool state)
    {
        ToolTipEnable = state;
    };
    /// returns tooltips brackground color
    bool getToolTipEnableBgColor()
    {
        return ToolTipEnableBgColor;
    };
    /// changes tooltips backround enabled/disabled
    void setToolTipEnableBgColor(bool state)
    {
        ToolTipEnableBgColor = state;
    };
    /// returns one element of the TooltipsColors-array
    int getTooltipsColors(int element)
    {
        return TooltipsColors[element];
    };
    /// transfers a value to the TooltipsColors-array
    void setTooltipsColors(int element, int param)
    {
        TooltipsColors[element] = param;
    };

    /*                          Sound Functions
    ------------------------------------------------------------------------*/

    /// Gets the soundManager.
    iSoundManager* GetSoundManager()
    {
        return soundManager;
    };

    /*                       Subcription Functions
    ------------------------------------------------------------------------*/

    /// Unsubscribe the given subscriber.
    void UnSubscribe(iPAWSSubscriber* listener);

    /// Subscribe to a named piece of data, so updates are received automatically.
    void Subscribe(const char* dataname,iPAWSSubscriber* listener);

    /// Announce a change in a named element to all subscribers.
    void Publish(const csString &dataname,PAWSData &data);

    /// Publish a string to all subscribers.
    void Publish(const csString &dataname,const char* datavalue);

    /// Publish a boolean value to all subscribers.
    void Publish(const csString &dataname,bool  datavalue);

    /// Publish an int to all subscribers.
    void Publish(const csString &dataname,int   datavalue);

    /// Publish an unsigned int to all subscribers.
    void Publish(const csString &dataname,unsigned int   datavalue);

    /// Publish a float to all subscribers.
    void Publish(const csString &dataname,float datavalue);

    /// Publish a coloured string to all subscribers.
    void Publish(const csString &dataname, const char* datavalue, int color);

    /// Publish nothing to all subscribers. (Used for one-time named signals.)
    void Publish(const csString &dataname);

    /// Return a list of all subscribers.
    csArray<iPAWSSubscriber*> ListSubscribers(const char* dataname);

    MathEnvironment &ExtraScriptVars()
    {
        return extraScriptVars;
    }

    csString &getVFSPathToSkin()
    {
        return vfsPathToSkin;
    }

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    /** RequestClipboardContent will result in a call to OnClipboard in
     *  in the widget with focus. This needs to be implemented asyncronos
     *  due to the nature of the X11 system.
     */
    void RequestClipboardContent();
#endif

protected:
    MathEnvironment extraScriptVars;

    psPoint MouseLocation(iEvent &ev);

    /// VFS Mount directory mapping to the specified skin zip file
    csString vfsPathToSkin;

    /// Localized file object registry.
    psLocalization* localization;

    /// The preference/default manager.
    pawsPrefManager* prefs;

    /// The last widget that the mouse clicked on. ( Hence the focused one ).
    pawsWidget* currentFocusedWidget;

    /// Does the currentFocusedWidget take focus from the control system?
    bool focusOverridesControls;

    /// The last widget that the mouse moved over. ( For alpha fade effects ).
    pawsWidget* mouseoverWidget;

    /// The actual widget that was last faded.
    pawsWidget* lastfadeWidget;

    /// The time mouse has been over the last widget.
    csTicks timeOver;

    /// The time mouse has been over a  widget before showing the tooltip.
    int tipDelay;

    /// Current widget that is being moved.
    pawsWidget* movingWidget;

    /// Current modal widget.
    pawsWidget* modalWidget;

    /// Holds the widget that is being resized.
    pawsWidget* resizingWidget;

    /// The resize flags.
    int resizingFlags;

    /// Helper function to load standard factories.
    void RegisterFactories();

    /** @brief Process mouse movement events.
     *
     *  Determines if a widget is moving or being resized. If there is no modal
     *  widget it operates on the topmost widget at the event coordinates.
     *
     *  @param data csMouseEventData containing the mouse event data.
     *  @return TRUE if it moves, resizes or fades in a widget.
     */
    bool HandleMouseMove(csMouseEventData &data);

    /** @brief Process mouse down events.
     *
     *  Determines the widget at event coordinates.
     *  Calls OnMouseDown() on the currentFocusedWidget or widget at the event
     *  coordinates.
     *  @param data csMouseEventData containing the mouse event data.
     *  @return TRUE if event stops moving or resizing.
     */
    bool HandleMouseDown(csMouseEventData &data);


    /** @brief Process mouse double click events.
      *
      * Calls OnDoubleClick on the currentFocusedWidget or widget at the event
      * coordinates.
      *
      * @param data csMouseEventData containing the mouse event data.
      * @return Results of OnDoubleClick or FALSE.
      */
    bool HandleDoubleClick(csMouseEventData &data);

    /** @brief Process mouse up events.
     *
     *  Stops moving or resizing and turns off the corresponding flag.
     *  Calls OnMouseUp on the currentFocusedWidget or widget at the event
     *  coordinates.
     *
     *  @param data csMouseEventData containing the mouse event data.
     *  @return Results of OnMouseUp or FALSE.
     */
    bool HandleMouseUp(csMouseEventData &data);

    /** @brief Process key down events.
     *
     * If a widget has focus it extracts the event keycode, key and modifiers
     * and calls OnKeyDown on the focused widget.
     *
     * @param event iEvent to process.
     * @return TRUE if a widget has focus AND the current event type is key up
     * while hadKeyDown was set OR if OnKeyDown returns TRUE when called on
     * the focused widget.
     * @remarks Sets hadKeyDown to result of OnKeyDown call.
     */
    bool HandleKeyDown(iEvent &event);

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    /** @brief Process Selection Notify events.
     *
     * If a widget has focus it extracts the clipboard content
     * and calls OnClipboard on the focused widget.
     *
     * @param event iEvent to process.
     * @return TRUE if a widget has focus AND the
     * OnClipboard returns TRUE when called.
     */
    bool HandleSelectionNotify(iEvent &event);
#endif

    /** The object registry.
     */
    iObjectRegistry* objectReg;

    /** Pointer to the Crystal Space iGraphics2D renderer used to display
     * 2D graphics.
     */
    csRef<iGraphics2D> graphics2D;

    /** Pointer to the Crystal Space iGraphics3D renderer used to display
     * 3D graphics.
     */
    csRef<iGraphics3D> graphics3D;

    /** For event parsing.
     */
    csRef<iEventNameRegistry> nameRegistry;

    /// The main handler widget.
    pawsMainWidget* mainWidget;

    /// Array of paws object view widgets;
    csArray<pawsWidget*> objectViews;

    /// The texture manager.
    pawsTextureManager* textureManager;

    /// The mouse pointer.
    pawsMouse* mouse;

    /// Resized image.
    csRef<iPawsImage> resizeImg;

    /// An array of pointers to available factories.
    csPDelArray<pawsWidgetFactory> factories;

    /// Pointer to the Crystal Space iVFS file system.
    csRef<iVFS> vfs;

    /// Pointer to the Crystal Space iDocumentSystem.
    csRef<iDocumentSystem>  xml;

    /// Flag for key down function.
    bool hadKeyDown;

    /// Render texture for gui rendering.
    csRef<iTextureHandle> guiTexture;

    /// Whether to use r2t for the gui.
    bool render2texture;

    /**
     * The widget that is drag'n'dropped across the screen by the mouse.
     *
     * If it is not NULL, it is drawn instead of mouse on its position.
     * This widget has no parent widget (same as with pawsMainWidget)
     */
    pawsWidget* dragDropWidget;

    /// The font resizing factor for all widgets
    float fontFactor;


    /*                      Sound Member Variables
    ------------------------------------------------------------------------*/

    csRef<iSoundManager> soundManager;  ///< planeshift's sound manager

    /**
     * Parses given file and returns the \<widget_description\> tag of it
     *
     * @return NULL on failure.
     */
    csPtr<iDocumentNode> ParseWidgetFile(const char* widgetFile);

    /// PAWS style definitions.
    pawsStyles* styles;

    /// Table of subscriptions.
    PAWSSubscriptionsHash subscriptions;


    /*                      Shortcuts for events
    ------------------------------------------------------------------------*/
    /// Shortcut for event mouse move
    csEventID MouseMove;
    /// Shortcut for event mouse down
    csEventID MouseDown;
    /// Shortcut for event mouse double click
    csEventID MouseDoubleClick;
    /// Shortcut for event mouse up
    csEventID MouseUp;
    /// Shortcut for event key down
    csEventID KeyboardDown;
    /// Shortcut for event key up
    csEventID KeyboardUp;

    /*                      X Clipboard
    ------------------------------------------------------------------------*/
#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    csRef<iXWindow> xwin;

    /// Shortcut for event Selection Notify
    csEventID SelectionNotifyEvent;

#endif
};


/// Data types for pub/sub.
enum PAWSDATATYPE
{
    PAWS_DATA_UNKNOWN,
    PAWS_DATA_STR,
    PAWS_DATA_BOOL,
    PAWS_DATA_INT,
    PAWS_DATA_UINT,
    PAWS_DATA_FLOAT,
    PAWS_DATA_INT_STR
};


struct PAWSData
{
    static csString temp_buffer;

    PAWSDATATYPE type;
    union
    {
        int intval;
        unsigned int uintval;
        float floatval;
        bool boolval;
    };
    csString str;  //csString cannot be inside union

    PAWSData()
    {
        type=PAWS_DATA_UNKNOWN;
    }
    PAWSData &operator=(PAWSData &other)
    {
        type = other.type;
        intval = other.intval;
        str = other.str;
        return *this;
    }

    bool IsData()
    {
        return type != PAWS_DATA_UNKNOWN;
    }

    const char* GetStr();
    float GetFloat();
    int GetInt();
    unsigned int GetUInt();
    bool GetBool();
};


struct iPAWSSubscriber
{
    virtual void OnUpdateData(const char* name,PAWSData &data) = 0;
    virtual void NewSubscription(const char* name) = 0;
    virtual ~iPAWSSubscriber() {};
};

struct PAWSSubscription
{
    PAWSData lastKnownValue;
    iPAWSSubscriber* subscriber;
};

/** @} */

#endif



