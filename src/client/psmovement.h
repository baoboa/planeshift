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

#ifndef __PSMOVEMENT_H__
#define __PSMOVEMENT_H__

#include <csutil/parray.h>
#include <csutil/csstring.h>
#include <csgeom/vector3.h>

#include "pscelclient.h"
#include "net/cmdbase.h"

class psControlManager;
struct psPoint;

/** An entity's translational and angular velocity.
 *
 *  Contains vectors for 3-dimensional movement and rotation.
 *  Operators perform componentwise computations.
 */
struct psVelocity
{
    psVelocity() : move(0.0f), rotate(0.0f) {}
    psVelocity(csVector3 m, csVector3 r) :  move(m), rotate(r) {}

    csVector3 move;   ///< Translational velocity
    csVector3 rotate; ///< Angular velocity

    psVelocity& operator = (const psVelocity& v);
    psVelocity& operator += (const psVelocity& v);
    psVelocity& operator -= (const psVelocity& v);
    psVelocity& operator *= (const psVelocity& v);
    psVelocity& operator /= (const psVelocity& v);
};


/// A character mode and its properties
struct psCharMode
{
    uint32 id;            ///< ID of this character mode from server DB
    csString name;        ///< Name of this mode
    psVelocity modifier;  ///< Velocity multiplier for this mode
    csString idle_anim;   ///< Base animation for this mode
};

/// A character movement and its properties
struct psMovement
{
    uint32 id;            ///< ID of this movement type from server DB
    csString name;        ///< Name of this movement
    psVelocity motion;    ///< Motion aplied for this movement
};

/// Used to save movement state to detect changes.
struct psMoveState
{
    uint activeMoves;
    bool autoMove;
    bool sneaking;
};

/** Manages main character movements.
 *
 *  Starts/stops modes and movements for the controled character.  Modes and movements
 *  are stored in the server's database and requested upon login.  The primary way
 *  this class is accessed is via psTriggerHandler function calls from button presses
 *  registered by psControlManager.
 */
class psMovementManager : public psClientNetSubscriber
{
protected:
    csPDelArray<psCharMode> modes;  ///< All available character modes
    csPDelArray<psMovement> moves;  ///< All available movement types
    
    const psCharMode* defaultmode;  ///< Default actor mode

    GEMClientActor* actor;               ///< Actor we're moving here

    const psCharMode* actormode;  ///< Current active mode
    bool onGround;          ///< Is actor on ground or airborne?
    uint activeMoves;       ///< Bit mask for active moves
    psVelocity move_total;  ///< Current total of all active move velocities

    void SetupMovements(psMovementInfoMessage& msg);
    void SetupControls();
    bool ready;   ///< Have movements been setup?
    bool locked;  ///< Is this player allowed to move?

    psControlManager* controls;

    void UpdateVelocity();
    
    void SetActorMode(const psCharMode* mode);
    
    // Movement modifier handling
    void HandleMod(psMoveModMsg& msg);
    void ApplyMod(psVelocity& vel);
    psMoveModMsg::ModType activeModType;
    psVelocity activeMod;

    bool autoMove;
    bool toggleRun;
    bool mouseLook;
    bool mouseLookCanAct;
    bool mouseZoom;
    bool mouseMove;
    bool sneaking;
    int mouseAutoMove;
    int runToMarkerID;
    float lastDist;
    csVector3 runToDiff;

    // Mouse settings
    float sensY;
    float sensX;
    bool invertedMouse;
	float lastDeltaX;
	float lastDeltaY;

    csEventID event_frame;
    csEventID event_mousemove;

    const psMovement* forward;
    const psMovement* backward;
    const psCharMode* run;
    const psCharMode* walk;

    uint kbdRotate;
    
public:
    psMovementManager(iEventNameRegistry* eventname_reg, psControlManager* controls);
    ~psMovementManager();

    void HandleMessage(MsgEntry* me);
    bool HandleEvent(iEvent& event);

    void SetActor(GEMClientActor* actor);
    GEMClientActor* ControlledActor() { return actor; }

    // Functions to find movement and mode info
    const char* GetModeIdleAnim(size_t id) const { return modes[id]->idle_anim; }
    const psCharMode* FindCharMode(size_t id) const { return modes[id]; }
    const psCharMode* FindCharMode(const char* name) const;
    const psMovement* FindMovement(const char* name) const;

    bool IsReady() { return ready; }  ///< Ready to receive events?
    bool IsLocked() { return locked; }
    void LockMoves(bool v);

    // Movements
    void Start(const psCharMode* mode);  ///< Switch to a new mode
    void Stop(const psCharMode* mode);   ///< Switch back to normal mode
    void Start(const psMovement* move);  ///< Add a new movement
    void Stop(const psMovement* move);   ///< Stop an existing movement
    void Push(const psMovement* move);   ///< Use a movement once

    /// Stops all movements, resets mode, cancels mousemove, and halts the actor
    void StopAllMovement();

    /// Stops all user applied movements/modes (does not halt actor if falling or mousemove)
    void StopControlledMovement();

    // Mouse settings
    void SetMouseSensX(float v) { sensX = v; }
    void SetMouseSensY(float v) { sensY = v; }
    void SetInvertedMouse(bool v) { invertedMouse = v; }
    bool GetInvertedMouse() { return invertedMouse; }
    void LoadMouseSettings();

    void MouseLookCanAct(bool v);
    bool MouseLookCanAct(){ return mouseLookCanAct; }

    void MouseLook(bool v);
    bool MouseLook() { return mouseLook; }
    void MouseLook(iEvent& ev);
	void UpdateMouseLook();

    void MouseZoom(bool v) { mouseZoom = v; }
    bool MouseZoom() { return mouseZoom; }
    void MouseZoom(iEvent& ev);

    void SetMouseMove(bool v) { mouseMove = v; }
    bool MouseMove() { return mouseMove; }
    void SetRunToPos(psPoint& mouse);
    void CancelRunTo();
    void UpdateRunTo();
    
    void ToggleAutoMove();
    
    /** Toggle the character run mode.
     *
     * @return Return the current run state.
     */
    bool ToggleRun();

    /** Set the character run mode.
     *
     * True to enable run mode. False to disable.
     *
     * @return Return the current run state.
     */
    bool SetRun(bool runState);

    void SetSneaking(bool v) { sneaking = v; }
    bool Sneaking() { return sneaking; }

    void SaveMoveState(psMoveState& state)
    {
        state.activeMoves = activeMoves;
        state.autoMove = autoMove;
        state.sneaking = sneaking;
    }

    bool MoveStateChanged(psMoveState& state)
    {
        return state.activeMoves != activeMoves ||
               state.autoMove != autoMove ||
               state.sneaking != sneaking;
    }
};

#endif

