/* * pawscharcreatemain.cpp - author: Andrew Craig
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
#include <ctype.h>
#include <imesh/spritecal3d.h>
#include <igraphic/image.h>
#include <ivideo/txtmgr.h>
#include <imap/loader.h>

#include "globals.h"
#include "charapp.h"
#include <ibgloader.h>
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/charmessages.h"

#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsradio.h"
#include "paws/pawsokbox.h"
#include "paws/pawsobjectview.h"
#include "paws/pawsmainwidget.h"

#include "pawscharpick.h"
#include "pawscharcreatemain.h"
#include "psclientchar.h"

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  PAWS BUTTON IDENTIFIERS
//////////////////////////////////////////////////////////////////////////////
#define BACK_BUTTON     1000
#define QUICK_BUTTON 2000
#define NEXT_BUTTON     3000

#define MALE_BUTTON     100
#define FEMALE_BUTTON   200

#define NAMEHELP_BUTTON 900

//////////////////////////////////////////////////////////////////////////////

// Copied from the server
bool FilterName(const char* name);

//////////////////////////////////////////////////////////////////////////////
pawsCreationMain::pawsCreationMain()
{
    createManager = psengine->GetCharManager()->GetCreation();

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAR_CREATE_CP);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAR_CREATE_NAME);
    createManager->SetGender( PSCHARACTER_GENDER_MALE );
    currentGender = PSCHARACTER_GENDER_MALE;
    lastGender = -1;

    currentFaceChoice = 0;
    activeHairStyle = 0;
    currentBeardStyleChoice = 0;
    activeHairColour = 0;
    currentSkinColour = 0;
    race = 0;

    nameWarning = 0;

    charApp = new psCharAppearance(psengine->GetObjectRegistry());
    loaded = true;

    psengine->RegisterDelayedLoader(this);
}


void pawsCreationMain::Reset()
{
    createManager->SetGender( PSCHARACTER_GENDER_MALE );
    currentGender = PSCHARACTER_GENDER_MALE;

    currentFaceChoice = 0;
    activeHairStyle = 0;
    currentBeardStyleChoice = 0;
    activeHairColour = 0;
    currentSkinColour = 0;
    race = 0;

    view->Clear();

    pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
    raceBox->TurnAllOff();

    pawsEditTextBox* firstname = (pawsEditTextBox*)FindWidget("charfirstnametext");
    firstname->SetText("");
    pawsEditTextBox* lastname = (pawsEditTextBox*)FindWidget("charlastnametext");
    lastname->SetText("");
    nameWarning = 0;

    ResetAllWindows();
}


pawsCreationMain::~pawsCreationMain()
{
    psengine->UnregisterDelayedLoader(this);
}

bool pawsCreationMain::PostSetup()
{
    cpPoints = (pawsTextBox*)FindWidget("cppoints");

    if ( !(cpPoints) )
    {
        return false;
    }


    view = (pawsObjectView*)FindWidget("ModelView");
    while(!view->ContinueLoad())
    {
        csSleep(100);
    }

    faceLabel = (pawsTextBox*)FindWidget( "Face" );
    hairStyleLabel = (pawsTextBox*)FindWidget( "HairStyles" );
    beardStyleLabel = (pawsTextBox*)FindWidget( "BeardStyles" );
    hairColourLabel = (pawsTextBox*)FindWidget( "HairColours" );
    skinColourLabel = (pawsTextBox*)FindWidget( "SkinColours" );

    lastnameTextBox  = dynamic_cast <pawsEditTextBox*> (FindWidget("charlastnametext"));
    if (lastnameTextBox == NULL)
        return false;

    firstnameTextBox = dynamic_cast <pawsEditTextBox*> (FindWidget("charfirstnametext"));
    if (firstnameTextBox == NULL)
        return false;

    maleButton = (pawsButton*)FindWidget("MaleButton");
    maleButton->SetToggle(true);
    femaleButton = (pawsButton*)FindWidget("FemaleButton");
    femaleButton->SetToggle(true);

    GrayRaceButtons();
    GrayStyleButtons();

    createManager->GetTraitData();

    return true;
}

void pawsCreationMain::ResetAllWindows()
{
    const char *names[] =
    {
        "CharBirth", "birth.xml",
        "Childhood", "childhood.xml",
        "LifeEvents","lifeevents.xml",
        "Parents","parents.xml",
        /*"Paths","paths.xml",*/
        NULL
    };

    pawsWidget * wnd;
    pawsMainWidget * mainWidget = PawsManager::GetSingleton().GetMainWidget();

    for (int nameNum = 0; names[nameNum] != NULL; nameNum += 2)
    {
        wnd = PawsManager::GetSingleton().FindWidget(names[nameNum]);
        if (wnd != NULL)
        {
            mainWidget->DeleteChild(wnd);
            PawsManager::GetSingleton().LoadWidget(names[nameNum+1]);
        }
        else
            Error2("Failed to find window %s", names[nameNum]);
    }
    createManager->ClearChoices();
}

