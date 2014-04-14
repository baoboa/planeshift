/*
 * pawscharparents.cpp - Author: Andrew Craig
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
#include <psconfig.h>

#include "pawscharparents.h"
#include "paws/pawslistbox.h"
#include "paws/pawscombo.h"
#include "paws/pawsbutton.h"
#include "paws/pawsradio.h"
#include "net/charmessages.h"
#include "psclientchar.h"
#include "pawscharcreatemain.h"

#include "globals.h"

////////////////////////////////////////////////////////////////////////////////
#define FATHER_JOB_LISTBOX 1000
#define MOTHER_JOB_LISTBOX 2000
#define RELIGION_LISTBOX   4000

#define NEXT_BUTTON 5000
#define BACK_BUTTON 6000
////////////////////////////////////////////////////////////////////////////////


pawsCharParents::pawsCharParents()
{
    createManager = psengine->GetCharManager()->GetCreation();
    
    dataLoaded = false;
    
    lastFatherChoice   = -1;
    lastReligionChoice = -1;
    lastMotherChoice   = -1;
    religionCount = 0;
    
    fMod = 1;
    fLastMod = 1;
    mMod = 1;
    mLastMod = 1;
}


pawsCharParents::~pawsCharParents()
{
}

bool pawsCharParents::PostSetup()
{
    /*motherNameTextBox = (pawsEditTextBox*)FindWidget("mname");
    fatherNameTextBox = (pawsEditTextBox*)FindWidget("fname");
    if (!motherNameTextBox || !fatherNameTextBox)
        return false;*/

    fatherJobDesc = (pawsMultiLineTextBox*)FindWidget( "fjobdesc" );    
    if ( !fatherJobDesc )
        return false;
           
    motherJobDesc = (pawsMultiLineTextBox*)FindWidget( "mjobdesc" );    
    if ( !fatherJobDesc )
        return false;
        
    motherStatus = (pawsRadioButtonGroup*)FindWidget("mstatus");
    fatherStatus = (pawsRadioButtonGroup*)FindWidget("fstatus");
    if (!motherStatus || !fatherStatus)
        return false;
    /// default = "Normal"
    if (!motherStatus->SetActive("MNormal") || !fatherStatus->SetActive("FNormal"))
        return false;

    religionDesc = (pawsMultiLineTextBox*)FindWidget("religiondesc");
    if ( !religionDesc )
        return false;
    
    cpBox = (pawsTextBox*)FindWidget("CP");
    if ( !cpBox )
        return false;        
             
    return true;
}


void pawsCharParents::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        // Figure out which list box was checked and act accordingly
        switch ( widget->GetID() )
        {
            case FATHER_JOB_LISTBOX:
            {
                pawsListBoxRow* row = widget->GetSelectedRow();            
                
                fatherJobDesc->SetText( createManager->GetDescription(row->GetID()) );
                            
                createManager->RemoveChoice( lastFatherChoice, fLastMod );
                createManager->AddChoice( row->GetID(), fMod );
                lastFatherChoice = row->GetID();
                UpdateCP();
                return;
            }
            case MOTHER_JOB_LISTBOX:
            {
                pawsListBoxRow* row = widget->GetSelectedRow();            
                
                motherJobDesc->SetText( createManager->GetDescription(row->GetID()) );
                            
                createManager->RemoveChoice( lastMotherChoice, mLastMod );
                createManager->AddChoice( row->GetID(), mMod );
                lastMotherChoice = row->GetID();
                UpdateCP();
                return;
            }
            
            
            case RELIGION_LISTBOX:
            {
                pawsListBoxRow* row = widget->GetSelectedRow();                

                religionDesc->SetText( createManager->GetDescription(row->GetID()) );

                createManager->RemoveChoice( lastReligionChoice );
                createManager->AddChoice( row->GetID() );
                lastReligionChoice = row->GetID();
                UpdateCP();
                return;
            }
        }
    }
}

void pawsCharParents::Draw()
{
    pawsWidget::Draw();

    // Check to see if we are waiting for data from the server. Should have a waiting
    // cursor if this fails.     
    if (!dataLoaded && createManager->HasParentData())
    {
        PopulateFields();
        dataLoaded = true;
    }                
}

void pawsCharParents::HandleFatherStatus( int newMod )
{
    fMod = newMod;
        
    if ( fLastMod != fMod )
    {
        createManager->RemoveChoice( lastFatherChoice, fLastMod );                                    
        fLastMod = fMod;
        createManager->SetFatherMod( fMod );            
        createManager->AddChoice( lastFatherChoice, fMod );                                    
        UpdateCP();
    }            
}

