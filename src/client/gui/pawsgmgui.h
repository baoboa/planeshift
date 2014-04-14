/*
* pawsgmgui.h - Author: Andrew Robberts
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

#ifndef PAWS_GMGUI_WINDOW
#define PAWS_GMGUI_WINDOW
// PS INCLUDES
#include "paws/pawswidget.h"
#include "paws/pawsbutton.h"
#include "paws/pawstextbox.h"
#include "paws/pawscheckbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsstringpromptwindow.h"
#include "net/cmdbase.h"
#include "chatwindow.h"
#include "psclientchar.h"

class pawsGmGUIWindow : public pawsWidget, public psCmdBase, public iOnStringEnteredAction
{
    bool isVisible;
    bool isInvincible;
    bool isViewAllObjects;
    bool isNeverTired;
    bool isQuestTester;
    bool isInfiniteMana;
    bool isFiniteInv;
    bool isSafeFall;
    bool isInstantCast;
    bool isGiveKillExp;
    bool isAttackable;
    bool isBuddyHide;

public:
    pawsGmGUIWindow();

    virtual ~pawsGmGUIWindow();

    /// Handles petition server messages
    void HandleMessage( MsgEntry* message );

    /// Handles commands
    const char* HandleCommand(const char* cmd);

    virtual bool PostSetup();
    virtual void Show();
    virtual void OnListAction( pawsListBox* widget, int status );

    void HideWidget(const char* name);
    void ShowWidget(const char* name);

    /// Handle button clicks
    bool OnButtonReleased(int mouseButton, int keyModifier, pawsWidget* reporter);

    const char* GetSelectedName();
    const char* GetSelectedSector();
    int GetSelectedGender();

    void QueryServer();

    void OnStringEntered(const char *name,int param,const char *value);

    /**
     * Returns the currently selected tab in this window.
     * @return the id of the selected tab (0 players, 1 action locations, 2 gm options
     */
    int GetCurrentTab();

protected:
    pawsChatWindow* chatWindow;
    CmdHandler *cmdsource;

    csRef<iVFS> vfs;

    void SetSecurity();

    void QueryActionLocations();

    void FillPlayerList(psGMGuiMessage& msg);
    void FillActionList(psMapActionMessage& msg);

    pawsListBox* playerList;
    pawsListBox* actionList;

    pawsTextBox* playerCount;

    // Widgets in the Attributes tab
    pawsCheckBox* cbInvincible;
    pawsCheckBox* cbInvisible;
    pawsCheckBox* cbViewAll;
    pawsCheckBox* cbNeverTired;
    pawsCheckBox* cbNoFallDamage;
    pawsCheckBox* cbInfiniteInventory;
    pawsCheckBox* cbQuestTester;
    pawsCheckBox* cbInfiniteMana;
    pawsCheckBox* cbInstantCast;
    pawsCheckBox* cbGiveKillExp;
    pawsCheckBox* cbAttackable;
    pawsCheckBox* cbBuddyHide;

    csString cmdToExectute;
    csString actionXML;
};
CREATE_PAWS_FACTORY( pawsGmGUIWindow );
#endif
