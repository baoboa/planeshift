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
// pawscrollbar.h: interface for the pawscrollbar class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_SCROLLBAR_HEADER
#define PAWS_SCROLLBAR_HEADER

#include "pawswidget.h"

struct iVirtualClock;
class pawsButton;

/**
 * \addtogroup common_paws
 * @{ */

class pawsThumb : public pawsWidget
{
public:
    pawsThumb()
    {
        factory = "pawsThumb";
    };
    pawsThumb(const pawsThumb &origin): pawsWidget(origin)
    {

    }
    bool OnMouseUp(int button, int modifiers, int x, int y)
    {
        if(parent)
            return parent->OnMouseUp(button, modifiers, x, y);
        else
            return false;
    }

    void OnLostFocus()
    {
        OnMouseUp(0,0,0,0);
        pawsWidget::OnLostFocus();
    }

};
CREATE_PAWS_FACTORY(pawsThumb);
/** A simple scroll bar widget.
 */
class pawsScrollBar : public pawsWidget
{
public:
    pawsScrollBar();
    pawsScrollBar(const pawsScrollBar &origin);
    virtual ~pawsScrollBar();

    virtual bool Setup(iDocumentNode* node);
    virtual bool PostSetup();
    virtual bool OnMouseDown(int button, int modifiers, int x, int y);
    virtual bool OnMouseUp(int button, int modifiers, int x, int y);

    /* This is used to register child button presses.
     * In the scroll bar case it will catch the up and down arrows.
     * @param button The mouse button that was pressed.
     * @param widget The child Widget that was pressed.
     * @return true if the scroll bar handled the button.
     */
    virtual bool OnButtonPressed(int button, int keyModifier, pawsWidget* widget);

    virtual bool OnButtonReleased(int button, int keyModifier, pawsWidget* widget);

    /** Set the max value that this scroll bar should have.
     * @param value The max value that this bar can have.
     */
    virtual void SetMaxValue(float value);
    virtual float GetMaxValue()
    {
        return maxValue;
    }

    /** Set the min value that this scroll bar should have.
     * @param value The min value that this bar can have.
     */
    virtual void SetMinValue(float value);
    virtual float GetMinValue()
    {
        return minValue;
    }

    /** Sets if the constraint of current scrollbar value should be used
     * @param limited Should the value be limited ?
     */
    virtual void EnableValueLimit(bool limited);

    /**
     * Set the current value that this scroll bar should have.
     *
     * @param value The current value of this bar.
     * @param triggerEvent
     * @param publish
     */
    virtual void SetCurrentValue(float value, bool triggerEvent = true, bool publish=true);

    /** Get the current value of the scroll bar.
     * @return The current value.
     */
    virtual float GetCurrentValue();

    /** Set the amount that this scroll bar should change by.
     * @param tick The tick value to change by.
     */
    virtual void SetTickValue(float tick);

    /**
     * Returns the tick value of this scroll bar.
     * @return the tick value of this scroll bar.
     */
    virtual float GetTickValue()
    {
        return tickValue;
    }

    virtual void Draw();

    virtual void SetHorizontal(bool value);

    /**
     * Sets if scrollbar should return reversed scroll value: left/top=maxValue, right/bottom=minValue
     */
    virtual void SetReversed(bool reversed)
    {
        this->reversed = reversed;
    }

    // Scroll bars should not be focused.
    virtual bool OnGainFocus(bool /*notifyParent*/ = true)
    {
        return false;
    }

    virtual void OnResize();

    virtual bool OnMouseExit();

    virtual void OnUpdateData(const char* dataname,PAWSData &data);

    /** Scrolls one step up
     * @return true if the scrollbar was scrolled
     */
    bool ScrollUp();
    /** Scrolls one step down
     * @return true if the scrollbar was scrolled
     */
    bool ScrollDown();
protected:
    /** Sets position and size of thumb on screen according to currentValue */
    void SetThumbLayout();

    /** Sets position of thumb on screen and currentValue according to mouse position */
    void MoveThumbToMouse();

    /** Adjusts currentValue so that 0<=currentValue<=maxValue */
    void LimitCurrentValue();

    int GetScrollBarSize();

    int GetThumbScaleLength();

    /** Sets positions and sizes of up and down buttons */
    void SetButtonLayout();

    void SetThumbVisibility();

    float currentValue;
    float maxValue;
    float minValue;

    /// This is how much value should change on a scroll.
    float tickValue;

    csRef<iVirtualClock> clock;

    /// Keep track of ticks for scrolling
    csTicks scrollTicks;
    bool mouseDown;
    bool reversed;

    int lastButton;
    int lastModifiers;
    pawsWidget* lastWidget;

    pawsButton* upButton, * downButton;

    pawsWidget* thumb;
    bool mouseIsDraggingThumb;  // is the user currently dragging the thumb with mouse ?
    int thumbDragPoint;         // coordinate of the point where mouse button was pressed down
    // when the user began drag'n'drop of thumb (relative to thumb position)

    csString upGrey;
    csString upUnpressed;
    csString upPressed;
    int      upOffsetx;
    int      upOffsety;
    int      upWidth;
    int      upHeight;

    csString downGrey;
    csString downUnpressed;
    csString downPressed;
    int      downOffsetx;
    int      downOffsety;
    int      downWidth;
    int      downHeight;

    csString thumbStopped;
    csString thumbMoving;

    bool horizontal;
    bool limited;
};

//--------------------------------------------------------------------------------
CREATE_PAWS_FACTORY(pawsScrollBar);


/** @} */

#endif
