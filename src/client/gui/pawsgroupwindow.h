/*
 * pawsgroupwindow.h - Author: Andrew Craig
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

#ifndef PAWS_GROUP_WINDOW_HEADER
#define PAWS_GROUP_WINDOW_HEADER

#include "pscelclient.h"
#include "paws/pawswidget.h"
#include "paws/pawsstringpromptwindow.h"
#include "net/cmdhandler.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

class pawsTextBox;
class pawsMessageTextBox;
class pawsListBox;

/**  This window shows the current members that are in your group.
 *  
 */
class pawsGroupWindow : public pawsControlledWindow, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:

    pawsGroupWindow();
    virtual ~pawsGroupWindow();

    void HandleMessage( MsgEntry* message );
    bool PostSetup();
    void OnStringEntered(const char *name,int param,const char *value);

    bool OnMenuAction(pawsWidget *widget, const pawsMenuAction & action);

    // Set the stats of a group member.
    void SetStats( GEMClientActor * actor );

private:
    pawsListBox* memberList;

    void Draw();
    void HandleGroup( csString& string );
    void HandleMembers( csString& string );
    
    pawsChatWindow* chatWindow;
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY( pawsGroupWindow );


#endif 


