/*
 * pawsmusicwindow.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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


#include <psconfig.h>
#include "pawsmusicwindow.h"

//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <iutil/cfgmgr.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <globals.h>
#include <net/clientmsghandler.h>
#include <paws/pawsborder.h>
#include <paws/pawsbutton.h>
#include <paws/pawscrollbar.h>
#include <music/musicutil.h>

//====================================================================================
// Local Includes
//====================================================================================
#include "pawssheetline.h"

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------
#define USE_PLAYED_TEMPO true
#define USE_SCORE_TEMPO false


pawsMusicWindow::pawsMusicWindow()
{
    playButton = 0; // necessary for ResetState to work correctly
    ResetState();

    // subscribing to song manager and for musical sheet messages
    psengine->GetSongManager()->Subscribe(this);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_MUSICAL_SHEET);
}

pawsMusicWindow::pawsMusicWindow(const pawsMusicWindow* origin)
{
    // TODO currently useless
}

pawsMusicWindow::~pawsMusicWindow()
{
    DeleteAll();

    // unsubscribing to song manager and for musical sheet messages
    psengine->GetSongManager()->Unsubscribe(this);
    psengine->GetMsgHandler()->Unsubscribe(this,MSGTYPE_MUSICAL_SHEET);
}

void pawsMusicWindow::OnChordSelection(SheetLine* sheetLine, Measure* measure, Chord* chord, int notePosition, bool before)
{
    bool done = false;      // used to return before adding notes to the selected chord

    if(!editButton->GetState() || pendingString)
    {
        return;
    }

    // handling chord selection
    selectedSheetLine = sheetLine;

    if(chord != selectedChord)
    {
        // updating selection
        selectedChord = chord;

        if(measure != selectedMeasure)
        {
            if(insertMode)
            {
                SetInsertMode(false);
            }

            selectedMeasure = measure;

            // selecting repeats button
            startRepeatButton->SetState(selectedMeasure->GetStartRepeat());
            endRepeatButton->SetState(selectedMeasure->GetEndRepeat() > 0);
            endingButton->SetState(selectedMeasure->GetEnding() > 0);
        }

        // selecting duration
        SetDurationButton(chord->GetDuration());

        done = true;
    }

    // handling note insertion
    if(insertNoteButton->GetState() && !selectedChord->IsEmpty())
    {
        SetInsertMode(true);

        selectedChord = selectedMeasure->InsertChord(selectedChord, before);

        UpdateLines(selectedSheetLine, false);
        
        done = false;   // the note must be added to the chord
    }

    if(done)
    {
        return;
    }

    // handling adding note to the chord
    int alter;
    bool isRest = restButton->GetState();

    switch(selectedAlterButton)
    {
    case FLAT_BUTTON:
        alter = FLAT;
        break;
    case NATURAL_BUTTON:
        alter = NATURAL;
        break;
    case SHARP_BUTTON:
        alter = SHARP;
        break;
    default:
        alter = UNALTERED;
    }

    selectedChord->AddNote(notePosition, alter, isRest);

    // if this is the last chord this is the first note: advance
    if(selectedChord->Next() == 0)
    {
        int dur = GetSelectedDuration();
        selectedChord->SetDuration(dur);

        // updating selected chord
        // note that the new duration can be different than the previous dur value
        selectedChord = sheetLine->Advance(GetDivisions());
        dur = selectedChord->Prev()->GetDuration();
        selectedChord->SetDuration(dur);
        SetDurationButton(dur);

        // updating selected measure
        if(selectedMeasure->Next() != 0)
        {
            selectedMeasure = selectedMeasure->Next();
        }

        // checking if a new line has been added
        if(!sheetLine->HasLastMeasure())
        {
            selectedSheetLine = sheetLine->Next();
            OnScroll(SCROLL_DOWN, scrollBar);
        }
    }
}

void pawsMusicWindow::GetMeter(int &b, int &bType)
{
    b = atoi(beats.GetData());
    bType = atoi(beatType.GetData());
}

uint pawsMusicWindow::GetDivisions()
{
    int b;          // integer beat
    int bType;      // integer beat type
    GetMeter(b, bType);

    return 16 / bType * b;
}


    //---------------------------//
    // FROM iSongManagerListener //
    //---------------------------//


void pawsMusicWindow::OnMainPlayerSongStop()
{
    playButton->SetState(false);
}


    //------------------------------//
    // FROM iOnMeterEnteredListener //
    //------------------------------//


void pawsMusicWindow::OnMeterEntered(const char* name, csString newBeats, csString newBeatType)
{
    pendingString = false;

    if(csStrCaseCmp(name, "Meter") == 0)
    {
        if(newBeats.Length() == 0 || newBeatType.Length() == 0)
        {
            return;
        }

        if(beats != newBeats || beatType != newBeatType)
        {
            Measure* m;

            beats = newBeats;
            beatType = newBeatType;
            int newDivisions = GetDivisions();

            // updating all measures
            for(m = measuresHead; m->Next() != 0; m = m->Next())
            {
                m->Cut(newDivisions);
                m->Fill(newDivisions);
            }

            // handling the last input measures
            if(m->Cut(newDivisions))
            {
                SheetLine* lastLine = GetSheetLine(-1);
                m->Fill(newDivisions);
                lastLine->Advance(newDivisions);
            }

            UpdateLines(linesHead, true);
        }
    }
}


    //-----------------------------//
    // FROM iOnStringEnteredAction //
    //-----------------------------//


void pawsMusicWindow::OnStringEntered(const char* name, int param, const char* value)
{
    pendingString = false;

    if(value == 0 || strlen(value) == 0)
    {
        // if the player click Cancel after clicking
        // the Play button, it remains down
        if(csStrCaseCmp(name, "PlayedBPM") == 0)
        {
            playButton->SetState(false);
        }

        return;
    }

    if(csStrCaseCmp(name, "Repeat") == 0)
    {
        int nRepeat = atoi(value);

        if(nRepeat < 0)
        {
            nRepeat = 0;
        }

        selectedMeasure->SetEndRepeat(nRepeat);
        endRepeatButton->SetState(nRepeat > 0);
    }
    else if(csStrCaseCmp(name, "Load") == 0)
    {
        // getting the file's name
        csString fileName = psengine->GetConfig()->GetStr("Planeshift.GUI.Music.Dir", "/planeshift/userdata/musicalsheets/");
        fileName.Append(value);

        // parsing
        csRef<iDocument> xml = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), fileName);

        if(xml.IsValid())
        {
            uint32_t tempItemID = currentItemID; // the item ID will be resetted when the previous sheet is unload

            if(!LoadXML(xml))
            {
                psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("Cannot load: illegal XML syntax!"));
                msg.FireEvent();
            }
            else
            {
                // this ensure that the score will be saved on hiding
                currentItemID = tempItemID;
                edited = true;
            }
        }
        else
        {
            psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("File not found or invalid!"));
            msg.FireEvent();
        }
    }
    else if(csStrCaseCmp(name, "Title") == 0)
    {
        SetTitle(value);
    }
    else if(csStrCaseCmp(name, "Fifths") == 0)
    {
        int newFifths = atoi(value); // when not valid 0 is returned

        // check correct input
        if(newFifths < -7 || newFifths > 7)
        {
            return;
        }

        fifths = newFifths;
    }
    else if(csStrCaseCmp(name, "BPM") == 0)
    {
        int newTempo = atoi(value);

        if(newTempo <= 0 || newTempo > 280)
        {
            return;
        }

        tempo = newTempo;
    }
    else if(csStrCaseCmp(name, "PlayedBPM") == 0)
    {
        int newPlayedTempo = atoi(value);

        if(newPlayedTempo <= 0 || newPlayedTempo > 280)
        {
            return;
        }

        playedTempo = newPlayedTempo;

        PlayStop();
    }
}


    //----------------------------//
    // FROM psClientNetSubscriber //
    //----------------------------//


void pawsMusicWindow::HandleMessage(MsgEntry* message)
{
    csRef<iDocument> sheet;
    csRef<iDocumentSystem> docSys;

    // decoding message
    psMusicalSheetMessage msg(message);

    // loading sheet
    docSys = csQueryRegistry<iDocumentSystem>(PawsManager::GetSingleton().GetObjectRegistry());
    sheet = docSys->CreateDocument();
    sheet->Parse(msg.musicalSheet.GetDataSafe(), true);

    //disable edit mode if we are loading a new sheet.
    editButton->SetState(false);

    if(!LoadXML(sheet))
    {
        psSystemMessage msg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("Cannot load: illegal XML syntax!"));
        msg.FireEvent();

        return;
    }

    // setting client number, readOnly and title
    currentItemID = msg.itemID;
    readOnly = msg.readOnly;
    SetTitle(msg.songTitle);

    //if the sheet is readonly hide also the edit button itself else show it
    editButton->SetVisibility(!readOnly);
    //disable also save and load if it's the case
    saveButton->SetVisibility(!readOnly);
    loadButton->SetVisibility(!readOnly);
    
    Show();
}


    //-----------------//
    // FROM pawsWidget //
    //-----------------//


double pawsMusicWindow::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    if(csStrCaseCmp(functionName, "ClickEditButton") == 0)
    {
        ToggleEditMode(editButton->GetState());
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickPlayButton") == 0)
    {
        if(playButton->GetState()) // play
        {
            SetPlayedBPM();
        }
        else
        {
            PlayStop();
        }
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickDotButton") == 0)
    {
        ChangeChordDuration();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickDeleteChordButton") == 0)
    {
        DeleteSelectedChord();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickDeleteMeasureButton") == 0)
    {
        DeleteSelectedMeasure();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickInsertMeasureButton") == 0)
    {
        InsertMeasure();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickStartRepeatButton") == 0)
    {
        selectedMeasure->SetStartRepeat(startRepeatButton->GetState());
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickEndRepeatButton") == 0)
    {
        SetEndRepeat();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickEndingButton") == 0)
    {
        SetEnding();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickLoadButton") == 0)
    {
        Load();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickSaveButton") == 0)
    {
        Save();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickDoubleStaffButton") == 0)
    {
        SwitchDoubleStaff();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickTonalityButton") == 0)
    {
        ChangeTonality();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickMeterButton") == 0)
    {
        ChangeMeter();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickBPMButton") == 0)
    {
        ChangeBPM();
        return 0.0;
    }
    else if(csStrCaseCmp(functionName, "ClickTitleButton") == 0)
    {
        ChangeSongTitle();
        return 0.0;
    }

    return pawsWidget::CalcFunction(env, functionName, params);
}

void pawsMusicWindow::Hide()
{
    if(edited)
    {
        csString xmlScore;              // uncompressed musical sheet in XML format

        if(editButton->GetState()) // score is inconsistent
        {
            ToggleEditMode(false);
        }

        // creating XML score
        xmlScore = ToXML(USE_SCORE_TEMPO);

        // sending message
        psMusicalSheetMessage message(0, currentItemID, readOnly, false, border->GetTitle(), xmlScore);
        message.SendMessage();
    }
    pawsWidget::Hide();
}

bool pawsMusicWindow::PostSetup()
{
    // retrieving tools
    editButton = static_cast<pawsButton*>(FindWidget("EditButton"));
    playButton = static_cast<pawsButton*>(FindWidget("PlayButton"));
    loadButton = FindWidget("LoadButton");
    saveButton = FindWidget("SaveButton");
    titleButton = FindWidget("TitleButton");

    doubleStaffButton = static_cast<pawsButton*>(FindWidget("DoubleStaffButton"));
    tonalityButton = FindWidget("TonalityButton");
    meterButton = FindWidget("MeterButton");
    bpmButton = FindWidget("BPMButton");
    restButton = static_cast<pawsButton*>(FindWidget("RestButton"));

    sixteenthButton = static_cast<pawsButton*>(FindWidget("SixteenthButton"));
    eighthButton = static_cast<pawsButton*>(FindWidget("EighthButton"));
    quarterButton = static_cast<pawsButton*>(FindWidget("QuarterButton"));
    halfButton = static_cast<pawsButton*>(FindWidget("HalfButton"));
    wholeButton = static_cast<pawsButton*>(FindWidget("WholeButton"));
    dotButton = static_cast<pawsButton*>(FindWidget("DotButton"));

    flatButton = static_cast<pawsButton*>(FindWidget("FlatButton"));
    // naturalButton = static_cast<pawsButton*>(FindWidget("NaturalButton"));
    sharpButton = static_cast<pawsButton*>(FindWidget("SharpButton"));

    deleteChordButton = FindWidget("DeleteChordButton");
    deleteMeasureButton = FindWidget("DeleteMeasureButton");
    insertNoteButton = static_cast<pawsButton*>(FindWidget("InsertNoteButton"));
    insertMeasureButton = FindWidget("InsertMeasureButton");

    startRepeatButton = static_cast<pawsButton*>(FindWidget("StartRepeatButton"));
    endRepeatButton = static_cast<pawsButton*>(FindWidget("EndRepeatButton"));
    endingButton = static_cast<pawsButton*>(FindWidget("EndingButton"));

    pawsLine1 = static_cast<pawsSheetLine*>(FindWidget("FirstLine"));
    pawsLine2 = static_cast<pawsSheetLine*>(FindWidget("SecondLine"));

    scrollBar = static_cast<pawsScrollBar*>(FindWidget("ScrollBar"));

    // default selected buttons
    quarterButton->SetState(true);

    // hide edit buttons
    SetToolbarButtons();
    UpdateScrollBar();

    return true;
}

bool pawsMusicWindow::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    int widgetID = widget->GetID();

    // duration buttons
    if(SIXTEENTH_BUTTON <= widgetID && widgetID <= WHOLE_BUTTON)
    {
        SelectDuration(widgetID);
    }
    // alteration buttona
    else if(FLAT_BUTTON <= widgetID && widgetID <= SHARP_BUTTON)
    {
        SelectAlteration(widgetID);
    }

    return true;
}

bool pawsMusicWindow::OnScroll(int scrollDirection, pawsScrollBar* widget)
{
    bool updateScrollBar = false;
    SheetLine* sheetLine;

    switch(scrollDirection)
    {
    case SCROLL_UP:
        sheetLine = pawsLine1->GetLine();

        if(sheetLine->HasFirstMeasure())
        {
            return true;
        }
        
        pawsLine1->SetLine(sheetLine->Prev());
        pawsLine2->SetLine(sheetLine);
        updateScrollBar = true;
        break;

    case SCROLL_DOWN:
        sheetLine = pawsLine2->GetLine();

        if(sheetLine == 0) // only the first line is displayed
        {
            pawsLine2->SetLine(linesHead->Next());
            return true;
        }

        if(sheetLine->HasLastMeasure())
        {
            return true;
        }

        pawsLine1->SetLine(sheetLine);
        pawsLine2->SetLine(sheetLine->Next());
        updateScrollBar = true;
        break;

    case SCROLL_THUMB:
        int currentTick = scrollBar->GetCurrentValue() / scrollBar->GetTickValue();
        pawsLine1->SetLine(GetSheetLine(currentTick));
        pawsLine2->SetLine(GetSheetLine(currentTick + 1));
        // the scroll bar should not be updated
        break;
    }

    // updating
    if(updateScrollBar)
    {
        UpdateScrollBar();
    }

    return true;
}


    //------------------//
    // HELPER FUNCTIONS //
    //------------------//


void pawsMusicWindow::OnSheetLineCallback(void* object, SheetLine* sheetLine)
{
    pawsMusicWindow* musicWindow = (pawsMusicWindow*)object;

    if(sheetLine->Prev() == 0 && sheetLine->Next() == 0
        && sheetLine == musicWindow->selectedSheetLine)
    {
        musicWindow->selectedSheetLine = 0;
    }

    musicWindow->UpdateScrollBar();
}

void pawsMusicWindow::Unload()
{
    DeleteAll();
    ResetState();
    SetToolbarButtons();
}

bool pawsMusicWindow::LoadXML(csRef<iDocument> sheet)
{
    int quarterDivisions;
    int scoreFifths;
    int iBeats;
    int iBeatType;
    int scoreTempo;

    Measure* lastMeasure;
    csRef<iDocumentNode> rootNode;
    csRefArray<iDocumentNode> measures;

    if(sheet == 0)
    {
        return false;
    }

    // checking if this is an empty score
    rootNode = sheet->GetRoot();
    if(rootNode == 0)
    {
        Unload();
        return true;
    }

    // getting measures
    if(!psMusic::GetMeasures(sheet, measures))
    {
        return false;
    }

    //if there are no measures it's still valid
    if(measures.IsEmpty())
    {
        Unload();
        return true;
    }

    // getting attributes. It's necessary to provide temporary attributes because
    // if GetAttributes is successful the previous sheet will be unloaded and the
    // attributes resetted.
    if(!psMusic::GetAttributes(sheet, quarterDivisions, scoreFifths, iBeats, iBeatType, scoreTempo))
    {
        return false;
    }

    // everything is ok, unload previous one
    Unload();

    // updating attributes
    fifths = scoreFifths;
    beats.Empty();
    beatType.Empty();
    beats.Append(iBeats);
    beatType.Append(iBeatType);
    tempo = scoreTempo;
    playedTempo = scoreTempo;

    // creating data structures
    measuresHead = new Measure(measures[0], quarterDivisions);
    lastMeasure = measuresHead;

    for(size_t i = 1; i < measures.GetSize(); i++)
    {
        lastMeasure->AttachMeasure(new Measure(measures[i], quarterDivisions));
        lastMeasure = lastMeasure->Next();
    }

    // adding lines and display
    linesHead = new SheetLine(measuresHead);
    linesHead->SetCallback(this, &OnSheetLineCallback);

    // setting size
    pawsLine1->SetLine(linesHead, false);

    // updating selection
    selectedSheetLine = linesHead;
    selectedMeasure = measuresHead;

    // resizing and display
    UpdateLines(linesHead, false);

    return true;
}

csString pawsMusicWindow::ToXML(bool usePlayedTempo)
{
    csString attributes = "<attributes>";
    csString doc = "<score-partwise version=\"2.0\">";

    // doc header
    doc += "<part-list><score-part id=\"P1\">";

    doc.AppendFmt("<part-name>%s</part-name>", border->GetTitle());

    doc += "</score-part></part-list><part id=\"P1\">";

    // attibutes and direction
    attributes.AppendFmt("<divisions>%d</divisions><key><fifths>%d</fifths></key>", 4, fifths);
    attributes.AppendFmt("<time><beats>%s</beats><beat-type>%s</beat-type></time></attributes>", beats.GetData(), beatType.GetData());
    if(usePlayedTempo)
    {
        attributes.AppendFmt("<direction><sound tempo=\"%d\"/></direction>", playedTempo);
    }
    else
    {
        attributes.AppendFmt("<direction><sound tempo=\"%d\"/></direction>", tempo);
    }

    // measures
    int measureNum = 1;
    for(Measure* m = measuresHead; m != 0; m = m->Next())
    {
        if(m == measuresHead)
        {
            doc += m->ToXML(measureNum, attributes);
        }
        else
        {
            doc += m->ToXML(measureNum);
        }

        measureNum++;
    }

    doc += "</part></score-partwise>";

    return doc;
}

SheetLine* pawsMusicWindow::GetSheetLine(int index)
{
    SheetLine* sheetLine;

    if(linesHead == 0)
    {
        return 0;
    }

    sheetLine = linesHead;

    if(index < 0)
    {
        while(sheetLine->Next() != 0)
        {
            sheetLine = sheetLine->Next();
        }
    }
    else
    {
        int count = 0;
        while(count < index && sheetLine != 0)
        {
            sheetLine = sheetLine->Next();
            count++;
        }
    }

    return sheetLine;
}

SheetLine* pawsMusicWindow::GetSelectedSheetLine()
{
    SheetLine* sheetLine;

    // checking if there is a selection
    if(selectedMeasure == 0)
    {
        return 0;
    }

    // checking cache variable
    if(selectedSheetLine != 0)
    {
        if(selectedSheetLine->Contains(selectedMeasure))
        {
            return selectedSheetLine;
        }
    }

    // getting selected SheetLine
    for(sheetLine = linesHead; sheetLine != 0; sheetLine = sheetLine->Next())
    {
        if(sheetLine->Contains(selectedMeasure))
        {
            break;
        }
    }

    // updating selection
    selectedSheetLine = sheetLine;

    return sheetLine;
}

void pawsMusicWindow::UpdateScrollBar()
{
    int nCurrentLine;
    float newTickValue;

    int nSheetLine = 0;
    SheetLine* currentSheetLine = pawsLine1->GetLine();

    // counting sheet lines
    for(SheetLine* l = linesHead; l != 0; l = l->Next())
    {
        nSheetLine++;

        if(l == currentSheetLine)
        {
            nCurrentLine = nSheetLine;
        }
    }

    if(nSheetLine < 3)
    {
        scrollBar->Hide();
        return;
    }
    else
    {
        scrollBar->Show();
    }

    // updating tick value
    newTickValue = scrollBar->GetMaxValue() / (nSheetLine - 2);
    scrollBar->SetTickValue(newTickValue);
    
    // updating position
    scrollBar->SetCurrentValue(newTickValue * (nCurrentLine - 1));
}

void pawsMusicWindow::UpdateLines(SheetLine* line, bool forceAll)
{
    line->ResizeAll(forceAll);
    DisplaySelectedMeasure();
}

void pawsMusicWindow::DisplaySelectedMeasure()
{
    SheetLine* sheetLine = GetSelectedSheetLine();

    if(sheetLine == 0)
    {
        return;
    }

    if(linesHead->Next() == 0)
    {
        pawsLine1->SetLine(sheetLine);
        pawsLine2->SetLine(0);
    }
    else if(sheetLine->Next() != 0 && pawsLine1->GetLine() == sheetLine)
    {
        pawsLine2->SetLine(sheetLine->Next());
    }
    else
    {
        pawsLine1->SetLine(sheetLine->Prev());
        pawsLine2->SetLine(sheetLine);
    }
    
    UpdateScrollBar();
}

pawsButton* pawsMusicWindow::GetButton(int buttonID)
{
    switch(buttonID)
    {
    case SIXTEENTH_BUTTON:
        return sixteenthButton;
    case EIGHTH_BUTTON:
        return eighthButton;
    case QUARTER_BUTTON:
        return quarterButton;
    case HALF_BUTTON:
        return halfButton;
    case WHOLE_BUTTON:
        return wholeButton;
    case FLAT_BUTTON:
        return flatButton;
    /* case NATURAL_BUTTON:
        return naturalButton; */
    case SHARP_BUTTON:
        return sharpButton;
    }

    return 0;
}

