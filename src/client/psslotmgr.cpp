/*
 * psslotmgr.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 */

#include <csutil/event.h>

#include <psconfig.h>
#include "psslotmgr.h"


//=============================================================================
// PlaneShift Includes
//=============================================================================
#include <iscenemanipulate.h>
#include "paws/pawsmainwidget.h"
#include "gui/pawsslot.h"
#include "gui/pawsdndbutton.h"
#include "pscamera.h"
#include "globals.h"
#include "pscharcontrol.h"


//=============================================================================
// Classes
//=============================================================================


psSlotManager::psSlotManager()
{
    isDragging = false;
    isPlacing = false;
    isRotating = false;
    draggingSlot.stackCount = 0;
    last_count = -1;

    // Initialize event shortcuts
    MouseMove = csevMouseMove (psengine->GetEventNameRegistry(), 0);
    MouseDown = csevMouseDown (psengine->GetEventNameRegistry(), 0);
    MouseUp = csevMouseUp (psengine->GetEventNameRegistry(), 0);
    KeyDown = csevKeyboardDown (psengine->GetEventNameRegistry());
    KeyUp = csevKeyboardUp (psengine->GetEventNameRegistry());
}


psSlotManager::~psSlotManager()
{
}


bool psSlotManager::HandleEvent( iEvent& ev )
{
    if(isDragging)
    {
        int button = csMouseEventHelper::GetButton(&ev);
        if(ev.Name == MouseMove)
        {
            if(isPlacing)
            {
                // Update item position.
                UpdateItem();
            }
        }
        else if(ev.Name == MouseDown)
        {
            if(button == csmbLeft) // Left
            {
                if(isPlacing)
                {
                    // Drop the item at the current position.
                    DropItem(!(csMouseEventHelper::GetModifiers(&ev) & CSMASK_SHIFT));
                    return true;
                }
                else
                {
                    PlaceItem();
                }
            }
            else if(button == csmbRight) // right
            {
                if(!isRotating)
                {
                    basePoint = PawsManager::GetSingleton().GetMouse()->GetPosition();
                    isRotating = true;
                    if(csMouseEventHelper::GetModifiers(&ev) & CSMASK_SHIFT)
                    {
                        psengine->GetSceneManipulator()->SetRotation(PS_MANIPULATE_PITCH,PS_MANIPULATE_YAW);
                    }
                    else
                    {
                        psengine->GetSceneManipulator()->SetRotation(PS_MANIPULATE_ROLL,PS_MANIPULATE_NONE);
                    }
                    return true;
                }
            }
            else
            {
                CancelDrag();
            }
        }
        else if(ev.Name == MouseUp)
        {
            if(button == csmbRight) // right
            {
                if(isRotating)
                {
                    //PawsManager::GetSingleton().GetMouse()->SetPosition(basePoint.x, basePoint.y);
                    psengine->GetG2D()->SetMousePosition(basePoint.x, basePoint.y);
                    psengine->GetSceneManipulator()->SetPosition(csVector2(basePoint.x, basePoint.y));
                    isRotating = false;
                    return true;
                }
            }
        }
        /*else if(ev.Name == KeyDown)
        {
        }
        else if(ev.Name == KeyUp)
        {
        }*/
    }

    return false;
}


void psSlotManager::CancelDrag()
{

    isDragging = false;

    if(isPlacing)
    {
        psengine->GetSceneManipulator()->RemoveSelected();
        if(hadInventory)
        {
            PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow")->Show();
        }
        isPlacing = false;
        isRotating = false;
        hadInventory = false;
    }

    if( draggingSlot.stackCount>0 ) 
    {
        pawsSlot* dragging = (pawsSlot*)PawsManager::GetSingleton().GetDragDropWidget();
        if ( !dragging )
        {
            return;
        }

        if( draggingSlot.slot )
        {
            ((pawsSlot *)draggingSlot.slot)->SetPurifyStatus(dragging->GetPurifyStatus());
            int oldStack =  ((pawsSlot *)draggingSlot.slot)->StackCount();
            oldStack += draggingSlot.stackCount;

            csString res;
            if(dragging->Image())
                res = dragging->Image()->GetName();
            else
                res.Clear();

            ((pawsSlot *)draggingSlot.slot)->PlaceItem(res, draggingSlot.meshFactName, draggingSlot.materialName, oldStack);
            ((pawsSlot *)draggingSlot.slot)->SetToolTip(draggingSlot.toolTip);
            ((pawsSlot *)draggingSlot.slot)->SetBartenderAction(draggingSlot.Action);
        }
    }
        
    PawsManager::GetSingleton().SetDragDropWidget( NULL );
}


