/*
 * pawscraftcancelwindow.h by Tapped
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

#ifndef PAWS_CRAFTCANCEL_WINDOW_HEADER
#define PAWS_CRAFTCANCEL_WINDOW_HEADER

#include "paws/pawswidget.h"
class pawsProgressBar;

#include "net/cmdbase.h"
#include "net/msghandler.h"

/** This handles all the details about how the craft cancel works.
 */
class pawsCraftCancelWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    virtual ~pawsCraftCancelWindow();
    virtual void HandleMessage(MsgEntry* me);

    virtual bool PostSetup();
    virtual void Draw();
    /** This is called every time the client is crafting something
    *@param craftTime a unsigned int which holds the time when the item is ready
    */
    void Start(csTicks craftTime);
    
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
private:
    /**This would close the window, and send a cancel message to the server
    */
    void Cancel();

    pawsProgressBar*    craftProgress;///<Craft progression bar
    csTicks             startTime,craftTime;///<The start time and the finish time
};

CREATE_PAWS_FACTORY( pawsCraftCancelWindow );

#endif 
