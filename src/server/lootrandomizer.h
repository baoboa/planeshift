/*
 * lootrandomizer.h by Stefano Angeleri
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 */

#ifndef __LOOTRANDOMIZER_H__
#define __LOOTRANDOMIZER_H__

class RandomizedOverlay;

/**
 * \addtogroup server
 * @{ */

/**
 * This class holds one loot modifier
 * The lootRandomizer contions arrays of these
 */
struct LootModifier
{
    uint32_t id;                       ///< The id assigned to this modifier in the db. It's referenced by psitem.
    uint32_t mod_id;                   ///< The modifier position ID
    csString modifier_type;            ///< The type of modifier (suffix, prefix, adjective)
    csString name;                     ///< The part of name which this modifier will add when used. The position is determined by modifier_type
    csString effect;                   ///< Declares modifiers to some stats like weight, weapon speed, resistance/attack types.
    csString equip_script;             ///< The part of equip_script to add to the item when this modifier is used.
    float    probability;              ///< The probability for this loot modifier to happen when generating a random item.
    float    probabilityRange;         ///< The probability for this loot modifier expressed as range between current number and previous one.
    csString stat_req_modifier;        ///< The additional requirements to stats when this modifier is used.
    float    cost_modifier;            ///< The cost of this modifier @see CalcModifierCostCap
    csString mesh;                     ///< The mesh this modifier will use for the random item generated.
    csString icon;                     ///< The icon this modifier will use for the random item generated.
    csString not_usable_with;          ///< Defines which modifiers this isn't usable with.
    csHash<bool, uint32_t> itemRestrain; ///< Contains if the itemid is allowed or not. item id 0 means all items, false means disallowed.

    // Return true if this modifier is allowed with the given item stats ID.
    bool IsAllowed(uint32_t itemID);
};

/**
 * This structure contains the parsed data from Attributes
 * recarding script variables.
 */
struct ValueModifier
{
    csString name;  ///< The name of the variable.
    csString op;    ///< The operation parsed for this reference to the variable ADD, MUL, VAL (VAL is the default value, at least one is needed).
    float value;    ///< The value to apply to the variable through the requested operation in op.
};

class MathScript;
/**
 * This class stores an array of LootModifiers and randomizes
 * loot stats.
 */
class LootRandomizer
{
protected:
    csArray<LootModifier*> prefixes;   ///< Keeps all the loot modifiers of type "prefix"
    csArray<LootModifier*> suffixes;   ///< Keeps all the loot modifiers of type "suffix"
    csArray<LootModifier*> adjectives; ///< Keeps all the loot modifiers of type "adjective"
    csHash<LootModifier*, uint32_t> LootModifiersById;  ///< Keeps all the lootmodifiers for faster access when we know the id.

    // precalculated max values for probability. min is always 0
    float prefix_max;
    float adjective_max;
    float suffix_max;

public:
    /**
     * Constructor.
     *
     *  @param cachemanager A pointer to the cache manager.
     */
    LootRandomizer(CacheManager* cachemanager);
    /**
     * Destructor
     */
    ~LootRandomizer();

    /**
     * This adds another item to the entries array.
     */
    void AddLootModifier(LootModifier* entry);

    /**
     * Gets a loot modifier from it's id.
     *
     * @param id The id of the item we are searching for.
     * @return A pointer to the loot modifier which is referenced by the id we were searching for.
     */
    LootModifier* GetModifier(uint32_t id);
 
    /**
     * Gets a loot modifier from it's id.
     *
     * @param id The id of the basic item stats we are searching for.
     * @param mods The list of modifiers returned
     * @return True if the array was filled in.
     */
    bool GetModifiers(uint32_t itemID, csArray<psGMSpawnMods::ItemModifier>& mods);

    /**
     * Validates and sets the loot modifiers with the given ids.
     *
     * @param item The item instance being modified.
     * @param mods The ids of the item modifiers to set.
     * @return A pointer to the modified item.
     */
    psItem* SetModifiers(psItem* item, csArray<uint32_t>& mods);

    /**
     * This randomizes the current loot item and returns the item with the modifiers applied.
     *
     * @param item The item instance which we will be randomizing.
     * @param cost The maximum "cost" of the randomization we can apply @see CalcModifierCostCap
     * @param lootTesting Says if we really are applying the modifiers.
     * @param numModifiers Forces the amount of modifiers to apply.
     * @return A pointer to the resulting item (it's the same pointer which was passed to the function).
     */
    psItem* RandomizeItem(psItem* item,
                          float cost,
                          bool lootTesting = false,
                          size_t numModifiers = 0);

    /**
     * Runs the LootModifierCap mathscript and returns the result, used
     * by other function to determine if a modifier can be added.
     *
     * @param chr The psCharacter which is being analyzed.
     * @return A number determining the cost cap for the random modifiers.
     */
    float CalcModifierCostCap(psCharacter* chr);

    /**
     * Applies modifications to a randomized overlay depending on the requested ids.
     *
     * @param baseItem The basic item which will have the overlay generated for.
     * @param overlay A pointer to the overlay where we will save the modifications to apply to this item.
     * @param modifiersIds An array with all the ids of the modifiers which we will need to apply to the overlay.
     */
    void ApplyModifier(psItemStats* baseItem, RandomizedOverlay* overlay, csArray<uint32_t> &modifiersIds);

    /**
     * Returns the percent probability of a modifier based on the total number of modifiers available of that type.
     *
     * @param modifierID the ID of the modifier to evaluate.
     * @param modifierType 0=prefix, 1=suffix, 2=adjective
     * @return the float percentage, or 0 if the modifier is invalid or has no probability
     */
    float GetModifierPercentProbability(int modifierID, int modifierType);

protected:
    MathScript* modifierCostCalc; ///<A cached reference to the LootModifierCap math script.
    CacheManager* cacheManager; ///<A reference to the cachemanager.

private:
    void AddModifier(LootModifier* oper1, LootModifier* oper2);

    /**
     * Sets an attribute to the item overlay. utility function used when parsing the loot modifiers xml.
     *
     * @param op The operation to do with the attributes. (+, -, *)
     * @param attrName The name of the attribute we are changing.
     * @param modifier The amount to change of the attribute (right operand, left operand is the basic attribute)
     * @param overlay The randomization overlay where we are applying these attributes.
     * @param baseItem The base item of the item we are applying these attributes to
     * @param values An array which will be filled with the variables defined as attribute (var.Name). It's important the name
     *                starts with an Uppercase letter because of mathscript restrains.
     * @return TRUE if the operation succeded.
     */
    bool SetAttribute(const csString &op, const csString &attrName, float modifier, RandomizedOverlay* overlay, psItemStats* baseItem, csArray<ValueModifier> &values);

    /**
     * Sets the attributes in the passed float array according to the passed parameters.
     *
     * @param value An array of pointers to floats containing the value to be modified.
     * @param modifier The amount to apply through all the array values.
     * @param amount The size of the passed array of pointers.
     * @param op The specifier of the operation to fullfill: ADD, MUL, VAL.
     */
    void SetAttributeApplyOP(float* value[], float modifier, size_t amount, const csString &op);

    /**
     * Generates the equip script amended with the defined variables.
     *
     * @param name The name of the item.
     * @param equip_script The inner equip script defined in the equip_script column of the database.
     * @param values An array of values parsed from the set attributes which will be used to add in the
     *               equip_script environment those variables.
     * @return The generated script to be used as equip_script.
     */
    csString GenerateScriptXML(csString &name, csString &equip_script, csArray<ValueModifier> &values);
};

/** @} */

#endif