void psSlotManager::OnNumberEntered(const char* /*name*/, int param, int count)
{
    if ( count == -1 )
        return;

    pawsSlot* parent = NULL;
    size_t i = (size_t)param;
    if (i < slotsInUse.GetSize())
    {
        // Get the slot ptr
        parent = slotsInUse[i];
        slotsInUse[i] = NULL;

        // Clean up the trailing NULLs  (can't just delete the index, as that would change other indicies)
        for (i=slotsInUse.GetSize()-1; i!=(size_t)-1; i--)
        {
            if (slotsInUse[i] == NULL)
                slotsInUse.DeleteIndex(i);
            else break;
        }
    }

    if (!parent)
        return;

    last_count = count;
    int purifyStatus = parent->GetPurifyStatus();
    int newStack = parent->StackCount() - count;

    pawsSlot* widget = new pawsSlot();
    widget->SetRelativeFrame( 0,0, parent->GetDefaultFrame().Width(), parent->GetDefaultFrame().Height() );

    if (parent->Image())
        widget->PlaceItem( parent->Image()->GetName(), parent->GetMeshFactName(), parent->GetMaterialName(), count );
    else
        widget->PlaceItem( NULL, parent->GetMeshFactName(), parent->GetMaterialName(), count );

    widget->SetPurifyStatus( purifyStatus );
    widget->SetBackgroundAlpha(0);
    widget->SetParent(NULL);
    widget->DrawStackCount(parent->IsDrawingStackCount());

    SetDragDetails(parent, count);
    parent->StackCount( newStack );
    isDragging = true;
    PawsManager::GetSingleton().SetDragDropWidget( widget );
}


void psSlotManager::SetDragDetails(pawsDnDButton* target )
{
    draggingSlot.stackCount      = 0;			//the only way it can be zero is if this is a DnDButton...
    draggingSlot.slot            = target;
}

void psSlotManager::SetDragDetails(pawsSlot* slot, int count)
{
    draggingSlot.containerID     = slot->ContainerID();
    draggingSlot.slotID          = slot->ID();
    draggingSlot.stackCount      = count;
    draggingSlot.slot            = slot;
    draggingSlot.meshFactName    = slot->GetMeshFactName();
    draggingSlot.materialName    = slot->GetMaterialName();
    draggingSlot.toolTip         = slot->GetToolTip();
    draggingSlot.Action = slot->GetAction();
    //checks if we are dragging the whole thing or not
    draggingSlot.split           = slot->StackCount() != count;
}


void psSlotManager::PlaceItem()
{
    if( draggingSlot.stackCount== 0 ) //stackCount==0 means this is a DnDButton. 
    {
        return;
    }

    // Get WS position.
    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();

    // Create mesh.
    outline = psengine->GetSceneManipulator()->CreateAndSelectMesh(draggingSlot.meshFactName,
        draggingSlot.materialName, psengine->GetPSCamera()->GetICamera()->GetCamera(), csVector2(p.x, p.y));

    // If created mesh is valid.
    if(outline)
    {
        // Hide the inventory so we can see where we're dropping.
        pawsWidget* inventory = PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow");
        hadInventory = inventory->IsVisible();
        inventory->Hide();

        // Get rid of icon.
        PawsManager::GetSingleton().SetDragDropWidget( NULL );

        // Mark placing.
        isPlacing = true;
    }
}


void psSlotManager::UpdateItem()
{
    // Get new position.
    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();

    // If we're rotating then we use mouse movement to determine rotation.
    if(isRotating)
    {
        psengine->GetSceneManipulator()->RotateSelected(csVector2(p.x, p.y));
    }
    else
    {
        // Else we use it to determine item position.
        psengine->GetSceneManipulator()->TranslateSelected(false,
            psengine->GetPSCamera()->GetICamera()->GetCamera(), csVector2(p.x, p.y));
    }
}


