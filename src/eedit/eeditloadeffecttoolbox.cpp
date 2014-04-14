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
#include "eeditloadeffecttoolbox.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"
#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"

EEditLoadEffectToolbox::EEditLoadEffectToolbox() : scfImplementationType(this)
{
}

EEditLoadEffectToolbox::~EEditLoadEffectToolbox()
{
}

void EEditLoadEffectToolbox::Update(unsigned int elapsed)
{
}

size_t EEditLoadEffectToolbox::GetType() const
{
    return T_LOAD_EFFECT;
}

const char * EEditLoadEffectToolbox::GetName() const
{
    return "Load Effect";
}
    
int EEditLoadEffectToolbox::SortTextBox(pawsWidget * widgetA, pawsWidget * widgetB)
{
    pawsTextBox * textBoxA, * textBoxB;
    const char  * textA,    * textB;
    
    textBoxA = dynamic_cast <pawsTextBox*> (widgetA);
    textBoxB = dynamic_cast <pawsTextBox*> (widgetB);
    assert(textBoxA && textBoxB);
    textA = textBoxA->GetText();
    if (textA == NULL)
        textA = "";
    textB = textBoxB->GetText();
    if (textB == NULL)
        textB = "";
    
    return strcmp(textA, textB);
}

void EEditLoadEffectToolbox::FillList(psEffectManager * effectManager)
{
    if (!effectList)
        return;

    effectList->Clear();
    
    size_t a=0;
    csHash<psEffect *, csString>::GlobalIterator it = effectManager->GetEffectsIterator();
    while (it.HasNext())
    {
        psEffect * effect = it.Next();
        
        effectList->NewRow(a);
        pawsListBoxRow * row = effectList->GetRow(a);
        if (!row)
            return;

        pawsTextBox * col = (pawsTextBox *)row->GetColumn(0);
        if (!col)
            return;
        
        csString effectName = effect->GetName();
        col->SetText(effectName);
        if (editApp->GetCurrEffectName() == effectName)
            effectList->Select(row);

        ++a;
    }
    effectList->SetSortingFunc(0, SortTextBox);
    effectList->SetSortedColumn(0);
    effectList->SortRows();
}

void EEditLoadEffectToolbox::SelectEffect(const csString & effectName)
{
    for (size_t a=0; a<effectList->GetRowCount(); ++a)
    {
        if (effectList->GetTextCellValue(a, 0) == effectName)
            effectList->GetTextCell(a, 0)->SetColour(0x00ffff);
        else
            effectList->GetTextCell(a, 0)->SetColour(0xffffff);
    }
}

bool EEditLoadEffectToolbox::PostSetup()
{
    effectList = (pawsListBox *)FindWidget("effect_list");              CS_ASSERT(effectList);
    openEffectButton = (pawsButton *)FindWidget("load_effect_button");  CS_ASSERT(openEffectButton);
    refreshButton = (pawsButton *)FindWidget("refresh_button");         CS_ASSERT(refreshButton);

    return true;
}

bool EEditLoadEffectToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    if (widget == openEffectButton)
    {
        editApp->SetCurrEffect(effectList->GetTextCellValue(effectList->GetSelectedRowNum(), 0));
        return true;
    }
    else if (widget == refreshButton)
    {
        editApp->RefreshEffectList();
        return true;
    }
    return false;
}

void EEditLoadEffectToolbox::OnListAction(pawsListBox* selected, int status)
{
    if (status == LISTBOX_SELECTED && selected == effectList)
        editApp->SetCurrEffect(effectList->GetTextCellValue(effectList->GetSelectedRowNum(), 0));
}
