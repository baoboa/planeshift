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

#ifndef PAWS_SKILL_WINDOW_HEADER
#define PAWS_SKILL_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "net/cmdbase.h"
#include "util/skillcache.h"
#include "gui/pawscontrolwindow.h"
#include "paws/pawsnumberpromptwindow.h"

class pawsTextBox;
class pawsListBox;
class pawsListBoxRow;
class pawsMultiLineTextBox;
class pawsProgressBar;
class pawsObjectView;

/** Describes a skill description inside the GUI system.
 */
class psSkillDescription
{
public:
    psSkillDescription()
    {
        category = 0;
        description.Clear();
    }

    psSkillDescription(int cat, const char *str)
    {
        category = cat;
        description = str;
    }

    void Update(int cat, const char *str)
    {
        category = cat;
        description = str;
    }

    int GetCategory() const
    {
        return category;
    }

    const char *GetDescription() const
    {
        return (const char *)description;
    }

private:
    int category;
    csString description;
};

//-----------------------------------------------------------------------------

/** This handles all the details about how the skill window.
 */
class pawsSkillWindow : public pawsControlledWindow, public psClientNetSubscriber, public iOnNumberEnteredAction
{
public:
    pawsSkillWindow();
    /// TODO: Copy constructor, useless currently. Would be implemented later.
    pawsSkillWindow(const pawsSkillWindow& origin);
    virtual ~pawsSkillWindow();

    bool PostSetup();
    virtual void HandleMessage(MsgEntry *msg);
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    void OnListAction( pawsListBox* listbox, int status );

    virtual void Draw();
    virtual void Show();
    virtual void Hide();
    virtual void Close();

    virtual void OnNumberEntered(const char *name,int param,int number);
protected:

    enum {
        CAT_STATS,
        CAT_COMBAT,
        CAT_MAGIC,
        CAT_JOBS,
        CAT_VARIOUS,
        CAT_FACTION
    };

    struct pawsSkill {
        pawsListBox *list;
        pawsMultiLineTextBox* description;
        const char* indWidget;
        const char* tabName;
        csArray<pawsListBoxRow*> unsortedSkills;
    };

    bool SetupDoll();

    void BuySkill();
    void BuyMaxSkill();

    void HandleSkillList(int selectedNameId, bool flush);
    void HandleSkillDescription( csString& description );

    /** @brief This method is used for making the tab button blinking.
     *
     *  It is used while training: a trainer can train skills in a certain
     *  category, so with the flashing of the tab (the category) the player
     *  knows that a certain skill is available to be trained
     */
    void FlashTabButton(const char* buttonName, bool flash);

    /** This handles the skill list for each category */
    void HandleSkillCategory(psSkillCacheItem* skillInfo, unsigned &idx, bool flush);

    pawsSkill skills[5];
    pawsListBox *factionList;
    pawsMultiLineTextBox *factionsDescription;
    pawsProgressBar *hpBar, *manaBar, *pysStaminaBar, *menStaminaBar, *experienceBar;
    pawsTextBox *hpFrac, *manaFrac, *pysStaminaFrac, *menStaminaFrac, *experiencePerc;

    bool filter, train;
    unsigned int x; ///< Stores topnode "X" information (progression points)

    csString skillString;
    csString selectedSkill;

    int hitpointsMax, manaMax, physStaminaMax, menStaminaMax;

    int currentTab, previousTab; ///< Used for storing which is the current tab and the previous one

    csRef<iDocumentSystem> xml;

    psSkillCache skillCache; ///< Local copy of skills and stats
    csHash<psSkillDescription *> skillDescriptions; ///< Local copy of skill and stat descriptions

    void HandleFactionMsg(MsgEntry* me);

    struct FactionRating
    {
        csString        name;
        int             rating;
        pawsListBoxRow* row;
    };

    csPDelArray<FactionRating>   factions;      ///< The factions by name

    /// Flag if we have sent our initial request for faction information. Only sent
    /// once and everything else is an update.
    bool factRequest;
    psCharAppearance* charApp;
};

CREATE_PAWS_FACTORY( pawsSkillWindow );

//-----------------------------------------------------------------------------

/** pawsSkillIndicator is a widget that graphically displays skill status */

class pawsSkillIndicator : public pawsWidget
{
public:
    pawsSkillIndicator();

    void Draw();
    void Set(unsigned int x, int rank, int y, int yCost, int z, int zCost);
protected:
    /** @brief Calculates relative (to widget) horizontal coordinate of a point on the skill bar */
    unsigned int GetRelCoord(unsigned int pt);

    void DrawSkillProgressBar(int x, int y, int width, int height,
                              int start_r, int start_g, int start_b);

    unsigned int x; ///< progression points
    int rank, y, yCost, z, zCost; ///< Skill status

    iGraphics2D * g2d;
};

CREATE_PAWS_FACTORY( pawsSkillIndicator );

#endif