void pawsMusicWindow::DeleteSelectedChord()
{
    if(selectedMeasure == 0)
    {
        return;
    }

    selectedMeasure->DeleteChord(selectedChord);
    SetInsertMode(true);

    // Resizing selected line
    SheetLine* sheetLine = GetSelectedSheetLine();
    UpdateLines(sheetLine, false);
}

void pawsMusicWindow::DeleteSelectedMeasure()
{
    Measure* nextMeasure;
    SheetLine* selLine;

    if(selectedMeasure == 0)
    {
        return;
    }

    selLine = GetSelectedSheetLine();
    nextMeasure = selectedMeasure->Next();

    // checking if this is the first one adjust measuresHead
    if(selectedMeasure == measuresHead && nextMeasure != 0)
    {
        measuresHead = nextMeasure;
    }

    selLine->DeleteMeasure(selectedMeasure);

    // updating
    // if nextMeasure is null selectedMeasure has just been emptied
    if(nextMeasure != 0)
    {
        selectedMeasure = nextMeasure;
    }

    UpdateLines(selLine, false);
    insertMode = false; // important if the old selected measure was in insert mode
}

void pawsMusicWindow::InsertMeasure()
{
    Measure* newMeasure;
    SheetLine* selLine;

    if(selectedMeasure == 0)
    {
        return;
    }
    selLine = GetSelectedSheetLine();

    // inserting
    newMeasure = selLine->InsertNewMeasure(selectedMeasure);

    // adjusting measures head
    if(selectedMeasure == measuresHead)
    {
        measuresHead = newMeasure;
    }

    // updating
    selectedMeasure = newMeasure;
    selectedChord = newMeasure->GetFirstChord();
    SetInsertMode(true);
    UpdateLines(selLine, false);
}

