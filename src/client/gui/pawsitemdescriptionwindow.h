/*
 * pawsitemdesciptionwindow.h - Author: Andrew Craig
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

#ifndef PAWS_ITEM_DESCRIPTION_WINDOW_HEADER
#define PAWS_ITEM_DESCRIPTION_WINDOW_HEADER

#include "paws/pawswidget.h"

class pawsTextBox;
class pawsMultiLineTextBox;

/** A window that shows the description of an item.
 */
class pawsItemDescriptionWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    pawsItemDescriptionWindow();
    virtual ~pawsItemDescriptionWindow();

    bool PostSetup();

    void HandleMessage( MsgEntry* message );
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    void ShowItemDescription(csString &desc, csString &itemName, csString &icon);
    void ShowItemDescription(csString &desc, csString &itemName, csString &icon, int count);

private:
    pawsTextBox*            name;
    pawsMultiLineTextBox*   description;
    pawsWidget*             pic;
};

CREATE_PAWS_FACTORY( pawsItemDescriptionWindow );


#endif 


