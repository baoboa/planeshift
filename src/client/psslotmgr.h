/*
 * psslotmgr.h
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

#ifndef PS_SLOT_MANAGER
#define PS_SLOT_MANAGER

#include <csutil/array.h>

#include "paws/pawsnumberpromptwindow.h"

//=============================================================================
// Forward Declarations
//=============================================================================
class pawsSlot;
class pawsDnDButton;
class MsgHandler;
struct iMeshWrapper;

//=============================================================================
// Classes
//=============================================================================


class psSlotManager : public iOnNumberEnteredAction
{
public:
    psSlotManager();
    virtual ~psSlotManager();

    bool HandleEvent( iEvent& ev );

    void Handle( pawsSlot* slot, bool grabOne = false, bool grabAll = false );
    void Handle( pawsDnDButton* target );

    void SetDragDetails(pawsSlot* slot, int count);
    void SetDragDetails(pawsDnDButton *target );

    bool IsDragging() { return isDragging; }
    bool IsPlacing() { return isPlacing; }

    /** Set the state of the drag flag. */
    void SetDragFlag( bool flag ) { isDragging = flag; }

    int HoldingContainerID() { return draggingSlot.containerID; }
    int HoldingSlotID() { return draggingSlot.slotID; }

    void CancelDrag();

    void OnNumberEntered(const char *name,int param,int value);


private:
    struct DraggingSlot
    {
        int containerID;
        int slotID;
        int stackCount;
        pawsWidget* slot;
        csString meshFactName;
        csString materialName;
        csString toolTip;
        csString Action;
        int parentID;
        bool split; ///< Holds if the slot we are dragging contains a split of a stack.
    } draggingSlot;

    void PlaceItem();
    void UpdateItem();
    void DropItem(bool guard);


    bool isDragging;
    bool isPlacing;
    bool isRotating;
    bool hadInventory;

    psPoint basePoint;

    csArray<pawsSlot*> slotsInUse;
    int last_count;

    csEventID MouseMove; ///< Shortcut for event mouse move
    csEventID MouseDown; ///< Shortcut for event mouse down
    csEventID MouseUp;   ///< Shortcut for event mouse up
    csEventID KeyDown;   ///< Shortcut for event key down
    csEventID KeyUp;     ///< Shortcut for event key up

    csString draggedMesh;
    iMeshWrapper* outline;
};
#endif
