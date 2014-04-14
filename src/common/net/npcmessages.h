/*
* npcmessages.h
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
#ifndef __NPCMESSAGES_H__
#define __NPCMESSAGES_H__

#include "net/message.h"
#include "net/messages.h"
#include <csutil/csstring.h>
#include <csutil/databuf.h>
#include <csgeom/vector3.h>
#include "util/psstring.h"


// Forward declarations
class Location;
class LocationType;
class Waypoint;
class WaypointAlias;
class psPath;
class psPathPoint;
struct iSector;

/**
 * \addtogroup messages
 * @{ */

/**
* The message sent from superclient to server on login.
*/
class psNPCAuthenticationMessage : public psAuthenticationMessage
{
public:

    /**
    * This function creates a PS Message struct given a userid and
    * password to send out. This would be used for outgoing, new message
    * creation when a superclient wants to log in.
    */
    psNPCAuthenticationMessage(uint32_t clientnum,
        const char *userid,
        const char *password);

    /**
    * This constructor receives a PS Message struct and cracks it apart
    * to provide more easily usable fields.  It is intended for use on
    * incoming messages.
    */
    psNPCAuthenticationMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    bool NetVersionOk();
};

/**
* The message sent from server to superclient after successful
* login.  It is an XML message of all the basic info needed for
* all npc's managed by this particular superclient based on the login.
*/
class psNPCListMessage : public psMessageCracker
{
public:

    psString xml;

    /// Create psMessageBytes struct for outbound use
    psNPCListMessage(uint32_t clientToken,int size);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCListMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* The message sent from superclient to server after
* receiving all entities.  It activates the NPCs on the server.
*/
class psNPCReadyMessage : public psMessageCracker
{
public:
    /// Create psMessageBytes struct for outbound use
	psNPCReadyMessage();

    /// Crack incoming psMessageBytes struct for inbound use
	psNPCReadyMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* The 2nd message sent from server to superclient after successful
* login.  It is a list of strings, each string with the name
* of a CS world map zip file to load.
*/
class psMapListMessage : public psMessageCracker
{
public:

    csArray<csString> map;

    /// Create psMessageBytes struct for outbound use
    psMapListMessage(uint32_t clientToken,csString& str);

    /// Crack incoming psMessageBytes struct for inbound use
    psMapListMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* The 3rd message sent from server to superclient after successful
* login.  It is a list of races and its properties.
*/
class psNPCRaceListMessage : public psMessageCracker
{
public:

    typedef struct
    {
        csString  name;      ///< On average the name should not exceed 100 characters
        float     walkSpeed;
        float     runSpeed;
        csVector3 size;
        float     scale;     ///< The scale override of this race.
    } NPCRaceInfo_t;
    

    csArray<NPCRaceInfo_t> raceInfo;

    /// Create psMessageBytes struct for outbound use
    psNPCRaceListMessage(uint32_t clientToken,int count);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCRaceListMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Add Race Info to the message
     */
    void AddRace(csString& name, float walkSpeed, float runSpeed, const csVector3& size, float scale, bool last);

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* The message sent from server to superclient after successful
* login.  It is an XML message of all the basic info needed for
* all npc's managed by this particular superclient based on the login.
*/
class psNPCCommandsMessage : public psMessageCracker
{
public:

    enum Flags
    {
        NONE        = 0,
        INVISIBLE   = 1 << 0,
        INVINCIBLE  = 1 << 1,
        IS_ALIVE    = 1 << 2
    };

    enum PerceptionType
    {
        // Commands go from superclient to server
        CMD_TERMINATOR,
        CMD_ASSESS,
        CMD_ATTACK,
        CMD_BUSY,
        CMD_CAST,
        CMD_DELETE_NPC,
        CMD_DEQUIP,
        CMD_DRDATA,
        CMD_DROP,
        CMD_EMOTE,
        CMD_EQUIP,
        CMD_TEMPORARILY_IMPERVIOUS,
        CMD_INFO_REPLY,
        CMD_PICKUP,
        CMD_RESURRECT,
        CMD_SEQUENCE,
        CMD_SCRIPT,             // used for superclient to request a progress script to be run at server
        CMD_SIT,
        CMD_SPAWN,
        CMD_SPAWN_BUILDING,     // used for superclient to request a new building
        CMD_TALK,
        CMD_TRANSFER,
        CMD_UNBUILD,            // used for superclient to request a building to be tearn down
        CMD_VISIBILITY,
        CMD_WORK,
        CMD_CONTROL,
        CMD_LOOT,
        // Perceptions go from server to superclient
        PCPT_ANYRANGEPLAYER,    
        PCPT_ASSESS, 
        PCPT_ATTACK,
        PCPT_CHANGE_BRAIN,      ///< Command to superclient used to change the brain of a npc.
        PCPT_DEATH,
        PCPT_DEBUG_NPC,         ///< Command the superclient to change the debug level of a npc.
        PCPT_DEBUG_TRIBE,       ///< Command the superclient to change the debug level of a tribe.
        PCPT_DMG,
        PCPT_FAILED_TO_ATTACK,
        PCPT_FLAG,
        PCPT_GROUPATTACK,
        PCPT_INFO_REQUEST,      // Command to superclient, not a perception
        PCPT_INVENTORY,
        PCPT_LONGRANGEPLAYER,
        PCPT_NPCCMD,
        PCPT_OWNER_ACTION,
        PCPT_OWNER_CMD,
        PCPT_PERCEPT,
        PCPT_SHORTRANGEPLAYER,
        PCPT_SPAWNED,
        PCPT_SPELL,
        PCPT_STAT_DR,        
        PCPT_SPOKEN_TO,
        PCPT_TALK,   
        PCPT_TELEPORT,
        PCPT_TRANSFER,
        PCPT_VERYSHORTRANGEPLAYER,
        PCPT_CHANGE_OWNER,      // Command to superclient, not a perception
    };

