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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/eventnames.h>
#include <csutil/event.h>
#include <csutil/inputdef.h>
#include <csutil/xmltiny.h>
#include <iutil/vfs.h>
#include <ivideo/graph2d.h>
#include <iengine/camera.h>
#include <iengine/sector.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"

#include "util/psutil.h"

#include "paws/psmousebinds.h"
#include "paws/pawswidget.h"

#include "gui/shortcutwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/psmainwidget.h"

#include "engine/linmove.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharcontrol.h"
#include "pscamera.h"
#include "globals.h"




#define CUSTOM_CONTROLS_FILE "/planeshift/userdata/options/controls.xml"
#define DEFAULT_CONTROLS_FILE "/planeshift/data/options/controls_def.xml"


//#define CONTROLS_DEBUG


//----------------------------------------------------------------------------------------------------


psTriggerHandler* psControl::handler = NULL;

inline void psControl::Execute() const
{
    if(function)
    {
        (handler->*function)(this,state);
    }
}

csString psControl::ToString() const
{
    return ComboToString(device,button,mods);
}


//----------------------------------------------------------------------------------------------------


csString ComboToString(psControl::Device device, uint button, uint32 mods)
{
    csKeyModifiers modifiers;
    csKeyEventHelper::GetModifiers(mods,modifiers);
    iEventNameRegistry* nr = PawsManager::GetSingleton().GetEventNameRegistry();
    switch(device)
    {
        case psControl::NONE:
            return NO_BIND;
        case psControl::KEYBOARD:
            return csInputDefinition::GetKeyString(nr,button,&modifiers,false);
        case psControl::MOUSE:
            return psMouseBinds::MouseButtonToString(button,mods);
        default:
            CS_ASSERT(!"Invalid psControl::Device!");
            return "ERROR";
    }
}

csString GetDisplayName(const char* n)
{
    csString name(n);

    if(name.StartsWith("Shortcut"))
    {
        int id = atoi(name.DeleteAt(0, name.FindFirst(' ')));
        if(id > 0)
        {
            static pawsShortcutWindow* shortcuts = NULL;
            if(!shortcuts)
                shortcuts = dynamic_cast<pawsShortcutWindow*>(PawsManager::GetSingleton().FindWidget("ShortcutMenu"));

            if(shortcuts)
                name = shortcuts->GetCommandName(id-1);
        }
    }

    if(name.Slice(name.FindLast(' ')+1,5) == "(sec)")
    {
        name = name.Truncate(name.Length()-6);
    }

    return name;
}

uint32 GetPSMouseMods(const iEvent* event)
{
    if(!event)
    {
        return 0;
    }
    uint32 modifiers;
    const void* m;
    size_t mSize;
    (void) event->Retrieve("keyModifiers", m, mSize);
    modifiers = csKeyEventHelper::GetModifiersBits (*((csKeyModifiers*)m));
    return modifiers & PS_MODS_MASK;
}

uint32 GetPSKeyMods(const iEvent* event)
{
    csKeyModifiers m;
    csKeyEventHelper::GetModifiers(event,m);

    bool shift = m.modifiers[csKeyModifierTypeShift] != 0;
    bool ctrl = m.modifiers[csKeyModifierTypeCtrl] != 0;
    bool alt = m.modifiers[csKeyModifierTypeAlt] != 0;

    return (shift << csKeyModifierTypeShift)
           | (ctrl << csKeyModifierTypeCtrl)
           | (alt << csKeyModifierTypeAlt);
}

//----------------------------------------------------------------------------------------------------


psControlManager::psControlManager(iEventNameRegistry* eventname_reg, psTriggerHandler* handler)
{
    CS_ASSERT(eventname_reg);
    event_key_down = csevKeyboardDown(eventname_reg);
    event_key_up = csevKeyboardUp(eventname_reg);
    event_mouse_down = csevMouseDown(eventname_reg,0);
    event_mouse_up = csevMouseUp(eventname_reg,0);

    psControl::handler = handler;
    CS_ASSERT(handler);
}

