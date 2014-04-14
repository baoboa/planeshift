/*
 * pawswidget.h - Author: Andrew Craig
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
 * pawswidget.h: interface for the pawsWidget class.
 ---------------------------------------------------------------------------*/

#ifndef PAWS_WIDGET_HEADER
#define PAWS_WIDGET_HEADER

#include <ivideo/graph2d.h>

#include <csutil/array.h>
#include <csutil/csstring.h>

#include <csgeom/csrectrg.h>
#include <csgeom/vector2.h>

#include <iutil/document.h>

//#include "net/cmdbase.h"
#include "pawsmanager.h"
#include "util/scriptvar.h"

/**
 * \addtogroup common_paws
 * @{ */

#define ATTACH_LEFT     2
#define ATTACH_RIGHT    4
#define ATTACH_BOTTOM   8
#define ATTACH_TOP      16
#define PROPORTIONAL_LEFT   32
#define PROPORTIONAL_RIGHT  64
#define PROPORTIONAL_TOP    128
#define PROPORTIONAL_BOTTOM 256

#define SCROLL_UP     -1000
#define SCROLL_DOWN   -2000
#define SCROLL_THUMB  -3000
#define SCROLL_AUTO   -4000
#define SCROLL_SET    -5000

#define DEFAULT_MIN_HEIGHT  5
#define DEFAULT_MIN_WIDTH   5


#define RESIZE_LEFT    2
#define RESIZE_RIGHT   4
#define RESIZE_TOP     8
#define RESIZE_BOTTOM 16

enum
{
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT
};

#define BORDER_INNER_BEVEL 0x1
#define BORDER_OUTER_BEVEL 0x2
#define BORDER_REVERSED    0x4
#define BORDER_BUMP        (BORDER_OUTER_BEVEL)
#define BORDER_RAISED      (BORDER_INNER_BEVEL|BORDER_OUTER_BEVEL)
#define BORDER_SUNKEN      (BORDER_REVERSED|BORDER_OUTER_BEVEL)
#define BORDER_ETCHED      (BORDER_REVERSED|BORDER_RAISED)


#define DEFAULT_FONT_SIZE   12

#define DEFAULT_FONT_STYLE     0
#define FONT_STYLE_DROPSHADOW  1
#define FONT_STYLE_BOLD        2


enum PAWS_WIDGET_SCRIPT_EVENTS
{
    PW_SCRIPT_EVENT_SHOW = 0,
    PW_SCRIPT_EVENT_HIDE,
    PW_SCRIPT_EVENT_MOUSEDOWN,
    PW_SCRIPT_EVENT_MOUSEUP,
    PW_SCRIPT_EVENT_VALUECHANGED,

    PW_SCRIPT_EVENT_COUNT
};

struct iPawsImage;
class PawsManager;
class pawsBorder;
class pawsScrollBar;
class pawsListBox;
class pawsButton;
class pawsMenu;
class pawsMenuAction;
class WidgetConfigWindow;
class pawsScript;
class pawsTitle;

/**
 * The main base widget that all other widgets should inherit from.
 */
class pawsWidget : public iPAWSSubscriber, public iScriptableVar
{
public:
    /// A class that can be inherited to store extra data in a widget.
    class iWidgetData {};

protected:
    /// factory name
    csString factory;

    /// The id of the widget.
    int id;

    /// This widget's parent.
    pawsWidget* parent;

    /// The 2D graphics interface.
    csRef<iGraphics2D> graphics2D;

    /// The default frame for the widget.
    csRect defaultFrame;

    /// The screen area of the widget.
    csRect screenFrame;

    /// Current clipping rectangle.
    csRect clipRect;

    /// reference to titleBar object, if any
    pawsTitle* titleBar;

    /// filename to load from or save to.
    csString filename;

    /// Used in SetTitle() for the new close button.
    pawsButton* close_widget;

    /**
     * The child widgets.
     *
     * Widgets are drawn from the end of this array to the beginning.
     * Children with alwaysOnTop=true are positioned before all other children
     * in this array.
     */
    csArray<pawsWidget*> children;

    /// Contains the children for tabbing.
    csArray<pawsWidget*> taborder;

    /// Widget to call if enter is pressed.
    pawsWidget* onEnter;

    /// Flag to determine visiblity.
    bool visible;

    /**
     * Determines if this widget should write/load it's position from a
     * config file.
     */
    bool saveWidgetPositions;

    /**
     * Determines if the settings (alpha, fade, etc) of this widget are
     * configurable.
     */
    bool configurable;

    /// Determines if this widget is movable.
    bool movable;

    /// Determines if this widget can be resized by user.
    bool isResizable;

    /// If the resize widget should be drawn
    bool showResize;

    /**
     * Determines if this widget should be auto resized with screen
     * resolution.
     */
    bool resizeToScreen;

    /// Enforce keeping the default aspect ratio when resizing.
    bool keepaspect;

    /// Is this windows painted on top of other windows
    bool alwaysOnTop;

    /// Defines the minimum width that the widget can be.
    int min_width;

    /// Defines the minimum height that the widget can be.
    int min_height;

    /// Defines the maximum width that the widget can be.
    int max_width;

    /// Defines the maximum height that the widget can be.
    int max_height;

    /// The name of this widget.
    csString name;

    /// Old name of the widget with "_close" appended by SetName().
    csString closeName;

    /// The variable that this widget is subscribed to
    csString subscribedVar;

    /**
     * Stores the bgColour for this widget.
     *
     * @remark Set to -1 if no colour should be used.
     */
    int bgColour;

    /// Background image.
    csRef<iPawsImage> bgImage;

    /// border created by GetBorder().
    pawsBorder* border;

    /// whether the title in the border should be shadow font
    bool borderTitleShadow;

