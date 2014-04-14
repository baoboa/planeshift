/*
 * pawsnumberpromptwindow.h - Author: Ondrej Hurt
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

#ifndef PAWS_COLOR_PROMPT_WINDOW_HEADER
#define PAWS_COLOR_PROMPT_WINDOW_HEADER

#include <csutil/list.h>
#include <iutil/document.h>
#include "pawspromptwindow.h"
#include "pawsbutton.h"
#include "pawscrollbar.h"

class pawsButton;
class pawsEditTextBox;
class pawsScrollbar;

/**
 * \addtogroup common_paws
 * @{ */

class iOnColorEnteredAction
{
public:
    virtual void OnColorEntered(const char* name,int param,int color) = 0;
    // can be -1
    virtual ~iOnColorEnteredAction() {};
};

class pawsColorInput : public pawsWidget
{
public:
    pawsColorInput()
    {
        SetRelativeFrameSize(140, 80);

        // we have three scroll bars, one for Red, Green and Blue and a button used for "Preview".
        // The preview works following: As soon as the user scrolls, the buttons background colour
        // changes to the set colour.
        scrollBarR = new pawsScrollBar();
        scrollBarR->SetHorizontal(true);
        scrollBarR->SetRelativeFrame(1, 1, 110, 20);
        scrollBarR->SetTickValue(1);
        scrollBarR->PostSetup();

        scrollBarG = new pawsScrollBar();
        scrollBarG->SetHorizontal(true);
        scrollBarG->SetRelativeFrame(1, 31, 110, 20);
        scrollBarG->SetTickValue(1);
        scrollBarG->PostSetup();

        scrollBarB = new pawsScrollBar();
        scrollBarB->SetHorizontal(true);
        scrollBarB->SetRelativeFrame(1, 61, 110, 20);
        scrollBarB->SetTickValue(1);
        scrollBarB->PostSetup();

        buttonPreview = new pawsButton();
        buttonPreview->SetRelativeFrame(115, 35, 20, 20);

        AddChild(scrollBarR);
        AddChild(scrollBarG);
        AddChild(scrollBarB);
        AddChild(buttonPreview);
    }

    pawsColorInput(const pawsColorInput &origin):pawsWidget(origin)
    {
        buttonPreview = 0;
        scrollBarB = 0;
        scrollBarG = 0;
        scrollBarR = 0;
        for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
        {
            if(origin.children[i] == origin.buttonPreview)
                buttonPreview = dynamic_cast<pawsButton*>(children[i]);
            else if(origin.children[i] == origin.scrollBarB)
                scrollBarB = dynamic_cast<pawsScrollBar*>(children[i]);
            else if(origin.children[i] == origin.scrollBarG)
                scrollBarG = dynamic_cast<pawsScrollBar*>(children[i]);
            else if(origin.children[i] == origin.scrollBarR)
                scrollBarR = dynamic_cast<pawsScrollBar*>(children[i]);

            if(buttonPreview != 0 && scrollBarB != 0 && scrollBarG != 0 && scrollBarR != 0) break;
        }
    }

    pawsScrollBar* scrollBarR;
    pawsScrollBar* scrollBarG;
    pawsScrollBar* scrollBarB;
    pawsButton* buttonPreview;
};

CREATE_PAWS_FACTORY(pawsColorInput);
/**
 * pawsColorPromptWindow is window that lets the user enter a color by the use of three sliders and a color preview.
 */
class pawsColorPromptWindow : public pawsPromptWindow
{
public:
    pawsColorPromptWindow();
    pawsColorPromptWindow(const pawsColorPromptWindow &origin);
    //from pawsWidget:
    virtual bool PostSetup();
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnKeyDown(utf32_char keyCode, utf32_char keyChar, int modifiers);
    virtual bool OnScroll(int scrollDirection, pawsScrollBar* widget);

    void Initialize(const csString &label, int color, int minColor, int maxColor,
                    iOnColorEnteredAction* action,const char* name, int param=0);

    static pawsColorPromptWindow* Create(const csString &label,
                                         int color, int minColor, int maxColor,
                                         iOnColorEnteredAction* action,const char* name, int param=0);


protected:
    /** restrict the RGB range for all components. eg minColor = 0, maxColor = 100 means that
     each color component (R, G, B) can be set to a value between 0 and 100. **/
    void SetBoundaries(int minColor, int maxColor);

    /** This is called when hitting OK*/
    void ColorWasEntered(int color);

    int maxColor, minColor;

    /** This is last valid input from user - we use it to fall back from invalid input */
    csString lastValidText;

    pawsScrollBar* scrollBarR;
    pawsScrollBar* scrollBarG;
    pawsScrollBar* scrollBarB;

    pawsButton* buttonPreview;

    iOnColorEnteredAction* action;
    csString name;
    int param;
};

CREATE_PAWS_FACTORY(pawsColorPromptWindow);

/** @} */

#endif

