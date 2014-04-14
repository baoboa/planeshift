/*
 * pawscharcreatemain.h - author: Andrew Craig
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
#ifndef PAWS_CHARACTER_CREATION_MAIN
#define PAWS_CHARACTER_CREATION_MAIN

#include "paws/pawswidget.h"
#include "paws/pawstextbox.h"
#include "psengine.h"

//////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
//////////////////////////////////////////////////////////////////////////////
class pawsMultiLineTextBox;
class pawsTextBox;
class psCreationManager;
class pawsRadioButtonGroup;
class pawsObjectView;
class psCharAppearance;
struct RaceDefinition;
//////////////////////////////////////////////////////////////////////////////

// general name validity funcs
bool CheckNameForRepeatingLetters(csString name);
bool FilterName(const char* name);
csString NormalizeCharacterName(const csString & name);

/** The main creation window for creating a new character. 
 * This is the screen where players pick their:
 * <UL> 
 *  <LI> Name
 *  <LI> Race
 *  <LI> Gender
 *  <LI> Appearance.
 * </UL>
 */
class pawsCreationMain : public pawsWidget, public psClientNetSubscriber, public DelayedLoader
{
public:
    
    pawsCreationMain();
    ~pawsCreationMain();
    
    void HandleMessage( MsgEntry* me );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    bool PostSetup();
    bool OnChange(pawsWidget *widget);
    void Show();
    bool CheckLoadStatus();

    /* Reloads all character creation windows so that they reset */
    void ResetAllWindows();    
    
    void Reset();
    
private:
    void SetHairStyle( int newStyle );
    void RemoveHairStyle( int oldStyle );

    /** Show the current CP
       */
    void UpdateCP();
    
    /** Change the face of a player.
      * @param currentFaceChoice  The number of the current choice in the available list.
      */
    void ChangeFace( int currentFaceChoice );   

    /** Gray race buttons that are unavailable with the current gender.
      */
    void GrayRaceButtons( );

    /** Gray style buttons that are unavailable with the current race/gender.
      */
    void GrayStyleButtons( );

    /** Select a new gender.
      */
    void SelectGender(int newGender);
    
    /// The label that has the face choice name.
    pawsTextBox* faceLabel;
    
    /// The current face customization choice.
    int currentFaceChoice;

    void ChangeBeardStyle( int currentChoice );                        
    pawsTextBox* beardStyleLabel;    
    int currentBeardStyleChoice;    
    
     /**
      * Change the hair style mesh of a player.
      *
      * @param currentChoice  The number of the current choice in the available list.
      * @param old Old value.
      */        
    void ChangeHairStyle( int currentChoice, int old );                        
    pawsTextBox* hairStyleLabel;    
    int activeHairStyle;    
    
    void ChangeHairColour( int currentChoice );                        
    int activeHairColour;
    pawsTextBox* hairColourLabel;
        
    void ChangeSkinColour( int currentChoice );                        
    int currentSkinColour;
    pawsTextBox* skinColourLabel;
            
    /// Needed to query data about races.
    psCreationManager* createManager;
    
    /// Quick access to volitle race description widget.
    pawsMultiLineTextBox* raceDescription;
    
    /// Hold the CP points left in creation.
    pawsTextBox* cpPoints;
    
    /// Gender radio button group
    pawsRadioButtonGroup* gender;        
 
    ///button used for the male selection
    pawsButton* maleButton; 
    ///button used for the female selection
    pawsButton* femaleButton;

    /// Paper doll view of model.
    pawsObjectView* view;   
    
    /// The current gender the player has selected.
    int currentGender;
    
    /// The last gender the player selected. Used because of Kran will always swith to male.
    int lastGender;  
    
    int lastRaceID;
    csString newWindow;
    /// The current race the player has selected.    
    RaceDefinition* race;
     
    
    /**
     * Updates the race + gender shown.
     *
     * @param id ID of the race to change to
     */
    void UpdateRace(int id);
 
    /// Variable to check make sure they were warned, if they did not use a random name
    bool nameWarning;
    ///The current name selected by the player.
    pawsEditTextBox* lastnameTextBox;
    pawsEditTextBox* firstnameTextBox;
    
    psCharAppearance* charApp;

    bool loaded;
    csString factName;
};


CREATE_PAWS_FACTORY( pawsCreationMain );

#endif