void pawsMusicWindow::SetInsertMode(bool toggle)
{
    if(toggle == insertMode)
    {
        return;
    }

    if(toggle)
    {
        // if this is the last chord do not entry in insert mode
        if(selectedMeasure->Next() != 0)
        {
            insertMode = true;
        }
    }
    else
    {
        uint divisions = GetDivisions();
        selectedMeasure->Cut(divisions);
        selectedMeasure->Fill(divisions);

        // getting and resizing the measure's line
        SheetLine* sheetLine = GetSelectedSheetLine();
        UpdateLines(sheetLine, false);

        insertMode = false;
    }
}

void pawsMusicWindow::ToggleEditMode(bool toggle)
{
    SheetLine* lastLine;
    Measure* lastMeasure;

    if(readOnly)
    {
        psSystemMessage sysMsg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("You cannot edit this musical sheet."));
        sysMsg.FireEvent();
        return;
    }

    SetToolbarButtons();

    // toggle on
    if(toggle)
    {
        // creating input measure
        lastMeasure = new Measure(new Chord());

        if(linesHead == 0)
        {
            linesHead = new SheetLine(lastMeasure);
            linesHead->SetCallback(this, &OnSheetLineCallback);
            measuresHead = lastMeasure;

            lastLine = linesHead;
        }
        else
        {
            lastLine = GetSheetLine(-1);
            lastLine->PushMeasure(lastMeasure);
        }

        // updating selection
        selectedSheetLine = lastLine;
        selectedMeasure = lastMeasure;
        selectedChord = lastMeasure->GetLastChord();

        UpdateLines(lastLine, false);
        edited = true;
    }

    // toggle off
    else
    {
        if(insertMode)
        {
            SetInsertMode(false);
        }

        // updating selection
        selectedChord = 0;
        selectedMeasure = 0;
        selectedSheetLine = 0;

        // getting last line and measure
        lastLine = GetSheetLine(-1);
        lastMeasure = lastLine->GetLastMeasure();

        // checking if the measure contains something
        if(lastMeasure->GetNChords() > 1)
        {
            lastMeasure->Fill(GetDivisions());
            UpdateLines(lastLine, false);
            return;
        }

        // the last measure must be deleted
        // is this the only measure?
        if(lastMeasure->Prev() == 0)
        {
            pawsLine1->SetLine(0);
            measuresHead = 0;
            linesHead = 0;
        }

        // does this line contain only this measure?
        if(lastMeasure == lastLine->GetFirstMeasure())
        {
            if(pawsLine2->GetLine() == lastLine)
            {
                if(pawsLine1->GetLine()->HasFirstMeasure())
                {
                    pawsLine2->SetLine(0);
                }
                else
                {
                    OnScroll(SCROLL_UP, scrollBar);
                }
            }

            delete lastLine;
            delete lastMeasure;
        }
        else
        {
            delete lastMeasure;
            lastLine->ResizeAll();
        }
    }
}

