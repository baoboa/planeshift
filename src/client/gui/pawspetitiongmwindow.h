/*
 * pawspetitiongmwindow.h - Author: Alexander Wiseman
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

#ifndef PAWS_PETITION_WINDOW_GM_HEADER
#define PAWS_PETITION_WINDOW_GM_HEADER

#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawsstringpromptwindow.h"

/// Enum of the columns for the listbox:
enum {
    PGMCOL_LVL = 0,
    PGMCOL_GM= 1,
    PGMCOL_PLAYER = 2,
    PGMCOL_ONLINE = 3,
    PGMCOL_STATUS = 4,
    PGMCOL_CREATED = 5,
    PGMCOL_PETITION = 6
};

/** Defines the length a petition can be before it is
 * truncated with an ellipse: "this is a long..."
 */
#define        MAX_GMPETITION_LENGTH   (65)

/** Window contains a manageable list of petitions.
 * with options for GMs handling them
 *
 * NOTE: the current expected columns for the listbox are
 *     as follows:
 *
 *    1) escalation level
 *    2) player
 *    3) created date/tiem
 *    4) petition
 */
class pawsPetitionGMWindow : public pawsWidget, public psCmdBase, public iOnStringEnteredAction
{
public:
    /// Constructor
    pawsPetitionGMWindow();

    /// Virtual destructor
    virtual ~pawsPetitionGMWindow();

    ///handles the request of petitions and checks if the user is allowed to use this window
    void Show();

    /// Handles petition server messages
    void HandleMessage( MsgEntry* message );

    /// Handles commands
    const char* HandleCommand(const char* cmd);

    /// Setup the widget with command/message handling capabilities
    bool PostSetup();

    /// Handle button clicks
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);

    void OnListAction( pawsListBox* selected, int status );

    /// Handle popup question window callback
    void OnStringEntered(const char *name,int param,const char *value);

protected:

    /// Queries the server for a list of petitions
    void QueryServer();

    /// Quicker way to set text for each column in the listbox:
    void SetText(size_t rowNum, int colNum, const char* fmt, ...);

    /// Function to add the petitions to the listbox given a csArray;
    void AddPetitions(csArray<psPetitionInfo> &petitions);

    /// Send network message to close a petition with explanation
    void CloseCurrPetition(const char * desc);


    /// List widget of petitions for easy access
    pawsListBox* petitionList;

    /// Holds the current row number (for cancellation)
    int currentRow;

    /// Holds the most recent petition message from the server
    psPetitionMessage petitionMessage;

    /// count the number of actual petitions
    int petCount;

    /// this is set to true the first time a player looks at this petition window
    /// this allows for most players to refrain from querying the petition list
    bool hasPetInterest;

    /// keep track of the selected petition so that we can select it after receiving a new petition list
    psPetitionInfo selectedPet;

    /// Displays text of petition
    pawsMessageTextBox * petText;
};

/** The pawsPetitionGMWindow factory
 */
CREATE_PAWS_FACTORY( pawsPetitionGMWindow );

#endif
