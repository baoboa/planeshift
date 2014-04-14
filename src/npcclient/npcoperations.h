/*
* npcoperations.h
*
* Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __NPCOPERATIONS_H__
#define __NPCOPERATIONS_H__

#include <psstdint.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/vector3.h>
#include <csutil/csstring.h>

struct iDocumentNode;
struct iSector;

//=============================================================================
// Library Includes
//=============================================================================
#include "util/psconst.h"
#include "util/pspath.h"
#include "util/pspathnetwork.h"
#include <tools/celhpf.h>
#include "net/npcmessages.h"
#include "util/edge.h"
#include "util/mathscript.h"

//=============================================================================
// Project Includes
//=============================================================================
#include "npc.h"
#include "reaction.h"
#include "perceptions.h"

/**
 * \addtogroup script_operations
 * @{ */

class NPC;
class gemNPCObject;
class gemNPCActor;
class MoveOperation;
class Waypoint;
class Behavior;

/**
* This is the base class for all operations in action scripts.
* We will use these to set animation actions, move and rotate
* the mob, attack and so forth.  These should be mostly visual
* or physical things, except for the ability to create new
* perceptions, which can kick off other actions.
*/
class ScriptOperation
{
public:
    enum VelSource
    {
        VEL_DEFAULT,
        VEL_USER,
        VEL_WALK,
        VEL_RUN
    };

    // Used to indicate the result of the Run and Advance operations.
    enum OperationResult
    {
        OPERATION_NOT_COMPLETED, ///< Used to indicate that an operation will complete at a later stage
        OPERATION_COMPLETED,     ///< All parts of this operation has been completed
        OPERATION_FAILED         ///< The operation failed
    };

    enum State
    {
        READY_TO_RUN,
        RUNNING,
        INTERRUPTED,
        COMPLETED
    };

protected:
    ////////////////////////////////////////////////////////////
    // Start of instance temp variables. These dosn't need to be copied.
    csString             name;

    csVector3            interrupted_position;
    iSector*             interrupted_sector;
    float                interrupted_angle;

    int                  consecCollisions; ///< Shared by move functions. Used by CheckMoveOk to detect collisions

    Behavior*            parent;

    State                state;
    // End of instance temp variables.
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // Start of shared values between operation

    // Failure handling
    csString             failurePerception;    ///< The Perception to fire when a operation return OPERATION_FAILED

    // Velocity is shared for all move operations
    VelSource            velSource;            ///< Source used for velocity
    float                vel;                  ///< Shared linear velocity, used by all that moves
    float                ang_vel;              ///< Shared angular velocity, used by all that rotates

    // Start Check Move OK parameters
    // Configuration paramters. Set by using LoadCheckMoveOk
    csString             collision;       ///< Perception names to use for collision detected by CheckMoveOk
    csString             outOfBounds;     ///< Perception names to use for out of bounds detected by CheckMoveOk
    csString             inBounds;        ///< Perception names to use for in bounds detected by CheckMoveOk
    csString             falling;        ///< Perception names to use for fall detected by CheckMoveOk
    bool                 checkTribeHome;  ///< Set to true if the tribe home should be checked by CheckMoveOk
    // End Check Move OK parameters


    // End of shared values between operations
    ////////////////////////////////////////////////////////////

    /// This function is used by MoveTo AND Navigate operations
    int StartMoveTo(NPC* npc, const csVector3 &dest, iSector* sector, float vel,const char* action, float &angle);

    void TurnTo(NPC* npc,const csVector3 &dest, iSector* destsect, csVector3 &forward, float &angle);

    /// Utility function used by many operation to stop movement of an NPC.
    static void StopMovement(NPC* npc);

public:

    ScriptOperation(const char* sciptName);
    ScriptOperation(const ScriptOperation* other);
    virtual ~ScriptOperation() {}

    virtual OperationResult Run(NPC* npc,bool interrupted)=0;
    virtual OperationResult Advance(float timedelta,NPC* npc);

    virtual void InterruptOperation(NPC* npc);
    virtual bool AtInterruptedPosition(const csVector3 &pos, const iSector* sector);
    virtual bool AtInterruptedAngle(const csVector3 &pos, const iSector* sector, float angle);
    virtual bool AtInterruptedPosition(NPC* npc);
    virtual bool AtInterruptedAngle(NPC* npc);

    virtual void SetState(State state)
    {
        this->state = state;
    }
    virtual State GetState() const
    {
        return state;
    }


private:
    /** Send a collition perception to the npc.
     *
     * Get the collisiont detection for this operation.
     * Can either be part of this operation, or the npctype.
     *
     * @param npc The npc to percept.
     */
    void SendCollitionPerception(NPC* npc);

public:
    /** Check if the move where ok.
     *
     * Check if a move operation has collided, walked out of bounds, or walked in bound. Will
     * fire perceptions at these events with the names given by \sa LoadCheckMoveOk
     *
     */
    virtual bool CheckMoveOk(NPC* npc, csVector3 oldPos, iSector* oldSector,
                             const csVector3 &newPos, iSector* newSector, int resultFromExtrapolate);