void pawsMusicWindow::PlayStop()
{
    if(playButton->GetState()) // play
    {
        if(editButton->GetState()) // sheet not consistent
        {
            ToggleEditMode(false);
            psengine->GetSongManager()->PlayMainPlayerSong(currentItemID, ToXML(USE_PLAYED_TEMPO));
            ToggleEditMode(true);
        }
        else
        {
            psengine->GetSongManager()->PlayMainPlayerSong(currentItemID, ToXML(USE_PLAYED_TEMPO));
        }
    }
    else // stop
    {
        psengine->GetSongManager()->StopMainPlayerSong(true);
    }
}

void pawsMusicWindow::Save()
{
    csString fileName;
    csString path;
    csString xml;
    iVFS* vfs = psengine->GetVFS();
    csRef<iConfigManager> configMgr = psengine->GetConfig();

    // if in edit mode the sheet is not consistent
    if(editButton->GetState())
    {
        ToggleEditMode(false);
        xml = ToXML(USE_SCORE_TEMPO);
        ToggleEditMode(true);
    }
    else
    {
        xml = ToXML(USE_SCORE_TEMPO);
    }

    // getting the file's name
    path = configMgr->GetStr("Planeshift.GUI.Music.Dir", "/planeshift/userdata/musicalsheets/");
    fileName.AppendFmt("%s.xml", border->GetTitle());
    
    // writing
    if(vfs->WriteFile(path + fileName, xml, xml.Length()))
    {
        psSystemMessage msg(0, MSG_ACK, "Sheet saved to %s", fileName.GetData());
        msg.FireEvent();
    }
    else
    {
        psSystemMessage msg(0, MSG_ERROR, "Cannot save the sheet to %s", fileName.GetData());
        msg.FireEvent();
    }
}

