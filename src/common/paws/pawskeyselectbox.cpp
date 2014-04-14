/*
 * pawstextbox.cpp - Author: Andrew Craig
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
// pawstextbox.cpp: implementation of the pawsTextBox class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>
#include <ctype.h>
#include <ivideo/fontserv.h>
#include <iutil/virtclk.h>
#include <iutil/evdefs.h>
#include <iutil/event.h>
#include <csutil/inputdef.h>

#include "pawskeyselectbox.h"
#include "pawsmanager.h"
#include "util/log.h"

pawsKeySelectBox::~pawsKeySelectBox()
{
}

pawsKeySelectBox::pawsKeySelectBox(const pawsKeySelectBox &origin)
    :pawsWidget(origin),
     text(origin.text),
     key(origin.key),
     modifiers(origin.modifiers),
     textX(origin.textX),
     textY(origin.textY)
{

}

bool pawsKeySelectBox::Setup(iDocumentNode* /*node*/)
{
    return true;
}

void pawsKeySelectBox::Draw()
{
    pawsWidget::Draw();
    DrawWidgetText((const char*)text, screenFrame.xmin + textX, screenFrame.ymin + textY);
}

bool pawsKeySelectBox::OnKeyDown(utf32_char keyCode, utf32_char /*keyChar*/, int modifiers)
{
    if(!CSKEY_IS_MODIFIER(keyCode))
        SetKey(keyCode, modifiers);
    return true;
}

void pawsKeySelectBox::SetKey(int _key, int _modifiers)
{
    modifiers = _modifiers;
    key = _key;

    text.Clear();
    if(modifiers != 0)
    {
        if(modifiers & CSMASK_CTRL)
            text = "Ctrl + ";
        if(modifiers & CSMASK_ALT)
            text += "Alt + ";
        if(modifiers & CSMASK_SHIFT)
            text += "Shift + ";
    }

    csKeyModifiers mods;
    memset(&mods, 0, sizeof(mods));
    text += csInputDefinition::GetKeyString(
                PawsManager::GetSingleton().GetEventNameRegistry(), key, &mods, true);
    CalcTextPos();
}

const char* pawsKeySelectBox::GetText() const
{
    return text;
}

void pawsKeySelectBox::SetText(const char* keyText)
{
    text = keyText;
    if(text.IsEmpty())
    {
        SetKey(0,0);
        return;
    }

    size_t spaceChar;
    while((spaceChar = text.FindFirst(' ')) > 0)
        text.DeleteAt(spaceChar);

    utf32_char keyCode;
    utf32_char cookedCode;
    csKeyModifiers mods;

    if(!csInputDefinition::ParseKey(PawsManager::GetSingleton().GetEventNameRegistry(),
                                    text, &keyCode, &cookedCode, &mods))
    {
        SetKey(0,0);
        return;
    }

    modifiers = 0;
    if(mods.modifiers[csKeyModifierTypeAlt] != 0)
        modifiers |= CSMASK_ALT;
    if(mods.modifiers[csKeyModifierTypeCtrl] != 0)
        modifiers |= CSMASK_CTRL;
    if(mods.modifiers[csKeyModifierTypeShift] != 0)
        modifiers |= CSMASK_SHIFT;

    SetKey(keyCode, modifiers);
}

int pawsKeySelectBox::GetBorderStyle()
{
    return BORDER_SUNKEN;
}

void pawsKeySelectBox::CalcTextPos()
{
    int textWidth, textHeight;
    GetFont()->GetDimensions(text, textWidth, textHeight);
    textWidth += 5;

    textX = (screenFrame.Width() - textWidth) / 2;
    textY = (screenFrame.Height() - textHeight) / 2;
}
