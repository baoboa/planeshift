/*
 * pawsguildwindow.h
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

#ifndef PAWS_GUILD_WINDOW_HEADER
#define PAWS_GUILD_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawsnumberpromptwindow.h"
#include "paws/pawsstringpromptwindow.h"
#include "paws/pawscombopromptwindow.h"
#include "gui/pawscontrolwindow.h"
#include "gui/chatwindow.h"

class pawsButton;
class pawsListBox;
class pawsCheckBox;
class pawsComboBox;
class pawsEditTextBox;

typedef struct
{
    int char_id;
    csString name, public_notes, private_notes;
    int points, level;
} guildMemberInfo;

class pawsGuildWindow : public pawsControlledWindow, public psClientNetSubscriber,public iOnNumberEnteredAction,public iOnStringEnteredAction,public iOnItemChosenAction
{
public:

    pawsGuildWindow();
    virtual ~pawsGuildWindow();

    virtual bool PostSetup();
    ///Handles when a button is pressed
    virtual bool OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget);
    virtual bool OnButtonReleased( int mouseButton, int keyModifier, pawsWidget* widget );
    virtual void Show();
    virtual void Hide();

    void HandleMessage( MsgEntry* me );

    bool IsCreatingGuild() { return creatingGuild;}
    void SetCreatingGuild(bool v) { creatingGuild = v;}

    size_t GetLevel( const char* levelName );

    /// Handle popup question window callback
    void OnStringEntered(const char *name,int param,const char *value);
    void OnNumberEntered(const char *name,int param,int value);
    void OnItemChosen(const char *name,int param,int itemNum, const csString & itemText);

private:

    /// Are we creating a guild already? (I.e, don't open the box twice)
    bool creatingGuild;

    /** Level names in listbox of levels are clickable buttons.
      * This method assigns widget IDs and notification target for each of them.
      */
    void SetupLevelNameButtons();

    /** Hides window and sends UNSUBSCRIBE_GUILD_DATA to server */
    void Deactivate();

    void HandleGuildData( csString& openString );
    void HandleLevelData( csString& openString );
    void HandleMemberData( csString& openString );
    void HandleAllianceData( csString& openString );

    guildMemberInfo * FindSelectedMemberInfo();
    guildMemberInfo * FindMemberInfo(const csString & name);
    guildMemberInfo * FindMemberInfo(int char_id);
    int FindMemberInListBox(const csString & name);

    /** Sets value of cell in listbox of guild members */
//    void SetMemberTextCell(int char_id, int colNum, const csString & text);

    void ExtractLevelInfo(csRef<iDocumentNode> levelNode);
    void HideLeaderCheckboxes();

    /** Opens pawsYesNoBox asking, if player really wants to leave his guild */
    void OpenGuildLeaveConfirm();

    /** Sets visibility of widgets in alliance tab according to mode:
      *     0=guild is not in alliance
      *     1=guild is alliance leader
      *     2=guild is alliance member
      */
    void SetAllianceWidgetVisibility(int mode);

    // Tab controls
    pawsWidget *permissionsPanel,*membersPanel,*alliancesPanel,*settingsPanel,*currentPanel;
    pawsButton *permissionsTab,*membersTab,*alliancesTab,*settingsTab,*currentTab;

    pawsTextBox     *guildName;
    pawsTextBox     *allianceName;
    pawsCheckBox    *guildSecret;
    pawsTextBox     *guildWebPage;
    pawsListBox     *levelList;
    pawsListBox     *memberList;
    pawsCheckBox    *onlineOnly;
    pawsCheckBox    *guildNotifications;
    pawsCheckBox    *allianceNotifications;
    pawsTextBox     *memberCount;
    pawsMultilineEditTextBox *motdEdit;
    pawsListBox     *allianceMemberList;

    csRef<iDocumentSystem> xml;

    csArray<guildMemberInfo> members;

    int char_id;            ///< character_id of our player
    int playerLevel;        ///< guild level of our player

    int max_guild_points;   ///< keeps the max amount of guild points allowed in this guild

    /** Stores names of guild levels. Number of the level is equal to the index in array so the 0. array member
      * is empty and should be ignored.
      */
    csArray<csString> levels;

    pawsChatWindow* chatWindow;
};

//--------------------------------------------------------------------------
CREATE_PAWS_FACTORY( pawsGuildWindow );

#endif
