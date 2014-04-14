/*
 * pawsconfigkeys.h - Author: Ondrej Hurt
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

#ifndef PAWS_CONFIG_KEYS_HEADER
#define PAWS_CONFIG_KEYS_HEADER

// CS INCLUDES
#include <csutil/array.h>
#include <iutil/event.h>

// CLIENT INCLUDES
#include "pscharcontrol.h"

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "pawsconfigwindow.h"

class pawsFingeringWindow;
class pawsFingeringReceiver;


#define NO_BIND ""


//////////////////////////////////////////////////////////////////////////////
//                        pawsFingeringReceiver
//////////////////////////////////////////////////////////////////////////////

/** This interface receive OnFingering notification from the FingeringWindow.
  */
class pawsFingeringReceiver 
{
public:
    /// Returns whether the combo was accepted and the fingering window should hide
    virtual bool OnFingering(csString string, psControl::Device device, uint button, uint32 mods) = 0;
    virtual ~pawsFingeringReceiver() {};
};

//////////////////////////////////////////////////////////////////////////////
//                        pawsConfigKeys
//////////////////////////////////////////////////////////////////////////////

/** This window is used to configure keyboard bindings. It is one of the windows 
  * that appear on the right side of pawsConfigWindow.
  */
class pawsConfigKeys : public pawsConfigSectionWindow, public pawsFingeringReceiver
{
public:
    pawsConfigKeys();
    
    /// from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig() { dirty = false; return true; }       
    virtual void SetDefault();
    
    void UpdateNicks(pawsTreeNode * subtreeRoot = NULL);

    /// from pawsWidget:
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    virtual void Draw();

    /// from FingeringWindow
    virtual bool OnFingering(csString string, psControl::Device device, uint button, uint32 mods);

protected:
    /** Creates the tree that makes the whole GUI.
      * @return success
      */
    bool CreateTree();
    /** Adds widgets to all nodes in subtree with root 'subtreeRoot'.
      * -- command label, key combination label and button that opens FingeringWindow    
      */
    void CreateTreeWidgets(pawsTreeNode * subtreeRoot);
    
    /// Sets all labels that hold key combinations (in subtree with root 'subtreeRoot')
    void SetKeyLabels(pawsTreeNode * subtreeRoot);
    
    /// Sets textual description of key combination to given command on screen    
    void SetTriggerTextOfCommand(const csString & command, const csString & trigger);
    
    /** loads pawsFingeringWindow widget from xml.
     * @return success    
     */
    bool FindFingeringWindow();

    pawsTree * tree;                    // the tree that makes whole window GUI
    pawsFingeringWindow * fingWnd;      // pointer to FingeringWindow (object is NOT OWNER)

    csString editedCmd;        // command that is being changed
};




CREATE_PAWS_FACTORY(pawsConfigKeys);



//////////////////////////////////////////////////////////////////////////////
//                        pawsFingeringWindow
//////////////////////////////////////////////////////////////////////////////

/** Class pawsFingeringWindow is a small dialog box that ask the user to press 
  * a key combination that will be bound to some action.
  */
class pawsFingeringWindow : public pawsWidget
{
public:

    pawsFingeringWindow();
    
    void ShowDialog(pawsFingeringReceiver * receiver, const char * editedCmd);

    // from pawsWidget:
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    virtual bool OnKeyDown( utf32_char keyCode, utf32_char keyChar, int modifiers );
    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
    virtual bool PostSetup();
    virtual void Hide();

    /* Prepares dialog GUI so that it can begin keypress detection.
     * @param cmdName The name of edited command.
     */
    void SetupGUIForDetection(const csString & cmdName);

    /// Shows string describing collision with key combination 'event' associated with 'command'         
    void SetCollisionInfo(const char* action);

    /// Sets widget that should be notified the result
    void SetNotify( pawsFingeringReceiver* widget );
        
    virtual bool GetFocusOverridesControls() const { return true; }

private:

    void RefreshCombo();

    pawsFingeringReceiver* notify;  /// Target of event notification
    
    pawsTextBox* labelTextBox;
    pawsTextBox* buttonTextBox;

    uint button;  /// Button pressed
    uint32 mods;  /// Mods used
    psControl::Device device;  /// Keyboard or mouse?
    
    csString combo;  /// Text version of key combination
};


CREATE_PAWS_FACTORY(pawsFingeringWindow);

#endif

