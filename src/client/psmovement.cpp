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
#include "globals.h"

#include <csutil/event.h>
#include <csutil/eventnames.h>
#include <iengine/mesh.h>
#include <imesh/object.h>
#include <ivideo/graph2d.h>

#include "engine/linmove.h"

#include "net/clientmsghandler.h"

#include "psmovement.h"
#include "pscharcontrol.h"
#include "pscamera.h"
#include "effects/pseffectmanager.h"


//#define MOVE_DEBUG

#define RUNTO_EPSILON   0.75f
#define USE_EXPERIMENTAL_AUTOMOVE 0

//----------------------------------------------------------------------------------------------------


// Some additional csVector3 operators to simplify things
inline csVector3& operator *= (csVector3& u, const csVector3& v)
{ u.x *= v.x; u.y *= v.y; u.z *= v.z; return u; }
inline csVector3& operator /= (csVector3& u, const csVector3& v)
{ u.x /= v.x; u.y /= v.y; u.z /= v.z; return u; }

// psVelocity operator definitions
inline psVelocity& psVelocity::operator = (const psVelocity& v)
{ move = v.move; rotate = v.rotate; return *this; }
inline psVelocity& psVelocity::operator += (const psVelocity& v)
{ move += v.move; rotate += v.rotate; return *this; }
inline psVelocity& psVelocity::operator -= (const psVelocity& v)
{ move -= v.move; rotate -= v.rotate; return *this; }
inline psVelocity& psVelocity::operator *= (const psVelocity& v)
{ move *= v.move; rotate *= v.rotate; return *this; }
inline psVelocity& psVelocity::operator /= (const psVelocity& v)
{ move /= v.move; rotate /= v.rotate; return *this; }


//----------------------------------------------------------------------------------------------------


psMovementManager::psMovementManager(iEventNameRegistry* eventname_reg, psControlManager* controls)
{
    CS_ASSERT(eventname_reg);
    event_frame = csevFrame(eventname_reg);
    event_mousemove = csevMouseMove(eventname_reg,0);

    actor = NULL;
    this->controls = controls;

    ready = false;
    psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MOVEINFO);
    psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MOVELOCK);
    psengine->GetMsgHandler()->Subscribe(this,MSGTYPE_MOVEMOD);

    defaultmode = NULL;
    actormode = NULL;

    onGround = true;
    locked = false;
    activeMoves = 0;

    autoMove = false;
    toggleRun = false;
    mouseAutoMove = false;
    mouseLook = false;
    mouseLookCanAct = false;
    mouseZoom = false;
    mouseMove = false;
    sneaking = false;
    runToMarkerID = 0;
    lastDist = 0.0f;
    runToDiff = csVector3(0.0f);
    lastDeltaX = 0.0f;
    lastDeltaY = 0.0f;

    sensY = 1.0f;
    sensX = 1.0f;
    invertedMouse = true;
    
    activeModType = psMoveModMsg::NONE;

    forward = NULL;
    backward = NULL;
    run = NULL;
    walk = NULL;

    kbdRotate = 0;
}

psMovementManager::~psMovementManager()
{
    ready = false;
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_MOVEINFO);
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_MOVELOCK);
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_MOVEMOD);
}

void psMovementManager::SetActor(GEMClientActor* a)
{
    actor = a ? a : psengine->GetCelClient()->GetMainPlayer() ;
    CS_ASSERT(actor);
    SetupControls();
}

const psCharMode* psMovementManager::FindCharMode(const char* name) const
{
    for (size_t i=0; i<modes.GetSize(); i++)
        if (modes[i]->name == name)
            return modes[i];
    return NULL;
}

const psMovement* psMovementManager::FindMovement(const char* name) const
{
    for (size_t i=0; i<moves.GetSize(); i++)
        if (moves[i]->name == name)
            return moves[i];
    return NULL;
}

void psMovementManager::LockMoves(bool v)
{
    locked = v;

    if (!actor)
        return;

    //we had a special event we need to unset stationary in case so the client
    //queries the server on the status at least one time.
    actor->stationary = false;
    actor->lastDRUpdateTime = csGetTicks();

    if (v)
    {
        StopAllMovement();
    }
    else
    {
        actor->Movement().ResetGravity();
    }
    
}