void pawsCharParents::HandleMotherStatus( int newMod )
{
    mMod = newMod;
        
    if ( mLastMod != mMod )
    {
        createManager->RemoveChoice( lastMotherChoice, mLastMod );                                    
        mLastMod = mMod;
        createManager->SetMotherMod( mMod );            
        createManager->AddChoice( lastMotherChoice, mMod );                                    
        UpdateCP();
    }            
}


bool pawsCharParents::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    csString name = widget->GetName();

    if (name == "randomize")
    {
        Randomize();
        return true;
    }
    else if (name == "MNormal")
    {
        HandleMotherStatus( 1 );
    }      
    else if (name == "MFamous")
    {
        HandleMotherStatus( 2 );
    }            
    else if (name == "MExceptional")
    {
        HandleMotherStatus( 3 );
    }        
    else if (name == "FNormal")
    {
        HandleFatherStatus( 1 );
    }  
    else if (name == "FFamous")
    {
        HandleFatherStatus( 2 );
    }        
    else if (name == "FExceptional")
    {
        HandleFatherStatus( 3 );
    }        
    
    switch ( widget->GetID() )
    {
        case BACK_BUTTON:
        {
            Hide();
            PawsManager::GetSingleton().FindWidget( "CharBirth" )->Show();
            return true;
        }
        
        case NEXT_BUTTON:
        {
            // Validate parents names
            /*csString mName(motherNameTextBox->GetText());
            csString fName(fatherNameTextBox->GetText());
			
            csString mFirstName = mName.Slice(0, mName.FindFirst(' '));
			csString mLastName;
			if (mName.FindFirst(' ') != SIZET_NOT_FOUND)
				mLastName = mName.Slice(mName.FindFirst(' ') + 1, mName.Length());
			else
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter a full name for mother"));
                error.FireEvent();
                return true;
            }
			
            csString fFirstName = fName.Slice(0, fName.FindFirst(' '));
			csString fLastName;
			if (fName.FindFirst(' ') != SIZET_NOT_FOUND)
				fLastName = fName.Slice(fName.FindFirst(' ') + 1, fName.Length());
			else
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter a full name for father"));
                error.FireEvent();
                return true;
            }

            if (fFirstName.Length() < 3 || fLastName.Length() < 3 || !FilterName(fFirstName) || !FilterName(fLastName) ||
				mFirstName.Length() < 3 || mLastName.Length() < 3 || !FilterName(mFirstName) || !FilterName(mLastName))
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter valid names for both parents"));
                error.FireEvent();
                return true;
            }
			
            if (!CheckNameForRepeatingLetters(fFirstName) || !CheckNameForRepeatingLetters(fLastName) ||
				!CheckNameForRepeatingLetters(mFirstName) || !CheckNameForRepeatingLetters(mLastName))
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("No more than 2 letters in a row allowed for parents' names"));
                error.FireEvent();
                return true;
            }
			
			mFirstName = NormalizeCharacterName(mFirstName);
			mLastName = NormalizeCharacterName(mLastName);
			fFirstName = NormalizeCharacterName(fFirstName);
			fLastName = NormalizeCharacterName(fLastName);
            motherNameTextBox->SetText(mFirstName + " " + mLastName);
            fatherNameTextBox->SetText(fFirstName + " " + fLastName);
            */
            // check other parental data
            if (lastFatherChoice == -1 || lastMotherChoice == -1 )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please choose parents' jobs"));
                error.FireEvent();
                return true;
            }

            // check religion selected
            pawsListBox* religionBox = (pawsListBox*)FindWidget("Religions");
            if (!religionBox->GetSelectedRow())
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please choose a religion"));
                error.FireEvent();
                return true;
            }

            Hide();
            createManager->GetChildhoodData();
            PawsManager::GetSingleton().FindWidget( "Childhood" )->Show();
            return true;
        }
    }
    
    return false;
}

void pawsCharParents::UpdateCP()
{
    int cp = createManager->GetCurrentCP();
        
    char buff[10];
    sprintf( buff, "CP: %d", cp );
    cpBox->SetText( buff );
}    

void pawsCharParents::Show()
{
    pawsWidget::Show();
    UpdateCP();  
}