    /** Check if the end point where ok.
     *
     * Check if a move operation has collided and not ended in right position.
     * Fire perceptions if not at end position.
     *
     */
    virtual bool CheckEndPointOk(NPC* npc, const csVector3 &myPos, iSector* mySector,
                                 const csVector3 &endPos, iSector* endSector);

    /** Return the Collision perception event name.
     *
     *  Will use the perception of the operation if defined, or it will
     *  check if a global perception event name is defined for the brain.
     */
    virtual const csString &GetCollisionPerception(NPC* npc);

    /** Return the Out of Bounds perception event name.
     *
     *  Will use the perception of the operation if defined, or it will
     *  check if a global perception event name is defined for the brain.
     */
    virtual const csString &GetOutOfBoundsPerception(NPC* npc);

    /** Return the In Bounds perception event name.
     *
     *  Will use the perception of the operation if defined, or it will
     *  check if a global perception event name is defined for the brain.
     */
    virtual const csString &GetInBoundsPerception(NPC* npc);

    /** Return the Falling perception event name.
     *
     *  Will use the falling perception of the operation if defined, or it will
     *  check if a global fall perception event name is defined for the brain.
     */
    virtual const csString &GetFallingPerception(NPC* npc);

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy()=0;

    /** Return the velocity for the npc.
     *
     *  Determined by the script this could be the
     *  walk, run, or a defined velocity.
     */
    virtual float GetVelocity(NPC* npc);

    virtual float GetAngularVelocity(NPC* npc);
    bool LoadVelocity(iDocumentNode* node);

    /** Load attributes for the CheckMoveOk check
     *
     * @param node The node to load attributes from.
     */
    bool LoadCheckMoveOk(iDocumentNode* node);

    /** Copy CheckMoveOk paramters from the source script
     *
     * @param source The script to copy from.
     *
     */
    void CopyCheckMoveOk(ScriptOperation*  source);

    /** Move a point somwhere random within radius of the orignial point.
     *
     * Will add a random part to the dest. The margin can
     * be used to prevent using the margin around the edge of the circle.
     *
     * @param dest   The center position of the cicle. Will return with the
     *               new position.
     * @param radius The radius of the circle.
     * @param margin A margin around the circle that will not be used.
     *
     */
    void AddRandomRange(csVector3 &dest, float radius, float margin = 0.0);
    void SetAnimation(NPC* npc, const char* name);

    virtual const char* GetName() const
    {
        return name.GetDataSafe();
    };

    /**
     * Called when the run operation return OPERATION_FAILED.
     * Default implementation will percept with the failure perception
     */
    virtual void Failure(NPC* npc);

    /** Set the parent behavior for this operation */
    void SetParent(Behavior* behavior);
};

//-----------------------------------------------------------------------------

/**
* Abstract common class for Move operations that use paths.
*/
class MovementOperation : public ScriptOperation
{
protected:

    // Instance variables
    csRef<iCelHPath> path;

    // Cache values for end position
    csVector3 endPos;
    iSector*  endSector;

    float currentDistance;          ///< The distance to the current local destination

    // Operation parameters
    csString         action;        ///< The animation used during chase

    // Constructor
    MovementOperation(const MovementOperation* other);

public:

    MovementOperation(const char*  name);

    virtual ~MovementOperation() { }

    virtual bool Load(iDocumentNode* node);

    bool EndPointChanged(const csVector3 &endPos, const iSector* endSector) const;

    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector) = 0;

    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector) = 0;

    virtual OperationResult Run(NPC* npc,bool interrupted);

    virtual OperationResult Advance(float timedelta,NPC* npc);

    virtual void InterruptOperation(NPC* npc);

};

//---------------------------------------------------------------------------
//         Following section contain specefix NPC operations.
//         Ordered alphabeticaly, with exception for when needed
//         due to inheritance. MoveOperation is out of order compared
//         to the implementation file.
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------

/**
* Will send an assessment request to the server. Upon return
* the npc will be percepted based on the assessment.
*/
class AssessOperation : public ScriptOperation
{
protected:
    csString physicalAssessmentPerception;
    csString magicalAssessmentPerception;
    csString overallAssessmentPerception;
public:

    AssessOperation(): ScriptOperation("Assess") {};
    virtual ~AssessOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Will build a tribe building.
*/
class BuildOperation : public ScriptOperation
{
protected:
    bool pickupable;
public:

    BuildOperation(): ScriptOperation("Build"),pickupable(false) {};
    virtual ~BuildOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Will unbuild a tribe building.
*/
class UnbuildOperation : public ScriptOperation
{
protected:
public:

    UnbuildOperation(): ScriptOperation("Unbuild") {};
    virtual ~UnbuildOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Will Set the busy indicator for an NPC.
*/
class BusyOperation : public ScriptOperation
{
protected:
    bool busy;
public:

    BusyOperation(bool busy): ScriptOperation(busy?"Busy":"Idle"),busy(busy) {};
    virtual ~BusyOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Let the NPC cast a spell.
*/
class CastOperation : public ScriptOperation
{
protected:
    csString spell;
    csString kFactor;

