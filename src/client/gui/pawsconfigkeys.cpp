/*
 * pawsclientconfig.cpp - Author: Ondrej Hurt
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
#include <csutil/inputdef.h>
#include <csutil/event.h>

#include "globals.h"
#include "gui/shortcutwindow.h"

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawsconfigkeys.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"


// layout of ConfigKeys window:
#define COMMAND_WIDTH 150
#define TRIGGER_WIDTH 150
#define COLUMN_SPACING 30


csRef<iDocumentNode> FindFirstWidget(iDocument * doc)
{
    csRef<iDocumentNode> root, topNode, widgetNode;
    
    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("No root in XML");
        return NULL;
    }
    topNode = root->GetNode("widget_description");
    if (topNode == NULL)
    {
        Error1("No <widget_description> in XML");
        return NULL;
    }
    widgetNode = topNode->GetNode("widget");
    if (widgetNode == NULL)
    {
        Error1("No <widget> in <widget_description>");
        return NULL;
    }
    return widgetNode;
}

//////////////////////////////////////////////////////////////////////
//
//                        pawsConfigKeys
//
//////////////////////////////////////////////////////////////////////

pawsConfigKeys::pawsConfigKeys()
{
    tree = NULL;
    fingWnd = NULL;
}

bool pawsConfigKeys::Initialize()
{
    SetName("ConfigKeys");
    
    if ( !CreateTree() )
        return false;

   // UseBorder("line");

    if ( !FindFingeringWindow() )
        return false;

    return true;
}

bool pawsConfigKeys::CreateTree()
{
    pawsTreeNode * root;

    /****************
    CS_ASSERT(tree == NULL);
    tree = new pawsTree();
    tree->SetRelativeFrame(0, 0,parent->GetDefaultFrame().Width(),parent->GetDefaultFrame().Height());
    tree->SetScrollBars(false, true);
    tree->SetTreeLayout(new pawsStdTreeLayout(tree, 5, 20));
    tree->SetTreeDecorator(new pawsStdTreeDecorator(tree, graphics2D, 0x0000ff, 0x00ffff, 13));
    AddChild(tree);

    if ( !tree->LoadFromFile("configkeys.xml") )
        return false;
    *********************/

    if (!LoadFromFile("configkeys.xml") )
		return false;

    tree = dynamic_cast<pawsTree *>(children[0]);
    tree->SetRelativeFrameSize(parent->GetScreenFrame().Width()-20, parent->GetScreenFrame().Height()-20);

    root = tree->GetRoot();
    if (root != NULL)
        CreateTreeWidgets(root);

    tree->SetScrollBars(false,true);
    return true;
}

void pawsConfigKeys::SetKeyLabels(pawsTreeNode * subtreeRoot)
{    
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;
    pawsTextBox * label;
    pawsTextBox * keyLabel;

    if (subtreeRoot != tree->GetRoot())
    {
        rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
        if (rootAsSeq != NULL)
        {
            label = dynamic_cast<pawsTextBox*> (rootAsSeq->GetSeqWidget(0));
            keyLabel = dynamic_cast<pawsTextBox*> (rootAsSeq->GetSeqWidget(1));
            CS_ASSERT(label && keyLabel);
        
            psCharController* manager = psengine->GetCharControl();
        
            // Get the bind ID for this action.
            const psControl* ctrl = manager->GetTrigger( subtreeRoot->GetName() );
            if (ctrl)
            {
                // Button combo
                keyLabel->SetText( ctrl->ToString() );
                
                // Action name
                label->SetText( GetDisplayName(ctrl->name) );
            }
            else
            {
                Error2("Unimplemented action '%s'!",subtreeRoot->GetName());
                keyLabel->SetText("???");
                label->SetText(subtreeRoot->GetName());
            }
        }
    }

    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        SetKeyLabels(child);
        child = child->GetNextSibling();
    }
}

