/*
 * pawsPetStatWindow.cpp
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
#include "util/strutil.h"
#include "util/psxmlparser.h"
#include "util/log.h"

// PAWS INCLUDES
#include "pawsskillwindow.h"
#include "pawspetstatwindow.h"
#include "pawschardescription.h"
#include "paws/pawstextbox.h"
#include "paws/pawslistbox.h"
#include "paws/pawsmanager.h"
#include "paws/pawsobjectview.h"
#include "paws/pawsprogressbar.h"
#include "gui/pawscontrolwindow.h"


#define BTN_BUY       100
#define BTN_FILTER    101
//#define BTN_QUIT    102
#define BTN_EDIT      102 ///< Edit description
#define BTN_STATS    1000 ///< Stats button for the tab panel
#define BTN_COMBAT   1001 ///< Combat button for the tab panel
#define BTN_MAGIC    1002 ///< Magic button for the tab panel
#define BTN_JOBS     1003 ///< Jobs button for the tab panel
#define BTN_VARIOUS  1004 ///< Various button for the tab panel

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsPetStatWindow::pawsPetStatWindow()
{
    selectedSkill.Clear();
    skillString.Clear();
    filter = false;
    target = NULL;
    
    hitpointsMax = 0;
    manaMax = 0;
    physStaminaMax = 0;
    menStaminaMax = 0;
    charApp = new psCharAppearance(PawsManager::GetSingleton().GetObjectRegistry());
}

pawsPetStatWindow::~pawsPetStatWindow()
{
   delete charApp;
}

bool pawsPetStatWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_PET_SKILL);

    xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());

    statsSkillList        = (pawsListBox*)FindWidget("StatsSkillList");
    statsSkillDescription = (pawsMultiLineTextBox*)FindWidget("StatsDescription");

    combatSkillList        = (pawsListBox*)FindWidget("CombatSkillList");
    combatSkillDescription = (pawsMultiLineTextBox*)FindWidget("CombatDescription");

    magicSkillList        = (pawsListBox*)FindWidget("MagicSkillList");
    magicSkillDescription = (pawsMultiLineTextBox*)FindWidget("MagicDescription");

    knownSpellList        = (pawsListBox*)FindWidget("KnownSpellList");
    knownSpellDescription = (pawsMultiLineTextBox*)FindWidget("KnownSpellDescription");

    variousSkillList        = (pawsListBox*)FindWidget("VariousSkillList");
    variousSkillDescription = (pawsMultiLineTextBox*)FindWidget("VariousDescription");
    
    hpBar = dynamic_cast <pawsProgressBar*> (FindWidget("HPBar"));
    manaBar = dynamic_cast <pawsProgressBar*> (FindWidget("ManaBar"));
    pysStaminaBar = dynamic_cast <pawsProgressBar*> (FindWidget("PysStaminaBar"));
    menStaminaBar = dynamic_cast <pawsProgressBar*> (FindWidget("MenStaminaBar"));
        
    hpCurrent = dynamic_cast <pawsTextBox*> (FindWidget("HPCurrent"));
    manaCurrent = dynamic_cast <pawsTextBox*> (FindWidget("ManaCurrent"));
    pysStaminaCurrent = dynamic_cast <pawsTextBox*> (FindWidget("PysStaminaCurrent"));
    menStaminaCurrent = dynamic_cast <pawsTextBox*> (FindWidget("MenStaminaCurrent"));
    
    hpTotal = dynamic_cast <pawsTextBox*> (FindWidget("HPTotal"));
    manaTotal = dynamic_cast <pawsTextBox*> (FindWidget("ManaTotal"));
    pysStaminaTotal = dynamic_cast <pawsTextBox*> (FindWidget("PysStaminaTotal"));
    menStaminaTotal = dynamic_cast <pawsTextBox*> (FindWidget("MenStaminaTotal"));

    if ( !statsSkillList || !statsSkillDescription || !combatSkillList 
         ||!combatSkillDescription || !magicSkillList || !magicSkillDescription 
         ||!knownSpellList || !knownSpellDescription || !variousSkillList 
         ||!variousSkillDescription || !hpBar || !manaBar || !pysStaminaBar || !menStaminaBar
         || !hpCurrent || !manaCurrent || !pysStaminaCurrent || !menStaminaCurrent
         || !hpTotal || !manaTotal || !pysStaminaTotal || !menStaminaTotal) { return false; }

    hpBar->SetTotalValue(1);
    manaBar->SetTotalValue(1);
    pysStaminaBar->SetTotalValue(1);
    menStaminaBar->SetTotalValue(1);
    return true;
}


void pawsPetStatWindow::HandleMessage( MsgEntry* me )
{
    psPetSkillMessage incoming(me);
       
    switch (incoming.command)
    {
        case psPetSkillMessage::SKILL_LIST:
        {
            skillString = "no";
//            if (!IsVisible() && incoming.openWindow) 
//                Show();
            skillString = incoming.commandData;
            HandleSkillList(skillString);

            SelectSkill(incoming.focusSkill);
            
            hitpointsMax = incoming.hitpointsMax;
            manaMax = incoming.manaMax;
            physStaminaMax = incoming.physStaminaMax;
            menStaminaMax = incoming.menStaminaMax;
            

            csString text;

            text.Format(": %i", hitpointsMax);
            hpTotal->SetText(text);

            text.Format(": %i", manaMax);
            manaTotal->SetText(text);

            text.Format(": %i", physStaminaMax);
            pysStaminaTotal->SetText(text);

            text.Format(": %i", menStaminaMax);
            menStaminaTotal->SetText(text);
            
            break;
        }
        
        case psPetSkillMessage::DESCRIPTION:
        {
            HandleSkillDescription(incoming.commandData);
            break;
        }
    }
}

void pawsPetStatWindow::Close()
{
    Hide();

    combatSkillList->Clear();
    statsSkillList->Clear();
    magicSkillList->Clear();
    knownSpellList->Clear();
    variousSkillList->Clear();
    skillString.Clear();
    selectedSkill.Clear();
}

bool pawsPetStatWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{       
    switch ( widget->GetID() )
    {
        case BTN_FILTER:
        {
            filter = !filter;
            if (!skillString.IsEmpty()) HandleSkillList( skillString );
            return true;
        }
        
        case BTN_BUY:
        {
            BuySkill();
            return true;
        }

        //case BTN_EDIT:
        //{
        //    pawsCharDescription* descWnd = (pawsCharDescription*)PawsManager::GetSingleton().FindWidget("DescriptionEdit");
        //    descWnd->PostSetup();
        //    descWnd->Show();
        //    return true;
        //}

    }
    return false;
}

void pawsPetStatWindow::SelectSkill(int skill)
{
    if(skill < 0 || skill > (int)unsortedSkills.GetSize())
        return;

    combatSkillList->Select(unsortedSkills[skill]);
}

void pawsPetStatWindow::HandleSkillList( csString& skillString )
{
    combatSkillList->Clear();
    statsSkillList->Clear();
    magicSkillList->Clear();
    knownSpellList->Clear();
    variousSkillList->Clear();

    unsortedSkills.DeleteAll();
    csRef<iDocument> skills = xml->CreateDocument();

    const char* error = skills->Parse( skillString );
    if ( error )
    {
        Error2( "Error Parsing Skill List: %s\n", error );
        return;
    }

    csRef<iDocumentNode> root = skills->GetRoot();
    if (!root)
    {
        Error1("No XML root in Skill List");
            return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("L");
    if(!topNode)
    {
        Error1("No <L> tag in Skill List");
        return;
    }
    unsigned int x = topNode->GetAttributeValueAsInt("X");
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    bool foundSelected = false;

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> skill = iter->Next();

        int R = skill->GetAttributeValueAsInt("R");
        int Y = skill->GetAttributeValueAsInt("Y");
        int Z = skill->GetAttributeValueAsInt("Z");
        //bool stat = skill->GetAttributeValueAsBool("S");
        int category = skill->GetAttributeValueAsInt("CAT");
     
        // filter out untrained skills
        if (filter  &&  R==0  &&  Y==0  &&  Z==0)
            continue;
        switch (category)
        {
        case 0://Stats
            {
                pawsListBoxRow* row = statsSkillList->NewRow();
                
                pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
                if (name == NULL) return;
                csString skillName = skill->GetAttributeValue("NAME");
                name->SetText( skillName );

                pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
                if (rank == NULL) return;
                rank->SetText( skill->GetAttributeValue("R") );

                pawsWidget * indCol = row->GetColumn(2);
                if (indCol == NULL) return;
                pawsSkillIndicator * indicator = dynamic_cast <pawsSkillIndicator*> (indCol->FindWidget("StatsIndicator"));
                if (indicator == NULL) return;
                indicator->Set(x, R, Y, skill->GetAttributeValueAsInt("YC"), Z, skill->GetAttributeValueAsInt("ZC"));

                unsortedSkills.Push(row);
                if (skillName == selectedSkill)
                {
                   statsSkillList->Select(row);
                   foundSelected = true;
                }
                break;
            }
        case 1://Combat skills
            {        
                pawsListBoxRow* row = combatSkillList->NewRow();
       
                pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
                if (name == NULL) return;
                csString skillName = skill->GetAttributeValue("NAME");
                name->SetText( skillName );

                pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
                if (rank == NULL) return;
                rank->SetText( skill->GetAttributeValue("R") );

                pawsWidget * indCol = row->GetColumn(2);
                if (indCol == NULL) return;
                pawsSkillIndicator * indicator = dynamic_cast <pawsSkillIndicator*> (indCol->FindWidget("CombatIndicator"));
                if (indicator == NULL) return;
                indicator->Set(x, R, Y, skill->GetAttributeValueAsInt("YC"), Z, skill->GetAttributeValueAsInt("ZC"));

                unsortedSkills.Push(row);
                if (skillName == selectedSkill)
                {
                    combatSkillList->Select(row);
                    foundSelected = true;
                }
                break;
            }
        case 2://Magic skills
            {
                pawsListBoxRow* row = magicSkillList->NewRow();
       
                pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
                if (name == NULL) return;
                csString skillName = skill->GetAttributeValue("NAME");
                name->SetText( skillName );

                pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
                if (rank == NULL) return;
                rank->SetText( skill->GetAttributeValue("R") );

                pawsWidget * indCol = row->GetColumn(2);
                if (indCol == NULL) return;
                pawsSkillIndicator * indicator = dynamic_cast <pawsSkillIndicator*> (indCol->FindWidget("MagicIndicator"));
                if (indicator == NULL) return;
                indicator->Set(x, R, Y, skill->GetAttributeValueAsInt("YC"), Z, skill->GetAttributeValueAsInt("ZC"));

                unsortedSkills.Push(row);
                if (skillName == selectedSkill)
                {
                    magicSkillList->Select(row);
                    foundSelected = true;
                }
                break;
            }
        case 3://Known Spells
            {
                //pawsListBoxRow* row = knownSpellList->NewRow();
    //   
                //pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
                //if (name == NULL) return;
                //csString spellName = skill->GetAttributeValue("NAME");
                //name->SetText( spellName );

                //pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
                //if (rank == NULL) return;
                //rank->SetText( skill->GetAttributeValue("R") );

                //pawsWidget * indCol = row->GetColumn(2);
                //if (indCol == NULL) return;
                //pawsSkillIndicator * indicator = dynamic_cast <pawsSkillIndicator*> (indCol->FindWidget("JobsIndicator"));
                //if (indicator == NULL) return;
                //indicator->Set(x, R, Y, skill->GetAttributeValueAsInt("YC"), Z, skill->GetAttributeValueAsInt("ZC"));

                //unsortedSkills.Push(row);
                //if (skillName == selectedSkill)
                //{
                //    jobsSkillList->Select(row);
                //    foundSelected = true;
                //}
                //break;
            }
        case 4://Various skills
            {
                pawsListBoxRow* row = variousSkillList->NewRow();
       
                pawsTextBox* name = dynamic_cast <pawsTextBox*> (row->GetColumn(0));
                if (name == NULL) return;
                csString skillName = skill->GetAttributeValue("NAME");
                name->SetText( skillName );

                pawsTextBox* rank = dynamic_cast <pawsTextBox*> (row->GetColumn(1));
                if (rank == NULL) return;
                rank->SetText( skill->GetAttributeValue("R") );

                pawsWidget * indCol = row->GetColumn(2);
                if (indCol == NULL) return;
                pawsSkillIndicator * indicator = dynamic_cast <pawsSkillIndicator*> (indCol->FindWidget("VariousIndicator"));
                if (indicator == NULL) return;
                indicator->Set(x, R, Y, skill->GetAttributeValueAsInt("YC"), Z, skill->GetAttributeValueAsInt("ZC"));

                unsortedSkills.Push(row);
                if (skillName == selectedSkill)
                {
                    variousSkillList->Select(row);
                    foundSelected = true;
                }
                break;
            }
        }
      /*  if(stat)
        {
            combatSkillList->MoveRow(combatSkillList->GetRowCount()-1,stati);
            stati++;
        }*/      
    }

    if (!foundSelected)
    {
        selectedSkill.Clear();       
    }

}