    enum PerceptionTalkType
    {
        TALK_SAY,
        TALK_ME,
        TALK_MY,
        TALK_NARRATE
    };

    /// Create psMessageBytes struct for outbound use
    psNPCCommandsMessage(uint32_t clientToken, int size);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCCommandsMessage(MsgEntry *message);

    PSF_DECLARE_MSG_FACTORY();
    
     /**
     * Convert the message into human readable string.
     *
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};


//helpers for message splitting
#define ALLENTITYPOS_SIZE_PER_ENTITY (4 * sizeof(float) + sizeof(bool) + sizeof(uint32_t) + 100*sizeof(char))
#define ALLENTITYPOS_MAX_AMOUNT  (MAX_MESSAGE_SIZE-2)/ALLENTITYPOS_SIZE_PER_ENTITY

/**
* The message sent from server to superclient every 2.5 seconds.
* This message is the positions (and sectors) of every person
* in the game.
*/
class psAllEntityPosMessage: public psMessageCracker
{
public:
    /// Hold the number of entity positions after the message is cracked
    int count;

    /// Create psMessageBytes struct for outbound use
    psAllEntityPosMessage() { count = 0; msg=NULL; }

    /// Crack incoming psMessageBytes struct for inbound use
    psAllEntityPosMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);

    /// Sets the max size of the buffer
    void SetLength(int size,int client);

    /// Add a new entity's position to the data buffer
    void Add(EID id, csVector3 & pos, iSector* & sector, InstanceID instance, csStringSet* msgstrings, bool forced = false);

    /// Get the next entity and position from the buffer
    EID Get(csVector3 & pos, iSector* & sector, InstanceID & instance, bool &forced, csStringSet* msgstrings,
        csStringHashReversible* msgstringshash, iEngine* engine);
};

/**
* The message sent from server to superclient upon a successful
* work done.
*/
class psNPCWorkDoneMessage : public psMessageCracker
{
public:
    EID         npcEID;
    const char* resource;
    const char* nick;

    psNPCWorkDoneMessage(uint32_t clientToken, EID npcEID, const char* resource, const char* nick);
    
    psNPCWorkDoneMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();
    
    virtual csString ToString(NetBase::AccessPointers* accessPointers);
};

/**
* The message sent from server to superclient after successful
* NPC Creation.  
*/
class psNewNPCCreatedMessage : public psMessageCracker
{
public:
    PID new_npc_id, master_id, owner_id;

    /// Create psMessageBytes struct for outbound use
    psNewNPCCreatedMessage(uint32_t clientToken, PID new_npc_id, PID master_id, PID owner_id);

    /// Crack incoming psMessageBytes struct for inbound use
    psNewNPCCreatedMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* The message sent from server to superclient when a NPC
* is deleted from the server.
*/
class psNPCDeletedMessage : public psMessageCracker
{
public:
    PID npc_id;

    /// Create psMessageBytes struct for outbound use
    psNPCDeletedMessage(uint32_t clientToken, PID npc_id);

    /// Crack incoming psMessageBytes struct for inbound use
    psNPCDeletedMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* The message sent from client to server to control
* the players pet.
*/
class psPETCommandMessage : public psMessageCracker
{
public:

    typedef enum 
    {
        CMD_FOLLOW,
        CMD_STAY,
        CMD_DISMISS,
        CMD_SUMMON,
        CMD_ATTACK,
        CMD_GUARD,
        CMD_ASSIST,
        CMD_STOPATTACK,
        CMD_NAME,
        CMD_TARGET,
        CMD_RUN,
        CMD_WALK,
        CMD_LAST // LAST command, insert new commands before this
    } PetCommand_t;

    static const char *petCommandString[];

    int command;
    psString target;
    psString options;

    /// Create psMessageBytes struct for outbound use
    psPETCommandMessage(uint32_t clientToken, int cmd, const char * target, const char * options);