void psMovementManager::HandleMessage(MsgEntry* me)
{
    switch ( me->GetType() )
    {
        case MSGTYPE_MOVEINFO:  // Initial info about modes and moves
        {
            if (ready)
            {
                Bug1("Received second set of movement info!");
                return;
            }
            psMovementInfoMessage movemsg(me);
            SetupMovements(movemsg);
            return;
        }

        case MSGTYPE_MOVELOCK:  // Movement lockout started
        {
            // The server will override any attempts to move during a lockout.
            // The client should lockout controls too, until the server says otherwise.
            psMoveLockMessage lockmsg(me);
            LockMoves(lockmsg.locked);
            return;
        }

        case MSGTYPE_MOVEMOD:  // Movement modifier
        {
            psMoveModMsg modmsg(me);
            HandleMod(modmsg);
            return;
        }
    }
}

bool psMovementManager::HandleEvent( iEvent &event )
{
    if (!ready || !actor)  // Not fully loaded yet
        return false;

    if (event.Name == event_frame)
    {
        // If we've returned to the ground, update allowed velocity
        if ((!onGround && actor->Movement().IsOnGround()) ||
                (onGround && !actor->Movement().IsOnGround()))
            UpdateVelocity();

            // UpdateMouseLook will take care of "recent mouse-look turning" based on "bool mouseLook"
            UpdateMouseLook();

        if (mouseMove)
            UpdateRunTo();
    }
    else if (event.Name == event_mousemove)
    {
        if (mouseLook)
            MouseLook(event);
        else if (mouseZoom)
            MouseZoom(event);
    }

    return false;
}

void psMovementManager::HandleMod(psMoveModMsg& msg)
{
    csVector3 rotMod = csVector3(0,msg.rotationMod,0);
    if (msg.type == psMoveModMsg::PUSH)
    {
        psVelocity push(msg.movementMod,rotMod);
        
        // Execute once
        move_total += push;
        UpdateVelocity();
        move_total -= push;
    }
    else // Persistant mod
    {
        activeModType = msg.type;
        activeMod.move = msg.movementMod;
        activeMod.rotate = rotMod;

        UpdateVelocity();
    }
}

inline void psMovementManager::ApplyMod(psVelocity& vel)
{
    switch (activeModType)
    {
        default:
        case psMoveModMsg::NONE:
        case psMoveModMsg::PUSH:
            return;

        // Add to any existing movement
        case psMoveModMsg::ADDITION:
        {
            size_t i;
            for (i=0; i<3; i++)
            {
                if (vel.move[i] > SMALL_EPSILON)
                    vel.move[i] += activeMod.move[i];
                else if (vel.move[i] < -SMALL_EPSILON)
                    vel.move[i] -= activeMod.move[i];
            }
            for (i=0; i<3; i++)
            {
                if (vel.rotate[i] > SMALL_EPSILON)
                    vel.rotate[i] += activeMod.rotate[i];
                else if (vel.rotate[i] < -SMALL_EPSILON)
                    vel.rotate[i] -= activeMod.rotate[i];
            }
            return;
        }

        // Multiply
        case psMoveModMsg::MULTIPLIER:
        {
            vel *= activeMod;
            return;
        }

        // Add
        case psMoveModMsg::CONSTANT:
        {
            vel += activeMod;
            return;
        }
    }
}