void psControlManager::NewTrigger(const char* name, psControl::PressType type, psControl::TriggerFunction function)
{
    triggers.Push(new psControl(name,type,function));
}

bool psControlManager::MapTrigger(const char* name, psControl::Device device, uint button, uint32 mods)
{
    psControl* ctrl = GetTrigger(name);
    if(!ctrl)
        return false;  // Invalid trigger name

    psControl* other = GetMappedTrigger(device,button,mods);
    if(other && other->mods == mods)   // Ignore default GetMappedTrigger()
    {
        if(other!=ctrl)
        {
            return false; // This combo is already in use
        }
        else
        {
            return true; //already assigned to this control. Do nothing, but return success.
        }
    }


#ifdef CONTROLS_DEBUG
    printf("Mapping trigger \"%s\" to %s\n", name, ComboToString(device,button,mods).GetData());
#endif

    ResetTrigger(ctrl);  // Clear it out if already set to something else

    if(device == psControl::NONE)
        return true;  // Map removed

    ctrl->device = device;
    ctrl->button = button;
    ctrl->mods = mods;

    switch(device)
    {
        case psControl::KEYBOARD:
            keyboard.Put(button,ctrl);
            return true;
        case psControl::MOUSE:
            mouse.Put(button,ctrl);
            return true;
        default:
            CS_ASSERT(!"Invalid psControl::Device!");
            return false;
    }
}

void psControlManager::ResetTrigger(psControl* trigger)
{
    // Remove trigger from all hash maps
    keyboard.Delete(trigger->button,trigger);
    mouse.Delete(trigger->button,trigger);

    trigger->device = psControl::NONE;
    trigger->button = 0;
    trigger->mods = 0;
}

void psControlManager::ResetAllTriggers()
{
    for(size_t i=0; i<triggers.GetSize(); i++)
        if(triggers[i]->device != psControl::NONE)
            ResetTrigger(triggers[i]);

    CS_ASSERT(keyboard.IsEmpty() && mouse.IsEmpty());
}

psControl* psControlManager::GetTrigger(const char* name)
{
    for(size_t i=0; i<triggers.GetSize(); i++)
        if(triggers[i]->name == name)
            return triggers[i];
    return NULL;
}

psControl* psControlManager::GetMappedTrigger(psControl::Device device, uint button, uint32 mods)
{
    switch(device)
    {
        case psControl::NONE:
            return NULL;
        case psControl::KEYBOARD:
            return GetFromMap(keyboard,button,mods);
        case psControl::MOUSE:
            return GetFromMap(mouse,button,mods);
        default:
            CS_ASSERT(!"Invalid psControl::Device!");
            return NULL;
    }
}

psControl* psControlManager::GetFromMap(const psControlMap &ctrlmap, uint button, uint32 mods)
{
    const csArray<psControl*> maps = ctrlmap.GetAll(button);
    psControl* modless = NULL;
    for(size_t i=0; i<maps.GetSize(); i++)
    {
        if(maps[i]->mods == mods)    // Find a trigger with matching mods
            return maps[i];
        else if(maps[i]->mods == 0)  // Find modless trigger as a fallback
            modless = maps[i];
    }
    return modless;
}

csArray<psControl*>* psControlManager::GetMappedTriggers(psControl::Device device, uint button)
{
    switch(device)
    {
        case psControl::NONE:
            return NULL;
        case psControl::KEYBOARD:
            return GetArrayFromMap(keyboard,button);
        case psControl::MOUSE:
            return GetArrayFromMap(mouse,button);
        default:
            CS_ASSERT(!"Invalid psControl::Device!");
            return NULL;
    }
}

csArray<psControl*>* psControlManager::GetArrayFromMap(const psControlMap &ctrlmap, uint button)
{
    csArray<psControl*>* maps = new csArray<psControl*>(ctrlmap.GetAll(button));
    return maps;
}