    /// The attachpoints for when a resize takes place.
    int attachFlags;

    /// Flag for widget focus.
    bool hasFocus;

    /// Flag for mouse over behavior.
    bool hasMouseFocus;

    /// The direction and percentage of current fade in progress.
    float fadeVal;

    /// The original background alpha.
    int alpha;

    /// The minimum background alpha.
    int alphaMin;

    /// Set to false to disable fading.
    bool fade;

    /// The speed of the fading process.
    float fadeSpeed;

    /**
     * Path to the file that describes context menu of our widget
     * (invoked by mouse right-click).
     *
     * @remark If blank, then there is no context menu for our widget.
     */
    csString contextMenuFile;

    /**
     * Existing context menu of the widget.
     *
     * @remark NULL if there is no menu created at the moment.
     */
    pawsMenu* contextMenu;

    /// Custom border colors for this window.
    int borderColours[5];

    /// Flag to indicate using custom border colors.
    bool hasBorderColours;

    /// Optional font to use when drawing this widget.
    csRef<iFont> myFont;

    /// Optional color to use when drawing text with this widget.
    int defaultFontColour;

    /// Optional color to use when doing text dropshadows.
    int defaultFontShadowColour;

    /// Default font size.
    float defaultFontSize;

    /// Current font size.
    float fontSize;

    /// Determines whether or not to scale font when a widget it resized.
    bool scaleFont;

    /// Name of currently selected font.
    csString fontName;

    /// Current font style.
    int fontStyle;

    /**
     * Used in the SelfPopulate functions to map to xml nodes for
     * each widget.
     */
    csString xmlbinding;

    /// Tooltip to be displayed.
    csString toolTip;

    /// Default tool tip string for the widget.
    csString defaultToolTip;

    /// Masking image, used for nice-looking stuff.
    csRef<iPawsImage> maskImage;

    /**
     * Flag determines if WidgetAT() ignores this widget.
     *
     * @remark Default is FALSE. Set during LoadAttributes().
     */
    bool ignore;

    /// Stores margin value. Used in LoadAttributes().
    int margin;

    /// Holds any extra data the widget may need.
    iWidgetData* extraData;

    /// Whether we need to do a r2t update.
    bool needsRender;

    /**
     * Whether to draw this widget (different from visible,
     * used to decide if Draw() is called via parent-child tree).
     */
    bool parentDraw;

    /**
     * Flag of whether OnUpdateData should overwrite the previous value or add to it.
     *
     * Used by Multi-line edit currently.
     *
     * @remark TRUE by default because everything else should overwrite.
     */
    bool overwrite_subscription;

    csString subscription_format;

    pawsScript* scriptEvents[PW_SCRIPT_EVENT_COUNT];

    csHash<csString,csString> defaultWidgetStyles;

    bool ReadDefaultWidgetStyles(iDocumentNode* node);

public:

    pawsWidget();
    pawsWidget(const pawsWidget &origin);


    virtual ~pawsWidget();

    /**
     * Locate a widget that is at these screen coordindates.
     *
     * This will recurse through all the children as well looking
     * for the lowest widget that contains these coordinates.
     *
     * @param x The x screen position
     * @param y The y screen position
     * @return NULL if this widget does not contain these coordinates.
     *         this if it contains these coordinates and none of it's children
     *         do.
     */
    virtual pawsWidget* WidgetAt(int x, int y);

    virtual void Ignore(bool ig)
    {
        ignore = ig;
    }

    /**
     * Does an action based on this string.
     *
     * @param action The action to perform.
     */
    virtual void PerformAction(const char* action);

    /**
     * Returns the csRect that defines widget area.
     *
     * @return screenFrame
     */
    virtual csRect GetScreenFrame();

    /**
     * Returns the default csRect.
     *
     * @return defaultFrame
     */
    virtual csRect GetDefaultFrame()
    {
        return defaultFrame;
    }

    /**
     * Is the widget currently set visible?
     *
     * @return bool
     */
    bool IsVisible()
    {
        return visible && (!parent || parent->IsVisible());
    }

    /**
     * Make the widget visible or hides it.
     *
     * @param visible TRUE calls Show(), FALSE calls Hide().
     */
    void SetVisibility(bool visible)
    {
        if(visible) Show();
        else Hide();
    }

    /**
     * Makes widget visible and brings it to the front.
     *
     * Sets visible TRUE shows border if present then calls BringToTop() on
     * itself.
     */
    virtual void Show();

    /**
     * Makes widget visible and brings it to the front but behind widget with
     * current focus.
     *
     * Sets visible TRUE shows border if present then calls Show() on widget
     * that had focus when ShowBehind() was called.
     */
    virtual void ShowBehind();

    /**
     * Makes widget invisible and removes focus if widget has current focus.
     *
     * Sets visible FALSE, hides border if present and then if focused when
     * called it calls SetCurrentFocusedWidget(NULL).
     */
    virtual void Hide();

    /**
     * Simply calls Hide() unless overidden.
     */
    virtual void Close()
    {
        Hide();
    }

    /**
     * Test button for activity.
     *
     * @param button The button to test.
     * @param modifiers Modifier to use.
     * @param pressedWidget The widget with the event.
     * @return TRUE if pressed, FALSE otherwise
     */
    virtual bool CheckButtonPressed(int button, int modifiers, pawsWidget* pressedWidget);

    /**
     * Test button for activity.
     *
     * @param button The button to test.
     * @param modifiers Modifier to use.
     * @param pressedWidget The widget with the event.
     * @return TRUE if pressed, FALSE otherwise
     */
    virtual bool CheckButtonReleased(int button, int modifiers, pawsWidget* pressedWidget);

