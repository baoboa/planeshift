/*
 * pawsnumberpromptwindow.cpp - Author: Ondrej Hurt
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


// CS INCLUDES
#include <psconfig.h>
#include <ctype.h>

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawsnumberpromptwindow.h"
#include "pawstextbox.h"
#include "pawsbutton.h"
#include "pawscrollbar.h"
#include "pawsmanager.h"
#include "pawsmainwidget.h"

class pawsNumberInput : public pawsWidget
{
public:
    pawsNumberInput()
    {
        SetRelativeFrameSize(110, 60);

        editBox = new pawsEditTextBox();
        AddChild(editBox);
        editBox->SetColour(graphics2D->FindRGB(255,255,255));
        editBox->SetRelativeFrame(20, 1, 70, 20);
        editBox->SetBackground("Scaling Field Background");

        scrollBar = new pawsScrollBar();
        scrollBar->SetHorizontal(true);
        scrollBar->SetRelativeFrame(0, 33, 110, 20);
        scrollBar->SetTickValue(1);
        scrollBar->PostSetup();
        AddChild(scrollBar);

    }

    pawsNumberInput(const pawsNumberInput &origin):pawsWidget(origin)
    {
        editBox = 0;
        scrollBar = 0;
        for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
        {
            if(origin.editBox == origin.children[i])
                editBox = dynamic_cast<pawsEditTextBox*>(children[i]);
            else if(origin.scrollBar == origin.children[i])
                scrollBar = dynamic_cast<pawsScrollBar*>(children[i]);

            if(editBox != 0 && scrollBar!=0) break;
        }
    }

    pawsEditTextBox* editBox;
    pawsScrollBar* scrollBar;
};

pawsNumberPromptWindow::pawsNumberPromptWindow()
{
    editBox        = NULL;
    scrollBar      = NULL;
    action         = NULL;

    SetBoundaries(0, 0);
    lastValidText.Clear();

    SetSpacing(5);
    factory = "pawsNumberPromptWindow";
}
pawsNumberPromptWindow::pawsNumberPromptWindow(const pawsNumberPromptWindow &origin)
    :pawsPromptWindow(origin),
     maxNumber(origin.maxNumber),
     minNumber(origin.minNumber),
     maxDigits(origin.maxDigits),
     lastValidText(origin.lastValidText),
     action(origin.action),
     name(origin.name),
     param(origin.param)
{
    pawsNumberInput* input = dynamic_cast<pawsNumberInput*>(inputWidget);
    if(input)
    {
        editBox = input->editBox;
        scrollBar = input->scrollBar;
    }
}
bool pawsNumberPromptWindow::PostSetup()
{
    pawsPromptWindow::PostSetup();

    pawsNumberInput* numberInput = new pawsNumberInput();
    AddChild(numberInput);
    inputWidget = numberInput;
    editBox    = numberInput->editBox;
    scrollBar  = numberInput->scrollBar;

    //LayoutWindow();

    return true;
}

void pawsNumberPromptWindow::Close()
{
    Hide();
    if(action != NULL)
    {
        action->OnNumberEntered("Cancel",0,-1);
        action = NULL;
    }
    parent->DeleteChild(this);
}

void pawsNumberPromptWindow::LayoutWindow()
{
    pawsPromptWindow::LayoutWindow();
    inputWidget->SetRelativeFramePos(inputWidget->GetDefaultFrame().xmin, inputWidget->GetDefaultFrame().ymin-3);
}

bool pawsNumberPromptWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(action == NULL)
        return false;

    if(widget == okButton)
    {
        if(TextIsValidForOutput(editBox->GetText()))
            NumberWasEntered(atoi(editBox->GetText()));
        return true;
    }
    else if(widget == cancelButton)
    {
        Close();
        return true;
    }
    return false;
}

void pawsNumberPromptWindow::SetBoundaries(int minNumber, int maxNumber)
{
    csString numberStr;
    int div;

    this->minNumber = minNumber;
    this->maxNumber = maxNumber;

    if(scrollBar != NULL)
    {
        scrollBar->SetMinValue(minNumber);
        scrollBar->SetMaxValue(maxNumber);
    }

    if(label != NULL)
    {
        if(!strcmp(label->GetText(),""))
        {
            numberStr.Format("%d",maxNumber);
            label->SetText("Max "+numberStr);
        }
    }

    maxDigits = 0;
    div = maxNumber;
    while(div > 0)
    {
        maxDigits++;
        div /= 10;
    }
}

bool pawsNumberPromptWindow::OnScroll(int /*scrollDirection*/, pawsScrollBar* /*widget*/)
{
    lastValidText.Format("%d",(int)scrollBar->GetCurrentValue());
    editBox->SetText(lastValidText);
    return true;
}

bool pawsNumberPromptWindow::TextIsValidForEditing(const csString &text)
{
    if(text.Length() > (unsigned int)maxDigits)
        return false;

    for(size_t i=0; i < text.Length(); i++)
        if(!isdigit(text.GetAt(i)))
            return false;

    if(
        (text.GetData() != NULL)
        &&
        (atoi(text.GetData()) > maxNumber)
    )
        return false;
    return true;
}

bool pawsNumberPromptWindow::TextIsValidForOutput(const csString &text)
{
    if(text.Length() == 0)
        return false;

    int number = atoi(text.GetData());
    return TextIsValidForEditing(text) && (number >= 0) && (number >= minNumber);
}

bool pawsNumberPromptWindow::OnChange(pawsWidget* widget)
{
    csString text;

    if(widget == editBox)
    {
        text = editBox->GetText();
        if(TextIsValidForEditing(text))
        {
            lastValidText = text;
            // This does automatic entering ( ie no chance to select ok )
            //if (TextIsValidForOutput(text) && (text.GetSize() == maxDigits))
            //    NumberWasEntered(atoi(text.GetData()));
        }
        else
            editBox->SetText(lastValidText);
    }
    return true;
}

void pawsNumberPromptWindow::NumberWasEntered(int number)
{
    if(action != NULL)
    {
        action->OnNumberEntered(name,param,number);
        action = NULL;
    }
    parent->DeleteChild(this);
}

void pawsNumberPromptWindow::Initialize(const csString &labelStr, int number, int minNumber, int maxNumber,
                                        iOnNumberEnteredAction* action,const char* name, int param)
{
    PawsManager::GetSingleton().SetCurrentFocusedWidget(editBox);

    SetLabel(labelStr);

    if(label != NULL)
        label->SetText(labelStr);

    SetBoundaries(minNumber, maxNumber);
    this->action = action;
    this->name   = name;
    this->param  = param;

    if(number != -1)
    {
        csString numberStr;
        numberStr.Format("%d",number);
        editBox->SetText(numberStr);
        scrollBar->SetCurrentValue(number);
    }
    else
    {
        scrollBar->SetCurrentValue(1);
        editBox->SetText("1");
    }
    PawsManager::GetSingleton().SetCurrentFocusedWidget(editBox);
}


pawsNumberPromptWindow* pawsNumberPromptWindow::Create(const csString &label,
        int number, int minNumber, int maxNumber,
        iOnNumberEnteredAction* action,const char* name, int param)
{
    pawsNumberPromptWindow* w = new pawsNumberPromptWindow();
    PawsManager::GetSingleton().GetMainWidget()->AddChild(w);
    w->PostSetup();
    w->UseBorder();
    w->SetTitle("Amount", "Scaling Title Bar", "center", "no");
    w->SetBackground("Scaling Window Background");
    PawsManager::GetSingleton().SetModalWidget(w);
    w->Initialize(label, number, minNumber, maxNumber, action,name,param);
    return w;
}
