/*
 * pawsconfigtextpage.h - Author: Luca Pancallo
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

#ifndef PAWS_CONFIG_GENERIC_HEADER
#define PAWS_CONFIG_GENERIC_HEADER

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"

class pawsMultiLineTextBox;

/* This class is used as part of the PlaneShift Configuration Window
 * as one panel on the right side. It has a textbox which can be filled by
 * the configwindow.xml file with any text.
 */
class pawsConfigTextPage : public pawsConfigSectionWindow
{
public:
    pawsConfigTextPage();
    void drawFrame();

    // Selects the text to display
    void setTextName(csString* textName);

    //from pawsWidget:
    virtual bool PostSetup();

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();
    virtual void Show();

    // from pawsWidget
    bool OnChange(pawsWidget* /*widget*/)
    {
        dirty = true;
        return true;
    }
    virtual bool OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
    {
        dirty = true;
        return true;
    }
    virtual void OnListAction(pawsListBox* selected, int status);

private:
    pawsMultiLineTextBox* text;
    csString* textName;

};

CREATE_PAWS_FACTORY(pawsConfigTextPage);
#endif
