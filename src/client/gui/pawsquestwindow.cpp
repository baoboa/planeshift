/*
* pawsquestwindow.cpp - Author: Keith Fulton
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

#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/xmltiny.h>
#include <ivideo/fontserv.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "gui/pawscontrolwindow.h"

#include "paws/pawsprefmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsyesnobox.h"

#include "util/strutil.h"

#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pawsquestwindow.h"
#include "inventorywindow.h"
#include "pscelclient.h"
#include "globals.h"

#define DISCARD_BUTTON                    1201
#define VIEW_BUTTON                       1200
#define SAVE_BUTTON                       1203
#define CANCEL_BUTTON                     1204
#define EVALUATE_BUTTON                   1205
#define TAB_UNCOMPLETED_QUESTS_OR_EVENTS  1000
#define TAB_COMPLETED_QUESTS_OR_EVENTS    1001
#define QUEST_TAB_BUTTON                   300
#define EVENT_TAB_BUTTON                   301

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsQuestListWindow::pawsQuestListWindow()
    : psCmdBase(NULL,NULL,  PawsManager::GetSingleton().GetObjectRegistry())
{
    vfs =  csQueryRegistry<iVFS > (PawsManager::GetSingleton().GetObjectRegistry());
    xml = psengine->GetXMLParser();

    questList = NULL;
    questTab = NULL;
    completedQuestList = NULL;
    uncompletedQuestList = NULL;
    eventList = NULL;
    eventTab = NULL;
    completedEventList = NULL;
    uncompletedEventList = NULL;
    description = NULL;
    notes = NULL;
    total = NULL;
    currentTab = NULL;

    populateGMEventLists = false;
    populateQuestLists = false;

    questID = -1;

    VoteBuffer = 10;

    filename = "/planeshift/userdata/questnotes_";
    filename.Append(psengine->GetMainPlayerName());
    filename.Append(".xml");
    filename.ReplaceAll(" ", "_");

    // Load the quest notes file (if it doesn't exist, it will be made on first save)
    if(vfs->Exists(filename))
        LoadNotes(filename);
}

pawsQuestListWindow::~pawsQuestListWindow()
{
    if(msgqueue)
    {
        msgqueue->Unsubscribe(this, MSGTYPE_QUESTLIST);
        msgqueue->Unsubscribe(this, MSGTYPE_QUESTINFO);

        msgqueue->Unsubscribe(this, MSGTYPE_GMEVENT_LIST);
        msgqueue->Unsubscribe(this, MSGTYPE_GMEVENT_INFO);
    }

    while(quest_notes.GetSize())
        delete quest_notes.Pop();
}

void pawsQuestListWindow::Show(void)
{
    eventList->Clear();
    uncompletedEventList->Clear();
    description->Clear();
    notes->Clear();

    psUserCmdMessage msg("/quests");
    psengine->GetMsgHandler()->SendMessage(msg.msg);

    pawsControlledWindow::Show();
}

bool pawsQuestListWindow::PostSetup()
{
    // Setup this widget to receive messages and commands
    if(!psCmdBase::Setup(psengine->GetMsgHandler(),
                         psengine->GetCmdHandler()))
        return false;

    // Subscribe to certain types of messages (those we want to handle)
    msgqueue->Subscribe(this, MSGTYPE_QUESTLIST);
    msgqueue->Subscribe(this, MSGTYPE_QUESTINFO);

    msgqueue->Subscribe(this, MSGTYPE_GMEVENT_LIST);
    msgqueue->Subscribe(this, MSGTYPE_GMEVENT_INFO);

    questTab             = (pawsTabWindow*)FindWidget("QuestTabs");
    completedQuestList   = (pawsListBox*)FindWidget("CompletedQuestList");
    uncompletedQuestList = (pawsListBox*)FindWidget("UncompletedQuestList");

    // get pointer to active listbox:
    questList  = (pawsListBox*)questTab->GetActiveTab();

    eventTab             = (pawsTabWindow*)FindWidget("EventTabs");
    completedEventList   = (pawsListBox*)FindWidget("CompletedEventList");
    uncompletedEventList = (pawsListBox*)FindWidget("UncompletedEventList");

    // get pointer to active listbox:
    eventList  = (pawsListBox*)eventTab->GetActiveTab();

    total = (pawsTextBox*)FindWidget("Total");
    description = (pawsMessageTextBox*)FindWidget("Description");
    notes = (pawsMultilineEditTextBox*)FindWidget("Notes");

    EvaluateBtn = (pawsButton*)FindWidget("Evaluate");

    completedQuestList->SetSortingFunc(0, textBoxSortFunc);
    uncompletedQuestList->SetSortingFunc(0, textBoxSortFunc);
    completedEventList->SetSortingFunc(0, textBoxSortFunc);
    uncompletedEventList->SetSortingFunc(0, textBoxSortFunc);

    QuestListsBtn = (pawsButton*)FindWidget("QuestLists");
    QuestListsBtn->SetState(true);  ///Set the button down so it looks pressed on load

    PopulateQuestTab(); ///open questlist on opening

    return true;
}

//////////////////////////////////////////////////////////////////////
// Command and Message Handling
//////////////////////////////////////////////////////////////////////

const char* pawsQuestListWindow::HandleCommand(const char* /*cmd*/)
{
    return NULL;
}

