/**
 * pawswritingwindow.h - Author: Daniel Fryer
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWSWRITINGWINDOW_H
#define PAWSWRITINGWINDOW_H

#include "net/cmdbase.h"
#include "paws/pawswidget.h"
#include "paws/pawstextbox.h"
#include "paws/pawsstringpromptwindow.h"

#define MAX_TITLE_LEN 50
#define MAX_CONTENT_LEN 65450

class pawsEditTextBox;
/**
lalalala
*/

class pawsWritingWindow: public pawsWidget, public psClientNetSubscriber, public iOnStringEnteredAction
/* la? */
{
/* la. */
public:
    //not only load from XML but also dynamically activate widgety things based on 
    //the inks & pens that the server sends us
    pawsWritingWindow();
    virtual ~pawsWritingWindow();

    void OnStringEntered(const char *name, int param,const char *value);
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    bool PostSetup();
    void RequestDetails();

    void HandleMessage( MsgEntry* me );
private:
    pawsEditTextBox*          name;

    pawsMultilineEditTextBox* lefttext;
    pawsMultilineEditTextBox* righttext;

    //so we know what book we're talking about here
    int slotID;
    int containerID;
};

CREATE_PAWS_FACTORY( pawsWritingWindow );
#endif
