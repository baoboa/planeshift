/*
 * Author: Jorrit Tyberghein
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "eeditrequestcombo.h"
#include "eeditglobals.h"

bool EEditRequestCombo::OnButtonPressed( int mouseButton, int keyModifer, pawsWidget* widget )
{
    if (widget == okButton)
    {
	pawsListBoxRow * row = choicesList->GetSelectedRow();
	if (row)
	{
	    if (callback)
	    {
	        pawsTextBox* col = (pawsTextBox *)row->GetColumn(0);
		callback->Select(col->GetText());
		callback = 0;
	    }
            Hide();
	    return true;
	}
    }
    else if (widget == cancelButton)
    {
	callback = 0;
        Hide();
	return true;
    }
    return false;
}


bool EEditRequestCombo::PostSetup()
{
    choicesList = (pawsListBox *)FindWidget("choices");		CS_ASSERT(choicesList);
    okButton = (pawsButton *)FindWidget("ok_button");		CS_ASSERT(okButton);
    cancelButton = (pawsButton *)FindWidget("cancel_button");	CS_ASSERT(cancelButton);

    return true;
}

void EEditRequestCombo::ClearChoices ()
{
    choicesList->Clear();
}

void EEditRequestCombo::AddChoice (const char* choice)
{
    size_t i = choicesList->GetRowCount();
    choicesList->NewRow(i);
    pawsListBoxRow * row = choicesList->GetRow(i);
    if (!row) return;

    pawsTextBox* col = (pawsTextBox *)row->GetColumn(0);
    col->SetText(choice);
}

