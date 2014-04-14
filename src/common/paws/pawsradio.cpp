/*
 * pawsradio.cpp - Author: Andrew Craig
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

#include "pawsradio.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"
#include "pawstextbox.h"

//---------------------------------------------------------------------------------
pawsRadioButton::pawsRadioButton()
{
    down=false;
    radioOff ="radiooff";
    radioOn  ="radioon";
    size = 16;
    factory = "pawsRadioButton";
}

pawsRadioButton::pawsRadioButton(const pawsRadioButton &origin)
    :pawsButton(origin),
     radioOff(origin.radioOff),
     radioOn(origin.radioOn),
     size(origin.size)
{
    radioButton = 0;
    text = 0;
    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        if(origin.radioButton == origin.children[i])
            radioButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.text == origin.children[i])
            text = dynamic_cast<pawsTextBox*>(children[i]);
        if(radioButton != 0 && text != 0)
            break;
    }
}
void pawsRadioButton::SetState(bool state)
{
    radioButton->SetState(state);
}

pawsRadioButton::~pawsRadioButton()
{
}
pawsRadioButton* pawsRadioButton::Create(const char* txt, psRadioPos pos , bool /*state*/)
{
    pawsRadioButtonGroup* rbg = dynamic_cast<pawsRadioButtonGroup*>(parent);

    // if this radio button is part of a radio group
    if(rbg!=NULL)
    {
        // get the way the off and up radio buttons should look from there
        radioOff = rbg->GetRadioOffImage();
        radioOn  = rbg->GetRadioOnImage();
        size     = rbg->GetRadioSize();
    }
    radioOff ="radiooff";
    radioOn  ="radioon";
    size = 16;

    ///////////////////////////////////////////////////////////////////////
    // Create the radio button
    ///////////////////////////////////////////////////////////////////////
    radioButton = new pawsButton;
    AddChild(radioButton);

    csRect buttonRect;
    csRect textRect;

    if(pos == POS_LEFT)
    {
        buttonRect = csRect(defaultFrame.Width()-size, 4,
                            defaultFrame.Width(), size+4);

        textRect   = csRect(0, 4, defaultFrame.Width() - size, defaultFrame.Height());
    }

    if(pos == POS_RIGHT)
    {
        buttonRect = csRect(4, 4,
                            size+4, size+4);

        textRect   = csRect(size+6, 4, defaultFrame.Width()-size, defaultFrame.Height());
    }

    if(pos == POS_UNDERNEATH)
    {
        buttonRect = csRect(defaultFrame.Width() / 2 - size /2, 4, defaultFrame.Width()/2 + size/2, 4+size);
        textRect = csRect(0,size+6, defaultFrame.Width() , defaultFrame.Height()-(size+6));
    }

    if(pos == POS_ABOVE)
    {
        buttonRect = csRect(defaultFrame.Width() / 2 - size /2, 22, defaultFrame.Width()/2 + size/2, size+22);
        textRect = csRect(0,4, defaultFrame.Width() , size+4);
    }

    radioButton->SetRelativeFrame(buttonRect.xmin, buttonRect.ymin,
                                  buttonRect.Width(), buttonRect.Height());
    radioButton->SetUpImage(radioOff);
    radioButton->SetDownImage(radioOn);
    radioButton->SetState(false);
    radioButton->SetToggle(true);
    radioButton->PostSetup();
    radioButton->SetID(id);


    ///////////////////////////////////////////////////////////////////////
    // Create the textbox that has the current selected choice
    ///////////////////////////////////////////////////////////////////////

    text = new pawsTextBox;
    AddChild(text);

    // Puts the button at the edge of the text box widget
    text->SetRelativeFrame(textRect.xmin, textRect.ymin,
                           textRect.Width(), textRect.Height());
    text->PostSetup();

    text->SetText(txt);
    text->SetID(id);

    return this;
}
bool pawsRadioButton::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> textNode = node->GetNode("text");

    if(!textNode)
    {
        Error2("%s XML is defined incorrectly. No <text /> tag found", name.GetData());
        return false;
    }

    csString pos(textNode->GetAttributeValue("position"));

    pawsRadioButtonGroup* rbg = dynamic_cast<pawsRadioButtonGroup*>(parent);

    // if this radio button is part of a radio group
    if(rbg!=NULL)
    {
        // get the way the off and up radio buttons should look from there
        radioOff = rbg->GetRadioOffImage();
        radioOn  = rbg->GetRadioOnImage();
        size     = rbg->GetRadioSize();
    }
    //override group settings
    csRef<iDocumentNode> radioNode = node->GetNode("radio");

    if(radioNode)
    {
        csRef<iDocumentAttribute> attr;

        attr = radioNode->GetAttribute("off");
        if(attr)
            radioOff = attr->GetValue();

        attr = radioNode->GetAttribute("on");
        if(attr)
            radioOn  = attr->GetValue();

        attr = radioNode->GetAttribute("size");
        if(attr)
            size     = attr->GetValueAsInt();
    }



    ///////////////////////////////////////////////////////////////////////
    // Create the radio button
    ///////////////////////////////////////////////////////////////////////
    radioButton = new pawsButton;
    AddChild(radioButton);

    csRect buttonRect;
    csRect textRect;

    if(pos == "left")
    {
        buttonRect = csRect(defaultFrame.Width()-size, 4,
                            defaultFrame.Width(), size+4);

        textRect   = csRect(0, 4, defaultFrame.Width() - size, defaultFrame.Height());
    }

    if(pos == "right")
    {
        buttonRect = csRect(4, 4,
                            size+4, size+4);

        textRect   = csRect(size+6, 4, defaultFrame.Width()-size, defaultFrame.Height());
    }

    if(pos == "underneath")
    {
        buttonRect = csRect(defaultFrame.Width() / 2 - size /2, 4, defaultFrame.Width()/2 + size/2, 4+size);
        textRect = csRect(0,size+6, defaultFrame.Width() , defaultFrame.Height()-(size+6));
    }

    if(pos == "above")
    {
        buttonRect = csRect(defaultFrame.Width() / 2 - size /2, 22, defaultFrame.Width()/2 + size/2, size+22);
        textRect = csRect(0,4, defaultFrame.Width() , size+4);
    }

    radioButton->SetRelativeFrame(buttonRect.xmin, buttonRect.ymin,
                                  buttonRect.Width(), buttonRect.Height());
    radioButton->SetUpImage(radioOff);
    radioButton->SetDownImage(radioOn);
    radioButton->SetState(false);
    radioButton->SetToggle(true);
    radioButton->PostSetup();
    radioButton->SetID(id);


    ///////////////////////////////////////////////////////////////////////
    // Create the textbox that has the current selected choice
    ///////////////////////////////////////////////////////////////////////
    csString str(textNode->GetAttributeValue("string"));

    text = new pawsTextBox;
    AddChild(text);

    // Puts the button at the edge of the text box widget
    text->SetRelativeFrame(textRect.xmin, textRect.ymin,
                           textRect.Width(), textRect.Height());
    text->PostSetup();

    text->SetText(str);
    text->SetID(id);

    return true;
}


