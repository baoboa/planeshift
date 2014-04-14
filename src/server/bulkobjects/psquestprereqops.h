/*
 * psquest.h
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

#ifndef __PSQUESTPREREQOPS_H__
#define __PSQUESTPREREQOPS_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/refarr.h>
#include <csutil/refcount.h>

//=============================================================================
// Project Includes
//=============================================================================

#include "pstrait.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psattack.h"

class psCharacter;
class psQuest;
class psSkillInfo;
struct Faction;

/**
 * \addtogroup bulkobjects
 * @{ */

/**
 * Pure virtual base quest prerequisite operator class
 *
 * This base class define the needed functions for every
 * prerequisite operator.
 */
class psQuestPrereqOp : public csRefCount
{
public:
    /**
     * Destructor for the prerequisite operator.
     *
     */
    virtual ~psQuestPrereqOp() {};

    /**
     * Check for valid prerequisite
     *
     * Override this function to generate a test for any prerequisite.
     *
     * @param  character The character that are checking for a trigger
     * @return True if the prerequisite is true and the trigger holding
     *         the prerequisite should be available.
     */
    virtual bool Check(psCharacter* character) = 0;

    /**
     * Convert the prerequisite script to a xml string
     *
     * Wrapps the operator in \<pre\>...\</pre\> tags.
     *
     * @return XML string for the prerequisite script.
     */
    virtual csString GetScript();

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Override this function to return the prerequisite xml tag for your new
     * operator.
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp() = 0;

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy() = 0;
};

/**
 * Basis list prerequisite operator.
 *
 * Define basic operations for operations needing any number of
 * prerequisites to be complete. Like an and operator with any numbers
 * of childs.
 */
class psQuestPrereqOpList: public psQuestPrereqOp
{
protected:
    /**
     * The list of child prerequisite operators for this list operator.
     */
    csRefArray<psQuestPrereqOp> prereqlist;
public:

    /**
     * Destructor for the list prerequisite operator.
     *
     * Will delete any prerequisite pushed on to the list
     * of child prerequisites.
     */
    virtual ~psQuestPrereqOpList() {}

    /**
     * Push a new child prerequisite onto the child list.
     *
     * Add another prerequisite operator to the list of childs
     * for this list prerequisite.
     *
     * @param  prereqOp The prerequisite operator to be appended to the list.
     */
    virtual void Push(csRef<psQuestPrereqOp> prereqOp);

    /**
     * Insert a new child prerequisite onto the child list.
     *
     * Add another prerequisite operator to the list of childs
     * for this list prerequisite.
     *
     * @param  n Insert the \c prereqOp before prerequisite \c n
     * @param  prereqOp The prerequisite operator to be inserted to the list.
     */
    virtual void Insert(size_t n, csRef<psQuestPrereqOp> prereqOp);

};

/**
 * And Prerequisite operator.
 *
 * A multi term and operator. Every prerequisite have to be true
 * for this operator to be valid.
 */
class psQuestPrereqOpAnd: public psQuestPrereqOpList
{
public:

    /**
     * Destructor for the and prerequisite operator.
     */
    virtual ~psQuestPrereqOpAnd() {}

    /**
     * Check if all child prerequisites are true
     *
     * prerequisite = child1 and child2 and ... childN
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if all the child prerequisites are true
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator to the xml string:
     * \<and\>\<child1/\>...\<childN/\>\</and\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};


/**
 * Or prerequisite operator.
 *
 * A multi term or operator. One prerequisite have to be true
 * for this operator to be valid.
 */
class psQuestPrereqOpOr: public psQuestPrereqOpList
{
public:

    /**
     * Destructor for the or prerequisite operator.
     */
    virtual ~psQuestPrereqOpOr() {}

    /**
     * Check if any of the child prerequisites are true
     *
     * prerequisite = child1 or child 2 or ... childN
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if one the child prerequisites are true
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator to the xml string:
     * \<or\>\<child1/\>...\<childN/\>\</or\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Require prerequisite operator.
 *
 * A multi term require operator. The given minimum and/or maximum of
 * childs have to be completed for this prerequisite to be true.
 */
class psQuestPrereqOpRequire: public psQuestPrereqOpList
{
    int min,max;
public:

    /**
     * Construct a require operator.
     *
     * @param min_required   The minimum of childs needed. There are no lower
     *                       limit if this is set to -1.
     * @param max_required   The maximum of childs needed. There are no upper
     *                       limit if this is set to -1.
     */
    psQuestPrereqOpRequire(int min_required,int max_required);

