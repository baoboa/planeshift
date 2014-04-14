/*
 * pawsconfigchatfont.h - Author: Joe Lyon
 *
 * Copyright (C) 2014 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_CONFIG_CHATFONTHEADER
#define PAWS_CONFIG_CHATFONTHEADER

// CS INCLUDES
#include <csutil/array.h>
#include <iutil/document.h>

#include <csutil/csstring.h>
#include <csutil/stringarray.h>
#include <csutil/array.h>


// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawscombo.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"
#include "pawsactivemagicwindow.h"
#include "paws/pawscrollbar.h"


class pawsCheckBox;
class pawsScrollBar;
class pawsRadioButtonGroup;

/*
 * class pawsConfigChatFont is options screen for configuration of Chat window font
 */
class pawsConfigChatFont : public pawsConfigSectionWindow
{
public:
    pawsConfigChatFont();

    //from pawsWidget:
    virtual bool PostSetup();
    virtual bool OnScroll(int,pawsScrollBar*);
    virtual bool OnButtonPressed(int, int, pawsWidget*);
    virtual void OnListAction(pawsListBox* selected, int status);
    virtual void Show();
    virtual void Hide();

    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();

    void PickText( const char * fontName, int size );


protected:

    pawsChatWindow* ChatWindow;

    pawsComboBox*          textFont;
    pawsScrollBar*         textSize;
    pawsScrollBar*         textSpacing;

    bool                   loaded;

};


CREATE_PAWS_FACTORY(pawsConfigChatFont);


#endif

