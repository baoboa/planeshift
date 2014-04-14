/*
 * pawssummary.cpp - Author: Andrew Craig
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
#include "util/psconst.h"
#include "globals.h"
#include "psclientchar.h"
#include "pawssummary.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsyesnobox.h"
#include <ibgloader.h>
#include "paws/pawsobjectview.h"
#include "charapp.h"

#define BACK_BUTTON   6000
#define UPLOAD_BUTTON 5000
#define UPLOAD_VERIFY true

struct RaceDefinition;

pawsSummaryWindow::pawsSummaryWindow()
{
    createManager = psengine->GetCharManager()->GetCreation();
    redoVerification = false;
    requestSent = false;
    loaded = true;
    
    charApp = new psCharAppearance(psengine->GetObjectRegistry());

    psengine->RegisterDelayedLoader(this);
}

pawsSummaryWindow::~pawsSummaryWindow()
{
}

bool pawsSummaryWindow::PostSetup()
{
    skillsList = (pawsListBox*)FindWidget("skill_list");
    if ( skillsList == NULL ) return false;
    
    statsList = (pawsListBox*)FindWidget("stat_list");
    if ( statsList == NULL ) return false;
    
    serverStatus  = (pawsTextBox*)FindWidget("server_status");
    if ( serverStatus == NULL ) return false;
    
    view = (pawsObjectView*)FindWidget("ModelView");
    if ( view == NULL ) return false;
    while(!view->ContinueLoad())
    {
        csSleep(100);
    }
    
    return true;
}

void pawsSummaryWindow::Update()
{
    pawsTextBox* label_name = (pawsTextBox*) FindWidget("label_name");
    label_name->SetText(createManager->GetName().GetData());

    csString textString;
    csString lifeString;
    csString tempString;

    pawsMultiLineTextBox* text = (pawsMultiLineTextBox*) FindWidget("text_birth");
    pawsMultiLineTextBox* lifeText = (pawsMultiLineTextBox*) FindWidget("text_life");

    if (createManager->GetSelectedRace() == 9)
        textString.Append("Gender: neutral\n");
    else if (createManager->GetSelectedGender() == PSCHARACTER_GENDER_FEMALE)
        textString.Append("Gender: female\n");
    else if (createManager->GetSelectedGender() == PSCHARACTER_GENDER_MALE)
        textString.Append("Gender: male\n");

    tempString.Format("Race: %s\n", createManager->GetRace(createManager->GetSelectedRace())->name.GetData());
    textString.Append(tempString);

    csArray<uint32_t> choices = createManager->GetChoicesMade();
    csArray<uint32_t>::Iterator iter = choices.GetIterator();

    while(iter.HasNext())
    {
        CreationChoice* choice = createManager->GetChoice(iter.Next());
        switch(choice->choiceArea)
        {
        case FATHER_JOB:
            tempString.Format("Father: %s%s\n", createManager->GetFatherModAsText(), choice->name.GetData());
            break;
        case MOTHER_JOB:
            tempString.Format("Mother: %s%s\n", createManager->GetMotherModAsText(), choice->name.GetData());
            break;
        case RELIGION:
            tempString.Format("Religion: %s\n", choice->name.GetData());
            break;
        case BIRTH_EVENT:
            tempString.Format("Event: %s\n", choice->name.GetData());
            break;
        case CHILD_ACTIVITY:
            tempString.Format("Preferred Child Activity:\n%s\n", choice->name.GetData());
            break;
        case CHILD_HOUSE:
            tempString.Format("Home: %s\n", choice->name.GetData());
            break;
        case CHILD_SIBLINGS:
            tempString.Format("Siblings: %s\n", choice->name.GetData());
            break;
        default:
            continue;
        }
        if (choice->choiceArea != CHILD_ACTIVITY)
            textString.Append(tempString);
        else
            lifeString.Append(tempString);
    }
    text->SetText(textString);

    csArray<uint32_t> lifeChoices = createManager->GetLifeChoices();
    csArray<uint32_t>::Iterator lifeIter = lifeChoices.GetIterator();

    while(lifeIter.HasNext())
    {
        LifeEventChoice* lifeEvent = createManager->FindLifeEvent(lifeIter.Next());
        if (lifeEvent)
        {
            tempString.Format("%s\n", lifeEvent->name.GetData());
        }
        else
        {
            tempString.Format("%s\n", "Error");
        }
        
        lifeString.Append(tempString);
    }
    lifeText->SetText(lifeString);    
}

bool pawsSummaryWindow::CheckLoadStatus()
{
    csString factName = createManager->GetModelName(createManager->GetSelectedRace(),createManager->GetSelectedGender());
    if(!loaded)
    {
        if(view->View(factName))
        {
            iMeshWrapper* mesh = view->GetObject();
            charApp->SetMesh(mesh);
            if (!mesh)
            {
                PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(
                                                             "Couldn't find mesh! Please run the updater"));
                return true;
            }

            csRef<iSpriteCal3DState> spstate =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
            if (spstate)
            {
                // Setup cal3d to select random 0 velocity anims
                spstate->SetVelocity(0.0,&psengine->GetRandomGen());
            }

            //set the traits
            int face, hairStyle, beardStyle, hairColour, skinColour;
            createManager->GetCustomization(face, hairStyle, beardStyle, hairColour, skinColour);
            if(hairColour)
                charApp->HairColor(createManager->GetTrait(hairColour)->shader);
            if(hairStyle)
                charApp->HairMesh(createManager->GetTrait(hairStyle)->mesh);
            if(beardStyle)
                charApp->BeardMesh(createManager->GetTrait(beardStyle)->mesh);
            if(face)
                charApp->FaceTexture(createManager->GetTrait(face)->material);
            
            if(skinColour)
            {
                Trait * trait = createManager->GetTrait(skinColour);
                while ( trait )
                {
                    charApp->SetSkinTone(trait->mesh, trait->material);
                    trait = trait->next_trait;
                }
            }
            view->UnlockCamera();

            loaded = true;
        }
    }

    return false;
}


void pawsSummaryWindow::Show()
{
    pawsWidget::Show();
    redoVerification = true;        
    requestSent = false;
    serverStatus->SetText(PawsManager::GetSingleton().Translate("Please wait for verification...."));
}

void pawsSummaryWindow::Draw()
{
    pawsWidget::Draw();
    
    if ( redoVerification && !requestSent )
    {
        if (  createManager->GetCurrentCP() < 0 ) 
        {
            PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate("You cannot have a negative CP value.")); 
            PawsManager::GetSingleton().FindWidget("LifeEvents")->Show();            
            PawsManager::GetSingleton().FindWidget("Summary")->Hide();
            return;
        }

        requestSent = true;
        createManager->UploadChar( UPLOAD_VERIFY );
    }        
}

void pawsSummaryWindow::SetVerify( csArray<psCharVerificationMesg::Attribute> stats,
                                   csArray<psCharVerificationMesg::Attribute> skills )
{
    statsList->Clear();
    skillsList->Clear();
    
    for ( size_t x = 0; x < skills.GetSize(); x++ )
    {       
        if (skills[x].value)
        {
            pawsListBoxRow* row = skillsList->NewRow();
            pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
            name->SetText( skills[x].name );

            pawsTextBox* val = (pawsTextBox*)row->GetColumn(1);
            val->FormatText( "%d", skills[x].value );
        }
    }
    
    for ( size_t x = 0; x < stats.GetSize(); x++ )
    {       
        if (stats[x].value)
        {
            pawsListBoxRow* row = statsList->NewRow();
            pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
            name->SetText( stats[x].name );
                       
            pawsTextBox* val = (pawsTextBox*)row->GetColumn(1);
            val->FormatText( "%d", stats[x].value );
        }
    }
     
    serverStatus->SetText(PawsManager::GetSingleton().Translate("Verification complete"));     
    //load the model
    loaded = false;
    CheckLoadStatus();
}

bool pawsSummaryWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(!widget)
        return false;

    switch ( widget->GetID() )
    {
        case CONFIRM_YES:
        {
            createManager->UploadChar();
            return true;
        }
        case CONFIRM_NO:
        {
            PawsManager::GetSingleton().SetModalWidget(NULL);
            widget->GetParent()->Hide();
            return true;
        }
        case BACK_BUTTON:
        {
            Hide();
            PawsManager::GetSingleton().FindWidget( "LifeEvents" )->Show();
            
            return true;
        }
        case UPLOAD_BUTTON:
        {           
            csString cpwarning = PawsManager::GetSingleton().Translate("Are you sure you want to upload?  ");
            // Also show CP left if more than zero
            if ( createManager->GetCurrentCP() > 0 )
            {
                cpwarning += PawsManager::GetSingleton().Translate("You have ");
                cpwarning += createManager->GetCurrentCP();
                cpwarning += PawsManager::GetSingleton().Translate(" CP left.");
            }
            //we set this widget as modal and we don't want it to be
            //translated as we did already our homework
            PawsManager::GetSingleton().CreateYesNoBox(cpwarning, this, true, false);
            return true;
        }
    }
    
    return false;
}

