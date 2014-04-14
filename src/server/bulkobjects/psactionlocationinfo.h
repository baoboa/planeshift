/*
* psactionlocationinfo.h
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits :
*            Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : Container for the action_locations db table.
*
*/
#ifndef __PSACTIONLOCATION_H__
#define __PSACTIONLOCATION_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/poolallocator.h"
#include "util/psconst.h"

#include <idal.h>

//=============================================================================
// Local Includes
//=============================================================================

class CacheManager;
class gemActionLocation;
class gemItem;
struct iDocumentNode;
class MathExpression;

/**
 * \addtogroup bulkobjects
 * @{ */

/**
 * This huge class stores all the properties of any object
 * a player can have in the game.  All stats, bonuses, maluses,
 * magic stat alterations, combat properties, spell effects,
 * and so forth are all stored here.
 */
class psActionLocation
{
public:
    typedef enum
    {
        TRIGGERTYPE_NONE,   /// Default initial value, should not exist after loaded.
        TRIGGERTYPE_SELECT,
        TRIGGERTYPE_PROXIMITY
    } TriggerType;

    static const char* TriggerTypeStr[];

    psActionLocation();
    ~psActionLocation();

    bool Load(iResultRow &row);
    bool Load(csRef<iDocumentNode> root);
    bool Save();
    bool Delete();
    csString ToXML() const;

    int IsMatch(psActionLocation* compare);

    void SetGemObject(gemActionLocation* gemAction);
    gemActionLocation* GetGemObject(void);

    gemItem* GetRealItem();

    const csVector3 &GetPosition()
    {
        return position;
    }
    const csString &GetSectorName()
    {
        return sectorname;
    }
    //void SetLocationInWorld(psSectorInfo *sectorinfo,float loc_x,float loc_y,float loc_z,float loc_yrot);
    void Send(int clientnum);

    // Returns the result of the XML parsing of the action lcoation reponse string and
    //  the setting of the action location member variables
    bool ParseResponse();

    /// Returns instance ID of referenced in action location response string
    ///  This is either a container ID or a lock ID
    InstanceID GetInstanceID() const
    {
        return instanceID;
    }
    void SetInstanceID(InstanceID newID)
    {
        instanceID = newID;
    }

    /// Returns the enter script in entrance action location response string
    MathExpression* GetEnterScript() const
    {
        return enterScript;
    }

    /// Returns the entrance type in entrance action location response string
    const csString &GetEntranceType() const
    {
        return entranceType;
    }
    void SetEntranceType(const csString &newType)
    {
        entranceType = newType;
    }

    /// Returns true if this action location is a minigame board
    bool IsGameBoard() const
    {
        return isGameBoard;
    }

    /// Returns true if this action location will run a script and can be examined
    bool IsExamineScript() const
    {
        return isExamineScript;
    }

    /// Returns true if this action location is an entrance
    bool IsEntrance() const
    {
        return isEntrance;
    }
    void SetIsEntrance(bool flag)
    {
        isEntrance = flag;
    }

    /// Returns true if this action location is an lockable entrance
    bool IsLockable() const
    {
        return isLockable;
    }
    void SetIsLockable(bool flag)
    {
        isLockable = flag;
    }

    /// Returns true if this action location is a container
    bool IsContainer() const
    {
        return isContainer;
    }

    /// Returns true if this action location has return tag
    bool IsReturn() const
    {
        return isReturn;
    }

    /// Returns true if this action location is actaive
    bool IsActive() const
    {
        return isActive;
    }
    void SetActive(bool flag)
    {
        isActive = flag;
    }

    /// Returns action location description
    const csString &GetDescription() const
    {
        return description;
    }
    void SetDescription(const csString &newDescription)
    {
        description = newDescription;
    }

    /// Returns or sets entrance location memebers
    csVector3 GetEntrancePosition() const
    {
        return entrancePosition;
    }
    void SetEntrancePosition(csVector3 newPosition)
    {
        entrancePosition = newPosition;
    }
    float GetEntranceRotation() const
    {
        return entranceRot;
    }
    void SetEntranceRotation(float newRot)
    {
        entranceRot = newRot;
    }
    const csString &GetEntranceSector() const
    {
        return entranceSector;
    }
    void SetEntranceSector(const csString &newSector)
    {
        entranceSector = newSector;
    }
    ///< Get entrace Instance in the world if not defined number of the action location
    const InstanceID GetEntranceInstance() const
    {
        return entranceInstance;
    }
    ///< Set entrace Instance in the world if not defined number of the action location
    void SetEntranceInstance(const InstanceID newInstance)
    {
        entranceInstance = newInstance;
    }

