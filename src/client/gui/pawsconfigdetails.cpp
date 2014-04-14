/*
 * pawsconfigdetails.cpp - Author: Ondrej Hurt
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
#include <csutil/xmltiny.h>
#include <csutil/objreg.h>

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawsconfigdetails.h"
#include "paws/pawstextbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"

#include "pscamera.h"
#include "globals.h"

//////////////////////////////////////////////////////////////////////
//
//                        pawsConfigDetails
//
//////////////////////////////////////////////////////////////////////

bool pawsConfigDetails::Initialize()
{
    return LoadFromFile("configdetails.xml");
}

bool pawsConfigDetails::PostSetup()
{
    adaptCheck = dynamic_cast <pawsCheckBox*> (FindWidget("AdaptCheck"));
    if (!adaptCheck) return false;
    adaptCheck->SetState(true);

    adaptConfig = FindWidget("AdaptConfig");
    if (!adaptConfig) return false;
    fixedConfig = FindWidget("FixedConfig");
    if (!fixedConfig) return false;

    distScroll = dynamic_cast <pawsScrollBar*> (FindWidget("DistScroll"));
    if (!distScroll) return false;
    minDistScroll = dynamic_cast <pawsScrollBar*> (FindWidget("MinDistScroll"));
    if (!minDistScroll) return false;
    minFPSscroll = dynamic_cast <pawsScrollBar*> (FindWidget("MinFPSScroll"));
    if (!minFPSscroll) return false;
    maxFPSscroll = dynamic_cast <pawsScrollBar*> (FindWidget("MaxFPSScroll"));
    if (!maxFPSscroll) return false;
    capFPSscroll = dynamic_cast <pawsScrollBar*> (FindWidget("capFPSScroll"));
    if (!capFPSscroll) return false;

//    distScroll->SetMinValue(5);
    distScroll->SetMaxValue(1000);
    distScroll->EnableValueLimit(true);
//    minDistScroll->SetMinValue(5);
    minDistScroll->SetMaxValue(200);
    minDistScroll->EnableValueLimit(true);
//    minFPSscroll->SetMinValue(1);
    minFPSscroll->SetMaxValue(100);
    minFPSscroll->EnableValueLimit(true);
//    maxFPSscroll->SetMinValue(10);
    maxFPSscroll->SetMaxValue(100);
    maxFPSscroll->EnableValueLimit(true);
//    capFPSscroll->SetMinValue(10);
    capFPSscroll->SetMaxValue(200);
    capFPSscroll->EnableValueLimit(true);

    return true;
}

void pawsConfigDetails::ConstraintScrollBars()
{
    if (minFPSscroll->GetCurrentValue() > maxFPSscroll->GetCurrentValue())
        maxFPSscroll->SetCurrentValue(minFPSscroll->GetCurrentValue());
}

bool pawsConfigDetails::OnChange(pawsWidget* widget)
{
    pawsScrollBar* scrollBarWidget;
    pawsEditTextBox* textBoxWidget = dynamic_cast<pawsEditTextBox*> (widget);
    if(!textBoxWidget)
        return false;
    csString textName = textBoxWidget->GetName();
    csString scrollName;

    scrollName = textName.Slice(0, textName.Length()-5);
    scrollName += "Scroll";
    
    scrollBarWidget = dynamic_cast<pawsScrollBar*> (FindWidget(scrollName));
    if (scrollBarWidget)
    {
        //NOTE: the check for outofbounds is done by the scroolbar widget so no need to precheck here
        scrollBarWidget->SetCurrentValue(atoi(textBoxWidget->GetText()));
    }
    return true;
}

bool pawsConfigDetails::OnScroll(int /*scrollDirection*/, pawsScrollBar* widget)
{
    csString valueName, scrollName;
    pawsEditTextBox * value;

    ConstraintScrollBars();

    scrollName = widget->GetName();
    valueName = scrollName.Slice(0, scrollName.Length()-6);
    valueName += "Value";
    value = dynamic_cast<pawsEditTextBox*> (FindWidget(valueName));
    if (value)
    {
        csString num;
        num.Format("%i", (int)widget->GetCurrentValue());
        value->SetText(num);
    }
    dirty = true;
    return true;
}

