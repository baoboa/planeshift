/*
 * pawsborder.cpp - Author: Andrew Craig
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
// pawsborder.cpp: implementation of the pawsBorder class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>

#include <ivideo/fontserv.h>

#include "pawsborder.h"
#include "pawswidget.h"
#include "pawsmanager.h"
#include "pawsprefmanager.h"

pawsBorder::pawsBorder(const char* name)
{
    usingGraphics = false;
    UseBorder(name);
    draw = true;
    justTitle = false;
    shadowFont = true;
}
pawsBorder::pawsBorder(const pawsBorder &origin) :
    frame(origin.frame),
    parent(NULL),
    title(origin.title),
    titleImage(origin.titleImage),
    align(origin.align),
    usingGraphics(origin.usingGraphics),
    draw(origin.draw),
    justTitle(origin.justTitle),
    style(origin.style),
    shadowFont(origin.shadowFont)
{
    for(unsigned int i = 0 ; i < PAWS_BORDER_MAX ; i++)
        borderImages[i] = origin.borderImages[i];
}
void pawsBorder::UseBorder(const char* name)
{
    csString stylestr(name);
    // If the type is line then this is a simple border drawn using lines.
    if(name && stylestr != "line")
    {
        BorderDefinition* def = PawsManager::GetSingleton().GetPrefs()->GetBorderDefinition(name);
        if(def)
        {
            // For each border area create the image for it based on the
            // border definition.
            for(int x = 0; x < PAWS_BORDER_MAX; x++)
            {
                borderImages[x] = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(def->descriptions[x]);
                if(!borderImages[x])
                {
                    Warning2(LOG_PAWS, "Could not retrieve border drawable: >%s<", def->descriptions[x].GetData());
                    return;
                }
                if(borderImages[x]->GetWidth() == 0 || borderImages[x]->GetHeight() == 0)
                {
                    Warning2(LOG_PAWS, "Invalid dimensions on border drawable: >%s<", def->descriptions[x].GetData());
                    return;
                }
            }
            usingGraphics = true;
        }
        else
        {
            Warning2(LOG_PAWS, "Could not retrieve border definiton: >%s<", name);
        }
    }
    style = BORDER_BUMP;
}


pawsBorder::~pawsBorder()
{
}

void pawsBorder::SetParent(pawsWidget* newParent)
{
    parent = newParent;
}

csRect pawsBorder::GetRect()
{
    csRect frame = parent->GetScreenFrame();

    if(usingGraphics)
    {
        frame.xmin-=borderImages[PAWS_BORDER_TOPLEFT]->GetWidth();
        frame.ymin-=borderImages[PAWS_BORDER_TOPLEFT]->GetHeight();
        frame.xmax+=borderImages[PAWS_BORDER_BOTTOMRIGHT]->GetWidth();
        frame.ymax+=borderImages[PAWS_BORDER_BOTTOMRIGHT]->GetHeight();
    }

    if(title.Length())
    {
        int width,height;
        parent->GetFont()->GetMaxSize(width, height);
        frame.ymin -= height+6;
    }
    return frame;
}

void pawsBorder::Draw()
{
    if(!draw)
        return;

    frame = parent->GetScreenFrame();

    if(title.Length())
        DrawTitle(frame);  // This adjusts the frame to preserve the title bar if present.

    if(!usingGraphics)
    {
        //if ( !justTitle )
        // DrawFrame(frame);
        return;
    }


    /////////////////////////////
    // Draw tiles across the top and bottom
    /////////////////////////////
    borderImages[PAWS_BORDER_TOPLEFT]->Draw(frame.xmin - borderImages[PAWS_BORDER_TOPLEFT]->GetWidth(),
                                            frame.ymin - borderImages[PAWS_BORDER_TOPLEFT]->GetHeight());
    borderImages[PAWS_BORDER_BOTTOMLEFT]->Draw(frame.xmin - borderImages[PAWS_BORDER_BOTTOMLEFT]->GetWidth(),
            frame.ymax);

    int locX, locY = frame.ymin - borderImages[PAWS_BORDER_TOPMIDDLE]->GetHeight();
    for(locX = frame.xmin; locX < frame.xmax; locX += borderImages[PAWS_BORDER_TOPMIDDLE]->GetWidth())
    {
        borderImages[PAWS_BORDER_TOPMIDDLE]->Draw(locX, locY, csMin(borderImages[PAWS_BORDER_TOPMIDDLE]->GetWidth(),frame.xmax-locX), 0);
        borderImages[PAWS_BORDER_BOTTOMMIDDLE]->Draw(locX, frame.ymax, csMin(borderImages[PAWS_BORDER_BOTTOMMIDDLE]->GetWidth(),frame.xmax-locX), 0);
    }

    /////////////////////////////
    // Draw tiles down the left and right
    /////////////////////////////

    locX = frame.xmin - borderImages[PAWS_BORDER_LEFTMIDDLE]->GetWidth();
    for(locY = frame.ymin; locY < frame.ymax; locY += borderImages[PAWS_BORDER_LEFTMIDDLE]->GetHeight())
    {
        borderImages[PAWS_BORDER_LEFTMIDDLE]->Draw(locX, locY, 0, csMin(borderImages[PAWS_BORDER_LEFTMIDDLE]->GetHeight(), frame.ymax-locY));
        borderImages[PAWS_BORDER_RIGHTMIDDLE]->Draw(frame.xmax, locY, 0, csMin(borderImages[PAWS_BORDER_RIGHTMIDDLE]->GetHeight(), frame.ymax-locY));
    }

    borderImages[PAWS_BORDER_TOPRIGHT]->Draw(frame.xmax,
            frame.ymin - borderImages[PAWS_BORDER_TOPRIGHT]->GetHeight());

    borderImages[PAWS_BORDER_BOTTOMRIGHT]->Draw(frame.xmax, frame.ymax);
}



void pawsBorder::DrawTitle(csRect &frame)
{
    int width,height;
    parent->GetFont()->GetMaxSize(width, height);
    if(titleImage)
        frame.ymin -= height+6;

    csRect r(frame);
    r.ymax = r.ymin + height+6;
    if(titleImage)
    {
        int fadeVal = parent->GetFadeVal();
        int alpha = parent->GetMaxAlpha();
        int alphaMin = parent->GetMinAlpha();
        titleImage->Draw(r, (int)(alphaMin + (alpha-alphaMin) * fadeVal * 0.010));
    }
    if(title.Length())
    {
        int drawX=0;
        int drawY=0;
        int width=0;
        int height=0;

        parent->GetFont()->GetDimensions(title , width, height);

        int midX = r.Width() / 2;
        int midY = r.Height() / 2;

        drawX = r.xmin + midX - width/2;
        drawY = r.ymin + midY - height/2;

        parent->DrawWidgetText(title,drawX,drawY, (titleImage && shadowFont) ? 1 : 0);
    }
}

void pawsBorder::DrawFrame(csRect frame)
{
    csRef<iGraphics2D> gfx = PawsManager::GetSingleton().GetGraphics2D();
    DrawBumpFrame(gfx, parent, frame, parent->GetBorderStyle());
}

void DrawBumpFrame(iGraphics2D* graphics2D, pawsWidget* widget, csRect frame, int flags)
{
    int hi;
    int hi2;
    int lo;
    int lo2;
    int black;

//    if ( !widget->HasFocus() )
    {
        black = widget->GetBorderColour(0);

        hi    = widget->GetBorderColour(1);
        lo    = widget->GetBorderColour(2);

        hi2   = widget->GetBorderColour(3);
        lo2   = widget->GetBorderColour(4);
    }
    /*    else // Temp fix to know what the highlighted widget is.
        {
            black = graphics2D->FindRGB( 0,0,0 );

            hi = graphics2D->FindRGB( 255,0,0 );
            lo = graphics2D->FindRGB( 255,0,0 );

            hi2 = graphics2D->FindRGB( 255,0,0 );
            lo2 = graphics2D->FindRGB( 255,0,0 );
        }*/

    if(flags & BORDER_REVERSED)
    {
        int swap;
        black = hi;
        swap = hi;
        hi=lo;
        lo=swap;
        swap = hi2;
        hi2=lo2;
        lo2=swap;
    }

    if(flags & BORDER_OUTER_BEVEL)
    {
        // Top
        graphics2D->DrawLine(frame.xmin + 0,
                             frame.ymin + 0,
                             frame.xmax - 2,
                             frame.ymin + 0,
                             hi);
        graphics2D->DrawLine(frame.xmin + 1,
                             frame.ymin + 1,
                             frame.xmax - 3,
                             frame.ymin + 1,
                             hi2);

        // Left
        graphics2D->DrawLine(frame.xmin + 0,
                             frame.ymin + 0,
                             frame.xmin + 0,
                             frame.ymax - 2,
                             hi);

        graphics2D->DrawLine(frame.xmin + 1,
                             frame.ymin + 1,
                             frame.xmin + 1,
                             frame.ymax - 3,
                             hi2);

        // Bottom
        if(!(flags & BORDER_REVERSED))
        {
            graphics2D->DrawLine(frame.xmin + 1,
                                 frame.ymax - 1,
                                 frame.xmax - 1,
                                 frame.ymax - 1,
                                 black);
        }
        graphics2D->DrawLine(frame.xmin + 2,
                             frame.ymax - 2,
                             frame.xmax - 2,
                             frame.ymax - 2,
                             lo);
        // Right
        graphics2D->DrawLine(frame.xmax - 2,
                             frame.ymin + 2,
                             frame.xmax - 2,
                             frame.ymax - 2,
                             lo);

        if(!(flags & BORDER_REVERSED))
        {
            graphics2D->DrawLine(frame.xmax - 1,
                                 frame.ymin + 1,
                                 frame.xmax - 1,
                                 frame.ymax - 1,
                                 black);
        }
    }

    if(flags & BORDER_INNER_BEVEL)
    {
        // Top
        graphics2D->DrawLine(frame.xmin + 2,
                             frame.ymin + 2,
                             frame.xmax - 4,
                             frame.ymin + 2,
                             lo2);
        graphics2D->DrawLine(frame.xmin + 3,
                             frame.ymin + 3,
                             frame.xmax - 4,
                             frame.ymin + 3,
                             black);

        // Left
        graphics2D->DrawLine(frame.xmin + 2,
                             frame.ymin + 2,
                             frame.xmin + 2,
                             frame.ymax - 4,
                             lo2);

        graphics2D->DrawLine(frame.xmin + 3,
                             frame.ymin + 3,
                             frame.xmin + 3,
                             frame.ymax - 5,
                             black);

        // Bottom
        graphics2D->DrawLine(frame.xmin + 3,
                             frame.ymax - 3,
                             frame.xmax - 3,
                             frame.ymax - 3,
                             lo2);

        graphics2D->DrawLine(frame.xmin + 4,
                             frame.ymax - 4,
                             frame.xmax - 4,
                             frame.ymax - 4,
                             hi2);

        // Right
        graphics2D->DrawLine(frame.xmax - 3,
                             frame.ymin + 2,
                             frame.xmax - 3,
                             frame.ymax - 3,
                             lo2);


        graphics2D->DrawLine(frame.xmax - 4,
                             frame.ymin + 3,
                             frame.xmax - 4,
                             frame.ymax - 4,
                             hi2);
    }
}

void pawsBorder::SetTitle(const char* t, bool shadow)
{
    title = t;
    shadowFont = shadow;
}

