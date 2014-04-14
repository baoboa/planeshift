/*
 * pawsskillwindow.cpp
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

#include <psconfig.h>
#include <imesh/spritecal3d.h>

#include "globals.h"
#include "pscelclient.h"
#include "clientvitals.h"
#include "../charapp.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "util/skillcache.h"
#include "util/strutil.h"
#include "util/psxmlparser.h"

// PAWS INCLUDES
#include "pawsskillwindow.h"
#include "pawschardescription.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsobjectview.h"
#include "paws/pawsprogressbar.h"
#include "gui/pawscontrolwindow.h"

#define BTN_BUY       100
#define BTN_FILTER    101
#define BTN_BUYLVL    103
#define BTN_STATS    1000 ///< Stats button for the tab panel
#define BTN_COMBAT   1001 ///< Combat button for the tab panel
#define BTN_MAGIC    1002 ///< Magic button for the tab panel
#define BTN_JOBS     1003 ///< Jobs button for the tab panel
#define BTN_VARIOUS  1004 ///< Various button for the tab panel
#define BTN_FACTION  1005

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsSkillWindow::pawsSkillWindow()
{
    filter = false;
    train = false;

    hitpointsMax = 0;
    manaMax = 0;
    physStaminaMax = 0;
    menStaminaMax = 0;

    factRequest = false;
    charApp = new psCharAppearance(PawsManager::GetSingleton().GetObjectRegistry());
}

pawsSkillWindow::pawsSkillWindow(const pawsSkillWindow& origin)
{
}

pawsSkillWindow::~pawsSkillWindow()
{
    // Delete cached skill description strings
    csHash<psSkillDescription *>::GlobalIterator i = skillDescriptions.GetIterator();
    while (i.HasNext())
    {
        delete i.Next();
    }
    skillDescriptions.DeleteAll();
    delete charApp;
}

bool pawsSkillWindow::PostSetup()
{
    // Setup the Doll
    if ( !SetupDoll() )
    {
        return false;
    }

    xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());

    skills[CAT_STATS].list =
        static_cast<pawsListBox*>(FindWidget("StatsSkillList"));
    skills[CAT_STATS].description =
        static_cast<pawsMultiLineTextBox*>(FindWidget("StatsDescription"));
    skills[CAT_STATS].indWidget = "StatsIndicator";
    skills[CAT_STATS].tabName = "Stats Button";

    skills[CAT_COMBAT].list =
        static_cast<pawsListBox*>(FindWidget("CombatSkillList"));
    skills[CAT_COMBAT].description =
        static_cast<pawsMultiLineTextBox*>(FindWidget("CombatDescription"));
    skills[CAT_COMBAT].indWidget = "CombatIndicator";
    skills[CAT_COMBAT].tabName = "Combat Button";

    skills[CAT_MAGIC].list =
        static_cast<pawsListBox*>(FindWidget("MagicSkillList"));
    skills[CAT_MAGIC].description =
        static_cast<pawsMultiLineTextBox*>(FindWidget("MagicDescription"));
    skills[CAT_MAGIC].indWidget = "MagicIndicator";
    skills[CAT_MAGIC].tabName = "Magic Button";

    skills[CAT_JOBS].list =
        static_cast<pawsListBox*>(FindWidget("JobsSkillList"));
    skills[CAT_JOBS].description =
        static_cast<pawsMultiLineTextBox*>(FindWidget("JobsDescription"));
    skills[CAT_JOBS].indWidget = "JobsIndicator";
    skills[CAT_JOBS].tabName = "Jobs Button";

    skills[CAT_VARIOUS].list =
        static_cast<pawsListBox*>(FindWidget("VariousSkillList"));
    skills[CAT_VARIOUS].description =
        static_cast<pawsMultiLineTextBox*>(FindWidget("VariousDescription"));
    skills[CAT_VARIOUS].indWidget = "VariousIndicator";
    skills[CAT_VARIOUS].tabName = "Various Button";

    factionList = static_cast<pawsListBox*>(FindWidget("FactionList"));
    factionsDescription = static_cast<pawsMultiLineTextBox*>(FindWidget("FactionDescription"));

    hpBar = dynamic_cast <pawsProgressBar*> (FindWidget("HPBar"));
    manaBar = dynamic_cast <pawsProgressBar*> (FindWidget("ManaBar"));
    pysStaminaBar = dynamic_cast <pawsProgressBar*> (FindWidget("PysStaminaBar"));
    menStaminaBar = dynamic_cast <pawsProgressBar*> (FindWidget("MenStaminaBar"));
    experienceBar = dynamic_cast <pawsProgressBar*> (FindWidget("ExperienceBar"));

    hpFrac = dynamic_cast <pawsTextBox*> (FindWidget("HPFrac"));
    manaFrac = dynamic_cast <pawsTextBox*> (FindWidget("ManaFrac"));
    pysStaminaFrac = dynamic_cast <pawsTextBox*> (FindWidget("PysStaminaFrac"));
    menStaminaFrac = dynamic_cast <pawsTextBox*> (FindWidget("MenStaminaFrac"));
    experiencePerc = dynamic_cast <pawsTextBox*> (FindWidget("ExperiencePerc"));

    if ( !hpBar || !manaBar || !pysStaminaBar || !menStaminaBar || !experienceBar
         || !hpFrac || !manaFrac || !pysStaminaFrac || !menStaminaFrac || !experiencePerc )
    {
        return false;
    }

    hpBar->SetTotalValue(1);
    manaBar->SetTotalValue(1);
    pysStaminaBar->SetTotalValue(1);
    menStaminaBar->SetTotalValue(1);
    experienceBar->SetTotalValue(1);

    currentTab =0;
    previousTab =0;

    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GUISKILL);
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_FACTION_INFO);

    return true;
}

bool pawsSkillWindow::SetupDoll()
{
    pawsObjectView* widget = dynamic_cast<pawsObjectView*>(FindWidget("Doll"));
    GEMClientActor* actor = psengine->GetCelClient()->GetMainPlayer();
    if (!widget || !actor)
    {
        return true; // doll not wanted, not an error
    }

    // Set the doll view
    while(!widget->View(actor->GetFactName()))
    {
        continue;
    }

    CS_ASSERT(widget->GetObject()->GetMeshObject());

    // Set the charApp.
    widget->SetCharApp(charApp);

    // Register this doll for updates
    widget->SetID(actor->GetEID().Unbox());

    csRef<iSpriteCal3DState> spstate = scfQueryInterface<iSpriteCal3DState> (widget->GetObject()->GetMeshObject());
    if (spstate)
    {
        // Setup cal3d to select random 0 velocity anims
        spstate->SetVelocity(0.0,&psengine->GetRandomGen());
    }

    charApp->SetMesh(widget->GetObject());

    charApp->ApplyTraits(actor->traits);
    charApp->ApplyEquipment(actor->equipment);

    widget->EnableMouseControl(true);

    //return (a && e);
    return true;
}

void pawsSkillWindow::HandleFactionMsg(MsgEntry* me)
{
    psFactionMessage factMsg(me);
    csString ratingStr;
    csList<csString> rowEntry;

    if ( factMsg.cmd == psFactionMessage::MSG_FULL_LIST )
    {
        // Flag that we have received our faction information.
        factRequest = true;

        factions.DeleteAll();
        factionList->Clear();

        //Put all factions in the list.
        for ( size_t z = 0; z < factMsg.factionInfo.GetSize(); z++ )
        {
            FactionRating *fact = new FactionRating;
            fact->name = factMsg.factionInfo[z]->faction;
            fact->rating = factMsg.factionInfo[z]->rating;
            factions.Push(fact);

            rowEntry.PushBack(fact->name);
            ratingStr.Format("%d", fact->rating);
            rowEntry.PushBack(ratingStr);
            fact->row = factionList->NewTextBoxRow(rowEntry);
        }
    }
    else if (factRequest)   // ignore MSG_UPDATE if weve not had full list first
    {
        //Put all the faction updates into the gui.
        for ( size_t z = 0; z < factMsg.factionInfo.GetSize(); z++ )
        {
            bool found = false;

            // Find out where that faction is in our list
            size_t idx = 0;
            for (size_t t = 0; t < factions.GetSize(); t++ )
            {
                if ( factions[t]->name == factMsg.factionInfo[z]->faction )
                {
                    factions[t]->rating = factMsg.factionInfo[z]->rating;
                    found = true;
                    idx = t;
                    break;
                }
            }

            // If it was found above then we can just update it.
            if (found)
            {
                pawsListBoxRow* row = factions[idx]->row;
                if (row)
                {
                    pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
                    if (rank == NULL)
                    {
                        return;
                    }

                    ratingStr.Format("%d", factions[idx]->rating);
                    rank->SetText(ratingStr);
                }
            }
            else
            {
                // We don't know about this faction level yet so add it in.
                FactionRating *fact = new FactionRating;
                fact->name = factMsg.factionInfo[z]->faction;
                fact->rating = factMsg.factionInfo[z]->rating;
                factions.Push(fact);

                rowEntry.PushBack(fact->name);
                ratingStr.Format("%d", fact->rating);
                rowEntry.PushBack(ratingStr);
                fact->row = factionList->NewTextBoxRow(rowEntry);
            }
        }
    }
}

void pawsSkillWindow::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_FACTION_INFO:
        {
            if (!train)
                HandleFactionMsg(me);
            break;
        }

        case MSGTYPE_GUISKILL:
        {
            psGUISkillMessage incoming(me);

            switch (incoming.command)
            {
                case psGUISkillMessage::SKILL_LIST:
                {
                    bool flush = (train != incoming.trainingWindow) || incoming.openWindow;
                    train = incoming.trainingWindow;
                    if (train)
                    {
                        factRequest = false;
                        factions.DeleteAll();
                        factionList->Clear();
                    }

                    skillString = "no";
                    if (!IsVisible() && incoming.openWindow)
                    {
                        Show();
                    }
                    skillString = incoming.commandData;
                    skillCache.apply(&incoming.skillCache);

                    if (IsVisible())
                        HandleSkillList(incoming.focusSkill, flush);

                    hitpointsMax = incoming.hitpointsMax;
                    manaMax = incoming.manaMax;
                    physStaminaMax = incoming.physStaminaMax;
                    menStaminaMax = incoming.menStaminaMax;

                    break;
                }

                case psGUISkillMessage::DESCRIPTION:
                {
                    HandleSkillDescription(incoming.commandData);
                    break;
                }
            }
        }
    }
}

void pawsSkillWindow::Close()
{
    Hide();

    skills[CAT_STATS].list->Clear();
    skills[CAT_COMBAT].list->Clear();
    skills[CAT_MAGIC].list->Clear();
    skills[CAT_JOBS].list->Clear();
    skills[CAT_VARIOUS].list->Clear();

    skills[CAT_STATS].unsortedSkills.DeleteAll();
    skills[CAT_COMBAT].unsortedSkills.DeleteAll();
    skills[CAT_MAGIC].unsortedSkills.DeleteAll();
    skills[CAT_JOBS].unsortedSkills.DeleteAll();
    skills[CAT_VARIOUS].unsortedSkills.DeleteAll();

    skillString.Clear();
    selectedSkill.Clear();
}

bool pawsSkillWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {
        case BTN_FILTER:
        {
            filter = !filter;
            HandleSkillList(-1, true);
            return true;
        }

        case BTN_BUY:
        {
            BuySkill();
            return true;
        }

        case BTN_BUYLVL:
        {
            BuyMaxSkill();
            return true;
        }

        case BTN_STATS:
        case BTN_FACTION:
        case BTN_COMBAT:
        case BTN_MAGIC:
        case BTN_JOBS:
        case BTN_VARIOUS:
        {
            previousTab = currentTab; //Keep the selection if we are hitting the same tab where we are.
            currentTab = widget->GetID() - 1000;
            if (previousTab != currentTab)
            {
                selectedSkill.Clear();
            }
            break;
        }

    }
    return false;
}

void pawsSkillWindow::HandleSkillList(int selectedNameId, bool flush)
{
    if (!flush && !skillCache.isModified())
        return;

    if (skillCache.hasRemoved())
    {
        flush = true;
    }
    else if (!flush && filter)
    {
        // Flush if a row is changing from being displayed to
        // not being displayed.
        psSkillCacheIter p = skillCache.iterBegin();
        while (p.HasNext())
        {
            psSkillCacheItem *item = p.Next();
            if (item->isModified() &&
                item->getRank() == 0 &&
                item->getKnowledge() == 0 &&
                item->getPractice() == 0)
            {
                flush = true;
            }
        }
    }

    if (flush)
    {
        // Clear descriptions
        skills[CAT_STATS].description->SetText("");
        skills[CAT_COMBAT].description->SetText("");
        skills[CAT_MAGIC].description->SetText("");
        skills[CAT_JOBS].description->SetText("");
        skills[CAT_VARIOUS].description->SetText("");

        // Clear everything on a flush
        skills[CAT_STATS].list->Clear();
        skills[CAT_COMBAT].list->Clear();
        skills[CAT_MAGIC].list->Clear();
        skills[CAT_JOBS].list->Clear();
        skills[CAT_VARIOUS].list->Clear();

        skills[CAT_STATS].unsortedSkills.DeleteAll();
        skills[CAT_COMBAT].unsortedSkills.DeleteAll();
        skills[CAT_MAGIC].unsortedSkills.DeleteAll();
        skills[CAT_JOBS].unsortedSkills.DeleteAll();
        skills[CAT_VARIOUS].unsortedSkills.DeleteAll();
    }

    x = skillCache.getProgressionPoints();

    // Row index counters into unsortedSkills
    unsigned idx[5] = { 0, 0, 0, 0, 0 };
    psSkillCacheIter p = skillCache.iterBegin();
    while (p.HasNext())
    {
        psSkillCacheItem *skill = p.Next();

        if (skill->getCategory() >= 5)
            continue;

        HandleSkillCategory(skill, idx[skill->getCategory()], flush);
    }
    skillCache.setRemoved(false);
    skillCache.setModified(false);

    if (flush)
    {
        skills[CAT_STATS].list->SetSortedColumn(0);
        skills[CAT_COMBAT].list->SetSortedColumn(0);
        skills[CAT_MAGIC].list->SetSortedColumn(0);
        skills[CAT_JOBS].list->SetSortedColumn(0);
        skills[CAT_VARIOUS].list->SetSortedColumn(0);
    }
}

void pawsSkillWindow::HandleSkillDescription( csString& description )
{
    /* Example message:
    <DESCRIPTION NAME="Climbing" DESC="This skill enable user to climb." />
    */
    csRef<iDocument> descDoc = xml->CreateDocument();

    const char* error = descDoc->Parse( description );
    if ( error )
    {
        Error2( "Error Parsing Skill Description: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = descDoc->GetRoot();
    if (!root)
    {
        Error1("No XML root in Skill Description");
        return;
    }
    csRef<iDocumentNode> desc = root->GetNode("DESCRIPTION");
    if(!desc)
    {
        Error1("No <DESCRIPTION> tag in Skill Description");
        return;
    }

    const char *nameStr = "";
    nameStr = desc->GetAttributeValue("NAME");

    const char* descStr = "";
    descStr = desc->GetAttributeValue("DESC");
    if((!descStr) || (!strcmp(descStr,""))|| (!strcmp(descStr,"(null)")))
    {
        descStr = "No description available";
    }
    int skillCategory = desc->GetAttributeValueAsInt("CAT");

    // Add to the cache
    if (nameStr && strcmp(nameStr, "") && strcmp(nameStr, "(null)"))
    {
        csStringID skillNameId = (csStringID)psengine->FindCommonStringId(nameStr);
        if (skillNameId != csInvalidStringID)
        {
            if (skillDescriptions.Contains(skillNameId))
            {
                psSkillDescription *p = skillDescriptions.Get(skillNameId, NULL);
                if (p)
                {
                    p->Update(skillCategory, descStr);
                }
            }
            else
            {
                psSkillDescription *item = new psSkillDescription(skillCategory, descStr);
                skillDescriptions.Put(skillNameId, item);
            }
        }
    }

    if (skillCategory == CAT_FACTION)
    {
        factionsDescription->SetText(descStr);
    }
    else if ((unsigned)skillCategory < (unsigned)CAT_FACTION)
    {
        skills[skillCategory].description->SetText(descStr);
    }
}

void pawsSkillWindow::OnNumberEntered(const char* /*name*/, int /*param*/, int number)
{
    csString commandData;

    if(number == -1)
        return;

    if (selectedSkill.IsEmpty())
    {
        PawsManager::GetSingleton().CreateWarningBox("You have to select a skill to buy.");
        return;
    }

    commandData.Format("<B NAME=\"%s\" AMOUNT=\"%d\"/>", EscpXML(selectedSkill).GetData(), number);
    psGUISkillMessage outgoing(psGUISkillMessage::BUY_SKILL, commandData);
    outgoing.SendMessage();
}

void pawsSkillWindow::BuyMaxSkill()
{
    csString commandData;
    if (selectedSkill.IsEmpty())
    {
        PawsManager::GetSingleton().CreateWarningBox("You have to select a skill to buy.");
        return;
    }

    csStringID skillId = psengine->FindCommonStringId(selectedSkill);
    if (skillId == csInvalidStringID)
    {
        PawsManager::GetSingleton().CreateWarningBox("You have to select a skill to buy.");
        return;
    }

    psSkillCacheItem* currSkill = skillCache.getItemBySkillId(skillId);
    if(!currSkill)
    {
        return;
    }
    unsigned short possibleTraining = currSkill->getKnowledgeCost() - currSkill->getKnowledge();

    if (skillCache.getProgressionPoints() < possibleTraining)
    {
        possibleTraining = skillCache.getProgressionPoints();
    }
        
    //check for 0 pp
    if(!possibleTraining)
    {
        if(!skillCache.getProgressionPoints())
        {
            PawsManager::GetSingleton().CreateWarningBox("You don't have PP to train.");
        }
        else
        {
            PawsManager::GetSingleton().CreateWarningBox("You can't train this skill anymore.");
        }
        return;
    }

    commandData.Format("<B NAME=\"%s\" AMOUNT=\"%d\"/>", EscpXML(selectedSkill).GetData(), possibleTraining);
    psGUISkillMessage msg(psGUISkillMessage::BUY_SKILL, commandData);
    msg.SendMessage();
}

void pawsSkillWindow::BuySkill()
{
    if (selectedSkill.IsEmpty())
    {
        PawsManager::GetSingleton().CreateWarningBox("You have to select a skill to buy.");
        return;
    }

    csStringID skillId = psengine->FindCommonStringId(selectedSkill);
    if (skillId == csInvalidStringID)
    {
        PawsManager::GetSingleton().CreateWarningBox("You have to select a skill to buy.");
        return;
    }
    psSkillCacheItem* currSkill = skillCache.getItemBySkillId(skillId);
    if(!currSkill)
    {
        PawsManager::GetSingleton().CreateWarningBox("You have to select a skill to buy.");
        return;
    }
    unsigned short possibleTraining = currSkill->getKnowledgeCost() - currSkill->getKnowledge();

    if (skillCache.getProgressionPoints() < possibleTraining)
        possibleTraining = skillCache.getProgressionPoints();
    
    //check for 0 pp
    if(!possibleTraining)
    {
        if(!skillCache.getProgressionPoints())
            PawsManager::GetSingleton().CreateWarningBox("You don't have PP to train.");
        else
            PawsManager::GetSingleton().CreateWarningBox("You can't train this skill anymore.");
        return;
    }

    pawsNumberPromptWindow::Create("Training amount", 0, 1, possibleTraining, this, "Training amount");
}

void pawsSkillWindow::Show()
{
    pawsControlledWindow::Show();

    // Hack set to no when show called because of an incoming skill table.
    if (skillString != "no")
    {
        skillCache.clear(); // Clear the skill cache before the new request
        psGUISkillMessage outgoing(psGUISkillMessage::REQUEST, "");
        outgoing.SendMessage();
    }

    // If this is the first time the window is open then we need to get our
    // full list of faction information.
    if (!train && !factRequest)
    {
        psFactionMessage factionRequest(0, psFactionMessage::MSG_FULL_LIST);
        factionRequest.BuildMsg();
        factionRequest.SendMessage();
    }
}

void pawsSkillWindow::Hide()
{
    psGUISkillMessage outgoing(psGUISkillMessage::QUIT, "");
    outgoing.SendMessage();
    skillCache.clear();
    pawsControlledWindow::Hide();
    train = false;
}


void pawsSkillWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status == LISTBOX_HIGHLIGHTED)
    {
        pawsListBoxRow* row = widget->GetSelectedRow();
        pawsTextBox* skillName = (pawsTextBox*)(row->GetColumn(0));

        selectedSkill.Replace( skillName->GetText() );

        // Try to find cached copy of the description string */
        csStringID skillNameId = (csStringID)psengine->FindCommonStringId(selectedSkill);

        psSkillDescription *desc = NULL;
        if (skillNameId != csInvalidStringID)
        {
            desc = skillDescriptions.Get(skillNameId, NULL);
        }

        if (!desc)
        {
            // Request from the server
            csString commandData;
            commandData.Format("<S NAME=\"%s\" />", EscpXML(selectedSkill).GetData());
            psGUISkillMessage outgoing( psGUISkillMessage::SKILL_SELECTED, commandData);
            outgoing.SendMessage();
        }
        else
        {
            // Use the cached version
            if (desc->GetCategory() == CAT_FACTION)
            {
                factionsDescription->SetText(desc->GetDescription());
            }
            else
            {
                skills[desc->GetCategory()].description->SetText(desc->GetDescription());
            }
        }
    }
}

