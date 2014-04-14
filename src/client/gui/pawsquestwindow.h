/*
 * pawsquestwindow.h - Author: Keith Fulton
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

#ifndef PAWS_QUEST_WINDOW_HEADER
#define PAWS_QUEST_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawstabwindow.h"
#include "gui/pawscontrolwindow.h"
#include "paws/pawsnumberpromptwindow.h"
#include "paws/pawsstringpromptwindow.h"

/// Enum of the columns for the quest listbox:
enum
{
    QCOL_NAME = 0,
    QCOL_ID   = 1,
    QCOL_STATUS = 2 // A for Active, C for completed
};

struct QuestNote
{
    int id;
    size_t topLine;
    csString notes;
};

/// Enum of the columns for the GM event listbox
enum
{
    EVCOL_NAME = 0,
    EVCOL_ROLE = 1,
    EVCOL_ID   = 2
};

/**
 * Window contains a list of the available loot items.
 * with options to take them, roll for them or cancel the window.
 *
 * NOTE: the current expected columns for the listbox are
 *     as follows:
 *
 *  1) icon
 *  2) item name
 *  3) item id (hidden)
 */
class pawsQuestListWindow : public pawsControlledWindow, public psCmdBase, public iOnStringEnteredAction, public iOnNumberEnteredAction
{
public:
    /// Constructor
    pawsQuestListWindow();

    /// Virtual destructor
    virtual ~pawsQuestListWindow();

    /// Show the quest window
    void Show(void);

    /// Handles petition server messages
    void HandleMessage( MsgEntry* message );

    /// Handles commands
    const char* HandleCommand(const char* cmd);

    /// Setup the widget with command/message handling capabilities
    bool PostSetup();

    /// Handle button clicks
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* reporter);

    /// Handle question boxes
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);

    void OnListAction( pawsListBox* selected, int status );

    /// Save quest notes
    void SaveNotes(const char * fileName);

    /// Load quest notes
    void LoadNotes(const char * fileName);

    /// Show quest notes
    void ShowNotes();
    
    /// Handle popup vote question window callback
    void OnStringEntered(const char *name, int param, const char *value);
    
    /// Handle popup comment question window callback
    void OnNumberEntered(const char *name, int param, int value);

protected:

    /// Ask server for data about given quest
    void RequestQuestData(int id);

    /// Ask server for data about a given GM-Event
    void RequestGMEventData(int id);

    /// Ask server to discard a given quest
    void DiscardQuest(int id);

    /// Ask server to discard a given GM event
    void DiscardGMEvent(int id);
    
    /** @brief Sends an evaluation on the gm event to the server
     * 
     *  @param id: The id of the event which is being evaluated
     *  @param vote: a number from 1 to 10 stating the vote to assign to the event
     *  @param comment: A comment about the event
     */
    void EvaluateGMEvent(int id, uint8_t vote, csString comment);

    /// Populate Quest tab
    void PopulateQuestTab(void);

    /// Populate GM Events tab
    void PopulateGMEventTab(void);

    /// The selected tab of quests
    pawsTabWindow* questTab;

    /// The list of quests
    pawsListBox* questList;

    /// The list of completed quests
    pawsListBox* completedQuestList;

    /// The list of uncompleted quests (discarded quests are hidden; deleted after expiration)
    pawsListBox* uncompletedQuestList;

    /// XML strings for quests
    csString completedQuests;
    csString uncompletedQuests;

    /// flag to indicate to populate new quest xml string
    bool populateQuestLists;

    /// The selected tab of events
    pawsTabWindow* eventTab;

    /// The list of events
    pawsListBox* eventList;

    /// The list of completed events
    pawsListBox* completedEventList;

    /// The list of uncompleted events (discarded events are hidden; deleted after expiration)
    pawsListBox* uncompletedEventList;

    /// XML strings for gm events
    csString completedEvents;
    csString uncompletedEvents;

    /// flag to indicate to populate new quest xml string
    bool populateGMEventLists;

    /// current top tab
    pawsTabWindow* currentTab;

    pawsMessageTextBox* description;

    pawsMultilineEditTextBox* notes;

    pawsTextBox* total;
    
    /// The button which brings up the evaluation procedure
    pawsButton* EvaluateBtn;

    int questID;        ///< ID of selected quest (-1 = no quest selected)
    int questIDBuffer;  ///< ID of pending discard (in case selection changes during prompt)
    
    int VoteBuffer;     ///< Last vote expressed

    csArray<QuestNote*> quest_notes;

    csRef<iVFS> vfs;
    csRef<iDocumentSystem> xml;
    csString filename;
    csString TotalNumberStr;
    pawsButton* QuestListsBtn;
};

/** The pawsLootWindow factory
 */
CREATE_PAWS_FACTORY( pawsQuestListWindow );

#endif
