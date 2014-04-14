/*
 * pawsgmspawn.h - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_GMSPAWN_HEADER
#define PAWS_GMSPAWN_HEADER

#include "paws/pawstree.h"
#include "psengine.h"

class pawsObjectView;
class pawsTextBox;
class pawsEditTextBox;
class pawsCheckBox;
class pawsModsWindow;

class pawsItemTree : public pawsSimpleTree
{
public:
    bool OnDoubleClick(int button, int modifiers, int x, int y);
};

class pawsGMSpawnWindow : public pawsWidget, public psClientNetSubscriber, public DelayedLoader
{
public:
    pawsGMSpawnWindow();
    virtual ~pawsGMSpawnWindow();

    bool PostSetup();
    void Show();
    void Close();
    void HandleMessage(MsgEntry* me);
    bool OnSelected(pawsWidget* widget);

    bool OnButtonPressed(int button,int keyModifier,pawsWidget* widget);

    bool CheckLoadStatus();

    void SetItemModifier(const char* name, uint32_t id, uint32_t type);

private:
    pawsItemTree*                   itemTree;
    pawsObjectView*                 objView;
    pawsWidget*                     itemImage;
    pawsTextBox*                    itemName;
    pawsCheckBox*                   cbForce;
    pawsCheckBox*                   cbLockable;
    pawsCheckBox*                   cbLocked;
    pawsCheckBox*                   cbPickupable;
    pawsCheckBox*                   cbPickupableWeak;
    pawsCheckBox*                   cbCollidable;
    pawsCheckBox*                   cbUnpickable;
    pawsCheckBox*                   cbTransient;
    pawsCheckBox*                   cbSettingItem;
    pawsCheckBox*                   cbNPCOwned;
    pawsEditTextBox*                itemCount;
    pawsEditTextBox*                itemQuality;
    pawsEditTextBox*                lockSkill;
    pawsEditTextBox*                lockStr;
    pawsTextBox*                    factname;
    pawsTextBox*                    meshname;
    pawsTextBox*                    imagename;
    pawsTextBox*                    modname[psGMSpawnMods::ITEM_NUM_TYPES];

    struct Item
    {
        csString name;
        csString category;
        csString mesh;
        csString icon;
    };

    csArray<Item>   items;

    csString currentItem;
    bool loaded;
    csString factName;
    csArray<uint32_t> mods;
    pawsModsWindow* modwindow;
};


CREATE_PAWS_FACTORY( pawsGMSpawnWindow );
#endif
