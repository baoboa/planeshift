/*
 * attacklist.cpp              author: hartra344 [at] gmail [dot] com
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_ATTACK_WINDOW_HEADER
#define PAWS_ATTACK_WINDOW_HEADER

#include "paws/pawswidget.h"
class pawsTextBox;
class pawsListBox;
class pawsMessageTextBox;
class pawsSlot;

#include "net/cmdbase.h"
#include "gui/pawscontrolwindow.h"


/** This handles all the details about how the spell book works.
 */
class pawsAttackBookWindow : public pawsControlledWindow, public psClientNetSubscriber
{
public:

    pawsAttackBookWindow();
    virtual ~pawsAttackBookWindow();

    bool PostSetup();

    virtual void HandleMessage(MsgEntry* msg);

    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    void OnListAction(pawsListBox* listbox, int status);
    void Show();
private:

    void Queue();

    void Close();

    void HandleAttacks(MsgEntry* me);

    pawsListBox*        attackList;
    pawsMessageTextBox* attackDescription;
    pawsSlot*           attackImage;

    csString selectedAttack;

    csHash<csString, csString> descriptions_Hash;
    csHash<csString, csString> images_Hash;
};

CREATE_PAWS_FACTORY(pawsAttackBookWindow);


#endif



