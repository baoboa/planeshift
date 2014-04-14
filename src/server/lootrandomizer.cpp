/*
 * lootrandomizer.cpp by Stefano Angeleri
 *
 * Copyright (C) 2012 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/randomgen.h>
#include <csutil/csstring.h>
#include <iutil/document.h>
#include <csutil/xmltiny.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/psitem.h"
#include "bulkobjects/psitemstats.h"

#include "util/mathscript.h"
#include "util/psconst.h"
#include "util/serverconsole.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "cachemanager.h"
#include "psserver.h"
#include "globals.h"
#include "lootrandomizer.h"
#include "scripting.h"

bool LootModifier::IsAllowed(uint32_t itemID)
{
    // First check if the item is among the restraints.
    if(itemRestrain.Contains(itemID))
    {
        // If it was found, we get it's result.
        if(!itemRestrain.Get(itemID, false))
        {
            // If it was false, it means the item was disallowed.
            return false;
        }
    }

    // The second check is for the generic restraint (issued as itemID = 0)
    // which says what is the default for this modifier when an itemID specific
    // restraint is not found. If this isn't found either, the default will be
    // to allow the modifier for the current item. Maintaining the previous
    // behaviour.
    else if(itemRestrain.Contains(0))
    {
        // If it was found, we get it's result.
        if(!itemRestrain.Get(0, false))
        {
            // If it was false, it means unspecified items were disallowed.
            return false;
        }
    }
    return true;
}

LootRandomizer::LootRandomizer(CacheManager* cachemanager)
{
    prefix_max = 0;
    adjective_max = 0;
    suffix_max = 0;
    cacheManager = cachemanager;

    // Find any math scripts that are needed
    modifierCostCalc = psserver->GetMathScriptEngine()->FindScript("LootModifierCostCap");
}

LootRandomizer::~LootRandomizer()
{
    LootModifier* e;
    while(prefixes.GetSize())
    {
        e = prefixes.Pop();
        delete e;
    }
    while(suffixes.GetSize())
    {
        e = suffixes.Pop();
        delete e;
    }
    while(adjectives.GetSize())
    {
        e = adjectives.Pop();
        delete e;
    }
}

void LootRandomizer::AddLootModifier(LootModifier* entry)
{
    if(entry->modifier_type.CompareNoCase("prefix"))
    {
        entry->mod_id = psGMSpawnMods::ITEM_PREFIX;
        prefixes.Push(entry);
        if(entry->probability > prefix_max)
        {
            prefix_max = entry->probability;
        }
    }
    else if(entry->modifier_type.CompareNoCase("suffix"))
    {
        entry->mod_id = psGMSpawnMods::ITEM_SUFFIX;
        suffixes.Push(entry);
        if(entry->probability > suffix_max)
        {
            suffix_max = entry->probability;
        }
    }
    else if(entry->modifier_type.CompareNoCase("adjective"))
    {
        entry->mod_id = psGMSpawnMods::ITEM_ADJECTIVE;
        adjectives.Push(entry);
        if(entry->probability > adjective_max)
        {
            adjective_max = entry->probability;
        }
    }
    else
    {
        Error2("Unsupported modifier type %s", entry->modifier_type.GetData());
        return;
    }

    //put the lootmodifier in an hash for a faster access when we just need to look it up by id.
    LootModifiersById.Put(entry->id, entry);
}

psItem* LootRandomizer::RandomizeItem(psItem* item, float maxcost, bool lootTesting, size_t numModifiers)
{
    uint32_t rand;
    uint32_t modifierType;
    csArray<uint32_t> selectedModifierTypes;
    float totalCost = item->GetBaseStats()->GetPrice().GetTrias();
    uint32 itemID = item->GetBaseStats()->GetUID();

    // Set up ModifierTypes
    // The Order of the modifiers is significant. It determines the priority of the modifiers, currently this is
    // Suffixes, Prefixes, Adjectives : So we add them in reverse order so the highest priority is applied last
    selectedModifierTypes.Push(psGMSpawnMods::ITEM_SUFFIX);
    selectedModifierTypes.Push(psGMSpawnMods::ITEM_PREFIX);
    selectedModifierTypes.Push(psGMSpawnMods::ITEM_ADJECTIVE);

    // Determine Probability of number of modifiers ( 0-3 )
    if(!lootTesting)
    {
        rand = psserver->rng->Get(100);   // Range of 0 - 99
        if(rand < 1)    // 1% chance
            numModifiers = 3;
        else if(rand < 8)    // 7% chance
            numModifiers = 2;
        else if(rand < 30)    // 22% chance
            numModifiers = 1;
        else // 70% chance
            numModifiers = 0;
    }

    // If there are no additional modifiers return original stats
    if(numModifiers == 0)
        return item;

    if(numModifiers != 3)
    {
        while(selectedModifierTypes.GetSize() != numModifiers)
        {
            rand = psserver->rng->Get(99);   // Range of 0 - 98
            if(rand < 60)
                selectedModifierTypes.Delete(psGMSpawnMods::ITEM_SUFFIX);   // higher chance to be removed. 60% chance
            else if(rand < 85)
                selectedModifierTypes.Delete(psGMSpawnMods::ITEM_PREFIX);   // 25% chance
            else
                selectedModifierTypes.Delete(psGMSpawnMods::ITEM_ADJECTIVE);   // lower chance to be removed. 14% chance
        }
    }

    // for each modifiertype roll a dice to see which modifier we get
    while(selectedModifierTypes.GetSize() != 0)
    {
        modifierType = selectedModifierTypes.Pop();
        int newModifier, probability;
        int max_probability = 0;
        LootModifier* lootModifier = NULL;
        csArray<LootModifier*>* modifierList = NULL;

        if(modifierType == psGMSpawnMods::ITEM_PREFIX)
        {
            modifierList = &prefixes;
            max_probability=(int)prefix_max;
        }
        else if(modifierType == psGMSpawnMods::ITEM_SUFFIX)
        {
            modifierList = &suffixes;
            max_probability=(int)suffix_max;
        }
        else if(modifierType == psGMSpawnMods::ITEM_ADJECTIVE)
        {
            modifierList = &adjectives;
            max_probability=(int)adjective_max;
        }
        else
        {
            //this shouldn't be reached
            Error1("BUG in randomizeItem(). Found an unsupported modifier");
            continue;
        }

        // Get min probability <= probability <= max probability in modifiers list
        //probability = psserver->rng->Get( (int)((*modifierList)[ modifierList->Length() - 1 ]->probability - (int) (*modifierList)[0]->probability ) + 1) + (int) (*modifierList)[0]->probability;
        // Get returns a number < limit (Get a uint32 integer random number in range 0 <= num < iLimit.)
        // so we must increase it of 1 in order to pick the case with the highest "probability" and exclude 0
        // which means "disabled" or "manual".
        probability = psserver->rng->Get(max_probability) + 1.0f;
        for(newModifier = (int)modifierList->GetSize() - 1; newModifier >= 0 ; newModifier--)
        {
            if(!(*modifierList)[newModifier]->IsAllowed(itemID))
            {
                continue;
            }

            float item_prob = ((*modifierList)[newModifier]->probability);

            // Probability starts from 1 items under this are automatically excluded
            // as "disabled" from this means of randomization.
            if(item_prob >= 1.0f && probability >=  item_prob)
            {
                if(maxcost >= totalCost * (*modifierList)[newModifier]->cost_modifier ||
                        lootTesting)
                {
                    lootModifier = (*modifierList)[ newModifier ];
                    totalCost = totalCost * (*modifierList)[newModifier]->cost_modifier;
                    break;
                }
            }
        }

        // if just testing loot randomizing, then dont want equip/dequip events
        //if (lootTesting && lootModifier)
        //{
        //    lootModifier->prg_evt_equip.Empty();
        //}

        //add this modifier to the item
        if(lootModifier) item->AddLootModifier(lootModifier->id, modifierType);
    }
    //regenerate the modifiers cache of the item
    item->UpdateModifiers();

    // If we arrive here, at least one modifier as been added, so we make the item "identifiable"
    item->SetIsIdentifiable(true);

    return item;
}

bool LootRandomizer::GetModifiers(uint32_t itemID,
                                  csArray<psGMSpawnMods::ItemModifier>& mods)
{
    csHash<LootModifier*, uint32_t>::GlobalIterator it =
        LootModifiersById.GetIterator();
    while(it.HasNext())
    {
        LootModifier* lootModifier = it.Next();
        if(!lootModifier->IsAllowed(itemID))
        {
            continue;
        }

        psGMSpawnMods::ItemModifier mod;
        mod.name = lootModifier->name;
        mod.id = lootModifier->id;
        mod.type = lootModifier->mod_id;
        mods.Push(mod);
    }

    return mods.GetSize() != 0;
}

psItem* LootRandomizer::SetModifiers(psItem* item, csArray<uint32_t>& mods)
{
    if (mods.GetSize() == 0)
        return item;

    bool found = false;
    uint32_t itemID = item->GetBaseStats()->GetUID();

    for(size_t i = 0; i < mods.GetSize(); i++)
    {
        LootModifier* lootModifier = GetModifier(mods[i]);

        if(!lootModifier || mods[i] == 0)
            continue;

        if(!lootModifier->IsAllowed(itemID))
        {
            continue;
        }

        printf("found\n");
        item->AddLootModifier(lootModifier->id, lootModifier->mod_id);
        found = true;
    }

    if (found)
    {
        // Regenerate the modifiers cache of the item
        item->UpdateModifiers();

        // At least one modifier as been added, so we make the item "identifiable"
        item->SetIsIdentifiable(true);
    }

    return item;
}

void LootRandomizer::AddModifier(LootModifier* oper1, LootModifier* oper2)
{
    csString newName;
    // Change Name
    if(oper2->mod_id == psGMSpawnMods::ITEM_PREFIX)
    {
        newName.Append(oper2->name);
        newName.Append(" ");
        newName.Append(oper1->name);
    }
    else if(oper2->mod_id == psGMSpawnMods::ITEM_SUFFIX)
    {
        newName.Append(oper1->name);
        newName.Append(" ");
        newName.Append(oper2->name);
    }
    else if(oper2->mod_id == psGMSpawnMods::ITEM_ADJECTIVE)
    {
        newName.Append(oper2->name);
        newName.Append(" ");
        newName.Append(oper1->name);
    }
    oper1->name = newName;

    // Adjust Price
    oper1->cost_modifier *= oper2->cost_modifier;
    oper1->effect.Append(oper2->effect);
    oper1->mesh = oper2->mesh;
    oper1->icon = oper2->icon;
    oper1->stat_req_modifier.Append(oper2->stat_req_modifier);

    // equip script
    oper1->equip_script.Append(oper2->equip_script);
}

void LootRandomizer::SetAttributeApplyOP(float* value[], float modifier, size_t amount,  const csString &op)
{
    // Operations  = ADD, MUL, SET
    for(size_t i = 0; i < amount; i++)
    {
        if(op.CompareNoCase("ADD"))
        {
            if(value[i]) *value[i] += modifier;
        }

        else if(op.CompareNoCase("MUL"))
        {
            if(value[i]) *value[i] *= modifier;
        }

        else if(op.CompareNoCase("VAL"))
        {
            if(value[i]) *value[i] = modifier;
        }
    }
}

bool LootRandomizer::SetAttribute(const csString &op, const csString &attrName, float modifier, RandomizedOverlay* overlay, psItemStats* baseItem, csArray<ValueModifier> &values)
{
    float* value[3] = { NULL, NULL, NULL };
    // Attribute Names:
    // item
    //        weight
    //        damage
    //            slash
    //            pierce
    //            blunt
    //            ...
    //        protection
    //            slash
    //            pierce
    //            blunt
    //            ...
    //        speed

    csString AttributeName(attrName);
    AttributeName.Downcase();
    if(AttributeName.StartsWith("var."))
    {
        //if var. was found we have a script variable
        //parse it and place it in the array which will be used later
        //to handle those variables.
        ValueModifier val;

        //start the name of the variable after var.
        val.name = attrName.Slice(4);
        val.value = modifier;
        val.op = op;

        //add the variable to the array
        values.Push(val);
        return true;
    }
    if(AttributeName.Compare("item.weight"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->weight))
        {
            overlay->weight = baseItem->GetWeight();
        }

        value[0] = &overlay->weight;
    }
    else if(AttributeName.Compare("item.speed"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->latency))
        {
            overlay->latency = baseItem->Weapon().Latency();
        }

        value[0] = &overlay->latency;
    }
    else if(AttributeName.Compare("item.damage"))
    {
        for(int i = 0; i < 3; i++)
        {
            // Initialize the value if needed
            if(CS::IsNaN(overlay->damageStats[i]))
            {
                overlay->damageStats[i] = baseItem->Weapon().Damage((PSITEMSTATS_DAMAGETYPE)i);
            }
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
        value[1] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
        value[2] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if(AttributeName.Compare("item.damage.slash"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH]))
        {
            overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH] = baseItem->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_SLASH);
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
    }
    else if(AttributeName.Compare("item.damage.pierce"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE]))
        {
            overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE] = baseItem->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_PIERCE);
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if(AttributeName.Compare("item.damage.blunt"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT]))
        {
            overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT] = baseItem->Weapon().Damage(PSITEMSTATS_DAMAGETYPE_BLUNT);
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
    }
    else if(AttributeName.Compare("item.protection"))
    {
        for(int i = 0; i < 3; i++)
        {
            // Initialize the value if needed
            if(CS::IsNaN(overlay->damageStats[i]))
            {
                overlay->damageStats[i] = baseItem->Armor().Protection((PSITEMSTATS_DAMAGETYPE)i);
            }
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
        value[1] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
        value[2] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if(AttributeName.Compare("item.protection.slash"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH]))
        {
            overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH] = baseItem->Armor().Protection(PSITEMSTATS_DAMAGETYPE_SLASH);
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_SLASH];
    }
    else if(AttributeName.Compare("item.protection.pierce"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE]))
        {
            overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE] = baseItem->Armor().Protection(PSITEMSTATS_DAMAGETYPE_PIERCE);
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_PIERCE];
    }
    else if(AttributeName.Compare("item.protection.blunt"))
    {
        // Initialize the value if needed
        if(CS::IsNaN(overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT]))
        {
            overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT] = baseItem->Armor().Protection(PSITEMSTATS_DAMAGETYPE_BLUNT);
        }

        value[0] = &overlay->damageStats[PSITEMSTATS_DAMAGETYPE_BLUNT];
    }

    SetAttributeApplyOP(value, modifier, 3, op);

    return true;
}

void LootRandomizer::ApplyModifier(psItemStats* baseItem, RandomizedOverlay* overlay, csArray<uint32_t> &modifierIds)
{
    LootModifier mod;

    //set up default mod data
    mod.cost_modifier = 1;
    mod.name = baseItem->GetName();
    csArray<ValueModifier> variableValues;

    //creates the full lootmodifier from the ids being applied.
    //0 should be left empty in the database as it acts as NO HIT.
    for(size_t i = 0; i < modifierIds.GetSize(); i++)
    {
        uint32_t modID = modifierIds.Get(i);
        if(modID) //0 means nothing to do, no need to search for the void.
        {
            LootModifier* partialModifier = GetModifier(modID);
            if(partialModifier)
            {
                overlay->active = true;
                AddModifier(&mod, partialModifier);
            }
        }
    }
    //all is done no modifiers where found. so we just get out
    if(overlay->active == false)
        return;

    overlay->name = mod.name;
    overlay->price = psMoney(baseItem->GetPrice().GetTrias() * mod.cost_modifier);
    if(mod.mesh.Length() > 0)
        overlay->mesh = mod.mesh;
    if(mod.icon.Length() > 0)
        overlay->icon = mod.icon;

    // Apply effect
    csString xmlItemMod;

    xmlItemMod.Append("<ModiferEffects>");
    xmlItemMod.Append(mod.effect);
    xmlItemMod.Append("</ModiferEffects>");

    // Read the ModiferEffects XML into a doc*/
    csRef<iDocument> xmlDoc = ParseString(xmlItemMod);
    if(!xmlDoc)
    {
        Error1("Parse error in Loot Randomizer");
        return;
    }
    csRef<iDocumentNode> root    = xmlDoc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Loot Randomizer");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("ModiferEffects");//Are we sure it is "Modifer"?
    if(!topNode)
    {
        Error1("No <ModiferEffects> in Loot Randomizer");
        return;
    }

    csRef<iDocumentNodeIterator> nodeList = topNode->GetNodes("ModiferEffect");

    // For Each ModiferEffect
    csRef<iDocumentNode> node;
    while(nodeList->HasNext())
    {
        node = nodeList->Next();
        //Determine the Effect
        csString EffectOp = node->GetAttribute("operation")->GetValue();
        csString EffectName = node->GetAttribute("name")->GetValue();
        float EffectValue = node->GetAttribute("value")->GetValueAsFloat();

        //Add to the Attributes
        if(!SetAttribute(EffectOp, EffectName, EffectValue, overlay, baseItem, variableValues))
        {
            // display error and continue
            Error2("Unable to set attribute %s on new loot item.",EffectName.GetData());
        }
    }

    // Apply stat_req_modifier
    csString xmlStatReq;

    xmlStatReq.Append("<StatReqs>");
    xmlStatReq.Append(mod.stat_req_modifier);
    xmlStatReq.Append("</StatReqs>");

    // Read the Stat_Req XML into a doc
    xmlDoc = ParseString(xmlStatReq);
    if(!xmlDoc)
    {
        Error1("Parse error in Loot Randomizer");
        return;
    }
    root    = xmlDoc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Loot Randomizer");
        return;
    }
    topNode = root->GetNode("StatReqs");
    if(!topNode)
    {
        Error1("No <statreqs> in Loot Randomizer");
        return;
    }

    nodeList = topNode->GetNodes("StatReq");
    // For Each Stat_Req
    while(nodeList->HasNext())
    {
        node = nodeList->Next();
        //Determine the STAT
        ItemRequirement req;
        req.name = node->GetAttribute("name")->GetValue();
        req.min_value = node->GetAttribute("value")->GetValueAsFloat();
        //Add to the Requirements
        overlay->reqs.Push(req);
    }

    // Apply equip script
    if(!mod.equip_script.IsEmpty())
    {
        csString scriptXML = GenerateScriptXML(mod.name, mod.equip_script, variableValues);
        overlay->equip_script = ApplicativeScript::Create(psserver->entitymanager, psserver->GetCacheManager(), scriptXML);
    }

    //clamp speed at 1.5s to keep consistency with item_stats
    if(!CS::IsNaN(overlay->latency) && overlay->latency < 1.5F)
        overlay->latency = 1.5F;
}

