/*
 * Author: Andrew Robberts
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

#ifndef PS_CAMERA_HEADER
#define PS_CAMERA_HEADER
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/ref.h>
#include <iutil/virtclk.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psoptions.h"

#define NPC_MODE_DISTANCE 5.0f

struct RaceDefinition;
struct iView;
struct iPerspectiveCamera;
struct iSector;
struct iCollideSystem;
struct iMeshWrapper;

class GEMClientActor;
class GEMClientObject;

class psCamera : public iOptionsClass, public psCmdBase
{
public:

    psCamera();
    virtual ~psCamera();

    void LoadOptions();
    void SaveOptions();

    // the different camera modes
    enum CAMERA_MODES
    {
        CAMERA_FIRST_PERSON = 0,
        CAMERA_THIRD_PERSON,
        CAMERA_M64_THIRD_PERSON,
        CAMERA_LARA_THIRD_PERSON,
        CAMERA_FREE,

        // a special hidden camera mode that contains the actual data to use (all other camera modes set this one accordingly)
        CAMERA_ACTUAL_DATA, // this should be the first entry here after the real camera modes
        CAMERA_LAST_ACTUAL, // keep reference to what the actual data was last frame

        // This is a camera mode that occurs in the transition phase in between normal camera modes.
        CAMERA_TRANSITION,

        // camera mode for talking to an NPC
        CAMERA_NPCTALK,

        // A camera mode that is used when a collision occurs.
        // It allows for collisions to be rectified faster than normal
        CAMERA_COLLISION,
        CAMERA_ERR, // this is the error between the actual camera pos and its ideal

        CAMERA_MODES_COUNT
    };

    struct DistanceCfg
    {
        bool adaptive;      // is distance adapted automatically ?
        int dist;           // fixed distance (used when adaptive=false)
        int minFPS;         // acceptable FPS interval (used when adaptive=true)
        int maxFPS;
        int minDist;        // minimum acceptable distance (used when adaptive=true)
    };

    const char *HandleCommand(const char *cmd);
    void HandleMessage(MsgEntry *msg);


    /** creates the viewport
     *   @param entity the actor for the view
     *   @return true on success, false on error
     */
    bool InitializeView(GEMClientActor* entity);

    /** Set the actor the camera is following
     * @param entity the actor for the view
     */
    void SetActor(GEMClientActor* entity);

    /** Replace the actor the camera is targeting when in npc camera mode.
     *  It's replaced only if the actor is equal. This is to be used when
     *  deleting actor objects or replacing them, in order to avoid a pending
     *  invalid pointer in pscamera.
     *  @param currentEntity The actor which is being replaced/deleted.
     *  @param replacementEntity The actor instance which is being created. Pass NULL if it's
     *                           only being destroyed.
     */
    void npcTargetReplaceIfEqual(GEMClientObject *currentEntity, GEMClientObject* replacementEntity);

    /** loads the camera settings from a file
     *   @param useDefault if true, use the default config file even if the normal exists
     *   @param overrideCurrent if true, change current camera settings with loaded data
     *   @return true on success, false otherwise
     */
    bool LoadFromFile(bool useDefault=false, bool overrideCurrent=true);

    /** saves the camera settings to a file
     *   @return true on success, false otherwise
     */
    bool SaveToFile();

    /** applies the camera data to the viewport and renders the 3D scene
     *   @return true on success, false on error
     */
    bool Draw();

    /** sets the current position of the camera (different for each mode)
     *   @param pos The new camera position for the current mode
     * @param mode the camera mode
     */
    void SetPosition(const csVector3& pos, int mode = -1);

    /** returns the current position of the camera (different for each mode)
     * @param mode the camera mode
     *   @return The current camera position for the current mode
     */
    csVector3 GetPosition(int mode = -1) const;

    /**
     * Sets the current target of the camera (different for each mode).
     *
     * @param tar The new camera target for the current mode
     * @param mode the camera mode
     */
    void SetTarget(const csVector3& tar, int mode = -1);

    /** returns the current position of the camera (different for each mode)
     * @param mode the camera mode
     * @return The current camera target for the current mode
     */
    csVector3 GetTarget(int mode = -1) const;

    /**
     * Sets the current up vector of the camera (different for each mode).
     *
     * @param up The new camera up vector for the current mode
     * @param mode the camera mode
     */
    void SetUp(const csVector3& up, int mode = -1);

    /**
     * Returns the current up vector of the camera (different for each mode).
     *
     * @param mode the camera mode
     * @return The current camera up vector for the current mode
     */
    csVector3 GetUp(int mode = -1) const;

    /** locks the current camera mode so it cannot be changed
     *   @param state the desired state
     */
    void LockCameraMode(bool state);

    /** sets the current camera mode
     *   @param mode the desired camera mode
     */
    void SetCameraMode(int mode);

    /** changes the current camera mode to the next available camera mode
     */
    void NextCameraMode();

    /** returns the current camera mode (as integer)
     *   @return The current camera mode
     */
    int GetCameraMode() const;

    /** returns the current camera mode (text description)
     *   @return A csString describing the current camera mode
     */
    csString GetCameraModeVerbose() const;

    /** returns the camera that CS use
     *   @return An iCamera that CS uses
     */
    iPerspectiveCamera *GetICamera();

    /** returns the CS viewport
     *   @return an iView of this camera's CS viewport
     */
    iView *GetView();

    /** returns the 3D point where the line under the given point intersects with the world
     *   @param x the x part of the 2D point
     *   @param y the y part of the 2D point
     *   @param worldCoord container to hold the calculated 3D world vector under the 2D point
     *   @param untransfCoord container to hold the untransformed 3D world vector (untouched by warp portals)
     *   @return the mesh it intersects with
     */
    iMeshWrapper * Get3DPointFrom2D(int x, int y, csVector3 * worldCoord=0, csVector3 * untransfCoord=0);

    /**
     * Returns the mesh under the 2D point
     *
     * @param x the x part of the 2D point
     * @param y the y part of the 2D point
     * @param[out] pos The best found position.
     * @param poly Not used
     * @return the mesh under the 2D point
     */
    iMeshWrapper* FindMeshUnder2D(int x, int y, csVector3  * pos = NULL, int *poly = NULL);

    /** sets the pitch (up/down) of the camera
     *   @param pitch the new pitch of the camera
     *   @param mode Optional - the camera mode to apply it to (leave blank for current)
     */
    void SetPitch(float pitch, int mode = -1);

    /** moves the pitch (up/down) of the camera
     *   @param deltaPitch the amount to move from the current pitch
     *   @param mode Optional - the camera mode to apply it to (leave blank for current)
     */
    void MovePitch(float deltaPitch, int mode = -1);

    /** returns the pitch (up/down) of the camera
     *   @param mode Optional - the camera mode to get it from (leave blank for current)
     *   @return the pitch (up/down) of the camera
     */
    float GetPitch(int mode = -1) const;

    /** sets the pitch (up/down) velocity of the camera
     *   @param pitchVel the velocity of the pitch
     */
    void SetPitchVelocity(float pitchVel);

    /** gets the pitch (up/down) velocity of the camera
     *   @return the pitch (up/down) of the camera
     */
    float GetPitchVelocity() const;

    /** sets the yaw (left/right) of the camera
     *   @param yaw the new yaw of the camera
     *   @param mode Optional - the camera mode to apply it to (leave blank for current)
     */
    void SetYaw(float yaw, int mode = -1);

    /** moves the yaw (left/right) of the camera
     *   @param deltaYaw the amount to move from the current yaw
     *   @param mode Optional - the camera mode to apply it to (leave blank for current)
     */
    void MoveYaw(float deltaYaw, int mode = -1);

    /** returns the yaw (left/right) of the camera
     *   @param mode Optional - the camera mode to get it from (leave blank for current)
     *   @return the yaw (left/right) of the camera
     */
    float GetYaw(int mode = -1) const;

    /** sets the yaw (up/down) velocity of the camera
     *   @param yawVel the velocity of the yaw
     */
    void SetYawVelocity(float yawVel);

    /** gets the yaw (up/down) velocity of the camera
     *   @return the yaw (up/down) of the camera
     */
    float GetYawVelocity() const;

    /** sets the distance from the camera position to its target
     *   @param distance the new distance
     *   @param mode Optional - the camera mode to apply it to (leave blank for current)
     */
    void SetDistance(float distance, int mode = -1);

    /** moves the distance from the camera position to its target
     *   @param deltaDistance the amount to displace the current distance
     *   @param mode Optional - the camera mode to apply it to (leave blank for current)
     */
    void MoveDistance(float deltaDistance, int mode = -1);

    /** returns the distance from the camera position to its target
     *   @param mode Optional - the camera mode to get it from (leave blank for current)
     *   @return the distance from the camera position to its target
     */
    float GetDistance(int mode = -1) const;

    /** resets the actual camera data to the player position and stuff
     */
    void ResetActualCameraData();

    /** flags the camera positioning as bad so that it will reset it next update loop
     */
    void ResetCameraPositioning();

    /** moves decides, based on the camera mode, whether the camera should be rotated along-side the player
     *   @return true if the camera should be rotated, false otherwise
     */
    bool RotateCameraWithPlayer() const;

    /** returns the forward vector of the camera
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the forward vector
     */
    csVector3 GetForwardVector(int mode = -1) const;

    /** returns the forward vector of the camera
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the forward vector
     */
    csVector3 GetRightVector(int mode = -1) const;

    /** returns the min distance from the camera position to the target
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the min distance
     */
    float GetMinDistance(int mode = -1) const;

    /** sets the min distance from the camera position to the target
     *   @param dist the min distance
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetMinDistance(float dist, int mode = -1);

    /** returns the max distance from the camera position to the target
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the max distance
     */
    float GetMaxDistance(int mode = -1) const;

    /** sets the max distance from the camera position to the target
     *   @param dist the max distance
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetMaxDistance(float dist, int mode = -1);

    /** returns the turning speed of the camera (ignored for most camera modes)
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the turning speed
     */
    float GetTurnSpeed(int mode = -1) const;

    /** sets the turning speed of the camera (ignored for most camera modes)
     *   @param speed the turning speed
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetTurnSpeed(float speed, int mode = -1);

    /** returns the spring coefficient of the camera
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the spring coefficient
     */
    float GetSpringCoef(int mode = -1) const;

    /** sets the spring coefficient of the camera
     *   @param coef the spring coefficient
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetSpringCoef(float coef, int mode = -1);

    /** returns the dampening coefficient of the camera
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the dampening coefficient
     */
    float GetDampeningCoef(int mode = -1) const;

    /** sets the dampening coefficient of the camera
     *   @param coef the dampening coefficient
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetDampeningCoef(float coef, int mode = -1);

    /** returns the spring length of the camera
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the spring length
     */
    float GetSpringLength(int mode = -1) const;

    /** sets the spring length of the camera
     *   @param length the spring length
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetSpringLength(float length, int mode = -1);

    /** returns whether the camera is performing CD or not
     *   @return true if using CD, false otherwise
     */
    bool CheckCameraCD() const;

    /** sets whether the camera is performing CD or not
     *   @param useCD true if the camera should perform CD, false otherwise
     */
    void SetCameraCD(bool useCD);

    /** returns whether the npc chat camera is used or not
     *  @return true if using npc chat camera, false otherwise
     */
    bool GetUseNPCCam();

    /** sets whether the npc chat camera is used or not
     *  @param useNPCCam true if the npc chat camera should be used, false otherwise
     */
    void SetUseNPCCam(bool useNPCCam);

    /** gets the camera transition threshold (the distance between camera position and ideal where the camera ceases to be in transition)
     *   @return the camera transition threshold
     */
    float GetTransitionThreshold() const;

    /** sets the camera transition threshold (the distance between camera position and ideal where the camera ceases to be in transition)
     *   @param threshold the camera transition threshold
     */
    void SetTransitionThreshold(float threshold);

    /** returns the default (starting) pitch of the specific camera mode
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the default (starting) pitch
     */
    float GetDefaultPitch(int mode = -1) const;

    /** sets the default (starting) pitch for the specific camera mode
     *   @param pitch the new default (starting) pitch
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetDefaultPitch(float pitch, int mode = -1);

    /** returns the default (starting) yaw of the specific camera mode
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the default (starting) yaw
     */
    float GetDefaultYaw(int mode = -1) const;

    /** sets the default (starting) yaw for the specific camera mode
     *   @param yaw the new default (starting) yaw
     *   @param mode Optional - the camera mode to set the data (leave blank for current)
     */
    void SetDefaultYaw(float yaw, int mode = -1);

    /** returns the swing coefficient of the specific camera mode
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     *   @return the swing coefficient
     */
    float GetSwingCoef(int mode = -1) const;

    /** sets the swing coefficient of the specific camera mode
     *   @param swingCoef the new swing coefficient
     *   @param mode Optional - the camera mode to get the data from (leave blank for current)
     */
    void SetSwingCoef(float swingCoef, int mode = -1);

    /** puts camera to fixed distance clipping mode - distance clipping will be set to constant value
     *   @param dist the maximum visible distance
     */
    void UseFixedDistanceClipping(float dist);

    /** puts camera to adaptive distance clipping mode - camera will automatically adjust the distance to stay in given fps interval
     *   @param minFPS minimum fps (viewing distance will shrink when fps is lower)
     *   @param maxFPS maximum fps (viewing distance will rise when fps is higher)
     *   @param minDist lower bound on distance that will be never crossed
     */
    void UseAdaptiveDistanceClipping(int minFPS, int maxFPS, int minDist);

     /** gets the last used camera mode
     *   @return the last camera mode
     */
    int GetLastCameraMode();

    DistanceCfg GetDistanceCfg();
    void SetDistanceCfg(DistanceCfg newcfg);

    int GetFixedDistClip() { return fixedDistClip; }

    bool IsInitialized() { return cameraInitialized; }