void psControlManager::HandleButton(psControl::Device device, uint button, uint32 mods, bool newState)
{
    psControl* ctrlPressed = GetMappedTrigger(device,button,mods); //First trigger
    if(ctrlPressed == NULL)
        return;  // Not mapped

    csArray<psControl*>* ctrls = GetMappedTriggers(device,button); //All triggers (for stopping triggers with mods that may not exist now)
    psControl* ctrl = ctrlPressed;
    uint i = 0;
    do
    {
        if(!newState)
            ctrl = (*ctrls)[i];
        switch(ctrl->type)
        {
            case psControl::NORMAL:
            {
                // Clear other triggers of the button if they have changed
                if(ctrl->state == newState && ctrl->name != ctrlPressed->name)
                    break;

#ifdef CONTROLS_DEBUG
                printf("Handling normal map: \"%s\" for %s\n", ctrl->name.GetData(), ctrl->ToString().GetData());
#endif

                ctrl->state = newState;
                ctrl->Execute();
                break;
            }

            case psControl::TOGGLE:
            {
                if(newState != true)
                    break;  // Change on presses only

#ifdef CONTROLS_DEBUG
                printf("Handling toggle map: \"%s\" for %s\n", ctrl->name.GetData(), ctrl->ToString().GetData());
#endif

                ctrl->state = !ctrl->state;
                ctrl->Execute();
                break;

            }
        }
        i++;
    }
    while((!newState)&&(i<(*ctrls).GetSize()));
    delete ctrls;
    return;
}

bool psControlManager::SetTriggerData(const char* name, const void* ptr)
{
    bool found = false;
    for(size_t i=0; i<triggers.GetSize(); i++)
    {
        if(triggers[i]->name.StartsWith(name,true))
        {
            // Set pointer
            triggers[i]->data = static_cast<psControl::DataPtr>(ptr);
            found = true;
        }
    }
    return found;
}

bool psControlManager::HandleEvent(iEvent &event)
{
    if(csKeyEventHelper::GetAutoRepeat(&event))
        return true;  // Ignore auto-repeats

    if(event.Name == event_key_down || event.Name == event_key_up)     // Is this a key press?
    {
        utf32_char key = csKeyEventHelper::GetRawCode(&event);
        uint32 mods = GetPSKeyMods(&event);
        bool state = (event.Name == event_key_down);
        //printf("KeyPress: %c %u %s\n", key, mods, state?"down":"up" );
        HandleButton(psControl::KEYBOARD,key,mods,state);
        return true;
    }
    else if(event.Name == event_mouse_down || event.Name == event_mouse_up)     // Mouse button press?
    {
        uint button = csMouseEventHelper::GetButton(&event);
        uint32 mods = GetPSMouseMods(&event);
        bool state = (event.Name == event_mouse_down);
        //printf("MouseClick: btn%u %u %s\n", button, mods, state?"down":"up" );
        HandleButton(psControl::MOUSE,button,mods,state);
        return true;
    }

    return false;
}


//----------------------------------------------------------------------------------------------------


psTriggerHandler::psTriggerHandler(psCharController* controler)
{
    charcontrol = controler;
    movement = charcontrol->GetMovementManager();
}

psTriggerHandler::~psTriggerHandler()
{
    charcontrol = NULL;
    movement = NULL;
}

void psTriggerHandler::HandleBrightnessUp(const psControl* /*trigger*/, bool /*value*/)
{
    psengine->AdjustBrightnessCorrectionUp();
}

void psTriggerHandler::HandleBrightnessDown(const psControl* /*trigger*/, bool /*value*/)
{
    psengine->AdjustBrightnessCorrectionDown();
}

void psTriggerHandler::HandleBrightnessReset(const psControl* /*trigger*/, bool /*value*/)
{
    psengine->ResetBrightnessCorrection();
}

void psTriggerHandler::HandleMovement(const psControl* trigger, bool value)
{
    const psMovement* move = static_cast<const psMovement*>(trigger->data);
    if(move)
    {
        movement->CancelRunTo();
        if(value)
        {
            movement->Start(move);
        }
        else
        {
            movement->Stop(move);
        }
    }
    else
    {
        Error2("Movement trigger '%s' is not ready", trigger->name.GetData());
    }
}