bool pawsRadioButton::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* /*widget*/)
{
    if(parent)
        return parent->OnButtonPressed(mouseButton, keyModifier, this);

    return false;
}

bool pawsRadioButton::GetState()
{
    if(radioButton != NULL)
        return radioButton->GetState();
    else
        return false;
}


//--------------------------------------------------------------
//  START OF pawsRadioButtonGroup
//--------------------------------------------------------------

pawsRadioButtonGroup::pawsRadioButtonGroup()
{
    radioOff = "radiooff";
    radioOn  = "radioon";
    size     = 16;
    factory = "pawsRadioButtonGroup";
}


pawsRadioButtonGroup::~pawsRadioButtonGroup()
{
}

bool pawsRadioButtonGroup::SetActive(const char* widgetName)
{
    bool okFlag = false;
    for(size_t x=0; x < children.GetSize(); x++)
    {
        csString factory(children[x]->GetType());
        if(factory == "pawsRadioButton")
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            if(strcmp(children[x]->GetName(), widgetName))
            {
                radButton->SetState(false);
            }
            else
            {
                okFlag = true;
                radButton->SetState(true);
            }
        }
    }
    return okFlag;
}

void pawsRadioButtonGroup::TurnAllOff()
{
    for(size_t x=0; x<children.GetSize(); x++)
    {
        csString factory = csString(children[x]->GetType());
        if(factory == "pawsRadioButton")
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            radButton->SetState(false);
        }
    }
}