void pawsCreationMain::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_CHAR_CREATE_CP:
        {
            int race = me->GetInt32();

            createManager->SetCurrentCP( createManager->GetRaceCP( race ) );

            UpdateCP();
            return;
        }

        case MSGTYPE_CHAR_CREATE_NAME:
        {
            psNameCheckMessage msg;
            msg.FromServer(me);
            if (!msg.accepted)
            {
                PawsManager::GetSingleton().CreateWarningBox( msg.reason );
            }
            else
            {
                if ( newWindow.Length() > 0 )
                {
                    Hide();
                    pawsEditTextBox* lastname = (pawsEditTextBox*)FindWidget("charlastnametext");
                    pawsEditTextBox* firstname = (pawsEditTextBox*)FindWidget("charfirstnametext");
                    if (firstname && lastname)
                    {
                        csString csfirstname = firstname->GetText();
                        csString cslastname = lastname->GetText();
                                 
                        createManager->SetName( csfirstname + " " + cslastname);
		    }
                    PawsManager::GetSingleton().FindWidget(newWindow)->Show();
                }
            }
        }
    }
}

void pawsCreationMain::ChangeSkinColour( int currentChoice )
{
    if ( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() == 0 )
    {
        skinColourLabel->SetText(PawsManager::GetSingleton().Translate("Skin Colour"));
        return;
    }

    Trait * trait = race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender][currentChoice];

    while ( trait )
    {
        charApp->SetSkinTone(trait->mesh, trait->material);
        trait = trait->next_trait;
    }

    skinColourLabel->SetText( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender][currentChoice]->name );

    //wait for the skin to load
    while(!view->ContinueLoad(true))
    {
        csSleep(100);
    }
}

void pawsCreationMain::ChangeHairColour( int newHair )
{
    if ( newHair <(int) race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() )
    {
        Trait* trait = race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender][newHair];

        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0);
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true, true);

        charApp->HairColor(trait->shader);

        hairColourLabel->SetText( trait->name );

        activeHairColour = newHair;
    }
    else
        hairColourLabel->SetText(PawsManager::GetSingleton().Translate("Hair Colour"));
}



void pawsCreationMain::SetHairStyle( int newStyle )
{
    if ( newStyle < (int)race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() )
    {
        Trait* trait = race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender][newStyle];
        charApp->HairMesh(trait->mesh);
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0);
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true, true);
        hairStyleLabel->SetText( trait->name );
    }
    else
        hairStyleLabel->SetText(PawsManager::GetSingleton().Translate("Hair Style"));
}

void pawsCreationMain::ChangeHairStyle(int newChoice, int /*oldChoice*/)
{
//    RemoveHairStyle( oldChoice );
    SetHairStyle( newChoice );
}



void pawsCreationMain::ChangeBeardStyle( int newStyle )
{
    if ( newStyle < (int)race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() )
    {
        Trait* trait = race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][newStyle];
        charApp->BeardMesh(trait->mesh);
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0);
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true, true);
        beardStyleLabel->SetText( trait->name );
    }
    else
        beardStyleLabel->SetText(PawsManager::GetSingleton().Translate("Beard Style"));


    if ( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() == 0 )
    {
        beardStyleLabel->SetText(PawsManager::GetSingleton().Translate("Beard Style"));
        return;
    }

//    iMeshWrapper * mesh = view->GetObject();

//    psengine->SetTrait(mesh,race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][currentChoice]);
//    beardStyleLabel->SetText( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][currentChoice]->name );