    /**
     * Allow pawsButton to simulate button pushes based on keypresses.
     * Return true if key is handled.
     */
    virtual bool CheckKeyHandled(int /*keyCode*/)
    {
        return false;
    }

    /**
     * Add a child widget to this widget.
     *
     * This widget is then responsible for deleting this child.
     *
     * @param widget The child to add.
     */
    void AddChild(pawsWidget* widget);

    /**
     * Add a child widget to this widget at a specified position.
     *
     * This widget is then responsible for deleting this child.
     *
     * @param widget The child to add.
     */
    void AddChild(size_t Index, pawsWidget* widget);

    /**
     * Removes the widget from list of children and destructs it.
     *
     * @param widget The child to delete.     */
    virtual void DeleteChild(pawsWidget* widget);

    /**
     * Removes and destructs itself.
     */
    void DeleteYourself()
    {
        parent->DeleteChild(this);
    }

    /**
     * Removes the widget from list of children but does NOT destruct it.
     *
     * @param widget The widget to remove.
     * @remark Ownership goes to the caller.
     */
    void RemoveChild(pawsWidget* widget);

    /**
     * Used if you need to loop through the children of a widget.
     *
     * @param i ID of the child
     */
    pawsWidget* GetChild(size_t i)
    {
        return children.Get(i);
    }

    /**
     * Used if you need to loop through the children of a widget
     *
     * @return Count of the children
     */
    size_t GetChildrenCount()
    {
        return children.GetSize();
    }

    /**
     * Returns true, if 'widget' is child of our widget, even if it
     * is indirect.
     *
     * @param widget The widget to test.
     */
    bool IsIndirectChild(pawsWidget* widget);

    /**
     * Returns true, if this widget is the child of 'someParent'.
     *
     * @param someParent The parent to check for.
     */
    bool IsChildOf(pawsWidget* someParent);

    /**
     * Returns true if widget equals this, or widget is a child of this.
     *
     * If widget is null, false is returned.
     */
    bool Includes(pawsWidget* widget);

    /**
     * Set the owner of this widget.
     *
     * @param widget The owner of this widget.
     */
    void SetParent(pawsWidget* widget);

    /**
     * Find a child widget of this widget.
     *
     * Recurses down through the children as well looking for widget.
     *
     * @param name The name of the widget to look for.
     * @param complain If true, print an error message if widget is not found.
     * @return A pointer to the widget if found. NULL otherwise.
     */
    pawsWidget* FindWidget(const char* name, bool complain = true);

    /**
     * Find a child widget of this widget.
     *
     * Recurses down through the children as well looking for widget.
     *
     * @param id The id of the widget to look for.
     * @param complain If true, print an error message if widget is not found.
     * @return A pointer to the widget if found. NULL otherwise.
     */
    pawsWidget* FindWidget(int id, bool complain = true);

    /**
     * Find a child widget of this widget with the given XML binding.
     *
     * Recurses down through the children as well looking for widget.
     *
     * @param xmlbinding The xmlbinding of the widget to look for.
     * @return A pointer to the widget if found. NULL otherwise.
     */
    pawsWidget* FindWidgetXMLBinding(const char* xmlbinding);

    /**
     * Get the xml nodes of this widget.
     *
     * @return xmlbinding
     */
    virtual const csString &GetXMLBinding()
    {
        return xmlbinding;
    }

    /**
     * Sets the xml nodes of this widget.
     *
     * @param xmlbinding
     */
    virtual void SetXMLBinding(csString &xmlbinding)
    {
        this->xmlbinding = xmlbinding;
    }

    /**
     * Get this widget's parent.
     *
     * @return parent
     */
    pawsWidget* GetParent()
    {
        return parent;
    }

    /**
     * Load a widget based on its \<widget\>\</widget\> tag.
     *
     * @param node The xml data for the widget.
     * @remark Recursivly loads all child widgets.
     */
    virtual bool Load(iDocumentNode* node);

    /**
     * Load standard widget attributes based on its \<widget\>\</widget\> tag.
     *
     * @param node The xml data for the widget.
     */
    virtual bool LoadAttributes(iDocumentNode* node);

    /**
     * Load event scripts for this widget.
     *
     * @param node The xml node for the widget.
     */
    virtual bool LoadEventScripts(iDocumentNode* node);

    /**
     * Load widget children based on subtags of its \<widget\>\</widget\> tag.
     *
     * @param node The xml data for the widget.
     */
    virtual bool LoadChildren(iDocumentNode* node);

    /**
     * Parses XML file 'fileName', finds first widget tag and Load()s itself
     * from this tag.
     *
     * 'fileName' is standard path of the XML file to load.
     *
     * @see psLocalization::FindLocalizedFile()
     */
    bool LoadFromFile(const csString &fileName);

    /**
     * Setup this widget.
     */
    virtual bool Setup(iDocumentNode* /*node*/)
    {
        return true;
    }

    /**
     * This is called after the widget and all of it's children have been
     * created.
     *
     * @remark This can be useful for widgets that want to get pointers to
     * some of it's children for quick access.
     */
    virtual bool PostSetup()
    {
        return true;
    }

    /**
     * This function parses the xml string and calls SelfPopulate
     * with the resulting DOM structure if valid.
     *
     * @param xmlstr The xml string to process.
     * @remark This is NOT the same thing as Setup, which is defining the
     * widget structure. This is for data.
     */
    virtual bool SelfPopulateXML(const char* xmlstr);

    /**
     * This function allows a widget to fill in its own contents from an xml
     * node supplied and calls the same function for all children.
     *
     * @param node The xml data for the widget.
     * @return bool TRUE if no children, FALSE if there are problems with a
     * node.
     * @remarks User should overload this function to populate different types
     * of widgets. Call this base class function to populate children.
     * This is NOT the same as Setup(), which is defining height, width,
     * styles, etc.  This is data.
     */
    virtual bool SelfPopulate(iDocumentNode* node);

