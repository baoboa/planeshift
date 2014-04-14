/*
 * pawscombopromptwindow.cpp - Author: Ondrej Hurt
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

// COMMON INCLUDES
#include "util/log.h"

// PAWS INCLUDES
#include "pawscombopromptwindow.h"
#include "pawsbutton.h"
#include "pawslistbox.h"
#include "pawsmainwidget.h"





pawsComboPromptWindow::pawsComboPromptWindow()
{
    action = NULL;
    factory = "pawsComboPromptWindow";
}

pawsComboPromptWindow::pawsComboPromptWindow(const pawsComboPromptWindow &origin)
    :pawsPromptWindow(origin),
     action(0),
     name(origin.name),
     param(origin.param)


{
    for(unsigned int i = 0 ; i< origin.children.GetSize(); i++)
    {
        if(origin.wrapper == origin.children[i])
        {
            wrapper = dynamic_cast<ComboWrapper*>(children[i]);
            if(wrapper != 0) break;
        }
    }
}

bool pawsComboPromptWindow::PostSetup()
{
    pawsPromptWindow::PostSetup();

    wrapper = new ComboWrapper(200, 270);
    AddChild(wrapper);
    wrapper->PostSetup();
    inputWidget = wrapper;

    LayoutWindow();
    return true;
}

void pawsComboPromptWindow::Close()
{
    Hide();
    if(action != NULL)
    {
        action->OnItemChosen(name, param, -1, "");
        action = NULL;
    }

    parent->DeleteChild(this);
}

void pawsComboPromptWindow::NewOption(const csString &text)
{
    wrapper->combo->NewOption(text);
}

void pawsComboPromptWindow::Select(int optionNum)
{
    wrapper->combo->Select(optionNum);
}

bool pawsComboPromptWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(action == NULL)
        return false;

    if(widget == okButton)
    {
        action->OnItemChosen(name,
                             param,
                             wrapper->combo->GetSelectedRowNum(),
                             wrapper->combo->GetSelectedRowString());
        action = NULL;
        parent->DeleteChild(this);
        return true;
    }
    else if(widget == cancelButton)
    {
        action->OnItemChosen(name,param,-1, "");
        action = NULL;
        parent->DeleteChild(this);
        return true;
    }
    return false;
}

pawsComboPromptWindow* pawsComboPromptWindow::Create(const csString &label,
        iOnItemChosenAction* action,const char* name, int param)
{
    pawsComboPromptWindow* c = new pawsComboPromptWindow();
    PawsManager::GetSingleton().GetMainWidget()->AddChild(c);
    c->PostSetup();
    c->SetAction(action,name,param);
    c->SetLabel(label);
    c->SetAppropriatePos();
    PawsManager::GetSingleton().SetModalWidget(c);
    return c;
}