bool pawsRadioButtonGroup::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    for(size_t x=0; x<children.GetSize(); x++)
    {
        csString factory = csString(children[x]->GetType());

        if(factory == "pawsRadioButton")
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];

            if(radButton == widget)
                radButton->SetState(true);
            else
                radButton->SetState(false);
        }
    }

    if(parent)
        parent->OnChange(this);

    if(parent)
        return parent->OnButtonPressed(mouseButton, keyModifier, widget);

    return false;
}

csString pawsRadioButtonGroup::GetActive()
{
    for(size_t x=0; x<children.GetSize(); x++)
    {
        csString factory = csString(children[x]->GetType());
        if(factory == "pawsRadioButton")
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            if(radButton->GetState())
                return radButton->GetName();
        }
    }
    return "";
}

int pawsRadioButtonGroup::GetActiveID()
{
    for(size_t x=0; x<children.GetSize(); x++)
    {
        csString factory = csString(children[x]->GetType());
        if(factory == "pawsRadioButton")
        {
            pawsRadioButton* radButton = (pawsRadioButton*)children[x];
            if(radButton->GetState())
                return radButton->GetID();
        }
    }
    return -1;
}

bool pawsRadioButtonGroup::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> radioNode = node->GetNode("radio");
    csRef<iDocumentNode> widgetNode;                                        //for compatibility with obsolete RBG-widget concept
    csRef<iDocumentNodeIterator> radioNodes = node->GetNodes("radionode");
    csRef<iDocumentNodeIterator> widgetNodes = node->GetNodes("widget");      //for compatibility with obsolete RBG-widget concept

    if(radioNode)
    {
        csRef<iDocumentAttribute> attr;

        attr = radioNode->GetAttribute("off");
        if(attr)
            radioOff = attr->GetValue();

        attr = radioNode->GetAttribute("on");
        if(attr)
            radioOn  = attr->GetValue();

        attr = radioNode->GetAttribute("size");
        if(attr)
            size     = attr->GetValueAsInt();
    }

    pawsWidget* radioWidget;
    pawsWidget* widgetNodeWidget;
    while(radioNodes->HasNext())
    {
        radioNode = radioNodes->Next();
        radioWidget = PawsManager::GetSingleton().CreateWidget("pawsRadioButton");
        AddChild(radioWidget);
        if(!radioWidget->Load(radioNode))
        {
            delete radioNode;
            return false;
        }
    }
    csRef<iDocumentAttribute> factoryAttr;
    csString factory;
    while(widgetNodes->HasNext())
    {
        widgetNode = widgetNodes->Next();
        factoryAttr = widgetNode->GetAttribute("factory");
        if(factoryAttr)
            factory = factoryAttr->GetValue();
        if(factory != "pawsRadioButton")        //ignore other widgets
            continue;
        widgetNodeWidget = PawsManager::GetSingleton().CreateWidget("pawsRadioButton");
        if(!widgetNodeWidget || !widgetNodeWidget->Load(widgetNode))
        {
            delete widgetNode;
            return false;
        }
        AddChild(widgetNodeWidget);
    }

    return true;
}