    /** Constructor for this operation, used by the MakeCopy.
     *
     *  This constructor will copy all the Operation Parameters
     *  from the other operation and initialize all Instance Variables
     *  to default values.
     */
    CastOperation(const CastOperation* other);

public:
    CastOperation();
    virtual ~CastOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Let the NPC change brain.
*/
class ChangeBrainOperation : public ScriptOperation
{
protected:
    csString brain;

    /** Constructor for this operation, used by the MakeCopy.
     *
     *  This constructor will copy all the Operation Parameters
     *  from the other operation and initialize all Instance Variables
     *  to default values.
     */
    ChangeBrainOperation(const ChangeBrainOperation* other);

public:
    ChangeBrainOperation();
    virtual ~ChangeBrainOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Detect and chase a target until reached o out of bound.
*   Chase updates periodically and turns, moving towards a certain
*   location.  This is normally used to chase a targeted player.
*/
class ChaseOperation : public MovementOperation
{
protected:
    /** @name Instance varaibles
     *  These parameters are local to one instance of the operation.
     */
    //@{
    EID              targetEID;              ///< The EID of the chased target
    float            offsetAngle;            ///< The actual offset angle in radians
    csVector3        offsetDelta;            ///< The actual delta relative to target
    float            adaptivVelScale;               ///< Scale of velocity for adaptive vel.
    MathScript* calcAdaptivVelocity; ///< This is the particular calculation for adaptiv velocity.
    //@}

    /** @name Operation Parameters
     *  These parameters are initialized by the Load function
     *  and copied by the MakeCopy function.
     */
    //@{
    int              type;                   ///< The type of chase to perform
    float            searchRange;            ///< Search for targets within this range
    float            chaseRange;             ///< Chase as long targets are within this range.
    ///< Chase forever if set to -1.
    csString         offsetAttribute;        ///< Used to stop a offset from the target.
    float            offsetAngleMax;         ///< The maximum offset angle in radians
    float            sideOffset;             ///< Add a offset to the side of the target
    bool             offsetRelativeHeading;  ///< Set to true will make the offset relative target heading
    csString         adaptivVelocityScript;  ///< Script to do adaptiv velocity adjustments.
    //@}

    enum
    {
        NEAREST_ACTOR,   ///< Sense Players and NPC's
        NEAREST_NPC,     ///< Sense only NPC's
        NEAREST_PLAYER,  ///< Sense only players
        OWNER,           ///< Sense only the owner
        TARGET           ///< Sense only target
    };
    static const char* typeStr[];

    /** Constructor for this operation, used by the MakeCopy.
     *
     *  This constructor will copy all the Operation Parameters
     *  from the other operation and initialize all Instance Variables
     *  to default values.
     */
    ChaseOperation(const ChaseOperation* other);

public:
    /** Constructor for this operation.
     *
     *  Construct this operation and initialzie with default
     *  Instance Variables. The Operation Parameters
     */
    ChaseOperation();

    /** Destructor for this operation
     */
    virtual ~ChaseOperation() {}

    /** Calculate any offset from target to stop chase.
     *
     *  The chase support several different modes. The default is that the
     *  chaser ends up at the top of the target. This function calculate the offset
     *  as a 3D vector that represents the distance from the target to the end
     *  point of the chase.
     */
    csVector3 CalculateOffsetDelta(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   const csVector3 &endPos, const iSector* endSector,
                                   float endRot) const;

    /** Calculate the end position for this chase.
     *
     *  Called by the Run funciton in the MovementOperation. Will calculate
     *  end posiiton for the chase including any offset deltas.
     */
    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector);

    /** Call to update the target to chase.
     *
     *  When called the chase target will be checked to se if a new target should be selected.
     *  This mostly apply to chases of type NEAREST_*
     */
    virtual gemNPCActor* UpdateChaseTarget(NPC* npc, const csVector3 &myPos, const iSector* mySector);

    /** Update end position for moving targets.
     *
     *  Called from advance in the MovementOperation. Will update the end position for
     *  targets that moves. This may include change of target as well.
     */
    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector);

    /** Load Operation Parameters from xml.
     *
     *  Load this operation from the given node. This will
     *  initialzie the Operation Parameters for this operation.
     */
    virtual bool Load(iDocumentNode* node);

    /**
     * Resolve the offset attribute.
     */
    virtual float GetOffset(NPC* npc) const;

    /** Return adapativ velocity adjustment.
     *
     *  Should return a value percent vise adjustment. Eg. 1.1 adjust velocity
     *  up with 10%.
     */
    float AdaptivVelocity(NPC* npc, float distance);

    /** Return the velocity adjusted by adaptivVelScale
     *
     */
    virtual float GetVelocity(NPC* npc);


    /** Make a deep copy of this operation.
     *
     *  MakeCopy will make a copy of all Operation Parameters and reset each
     *  Instance Variable.
     */
    virtual ScriptOperation* MakeCopy();
};


//-----------------------------------------------------------------------------

/**
* Will copy a locate.
*/
class CopyLocateOperation : public ScriptOperation
{
protected:
    csString source;
    csString destination;
    unsigned int flags;
public:

