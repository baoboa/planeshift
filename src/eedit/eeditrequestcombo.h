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

#ifndef EEDIT_REQUESTCOMBO_HEADER
#define EEDIT_REQUESTCOMBO_HEADER

#include "paws/pawsmanager.h"
#include "paws/pawswidget.h"
#include "paws/pawslistbox.h"
#include "paws/pawsbutton.h"

/**
 * \addtogroup eedit
 * @{ */

struct iEEditRequestComboCallback : public virtual iBase
{
    SCF_INTERFACE(iEEditRequestComboCallback, 2, 0, 0);
    virtual void Select (const char* string) = 0;
};

class EEditRequestCombo  : public pawsWidget
{
public:
    EEditRequestCombo() { }

    virtual bool PostSetup(); 

    void ClearChoices ();
    void AddChoice (const char* choice);
    void SetCallback (iEEditRequestComboCallback* cb)
    {
        callback = cb;
    }

private:
    bool OnButtonPressed( int mouseButton, int keyModifer, pawsWidget* widget );

    pawsListBox  * choicesList;
    pawsButton   * okButton;
    pawsButton   * cancelButton;

    csRef<iEEditRequestComboCallback> callback;
};

CREATE_PAWS_FACTORY( EEditRequestCombo );

/** @} */

#endif

