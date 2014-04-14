/*
 * pawsconfigmouse.cpp - Author: AndrewRobberts
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
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>

// Client INCLUDES
#include "globals.h"
#include "pscharcontrol.h"

// COMMON INCLUDES
#include "../../common/util/log.h"

// PAWS INCLUDES
#include "pawsconfigmouse.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "paws/pawsspinbox.h"

// layout of ConfigKeys window:
#define COMMAND_WIDTH 150
#define TRIGGER_WIDTH 150
#define COLUMN_SPACING 30

#define NO_BIND ""

#define MOUSE_FILE         "/planeshift/userdata/options/mouse.xml"
#define DEFAULT_MOUSE_FILE "/planeshift/data/options/mouse_def.xml"

//////////////////////////////////////////////////////////////////////
//
//                        pawsConfigMouse
//
//////////////////////////////////////////////////////////////////////

pawsConfigMouse::~pawsConfigMouse()
{
}


bool pawsConfigMouse::Initialize()
{
    if ( ! CreateTree())
        return false;

    UseBorder("line");
    return true;
}


bool pawsConfigMouse::LoadConfig()
{
    csString fileName = MOUSE_FILE;
    if (!psengine->GetVFS()->Exists(fileName))
    {
        fileName = DEFAULT_MOUSE_FILE;
    }
    return LoadMouse(fileName);
}

bool pawsConfigMouse::LoadMouse(const char * fileName)
{
    if ( binds.LoadFromFile(psengine->GetObjectRegistry(), fileName ) )
    {
        dirty = false;
        SetActionLabels(tree->GetRoot());
        return true;
    }
    else
        return false;
}

bool pawsConfigMouse::SaveConfig()
{
    // Transfer the settings
    /*pawsSeqTreeNode* invertN = (pawsSeqTreeNode*)FindWidget("InvertMouse");
    pawsSeqTreeNode* vertN =   (pawsSeqTreeNode*)FindWidget("VertSensitivity");
    pawsSeqTreeNode* hertN =   (pawsSeqTreeNode*)FindWidget("HorzSensitivity");

    pawsSpinBox* vert = (pawsSpinBox*)vertN->GetSeqWidget(1);
    pawsSpinBox* hert = (pawsSpinBox*)hertN->GetSeqWidget(1);

    pawsButton* invert = (pawsButton*)invertN->GetSeqWidget(1);

    binds.SetOnOff("InvertMouse", invert->GetState() );
    binds.SetInt("VertSensitivity", vert->GetValue() );
    binds.SetInt("HorzSensitivity", hert->GetValue() );    */

    bool ok1 = binds.SaveToFile(psengine->GetObjectRegistry(), MOUSE_FILE);
    dirty = false;

    bool ok2 = psengine->GetMouseBinds()->LoadFromFile(psengine->GetObjectRegistry(), MOUSE_FILE);

    psengine->GetCharControl()->GetMovementManager()->LoadMouseSettings();

    return ok1 && ok2;
}

bool pawsConfigMouse::OnChange(pawsWidget* widget)
{
    dirty=true;
    pawsSpinBox* box = dynamic_cast<pawsSpinBox*>(widget);
    if(!box)
        return true;

    binds.SetInt(box->GetParent()->GetName(),(int)box->GetValue());
    return true;
}

void pawsConfigMouse::SetDefault()
{
    LoadMouse(DEFAULT_MOUSE_FILE);
    dirty = true;
}

bool pawsConfigMouse::CreateTree()
{
    pawsTreeNode * root;

//    tree = new pawsTree();
//    tree->MoveTo(screenFrame.xmin, screenFrame.ymin);
//    tree->SetSize(screenFrame.Width(), screenFrame.Height());
//    tree->SetScrollBars(false, true);
//    tree->SetTreeLayout(new pawsStdTreeLayout(tree, 5, 20));
//    tree->SetTreeDecorator(new pawsStdTreeDecorator(tree, graphics2D, 0x0000ff, 0x00ffff, 13));
//    AddChild(tree);

    if (!LoadFromFile("configmouse.xml"))
        return false;

    tree = dynamic_cast<pawsTree *>(children[0]);
    tree->SetRelativeFrameSize(parent->GetScreenFrame().Width()-20, parent->GetScreenFrame().Height()-20);

    root = tree->GetRoot();
    if (root != NULL)
        CreateTreeWidgets(root);

    tree->SetScrollBars(false,true);

    return true;
}

