/*
* recipetreenode.h
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

#ifndef __RECIPETREENODE_H__
#define __RECIPETREENODE_H__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/array.h>
//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "tribe.h"
#include "recipe.h"

/**
 * \addtogroup npcclient
 * @{ */

class Recipe;

#define CYCLIC_RECIPE_PRIORITY 100

/**
 * This class represents a leaf in a recipe tree.
 * The recipe tree is used by tribes to decide which action to take and in which order.
 * It's more useful than a normal array because the recipes we want to execute first are
 * nodes with no wait time and no children. Besides, it keeps the relations between recipes.
 */

class RecipeTreeNode
{
public:
    typedef enum
    {
        REQ_CONCENTRATED,
        REQ_DISTRIBUTED
    } RequirementParseType;
    static const char* RequirementParseTypeString[];


    Recipe*                  recipe;     ///< The recipe this leaf is about
    int                      priority;   ///< The priority of this recipe. (Level in tree, where root = 0)
    int                      wait;       ///< Wait time
    int                      nextStep;   ///< The next step which needs to be executed
    int                      cost;       ///< Computed cost for this recipe. In regard of tribe.

    RequirementParseType     requirementParseType; ///< Holds the way the Recipe Manager should meet requirements
    int                      nextReq;    ///< Next requirement that should be met

    RecipeTreeNode*          parent;   ///< Link to parent node
    csArray<RecipeTreeNode*> children; ///< Link to children nodes

    /** Constructor */
    RecipeTreeNode(Recipe* newRecipe, int newCost, RecipeTreeNode* parent = NULL);
    ~RecipeTreeNode();

    /** Add a new child */
    void AddChild(RecipeTreeNode* child);

    /** Add a new child to a parent descending from this node. True if successful */
    bool AddChild(RecipeTreeNode* child, Recipe* parent);

    /** Remove a descendant */
    bool RemoveChild(RecipeTreeNode* child);

    /** Remove a child based on recipe data. True if successful */
    bool RemoveChild(Recipe* child);

    /** Returns true if it's root */
    bool IsRoot()
    {
        return parent == NULL;
    };

    /** Returns true if it's a leaf */
    bool IsLeaf()
    {
        return children.GetSize() == 0;
    }

    /** Gets a recipe from this tree */
    RecipeTreeNode* GetTreeRecipe(Recipe* searchRecipe);

    /**
     * Gets Next Recipe. (highest level leaf with no wait time)
     * It's best to use it on the root node, since it runs a BFS and can ignore recipes otherwise.
     */
    RecipeTreeNode* GetNextRecipe();

    /** Updates a wait time on one recipe */
    bool ModifyWait(Recipe* theRecipe, int delta);

    /** Update wait times */
    void UpdateWaitTimes(int delta);

    /** Dump tree to console */
    void DumpRecipeTree()
    {
        DumpRecipeTree(1);
    }
    void DumpRecipeTree(int index);
    void DumpRecipeTreeRecipes()
    {
        DumpRecipeTreeRecipes(1);
    }
    void DumpRecipeTreeRecipes(int index);
};

/** @} */

#endif
