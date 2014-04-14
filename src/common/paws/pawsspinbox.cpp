/*
 * pawscheckbox.cpp
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
#include <psconfig.h>

#include "pawsspinbox.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"



//---------------------------------------------------------------------------------

struct SpinBoxTimerEvent : public scfImplementation1<SpinBoxTimerEvent,iTimerEvent>
{
    pawsSpinBox* spinbox;

    SpinBoxTimerEvent(pawsSpinBox* s)
        : scfImplementationType(this)
    {
        SpinBoxTimerEvent::spinbox = s;
    }

    virtual ~SpinBoxTimerEvent() {}

    virtual bool Perform(iTimerEvent* ev)
    {
        if(spinbox)
            return spinbox->Perform(ev);
        return false;
    }
};

//---------------------------------------------------------------------------------

pawsSpinBox::pawsSpinBox()
{
    max = 100;
    min = 0;
    inc = 1;

    spinState   = SPIN_STOP;
    spinCounter = 0;
    factory = "pawsSpinBox";

    globalTimer = csEventTimer::GetStandardTimer(PawsManager::GetSingleton().GetObjectRegistry());
    timerEvent  = new SpinBoxTimerEvent(this);

}

pawsSpinBox::pawsSpinBox(const pawsSpinBox &origin)
    :pawsWidget(origin),
     globalTimer(origin.globalTimer),
     spinCounter(origin.spinCounter),
     spinState(origin.spinState),
     max(origin.max),
     min(origin.min),
     inc(origin.inc)
{
    downButton = 0;
    upButton = 0;
    text = 0;
    timerEvent = new SpinBoxTimerEvent(this);

    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        if(origin.downButton == origin.children[i])
            downButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.upButton == origin.children[i])
            upButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.text == origin.children[i])
            text = dynamic_cast<pawsEditTextBox*>(children[i]);

        if(downButton!=0 && upButton != 0 && text != 0) break;
    }
}

pawsSpinBox::~pawsSpinBox()
{
}

bool pawsSpinBox::Perform(iTimerEvent* /*ev*/)
{
    if(spinState)
    {
        Spin();
        spinCounter++;
        globalTimer->AddTimerEvent(timerEvent,SPIN_INTERVAL);
    }

    return false;
}

bool pawsSpinBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> textNode = node->GetNode("number");
    if(!textNode)
    {
        Error2("%s XML is defined incorrectly. No <number /> tag found", name.GetData());
        return false;
    }
    csString pos(textNode->GetAttributeValue("position"));
    csString val(textNode->GetAttributeValue("default"));
    csString Min(textNode->GetAttributeValue("min"));
    csString Max(textNode->GetAttributeValue("max"));
    csString Inc(textNode->GetAttributeValue("inc"));

    return ManualSetup(val, atof(Min.GetData()), atof(Max.GetData()), atof(Inc.GetData()), pos);
}


bool pawsSpinBox::ManualSetup(csString &value, float Min, float Max, float Inc, csString &pos)
{

    upButton   = new pawsButton;
    AddChild(upButton);
    downButton = new pawsButton;
    AddChild(downButton);

    csRect buttonsRect;
    csRect textRect;

    if(pos == "left")
    {
        buttonsRect = csRect(defaultFrame.Width()-16, 0,
                             defaultFrame.Width(), defaultFrame.Height());

        textRect   = csRect(0, 4, defaultFrame.Width() - 16, defaultFrame.Height());
    }

    if(pos == "right")
    {
        buttonsRect = csRect(4, 0,
                             20, defaultFrame.Height());

        textRect   = csRect(22, 4, defaultFrame.Width()-16, defaultFrame.Height());
    }

    upButton->SetRelativeFrame(buttonsRect.xmin, buttonsRect.ymin,
                               buttonsRect.Width(), buttonsRect.Height()/2);
    upButton->SetUpImage("SpinUp");
    upButton->SetToggle(false);
    upButton->PostSetup();

    downButton->SetRelativeFrame(buttonsRect.xmin, buttonsRect.ymin + buttonsRect.Height()/2,
                                 buttonsRect.Width(), buttonsRect.Height()/2);
    downButton->SetUpImage("SpinDown");
    downButton->SetToggle(false);
    downButton->PostSetup();


    // Create the textbox that has the value
    csString str(value);

    text = new pawsEditTextBox;
    AddChild(text);

    // Puts the box at the edge of the text box widget
    text->SetRelativeFrame(textRect.xmin, textRect.ymin,
                           textRect.Width(), textRect.Height());
    text->PostSetup();
    text->SetText(str);
    text->SetID(id);

    min = Min;
    max = Max;
    inc = Inc;

    return true;
}

void pawsSpinBox::SetRange(float Min, float Max, float Inc)
{
    min = Min;
    max = Max;
    inc = Inc;
}

bool pawsSpinBox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if(mouseButton == 4)
    {
        SetValue(GetValue() + inc);
        parent->OnChange(this);
        return true;
    }
    else if(mouseButton == 5)
    {
        SetValue(GetValue() - inc);
        parent->OnChange(this);
        return true;
    }

    if(widget == upButton)
    {
        spinState    = SPIN_UP;
        spinCounter = 0;
        globalTimer->AddTimerEvent(timerEvent,SPIN_START_DELAY);
        Spin();
    }
    else if(widget == downButton)
    {
        spinState    = SPIN_DOWN;
        spinCounter = 0;
        globalTimer->AddTimerEvent(timerEvent,SPIN_START_DELAY);
        Spin();
    }

    if(parent)
        return parent->OnButtonPressed(mouseButton, keyModifier, this);

    return false;
}


void pawsSpinBox::Spin()
{
    float speed;

    // make the increments larger after some time
    if(spinCounter>40)
        speed = 10.0f;
    else if(spinCounter>20)
        speed = 5.0f;
    else
        speed = 1.0f;

    float val = atof(text->GetText());

    // calculate new value
    if(spinState==SPIN_UP)
        val += inc * speed;
    else if(spinState==SPIN_DOWN)
        val -= inc * speed;

    // make sure the value is valid
    if(val < min)
        val = min;
    if(val > max)
        val = max;

    SetValue(val);
    parent->OnChange(this);
}


void pawsSpinBox::OnLostFocus()
{
    // check if the value in the editbox is valid and
    // change it if necessary
    float val = GetValue();
    if(val < min)
        val = min;
    if(val > max)
        val = max;
    SetValue(val);

    // Convert the LostFocus to an OnChange.
    //  Since OnLostFocus() gives no information about which child lost focus
    //   it can be difficult for a parent to handle properly
    if(parent)
    {
        parent->OnChange(this);
        parent->OnLostFocus();
    }
}


bool pawsSpinBox::OnButtonReleased(int button, int keyModifier, pawsWidget* widget)
{
    if(widget == upButton || widget == downButton)
    {
        spinState    = SPIN_STOP;
        spinCounter = 0;
        globalTimer->RemoveTimerEvent(timerEvent);
    }

    if(parent)
        return parent->OnButtonReleased(button, keyModifier,widget);

    return false;
}


void pawsSpinBox::SetValue(float value)
{
    csString newVal("");
    newVal += value;
    text->SetText(newVal);
}

float pawsSpinBox::GetValue()
{
    return atof(text->GetText());
}