void pawsCharParents::PopulateFields()
{
    pawsListBox* fatherBox = (pawsListBox*)FindWidget("fatherjobs");
    pawsListBox* motherBox = (pawsListBox*)FindWidget("motherjobs");
    pawsListBox* religionBox = (pawsListBox*)FindWidget("Religions");
    
    // For all the data available for the parents screen figure out where 
    // it should go. 
    fatherBox->AutoScrollUpdate(false); // Optimize
    motherBox->AutoScrollUpdate(false); // Optimize

    for (size_t x=0; x < createManager->parentData.GetSize(); x++ )
    {    
        switch ( createManager->parentData[x].choiceArea )
        {
            case FATHER_JOB:
            {
                pawsListBoxRow* row =  fatherBox->NewRow();            
                row->SetID( createManager->parentData[x].id );
                pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
                name->SetText( createManager->parentData[x].name  );              
                name->FormatToolTip("%s %d\n", PawsManager::GetSingleton().Translate("CP Cost:").GetData(), createManager->parentData[x].cpCost );                  
                break;
            }
            
            case MOTHER_JOB:
            {
                pawsListBoxRow* row =  motherBox->NewRow();            
                row->SetID( createManager->parentData[x].id );
                pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
                name->SetText( createManager->parentData[x].name  );                     
                name->FormatToolTip("%s %d\n", PawsManager::GetSingleton().Translate("CP Cost:").GetData(), createManager->parentData[x].cpCost );                  
                break;
            }
            case RELIGION:
            {
                pawsListBoxRow* row = religionBox->NewRow();
                pawsTextBox* box = (pawsTextBox*)row->GetColumn(0);
                row->SetID( createManager->parentData[x].id );
                box->SetID( createManager->parentData[x].id );
                box->SetText( createManager->parentData[x].name );
                box->FormatToolTip("%s %d\n", PawsManager::GetSingleton().Translate("CP Cost:").GetData(), createManager->parentData[x].cpCost );                  
                religionCount++;  
                break;                           
            }
         }                         
    }

    // Update because we disabled auto scroll update
    fatherBox->SetScrollBarMaxValue();
    motherBox->SetScrollBarMaxValue();

    UpdateCP();
}


void pawsCharParents::Randomize()
{
    pawsListBox* fatherBox = (pawsListBox*)FindWidget("fatherjobs");
    pawsListBox* motherBox = (pawsListBox*)FindWidget("motherjobs");
    pawsListBox* religionBox = (pawsListBox*)FindWidget("Religions");

    /*// randomize names
	csString lastName = createManager->GetName().Slice(createManager->GetName().Find(" ")+1);
	
    // make uppper case
    csString upper( lastName );
    upper.Upcase();
    lastName.SetAt(0, upper.GetAt(0) );

    csString randomName;
    createManager->GenerateName(NAMEGENERATOR_MALE_FIRST_NAME,
                                randomName,3,5);

    // make uppper case
    upper = randomName;
    upper.Upcase();
    randomName.SetAt(0, upper.GetAt(0) );

	// join first and last name
	csString fullName;
	fullName.Format("%s %s", randomName.GetData(), lastName.GetData());
	
    fatherNameTextBox->SetText( fullName );
    
    createManager->GenerateName(NAMEGENERATOR_FEMALE_FIRST_NAME,
                                randomName,5,7);

    // make uppper case
    upper = randomName;
    upper.Upcase();
    randomName.SetAt(0, upper.GetAt(0) );

	// join first and last name
	fullName.Format("%s %s", randomName.GetData(), lastName.GetData());
	
    motherNameTextBox->SetText( fullName );
    */
    //randomize jobs
    fatherBox->Select(fatherBox->GetRow(psengine->GetRandomGen().Get(fatherBox->GetRowCount())));
    motherBox->Select(motherBox->GetRow(psengine->GetRandomGen().Get(motherBox->GetRowCount())));

    //randomize religion
    religionBox->Select(religionBox->GetRow(psengine->GetRandomGen().Get(religionBox->GetRowCount())));

    // set status to normal
    pawsRadioButtonGroup* fstatus = (pawsRadioButtonGroup*)FindWidget("fstatus");
    pawsRadioButtonGroup* mstatus = (pawsRadioButtonGroup*)FindWidget("mstatus");

    mstatus->SetActive("MNormal");
    fstatus->SetActive("FNormal");
    
    HandleMotherStatus( 1 );
    HandleFatherStatus( 1 );
                                   
}
