/*
* npcgui.cpp - Author: Mike Gist
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

#include <psconfig.h>

#include <cstool/initapp.h>
#include <iutil/cfgmgr.h>
#include <iutil/eventq.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>

#include "paws/pawsmainwidget.h"
#include "paws/pawsmanager.h"

#include "npcgui.h"

NpcGui::NpcGui(iObjectRegistry* object_reg, psNPCClient* npcclient)
    : object_reg(object_reg), npcclient(npcclient),paws(NULL),mainWidget(NULL),
      drawScreen(false),elapsed(0),guiWidget(NULL)
{
}

bool NpcGui::Initialise()
{
    g3d = csQueryRegistry<iGraphics3D> (object_reg);
    if(!g3d)
    {
        printf("iGraphics3D failed to Init!\n");
        return false;
    }

    g2d = g3d->GetDriver2D();
    if(!g2d)
    {
        printf("iGraphics2D failed to Init!\n");
        return false;
    }

    if(!csInitializer::OpenApplication(object_reg))
    {
        printf("Error initialising app (CRYSTAL not set?)\n");
        return false;
    }

    iNativeWindow* nw = g2d->GetNativeWindow();
    if(nw)
        nw->SetTitle("PlaneShift NPC Client");

    g2d->AllowResize(true);

    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (object_reg);
    if(!configManager)
    {
        printf("configManager failed to Init!\n");
        return false;
    }

    csRef<iVFS> vfs = csQueryRegistry<iVFS> (object_reg);
    if(!vfs)
    {
        printf("vfs failed to Init!\n");
        return false;
    }

    // paws initialization
    csString skinPath;
    skinPath += configManager->GetStr("PlaneShift.GUI.Skin.Dir", "/planeshift/art/skins/");
    skinPath += configManager->GetStr("PlaneShift.GUI.Skin.Selected", "default.zip");
    // This could be a file or a dir
    csString slash(CS_PATH_SEPARATOR);
    if(vfs->Exists(skinPath + slash))
    {
        skinPath += slash;
    }
    else if(vfs->Exists(skinPath + ".zip"))
    {
        skinPath += ".zip";
    }

    paws = new PawsManager(object_reg, skinPath);
    if(!paws)
    {
        printf("Failed to init PAWS!\n");
        return false;
    }

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    // Register factory
    guiWidget = new pawsNPCClientWindowFactory();

    // Load and assign a default button click sound for pawsbutton
    //paws->LoadSound("/planeshift/art/sounds/gui/next.wav","sound.standardButtonClick");

    // Load widgets
    if(!paws->LoadWidget("data/gui/npcclient.xml"))
    {
        printf("Warning: Loading 'data/gui/npcclient.xml' failed!");
        return false;
    }

    pawsWidget* npcclientWidget = paws->FindWidget("NPCClient");
    npcclientWidget->SetBackgroundAlpha(0);

    // Set mouse image.
    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    // Register our event handler
    event_handler.AttachNew(new EventHandler(this));
    csEventID esub[] =
    {
        csevFrame(object_reg),
        csevMouseEvent(object_reg),
        csevKeyboardEvent(object_reg),
        csevQuit(object_reg),
        CS_EVENTLIST_END
    };

    queue = csQueryRegistry<iEventQueue> (object_reg);
    if(!queue)
    {
        printf("No iEventQueue plugin!\n");
        return false;
    }
    queue->RegisterListener(event_handler, esub);

    return true;
}

NpcGui::~NpcGui()
{
    csInitializer::CloseApplication(object_reg);
}

bool NpcGui::HandleEvent(iEvent &ev)
{
    if(paws->HandleEvent(ev))
        return true;

    if(ev.Name == csevFrame(object_reg))
    {
        if(drawScreen)
        {
            FrameLimit();
            g3d->BeginDraw(CSDRAW_2DGRAPHICS);
            paws->Draw();
        }
        else
        {
            csSleep(150);
        }

        g3d->FinishDraw();
        g3d->Print(NULL);
        return true;
    }
    else if(ev.Name == csevCanvasHidden(object_reg, g2d))
    {
        drawScreen = false;
    }
    else if(ev.Name == csevCanvasExposed(object_reg, g2d))
    {
        drawScreen = true;
    }
    return false;
}

void NpcGui::FrameLimit()
{
    csTicks sleeptime;
    csTicks elapsedTime = csGetTicks() - elapsed;

    // Define sleeptime to limit fps to around 30
    sleeptime = 30;

    // Here we sacrifice drawing time
    if(elapsedTime < sleeptime)
        csSleep(sleeptime - elapsedTime);

    elapsed = csGetTicks();
}

pawsNPCClientWindow::pawsNPCClientWindow()
{
}
