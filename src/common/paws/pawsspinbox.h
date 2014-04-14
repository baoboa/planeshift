/*
 * pawsspinbox.h
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
#ifndef PAWS_SPINBOX_HEADER
#define PAWS_SPINBOX_HEADER


#include "pawstextbox.h"
#include "pawsbutton.h"
#include <csutil/timer.h>


/**
 * \addtogroup common_paws
 * @{ */

#define SPIN_INTERVAL        100
#define SPIN_START_DELAY     500

#define SPIN_STOP   0
#define SPIN_UP     1
#define SPIN_DOWN   2

struct SpinBoxTimerEvent;

/**
 * A combination widget that has up and down arrows and a text label.
 *
 * This widget is defined in an XML as:
 *
 *   \<widget name="nameHere" factory="pawsSpinBox" id="intIDNumber"\>
 *       \<!-- The size of the entire widget including text and button. --\>
 *       \<frame x="75" y="5" width="70" height="30" /\>
 *
 *       \<!-- The text label to use and it's position relative to the button --\>
 *       \<number default="5" min="0" max="100" inc="1" position="right"/\>
 *   \</widget\>
 *
 * Current supported positions are left/right.
 */
class pawsSpinBox : public pawsWidget
{
public:
    pawsSpinBox();
    virtual ~pawsSpinBox();
    pawsSpinBox(const pawsSpinBox &origin);

    virtual bool Setup(iDocumentNode* node);
    virtual bool ManualSetup(csString &value, float Min, float Max, float Inc, csString &pos);
    virtual void SetRange(float Min, float Max, float Inc);

    virtual void SetValue(float value);
    virtual float GetValue();

    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnButtonReleased(int button, int keyModifier, pawsWidget* widget);

    virtual void OnLostFocus();

    virtual bool Perform(iTimerEvent* ev);
    virtual void Spin();

private:
    csRef<iEventTimer>    globalTimer;
    SpinBoxTimerEvent*    timerEvent;

    int spinCounter;
    int spinState;

    pawsButton* upButton;
    pawsButton* downButton;
    pawsEditTextBox* text;

    float max;
    float min;
    float inc;
};
CREATE_PAWS_FACTORY(pawsSpinBox);

/** @} */

#endif
