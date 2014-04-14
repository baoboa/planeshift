/*
 * Author: Ralph Campbell
 *
 * Copyright (C) 2014 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "paws/pawsmanager.h"
#include "paws/pawslistbox.h"
#include "pawsmodswindow.h"
#include "pawsgmspawn.h"
#include "globals.h"

#define TELL    1000
#define REMOVE  2000
#define ADD     3000
#define EDIT    4000

pawsModsWindow::pawsModsWindow() :
    spawnw(NULL)
{
    for (unsigned i = 0; i < psGMSpawnMods::ITEM_NUM_TYPES; i++)
    {
        list[i] = NULL;
        selectedRow[i] = -1;
    }
}

pawsModsWindow::~pawsModsWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_GMSPAWNMODS);
}

bool pawsModsWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GMSPAWNMODS);

    list[psGMSpawnMods::ITEM_PREFIX] = (pawsListBox*)FindWidget("prefixList");
    list[psGMSpawnMods::ITEM_SUFFIX] = (pawsListBox*)FindWidget("suffixList");
    list[psGMSpawnMods::ITEM_ADJECTIVE] = (pawsListBox*)FindWidget("adjectiveList");
    list[psGMSpawnMods::ITEM_PREFIX]->UseTitleRow(false);
    list[psGMSpawnMods::ITEM_SUFFIX]->UseTitleRow(false);
    list[psGMSpawnMods::ITEM_ADJECTIVE]->UseTitleRow(false);
    spawnw = (pawsGMSpawnWindow*)PawsManager::GetSingleton().FindWidget("GMSpawnWindow");

    return list[psGMSpawnMods::ITEM_PREFIX] &&
           list[psGMSpawnMods::ITEM_SUFFIX] &&
           list[psGMSpawnMods::ITEM_ADJECTIVE] &&
           spawnw;
}

void pawsModsWindow::HandleMessage(MsgEntry* me)
{
    psGMSpawnMods msg(me);

    mods = msg.mods;
    list[psGMSpawnMods::ITEM_PREFIX]->Clear();
    list[psGMSpawnMods::ITEM_SUFFIX]->Clear();
    list[psGMSpawnMods::ITEM_ADJECTIVE]->Clear();

    // Fill in the list boxes with the choices.
    for (size_t i = 0; i < mods.GetSize(); i++)
    {
        const char* name = mods[i].name.GetData();
        pawsListBox* l = list[mods[i].type];
        pawsListBoxRow* row = l->NewRow();
        pawsTextBox* tb = (pawsTextBox*)row->GetColumn(0);
        tb->SetText(name);
        tb->SetName(name);
    }

    list[psGMSpawnMods::ITEM_PREFIX]->SortRows();
    list[psGMSpawnMods::ITEM_SUFFIX]->SortRows();
    list[psGMSpawnMods::ITEM_ADJECTIVE]->SortRows();

    Show();
}

void pawsModsWindow::OnListAction(pawsListBox* widget, int status)
{
    uint32_t type;
    if (widget == list[psGMSpawnMods::ITEM_PREFIX])
        type = psGMSpawnMods::ITEM_PREFIX;
    else if (widget == list[psGMSpawnMods::ITEM_SUFFIX])
        type = psGMSpawnMods::ITEM_SUFFIX;
    else if (widget == list[psGMSpawnMods::ITEM_ADJECTIVE])
        type = psGMSpawnMods::ITEM_ADJECTIVE;
    else
        return;

    if (status == LISTBOX_HIGHLIGHTED)
    {
        // Selecting the same entry toggles it on/off
        if (selectedRow[type] >= 0 &&
            selectedRow[type] == widget->GetSelectedRowNum())
        {
            selectedRow[type] = -1;
            widget->SelectByIndex(-1, false);
            spawnw->SetItemModifier("", 0, type);
            return;
        }
        pawsListBoxRow* row = widget->GetSelectedRow();
        if (row)
        {
            const char* name = ((pawsTextBox*)row->GetColumn(0))->GetText();
            uint32_t id = 0;
            for (size_t i = 0; i < mods.GetSize(); i++)
            {
                if (mods[i].name == name)
                {
                    id = mods[i].id;
                    break;
                }
            }
            selectedRow[type] = widget->GetSelectedRowNum();
            spawnw->SetItemModifier(name, id, type);
        }
        else
        {
            selectedRow[type] = -1;
            spawnw->SetItemModifier("", 0, type);
        }
    }
#if 0
    else if (status == LISTBOX_SELECTED)
    {
        spawnw->SetItemModifier(list[mods[i].type]
    }
#endif
}

void pawsModsWindow::Show()
{
    selectedRow[psGMSpawnMods::ITEM_PREFIX] = -1;
    selectedRow[psGMSpawnMods::ITEM_SUFFIX] = -1;
    selectedRow[psGMSpawnMods::ITEM_ADJECTIVE] = -1;
    pawsWidget::Show();
}