    CopyLocateOperation(): ScriptOperation("CopyLocate"), flags(NPC::LOCATION_ALL) {};
    virtual ~CopyLocateOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Let the NPC delete it self.
*/
class DeleteNPCOperation : public ScriptOperation
{
protected:
    /**
     * Constructor for this operation, used by the MakeCopy.
     *
     * This constructor will copy all the Operation Parameters
     * from the other operation and initialize all Instance Variables
     * to default values.
     */
    DeleteNPCOperation(const DeleteNPCOperation* other);

public:
    DeleteNPCOperation();
    virtual ~DeleteNPCOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Moving entails a velocity vector and an animation action.
*/
class MoveOperation : public ScriptOperation
{
protected:
    csString  action;
    float     duration;

    float     angle;

    // Instance temp variables. These dosn't need to be copied.
    float remaining;

    MoveOperation(const char*  n): ScriptOperation(n),remaining(0.0f)
    {
        duration = 0;
        ang_vel = 0;
        angle = 0;
    }
public:

    MoveOperation(): ScriptOperation("Move"),remaining(0.0f)
    {
        duration = 0;
        ang_vel = 0;
        angle = 0;
    }
    virtual ~MoveOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual void InterruptOperation(NPC* npc);

};

//-----------------------------------------------------------------------------

/**
* Turn on and off auto remembering of perceptions
*/
class AutoMemorizeOperation : public ScriptOperation
{
protected:
    csString types;
    bool     enable;
public:

    AutoMemorizeOperation(): ScriptOperation("AutoMemorize"), enable(false) { }
    virtual ~AutoMemorizeOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
};

//-----------------------------------------------------------------------------

/**
* Moving entails a circle with radius at a velocity and an animation action.
*/
class CircleOperation : public MoveOperation
{
protected:
    float radius;
public:

    CircleOperation(): MoveOperation("Circle")
    {
        radius = 0.0f;
    }
    virtual ~CircleOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
};

//-----------------------------------------------------------------------------

/**
* Control another actor.
*/
class ControlOperation : public ScriptOperation
{
protected:
    bool control;  // Should this operation take or release control
public:

    ControlOperation(bool control): ScriptOperation("Control"), control(control) { }
    virtual ~ControlOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
};

//-----------------------------------------------------------------------------

/**
* Debug will turn on and off debug for the npc. Used for debuging
*/
class DebugOperation : public ScriptOperation
{
protected:
    int      level;
    csString exclusive;

public:

    DebugOperation(): ScriptOperation("Debug"),level(0) {};
    virtual ~DebugOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Dequip will tell the npc to dequip a item
*/
class DequipOperation : public ScriptOperation
{
protected:
    csString slot;

public:

    DequipOperation(): ScriptOperation("Dequip") {};
    virtual ~DequipOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Work will make the NPC work for a resource.
 *
 *  This class is the implementation of the work operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples: <pre>
 *  \<work type="dig" resource="tribe:wealth" /\>
 *  \<work type="dig" resource="Gold Ore" /\> </pre>
 */
class WorkOperation : public ScriptOperation
{
protected:
    csString type;
    csString resource; ///< The name of the resource to work for.

public:

    WorkOperation(): ScriptOperation("Work") {};
    virtual ~WorkOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Drop will make the NPC drop whatever he is
* holding.
*/
class DropOperation : public ScriptOperation
{
protected:
    csString slot;

public:

    DropOperation(): ScriptOperation("Drop") {};
    virtual ~DropOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
 * Eat will take a bite of a nearby dead actor and add resource to tribe wealth.
 *
 * Examples: <pre>
 * \<eat resource="tribe:wealth" /\>
 * \<eat resource="Flesh" /\> </pre>
 */
class EatOperation : public ScriptOperation
{
protected:
    csString resource;

public:

    EatOperation(): ScriptOperation("Eat") {};
    virtual ~EatOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Emote will make the NPC show an emotion
 *
 *  This class is the implementation of the emote operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples: <pre>
 *  \<emote cmd="greet" /\> </pre>
 */
class EmoteOperation : public ScriptOperation
{
protected:
    csString cmd; ///< The emote command

public:

    EmoteOperation(): ScriptOperation("Emote") {};
    virtual ~EmoteOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Equip will tell the npc to equip a item
*
* Examples: <pre>
* \<equip item="Sword" slot="righthand" count="1" /\> </pre>
*/
class EquipOperation : public ScriptOperation
{
protected:
    csString item;
    csString slot;
    int      count; // Number of items to pick up from a stack

public:

    EquipOperation(): ScriptOperation("Equip"),count(0) {};
    virtual ~EquipOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Hate list modifications.
*/
class HateListOperation : public ScriptOperation
{
protected:
    /**
     * Flags definitions for use in the flags variable.
     */
    enum
    {
        MAX_HATE = 0x0001,
        MIN_HATE = 0x0002,
        ABS_HATE = 0x0004,
        DELTA_HATE = 0x0008,
        HATE_PERCEPTION = 0x0010  ///< Use last perception instead of target
    };

    unsigned int flags;           ///< Flags
    float        maxHate;         ///< Hate should have a max value if MAX_HATE is set
    float        minHate;         ///< Hate should bave a min value if MIN_HATE is set
    float        absoluteHate;    ///< Hate should be set to this value if ABS_HATE is set
    float        deltaHate;       ///< Added to hate if DELTA_HATE is set
public:

