/*
 * Author: Andrew Robberts
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

#include "eeditshortcutstoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawslistbox.h"
#include "paws/pawskeyselectbox.h"
#include "paws/pawsbutton.h"

EEditShortcutsToolbox::EEditShortcutsToolbox() : scfImplementationType(this)
{
}

EEditShortcutsToolbox::~EEditShortcutsToolbox()
{
}

void EEditShortcutsToolbox::AddShortcut(const char * name)
{
    size_t a = shortcutsList->GetRowCount();

    shortcutsList->NewRow(a);
    pawsListBoxRow * row = shortcutsList->GetRow(a);
    if (!row)
        return;

    pawsTextBox * col = (pawsTextBox *)row->GetColumn(0);
    if (!col)
        return;

    pawsKeySelectBox * key = (pawsKeySelectBox *)row->GetColumn(1);
    if (!key)
        return;

    col->SetText(name);
    key->SetText(editApp->GetConfigString(csString("EEdit.Shortcut.") + name, ""));

    if (key->GetKey() != 0)
        shortcuts.Push(EEditShortcutKey(name, key->GetKey(), key->GetModifiers()));
}

void EEditShortcutsToolbox::ExecuteShortcutCommand(int key, int modifiers) const
{
    for (size_t a=0; a<shortcuts.GetSize(); ++a)
    {
        if (shortcuts[a].key == key && shortcuts[a].modifiers == modifiers)
            editApp->ExecuteCommand(shortcuts[a].command);
    }
}

void EEditShortcutsToolbox::Update(unsigned int elapsed)
{
}

size_t EEditShortcutsToolbox::GetType() const
{
    return T_SHORTCUTS;
}

const char * EEditShortcutsToolbox::GetName() const
{
    return "Shortcuts";
}
    
bool EEditShortcutsToolbox::PostSetup()
{
    shortcutsList = (pawsListBox *)FindWidget("shortcuts_list");    CS_ASSERT(shortcutsList);
    applyButton   = (pawsButton *) FindWidget("apply");             CS_ASSERT(applyButton);
    cancelButton  = (pawsButton *) FindWidget("cancel");            CS_ASSERT(cancelButton);
    okButton      = (pawsButton *) FindWidget("ok");                CS_ASSERT(okButton);

    return true;
}

bool EEditShortcutsToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)
{
    if (widget == applyButton)
    {
        SaveShortcuts();
        return true;
    }
    else if (widget == cancelButton)
    {
        RedrawShortcuts();
        Hide();
        return true;
    }
    else if (widget == okButton)
    {
        SaveShortcuts();
        Hide();
        return true;
    }

    return false;
}

void EEditShortcutsToolbox::Show()
{
    SetRelativeFramePos(editApp->GetGraphics2D()->GetWidth()/2 - GetActualWidth(screenFrame.Width())/2,
                        editApp->GetGraphics2D()->GetHeight()/2 - GetActualHeight(screenFrame.Height())/2);
    pawsWidget::Show();
}

void EEditShortcutsToolbox::OnListAction(pawsListBox * selected, int status)
{
    pawsListBoxRow * row = shortcutsList->GetSelectedRow();
    if (!row)
        return;

    pawsWidget * keySelect = row->GetColumn(1);
    if (!keySelect)
        return;

    PawsManager::GetSingleton().SetCurrentFocusedWidget(keySelect);
}

void EEditShortcutsToolbox::SaveShortcuts()
{
    shortcuts.DeleteAll();
    
    for (size_t a=0; a<(size_t)shortcutsList->GetRowCount(); ++a)
    {
        pawsListBoxRow * row = shortcutsList->GetRow(a);
        if (!row)
            return;

        pawsTextBox * col = (pawsTextBox *)row->GetColumn(0);
        if (!col)
            return;

        pawsKeySelectBox * key = (pawsKeySelectBox *)row->GetColumn(1);
        if (!key)
            return;

        shortcuts.Push(EEditShortcutKey(col->GetText(), key->GetKey(), key->GetModifiers()));
        editApp->SetConfigString(csString("EEdit.Shortcut.") + col->GetText(), key->GetText());
    }
}

void EEditShortcutsToolbox::RedrawShortcuts()
{
    shortcutsList->Clear();

    for (size_t a=0; a<shortcuts.GetSize(); ++a)
    {
        shortcutsList->NewRow(a);
        pawsListBoxRow * row = shortcutsList->GetRow(a);
        if (!row)
            return;

        pawsTextBox * col = (pawsTextBox *)row->GetColumn(0);
        if (!col)
            return;

        pawsKeySelectBox * key = (pawsKeySelectBox *)row->GetColumn(1);
        if (!key)
            return;

        col->SetText(shortcuts[a].command);
        key->SetKey(shortcuts[a].key, shortcuts[a].modifiers);
    }
}