    /**
     * Destructor for the require prerequisite operator.
     */
    virtual ~psQuestPrereqOpRequire() {}

    /**
     * Check if a number of prerequisites are true. Limited by
     * a min and a max number. If any of thise are -1 the limit
     * will be ignored.
     *
     * number = number of childs true
     * prerequisite = number >= min and number <= max
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if more than min and less than max of the child prerequisites are true
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator into the xml string:
     * \<require [min="min"] [max="max"]\>\<child1\>...\<childN\>\</require\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Not prerequisite operator.
 *
 * Invert the result of the one child.
 */
class psQuestPrereqOpNot: public psQuestPrereqOpList
{
public:

    /**
     * Destructor for the not prerequisite operator.
     */
    virtual ~psQuestPrereqOpNot() {}

    /**
     * Check if the child prerequisite is false.
     *
     * prerequisite = not child
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the one child prerequisite is false
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator into the xml string:
     * \<not\>\<child\>\</not\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Quest Completed prerequisite operator.
 *
 * The given quest have to be completed for this prerequisite
 * to be true.
 */
class psQuestPrereqOpQuestCompleted: public psQuestPrereqOp
{
protected:
    /**
     * The quest that need to be completed.
     */
    psQuest* quest;
    csString name;

public:

    /**
     * Construct a quest completed operator
     *
     * @param quest The quest that need to be completed
     */
    psQuestPrereqOpQuestCompleted(psQuest* quest):quest(quest) {};

    /**
      * Construct a quest completed operator
      *
      * @param questName The quest that need to be completed
      */
    psQuestPrereqOpQuestCompleted(csString questName);

    /**
     * Destructor for the quest completed prerequisite operator.
     */
    virtual ~psQuestPrereqOpQuestCompleted() {}

    /**
     * Check if the given quest is completed.
     *
     * prerequisite = Is the quest completed.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the charachter have completed the given quest.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator into the xml string:
     * \<completed quest="quest name"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Quest assigned prerequisite operator.
 *
 * The given quest have to be assigned for this prerequisite
 * to be true.
 */
class psQuestPrereqOpQuestAssigned: public psQuestPrereqOp
{
protected:
    /**
     * The quest that need to be assigned.
     */
    psQuest* quest;
public:

    /**
     * Construct a quest assigned operator
     *
     * @param quest The quest that need to be assigned.
     */
    psQuestPrereqOpQuestAssigned(psQuest* quest):quest(quest) {};

    /**
     */
    virtual ~psQuestPrereqOpQuestAssigned() {}

    /**
     * Check if the given quest is assigned.
     *
     * prerequisite = Is the quest assigned.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the character have assigned the given quest.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator into the xml string:
     * \<assigned quest="quest name"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Quest Completed Category operator
 *
 * Need to have completed a number of quests for a given category.
 */
class psQuestPrereqOpQuestCompletedCategory: public psQuestPrereqOp
{
protected:

    /**
     * The minimum of quest in the given category that need to be completed.
     */
    int min;

    /**
     * The maximum of quest in the given category that need to be completed.
     */
    int max;

    /**
     * The category that will be tested for.
     */
    csString category;
public:

    /**
     * Construct a quest completed category opererator
     *
     * @param quest_category The category that the user is tested agains.
     * @param min_required   The minimum of quests needed. There are no lower
     *                       limit if this is set to -1.
     * @param max_required   The maximum of quests needed. There are no upper
     *                       limit if this is set to -1.
     */
    psQuestPrereqOpQuestCompletedCategory(csString quest_category,
                                          int min_required,int max_required)
        : min(min_required),max(max_required),category(quest_category) {};

    /**
     * Destructor for the quest completed category prerequisite operator.
     */
    virtual ~psQuestPrereqOpQuestCompletedCategory() {}

    /**
     * Check if the charater have completed a number quests with the given category.
     *
     * number = number of quests completed in the given category.
     * prerequisite = number >= min and number <= max
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if more than min and less than max quests of the given category is completed.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator into the xml string: \<completed category="category" [min="min"] [max="max"] /\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Faction prerequisite operator.
 *
 * The given faction level is needed for this prerequisite
 * to be true.
 */
class psQuestPrereqOpFaction: public psQuestPrereqOp
{
protected:
    /**
     * The faction that is to be checked.
     */
    Faction* faction;

    /**
     * The faction level needed
     */
    int value;
    bool max;
public:

    /**
     * Construct a faction operator
     *
     * @param faction The quest that need to be assigned.
     * @param value The value
     * @param max Is the level a max.
     */
    psQuestPrereqOpFaction(Faction* faction, int value, bool max):faction(faction),value(value),max(max) {};