void psTriggerHandler::HandleMovementJump(const psControl* trigger, bool value)
{
    const psMovement* move = static_cast<const psMovement*>(trigger->data);
    if(move)
    {
        if(value)
            movement->Push(move);
    }
    else
    {
        Error2("Movement trigger '%s' is not ready", trigger->name.GetData());
    }
}

void psTriggerHandler::HandleMode(const psControl* trigger, bool value)
{
    const psCharMode* mode = static_cast<const psCharMode*>(trigger->data);
    if(mode)
    {
        if(value)
            movement->Start(mode);
        else
            movement->Stop(mode);
    }
    else
    {
        Error2("Mode trigger '%s' is not ready", trigger->name.GetData());
    }
}

void psTriggerHandler::HandleModeRun(const psControl* /*trigger*/, bool /*value*/)
{
    if(movement && !movement->Sneaking())
    {
        movement->ToggleRun();
    }
}

void psTriggerHandler::HandleModeSneak(const psControl* trigger, bool value)
{
    HandleMode(trigger, value);
    if(movement)
    {
        movement->SetSneaking(value);
    }
}

void psTriggerHandler::HandleAutoMove(const psControl* /*trigger*/, bool /*value*/)
{
    if(movement)
    {
        movement->ToggleAutoMove();
    }
}

void psTriggerHandler::HandleLook(const psControl* trigger, bool value)
{
    if(value)
    {
        if(trigger->name == "Look up")
            psengine->GetPSCamera()->SetPitchVelocity(1.0f);
        else
            psengine->GetPSCamera()->SetPitchVelocity(-1.0f);
    }
    else
    {
        psengine->GetPSCamera()->SetPitchVelocity(0.0f);
    }
}

void psTriggerHandler::HandleZoom(const psControl* trigger, bool /*value*/)
{
    // KL: Removed check for value because we now treat this event as TOGGLE.
    if(trigger->name == "Zoom in")
    {
        if(psengine->GetPSCamera()->GetMinDistance() == psengine->GetPSCamera()->GetDistance() &&
                psengine->GetPSCamera()->GetCameraMode() != psCamera::CAMERA_FIRST_PERSON)
        {
            psengine->GetPSCamera()->SetCameraMode(psCamera::CAMERA_FIRST_PERSON);
        }
        psengine->GetPSCamera()->MoveDistance(-1.0f);
    }
    else
    {
        if(psengine->GetPSCamera()->GetMinDistance() == psengine->GetPSCamera()->GetDistance() &&
                psengine->GetPSCamera()->GetCameraMode() == psCamera::CAMERA_FIRST_PERSON)
        {
            psengine->GetPSCamera()->SetCameraMode(psengine->GetPSCamera()->GetLastCameraMode());
        }
        psengine->GetPSCamera()->MoveDistance(1.0f);
    }

    //should be SetPitch... but there is no method to do that currently so the movement is blocky for now
}

void psTriggerHandler::HandleMouseLook(const psControl* /*trigger*/, bool value)
{
    // RS: invert mouselook status so the mouselook button can be
    // used to temporarily switch out of mouselook mode too
    movement->MouseLook(!movement->MouseLook());
    if(!movement->MouseZoom() && movement->MouseLook())    // KL: No CenterMouse while in MouseZoom mode.
    {
        charcontrol->CenterMouse(value);
    }

    if(movement->MouseLook())
    {
        //we set modality to the main widget so it's not possible for other widgets to grab focus
        //which could make weird situations
        PawsManager::GetSingleton().SetModalWidget(PawsManager::GetSingleton().GetMainWidget());
        PawsManager::GetSingleton().GetMouse()->Hide(true);

        if(psengine->GetPSCamera()->GetCameraMode()
                == psCamera::CAMERA_FIRST_PERSON)
        {
            PawsManager::GetSingleton().GetMouse()->WantCrosshair();
        }
        else
        {
            PawsManager::GetSingleton().GetMouse()->WantCrosshair(false);
        }
    }
    else
    {
        //if the main widget owns modality status we remove it from it.
        //we check for this because this can happen also if the mouselook() returns false because it wasn't
        //allowed from the main widget to activate: in that case it means, most probably, another widget has modality
        //and it must retain it.
        if(PawsManager::GetSingleton().GetModalWidget() == PawsManager::GetSingleton().GetMainWidget())
            PawsManager::GetSingleton().SetModalWidget(NULL);
        PawsManager::GetSingleton().GetMouse()->Hide(false);
    }
}

