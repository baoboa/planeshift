/*
 * pawsconfigpvp.h - Author: Christian Svensson
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

#ifndef PAWS_CONFIG_CHAT_HEADER
#define PAWS_CONFIG_CHAT_HEADER

// CS INCLUDES
#include <csutil/array.h>
#include <iutil/document.h>

// PAWS INCLUDES
#include "paws/pawswidget.h"
#include "pawsconfigwindow.h"
#include "util/psxmlparser.h"

class pawsChatWindow;
class pawsCheckBox;
class pawsEditTextBox;
class pawsRadioButtonGroup;

class pawsConfigChat : public pawsConfigSectionWindow
{
public:
    pawsConfigChat();

    //from pawsWidget:
    virtual bool PostSetup();
    virtual bool OnChange(pawsWidget * widget);
    
    // from pawsConfigSectionWindow:
    virtual bool Initialize();
    virtual bool LoadConfig();
    virtual bool SaveConfig();
    virtual void SetDefault();
    virtual void Show();
    
protected:
    pawsChatWindow* chatWindow;

    pawsEditTextBox* systemR;
    pawsEditTextBox* systemG;
    pawsEditTextBox* systemB;
    pawsEditTextBox* adminR;
    pawsEditTextBox* adminG;
    pawsEditTextBox* adminB;
    pawsEditTextBox* playerR;
    pawsEditTextBox* playerG;
    pawsEditTextBox* playerB;
    pawsEditTextBox* chatR;
    pawsEditTextBox* chatG;
    pawsEditTextBox* chatB;
    pawsEditTextBox* npcR;
    pawsEditTextBox* npcG;
    pawsEditTextBox* npcB;
    pawsEditTextBox* tellR;
    pawsEditTextBox* tellG;
    pawsEditTextBox* tellB;
    pawsEditTextBox* shoutR;
    pawsEditTextBox* shoutG;
    pawsEditTextBox* shoutB;
    pawsEditTextBox* guildR;
    pawsEditTextBox* guildG;
    pawsEditTextBox* guildB;
    pawsEditTextBox* allianceR;
    pawsEditTextBox* allianceG;
    pawsEditTextBox* allianceB;
    pawsEditTextBox* yourR;
    pawsEditTextBox* yourG;
    pawsEditTextBox* yourB;
    pawsEditTextBox* groupR;
    pawsEditTextBox* groupG;
    pawsEditTextBox* groupB;
    pawsEditTextBox* auctionR;
    pawsEditTextBox* auctionG;
    pawsEditTextBox* auctionB;
    pawsEditTextBox* helpR;
    pawsEditTextBox* helpG;
    pawsEditTextBox* helpB;
    
    pawsCheckBox* loose;
    pawsCheckBox* mouseFocus;
    pawsCheckBox* badwordsIncoming, *badwordsOutgoing;
    pawsRadioButtonGroup* selectTabStyleGroup;
    pawsCheckBox* echoScreenInSystem;
    pawsCheckBox* mainBrackets;
    pawsCheckBox* yourColorMix;
    pawsCheckBox* joinDefaultChannel;
    pawsCheckBox* defaultlastchat;
};


CREATE_PAWS_FACTORY(pawsConfigChat);


#endif 

