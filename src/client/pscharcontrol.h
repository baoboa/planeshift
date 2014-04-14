/*
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __PSCHARCONTROL_H__
#define __PSCHARCONTROL_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>
#include <csutil/csstring.h>
#include <iutil/eventnames.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "psmovement.h"

class psTriggerHandler;
class psCharController;


/** Defines a generic button trigger.
 *
 *  Each game action that may be activated by a button press has a psControl object.
 *  (or 2, if a secondary trigger is created)  These are managed by psControlManager
 *  which takes incoming CS input events and looks up any registered triggers  Note
 *  that for shortcuts and secondary triggers the internal name listed in the object
 *  may differ from the display name which should be used in the GUI.  The function
 *  GetDisplayName() may be used to convert the name.  (ex: "Shortcut 1" -> "Greet")
 */
struct psControl
{
    /// Pointer to a member function in the psTriggerHandler class.
    typedef void (psTriggerHandler::*TriggerFunction)(const psControl*, bool);

    /// Pointer to an object used by a TriggerFunction.
    typedef const void* DataPtr;

    enum PressType
    {
        NORMAL,  ///< Triggers on press and un-triggers on release
        TOGGLE   ///< Triggers on press and un-triggers on another press
    };

    enum Device
    {
        NONE,      ///< Not set
        KEYBOARD,  ///< Keypress
        MOUSE      ///< Mouseclick
    };

    psControl(const char* n, PressType t, TriggerFunction f)
        : name(n), state(false), device(NONE), button(0), mods(0), type(t), function(f), data(NULL) {}

    csString name;             ///< Name of this control
    bool state;                ///< Is this active?
    Device device;             ///< Location of button
    uint button;               ///< Button this is bound to
    uint32 mods;               ///< Modifiers to go with the button
    PressType type;            ///< Trigger method
    TriggerFunction function;  ///< Function called when triggered
    DataPtr data;              ///< Optional pointer to data needed by function

    void Execute() const;      ///< Does whatever this does
    csString ToString() const; ///< Returns button combo in string form

    static psTriggerHandler* handler;  ///< Static pointer to the function handler class
};


/// Returns a string of the form "mods+button"
csString ComboToString(psControl::Device device, uint button, uint32 mods);

/// Resolves the custom name for a trigger  (shortcut's name, or removes "(sec)" qualifiers)
csString GetDisplayName(const char* name);

/** Function which is used to extract just the modifier bits we use from a CS event.
 *  We use only shift, alt, and ctrl.  The full CS mods also contain caps lock, num
 *  lock, scroll lock, etc. which we do not want to affect button mappings.
 */
// CS strangeness requires different methods for mouse and key modifiers
uint32 GetPSKeyMods(const iEvent* event);
uint32 GetPSMouseMods(const iEvent* event);

/** Mask which may be used to convert CS generated modifier bits to ones used here.
 *  We use only shift, alt, and ctrl.  The full CS mods also contain caps lock, num
 *  lock, scroll lock, etc. which we do not want to affect button mappings.
 */
#define PS_MODS_MASK CSMASK_ALLSHIFTS


/** Manages button functions and triggers.
 *
 *  Each game function which may be controled by a button has a psControl object called a "trigger".
 *  After creating a trigger using the NewTrigger() function, from psCharController::CreateKeys(),
 *  each trigger is assigned a name and a function to call back to when executed.  MapTrigger() is
 *  used to set a trigger to a button combination (device + button + mods).  A pointer to this is then
 *  stored in the psControlMap (hash map) for the device in question.  Additional optional data can be
 *  assigned to a trigger using SetTriggerData().  (ex: modes/movements)  When HandleEvent() receives
 *  an input event the device, button, and mods are determined.  The trigger is retrieved from the map
 *  and the function is executed, passing itself and the button's state as arguments.  The trigger
 *  function then takes this information and does whatever it does.
 */
class psControlManager
{
public:
    psControlManager(iEventNameRegistry* eventname_reg, psTriggerHandler* handler);

    bool HandleEvent(iEvent &event);

    /// Creates a new trigger
    void NewTrigger(const char* name, psControl::PressType type, psControl::TriggerFunction function);

    /// Sets a trigger to the desired button
    bool MapTrigger(const char* name, psControl::Device device, uint button, uint32 mods);

    void ResetTrigger(psControl* trigger);    ///< Resets a trigger's mapping
    void ResetAllTriggers();                  ///< Resets all mappings

    /// Gets a trigger from the master list by name
    psControl* GetTrigger(const char* name);

    /// Gets a mapped trigger for a device by button
    psControl* GetMappedTrigger(psControl::Device device, uint button, uint32 mods);

    /// Gets all mapped triggers for a device by button ignoring mods
    csArray<psControl*>* GetMappedTriggers(psControl::Device device, uint button);