// KL: New function HandleMouseLookToggle to handle keyboard toggle for MouseLook seperately.
void psTriggerHandler::HandleMouseLookToggle(const psControl* trigger, bool value)
{
    if(!value)
    {
        return;
    }

    HandleMouseLook(trigger,!movement->MouseLook());
}

void psTriggerHandler::HandleMouseZoom(const psControl* /*trigger*/, bool value)
{
    // KL: Changed order to match HandleMouseLookToggle (no change in functionality, just to keep code more readable).
    if(movement->MouseZoom()!=value)  // KL: Switch only if necessary to avoid jumping mouse cursor or overwriting last saved position.
    {
        movement->MouseZoom(value);
        if(!movement->MouseLook())  // KL: No CenterMouse while in MouseLook mode.
            charcontrol->CenterMouse(value);
    }
}


void psTriggerHandler::HandleMouseMove(const psControl* /*trigger*/, bool value)
{
    if(value && psengine->GetCelClient()
            && psengine->GetCelClient()->GetMainPlayer()
            && psengine->GetCelClient()->GetMainPlayer()->IsAlive())
    {
        if(movement->MouseMove())
        {
            psPoint pos = PawsManager::GetSingleton().GetMouse()->GetPosition();
            movement->SetRunToPos(pos);
        }
        movement->SetMouseMove(value);
    }
}

void psTriggerHandler::HandleCameraMode(const psControl* /*trigger*/, bool value)
{
    if(value)
        psengine->GetPSCamera()->NextCameraMode();
}

void psTriggerHandler::HandleCenterCamera(const psControl* /*trigger*/, bool value)
{
    if(value)
    {
        float       rotY;
        csVector3   pos;
        iSector*    sector;

        psCamera* cam = psengine->GetPSCamera();
        movement->ControlledActor()->Movement().GetLastPosition(pos, rotY, sector);

        cam->SetPosition(pos + csVector3(sinf(rotY)*cam->GetMaxDistance(),0,cosf(rotY)*cam->GetMaxDistance()));
        cam->SetYaw(rotY);

        if(cam->GetCameraMode() == psCamera::CAMERA_FREE)
            cam->SetPitch(0);
    }
}

void psTriggerHandler::HandleMovementAction(const psControl* trigger, bool /*value*/)
{
    if(trigger->name == "Sit" && psengine->GetCelClient()->GetMainPlayer())
    {
        if(psengine->GetCelClient()->GetMainPlayer()->GetMode() == psModeMessage::SIT ||
                psengine->GetCelClient()->GetMainPlayer()->GetMode() == psModeMessage::OVERWEIGHT)
        {
            psengine->GetCmdHandler()->Execute("/stand");
        }
        else
        {
            psengine->GetCmdHandler()->Execute("/sit");
        }
    }
}

void psTriggerHandler::HandleShortcut(const psControl* trigger,bool value)
{
    static pawsShortcutWindow* shortcutwin = NULL;
    if(!shortcutwin)
        shortcutwin = dynamic_cast<pawsShortcutWindow*>(PawsManager::GetSingleton().FindWidget("ShortcutMenu"));

    if(shortcutwin && value)
    {
        size_t start = trigger->name.FindLast(' ') + 1;
        csString cutter = trigger->name.Slice(start,trigger->name.Length()-start);
        int sc = atoi(cutter.GetDataSafe());
        if(sc == 0)
        {
            Error1("Invalid extraction in shortcut handle!");
            return;
        }

        //shortcutwin->ExecuteCommand(sc-1,false);
        shortcutwin->ExecuteCommand(sc-1);
    }
}

void psTriggerHandler::HandleWindow(const psControl* trigger, bool value)
{
    static pawsControlWindow* ctrlWindow = NULL;
    if(!ctrlWindow)
        ctrlWindow = dynamic_cast<pawsControlWindow*>(PawsManager::GetSingleton().FindWidget("ControlWindow"));

    if(ctrlWindow && !value)
    {
        ctrlWindow->HandleWindowName(trigger->name);
    }
}


