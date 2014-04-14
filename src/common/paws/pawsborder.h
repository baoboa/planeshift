/*
 * pawsborder.h - Author: Andrew Craig
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
// pawsborder.h: interface for the pawsBorder class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_BORDER_HEADER
#define PAWS_BORDER_HEADER

#include <ivideo/graph2d.h>
#include <csgeom/csrectrg.h>
#include "pawswidget.h"

/**
 * \addtogroup common_paws
 * @{ */

////////////////////////////////////////////////////////////////////////////////
//  Function Declarations
////////////////////////////////////////////////////////////////////////////////
void DrawBumpFrame(iGraphics2D* graphics2D,
                   pawsWidget* widget,
                   csRect frame, int flags);

////////////////////////////////////////////////////////////////////////////////


// A list of the border image positions
enum BorderPositions
{
    PAWS_BORDER_TOPLEFT,
    PAWS_BORDER_TOPRIGHT,
    PAWS_BORDER_BOTTOMLEFT,
    PAWS_BORDER_BOTTOMRIGHT,
    PAWS_BORDER_LEFTMIDDLE,
    PAWS_BORDER_RIGHTMIDDLE,
    PAWS_BORDER_TOPMIDDLE,
    PAWS_BORDER_BOTTOMMIDDLE,
    PAWS_BORDER_MAX
};


/** Defines the images that make the border up.
  * These are stored by the PAWS preference manager and can be
  * retrived in order to create a border for a widget.
  */
struct BorderDefinition
{
    /// The name of this border style.
    csString name;

    /// The image resources for each of the border areas.
    csString descriptions[PAWS_BORDER_MAX];
};

/** This is a class that draws the border around a widget.
  * A grapical border consists of 8 images ( as seen in the enum ) that
  * can create a border.
  * The corner images are drawn easily and the images along the sides are
  * tiled to fill in the space.
  */
class pawsBorder
{
public:
    pawsBorder(const char* styleName);
    pawsBorder(const pawsBorder &origin);
    ~pawsBorder();

    void SetParent(pawsWidget* parent);
    void Draw();
    /** Use a particular style for this border.
      * @param style The style name to use for the border. lines is the default one
                     that will draw basic lines around the widget. If style is NULL it
                     will default to lines.
      */
    void UseBorder(const char* style);

    /// Draws just the title bar.
    void JustTitle()
    {
        justTitle = true;
    }

    void Hide()
    {
        draw = false;
    }
    void Show()
    {
        draw = true;
    }
    csRect GetRect();

    void SetTitle(const char* t, bool shadow = true);
    void SetTitleImage(iPawsImage* drawable)
    {
        titleImage = drawable;    // This will take delete responsibility
    }
    void SetTitleAlign(int al)
    {
        align = al;
    }

    iPawsImage* GetTitleImage()
    {
        return titleImage;
    }
    const char* GetTitle()
    {
        return title;
    }

protected:

    void DrawTitle(csRect &frame);
    void DrawFrame(csRect frame);

    csRect frame;
    pawsWidget* parent;

    csRef<iPawsImage> borderImages[PAWS_BORDER_MAX];

    /// Title bar text
    csString title;
    /// Title bar image
    csRef<iPawsImage> titleImage;
    /// Alignment of title text
    int align;

    bool usingGraphics;
    bool draw;
    bool justTitle;
    int style;
    bool shadowFont;
};

/** @} */

#endif


