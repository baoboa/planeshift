/*
 * pawscreditswindow.h - Author: Christian Svensson
 *
 * Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef PAWS_CREDITS_WINDOW_HEADER
#define PAWS_CREDITS_WINDOW_HEADER

#include "globals.h"
#include "paws/pawswidget.h"
#include <ivideo/fontserv.h>
 
class pawsProgressBar;

class pawsCreditsWindow : public pawsWidget
{
public:
    ~pawsCreditsWindow(){}
    
    bool PostSetup(); 
    void Draw(); 
    void Hide();
    void Show();
    bool OnButtonPressed(int button, int keyModifier, pawsWidget *widget);
    
private:
    //to count the time
    csTicks initialTime;

    /// This is the big buffer of credit text.
    csString mainstring;

    // scroll credits up 1 pixel every n ms
    csTicks scrollRate;

    // Y position of first line
    int textYofs;

    // number of lines displayed
    size_t numLines;

    // height of the font
    int textHeight;

    //this is what position to start from when displaying the text
    int startPos;

    // the font used
    csRef<iFont> font;

    int pixelAt;

    //default colors (Loaded from file)
    int standardR,standardG,standardB;
    int atR,atG,atB; // @
    int andR,andG,andB; // &

    //Props loaded from file
    int width;
    int height;
    int x;
    int y;

    bool loop;
    csString endMsg;
    int endColor;
    bool box;

    //Speed
    int speed;

    //array
    csArray<csString> credits;
    csArray<int> styles;     
};

CREATE_PAWS_FACTORY( pawsCreditsWindow );

#endif