//----------------------------------------------------------------------------------------------------


psCharController::psCharController(iEventNameRegistry* eventname_reg)
    : handler(this),
      controls(eventname_reg, &handler),
      movement(eventname_reg, &controls)
{
    CS_ASSERT(eventname_reg);
    event_mouseclick = csevMouseClick(eventname_reg,0);

    ready = false;
}

psCharController::~psCharController()
{
}

bool psCharController::Initialize()
{
    // Create the controls
    CreateKeys();

    // Load what the keys are set to - initially this is the default key set;
    // character specific key mappings deferred until the character name is defined. (called from shortcutwindow.cpp)

    movement.LoadMouseSettings();

    // Get movement types from server
    psMsgRequestMovement msg;
    psengine->GetMsgHandler()->SendMessage(msg.msg);

    ready = true;

    return true;
}

void psCharController::LoadKeyFile()
{
// if there's a new style character-specific file then load it
    csString CommandFileName,
             CharName(psengine->GetMainPlayerName());
    size_t   spPos = CharName.FindFirst(' ');

    if(spPos != (size_t) -1)
    {
        //there is a space in the name
        CommandFileName = CharName.Slice(0,spPos);
    }
    else
    {
        CommandFileName = CharName;
    }

    CommandFileName.Insert(0, "/planeshift/userdata/options/controls_");
    CommandFileName.Append(".xml");
    if(psengine->GetVFS()->Exists(CommandFileName.GetData()))
    {
        LoadKeys(CommandFileName.GetData());
    }
    else
    {
        if(psengine->GetVFS()->Exists(CUSTOM_CONTROLS_FILE))
        {
            LoadKeys(CUSTOM_CONTROLS_FILE);
        }
        else
        {
            LoadKeys(DEFAULT_CONTROLS_FILE);
        }
        SaveKeys();
    }
}

void psCharController::CreateKeys()
{
    // Movement
    controls.NewTrigger("Forward"      , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Backward"     , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Rotate left"  , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Rotate right" , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Strafe left"  , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Strafe right" , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Brightness up" , psControl::TOGGLE, &psTriggerHandler::HandleBrightnessUp);
    controls.NewTrigger("Brightness down" , psControl::TOGGLE, &psTriggerHandler::HandleBrightnessDown);
    controls.NewTrigger("Brightness Reset" , psControl::TOGGLE, &psTriggerHandler::HandleBrightnessReset);
    controls.NewTrigger("Forward (sec)"      , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Backward (sec)"     , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Rotate left (sec)"  , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Rotate right (sec)" , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Strafe left (sec)"  , psControl::NORMAL, &psTriggerHandler::HandleMovement);
    controls.NewTrigger("Strafe right (sec)" , psControl::NORMAL, &psTriggerHandler::HandleMovement);

    controls.NewTrigger("Run"     , psControl::NORMAL, &psTriggerHandler::HandleModeRun);
    controls.NewTrigger("AutoMove" , psControl::TOGGLE, &psTriggerHandler::HandleAutoMove);
    controls.NewTrigger("MouseMove", psControl::NORMAL, &psTriggerHandler::HandleMouseMove);
    controls.NewTrigger("Sneak"   , psControl::NORMAL, &psTriggerHandler::HandleModeSneak);
    controls.NewTrigger("ToggleRun" , psControl::TOGGLE, &psTriggerHandler::HandleModeRun);

    controls.NewTrigger("Jump", psControl::NORMAL, &psTriggerHandler::HandleMovementJump);
    controls.NewTrigger("Sit" , psControl::TOGGLE, &psTriggerHandler::HandleMovementAction);

    // Camera
    controls.NewTrigger("Look up"            , psControl::NORMAL, &psTriggerHandler::HandleLook);
    controls.NewTrigger("Look down"          , psControl::NORMAL, &psTriggerHandler::HandleLook);
    controls.NewTrigger("Zoom in"            , psControl::TOGGLE, &psTriggerHandler::HandleZoom);
    controls.NewTrigger("Zoom out"           , psControl::TOGGLE, &psTriggerHandler::HandleZoom);
    controls.NewTrigger("Camera mode"        , psControl::NORMAL, &psTriggerHandler::HandleCameraMode);
    controls.NewTrigger("Center camera"      , psControl::NORMAL, &psTriggerHandler::HandleCenterCamera);
    controls.NewTrigger("CameraForward/Back" , psControl::NORMAL, &psTriggerHandler::HandleMouseZoom);
    controls.NewTrigger("MouseLook"          , psControl::NORMAL, &psTriggerHandler::HandleMouseLook);
    controls.NewTrigger("Toggle MouseLook"   , psControl::NORMAL, &psTriggerHandler::HandleMouseLookToggle);

    // Windows
    controls.NewTrigger("Buddy"          , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Buy"            , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Communications" , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Help"           , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Inventory"      , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Bag"            , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Options"        , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Quit"           , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Spell book"     , psControl::NORMAL, &psTriggerHandler::HandleWindow);
    controls.NewTrigger("Stats"          , psControl::NORMAL, &psTriggerHandler::HandleWindow);

    // Used raw; checked via MatchTrigger() when conditions are correct
    controls.NewTrigger("Toggle chat", psControl::NORMAL, NULL);
    controls.NewTrigger("Reply tell" , psControl::NORMAL, NULL);
    controls.NewTrigger("Close"      , psControl::NORMAL, NULL);

    // Shortcuts
    for(int i=0; i<NUM_SHORTCUTS; i++)
    {
        csString shortcut;
        shortcut.Format("Shortcut %d", i+1);
        controls.NewTrigger(shortcut, psControl::NORMAL, &psTriggerHandler::HandleShortcut);
    }
}

