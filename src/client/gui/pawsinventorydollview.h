/*
 * pawsinventorydollview.h
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_INVENTORY_DOLL_VIEW_HEADER
#define PAWS_INVENTORY_DOLL_VIEW_HEADER

#include "paws/pawsobjectview.h"

/** The character window - here we add a mouse listener so you can 
 * drag-n-drop inventory onto the avatar and have it automagically allocated 
 * to the correct spot.
 */
class pawsInventoryDollView : public pawsObjectView
{
public:
    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
};

CREATE_PAWS_FACTORY( pawsInventoryDollView );

//--------------------------------------------------------------------------

#endif