void psMovementManager::SetupMovements(psMovementInfoMessage& movemsg)
{
    #ifdef MOVE_DEBUG
        printf("\nReceived character modes:\n");
    #endif
    for (size_t i=0; i<movemsg.modes; i++)
    {
        uint32 id;
        const char* name;
        csVector3 move_mod;
        csVector3 rotate_mod;
        const char* idle_anim;
        movemsg.GetMode(id, name, move_mod, rotate_mod, idle_anim);

        #ifdef MOVE_DEBUG
            printf("Got mode: %u, %s, (%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f), %s\n",
                    id,name,move_mod.x,move_mod.y,move_mod.z,
                    rotate_mod.x,rotate_mod.y,rotate_mod.z,idle_anim);
        #endif

        psCharMode* newmode = new psCharMode;
        newmode->id = id;
        newmode->name = name;
        newmode->modifier.move = move_mod;
        newmode->modifier.rotate = rotate_mod;
        newmode->idle_anim = idle_anim;

        modes.Put(id,newmode);
    }
    if (movemsg.modes == 0)
    {
        Bug1("Received no character modes!");
    }
    
    #ifdef MOVE_DEBUG
        printf("\nReceived movement types:\n");
    #endif
    for (size_t i=0; i<movemsg.moves; i++)
    {
        uint32 id;
        const char* name;
        csVector3 base_move;
        csVector3 base_rotate;
        movemsg.GetMove(id, name, base_move, base_rotate);

        #ifdef MOVE_DEBUG
            printf("Got move: %u, %s, (%.2f,%.2f,%.2f), (%.2f,%.2f,%.2f)\n",
                    id,name,base_move.x,base_move.y,base_move.z,
                    base_rotate.x,base_rotate.y,base_rotate.z);
        #endif

        psMovement* newmove = new psMovement;
        newmove->id = id;
        newmove->name = name;
        newmove->motion.move = base_move;
        newmove->motion.rotate = base_rotate;
        
        
        CS_ASSERT(id < sizeof(activeMoves)*8);
        moves.Put(id,newmove);
    }
    if (movemsg.moves == 0)
    {
        Bug1("Received no movement types!");
    }

    forward = FindMovement("forward");
    backward = FindMovement("backward");
    run = FindCharMode("run");
    walk = FindCharMode("normal");

    const psMovement* kbdrotL = FindMovement("rotate left");
    const psMovement* kbdrotR = FindMovement("rotate right");
    if (kbdrotL && kbdrotR)
        kbdRotate = (1 << kbdrotL->id) | (1 << kbdrotR->id);

    //TODO: make this configurable?
    defaultmode = walk;
    actormode = run;
    toggleRun = true;

    ready = true;
}

void psMovementManager::SetupControls()
{
    for (size_t i=0; i<modes.GetSize(); i++)
    {
        if ( controls->SetTriggerData(modes[i]->name,modes[i]) )
        {
            #ifdef MOVE_DEBUG
                printf("Set control for mode %s\n", modes[i]->name.GetData() );
            #endif
        }
    }
    for (size_t i=0; i<moves.GetSize(); i++)
    {
        if ( controls->SetTriggerData(moves[i]->name,moves[i]) )
        {
            #ifdef MOVE_DEBUG
                printf("Set control for moves %s\n", moves[i]->name.GetData() );
            #endif
        }
    }
}

void psMovementManager::SetActorMode(const psCharMode* mode)
{
    if(actor)
    {
        actormode = mode;
        if (actor->GetMode() == psModeMessage::PEACE)
        {
            actor->SetIdleAnimation(actormode->idle_anim); 
        }
    }
}

void psMovementManager::Start(const psCharMode* mode)
{
    #ifdef MOVE_DEBUG
        printf("Starting mode %s\n", mode->name.GetData() );
    #endif

    SetActorMode(mode);
    UpdateVelocity();
}

void psMovementManager::Stop(const psCharMode* mode)
{
    #ifdef MOVE_DEBUG
        printf("Stopping mode %s\n", mode->name.GetData() );
    #else
        (void)mode; // surpress unused variable warning
    #endif

    SetActorMode(defaultmode);
    UpdateVelocity();
}

void psMovementManager::Start(const psMovement* move)
{
    #ifdef MOVE_DEBUG
        printf("Starting move %s\n", move->name.GetData() );
    #endif

    // Cancel automove if starting to move forward on own
    if ((move == forward || move == backward ) && autoMove)
        ToggleAutoMove();

    uint bit = (1 << move->id);
    if (activeMoves & bit)
        return;  // Already active

    activeMoves |= bit;
    move_total += move->motion;
    UpdateVelocity();
}

