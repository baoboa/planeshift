/*
 * pawschild.cpp - Author: Andrew Craig
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
#include "pawschild.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawscombo.h"

#include "net/charmessages.h"
#include "globals.h"

////////////////////////////////////////////////////////////////////////////////
#define NEXT_BUTTON 5000
#define BACK_BUTTON 6000

#define EVENT_LISTBOX       1
#define ACTIVITY_LISTBOX    2
#define HOUSE_LISTBOX       3
////////////////////////////////////////////////////////////////////////////////

pawsChildhoodWindow::pawsChildhoodWindow()
{
    createManager = psengine->GetCharManager()->GetCreation();
   
    dataLoaded = false;
    
    lastEventChoice     = 0;
    lastActivityChoice  = 0;
    lastHouseChoice     = 0;
}

pawsChildhoodWindow::~pawsChildhoodWindow()
{
}

void pawsChildhoodWindow::Show()
{
    UpdateCP();
    pawsWidget::Show();    
}

bool pawsChildhoodWindow::PostSetup()
{
    eventDesc = (pawsMultiLineTextBox*)FindWidget( "bdesc" );    
    if ( !eventDesc )
        return false;
        
    activityDesc = (pawsMultiLineTextBox*)FindWidget( "adesc" );    
    if ( !activityDesc )
        return false;
        
                        
    houseDesc = (pawsMultiLineTextBox*)FindWidget("hdesc");
    if ( !houseDesc )
        return false;
    
    /*        
    siblingsDesc = (pawsMultiLineTextBox*)FindWidget("sdesc");
    if ( !siblingsDesc )
        return false;
    */        
                
    cpBox = (pawsTextBox*)FindWidget("CP");
    if ( !cpBox )
        return false;

    return true;
}


bool pawsChildhoodWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    
    if (!strcmp(widget->GetName(),"randomize"))
    {
        Randomize();
        return true;
    }

    switch ( widget->GetID() )
    {
        
        case BACK_BUTTON:
        {
            Hide();
            PawsManager::GetSingleton().FindWidget( "Parents" )->Show();
            return true;
        }
        
        case NEXT_BUTTON:
        {
            if (lastActivityChoice == 0 || lastEventChoice == 0 || lastHouseChoice == 0)
            {
                psSystemMessage error(0,MSG_ERROR,"Please fill in your childhood");
                error.FireEvent();
                return true;
            }
            Hide();
            createManager->GetLifeEventData();
            PawsManager::GetSingleton().FindWidget( "LifeEvents" )->Show();
            return true;
        }
      
    }
    
    return false;
}

void pawsChildhoodWindow::PopulateFields()
{    
    UpdateCP();
    for (size_t x=0; x < createManager->childhoodData.GetSize(); x++ )
    {
      
        switch ( createManager->childhoodData[x].choiceArea )
        {
            case BIRTH_EVENT:
            {
                pawsListBox* listbox = (pawsListBox*)FindWidget( "bevent" );
                pawsListBoxRow* row = listbox->NewRow();
                row->SetID( createManager->childhoodData[x].id );
                pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
                name->SetText( createManager->childhoodData[x].name  );   
                name->FormatToolTip("CP Cost: %d\n", createManager->childhoodData[x].cpCost );
                
                break;                  
            }
            
            case CHILD_ACTIVITY:
            {
                pawsListBox* listbox = (pawsListBox*)FindWidget( "activity" );
                pawsListBoxRow* row = listbox->NewRow();
                row->SetID( createManager->childhoodData[x].id );
                pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
                name->SetText( createManager->childhoodData[x].name  );          
                name->FormatToolTip("CP Cost: %d\n", createManager->childhoodData[x].cpCost );     
                break;
            }
            
            case CHILD_HOUSE:
            {
                
                pawsListBox* houseBox = (pawsListBox*)FindWidget("house");
                pawsListBoxRow* row = houseBox->NewRow();
                row->SetID( createManager->childhoodData[x].id );
                pawsTextBox* name = (pawsTextBox*)row->GetColumn(0);
                name->SetText( createManager->childhoodData[x].name  );   
                name->FormatToolTip("CP Cost: %d\n", createManager->childhoodData[x].cpCost );
                break;
            }          
        }   
    }      
}

void pawsChildhoodWindow::Randomize()
{
    pawsListBox* birthBox = (pawsListBox*)FindWidget("bevent");
    pawsListBox* activityBox = (pawsListBox*)FindWidget("activity");
    pawsListBox* houseBox = (pawsListBox*)FindWidget("house");

    birthBox->Select(birthBox->GetRow(psengine->GetRandomGen().Get(birthBox->GetRowCount())));
    activityBox->Select(activityBox->GetRow(psengine->GetRandomGen().Get(activityBox->GetRowCount())));
    houseBox->Select(houseBox->GetRow(psengine->GetRandomGen().Get(houseBox->GetRowCount())));
    UpdateCP();

    // Update the choices made
    OnListAction(birthBox,LISTBOX_HIGHLIGHTED);
    OnListAction(activityBox,LISTBOX_HIGHLIGHTED);
    OnListAction(houseBox,LISTBOX_HIGHLIGHTED);
}

void pawsChildhoodWindow::Draw()
{
    pawsWidget::Draw();
    // Check to see if we are waiting for data from the server. Should have a waiting
    // cursor if this fails.     
    if (!dataLoaded && createManager->HasChildhoodData())
    {
        PopulateFields();
        dataLoaded = true;                
    }
}


void pawsChildhoodWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        // Figure out which list box was checked and act accordingly
        pawsListBoxRow* row = widget->GetSelectedRow();            
        
        switch ( widget->GetID() )
        {
            case EVENT_LISTBOX:
            {
                 
                createManager->RemoveChoice( lastEventChoice );
                createManager->AddChoice( row->GetID() );
                lastEventChoice = row->GetID();
                
                eventDesc->SetText( createManager->GetDescription(row->GetID()) );
                break;        
            }        
            case ACTIVITY_LISTBOX:
            {
     
                createManager->RemoveChoice( lastActivityChoice );
                createManager->AddChoice( row->GetID() );
                lastActivityChoice = row->GetID();        
                
                activityDesc->SetText( createManager->GetDescription(row->GetID()) );
                break;        
            }        

            case HOUSE_LISTBOX:
            {   
     
                createManager->RemoveChoice( lastHouseChoice );
                createManager->AddChoice( row->GetID() );
                lastHouseChoice = row->GetID();                 
                
                houseDesc->SetText( createManager->GetDescription(row->GetID()) );
                break;
            }
        }   
        UpdateCP();
    };
}



void pawsChildhoodWindow::UpdateCP()
{
    int cp = createManager->GetCurrentCP();
    
    char buff[10];
    sprintf( buff, "CP: %d", cp );
    cpBox->SetText( buff );
}    
