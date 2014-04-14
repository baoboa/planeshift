/*
 * pawscharbirth.cpp - Author: Andrew Dai
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

#include "paws/pawsradio.h"
#include "paws/pawslistbox.h"
#include "paws/pawscombo.h"
#include "paws/pawstextbox.h"
#include "paws/pawsbutton.h"
#include "pawscharbirth.h"
#include "paws/pawsmanager.h"

#include "util/psxmlparser.h"

////////////////////////////////////////////////////////////////////////////////
#define MONTH_COMBOBOX      5
#define NEXT_BUTTON 5000
#define BACK_BUTTON 6000
////////////////////////////////////////////////////////////////////////////////


pawsCharBirth::pawsCharBirth():
    cpBox(NULL),months(NULL),days(NULL)
{
    createManager = psengine->GetCharManager()->GetCreation();
    lastSiblingsChoice = -1;
    lastZodiacChoice = -1;
    sibCount = 0;
    dataLoaded = false;
}

pawsCharBirth::~pawsCharBirth()
{
}

bool pawsCharBirth::PostSetup()
{
    cpBox = (pawsTextBox*)FindWidget("CP");
    if ( !cpBox )
        return false;        
        
    months = (pawsComboBox*)FindWidget("month");
    if (!months)
        return false;

    // add the days to the list
    days = (pawsComboBox*)FindWidget("day");
    if (!days)
        return false;
    
    for (int i = 1; i <= 32;i++)
    {
        csString date;
        date.Format("%d",i);
        if (date.Length() == 1)
            date = "0" + date;

        days->NewOption(date);
    }

    // Load the zodiacs
    csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(),"/this/data/gui/zodiacs.xml");
    if (!doc)
    {
        //error message will be shown in ParseFile
        return false;
    }
    csRef<iDocumentNode> root = doc->GetRoot();    
    if (!root)
    {
        Error1("Couldn't find any root node in zodiacs.xml!");
        return false;
    }

    csRef<iDocumentNode> zodiacs = root->GetNode("zodiacs");
    if (!zodiacs)
    {
        Error1("Couldn't find any zodiacs node in zodiacs.xml!");
        return false;        
    }

    csRef<iDocumentNodeIterator> zodiacsIre = zodiacs->GetNodes();

    while(zodiacsIre->HasNext())
    {
        csRef<iDocumentNode> node = zodiacsIre->Next();

        if (!strcmp(node->GetValue(),"zodiac"))
        {
            Zodiac* zodiac = new Zodiac;
            zodiac->name = node->GetAttributeValue("name");
            zodiac->img = node->GetAttributeValue("img");
            zodiac->desc = node->GetContentsValue();
            zodiac->month = node->GetAttributeValueAsInt("month");
            this->zodiacs.Push(zodiac);

            // Add the month to the list
            months->NewOption(zodiac->name)->SetName(zodiac->name);
        }
    }

    return true;
}

bool pawsCharBirth::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
        /// Check to see if it is one of the sibling buttons
    if ( strncmp( widget->GetName(), "sibling", 7 ) == 0 )
    {
        if (lastSiblingsChoice != -1)
            createManager->RemoveChoice( lastSiblingsChoice );
        createManager->AddChoice( widget->GetID() );
        lastSiblingsChoice = widget->GetID();                           
        pawsMultiLineTextBox* siblingsDesc = (pawsMultiLineTextBox*) FindWidget("sibling_desc");
        siblingsDesc->SetText( createManager->GetDescription(widget->GetID()) );
        return true;        
    } 

    if ( strcmp(widget->GetName(),"randomize") == 0 )
    {
        Randomize();
        return true;
    }

    int id = widget->GetID();
    switch ( id )
    {   
    case BACK_BUTTON:
        {
            Hide();
            PawsManager::GetSingleton().FindWidget( "CharCreateMain" )->Show();
            return true;
        }
    case NEXT_BUTTON:
        {
            if (months->GetSelectedRowNum() == -1)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please choose your birth month"));
                error.FireEvent();
                return true;
            }

            if (days->GetSelectedRowNum() == -1)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please choose your birthday"));
                error.FireEvent();
                return true;
            }

            if (lastSiblingsChoice == -1)
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please choose your sibling count"));
                error.FireEvent();
                return true;
            }

            createManager->GetParentData();
            Hide();
            PawsManager::GetSingleton().FindWidget( "Parents" )->Show();
            return true;
        }
    }
    return false;
}

void pawsCharBirth::Randomize()
{
    pawsListBoxRow * row = months->Select(psengine->GetRandomGen().Get(months->GetRowCount()));
    if ( lastZodiacChoice != -1 )
    {
        createManager->RemoveChoice( lastZodiacChoice );            
    }
                                    
    createManager->AddChoice( row->GetID() );
    lastZodiacChoice = row->GetID();
    
    days->Select(psengine->GetRandomGen().Get(days->GetRowCount()));
   
    char widgetName[100];                
    sprintf( widgetName, "sibling%d", psengine->GetRandomGen().Get(sibCount) );
    pawsRadioButton* widget = (pawsRadioButton*) FindWidget(widgetName);
    pawsRadioButtonGroup* group = (pawsRadioButtonGroup*) FindWidget("Siblings");
    group->SetActive(widgetName);
    
    // Choose a random one to begin with.
    if (lastSiblingsChoice != -1)
        createManager->RemoveChoice( lastSiblingsChoice );      
    createManager->AddChoice( widget->GetID() );
    lastSiblingsChoice = widget->GetID();                           
    pawsMultiLineTextBox* siblingsDesc = (pawsMultiLineTextBox*) FindWidget("sibling_desc");
    siblingsDesc->SetText( createManager->GetDescription(widget->GetID()) );
    
    UpdateCP();
}

void pawsCharBirth::PopulateFields()
{    
    sibCount = 0;
    for (size_t x=0; x < createManager->childhoodData.GetSize(); x++ )
    {
        if ( createManager->childhoodData[x].choiceArea == CHILD_SIBLINGS)
        {      
            char widgetName[100];                
            sprintf( widgetName, "sibling%d", sibCount );                
            pawsRadioButton* button = (pawsRadioButton*)FindWidget( widgetName );
            button->SetID( createManager->childhoodData[x].id );
            button->Show();
            pawsTextBox* name = button->GetTextBox();
            name->SetText( createManager->childhoodData[x].name.GetData() );
            sibCount++;                                      
        }             
        if ( createManager->childhoodData[x].choiceArea == ZODIAC )
        {
            csString name( createManager->childhoodData[x].name );
            
            pawsListBoxRow* row = (pawsListBoxRow*)FindWidget( name );
            if ( row )
            {
                row->SetID( createManager->childhoodData[x].id );               
                csString buff;
                buff.Format( "CP: %d\n", createManager->childhoodData[x].cpCost );
                row->SetToolTip( buff );
            }                
        }                                   
    }    
}

void pawsCharBirth::Draw()
{
    pawsWidget::Draw();
    // Check to see if we are waiting for data from the server. Should have a waiting
    // cursor if this fails.     
    if (!dataLoaded && createManager->HasChildhoodData())
    {
        PopulateFields();

        //set first item in month and day
        months->Select(0);
        days->Select(0);

        dataLoaded = true;
    }
}

void pawsCharBirth::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        // Figure out which list box was checked and act accordingly
        pawsListBoxRow* row = widget->GetSelectedRow();            

        if (widget->GetID() == MONTH_COMBOBOX)
        {
            pawsWidget* imgBox = FindWidget("zodiac_img");
            pawsTextBox* textBox = (pawsTextBox*)FindWidget("zodiac");
            pawsMultiLineTextBox* descBox = (pawsMultiLineTextBox*)FindWidget("zodiac_description");

            if (!imgBox || !textBox || !descBox)
                return;

            Zodiac* zodiac = GetZodiac(widget->GetSelectedRowNum()+1);
            if (!zodiac)
            {
                Error2("No zodiac found for month number %d!",widget->GetSelectedRowNum()+1);
                return;
            }

            imgBox->SetBackground(zodiac->img);
            imgBox->SetBackgroundAlpha(0);
            textBox->SetText(zodiac->name);
            descBox->SetText(zodiac->desc);
            
            if ( lastZodiacChoice != -1 )
            {
                createManager->RemoveChoice( lastZodiacChoice );            
            }
                                    
            createManager->AddChoice( row->GetID() );
            lastZodiacChoice = row->GetID();
        
            UpdateCP();                        
        }
    }
    UpdateCP();
}

void pawsCharBirth::UpdateCP()
{
    int cp = createManager->GetCurrentCP();
    
    char buff[10];
    sprintf( buff, "CP: %d", cp );
    cpBox->SetText( buff );
}    

void pawsCharBirth::Show()
{
    pawsWidget::Show();
    UpdateCP();  
}

Zodiac* pawsCharBirth::GetZodiac(unsigned int month)
{
    for (unsigned int i = 0; i < (unsigned int)zodiacs.GetSize();i++)
    {
        Zodiac* zodiac = zodiacs[i];
        if (zodiac->month == month)
            return zodiac;
    }
    return NULL;
}
