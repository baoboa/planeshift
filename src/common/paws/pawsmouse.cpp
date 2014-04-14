/*
 * pawsmouse.cpp - Author: Andrew Craig
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
// pawsmouse.cpp: implementation of the pawsMouse class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>

#include <csutil/ref.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <igraphic/imageio.h>
#include <iutil/vfs.h>
#include <csutil/databuf.h>
#include <csutil/cfgmgr.h>

#include "pawsmouse.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"
#include "pawsimagedrawable.h"
#include "pawswidget.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsMouse::pawsMouse()
{
    graphics3D = PawsManager::GetSingleton().GetGraphics3D();
    vfs =  csQueryRegistry<iVFS > (PawsManager::GetSingleton().GetObjectRegistry());
    imageLoader =  csQueryRegistry<iImageIO > (PawsManager::GetSingleton().GetObjectRegistry());

    currentPosition = psPoint(0,0);
    deltas = psPoint(0, 0);
    hidden = false;
    crosshair = false;
    crosshairImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("Crosshair Mouse Pointer");
    useOS = false;
    csRef<iConfigManager> cfg =  csQueryRegistry<iConfigManager> (PawsManager::GetSingleton().GetObjectRegistry());
    basicCursor = cfg->GetBool("PlaneShift.GUI.BasicCursor");
}

pawsMouse::~pawsMouse()
{
}

void pawsMouse::SetPosition(int x, int y)
{
    deltas.x = x - currentPosition.x;
    deltas.y = y - currentPosition.y;

    currentPosition.x = x;
    currentPosition.y = y;

    UpdateDragPosition();
}

void pawsMouse::UpdateDragPosition()
{
    //update the drag and drop widget if any
    pawsWidget* widget = PawsManager::GetSingleton().GetDragDropWidget();
    if(widget)
    {
        csRect frame = widget->GetScreenFrame();
        widget->MoveTo(currentPosition.x - frame.Width()/2, currentPosition.y - frame.Height()/2);
    }
}

void pawsMouse::ChangeImage(const char* imageName)
{
    if(basicCursor)
    {
        graphics3D->GetDriver2D()->SetMouseCursor(csmcArrow);
        useOS = true;
        return;
    }
    cursorImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(imageName);
    if(!cursorImage.IsValid())
    {
        Error2("Fatal error: Can't find cursor '%s'!", imageName);
        exit(1);
    }
    SetOSMouse(cursorImage);
}

void pawsMouse::ChangeImage(iPawsImage* drawable)
{
    if(basicCursor)
    {
        graphics3D->GetDriver2D()->SetMouseCursor(csmcArrow);
        useOS = true;
        return;
    }
    cursorImage = drawable;
    SetOSMouse(cursorImage);
}

void pawsMouse::SetOSMouse(iPawsImage* drawable)
{
    pawsImageDrawable* pwDraw = dynamic_cast<pawsImageDrawable*>((iPawsImage*)drawable);
    if(!pwDraw)
        return;

    image = pwDraw->GetImage();
    if(!image)
        return;

    iGraphics2D* g2d = graphics3D->GetDriver2D();

    transparentR = pwDraw->GetTransparentRed();
    transparentG = pwDraw->GetTransparentGreen();
    transparentB = pwDraw->GetTransparentBlue();

    // Attempt to use image to enable OS level cursor
    csRGBcolor color(transparentR, transparentG, transparentB);
    if(g2d->SetMouseCursor(image, &color))
    {
        if(!useOS)
            Debug1(LOG_PAWS,0,"Using OS Cursor\n");
        useOS = true;
        return;
    }
    else g2d->SetMouseCursor(csmcNone);
}

void pawsMouse::Draw()
{
    pawsWidget* widget = PawsManager::GetSingleton().GetDragDropWidget();
    if(widget)
        widget->Draw();
    else if(!useOS && !hidden && cursorImage)
        cursorImage->Draw(currentPosition.x , currentPosition.y);
    if(crosshair && crosshairImage)
        crosshairImage->Draw(graphics3D->GetDriver2D()->GetWidth() / 2, graphics3D->GetDriver2D()->GetHeight() / 2);
}

void pawsMouse::Hide(bool h)
{
    hidden = h;
    if(useOS)
    {
        if(h)
            graphics3D->GetDriver2D()->SetMouseCursor(csmcNone);
        else
        {
            if(basicCursor)
                graphics3D->GetDriver2D()->SetMouseCursor(csmcArrow);
            else
            {
                csRGBcolor color(transparentR, transparentG, transparentB);
                graphics3D->GetDriver2D()->SetMouseCursor(image, &color);
            }
        }
    }
}