void pawsMusicWindow::Load()
{
    if(readOnly)
    {
        psSystemMessage sysMsg(0, MSG_ERROR, PawsManager::GetSingleton().Translate("You cannot edit this musical sheet."));
        sysMsg.FireEvent();
        return;
    }

    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // This window calls OnStringEntered when Ok is pressed.
    pawsStringPromptWindow::Create("File name", "",
        false, 220, 20, this, "Load");
}

void pawsMusicWindow::ChangeSongTitle()
{
    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // This window calls OnStringEntered when Ok is pressed.
    pawsStringPromptWindow::Create("Song Title", border->GetTitle(),
        false, 220, 20, this, "Title");
}

void pawsMusicWindow::SwitchDoubleStaff()
{
    bool newStaffStatus = !doubleStaffButton->GetState();
    pawsLine1->SetDoubleStaff(newStaffStatus);
    pawsLine2->SetDoubleStaff(newStaffStatus);

    // resizing
    for(SheetLine* s = linesHead; s != 0; s = s->Next())
    {
        pawsLine1->SetLine(s, false);
    }

    UpdateLines(linesHead, true);
}

void pawsMusicWindow::ChangeTonality()
{
    csString strFifths;

    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // This window calls OnStringEntered when Ok is pressed.
    strFifths = fifths;
    pawsStringPromptWindow::Create("Number of alterations", strFifths,
        false, 220, 20, this, "Fifths");
}