/* TODO: Move this code to psengine SetTrait
    if ( race->beardStyles[currentGender].GetSize() == 0 )
        return;

    iMeshWrapper * mesh = view->GetObject();
    csRef<iSpriteCal3DState> spstate =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());

    if ( lastAddedBeardStyle != -1 )
    {
        spstate->DetachCoreMesh( race->beardStyles[currentGender][lastAddedBeardStyle]->mesh ) ;
    }

    // If the choice is no mesh then make sure there is no new mesh added.
    if ( race->beardStyles[currentGender][currentChoice]->mesh.GetSize() == 0 )
    {
        lastAddedBeardStyle = -1;
        beardStyleLabel->SetText( race->beardStyles[currentGender][currentChoice]->name );
        return;
    }
    else
    {
        // Check to see if a mesh was given:
        bool attach = spstate->AttachCoreMesh( race->beardStyles[currentGender][currentChoice]->mesh );

        if ( currentHairColour != -1 )
        {
            // Use the 1. hair colour for the beard
            iMaterialWrapper* material = psengine->LoadMaterial( race->hairColours[currentGender][currentHairColour]->materials[0]->material,
                                                       race->hairColours[currentGender][currentHairColour]->materials[0]->texture  );

            // If the material was found then all is ok
            if (material)
            {
                spstate->SetMaterial( race->beardStyles[currentGender][currentChoice]->mesh, material );
            }
            else // Remove the mesh since there is no valid material for it.
            {
                Warning2( LOG_NEWCHAR, "Material Not Loaded ( No Texture: %s )", race->hairColours[currentGender][currentHairColour]->materials[0]->texture.GetData() );
                spstate->DetachCoreMesh( race->beardStyles[currentGender][currentChoice]->mesh ) ;
            }
        }
    }


    lastAddedBeardStyle = currentChoice;
    beardStyleLabel->SetText( race->beardStyles[currentGender][currentChoice]->name );
*/
}


void pawsCreationMain::ChangeFace( int newFace )
{
    if ( newFace < (int)race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() )
    {
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0);
        Trait* trait = race->location[PSTRAIT_LOCATION_FACE][currentGender][newFace];
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true, true);

        charApp->FaceTexture(trait->material);

        faceLabel->SetText( trait->name );
    }
    else
        faceLabel->SetText(PawsManager::GetSingleton().Translate("Face"));

    //wait for the face to load
    while(!view->ContinueLoad(true))
    {
        csSleep(100);
    }
}


void pawsCreationMain::GrayRaceButtons( )
{
    pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
    for (unsigned id = 0; id < 12; id++)
    {
        pawsRadioButton* button = (pawsRadioButton*)raceBox->FindWidget(id);
        if (button == NULL)
            continue;
        if (!createManager->IsAvailable(id,currentGender))
            button->GetTextBox()->Grayed(true);
        else
            button->GetTextBox()->Grayed(false);
    }
}

void pawsCreationMain::GrayStyleButtons( )
{
    bool faceEnable = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize())
    {
        faceEnable = false;
    }
    faceLabel->Grayed(faceEnable);

    bool hairStyleEnable = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize())
    {
        hairStyleEnable = false;
    }
    hairStyleLabel->Grayed(hairStyleEnable);

    bool beardStyle = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize())
    {
        beardStyle = false;
    }
    beardStyleLabel->Grayed(beardStyle);

    bool hairColour = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize())
    {
        hairColour = false;
    }
    hairColourLabel->Grayed(hairColour);

    bool skinColour = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize())
    {
        skinColour = false;
    }
    skinColourLabel->Grayed(skinColour);
}

void pawsCreationMain::SelectGender(int newGender)
{
    currentGender = newGender;

    GrayRaceButtons();
    GrayStyleButtons();

    if( createManager->GetSelectedRace() == -1)
        return;

    int race = createManager->GetSelectedRace();

    while (!createManager->IsAvailable(race, currentGender))
    {
        race++;
        if (race > 11)
        {
            race = 0;
            pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
            raceBox->TurnAllOff();
            return;
        }

    }

    if (race != createManager->GetSelectedRace())
    {
        //PawsManager::GetSingleton().CreateWarningBox( "That gender isn't implemented for that race yet, sorry." );
        pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
        raceBox->TurnAllOff();
        pawsRadioButton* button = (pawsRadioButton*)raceBox->FindWidget(race);
        button->SetState(true);
    }

    createManager->SetGender( currentGender );

    // Trigger gender update
    UpdateRace(race);
}

