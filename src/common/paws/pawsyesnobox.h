/*
 * pawsyesnobox.h - Author: Andrew Craig
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

#ifndef PAWS_YESNO_BOX_HEADER
#define PAWS_YESNO_BOX_HEADER

#include "pawswidget.h"
class pawsMultiLineTextBox;

/**
 * \addtogroup common_paws
 * @{ */

////////////////////////////////////////////////////////////////////////////
//  GUI BUTTONS
////////////////////////////////////////////////////////////////////////////
#define CONFIRM_YES -10
#define CONFIRM_NO  -20
////////////////////////////////////////////////////////////////////////////

typedef void (*YesNoResponseFunc)(bool, void*);

/** This is a yes/no box used to do confirms.
 */
class pawsYesNoBox : public pawsWidget
{
public:

    pawsYesNoBox();
    pawsYesNoBox(const pawsYesNoBox &origin);
    virtual ~pawsYesNoBox();

    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* widget);
    void SetNotify(pawsWidget* widget);

    bool PostSetup();
    void SetText(const char* text);

    void SetID(int yes = CONFIRM_YES, int no = CONFIRM_NO);
    void Hide();
    void SetCallBack(YesNoResponseFunc handler , void* owner, const char* text);

    /** Static function that creates and sets up pawsYesNoBox:
     */
    static pawsYesNoBox* Create(pawsWidget* notify, const csString &text,
                                int yesID = CONFIRM_YES, int noID = CONFIRM_NO);
private:
    YesNoResponseFunc handler;
    void* owner;

    pawsMultiLineTextBox* text;
    pawsWidget* notify;

    pawsWidget* yesButton;
    pawsWidget* noButton;
    bool useCustomIDs;
};

CREATE_PAWS_FACTORY(pawsYesNoBox);

/** @} */

#endif


