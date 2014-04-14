/*
 * pawsinfowindow.h - Author: Andrew Craig
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
// pawsinfowindow.h: interface for the pawsInfoWindow class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PAWS_INFO_WINDOW_HEADER
#define PAWS_INFO_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "net/cmdbase.h"
#include "gui/pawscontrolwindow.h"
#include "net/message.h"
#include "gui/pawsslot.h"

class pawsTextBox;
class pawsProgressBar;
class pawsScrollBar;
class pawsButton;
class GEMClientObject;
class GEMClientActor;

/** A window to show the player's vitals, the target's name and health, and the time.
 *  It also has the controls for combat stances and spell power.
 */
class pawsInfoWindow : public pawsControlledWindow, public psClientNetSubscriber
{
public:
    virtual ~pawsInfoWindow();

    virtual void Show();
    virtual void Draw();
    bool PostSetup();

    virtual void HandleMessage(MsgEntry *msg);

    virtual bool OnScroll( int direction, pawsScrollBar* widget );
    virtual bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* reporter );

    void SetStanceHighlight(uint stance);
    void UpdateAttkQueue(MsgEntry* me);

private:
    pawsTextBox     *targetName;
    pawsProgressBar *main_hp;
    pawsProgressBar *main_mana;
    pawsProgressBar *main_stamina[2];
    pawsProgressBar *target_hp;
    pawsScrollBar   *kFactor;
    pawsTextBox     *kFactorPct;
    pawsButton      *stanceButton0;
    pawsButton      *stanceButton1;
    pawsButton      *stanceButton2;
    pawsButton      *stanceButton3;
    pawsButton      *stanceButton4;
    pawsButton      *stanceButton5;

    pawsSlot*        attackImage[5];

    pawsTextBox* timeOfDay;
    csString stanceConvert(const uint ID);
    
    uint selectedstance;

    enum baseStances {
        BLOODY = 1,
        AGGRESSIVE,
        NORMAL,
        DEFENSIVE,
        FULLYDEFENSIVE };
};

CREATE_PAWS_FACTORY( pawsInfoWindow );


#endif