bool pawsCreationMain::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
     ////////////////////////////////////////////////////////////////////
     // Previous face.
     ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 1 <") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() == 0 )
                return true;

            currentFaceChoice--;

            if ( currentFaceChoice < 0 )
            {
                currentFaceChoice =  (int)race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize()-1;
            }


            ChangeFace( currentFaceChoice );
        }
        return true;
    }

     ////////////////////////////////////////////////////////////////////
     // Next Set of faces.
     ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 1 >") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() == 0 )
            {
                return true;
            }

            currentFaceChoice++;

            if ( currentFaceChoice == (int)race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() )
            {
                currentFaceChoice = 0;
            }

            ChangeFace( currentFaceChoice );
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////
    // Next Set of hair Styles.
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 2 >") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }

            int old = activeHairStyle;
            activeHairStyle++;

            if ( activeHairStyle == (int)race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() )
            {
                activeHairStyle = 0;
            }

            ChangeHairStyle( activeHairStyle, old );
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////
    // Previous Set of Hair Styles.
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 2 <") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }

            int old = activeHairStyle;
            activeHairStyle--;

            if ( activeHairStyle < 0 )
            {
                activeHairStyle = (int)race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize()-1;
            }

            ChangeHairStyle( activeHairStyle, old );
        }

        return true;
    }


    ////////////////////////////////////////////////////////////////////
    // Next Beard Style.
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 3 >") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }

            currentBeardStyleChoice++;

            if ( currentBeardStyleChoice == (int)race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() )
            {
                currentBeardStyleChoice = 0;
            }


            ChangeBeardStyle( currentBeardStyleChoice );
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////
    // Previous Set of Beard Styles.
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 3 <") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }

            currentBeardStyleChoice--;

            if ( currentBeardStyleChoice < 0 )
            {
                currentBeardStyleChoice = (int)race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize()-1;
            }

            ChangeBeardStyle( currentBeardStyleChoice );
        }

        return true;
    }



    ////////////////////////////////////////////////////////////////////
    // Next Hair Colours.
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 4 >") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() == 0 )
            {
                return true;
            }

            activeHairColour++;

            if ( activeHairColour >= (int)race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() )
            {
                activeHairColour = 0;
            }

            ChangeHairColour( activeHairColour );
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////
    // Previous Set of HairColours
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 4 <") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() == 0 )
                return true;

            activeHairColour--;

            if ( activeHairColour < 0 )
                activeHairColour = (int)race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize()-1;

            ChangeHairColour( activeHairColour );
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////
    // Skin Colours.
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 5 >") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() == 0 )
                return true;

            currentSkinColour++;

            if ( currentSkinColour == (int)race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() )
                currentSkinColour = 0;

            ChangeSkinColour( currentSkinColour );
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////
    // Previous Set of Skin Colours
    ////////////////////////////////////////////////////////////////////
    if ( strcmp( widget->GetName(), "Custom Choice Set 5 <") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() == 0 )
                return true;

            currentSkinColour--;

            if ( currentSkinColour < 0 )
                currentSkinColour = (int)race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize()-1;

            ChangeSkinColour( currentSkinColour );
        }

        return true;
    }



    /////////////////////////////////////////////////////
    // RACE SELECTION BUTTONS
    /////////////////////////////////////////////////////
    if ( widget->GetID() >= 0 && widget->GetID() <= 11 )
    {
        if (
            !createManager->IsAvailable(widget->GetID(),1) &&
            !createManager->IsAvailable(widget->GetID(),2)
            )
        {
            PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(
                                                         "This race isn't implemented yet, sorry"));
            pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");

            if ( lastRaceID != -1 )
            {
                csString raceActive;
                raceActive.Format("race%d", lastRaceID );
                if ( raceBox )
                    raceBox->SetActive( raceActive );
            }
            else
            {
                if ( raceBox )
                    raceBox->TurnAllOff();
            }

            return true;
        }

        if (lastRaceID == -1)
        {
            SelectGender(lastGender);
            currentGender = lastGender;
        }

        // If current gender isn't available, try every possible
        if (!createManager->IsAvailable(widget->GetID(),currentGender))
        {
            if (createManager->IsAvailable(widget->GetID(),PSCHARACTER_GENDER_FEMALE))
                SelectGender(PSCHARACTER_GENDER_FEMALE);
            else if (createManager->IsAvailable(widget->GetID(),PSCHARACTER_GENDER_MALE))
                SelectGender(PSCHARACTER_GENDER_MALE);

            lastGender = currentGender;
        }

        pawsMultiLineTextBox* racedesc = (pawsMultiLineTextBox*)PawsManager::GetSingleton().FindWidget("race_description");
        racedesc->SetText(  createManager->GetRaceDescription( widget->GetID() ) );


        createManager->SetGender(currentGender);
        UpdateRace(widget->GetID());
        ResetAllWindows();
        return true;
    }



    if ( strcmp( widget->GetName(), "randomName" ) == 0 )
    {
        csString firstName;
        csString lastName;

        if ( currentGender == PSCHARACTER_GENDER_FEMALE )
        {
            createManager->GenerateName(NAMEGENERATOR_FEMALE_FIRST_NAME, firstName,5,5);
        }
        else
        {
           createManager->GenerateName(NAMEGENERATOR_MALE_FIRST_NAME, firstName,4,7);
        }

        createManager->GenerateName(NAMEGENERATOR_FAMILY_NAME, lastName,4,7);

        csString upper( firstName );
        upper.Upcase();
        firstName.SetAt(0, upper.GetAt(0) );

        upper = lastName;
        upper.Upcase();
        lastName.SetAt(0, upper.GetAt(0) );

        pawsEditTextBox* lastname = (pawsEditTextBox*)FindWidget("charlastnametext");
        pawsEditTextBox* firstname = (pawsEditTextBox*)FindWidget("charfirstnametext");
	       
	firstname->SetText(firstName);
	lastname->SetText(lastName);
        firstname->SetCursorPosition(0);
        nameWarning = 1; // no need to warn them
    }

    /////////////////////////////////////////////////////
    // END OF RACE BUTTONS
    /////////////////////////////////////////////////////
    switch ( widget->GetID() )
    {
        case MALE_BUTTON:
            maleButton->SetState(true);
            femaleButton->SetState(false);
            SelectGender(PSCHARACTER_GENDER_MALE);
            return true;
        case FEMALE_BUTTON:
            maleButton->SetState(false);
            femaleButton->SetState(true);
            SelectGender(PSCHARACTER_GENDER_FEMALE);
            return true;
        case BACK_BUTTON:
        {
            Hide();
            pawsCharacterPickerWindow* charPicker = (pawsCharacterPickerWindow*)PawsManager::GetSingleton().FindWidget( "CharPickerWindow" );
            charPicker->Show();
            return true;
        }
        case NAMEHELP_BUTTON:
        {
            csString help;
            help = "A first and last name are required. ";
            help += "You can use any alphabetic (A-Z) in your name, but no numbers. ";
            help += "Each name must be between 3 and 27 letters.\n";
            help += "The name should follow the rules here:\n";
            help += "http://www.planeshift.it/naming.html \n";
            help += "If your name is inappropriate, a Game Master (GM) will change it.";
            PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(help));
            return true;
        }
        case NEXT_BUTTON:
        case QUICK_BUTTON:
        {
            // Check to see if a race was selected.
            if ( race == 0 )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please select a race"));
                error.FireEvent();
                return true;
            }

            pawsEditTextBox* lastname = (pawsEditTextBox*)FindWidget("charlastnametext");
            pawsEditTextBox* firstname = (pawsEditTextBox*)FindWidget("charfirstnametext");

	    csString csfirstname = firstname->GetText();
            csString cslastname = lastname->GetText();

            // Check to see a name was entered.
            if ( csfirstname.Length() == 0 )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter a first name"));
                error.FireEvent();
                return true;
            }

            if ( cslastname.Length() == 0 )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter a last name"));
                error.FireEvent();
                return true;
            }

            csString selectedName(csfirstname + " " + cslastname);

            if ( !CheckNameForRepeatingLetters(selectedName) )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "No more than 2 letters in a row allowed"));
                error.FireEvent();
                return true;
            }	    

            // Check the firstname
            if ( csfirstname.Length() < 3 || !FilterName(firstname->GetText()) )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "First name must contain more than two letters and contain only alphabet letters"));
                error.FireEvent();
                return true;
            }

            // Check the lastname
            if ( cslastname.Length() < 3 || !FilterName(lastname->GetText()) )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "Last name must contain more than two letters and contain only alphabet letters"));
                error.FireEvent();
                return true;
            }

            firstname->SetText(NormalizeCharacterName(csfirstname));
            lastname->SetText(NormalizeCharacterName(cslastname));

            // Check to see if they entered their own name.
            if (nameWarning == 0)
            {
                csString policy;
                policy =  "Before clicking next again, please make sure that your character name ";
                policy += "is not offensive and would be appropriate for a medieval setting. ";
                policy += "It must be unique and not similar to that of any real person or thing. ";
                policy += "Names in violation of this policy (www.planeshift.it/naming.html) will ";
                policy += "be changed by authorized people in-game.  ";
                policy += "The game forums can be found at:  http://www.hydlaa.com/smf/";
                PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(policy));

                nameWarning = 1;
                return true;
            }

            // Set our choices in the creation manager
            createManager->SetCustomization( race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_FACE][currentGender][currentFaceChoice]->uid:0,
                                             race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender][activeHairStyle]->uid:0,
                                             race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][currentBeardStyleChoice]->uid:0,
                                             race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender][activeHairColour]->uid:0,
                                             race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender][currentSkinColour]->uid:0 );


            if ( widget->GetID() == NEXT_BUTTON )
            {
                createManager->GetChildhoodData();
                newWindow = "CharBirth";
            }
            else
            {
                newWindow = "Paths";
            }

            psNameCheckMessage msg(csfirstname, cslastname);
            msg.SendMessage();
        }
    }

    return false;
}

