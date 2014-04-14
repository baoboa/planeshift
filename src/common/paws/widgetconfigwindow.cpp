/*
 * widgetconfigwindow.cpp - Author: Ian Donderwinkel
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


// CS INCLUDES
#include <psconfig.h>

#include "pawswidget.h"
#include "pawscrollbar.h"
#include "pawstextbox.h"
#include "pawsprogressbar.h"
#include "pawsbutton.h"
#include "pawscheckbox.h"
#include "pawsmainwidget.h"
#include "pawswidget.h"
#include "pawsmanager.h"
#include "util/psstring.h"

#include "widgetconfigwindow.h"

//-----------------------------------------------------------------------------
//                            class WidgetConfigWindow
//-----------------------------------------------------------------------------
WidgetConfigWindow::WidgetConfigWindow()
{
    SetAlwaysOnTop(true);
    PawsManager::GetSingleton().SetCurrentFocusedWidget(this);
    configWidget = NULL;
}


/**
 * @brief Handle any events involving one of the scrollbars being "scrolled"
 */
bool WidgetConfigWindow::OnScroll(int /*direction*/, pawsScrollBar* widget)
{
    if(!configWidget)
        return true;

    psString pct;
    if(widget == scrollBarMinAlpha)
    {
        // get current minimum alpha value from scrollbar
        float value = scrollBarMinAlpha->GetCurrentValue();

        // this minimum value must be less or equal to the maximum alpha value
        if(value<=scrollBarMaxAlpha->GetCurrentValue())
        {
            // update the progress bar
            progressBarMinAlpha->SetCurrentValue(value);
            // set the alpha value of the widget to this value
            // to give the user an idea of how this will look
            // DO NOT call draw() here, it is not necessary
            configWidget->SetBackgroundAlpha((int)value);

            // show alpha in %
            pct.Format("%.0f%%",value/2.55f);
            textMinAlphaPct->SetText(pct);

            currentMinAlpha = value;
        }
        else
        {
            // undo scrollbar action
            scrollBarMinAlpha->SetCurrentValue(scrollBarMaxAlpha->GetCurrentValue());
        }
    }
    else if(widget == scrollBarMaxAlpha)
    {
        // update the scroll and progress bar for max alpha
        float value = scrollBarMaxAlpha->GetCurrentValue();

        // max alpha must not be less than min alpha
        if(value>=scrollBarMinAlpha->GetCurrentValue())
        {
            progressBarMaxAlpha->SetCurrentValue(value);
            // If we are fading, then show possible max alpha.
            if(checkboxFade->GetState())
                configWidget->SetBackgroundAlpha((int)value);

            pct.Format("%.0f%%",value/2.55f);
            textMaxAlphaPct->SetText(pct);

            currentMaxAlpha = value;
        }
        else
        {
            scrollBarMaxAlpha->SetCurrentValue(scrollBarMinAlpha->GetCurrentValue());
        }
    }
    else if(widget == scrollBarFadeSpeed)
    {
        // update the scroll and progress bar
        float value = scrollBarFadeSpeed->GetCurrentValue();
        progressBarFadeSpeed->SetCurrentValue(value);
        configWidget->SetFadeSpeed(value);

        pct.Format("%.0f%%",value/0.10f);
        textFadeSpeedPct->SetText(pct);
        currentFadeSpeed = value;
    }

    return true;
}


void WidgetConfigWindow::restoreSettings()
{
    // restore settings
    configWidget->SetMinAlpha((int)oldMinAlpha);
    configWidget->SetMaxAlpha((int)oldMaxAlpha);
    configWidget->SetFadeSpeed(oldFadeSpeed);
    configWidget->SetFade(oldFadeStatus);
    configWidget->SetFontScaling(oldFontStatus);
}

/**
 * @brief Handle all button presses.
 */
bool WidgetConfigWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(widget == buttonOK)
    {
        // store
        configWidget->SetMinAlpha((int)currentMinAlpha);
        configWidget->SetMaxAlpha((int)currentMaxAlpha);

        // close this window
        //PawsManager::GetSingleton().GetMainWidget()->DeleteChild(this);
        Hide();
        return true;
    }
    else if(widget == buttonCancel)
    {
        //just act like if the user closed the window.
        //it will restore the previous settings and hide the widget.
        Close();
        return true;
    }
    else if(widget == checkboxFade)
    {
        // enables/disables fading
        configWidget->SetFade(!configWidget->isFadeEnabled());
        currentFadeStatus = configWidget->isFadeEnabled();
        checkboxFade->SetState(currentFadeStatus);

        checkboxFade->SetText(currentFadeStatus ? "Enabled" : "Disabled");

        return true;
    }
    else if(widget == checkboxFont)
    {
        // enables/disables font scaling
        configWidget->SetFontScaling(!configWidget->isScalingFont());
        currentFontStatus = configWidget->isScalingFont();
        checkboxFont->SetState(currentFontStatus);

        checkboxFont->SetText(currentFontStatus ? "Enabled" : "Disabled");

        return true;
    }
    else if(widget == buttonApply)
    {
        pawsMainWidget* main = PawsManager::GetSingleton().GetMainWidget();
        main->ApplyWindowSettingsOnChildren(configWidget,
                                            (int)currentMinAlpha,
                                            (int)currentMaxAlpha,
                                            currentFadeSpeed,
                                            currentFadeStatus,
                                            currentFontStatus);
        Hide();
        return true;
    }
    return false;
}

