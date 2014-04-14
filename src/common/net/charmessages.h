#ifndef PS_CHAR_MESSAGES_H
#define PS_CHAR_MESSAGES_H

#include <csutil/parray.h>
#include "net/messages.h"

/**
 * \addtogroup messages
 * @{ */

////////////////////////////////////////////////////////////////////////////////
//  GENDER VALUES FOR CHARACTERS
////////////////////////////////////////////////////////////////////////////////
enum PSCHARACTER_GENDER {
    PSCHARACTER_GENDER_NONE = 0,
    PSCHARACTER_GENDER_FEMALE = 1,
    PSCHARACTER_GENDER_MALE = 2,
    PSCHARACTER_GENDER_COUNT = 3
};
////////////////////////////////////////////////////////////////////////////////

/** Define the player controled base customization that their model can have.
  */
enum PSTRAIT_LOCATION {
    PSTRAIT_LOCATION_NONE = 0,
    PSTRAIT_LOCATION_FACE,
    PSTRAIT_LOCATION_HAIR_STYLE,
    PSTRAIT_LOCATION_BEARD_STYLE,
    PSTRAIT_LOCATION_HAIR_COLOR,
    PSTRAIT_LOCATION_SKIN_TONE,
    PSTRAIT_LOCATION_ITEM,
    PSTRAIT_LOCATION_EYE_COLOR,
    PSTRAIT_LOCATION_COUNT
};
//NOTE: Remember to update the locationString in pstrait.cpp each time something is changed here

////////////////////////////////////////////////////////////////////////////////



///Used to confirm that a character has been uploaded.
/**This message is used to convey the success to the client
 * that the character they created was done successfully.
 */
class psCharApprovedMessage : public psMessageCracker
{
public:
    ///Constructed on server for client
    psCharApprovedMessage( uint32_t clientnum );

    ///Used by client to crack message.
    psCharApprovedMessage ( MsgEntry* msg );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers*  accessPointers);
};


//--------------------------------------------------------------------------



/** Opposite of psCharApprovedMessage. 
   Hands back a type and error message for the client to display.
  */
class psCharRejectedMessage : public psMessageCracker
{
public:
    /// Possible Character Rejection Reasons.
    enum ErrorTypes
    {
        UNKNOWN,
        NON_LEGAL_NAME,
        NON_UNIQUE_NAME,
        RESERVED_NAME,
        INVALID_CREATION,
        FAILED_ACCOUNT
    };
    
    ///Constructed on server for the client
    psCharRejectedMessage ( uint32_t clientnum, int type=UNKNOWN, const char *msg="Unknown error" );

    ///Constructed on client to crack message
    psCharRejectedMessage( MsgEntry* msg );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers*  accessPointers);
    
    /// The error type ( from enum )
    int errorType;
    
    /// Holds error message from the server. 
    csString errorMesg;
};


//--------------------------------------------------------------------------

/** Message that has a list of the stats and skills that will be created for
 *   a character.
 */  
class psCharVerificationMesg : public psMessageCracker
{
public:
    struct Attribute
    {
        csString name;
        int value;
    };
    
    psCharVerificationMesg( uint32_t client );
    psCharVerificationMesg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers*  accessPointers);
    
    /** Add a new variable to the list.  
      * @param value = the value or rank of the attribute
      * @param attributeName = the name of the stat/skill 
      */
    void AddStat( int value, const char* attributeName );        
    void AddSkill( int value, const char* attributeName );        
    
    // Build the message
    void Construct();
    
    csArray<psCharVerificationMesg::Attribute> stats;
    csArray<psCharVerificationMesg::Attribute> skills;    
    
    // Used in the creation of message.
    size_t runningSize;
    
    // target of message
    uint32_t clientnum;
};

//--------------------------------------------------------------------------

class psCharUploadMessage : public psMessageCracker
{
public:
    psCharUploadMessage( bool verify, const char* name, const char* lastname, int race, int gender, 
                         csArray<uint32_t> choices, int motherMod, int fatherMod, csArray<uint32_t> lifeEvents,
                         int selectedFace, int selectedHairStyle, int selectedBeardStyle, 
                         int selectedHairColour, int selectedSkinColour, const char* bio, const char* path );
                         