float LootRandomizer::GetModifierPercentProbability(int modifierID, int modifierType)
{
    LootModifier* modifier = GetModifier(modifierID);

    if(!modifier)
        return 0;

    if(modifierType == psGMSpawnMods::ITEM_PREFIX)
        return modifier->probabilityRange/prefix_max;
    // 1=suffix
    else if(modifierType == psGMSpawnMods::ITEM_SUFFIX)
        return modifier->probabilityRange/suffix_max;
    // 2=adjective
    else if(modifierType == psGMSpawnMods::ITEM_ADJECTIVE)
        return modifier->probabilityRange/adjective_max;
    else
        Error2("Unknown modifierID: %d",modifierID);

    return 0;
}

csString LootRandomizer::GenerateScriptXML(csString &name, csString &equip_script, csArray<ValueModifier> &values)
{
    csString scriptXML;
    csHash<float, csString> results;

    //prepare the heading of the apply script
    scriptXML.Format("<apply aim=\"Actor\" name=\"%s\" type=\"buff\">", name.GetData());

    //parse the values and try to find the default values for all variables
    //and put them in the hash in order to simplify access.
    for(size_t i = 0; i < values.GetSize(); ++i)
    {
        //Do it only for the VAL references which identify a default value.
        if(values[i].op.CompareNoCase("VAL"))
        {
            results.PutUnique(values[i].name, values[i].value);
            //delete the entry as we don't need it anymore, in order to reduce the
            //amount of items to check later
            values.DeleteIndex(i);
            //reduce the index as we have removed an item
            i--;
        }
    }
    //then do all the others operation entries, in order of definition.
    for(size_t i = 0; i < values.GetSize(); ++i)
    {
        //first of all check if the item is available in its default value
        //if it's not we are ignoring the operation as the variable is not
        //defined.
        if(results.Contains(values[i].name))
        {
            //we don't check for null as at this point it's safe the presence
            //of the variable in the hash (due to the check of contains).
            float* value[1] = {results.GetElementPointer(values[i].name)};
            SetAttributeApplyOP(value, values[i].value, 1, values[i].op);
        }
        else
        {
            Error2("Unable to find base value for variable %s", values[i].name.GetData());
        }
    }

    //if we have any variable defined we add their definition on top of the script.
    //By defining each variable in a <let> entry for the script. It will be skipped
    //if no variables are defined.
    if(!results.IsEmpty())
    {
        scriptXML.AppendFmt("<let vars=\"");
        csHash<float, csString>::GlobalIterator it = results.GetIterator();
        while(it.HasNext())
        {
            csTuple2<float, csString> entry = it.NextTuple();
            scriptXML.AppendFmt("%s = %f;", entry.second.GetData(), entry.first);
        }
        scriptXML.AppendFmt("\" />");
    }

    //now copy the body of the script containing the equip script from the random loot entries.
    scriptXML.AppendFmt("%s</apply>", equip_script.GetData());

    return scriptXML;
}


LootModifier* LootRandomizer::GetModifier(uint32_t id)
{
    return LootModifiersById.Get(id, NULL);
}

float LootRandomizer::CalcModifierCostCap(psCharacter* chr)
{
    float result = 1000.0;

    if(modifierCostCalc)
    {
        // Use the mob's attributes to calculate modifier cost cap
        MathEnvironment env;
        env.Define("Actor",   chr);
        env.Define("MaxHP",   chr->GetMaxHP().Current());
        env.Define("MaxMana", chr->GetMaxMana().Current());

        (void) modifierCostCalc->Evaluate(&env);
        MathVar* modcap = env.Lookup("ModCap");
        result = modcap->GetValue();
    }
    Debug2(LOG_LOOT,0,"DEBUG: Calculated cost cap %f\n", result);
    return result;
}