void pawsConfigKeys::CreateTreeWidgets(pawsTreeNode * subtreeRoot)
{
    pawsTextBox * label, * key;
    pawsButton * button;
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;


    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL   &&   rootAsSeq != tree->GetRoot())
    {
        label = new pawsTextBox();
        label->SetRelativeFrameSize(GetActualWidth(COMMAND_WIDTH), 20);
        label->SetColour(0xffffff);
        label->Show();
        label->SetParent( this );
        label->SetRelativeFrame( 0, 0, GetActualWidth(COMMAND_WIDTH), 20 );
        label->VertAdjust(pawsTextBox::vertCENTRE);
        rootAsSeq->AddSeqWidget(label, GetActualWidth(COMMAND_WIDTH) + GetActualWidth(COLUMN_SPACING));

        key = new pawsTextBox();        
        key->SetRelativeFrameSize(GetActualWidth(TRIGGER_WIDTH), 20);
        key->SetColour(0xffffff);
        key->Show();
        key->SetParent( this );
        key->SetRelativeFrame( GetActualWidth(COMMAND_WIDTH)+5, 0, GetActualWidth(TRIGGER_WIDTH), 20 );
        key->VertAdjust(pawsTextBox::vertCENTRE);
        rootAsSeq->AddSeqWidget(key, GetActualWidth(TRIGGER_WIDTH) + GetActualWidth(COLUMN_SPACING));

        button = new pawsButton();
        button->SetParent( this );
        button->SetNotify(this);
        button->SetRelativeFrameSize(30, 20);
        button->SetUpImage("Standard Button");
        button->SetDownImage("Standard Button Down");
        button->SetText("Set");
        button->SetToggle(false);
        button->Show();
        button->SetRelativeFrame( GetActualWidth(COMMAND_WIDTH)+GetActualWidth(TRIGGER_WIDTH)+5, 0, 30, 20 );
        rootAsSeq->AddSeqWidget(button);
    }
    
    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        CreateTreeWidgets(child);
        child = child->GetNextSibling();
    }
}

void pawsConfigKeys::Draw()
{
    pawsWidget::Draw();
}

void pawsConfigKeys::UpdateNicks(pawsTreeNode * subtreeRoot)
{
    if (subtreeRoot == NULL)
        subtreeRoot = tree->GetRoot();

    if (subtreeRoot == NULL) return;
    
    pawsSeqTreeNode * rootAsSeq;
    pawsTreeNode * child;
    pawsTextBox * label;

    rootAsSeq = dynamic_cast<pawsSeqTreeNode*> (subtreeRoot);
    if (rootAsSeq != NULL   &&   rootAsSeq != tree->GetRoot())
    {
        label = dynamic_cast<pawsTextBox*> (rootAsSeq->GetSeqWidget(0));
        CS_ASSERT(label);
        
        psCharController* manager = psengine->GetCharControl();
        
        // Update the name label
        const psControl* ctrl = manager->GetTrigger( subtreeRoot->GetName() );
        if (ctrl)
            label->SetText( GetDisplayName(ctrl->name) );
    }


    child = subtreeRoot->GetFirstChild();
    while (child != NULL)
    {
        UpdateNicks(child);
        child = child->GetNextSibling();
    }
}


bool pawsConfigKeys::FindFingeringWindow()
{
    pawsWidget* fingWndAsWidget = PawsManager::GetSingleton().FindWidget("FingeringWindow");
    if (fingWndAsWidget == NULL)
    {
        Error1("Could not find widget FingeringWindow");
        return false;
    }
    fingWnd = dynamic_cast<pawsFingeringWindow *>(fingWndAsWidget);
    if (fingWnd == NULL)
    {
        Error1("FingeringWindow is not pawsFingeringWindow");
        return false;
    }
    fingWnd->SetNotify(this);

    return true;
}

bool pawsConfigKeys::LoadConfig()
{
    SetKeyLabels(tree->GetRoot());
    dirty = false;
    return true;       
}

void pawsConfigKeys::SetDefault()
{
    psengine->GetCharControl()->LoadDefaultKeys();

    pawsShortcutWindow* shortcuts = dynamic_cast<pawsShortcutWindow*>
                                       (PawsManager::GetSingleton().FindWidget("ShortcutMenu"));
    if (shortcuts)
        shortcuts->LoadDefaultCommands();

    SetKeyLabels(tree->GetRoot());
    dirty = true;
}

csString GetCommandOfButton(pawsWidget * widget)
{
    pawsTreeNode* buttonNode = dynamic_cast <pawsTreeNode*> (widget->GetParent());
    CS_ASSERT(buttonNode);
    return buttonNode->GetName();
}

void pawsConfigKeys::SetTriggerTextOfCommand(const csString & command, const csString & trigger)
{
    pawsSeqTreeNode* buttonNode = dynamic_cast <pawsSeqTreeNode*> (tree->FindNodeByName(command));
    CS_ASSERT(buttonNode);
    pawsTextBox* textBox = dynamic_cast <pawsTextBox*> (buttonNode->GetSeqWidget(1));
    CS_ASSERT(textBox);
    textBox->SetText(trigger);
}

bool pawsConfigKeys::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    editedCmd = GetCommandOfButton(widget);
    if (editedCmd.IsEmpty())
        return false;

    fingWnd->ShowDialog(this, GetDisplayName(editedCmd));
    return true;
}

