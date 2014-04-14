/*
 * pawsdetailwindow.h - Author: Christian Svensson
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
// pawsdetailwindow.h: interface for the pawsDetailWindow class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_DETAIL_WINDOW_HEADER
#define PAWS_DETAIL_WINDOW_HEADER

#include "paws/pawswidget.h"

class pawsTextBox;
class pawsMultiLineTextBox;
class GEMClientObject;

/** A simple info window that displays the position and sector of player.
 */
class pawsDetailWindow : public pawsWidget, public psClientNetSubscriber
{
public:

    pawsDetailWindow();
    virtual ~pawsDetailWindow();

    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void UpdateTabsVisibility(bool Skills, bool CharCreation, bool OOCDescription);
    bool SelectTab( pawsWidget* widget );

    bool PostSetup();
    bool FillInfo();

    void HandleMessage( MsgEntry* me );
    void RequestDetails();
private:
    pawsMultiLineTextBox    *intro;
    pawsMultiLineTextBox    *description;

    pawsButton *editButton;
   
    csArray<csString>  skills;
    csString storedescription;
    csString storedoocdescription;
    csString storedcreationinfo;
    bool details_editable;
    GEMClientObject* target;
    pawsButton* lastTab;
    
};

CREATE_PAWS_FACTORY( pawsDetailWindow );


#endif 


