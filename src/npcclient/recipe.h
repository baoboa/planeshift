/*
* recipe.h
*
* Copyright (C) 2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef __RECIPE_H__
#define __RECIPE_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
#include <csutil/stringarray.h>
#include <csutil/csstring.h>
//=============================================================================
// Project Includes
//=============================================================================
#include <util/psdatabase.h>
#include <util/psutil.h>
//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "tribe.h"
#include "recipetreenode.h"
#include "npcbehave.h"


/**
 * \addtogroup npcclient
 * @{ */

class psNPCClient;
class EventManager;
class RecipeTreeNode;

/**
 * This object represents recipes for the tribe AI.
 */

class Recipe
{
public:

    /** Types of requirements */
    typedef enum
    {
        REQ_TYPE_BUILDING,
        REQ_TYPE_ITEM,
        REQ_TYPE_KNOWLEDGE,
        REQ_TYPE_MEMORY,
        REQ_TYPE_RECIPE,
        REQ_TYPE_RESOURCE,
        REQ_TYPE_TRADER,
        REQ_TYPE_TRIBESMAN
    } RequirementType;
    static const char* RequirementTypeString[];

    /** Data structure to keep information about requirements */
    struct Requirement
    {
        RequirementType type;          ///< Type. Can be workforce, resource, item, knowledge
        csString        name;          ///< Name of the requirement.
        csString        quantity;      ///< Number needed.
        csString        recipe;        ///< If requirement isn't meet use this recipe to get it.
        csString        buffer;        ///< Use this buffer to store name for recipe to use later.
        csString        reqText;       ///< The unparsed requirment function for this.
    };

    /** Construct a Recipe object */
    Recipe();

    /** Destruct a Recipe object */
    virtual ~Recipe() { };

    /** Load a recipe */
    bool Load(iResultRow &row);

    /** Dump all details about the recipe */
    void Dump();

    /** Dumps algorithm to console */
    void DumpAlgorithm();

    /** Dumps requirements to console */
    void DumpRequirements();

    /** Getter for Recipe's ID */
    int GetID()
    {
        return id;
    }

    /** Getter for Recipe's Name */
    csString GetName()
    {
        return name;
    }

    /** Getter for Recipe's Algorithm */
    csStringArray GetAlgorithm()
    {
        return algorithm;
    }

    /** Getter for Persistance */
    bool IsPersistent()
    {
        return persistence;
    }

    /** Getter for Requirements */
    csArray<Requirement> GetRequirements()
    {
        return requirements;
    }

private:
    bool                    persistence;  ///< True if recipe never ends
    bool                    unique;       ///< True if recipe is unique
    int                     id;
    csString                name;
    csArray<Requirement>    requirements; ///< Keeps all the requirements for this recipe.
    csStringArray           algorithm;    ///< Keeps the steps of the algorithm.
    int                     steps;        ///< Number of steps the algorithm has.
};

/**
 * Class that represents the Recipe Manager of the game.
 * This object loads and stores all the recipes in-game. It also
 * keeps track of the progress the tribes have in their recipes
 * and triggers changes in tribes once recipes are completed.
 */
class RecipeManager
{
public:
    /** Keeps details of all tribes enrolled in the manager */
    struct TribeData
    {
        int     tribeID;
        Recipe* tribalRecipe; ///< Recipe keeping tribe information
        int     currentStep;  ///< Keeps the step the tribe is currently at in the recipe.

        // Tribe Traits
        csString aggressivity;
        csString brain;
        csString growth;
        csString unity;
        csString sleepPeriod;

        // Global Tribe NPCType
        csString global;                ///< Holds the parent NPCType available for the whole tribe
    };

    /** Keeps details about the matches between script actions and NPC Operations */
    struct Correspondence
    {
        csString      function;        ///< Function written in the database
        csStringArray npcOperations;   ///< More than one operation can be applied for a function
    };

    /** Construct the Recipe Manager */
    RecipeManager(psNPCClient* NPCClient, EventManager* eventManager);

    /** Destruct the Recipe Manager */
    virtual ~RecipeManager();

    /** Preparser
     *
     * Method to preparse the scripts in recipes in order
     * to replace variables used while scripting.
     * E.g. GROW_RESOURCE, HOME
     *
     * @param function  The function to preparse
     * @param tribe     The tribe from which values are needed
     * @return          The preparsed text with actual values
     */
    csString Preparse(csString function, Tribe* tribe);

    /** Loads all recipes from the database */
    bool LoadRecipes();

    /** Add a tribe
     *
     * Method to load a tribe pointer into the tribes list
     * held by the manager.
     * Required for all tribes we want the manager to hold.
     *
     * @param tribe The tribe to be added.
     * @return True if the tribe where added successfully
     */
    bool AddTribe(Tribe* tribe);

    /** Compute global npctype
     *
     * Computes a global part of an npctype for each tribe. The global
     * npctype contains general/common data concerning tribe members.
     * E.g. : Behavior when attacked, behavior during day/night
     *        + reactions that change to above-mentioned behaviors
     * After assembling the NPCType it sends the string to the npcclient
     * who creates a new object and pushes into npctypes hash.
     *
     * @param tribe       Pointer to the tribe on which to create NPCType
     */
    void CreateGlobalNPCType(Tribe* tribe);

    /** Parse function
     *
     * Applies changes to the tribe based on the function
     * received as an argument.
     *
     * @param function    String containing a function and it's arguments
     * @param tribe       The tribe on which to apply the changes
     * @param npcs        The list of npcs to send perceptions to
     * @param recipe      The recipe this function belongs to
     * @return            True if function is done, false if it needs wait time.
     */
    bool ParseFunction(csString function, Tribe* tribe, csArray<NPC*> &npcs, Recipe* recipe);

    /** Check Requirement
     *
     * Checks a requirement.
     * For unmet requirements this function enrolls needed
     * recipes in order to meet the requirements.
     *
     * @param requirement Data structure containing requirement
     * @param tribe       Tribe to check requirements on
     * @param recipe      The recipe this requirement belongs to
     * @return            True if all requirements are met, false otherwise.
     */
    bool ParseRequirement(const Recipe::Requirement &requirement, Tribe* tribe, Recipe* recipe);

    /** Apply Recipe
     *
     * Applies a recipe to a tribe.
     *
     * @param bestRecipe The recipeTreeNode to apply
     * @param tribe      The tribe on which to apply
     * @param step       The step on which to start
     * @return           The step on which the algorithm stopped, or -1 if completed.
     */
    int ApplyRecipe(RecipeTreeNode* bestRecipe, Tribe* tribe, int step);

    /** Get pointer to recipe based on ID */
    Recipe* GetRecipe(int recipeID);

    /** Get pointer to recipe based on Recipe's Name */
    Recipe* GetRecipe(csString recipesName);

    /** Get computed tribeData for a tribe */
    TribeData* GetTribeData(Tribe* tribe);

private:
    /** Dump Error */
    void DumpError(csString recipeName, csString functionName, int expectedArgs, int receivedArgs);

    csArray<Tribe*>         tribes;    ///< Tribes enrolled to the manager
    csArray<TribeData>      tribeData; ///< Recipe data about tribes
    csArray<Recipe*>        recipes;   ///< List of all recipes in-game.

    psNPCClient*            npcclient;    ///< Link to npcclient
    EventManager*           eventmanager; ///< Link to event manager
};

/** @} */

#endif