void pawsConfigMouse::CreateTreeWidgets(pawsTreeNode * subtreeRoot)
{
    pawsTextBox * label, * key;
    pawsButton * button;
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;


    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL)
    {
        if (!strcmp(subtreeRoot->GetAttr("type"), "action"))
        {
            label = new pawsTextBox();
            label->SetSize(GetActualWidth(COMMAND_WIDTH), 20);
            label->SetText(subtreeRoot->GetName());
            label->SetColour(0xffffff);
            label->Show();
            label->SetParent( this );
            label->SetRelativeFrame( 0, 0, GetActualWidth(COMMAND_WIDTH), 20 );
            rootAsSeq->AddSeqWidget(label, GetActualWidth(COMMAND_WIDTH) + GetActualWidth(COLUMN_SPACING));

            key = new pawsTextBox();
            key->SetSize(GetActualWidth(TRIGGER_WIDTH), 20);
            key->SetColour(0xffffff);
            key->Show();
            key->SetParent( this );
            key->SetRelativeFrame( GetActualWidth(COMMAND_WIDTH)+5, 0, GetActualWidth(TRIGGER_WIDTH), 20 );        
            rootAsSeq->AddSeqWidget(key, GetActualWidth(TRIGGER_WIDTH) + GetActualWidth(COLUMN_SPACING));

            button = new pawsButton();
            button->SetNotify(this);
            button->SetParent( this );
            button->SetSize(30, 20);
            button->SetUpImage("Standard Button");
            button->SetDownImage("Standard Button Down");
            button->SetText(PawsManager::GetSingleton().Translate("Set"));
            button->SetToggle(false);
            button->Show();
            button->SetRelativeFrame( GetActualWidth(COMMAND_WIDTH)+GetActualWidth(TRIGGER_WIDTH)+5, 0, 30, 20 );
            rootAsSeq->AddSeqWidget(button);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "onoff"))
        {
            label = new pawsTextBox();
            label->SetSize(GetActualWidth(COMMAND_WIDTH), 20);
            label->SetText(subtreeRoot->GetName());
            label->SetColour(0xffffff);
            label->Show();
            label->SetRelativeFrame( 0, 0, GetActualWidth(COMMAND_WIDTH), 20 );
            rootAsSeq->AddSeqWidget(label, GetActualWidth(COMMAND_WIDTH) + GetActualWidth(COLUMN_SPACING));

            button = new pawsButton();
            button->SetNotify(this);
            button->SetUpImage("Checkbox Off");
            button->SetDownImage("Checkbox On");
            button->SetToggle(true);
            button->Show();
            button->SetRelativeFrame( GetActualWidth(COMMAND_WIDTH)+30, 0, 16, 16 );
            rootAsSeq->AddSeqWidget(button);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "natnum"))
        {
            label = new pawsTextBox();
            label->SetSize(GetActualWidth(COMMAND_WIDTH), 20);
            label->SetText(subtreeRoot->GetName());
            label->SetColour(0xffffff);
            label->Show();
            label->SetRelativeFrame( 0, 0, GetActualWidth(COMMAND_WIDTH), 20 );
            rootAsSeq->AddSeqWidget(label, GetActualWidth(COMMAND_WIDTH) + GetActualWidth(COLUMN_SPACING));

            pawsSpinBox* spin = new pawsSpinBox();
            spin->SetSize(GetActualWidth(TRIGGER_WIDTH), 20);
            spin->SetRelativeFrame( GetActualWidth(COMMAND_WIDTH)+5, 0, GetActualWidth(TRIGGER_WIDTH), 20 );
            csString val("0");
            csString pos("right");
            spin->ManualSetup(val, 0.0f, 100.0f, 1.0f, pos);
            spin->Show();
            rootAsSeq->AddSeqWidget(spin, GetActualWidth(TRIGGER_WIDTH) + GetActualWidth(COLUMN_SPACING));
        }
    }

    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        CreateTreeWidgets(child);
        child = child->GetNextSibling();
    }
}

