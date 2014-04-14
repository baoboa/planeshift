/*
* pawstabwindow.h - Author: Andrew Dai
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
#ifndef PAWS_TAB_WINDOW
#define PAWS_TAB_WINDOW

#include "pawswidget.h"
#include "pawsbutton.h"

/**
 * \addtogroup common_paws
 * @{ */

/**
 * This window is supposed to be a generic widget for using tabs to show and hide subwindows
 * automatically.
 */
class pawsTabWindow : public pawsWidget
{
public:
    pawsTabWindow()
    {
        factory = "pawsTabWindow";
    }
    pawsTabWindow(const pawsTabWindow &origin);
    virtual bool PostSetup();

    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);

    void SetTab(int id);
    void SetTab(const csString &name);

    pawsWidget* GetActiveTab()
    {
        return activeTab;
    }

    bool FlashButtonFor(pawsWidget* widget)
    {
        if(!widget->IsVisible())
        {
            pawsButton* button = dynamic_cast<pawsButton*>(FindWidget(widget->GetID() - 100));
            if(!button)
                return false;
            button->Flash(true, FLASH_HIGHLIGHT);
            return true;
        }
        return false;
    }

protected:
    pawsWidget* activeTab;
    pawsButton* lastButton;

};

CREATE_PAWS_FACTORY(pawsTabWindow);

/** @} */

#endif