void psSlotManager::DropItem(bool guard)
{
    // get final position and rotation
    psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();
    csVector3 pos;
    csVector3 rot;
    psengine->GetSceneManipulator()->GetPosition(pos, rot, csVector2(p.x, p.y));

    // Send drop message.
    psSlotMovementMsg msg( draggingSlot.containerID, draggingSlot.slotID,
      CONTAINER_WORLD, 0, draggingSlot.stackCount, &pos, &rot, guard, false);
    msg.SendMessage();

    // Remove outline mesh.
    psengine->GetSceneManipulator()->RemoveSelected();

    // Show inventory window again.
    if(hadInventory)
    {
        PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow")->Show();
    }

    // Reset flags.
    isDragging = false;
    isPlacing = false;
    isRotating = false;
    hadInventory = false;
}


void psSlotManager::Handle( pawsSlot* slot, bool grabOne, bool grabAll )
{
    if ( !isDragging )
    {
        // Make sure other code isn't drag-and-dropping a different object.
        pawsWidget *dndWidget = PawsManager::GetSingleton().GetDragDropWidget();
        if (dndWidget)
            return;

        if( !slot->GetLock() ){
            int stackCount = slot->StackCount();
            if ( stackCount > 0 )
            {
                int tmpID = (int)slotsInUse.Push(slot);

                if ( stackCount == 1 || grabOne )
                {
                    int old_count = last_count;
                    OnNumberEntered("StackCount",tmpID, 1);
                    last_count = old_count;
                }
                else if ( grabAll )
                {
                    int old_count = last_count;
                    OnNumberEntered("StackCount",tmpID, stackCount);
                    last_count = old_count;
                }
                else // Ask for the number of items to grab
                {
                    csString max;
                    max.Format("Max %d", stackCount );

                    pawsNumberPromptWindow::Create(max, last_count,
                                                   1, stackCount, this, "StackCount", tmpID);
                }
            }
        }
    }
    else
    {
        // Do nothing if it's the same slot and we aren't dropping a split stack.
        // Note that crafters use this to pick up one item from a stack,
        // drop it on the stack, and have the server put the item in a
        // different slot.
        if(slot == draggingSlot.slot && !draggingSlot.split)
        {
            CancelDrag();
            return;
        }
        if( !slot->GetLock() )
        {
            //printf("Slot->ID: %d\n", slot->ID() );
            //printf("Container: %d\n", slot->ContainerID() );
            //printf("DraggingSlot.ID %d\n", draggingSlot.slotID);
            

            if ( draggingSlot.containerID == CONTAINER_SPELL_BOOK )
            {
                // Stop dragging the spell around
                CancelDrag();

                // Set the image to this slot.
                slot->PlaceItem( ((pawsSlot *)draggingSlot.slot)->ImageName(), "", "", draggingSlot.stackCount);
            }
            else
            {
                psSlotMovementMsg msg( draggingSlot.containerID, draggingSlot.slotID,
                               slot->ContainerID(), slot->ID(),
                               draggingSlot.stackCount );
                msg.SendMessage();

                // Reset widgets/objects/status.
                PawsManager::GetSingleton().SetDragDropWidget( NULL );
                isDragging = false;
                if(isPlacing)
                {
                    psengine->GetSceneManipulator()->RemoveSelected();
                    if(hadInventory)
                    {
                        PawsManager::GetSingleton().GetMainWidget()->FindWidget("InventoryWindow")->Show();
                    }
                    isPlacing = false;
                    isRotating = false;
                    hadInventory = false;
                }
            }
        }
    }
}

