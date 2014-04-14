/*
 * pawsmeterpromptwindow.cpp, Author: Andrea Rizzi <88whacko@gmail.com>
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
#include "pawsmeterpromptwindow.h"

//====================================================================================
// Local Includes
//====================================================================================
#include "pawscombo.h"
#include "pawsbutton.h"
#include "pawstextbox.h"
#include "pawsmainwidget.h"


pawsMeterInput::pawsMeterInput()
{
    SetRelativeFrameSize(170, 200);

    beatsLabel = new pawsTextBox();
    beatsLabel->PostSetup();
    beatsLabel->SetText("Beats");
    beatsLabel->SetSizeByText(5,5);
    beatsLabel->SetRelativeFramePos(1, 1);

    beatTypeLabel = new pawsTextBox();
    beatTypeLabel->PostSetup();
    beatTypeLabel->SetText("Beat Type");
    beatTypeLabel->SetSizeByText(5,5);
    beatTypeLabel->SetRelativeFramePos(1, 2 + beatsLabel->GetActualHeight());

    beatTypeInput = new pawsComboBox();
    beatTypeInput->SetRelativeFrame(2 + beatTypeLabel->GetActualWidth(), 2 + beatsLabel->GetActualHeight(), 60, 30);
    beatTypeInput->SetNumRows(5);
    beatTypeInput->SetRowHeight(25);
    beatTypeInput->SetSorted(false);
    beatTypeInput->PostSetup();

    beatTypeInput->NewOption(csString("1"));
    beatTypeInput->NewOption(csString("2"));
    beatTypeInput->NewOption(csString("4"));
    beatTypeInput->NewOption(csString("8"));
    beatTypeInput->NewOption(csString("16"));

    beatsInput = new pawsEditTextBox();
    beatsInput->SetRelativeFrame(beatTypeLabel->GetActualWidth(), 1, beatTypeInput->GetActualWidth(), beatsLabel->GetActualHeight());
    beatsInput->PostSetup();
    beatsInput->UseBorder();

    AddChild(beatsLabel);
    AddChild(beatTypeLabel);
    AddChild(beatsInput);
    AddChild(beatTypeInput);
}

pawsMeterInput::pawsMeterInput(const pawsMeterInput &origin)
{
    SetRelativeFrameSize(origin.screenFrame.Width(), origin.screenFrame.Height());

    beatsLabel = new pawsTextBox(*origin.beatsLabel);
    beatTypeLabel = new pawsTextBox(*origin.beatsLabel);
    beatsInput = new pawsEditTextBox(*origin.beatsInput);
    beatTypeInput = new pawsComboBox(*origin.beatTypeInput);

    AddChild(beatsLabel);
    AddChild(beatTypeLabel);
    AddChild(beatsInput);
    AddChild(beatTypeInput);
}

void pawsMeterInput::Initialize(const char* initialBeats, const char* initialBeatType, size_t beatsMaxLength)
{
    beatsInput->SetText(initialBeats);
    beatsInput->SetMaxLength(beatsMaxLength);

    beatTypeInput->Select(initialBeatType);

    PawsManager::GetSingleton().SetCurrentFocusedWidget(beatsInput);
}

const char* pawsMeterInput::GetBeats()
{
    return beatsInput->GetText();
}

csString pawsMeterInput::GetBeatType()
{
    return beatTypeInput->GetSelectedRowString();
}

//------------------------------------------------------------------------------------

pawsMeterPromptWindow::pawsMeterPromptWindow()
{
    meterListener = 0;
}

pawsMeterPromptWindow::pawsMeterPromptWindow(const pawsMeterPromptWindow &origin)
{
    pawsMeterInput* meterInput = new pawsMeterInput(*(static_cast<pawsMeterInput*>(origin.inputWidget)));
    AddChild(meterInput);
    inputWidget = meterInput;

    actionName = origin.actionName;
    meterListener = origin.meterListener;
}

void pawsMeterPromptWindow::Initialize(const char* actionName, const char* title, const char* initialBeats,
                                       const char* initialBeatType, size_t beatsMaxLength, iOnMeterEnteredListener* listener)
{
    SetLabel(csString(title));

    this->actionName = actionName;
    this->meterListener = listener;
    static_cast<pawsMeterInput*>(inputWidget)->Initialize(initialBeats, initialBeatType, beatsMaxLength);
}

bool pawsMeterPromptWindow::PostSetup()
{
    pawsPromptWindow::PostSetup();

    pawsMeterInput* meterInput = new pawsMeterInput();
    AddChild(meterInput);
    inputWidget = meterInput;

    return true;
}

bool pawsMeterPromptWindow::OnButtonPressed(int /* mouseButton */, int /* keyModifier */, pawsWidget* widget)
{
    if(meterListener == 0)
    {
        return false;
    }

    if(widget == okButton)
    {
        pawsMeterInput* meterInput = static_cast<pawsMeterInput*>(inputWidget);
        int beats = atoi(meterInput->GetBeats());

        // checking user input
        if(beats > 0)
        {
            csString newBeats;
            csString newBeatType;

            // this conversion cuts eventual whitespaces in the meterInput's text box
            newBeats = beats;
            newBeatType = meterInput->GetBeatType();

            meterListener->OnMeterEntered(actionName, newBeats, newBeatType);
        }

        parent->DeleteChild(this);
        return true;
    }
    else if(widget == cancelButton)
    {
        parent->DeleteChild(this);
        return true;
    }

    return false;
}

pawsMeterPromptWindow* pawsMeterPromptWindow::Create(const char* actionName, const char* initialBeats, const char* initialBeatType,
        size_t beatsMaxLength, iOnMeterEnteredListener* listener)
{
    pawsMeterPromptWindow* meterPrompt = new pawsMeterPromptWindow();
    PawsManager::GetSingleton().GetMainWidget()->AddChild(meterPrompt);
    PawsManager::GetSingleton().SetCurrentFocusedWidget(meterPrompt);

    meterPrompt->PostSetup();
    meterPrompt->BringToTop(meterPrompt);
    meterPrompt->SetMovable(true);
    meterPrompt->UseBorder();
    meterPrompt->SetBackground("Scaling Window Background");
    meterPrompt->Initialize(actionName, "Change Meter", initialBeats, initialBeatType, beatsMaxLength, listener);

    return meterPrompt;
}