    HateListOperation(): ScriptOperation("HateList"), flags(0),maxHate(0.0),minHate(0.0),absoluteHate(0.0),deltaHate(0.0) {};
    virtual ~HateListOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};


//-----------------------------------------------------------------------------

/**
* Invisible will make the npc invisible.
*/
class InvisibleOperation : public ScriptOperation
{
public:

    InvisibleOperation(): ScriptOperation("Invisible") {};
    virtual ~InvisibleOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Locate is a very powerful function which will find
* the nearest object of the named tag, within a range.
*/
class LocateOperation : public ScriptOperation
{
protected:
    // Instance variables
    bool        staticLocated;
    NPC::Locate storedStaticLocated;

    // Operation parameters
    csString  object;
    float     range;
    bool      static_loc;
    bool      random;
    csString  locateOutsideRegion; ///< Locate targets outside a region if a region exist. Default false
    bool      locateInvisible;     ///< Locate invisible targets
    bool      locateInvincible;    ///< Locate invincible targets
    csString  destination;         ///< Alternate destination instead of "Active" locate.


public:
    LocateOperation();
    LocateOperation(const LocateOperation* other);
    virtual ~LocateOperation() { }

    /// Use -1 for located_range if range scheck sould be skipped.
    Waypoint* CalculateWaypoint(NPC* npc, csVector3 located_pos, iSector* located_sector, float located_range);

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
private:
};

//-----------------------------------------------------------------------------

/**
* LoopBegin operation will only print LoopBegin for debug purpose.
* Looping will be done by the LoopEndOperation.
*/
class LoopBeginOperation : public ScriptOperation
{
public:
    int iterations;

public:

    LoopBeginOperation(): ScriptOperation("BeginLoop")
    {
        iterations=0;
    }
    virtual ~LoopBeginOperation() { }

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* LoopEnd operation will jump back to the beginning
* of the loop.
*/
class LoopEndOperation : public ScriptOperation
{
protected:
    int loopback_op;
    int current;
    int iterations;

public:

    LoopEndOperation(int which,int iterations): ScriptOperation("LoopEnd")
    {
        loopback_op = which;
        this->iterations = iterations;
        current = 0;
    }
    virtual ~LoopEndOperation() { }
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Melee will tell the npc to attack the most hated
* entity within range.
*/
class MeleeOperation : public ScriptOperation
{
protected:
    float        seek_range, melee_range;
    gemNPCActor* attacked_ent;
    csString     attackOutsideRegion;  ///< Attack even outside region if a region is defined.
    bool         attackInvisible;
    bool         attackInvincible;
    csString     stance;
    csString     attackMostHatedTribeTarget; ///< Melee operation "tribe" attribute. When set to true the
    ///< melee operation will use the most hated of all the tribe
    ///< members when deciding target.
public:

    MeleeOperation(): ScriptOperation("Melee"),
        attackInvisible(false),attackInvincible(false)
    {
        attacked_ent=NULL;
        seek_range=0;
        melee_range=0;
    }
    virtual ~MeleeOperation() {}

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual void InterruptOperation(NPC* npc);
};

//-----------------------------------------------------------------------------

/**
* Memorize will make the npc to setup a spawn point here
*/
class MemorizeOperation : public ScriptOperation
{
protected:

public:

    MemorizeOperation(): ScriptOperation("Memorize") {};
    virtual ~MemorizeOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

// MoveOperation - Definition has been moved before the first user of this
//                 class as a base class.

//-----------------------------------------------------------------------------

/**
* MovePath specifies the name of a path and an animation action.
*/
class MovePathOperation : public ScriptOperation
{
protected:
    // Parameters
    csString           anim;
    csString           pathname;
    psPath::Direction  direction;

    // Internal variables
    psPath*            path;
    psPathAnchor*      anchor;

public:

    MovePathOperation(): ScriptOperation("MovePath"), direction(psPath::FORWARD), path(0), anchor(0) { }
    virtual ~MovePathOperation()
    {
        delete anchor;
    }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual void InterruptOperation(NPC* npc);
};

//-----------------------------------------------------------------------------

/**
* Moving to a spot entails a position vector, a linear velocity,
* and an animation action.
*/
class MoveToOperation : public MovementOperation
{
protected:
    csVector3 destPos;
    iSector*  destSector;

    csString  memoryCheck; ///< Used to keep memory name in case coordinates are taken from there
    csString  action;
public:
    MoveToOperation();
    MoveToOperation(const MoveToOperation* other);
    virtual ~MoveToOperation() { }

    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector);

    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector);

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Navigate moves the NPC to the position and orientation
* of the last located thing.  (See LocateOperation)
*/
class NavigateOperation : public MovementOperation
{
protected:
    csString action;
    bool     forceEndPosition;

    float     endAngle;    ///< The angle of the target
    iSector*  endSector;   ///< The sector of the target of the navigate
    csVector3 endPos;      ///< The end position of the target of the navigate

public:

    NavigateOperation();
protected:
    NavigateOperation(const NavigateOperation* other);
public:
    virtual ~NavigateOperation() {};