bool pawsConfigKeys::OnFingering(csString string, psControl::Device device, uint button, uint32 mods)
{
    if (fingWnd == NULL || !fingWnd->IsVisible())
        return true;

    bool changed = false;

    if (string == NO_BIND)  // Removing trigger
    {
        psengine->GetCharControl()->RemapTrigger(editedCmd,psControl::NONE,0,0);
        changed = true;
    }
    else  // Changing trigger
    {
        changed = psengine->GetCharControl()->RemapTrigger(editedCmd,device,button,mods);
    }

    if (changed)
    {
        SetTriggerTextOfCommand(editedCmd,string);
        dirty = true;
        return true;  // Hide fingering window
    }
    else  // Map already exists
    {
        const psControl* other = psengine->GetCharControl()->GetMappedTrigger(device,button,mods);
        CS_ASSERT(other);
        if (other->name != editedCmd)
        {
            fingWnd->SetCollisionInfo(GetDisplayName(other->name));
            return false;  // Keep fingering window up; invalid combo to set
        }
        else return true;  // Hide fingering window; combo is the same as old
    }
}



//////////////////////////////////////////////////////////////////////
//
//                        pawsFingeringWindow
//
//////////////////////////////////////////////////////////////////////

pawsFingeringWindow::pawsFingeringWindow()
{
    labelTextBox = NULL;
    buttonTextBox = NULL;
    notify = NULL;
    button = 0;
    mods = 0;
    device = psControl::NONE;
}

void pawsFingeringWindow::ShowDialog(pawsFingeringReceiver * receiver, const char * editedCmd)
{
    SetNotify(receiver);
    SetupGUIForDetection(editedCmd);
    Show();
    PawsManager::GetSingleton().SetModalWidget(this);
}

bool pawsFingeringWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if (!strcmp(widget->GetName(),"CancelButton"))
    {
        Hide();
        return true;
    }
    else if (!strcmp(widget->GetName(),"NoneButton"))
    {
        if ( notify->OnFingering(NO_BIND,psControl::NONE,0,0) )
            Hide();
        return true;
    }
    else if (!strcmp(widget->GetName(),"SetButton"))
    {
        if ( notify->OnFingering(combo,device,button,mods) )
            Hide();
        return true;
    }
    else
        return false;
}

void pawsFingeringWindow::Hide()
{
    PawsManager::GetSingleton().SetModalWidget(NULL);
    PawsManager::GetSingleton().SetCurrentFocusedWidget(dynamic_cast<pawsWidget*>(notify));
    pawsWidget::Hide();
}

void pawsFingeringWindow::SetNotify( pawsFingeringReceiver* receiver )
{
    notify = receiver;
}
    
bool pawsFingeringWindow::PostSetup()
{
    labelTextBox = (pawsTextBox*)FindWidget("LabelTextBox");
    if (!labelTextBox)
        return false;

    buttonTextBox = (pawsTextBox*)FindWidget("ButtonTextBox");
    if (!buttonTextBox)
        return false;

    return true;
}

void pawsFingeringWindow::SetupGUIForDetection(const csString & command)
{
    labelTextBox->SetText("Press key combination for \"" + command + "\"");
    buttonTextBox->SetText("");
}

void pawsFingeringWindow::SetCollisionInfo(const char* action)
{
    csString str = "\"";
    str += combo;
    str += "\" is already set to \"";
    str += action;
    str += "\"";
    buttonTextBox->SetText(str);
}

bool pawsFingeringWindow::OnKeyDown(utf32_char keyCode, utf32_char /*keyChar*/, int modifiers)
{
    /* Ignore autorepeats.
     * You can get odd results like ctrl+Lctrl if you accept them here,
     * and they make it harder to capture mouse events in OnMouseDown().
     */
    if ( csKeyEventHelper::GetAutoRepeat(psengine->GetLastEvent()) )
        return true;

    device = psControl::KEYBOARD;
    button = keyCode;
    mods = modifiers & PS_MODS_MASK;
    RefreshCombo();
    return true;
}

bool pawsFingeringWindow::OnMouseDown(int mousebtn, int modifiers, int /*x*/, int /*y*/)
{
    device = psControl::MOUSE;
    button = mousebtn;
    mods = modifiers & PS_MODS_MASK;
    RefreshCombo();
    return true;
}

void pawsFingeringWindow::RefreshCombo()
{
    combo = ComboToString(device,button,mods);
    buttonTextBox->SetText(combo);
}
