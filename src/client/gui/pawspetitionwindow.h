/*
 * pawspetitionwindow.h - Author: Alexander Wiseman
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

#ifndef PAWS_PETITION_WINDOW_HEADER
#define PAWS_PETITION_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawsstringpromptwindow.h"
#include "gui/pawscontrolwindow.h"

/// Enum of the columns for the listbox:
enum {
    PCOL_GM = 0,
    PCOL_STATUS = 1,
    PCOL_CREATED = 2,
    PCOL_PETITION = 3
};

/** Defines the length a petition can be before it is
 * truncated with an ellipse: "this is a long..."
 */
#define     MAX_PETITION_LENGTH     (65)


/** Window contains a list of the user's petitions.
 * with options to view their resolutions or cancel them
 *
 * NOTE: the current expected columns for the listbox are
 *   as follows:
 *
 *  1) gm
 *  2) status
 *  3) created
 *  4) petition
 */
class pawsPetitionWindow : public pawsControlledWindow, public psCmdBase, public iOnStringEnteredAction
{
public:
    /// Constructor
    pawsPetitionWindow();

    /// Virtual destructor
    virtual ~pawsPetitionWindow();

    virtual void Show();

    /// Handles petition server messages
    void HandleMessage( MsgEntry* message );

    /// Handles commands
    const char* HandleCommand(const char* cmd);

    /// Setup the widget with command/message handling capabilities
    bool PostSetup();

    /// Handle button clicks
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);

    void OnListAction( pawsListBox* selected, int status );

    void Close();
    /**
     * Handle the prompt window for entering a petition.
     */
    void OnStringEntered(const char *name, int param,const char *value);

protected:

    /// Queries the server for a list of petitions
    void QueryServer();

    /// Quicker way to set text for each column in the listbox:
    void SetText(size_t rowNum, int colNum, const char* fmt, ...);

    /// Function to add the petitions to the listbox given a csArray;
    void AddPetitions(csArray<psPetitionInfo> &petitions);

    /// List widget of petitions for easy access
    pawsListBox* petitionList;

    /// Holds the current row number (for cancellation)
    int currentRow;

    /// Holds the most recent petition message from the server
    psPetitionMessage petitionMessage;

    /// to count the number of actual petitions in the list
    int petCount;

    /// this is set to true the first time a player looks at this petition window
    /// this allows for most players to refrain from querying the petition list
    bool hasPetInterest;

    /// keep track of the selected petition so that we can select it after receiving a new petition list
    psPetitionInfo selectedPet;

    /// Displays text of petition
    pawsMultilineEditTextBox * petText;
    
    bool updateSelectedPetition; ///< used to know if we have to update the selected petition after deleting one, or saving one

};

/** The pawsPetitionWindow factory
 */
CREATE_PAWS_FACTORY( pawsPetitionWindow );

#endif
