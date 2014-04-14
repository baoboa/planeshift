/*
 * pawslife.cpp - Author: Andrew Craig
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
#include "globals.h"
#include "psclientchar.h"

#include "paws/pawsmanager.h"
#include "paws/pawsselector.h"
#include "paws/pawslistbox.h"
#include "paws/pawstextbox.h"

#include "pawslife.h"
#include "pawssummary.h"

////////////////////////////////////////////////////////////////////////////////
#define NEXT_BUTTON 5000
#define BACK_BUTTON 6000
////////////////////////////////////////////////////////////////////////////////


pawsLifeEventWindow::pawsLifeEventWindow()
{
    dataLoaded = false;
    createManager = psengine->GetCharManager()->GetCreation();    
}

pawsLifeEventWindow::~pawsLifeEventWindow()
{
}


bool pawsLifeEventWindow::PostSetup()
{
    choiceSelection = (pawsSelectorBox*)FindWidget("Life Event");
    if ( choiceSelection == NULL )
        return false;  
        
    cpBox = (pawsTextBox*)FindWidget("CP");
    if ( !cpBox )
        return false;   
                
    choiceDesc = (pawsMultiLineTextBox*)FindWidget("EventDesc");
    if ( !choiceDesc )
        return false;
        
    selectedDesc = (pawsMultiLineTextBox*)FindWidget("SelEventDesc");
    if ( !selectedDesc )
        return false;
        
    return true;     
}


bool pawsLifeEventWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if ( strcmp(widget->GetName(),"randomize") == 0 )
    {
        Randomize();
        return true;
    }

    switch ( widget->GetID() )
    {
        case SELECTOR_ADD_BUTTON:
        {
            AddLifeEventAndDependencies();
            return true;
        }
        
        case SELECTOR_REMOVE_BUTTON:
        {
            pawsListBoxRow* moved = choiceSelection->GetMoved();    
            int id = moved->GetID();
            createManager->RemoveLifeEvent( id );
            LifeEventChoice* event = createManager->FindLifeEvent( id );
            
            if (event != NULL)
            {
                // remove this choice and any of the ones it adds
                for (size_t ridx = 0; ridx < event->adds.GetSize(); ridx++ )
                {
                    choiceSelection->RemoveFromAvailable( event->adds[ridx] );
                    choiceSelection->RemoveFromSelected( event->adds[ridx] );
                    createManager->RemoveLifeEvent( event->adds[ridx] );
                }                                    
                            
                if ( id != -1 )
                {
                    for (size_t ridx = 0; ridx < event->removes.GetSize(); ridx++ )
                    {
                        LifeEventChoice* addevent = createManager->FindLifeEvent( event->removes[ridx] );
                        if (addevent != NULL)
                        {
                            pawsListBoxRow* row = choiceSelection->CreateOption();         
                            pawsTextBox* text = (pawsTextBox*)row->GetColumn(0);
                            text->SetText( addevent->name );
                            row->SetID( addevent->id );
                        }
                        else
                            Error2("Failed to find life event %i", event->removes[ridx]);
                    }
                }
            }
            else
                Error2("Failed to find life event %i", id);
            UpdateCP();
            return true;                        
        }
               
        case BACK_BUTTON:
        {
            Hide();
            PawsManager::GetSingleton().FindWidget( "Childhood" )->Show();
            return true;
        }
        
        case NEXT_BUTTON:
        {
            if (createManager->GetNumberOfLifeChoices() == 0)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please choose life event(s)"));
                error.FireEvent();
                return true;
            }
            Hide();
            pawsSummaryWindow* window = (pawsSummaryWindow*)PawsManager::GetSingleton().FindWidget( "Summary" );
            window->Update();
            window->Show();
            return true;
        }
      
    }
    
    return false;
}


void pawsLifeEventWindow::Draw()
{
    pawsWidget::Draw();

    // Check to see if we are waiting for data from the server. Should have a waiting
    // cursor if this fails.
    if (!dataLoaded && createManager->HasLifeEventData())
    {
        PopulateFields();
        dataLoaded = true;
    }
}

void pawsLifeEventWindow::PopulateFields()
{
    for (size_t x=0; x < createManager->lifeEventData.GetSize(); x++ )
    {
        if ( createManager->lifeEventData[x].common == 'Y' )
        {
            pawsListBoxRow* row = choiceSelection->CreateOption();
            pawsTextBox* text = (pawsTextBox*)row->GetColumn(0);
            text->SetText( createManager->lifeEventData[x].name );
            row->SetID( createManager->lifeEventData[x].id );
            text->FormatToolTip("%s %d\n", PawsManager::GetSingleton().Translate("CP Cost:").GetData(), createManager->lifeEventData[x].cpCost );
        }
    }
    UpdateCP();
}


void pawsLifeEventWindow::UpdateCP()
{
    int cp = createManager->GetCurrentCP();
    
    char buff[10];
    sprintf( buff, "CP: %d", cp );
    cpBox->SetText( buff );
} 

void pawsLifeEventWindow::Show()
{
    pawsWidget::Show();
    UpdateCP();  
}

void pawsLifeEventWindow::OnListAction(  pawsListBox* widget, int status )
{
    choiceDesc->SetText("");
    selectedDesc->SetText("");
    pawsListBoxRow* row = NULL;
    
    if ( status==LISTBOX_HIGHLIGHTED )
    {      
        row = widget->GetSelectedRow();
        if (!row )
            return;
        
        choiceDesc->SetText( createManager->GetLifeEventDescription( row->GetID() ) );            

    }                       
}

void pawsLifeEventWindow::Randomize(void)
{
    int availableLifeEventsCount = choiceSelection->GetAvailableCount();

    if (availableLifeEventsCount)
    {
        int randomLifeEvent = (int)psengine->GetRandomGen().Get(availableLifeEventsCount);
        if (choiceSelection->SelectAndMoveRow(randomLifeEvent))
            AddLifeEventAndDependencies();
    }
}

/// Adds a 'moved' life event to a characters generation, and resolves dependent
/// life events
void pawsLifeEventWindow::AddLifeEventAndDependencies (void)
{
    pawsListBoxRow* moved = choiceSelection->GetMoved();    
    int id = moved->GetID();
    createManager->AddLifeEvent( id );

    if ( id != -1 )
    {
        LifeEventChoice* event = createManager->FindLifeEvent( id );
        if (event != NULL)
        {
            // Remove the ones this choices removes
            for (size_t ridx = 0; ridx < event->removes.GetSize(); ridx++ )
            {
                choiceSelection->RemoveFromAvailable( event->removes[ridx] );
            }                                    
    
            // Add in the new option this choices opens up                
            for (size_t aidx = 0; aidx < event->adds.GetSize(); aidx++ )
            {
                LifeEventChoice* addevent = createManager->FindLifeEvent( event->adds[aidx] );
                if (addevent != NULL)
                {
                    pawsListBoxRow* row = choiceSelection->CreateOption();         
                    pawsTextBox* text = (pawsTextBox*)row->GetColumn(0);
                    text->SetText( addevent->name );
                    row->SetID( addevent->id );                                    
                }
                else
                    Error2("Failed to find life event %i", event->adds[aidx]);
            }                                    
        }
        else
            Error2("Failed to find life event %i", id);
    }

    UpdateCP();
}