void pawsPetStatWindow::HandleSkillDescription( csString& description )
{   
    /* Example message:
        <DESCRIPTION DESC="This skill enable user to climb." /> 
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

    const char* descStr = "";
    descStr = desc->GetAttributeValue("DESC");
    if((!descStr) || (!strcmp(descStr,""))|| (!strcmp(descStr,"(null)")))
        descStr = "No description available";
    int skillCategory = desc->GetAttributeValueAsInt("CAT");
    
    switch (skillCategory)
    {
    case 0://Stats
        {
             statsSkillDescription->SetText(descStr);
             break;
        }
    case 1://Combat skills
        {
            combatSkillDescription->SetText( descStr );
            break;
        }
    case 2://Magic skills
        {
            magicSkillDescription->SetText(descStr);
            break;
        }
    case 3://Jobs skills
        {
            knownSpellDescription->SetText(descStr);
            break;
        }
    case 4://Various skills
        {
            variousSkillDescription->SetText(descStr);
            break;
        }
    }
}
void pawsPetStatWindow::BuySkill()
{
    csString commandData;
    commandData.Format("<B NAME=\"%s\" />", EscpXML(selectedSkill).GetData());
    psPetSkillMessage outgoing( psPetSkillMessage::BUY_SKILL, commandData);
    outgoing.SendMessage();
}

void pawsPetStatWindow::Show()
{
    pawsWidget::Show();

    // Hack set to no when show called because of an incoming skill table.
    if (skillString != "no")
    {
        psPetSkillMessage msg(psPetSkillMessage::REQUEST, "");
        msg.SendMessage();
    }
}

void pawsPetStatWindow::Hide()
{
    psPetSkillMessage msg(psPetSkillMessage::QUIT, "");
    msg.SendMessage();
    pawsWidget::Hide();
}

void pawsPetStatWindow::OnListAction( pawsListBox* widget, int status )
{
    if (status==LISTBOX_HIGHLIGHTED)
    {
        pawsListBoxRow* row = widget->GetSelectedRow();

        pawsTextBox* skillName = (pawsTextBox*)(row->GetColumn(0));

        selectedSkill.Replace( skillName->GetText() );

        csString commandData;
        commandData.Format("<S NAME=\"%s\" />", EscpXML(selectedSkill).GetData());
        psPetSkillMessage msg(psPetSkillMessage::SKILL_SELECTED, commandData);
        msg.SendMessage();
    }
}

void pawsPetStatWindow::Draw()
{    
    if ( target )
    {
        target->GetVitalMgr()->Predict( csGetTicks(), targetID );
    }
    pawsWidget::Draw();
}

void pawsPetStatWindow::SetTarget( GEMClientActor* actor ) 
{ 
    csString signal;
    target = actor ? actor : NULL;
    if ( target )
    {
        // Setup the Doll
        if ( !SetupDoll() )
            return;

        // Set control Subscriptions
        targetID.Clear();
        targetID.Append(actor->GetEID().Unbox());

        signal.Format("fVitalValue%d:%s", VITAL_HITPOINTS, targetID.GetData());
        PawsManager::GetSingleton().Subscribe(signal, hpBar);
        PawsManager::GetSingleton().Subscribe(signal, hpCurrent);

        signal.Format("fVitalValue%d:%s", VITAL_MANA, targetID.GetData());
        PawsManager::GetSingleton().Subscribe(signal, manaBar);
        PawsManager::GetSingleton().Subscribe(signal, manaCurrent);

        signal.Format("fVitalValue%d:%s", VITAL_PYSSTAMINA, targetID.GetData());
        PawsManager::GetSingleton().Subscribe(signal, pysStaminaBar);
        PawsManager::GetSingleton().Subscribe(signal, pysStaminaCurrent);

        signal.Format("fVitalValue%d:%s", VITAL_MENSTAMINA, targetID.GetData());
        PawsManager::GetSingleton().Subscribe(signal, menStaminaBar);
        PawsManager::GetSingleton().Subscribe(signal, menStaminaCurrent);
    }
}

bool pawsPetStatWindow::SetupDoll()
{
    pawsObjectView* widget = dynamic_cast<pawsObjectView*>(FindWidget("Doll"));
    GEMClientActor* actor = target;
    if (!widget || !actor)
        return false;

    // Set the doll view
    while(!widget->View(actor->GetFactName()))
    {
        continue;
    }

    CS_ASSERT(widget->GetObject()->GetMeshObject());

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

    return true;
}