void psMovementManager::Stop(const psMovement* move)
{
    #ifdef MOVE_DEBUG
        printf("Stopping move %s\n", move->name.GetData() );
    #endif

    // Don't cancel automove
    if (move == forward && autoMove)
        return;

    uint bit = (1 << move->id);
    if (!(activeMoves & bit))
        return;  // Not active

    activeMoves &= ~bit;
    move_total -= move->motion;
    UpdateVelocity();
}

void psMovementManager::Push(const psMovement* move)
{
    #ifdef MOVE_DEBUG
        printf("Pushing move %s\n", move->name.GetData() );
    #endif

    uint bit = (1 << move->id);
    if (activeMoves & bit)
        return;  // Already active

    move_total += move->motion;
    UpdateVelocity();
    move_total -= move->motion;
}

void psMovementManager::StopAllMovement()
{
    if (!ready || !actor)  // Not fully loaded yet
        return;

    // Cancel all active
    activeMoves = 0;
    move_total = psVelocity(0.0f,0.0f);
    if(!toggleRun)
        SetActorMode(defaultmode);
    else
        SetActorMode(run);

    // Halt actor
    actor->Movement().SetVelocity(0);
    actor->Movement().SetAngularVelocity(0);
    actor->SetAnimationVelocity(0);

    // Remove run-to effect
    if (runToMarkerID != 0)
    {
        psengine->GetEffectManager()->DeleteEffect(runToMarkerID);
        runToMarkerID = 0;
    }
    autoMove = false;
}

void psMovementManager::StopControlledMovement()
{
    // Don't stop if not fully loaded yet, run-to or automove is active, or character is already stopped
    if (!ready || !actor || runToMarkerID != 0 || autoMove || activeMoves == 0)
        return;

    // Cancel all active
    activeMoves = 0;
    move_total = psVelocity(0.0f,0.0f);

    if (autoMove)  // Don't stop automove
    {
        Start(forward);
        autoMove = true;
    }
    else
    {
        SetActorMode(defaultmode);
    }

    UpdateVelocity();
}

void psMovementManager::UpdateVelocity()
{
    /// While the client is locked out the server will also reject any attempts to move.
    if (locked || !actor)
        return;

    // Start with the total of all applied movements
    psVelocity vel = move_total;
    
    // Apply special modifier, if we have one
    ApplyMod(vel);

    /** When falling or otherwise not on the ground we restrict movement.
     *  Some control is required to prevent collision detection issues,
     *  and to give the limited ability to turn or glide mid-flight.
     */
    onGround = actor->Movement().IsOnGround();

    // Apply mode's modifier
    vel *= actormode->modifier;
    if (onGround)  // Normal
    {

        #ifdef MOVE_DEBUG
            printf("On Ground: Applying velocity modifier (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f)\n",
                    actormode->modifier.move.x,actormode->modifier.move.y,actormode->modifier.move.z,
                    actormode->modifier.rotate.x,actormode->modifier.rotate.y,actormode->modifier.rotate.z);
            printf("On Ground: Changing velocity to (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f)\n",
                    vel.move.x,vel.move.y,vel.move.z,
                    vel.rotate.x,vel.rotate.y,vel.rotate.z);
        #endif

        // Set vertical
        actor->Movement().ClearWorldVelocity();
        actor->Movement().AddVelocity( csVector3(0,vel.move.y,0) );

        // Set horizontal
        actor->Movement().SetVelocity( csVector3(vel.move.x,0,vel.move.z) );
    
        // Set rotation
        actor->Movement().SetAngularVelocity( vel.rotate );

        // Update animating speed
        actor->SetAnimationVelocity( vel.move );
    }
    else  // Airborne
    {
        #ifdef MOVE_DEBUG
            printf("In Air: Adding velocity (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f)\n",
                    vel.move.x,vel.move.y,vel.move.z,
                    vel.rotate.x,vel.rotate.y,vel.rotate.z);
        #endif

        // Add to existing velocity
        if (vel.move.y <= 0 && (vel.move.x != 0 || vel.move.z != 0))
            actor->Movement().SetVelocity(csVector3(vel.move.x,0,vel.move.z));

        // Set rotation
        actor->Movement().SetAngularVelocity( vel.rotate );
    }
}

