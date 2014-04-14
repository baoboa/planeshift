/**
 * @file pawscheckbox.cpp
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

#include "pawscheckbox.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"
#include "pawstextbox.h"

//---------------------------------------------------------------------------------
pawsCheckBox::pawsCheckBox()
    : textOffsetX(0), textOffsetY(0)
{
    checkBoxOff = "Checkbox Off";
    checkBoxOn  = "Checkbox On";
    checkBoxSize = 16;
    checkBox = 0;
    factory = "pawsCheckBox";
}

pawsCheckBox::pawsCheckBox(const pawsCheckBox &origin)
    : pawsWidget(origin),
      textOffsetX(origin.textOffsetX),
      textOffsetY(origin.textOffsetY),
      checkBoxOff(origin.checkBoxOff),
      checkBoxOn(origin.checkBoxOn),
      checkBoxGrey(origin.checkBoxGrey),
      checkBoxSize(origin.checkBoxSize)
{
    text = 0;
    checkBox = 0;
    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        if(origin.children[i] == origin.checkBox)
            checkBox = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.children[i] == origin.text)
            text = dynamic_cast<pawsTextBox*>(children[i]);

        if(text != 0 && checkBox != 0) break;
    }
}

pawsCheckBox::~pawsCheckBox()
{
}

void pawsCheckBox::SetState(bool state)
{
    checkBox->SetState(state);
}

void pawsCheckBox::SetText(const char* string)
{
    text->SetText(string);
}

bool pawsCheckBox::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> textNode = node->GetNode("text");

    if(!textNode)
    {
        Error2("%s XML is defined incorrectly. No <text> tag found", name.GetData());
        return false;
    }

    csString pos(textNode->GetAttributeValue("position"));
    textOffsetX = textNode->GetAttributeValueAsInt("offsetx");
    textOffsetY = textNode->GetAttributeValueAsInt("offsety");


    ///////////////////////////////////////////////////////////////////////
    // Create the check box
    ///////////////////////////////////////////////////////////////////////
    checkBox = new pawsButton;
    AddChild(checkBox);
    checkBox->SetNotify(this);

    csRect boxRect;
    csRect textRect;

    csRef<iDocumentNode> checkBoxNode = node->GetNode("checkbox");

    if(checkBoxNode)
    {
        csRef<iDocumentAttribute> attr;

        attr = checkBoxNode->GetAttribute("off");
        if(attr)
            checkBoxOff = attr->GetValue();

        attr = checkBoxNode->GetAttribute("on");
        if(attr)
            checkBoxOn  = attr->GetValue();

        attr = checkBoxNode->GetAttribute("greyoff");
        if(attr)
            checkBox->SetGreyUpImage(attr->GetValue());

        attr = checkBoxNode->GetAttribute("greyon");
        if(attr)
            checkBox->SetGreyDownImage(attr->GetValue());

        attr = checkBoxNode->GetAttribute("size");
        if(attr)
            checkBoxSize     = attr->GetValueAsInt();
    }

    if(pos == "left")
    {
        boxRect = csRect(defaultFrame.Width() - GetActualWidth(checkBoxSize), 4,
                         defaultFrame.Width(), GetActualHeight(checkBoxSize) + 4);

        textRect   = csRect(0 + textOffsetX, 4 + textOffsetY, defaultFrame.Width() - GetActualWidth(checkBoxSize), defaultFrame.Height());
    }
    else
    {
        boxRect = csRect(4, 4,
                         GetActualWidth(checkBoxSize) + 4, GetActualHeight(checkBoxSize) + 4);

        textRect   = csRect(4 + GetActualWidth(checkBoxSize) + 2 + textOffsetX, 4 + textOffsetY, defaultFrame.Width(), defaultFrame.Height());
    }

    checkBox->SetRelativeFrame(boxRect.xmin, boxRect.ymin,
                               checkBoxSize, checkBoxSize);
    checkBox->SetUpImage(checkBoxOff);
    checkBox->SetDownImage(checkBoxOn);
    checkBox->SetState(false);
    checkBox->SetToggle(true);
    checkBox->PostSetup();
    checkBox->SetID(id);

    // copy publish list over to checkbox
    checkBox->publishList = publishList;


    ///////////////////////////////////////////////////////////////////////
    // Create the textbox that has the current selected choice
    ///////////////////////////////////////////////////////////////////////
    csString str(PawsManager::GetSingleton().Translate(textNode->GetAttributeValue("string")));

    text = new pawsTextBox;
    AddChild(text);

    // Puts the box at the edge of the text box widget
    text->SetRelativeFrame(textRect.xmin, textRect.ymin,
                           textRect.Width(), textRect.Height());
    text->PostSetup();

    text->SetText(str);
    text->SetID(id);

    return true;
}

bool pawsCheckBox::SelfPopulate(iDocumentNode* node)
{
    if(node->GetAttributeValue("text"))
    {
        checkBox->SetText(node->GetAttributeValue("text"));
    }

    if(node->GetAttributeValue("down"))
    {
        checkBox->SetState(strcmp(node->GetAttributeValue("down"),"true")==0);
    }

    return true;
}

bool pawsCheckBox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* /*widget*/)
{
    if(parent)
        return parent->OnButtonPressed(mouseButton, keyModifier, this);
    else
        return false;
}

bool pawsCheckBox::GetState()
{
    if(checkBox != NULL)
        return checkBox->GetState();
    else
        return false;
}

const char* pawsCheckBox::GetText()
{
    if(text != NULL)
        return text->GetText();
    else
        return NULL;
}

void pawsCheckBox::SetImages(const char* up, const char* down)
{
    checkBox->SetUpImage(up);
    checkBox->SetDownImage(down);
}

void pawsCheckBox::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    // This is called automatically whenever subscribed data is published.
    if(checkBox)
    {
        checkBox->SetState(value.GetBool(), false);
    }
}

double pawsCheckBox::GetProperty(MathEnvironment* env, const char* ptr)
{
    if(!strcasecmp(ptr, "checked"))
        return checkBox->GetState() ? 1.0 : 0.0;
    else if(!strcasecmp(ptr, "greyed"))
        return checkBox->IsEnabled() ? 1.0 : 0.0;
    return pawsWidget::GetProperty(env, ptr);
}

void pawsCheckBox::SetProperty(const char* ptr, double value)
{
    if(!strcasecmp(ptr, "checked"))
        SetState(fabs(value) > 0.5);
    else if(!strcasecmp(ptr, "greyed"))
        checkBox->SetEnabled(fabs(value) <= 0.5);
    else
        pawsWidget::SetProperty(ptr, value);
}