    virtual ScriptOperation* MakeCopy();

    virtual bool GetEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                csVector3 &endPos, iSector* &endSector);

    virtual bool UpdateEndPosition(NPC* npc, const csVector3 &myPos, const iSector* mySector,
                                   csVector3 &endPos, iSector* &endSector);

    virtual bool Load(iDocumentNode* node);
};

//-----------------------------------------------------------------------------

/** No Operation(NOP) Operation
 *
 */
class NOPOperation : public ScriptOperation
{
protected:
public:

    NOPOperation(): ScriptOperation("NOPE") {};
    virtual ~NOPOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Send a custon perception from a behavior script
 *
 * Will send the custom perception at the given time in the script.
 */
class PerceptOperation : public ScriptOperation
{
protected:
    enum TargetType
    {
        SELF,
        ALL,
        TRIBE,
        TARGET
    };

    class DelayedPerceptOperationGameEvent : public psGameEvent
    {
    protected:
        NPC*       npc;
        Perception pcpt;
        TargetType target;
        float      maxRange;

    public:
        DelayedPerceptOperationGameEvent(int offsetTicks, NPC* npc, Perception &pcpt, TargetType target, float maxRange);
        virtual void Trigger();  // Abstract event processing function
        virtual csString ToString() const;
    };

    csString   perception; ///< The perception name to send
    csString   type;       ///< The type value of the perception
    TargetType target;     ///< Hold the target for the perception, default SELF
    float      maxRange;   ///< Is there a max range for this, 0.0 is without limit
    csString   condition;  ///< A condition for when the perception should be fired
    csString   failedPerception; ///< A perception to fire if the condition fails
    csString   delayed;          ///< If set this perception should be delayed.

    MathScript* calcCondition; ///< This is the particular calculation for condition.
public:

    PerceptOperation() :
        ScriptOperation("Percept"),
        target(SELF),
        maxRange(0.0),
        calcCondition(0)
    {
    }
    virtual ~PerceptOperation() {}
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
    virtual bool CheckCondition(NPC* npc);
    static void TriggerEvent(NPC* npc, Perception &pcpt, TargetType target, float maxRange);
};

//-----------------------------------------------------------------------------

/**
* Pickup will tell the npc to pickup a nearby
* entity (or fake it).
*/
class PickupOperation : public ScriptOperation
{
protected:
    csString object;
    csString slot;
    int      count; // Number of items to pick up from a stack

public:

    PickupOperation(): ScriptOperation("Pickup"),count(0) {};
    virtual ~PickupOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Reproduce will make the npc to setup a spawn point here
*/
class ReproduceOperation : public ScriptOperation
{
protected:
    csString tribeMemberType;
public:

    ReproduceOperation(): ScriptOperation("Reproduce") {};
    virtual ~ReproduceOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Resurrect will make the npc to setup a spawn point here
*/
class ResurrectOperation : public ScriptOperation
{
protected:

public:

    ResurrectOperation(): ScriptOperation("Resurrect") {};
    virtual ~ResurrectOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Implement the reward NPC script operation.
 *
 * Reward will add a given resource to the Tribe that the
 * current NPC is a member of.
 *
 * Examples: <pre>
 * \<reward resource="Gold Ore" count="2" /\>
 * \<reward resource="tribe:wealth" /\> </pre>
 */
class RewardOperation : public ScriptOperation
{
protected:
    csString resource; ///< The Resource to be rewarded to the tribe.
    int count;         ///< The number of the resource to reward to the tribe.

public:

    RewardOperation(): ScriptOperation("Reward"), count(0) {};
    virtual ~RewardOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Rotating requires storing or determining the angle to
* rotate to, and the animation action.
*/
class RotateOperation : public ScriptOperation
{
protected:
    enum
    {
        ROT_UNKNOWN,
        ROT_ABSOLUTE,               // Rotate to this world angle
        ROT_RELATIVE,               // Rotate delta angle from current npd heading
        ROT_TARGET,                 // Rotate to face target
        ROT_LOCATE_DESTINATION,     // Rotate to face located destination
        ROT_LOCATE_ROTATION,        // Rotate to face located rotatation
        ROT_RANDOM,                 // Rotate a random angle
        ROT_REGION,                 // Rotate to an angle within the region
        ROT_TRIBE_HOME              // Rotate to an angle within tribe home
    };
    int       op_type;              // Type of rotation. See enum above.
    float     min_range, max_range; // Min,Max values for random and region rotation
    float     delta_angle;          // Value to rotate for relative rotation

    float     target_angle;         // Calculated end rotation for every rotation and
    // input to absolute rotation
    float     angle_delta;          // Calculated angle that is needed to rotate to target_angle

    csString  action;               // Animation to use in the rotation

    // Instance temp variables. These dosn't need to be copied.
    float remaining;
public:

    RotateOperation(): ScriptOperation("Rotate")
    {
        vel=0;
        ang_vel=999;
        op_type=ROT_UNKNOWN;
        min_range=0;
        max_range=0;
        delta_angle=0;
        target_angle=0;
        angle_delta=0;
        remaining = 0.0;
    }
    virtual ~RotateOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual void InterruptOperation(NPC* npc);

