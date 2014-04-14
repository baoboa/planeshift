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
#ifndef PAWS_MODS_WINDOW_HEADER
#define PAWS_MODS_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "net/cmdbase.h"

class pawsListBox;
class pawsGMSpawnWindow;

/**
 * The Mods window shows 3 lists of item modifiers when creating items
 * from the pawsGMSpawnWindow.
 */
class pawsModsWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    pawsModsWindow();
    virtual ~pawsModsWindow();

    bool PostSetup();

    void HandleMessage(MsgEntry* me);

    void OnListAction(pawsListBox* widget, int status);

    void Show();

private:
    pawsListBox* list[psGMSpawnMods::ITEM_NUM_TYPES];
    int selectedRow[psGMSpawnMods::ITEM_NUM_TYPES];
    csArray<psGMSpawnMods::ItemModifier> mods;

    pawsGMSpawnWindow* spawnw;
};

CREATE_PAWS_FACTORY( pawsModsWindow );

#endif
