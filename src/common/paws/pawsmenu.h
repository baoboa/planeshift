/*
 * pawsmenu.h - Author: Ondrej Hurt
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
 */

#ifndef PAWS_MENU_HEADER
#define PAWS_MENU_HEADER

#include <csutil/parray.h>
#include <iutil/document.h>
#include "pawswidget.h"
#include "pawstextbox.h"
#include "pawsbutton.h"


class pawsIMenu;
class pawsMenu;
class pawsIMenuItem;
class pawsMenuItem;

/**
 * \addtogroup common_paws
 * @{ */

/**
 * When pawsMenu is created, target of notification messages (OnMenuAction) must be set by SetNotify() method.
 * When pawsMenu creates a submenu, it sets the target of notification messages automatically.
 *
 * Events with action name="submenu" are handled by the menu and are not delivered to subscriber.
 *
 * When menu wants to destruct itself, it must not do this directly but
 * it must call DestroyMenu() method. This notifies parent and children of menu about the destruction
 * and then sends MENU_DESTROY_ACTION_NAME event via OnMenuAction().
 * The receiving widget is responsible for destruction of the menu.
 * This way the widget which uses the menu knows about its destruction and all references
 * to the menu are dropped.
 *
 * All pawsMenu instances should be added to children of mainWidget.
 *
 *
 *
 * pawsMenuItem XML definition
 * ---------------------------
 * attributes:
 *    image        - name of image (if missing, image is disabled)
 *    label        - text of label
 *    colour       - colour of label
 *    checked      - state of checkbox [yes|no] (if missing, checkbox is disabled)
 *    CheckboxOn   - images of checkbox states (if missing, default is used)
 *    CheckboxOff
 *    LabelWidth   - width of label (if missing, it is set to the width of the letters inside)
 *    spacing      - horizontal spacing between image, label and checkbox (if missing, default is used)
 *    border       - space between border and inner components (if missing, default is used)
 *
 *    action       - name of event that is sent when user activates the item
 *    param1       - arbitrary number of additional variables that are included in the event
 *    param2
 *       .
 *       .
 *       .
 *    paramN
 *
 *
 * pawsMenu XML definition
 * -----------------------
 * attributes:
 *    align     - "left" or "center"
 *    autosize  - if "true", then widget size is calculated and set so its content just fits in
 *    colour    - colour of menu label
 *
 * All widgets inside are added as menu items.
 *
 */


/**
 * pawsMenuAction - when OnMenuAction event is invoked, this structure is sent to event handler
 * and tells it what action the handler should take.
 *
 * Content of pawsMenuAction is defined in each regular menu item.
 */
class pawsMenuAction
{
public:
    csString name;
    csArray<csString> params;
};

/**
 * Value of pawsMenuAction::name when window request its destruction.
 */
#define MENU_DESTROY_ACTION_NAME "MenuWantsDestroy"

//-----------------------------------------------------------------------------
//                          interface pawsIMenu
//-----------------------------------------------------------------------------

/**
 * Possible reasons of closing of menu.
 */
enum pawsMenuClose
{
    closeAction,          ///< Action to close the menu.
    closeSiblingOpened,   ///< Sibling was opened.
    closeCloseClicked,    ///< Close was cliked.
    closeParentClosed,    ///< Parent closed.
    closeChildClosed      ///< Child closed.
};

/**
 * pawsIMenu is common interface to menus \ref pawsMenu.
 */
class pawsIMenu : public pawsWidget
{
public:
    /**
     * Constructor.
     */
    pawsIMenu() {  };

    /**
     * Constructor.
     */
    pawsIMenu(const pawsIMenu &origin): pawsWidget(origin)
    {

    }

    /**
     * Sets parent menu of submenu.
     */
    virtual void SetParentMenu(pawsIMenu* parentMenu) = 0;

    /**
     * Called when parent menu was closed.
     */
    virtual void OnParentMenuDestroyed(pawsMenuClose reason) = 0;

