/*
* reactions.cpp by Anders Reggestad <andersr@pvv.org>
*
* Copyright (C) 2013 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <iutil/document.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/log.h"
#include "util/strutil.h"
#include "util/psutil.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "gem.h"
#include "globals.h"
#include "npc.h"
#include "npcbehave.h"
#include "npcclient.h"
#include "perceptions.h"
#include "reaction.h"

/*----------------------------------------------------------------------------*/

Reaction::Reaction():
    range(0.0f),
    factionDiff(0),
    activeOnly(false),
    inactiveOnly(false),
    reactWhenInvisible(false),
    reactWhenInvincible(false),
    desireType(DESIRE_GUARANTEED),
    desireValue(0.0f),
    weight(0.0f)
{
}

bool Reaction::Load(iDocumentNode* node,BehaviorSet &behaviors)
{
    // Handle hooking up to the perception
    eventType              = node->GetAttributeValue("event");
    if(eventType.IsEmpty())
    {
        Error2("No event type specefied for node: %s",node->GetValue());
        return false;
    }

    if(eventType.Find("$") != ((size_t)-1))
    {
        Error3("Event type with variables not allowed for %s. From node: %s",eventType.GetData(),node->GetValue());
        return false;
    }

    // Default to guaranteed
    desireType = DESIRE_GUARANTEED;
    desireValue = 0.0f;

    if(node->GetAttribute("delta"))
    {
        desireValue = node->GetAttributeValueAsFloat("delta");
        desireType = DESIRE_DELTA;

        if(fabs(desireValue)<SMALL_EPSILON)   // 0 means no change
        {
            desireType = DESIRE_NONE;
        }
    }

    if(node->GetAttribute("absolute"))
    {
        if(desireType == DESIRE_NONE || desireType == DESIRE_DELTA)
        {
            Error1("Reaction can't be both absolute and delta reaction");
            return false;
        }
        desireValue = node->GetAttributeValueAsFloat("absolute");
        desireType = DESIRE_ABSOLUTE;
    }

    // Handle hooking up to the right behavior
    csString name = node->GetAttributeValue("behavior");
    if(!name.IsEmpty())
    {
        csArray<csString> names = psSplit(name,',');
        for(size_t i = 0; i < names.GetSize(); i++)
        {
            Behavior* behavior = behaviors.Find(names[i]);
            if(!behavior)
            {
                Error3("Reaction for event type '%s' specified unknown behavior of '%s'. Error in XML.",GetEventType().GetDataSafe(),(const char*)names[i]);
                return false;
            }
            affected.Push(behavior);
        }
    }

    range                  = node->GetAttributeValueAsFloat("range");
    weight                 = node->GetAttributeValueAsFloat("weight");
    factionDiff            = node->GetAttributeValueAsInt("faction_diff");
    oper                   = node->GetAttributeValue("oper");
    condition              = node->GetAttributeValue("condition");
    if(!condition.IsEmpty())
    {
        calcCondition = npcclient->GetMathScriptEngine()->FindScript(condition);
        if(!calcCondition)
        {
            Error2("Failed to load math script for Reaction condition '%s'",condition.GetDataSafe());
            return false;
        }
    }

    // Decode the value field, It is in the form value="1,2,,4"
    csString valueAttr = node->GetAttributeValue("value");
    csArray<csString> valueStr = psSplit(valueAttr , ',');
    for(size_t ii=0; ii < valueStr.GetSize(); ii++)
    {
        if(!valueStr[ii].IsEmpty())
        {
            values.Push(atoi(valueStr[ii]));
            valuesValid.Push(true);
        }
        else
        {
            values.Push(0);
            valuesValid.Push(false);
        }
    }
    // Decode the random field, It is in the form random="1,2,,4"
    csString randomAttr = node->GetAttributeValue("random");
    csArray<csString> randomStr = psSplit(randomAttr, ',');
    for(size_t ii=0; ii < randomStr.GetSize(); ii++)
    {
        if(!randomStr[ii].IsEmpty())
        {
            randoms.Push(atoi(randomStr[ii]));
            randomsValid.Push(true);
        }
        else
        {
            randoms.Push(0);
            randomsValid.Push(false);
        }
    }

    type                   = node->GetAttributeValue("type");
    activeOnly             = node->GetAttributeValueAsBool("active_only");
    inactiveOnly           = node->GetAttributeValueAsBool("inactive_only");
    reactWhenDead          = node->GetAttributeValueAsBool("when_dead",false);
    reactWhenInvisible     = node->GetAttributeValueAsBool("when_invisible",false);
    reactWhenInvincible    = node->GetAttributeValueAsBool("when_invincible",false);
    csString tmp           = node->GetAttributeValue("only_interrupt");
    if(tmp.Length())
    {
        onlyInterrupt = psSplit(tmp,',');
    }
    tmp                    = node->GetAttributeValue("do_not_interrupt");
    if(tmp.Length())
    {
        doNotInterrupt = psSplit(tmp,',');
    }

    return true;
}

