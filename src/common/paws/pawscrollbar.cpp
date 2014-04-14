/*
 * pawscrollbar.h - Author: Andrew Craig
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
// pawscrollbar.cpp: implementation of the pawscrollbar class.
//
//////////////////////////////////////////////////////////////////////

#include <psconfig.h>
#include <iutil/virtclk.h>
#include "pawscrollbar.h"
#include "pawsmanager.h"
#include "pawsbutton.h"

#define SCROLL_TICKS         150

#define THUMB_MARGIN         2
#define SCROLL_UP_SND        "gui.scrollup"
#define SCROLL_DOWN_SND      "gui.scrolldown"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
pawsScrollBar::pawsScrollBar()
{
    currentValue    = 0;
    maxValue        = 0;
    minValue        = 0;
    tickValue       = 1;
    horizontal      = false;
    reversed        = false;
    limited         = true;
    factory         = "pawsScrollBar";

    // Used when the mouse button is held down
    clock =  csQueryRegistry<iVirtualClock > (PawsManager::GetSingleton().GetObjectRegistry());

    scrollTicks = clock->GetCurrentTicks();
    mouseDown = false;

    upButton = downButton = NULL;

    thumb = NULL;
    mouseIsDraggingThumb = false;

    upGrey.Clear();
    upUnpressed.Clear();
    upPressed.Clear();
    upOffsetx = 0;
    upOffsety = 0;
    upWidth = 0;
    upHeight = 0;

    downGrey.Clear();
    downUnpressed.Clear();
    downPressed.Clear();
    downOffsetx = 0;
    downOffsety = 0;
    downWidth = 0;
    downHeight = 0;

    thumbStopped = "ScrollBar Thumb";
    thumbMoving = "ScrollBar Thumb Moving";
}

pawsScrollBar::pawsScrollBar(const pawsScrollBar &origin)
    :pawsWidget(origin),
     currentValue(origin.currentValue),
     maxValue(origin.maxValue),
     minValue(origin.minValue),
     tickValue(origin.tickValue),
     clock(origin.clock),
     scrollTicks(origin.scrollTicks),
     mouseDown(origin.mouseDown),
     reversed(origin.reversed),
     lastButton(origin.lastButton),
     lastModifiers(origin.lastModifiers),
     mouseIsDraggingThumb(origin.mouseIsDraggingThumb),
     upGrey(origin.upGrey),
     upUnpressed(origin.upUnpressed),
     upPressed(origin.upPressed),
     upOffsetx(origin.upOffsetx),
     upOffsety(origin.upOffsety),
     upWidth(origin.upWidth),
     upHeight(origin.upHeight),
     downGrey(origin.downGrey),
     downUnpressed(origin.downUnpressed),
     downPressed(origin.downPressed),
     downOffsetx(origin.downOffsetx),
     downOffsety(origin.downOffsety),
     downWidth(origin.downWidth),
     downHeight(origin.downHeight),
     thumbStopped(origin.thumbStopped),
     thumbMoving(origin.thumbMoving),
     horizontal(origin.horizontal),
     limited(origin.limited)
{
    upButton = downButton = NULL;
    thumb = NULL;
    lastWidget = 0;
    thumbDragPoint = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        if(origin.upButton == origin.children[i])
            upButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.downButton == origin.children[i])
            downButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.thumb == origin.children[i])
            thumb = children[i];

        if(upButton!= 0 && downButton != 0 && thumb != 0)
            break;
    }
}

pawsScrollBar::~pawsScrollBar()
{

}

bool pawsScrollBar::Setup(iDocumentNode* node)
{
    // Check for direction
    csRef<iDocumentAttribute> directionAttribute = node->GetAttribute("direction");
    if(directionAttribute)
    {
        csString value(directionAttribute->GetValue());
        if(value == "horizontal")
            SetHorizontal(true);
        else
            SetHorizontal(false);
    }

    limited = false;
    csRef<iDocumentAttribute> attr = node->GetAttribute("maxValue");
    if(attr)
    {
        maxValue = attr->GetValueAsFloat();
        limited = true;
    }

    attr = node->GetAttribute("minValue");
    if(attr)
    {
        minValue = attr->GetValueAsFloat();
        limited = true;
    }

    attr = node->GetAttribute("tick");
    if(attr)
        tickValue = attr->GetValueAsFloat();

    // up button settings
    csRef<iDocumentNode> upNode = node->GetNode("up");
    if(upNode)
    {
        attr = upNode->GetAttribute("grey");
        if(attr)
            upGrey = attr->GetValue();

        attr = upNode->GetAttribute("unpressed");
        if(attr)
            upUnpressed = attr->GetValue();

        attr = upNode->GetAttribute("pressed");
        if(attr)
            upPressed = attr->GetValue();

        attr = upNode->GetAttribute("offsetx");
        if(attr)
            upOffsetx = attr->GetValueAsInt();

        attr = upNode->GetAttribute("offsety");
        if(attr)
            upOffsety = attr->GetValueAsInt();

        attr = upNode->GetAttribute("width");
        if(attr)
            upWidth = attr->GetValueAsInt();

        attr = upNode->GetAttribute("height");
        if(attr)
            upHeight = attr->GetValueAsInt();
    }

    // down button settings
    csRef<iDocumentNode> downNode = node->GetNode("down");
    if(downNode)
    {
        attr = downNode->GetAttribute("grey");
        if(attr)
            downGrey = attr->GetValue();

        attr = downNode->GetAttribute("unpressed");
        if(attr)
            downUnpressed = attr->GetValue();

        attr = downNode->GetAttribute("pressed");
        if(attr)
            downPressed = attr->GetValue();

        attr = downNode->GetAttribute("offsetx");
        if(attr)
            downOffsetx = attr->GetValueAsInt();

        attr = downNode->GetAttribute("offsety");
        if(attr)
            downOffsety = attr->GetValueAsInt();

        attr = downNode->GetAttribute("width");
        if(attr)
            downWidth = attr->GetValueAsInt();

        attr = downNode->GetAttribute("height");
        if(attr)
            downHeight = attr->GetValueAsInt();
    }

    // thumb settings
    csRef<iDocumentNode> thumbNode = node->GetNode("thumb");
    if(thumbNode)
    {
        attr = thumbNode->GetAttribute("stopped");
        if(attr)
            thumbStopped = attr->GetValue();

        attr = thumbNode->GetAttribute("moving");
        if(attr)
            thumbMoving = attr->GetValue();
    }

    return true;
}

void pawsScrollBar::SetMaxValue(float value)
{
    maxValue = value;
    if(limited)
        LimitCurrentValue();
    if(parent != NULL)
        parent->OnScroll(SCROLL_AUTO, this);
    SetThumbLayout();
}

void pawsScrollBar::SetMinValue(float value)
{
    minValue = value;
    if(limited)
        LimitCurrentValue();
    if(parent != NULL)
        parent->OnScroll(SCROLL_AUTO, this);
    SetThumbLayout();
}

void pawsScrollBar::SetCurrentValue(float value, bool triggerEvent, bool publish)
{
    if(reversed)
        currentValue = maxValue-value;
    else
        currentValue = value;
    if(limited)
        LimitCurrentValue();
    SetThumbLayout();

    if(triggerEvent)
        parent->OnScroll(SCROLL_SET, this);

    if(publish)
    {
        for(size_t a=0; a<publishList.GetSize(); ++a)
            PawsManager::GetSingleton().Publish(publishList[a], currentValue);
    }
}

void pawsScrollBar::SetHorizontal(bool value)
{
    horizontal = value;
    SetThumbLayout();
}

bool pawsScrollBar::PostSetup()
{
    // Create the scroll up/left button
    upButton = new pawsButton;
    upButton->SetParent(this);
    int flags;
    if(!horizontal)
    {
        flags = ATTACH_TOP | ATTACH_RIGHT;
        if(upUnpressed.IsEmpty())
            upUnpressed = "Up Arrow";
        if(upPressed.IsEmpty())
            upPressed = "Up Arrow";
    }
    else
    {
        flags = ATTACH_LEFT | ATTACH_BOTTOM;
        if(upUnpressed.IsEmpty())
            upUnpressed = "Left Arrow";
        if(upPressed.IsEmpty())
            upPressed = "Left Arrow";
    }

    upButton->SetUpImage(upUnpressed);
    upButton->SetDownImage(upPressed);
    if(!upGrey.IsEmpty())
    {
        upButton->SetGreyUpImage(upGrey);
        upButton->SetGreyDownImage(upGrey);
    }

    //Add the sound
    upButton->SetSound(SCROLL_UP_SND);

    upButton->SetAttachFlags(flags);
    upButton->SetID(SCROLL_UP);
    AddChild(upButton);

    // Create the scroll down button.
    downButton = new pawsButton;
    downButton->SetParent(this);
    if(!horizontal)
    {
        flags = ATTACH_BOTTOM | ATTACH_RIGHT;
        if(downUnpressed.IsEmpty())
            downUnpressed = "Down Arrow";
        if(downPressed.IsEmpty())
            downPressed = "Down Arrow";
    }
    else
    {
        flags = ATTACH_RIGHT | ATTACH_BOTTOM;
        if(downUnpressed.IsEmpty())
            downUnpressed = "Right Arrow";
        if(downPressed.IsEmpty())
            downPressed = "Right Arrow";
    }
    downButton->SetUpImage(downUnpressed);
    downButton->SetDownImage(downPressed);
    if(!downGrey.IsEmpty())
    {
        downButton->SetGreyUpImage(downGrey);
        downButton->SetGreyDownImage(downGrey);
    }

    //Add the sound
    downButton->SetSound(SCROLL_DOWN_SND);

    downButton->SetAttachFlags(flags);
    downButton->SetID(SCROLL_DOWN);
    AddChild(downButton);

    SetButtonLayout();


    thumb = new pawsThumb();
    AddChild(thumb);
    thumb->SetBackground(thumbStopped);
    thumb->SetID(SCROLL_THUMB);

    SetThumbVisibility();
    if(thumb->IsVisible())
        SetThumbLayout();
    return true;
}

void pawsScrollBar::SetThumbVisibility()
{
    if(thumb != NULL)
    {
        if(GetScrollBarSize()*0.1 < GetThumbScaleLength())
            thumb->Show();
        else
            thumb->Hide();
    }
}

void pawsScrollBar::OnResize()
{
    SetThumbVisibility();
    if(thumb && thumb->IsVisible())
        SetThumbLayout();

    SetButtonLayout();
}

void pawsScrollBar::SetThumbLayout()
{
    int scrollBarSize = GetScrollBarSize();

    if(!thumb)
        return;

    thumb->SetSize(scrollBarSize-2*THUMB_MARGIN, scrollBarSize-2*THUMB_MARGIN);

    if(maxValue == 0)
    {
        if(horizontal)
            thumb->MoveTo(screenFrame.xmin+scrollBarSize, screenFrame.ymin+THUMB_MARGIN);
        else
            thumb->MoveTo(screenFrame.xmin+THUMB_MARGIN, screenFrame.ymin+scrollBarSize);
    }
    else
    {
        if(horizontal)
            thumb->MoveTo(screenFrame.xmin+scrollBarSize
                          +
                          int((currentValue-minValue)/(maxValue-minValue)*GetThumbScaleLength()),
                          screenFrame.ymin+THUMB_MARGIN);
        else
        {
            thumb->MoveTo(screenFrame.xmin+THUMB_MARGIN,
                          screenFrame.ymin+scrollBarSize
                          +
                          int((currentValue-minValue)/(maxValue-minValue)*GetThumbScaleLength()));
        }
    }
}

bool pawsScrollBar::OnMouseDown(int button, int /*modifiers*/, int x, int y)
{
    if(button == csmbWheelUp || button == csmbHWheelLeft)
    {
        ScrollUp();
        // Always return true, we don't want the scroll to go to the parent widget(s)
        return true;
    }
    else if(button == csmbWheelDown || button == csmbHWheelRight)
    {
        ScrollDown();
        return true;
    }
    else if(WidgetAt(x, y) == thumb)
    {
        if(horizontal)
            thumbDragPoint = x - thumb->GetScreenFrame().xmin;
        else
            thumbDragPoint = y - thumb->GetScreenFrame().ymin;

        mouseIsDraggingThumb = true;
        thumb->SetBackground(thumbMoving);
    }
    else
    {
        MoveThumbToMouse();
    }

    return true;
}