void pawsMusicWindow::ChangeMeter()
{
    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // This window calls OnStringEntered when Ok is pressed.
    pawsMeterPromptWindow::Create("Meter", beats, beatType, 2, this);
}

void pawsMusicWindow::ChangeBPM()
{
    csString strTempo;

    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // This window calls OnStringEntered when Ok is pressed.
    strTempo = tempo;
    pawsStringPromptWindow::Create("BPM", strTempo,
        false, 220, 20, this, "BPM");
}

void pawsMusicWindow::SetPlayedBPM()
{
    csString strTempo;

    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // This window calls OnStringEntered when Ok is pressed.
    strTempo = playedTempo;
    pawsStringPromptWindow::Create("Played BPM", strTempo,
        false, 220, 20, this, "PlayedBPM");
}

void pawsMusicWindow::ChangeChordDuration()
{
    int currentDuration;

    if(selectedChord->IsEmpty())
    {
        return;
    }

    // setting duration
    currentDuration = GetSelectedDuration();
    selectedChord->SetDuration(currentDuration);
    SetInsertMode(true);
}

void pawsMusicWindow::SetEndRepeat()
{
    csString strEndRepeat;

    if(pendingString)
    {
        return;
    }

    pendingString = true;

    // restoring buttons state
    endRepeatButton->SetState(selectedMeasure->GetEndRepeat() > 0);

    // This window calls OnStringEntered when Ok is pressed.
    strEndRepeat = selectedMeasure->GetEndRepeat();
    pawsStringPromptWindow::Create("Number of repetition", strEndRepeat,
        false, 220, 20, this, "Repeat");
}

