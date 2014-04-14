/*
 * pawspath.cpp - Author: Andrew Craig
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

#include "paws/pawsbutton.h"
#include "paws/pawsyesnobox.h"
#include "pawspath.h"
#include "pawssummary.h"
#include "paws/pawsmanager.h"

////////////////////////////////////////////////////////////////////////////////
#define PATH_0 1000
#define RANDOM_BUTTON 4000
#define UPLOAD_BUTTON 5000
#define BACK_BUTTON 6000
////////////////////////////////////////////////////////////////////////////////

#define NO_CHOSEN_PATH -1

pawsPathWindow::pawsPathWindow()
{
    createManager = psengine->GetCharManager()->GetCreation();
    charCreateMain = (pawsCreationMain*) PawsManager::GetSingleton().FindWidget("CharCreateMain");
}

pawsPathWindow::~pawsPathWindow()
{
}

bool pawsPathWindow::PostSetup()
{
    int i = 1;
    csPDelArray<PathDefinition>::Iterator iter = createManager->GetPathIterator();
    while(iter.HasNext())
    {
        PathDefinition* path = iter.Next();
        pawsButton* widget = (pawsButton*) FindWidget(PATH_0 + i);
        widget->SetText(path->name);
        i++;
    }
    chosenPath = NO_CHOSEN_PATH;
    LabelHeaderVisibility(false);
    return true;
}

bool pawsPathWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    int id = widget->GetID();
    if (id <= PATH_0 + 6 && id > PATH_0)
    {
        SetPath(id - 1001);
        return true;
    }

    switch ( id )
    {   
        case BACK_BUTTON:
        {
            ClearPath();
            Hide();
            charCreateMain->Show();
            return true;
        }      
        case UPLOAD_BUTTON:
        {
            if (chosenPath == NO_CHOSEN_PATH)
            {
                psSystemMessage error(0,MSG_ERROR,
                                      PawsManager::GetSingleton().Translate("Please select a path"));
                error.FireEvent();
            }
            else
            {
                PawsManager::GetSingleton().CreateYesNoBox(
                    PawsManager::GetSingleton().Translate("Are you sure you want to upload your character?"),
                    this );
            }
            return true;
        }
        case RANDOM_BUTTON:
        {
            int i = psengine->GetRandomGen().Get(6) + 1001;
            //pawsButton* button = (pawsButton*) FindWidget(i);
            SetPath(i - 1001);
            return true;
        }
        case CONFIRM_YES:
        {
            Hide();
            createManager->UploadChar();
        }
    }
    
    return false;
}

void pawsPathWindow::SetPath(int i)
{
    chosenPath = i;

    charCreateMain->ResetAllWindows();
    PathDefinition* path = createManager->GetPath(i);
    createManager->SetPath( path->name );
    pawsMultiLineTextBox* label = (pawsMultiLineTextBox*) FindWidget("label_description");
    label->SetText(path->info);
    
 {   
        // put something in parents
        pawsMultiLineTextBox* label = (pawsMultiLineTextBox*) FindWidget("label_parents");
        label->SetText(path->parents);
    
}


{
        // put something in life
        pawsMultiLineTextBox* label = (pawsMultiLineTextBox*) FindWidget("label_life");
        label->SetText(path->life);

}

    {
        // Show Stats
        label = (pawsMultiLineTextBox*) FindWidget("label_stat");
        csPDelArray<PathDefinition::Bonus>::Iterator iter = path->statBonuses.GetIterator();
        csString statString;
        csString valueString;

        while(iter.HasNext())
        {
            PathDefinition::Bonus* bonus = iter.Next();
            int percent = (int) (bonus->value * 100.0);
            statString.AppendFmt("%s\n", bonus->name.GetData());
            valueString.AppendFmt("%d%%\n", percent);
        }

        label->SetText(statString);
        label = (pawsMultiLineTextBox*) FindWidget("values_stat");
        label->SetText(valueString);

    }

    {
        // Show Skills
        label = (pawsMultiLineTextBox*) FindWidget("label_skill");
        csPDelArray<PathDefinition::Bonus>::Iterator iter = path->skillBonuses.GetIterator();
        csString skillString;
        csString valueString;

        while(iter.HasNext())
        {
            PathDefinition::Bonus* bonus = iter.Next();
            skillString.AppendFmt("%s\n", bonus->name.GetData());
            valueString.AppendFmt("%.f\n", bonus->value);
        }

        label->SetText(skillString);
        label = (pawsMultiLineTextBox*) FindWidget("values_skill");
        label->SetText(valueString);
    }

    {
        // Show path symbol
        pawsWidget * symbol = FindWidget("PathSymbol");
        if (symbol != NULL)
            symbol->SetBackground("PathSymbol " + path->name);
        if (!symbol->IsVisible())
            symbol->Show();
    }
    
    LabelHeaderVisibility(true);
}

void pawsPathWindow::LabelHeaderVisibility(bool visible)
{
    {
        pawsWidget * label = FindWidget("label_parents_header");
        if (label)
        {
            if(visible) label->Show();
            else        label->Hide();
        }
    }
    
    {
        pawsWidget * label = FindWidget("label_life_header");
        if (label)
        {
            if(visible) label->Show();
            else        label->Hide();
        }
    }

    {
        pawsWidget * label = FindWidget("label_stat_header");
        if (label)
        {
            if(visible) label->Show();
            else        label->Hide();
        }
    }
    
    {
        pawsWidget * label = FindWidget("values_stat_header");
        if (label)
        {
            if(visible) label->Show();
            else        label->Hide();
        }
    }

    {
        pawsWidget * label = FindWidget("label_skill_header");
        if (label)
        {
            if(visible) label->Show();
            else        label->Hide();
        }
    }
    
    {
        pawsWidget * label = FindWidget("values_skill_header");
        if (label)
        {
            if(visible) label->Show();
            else        label->Hide();
        }
    }
}

void pawsPathWindow::ClearPath(void)
{
    chosenPath = NO_CHOSEN_PATH;

    createManager->SetPath( "None" );

    pawsMultiLineTextBox* label = (pawsMultiLineTextBox*) FindWidget("label_description");
    label->SetText("");
    label = (pawsMultiLineTextBox*) FindWidget("label_stat");
    label->SetText("");
    label = (pawsMultiLineTextBox*) FindWidget("values_stat");
    label->SetText("");
    label = (pawsMultiLineTextBox*) FindWidget("label_skill");
    label->SetText("");
    label = (pawsMultiLineTextBox*) FindWidget("values_skill");
    label->SetText("");
    label = (pawsMultiLineTextBox*) FindWidget("label_parents");
    label->SetText("");
    label = (pawsMultiLineTextBox*) FindWidget("label_life");
    label->SetText("");

    pawsWidget * symbol = FindWidget("PathSymbol");
    symbol->Hide();
    LabelHeaderVisibility(false);
}

