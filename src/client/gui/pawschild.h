/*
 * pawschild.h - Author: Andrew Craig
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
#ifndef PAWS_CHILDHOOD_WINDOW_HEADER
#define PAWS_CHILDHOOD_EVENT_WINDOW_HEADER
 
#include "paws/pawswidget.h"
#include "psclientchar.h"


class pawsMultiLineTextBox;
class pawsTextBox;

/** This is the character creation screen where player selects childhood 
  * details.
  *
  *  This screen has sections for:
  *     The Event on your birth day
  *     The main activity you did as a child
  *     The house you grew up in.
  */
class pawsChildhoodWindow : public pawsWidget
{
public:
    pawsChildhoodWindow();
    ~pawsChildhoodWindow(); 
    bool PostSetup();
    void OnListAction( pawsListBox* widget, int status );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    virtual void Draw();
    void Randomize();

    virtual void Show();
private:
    void PopulateFields();
    void UpdateCP();
    
    psCreationManager* createManager;
    
    pawsMultiLineTextBox* eventDesc;
    pawsMultiLineTextBox* activityDesc;
    pawsMultiLineTextBox* houseDesc;
    
    pawsTextBox* cpBox;
    
    bool dataLoaded;
    
    int lastEventChoice;
    int lastActivityChoice;
    int lastHouseChoice;    
};

CREATE_PAWS_FACTORY( pawsChildhoodWindow ); 
 
#endif