bool pawsCreationMain::OnChange(pawsWidget *widget)
{
    // If we click on the name box (and we change it), then we might get the name policy warning again
    if (widget == lastnameTextBox || widget == firstnameTextBox)
        nameWarning=0;
    return true;
}
void pawsCreationMain::Show()
{
    // Play some music
    psengine->GetSoundManager()->LoadActiveSector("charcreation");

    pawsWidget::Show();
    UpdateCP();
}

void pawsCreationMain::UpdateRace(int id)
{
    loaded = false;
    int raceCP = createManager->GetRaceCP( id );
    lastRaceID = id;

    if ( raceCP != REQUESTING_CP && createManager->GetSelectedRace() != id )
    {
        // Reset CP only when changing the race
        createManager->SetCurrentCP( raceCP );
        UpdateCP();
    }

    createManager->SetRace( id );
    race = createManager->GetRace(id);

    currentGender = createManager->GetSelectedGender();

    if ( lastGender != -1 )
    {
        currentGender = lastGender;
    }

    createManager->SetGender( currentGender );
    lastGender = -1;

    // Store the factory name for the selected model.
    factName = createManager->GetModelName( id, currentGender );

    // Show the model for the selected race.
    view->Show();
    view->EnableMouseControl(true);

    //temporary hardcoding needs removal
    maleButton->SetEnabled(id != 9);
    femaleButton->SetEnabled(id != 9);
    if(id == 9)
    {
        maleButton->SetState(false);
        femaleButton->SetState(false);
    }
    else
    {
        maleButton->SetState(createManager->GetSelectedGender() == PSCHARACTER_GENDER_MALE);
        femaleButton->SetState(createManager->GetSelectedGender() == PSCHARACTER_GENDER_FEMALE);
    }


    CheckLoadStatus();
}

