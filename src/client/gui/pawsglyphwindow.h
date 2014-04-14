/*
 * glyph.h - Author: Anders Reggestad
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

#ifndef PAWS_GLYPH_WINDOW
#define PAWS_GLYPH_WINDOW

// CS INCLUDES
#include <csutil/array.h>
#include <imap/loader.h>

#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "pawsslot.h"
#include "inventorywindow.h"
#include "util/psconst.h"
#include "net/messages.h"

#define FLOATING_SLOT  GLYPH_LIST_MAX_SLOTS

struct iEngine;


class pawsGlyphSlot : public pawsSlot
{
public:
    pawsGlyphSlot()
    {
        dragDrop = false;
        emptyOnZeroCount = true;
        Clear();
    }
    int GetStatID()
    {
        return statID;
    }
    void SetStatID(int statID)
    {
        this->statID = statID;
    }
    void Clear()
    {
        statID = 0;

        pawsSlot::Clear();
    }
    int Way()
    {
        return wayID;
    }
    void SetWay(int way)
    {
        wayID = way;
    }
protected:
    int statID;
    int wayID;
};

CREATE_PAWS_FACTORY( pawsGlyphSlot );



class pawsGlyphWindow  : public pawsWidget, public psClientNetSubscriber
{
public:

    pawsGlyphWindow();
    virtual ~pawsGlyphWindow();

    /** For the iNetSubsciber interface.
     */
    void HandleMessage( MsgEntry* message );

    bool PostSetup();
    void Show();
    void Hide();
    bool OnMouseDown(int button, int modifiers, int x, int y);
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );

protected:
    void HandleAssemble( MsgEntry* me );
    void HandleGlyphList( MsgEntry* me );
    
    /** Create new row in listbox of glyph slots */
    void NewGlyphSlotRow(int wayNum);

    /** Finds free glyph slot (may create new slots when all are occupied) */
    pawsGlyphSlot * FindFreeSlot(int wayNum);

    /** Drag and drop */
    void StartDrag(pawsGlyphSlot *sourceSlot);
    void StopDrag(pawsGlyphSlot *sourceSlot);

    /** Sends content of assembler to server, if infoRequest is true is only to check if the assembler content is from a spell that you already know */
    void SendAssembler(bool infoRequest = false);

    /*** Clears the Spell name and Description fields. ***/
    void ClearSpell();

    pawsMessageTextBox * description;
    pawsTextBox * spellName;
    pawsWidget *spellImage;
    csArray <pawsListBox*> ways;

    pawsGlyphSlot * assembler[GLYPH_ASSEMBLER_SLOTS];
};


CREATE_PAWS_FACTORY( pawsGlyphWindow );



#endif 