/**
 *  @brief Set all initial values for the given widget.
 */
void WidgetConfigWindow::SetConfigurableWidget(pawsWidget* configWidget)
{
    //if it's the same widget we ignore the input we are already open with this.
    if(this->configWidget == configWidget)
        return;
    //if we were working on a different widget we restore the previous widget setting
    //before proceeding.
    else if(this->configWidget != NULL)
        restoreSettings();

    this->configWidget = configWidget;

    // get new values
    currentMinAlpha   = configWidget->GetMinAlpha();
    currentMaxAlpha   = configWidget->GetMaxAlpha();
    currentFadeStatus = configWidget->isFadeEnabled();
    currentFadeSpeed  = configWidget->GetFadeSpeed();
    currentFontStatus = configWidget->isScalingFont();

    // set title "Window Settings (widgetname)"
    psString title;
    title = "Window Settings (";
    title += configWidget->GetName();
    title += ")";
    this->SetTitle(title);

    csString closeButtonName(configWidget->GetName());
    closeButtonName.Append("settings_close");
    if(close_widget)
        close_widget->SetName(closeButtonName);

    // store old values
    oldMinAlpha = currentMinAlpha;
    oldMaxAlpha = currentMaxAlpha;
    oldFadeSpeed  = currentFadeSpeed;
    oldFontStatus = currentFontStatus;
    oldFadeStatus = currentFadeStatus;

    scrollBarMaxAlpha->SetCurrentValue(currentMaxAlpha);
    progressBarMaxAlpha->SetCurrentValue(currentMaxAlpha);

    scrollBarMinAlpha->SetCurrentValue(currentMinAlpha);
    progressBarMinAlpha->SetCurrentValue(currentMinAlpha);

    scrollBarFadeSpeed->SetCurrentValue(currentFadeSpeed);
    progressBarFadeSpeed->SetCurrentValue(currentFadeSpeed);

    psString pct;
    pct.Format("%.0f%%",currentMinAlpha/2.55f);
    textMinAlphaPct->SetText(pct);

    pct.Format("%.0f%%",currentMaxAlpha/2.55f);
    textMaxAlphaPct->SetText(pct);

    pct.Format("%.0f%%",currentFadeSpeed/0.10f);
    textFadeSpeedPct->SetText(pct);

    checkboxFade->SetState(configWidget->isFadeEnabled());
    checkboxFade->SetText(checkboxFade->GetState() ? "Enabled" : "Disabled");

    checkboxFont->SetState(configWidget->isScalingFont());
    checkboxFont->SetText(checkboxFont->GetState() ? "Enabled" : "Disabled");
}


/*
 * @brief Perform some sanity checks, then position the window.
 */
bool WidgetConfigWindow::PostSetup()
{
    if((scrollBarMinAlpha   =(pawsScrollBar*)this->FindWidget("MinAlphaScroll")) == NULL) return false;
    if((progressBarMinAlpha =(pawsProgressBar*)this->FindWidget("MinAlphaProgressBar")) == NULL) return false;
    scrollBarMinAlpha->SetMaxValue(255);
    scrollBarMinAlpha->SetTickValue(5);
    scrollBarMinAlpha->EnableValueLimit(true);
    progressBarMinAlpha->SetTotalValue(255);

    if((scrollBarMaxAlpha   =(pawsScrollBar*)this->FindWidget("MaxAlphaScroll")) == NULL) return false;
    if((progressBarMaxAlpha =(pawsProgressBar*)this->FindWidget("MaxAlphaProgressBar")) == NULL) return false;
    scrollBarMaxAlpha->SetMaxValue(255);
    scrollBarMaxAlpha->SetTickValue(5);
    scrollBarMaxAlpha->EnableValueLimit(true);
    progressBarMaxAlpha->SetTotalValue(255);

    if((scrollBarFadeSpeed      =(pawsScrollBar*)this->FindWidget("FadeSpeedScroll")) == NULL) return false;
    if((progressBarFadeSpeed =(pawsProgressBar*)this->FindWidget("FadeSpeedProgressBar")) == NULL) return false;
    scrollBarFadeSpeed->SetMaxValue(10);
    scrollBarFadeSpeed->SetTickValue(1);
    scrollBarFadeSpeed->EnableValueLimit(true);
    progressBarFadeSpeed->SetTotalValue(10);

    if((buttonOK     =(pawsButton*)this->FindWidget("OKButton")) == NULL) return false;
    if((buttonCancel =(pawsButton*)this->FindWidget("CancelButton")) == NULL) return false;
    if((buttonApply  =(pawsButton*)this->FindWidget("ApplyAllButton")) == NULL) return false;
    if((checkboxFade   =(pawsCheckBox*)this->FindWidget("FadeButton")) == NULL) return false;
    if((checkboxFont   =(pawsCheckBox*)this->FindWidget("FontButton")) == NULL) return false;

    if((textMinAlphaPct   =(pawsTextBox*)this->FindWidget("MinAlphaCurrentPct")) == NULL) return false;
    if((textMaxAlphaPct   =(pawsTextBox*)this->FindWidget("MaxAlphaCurrentPct")) == NULL) return false;
    if((textFadeSpeedPct  =(pawsTextBox*)this->FindWidget("FadeSpeedCurrentPct")) == NULL) return false;

    // place window on current mouse position
    MoveTo((int)PawsManager::GetSingleton().GetMouse()->GetPosition().x,
           (int)PawsManager::GetSingleton().GetMouse()->GetPosition().y);

    return true;
}