    psCharUploadMessage( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers*  accessPointers);
    
    
public:  
    bool verify; 
    csString name;
    csString lastname;
    int   race;
    int   gender;    
    
    csArray<uint32_t> choices;
    csArray<uint32_t> lifeEvents;
    
    int selectedFace;
    int selectedHairStyle;
    int selectedBeardStyle;
    int selectedHairColour;
    int selectedSkinColour;      
    
    int fatherMod;
    int motherMod;

    csString bio;
    csString path;
};

//--------------------------------------------------------------------------

/** This is a list of all the possible choice areas.  
  * This are sort of widget related.  For example FATHER_JOB will refer to a
  * choice that is for the father job area of the parents screen and will have 
  * a name and a description.
  */
enum CreationAreas
{
    ZODIAC,
    FATHER_JOB,
    MOTHER_JOB,
    RELIGION, 
    BIRTH_EVENT,
    CHILD_ACTIVITY,
    CHILD_HOUSE,
    CHILD_SIBLINGS    
};

/** A Creation Choice that the client can make. All choices fit this profile.
  */
struct CreationChoice
{
    int id;
    csString name;
    csString description;
    int choiceArea;
    int cpCost;
};

/** A general message class for sending a character creation choice.
  * Since all the choices fit this profile we have a shared message type to 
  * send a choice.  They are seperated by using different listener's so subscribers
  * can listen for different types of choice requests.
  *
  * This can be a single choice or a list of choices. 
  */
class psCreationChoiceMsg : public psMessageCracker
{
public:   
    
    /** Create a new message destined for server.
     * @param listener the subscribers that should get this message.
     */
    psCreationChoiceMsg( int listener );
    
    /** Crack open the message and get any data in the message.
     */
    psCreationChoiceMsg( MsgEntry* me );    
    
    /** Create a new message that will be list of choices.
     * This should only really be done by the server. 
     *
     *  @param clientTo the destination of this message.
     *  @param totalChoices The total number of choices that this message will carry.
     *  @param listener The subscribers that should get this message.
     */     
    psCreationChoiceMsg( int clientTo, int totalChoices, int listener );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers*  accessPointers);
    
    /** Add a new choice the the message payload.
     *
     * @param id The unique ID for this choice ( the database id in this case )
     * @param name The name of this choice.
     * @param description The description of this creation choice.
     * @param area The Creation area for this choice. Should be one of the CreationAreas
     *             enums.
     * @param cost The CP cost of the choice.                   
     */
    void AddChoice( int id, const char* name, const char* description, int area, int cost );

    /// Resize the message to the right size. 
    void ConstructMessage();
       
    /// Holds a list of all the choices in this message.         
    csArray<CreationChoice> choices;        
};


//------------------------------------------------------------------------------

/** Defines the structure needed to send a life event across the network.
  */
class LifeEventChoice
{
public:
    int id;                     /// The DB id 
    csString name;              /// Name of choice ( will be displayed in list box )
    csString description;       /// Description of choice ( will be displayed )
    csArray<int> adds;          /// List of choices current choice can add.
    csArray<int> removes;       /// List of chices current choice removes.
    int common;                 /// This is a base choice not dependent on others. 
    int cpCost;
};


/** Defines a Life Event message. 
    This message class is used to pack all the life event choices to send to 
    the client.
*/
class  psLifeEventMsg : public psMessageCracker
{
public:
    /** A request to the server for the life choice list!
     */
    psLifeEventMsg();
    
    psLifeEventMsg( uint32_t client );
    
    psLifeEventMsg( MsgEntry* me );

    PSF_DECLARE_MSG_FACTORY();

    /**
     * Convert the message into human readable string.
     *
     * @param accessPointers A struct to a number of access pointers.
     * @return Return a human readable string for the message.
     */
    virtual csString ToString(NetBase::AccessPointers*  accessPointers);
    
    void AddEvent( LifeEventChoice *choice );
    
    void ConstructMessage();
        
    csPDelArray<LifeEventChoice> choices;
    
    uint32_t toclient;
    size_t runningSize;
};

/** @} */

#endif 

   

