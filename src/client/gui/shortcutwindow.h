/*
* shortcutwindow.h - Author: Andrew Dai
* Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Revision Author: Joe Lyon
* Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_SHORTCUT_WINDOW
#define PAWS_SHORTCUT_WINDOW 

//=============================================================================
// Library Includes
//=============================================================================
// COMMON INCLUDES
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

// CLIENT INCLUDES
#include "pscelclient.h"
#include "../globals.h"
#include "clientvitals.h"
#include "psclientchar.h"

// PAWS INCLUDES
#include "gui/pawscontrolwindow.h"
#include "gui/pawsconfigkeys.h"
#include "paws/pawsprogressbar.h"
#include "pawsscrollmenu.h"


//=============================================================================
// Forward Declarations
//=============================================================================
class pawsChatWindow;

//=============================================================================
// Defines
//=============================================================================
#define NUM_SHORTCUTS    256

//=============================================================================
// Classes 
//=============================================================================

/**
 * 
 */
class pawsShortcutWindow : public pawsControlledWindow, public pawsFingeringReceiver, public psClientNetSubscriber
{
public:
    pawsShortcutWindow();

    virtual ~pawsShortcutWindow();

    virtual bool Setup(iDocumentNode *node);
    virtual bool PostSetup();

    bool OnMouseDown( int button, int modifiers, int x, int y );
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);
    bool OnScroll(int direction, pawsScrollBar* widget);

    void OnResize();
    bool OnFingering(csString string, psControl::Device device, uint button, uint32 mods);

    /**
     * Execute a short cut script.
     *
     * @param shortcutNum is the button ordinal number 
     */
    void ExecuteCommand(int shortcutNum );
    
    /**
     * Get the short cut script.
     *
     * @param shortcutNum is the button ordinal number 
     */
    const csString& GetCommandName(int shortcutNum );

    /**
     * Get the text of a buttons assigned shortcut key 
     *
     * @param shortcutNum is the button ordinal number
     */
    csString GetTriggerText(int shortcutNum);

    /**
     * Load the commands, icon names and shortcut text keys
    **/
    void LoadDefaultCommands();
    void LoadCommandsFile();
    void LoadCommands(const char *FN);
    void SaveCommands();
    void SaveCommands(const char *FN);
    
    void ResetEditWindow();

    void Show();

    void StartMonitors();
    void StopMonitors();
    int GetMonitorState();
   /**
    * return the name of the font
    */
    char const * GetFontName()
    {
        return fontName;
    }

   /**
    * Load the preferences set by the Shortcut Configuration interface
    */
    bool LoadUserPrefs();

protected:
    /// chat window for easy access
    pawsChatWindow* chatWindow;
    CmdHandler *cmdsource;

    csArray<csString> cmds;
    csArray<csString> names;
    csArray<csString> toolTips;
    csArray<csString> icon;

    csRef<iVFS> vfs;

    // The widget that holds the command data during editing
    pawsMultilineEditTextBox* textBox;

    // The widget that holds the button label data during editing
    pawsEditTextBox* labelBox;

    // The widget that holds the shortcut lable data during editing
    pawsTextBox* shortcutText;

    pawsTextBox* title;

    // Widget used to configure the shortcuts
    pawsWidget* subWidget;

    pawsScrollMenu* iconPalette;
    pawsDnDButton*  iconDisplay;
    int             iconDisplayID;

    csString buttonBackgroundImage;

    size_t edit;
    pawsWidget *editedButton;

    virtual void HandleMessage(MsgEntry *msg);


private:
    //status bars, optional;configured in XML
    pawsProgressBar *main_hp;
    pawsProgressBar *main_mana;
    pawsProgressBar *phys_stamina;
    pawsProgressBar *ment_stamina;
    pawsScrollMenu  *MenuBar;

    //custom controls, optional;configured in XML
    pawsButton      *UpButton;
    pawsButton      *DownButton;
    pawsScrollBar   *iconScrollBar;

    csArray<csString>    allIcons;
    csArray<csString>    allNames; //not populated at this time...
    csArray<csString>    stubArray;

    csString          fileName;

    size_t            position;
    size_t            buttonWidth;
    int               textSpacing;
    float             scrollSize;
    int               EditMode;
};


CREATE_PAWS_FACTORY( pawsShortcutWindow );
#endif