void pawsQuestListWindow::HandleMessage(MsgEntry* me)
{
    switch(me->GetType())
    {
        case MSGTYPE_QUESTLIST:
        {
            // Get the petition message data from the server
            psQuestListMessage message(me);

            // The quest xml string contains all the quests for the
            // current user. The quests that have been completed
            // will be added to a different listbox than the quests that
            // still need to be done. So here we'll split the quest xml
            // string into two parts according to the quest status.
            psXMLString questxml = psXMLString(message.questxml);
            psXMLString section;

            size_t start = questxml.FindTag("q>");
            size_t end   = questxml.FindTag("/quest");

            completedQuests   = "<quests>";
            uncompletedQuests = "<quests>";

            if(start != SIZET_NOT_FOUND)
            {
                do
                {
                    // a quest looks like this:
                    // <q><image icon="%s" /><desc text="%s" /><id text="%d" /><status text="%c" /></q>
                    start += questxml.GetTagSection((int)start,"q",section);
                    csString str = "status text=\"";
                    char status = section.GetAt(section.FindSubString(str) + str.Length());

                    if(status == 'A')
                        uncompletedQuests += section;
                    else if(status == 'C')
                        completedQuests += section;
                }
                while(section.Length()!=0 && start<end);
            }

            completedQuests   += "</quests>";
            uncompletedQuests += "</quests>";

            completedQuestList->Clear();
            uncompletedQuestList->Clear();
            description->Clear();
            notes->Clear();
            EvaluateBtn->Hide();
            populateQuestLists = true;

            // Reset window
            if(!currentTab || currentTab == questTab)
            {
                PopulateQuestTab();
            }
            break;
        }

        case MSGTYPE_QUESTINFO:
        {
            psQuestInfoMessage message(me);
            EvaluateBtn->Hide();
            SelfPopulateXML(message.xml);
            break;
        }

        case MSGTYPE_GMEVENT_LIST:
        {
            // get the GM events data
            psGMEventListMessage message(me);

            // The event xml string contains all the GM events for the
            // current user. The events that have been completed
            // will be added to a different listbox than the events that
            // still need to be done. So here we'll split the event xml
            // string into two parts according to the event status.
            psXMLString gmeventxml = psXMLString(message.gmEventsXML);
            psXMLString section;

            size_t start = gmeventxml.FindTag("event>");
            size_t end   = gmeventxml.FindTag("/gmevents");

            completedEvents   = "<gmevents>";
            uncompletedEvents = "<gmevents>";

            if(start != SIZET_NOT_FOUND)
            {
                do
                {
                    // a gm event looks like this:
                    // <event><name text="%s" /><role text="%c" /><status text="%c" /><id text="%d" /></event>
                    start += gmeventxml.GetTagSection((int)start,"event",section);
                    csString str = "status text=\"";
                    char status = section.GetAt(section.FindSubString(str) + str.Length());

                    if(status == 'R')
                        uncompletedEvents += section;
                    else if(status == 'C')
                        completedEvents += section;
                }
                while(section.Length()!=0 && start<end);
            }

            completedEvents   += "</gmevents>";
            uncompletedEvents += "</gmevents>";

            completedEventList->Clear();
            uncompletedEventList->Clear();
            description->Clear();
            notes->Clear();
            EvaluateBtn->Hide();
            populateGMEventLists = true;

            if(!currentTab || currentTab == eventTab)
            {
                PopulateGMEventTab();
            }
            break;
        }

        case MSGTYPE_GMEVENT_INFO:
        {
            psGMEventInfoMessage message(me);
            EvaluateBtn->SetVisibility(message.Evaluatable);
            SelfPopulateXML(message.xml);
            break;
        }
    }
}