void pawsMusicWindow::SetEnding()
{
    selectedMeasure->SetEnding(endingButton->GetState());

    // updating repeat buttons
    if(selectedMeasure->GetEndRepeat() == 0)
    {
        endRepeatButton->SetState(false);
    }
    else
    {
        endRepeatButton->SetState(true);
    }
}

int pawsMusicWindow::GetSelectedDuration()
{
    switch(selectedDurButton)
    {
    case SIXTEENTH_BUTTON:
        return SIXTEENTH_DURATION;
    case EIGHTH_BUTTON:
        if(dotButton->GetState())
            return DOTTED_EIGHTH_DURATION;
        else
            return EIGHTH_DURATION;
    case QUARTER_BUTTON:
        if(dotButton->GetState())
            return DOTTED_QUARTER_DURATION;
        else
            return QUARTER_DURATION;
    case HALF_BUTTON:
        if(dotButton->GetState())
            return DOTTED_HALF_DURATION;
        else
            return HALF_DURATION;
    case WHOLE_BUTTON:
        if(dotButton->GetState())
            return DOTTED_WHOLE_DURATION;
        else
            return WHOLE_DURATION;
    }

    return 0;
}

void pawsMusicWindow::SetDurationButton(int duration)
{
    switch(duration)
    {
    case SIXTEENTH_DURATION:
        SelectDuration(SIXTEENTH_BUTTON);
        dotButton->SetState(false);
        return;
    case EIGHTH_DURATION:
        SelectDuration(EIGHTH_BUTTON);
        dotButton->SetState(false);
        return;
    case DOTTED_EIGHTH_DURATION:
        SelectDuration(EIGHTH_BUTTON);
        dotButton->SetState(true);
        return;
    case QUARTER_DURATION:
        SelectDuration(QUARTER_BUTTON);
        dotButton->SetState(false);
        return;
    case DOTTED_QUARTER_DURATION:
        SelectDuration(QUARTER_BUTTON);
        dotButton->SetState(true);
        return;
    case HALF_DURATION:
        SelectDuration(HALF_BUTTON);
        dotButton->SetState(false);
        return;
    case DOTTED_HALF_DURATION:
        SelectDuration(HALF_BUTTON);
        dotButton->SetState(true);
        return;
    case WHOLE_DURATION:
        SelectDuration(WHOLE_BUTTON);
        dotButton->SetState(false);
        return;
    case DOTTED_WHOLE_DURATION:
        SelectDuration(WHOLE_BUTTON);
        dotButton->SetState(true);
        return;
    }
}

