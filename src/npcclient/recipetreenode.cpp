/*
* recipetreenode.cpp             by Zee (RoAnduku@gmail.com)
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

#include <psconfig.h>
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
#include "recipetreenode.h"

const char* RecipeTreeNode::RequirementParseTypeString[] = {"REQ_CONCENTRATED","REQ_DISTRIBUTED"};


RecipeTreeNode::RecipeTreeNode(Recipe* newRecipe, int newCost, RecipeTreeNode* parent):
    parent(NULL)
{
    // General inits
    recipe     = newRecipe;
    cost       = newCost; // TODO -- Make it work
    nextStep   = 0;
    wait       = 0;

    if(!parent)
    {
        priority = 0;
    }
    else
    {
        priority = 1 + parent->priority;
    }

    // Requirement parsing inits
    nextReq    = 0;
    requirementParseType = REQ_CONCENTRATED;
}

RecipeTreeNode::~RecipeTreeNode()
{
    while(!children.IsEmpty())
    {
        delete children.Pop();
    }
}

void RecipeTreeNode::AddChild(RecipeTreeNode* child)
{
    children.Push(child);
    // Each level in the tree has one higher priority,
    // all nodes on same level will have same priority.
    child->priority = priority + 1;
}

bool RecipeTreeNode::AddChild(RecipeTreeNode* child, Recipe* parent)
{
    if(recipe == parent)
    {
        AddChild(child);
        return true;
    }
    for(size_t i=0; i<children.GetSize(); i++)
    {
        if(children[i]->AddChild(child,parent))
            return true;
    }
    return false;
}

bool RecipeTreeNode::RemoveChild(RecipeTreeNode* child)
{
    for(size_t i=0; i<children.GetSize(); i++)
    {
        if(children[i] == child)
        {
            children.Delete(child);
            delete child;
            return true;
        }
        if(children[i]->RemoveChild(child))
        {
            return true;
        }
    }
    return false;
}

bool RecipeTreeNode::RemoveChild(Recipe* child)
{
    for(size_t i=0; i<children.GetSize(); i++)
    {
        if(children[i]->recipe == child)
        {
            // For persistent recipes just reset steps
            if(children[i]->recipe->IsPersistent())
            {
                children[i]->nextStep = 0;
                children[i]->nextReq = 0;
                return true;
            }
            RemoveChild(children[i]);
            return true;
        }
        if(children[i]->RemoveChild(child))
            return true;
    }
    return false;
}

RecipeTreeNode* RecipeTreeNode::GetTreeRecipe(Recipe* searchRecipe)
{
    if(recipe == searchRecipe)
    {
        return this;
    }
    RecipeTreeNode* search;
    for(size_t i=0; i<children.GetSize(); i++)
    {
        search = children[i]->GetTreeRecipe(searchRecipe);
        if(search)
            return search;
    }
    return NULL;
}

RecipeTreeNode* RecipeTreeNode::GetNextRecipe()
{
    if(IsLeaf())
    {
        return this;
    }

    csArray<RecipeTreeNode*> queue;
    RecipeTreeNode* bestNode = this;
    RecipeTreeNode* node;

    queue.Push(this);

    while(!queue.IsEmpty())
    {
        // Investigate next children
        node = queue.Pop();

        // Select best recipe
        if(node->IsLeaf())
        {
            if(node->wait < bestNode->wait)
            {
                bestNode = node;
            }
            else if(node->wait == bestNode->wait && node->priority > bestNode->priority)
            {
                bestNode = node;
            }
        }

        for(size_t i=0; i<node->children.GetSize(); i++)
        {
            queue.Push(node->children[i]);
        }
    }

    return bestNode == this ? NULL : bestNode;
}

void RecipeTreeNode::UpdateWaitTimes(int delta)
{
    // Substract the delta from the wait
    wait -= delta;
    if(wait < 0)
    {
        wait = 0;
    }

    for(size_t i=0; i<children.GetSize(); i++)
    {
        children[i]->UpdateWaitTimes(delta);
    }
}

bool RecipeTreeNode::ModifyWait(Recipe* theRecipe, int delta)
{
    if(recipe == theRecipe)
    {
        // Add the delta to the waiting time.
        wait += delta;
        if(wait < 0)
        {
            wait = 0;
        }
        return true;
    }
    for(size_t i=0; i<children.GetSize(); i++)
    {
        if(children[i]->ModifyWait(theRecipe,delta))
        {
            return true;
        }
    }
    return false;
}

void RecipeTreeNode::DumpRecipeTree(int index)
{
    CPrintf(CON_CMDOUTPUT, "|%4d|%4d|%30s|%4d|%4d|%4d|%4d|\n",
            index,
            recipe->GetID(),
            recipe->GetName().GetData(),
            recipe->IsPersistent(),
            priority,
            wait,
            nextStep);
    for(size_t i=0; i<children.GetSize(); i++)
    {
        index++;
        children[i]->DumpRecipeTree(index);
    }
}

void RecipeTreeNode::DumpRecipeTreeRecipes(int index)
{
    CPrintf(CON_NOTIFY,"TreeNode at %d\n",index);
    CPrintf(CON_NOTIFY,"RequirementParseType: %s\n", RequirementParseTypeString[requirementParseType]);
    recipe->Dump();
    CPrintf(CON_NOTIFY, "\n");
    for(size_t i=0; i<children.GetSize(); i++)
    {
        index++;
        children[i]->DumpRecipeTreeRecipes(index);
    }
}