void psSlotManager::Handle( pawsDnDButton* target )
{
    if( target==NULL )
    {
       return;
    }
    if ( !IsDragging() )
    {
        // Make sure other code isn't drag-and-dropping a different object.
        pawsWidget *dndWidget = PawsManager::GetSingleton().GetDragDropWidget();
        if (dndWidget)
        {
            return;
        }

        if( (target->GetMaskingImage()==NULL || *(target->GetMaskingImage()->GetName())==0) && (target->GetName()==NULL || *(target->GetName())==0 ) )
        { //there's nothing in this button, don't drag it.
           return;
        }

        pawsDnDButton* widget = new pawsDnDButton();
        widget->SetRelativeFrame( 0,0, target->GetDefaultFrame().Width(), target->GetDefaultFrame().Height() );
        widget->PlaceItem( target->GetMaskingImage()!=NULL?target->GetMaskingImage()->GetName():NULL, target->GetName(), target->GetToolTip(), target->GetAction() );
        if( target->GetMaskingImage()==NULL || *(target->GetMaskingImage()->GetName()) == 0 )
        {
            widget->SetText(target->GetName());
        }

        widget->SetBackgroundAlpha(0);
        widget->SetParent( NULL );
        widget->SetIndexBase( target->GetIndexBase() );

        //SetDragDetails(parent, 0);

        SetDragDetails(target);
        isDragging = true;
        PawsManager::GetSingleton().SetDragDropWidget( widget );
    }
    else if( draggingSlot.stackCount == 0 ) //dragging from a dndbutton
    {
        //do nothing if it's the same slot and we aren't dragging a split item
        if(target == (pawsDnDButton *)draggingSlot.slot )
        {
            CancelDrag();
            return;
        }
        if ( target->GetDnDLock() ) //state "down" == true == editable
        {
            target->Clear();

            target->SetImageNameCallback( ((pawsDnDButton *)draggingSlot.slot)->GetImageNameCallback() );
            target->SetNameCallback( ((pawsDnDButton *)draggingSlot.slot)->GetNameCallback() );
            target->SetActionCallback( ((pawsDnDButton *)draggingSlot.slot)->GetActionCallback() );

            if( target->PlaceItem( ((pawsDnDButton *)draggingSlot.slot)->GetMaskingImage()!=NULL?((pawsDnDButton *)draggingSlot.slot)->GetMaskingImage()->GetName():NULL,  ((pawsDnDButton *)draggingSlot.slot)->GetName(), ((pawsDnDButton *)draggingSlot.slot)->GetToolTip(),  ((pawsDnDButton *)draggingSlot.slot)->GetAction() ) )
            {
                //move key bindings
                csString          editedCmd;
                const psControl*  keyBinding;
                csString          keyName;
                uint32            keyButton,
                                  keyMods;
                psControl::Device           keyDevice;

              
                editedCmd.Format("Shortcut %d",((pawsDnDButton *)draggingSlot.slot)->GetButtonIndex()+1 );
                keyBinding =  psengine->GetCharControl()->GetTrigger( editedCmd );

                if( keyBinding->name.Length()>0 )
                {
                    keyButton  = keyBinding->button;
                    keyMods    = keyBinding->mods;
                    keyDevice  = keyBinding->device;

                    psengine->GetCharControl()->RemapTrigger(editedCmd,psControl::NONE,0,0);
             
                    editedCmd.Format("Shortcut %d",target->GetButtonIndex()+1 );
                    psengine->GetCharControl()->RemapTrigger( editedCmd, keyDevice, keyButton, keyMods );
                }

                //set text if there's no icon
                if(  ((pawsDnDButton *)draggingSlot.slot)->GetMaskingImage()==NULL || *(((pawsDnDButton *)draggingSlot.slot)->GetMaskingImage()->GetName()) == 0 )
                {
                    target->SetText( ((pawsDnDButton *)draggingSlot.slot)->GetName() );
                }

                //clear contents of the originating button
                ((pawsDnDButton *)draggingSlot.slot)->Clear();

                //redraw the parent window to change layout
                target->SetParent( ((pawsDnDButton *)draggingSlot.slot)->GetParent() );
                target->GetParent()->GetParent()->OnResize(); // parent is the buttonHolder, grandparent is the shortcut window.

            }
            CancelDrag();
        }
    }
    else //dragging from a Slot
    {
        CancelDrag();
        if( !draggingSlot.Action.IsEmpty() )
        {
            target->PlaceItem( ((pawsSlot *)draggingSlot.slot)->ImageName(), draggingSlot.toolTip, draggingSlot.toolTip, draggingSlot.Action );
        }
        else
        {
            csString t = "/use " + draggingSlot.toolTip;
            target->PlaceItem( ((pawsSlot *)draggingSlot.slot)->ImageName(), draggingSlot.toolTip, draggingSlot.toolTip, t );
            draggingSlot.stackCount--;
        }
    }

}