    /**
     * Called when child menu was closed.
     */
    virtual void OnChildMenuDestroyed(pawsIMenu* child, pawsMenuClose reason) = 0;

    /**
     * Called when another submenu of parent menu was opened.
     */
    virtual void OnSiblingOpened() = 0;

    /**
     * Processes action invoked by user:
     *    - according to action type either opens a new menu or sends notification to subscriber
     * 'item' is menu item that was activated
     * 'action' is description of the action that should be taken
     */
    virtual void DoAction(pawsIMenuItem* item) = 0;

    /**
     * Sets widget that should be receiving OnMenuAction events
     */
    virtual void SetNotify(pawsWidget* notifyTarget) = 0;

};


//-----------------------------------------------------------------------------
//                            class pawsMenu
//-----------------------------------------------------------------------------

/**
 * Possible vertical alignments of menu items:
 */
enum pawsMenuAlign {alignLeft, alignCenter};


/**
 * pawsMenu is standard PAWS menu widget.
 */
class pawsMenu : public pawsIMenu
{
public:
    pawsMenu();
    ~pawsMenu();
    pawsMenu(const pawsMenu &origin);

    //from pawsWidget:
    virtual bool OnButtonPressed(int button, int keyModifier, pawsWidget* widget);
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool Setup(iDocumentNode* node);
    virtual bool PostSetup();
    virtual void Draw();

    //from pawsIMenu:
    virtual void SetParentMenu(pawsIMenu* parentMenu);
    virtual void OnParentMenuDestroyed(pawsMenuClose reason);
    virtual void OnChildMenuDestroyed(pawsIMenu* child, pawsMenuClose reason);
    virtual void OnSiblingOpened();
    virtual void DoAction(pawsIMenuItem* item);
    virtual void SetNotify(pawsWidget* notifyTarget);


    /* Adds 'item' to the menu, before the 'nextItem' item.
     * If 'nextItem' is NULL, item is added to the end.
     * OWNERSHIP of 'item' goes to pawsMenu
     */
    void AddItem(pawsIMenuItem* item, pawsIMenuItem* nextItem=NULL);

    /**
     * Item is deleted from the menu and destructed.
     */
    void DeleteItem(pawsIMenuItem* item);

    /**
     * Sets action that should be invoked when 'item' is activated by user
     */
    void SetItemAction(pawsIMenuItem* item, const pawsMenuAction &action);

    /**
     * Does menu have any submenus ?
     */
    bool HasSubmenus();

protected:
    /**
     * Calculates and sets position of each menu item
     */
    void SetPositionsOfItems();

    /**
     * Sends OnMenuAction event to notification target
     */
    void SendOnMenuAction(const pawsMenuAction &action);

    /**
     * Sends OnMenuAction event that requests destruction of this menu (MENU_DESTROY_ACTION_NAME)
     */
    void SendDestroyAction();

    /**
     * Sends MENU_DESTROY_ACTION_NAME event and notifies its parent and children.
     * Thanks to this all references from parent and children and the widget that is catching
     * menu events are dropped before the destruction.
     */
    void DestroyMenu(pawsMenuClose reason);

    /**
     * Calculates dimensions of menu items
     */
    int GetContentWidth();
    int GetContentHeight();

    /**
     * Calculates and sets its size according to size of its content
     */
    void Autosize();

    /**
     * Sets appropriate position of a submenu on screen
     */
    void SetSubmenuPos(pawsMenu* submenu, int recommY);

    /**
     * Searches for node with attribute "name" equal to parameter in 'node' and its children.
     * If succeeds returns pointer to the node found, or returns NULL.
     */
    csPtr<iDocumentNode> FindSubmenuNode(iDocumentNode* node, const csString &name);

    /**
     * Sets positions of the Sticky and Close buttons
     */
    void SetButtonPositions();


    pawsIMenu* parentMenu;

    /**
     * list of menu items
     */
    csArray<pawsIMenuItem*> items;

    /**
     * list of open submenus
     */
    csArray<pawsIMenu*> submenus;