void Reaction::DeepCopy(Reaction &other,BehaviorSet &behaviors)
{
    desireValue            = other.desireValue;
    desireType             = other.desireType;
    for(size_t i = 0; i < other.affected.GetSize(); i++)
    {
        Behavior* behavior = behaviors.Find(other.affected[i]->GetName());
        affected.Push(behavior);
    }
    eventType              = other.eventType;
    range                  = other.range;
    factionDiff            = other.factionDiff;
    oper                   = other.oper;
    condition              = other.condition;
    calcCondition          = other.calcCondition;
    weight                 = other.weight;
    values                 = other.values;
    valuesValid            = other.valuesValid;
    randoms                = other.randoms;
    randomsValid           = other.randomsValid;
    type                   = other.type;
    activeOnly             = other.activeOnly;
    inactiveOnly           = other.inactiveOnly;
    reactWhenDead          = other.reactWhenDead;
    reactWhenInvisible     = other.reactWhenInvisible;
    reactWhenInvincible    = other.reactWhenInvincible;
    onlyInterrupt          = other.onlyInterrupt;
    doNotInterrupt         = other.doNotInterrupt;

    // For now depend on that each npc do a deep copy to create its instance of the reaction
    for(uint ii=0; ii < values.GetSize(); ii++)
    {
        if(GetRandomValid((int)ii))
        {
            values[ii] += psGetRandom(GetRandom((int)ii));
        }
    }

    // Some special cases to handle
    if(eventType == "time")
    {
        TimePerception::NormalizeReaction(this);
    }
}

void Reaction::React(NPC* who, Perception* pcpt)
{
    CS_ASSERT(who);

    // Check if the perception is a match for this reaction
    if(!pcpt->ShouldReact(this, who))
    {
        if(who->IsDebugging(20))
        {
            NPCDebug(who, 20, "Reaction '%s' skipping perception %s", GetEventType().GetDataSafe(), pcpt->ToString(who).GetDataSafe());
        }
        return;
    }

    // If dead we should not react unless reactWhenDead is set
    if(!(who->IsAlive() || reactWhenDead))
    {
        NPCDebug(who, 5, "Only react to '%s' when alive", GetEventType().GetDataSafe());
        return;
    }

    // Check if the active behavior should not be interrupted.
    if(who->GetCurrentBehavior() && DoNotInterrupt(who->GetCurrentBehavior()))
    {
        NPCDebug(who, 5, "Prevented from reacting to '%s' while not interrupt behavior '%s' is active",
                 GetEventType().GetDataSafe(),who->GetCurrentBehavior()->GetName());
        return;
    }

    // Check if this reaction is limited to only interrupt some given behaviors.
    if(who->GetCurrentBehavior() && OnlyInterrupt(who->GetCurrentBehavior()))
    {
        NPCDebug(who, 5, "Prevented from reacting to '%s' since behavior '%s' should not be interrupted",
                 GetEventType().GetDataSafe(),who->GetCurrentBehavior()->GetName());
        return;
    }


    // Check condition
    if(!condition.IsEmpty())
    {
        MathEnvironment env;

        env.Define("NPCClient",                npcclient);
        env.Define("NPC",                      who);
        env.Define("Result",                   0.0);

        calcCondition->Evaluate(&env);

        MathVar* result   = env.Lookup("Result");

        if(result->GetValue() == 0.0)
        {
            NPCDebug(who, 5, "Prevented from reacting to '%s' since condition is false",
                     GetEventType().GetDataSafe());
            return;
        }
    }



    // We should no react and triggerd all affected behaviors

    // For debug get the time this reaction was triggered
    GetTimeOfDay(lastTriggered);


    // Adjust the needs for the triggered behaviors
    for(size_t i = 0; i < affected.GetSize(); i++)
    {

        // When activeOnly flag is set we should do nothing
        // if the affected behaviour is inactive.
        if(activeOnly && !affected[i]->IsActive())
            break;

        // When inactiveOnly flag is set we should do nothing
        // if the affected behaviour is active.
        if(inactiveOnly && affected[i]->IsActive())
            break;


        NPCDebug(who, 2, "Reaction '%s[%s]' reacting to perception: %s", GetEventType().GetDataSafe(), type.GetDataSafe(), pcpt->ToString(who).GetDataSafe());
        switch(desireType)
        {
            case DESIRE_NONE:
                NPCDebug(who, 10, "No change to need for behavior %s.", affected[i]->GetName());
                break;
            case DESIRE_ABSOLUTE:
                NPCDebug(who, 10, "Setting %1.1f need to behavior %s", desireValue, affected[i]->GetName());
                affected[i]->ApplyNeedAbsolute(who, desireValue);
                break;
            case DESIRE_DELTA:
                NPCDebug(who, 10, "Adding %1.1f need to behavior %s", desireValue, affected[i]->GetName());
                affected[i]->ApplyNeedDelta(who, desireValue);
                break;
            case DESIRE_GUARANTEED:
                NPCDebug(who, 10, "Guaranteed need to behavior %s", affected[i]->GetName());

                float highest = who->GetBrain()->GetHighestNeed(who);
                if(who->GetCurrentBehavior() != affected[i])
                {
                    affected[i]->ApplyNeedAbsolute(who, highest + 25);
                    affected[i]->SetCompletionDecay(-1);
                }
                break;
        }
    }

    // Execute the perception
    pcpt->ExecutePerception(who, weight);

    Perception* p = pcpt->MakeCopy();
    who->SetLastPerception(p);

}

