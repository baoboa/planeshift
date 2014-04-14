/*
 * pawsconfigchatbubbles.h - Author: Steven Patrick
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

#ifndef PAWS_CONFIG_CHATBUBBLES_HEADER
#define PAWS_CONFIG_CHATBUBBLES_HEADER

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"

class psChatBubbles;
class pawsCheckBox;
class pawsEditTextBox;
class pawsScrollBar;
class pawsTextBox;
class pawsComboBox;

struct pawsBubbleChatType
{
	int chatType;
	csString type;
	int startLineNo;
	
	pawsCheckBox *enabled;
	pawsTextBox *Text;
	pawsTextBox *Shadow;
	pawsTextBox *Outline;
	pawsTextBox *AlignText;
	
	pawsEditTextBox *TextR;
	pawsEditTextBox *TextG;
	pawsEditTextBox *TextB;
	pawsEditTextBox *ShadowR;
	pawsEditTextBox *ShadowG;
	pawsEditTextBox *ShadowB;	
	pawsEditTextBox *OutlineR;
	pawsEditTextBox *OutlineG;
	pawsEditTextBox *OutlineB;
	
	pawsComboBox *Align;
};

class pawsConfigChatBubbles : public pawsConfigSectionWindow
{
public:
	pawsConfigChatBubbles();
	void drawFrame();

    //from pawsWidget:
	virtual bool PostSetup();
	
    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();
    virtual void Show();	
	
	// from pawsWidget
	bool OnChange(pawsWidget* /*widget*/) { dirty = true; return true; }
	virtual bool OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* /*widget*/)
        {
            dirty = true;
            return true;
        }
	bool OnMouseDown(int button, int modifiers, int x, int y );
	bool OnScroll( int direction, pawsScrollBar* widget );
	virtual void OnListAction( pawsListBox* selected, int status );

private:
	psChatBubbles *chatBubbles;

	pawsCheckBox *allEnabled;
	
	csArray<pawsBubbleChatType> pawsBubbleChatTypes;
	
	pawsTextBox *maxLineLenText;
	pawsEditTextBox *maxLineLen;
	pawsTextBox *shortPhraseCharCountText;
	pawsEditTextBox *shortPhraseCharCount;
	pawsTextBox *longPhraseLineCountText;
	pawsEditTextBox *longPhraseLineCount;
	
	pawsScrollBar *scrollBar;
    pawsCheckBox *mixActionColours;
};

CREATE_PAWS_FACTORY( pawsConfigChatBubbles );
#endif