void pawsConfigMouse::SetActionLabels(pawsTreeNode * subtreeRoot)
{
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;
    pawsTextBox * actionLabel;
    csString boundAction;


    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL)
    {
        if (!strcmp(subtreeRoot->GetAttr("type"), "action"))
        {
            actionLabel = dynamic_cast<pawsTextBox*> (rootAsSeq->GetSeqWidget(1));
            assert(actionLabel);
            if (binds.GetBind(subtreeRoot->GetName(), boundAction))
                actionLabel->SetText(boundAction);
            else
                actionLabel->SetText(NO_BIND);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "onoff"))
        {
            pawsButton* actionButton = dynamic_cast<pawsButton*> (rootAsSeq->GetSeqWidget(1));
            assert(actionButton);
            bool isOn;
            if (binds.GetOnOff(subtreeRoot->GetName(), isOn))
                actionButton->SetState(isOn);
            else
                actionButton->SetState(false);
        }
        else if (!strcmp(subtreeRoot->GetAttr("type"), "natnum"))
        {
            pawsSpinBox* spin = dynamic_cast<pawsSpinBox*> (rootAsSeq->GetSeqWidget(1));
            csString val;
            if (binds.GetInt(subtreeRoot->GetName(), boundAction))
                spin->SetValue(atof(boundAction));
            else
                spin->SetValue(0);
        }
    }

    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        SetActionLabels(child);
        child = child->GetNextSibling();
    }
}

csString GetActionOfButton(pawsWidget * widget)
{
    pawsTreeNode * buttonNode;

    buttonNode = dynamic_cast <pawsTreeNode*> (widget->GetParent());
    assert(buttonNode);
    return buttonNode->GetName();
}

bool pawsConfigMouse::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    csString editedAction = GetActionOfButton(widget);
    if (editedAction.IsEmpty())
        return false;

    pawsSeqTreeNode * buttonNode;
    pawsTextBox * textBox;

    buttonNode = dynamic_cast <pawsSeqTreeNode*> (tree->FindNodeByName(editedAction));
    assert(buttonNode);

    if (!strcmp(buttonNode->GetAttr("type"), "action"))
    {
        textBox = dynamic_cast <pawsTextBox*> (buttonNode->GetSeqWidget(1));
        assert(textBox);

        binds.Bind(editedAction, mouseButton, keyModifier);

        csString str;
        binds.GetBind(editedAction,str);
        textBox->SetText(str);
        
        dirty=true;
    }
    else if (!strcmp(buttonNode->GetAttr("type"), "onoff"))
    {
        pawsButton* button = dynamic_cast <pawsButton*> (buttonNode->GetSeqWidget(1));
        assert(button);
        
        bool value = button->GetState();
        dirty=true;
        binds.SetOnOff(editedAction, value);
        HandleEditedAction( editedAction );        
    }
    else if (!strcmp(buttonNode->GetAttr("type"), "natnum"))
    {
        pawsSpinBox* spin = dynamic_cast <pawsSpinBox*> (buttonNode->GetSeqWidget(1));
        assert(spin);

        binds.SetInt(editedAction, (int)spin->GetValue());
        dirty=true;
    }

    return true;
}

void pawsConfigMouse::HandleEditedAction( csString& /*editedAction*/ )
{
/*
    if ( editedAction == "InvertMouse" )
    {
        psMovementManager* mov = psengine->GetCharControl()->GetMovementManager();
        mov->SetInvertedMouse(!mov->GetInvertedMouse());
    }
*/
}
