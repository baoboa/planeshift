/*
 * pawssummary.h - Author: Andrew Craig
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
#ifndef PAWS_CHAR_CREATE_SUMMARY_WINDOW_HEADER
#define PAWS_CHAR_CREATE_SUMMARY_WINDOW_HEADER
 
#include "paws/pawswidget.h"
#include "paws/pawstextbox.h"
#include "psengine.h"

class psCreationManager;
class pawsObjectView;
class psCharAppearance;
 
class pawsSummaryWindow : public pawsWidget, public DelayedLoader
{
public:
    pawsSummaryWindow();
    ~pawsSummaryWindow(); 
    bool OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget );
    void Update();
    bool PostSetup();
    
    void Draw();
    void Show();
    void SetVerify( csArray<psCharVerificationMesg::Attribute> stats,
                    csArray<psCharVerificationMesg::Attribute> skill );
       
    ///loads the mesh in the char view
    bool CheckLoadStatus();

private:  
    psCreationManager* createManager;
    /// Toggles if the information on display is dirty.   
    bool redoVerification;
    
    /// Toggles if we've already send a request for character information.
    bool requestSent;
    
    /// List of stat values that the new character will have.
    pawsListBox*  statsList;
    
    /// List of skill values that the new character will have.
    pawsListBox*  skillsList;
    
    ///retains if the mesh has been loaded
    bool loaded;
    
    /// Paper doll view of model.
    pawsObjectView* view;
    
    /// Character apparence data
    psCharAppearance* charApp;
    
    
    pawsTextBox* serverStatus;
};

CREATE_PAWS_FACTORY( pawsSummaryWindow ); 
 
#endif


