/*
 * pawscharparents.h - Author: Andrew Craig
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
#ifndef PAWS_CHARACTER_CREATION_PARENTS
#define PAWS_CHARACTER_CREATION_PARENTS

#include "paws/pawswidget.h"
#include "paws/pawsradio.h"
#include "paws/pawstextbox.h"

//////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS
//////////////////////////////////////////////////////////////////////////////
class psCreationManager;
class pawsMultiLineTextBox;
class pawsComboBox;
class pawsTextBox;
//////////////////////////////////////////////////////////////////////////////

/** Handles the Parent screen in character creation.
 */
class pawsCharParents : public pawsWidget
{
public:
    pawsCharParents();
    ~pawsCharParents();   
    
    void Draw();
    
    bool PostSetup();
    void OnListAction( pawsListBox* widget, int status );
    void Show();
    
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
private:
    void Randomize();
    
    /// Needed to query data about races.
    psCreationManager* createManager;
    
    /// Set to true once the data has been populated from the server. 
    bool dataLoaded;

    /*/// parents' names widgets
    pawsEditTextBox* fatherNameTextBox;
    pawsEditTextBox* motherNameTextBox;*/
   
    /// Quick access to this widget since it is can change often.
    pawsMultiLineTextBox* fatherJobDesc;
    pawsMultiLineTextBox* motherJobDesc;

    /// widgets for the normal/famous/exceptional status buttons
    pawsRadioButtonGroup* fatherStatus;
    pawsRadioButtonGroup* motherStatus;

    pawsComboBox* religionBox;
    pawsMultiLineTextBox* religionDesc;
    
    pawsTextBox* cpBox;
    
    /** Fill in the required fields with data.
      * When the client has the necessary data from the sever about the 
      * parents screen, this will fill in things like the jobs list box.
      */
    void PopulateFields();
    
    /// Figures out how to get the CP costs based on the status of the parents.
    void HandleMotherStatus( int newMod );
    void HandleFatherStatus( int newMod );    
    
    void UpdateCP();
 
    int lastFatherChoice;
    int lastMotherChoice;
    int lastReligionChoice;  
    int religionCount;   
    
    int fMod;
    int fLastMod;
    
    int mMod;
    int mLastMod;
    
          
};

CREATE_PAWS_FACTORY( pawsCharParents );

#endif