    /**
     * Does the first part of the drawing. It draws the background of the window.
     *
     * @return TRUE if the widget can be drawn.
     */
    bool DrawWindow();

    /**
     * Does the second part of the drawing drawing the background of the window.
     */
    void DrawForeground();

    /**
     * Draws the widget and all of it's children.
     *
     * @remarks Uses clipping rect of it's parent to define drawing area.
     * If the drawing area defined is empty it returns.
     */
    virtual void Draw();

    virtual void Draw3D(iGraphics3D*) {}

    /**
     * Draws the background with a color or an image.
     *
     * @remarks Uses focus status to apply appropriate fading.
     */
    virtual void DrawBackground();

    /**
     * Draws all children marked visible.
     */
    virtual void DrawChildren();

    /**
     * Draws the mask picture.
     *
     * @remark Uses focus status to apply appropriate fading.
     */
    virtual void DrawMask();

    /**
     * Whether to draw via the parent-child draw tree.
     */
    virtual bool ParentDraw() const
    {
        return parentDraw || !PawsManager::GetSingleton().UsingR2T();
    }

    /**
     * Test widget to see if it is resizable.
     *
     * @return isResizable Flag to set a widget resizable.
     * @remark This is set by marking the resizable attribute yes or no in the
     * widget's xml data.
     */
    bool IsResizable()
    {
        return isResizable;
    }

    /**
     * Sets the showResize flag, controlling if the resize widget should be drawn.
     *
     * @param v Value
     */
    void SetResizeShow(bool v)
    {
        showResize = v;
    }

    /**
     * Registers mode with the windowManager.
     *
     * @param isModal set TRUE to make the widget modal.
     */
    void SetModalState(bool isModal);

    /**
     * Changes hasFocus to TRUE and reports status to parent.
     *
     * @return bool
     * @remark Acts recursively on it's parents. If the widget cannot
     * be focused, this function needs be overridden.
     */
    virtual bool OnGainFocus(bool /*notifyParent*/ = true)
    {
        hasFocus = true;
        if(hasFocus) return true;
        if(parent) parent->OnGainFocus();
        return true;
    }

    /**
     * Sets hasFocus false and notifys parent.
     */
    virtual void OnLostFocus()
    {
        if(!hasFocus) return;
        hasFocus = false;
        if(parent) parent->OnLostFocus();
    }

    /**
     * Sets the new position of the close button.
     */
    virtual void OnResize();

    /**
     * Called once the mouse up is done after resizing a widget.
     */
    virtual void StopResize();

    /**
     * Get the color for this widgets border.
     *
     * @param which The index value in borderColor to get.
     * @returns It's own border color if it has one, if it doesn't have
     * it's own then the parent's, or if no parent it returns the default
     * for the window manager.
     */
    virtual int GetBorderColour(int which);

    /**
     * Set the size of this widget and it's position relative to the parent.
     *
     * @param x The offset from the parent's minimum x value.
     * @param y The offset from the parent's minimum y value.
     * @param width Value to be used for default width.
     * @param height Value to be used for default height.
     */
    virtual void SetRelativeFrame(int x, int y, int width, int height);

    /**
     * Resets the position, width and height to the default position.
     */
    virtual void ResetToDefaultFrame()
    {
        screenFrame = defaultFrame;
    }

    /**
     * Set the position of this widget relative to the parent.
     *
     * @param x The offset from the parent's minimum x value.
     * @param y The offset from the parent's minimum y value.
     * @remark Recalculates the screen positions of all of it's children.
     */
    virtual void SetRelativeFramePos(int x, int y);

    /**
     * Sets defaultFrame and screenFrame size attributes.
     *
     * @param width Value to be used for default width.
     * @param height Value to be used for default height.
     * @remark Recalculates the screen positions of all of it's children.
     */
    virtual void SetRelativeFrameSize(int width, int height);

    /**
     * Modify attachFlags to control widget construction.
     *
     * @param flags The new value for attachFlags.
     * @remark attachFlags is 0 by default.
     */
    virtual void SetAttachFlags(int flags)
    {
        attachFlags = flags;
    }

