/*
 * psspell.cpp
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

#include "psspell.h"
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csgeom/math.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/log.h"
#include "util/mathscript.h"
#include "util/psdatabase.h"

#include "psserver.h"
#include "gem.h"
#include "client.h"
#include "cachemanager.h"
#include "entitymanager.h"
#include "commandmanager.h"
#include "progressionmanager.h"
#include "npcmanager.h"
#include "../globals.h"
#include "scripting.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psglyph.h"

#define SPELL_TOUCH_RANGE   3.0

psSpell::psSpell()
{
    castDuration = NULL;
    range        = NULL;
    aoeRadius    = NULL;
    aoeAngle     = NULL;
}

psSpell::~psSpell()
{
    if(castDuration)
    {
        delete castDuration;
        castDuration = NULL;
    }
    if(range)
    {
        delete range;
        range = NULL;
    }
    if(aoeRadius)
    {
        delete aoeRadius;
        aoeRadius = NULL;
    }
    if(aoeAngle)
    {
        delete aoeAngle;
        aoeAngle = NULL;
    }
}

bool psSpell::Load(iResultRow &row)
{
    id = row.GetInt("id");

    name = row["name"];

    way = psserver->GetCacheManager()->GetWayByID(row.GetInt("way_id"));

    image         = row["image_name"];
    psserver->GetCacheManager()->AddCommonStringID(image);

    description   = row["spell_description"];
    castingEffect = row["casting_effect"];
    realm         = row.GetInt("realm");
    maxPower      = row.GetFloat("max_power");
    offensive     = row.GetInt("offensive")!=0;
    targetTypes   = row.GetInt("target_type");

    // if spell is defensive then you must be able to cast it on friends, otherwise the entry is wrong.
    csString errorMsg;
    if(!offensive)
    {
        errorMsg.Format("Non-offensive spell '%s' cannot be cast on friends/self/item!", (const char*) name);
        CS_ASSERT_MSG(errorMsg, targetTypes & TARGET_FRIEND || targetTypes & TARGET_SELF || targetTypes & TARGET_ITEM);
    }
    else
    {
        errorMsg.Format("Offensive spell '%s' cannot be cast on enemies!", (const char*) name);
        CS_ASSERT_MSG(errorMsg, targetTypes & TARGET_FOE);
    }

    // Load math expressions
    castDuration = MathExpression::Create(row["cast_duration"]);
    range        = MathExpression::Create(row["range"]);
    aoeRadius    = MathExpression::Create(row["aoe_radius"]);
    aoeAngle     = MathExpression::Create(row["aoe_angle"]);

    outcome = psserver->GetProgressionManager()->FindScript(row["outcome"]);
    CS_ASSERT(castDuration && range && aoeRadius && aoeAngle && outcome);

    // Load NPC perception data
    npcSpellCategory      = row["cstr_npc_spell_category"];
    if(!npcSpellCategory)
    {
        Error2("Invalid 'cstr_npc_spell_category' for spell '%s'\n", name.GetData());
    }
    npcSpellCategoryID    = psserver->GetCacheManager()->FindCommonStringID(npcSpellCategory);
    npcSpellRelativePower = row.GetFloat("npc_spell_power");

    // Load glyph sequence/assembler info
    Result glyphs(db->Select("SELECT * from spell_glyphs WHERE spell_id=%d ORDER BY position ASC",id));
    if(glyphs.IsValid())
    {
        unsigned int i;
        for(i=0; i<glyphs.Count(); i++)
        {
            psItemStats* stats = psserver->GetCacheManager()->GetBasicItemStatsByID(glyphs[i].GetInt("item_id"));
            if(stats)
            {
                glyphList.Push(stats);
            }
            else
            {
                Error3("Can't find item stats for item ID(%d) for Spell %s!", glyphs[i].GetInt("item_id"), name.GetData());
                return false;
            }
        }
    }

    return true;
}

float psSpell::PowerLevel(psCharacter* caster, float kFactor) const
{
    MathEnvironment env;
    env.Define("RelatedStat", caster->GetSkillRank(way->related_stat_skill).Current());
    env.Define("KFactor", kFactor);
    env.Define("WaySkill", caster->GetSkillRank(way->skill).Current());

    (void) psserver->GetCacheManager()->GetSpellPowerLevel()->Evaluate(&env);

    MathVar* power = env.Lookup("PowerLevel");
    CS_ASSERT(power);
    return power->GetValue();
}

psSpellCost psSpell::ManaCost(psCharacter* caster, float kFactor) const
{
    psSpellCost cost;

    if(caster->GetActor()->infinitemana)
    {
        cost.mana = 0;
        cost.stamina = 0;
        return cost;
    }

    MathEnvironment env;
    env.Define("KFactor",     kFactor);
    env.Define("Realm",       realm);
    env.Define("RelatedStat", caster->GetSkillRank(way->related_stat_skill).Current());
    env.Define("WaySkill",    caster->GetSkillRank(way->skill).Current());
    env.Define("Caster", caster);

    (void) psserver->GetCacheManager()->GetManaCost()->Evaluate(&env);

    MathVar* manaCost = env.Lookup("ManaCost");
    MathVar* staminaCost = env.Lookup("StaminaCost");
    CS_ASSERT(manaCost);
    CS_ASSERT(staminaCost);
    cost.mana = manaCost->GetValue();
    cost.stamina = staminaCost->GetValue();
    return cost;
}

float psSpell::ChanceOfCastSuccess(psCharacter* caster, float kFactor) const
{
    MathEnvironment env;
    env.Define("KFactor",     kFactor);
    env.Define("Realm",       realm);
    env.Define("RelatedStat", caster->GetSkillRank(way->related_stat_skill).Current());
    env.Define("WaySkill",    caster->GetSkillRank(way->skill).Current());

    (void) psserver->GetCacheManager()->GetCastSuccess()->Evaluate(&env);

    MathVar* chance = env.Lookup("ChanceOfSuccess");
    CS_ASSERT(chance);
    return chance->GetValue();
}

float psSpell::ChanceOfResearchSuccess(psCharacter* researcher)
{
    if(realm > researcher->GetMaxAllowedRealm(way->skill))
        return 0.0;

    MathEnvironment env;
    env.Define("Spell", this);
    env.Define("WaySkill", way->skill);

    (void) psserver->GetCacheManager()->GetResearchSuccess()->Evaluate(&env);

    MathVar* chance = env.Lookup("ChanceOfSuccess");
    CS_ASSERT(chance);
    return chance->GetValue();
}

bool psSpell::MatchGlyphs(const csArray<psItemStats*> &assembler)
{
    if(assembler.GetSize() != glyphList.GetSize() || glyphList.IsEmpty())
        return false;

    for(size_t i = 0; i < glyphList.GetSize(); i++)
    {
        if(assembler[i] != glyphList[i])
        {
            return false;
        }
    }
    return true;
}

bool psSpell::CanCast(gemActor* caster, float kFactor, csString &reason, bool canCastAllSpells)
{
    psCharacter* casterChar = caster->GetCharacterData();

    const int mode = caster->GetMode();
    if(mode != PSCHARACTER_MODE_PEACE && mode != PSCHARACTER_MODE_COMBAT)
    {
        reason.Format("You can't cast spells while %s.", caster->GetModeStr());
        return false;
    }

    if(caster->IsSpellCasting())
    {
        reason = "You are already casting a spell.";
        return false;
    }

    // Check for sufficient Mana
    psSpellCost cost = ManaCost(casterChar, kFactor);
    if(casterChar->GetMana() < cost.mana)
    {
        reason.Format("You don't have the mana to cast %s.", name.GetData());
        return false;
    }

    if(casterChar->GetStamina(false) < cost.stamina)
    {
        reason.Format("You are too tired to cast %s.", name.GetData());
        return false;
    }

    // Skip testing some conditions for developers and game masters
    if(!canCastAllSpells)
    {
        if(realm > casterChar->GetMaxAllowedRealm(way->skill))
        {
            reason = "You have insufficient knowledge of this magic way to cast this spell.";
            return false;
        }

        if(!casterChar->Inventory().HasPurifiedGlyphs(glyphList))
        {
            reason.Format("You don't have the purified glyphs to cast %s.", name.GetData());
            return false;
        }
    }

    return true;
}

void psSpell::Cast(gemActor* caster, float kFactor, Client* client) const
{
    gemObject* target = caster->GetTargetObject();

    if(client)
    {
        if(offensive)
        {
            csString msg;
            if(!caster->IsAllowedToAttack(target,msg))   // this function sends sys error msg
            {
                psserver->SendSystemError(client->GetClientNum(), msg);
                return;
            }
        }
    }
    else
    {
        // Superclients are allowed to attack everything. (TODO: Fix this?)
    }


    float power = csMin(maxPower, PowerLevel(caster->GetCharacterData(), kFactor));
    float skill = caster->GetCharacterData()->GetSkillRank(way->skill).Current();
    float stat = caster->GetCharacterData()->GetSkillRank(way->related_stat_skill).Current();

    MathEnvironment env;
    env.Define("Power",       power);
    env.Define("WaySkill",    skill);
    env.Define("RelatedStat", stat);
    env.Define("KFactor",     kFactor);
    float max_range = range->Evaluate(&env);
    float castDurationFloat = castDuration->Evaluate(&env);
    csTicks castingDuration = (csTicks) csMax(castDurationFloat, 0.0f);

    if(max_range <= 0)
        target = caster;

    if(!target)
    {
        if(targetTypes & TARGET_SELF)
        {
            target = caster;
        }
        else
        {
            if(client)
            {
                psserver->SendSystemInfo(client->GetClientNum(), "You must select a target for %s", name.GetData());
            }
            else
            {
                Debug3(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s must select a target for %s.",caster->GetName(),name.GetData());
            }

            return;
        }
    }

    // Check for the right kind of target
    const int targetType = caster->GetTargetType(target);
    if(!(targetTypes & targetType))
    {
        csString allowedTypes;
        Client::GetTargetTypeName(targetTypes, allowedTypes);
        if(client)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "You cannot cast %s on %s. You can only cast it on %s.",
                                     name.GetData(), target ? target->GetName() : "that", allowedTypes.GetData());
        }
        else
        {
            csString targetTypeName;
            Client::GetTargetTypeName(targetType, targetTypeName);

            Debug6(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s  cannot cast %s on %s. You can only cast it on %s. Target is; %s",
                   caster->GetName(),name.GetData(), target ? target->GetName() : "that", allowedTypes.GetData(),targetTypeName.GetDataSafe());
        }

        return;
    }

    if(max_range > 0 && caster->RangeTo(target) > max_range)
    {
        if(client)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "%s is too far away to cast %s.", target->GetName(), name.GetData());
        }
        else
        {
            Debug4(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s: %s is too far away to cast %s.",caster->GetName(),target->GetName(), name.GetData());
        }

        return;
    }

    // All conditions for casting this spell are met!
    caster->SetMode(PSCHARACTER_MODE_SPELL_CASTING, castingDuration);
    if(client)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You start casting the spell %s", name.GetData());
    }
    else
    {
        Debug3(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s start casting the spell %s.",caster->GetName(), name.GetData());
    }


    // Allow developers and game masters to cast a spell immediately
    if(caster->instantcast)
    {
        castingDuration = 0;
    }

    if(!castingEffect.IsEmpty() && (castingDuration > 0 || caster->instantcast))
    {
        psEffectMessage fx(0, castingEffect, csVector3(0,0,0), caster->GetEID(), target->GetEID(), castingDuration, 0, 0.0f, 0.0f, 0.0f, 0.0f);
        fx.Multicast(caster->GetMulticastClients(), 0, PROX_LIST_ANY_RANGE);
    }

    psSpellCastGameEvent* evt = new psSpellCastGameEvent(this, caster, target, castingDuration, max_range, kFactor, power, client);
    evt->QueueEvent();
}

void psSpell::Affect(gemActor* caster, gemObject* target, float range, float kFactor, float power, Client* client, csTicks castingDuration) const
{
    const float chanceOfSuccess = ChanceOfCastSuccess(caster->GetCharacterData(), kFactor);
    float roll = psserver->GetRandom() * 100.f;
    Notify5(LOG_SPELLS, "%s casting %s with a chance of success = %.2f and roll = %.2f", caster->GetName(), name.GetData(), chanceOfSuccess, roll);

    psSpellCost cost = ManaCost(caster->GetCharacterData(), kFactor);

    if(chanceOfSuccess < 100.f && roll > chanceOfSuccess)
    {
        // Spell casting failed
        if(client)
        {
            psserver->SendSystemInfo(caster->GetClientID(), "You failed to cast the spell %s." , name.GetData());
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s faild to cast the spell %s.", caster->GetName(), name.GetData());
        }


        ProgressionScript* failure = psserver->GetProgressionManager()->FindScript("SpellFailure");
        MathEnvironment env;
        env.Define("Caster",        caster);
        env.Define("KFactor",       kFactor);
        env.Define("Power",         power);
        // this const cast is safe as scripts cannot change the spell via the scripting interface
        env.Define("Spell",         const_cast<psSpell*>(this));
        env.Define("Roll",          roll);

        failure->Run(&env);
        return;
    }

    // Drain full mana amount.
    caster->GetCharacterData()->AdjustMana(-cost.mana);
    caster->GetCharacterData()->AdjustStamina(-cost.stamina, false);
    caster->SendGroupStats();

    // Look for targets
    MathEnvironment env;
    env.Define("Power",       power);
    env.Define("WaySkill",    caster->GetCharacterData()->GetSkillRank(way->skill).Current());
    env.Define("RelatedStat", caster->GetCharacterData()->GetSkillRank(way->related_stat_skill).Current());
    float radius = aoeRadius->Evaluate(&env);
    float angle  = aoeAngle->Evaluate(&env);

    int affectedCount = 0;
    if(radius < 0.01f)  // single target
    {
        if(target && caster->RangeTo(target) <= range)
        {
            if(AffectTarget(caster, target, target, power, client))
            {
                affectedCount = 1;
            }
        }
        else
        {
            if(client)
            {
                psserver->SendSystemInfo(caster->GetClientID(), "%s is too far away for your spell to reach.", target ? target->GetName() : "Your target");
            }
            else
            {
                Debug3(LOG_SUPERCLIENT,caster->GetEID().Unbox(), "%s is too far away for %s to reach.",target ? target->GetName() : "Your target",caster->GetName());
            }


        }
    }
    else // AOE (Area of Effect)
    {
        csVector3 attackerPos;
        csVector3 targetPos;
        iSector* targetSector;
        InstanceID targetInstance = target->GetInstance();

        attackerPos = caster->GetPosition();
        target->GetPosition(targetPos, targetSector);

        // directional vector for a line from attacker to original target
        csVector3 attackerToTarget;
        attackerToTarget = targetPos - attackerPos;

        if(angle <= SMALL_EPSILON || angle > 360)
            angle = 360;

        angle = (angle/2)*(PI/180); // convert degrees to radians
        // divided by two so it's equally on both sides
        //CPrintf(CON_DEBUG, "Spell has an effect arc of %1.2f radians to either side of LOS.\n", angle);

        csArray<gemObject*> nearby = psserver->entitymanager->GetGEM()->FindNearbyEntities(targetSector, targetPos, targetInstance, radius);
        for(size_t i = 0; i < nearby.GetSize(); i++)
        {
            if(!(targetTypes & caster->GetTargetType(nearby[i])))
                continue;

            if(angle < PI)
            {
                csVector3 attackerToAffected = nearby[i]->GetPosition();

                // obtain a directional line for the vector from attacker to affacted target
                // note that this line does not originate at the original target because the
                // cone that shall include the hittable area shall originate at the attacker
                attackerToAffected -= attackerPos;

                // Angle between the line original target->attacker and original target->affected target
                float cosATAngle = attackerToAffected*attackerToTarget
                                   /(attackerToAffected.Norm()*attackerToTarget.Norm());

                // cap the value to meaningful ones to account for rounding issues
                if(cosATAngle > 1.0f || CS::IsNaN(cosATAngle))
                    cosATAngle = 1.0f;
                if(cosATAngle < -1.0f)
                    cosATAngle = -1.0f;

                // use the absolute value of the angle here to account for both sides equally - see above
                if(fabs(acosf(cosATAngle)) >= angle)
                    continue;
            }

            if(AffectTarget(caster, target, nearby[i], power, client))
                affectedCount++;
        }

        if(affectedCount > 0)
        {
            if(client)
            {
                psserver->SendSystemInfo(caster->GetClientID(), "%s affected %d %s.", name.GetData(), affectedCount, (affectedCount == 1) ? "target" : "targets");
            }
            else
            {
                Debug4(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s affected %d %s.",name.GetData(), affectedCount, (affectedCount == 1) ? "target" : "targets");
            }

        }
        else
        {
            if(client)
            {
                psserver->SendSystemInfo(caster->GetClientID(), "%s has no effect.", name.GetData());
            }
            else
            {
                Debug2(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s has no effect.", name.GetData());
            }
        }
    }

    if(affectedCount > 0)
    {
        MathEnvironment env;
        env.Define("Caster",caster);
        env.Define("Realm", realm);
        env.Define("Spell", const_cast<psSpell*>(this));
        env.Define("CastDuration", (float)castingDuration);
        env.Define("MaxRealm", caster->GetCharacterData()->GetMaxAllowedRealm(way->skill));

        (void) psserver->GetCacheManager()->GetSpellPractice()->Evaluate(&env);

        MathVar* var = env.Lookup("PracticePoints");
        CS_ASSERT(var);
        caster->GetCharacterData()->Skills().AddSkillPractice(way->skill, var->GetRoundValue());
    }
}

bool psSpell::AffectTarget(gemActor* caster, gemObject* origTarget, gemObject* target, float power, Client* client) const
{
    if(offensive)
    {
        csString msg;
        if(!caster->IsAllowedToAttack(target,msg))
        {
            if(client)
            {
                psserver->SendSystemError(client->GetClientNum(), msg);
            }
            else
            {
                Debug2(LOG_SUPERCLIENT,caster->GetEID().Unbox(), "%s", msg.GetDataSafe());
            }

            return false;
        }

        gemActor* attackee = dynamic_cast<gemActor*>(target);
        if(attackee)
        {
            gemNPC* targetNPC = dynamic_cast<gemNPC*>(target);
            if(targetNPC)
            {
                psserver->GetNPCManager()->QueueAttackPerception(caster, targetNPC);
            }
        }
    }

    // Spell hit successfully.  Run the script.
    MathEnvironment env;
    env.Define("Caster", caster);
    env.Define("Target", target);
    env.Define("OrigTarget", origTarget); // the epicentre of an AOE attack/original cast target
    env.Define("Power",  power);

    outcome->Run(&env);

    // ???: If the NPC had a saving throw, I don't think we're supposed to tell them.  But we can't know if they did or not.
    psserver->GetNPCManager()->QueueSpellPerception(caster,target,npcSpellCategory,npcSpellCategoryID,npcSpellRelativePower);

    return true;
}

///
/// iScriptableVar Interface Implementation
///
double psSpell::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "Realm")
    {
        return realm;
    }
    else if(property == "Way")
    {
        return way->id;
    }
    Error2("Requested psSpell property not found '%s'", ptr);
    return 0;
}

double psSpell::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    csString function(functionName);
    if(function == "ManaCost")
    {
        iScriptableVar* obj = env->GetPointer(params[0]);
        gemActor* caster = dynamic_cast<gemActor*>(obj);
        return ManaCost(caster->GetCharacterData(), params[1]).mana;
    }
    else if(function == "StaminaCost")
    {
        iScriptableVar* obj = env->GetPointer(params[0]);
        gemActor* caster = dynamic_cast<gemActor*>(obj);
        return ManaCost(caster->GetCharacterData(), params[1]).stamina;
    }
    else if(function == "SuccessChance")
    {
        iScriptableVar* obj = env->GetPointer(params[0]);
        gemActor* caster = dynamic_cast<gemActor*>(obj);
        return ChanceOfCastSuccess(caster->GetCharacterData(), params[1]);
    }

    return 0;
}

//-----------------------------------------------------------------------------

psSpellCastGameEvent::psSpellCastGameEvent(const psSpell* spell,
        gemActor* caster,
        gemObject* target,
        csTicks castingDuration,
        float max_range,
        float kFactor,
        float powerLevel,
        Client* client)
    : psGameEvent(0, castingDuration, "psSpellCastGameEvent")
{
    this->spell      = spell;
    this->caster     = caster;
    this->target     = target;
    valid            = true;
    this->max_range  = max_range;
    this->kFactor    = kFactor;
    this->powerLevel = powerLevel;
    this->client     = client;
    this->duration = castingDuration;

    target->RegisterCallback(this);
    caster->RegisterCallback(this);
    caster->SetSpellCasting(this);
}

psSpellCastGameEvent::~psSpellCastGameEvent()
{
    if(target)
    {
        target->UnregisterCallback(this);
    }
    if(caster)
    {
        caster->UnregisterCallback(this);
    }
}

void psSpellCastGameEvent::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    if(target)
    {
        target->UnregisterCallback(this);
    }
    if(caster)
    {
        caster->UnregisterCallback(this);
    }

    Interrupt();

    target = NULL;
    caster = NULL;
}

void psSpellCastGameEvent::Interrupt()
{
    if(!IsValid())
        return;

    if(target && !target->IsAlive())
    {
        if(client)
        {
            psserver->SendSystemError(client->GetClientNum(), "%s is already dead.", (const char*) target->GetName());
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s: %s is already dead.",caster->GetName(), (const char*) target->GetName());
        }
    }
    else
    {
        if(client)
        {
            psserver->SendSystemInfo(client->GetClientNum(), "Your spell (%s) has been interrupted!", spell->GetName().GetData());
        }
        else
        {
            Debug3(LOG_SUPERCLIENT,caster->GetEID().Unbox(),"%s spell (%s) has been interrupted!",caster->GetName(), spell->GetName().GetData());
        }
    }

    caster->SetMode(PSCHARACTER_MODE_PEACE);
    caster->SetSpellCasting(NULL);

    // Stop event from beeing executed when trigged.
    SetValid(false);
}

void psSpellCastGameEvent::Trigger()
{
    // Make sure caster is alive...there might be UDP jitter problems (PS#2728).
    if(caster->IsAlive())
    {
        spell->Affect(caster, target, max_range, kFactor, powerLevel, client, duration);
    }

    // Spell casting complete, we are now in PEACE mode again.
    caster->SetMode(PSCHARACTER_MODE_PEACE);
    caster->SetSpellCasting(NULL);
}