    /**
     * Destructor.
     */
    virtual ~psQuestPrereqOpFaction() {}

    /**
     * Check if the faction is above level.
     *
     * prerequisite = Is the faction positive.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the faction is positive.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string
     *
     * Convert the operator into the xml string:
     * \<faction name="faction name" value="faction value"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Inventory prerequisite operator.
 *
 * The given item must be equiped or just in inventory (includes the first)
 * for this prerequisite to be true.
 */
class psQuestPrereqOpItem : public psQuestPrereqOp
{
protected:
    csString itemName;
    csString categoryName;
    bool includeInventory;
    float qualityMin;
    float qualityMax;

public:

    /**
     * Construct an inventory operator.
     *
     * @param itemName The name of the base item we are searching for.
     * @param categoryName The name of the category.
     * @param includeInventory if true it will search either equipment and inventory
     *                         else only inventory.
     * @param qualityMin A minimum quality.
     * @param qualityMax A maximum quality
     */
    psQuestPrereqOpItem(const char* itemName, const char* categoryName, bool includeInventory, float qualityMin, float qualityMax):
        itemName(itemName), categoryName(categoryName), includeInventory(includeInventory),
        qualityMin(qualityMin), qualityMax(qualityMax) {};

    /**
     * Destructor.
     */
    virtual ~psQuestPrereqOpItem() {}

    /**
     * Check if the specified item is in the player inventory/equipment.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the item was found.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<item inventory="true/false" name="baseitemname"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Active magic prerequisite operator.
 *
 * The actor must have a certain active magic (buff or debuff).
 */
class psQuestPrereqOpActiveMagic : public psQuestPrereqOp
{
protected:
    csString activeMagic;

public:

    /**
     * Construct an active magic operator.
     *
     * @param activeMagic The name of the magic that's required to be active.
     */
    psQuestPrereqOpActiveMagic(const char* activeMagic):activeMagic(activeMagic) {};

    virtual ~psQuestPrereqOpActiveMagic() {}

    /**
     * Check if the specified magic is active.
     *
     * @param  character The character that are checking for a prerequisite.
     * @return True if the magic is active.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<activemagic name="-activemagic"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Trait prerequisite operator.
 *
 * The actor must have a certain Trait in a certain position.
 */
class psQuestPrereqOpTrait : public psQuestPrereqOp
{
protected:
    csString traitName;
    PSTRAIT_LOCATION traitLocation;

public:

    /**
     * Construct a Trait operator.
     *
     */
    psQuestPrereqOpTrait(const char* traitName, csString traitLocationString):traitName(traitName)
    {
        for(int position = PSTRAIT_LOCATION_NONE; position < PSTRAIT_LOCATION_COUNT; position++)
        {
            if((csString)psTrait::locationString[position] == traitLocationString)
            {
                traitLocation = (PSTRAIT_LOCATION) position;
                break;
            }
        }
        // TraitLocation = (PSTRAIT_LOCATION)1;

    };

    virtual ~psQuestPrereqOpTrait() {}

    /**
     * Check if the specified trait is present.
     *
     * @param  character The character that are checking for a prerequisite.
     * @return True if the trait is present.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<activemagic name="-activemagic"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * race prerequisite operator.
 *
 * The actor must be of a certain race.
 */
class psQuestPrereqOpRace : public psQuestPrereqOp
{
protected:
    csString race;

public:

    /**
     * Construct a race operator.
     *
     * @param race The name of the race the actor is required to be.
     */
    psQuestPrereqOpRace(const char* race):race(race) {};

    virtual ~psQuestPrereqOpRace() {}

    /**
     * Check if the character is of the specific race.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the race is the one we are looking for.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<race name="racename"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Gender prerequisite operator.
 *
 * The actor must be of a certain gender.
 */
class psQuestPrereqOpGender : public psQuestPrereqOp
{
protected:
    csString gender;

public:

    /**
     * Construct a gender operator.
     *
     * @param gender The sex the character must be.
     */
    psQuestPrereqOpGender(const char* gender):gender(gender) {};

    virtual ~psQuestPrereqOpGender() {}

    /**
     * Check if the character is of the specified gender.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the gender is the one we are looking for.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<gender type="Gender Initial"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Spell knownledge prerequisite operator.
 *
 * The actor must know a certain spell.
 */
class psQuestPrereqOpKnownSpell : public psQuestPrereqOp
{
protected:
    csString spell;

public:

    /**
     * Construct a Spell Known operator.
     *
     * @param spell The spell the character must know.
     */
    psQuestPrereqOpKnownSpell(const char* spell):spell(spell) {};

    virtual ~psQuestPrereqOpKnownSpell() {}

    /**
     * Check if the character known the specified spell.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the spell we are looking for is known.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<knownspell spell="Spell Name"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Guild prerequisite operator.
 *
 * The actor must be in a certain type of guild or none.
 */
class psQuestPrereqOpGuild : public psQuestPrereqOp
{
protected:
    csString guildtype;
    csString guildName;

public:

    /**
     * Construct a guild operator.
     *
     * @param guildtype The type of guild the character must be
     * @param guildName The name of the quild.
     */
    psQuestPrereqOpGuild(const char* guildtype, const char* guildName):guildtype(guildtype),guildName(guildName) {};

    virtual ~psQuestPrereqOpGuild() {}

    /**
     * Check if the character is in a specified type of guild.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the character is in the guild type we are looking for.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<guild type="type of the guild"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Marriage prerequisite operator.
 *
 * The actor must be married or not.
 */
class psQuestPrereqOpMarriage : public psQuestPrereqOp
{
public:

    /**
     * Construct a marriage operator.
     */
    psQuestPrereqOpMarriage() {};

    virtual ~psQuestPrereqOpMarriage() {}

    /**
     * Check if the character is married or not.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if the character is married or not.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<marriage status="yes/no"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * advisor points prerequisite operator.
 *
 * The advisorpoints must be beetwen maxPoints and minPoints.
 */
class psQuestPrereqOpAdvisorPoints : public psQuestPrereqOp
{
protected:
    int minPoints, maxPoints;
    csString type;

public:

    /**
     * Construct an advisor points operator.
     *
     * @param minPoints Minimal advisor points.
     * @param maxPoints Maximal advisor points.
     * @param type Type of the check.
     */
    psQuestPrereqOpAdvisorPoints(int minPoints, int maxPoints, csString type):minPoints(minPoints),maxPoints(maxPoints),type(type) {};

    virtual ~psQuestPrereqOpAdvisorPoints() {}

    /**
     * Check if within the advisor points range.
     *
     * @param  character The character that are checking for a prerequisite.
     * @return True if in the valid range.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<advisorpoints min="-min" max="-max" type="min/max/both"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Time online time prerequisite operator.
 *
 * The time must be between mintime and maxtime.
 */
class psQuestPrereqOpTimeOnline : public psQuestPrereqOp
{
protected:
    unsigned int minTime, maxTime;

public:

    /**
     * Construct an online time operator.
     *
     * @param minTime Minimal time online.
     * @param maxTime Maximal time online.
     */
    psQuestPrereqOpTimeOnline(int minTime, int maxTime):minTime(minTime),maxTime(maxTime) {};

    virtual ~psQuestPrereqOpTimeOnline() {}

    /**
     * Check if within the time range.
     *
     * @param  character The character that are checking for a prerequisite.
     * @return True if in the valid range.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<onlinetime min="-min" max="-max" /\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Time of day prerequisite operator.
 *
 * The time must be between mintime and maxtime.
 */
class psQuestPrereqOpTimeOfDay : public psQuestPrereqOp
{
protected:
    int minTime, maxTime;

public:

    /**
     * Construct a time of the day operator.
     *
     * @param minTime Minimal time of day.
     * @param maxTime Maximal time of day.
     */
    psQuestPrereqOpTimeOfDay(int minTime, int maxTime):minTime(minTime),maxTime(maxTime) {};

    virtual ~psQuestPrereqOpTimeOfDay() {}

    /**
     * Check if within the time range.
     *
     * @param  character The character that are checking for a prerequisite.
     * @return True if in the valid range.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<timeofday min="-min" max="-max"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Variable set prerequisite operator.
 *
 * The character must have the specified variable set.
 */
class psQuestPrereqOpVariable : public psQuestPrereqOp
{
protected:
    csString variableName;

public:

    /**
     * Construct a variable operator.
     *
     * @param variableName The variable to check for assignment.
     */
    psQuestPrereqOpVariable(const char* variableName):variableName(variableName) {};

    virtual ~psQuestPrereqOpVariable() {}

    /**
     * Check if the character has the variable defined.
     *
     * @param  character The character that are checking for a prerequisite.
     * @return True if the variable we are searching for was found.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<variable name="variablename"/\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Xor prerequisite operator.
 *
 * A multi term or operator. Value of XORs between prerequisites
 * must be true.
 */
class psQuestPrereqOpXor: public psQuestPrereqOpList
{
public:

    /**
     * Destructor for the or prerequisite operator.
     */
    virtual ~psQuestPrereqOpXor() {}

    /**
     * Check if value of XORs between prerequisites is true.
     *
     * prerequisite = child1 or child 2 or ... childN
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if value of XORs between prerequisites is true.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator to the xml string:
     * \<xor\>\<child1/\>...\<childN/\>\</xor\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Skill prerequisite operator.
 *
 * The given skill level is required to be between min and max for this operator
 * to be true.
 */
class psQuestPrereqOpSkill: public psQuestPrereqOp
{
protected:
    /**
     * The skill name that is to be checked.
     */
    psSkillInfo* skill;

    int min;          ///< The minimum skill level
    int max;          ///< The maximum skill level
    bool allowBuffed; ///< Stores if we should allow buff to be taken in consideration
public:

    /**
     * Construct a skill operator.
     *
     * @param skill The skill which should be checked for.
     * @param min The minimum acceptable for this prerequisite to be true.
     * @param max The maximum acceptable for this prerequisite to be true.
     * @param allowBuffed Declares if buff should be taken in consideration.
     */
    psQuestPrereqOpSkill(psSkillInfo* skill, unsigned int min, unsigned int max, bool allowBuffed):skill(skill),min(min),max(max),allowBuffed(allowBuffed) {};

    /**
     * Destructor.
     */
    virtual ~psQuestPrereqOpSkill() {}

    /**
     * Check if the skill is in the range.
     *
     * @param  character The character that are checking for a prerequisite
     * @return True if min <= skill <= max.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the prerequisite operator to a xml string.
     *
     * Convert the operator into the xml string:
     * \<skill name="skill name" min="0" max="0" allowbuffed="false" /\>
     *
     * @return XML string for the prerequisite operator.
     */
    virtual csString GetScriptOp();

    /**
     * Copy the prerequisite operator.
     *
     * Override this function to return a copy of the prerequisite
     * operator.
     *
     * @return Copy of the prerequisite operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();
};

/**
 * Weapon requirement operator.
 *
 * Checks for various attack type specs
 * atm this is really only useable in the combat system.
 */
class psPrereqOpAttackType: public psQuestPrereqOp
{
 protected:
     psAttackType* attackType; ///< The required attack Type

 public:

    /**
     * Construct an attack type
     *
     */
    psPrereqOpAttackType(psAttackType* attackType) : attackType(attackType) {}

    /**
     * Destructor
     */
    virtual ~psPrereqOpAttackType() {}

    /**
     * Check if the attack typespecification is correct
     *
     * @param  character The character that are checking for a requirement
     * @return True if specs are correct.
     */
    virtual bool Check(psCharacter* character);

    /**
     * Convert the requirement operator to a xml string
     *
     * Convert the operator into the xml string:
     * <attacktype name = "[attack type name]" />
     *
     * @return XML string for the requirement operator.
     */
    virtual csString GetScriptOp();


    /**
     * Copy the requirement operator
     *
     * Override this function to return a copy of the requirement
     * operator.
     *
     * @return Copy of the requirement operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();

private:
    /**
     * Checks weapon's name, if specified
     *
     */
    bool checkWeapon(psCharacter* character, int slot);

    
    /**
     * Checks weapon's type, if specified
     *
     */
    bool checkWType(psCharacter* character, psItem* weapon);
};


/**
 * Stance requirement operator.
 *
 * Checks for various stances
 */
class psPrereqOpStance: public psQuestPrereqOp
{
 protected:
     csString stance;

 public:

    /**
     * Construct a stance operator
     *
     * @param stance The stance which should be checked for.
     */
    psPrereqOpStance(csString stance):stance(stance){};

    /**
     * Destructor
     */
    virtual ~psPrereqOpStance() {}


    /**
     * Check if the player stance is correct
     *
     * @param  character The character that are checking for a requirement
     * @return True if specs are correct.
     */
    virtual bool Check(psCharacter * character);


    /**
     * Convert the requirement operator to a xml string
     *
     * Convert the operator into the xml string:
     * <stance name = "stance name"/>
     *
     * @return XML string for the requirement operator.
     */
    virtual csString GetScriptOp();



    /**
     * Copy the requirement operator
     *
     * Override this function to return a copy of the requirement
     * operator.
     *
     * @return Copy of the requirement operator.
     */
    virtual csPtr<psQuestPrereqOp> Copy();

};



#endif