bool pawsQuestListWindow::OnButtonReleased(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    int button = widget->GetID();

    questList = (pawsListBox*)questTab->GetActiveTab();
    eventList = (pawsListBox*)eventTab->GetActiveTab();

    switch(button)
    {
        case CONFIRM_YES:
        {
            if(currentTab == questTab)
                DiscardQuest(questIDBuffer);
            else if(currentTab == eventTab)
                DiscardGMEvent(questIDBuffer);
            questID = -1;

            // Refresh window
            notes->Clear();
            description->Clear();
            eventList->Clear();
            questList->Clear();

            psUserCmdMessage msg("/quests");
            psengine->GetMsgHandler()->SendMessage(msg.msg);
            break;
        }
    }
    return true;
}

bool pawsQuestListWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    // We know that the calling widget is a button.
    int button = widget->GetID();

    questList = (pawsListBox*)questTab->GetActiveTab();
    eventList = (pawsListBox*)eventTab->GetActiveTab();

    switch(button)
    {
        case TAB_COMPLETED_QUESTS_OR_EVENTS:
        {
            if(currentTab == questTab)
            {
                questID = -1;
                completedQuestList->Select(NULL);

                TotalNumberStr.Format("%zu",completedQuestList->GetRowCount());
                total->SetText("Total: " + TotalNumberStr);
            }
            else
            {
                questID = -1;
                completedEventList->Select(NULL);

                TotalNumberStr.Format("%zu",completedEventList->GetRowCount());
                total->SetText("Total: " + TotalNumberStr);
            }
            description->Clear();
            notes->Clear();
            break;
        }

        case TAB_UNCOMPLETED_QUESTS_OR_EVENTS:
        {
            if(currentTab == questTab)
            {
                questID = -1;
                uncompletedQuestList->Select(NULL);

                TotalNumberStr.Format("%zu",uncompletedQuestList->GetRowCount());
                total->SetText("Total: " + TotalNumberStr);
            }
            else
            {
                questID = -1;
                uncompletedEventList->Select(NULL);

                TotalNumberStr.Format("%zu",uncompletedEventList->GetRowCount());
                total->SetText("Total: " + TotalNumberStr);
            }
            description->Clear();
            notes->Clear();
            break;
        }

        // These cases are for the discard funciton, and it's confirmation prompt
        case DISCARD_BUTTON:
        {
            if(currentTab == eventTab && questID != -1)
            {
                PawsManager::GetSingleton().CreateYesNoBox(
                    "Are you sure you want to discard the selected event?", this);
                questIDBuffer = questID;
            }
            else if(currentTab == questTab && questID != -1)
            {
                PawsManager::GetSingleton().CreateYesNoBox(
                    "Are you sure you want to discard the selected quest?", this);
                questIDBuffer = questID;
            }
            break;
        }

        case EVALUATE_BUTTON:
        {
            if(currentTab == eventTab && questID != -1)
            {
                pawsNumberPromptWindow::Create(PawsManager::GetSingleton().Translate("Select a vote (1-10)"), 10, 1, 10, this, "vote");
            }
            break;
        }

        // These cases are for the edit window child of this
        case SAVE_BUTTON:
        {
            if(currentTab == eventTab)
            {
                PawsManager::GetSingleton().CreateWarningBox(
                    "Notes for GM-Events not yet implemented. Sorry.", this);
            }
            else if(currentTab == questTab && questID != -1)
            {
                bool found = false;
                for(size_t i=0; i < quest_notes.GetSize(); i++)
                {
                    if(quest_notes[i]->id == questID)  // Change notes
                    {
                        quest_notes[i]->notes = notes->GetText();
                        found = true;
                        break;
                    }
                }

                if(!found)  // Add notes
                {
                    QuestNote* qn = new QuestNote;
                    qn->id = questID;
                    qn->notes = notes->GetText();
                    quest_notes.Push(qn);
                }

                SaveNotes(filename);
            }
            break;
        }
        case CANCEL_BUTTON:
        {
            if(currentTab == questTab)
            {
                ShowNotes();
            }
            break;
        }

        case QUEST_TAB_BUTTON:
        {
            if(currentTab != questTab)
            {
                if(currentTab)
                    currentTab->Hide();
                PopulateQuestTab();

                description->Clear();
                notes->Clear();
            }
            break;
        }

        case EVENT_TAB_BUTTON:
        {
            if(currentTab != eventTab)
            {
                if(currentTab)
                    currentTab->Hide();
                PopulateGMEventTab();
                description->Clear();
                notes->Clear();

                if(QuestListsBtn->IsDown())
                    QuestListsBtn->SetState(false);
            }
            break;
        }
    }
    return true;
}