private:

    struct CameraData
    {
        csVector3 worldPos;
        csVector3 worldTar;
        csVector3 worldUp;

        float pitch;
        float yaw;
        float roll; // not implemented yet (no need atm)

        float defaultPitch;
        float defaultYaw;
        float defaultRoll;

        float distance; // the distance between position and target
        float minDistance;
        float maxDistance;

        float turnSpeed;

        float springCoef;
        float InertialDampeningCoef;
        float springLength;

        float swingCoef;

        //initialize all here:
        CameraData()
        {
            worldPos.Set(0.0f,0.0f,0.0f);
            worldTar.Set(0.0f,0.0f,0.0f);
            worldUp.Set(0.0f,0.0f,0.0f);
            pitch = yaw = roll = 0;
            defaultPitch = defaultYaw = defaultRoll = 0.0f;
            distance = minDistance = maxDistance = 0.0f;
            turnSpeed  = 0.0f;
            springCoef = InertialDampeningCoef = springLength = 0.0f;
            swingCoef=0.0f;
        }
    };

    float CalcSpringCoef(float springyness, float scale) const;
    float CalcInertialDampeningCoef(float springyness, float scale) const;
    float EstimateSpringyness(size_t camMode) const;

    /** calculates the ideal camera position/target/up/yaw/pitch/roll for the current camera mode
     *   @param elapsedTicks the number of ticks that have occured since the last frame
     *   @param actorPos the world-space position of the actor
     *   @param actorEye the world-space position of the actor's eye
     *   @param actorYRot the actor's yaw rotation
     */
    void DoCameraIdealCalcs(const csTicks elapsedTicks, const csVector3& actorPos, const csVector3& actorEye, const float actorYRot);

    /** performs collision detection between the camera position and the player so that objects (ie, a wall) don't block your view
     *   @param pseudoTarget the first end of the line segment to test
     *   @param pseudoPosition the second end of the line segment to test
     *   @param sector the sector to start from (should be the sector where pseudoTarget resides)
     *                 this will be set to the sector where the line segment ends
     *   @return the new world position where it first collides without the world
     */
    csVector3 CalcCollisionPos(const csVector3& pseudoTarget, const csVector3& pseudoPosition, iSector*& sector);

    /** decides when the camera isn't in transition mode and handles taking it out of transition mode
     */
    void DoCameraTransition();

    /**
     * Handles the interpolation between the actual camera data and the ideal camera data for the current mode.
     *
     * @param isElastic when true, the camera will follow Hooke's law and dampening mechanics to interpolate, otherwise the interpolation is instant
     * @param elapsedTicks Elapsed ticks.
     * @param deltaIdeal the change between the ideal data of last frame and this one, used for dampening
     * @param sector The sector.
     */
    void DoElasticPhysics(bool isElastic, const csTicks elapsedTicks, const CameraData& deltaIdeal, iSector* sector);

    /**
     * Ensures that the actor is (in)visible when he's supposed to be.
     */
    void EnsureActorVisibility();

    /** calculates an elastic position
     *  This calculates an interpolated position modeled from Hooke's law and some nice inertial dampening physics
     *  NOTE: deltaTime should always be in seconds and this function shouldn't be modified to adjust any sort of scale.
     *        If you need to adjust for scale or any settings, change springCoef, dampCoef, and/or springLength
     *   @param currPos the current position (not necessarily camera position)
     *   @param idealPos the ideal position (basically the location of the spring
     *   @param deltaIdealPos the difference between the last idealPos and the current one (velocity) - this is used for inertial dampening
     *   @param deltaTime the elapsed time (in seconds)
     *   @param springCoef this is basically the springyness of the spring, as this value rises the spring becomes stronger and more rigid
     *   @param dampCoef this restricts/opposes movement of idealPos
     *   @param springLength this is the length of the spring, if the scale of the world increased 10x, this parameter would be increased 10x (leaving the others alone) to get the same physics
     *   @return the new position
     */
    csVector3 CalcElasticPos(csVector3 currPos, csVector3 idealPos, csVector3 deltaIdealPos,
                             float deltaTime, float springCoef, float dampCoef, float springLength) const;


    /** calculates an elastic floating point value
     *  This calculates an interpolated floating point modeled from Hooke's law and some nice inertial dampening physics
     *  NOTE: deltaTime should always be in seconds and this function shouldn't be modified to adjust any sort of scale.
     *        If you need to adjust for scale or any settings, change springCoef, dampCoef, and/or springLength
     *   @param curr the current value (not necessarily camera position)
     *   @param ideal the ideal value (basically the location of the spring
     *   @param deltaIdeal the difference between the last ideal and the current (velocity) - this is used for inertial dampening
     *   @param deltaTime the elapsed time (in seconds)
     *   @param springCoef this is basically the springyness of the spring, as this value rises the spring becomes stronger and more rigid
     *   @param dampCoef this restricts/opposes movement of idealPos
     *   @param springLength this is the length of the spring, if the scale of the world increased 10x, this parameter would be increased 10x (leaving the others alone) to get the same physics
     *   @return the new floating point
     */
    float CalcElasticFloat(float curr, float ideal, float deltaIdeal,
                           float deltaTime, float springCoef, float dampCoef, float springLength) const;


    /** calculates a new target and up vector from yaw, pitch, and roll, this one sets and gets using internal variables (no params or returns)
     *   @param mode Optional - the camera mode to apply and get the data
     */
    void CalculateFromYawPitchRoll(int mode = -1);

    /** calculates new yaw, pitch, and roll values from the target and position
     *   @param mode Optional - the camera mode to apply and get the data
     */
    void CalculateNewYawPitchRoll(int mode = -1);

    /** this is similar to CalculateFromYawPitchRoll() except this one keeps the target vector the way it is and calculates a new position
     *   @param mode Optional - the camera mode to apply and get the data
     */
    void CalculatePositionFromYawPitchRoll(int mode = -1);

    /** calculates a new yaw rotation value given a directional vector
     *   @param dir the direction of the camera or any direction vector you want
     *   @return the new yaw value of the direction vector
     */
    float CalculateNewYaw(csVector3 dir);

    /** ensures that the camera distance is within the range for the camera mode
     *   @param mode Optional - the camera mode to get/set the data (leave blank for current)
     */
    void EnsureCameraDistance(int mode = -1);

    /** checks to see if the actor should be visible in the given camera mode
     *   @param mode Optional - the camera mode to get/set the data (leave blank for current)
     *   @return true if actor is visible, false otherwise
     */
    bool IsActorVisible(int mode = -1) const;

    /** saturates the angle to ensure that no two angles are > 2*PI difference
     *   @param angle the unsaturated angle
     *   @return an angle between 0 and 2*PI
     */
    float SaturateAngle(float angle) const;

    /** clones camera data from one mode to another
     *   @param fromMode the camera mode to get the data from (not optional, but setting to -1 will use the current mode)
     *   @param toMode Optional - the camera mode to set the data (leave blank for current)
     *   @return true on success, false otherwise
     */
    bool CloneCameraModeData(int fromMode, int toMode = -1);

    /** gets/sets how far does the camera see - a limit on visible distance
     *   @param dist the maximum visible distance
     */
    void SetDistanceClipping(float dist);
    float GetDistanceClipping();

    /** calculates and sets appropriate level of distance clipping
      * this must be called every frame
      */
    void AdaptDistanceClipping();

    // each mode's camera data has to be handled seperately
    CameraData camData[CAMERA_MODES_COUNT];

    int lastCameraMode;
    int currCameraMode;

    // velocity stuff
    float pitchVelocity;
    float yawVelocity;

    // some camera settings
    // NOTE: the offsets are not in the format you think they are, due to speed issues the offset is restricted to being directly behind the actor (no yaw).
    csVector3 firstPersonPositionOffset;
    csVector3 thirdPersonPositionOffset;

    /* if the squared difference between the actual camera position and the ideal position is less than this,
     * then we're no longer in the transition phase.
     */
    float transitionThresholdSquared;
    bool inTransitionPhase;

    /// used to tell if the camera is in collision buffer phase
    bool hasCollision;

    /// used to make sure the camera starts off at the player pos and not outside of the map somewhere
    bool cameraHasBeenPositioned;

    /// perform collision detection between the camera target and position
    bool useCameraCD;

    /// use the npc chat camera when talking to NPCs
    bool useNPCCam;

    /// Have the settings been loaded from file and the camera mode been initialized?
    bool cameraInitialized;

    /// stores if camera mode is locked
    bool lockedCameraMode;

    csWeakRef<iSector> lastTargetSector;

    GEMClientActor* actor;

    /// viewport used by CS
    csRef<iView> view;

    /// virtual clock to keep track of the ticks passed between frames
    csRef<iVirtualClock> vc;

    /// Collision detection system.
    csRef<iCollideSystem> cdsys;

    /// the timer state of the last time Update() was called
    csTicks prevTicks;

    /// race definition, used for camera offsets
    RaceDefinition *race;

    DistanceCfg distanceCfg;

    /** @brief Keep a pointer to the rotated npc
     * In case we change of target, this allows to restore the rotation
     * of the previously selected npc
     */
    GEMClientActor* npcModeTarget;

    /// Keep our position when we started talking to the npc
    csVector3 npcModePosition;

    /// Keep the old rotation of the npc
    float npcOldRot;
    float vel;
    int fixedDistClip;
};

#endif