/** Bindings are stored in controls.xml with format is as follows:
 *  <controls>
 *      <bind action="" key="" />
 *      <bind action="" key="" />
 *      ...
 *  </controls>
 *  where 'action' is listed in CreateKeys() and 'key' is specified as 'modifiers+button'
 */
bool psCharController::LoadKeys(const char* filename)
{
    if(!psengine->GetVFS()->Exists(filename))
        return false;

    csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(), filename);
    if(doc == NULL)
    {
        Error2("Could not parse keys file %s.", filename);
        return false;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("No root in XML");
        return false;
    }

    csRef<iDocumentNode> node = root->GetNode("controls");
    if(node == NULL)
    {
        //check for os specific nodes if a global controls one wasn't found
        csRef<iDocumentNodeIterator> osNodes = root->GetNodes("os");
        while(osNodes->HasNext())
        {
            //gets the next element of the iterator
            csRef<iDocumentNode> osNode = osNodes->Next();
            //assigns the attribute name to a variable for easy access later
            csString osName = osNode->GetAttributeValue("name");
            if(osName == CS_PLATFORM_NAME) //compares the name with the platform we are in
            {
                //We found the right platform so we get the nested controls in there
                //and bail out
                node = osNode->GetNode("controls");
                break;
            }
            //we found a generic platform control scheme we assign this for now if nothing better comes
            else if(osName == "DEFAULT")
                node = osNode->GetNode("controls");
        }

        if(node == NULL) //nothing to do we still didn't find it so we bail out as erroring
        {
            Error1("No <controls> or <os> tag in XML");
            return false;
        }
    }

    iEventNameRegistry* nr = psengine->GetEventNameRegistry();
    csRef<iDocumentNodeIterator> binds = node->GetNodes("bind");
    while(binds->HasNext())
    {
        csRef<iDocumentNode> keyNode = binds->Next();

        csString key = keyNode->GetAttributeValue("key");
        if(key.IsEmpty())
            continue;

        csString action = keyNode->GetAttributeValue("action");
        if(action.IsEmpty())
        {
            Error2("Failed to get action for key '%s'", key.GetData());
            continue;
        }

        // Try to parse keyboard combo
        csKeyEventData data;
        bool parsed = csInputDefinition::ParseKey(nr, key, &data.codeRaw, &data.codeCooked, &data.modifiers);
        uint button = data.codeRaw;
        uint32 mods = csKeyEventHelper::GetModifiersBits(data.modifiers) & PS_MODS_MASK;
        psControl::Device device = psControl::KEYBOARD;

        // If that didn't work, try mouse
        if(!parsed)
        {
            parsed = psMouseBinds::StringToMouseButton(key,button,mods);
            device = psControl::MOUSE;
        }

        if(!parsed)
        {
            Error3("Failed to parse key combo '%s' for '%s'", key.GetData(), action.GetData());
            continue;
        }

        if(!controls.MapTrigger(action,device,button,mods))
        {
            Error3("Failed to map '%s' to '%s'", key.GetData(), action.GetData());
            continue;
        }
    }

    return true;
}