void pawsQuestListWindow::OnStringEntered(const char* /*name*/, int /*param*/, const char* value)
{
    if(!value)
        return; // Cancel was clicked

    csString text(value);
    text.Trim();
    EvaluateGMEvent(questID, VoteBuffer, text); //sends the data to the server
    RequestGMEventData(questID); //refreshes the informations (just disables the evaluate button in practice for now)
}

void pawsQuestListWindow::OnNumberEntered(const char* /*name*/, int /*param*/, int value)
{
    if(value == -1)  //not confirmed action
        return;

    VoteBuffer = value; //we store the value in a separate variable.

    pawsStringPromptWindow::Create(PawsManager::GetSingleton().Translate("Add a comment about this event if you wish"), csString(""),
                                   true, 500, 300, this, "comment", 0, true);  //now we can request a command
}

inline void pawsQuestListWindow::RequestQuestData(int id)
{
    psQuestInfoMessage info(0,psQuestInfoMessage::CMD_QUERY,id,NULL,NULL);
    msgqueue->SendMessage(info.msg);
}

inline void pawsQuestListWindow::RequestGMEventData(int id)
{
    psGMEventInfoMessage info(0,psGMEventInfoMessage::CMD_QUERY,id,NULL,NULL);
    msgqueue->SendMessage(info.msg);
}

inline void pawsQuestListWindow::DiscardQuest(int id)
{
    psQuestInfoMessage info(0,psQuestInfoMessage::CMD_DISCARD,id,NULL,NULL);
    msgqueue->SendMessage(info.msg);
}

inline void pawsQuestListWindow::DiscardGMEvent(int id)
{
    psGMEventInfoMessage info(0,psGMEventInfoMessage::CMD_DISCARD,id,NULL,NULL);
    msgqueue->SendMessage(info.msg);
}

inline void pawsQuestListWindow::EvaluateGMEvent(int Id, uint8_t Vote, csString Comment)
{
    csString Xml;
    Xml.Format("<GMEventEvaluation vote=\"%d\" comments=\"%s\" />", Vote, EscpXML(Comment).GetDataSafe());
    psGMEventInfoMessage info(0,psGMEventInfoMessage::CMD_EVAL,Id,NULL,Xml.GetData());
    msgqueue->SendMessage(info.msg);
}

void pawsQuestListWindow::OnListAction(pawsListBox* selected, int /*status*/)
{
    int previousQuestID = questID;
    size_t numOfQuests;
    size_t topLine = 0;
    int idColumn;

    if(currentTab == questTab)
        idColumn = QCOL_ID;
    else if(currentTab == eventTab)
        idColumn = EVCOL_ID;
    else
        return;

    pawsListBoxRow* row = selected->GetSelectedRow();
    if(row)
    {
        pawsTextBox* field = (pawsTextBox*)row->GetColumn(idColumn);
        questID = atoi(field->GetText());

        if(currentTab == questTab)        // GM Events dont support notes, yet!
        {
            numOfQuests = quest_notes.GetSize();
            for(size_t i=0; i < numOfQuests; i++)
            {
                // if no previous quest selected, then 0 all toplines
                if(previousQuestID < 0)
                {
                    quest_notes[i]->topLine = 0;
                }
                // store topline of 'old' quest notes
                else if(quest_notes[i]->id == previousQuestID)
                {
                    quest_notes[i]->topLine = notes->GetTopLine();
                }
            }
            for(size_t i=0; i < numOfQuests; i++)
            {
                // set new quest notes topline
                if(quest_notes[i]->id == questID)
                {
                    topLine = quest_notes[i]->topLine;
                }
            }
            notes->SetTopLine(topLine);

            RequestQuestData(questID);
            ShowNotes();
        }
        else
        {
            RequestGMEventData(questID);
        }
    }
}