void psMovementManager::LoadMouseSettings()
{
    bool inv;
    int v1, v2;

    psengine->GetMouseBinds()->GetOnOff("InvertMouse", inv );
    psengine->GetMouseBinds()->GetInt("VertSensitivity", v1); 
    psengine->GetMouseBinds()->GetInt("HorzSensitivity", v2);

    SetInvertedMouse( inv );
    SetMouseSensY( v1 );
    SetMouseSensX( v2 );
}

void psMovementManager::MouseLookCanAct(bool v){
    mouseLookCanAct = v;
}

void psMovementManager::MouseLook(bool v)
{
    if (!ready || !actor)
        return;

    if(mouseLookCanAct || !v)
    {
        mouseLook = v;
        psCamera* camera = psengine->GetPSCamera();
        if( camera->RotateCameraWithPlayer() )
        {
            if ( !locked )
            {
                if ( !mouseLook )
                {
                    csVector3 vec(0,0,0);
                    actor->Movement().SetAngularVelocity(vec);
                }
            }
        }
    }
    mouseLookCanAct = false;
}

void psMovementManager::UpdateMouseLook()
{
    if (!ready || !actor)
        return;

    psCamera* camera = psengine->GetPSCamera();

    if ( !mouseLook  )
    {
        if ( lastDeltaX == 0 && lastDeltaY == 0 )
        {
            return;
        }
        else
        { // mouseLook turned off. Reset.
            lastDeltaX = 0;
            lastDeltaY = 0;
            if ( camera->RotateCameraWithPlayer() )
            {
                csVector3 vec(0,0,0);
                actor->Movement().SetAngularVelocity(vec);
            }
            return;
        }
    }

    iGraphics2D* g2d = psengine->GetG2D();

    if(activeMoves & kbdRotate && camera->RotateCameraWithPlayer())
    {
        lastDeltaX = 0;
        lastDeltaY = 0;
        g2d->SetMousePosition(g2d->GetWidth() / 2 , g2d->GetHeight() / 2);
        return;
    }

    lastDeltaX *= lastDeltaX / g2d->GetWidth() + 0.5f;
    lastDeltaY *= lastDeltaY / g2d->GetHeight() + 0.5f;

    float deltaPitch = lastDeltaY * (sensY/25000.0f) * (invertedMouse ? 1.0f : -1.0f);
    float deltaYaw;

    if ( camera->RotateCameraWithPlayer() )
    {
        deltaYaw = 0.0f;
        if (!locked)
        {
            float spin = -1.0f * lastDeltaX * (sensX/200.0f);

            if (spin > 5) spin = 5.0f;
            if (spin < -5) spin = -5.0f; 
            actor->Movement().SetAngularVelocity(csVector3(0, spin, 0));
        }
    }
    else
    {
        deltaYaw = lastDeltaX * (sensX/25000.0f);
    }

    camera->MovePitch(deltaPitch);
    camera->MoveYaw(deltaYaw);
}

