/*
 * pawsgmspawn.cpp - Author: Christian Svensson
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

#include <psconfig.h>
#include <iutil/objreg.h>

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"

#include "globals.h"
#include <ibgloader.h>
#include "paws/pawslistbox.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsobjectview.h"
#include "paws/pawstextbox.h"
#include "paws/pawstree.h"
#include "paws/pawscheckbox.h"
#include "pawsgmspawn.h"
#include "pawsmodswindow.h"
#include "pscelclient.h"

pawsGMSpawnWindow::pawsGMSpawnWindow()
{
    itemTree = NULL;
    itemName = NULL;
    itemCount = NULL;
    itemQuality = NULL;
    objView = NULL;
    cbForce = NULL;
    cbLockable = NULL;
    cbLocked = NULL;
    cbPickupable = NULL;
    cbPickupableWeak = NULL;
    cbCollidable = NULL;
    cbUnpickable = NULL;
    cbTransient = NULL;
    cbSettingItem = NULL;
    cbNPCOwned = NULL;
    lockSkill = NULL;
    lockStr = NULL;
    mods.Push(0);
    mods.Push(0);
    mods.Push(0);
    modwindow = NULL;
}

pawsGMSpawnWindow::~pawsGMSpawnWindow()
{
    psengine->UnregisterDelayedLoader(this);
    psengine->GetMsgHandler()->Unsubscribe( this, MSGTYPE_GMSPAWNITEMS );
    psengine->GetMsgHandler()->Unsubscribe( this, MSGTYPE_GMSPAWNTYPES );
}

bool pawsGMSpawnWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_GMSPAWNITEMS );
    psengine->GetMsgHandler()->Subscribe( this, MSGTYPE_GMSPAWNTYPES );
    loaded = true;

    itemName = (pawsTextBox*)FindWidget("ItemName");
    itemCount= (pawsEditTextBox*)FindWidget("Count");
    itemQuality = (pawsEditTextBox*)FindWidget("Quality");
    objView  = (pawsObjectView*)FindWidget("ItemView");

    //wait for the view to complete loading
    while(!objView->ContinueLoad())
    {
        csSleep(100);
    }
    
    itemImage = FindWidget("ItemImage");
    cbForce  = (pawsCheckBox*)FindWidget("Force");
    cbLockable = (pawsCheckBox*)FindWidget("Lockable");
    cbLocked = (pawsCheckBox*)FindWidget("Locked");
    cbPickupable = (pawsCheckBox*)FindWidget("Pickupable");
    cbPickupableWeak = (pawsCheckBox*)FindWidget("PickupableWeak");
    cbCollidable = (pawsCheckBox*)FindWidget("Collidable");
    cbUnpickable = (pawsCheckBox*)FindWidget("Unpickable");
    cbTransient = (pawsCheckBox*)FindWidget("Transient");
    cbSettingItem = (pawsCheckBox*)FindWidget("Settingitem");
    cbNPCOwned = (pawsCheckBox*)FindWidget("Npcowned");
    lockSkill = (pawsEditTextBox*)FindWidget("LockSkill");
    lockStr = (pawsEditTextBox*)FindWidget("LockStr");
    factname = (pawsTextBox*)FindWidget("meshfactname");
    meshname = (pawsTextBox*)FindWidget("meshname");
    imagename = (pawsTextBox*)FindWidget("imagename");
    modname[psGMSpawnMods::ITEM_PREFIX] = (pawsTextBox*)FindWidget("prefix");
    modname[psGMSpawnMods::ITEM_SUFFIX] = (pawsTextBox*)FindWidget("suffix");
    modname[psGMSpawnMods::ITEM_ADJECTIVE] = (pawsTextBox*)FindWidget("adjective");

    // creates tree:
    itemTree = new pawsItemTree;
    if (itemTree == NULL)
    {
        Error1("Could not create widget pawsItemTree");
        return false;
    }

    AddChild(itemTree);
    itemTree->SetDefaultColor( graphics2D->FindRGB( 0,255,0 ) );
    itemTree->SetRelativeFrame(0,0,GetActualWidth(250),GetActualHeight(560));
    itemTree->SetNotify(this);
    itemTree->SetAttachFlags(ATTACH_TOP | ATTACH_BOTTOM | ATTACH_LEFT);
    itemTree->SetScrollBars(false, true);
    itemTree->UseBorder("line");
    itemTree->Resize();

    itemTree->InsertChildL("", "TypesRoot", "", "");

    pawsTreeNode * child = itemTree->GetRoot()->GetFirstChild();
    while (child != NULL)
    {
        child->CollapseAll();
        child = child->GetNextSibling();
    }

    itemTree->SetNotify(this);

    return true;
}

void pawsGMSpawnWindow::HandleMessage(MsgEntry* me)
{
    if(me->GetType() == MSGTYPE_GMSPAWNITEMS)
    {
        psGMSpawnItems msg(me);
        for(size_t i = 0;i < msg.items.GetSize(); i++)
        {
            itemTree->InsertChildL(msg.type, msg.items.Get(i).name, msg.items.Get(i).name, "");

            Item item;
            item.name       = msg.items.Get(i).name;
            item.mesh       = msg.items.Get(i).mesh;
            item.icon       = msg.items.Get(i).icon;
            item.category   = msg.type;
            items.Push(item);
        }

    }
    else if(me->GetType() == MSGTYPE_GMSPAWNTYPES)
    {
        Show();
        psGMSpawnTypes msg(me);
        for(size_t i = 0;i < msg.types.GetSize();i++)
        {
            itemTree->InsertChildL("TypesRoot", msg.types.Get(i), msg.types.Get(i), "");
        }
    }
}

bool pawsGMSpawnWindow::OnSelected(pawsWidget* widget)
{
    pawsTreeNode* node = (pawsTreeNode*)widget;
    if(node->GetFirstChild() != NULL)
        return true;

    if(!modwindow)
    {
        modwindow = (pawsModsWindow*)PawsManager::GetSingleton().FindWidget("ModsWindow");
        if(!modwindow)
            return true;
    }
    if(strcmp(node->GetParent()->GetName(),"TypesRoot"))
    {
        // If already selected, nothing to do.
        if(currentItem == node->GetName())
            return true;

        Item item;
        for(size_t i = 0;i < items.GetSize();i++)
        {
            if(items.Get(i).name == node->GetName())
            {
                item = items.Get(i);
                break;
            }
        }

        if ( !item.mesh )
        {
            objView->Clear();
        }
        else
        {
            factName = item.mesh;
            psengine->GetCelClient()->replaceRacialGroup(factName);
            loaded = false;
            CheckLoadStatus();
        }

        // set stuff
        itemName->SetText(item.name);
        itemCount->SetText("1");
        itemCount->Show();
        itemQuality->SetText("0");
        itemQuality->Show();
        itemImage->Show();
        itemImage->SetBackground(item.icon);
        if (!item.icon || (itemImage->GetBackground() != item.icon)) // if setting the background failed...hide it
            itemImage->Hide();
        itemName->SetText(item.name);
        pawsWidget* spawnBtn = FindWidget("Spawn");
        spawnBtn->Show();
        cbLockable->SetState(false);
        cbLocked->SetState(false);
        cbPickupable->SetState(true);
        cbPickupableWeak->SetState(true);
        cbCollidable->SetState(false);
        cbUnpickable->SetState(false);
        cbTransient->SetState(true);
        cbSettingItem->SetState(false);
        cbNPCOwned->SetState(false);
        lockSkill->SetText("Lockpicking");
        lockStr->SetText("5");
        imagename->SetText(item.icon);
        factname->SetText(item.mesh);
        meshname->SetText(item.name);
        modname[psGMSpawnMods::ITEM_PREFIX]->SetText("");
        modname[psGMSpawnMods::ITEM_SUFFIX]->SetText("");
        modname[psGMSpawnMods::ITEM_ADJECTIVE]->SetText("");
        mods[psGMSpawnMods::ITEM_PREFIX] = 0;
        mods[psGMSpawnMods::ITEM_SUFFIX] = 0;
        mods[psGMSpawnMods::ITEM_ADJECTIVE] = 0;

        cbLockable->Show();
        cbLocked->Show();
        cbPickupable->Show();
        cbPickupableWeak->Show();
        cbCollidable->Show();
        cbUnpickable->Show();
        cbTransient->Show();
        cbSettingItem->Show();
        cbNPCOwned->Show();
        lockStr->Hide();
        lockSkill->Hide();
        imagename->Show();
        meshname->Show();
        factname->Show();
        modname[psGMSpawnMods::ITEM_PREFIX]->Show();
        modname[psGMSpawnMods::ITEM_SUFFIX]->Show();
        modname[psGMSpawnMods::ITEM_ADJECTIVE]->Show();

        //No devs can't use those flags. It's pointless hacking here as the server double checks
        if(psengine->GetCelClient()->GetMainPlayer()->GetType() < 30)
        {
            cbSettingItem->Hide();
            cbNPCOwned->Hide();
        }

        modwindow->Hide();
        currentItem = item.name;
        return true;
    }

    modwindow->Hide();
    currentItem.Clear();

    psGMSpawnItems msg(widget->GetName());
    msg.SendMessage();

    return true;
}

bool pawsItemTree::OnDoubleClick(int button, int modifiers, int x, int y)
{
    if(root != NULL)
    {
        pawsTreeNode* node = FindNodeAt(root, x, y);
        if(node != NULL)
        {
            // If the parent is not the root,
            // then it is an item instead of a type category.
            if(strcmp(node->GetParent()->GetName(), "TypesRoot"))
            {
                psGMSpawnGetMods msg(node->GetName());
                msg.SendMessage();
                return true;
            }
        }
    }
    return false;
}

void pawsGMSpawnWindow::SetItemModifier(const char* name, uint32_t id, uint32_t type)
{
    if(type < sizeof(modname) / sizeof(modname[0]))
    {
        modname[type]->SetText(name);
        mods[type] = id;
    }
}

bool pawsGMSpawnWindow::CheckLoadStatus()
{
    if(!loaded)
    {
        loaded = objView->View(factName);
        if(loaded)
        {
            psengine->UnregisterDelayedLoader(this);
        }
        else
        {
            psengine->RegisterDelayedLoader(this);
        }
    }

    return true;
}

bool pawsGMSpawnWindow::OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(widget == cbLockable)
    {
        if(cbLockable->GetState())
        {
            lockSkill->SetText("Lockpicking");
            lockStr->SetText("5");

            lockStr->Show();
            lockSkill->Show();
        }
        else
        {
            lockStr->Hide();
            lockSkill->Hide();
        }
    }
    else if(widget && !strcmp(widget->GetName(),"Spawn"))
    {
        if(currentItem.IsEmpty())
            return true;

        if(modwindow)
            modwindow->Hide();

        psGMSpawnItem msg(
            currentItem,
            atol(itemCount->GetText()),
            cbLockable->GetState(),
            cbLocked->GetState(),
            lockSkill->GetText(),
            atoi(lockStr->GetText()),
            cbPickupable->GetState(),
            cbCollidable->GetState(),
            cbUnpickable->GetState(),
            cbTransient->GetState(),
            cbSettingItem->GetState(),
            cbNPCOwned->GetState(),
            cbPickupableWeak->GetState(),
            false,
            atof(itemQuality->GetText()),
            &mods);

        // Send spawn message
        psengine->GetMsgHandler()->SendMessage(msg.msg);
    }
    else if(widget->GetID() == 2001) // Rotate
    {
        iMeshWrapper* mesh = objView->GetObject();
        if(mesh)
        {
            static bool toggle = true;

            if(toggle)
                objView->Rotate(10,0.05f);
            else
                objView->Rotate(-1,0);

            toggle = !toggle;
        }
    }
    else if(widget->GetID() == 2002) // Up)
    {
        csVector3 pos = objView->GetCameraPosModifier();
        pos.y += 0.1f;
        objView->SetCameraPosModifier(pos);
    }
    else if(widget->GetID() == 2003) // Down
    {
        csVector3 pos = objView->GetCameraPosModifier();
        pos.y -= 0.1f;
        objView->SetCameraPosModifier(pos);
    }

    return true;
}

void pawsGMSpawnWindow::Show()
{
    itemTree->Clear();
    pawsWidget::Show();
}

void pawsGMSpawnWindow::Close()
{
    if(modwindow)
        modwindow->Close();
    pawsWidget::Close();
}
