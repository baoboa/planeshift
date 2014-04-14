/*
* pawstabwindow.cpp - Author: Andrew Dai
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
#include "pawstabwindow.h"
#include "pawsmanager.h"

pawsTabWindow::pawsTabWindow(const pawsTabWindow &origin)
    :pawsWidget(origin)
{
    lastButton = (pawsButton*)FindWidget(1000);
    if(!lastButton)
        lastButton = (pawsButton*)FindWidget(1200);
    lastButton->SetState(true);
    activeTab = FindWidget(1100);
    if(!activeTab)
        activeTab = FindWidget(1200);
}

bool pawsTabWindow::PostSetup()
{
    lastButton = (pawsButton*)FindWidget(1000);
    if(!lastButton)
        lastButton = (pawsButton*)FindWidget(1200);
    lastButton->SetState(true);
    activeTab = FindWidget(1100);
    if(!activeTab)
        activeTab = FindWidget(1200);

    // Hide all the panels so we can show the active one after
    for(size_t z = 0; z < children.GetSize(); z++)
    {
        if(strcmp(children[z]->GetType(), "pawsButton") != 0)
            children[z]->Hide();
    }
    activeTab->Show();
    PawsManager::GetSingleton().SetCurrentFocusedWidget(activeTab);
    Resize();
    return true;
}

bool pawsTabWindow::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    pawsButton* buttonWidget = (pawsButton*) widget;
    if(buttonWidget->GetParent() == this && buttonWidget->GetID() < 1200)   // only auto-flip for tabs that are owned by this TabWindow
    {
        pawsWidget* newWidget = FindWidget(buttonWidget->GetID()+100);
        if(newWidget)
        {
            lastButton->SetState(false);
            activeTab->Hide();
            newWidget->Show();
            buttonWidget->SetState(true);
            PawsManager::GetSingleton().SetCurrentFocusedWidget(parent);
            activeTab=newWidget;
            lastButton=buttonWidget;
            parent->OnButtonPressed(mouseButton, keyModifier, widget);
            return true;
        }
        return false;
    }
    if(buttonWidget->GetParent() == this)   // only handle buttons owned by this TabWindow
    {
        lastButton->SetState(false);
        buttonWidget->SetState(true);
        lastButton=buttonWidget;
    }

    // In any case, pass up the chain to the parent of this tab.
    parent->OnButtonPressed(mouseButton, keyModifier, widget);
    return true;
}

void pawsTabWindow::SetTab(int id)
{
    pawsButton* buttonWidget = (pawsButton*) FindWidget(id);
    pawsWidget* newWidget = FindWidget(id+100);
    if(!buttonWidget || !newWidget)
        return;
    lastButton->SetState(false);
    activeTab->Hide();
    newWidget->Show();
    buttonWidget->SetState(true);
    PawsManager::GetSingleton().SetCurrentFocusedWidget(parent);
    activeTab=newWidget;
    lastButton=buttonWidget;
}

void pawsTabWindow::SetTab(const csString &name)
{
    pawsWidget* tab = FindWidget(name);
    if(!tab)
        return;
    if(!dynamic_cast<pawsButton*>(tab))
        SetTab(tab->GetID() - 100);
    else
        SetTab(tab->GetID());
}