void psMovementManager::MouseLook(iEvent& ev)
{
    if (!ready || !actor)
        return;

    psCamera* camera = psengine->GetPSCamera();
    iGraphics2D* g2d = psengine->GetG2D();

    int mouseX = csMouseEventHelper::GetX(&ev);
    int mouseY = csMouseEventHelper::GetY(&ev);
    int centerX = g2d->GetWidth() / 2;
    int centerY = g2d->GetHeight() / 2;
    float deltaX = float(mouseX - centerX);
    float deltaY = float(mouseY - centerY);

    if (activeMoves & kbdRotate && camera->RotateCameraWithPlayer())
    {
        lastDeltaX = 0;
        lastDeltaY = 0;
        g2d->SetMousePosition(centerX, centerY);
        return;
    }

    if ( deltaX == 0 && deltaY == 0)
    {// No actual event but caused by reseting the mouse position
        return;
    }
    // remember the current values.
    lastDeltaX = deltaX;
    lastDeltaY = deltaY;

    // Recenter mouse so we don't lose focus
    g2d->SetMousePosition(centerX, centerY);

    float deltaPitch = deltaY * (sensY/25000.0f) * (invertedMouse ? 1.0f : -1.0f);
    float deltaYaw;

    if ( camera->RotateCameraWithPlayer() )
    {
        deltaYaw = 0.0f;
        if (!locked)
        {
            float spin = -1.0f * deltaX * (sensX/200.0f);

            if (spin > 5) spin = 5.0f; // the limitation to +-5.0f slowed the horizontal (LAWI) << on purpose.
            if (spin < -5) spin = -5.0f; 
            //if (spin > 20) spin = 20.0f; // removing that hack which enables "turning with lightspeed"
            //if (spin < -20) spin = -20.0f;
            actor->Movement().SetAngularVelocity(csVector3(0, spin, 0));
        }
    }
    else
    {
        deltaYaw = deltaX * (sensX/25000.0f);
    }

    camera->MovePitch(deltaPitch);
    camera->MoveYaw(deltaYaw);
}

void psMovementManager::MouseZoom(iEvent& ev)
{
    int mouseY = csMouseEventHelper::GetY(&ev);

    // Recenter mouse so we don't lose focus
    iGraphics2D* g2d = psengine->GetG2D();
    int centerX = g2d->GetWidth() / 2;
    int centerY = g2d->GetHeight() / 2;
    g2d->SetMousePosition(centerX, centerY);

    psengine->GetPSCamera()->MoveDistance( float(mouseY - centerY) * (sensY/2500.0f) );
}

void psMovementManager::SetRunToPos(psPoint& mouse)
{
    if (!ready || !actor)
        return;

    csVector3 tmp, tmpDiff;
    iMeshWrapper *mesh = psengine->GetPSCamera()->Get3DPointFrom2D(mouse.x, mouse.y, &tmp, &tmpDiff);
    if (mesh)
    {
        // Stop and remove run-to marker, if one exists
        psengine->GetEffectManager()->DeleteEffect(runToMarkerID);
        runToMarkerID = 0;

        iSector* sector = actor->Movement().GetSector();
        csRef<iMeshWrapper> actormesh = actor->GetMesh();
        runToMarkerID = psengine->GetEffectManager()->RenderEffect("marker", sector, tmp, actormesh);
        runToDiff = tmpDiff;
        lastDist = tmpDiff.SquaredNorm() + 1.0f;

    }
    else
    {
        Error1("Failed to find mesh for SetRunToPos");
    }
}

void psMovementManager::CancelRunTo()
{
    if (runToMarkerID != 0)
        StopAllMovement();
    SetMouseMove(false);
}

