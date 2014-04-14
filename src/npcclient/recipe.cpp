/*
* recipe.cpp             by Zee (RoAnduku@gmail.com)
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
#include <stdlib.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/stringarray.h>
#include <csutil/xmltiny.h>
#include <iutil/object.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/psdatabase.h"
#include "util/log.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "recipe.h"
#include "npc.h"

const char* Recipe::RequirementTypeString[] =
{
    "REQ_TYPE_BUILDING",
    "REQ_TYPE_ITEM",
    "REQ_TYPE_KNOWLEDGE",
    "REQ_TYPE_MEMORY",
    "REQ_TYPE_RECIPE",
    "REQ_TYPE_RESOURCE",
    "REQ_TYPE_TRADER",
    "REQ_TYPE_TRIBESMAN"
};


Recipe::Recipe()
{
    id          = -1;
    name        = "";
    persistence = false;
    unique      = false;
}

csStringArray SplitFunctions(csString buffer)
{
    csStringArray result;
    csStringArray lines;
    lines.SplitString(buffer, "\n");
    for(size_t i=0; i<lines.GetSize(); i++)
    {
        csString line = lines.Get(i);

        if(line.IsEmpty())
        {
            continue; // Skip blank lines
        }
        if(line.GetAt(0) == '#')
        {
            continue; // Skip lines starting with a comment
        }

        csStringArray functions;
        functions.SplitString(line, ";");
        for(size_t j=0; j<functions.GetSize(); j++)
        {
            csString function = functions.Get(j);
            function.Trim();
            if(function.IsEmpty())
            {
                continue; // Skip empty functions
            }
            result.Push(function);
        }
    }
    return result;
}


bool Recipe::Load(iResultRow &row)
{
    csStringArray unparsedReq;
    csString      reqText;
    int pers     = row.GetInt("persistent");
    int isUnique = row.GetInt("uniqueness");
    id           = row.GetInt("id");
    name         = row["name"];

    // Set persistance
    if(pers == 1) persistence = true;

    // Set unique
    if(isUnique == 1) unique = true;

    // Load algorithm
    algorithm = SplitFunctions(row["algorithm"]);

    // Load requirements
    unparsedReq = SplitFunctions(row["requirements"]);

    // Parse Requirements
    for(size_t i=0; i<unparsedReq.GetSize(); i++)
    {
        Requirement newReq;
        newReq.reqText = unparsedReq.Get(i);

        // Split into function & argument 1 & argument 2
        csStringArray explodedReq;
        explodedReq.SplitString(newReq.reqText, "(,)");
        size_t index = 0;
        reqText = explodedReq.Get(index++);

        // Flags used to check parameters
        bool recipeRequired = false;
        bool bufferRequired = false;

        // And check its contents
        if(reqText == "building")
        {
            newReq.type = Recipe::REQ_TYPE_BUILDING;
            recipeRequired = true;
        }
        else if(reqText == "tribesman")
        {
            newReq.type = Recipe::REQ_TYPE_TRIBESMAN;
            recipeRequired = true;
        }
        else if(reqText == "resource")
        {
            newReq.type = Recipe::REQ_TYPE_RESOURCE;
            recipeRequired = true;
            bufferRequired = true;
        }
        else if(reqText == "item")
        {
            newReq.type = Recipe::REQ_TYPE_ITEM;
            recipeRequired = true;
        }
        else if(reqText == "knowledge")
        {
            newReq.type = Recipe::REQ_TYPE_KNOWLEDGE;
            recipeRequired = true;
        }
        else if(reqText == "recipe")
        {
            newReq.type = Recipe::REQ_TYPE_RECIPE;
        }
        else if(reqText == "trader")
        {
            newReq.type = Recipe::REQ_TYPE_TRADER;
        }
        else if(reqText == "memory")
        {
            newReq.type = Recipe::REQ_TYPE_MEMORY;
            recipeRequired = true;
        }
        else
        {
            Error2("Unknown recipe requirement: %s. Bail.", explodedReq.Get(0));
            exit(1);
        }

        newReq.name = explodedReq.Get(index++);
        newReq.quantity = explodedReq.Get(index++);
        if(recipeRequired)
        {
            if(explodedReq.GetSize() > index)
            {
                newReq.recipe = explodedReq.Get(index++);
            }
            else
            {
                // We requried a recipe, but none where given.
                Error3("Recipe %s requirements require recipe: %s", name.GetDataSafe(), explodedReq.Get(0));
                return false;
            }
        }
        if(bufferRequired)
        {
            if(explodedReq.GetSize() > index)
            {
                newReq.buffer = explodedReq.Get(index++);
            }
            else
            {
                // We requried a recipe, but none where given.
                Error3("Recipe %s requirements require buffer: %s", name.GetDataSafe(), explodedReq.Get(0));
                return false;
            }
        }

        requirements.Push(newReq);
    }

    return true;
}

void Recipe::Dump()
{
    CPrintf(CON_NOTIFY, "Dumping recipe %d.\n", id);
    CPrintf(CON_NOTIFY, "Name: %s \n", name.GetData());
    DumpRequirements();
    DumpAlgorithm();
    CPrintf(CON_NOTIFY, "Persistent: %d \n", persistence);
    CPrintf(CON_NOTIFY, "Unique: %d \n", unique);
}

void Recipe::DumpAlgorithm()
{
    CPrintf(CON_NOTIFY, "Algorithms:\n");
    if(algorithm.GetSize())
    {
        for(size_t i=0; i<algorithm.GetSize(); i++)
        {
            CPrintf(CON_NOTIFY, "%d) %s\n", (i+1), algorithm.Get(i));
        }
    }
    else
    {
        CPrintf(CON_NOTIFY, "None.\n");
    }
}

void Recipe::DumpRequirements()
{
    CPrintf(CON_NOTIFY, "Requirements:\n");
    if(requirements.GetSize())
    {
        for(size_t i=0; i<requirements.GetSize(); i++)
        {
            CPrintf(CON_NOTIFY, "%d) %s\n", (i+1), requirements[i].reqText.GetData());
        }
    }
    else
    {
        CPrintf(CON_NOTIFY, "None.\n");
    }
}


RecipeManager::RecipeManager(psNPCClient* NPCClient, EventManager* eventManager)
{
    npcclient    = NPCClient;
    eventmanager = eventManager;
}

RecipeManager::~RecipeManager()
{
    while(!recipes.IsEmpty())
    {
        Recipe* recipe = recipes.Pop();
        delete recipe;
    }
}

csString RecipeManager::Preparse(csString function, Tribe* tribe)
{
    csString container;

    tribe->ReplaceBuffers(function);

    container = "";
    container.Append(tribe->GetReproductionCost());
    function.ReplaceAll("$REPRODUCTION_COST", container.GetData());

    container = "";
    container.Append(tribe->GetNeededResource());
    function.ReplaceAll("$REPRODUCTION_RESOURCE", container.GetData());

    container = "";
    container.Append(tribe->GetNeededResourceAreaType());
    function.ReplaceAll("$RESOURCE_AREA", container.GetData());

    return function;
}

bool RecipeManager::LoadRecipes()
{
    Result rs(db->Select("SELECT * FROM tribe_recipes"));

    if(!rs.IsValid())
    {
        Error2("Could not load recipes from the database. Reason: %s", db->GetLastError());
        return false;
    }

    for(size_t i=0; i<rs.Count(); i++)
    {
        Recipe* newRecipe = new Recipe;
        recipes.Push(newRecipe);
        newRecipe->Load(rs[i]);
    }
    CPrintf(CON_NOTIFY, "Loaded %d recipes.\n",rs.Count());
    return true;
}

bool RecipeManager::AddTribe(Tribe* tribe)
{
    TribeData newTribe;
    newTribe.tribeID = tribe->GetID();

    // Get Tribal Recipe
    newTribe.tribalRecipe = tribe->GetTribalRecipe();

    csStringArray tribeStats = newTribe.tribalRecipe->GetAlgorithm();
    csStringArray keywords;
    csString      keyword;

    // Get Tribe Stats
    for(size_t i=0; i<tribeStats.GetSize(); i++)
    {
        keywords.SplitString(tribeStats.Get(i), "(,)");
        keyword = keywords.Get(0);

        if(keyword == "aggressivity")
        {
            newTribe.aggressivity = keywords.Get(1);
        }
        else if(keyword == "brain")
        {
            newTribe.brain = keywords.Get(1);
        }
        else if(keyword == "growth")
        {
            newTribe.growth = keywords.Get(1);
        }
        else if(keyword == "unity")
        {
            newTribe.unity = keywords.Get(1);
        }
        else if(keyword == "sleepPeriod")
        {
            newTribe.sleepPeriod = keywords.Get(1);
            if((newTribe.sleepPeriod != "diurnal") &&
                    (newTribe.sleepPeriod != "nocturnal") &&
                    (newTribe.sleepPeriod != "nosleep"))
            {
                Error2("Recipe sleepPeriod(%s) with unkown period. Use: diurnal|nocturnal|nosleep",keywords.Get(1));
                return false;
            }

        }
        else if(keyword == "loadRecipe")
        {
            Recipe* newRecipe = GetRecipe(csString(keywords.Get(1)));

            if(!newRecipe)
            {
                Error2("Recipe not found for loadRecipe(%s)",keywords.Get(1));
                return false;
            }

            if(strncmp(keywords.Get(2), "distributed", 11) == 0)
            {
                tribe->AddRecipe(newRecipe, newTribe.tribalRecipe, true);
            }
            else
            {
                tribe->AddRecipe(newRecipe, newTribe.tribalRecipe, false);
            }
        }
        else
        {
            Error2("Unknown tribe stat: '%s'. Abandon ship.", keywords[0]);
            return false;
        }

        keywords.DeleteAll();
    }

    // Add structure to array
    tribeData.Push(newTribe);

    // Add tribes pointer to RM's array
    tribes.Push(tribe);

    // Create Tribal NPCType
    CreateGlobalNPCType(tribe);

    return true;
}

bool RecipeManager::ParseFunction(csString function, Tribe* tribe, csArray<NPC*> &npcs, Recipe* recipe)
{
    RDebug(tribe, 5, "Parse Function: '%s'",function.GetDataSafe());

    function = Preparse(function, tribe);

    RDebug(tribe, 5, "  - Preparsed: '%s'",function.GetDataSafe());

    csStringArray functionParts;
    csString      functionBody;
    csStringArray functionArguments;

    functionParts.SplitString(function, "()");
    functionBody = functionParts.Get(0);
    functionArguments.SplitString(functionParts.Get(1), ",");

    // Due split, empty items may be in the array
    for(size_t i=0; i<functionArguments.GetSize(); i++)
    {
        if(strlen(functionArguments.Get(i)) == 0)
            functionArguments.DeleteIndex(i);
    }

    int argSize = functionArguments.GetSize();

    // On each of the following conditions we check function number and arguments
    // and then apply the function effect

    if(functionBody == "aggressivity")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        TribeData* data = GetTribeData(tribe);
        data->aggressivity = functionArguments.Get(0);
        return true;
    }
    else if(functionBody == "alterResource")
    {
        if(argSize != 2)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 2, argSize);
            return false;
        }
        tribe->AddResource(functionArguments.Get(0), atoi(functionArguments.Get(1)));
        return true;
    }
    else if(functionBody == "brain")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        TribeData* data = GetTribeData(tribe);
        data->brain = functionArguments.Get(0);
        return true;
    }
    else if(functionBody == "growth")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        TribeData* data = GetTribeData(tribe);
        data->growth = functionArguments.Get(0);
        return true;
    }
    else if(functionBody == "unity")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        TribeData* data = GetTribeData(tribe);
        data->unity = functionArguments.Get(0);
        return true;
    }
    else if(functionBody == "sleepPeriod")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        TribeData* data = GetTribeData(tribe);
        data->sleepPeriod = functionArguments.Get(0);
        return true;
    }
    else if(functionBody == "loadLocation")
    {
        if(argSize != 4)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 4, argSize);
            return false;
        }
        csVector3 whatLocation;
        whatLocation[0] = atoi(functionArguments.Get(0));
        whatLocation[1] = atoi(functionArguments.Get(1));
        whatLocation[2] = atoi(functionArguments.Get(2));
        tribe->AddMemory("work",whatLocation,
                         tribe->GetHomeSector(),
                         10,NULL);

        return true;
    }
    else if(functionBody == "goWork")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 4, argSize);
            return false;
        }
        // Set the workers buffers with how many seconds they should work
        csString duration;
        duration.Append(atoi(functionArguments.Get(0)));
        printf("Work %s seconds.\n", duration.GetData());
        for(size_t i=0; i<npcs.GetSize(); i++)
        {
            npcs[i]->SetBuffer("Work_Duration",duration);
        }
        tribe->SendPerception("tribe:work", npcs);
        return true;
    }
    else if(functionBody == "bogus")
    {
        if(argSize != 0)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 0, argSize);
        }
        // The 'Slack' Function
        return true;
    }
    else if(functionBody == "loadCyclicRecipe")
    {
        if(argSize != 2)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 2, argSize);
            return false;
        }
        tribe->AddCyclicRecipe(GetRecipe(functionArguments.Get(0)),
                               atoi(functionArguments.Get(1)));

        return true;
    }
    else if(functionBody == "locateMemory" || functionBody == "locateResource")
    {
        if(argSize != 2)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 2, argSize);
            return false;
        }
        if(!tribe->LoadNPCMemoryBuffer(functionArguments.Get(0), npcs))
        {
            // No such memory, then explore for it.
            tribe->SetBuffer("Buffer", functionArguments.Get(0));
            tribe->AddRecipe(GetRecipe(functionArguments.Get(1)), recipe);
            return false;
        }
        return true;
    }
    else if(functionBody == "locateBuildingSpot")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }

        Tribe::Asset* spot = tribe->GetAsset(Tribe::ASSET_TYPE_BUILDINGSPOT, functionArguments.Get(0), Tribe::ASSET_STATUS_NOT_USED);

        if(!spot)
        {
            // Panic, no spot reserved for this building
            return false;
        }

        spot->status = Tribe::ASSET_STATUS_INCONSTRUCTION;

        for(size_t i=0; i<npcs.GetSize(); i++)
        {
            npcs[i]->SetBuildingSpot(spot);
            npcs[i]->SetBuffer("Building",functionArguments.Get(0));
            npcs[i]->SetBuffer("Work_Duration","10");
        }

        return true;
    }
    else if(functionBody == "addKnowledge")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        tribe->AddKnowledge(functionArguments.Get(0));
        return true;
    }
    else if(functionBody == "reserveSpot")
    {
        if(argSize != 4)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 4, argSize);
            return false;
        }
        csVector3 where;
        const char* buildingName = functionArguments.Get(3);
        where[0] = atoi(functionArguments.Get(0));
        where[1] = atoi(functionArguments.Get(1));
        where[2] = atoi(functionArguments.Get(2));
        iSector* sector = NULL; // Position for spots are delta values from tribe home so sector dosn't apply.

        if(!tribe->GetAsset(Tribe::ASSET_TYPE_BUILDINGSPOT, buildingName, where, sector))
        {
            tribe->AddAsset(Tribe::ASSET_TYPE_BUILDINGSPOT, buildingName, where, sector, Tribe::ASSET_STATUS_NOT_USED);
        }

        return true;
    }
    else if(functionBody == "attack")
    {
        if(argSize != 0)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 0, argSize);
            return false;
        }
        tribe->SendPerception("tribe:attack", npcs);
        return true;
    }
    else if(functionBody == "gather")
    {
        if(argSize != 0)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 0, argSize);
            return false;
        }
        tribe->SendPerception("tribe:gather", npcs);
        return true;
    }
    else if(functionBody == "mine")
    {
        if(argSize != 0)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 0, argSize);
            return false;
        }
        tribe->SendPerception("tribe:mine", npcs);
        return true;
    }
    else if(functionBody == "select")
    {
        if(argSize != 2)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 2, argSize);
            return false;
        }
        npcs.Empty();
        npcs = tribe->SelectNPCs(functionArguments.Get(0), functionArguments.Get(1));

        csString str;
        for(size_t i=0; i<npcs.GetSize(); i++,str+=", ")
        {
            str += npcs[i]->GetName() + csString("(") + csString(ShowID(npcs[i]->GetEID())) + csString(")");
        }
        RDebug(tribe, 5, "Selected NPCs: %s",str.GetDataSafe());

        return true;
    }
    else if(functionBody == "explore")
    {
        if(argSize != 0)
        {
            printf("(%s)\n",functionArguments.Get(0));
            DumpError(recipe->GetName(), functionBody.GetData(), 0, argSize);
            return false;
        }

        tribe->SendPerception("tribe:explore", npcs);
        return true;
    }
    else if(functionBody == "percept")
    {
        if(argSize != 2)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 2, argSize);
            return false;
        }

        csString target = functionArguments.Get(0);
        if(target == "selection")
        {
            tribe->SendPerception(functionArguments.Get(1), npcs);
        }
        else if(target == "tribe")
        {
            tribe->SendPerception(functionArguments.Get(1));
        }
        else
        {
            Error2("Unknow target for percept: %s",target.GetDataSafe());
            return false;
        }
        return true;
    }
    else if(functionBody == "setBuffer")
    {
        if(argSize != 3)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 3, argSize);
            return false;
        }
        csString target = functionArguments.Get(0);
        if(target == "selection")
        {
            for(size_t i=0; i<npcs.GetSize(); i++)
            {
                npcs[i]->SetBuffer(functionArguments.Get(1),functionArguments.Get(2));
            }
        }
        else if(target == "tribe")
        {
            tribe->SetBuffer(functionArguments.Get(1),functionArguments.Get(2));
        }
        else
        {
            Error2("Unknow target for percept: %s",target.GetDataSafe());
            return false;
        }

        return true;
    }
    else if(functionBody == "wait")
    {
        if(argSize != 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        // Wait a number of seconds. Convert to ticks
        float time = atof(functionArguments.Get(0));
        tribe->ModifyWait(recipe, (int)(time*1000.0));

        // We return false to stop the execution
        return false;
    }
    else if(functionBody == "loadRecipe")
    {
        if(argSize < 1)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 1, argSize);
            return false;
        }
        if(argSize == 2 && (strncmp(functionArguments.Get(1),"distributed",11) == 0))
        {
            // We have a distributed requirements recipe
            tribe->AddRecipe(GetRecipe(functionArguments.Get(0)), recipe, true);
        }
        else
        {
            // We have a non distributed requirements recipe
            tribe->AddRecipe(GetRecipe(functionArguments.Get(0)), recipe, false);
        }
        // We return false because we want the loaded recipe to execute next, not the following steps
        return false;
    }
    else if(functionBody == "mate")
    {
        if(argSize != 0)
        {
            DumpError(recipe->GetName(), functionBody.GetData(), 0, argSize);
            return false;
        }
        tribe->SendPerception("tribe:breed", npcs);
        return true;
    }
    else
    {
        Error2("Unknown function primitive: %s. Bail.\n", functionBody.GetData());
        exit(1);
    }

    // Written just for common sense, will never get here... but who knows!
    return false;
}

bool RecipeManager::ParseRequirement(const Recipe::Requirement &requirement, Tribe* tribe, Recipe* recipe)
{
    csString name = Preparse(requirement.name, tribe);
    int      quantity = atoi(Preparse(requirement.quantity, tribe));

    RDebug(tribe, 5, "ParseRequirement: Type: %s Name: '%s'='%s' Quantity: '%s'='%d' Recipe: %s",
           Recipe::RequirementTypeString[requirement.type],requirement.name.GetDataSafe(),name.GetDataSafe(),
           requirement.quantity.GetDataSafe(),quantity,requirement.recipe.GetDataSafe());

    // Check type of requirement and do something for each
    if(requirement.type == Recipe::REQ_TYPE_BUILDING)
    {
        int count = tribe->AssetQuantity(Tribe::ASSET_TYPE_BUILDING, name);
        if(count >= quantity)
        {
            return true; // We have the required number of buildings
        }

        // Load receipes for each of the missing building
        for(int i=0; i<(quantity-count); i++)
        {
            tribe->AddRecipe(GetRecipe(requirement.recipe), recipe);
        }
        return false;
    }
    else if(requirement.type == Recipe::REQ_TYPE_TRIBESMAN)
    {
        if(quantity < 0)
        {
            return false;
        }

        if(tribe->CheckMembers(name, quantity))
            return true;

        // If Requirement is not met check to see
        // if more members can be spawned.
        // If they can be spawned... set the buffer to the required
        // type and add the generic recipe 'mate'
        // ~Same goes for other requirements~
        // Otherwise just wait for members to free.

        if(recipe->GetID()<5)
        {
            // Just wait for members to free for basic recipes
            return false;
        }

        if(tribe->CanGrow() && tribe->ShouldGrow())
        {
            // Start ... erm... cell-dividing :)
            tribe->SetBuffer("Buffer", name);
            tribe->AddRecipe(GetRecipe(requirement.recipe), recipe);
        }
        return false;
    }
    else if(requirement.type == Recipe::REQ_TYPE_RESOURCE)
    {
        if(tribe->CheckResource(name, quantity))
        {
            RDebug(tribe, 5, "Requirement resource %s quantity %d ok",name.GetDataSafe(),quantity);
            return true;
        }

        // Mine more resources of this kind
        tribe->SetBuffer(requirement.buffer, name);
        tribe->AddRecipe(GetRecipe(requirement.recipe), recipe);

        return false;
    }
    else if(requirement.type == Recipe::REQ_TYPE_ITEM)
    {
        int count = tribe->AssetQuantity(Tribe::ASSET_TYPE_ITEM, name);
        if(count >= quantity)
        {
            return true; // We have the required number of items
        }

        // Load receipes for each of the missing building
        for(int i=0; i<(quantity-count); i++)
        {
            tribe->AddRecipe(GetRecipe(requirement.recipe), recipe);
        }
        return false;
    }
    else if(requirement.type == Recipe::REQ_TYPE_KNOWLEDGE)
    {
        if(tribe->CheckKnowledge(name))
            return true;

        // Get this knowledge
        tribe->AddRecipe(GetRecipe(requirement.recipe), recipe);
        return false;
    }
    else if(requirement.type == Recipe::REQ_TYPE_RECIPE)
    {
        Recipe* required = GetRecipe(name);
        tribe->AddRecipe(required, recipe);
        return true;
    }
    else if(requirement.type == Recipe::REQ_TYPE_MEMORY)
    {
        float random = psGetRandom();
        bool probably = (random < ((float)quantity/100.0));

        if(probably || !tribe->FindMemory(name))
        {
            // Explore for not found memory
            tribe->AddRecipe(GetRecipe(requirement.recipe), recipe);
            return false;
        }

        return true;
    }
    else
    {
        Error2("How did type %d did even become possible? Bail.", requirement.type);
        exit(1);
    }
}

void RecipeManager::CreateGlobalNPCType(Tribe* tribe)
{
    csString assembledType;
    csString reaction;
    // Get our tribe's data
    TribeData* currentTribe = GetTribeData(tribe);

    assembledType = "<npctype name=\"tribe_";
    assembledType.Append(tribe->GetID());
    if(currentTribe->brain != "")
    {
        assembledType.Append("\" parent=\"");
        assembledType.Append(currentTribe->brain);
        assembledType.Append("\">");
    }
    else
    {
        assembledType.Append("\" parent=\"AbstractTribesman\">");
    }

    // Aggressivity Trait
    if(currentTribe->aggressivity == "warlike")
    {
        reaction = "<react event=\"player nearby\" behavior=\"aggressive_meet\" delta=\"150\" />\n";
        reaction += "<react event=\"attack\" behavior=\"aggressive_meet\" delta=\"150\"  weight=\"1\" />";
        assembledType += reaction;
    }
    else if(currentTribe->aggressivity == "neutral")
    {
        reaction = "<react event=\"player nearby\" behavior=\"peace_meet\" delta=\"100\" />\n";
        reaction += "<react event=\"attack\" behavior=\"normal_attacked\" delta=\"150\"  weight=\"1\" />";
        assembledType += reaction;
    }
    else if(currentTribe->aggressivity == "peaceful")
    {
        reaction = "<react event=\"player nearby\" behavior=\"peace_meet\" delta=\"100\" />\n";
        reaction += "<react event=\"attack\" behavior=\"coward_attacked\" delta=\"100\"  weight=\"1\" />\n";
        assembledType += reaction;
    }
    else
    {
        // This Shouldn't Happen Ever
        Error2("Error parsing tribe traits. Unknown trait: %s", currentTribe->aggressivity.GetData());
        exit(1);
    }

    // Unity Trait
    if(currentTribe->unity != "cowards")
    {
        // Tell tribe to help
        reaction = "<react event=\"attack\" behavior=\"united_attacked\" delta=\"100\" />";
        assembledType += reaction;
    }

    // Active period of the day trait
    if(currentTribe->sleepPeriod == "diurnal")
    {
        reaction = "<react event=\"time\" value=\"22,0,,,\" random=\",15,,,\" behavior=\"GoToSleep\" absolute=\"55\" />\n";
        reaction += "<react event=\"time\" value=\"6,0,,,\" random=\",15,,,\" behavior=\"WakeUp\" />";
        assembledType += reaction;
    }
    else if(currentTribe->sleepPeriod == "nocturnal")
    {
        reaction = "<react event=\"time\" value=\"8,0,,,\" random=\",15,,,\" behavior=\"GoToSleep\" absolute=\"55\"/>\n";
        reaction += "<react event=\"time\" value=\"18,0,,,\" random=\",15,,,\" behavior=\"WakeUp\" />";
        assembledType += reaction;
    }

    assembledType.Append("</npctype>");

    // Put the new NPCType into the npcclient's hashmap
    npcclient->AddNPCType(assembledType);

    // And save it into the tribeData, just in case
    currentTribe->global = assembledType;
}

int RecipeManager::ApplyRecipe(RecipeTreeNode* bestRecipe, Tribe* tribe, int step)
{
    Recipe*                      recipe       = bestRecipe->recipe;
    csArray<Recipe::Requirement> requirements = recipe->GetRequirements();
    csStringArray                algorithm    = recipe->GetAlgorithm();
    csString                     function;
    // selectedNPCs will keep the npcs required for the recipe
    // It is used so that the ParseFunction method can fire perceptions on them
    csArray<NPC*>                selectedNPCs;

    // Check all requirements
    size_t i = (bestRecipe->requirementParseType == RecipeTreeNode::REQ_DISTRIBUTED) ? bestRecipe->nextReq : 0;
    for(; i<requirements.GetSize(); i++)
    {
        RDebug(tribe, 5, "Parsing req(index:%zu) for recipe %s of type: %s\n",
               i, recipe->GetName().GetData(),
               RecipeTreeNode::RequirementParseTypeString[bestRecipe->requirementParseType]);


        if(ParseRequirement(requirements[i], tribe, recipe))
        {
            // Requirement met
            bestRecipe->nextReq = (i == (requirements.GetSize() - 1)) ? 0 : i + 1;

            RDebug(tribe, 5, "Requirement met");
        }
        else
        {
            // Keep same step for concentrated recipes... pick next one for distributed recipes
            bestRecipe->nextReq = (bestRecipe->requirementParseType == RecipeTreeNode::REQ_DISTRIBUTED) ? i + 1 : i;

            // Value code for 'Requirement not met'
            RDebug(tribe, 5, "Requirement NOT met");
            return -2;
        }
    }

    // If we got here it means requirements are met

    // Execute Algorithm
    for(size_t i=step; i<algorithm.GetSize(); i++)
    {
        function = algorithm.Get(i);
        // If algorithm step needs wait time, signal it and return the step
        if(!ParseFunction(function, tribe, selectedNPCs, recipe))
        {
            RDebug(tribe, 5, " ParseFunction - FAILED");
            return i+1;
        }
    }

    // If we reached the end of the algorithm then it is completed
    return -1;
}

Recipe* RecipeManager::GetRecipe(int recipeID)
{
    for(size_t i=0; i<recipes.GetSize(); i++)
        if(recipes[i]->GetID() == recipeID)
            return recipes[i];

    // Could not find it
    CPrintf(CON_ERROR, "Could not find recipe with id %d.\n", recipeID);
    return NULL;
}

Recipe* RecipeManager::GetRecipe(csString recipeName)
{
    for(size_t i=0; i<recipes.GetSize(); i++)
    {
        if(recipes[i]->GetName() == recipeName)
        {
            return recipes[i];
        }
    }


    // Could not find it
    CPrintf(CON_ERROR, "Could not find recipe %s.\n", recipeName.GetData());
    return NULL;
}

RecipeManager::TribeData* RecipeManager::GetTribeData(Tribe* tribe)
{
    for(size_t i=0; i<tribeData.GetSize(); i++)
        if(tribeData[i].tribeID == tribe->GetID())
            return &tribeData[i];

    return NULL;
}

void RecipeManager::DumpError(csString recipeName, csString functionName, int expectedArgs, int receivedArgs)
{
    CPrintf(CON_ERROR, "Error parsing %s. Function %s expected %d arguments and received %d.\n",
            recipeName.GetData(),
            functionName.GetData(),
            expectedArgs,
            receivedArgs);
}
