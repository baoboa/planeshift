/*
 * Author: Andrew Robberts
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

#include <psconfig.h>

#include "eediterrortoolbox.h"
#include "eeditglobals.h"
#include "eeditreporter.h"

#include "paws/pawsmanager.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawscheckbox.h"

EEditErrorToolbox::EEditErrorToolbox() : scfImplementationType(this)
{
    errorText = 0;
    loadingEffects = false;
}

EEditErrorToolbox::~EEditErrorToolbox()
{
    editApp->SetConfigBool("EEdit.Errors.OnlyEffects", onlyEffectErrors->GetState());
    editApp->SetConfigBool("EEdit.Errors.ShowErrors", showErrors->GetState());
    editApp->SetConfigBool("EEdit.Errors.ShowWarnings", showWarnings->GetState());
    editApp->SetConfigBool("EEdit.Errors.ShowNotifications", showNotifications->GetState());
}

const int BUG_COLOUR[] =     { 255,   0, 255 };
const int ERROR_COLOUR[] =   { 255,   0,   0 };
const int WARNING_COLOUR[] = { 255, 255,   0 };
const int NOTIFY_COLOUR[] =  { 255, 255, 255 };
const int DEBUG_COLOUR[] =   {   0, 255, 255 };

void EEditErrorToolbox::AddError(int severity, const char * msgId, const char * description)
{
    if (errorText == 0)
        return;

    int colour = -1;
    switch (severity)
    {
    case CS_REPORTER_SEVERITY_BUG:
        colour = editApp->GetGraphics2D()->FindRGB(BUG_COLOUR[0], BUG_COLOUR[1], BUG_COLOUR[2]);
        break;
    case CS_REPORTER_SEVERITY_ERROR:
        colour = editApp->GetGraphics2D()->FindRGB(ERROR_COLOUR[0], ERROR_COLOUR[1], ERROR_COLOUR[2]);
        break;
    case CS_REPORTER_SEVERITY_WARNING:
        colour = editApp->GetGraphics2D()->FindRGB(WARNING_COLOUR[0], WARNING_COLOUR[1], WARNING_COLOUR[2]);
        break;
    case CS_REPORTER_SEVERITY_NOTIFY:
        colour = editApp->GetGraphics2D()->FindRGB(NOTIFY_COLOUR[0], NOTIFY_COLOUR[1], NOTIFY_COLOUR[2]);
        break;
    case CS_REPORTER_SEVERITY_DEBUG:
        colour = editApp->GetGraphics2D()->FindRGB(DEBUG_COLOUR[0], DEBUG_COLOUR[1], DEBUG_COLOUR[2]);
        break;
    }
    char msg[1024];
    msg[0] = '\0';
    int msgIndex = 0;
    int index = 0;
    while (description[index] != '\0')
    {
        if (description[index] == '\n')
        {
            msg[msgIndex] = '\0';
            msgIndex = 0;
            errors.Push(EEditError(msg, colour, severity, loadingEffects));
            DisplayError(errors.Top());
        }
        else
            msg[msgIndex++] = description[index];
        ++index; 
    }
    msg[msgIndex] = '\0';
    errors.Push(EEditError(msg, colour, severity, loadingEffects));
    DisplayError(errors.Top());

    Show();
}

void EEditErrorToolbox::SetLoadingEffects(bool isLoadingEffects)
{
    loadingEffects = isLoadingEffects;
}

void EEditErrorToolbox::Clear()
{
    errorText->Clear();
    errors.DeleteAll();
}

void EEditErrorToolbox::ResetFilter()
{
    errorText->Clear();

    for (size_t a=0; a<errors.GetSize(); ++a)
        DisplayError(errors[a]);
}

void EEditErrorToolbox::Update(unsigned int elapsed)
{
}

size_t EEditErrorToolbox::GetType() const
{
    return T_ERROR;
}

const char * EEditErrorToolbox::GetName() const
{
    return "Errors";
}
    
bool EEditErrorToolbox::PostSetup()
{
    errorText         = (pawsMessageTextBox *) FindWidget("errors");               CS_ASSERT(errorText);
    clearButton       = (pawsButton *)         FindWidget("clear");                CS_ASSERT(clearButton);
    hideButton        = (pawsButton *)         FindWidget("hide");                 CS_ASSERT(hideButton);
    onlyEffectErrors  = (pawsCheckBox *)       FindWidget("only_effect_errors");   CS_ASSERT(onlyEffectErrors);
    showErrors        = (pawsCheckBox *)       FindWidget("show_errors");          CS_ASSERT(showErrors);
    showWarnings      = (pawsCheckBox *)       FindWidget("show_warnings");        CS_ASSERT(showWarnings);
    showNotifications = (pawsCheckBox *)       FindWidget("show_notifys");         CS_ASSERT(showNotifications);

    onlyEffectErrors->SetState(editApp->GetConfigBool("EEdit.Errors.OnlyEffects", false));
    showErrors->SetState(editApp->GetConfigBool("EEdit.Errors.ShowErrors", true));
    showWarnings->SetState(editApp->GetConfigBool("EEdit.Errors.ShowWarnings", true));
    showNotifications->SetState(editApp->GetConfigBool("EEdit.Errors.ShowNotifications", true));

    editApp->GetReporter()->SetErrorToolbox(this);

    return true;
}

bool EEditErrorToolbox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget * widget)
{
    if (widget == clearButton)
    {
        Clear();
        return true;
    }
    else if (widget == hideButton)
    {
        Clear();
        Hide();
        return true;
    }
    else if (widget == onlyEffectErrors || widget == showErrors || widget == showWarnings || widget == showNotifications)
    {
        ResetFilter();
        return true;
    }

    return false;
}

void EEditErrorToolbox::Show()
{
    SetRelativeFramePos(editApp->GetGraphics2D()->GetWidth()/2 - GetActualWidth(screenFrame.Width())/2,
                        editApp->GetGraphics2D()->GetHeight()/2 - GetActualHeight(screenFrame.Height())/2);
    pawsWidget::Show();
}

void EEditErrorToolbox::DisplayError(const EEditError & error)
{
    if (onlyEffectErrors->GetState() && !error.isEffectError)
        return;

    if (error.severity == CS_REPORTER_SEVERITY_ERROR && !showErrors->GetState())
        return;

    if (error.severity == CS_REPORTER_SEVERITY_WARNING && !showWarnings->GetState())
        return;

    if (error.severity == CS_REPORTER_SEVERITY_NOTIFY && !showNotifications->GetState())
        return;

    errorText->AddMessage(error.desc, error.colour);
}