bool Reaction::ShouldReact(gemNPCObject* actor)
{
    if(!actor) return false;

    if(!(!actor->IsInvisible() || reactWhenInvisible))
    {
        return false;
    }

    if(!(!actor->IsInvincible() || reactWhenInvincible))
    {
        return false;
    }

    return true;
}

bool Reaction::DoNotInterrupt(Behavior* behavior)
{
    if(doNotInterrupt.GetSize())
    {
        for(size_t i = 0; i < doNotInterrupt.GetSize(); i++)
        {
            if(doNotInterrupt[i].CompareNoCase(behavior->GetName()))
            {
                return true;
            }
        }
    }
    return false;
}

bool Reaction::OnlyInterrupt(Behavior* behavior)
{
    if(onlyInterrupt.GetSize())
    {
        for(size_t i = 0; i < onlyInterrupt.GetSize(); i++)
        {
            if(onlyInterrupt[i].CompareNoCase(behavior->GetName()))
            {
                return false; // The behavior is legal to interrupt
            }
        }
        return true; // The behavior isn't on the list of behaviors to interrupt
    }
    return false; // There are no limitation on who to interrupt
}

const csString &Reaction::GetEventType() const
{
    return eventType;
}


int Reaction::GetValue(int i)
{
    if(i < (int)values.GetSize())
    {
        return values[i];

    }
    return 0;
}

bool Reaction::GetValueValid(int i)
{
    if(i < (int)valuesValid.GetSize())
    {
        return valuesValid[i];

    }
    return false;
}

int Reaction::GetRandom(int i)
{
    if(i < (int)randoms.GetSize())
    {
        return randoms[i];

    }
    return 0;
}

bool Reaction::GetRandomValid(int i)
{
    if(i < (int)randomsValid.GetSize())
    {
        return randomsValid[i];

    }
    return false;
}

bool Reaction::SetValue(int i, int value)
{
    if(i < (int)values.GetSize())
    {
        values[i] = value;
        return true;
    }
    return false;
}

const csString Reaction::GetType(NPC* npc) const
{
    return psGameObject::ReplaceNPCVariables(npc, type);
}

char Reaction::GetOp()
{
    if(oper.Length())
    {
        return oper.GetAt(0);
    }
    else
    {
        return 0;
    }
}


csString Reaction::GetValue()
{
    csString result;
    for(int i = 0; i < (int)valuesValid.GetSize(); i++)
    {
        if(i != 0)
        {
            result.Append(",");
        }

        if(valuesValid[i])
        {
            result.AppendFmt("%d",values[i]);
        }
    }
    return result;
}

csString Reaction::GetAffectedBehaviors()
{
    csString result;
    for(size_t i = 0; i < affected.GetSize(); i++)
    {
        if(i != 0)
        {
            result.Append(", ");
        }
        result.Append(affected[i]->GetName());
    }
    if(!doNotInterrupt.IsEmpty())
    {
        result.Append(" No Int: ");
        for(size_t i = 0; i < doNotInterrupt.GetSize(); i++)
        {
            if(i != 0)
            {
                result.Append(", ");
            }
            result.Append(doNotInterrupt[i].GetDataSafe());
        }
    }
    if(!onlyInterrupt.IsEmpty())
    {
        result.Append(" Only Int: ");
        for(size_t i = 0; i < onlyInterrupt.GetSize(); i++)
        {
            if(i != 0)
            {
                result.Append(", ");
            }
            result.Append(onlyInterrupt[i].GetDataSafe());
        }
    }

    return result;
}


