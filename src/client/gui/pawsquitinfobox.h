/*
 * pawsquitinfobox.h - Author: Christian Svensson
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
//////////////////////////////////////////////////////////////////////
#ifndef PAWS_QUIT_INFO_WINDOW
#define PAWS_QUIT_INFO_WINDOW

#include <iutil/virtclk.h>
#include <csutil/csstring.h>
#include "paws/pawswidget.h"
#include "paws/pawsokbox.h"

class pawsQuitInfoBox : public pawsWidget
{
public:
    pawsQuitInfoBox();
    
    bool PostSetup();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    void SetBox(csString newText);

    void MoveToCenter();

    void Show();

private:
    csString text;
    pawsMultiLineTextBox* textBox;
    pawsWidget* okButton;    

};

CREATE_PAWS_FACTORY( pawsQuitInfoBox );

#endif