    float SeekAngle(NPC* npc, float targetYRot);           // Finds an angle which won't lead to a collision
};

//-----------------------------------------------------------------------------

/**
* Vel set the velocity source of the brain to run or walk.
*/
class VelSourceOperation : public ScriptOperation
{
protected:
public:

    VelSourceOperation(): ScriptOperation("VelSource")
    {
    }
    virtual ~VelSourceOperation() { }
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
    virtual OperationResult Run(NPC* npc,bool interrupted);
};

//-----------------------------------------------------------------------------

/** Script will make the progression script run at server
 *
 *  This class is the implementation of the script operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples: <pre>
 *  \<script name="my_script" /\> </pre>
 */
class ProgressScriptOperation : public ScriptOperation
{
protected:
    csString scriptName; ///< The name of the script to run

    // Instance temp variables. These dosn't need to be copied.

public:

    ProgressScriptOperation(): ScriptOperation("Script") {};
    virtual ~ProgressScriptOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Sequence will control a named sequence in the world.
*/
class SequenceOperation : public ScriptOperation
{
protected:
    enum // Sequence commands, should use same values as in the psSequenceMessage
    {
        UNKNOWN = 0,
        START = 1,
        STOP = 2,
        LOOP = 3
    };

    csString sequenceName;
    int      cmd;    // See enum above
    int      count;  // Number of times to run the sequence

public:

    SequenceOperation(): ScriptOperation("Sequence"),cmd(UNKNOWN),count(0) {};
    virtual ~SequenceOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* SetBuffer will set a buffer for tribe or npc.
*/
class SetBufferOperation : public ScriptOperation
{
protected:
    enum // Sequence commands, should use same values as in the psSequenceMessage
    {
        NPC_BUFFER = 0,
        TRIBE_BUFFER = 1
    };

    csString buffer;
    csString value;
    int      type;

public:

    SetBufferOperation(): ScriptOperation("SetBuffer"), type(NPC_BUFFER) {};
    virtual ~SetBufferOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* ShareMemories will make the npc share memoreis with tribe
*/
class ShareMemoriesOperation : public ScriptOperation
{
protected:

public:

    ShareMemoriesOperation(): ScriptOperation("ShareMemories") {};
    virtual ~ShareMemoriesOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** Sit will make the NPC sit or stand
 *
 *  This class is the implementation of the sit operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples: <pre>
 *  \<sit /\>
 *  \<standup /\> </pre>
 */
class SitOperation : public ScriptOperation
{
protected:
    bool sit; ///< True if sit false for stand

    // Instance temp variables. These dosn't need to be copied.
    float remaining;

public:

    SitOperation(bool sit): ScriptOperation("Sit"), sit(sit), remaining(0.0) {};
    virtual ~SitOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Talk will tell the npc to communicate to a nearby
* entity.
*/
class TalkOperation : public ScriptOperation
{
protected:
    typedef psNPCCommandsMessage::PerceptionTalkType TalkType;

    csString talkText;   ///< The text to send to the clients
    TalkType talkType;   ///< What kind of talk, default say
    bool     talkPublic; ///< Should this be public or only to the target
    bool     target;     ///< True if this is should go to the target of the NPC.
    csString command;    ///< Command to percept to target if target is a NPC.

public:

    TalkOperation(): ScriptOperation("Talk"),talkType(psNPCCommandsMessage::TALK_SAY),talkPublic(false),target(false) {};
    virtual ~TalkOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Teleport will teleport the NPC to the target position.
*/
class TeleportOperation : public ScriptOperation
{
protected:

public:

    TeleportOperation(): ScriptOperation("Teleport") {};
    virtual ~TeleportOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Transfer will transfer a item from the NPC to a target. The
* target might be a tribe.
*/
class TransferOperation : public ScriptOperation
{
protected:
    csString item;
    int count;
    csString target;

public:

    TransferOperation(): ScriptOperation("Transfer"), count(0) {};
    virtual ~TransferOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* TribeHome will make the npc to setup a spawn point here
*/
class TribeHomeOperation : public ScriptOperation
{
protected:

public:

    TribeHomeOperation(): ScriptOperation("Tribe_Home") {};
    virtual ~TribeHomeOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* TribeType will change the need set used by the npc.
*/
class TribeTypeOperation : public ScriptOperation
{
protected:
    csString tribeType;
public:

    TribeTypeOperation(): ScriptOperation("Tribe_Type") {};
    virtual ~TribeTypeOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Visible will make the npc visible.
*/
class VisibleOperation : public ScriptOperation
{
public:

    VisibleOperation(): ScriptOperation("Visible") {};
    virtual ~VisibleOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
* Wait will simply set the mesh animation to
* something and sit there for the desired number
* of seconds.
*/
class WaitOperation : public ScriptOperation
{
protected:
    csString duration;  ///< The duration to wait before continue
    csString random;    ///< A random duration to add to the duration
    csString action;

    // Instance temp variables. These dosn't need to be copied.
    float remaining;

public:

    WaitOperation(): ScriptOperation("Wait"),remaining(0.0f)
    {
        duration=0;
    }
    virtual ~WaitOperation() { }

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/**
 * Wander auto-navigates randomly between a network of waypoints specified
 * in the DB.
 */
class WanderOperation : public ScriptOperation
{
protected:
    ////////////////////////////////////////////////////////////
    // Start of instance temp variables. These dosn't need to be copied.

    class WanderRouteFilter : public psPathNetwork::RouteFilter
    {
    public:
        WanderRouteFilter(WanderOperation*  parent):parent(parent) {};
        virtual bool Filter(const Waypoint* waypoint) const;
    protected:
        WanderOperation*  parent;
    };

    WanderRouteFilter wanderRouteFilter;

    csList<Edge*> edgeList;

    csList<Edge*>::Iterator   edgeIterator;
    Edge*                     currentEdge;              ///< The current edge
    Edge::Iterator*           currentPathPointIterator; ///< The current iterator for path points
    psPathPoint*              currentPathPoint;         ///< The current local destination
    csVector3                 currentPointOffset;       ///< The offset used for current local dest.
    float                     currentDistance;          ///< The distance to the current local destination
//    psPath::PathPointIterator pointIterator;

    // End of instance temp variables.
    ////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////
    // Start of operation parameters
    csString  action;
    bool      random;
    bool      undergroundValid,underground;
    bool      underwaterValid,underwater;
    bool      privValid,priv;
    bool      pubValid,pub;
    bool      cityValid,city;
    bool      indoorValid,indoor;
    bool      pathValid,path;
    bool      roadValid,road;
    bool      groundValid,ground;
    // End of operation parameters
    ////////////////////////////////////////////////////////////

    /** Start move to point.
     *
     *  Set up a movement from current position to next local dest point.
     */
    bool StartMoveTo(NPC* npc, psPathPoint* point);

    /** Teleport to point.
     *
     *  Teleport from current position to next local dest point.
     */
    bool MoveTo(NPC* npc, psPathPoint* point);

    /** Calculate the edgeList to be used by wander.
     *
     *  Will calculate a list of edges between start and end for wander.
     *  For random wander the list is empty but still returning true.
     */
    OperationResult CalculateEdgeList(NPC* npc);


    /** Set the current path point iterator.
     */
    void SetPathPointIterator(Edge::Iterator* iterator);

public:

    WanderOperation();
    WanderOperation(const WanderOperation* other);
    virtual ~WanderOperation();

    virtual ScriptOperation* MakeCopy();


    /** Clear list of edges
     */
    void EdgeListClear()
    {
        edgeList.DeleteAll();
    }

    /** Get the next edge for local destination
     *
     *  Will return the next edge from the edge list or
     *  find a new radom edge for random wander.
     */
    Edge* GetNextEdge(NPC* npc);


    /** Get the next path point to use for local destination
     *
     *  Will return the next path point from the active edge.
     *  It will get next edge when at end of path. This include
     *  random edge if operation is random.
     */
    psPathPoint* GetNextPathPoint(NPC* npc, bool &teleport);

    /** Return the current path point
     *
     */
    psPathPoint* GetCurrentPathPoint(NPC* npc);


    /* Utility function to calcualte distance from npc to destPoint.
     */
    static float DistanceToDestPoint(NPC* npc, const csVector3 &destPos, const iSector* destSector);


    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual void InterruptOperation(NPC* npc);
    virtual bool Load(iDocumentNode* node);
};

//-----------------------------------------------------------------------------

/**
* Watch operation will tell if the targt goes out of range.
*/
class WatchOperation : public ScriptOperation
{
protected:
    float     watchRange;
    int       type;
    float     searchRange;       ///<  Used for watch of type NEAREST_*
    bool      watchInvisible;
    bool      watchInvincible;

    csWeakRef<gemNPCObject> watchedEnt;

    enum
    {
        NEAREST_ACTOR,   ///< Sense Players and NPC's
        NEAREST_NPC,     ///< Sense only NPC's
        NEAREST_PLAYER,  ///< Sense only players
        OWNER,
        TARGET
    };
    static const char*  typeStr[];

public:

    WatchOperation():ScriptOperation("Watch"),watchRange(0.0f),
        type(TARGET),searchRange(0.0f),watchInvisible(false),watchInvincible(false)
    {
        watchedEnt=NULL;
    }
    virtual ~WatchOperation() {}

    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();

    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual OperationResult Advance(float timedelta,NPC* npc);
    virtual void InterruptOperation(NPC* npc);

private:
    bool OutOfRange(NPC* npc);
};

//-----------------------------------------------------------------------------

/** Loot will make the NPC loot specified items
 *
 *  This class is the implementation of the loot operations
 *  used in behavior scripts for NPCS.
 *
 *  Examples: <pre>
 *  \<loot type="all"     /\>
 *  \<loot type="weapons" /\> </pre>
 */
class LootOperation : public ScriptOperation
{
protected:
    csString type;     ///< Type of items to loot

public:

    LootOperation(): ScriptOperation("Loot") {};
    virtual ~LootOperation() {};
    virtual OperationResult Run(NPC* npc,bool interrupted);
    virtual bool Load(iDocumentNode* node);
    virtual ScriptOperation* MakeCopy();
};

//-----------------------------------------------------------------------------

/** @} */

#endif