    /**
     * Manage mouse down event to test for and apply window changes.
     *
     * @param button Type of button: 1 resizable or movable, 2
     * context menu or config window.
     * @param modifiers Used with PAWS_CONSTRUCTION.
     * @param x Used to test for resize.
     * @param y Used to test for resize.
     * @return bool TRUE if movable or resizable.
     * @remark calls OnMouseDown on it's parent.
     */
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);

    /**
     * Manage mouse up event.
     *
     * @return bool Parent's result or FALSE if no parent.
     * @remark Acts recursively on it's parents.
     */
    virtual bool OnMouseUp(int button, int modifiers, int x, int y);

    /**
     * Manage mouse double click event.
     *
     * @return bool Parent's result or FALSE if no parent.
     * @remark Acts recursively on it's parents.
     */
    virtual bool OnDoubleClick(int button, int modifiers, int x, int y);


    /**
     * Called whenever the mouse enters this widget.
     *
     * @return bool Parent's result or TRUE if no parent.
     * @remark Acts recursively on it's parents.
     */
    virtual bool OnMouseEnter();

    /**
     * Called whenever the mouse leaves this widget.
     *
     * @return bool Parent's result or TRUE if no parent.
     */
    virtual bool OnMouseExit();

    /**
     * Called when mouse enters a child widget.
     *
     * If child wants to inform parent.
     * @param child The child widget.
     * @return true
     */
    virtual bool OnChildMouseEnter(pawsWidget* child);

    /**
     * Called when a mouse exits a child widget.
     *
     * If child wants to inform parent.
     * @param child The child widget.
     * @return bool Parent's result or TRUE if no parent.
     */
    virtual bool OnChildMouseExit(pawsWidget* child);

    /**
     * Process keydown messages.
     *
     * @param keyCode The code for the pressed key.
     * @param keyChar The key pressed.
     * @param modifiers Used to modify tab behavior.
     * @return bool TRUE for success FALSE if no action.
     * @remark If you override this, be sure to also override
     * GetFocusOverridesControls() as returning true
     */
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);

    /**
     * Process joypadDown messages.
     *
     * @param key The key pressed.
     * @param modifiers Used to modify behavior.
     * @return bool TRUE for success FALSE if no action.
     */
    virtual bool OnJoypadDown(int key, int modifiers);

    /**
     * Process Clipboard content, as a response to RequestClipboardContent
     *
     * @param content The content of the clipboard
     * @return bool TRUE for success FALSE if no action.
     */
    virtual bool OnClipboard(const csString &content);

    /**
     * Test focus of the widget.
     *
     * @return hasFocus Current status of the widget.
     */
    virtual bool HasFocus()
    {
        return hasFocus;
    }

    /**
     * Test if the widget should intercept all key presses.
     *
     * @remark False by default, should be true for text entry widgets
     */
    virtual bool GetFocusOverridesControls() const
    {
        return false;
    }

    /**
     * Move this widget up the z order to the top.
     *
     * If the widget is alwaysOnTop, it will be placed in position 0.
     * If the widget is not alwaysOnTop, it will be placed after the last
     * alwaysOnTop widget. Will also recurse up through the parents to bring
     * it up as well.
     * @param widget Widget to bring forward.
     */
    virtual void BringToTop(pawsWidget* widget);

    /**
     * Move this widget down the z order to the bottom
     *
     * @param widget Widget to bring forward.
     */
    virtual void SendToBottom(pawsWidget* widget);

    /**
     * Get the name of this widget.
     */
    const char* GetName()
    {
        return name;
    }

    /**
     * Set the name of this widget.
     */
    void SetName(const char* newName)
    {
        name.Replace(newName);
        closeName = name;
        closeName.Append("_close");
    }

    /**
     * Returns the closeName of this widget.
     */
    const char* GetCloseName()
    {
        return closeName;
    }

    /**
     * Move a widget by a delta amount.
     *
     * This will recurse through all the children calling MoveDelta on each.
     * @param dx Delta x
     * @param dy Delta y
     */
    virtual void MoveDelta(int dx, int dy);

    /**
     * Moves this widget and all of its children to a new screen location.
     *
     * @param x new x
     * @param y new y
     */
    virtual void MoveTo(int x, int y);

    /**
     * Move this widget so that its center is at given location.
     *
     * @param x Screen coordinate X.
     * @param y Screen coordinage Y.
     */
    virtual void CenterTo(int x, int y);

    /**
     * Move this widget so that its center is at mouse pointer, but it is fully on screen.
     */
    virtual void CenterToMouse();

    /**
     * Resize a widget based on the current mouse position.
     *
     * This will recurse through all the children calling Resize() on each.
     * @param flags  The resize direction.
     */
    virtual void Resize(int flags);

    /**
     * Resize a widget by a delta amount.
     *
     * This will recurse through all the children calling Resize() on each.
     * @param dx Delta x
     * @param dy Delta y
     * @param flags  The resize direction.
     */
    virtual void Resize(int dx, int dy, int flags);

    /**
     * Set the size of a particlar widget.
     *
     * This will also call Resize() all of the children widgets to
     * adjust to the new parent size.
     * @param newWidth The new width of this widget.
     * @param newHeight The new height of this widget.
     */
    virtual void SetSize(int newWidth, int newHeight);

    /**
     * Same as above, but does not resize children.
     */
    virtual void SetForceSize(int newWidth, int newHeight);

    /**
     * Creates a new border and links it to the widget.
     *
     * @param style The style of border to create.
     */
    virtual void UseBorder(const char* style = 0);

    /**
     * This returns the border created by UseBorder().
     */
    virtual pawsBorder* GetBorder()
    {
        return border;
    }

    /**
     * This returns the BORDER_BUMP style.
     *
     * @remark Other styles available include: BORDER_RAISED, BORDER_SUNKEN
     * and BORDER_ETCHED.
     */
    virtual int GetBorderStyle()
    {
        return BORDER_BUMP;
    }

    /**
     * Sets the background image, logs error if NULL.
     *
     * @param imageName The new background image.
     * @remark Loads image description and gets initial alpha level.
     */
    virtual void SetBackground(const char* imageName);

    /**
     * Retrieve the background image name.
     *
     * @return csString resourceName or "" if not found.
     */
    csString GetBackground();

    /**
     * Sets the alpha level of the background.
     *
     * @param alphaValue The new alpha level.
     * @remark Updates current alpha level sets alpha and alphaMin to
     * alphaValue.
     */
    virtual void SetBackgroundAlpha(int alphaValue);

    /**
     * Resize a widget based on it's parent's size.
     */
    virtual void Resize();

    /**
     * Called whenever a button is pressed.
     *
     * @param button The button pressed.
     * @param keyModifier Modifier key in effect.
     * @param widget The widget the button belongs to.
     * @return bool Parent's result or FALSE if no parent.
     */
    virtual bool OnButtonPressed(int button, int keyModifier, pawsWidget* widget)
    {
        if(parent)
            return parent->OnButtonPressed(button, keyModifier, widget);

        return true;
    }

    /**
     * Called whenever a button is released.
     * @param button The button released.
     * @param keyModifier Modifier key in effect.
     * @param widget The widget the button belongs to.
     * @return bool Parent's result or FALSE if no parent.
     */
    virtual bool OnButtonReleased(int button, int keyModifier, pawsWidget* widget)
    {
        if(parent)
            return parent->OnButtonReleased(button, keyModifier, widget);
        return false;
    }

    /**
     * Called whenever a window is scrolled.
     *
     * @param scrollDirection The direction to move.
     * @param widget The scrollbar widget being manipulated.
     * @return bool Parent's result or FALSE if no parent.
     */
    virtual bool OnScroll(int scrollDirection, pawsScrollBar* widget)
    {
        if(parent) return parent->OnScroll(scrollDirection, widget);
        return false;
    }

    /**
     * Called whenever a widget is selected.
     *
     * @param widget The selected widget.
     * @return bool
     */
    virtual bool OnSelected(pawsWidget* widget);

    /**
     * Called whenever a menu action occurs.
     *
     * @param widget The widget acted upon.
     * @param action The action to take.
     * @return TRUE on menu destroy and FALSE on configure
     */
    virtual bool OnMenuAction(pawsWidget* widget, const pawsMenuAction &action);

    /**
     * Called whenever a widget is selected.
     *
     * @param widget The widget acted upon.
     * @return Notify parent and return result FALSE if no parent.
     */
    virtual bool OnChange(pawsWidget* widget);

    /**
     * Change the id of a widget.
     *
     * @param newID The id number to change to.
     */
    void SetID(int newID)
    {
        id = newID;
    }

    /**
     * Gets the id of a widget.
     *
     * @return id The widget id.
     */
    int  GetID()
    {
        return id;
    }

    /**
     * Tests value of this widgets alwaysOnTop flag.
     *
     * @return bool alwaysOnTop.
     */
    bool IsAlwaysOnTop()
    {
        return alwaysOnTop;
    }

    /**
     * Sets value of this widgets alwaysOnTop flag.
     *
     * @param value True to set always on top.
     * @remark Will cause the parent to bring this widget to the top.
     */
    void SetAlwaysOnTop(bool value)
    {
        alwaysOnTop = value;
        if(parent)
            parent->BringToTop(this);
    }

    /**
     * Makes a widget movable.
     *
     * @param value True if movable.
     */
    void SetMovable(bool value)
    {
        movable = value;
    }

    /**
     * Changes the text for a tool tip.
     *
     * @param text The new text.
     */
    void SetToolTip(const char* text)
    {
        toolTip.Replace(text);
    }

    /**
     * Gets the text from a tool tip.
     *
     * @return csString The tool tip text.
     */
    csString &GetToolTip()
    {
        return toolTip;
    }

    /**
     * Changes the format of a tool tip.
     *
     * @param fmt The format string.
     * @param ... These define the new format.
     */
    void FormatToolTip(const char* fmt, ...);

    /**
     * Sets the tool tip to the one defined in the XML.
     */
    void SetDefaultToolTip()
    {
        toolTip.Replace(defaultToolTip);
    }

    /**
     * Called whenever an item in a child list box is selected.
     *
     * @param selected The listbox that has the item selected.
     * @param status From listbox when a row is selected.
     */
    virtual void OnListAction(pawsListBox* selected, int status)
    {
        if(parent) parent->OnListAction(selected,status);
    }

    /**
     * Saves the position of this widget to the config file.
     *
     * This is done if the savedWindowPos is set to true, this can be done
     * inside the xml file as  savepositions="yes"
     */
    virtual void SavePosition();

    /**
     * Get the position of this widget that was stored in a cfg file.
     *
     * @return A csrect that is the new position. Or return the position that
     * is in the XML file.
     */
    virtual csRect LoadPositions();

    /**
     * Sets the type of factory.
     *
     * @param myfactory The new factory type.
     */
    void SetFactory(const char* myfactory)
    {
        factory = myfactory;
    }

    /**
     * Gets the current factory type.
     *
     * @return factory The current factory type.
     */
    const char* GetType()
    {
        return factory;
    }

    /**
     * Returns the actual width assuming the passed value was in 800x600
     * resolution.
     */
    int GetActualWidth(int myValue=-1)
    {
        if(myValue == -1)
            return screenFrame.Width();

        if(!resizeToScreen)
            return myValue;

        int desktop = graphics2D->GetWidth();
        if(myValue > 800)
            return desktop;

        float value = (float)myValue;

        float ret = (value * desktop) / 800.0f + 0.5f;
        return ((int)ret);
    }

    /**
     * Returns the actual Height assuming the passed value was in 800x600
     * resolution.
     */
    int GetActualHeight(int myValue=-1)
    {
        if(myValue == -1)
            return screenFrame.Height();

        if(!resizeToScreen)
            return myValue;

        int desktop = graphics2D->GetHeight();
        if(myValue > 600)
            return desktop;

        float value = (float)myValue;

        float ret = (value * desktop) / 600.0f + 0.5f;
        return ((int)ret);
    }

    int GetLogicalWidth(int myValue)
    {
        if(!resizeToScreen)
            return myValue;

        int desktop = graphics2D->GetWidth();
        myValue *= 800;
        myValue /= desktop;
        return myValue;
    }

    int GetLogicalHeight(int myValue)
    {
        if(!resizeToScreen)
            return myValue;

        int desktop = graphics2D->GetHeight();
        myValue *= 600;
        myValue /= desktop;
        return myValue;
    }

    /**
     * Sets hasMouseFocus.
     *
     * @param value TRUE causes widget to react to mouse focus.
     * @remark hasMouseFocus is used by DrawBackground() to fade the
     * background in. Disabled by default.
     */
    virtual void MouseOver(bool value);

    /**
     * Used to control the fading feature of the widget.
     *
     * @param value True if fading feature should be on.
     */
    void SetFade(bool value);

    /**
     * Used to control the font scaling of the widget.
     *
     * @param value True if font scaling should be enabled.
     */
    void SetFontScaling(bool value);

    /**
     * Draw text in the widget at specified location.
     *
     * @param text The text to draw.
     * @param x The x screen position.
     * @param y The x screen position.
     * @param style When set at -1 causes widget to call GetFontStyle().
     * @remark Font style selected by precedence, this, parent and
     * DFFAULT_FONT_STYLE
     */
    void DrawWidgetText(const char* text, int x, int y, int style=-1);

    /**
     * Get the rectangle containing the text DrawWidgetText will produce.
     *
     * @param text The text.
     * @param x The x screen position.
     * @param y The x screen position.
     * @param style When set at -1 causes widget to call GetFontStyle().
     * @remark Font style selected by precedence, this, parent and
     * DFFAULT_FONT_STYLE
     */
    csRect GetWidgetTextRect(const char* text, int x, int y, int style=-1);

    /**
     * Set the minimum height and width to protect the widget from negatives.
     */
    void SetMinSize(int width, int height)
    {
        min_width = width;
        min_height = height;
    }

    void GetMinSize(int &width, int &height)
    {
        width = min_width;
        height = min_height;
    }

    /**
     * Set the max size for height and width.
     *
     * @remark Default is screen height and width.
     */
    void SetMaxSize(int width, int height)
    {
        max_width = width;
        max_height = height;
    }


    /**
     * Sets name of the PAWS xml-file that describes context menu of our widget.
     */
    void SetContextMenu(const csString &fileName)
    {
        contextMenuFile = fileName;
    }

    /**
     * If some part of the widget is not within the rectangle of its parent,
     * then the widget is moved inside (if it is small enough).
     */
    void MakeFullyVisible();

    /**
     * This re-calculates a widget's on screen position to draw based on
     * it's relative position and parent screen location.
     */
    void RecalcScreenPositions();

    /**
     * Determines clipping area to use.
     *
     * If a widget is the top level it clips to the canvas. If it has a parent
     * it calculates the intersection between either it's border or
     * screenframe and parent clip. Updates clipRect for future drawing.
     */
    void ClipToParent(bool allowForBackgroundBorder);

    /**
     * This returns the current clipping rectangle.
     *
     * @return csRect clipRect
     */
    csRect ClipRect()
    {
        return clipRect;
    }

    /**
     * Set text color.
     *
     * @param newColour The colour to use from now on for this widget.
     *                  If it's -1 it will make the selection at runtime
     *                  (by checking first the parent then the default colour)
     *                  If it's -2 (default) it will make the choice upon
     *                  this call by setting the default color.
     */
    void SetColour(int newColour = -2);

    /**
     * Set font to use programmatically.
     */
    void SetFont(const char* fontName, int Size=0);

    /**
     * Change font to new size and reload font to make it take effect.
     *
     * @param newSize The font size to change to.
     */
    void ChangeFontSize(float newSize);

    /**
     * Gets the current font size.
     *
     * @return It's own size if it has one, if not then the parent's,
     * or if it doesn't have a parent it returns DEFAULT_FONT_SIZE
     */
    float GetFontSize();

    /**
     * Gets the current font as an iFont.
     *
     * @return It's own font if it has one, if not then the parent's,
     * or if it doesn't have a parent it returns windowManager DEFAULT_FONT.
     */
    iFont* GetFont(bool scaled = true);

    /**
     * Gets the current font color.
     *
     * @return defaultFontColour if it is != -1, if it has a parent
     * use parent->GetFontColour(), if it doesn't have a parent it
     * returns windowManager GetDefaultFontColour().
     */
    virtual int GetFontColour();

    /**
     * Gets the current shadow color.
     *
     * @return default if myFont, if not then the parent's,
     * or if it doesn't have a parent it returns black.
     */
    int    GetFontShadowColour();

    /**
     * Gets the current font style.
     *
     * @return fontStyle if myFont, if not then the parent's,
     * or if it doesn't have a parent it returns DEFAULT_FONT_STYLE.
     */
    int    GetFontStyle();

    /**
     * Sets the current font style.
     */
    void   SetFontStyle(int style);

    /**
     * Determines if the coordinates are within this widget. Typically the border or screenFrame.
     *
     * @param x The x screen position.
     * @param y The y screen position.
     * @return TRUE if yes otherwise FALSE
     */
    virtual bool Contains(int x, int y);

    /**
     * Creates a popup window that the user can use to
     * adjust the settings of this window.
     */
    virtual void CreateWidgetConfigWindow();

    /**
     * Called by ApplyAlphaOnChildren to remove the config
     * window to prevent weird stuff.
     */
    virtual void DestroyWidgetConfigWindow();

    /**
     * Gets the minimim alpha value of this widget.
     */
    int GetMinAlpha()
    {
        return alphaMin;
    };

    /**
     * Gets the maximum alpha value of this widget.
     */
    int GetMaxAlpha()
    {
        return alpha;
    };

    /**
     * Gets the fade value of this widget
     */
    int GetFadeVal()
    {
        return (int)fadeVal;
    };

    /**
     * Sets the minimim alpha of this widget.
     */
    void SetMinAlpha(int value)
    {
        alphaMin=value;
    };

    /**
     * Sets the maximum alpha of this widget.
     */
    void SetMaxAlpha(int value)
    {
        alpha=value;
    };

    /**
     * Gets the fade status of this widget.
     */
    bool isFadeEnabled()
    {
        return fade;
    };

    /**
     * Gets the fading speed.
     */
    float GetFadeSpeed()
    {
        return fadeSpeed;
    };

    /**
     * Sets the fading speed.
     */
    void SetFadeSpeed(float speed)
    {
        fadeSpeed=speed;
    };

    /**
     * Returns whether or not font is being auto-scaled.
     */
    bool isScalingFont()
    {
        return scaleFont;
    }

    /**
     * Sets the border title of this widget.
     */
    void SetTitle(const char* title);

    /**
     * Saves current widget settings to an XML file.
     */
    virtual void SaveSettings();

    /**
     * Loads current widget settings from an XML file.
     */
    virtual void LoadSettings();

    /**
     * This draws the tool tip if a widget is visible and a tool tip is
     * available.
     *
     * @param x The x screen position.
     * @param y The y screen position.
     */
    virtual void DrawToolTip(int x, int y);

    /**
     * Prints the widget names for the widget and all it's children.
     *
     * @param tab String to set tab format.
     */
    virtual void Dump(csString tab = "");

    /**
     *Returns textual description of path to our widget through the widget
     * tree. (direct and indirect parents)
     */
    csString GetPathInWidgetTree();

    /**
     * Sets a masking image which will be drawn after the normal stuff.
     *
     * @param image The image resource name.
     */
    void SetMaskingImage(const char* image);

    /**
     * Clears the masking image.
     */
    void ClearMaskingImage();

    /**
     * Sets the background color
     *
     * @param r Red
     * @param g Green
     * @param b Blue
     */
    void SetBackgroundColor(int r,int g, int b);

    void ClearBackgroundColor()
    {
        bgColour = -1;
    }

    /**
     * Changes filename to the name provided.
     */
    void SetFilename(const char* name);

    /**
     * Returns the filename.
     */
    const char* GetFilename();

    /**
     * Reloads widget from XML file.
     */
    void ReloadWidget();

    /**
     * Stores extra data into this widget.
     *
     * @param data The extra data to store.
     * @remark The widget takes ownership of the data.
     */
    void SetExtraData(iWidgetData* data)
    {
        delete extraData;
        extraData = data;
    }

    /**
     * Grabs extra data stored in the widget.
     *
     * @return The extra data stored in this widget.
     */
    iWidgetData* GetExtraData()
    {
        return extraData;
    }

    /**
     * Tests if widget settings (alpha, fade, etc) are configurable.
     *
     * @return bool
     */
    bool IsConfigurable()
    {
        return configurable;
    }

    virtual void OnUpdateData(const char* /*dataname*/, PAWSData & /*data*/) {}
    virtual void NewSubscription(const char* dataname)
    {
        subscribedVar = dataname;
    }

    /**
     * Sets up the title bar for the widget.
     *
     * @param text The name that should go in the title.
     * @param image The name of the backround image for the title.
     * @param align The alignment of the text.
     * @param close_button "yes" means to add a close button to the title bar.
     * @param shadowTitle True if title should have shadow.
     *
     * @return true if all was successful.
     */
    bool SetTitle(const char* text, const char* image, const char* align, const char* close_button, const bool shadowTitle = true);

    void RemoveTitle();

    const char* FindDefaultWidgetStyle(const char* factoryName);

    /**
     * Marks that we need to r2t.
     */

    void SetNeedsRender(bool needs)
    {
        if(parent)
            parent->SetNeedsRender(needs);
        else
            needsRender = needs;
    }

    /**
     * Whether we need to r2t.
     */
    bool NeedsRender() const
    {
        return needsRender;
    }