bool pawsScrollBar::OnMouseUp(int /*button*/, int /*modifiers*/, int /*x*/, int /*y*/)
{
    mouseIsDraggingThumb = false;
    thumb->SetBackground(thumbStopped);

    return true;
}

bool pawsScrollBar::OnMouseExit()
{
    psPoint pos = PawsManager::GetSingleton().GetMouse()->GetPosition();
    pawsWidget* widget = WidgetAt(pos.x, pos.y);
    if(widget == thumb)
        return false;
    return true;
}

void pawsScrollBar::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    SetCurrentValue(value.GetFloat(), true, false);
}

float pawsScrollBar::GetCurrentValue()
{
    if(reversed)
        return maxValue-currentValue;
    else
        return currentValue;
}

void pawsScrollBar::SetTickValue(float tick)
{
    tickValue = tick;
}

void pawsScrollBar::LimitCurrentValue()
{
    if(currentValue >= maxValue)
    {
        currentValue = maxValue;
        if(downButton)
        {
            downButton->SetEnabled(false);
            mouseDown = false;
        }
    }
    else if(downButton)
        downButton->SetEnabled(true);

    if(currentValue <= minValue)
    {
        currentValue = minValue;
        if(upButton)
        {
            upButton->SetEnabled(false);
            mouseDown = false;
        }
    }
    else if(upButton)
        upButton->SetEnabled(true);

}

