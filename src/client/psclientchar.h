/*
 * psclientchar.h 
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
 * Manages details about the player as it talks to it's server version
 */
#ifndef PS_CLIENT_CHAR_H
#define PS_CLIENT_CHAR_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <iutil/document.h>
#include <csutil/refarr.h>
#include <imesh/spritecal3d.h>
#include <csgeom/vector4.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"
#include "net/charmessages.h"

#include "util/namegenerator.h"

//=============================================================================
// Local Includes
//=============================================================================

struct iVFS;
struct iObjectRegistry;
struct iSector;
struct iMeshObjectFactory;

class MsgHandler;
class MsgEntry;

class psCelClient;
class psCreationManager;

class GEMClientObject;

/** Defines a character trait.
*/
struct Trait
{
    void Load( iDocumentNode * node);
    PSTRAIT_LOCATION ConvertTraitLocationString(const char *locationstring);

    static const char* locationString[];
    const char* GetLocationString() const;
    unsigned int uid;
    unsigned int next_trait_uid;
    Trait *next_trait,*prev_trait;
    unsigned int raceID;
    PSCHARACTER_GENDER gender;
    PSTRAIT_LOCATION location;
    csString name;
    csString mesh;
    csString material;
    csString subMesh;
    csVector3 shader;   ///< The shader colour set for this trait. 
};


/** Manages character details for the client.
 *  This Manager handles things like applying the equipment/mesh changes to a
 *  gem object as well as targeting and effect issues.
 */
class psClientCharManager : public psClientNetSubscriber
{
public:
    psClientCharManager(iObjectRegistry *objectreg);    
    virtual ~psClientCharManager();
    bool Initialize( MsgHandler* msghandler, psCelClient* );
       
    ///Returns true when the character has been acked on the server
    bool Ready() { return ready; }
    
    /// Handle messages from server
    virtual void HandleMessage( MsgEntry* me );
    
    psCreationManager* GetCreation() { return charCreation; }

    /**
     * Sets the target of the character.
     *
     * This will send a select message to the server to inform it of the change in target.
     *  @param newTarget The new target of the character, 0 to select nothing.
     *  @param action the action string to send to the server
     *  @param notifyServer true if the server should be informed of the new target.
     */
    void SetTarget(GEMClientObject *newTarget, const char *action, bool notifyServer=true);

    /** locks the current target so it cannot be changed
     *   @param state the desired state
     */
    void LockTarget(bool state);

    /**Gets the target of the character.
     *  @return the target.
     */
    GEMClientObject * GetTarget() { return target; };

protected:
    /// Change a trait on a character.
    void ChangeTrait(MsgEntry* me);

    /// Handle animation changes for things from the server.
    void HandleAction(MsgEntry* me);

    /// Handle the equipment messages comming from the server. 
    void HandleEquipment(MsgEntry* me);

    /// Handle the effect message comming from the server.
    void HandleEffect(MsgEntry* me);

    void HandleEffectStop(MsgEntry* me);

    /// Handle a rejection of a character
    void HandleRejectCharMessage(MsgEntry* me);

    /// Handle the play sound message from the server
    void HandlePlaySound(MsgEntry* me);

    /// Handle update target message from the server
    void HandleTargetUpdate(MsgEntry* me);

    csRef<MsgHandler>         msghandler;
    iObjectRegistry*          objectReg;
    csRef<iVFS>               vfs;

    psCreationManager* charCreation;

    /// keeps track of the target effect ID
    unsigned int targetEffect;

    /// keeps track of what object you have targetted
    GEMClientObject * target;

    psCelClient *cel;
    bool ready;

    /// stores if target is locked
    bool lockedTarget;

    csHash<unsigned int, uint32_t> effectMapper;
};


//------------------------------------------------------------------------------
    

#define REQUESTING_CP -1


/** Defines a character creation choice for change of appearance. 
 */
struct CustomChoice
{    
    void Load( iDocumentNode* node );
    csString mesh;
    csString material;
    csString name; 
    csString texture;
    PSCHARACTER_GENDER gender;
    int id;
};

//------------------------------------------------------------------------------