bool pawsCreationMain::CheckLoadStatus()
{
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

            currentFaceChoice = 0;
            activeHairStyle = 0;
            currentBeardStyleChoice = 0;
            activeHairColour = 0;
            currentSkinColour = 0;

            ChangeFace(0);
            ChangeHairColour(0);
            SetHairStyle( activeHairStyle );
            ChangeSkinColour( currentSkinColour );
            ChangeBeardStyle( currentBeardStyleChoice );

            view->UnlockCamera();

            GrayStyleButtons();
            loaded = true;
        }
    }

    return false;
}


bool FilterName(const char* name)
{
    if (name == NULL)
        return false;

    size_t len = strlen(name);

    if ( (strspn(name,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
                != len) )
    {
        return false;
    }

    if (((int)strspn(((const char*)name)+1,
                     (const char*)"abcdefghijklmnopqrstuvwxyz") != (int)len - 1))
    {
        return false;
    }

    return true;
}

// Stolen from libsserverobjects - psCharacter
csString NormalizeCharacterName(const csString & name)
{
    csString normName = name;
    normName.Downcase();
    if (normName.Length() > 0)
        normName.SetAt(0,toupper(normName.GetAt(0)));
    return normName;
}

void pawsCreationMain::UpdateCP()
{
    int cp = createManager->GetCurrentCP();
    if (cp > 300 || cp < -300 ) // just some random values
        cp=0;

    char buff[10];
    sprintf( buff, "CP: %d", cp );
    cpPoints->SetText( buff );
}

bool CheckNameForRepeatingLetters(csString name)
{
    name.Downcase();

    char letter = ' ';
    int count = 1;

    for (size_t i=0; i<name.Length(); i++)
    {
        if (name[i] == letter)
        {
            count++;
            if (count > 2)
                return false;  // More than 2 of the same letter in a row
        }
        else
        {
            count = 1;
            letter = name[i];
        }
    }

    // No more than 2 of one letter in a row
    return true;
}
