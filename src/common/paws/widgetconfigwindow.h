/*
* widgetconfigwindow.h - Author: Ian Donderwinkel
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

#ifndef __WIDGET_CONFIG_WINDOW_
#define __WIDGET_CONFIG_WINDOW_

#include "pawswidget.h"
#include "pawsmanager.h"
#include "pawsmainwidget.h"

class pawsProgressBar;
class pawsButton;
class pawsCheckBox;
class pawsTextBox;
class pawsScrollBar;

/**
 * \addtogroup common_paws
 * @{ */

//-----------------------------------------------------------------------------
//                            class WidgetConfigWindow
//-----------------------------------------------------------------------------
class WidgetConfigWindow : public pawsWidget
{
public:

    WidgetConfigWindow();
    virtual ~WidgetConfigWindow() {};

    bool PostSetup();
    bool OnScroll(int direction, pawsScrollBar* widget);
    bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    /** Restores the settings of the widget being configured to their previous values.
     */
    void restoreSettings();
    /** Overrides the pawsWidget Close() in order to restore settings and null out
     *  the widget being configured.
     */
    void Close()
    {
        restoreSettings();
        configWidget = NULL;
        Hide();
    }

    void SetConfigurableWidget(pawsWidget* w);

private:

    /// old values (used to restore settings when 'cancel' is pressed)
    float    oldMinAlpha;
    float    oldMaxAlpha;
    float    oldFadeSpeed;
    bool     oldFadeStatus;
    bool     oldFontStatus;

    /// current values
    float   currentMinAlpha;
    float   currentMaxAlpha;
    float   currentFadeSpeed;
    bool    currentFadeStatus;
    bool    currentFontStatus;

    /// The scrollbar for adjusting the min Alpha of a window
    pawsScrollBar*        scrollBarMinAlpha;
    pawsProgressBar*    progressBarMinAlpha;

    /// The scrollbar for adjusting the max Alpha of a window
    pawsScrollBar*        scrollBarMaxAlpha;
    pawsProgressBar*    progressBarMaxAlpha;

    /// The scrollbar for adjusting the fadespeed of a window
    pawsScrollBar*        scrollBarFadeSpeed;
    pawsProgressBar*    progressBarFadeSpeed;

    /// OK/Cancel buttons
    pawsButton*            buttonOK;
    pawsButton*            buttonCancel;
    pawsButton*            buttonApply;

    /// Fade and Font checkbox
    pawsCheckBox*            checkboxFade;
    pawsCheckBox*            checkboxFont;

    /// Text boxes
    pawsTextBox*        textFadeStatus;
    pawsTextBox*        textFontStatus;
    pawsTextBox*        textMinAlphaPct;    // min Alpha in %
    pawsTextBox*        textMaxAlphaPct;    // max Alpha in %
    pawsTextBox*        textFadeSpeedPct;   // current fade value in %

    /// pointer to the widget that will have it's settings changed
    pawsWidget*    configWidget;
};

CREATE_PAWS_FACTORY(WidgetConfigWindow);


/** @} */

#endif
