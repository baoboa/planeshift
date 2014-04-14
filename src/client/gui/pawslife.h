/*
 * pawslife.h - Author: Andrew Craig
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
#ifndef PAWS_LIFE_EVENT_WINDOW_HEADER
#define PAWS_LIFE_EVENT_WINDOW_HEADER
 
#include "paws/pawswidget.h"

class pawsSelectorBox;
class psCreationManager;
class pawsTextBox;
class pawsMultiLineTextBox;

 
class pawsLifeEventWindow : public pawsWidget
{
public:
    pawsLifeEventWindow();
    ~pawsLifeEventWindow();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget ); 
    
    void Show();
    virtual void Draw();
    virtual void OnListAction(  pawsListBox* widget, int status );
    
    bool PostSetup();
    
private:
    psCreationManager* createManager;
    pawsTextBox* cpBox;
    bool dataLoaded;
    
    pawsSelectorBox* choiceSelection;
    
    pawsMultiLineTextBox* selectedDesc;
    pawsMultiLineTextBox* choiceDesc;
    
    void PopulateFields();
    void UpdateCP();
    
    void Randomize(void);
    void AddLifeEventAndDependencies(void);
};

CREATE_PAWS_FACTORY( pawsLifeEventWindow ); 
 
#endif