/** Defines a material set. This is used for things that can change multiple 
  * materials like the hair colour or skin tones.    
*/
struct MaterialSet
{
    void Load( iDocumentNode* node );
    csString name;
    PSCHARACTER_GENDER gender;
    csPDelArray<CustomChoice> materials;
    int id;
};


//------------------------------------------------------------------------------

/** Defines a race. 
  * Defines things about a race such as the name of the models and the camera
  * positions for the various camera modes. Also defines the zoom locations 
  * for the character creation.
  */
struct RaceDefinition
{
    csString name;              
    csString description;       ///< Description of race ( for character creation )
    
    csString femaleModelName;   
    csString maleModelName;

    bool femaleAvailable;
    bool maleAvailable;
            
    int startingCP;    
    
    // Camera positions for this race allows different heights
    csVector3 FollowPos, LookatPos, FirstPos, ViewerPos;
    
    csArray<Trait*> location[PSTRAIT_LOCATION_COUNT][PSCHARACTER_GENDER_COUNT];
    
    /// Camera location for character creation
    csVector3 zoomLocations[PSTRAIT_LOCATION_COUNT];
};

//------------------------------------------------------------------------------

/** Used to define a path in character creation.
 */
struct PathDefinition
{
    csString name;

    struct Bonus
    {
        csString name;
        float value;
    };

    csPDelArray<Bonus> statBonuses;
    csPDelArray<Bonus> skillBonuses;
    
    csString info;
    csString parents;
    csString life;
};

//------------------------------------------------------------------------------

/** Handles all the details of the character creation on the client side.
 * This class maintains the players choices, as well as the relevant information
 * that is needed on the screens.
 * This class communicates with the server to get any information that it needs
 * to show the client.  It caches this data as well so only new data needs to be 
 * requested from the server.
 *
 * This class also sends the final 'create' message to the server once the creation 
 * has been complete on the client. 
 */
class psCreationManager : public psClientNetSubscriber
{
public:
    
    psCreationManager(iObjectRegistry *objectreg);
    virtual ~psCreationManager();

    void HandleMessage( MsgEntry* me );
   
    /** Get the starting CP value for a race ( as defined in the enums ).
      * This information is requested from the server. If not yet ready it will 
      * return REQUESTING to let the caller know that it is not ready.
      *
      * @param race The race id to get the CP value from.
      * @return The CP value or REQUESTING
      */       
    int GetRaceCP( int race );
    
    /** Get a race desciption for a race.
      * This information is stored on the client in /data/races/descriptions.xml and 
      * should be available upon request ( ie no request to server ).
      *
      * @param race The race ID to find the description of.
      * @return The race description or "None" if race was not found.
      */
    const char* GetRaceDescription( int race );
    
    RaceDefinition* GetRace( int race );
    RaceDefinition* GetRace( const char* name );

    typedef csPDelArray<PathDefinition>::Iterator PathIterator;
    PathIterator GetPathIterator();

    PathDefinition* GetPath(int i);

    typedef csPDelArray<Trait>::Iterator TraitIterator;
    TraitIterator GetTraitIterator() { return traits.GetIterator(); };
    Trait* GetTrait(unsigned int uid);
    
    /** This requests the data needed to create the parent screen. 
     *  If the data is already on the client then does nothing. 
     */
    void GetParentData();
    void GetChildhoodData();
    void GetLifeEventData();
    void GetTraitData();

    
    /// returns true if the parent data is cached.
    bool HasParentData() { return hasParentData; }
    bool HasChildhoodData() { return hasChildhoodData; }
    bool HasLifeEventData() { return hasLifeEventData; }

    /** Gets the model file name for a race.
     *
     *  @param race One of the 12 race model files
     *  @param gender One of the 3 genders.
     *
     *  @return The string that is the name of the model file or NULL otherwise.
     */
    const char* GetModelName( int race, int gender );
    
    /// A list of all the choices that are for the parent screen. 
    csArray<CreationChoice> parentData;  
    
    /// Holds data for the childhood data screen.
    csArray<CreationChoice> childhoodData;
 
    //
    csArray<LifeEventChoice> lifeEventData;
    
    const char* GetDescription( int choiceID );
    
    int GetCurrentCP() { return currentCP; }
    void SetCurrentCP( int value ) { currentCP = value; }
    