void pawsSkillWindow::Draw()
{
    psClientVitals* vitals = NULL;
    if (psengine->GetCelClient() && psengine->GetCelClient()->GetMainPlayer() )
    {
        vitals = psengine->GetCelClient()->GetMainPlayer()->GetVitalMgr();
    }

    if (vitals)
    {
        csString buff;
        buff.Format("%1.0f/%i", vitals->GetHP()*hitpointsMax, hitpointsMax);
        hpFrac->SetText(buff);
        buff.Format("%1.0f/%i", vitals->GetMana()*manaMax, manaMax);
        manaFrac->SetText(buff);
        buff.Format("%1.0f/%i", vitals->GetPStamina()*physStaminaMax, physStaminaMax);
        pysStaminaFrac->SetText(buff);
        buff.Format("%1.0f/%i", vitals->GetMStamina()*menStaminaMax, menStaminaMax);
        menStaminaFrac->SetText(buff);
    }

    pawsWidget::Draw();
}

void pawsSkillWindow::FlashTabButton(const char* buttonName, bool flash)
{
    if (buttonName != NULL && FindWidget(buttonName))
    {
        ((pawsButton *) FindWidget(buttonName))->Flash(flash);
    }
}

void pawsSkillWindow::HandleSkillCategory(psSkillCacheItem *skillInfo,
                                          unsigned &idx, bool flush)
{
    int R = skillInfo->getRank();
    int Y = skillInfo->getKnowledge();
    int Z = skillInfo->getPractice();

    // filter out untrained skills
    if (filter && R == 0 && Y == 0 && Z == 0)
    {
        return;
    }

    if (!flush && !skillInfo->isModified())
    {
        // Skip update since there are no changes
        ++idx;
        return;
    }

    // Get the skill name string from common strings
    csString skillName = psengine->FindCommonString(skillInfo->getNameId());
    if (skillName.IsEmpty())
    {
        Error2("Invalid skill name with Id %d", skillInfo->getNameId());
        return;
    }

    unsigned cat = skillInfo->getCategory();
    pawsSkill& skill = skills[cat];

    pawsListBoxRow* row = NULL;
    if (flush)
    {
        row = skill.list->NewRow();
    }
    else
    {
        if (idx >= skill.unsortedSkills.GetSize())
        {
            Error3("Invalid skill index %d (cat %d)", idx, cat);
            return;
        }
        row = skill.unsortedSkills[idx];
    }

    pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
    if (name == NULL)
    {
        return;
    }

    name->SetText( skillName );

    pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
    if (rank == NULL)
    {
        return;
    }
    if (skillInfo->getRank() == skillInfo->getActualStat())
    {
        rank->FormatText("%i", skillInfo->getRank());
    }
    else
    {
        rank->FormatText("%i (%i)", skillInfo->getRank(), skillInfo->getActualStat());
    }

    pawsWidget * indCol = row->GetColumn(2);
    if (indCol == NULL)
    {
        return;
    }

    pawsSkillIndicator* indicator =
        dynamic_cast <pawsSkillIndicator*> (indCol->FindWidget(skill.indWidget));
    if (indicator == NULL)
    {
        return;
    }
    indicator->Set(x, R, Y, skillInfo->getKnowledgeCost(),
                   Z, skillInfo->getPracticeCost());

    if (flush)
    {
        skill.unsortedSkills.Push(row);
    }

    if (skillName == selectedSkill)
    {
        skill.list->Select(row);
    }

    //If we are training, flash the tab button
    FlashTabButton(skill.tabName, train);

    ++idx; // Update the row index
}