    /**
     * target of OnMenuAction events
     */
    pawsWidget* notifyTarget;

    pawsButton* stickyButton, * closeButton;
    pawsTextBox* label;

    csRef<iPawsImage> arrowImage;    // image of right arrow indicating that menu item invokes a submenu

    pawsMenuAlign align;

    /**
     * should Autosize() method be used ?
     */
    bool autosize;

    /**
     * Is the menu "stickied" on screen ?
     * Stickied menu can only be closed by clicking on the closing button
     */
    bool sticky;

    iGraphics2D* graphics2d;
};

CREATE_PAWS_FACTORY(pawsMenu);

//-----------------------------------------------------------------------------
//                            class pawsMenuItem
//-----------------------------------------------------------------------------

class pawsIMenuItem : public pawsWidget
{
public:
    pawsIMenuItem() {}
    pawsIMenuItem(const pawsIMenuItem &origin):pawsWidget(origin)
    {

    }
    /**
     * Makes the menu item send its menu event,
     */
    virtual void Invoke()
    {
    }

    /**
     * Sets/gets content of event that will be sent when the menu item is invoked.
     */
    virtual void SetAction(const pawsMenuAction & /*action*/)
    {
    }
    virtual pawsMenuAction GetAction()
    {
        pawsMenuAction action;
        return action;
    }
};

/**
 * class pawsMenuItem - standard menu item with label, checkbox (optional) and image (optional).
 */
class pawsMenuItem : public pawsIMenuItem
{
public:
    pawsMenuItem();
    pawsMenuItem(const pawsMenuItem &origin);
    //from pawsWidget:
    virtual bool Load(iDocumentNode* node);
    virtual bool Setup(iDocumentNode* node);
    virtual void Draw();

    //from pawsIMenuItem:
    virtual void Invoke();
    void SetAction(const pawsMenuAction &action);
    pawsMenuAction GetAction()
    {
        return action;
    }

    /**
     * Shows or hides checkbox or image:
     */
    void EnableCheckbox(bool enable);
    void EnableImage(bool enable);

    void SetImage(const csString &newImage);
    void SetLabel(const csString &newLabel);

    /**
     * Sets names of bitmaps:
     */
    void SetCheckboxImages(const csString &on, const csString &off);

    /**
     * Sets state of checkbox (true/false)
     */
    void SetCheckboxState(bool checked);

    /**
     * Sets layout values:
     * If labelWidth = -1, then sets width of the label so that the text fits right in.
     */
    void SetSizes(int labelWidth, int spacing, int border);

protected:
    /**
     * Calculates and sets positions of the checkbox, label and image.
     * Then sets the width of the whole menu item.
     */
    void SetLayout();

    /**
     * Reads pawsMenuAction data from XML
     */
    virtual void LoadAction(iDocumentNode* node);


    pawsWidget* image;
    pawsTextBox* label;
    pawsButton* checkbox;

    pawsMenuAction action;

    /**
     * Is image/checkbox visible ?
     */
    bool imageEnabled, checkboxEnabled;

    /**
     * Horizontal distance between parts of the menu item (image, label, checkbox)
     */
    int spacing;

    /**
     * Space between content of the menu item (image, label, checkbox) and border of the item.
     */
    int border;

    iGraphics2D* graphics2d;
};

CREATE_PAWS_FACTORY(pawsMenuItem);


//-----------------------------------------------------------------------------
//                            menu separator
//-----------------------------------------------------------------------------

/**
 * pawsMenuSeparator - special menu item that visually splits the menu to more parts.
 * No OnMenuAction() event can be invoked with this menu item.
 */

class pawsMenuSeparator : public pawsIMenuItem
{
public:
    pawsMenuSeparator();
    pawsMenuSeparator(const pawsMenuSeparator &origin);
    // from pawsWidget:
    virtual void Draw();

protected:
    iGraphics2D* graphics2d;
};

CREATE_PAWS_FACTORY(pawsMenuSeparator);


/** @} */

#endif