void pawsMusicWindow::SelectDuration(int buttonID)
{
    pawsButton* durationButton;

    // deselecting previous one
    if(selectedDurButton != buttonID)
    {
        durationButton = GetButton(selectedDurButton);
        durationButton->SetState(false);
    }

    // selecting new one
    durationButton = GetButton(buttonID);
    durationButton->SetState(true);

    if(selectedDurButton != buttonID)
    {
        selectedDurButton = buttonID;
        ChangeChordDuration();
    }
    else
    {
        selectedDurButton = buttonID;
    }
}

void pawsMusicWindow::SelectAlteration(int buttonID)
{
    pawsButton* alterButton;

    // deselecting previous one
    if(selectedAlterButton == buttonID)
    {
        selectedAlterButton = NO_SELECTED;
        return;
    }

    // deselecting previous one
    if(selectedAlterButton != NO_SELECTED)
    {
        alterButton = GetButton(selectedAlterButton);
        alterButton->SetState(false);
    }

    // selecting new one
    if(buttonID != NO_SELECTED)
    {
        selectedAlterButton = buttonID;
        alterButton = GetButton(selectedAlterButton);
        alterButton->SetState(true);
    }
}

void pawsMusicWindow::SetToolbarButtons()
{
    if(editButton->GetState())
    {
        titleButton->Show();
        doubleStaffButton->Show();
        tonalityButton->Show();
        meterButton->Show();
        bpmButton->Show();
        restButton->Show();
        sixteenthButton->Show();
        eighthButton->Show();
        quarterButton->Show();
        halfButton->Show();
        wholeButton->Show();
        dotButton->Show();
        flatButton->Show();
        // naturalButton->Show();
        sharpButton->Show();
        deleteChordButton->Show();
        deleteMeasureButton->Show();
        insertNoteButton->Show();
        insertMeasureButton->Show();
        startRepeatButton->Show();
        endRepeatButton->Show();
        endingButton->Show();
    }
    else
    {
        titleButton->Hide();
        doubleStaffButton->Hide();
        tonalityButton->Hide();
        meterButton->Hide();
        bpmButton->Hide();
        restButton->Hide();
        sixteenthButton->Hide();
        eighthButton->Hide();
        quarterButton->Hide();
        halfButton->Hide();
        wholeButton->Hide();
        dotButton->Hide();
        flatButton->Hide();
        // naturalButton->Hide();
        sharpButton->Hide();
        deleteChordButton->Hide();
        deleteMeasureButton->Hide();
        insertNoteButton->Hide();
        insertMeasureButton->Hide();
        startRepeatButton->Hide();
        endRepeatButton->Hide();
        endingButton->Hide();
    }
}

void pawsMusicWindow::ResetState()
{
    fifths = 0;
    beats = "4";
    beatType = "4";
    tempo = 90;
    playedTempo = 90;

    linesHead = 0;
    measuresHead = 0;

    selectedChord = 0;
    selectedMeasure = 0;
    selectedSheetLine = 0;

    selectedDurButton = QUARTER_BUTTON;
    selectedAlterButton = NO_SELECTED;

    currentItemID = 0;
    readOnly = false;
    edited = false;
    pendingString = false;
    insertMode = false;

    // stopping played song
    if(playButton != 0)
    {
        PlayStop();
    }
}

void pawsMusicWindow::DeleteAll()
{
    Measure* measureTemp;
    SheetLine* lineTemp;

    // deleting measures
    while(measuresHead != 0)
    {
        measureTemp = measuresHead->Next();
        delete measuresHead;
        measuresHead = measureTemp;
    }

    // deleting lines
    while(linesHead != 0)
    {
        lineTemp = linesHead->Next();
        delete linesHead;
        linesHead = lineTemp;
    }

    // updating pawsLines
    pawsLine1->SetLine(0);
    pawsLine2->SetLine(0);
}
