/*
 * tutorialwindow.h - Author: Keith Fulton
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

#ifndef PAWS_TUTORIAL_WINDOW_HEADER
#define PAWS_TUTORIAL_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawstabwindow.h"
#include "gui/pawscontrolwindow.h"


/** 
 * Window contains a button solely to popup another window showing instructions
 * if the user clicks the button.  The button appears whenever a tutorial
 * message is received from the server, and disappears within 1 minute if 
 * the button is not clicked.
 */
class pawsTutorialNotifyWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    /// Constructor
    pawsTutorialNotifyWindow();

    /// Virtual destructor
    virtual ~pawsTutorialNotifyWindow();

    /// Handles tutorial server messages
    void HandleMessage( MsgEntry* message );

    /// Setup the widget with command/message handling capabilities
    bool PostSetup();

    /// Handle button clicks
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);

    /// The click event of the Instruction window calls this, which activates the next message in the queue, if any.
    bool NextTutorial();

protected:
    /// Ptr to the widget that popups when you click the alerter
    pawsWidget *instr_container;
    /// Ptr to the widget that displays the actual text of the instructions for this message.
    pawsMultiLineTextBox* instructions;
    /// Queue of pending instructions to show, cleared as the user sees them.
    csArray<csString> instrArray;
};

/** The pawsTutorialNotifyWindow factory
 */
CREATE_PAWS_FACTORY( pawsTutorialNotifyWindow );

/** 
 * A window that shows the instructions if the TutorialNotify window button is clicked.
 * The TutorialNotify window sets the multi-line text field here before making the window
 * visible.
 */
class pawsTutorialWindow : public pawsWidget
{
public:
    pawsTutorialWindow();
    virtual ~pawsTutorialWindow();

    bool PostSetup();

    // bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

    void Close();

private:

    pawsTutorialNotifyWindow *notifywnd;
    pawsMultiLineTextBox *instructions;
};

CREATE_PAWS_FACTORY( pawsTutorialWindow );




#endif