void psMovementManager::UpdateRunTo()
{
    if (!ready || !actor)
        return;

    if (runToMarkerID != 0 && !locked)
    {
        csVector3 currPos;
        float yRot;
        iSector* sector;
        actor->Movement().GetLastPosition(currPos, yRot, sector);
        csVector3 diff = runToDiff - currPos;

        // only move the char if we are not stuck
        if (diff.SquaredNorm() > RUNTO_EPSILON*RUNTO_EPSILON)
        {
            float targetYRot = atan2(-diff.x,-diff.z);
            lastDist = diff.SquaredNorm();

#if USE_EXPERIMENTAL_AUTOMOVE

            // Try to avoid big ugly stuff in our path
            iSector* sector = movePropClass->GetSector();
            if ( sector )
            {
                csVector3 isect,start,end,dummy,box,legs;
                csRef<iCollideSystem> cdsys =  csQueryRegistry<iCollideSystem> (psengine->GetObjectRegistry ());
                // Construct the feeling broom

                // Calculate the start and end poses
                start= currPos;
                actor->Movement().GetCDDimensions(box, legs, dummy);
                
                // We can walk over some stuff
                start += csVector3(0,0.6f,0);
                end = start + csVector3(sinf(targetYRot), 0, cosf(targetYRot)) * -0.5;

                // Feel
                csIntersectingTriangle closest_tri;
                iMeshWrapper* sel = 0;
                float dist = csColliderHelper::TraceBeam (cdsys, sector,
                        start, end, true, closest_tri, isect, &sel);


                if(dist > 0)
                {
                    const float begin = (PI/6); // The lowest turning constant
                    
                    const float delta = (PI/8);
                    const float length = 0.5;

                    float left,right,turn;
                    for(int i = 1; i <= 3;i++)
                    {
                        csVector3 broomS[2],broomE[2];

                        // Left and right
                        left = targetYRot - (begin * float(i));
                        right = targetYRot + (begin * float(i));

                        // Construct the testing brooms
                        broomE[0] = broomS[0] = end;
                        broomS[0] = currPos + csVector3(sinf(left+delta),0,cosf(left+delta)) * -length;
                        broomE[0] = currPos + csVector3(sinf(left-delta),0,cosf(left-delta)) * -length;

                        broomE[1] = broomS[1] = end;
                        broomS[1] = currPos + csVector3(sinf(right+delta),0,cosf(right+delta)) * -length;
                        broomE[1] = currPos + csVector3(sinf(right-delta),0,cosf(right-delta)) * -length;

                        // The broom is already 0.6 over the ground, so we need to cut that out
                        broomS[0].y += legs.y + box.y - 0.6;
                        broomE[1].y += legs.y + box.y - 0.6;

                        // Check if we can get the broom through where we want to go
                        float dist = csColliderHelper::TraceBeam (cdsys, sector,
                                broomS[0], broomE[0], true, closest_tri, isect, &sel);

                        if(dist < 0)
                        {
                            turn = left;
                            break;
                        }

                        // Do again for the other side
                        dist = csColliderHelper::TraceBeam (cdsys, sector,
                                broomS[1], broomE[1], true, closest_tri, isect, &sel);

                        if(dist < 0)
                        {
                            turn = right;
                            break;
                        }


                    }
                    // Apply turn
                    targetYRot = turn;
                }
            }

#endif

            targetYRot = yRot-targetYRot;
            if (targetYRot > PI )
                 targetYRot -= TWO_PI;
            if (targetYRot > -1.0f && targetYRot < 1.0f)
                targetYRot *= 3;

            if (lastDist < 10.0f)
            {
                if (fabs(targetYRot) < 1)
                {
                    if (lastDist < 0.05)
                    {
                        if (mouseAutoMove > 1)
                            mouseAutoMove -= 2;
                        CancelRunTo();
                    }
                    else if (lastDist < 1.0f)
                    {
                        Stop(run);
                        Start(forward);
                    }
                    else
                    {
                        if (mouseAutoMove == 1)
                            Start(run);
                        Start(forward);
                    }
                }
                else
                {
                    Stop(forward);
                    Stop(run);
                    if (mouseAutoMove < 2)
                        mouseAutoMove+=2;
                }
            }
            else
            {
                if (mouseAutoMove)
                    Start(run);
                Start(forward);
            }

            // Turn towards target
            actor->Movement().SetAngularVelocity(csVector3(0, targetYRot, 0));
        }
        else
        {
            if (mouseAutoMove > 1)
                mouseAutoMove -= 2;
            CancelRunTo();
        }
    }
}

void psMovementManager::ToggleAutoMove()
{
    if (run && forward && !locked)
    {
        if(!mouseMove)
        {
            const bool nextAutoMove = !autoMove;
            StopAllMovement();
            if(nextAutoMove)
            {
                Start(forward);
            }
            autoMove = nextAutoMove;
        }
    }
}

bool psMovementManager::ToggleRun()
{
    if(toggleRun)
    {
        defaultmode = walk;
        Stop(run);
        toggleRun = false;
    }
    else
    {
        defaultmode = run;
        Start(run);
        toggleRun = true;
    }

    return toggleRun;
}

bool psMovementManager::SetRun(bool runState)
{
    if (runState)
    {
        defaultmode = run;
        Start(run);
        toggleRun = true;
    }
    else
    {
        defaultmode = walk;
        Start(walk);
        toggleRun = false;
    }

    return toggleRun;
}
