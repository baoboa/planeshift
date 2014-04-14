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
#include "eeditselectlist.h"
#include "eeditglobals.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawsspinbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawstextbox.h"

EEditSelectList::EEditSelectList() : scfImplementationType(this)
{
    selectCallback = 0;
}

EEditSelectList::~EEditSelectList()
{
    delete selectCallback;
}

void EEditSelectList::Select(csString * list, size_t listCount, const csString & defaultValue, EEditInputboxManager::iSelectList * callback, const csVector2 & pos)
{
    // fill the list
    for (size_t a=0; a<listCount; ++a)
    {
        value->NewRow(a);
        pawsListBoxRow * row = value->GetRow(a);
        if (!row)
        {
            printf("Warning: Couldn't create row for: %s\n", list[a].GetData());
            continue;
        }

        pawsTextBox * col = (pawsTextBox *)row->GetColumn(0);
        if (!col)
        {
            printf("Warning: Couldn't get column for row: %s\n", list[a].GetData());
            continue;
        }

        col->SetText(list[a]);
        if (list[a] == defaultValue)
            value->Select(row);
    }
    
    MoveTo((int)pos.x, (int)pos.y);
    delete selectCallback;
    selectCallback = callback;
    Show();
    SetAlwaysOnTop(true);
}

bool EEditSelectList::PostSetup()
{
    value = (pawsListBox *) FindWidget("select_value");
    ok    = (pawsButton *)  FindWidget("ok");
    
    return true;
}

bool EEditSelectList::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget == ok)
    {
        int rowNum = value->GetSelectedRowNum();
        if (selectCallback) selectCallback->Select(value->GetTextCellValue(rowNum, 0), rowNum);
        delete selectCallback;
        selectCallback = 0;
        Hide();
        return true;
    }
    
    return false;
}

void EEditSelectList::OnListAction(pawsListBox * selected, int status)
{
    if (status == LISTBOX_SELECTED && selected == value)
    {
        int rowNum = value->GetSelectedRowNum();
        if (selectCallback) selectCallback->Select(value->GetTextCellValue(rowNum, 0), rowNum);
        delete selectCallback;
        selectCallback = 0;
        Hide();
    }
}

