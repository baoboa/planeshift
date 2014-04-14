/*
 * pawsspellcancelwindow.h
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

#ifndef PAWS_SPELLCANCEL_WINDOW_HEADER
#define PAWS_SPELLCANCEL_WINDOW_HEADER

#include "paws/pawswidget.h"
class pawsProgressBar;

#include "net/subscriber.h"

/** This handles all the details about how the spell cancel works.
 */
class pawsSpellCancelWindow : public pawsWidget
{
public:
    pawsSpellCancelWindow();
    virtual ~pawsSpellCancelWindow();

    virtual bool PostSetup();
    virtual void Draw();

    void Start(csTicks castingTime);
    
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
private:

    void Cancel();

    pawsProgressBar*    spellProgress;
    csTicks             startTime,castingTime;
};

CREATE_PAWS_FACTORY( pawsSpellCancelWindow );


#endif 