protected:
    /**
     * This will check to see if the mouse is over the resize hot spot.
     */
    int ResizeFlags(int mouseX, int mouseY);

    /**
     * Creates context menu from file ('contextMenuFile' attribute)
     */
    bool CreateContextMenu();

    /**
     * Calculates the right position in array of children for a given
     * child (depends on always-on-top state).
     */
    int CalcChildPosition(pawsWidget* child);

    /**
     * Loads custom border color preferences.
     */
    void LoadBorderColours(iDocumentNode* node);

    /**
     * Gets button from widget and sets it's location.
     */
    void SetCloseButtonPos();

    /**
     * Convert from string flag to int flag.
     */
    int GetAttachFlag(const char* flag);

public:
    /**
     * Executes any pawsScript associated with the given event.
     */
    void RunScriptEvent(PAWS_WIDGET_SCRIPT_EVENTS event);

    const char* ToString()
    {
        return name.GetDataSafe();
    }
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    virtual void SetProperty(const char* ptr, double value);

    csArray<csString> publishList;
};

//----------------------------------------------------------------------------



class pawsWidgetFactory
{
public:
    virtual ~pawsWidgetFactory() {};

    virtual pawsWidget* Create();

    virtual pawsWidget* Create(const pawsWidget* origin);

    virtual const char* GetName()
    {
        return factoryName;
    }

protected:
    virtual void Register(const char* name);


    csString factoryName;
};

//---------------------------------------------------------------------------


class pawsBaseWidgetFactory : public pawsWidgetFactory
{
public:
    pawsBaseWidgetFactory()
    {
        Register("pawsWidget");
    }

    pawsWidget* Create()
    {
        return new pawsWidget();
    }
};


#define CREATE_PAWS_FACTORY( factoryName ) \
class factoryName##Factory : public pawsWidgetFactory \
{ \
public: \
    factoryName##Factory( ) \
    { \
        Register( #factoryName ); \
    } \
 \
    pawsWidget* Create() \
    { \
        return new factoryName( ); \
    } \
    pawsWidget* Create(const pawsWidget* origin)\
    {\
        const factoryName * widget = dynamic_cast<const factoryName *>(origin);\
        if(widget)\
        {\
            return new factoryName(*widget);\
        }\
        return NULL;\
    }\
}




/** @} */

#endif