/************************************************************************************
*
*                           graphical skill indicator
*
**************************************************************************************/

pawsSkillIndicator::pawsSkillIndicator()
{
    x = 0;
    rank = 0;
    y = 0;
    yCost = 0;
    z = 0;
    zCost = 0;

    g2d = PawsManager::GetSingleton().GetGraphics2D();
}

unsigned int pawsSkillIndicator::GetRelCoord(unsigned int pt)
{
    if (yCost + zCost == 0) // divide-by-zero guard
    {
        return 0;
    }
    else
    {
        int returnv = int(screenFrame.Width() * pt / float(yCost+zCost));
        if(returnv > screenFrame.Width())
        {
            returnv = screenFrame.Width();
        }

        return returnv;
    }
}

void pawsSkillIndicator::DrawSkillProgressBar(int x, int y, int width, int height,
                                int start_r, int start_g, int start_b)
{
    pawsProgressBar::DrawProgressBar(
        csRect(x, y, x+width-1, y+height-1), PawsManager::GetSingleton().GetGraphics3D(), 1,
        start_r, start_g, start_b, 100, 100, 100 );
}

void pawsSkillIndicator::Draw()
{
    csRect sf = screenFrame;

    unsigned int t1;

    if (y < yCost)
    {
        if (GetRelCoord(x) + GetRelCoord(y) < (unsigned int) sf.Width())
        {
            t1 = GetRelCoord(x) + GetRelCoord(y);
        }
        else
        {
            t1 = sf.Width();
        }

        int split = GetRelCoord(yCost);
        DrawSkillProgressBar(sf.xmin, sf.ymin, split, sf.Height(), 180, 180, 30);
        DrawSkillProgressBar(sf.xmin+split, sf.ymin, sf.Width()-split, sf.Height(), 180, 30, 30);
        DrawSkillProgressBar(sf.xmin, sf.ymin, t1, sf.Height()/2, 30, 30, 180);
        DrawSkillProgressBar(sf.xmin, sf.ymin, GetRelCoord(y), sf.Height(), 0, 80, 0);
    }
    else
    {
        int split = GetRelCoord(y+z);
        DrawSkillProgressBar(sf.xmin, sf.ymin, split, sf.Height(), 0, 80, 0);
        DrawSkillProgressBar(sf.xmin+split, sf.ymin, sf.Width()-split, sf.Height(), 180, 30, 30);
    }
    g2d->DrawLine(sf.xmin, sf.ymin, sf.xmax, sf.ymin, 0);
    g2d->DrawLine(sf.xmax, sf.ymin, sf.xmax, sf.ymax, 0);
    g2d->DrawLine(sf.xmin, sf.ymax, sf.xmax, sf.ymax, 0);
    g2d->DrawLine(sf.xmin, sf.ymin, sf.xmin, sf.ymax, 0);
}

void pawsSkillIndicator::Set(unsigned int x, int rank, int y, int yCost, int z, int zCost)
{
    this->x = x;
    this->rank = rank;
    //clamp ycost so if someone overtrained (due to training before changes to the training cost)
    //it won't show a wrong progress bar filled status.
    if(y > yCost)
        this->y = yCost;
    else
        this->y = y;
    this->yCost = yCost;
    this->z = z;
    this->zCost = zCost;
}