    /// Crack incoming psMessageBytes struct for inbound use
    psPETCommandMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers * accessPointers);
};

/**
* Server command message is for using npcclient as a remote command
* and debugging console for psserver.
*/
class psServerCommandMessage : public psMessageCracker
{
public:
    
    psString command;

    /// Create psMessageBytes struct for outbound use
    psServerCommandMessage(uint32_t clientToken, const char * target);

    /// Crack incoming psMessageBytes struct for inbound use
    psServerCommandMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers* accessPointers);
};

/** Handle PathNetwork changes from server to superclient.
*/
class psPathNetworkMessage : public psMessageCracker
{
public:

    typedef enum 
    {
        PATH_ADD_POINT,
        PATH_CREATE,
        PATH_RENAME,
        PATH_REMOVE_POINT,
        PATH_SET_FLAG,
        POINT_ADJUSTED,
        WAYPOINT_ADJUSTED,
        WAYPOINT_ALIAS_ADD,
        WAYPOINT_ALIAS_REMOVE,
        WAYPOINT_ALIAS_ROTATION_ANGLE,
        WAYPOINT_CREATE,
        WAYPOINT_RADIUS,
        WAYPOINT_RENAME,
        WAYPOINT_SET_FLAG
    } Command;

    Command   command;    ///< 
    uint32_t  id;         ///< ID of waypoint/point/segment
    csVector3 position;   ///< The position for new or adjusted elements
    iSector*  sector;     ///< The sector for new or adjusted elements
    csString  string;     ///< The string for add alias/flag set
    bool      enable;     ///< Enable or disable flags
    float     radius;     ///< The radius
    csString  flags;      ///< String with flags
    uint32_t  startId;
    uint32_t  stopId;
    uint32_t  secondId;
    float     rotationAngle; ///< The rotation angle for aliases
    uint32_t  aliasID;    ///< The id of aliases
    
    // Create psMessageBytes struct for outbound use
    
    /** Generic message to do a command where input is a path.
     */
    psPathNetworkMessage(Command command, const psPath* path);

    /** Generic message to do a command where input is a point for a path.
     */
    psPathNetworkMessage(Command command, const psPath* path, const psPathPoint* point);

    /** Generic message to do a command where input is a point for a path.
     */
    psPathNetworkMessage(Command command, const psPath* path, int secondId);
    
    
    /** Generic message to do a command where input is a string and a bool for a path.
     */
    psPathNetworkMessage(Command command, const psPath* path, const csString& string, bool enable);

    /** Generic message to do a command where input is a path point.
     */
    psPathNetworkMessage(Command command, const psPathPoint* point);

    /** Generic message to do a command where input is a waypoint.
     */
    psPathNetworkMessage(Command command, const Waypoint* waypoint);

    /** Generic message to do a command where input is a waypoint and an alias.
     */
    psPathNetworkMessage(Command command, const Waypoint* waypoint, const WaypointAlias* alias);

    /** Generic message to do a command where input is a waypoint and an string.
     */
    psPathNetworkMessage(Command command, const Waypoint* waypoint, const csString& string);

    /** Generic message to do a command where input is a string for a waypoint.
     */
    psPathNetworkMessage(Command command, const Waypoint* waypoint, const csString& string, bool enable);


    /// Crack incoming psMessageBytes struct for inbound use
    psPathNetworkMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers* accessPointers);
};

/** Handle Location changes from server to superclient.
*/
class psLocationMessage : public psMessageCracker
{
public:

    typedef enum 
    {
        LOCATION_ADJUSTED,
        LOCATION_CREATED,
        LOCATION_DELETED,
        LOCATION_INSERTED,
        LOCATION_RADIUS,
        LOCATION_RENAME,
        LOCATION_SET_FLAG,
        LOCATION_TYPE_ADD,
        LOCATION_TYPE_REMOVE
    } Command;

    Command   command;       ///< 
    uint32_t  id;            ///< ID of the location/location type
    csVector3 position;      ///< The position for new or adjusted elements
    iSector*  sector;        ///< The sector for new or adjusted elements
    bool      enable;        ///< Enable or disable flags
    float     radius;        ///< The radius
    float     rotationAngle; ///< The rotation angle for this element.
    csString  flags;         ///< String with flags
    csString  typeName;      ///< Type name
    csString  name;          ///< Name
    uint32_t  prevID;        ///< ID of previous point in a region.
    
    // Create psMessageBytes struct for outbound use
    
    /** Generic message to do a command where input is a Location.
     */
    psLocationMessage(Command command, const Location* location);

    /** Generic message to do a command where input is a Location type.
     */
    psLocationMessage(Command command, const LocationType* locationType);
    
    /** Generic message to do a command where input is a string.
     */
    psLocationMessage(Command command, const csString& name);

    /// Crack incoming psMessageBytes struct for inbound use
    psLocationMessage(MsgEntry *message);
    
    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers* accessPointers);
};

/** @} */

#endif

