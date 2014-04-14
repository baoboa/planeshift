/*
 * pawsskillwindow.h
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

#ifndef PAWS_PET_STAT_WINDOW_HEADER
#define PAWS_PET_STAT_WINDOW_HEADER

#include "paws/pawswidget.h"
class pawsTextBox;
class pawsListBox;
class pawsListBoxRow;
class pawsMultiLineTextBox;
class pawsProgressBar;
class pawsObjectView;
class psCharAppearance;

#include "net/cmdbase.h"
#include "gui/pawscontrolwindow.h"

/** This handles all the details about how the skill window.
 */
class pawsPetStatWindow : public pawsWidget, public psClientNetSubscriber
{
public:

    pawsPetStatWindow();
    virtual ~pawsPetStatWindow();

    bool PostSetup();
    virtual void HandleMessage(MsgEntry *msg);
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* listbox, int status );
    void Draw();
    virtual void Show();
    virtual void Hide();
    virtual void Close();

    void UpdateStats( GEMClientActor* actor );
    void SetTarget( GEMClientActor* actor );

protected:

    bool SetupDoll();

    void BuySkill();

    void HandleSkillList( csString& skills );
    void HandleSkillDescription( csString& description );
    void SelectSkill(int skill);

    pawsListBox*        statsSkillList, *combatSkillList, *magicSkillList, *knownSpellList, *variousSkillList;
    pawsMultiLineTextBox* combatSkillDescription, *magicSkillDescription, *knownSpellDescription;
    pawsMultiLineTextBox* variousSkillDescription, *statsSkillDescription;
    pawsProgressBar*    hpBar, *manaBar, *pysStaminaBar, *menStaminaBar;
    pawsTextBox        *hpCurrent, *hpTotal,
                       *manaCurrent, *manaTotal,
                       *pysStaminaCurrent, *pysStaminaTotal,
                       *menStaminaCurrent, *menStaminaTotal;

    csArray<pawsListBoxRow*> unsortedSkills; ///< Array keeping the server order of the skills

    bool filter;
    csString skillString;
    csString selectedSkill;

    int hitpointsMax, manaMax, physStaminaMax, menStaminaMax;

    csRef<iDocumentSystem> xml;

    GEMClientActor* target;
    csString targetID;
    psCharAppearance* charApp;
};

CREATE_PAWS_FACTORY( pawsPetStatWindow );


#endif