void pawsQuestListWindow::SaveNotes(const char* fileName)
{

    // Save quest notes to a local file
    char temp[20];
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot();
    csRef<iDocumentNode> parentMain = root->CreateNodeBefore(CS_NODE_ELEMENT);
    parentMain->SetValue("questnotes");
    csRef<iDocumentNode> parent;
    csRef<iDocumentNode> text;

    for(size_t i=0; i < quest_notes.GetSize(); i++)
    {
        QuestNote* qn = quest_notes[i];

        if(!(qn->notes).IsEmpty() &&
                !(qn->notes.Length() == 1 &&       // workaround to CS bug, test that file is not of length 1 containing a newline or carriage return
                  (qn->notes.GetAt(0)== '\n' ||     // newline
                   qn->notes.GetAt(0) == '\r' )))    // carriage return
        {
            parent = parentMain->CreateNodeBefore(CS_NODE_ELEMENT);
            sprintf(temp, "quest%d", qn->id);
            parent->SetValue(temp);

            text = parent->CreateNodeBefore(CS_NODE_TEXT);
            text->SetValue(qn->notes);
        }
    }
    doc->Write(vfs, fileName);
}

void pawsQuestListWindow::LoadNotes(const char* fileName)
{
    int id;
    csRef<iDocument> doc = xml->CreateDocument();

    csRef<iDataBuffer> buf(vfs->ReadFile(fileName));
    if(!buf || !buf->GetSize())
    {
        return;
    }
    const char* error = doc->Parse(buf);
    if(error)
    {
        Error2("Error loading quest notes: %s", error);
        return;
    }

    csRef<iDocumentNodeIterator> iter = doc->GetRoot()->GetNode("questnotes")->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> child = iter->Next();
        if(child->GetType() != CS_NODE_ELEMENT)
            continue;

        sscanf(child->GetValue(), "quest%d", &id);

        QuestNote* qn = new QuestNote;
        qn->id = id;
        qn->notes = child->GetContentsValue();
        quest_notes.Push(qn);
    }
}

void pawsQuestListWindow::ShowNotes()
{
    for(size_t i=0; i < quest_notes.GetSize(); i++)
    {
        if(quest_notes[i]->id == questID)
        {
            notes->SetText(quest_notes[i]->notes);
            return;
        }
    }

    notes->Clear(); // If didn't find notes, then blank
}

void pawsQuestListWindow::PopulateQuestTab(void)
{
    if(populateQuestLists)
    {
        // add quests to the (un)completedQuestList if new
        completedQuestList->SelfPopulateXML(completedQuests);
        uncompletedQuestList->SelfPopulateXML(uncompletedQuests);
        populateQuestLists = false;
    }
    currentTab = questTab;
    questTab->SetTab(TAB_UNCOMPLETED_QUESTS_OR_EVENTS);
    completedQuestList->Select(NULL);
    uncompletedQuestList->Select(NULL);

    completedQuestList->SortRows();
    uncompletedQuestList->SortRows();

    TotalNumberStr.Format("%zu",uncompletedQuestList->GetRowCount());
    total->SetText("Total: " + TotalNumberStr);

    EvaluateBtn->Hide();

    currentTab->Show();
}

void pawsQuestListWindow::PopulateGMEventTab(void)
{
    if(populateGMEventLists)
    {
        // add quests to the (un)completedQuestList if new
        completedEventList->SelfPopulateXML(completedEvents);
        uncompletedEventList->SelfPopulateXML(uncompletedEvents);
        populateGMEventLists = false;
    }
    currentTab = eventTab;
    eventTab->SetTab(TAB_UNCOMPLETED_QUESTS_OR_EVENTS);
    completedEventList->Select(NULL);
    uncompletedEventList->Select(NULL);

    completedEventList->SortRows();
    uncompletedEventList->SortRows();

    TotalNumberStr.Format("%zu",uncompletedEventList->GetRowCount());
    total->SetText("Total: " + TotalNumberStr);

    EvaluateBtn->Hide();

    currentTab->Show();
}