    /// Get the full array of triggers
    const csPDelArray<psControl> &GetAllTriggers()
    {
        return triggers;
    }

    /// Assign 'ptr' to all psControls starting with 'name'
    bool SetTriggerData(const char* name, const void* ptr);

protected:
    typedef csHash<psControl*,uint> psControlMap;  ///< Each device gets a hash map of buttons to triggers

    psControlMap keyboard;   ///< List of all keyboard triggers
    psControlMap mouse;      ///< List of all mouse triggers

    csPDelArray<psControl> triggers;  ///< Master list of all triggers

    // Event ID cache
    csEventID event_key_down;
    csEventID event_key_up;
    csEventID event_mouse_down;
    csEventID event_mouse_up;

    /// Gets the control from a map with the specified button and mods (if not found, finds without mods)
    static psControl* GetFromMap(const psControlMap &ctrlmap, uint button, uint32 mods);

    /// Gets the controls from a map with the specified button
    csArray<psControl*>* GetArrayFromMap(const psControlMap &ctrlmap, uint button);

    /// Handles an input event
    void HandleButton(psControl::Device device, uint button, uint32 mods, bool newState);


};


/** Handles functions for each trigger.
 *
 *  Each psControl object points to one of these functions.  (thus, the arguments
 *  must be the same)  When psControlManager registers an input event that is mapped
 *  to an active trigger, its TriggerFunction is executed.  The function then takes
 *  the passed psControl* and state and does whatever is needed to start the feature.
 */
class psTriggerHandler
{
public:
    psTriggerHandler(psCharController* controler);
    ~psTriggerHandler();

    // Functions called by triggers  (psControl stores pointers to these)
    void HandleBrightnessUp(const psControl* trigger, bool value);
    void HandleBrightnessDown(const psControl* trigger, bool value);
    void HandleBrightnessReset(const psControl* trigger, bool value);
    void HandleMovement(const psControl* trigger, bool value);
    void HandleMovementJump(const psControl* trigger, bool value);
    void HandleMode(const psControl* trigger, bool value);
    void HandleAutoMove(const psControl* trigger, bool value);
    void HandleLook(const psControl* trigger, bool value);
    void HandleZoom(const psControl* trigger, bool value);
    void HandleMouseLook(const psControl* trigger, bool value);
    void HandleMouseLookToggle(const psControl* trigger, bool value);
    void HandleMouseZoom(const psControl* trigger, bool value);
    void HandleMouseMove(const psControl* trigger, bool value);
    void HandleCameraMode(const psControl* trigger, bool value);
    void HandleCenterCamera(const psControl* trigger, bool value);
    void HandleMovementAction(const psControl* trigger, bool value);
    void HandleShortcut(const psControl* trigger, bool value);
    void HandleWindow(const psControl* trigger, bool value);
    void HandleModeRun(const psControl* trigger, bool value);
    void HandleModeSneak(const psControl* trigger, bool value);

protected:
    psCharController* charcontrol;
    psMovementManager* movement;
};


/** Manages all control and movement related activities.
 *
 *  Takes in events from psEngine and sends them to psControlManager and psMovementManager as needed.
 *  This serves as public access to the control system, and creates/loads/saves all control mappings.
 */
class psCharController
{
public:
    psCharController(iEventNameRegistry* eventname_reg);
    ~psCharController();

    bool Initialize();
    bool IsReady();

    ///Takes an event from psEngine, and dispatch it to either psControlManager or psMovementManager
    bool HandleEvent(iEvent &event);

    /// Provides access to the movement system
    psMovementManager* GetMovementManager()
    {
        return &movement;
    }

    /// Get a trigger by name
    const psControl* GetTrigger(const char* name);

    /// Get the triger mapped to a button, or NULL if none
    const psControl* GetMappedTrigger(psControl::Device device, uint button, uint32 mods);

    /// Changes the button set for a trigger. Returns false if does not exist or button is taken.
    bool RemapTrigger(const char* name, psControl::Device device, uint button, uint32 mods);

    /// Returns true if the trigger exists and is mapped to the specified combo
    bool MatchTrigger(const char* name, psControl::Device device, uint button, uint32 mods);

    /// loads the character-specific custom key mappings
    void LoadKeyFile();

    /// Resets key mappings to defaults
    void LoadDefaultKeys();

    void CenterMouse(bool value);
    void CancelMouseLook();

protected:
    psTriggerHandler handler;    ///< Trigger functions
    psControlManager controls;   ///< Key mappings
    psMovementManager movement;  ///< Movement system

    void CreateKeys();                ///< Create all triggers
    bool LoadKeys(const char* file);  ///< Load all trigger mappings from file
    void SaveKeys();                  ///< Save custom trigger mappings

    bool ready;  ///< Ready to process events?

    csEventID event_mouseclick;
};

#endif