    /// Returns or sets return location memebers
    csVector3 GetReturnPosition() const
    {
        return returnPosition;
    }
    void SetReturnPosition(csVector3 newPosition)
    {
        returnPosition = newPosition;
    }
    float GetReturnRotation() const
    {
        return returnRot;
    }
    void SetReturnRotation(float newRot)
    {
        returnRot = newRot;
    }
    const csString &GetReturnSector() const
    {
        return returnSector;
    }
    void SetReturnSector(const csString &newSector)
    {
        returnSector = newSector;
    }
    ///< Get return Instance in the world if not defined 0
    const InstanceID GetReturnInstance() const
    {
        return returnInstance;
    }
    ///< Set return Instance in the world if not defined 0
    void SetReturnInstance(const InstanceID newInstance)
    {
        returnInstance = newInstance;
    }

    /// Sets
    void SetName(const csString &newname)
    {
        name = newname;
    }
    void SetSectorName(const csString &newsector)
    {
        sectorname = newsector;
    }
    void SetMeshName(const csString &newmeshname)
    {
        meshname = newmeshname;
    }
    void SetTriggerType(const TriggerType &newtrigger)
    {
        triggertype = newtrigger;
    }
    TriggerType GetTriggerType() const
    {
        return triggertype;
    }
    const char* GetTriggerTypeAsString() const
    {
        return TriggerTypeStr[triggertype];
    }
    void SetResponseType(const csString &newresponsetype)
    {
        responsetype = newresponsetype;
    }
    void SetResponse(const csString &newresponse)
    {
        response = newresponse;
    }
    void SetPosition(csVector3 newposition)
    {
        position = newposition;
    }

    /** Sets the instance in the world of this action location, default INSTANCE_ALL
     *  @param instance The instance where this action location will be located
     */
    void SetInstance(const InstanceID instance)
    {
        pos_instance = instance;
    }
    InstanceID GetInstance()
    {
        return pos_instance;
    }
    void SetRadius(float newradius)
    {
        radius = newradius;
    }

    ///  The new operator is overriden to call PoolAllocator template functions
    void* operator new(size_t);
    ///  The delete operator is overriden to call PoolAllocator template functions
    void operator delete(void*);

    /// Get script to run
    csString GetScriptToRun()
    {
        return scriptToRun;
    }
    csString GetScriptParameters()
    {
        return scriptParameters;
    }

    uint32 id;
    size_t master_id;

    csString name;
    csString sectorname; ///< Sector Where item is located
    csString meshname; ///< Mesh name
    csString polygon; ///< Required if multiple mesh of same name in area
    csVector3 position; ///< x,y,z coordinates required for entrances
    InstanceID pos_instance; ///< The instance from where this action location will be accesible.
    float radius;
    TriggerType triggertype;
    csString responsetype;
    csString response;

    gemActionLocation* gemAction;

private:
    /// Static reference to the pool for all psItem objects
    static PoolAllocator<psActionLocation> actionpool;

    /**
     * Sets up member variables for different response strings.
     */
    void SetupEntrance(csRef<iDocumentNode> entranceNode);
    void SetupReturn(csRef<iDocumentNode> returnNode);
    void SetupContainer(csRef<iDocumentNode> containerNode);
    void SetupGameboard(csRef<iDocumentNode> boardNode);
    void SetupScript(csRef<iDocumentNode> scriptNode);
    void SetupDescription(csRef<iDocumentNode> descriptionNode);

    /// Flag indicating that this action location is a container
    bool isContainer;

    /// Flag indicating that this action location is a minigame
    bool isGameBoard;

    /// Flag indicating that this action location is an entrance
    bool isEntrance;

    /// Flag indicating that this action location is lockable
    bool isLockable;

    /// Flag indicator that location will display menu
    bool isActive;

    /// Flag indicator that location has return tag
    bool isReturn;

    /// Flag indicator that location will run a script and can be examined
    bool isExamineScript;

    /// Parameters used by the script to execute
    csString scriptToRun;
    csString scriptParameters;

    ///  This is either a container ID or a lock ID
    InstanceID instanceID;

    /// String containing the entrance type
    csString entranceType;

    // Position stuff for entrances
    csVector3 entrancePosition;
    float entranceRot;
    csString entranceSector;
    InstanceID entranceInstance;

    /// String containing the entrance script
    ///   Script return value of 0 indicates no entry
    MathExpression* enterScript;

    // Possition stuff for returns
    csVector3 returnPosition;
    float returnRot;
    csString returnSector;
    InstanceID returnInstance;


    /// String containing response description
    csString description;


    //DB Helper Functions
    unsigned int Insert(const char* table, const char** fieldnames, psStringArray &fieldvalues);
    bool         UpdateByKey(const char* table, const char* idname, const char* idvalue, const char** fieldnames, psStringArray &fieldvalues);
    bool         DeleteByKey(const char* table, const char* idname, const char* idvalue);
};

/** @} */

#endif

