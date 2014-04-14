/*
 * pawsconfigtextpage.cpp - Author: Luca Pancallo
 *
 * Copyright (C) 2001-2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//CS INCLUDES
#include <psconfig.h>

//PAWS INCLUDES
#include "pawsconfigtextpage.h"
#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"

//CLIENT INCLUDES
#include "../globals.h"


pawsConfigTextPage::pawsConfigTextPage()
{
    text = NULL;
}

bool pawsConfigTextPage::PostSetup()
{
    text = (pawsMultiLineTextBox*)FindWidget("config generic text");
    if(!text)
    {
        Error1("Could not locate description text widget!");
        return false;
    }

    drawFrame();

    return true;
}

void pawsConfigTextPage::drawFrame()
{
    //Clear the frame by hiding all widgets
    text->Hide();
    text->Show();
}

void pawsConfigTextPage::setTextName(csString* textName)
{
    // show translated text
    text->SetText(PawsManager::GetSingleton().Translate(*textName));

}

bool pawsConfigTextPage::Initialize()
{
    if(! LoadFromFile("configtextpage.xml"))
    {
        return false;
    }
    return true;
}

bool pawsConfigTextPage::LoadConfig()
{
    return true;
}

bool pawsConfigTextPage::SaveConfig()
{
    return true;
}

void pawsConfigTextPage::SetDefault()
{
    LoadConfig();
    SaveConfig();
}

void pawsConfigTextPage::Show()
{
    pawsWidget::Show();
}

void pawsConfigTextPage::OnListAction(pawsListBox* /*selected*/, int /*status*/)
{
    dirty = true;
}