void pawsConfigDetails::SetModeVisibility()
{
    if (adaptCheck->GetState())
    {
        adaptConfig->Show();
        fixedConfig->Hide();
    }
    else
    {
        adaptConfig->Hide();
        fixedConfig->Show();
    }
}

bool pawsConfigDetails::OnButtonPressed( int button, int keyModifier, pawsWidget* widget )
{
    if (widget == adaptCheck)
    {
        SetModeVisibility();
        dirty = true;
        return true;
    }
    return pawsWidget::OnButtonPressed(button,keyModifier,widget);
}

void pawsConfigDetails::SetScrollValue(const csString & name, int val)
{
    pawsEditTextBox* value = dynamic_cast<pawsEditTextBox*> (FindWidget(name+"Value"));
    if (!value) return;
    pawsScrollBar* scroll = dynamic_cast<pawsScrollBar*> (FindWidget(name+"Scroll"));
    if (!scroll) return;

    csString valStr;
    valStr.Format("%i", val);
    value->SetText(valStr);
    scroll->SetCurrentValue(val);
}

bool pawsConfigDetails::LoadConfig()
{
    psCamera::DistanceCfg cfg;

    psengine->GetPSCamera()->LoadFromFile(false,false);
    cfg = psengine->GetPSCamera()->GetDistanceCfg();
    adaptCheck->SetState(cfg.adaptive);
    SetModeVisibility();
    SetScrollValue("Dist", cfg.dist);
    SetScrollValue("MinFPS", cfg.minFPS);
    SetScrollValue("MaxFPS", cfg.maxFPS);
    SetScrollValue("MinDist", cfg.minDist);
    SetScrollValue("capFPS", psengine->getLimitFPS());

    dirty = false;
    return true;
}

int pawsConfigDetails::GetValue(const csString & name)
{
    pawsEditTextBox* value = dynamic_cast<pawsEditTextBox*> (FindWidget(name));
    if (!value) return 0;

    if (value->GetText() == NULL)
        return 0;

    return atoi(value->GetText());
}

bool pawsConfigDetails::SaveConfig()
{
    psCamera * cam = psengine->GetPSCamera();
    if (adaptCheck->GetState())
        cam->UseAdaptiveDistanceClipping(GetValue("MinFPSValue"), GetValue("MaxFPSValue"), GetValue("MinDistValue"));
    else
        cam->UseFixedDistanceClipping(GetValue("DistValue"));
    if (GetValue("capFPSValue") > 0)// hardcoded check to prevent a crash 
        psengine->setLimitFPS(GetValue("capFPSValue"));

    psCamera::DistanceCfg cfg;
    cfg.adaptive = adaptCheck->GetState();
    cfg.dist = GetValue("DistValue");
    cfg.minFPS = GetValue("MinFPSValue");
    cfg.maxFPS = GetValue("MaxFPSValue");
    cfg.minDist = GetValue("MinDistValue");
    psengine->GetPSCamera()->SetDistanceCfg(cfg);
    psengine->GetPSCamera()->SaveToFile();
    dirty = false;
    return true;
}

void pawsConfigDetails::SetDefault()
{
	psCamera::DistanceCfg cfg;

    psengine->GetPSCamera()->LoadFromFile(true);
    cfg = psengine->GetPSCamera()->GetDistanceCfg();
    adaptCheck->SetState(cfg.adaptive);
    SetModeVisibility();
    SetScrollValue("Dist", cfg.dist);
    SetScrollValue("MinFPS", cfg.minFPS);
    SetScrollValue("MaxFPS", cfg.maxFPS);
    SetScrollValue("MinDist", cfg.minDist);
    SetScrollValue("capFPS", psengine->getLimitFPS());

    dirty = true;

}
