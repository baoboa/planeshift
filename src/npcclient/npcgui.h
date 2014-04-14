/*
* npcgui.h - Author: Mike Gist
*
* Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __NPCGUI_H__
#define __NPCGUI_H__

#include "util/genericevent.h"

#include "paws/pawswidget.h"

/**
 * \addtogroup npcclient
 * @{ */

class EventHandler;
class PawsManager;
class pawsComboBox;
class pawsNPCClientWindowFactory;
class pawsMainWidget;
class pawsOkBox;
class pawsYesNoBox;
class psNPCClient;
struct iGraphics2D;
struct iGraphics3D;
struct iObjectRegistry;

class NpcGui
{
public:
    NpcGui(iObjectRegistry* object_reg, psNPCClient* npcclient);
    ~NpcGui();

    bool Initialise();

private:
    iObjectRegistry* object_reg;
    psNPCClient* npcclient;
    csRef<iGraphics3D> g3d;
    csRef<iGraphics2D> g2d;

    // PAWS
    PawsManager*    paws;
    pawsMainWidget* mainWidget;

    // Event handling.
    DeclareGenericEventHandler(EventHandler, NpcGui, "planeshift.launcher");
    csRef<EventHandler> event_handler;
    csRef<iEventQueue> queue;

    /* Handles an event from the event handler */
    bool HandleEvent(iEvent &ev);

    /* keeps track of whether the window is visible or not. */
    bool drawScreen;

    /* Limits the frame rate either by sleeping. */
    void FrameLimit();

    /* Time when the last frame was drawn. */
    csTicks elapsed;

    /* Widget for npcclient gui. */
    pawsNPCClientWindowFactory* guiWidget;
};

class pawsNPCClientWindow : public pawsWidget
{
public:
    pawsNPCClientWindow();

private:
};

CREATE_PAWS_FACTORY(pawsNPCClientWindow);

/** @} */

#endif // __NPCGUI_H__