void psCharController::LoadDefaultKeys()
{
    controls.ResetAllTriggers();  // Reset first
    LoadKeys(DEFAULT_CONTROLS_FILE);
}

void psCharController::SaveKeys()
{
    csString xml = "<controls>\n";

    const csPDelArray<psControl> &triggers = controls.GetAllTriggers();
    for(size_t i=0; i<triggers.GetSize(); i++)
        xml.AppendFmt("    <bind action=\"%s\" key=\"%s\" />\n",
                      triggers[i]->name.GetData(),
                      EscpXML(triggers[i]->ToString()).GetData());

    xml += "</controls>\n";

    csString CommandFileName,
             CharName(psengine->GetMainPlayerName());
    size_t   spPos = CharName.FindFirst(' ');

    if(spPos != (size_t) -1)
    {
        //there is a space in the name
        CommandFileName = CharName.Slice(0,spPos);
    }
    else
    {
        CommandFileName = CharName;
    }

    CommandFileName.Insert(0, "/planeshift/userdata/options/controls_");
    CommandFileName.Append(".xml");
    psengine->GetVFS()->WriteFile(CommandFileName.GetData(), xml.GetData(), xml.Length());
}

const psControl* psCharController::GetTrigger(const char* name)
{
    return controls.GetTrigger(name);
}

const psControl* psCharController::GetMappedTrigger(psControl::Device device, uint button, uint32 mods)
{
    return controls.GetMappedTrigger(device,button,mods);
}

bool psCharController::RemapTrigger(const char* name, psControl::Device device, uint button, uint32 mods)
{
    bool changed = controls.MapTrigger(name,device,button,mods);
    if(changed)
        SaveKeys();
    return changed;
}

bool psCharController::IsReady()
{
    return ready && movement.IsReady();
}

bool psCharController::HandleEvent(iEvent &event)
{
    if(!IsReady())
        return false;

    // RS: do not leave mouselook mode on mouseclick
    //if (event.Name == event_mouseclick && movement.MouseLook())
    //    CancelMouseLook();

    if(controls.HandleEvent(event))
        return true;

    if(movement.HandleEvent(event))
        return true;

    return false;
}

void psCharController::CenterMouse(bool value)
{
    static psPoint oldMousePos;
    PawsManager::GetSingleton().GetMouse()->Hide(value);
    if(value)
    {
        iGraphics2D* g2d = psengine->GetG2D();
        int width = g2d->GetWidth();
        int height = g2d->GetHeight();
        g2d->SetMousePosition(width/2, height/2);
        oldMousePos = PawsManager::GetSingleton().GetMouse()->GetPosition();
    }
    else
    {
        psengine->GetG2D()->SetMousePosition(oldMousePos.x, oldMousePos.y);
    }
}

void psCharController::CancelMouseLook()
{
    // KL: Fixed and simplified CancelMouseLook.
    if(GetMovementManager()->MouseLook())
    {
        GetMovementManager()->MouseLook(false);
        CenterMouse(false);
    }
}

bool psCharController::MatchTrigger(const char* name, psControl::Device device, uint button, uint32 mods)
{
    const psControl* trigger = controls.GetMappedTrigger(device,button,mods);
    return trigger && trigger->name == name;
}