    /// Return the selected Race
    void SetRace( int selectedRace );
    int  GetSelectedRace() { return selectedRace; }
    
    void SetGender( int gender ) { selectedGender = gender; }
    int  GetSelectedGender() { return selectedGender; }
    
    void UploadChar( bool doVerify = false );
    void SetName( const char* newName );
    csString GetName();
    
    LifeEventChoice* FindLifeEvent( int idNumber );

    void AddChoice( int choice, int modifier = 1 );
    void RemoveChoice( uint32_t choice, int modifier = 1 );
    csArray<uint32_t> GetChoicesMade() { return choicesMade;}
    CreationChoice* GetChoice(int id);
    void AddLifeEvent( int event );
    void RemoveLifeEvent( uint32_t event );
    csArray<uint32_t> GetLifeChoices() { return lifeEventsMade;}
    size_t GetNumberOfLifeChoices() { return lifeEventsMade.GetSize(); }
    
    int GetCost(int idChoice);
    int GetLifeCost( int id );
    
    void SetCustomization( int face, int hairStyle, int beardStyle, int hairColour, int skinColour );
    /** Gets the costumizations done and set with SetCostumization.
     *  @param face Gives the trait uid of the face costumization done or zero if none.
     *  @param hairStyle Gives the trait uid of the hair style costumization done or zero if none.
     *  @param beardStyle Gives the trait uid of the beard style costumization done or zero if none.
     *  @param hairColour Gives the trait uid of the hair colour costumization done or zero if none.
     *  @param skinColour Gives the trait uid of the skin colour costumization done or zero if none.
     */    
    void GetCustomization( int& face, int& hairStyle, int& beardStyle, int& hairColour, int& skinColour);
    
    void GenerateName(int type,csString &namebuffer,int max_low,int max_high)
    { nameGenerator->GenerateName( type, namebuffer, max_low, max_high ); }
        
    const char* GetLifeEventDescription( int id );

    bool IsAvailable(int id,int gender);
    
    void SetFatherMod( int mod ) { fatherMod = mod; }
    void SetMotherMod( int mod ) { motherMod = mod; }
    const char *GetFatherModAsText( void ) { switch (fatherMod) { case 2: return "Famous "; case 3: return "Exceptional "; default: return "";} }
    const char *GetMotherModAsText( void ) { switch (motherMod) { case 2: return "Famous "; case 3: return "Exceptional "; default: return "";} }

    void ClearChoices();
    void SetPath( const char* name ) { path = name; }
private:
    iObjectRegistry* objectReg;

    /// Player Current Path    
    csString path;
    
    /// Load data from xml file: /data/races/descriptions.xml
    void LoadRaceInformation();

    /// Load data from xml file: /data/races/quickpaths.xml
    void LoadPathInfo();
   
    /// Maintain a list of all the race descriptions.
    csPDelArray<RaceDefinition> raceDescriptions;    

    /// Maintain a list of all the paths.
    csPDelArray<PathDefinition> pathDefinitions;

    /// Maintain a list of all the traits.
    csPDelArray<Trait> traits;    
    
    csRef<MsgHandler> msgHandler;
    
    /// Stores how many outstanding requests we have for the parent data. 
    bool hasParentData;
    bool hasChildhoodData;
    bool hasLifeEventData;
    bool hasTraitData;

    void HandleVerify( MsgEntry* me );
    void HandleParentsData( MsgEntry* me );  
    void HandleChildhoodData( MsgEntry* me );
    void HandleLifeEventData( MsgEntry* me );
    void HandleTraitData( MsgEntry* me );
    
    NameGenerationSystem * nameGenerator;
    
    /////////////////////////////////////////
    // Data chosen in char creation screens
    /////////////////////////////////////////
    csString selectedName;
    int selectedRace;
    int selectedGender;
    int currentCP;
    csArray<uint32_t> choicesMade;
    csArray<uint32_t> lifeEventsMade;     
    
    int selectedFace;
    int selectedHairStyle;
    int selectedBeardStyle;
    int selectedHairColour;
    int selectedSkinColour;  
    
    int fatherMod;
    int motherMod;
};



#endif