bool pawsScrollBar::OnButtonPressed(int button, int keyModifier, pawsWidget* widget)
{
    mouseDown = true;

    scrollTicks = clock->GetCurrentTicks();

    lastButton = button;
    lastModifiers = keyModifier;
    lastWidget = widget;

    switch(widget->GetID())
    {
        case SCROLL_DOWN:
            return ScrollDown();
            break;

        case SCROLL_UP:
            return ScrollUp();
            break;
    }

    return false;
}

bool pawsScrollBar::OnButtonReleased(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
{
    mouseDown = false;
    return true;
}

void pawsScrollBar::MoveThumbToMouse()
{
    psPoint mousePos;
    int relMouseCoord,     // coordinate of mouse cursor relative to the rectangle that constraints thumb movement
        rectBegin,         // coordinate of the constraint rectangle
        rectSize,          // size of the constraint rectangle
        thumbPos;          // coordinate of thumb relative to the constraint rectangle

    int scrollBarSize = GetScrollBarSize();

    mousePos = PawsManager::GetSingleton().GetMouse()->GetPosition();
    if(horizontal)
    {
        rectBegin = screenFrame.xmin + scrollBarSize;
        relMouseCoord = mousePos.x - rectBegin;
    }
    else
    {
        rectBegin = screenFrame.ymin + scrollBarSize;
        relMouseCoord = mousePos.y - rectBegin;
    }

    rectSize = GetThumbScaleLength();

    if(mouseIsDraggingThumb)
        thumbPos = relMouseCoord - thumbDragPoint;
    else
        thumbPos = relMouseCoord - (scrollBarSize - 2*THUMB_MARGIN) / 2;
    thumbPos = csMax(thumbPos, 0);
    thumbPos = csMin(thumbPos, rectSize);

    if(horizontal)
        thumb->MoveTo(rectBegin+thumbPos, thumb->GetScreenFrame().ymin);
    else
        thumb->MoveTo(thumb->GetScreenFrame().xmin, rectBegin+thumbPos);

    currentValue = minValue + (float(thumbPos) / rectSize * (maxValue-minValue));
    if(limited)
        LimitCurrentValue();

    if(parent != NULL)
        parent->OnScroll(SCROLL_THUMB, this);
}

void pawsScrollBar::Draw()
{
    if(mouseIsDraggingThumb  &&  maxValue>0)
        MoveThumbToMouse();

    pawsWidget::Draw();

    if(mouseDown && (clock->GetCurrentTicks() - scrollTicks > SCROLL_TICKS))
    {
        OnButtonPressed(lastButton, lastModifiers, lastWidget);
    }
}

void pawsScrollBar::EnableValueLimit(bool limited)
{
    this->limited = limited;
}

int pawsScrollBar::GetScrollBarSize()
{
    return horizontal ? screenFrame.Height() : screenFrame.Width();
}

void pawsScrollBar::SetButtonLayout()
{
    int scrollBarSize = GetScrollBarSize();
    if(upWidth == 0)
        upWidth = scrollBarSize;
    if(upHeight == 0)
        upHeight = scrollBarSize;
    if(downWidth == 0)
        downWidth = scrollBarSize;
    if(downHeight == 0)
        downHeight = scrollBarSize;

    if(upButton)
        upButton->SetRelativeFrame(upOffsetx, upOffsety, upWidth, upHeight);

    if(downButton)
        downButton->SetRelativeFrame(defaultFrame.Width()-downWidth-downOffsetx, defaultFrame.Height()-downHeight-downOffsety, downWidth, downHeight);
}

int pawsScrollBar::GetThumbScaleLength()
{
    int scrollBarSize = GetScrollBarSize();

    if(horizontal)
        return screenFrame.Width()  - 3*scrollBarSize + 2*THUMB_MARGIN;
    else
        return screenFrame.Height() - 3*scrollBarSize + 2*THUMB_MARGIN;
}

bool pawsScrollBar::ScrollDown()
{
    if(currentValue < maxValue  ||  !limited)
    {
        currentValue += tickValue;
        if(limited)
            LimitCurrentValue();
        SetThumbLayout();
        return parent->OnScroll(SCROLL_DOWN, this);
    }
    return false;
}

bool pawsScrollBar::ScrollUp()
{
    if(currentValue > minValue  ||  !limited)
    {
        currentValue -= tickValue;
        if(limited)
            LimitCurrentValue();
        SetThumbLayout();
        return parent->OnScroll(SCROLL_UP, this);
    }
    return false;
}

