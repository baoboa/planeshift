/*
 * pawscontainerdesciptionwindow.h - Author: Thomas Towey
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

#ifndef PAWS_CONTAINER_DESCRIPTION_WINDOW_HEADER
#define PAWS_CONTAINER_DESCRIPTION_WINDOW_HEADER

#include "paws/pawsmanager.h"
#include "paws/pawswidget.h"
#include "util/psconst.h"
#include "gui/inventorywindow.h"

class pawsTextBox;
class pawsMessageTextBox;
class pawsSlot;

/** A window that shows the description of an container.
 */
class pawsContainerDescWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    pawsContainerDescWindow();
    virtual ~pawsContainerDescWindow();

    //from pawsWidget:
    bool PostSetup();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    //from iNetSubscriber:
    void HandleMessage( MsgEntry* message );

    /**
     * Return the container ID of the current displayed container.
     */
    ContainerID GetContainerID() { return containerID; }

private:
    void HandleUpdateItem( MsgEntry* me );
    void HandleViewContainer( MsgEntry* me );

    pawsTextBox*          name;
    pawsMultiLineTextBox* description;
    pawsWidget*           pic;
    ContainerID           containerID;
    pawsListBox*          contents;

    int                   containerSlots;
};

CREATE_PAWS_FACTORY( pawsContainerDescWindow );


#endif


