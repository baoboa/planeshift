/*
 * pawsinteractwindow.h - Author: Andrew Craig
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

#ifndef PAWS_INTERACT_WINDOW_HEADER
#define PAWS_INTERACT_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "gui/chatwindow.h"

class pawsTextBox;
class pawsMessageTextBox;

/** This is the window that allows you to interact with the world. It has 
 *  buttons for things like pickup/examine/open/ etc.
 */
class pawsInteractWindow : public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction
{
public:
    pawsInteractWindow();
    virtual ~pawsInteractWindow();

    bool PostSetup();

    void HandleMessage( MsgEntry* message );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    bool OnMouseDown(int button, int modifiers,int x,int y);
    void Hide();

    void Draw();

    void OnStringEntered(const char *name,int param,const char *value);

private:
    csArray<csString> names;
    csArray<int> types;

    int openTick;
    
    csString genericCommand;

    pawsChatWindow* chatWindow;
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY( pawsInteractWindow );


#endif 


