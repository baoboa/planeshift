/*
 * psquestprereqopts.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "rpgrules/factions.h"

#include "../gem.h"
#include "../globals.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psquestprereqops.h"
#include "pscharacter.h"
#include "psattack.h"
#include "psquest.h"
#include "weathermanager.h"
#include "cachemanager.h"
#include "psraceinfo.h"
#include "pstrait.h"
#include "client.h"
#include "psitemstats.h"

///////////////////////////////////////////////////////////////////////////////////////////

csString psQuestPrereqOp::GetScript()
{
    csString script;
    script.Append("<pre>");
    script.Append(GetScriptOp());
    script.Append("</pre>");
    return script;
}


///////////////////////////////////////////////////////////////////////////////////////////

void psQuestPrereqOpList::Push(csRef<psQuestPrereqOp> prereqOp)
{
    prereqlist.Push(prereqOp);
}

void psQuestPrereqOpList::Insert(size_t n, csRef<psQuestPrereqOp> prereqOp)
{
    prereqlist.Insert(n,prereqOp);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpAnd::Check(psCharacter* character)
{
    // Check if all prereqs are valid
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        if(!prereqlist[i]->Check(character))
        {
            return false;
        }
    }

    return true;
}

csString psQuestPrereqOpAnd::GetScriptOp()
{
    csString script;
    script.Append("<and>");
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</and>");
    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpAnd::Copy()
{
    csRef<psQuestPrereqOpAnd> copy;
    copy.AttachNew(new psQuestPrereqOpAnd());
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpOr::Check(psCharacter* character)
{
    // Check if any of the prereqs are valid
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        if(prereqlist[i]->Check(character))
        {
            return true;
        }
    }

    return false;
}

csString psQuestPrereqOpOr::GetScriptOp()
{
    csString script;
    script.Append("<or>");
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</or>");
    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpOr::Copy()
{
    csRef<psQuestPrereqOpOr> copy;
    copy.AttachNew(new psQuestPrereqOpOr());
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

psQuestPrereqOpRequire::psQuestPrereqOpRequire(int min_required,int max_required)
{
    min = min_required;
    max = max_required;
}

bool psQuestPrereqOpRequire::Check(psCharacter* character)
{
    // Count the number of prereqs that is valid.
    int count=0;
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        if(prereqlist[i]->Check(character))
        {
            count++;
        }
    }
    // Verify that the appropiate numbers of prereqs was counted.
    return ((min == -1 || count >= min) && (max == -1 || count <= max));
}

csString psQuestPrereqOpRequire::GetScriptOp()
{
    csString script;
    script.Append("<require");
    if(min != -1)
    {
        script.AppendFmt(" min=\"%d\"",min);
    }

    if(max != -1)
    {
        script.AppendFmt(" max=\"%d\"",max);
    }

    script.Append(">");
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</require>");
    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpRequire::Copy()
{
    csRef<psQuestPrereqOpRequire> copy;
    copy.AttachNew(new psQuestPrereqOpRequire(min,max));
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpNot::Check(psCharacter* character)
{
    return (prereqlist.GetSize() && !prereqlist[0]->Check(character));
}

csString psQuestPrereqOpNot::GetScriptOp()
{
    csString script;
    script.Append("<not>");
    if(prereqlist.GetSize())
    {
        script.Append(prereqlist[0]->GetScriptOp());
    }
    script.Append("</not>");
    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpNot::Copy()
{
    csRef<psQuestPrereqOpNot> copy;
    copy.AttachNew(new psQuestPrereqOpNot());
    copy->Push(prereqlist[0]->Copy());
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

psQuestPrereqOpQuestCompleted::psQuestPrereqOpQuestCompleted(csString questName)
{
    quest = NULL;
    name = questName;
}

bool psQuestPrereqOpQuestCompleted::Check(psCharacter* character)
{
    if(quest == NULL)
        quest = psserver->GetCacheManager()->GetQuestByName(name);
    return character->GetQuestMgr().CheckQuestCompleted(quest);
}

csString psQuestPrereqOpQuestCompleted::GetScriptOp()
{
    csString script;

    script.AppendFmt("<completed quest=\"%s\"/>",quest->GetName());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpQuestCompleted::Copy()
{
    csRef<psQuestPrereqOpQuestCompleted> copy;

    if(quest==NULL)
        copy.AttachNew(new psQuestPrereqOpQuestCompleted(name));
    else
        copy.AttachNew(new psQuestPrereqOpQuestCompleted(quest));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpQuestAssigned::Check(psCharacter* character)
{
    return character->GetQuestMgr().CheckQuestAssigned(quest);
}

csString psQuestPrereqOpQuestAssigned::GetScriptOp()
{
    csString script;

    script.AppendFmt("<assigned quest=\"%s\"/>",quest->GetName());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpQuestAssigned::Copy()
{
    csRef<psQuestPrereqOpQuestAssigned> copy;
    copy.AttachNew(new psQuestPrereqOpQuestAssigned(quest));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpQuestCompletedCategory::Check(psCharacter* character)
{
    int count = character->GetQuestMgr().NumberOfQuestsCompleted(category);

    Debug5(LOG_QUESTS, character->GetPID().Unbox(), "Check for category %s in range %d <= %d <= %d",
           category.GetDataSafe(),min,count,max);

    // Verify that the appropiate numbers of quest of given category is done.
    return ((min == -1 || min <= count) && (max == -1 || count <= max));
}

csString psQuestPrereqOpQuestCompletedCategory::GetScriptOp()
{
    csString script;

    script.AppendFmt("<completed category=\"%s\"",category.GetDataSafe());
    if(min != -1)
    {
        script.AppendFmt(" min=\"%d\"",min);
    }

    if(max != -1)
    {
        script.AppendFmt(" max=\"%d\"",max);
    }
    script.Append("/>");

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpQuestCompletedCategory::Copy()
{
    csRef<psQuestPrereqOpQuestCompletedCategory> copy;
    copy.AttachNew(new psQuestPrereqOpQuestCompletedCategory(category,min,max));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpFaction::Check(psCharacter* character)
{
    if(max)
    {
        // If value is max, make sure we're below it
        return !character->CheckFaction(faction,value);
    }
    return character->CheckFaction(faction,value);
}

csString psQuestPrereqOpFaction::GetScriptOp()
{
    csString script;

    script.AppendFmt("<faction name=\"%s\" value=\"%d\" max=\"%d\"/>",faction->name.GetData(),value,max);

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpFaction::Copy()
{
    csRef<psQuestPrereqOpFaction> copy;
    copy.AttachNew(new psQuestPrereqOpFaction(faction,value,max));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpItem::Check(psCharacter* character)  //TODO: extend this
{
    if(!categoryName.IsEmpty())
        return character->Inventory().hasItemCategory(categoryName, true, includeInventory, qualityMin, qualityMax);
    if(!itemName.IsEmpty())
        return character->Inventory().hasItemName(itemName, true, includeInventory, qualityMin, qualityMax);
    return false;
}

csString psQuestPrereqOpItem::GetScriptOp()
{
    csString script;

    script.Format("<item inventory=\"%s\" ", includeInventory? "true" : "false");
    if(!itemName.IsEmpty())
        script.AppendFmt("name=\"%s\" ", itemName.GetData());
    if(!categoryName.IsEmpty())
        script.AppendFmt("category=\"%s\" ", categoryName.GetData());
    if(qualityMin >= 1)
        script.AppendFmt("qualitymin=\"%f\" ", qualityMin);
    if(qualityMax >= 1)
        script.AppendFmt("qualitymax=\"%f\" ", qualityMax);
    script.Append("/>");
    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpItem::Copy()
{
    csRef<psQuestPrereqOpItem> copy;
    copy.AttachNew(new psQuestPrereqOpItem(itemName, categoryName, includeInventory, qualityMin, qualityMax));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpActiveMagic::Check(psCharacter* character)
{
    if(character->GetActor())
    {
        return character->GetActor()->ActiveSpellCount(activeMagic) > 0;
    }
    return false;
}

csString psQuestPrereqOpActiveMagic::GetScriptOp()
{
    csString script;

    script.Format("<activemagic name=\"%s\"/>", activeMagic.GetData());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpActiveMagic::Copy()
{
    csRef<psQuestPrereqOpActiveMagic> copy;
    copy.AttachNew(new psQuestPrereqOpActiveMagic(activeMagic));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpTrait::Check(psCharacter* character)
{
    if(character->GetActor())
    {
        psTrait* traitToCheck = character->GetTraitForLocation(traitLocation);
        if(traitToCheck)
        {
            return traitToCheck->name == traitName;
        }
    }
    return false;
}

csString psQuestPrereqOpTrait::GetScriptOp()
{
    csString script;

    script.Format("<trait name=\"%s\" location=\"%s\"/>", traitName.GetData(), psTrait::locationString[(int)traitLocation]);

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpTrait::Copy()
{
    csRef<psQuestPrereqOpTrait> copy;
    copy.AttachNew(new psQuestPrereqOpTrait(traitName, psTrait::locationString[(int)traitLocation]));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpRace::Check(psCharacter* character)
{
    if(character->GetRaceInfo())
    {
        return (race.CompareNoCase(character->GetRaceInfo()->GetRace()));
    }
    return false;
}

csString psQuestPrereqOpRace::GetScriptOp()
{
    csString script;

    script.Format("<race name=\"%s\"/>", race.GetData());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpRace::Copy()
{
    csRef<psQuestPrereqOpRace> copy;
    copy.AttachNew(new psQuestPrereqOpRace(race));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpGender::Check(psCharacter* character)
{
    if(character->GetRaceInfo())
    {
        return ((csString)character->GetRaceInfo()->GetGender() == gender);
    }
    return false;
}

csString psQuestPrereqOpGender::GetScriptOp()
{
    csString script;

    script.Format("<gender type=\"%s\"/>", gender.GetData());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpGender::Copy()
{
    csRef<psQuestPrereqOpGender> copy;
    copy.AttachNew(new psQuestPrereqOpGender(gender));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpKnownSpell::Check(psCharacter* character)
{
    if(character)
    {
        return (character->GetSpellByName(spell) != NULL);
    }
    return false;
}

csString psQuestPrereqOpKnownSpell::GetScriptOp()
{
    csString script;

    script.Format("<knownspell spell=\"%s\"/>", spell.GetData());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpKnownSpell::Copy()
{
    csRef<psQuestPrereqOpKnownSpell> copy;
    copy.AttachNew(new psQuestPrereqOpKnownSpell(spell));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpGuild::Check(psCharacter* character)
{
    if(!character->GetGuild())  //the player isn't in a guild
    {
        return (guildtype == "none"); //it was what we where looking for?
    }
    else
    {
        //first check the name, if any
        if(guildName.Length() && character->GetGuild()->GetName() != guildName)
            return false;
        if(guildtype == "both") //no need to check for the case it's in a guild
            return true;
        if(character->GetGuild()->IsSecret()) //the guild is secret
            return (guildtype == "secret");
        else //the guild is public
            return (guildtype == "public");
    }
    return false;
}

csString psQuestPrereqOpGuild::GetScriptOp()
{
    csString script;

    script.Format("<guild type=\"%s\"", guildtype.GetData());
    if(guildName.Length())
    {
        script.AppendFmt("name=\"%s\"", guildName.GetData());
    }
    script += "/>";

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpGuild::Copy()
{
    csRef<psQuestPrereqOpGuild> copy;
    copy.AttachNew(new psQuestPrereqOpGuild(guildtype, guildName));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpMarriage::Check(psCharacter* character)
{
    return character->GetIsMarried(); //is the character married?

    return false;
}

csString psQuestPrereqOpMarriage::GetScriptOp()
{
    csString script;

    script.Format("<married/>");

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpMarriage::Copy()
{
    csRef<psQuestPrereqOpMarriage> copy;
    copy.AttachNew(new psQuestPrereqOpMarriage());
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpTimeOnline::Check(psCharacter* character)
{
    if(maxTime)
    {
        // If value is max, make sure we're below it
        return (character->GetTotalOnlineTime() <= maxTime);
    }
    return (character->GetTotalOnlineTime() >= minTime);
}

csString psQuestPrereqOpTimeOnline::GetScriptOp()
{
    csString script;

    script.AppendFmt("<onlinetime ");
    if(minTime)
    {
        script.AppendFmt(" min=\"%d\"", minTime);
    }
    if(maxTime)
    {
        script.AppendFmt(" max=\"%d\"", maxTime);
    }
    script.Append(" />");

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpTimeOnline::Copy()
{
    csRef<psQuestPrereqOpTimeOnline> copy;
    copy.AttachNew(new psQuestPrereqOpTimeOnline(minTime, maxTime));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpAdvisorPoints::Check(psCharacter* character)
{
    if(character->GetActor())
    {
        if(character->GetActor()->GetClient())
        {
            if(type == "min")
                return (character->GetActor()->GetClient()->GetAdvisorPoints() > minPoints);
            if(type == "max")
                return (character->GetActor()->GetClient()->GetAdvisorPoints() < maxPoints);
            if(type == "both")
                return (character->GetActor()->GetClient()->GetAdvisorPoints() > minPoints &&
                        character->GetActor()->GetClient()->GetAdvisorPoints() < maxPoints);
        }
    }
    return false;
}

csString psQuestPrereqOpAdvisorPoints::GetScriptOp()
{
    csString script;

    script.Format("<advisorpoints min=\"%d\" max=\"%d\" type=\"%s\"/>", minPoints, maxPoints, type.GetDataSafe());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpAdvisorPoints::Copy()
{
    csRef<psQuestPrereqOpAdvisorPoints> copy;
    copy.AttachNew(new psQuestPrereqOpAdvisorPoints(minPoints, maxPoints, type));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpTimeOfDay::Check(psCharacter* character)
{
    int currTime = psserver->GetWeatherManager()->GetGameTODHour();

    if(minTime <= maxTime)
    {
        return (currTime <= maxTime) && (currTime >= minTime); // quests during the day
    }

    return (currTime >= maxTime) || (currTime <= minTime); // quests overnight
}

csString psQuestPrereqOpTimeOfDay::GetScriptOp()
{
    csString script;

    script.Format("<timeofday min=\"%d\" max=\"%d\"/>", minTime, maxTime);

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpTimeOfDay::Copy()
{
    csRef<psQuestPrereqOpTimeOfDay> copy;
    copy.AttachNew(new psQuestPrereqOpTimeOfDay(minTime, maxTime));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpVariable::Check(psCharacter* character)
{
    return character->HasVariableDefined(variableName);
}

csString psQuestPrereqOpVariable::GetScriptOp()
{
    csString script;

    script.Format("<variableset name=\"%s\"/>", variableName.GetData());

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpVariable::Copy()
{
    csRef<psQuestPrereqOpVariable> copy;
    copy.AttachNew(new psQuestPrereqOpVariable(variableName));
    return csPtr<psQuestPrereqOp>(copy);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpXor::Check(psCharacter* character)
{
    // Check if any of the prereqs are valid
    bool flag = 0;
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        flag ^= prereqlist[i]->Check(character);
    }

    return flag;
}

csString psQuestPrereqOpXor::GetScriptOp()
{
    csString script;
    script.Append("<xor>");
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        script.Append(prereqlist[i]->GetScriptOp());
    }
    script.Append("</xor>");
    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpXor::Copy()
{
    csRef<psQuestPrereqOpXor> copy;
    copy.AttachNew(new psQuestPrereqOpXor());
    for(size_t i = 0; i < prereqlist.GetSize(); i++)
    {
        copy->Push(prereqlist[i]->Copy());
    }
    return csPtr<psQuestPrereqOp>(copy);
}
///////////////////////////////////////////////////////////////////////////////////////////

bool psQuestPrereqOpSkill::Check(psCharacter* character)
{
    int skill_val;

    //if we allow buffed stats to be taken in consideration we take the
    //current skill else we take the base skill (without buff/debuff)
    if(allowBuffed)
        skill_val = character->GetSkillRank(skill->id).Current();
    else
        skill_val = character->GetSkillRank(skill->id).Base();

    if(max && skill_val > max)
    {
        return false;
    }
    if(min && skill_val < min)
    {
        return false;
    }
    return true;
}

csString psQuestPrereqOpSkill::GetScriptOp()
{
    csString script;

    script.AppendFmt("<skill name=\"%s\"", skill->name.GetData());
    if(min)
    {
        script.AppendFmt(" min=\"%d\"", min);
    }
    if(max)
    {
        script.AppendFmt(" max=\"%d\"", max);
    }

    script.AppendFmt(" allowbuffed=\"%s\" />", allowBuffed ? "true" : "false");

    return script;
}

csPtr<psQuestPrereqOp> psQuestPrereqOpSkill::Copy()
{
    csRef<psQuestPrereqOpSkill> copy;
    copy.AttachNew(new psQuestPrereqOpSkill(skill, min, max, allowBuffed));
    return csPtr<psQuestPrereqOp>(copy);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



bool psPrereqOpAttackType::Check(psCharacter* character)
{
    for (int slot=0; slot<PSCHARACTER_SLOT_BULK1; slot++)
    {
        if (character->Inventory().CanItemAttack((INVENTORY_SLOT_NUMBER) slot))
        {
            return checkWeapon(character,slot);
        }
    }

    return false;
}

bool psPrereqOpAttackType::checkWeapon(psCharacter *character, int slot)
{

    psItem *weapon=character->Inventory().GetEffectiveWeaponInSlot((INVENTORY_SLOT_NUMBER) slot);
    if(!attackType->weapon.IsEmpty())
    {
        if(!(attackType->weapon.CompareNoCase(weapon->GetName())))
        {
            return false;
        }
    }
    if(!checkWType(character,weapon))
        return false;

     return true;
}
bool psPrereqOpAttackType::checkWType(psCharacter* character, psItem* weapon)
{
    if(!attackType->weaponTypes.IsEmpty())
    {
        bool checkFlag = false;
        for(size_t i = 0; i < attackType->weaponTypes.GetSize(); i++)
        {
            if(attackType->weaponTypes[i] == weapon->GetWeaponType())
            {
                checkFlag = true;
            }
        }
        if(checkFlag != true)
            return false;
    }

    return true;
}
csString psPrereqOpAttackType::GetScriptOp()
{
    csString script = "<>";
    return script;
}

csPtr<psQuestPrereqOp> psPrereqOpAttackType::Copy()
{
    psPrereqOpAttackType* copy = new psPrereqOpAttackType(attackType);
    return csPtr<psQuestPrereqOp>(copy);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool psPrereqOpStance::Check(psCharacter * character)
{

    csString charstance = character->GetActor()->GetCombatStance().stance_name;

    //if the required stance matches the current stance return true.

    if(stance == charstance)
        return true;

    return false;
}

csString psPrereqOpStance::GetScriptOp()
{
    csString script;

    script.AppendFmt("<stance name=\"%s\" />", stance.GetData());
   
    return script;
}

csPtr<psQuestPrereqOp> psPrereqOpStance::Copy()
{
    csRef<psPrereqOpStance> copy;
    copy.AttachNew(new psPrereqOpStance(stance));
    return csPtr<psQuestPrereqOp>(copy);
}
