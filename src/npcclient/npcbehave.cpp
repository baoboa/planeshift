/*
* npcbehave.cpp by Keith Fulton <keith@paqrat.com>
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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csgeom/transfrm.h>
#include <iutil/document.h>
#include <iutil/vfs.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <csutil/xmltiny.h>



//=============================================================================
// Project Includes
//=============================================================================
#include "net/clientmsghandler.h"
#include "net/npcmessages.h"

#include "util/log.h"
#include "util/location.h"
#include "util/psconst.h"
#include "util/strutil.h"
#include "util/psutil.h"
#include "util/eventmanager.h"

#include "engine/psworld.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcoperations.h"
#include "npcbehave.h"
#include "npc.h"
#include "perceptions.h"
#include "networkmgr.h"
#include "npcmesh.h"
#include "gem.h"
#include "globals.h"

NPCType::NPCType()
    :npc(NULL),ang_vel(999),vel(999),velSource(ScriptOperation::VEL_DEFAULT)
{
}

NPCType::NPCType(NPCType &other, NPC* npc)
    :npc(npc)
{
    DeepCopy(other);
}

NPCType::~NPCType()
{
}

void NPCType::DeepCopy(NPCType &other)
{
    name                  = other.name;
    ang_vel               = other.ang_vel;
    velSource             = other.velSource;
    vel                   = other.vel;
    collisionPerception   = other.collisionPerception;
    outOfBoundsPerception = other.outOfBoundsPerception;
    inBoundsPerception    = other.inBoundsPerception;
    fallingPerception     = other.fallingPerception;

    behaviors.DeepCopy(other.behaviors);

    for(size_t x=0; x<other.reactions.GetSize(); x++)
    {
        AddReaction(new Reaction(*other.reactions[x],behaviors));
    }
}

bool NPCType::Load(iResultRow &row)
{
    csString parents = row.GetString("parents");
    if(!parents.IsEmpty()) // this npctype is a subclass of another npctype
    {
        csArray<csString> parent = psSplit(parents,',');
        for(size_t i = 0; i < parent.GetSize(); i++)
        {
            NPCType* superclass = npcclient->FindNPCType(parent[i]);
            if(superclass)
            {
                DeepCopy(*superclass);  // This pulls everything from the parent into this one.
            }
            else
            {
                Error3("NPCType(%s) specified parent npctype '%s' could not be found.",
                       row["id"],parent[i].GetDataSafe());
                return false;
            }
        }
    }

    name = row.GetString("name");
    if(name.Length() == 0)
    {
        Error2("NPCType(%s) has no name attribute. Error in DB",row["id"]);
        return false;
    }

    ang_vel = row.GetFloat("ang_vel");

    csString velStr = row.GetString("vel");
    velStr.Upcase();

    if(velStr.IsEmpty())
    {
        // Do nothing. Use velSource from constructor default value
        // or as inherited from superclass.
    }
    else if(velStr == "$WALK")
    {
        velSource = ScriptOperation::VEL_WALK;
    }
    else if(velStr == "$RUN")
    {
        velSource = ScriptOperation::VEL_RUN;
    }
    else if(row.GetFloat("vel"))
    {
        velSource = ScriptOperation::VEL_USER;
        vel = row.GetFloat("vel");
    }

    collisionPerception   = row.GetString("collision");
    outOfBoundsPerception = row.GetString("out_of_bounds");
    inBoundsPerception    = row.GetString("in_bounds");
    fallingPerception     = row.GetString("falling");

    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse(row.GetString("script"));
    if(error)
    {
        Error4("NPCType(%s) script parsing error:%s in %s", row["id"], error, name.GetData());
        return false;
    }
    csRef<iDocumentNode> node = doc->GetRoot();
    if(!node)
    {
        Error3("NPCType(%s) no XML root in npc type script of %s", row["id"], name.GetData());
        return false;
    }

    // Now read in behaviors and reactions
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a widget so read it's factory to create it.
        if(strcmp(node->GetValue(), "behavior") == 0)
        {
            Behavior* b = new Behavior;
            if(!b->Load(node))
            {
                Error4("NPCType(%s) could not load behavior '%s'. Error in DB XML in node '%s'.", row["id"],
                       b->GetName(),node->GetValue());
                delete b;
                return false;
            }
            behaviors.Add(b);
            Debug3(LOG_STARTUP,0, "Added behavior '%s' to type %s.\n",b->GetName(),name.GetData());
        }
        else if(strcmp(node->GetValue(), "react") == 0)
        {
            Reaction* r = new Reaction;
            if(!r->Load(node,behaviors))
            {
                Error2("NPCType(%s) could not load reaction. Error in DB XML", row["id"]);
                delete r;
                return false;
            }
            // check for duplicates and keeps the last one
            for(size_t i=0; i<reactions.GetSize(); i++)
            {
                // Same event with same type
                if((reactions[i]->GetEventType() == r->GetEventType())&&
                        (reactions[i]->type == r->type)&&
                        (reactions[i]->values == r->values))
                {
                    // Check if there is a mach in affected
                    for(size_t k=0; k< r->affected.GetSize(); k++)
                    {
                        for(size_t j=0; j< reactions[i]->affected.GetSize(); j++)
                        {
                            if(!strcmp(r->affected[k]->GetName(),reactions[i]->affected[j]->GetName()))
                            {
                                // Should probably delete and clear out here
                                // to allow for overiding of event,affected pairs.
                                // Though now give error, until needed.
                                Error5("NPCType(%s) reaction of type '%s' already connected to '%s' in '%s'", row["id"],
                                       r->GetEventType().GetDataSafe(),reactions[i]->affected[j]->GetName(), name.GetDataSafe());
                                delete r;
                                return false;
                                // delete reactions[i];
                                //reactions.DeleteIndex(i);
                                //break;
                            }
                        }
                    }
                }
            }

            InsertReaction(r);  // reactions get inserted at beginning so subclass ones take precedence over superclass.
        }
        else if(strcmp(node->GetValue(), "empty") == 0)
        {
        }
        else
        {
            Error2("NPCType(%s) node is not 'behavior', 'react', or 'empty'. Error in DB XML", row["id"]);
            return false;
        }
    }
    return true; // success
}

bool NPCType::Load(iDocumentNode* node)
{
    // Load the name into a temp variable to prevent parent
    // deap copy from overriding the name. Need the name for
    // error reporting.
    csString typeName = node->GetAttributeValue("name");
    if(typeName.Length() == 0)
    {
        Error1("NPCType has no name attribute. Error in XML");
        return false;
    }

    csString parents = node->GetAttributeValue("parent");
    if(!parents.IsEmpty())  // this npctype is a subclass of another npctype
    {
        csArray<csString> parent = psSplit(parents,',');
        for(size_t i = 0; i < parent.GetSize(); i++)
        {
            NPCType* superclass = npcclient->FindNPCType(parent[i]);
            if(superclass)
            {
                DeepCopy(*superclass);  // This pulls everything from the parent into this one.
            }
            else
            {
                Error3("NPCType '%s': Specified parent npctype '%s' could not be found.",
                       typeName.GetDataSafe(), parent[i].GetDataSafe());
                return false;
            }
        }
    }

    // Now assign the name to this NPC Type after the parent DeepCopy is done.
    name = typeName;

    if(node->GetAttributeValueAsFloat("ang_vel"))
        ang_vel = node->GetAttributeValueAsFloat("ang_vel");
    else
        ang_vel = 999;

    csString velStr = node->GetAttributeValue("vel");
    velStr.Upcase();

    if(velStr.IsEmpty())
    {
        // Do nothing. Use velSource from constructor default value
        // or as inherited from superclass.
    }
    else if(velStr == "$WALK")
    {
        velSource = ScriptOperation::VEL_WALK;
    }
    else if(velStr == "$RUN")
    {
        velSource = ScriptOperation::VEL_RUN;
    }
    else if(node->GetAttributeValueAsFloat("vel"))
    {
        velSource = ScriptOperation::VEL_USER;
        vel = node->GetAttributeValueAsFloat("vel");
    }

    collisionPerception   = node->GetAttributeValue("collision");
    outOfBoundsPerception = node->GetAttributeValue("out_of_bounds");
    inBoundsPerception    = node->GetAttributeValue("in_bounds");
    fallingPerception     = node->GetAttributeValue("falling");

    // Now read in behaviors and reactions
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a widget so read it's factory to create it.
        if(strcmp(node->GetValue(), "behavior") == 0)
        {
            Behavior* b = new Behavior;
            if(!b->Load(node))
            {
                Error3("Could not load behavior '%s'. Error in XML in node '%s'.",
                       b->GetName(),node->GetValue());
                delete b;
                return false;
            }
            behaviors.Add(b);
            Debug3(LOG_STARTUP,0, "Added behavior '%s' to type %s.\n",b->GetName(),name.GetData());
        }
        else if(strcmp(node->GetValue(), "react") == 0)
        {
            Reaction* r = new Reaction;
            if(!r->Load(node,behaviors))
            {
                Error1("Could not load reaction. Error in XML");
                delete r;
                return false;
            }
            // check for duplicates and keeps the last one
            for(size_t i=0; i<reactions.GetSize(); i++)
            {
                // Same event with same type
                if((reactions[i]->GetEventType() == r->GetEventType())&&
                        (reactions[i]->type == r->type)&&
                        (reactions[i]->values == r->values))
                {
                    // Check if there is a mach in affected
                    for(size_t k=0; k< r->affected.GetSize(); k++)
                    {
                        for(size_t j=0; j< reactions[i]->affected.GetSize(); j++)
                        {
                            if(!strcmp(r->affected[k]->GetName(),reactions[i]->affected[j]->GetName()))
                            {
                                // Should probably delete and clear out here
                                // to allow for overiding of event,affected pairs.
                                // Though now give error, until needed.
                                Error4("Reaction of type '%s' already connected to '%s' in '%s'",
                                       r->GetEventType().GetDataSafe(),reactions[i]->affected[j]->GetName(), name.GetDataSafe());
                                delete r;
                                return false;
                                // delete reactions[i];
                                //reactions.DeleteIndex(i);
                                //break;
                            }

                        }

                    }
                }
            }

            InsertReaction(r);  // reactions get inserted at beginning so subclass ones take precedence over superclass.
        }
        else
        {
            Error1("Node under NPCType is not 'behavior' or 'react'. Error in XML");
            return false;
        }
    }
    return true; // success
}

void NPCType::AddReaction(Reaction* reaction)
{
    reactions.Push(reaction);
    if(npc)
    {
        npcclient->RegisterReaction(npc, reaction);
    }
}

void NPCType::InsertReaction(Reaction* reaction)
{
    reactions.Insert(0, reaction);  // reactions get inserted at beginning so subclass ones take precedence over superclass.
    if(npc)
    {
        npcclient->RegisterReaction(npc, reaction);
    }
}

void NPCType::FirePerception(NPC* npc, Perception* pcpt)
{
    // Check if this NPC should Automatically memorize some types

    if(npc->HasAutoMemorizeTypes() && npc->GetTribe())
    {
        if(npc->ContainAutoMemorizeType(pcpt->GetType()))
        {
            npc->GetTribe()->Memorize(npc,pcpt);
        }
    }

    // Check all reactions
    for(size_t x=0; x<reactions.GetSize(); x++)
    {
        reactions[x]->React(npc, pcpt);
    }
}

csString NPCType::InfoReactions(NPC* npc)
{
    csString reply;

    for(size_t i=0; i<reactions.GetSize(); i++)
    {

        reply.AppendFmt("%s",reactions[i]->GetEventType().GetDataSafe());

        csString type = reactions[i]->GetType(npc);
        if(!type.IsEmpty())
        {
            reply.AppendFmt("[%s]",type.GetDataSafe());
        }
        if(!reactions[i]->GetValue().IsEmpty())
        {
            reply.AppendFmt("(%s)",reactions[i]->GetValue().GetDataSafe());
        }

        if(i == (reactions.GetSize()-1))
        {
            reply.Append(".");
        }
        else
        {
            reply.Append(", ");
        }

    }
    return reply;
}

void NPCType::DumpReactionList(csString &output, NPC* npc)
{
    output.AppendFmt("%-25s %-25s %-5s %-10s %-20s %s\n","Reaction","Type","Range","Value","Last","Affects");
    for(size_t i=0; i<reactions.GetSize(); i++)
    {
        output.AppendFmt("%-25s %-25s %5.1f %-10s %-20s %s\n",
                         reactions[i]->GetEventType().GetDataSafe(),reactions[i]->GetType(npc).GetDataSafe(),
                         reactions[i]->GetRange(),
                         reactions[i]->GetValue().GetDataSafe(),
                         reactions[i]->GetLastTriggerd().GetDataSafe(),
                         reactions[i]->GetAffectedBehaviors().GetDataSafe());
    }
}

void NPCType::ClearState(NPC* npc)
{
    behaviors.ClearState(npc);
}

void NPCType::Advance(csTicks delta, NPC* npc)
{
    behaviors.Advance(delta,npc);
}

void NPCType::Interrupt(NPC* npc)
{
    behaviors.Interrupt(npc);
}

float NPCType::GetHighestNeed(NPC* npc)
{
    return behaviors.GetHighestNeed(npc);
}



float NPCType::GetAngularVelocity(NPC* /*npc*/)
{
    if(ang_vel != 999)
    {
        return ang_vel;
    }
    else
    {
        return 360*TWO_PI/360;
    }
}

float NPCType::GetVelocity(NPC* npc)
{
    switch(velSource)
    {
        case ScriptOperation::VEL_DEFAULT:
            return 1.5;
        case ScriptOperation::VEL_USER:
            return vel;
        case ScriptOperation::VEL_WALK:
            return npc->GetWalkVelocity();
        case ScriptOperation::VEL_RUN:
            return npc->GetRunVelocity();
    }
    return 0.0; // Should not return
}

void NPCType::SetVelSource(ScriptOperation::VelSource velSource, float vel)
{
    this->velSource = velSource;
    this->vel = vel;
}


const csString &NPCType::GetCollisionPerception() const
{
    return collisionPerception;
}

const csString &NPCType::GetOutOfBoundsPerception() const
{
    return outOfBoundsPerception;
}

const csString &NPCType::GetInBoundsPerception() const
{
    return inBoundsPerception;
}

const csString &NPCType::GetFallingPerception() const
{
    return fallingPerception;
}


//---------------------------------------------------------------------------
BehaviorSet::BehaviorSet()
{
    active=NULL;
}

void BehaviorSet::ClearState(NPC* npc)
{
    // Ensure any existing script is ended correctly.
    Interrupt(npc);
    for(size_t i = 0; i<behaviors.GetSize(); i++)
    {
        behaviors[i]->ResetNeed();
        behaviors[i]->SetIsActive(false);
        behaviors[i]->ClearInterrupted();
    }
    active = NULL;
}

bool BehaviorSet::Add(Behavior* behavior)
{
    // Search for duplicates
    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        if(!strcmp(behaviors[i]->GetName(),behavior->GetName()))
        {
            behaviors[i] = behavior;  // substitute
            return false;
        }
    }
    // Insert as new behavior
    behaviors.Push(behavior);
    return true;
}

void BehaviorSet::UpdateNeeds(float delta, NPC* npc)
{
    // Go through and update needs based on time
    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        Behavior* b = behaviors[i];
        if(b->ApplicableToNPCState(npc))
        {
            b->UpdateNeed(delta,npc);

            if(behaviors[i]->CurrentNeed() != behaviors[i]->NewNeed())
            {
                NPCDebug(npc, 4, "Advancing %-30s:\t%1.1f ->%1.1f",
                         behaviors[i]->GetName(),
                         behaviors[i]->CurrentNeed(),
                         behaviors[i]->NewNeed());
            }
        }

        // Reset the start step so that looping behaviors could be detected in RunScript
        b->SetStartStep();
    }
}

Behavior* BehaviorSet::Schedule(NPC* npc)
{
    float max_need = -999.0;

    // Move the behavior with higest need on top of list.
    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        Behavior* b = behaviors[i];
        if(b->ApplicableToNPCState(npc))
        {
            if(b->NewNeed() > max_need)  // the advance causes re-ordering
            {
                if(i!=0)   // trivial swap if same element
                {
                    behaviors[i] = behaviors[0];
                    behaviors[0] = b;  // now highest need is elem 0
                }
                max_need = b->NewNeed();
            }
            b->CommitAdvance();   // Set current need to new need.
        }
    }

    // now that behaviours are correctly sorted, select the first one
    Behavior* new_behaviour = behaviors[0];

    // use it only if need > 0
    if(new_behaviour->CurrentNeed()<=0 || !new_behaviour->ApplicableToNPCState(npc))
    {
        if(npc->IsDebugging(3))
        {
            csString output;
            npc->DumpBehaviorList(output);
            NPCDebug(npc, 3, output.GetDataSafe());
        }
        NPCDebug(npc, 15,"NO Active or no applicable behavior.");
        return NULL;
    }

    return new_behaviour;
}


void BehaviorSet::Advance(csTicks delta, NPC* npc)
{
    float d = .001 * delta;

    UpdateNeeds(d, npc);

    Behavior::BehaviorResult result = Behavior::BEHAVIOR_FAILED;

    bool forceRunScript = false;

    if(active)
    {
        result = active->Advance(d, npc);

        if(result == Behavior::BEHAVIOR_COMPLETED ||
                result == Behavior::BEHAVIOR_FAILED)
        {
            // This behavior is done so set it inactive
            active->SetIsActive(false);
            active = NULL;
        }
        else if(result == Behavior::BEHAVIOR_WILL_COMPLETE_LATER)
        {
        }
        else // Behavior::BEHAVIOR_NOT_COMPLETED
        {
            forceRunScript = true;
        }
    }

    Execute(npc, forceRunScript);
}

void BehaviorSet::Execute(NPC* npc, bool forceRunScript)
{
    Behavior::BehaviorResult result = Behavior::BEHAVIOR_FAILED;

    Behavior* lastActiveBehavior = NULL;
    int behaviorCount = 0;

    // Loop through the behaviors. If they complete on Run than
    // multiple behaviors might be executed.
    while(true)
    {
        // Sort the behaviors according to need
        Behavior* newBehavior = Schedule(npc);
        if(!newBehavior)
        {
            return;
        }

        // Flag to mark if script operations should be run.
        bool runScript = false;

        if(newBehavior != active)
        {
            if(active)   // if there is a behavior allready assigned to this npc
            {
                NPCDebug(npc, 1, "Switching behavior from '%s' to '%s'",
                         active->GetName(),
                         newBehavior->GetName());

                // Interrupt and stop current behaviour
                active->InterruptScript(npc);
                active->SetIsActive(false);
            }
            else
            {
                // Check if this behavior has allready looped once
                if(newBehavior != lastActiveBehavior)
                {
                    NPCDebug(npc, 1, "Activating behavior '%s'",
                             newBehavior->GetName());
                }
                else
                {
                    break;
                }
            }


            // Set the new active behaviour
            active = newBehavior;
            // Activate the new behaviour
            active->SetIsActive(true);
            // Store the active behavior
            lastActiveBehavior = active;

            // Dump bahaviour list if changed
            if(npc->IsDebugging(3))
            {
                csString output;
                npc->DumpBehaviorList(output);
                NPCDebug(npc,3,output.GetDataSafe());
            }

            // If the interrupt has change the needs, try once
            if(GetHighestNeed(npc) > active->CurrentNeed())
            {
                NPCDebug(npc, 3, "Need to rechedule since a new need is higest");
                continue; // Start the check once more.
            }

            active->StartScript(npc);

            // Run script operations when a new operation become active
            runScript = true;
        }
        else
        {
            if(active && (result == Behavior::BEHAVIOR_NOT_COMPLETED || forceRunScript))
            {
                // Run script operations when last run script operations completed
                runScript = true;
            }
            else
            {
                break;
            }
        }

        if(runScript)
        {
            // Increment behavior count so that we can detect infinit loops
            behaviorCount++;
            if(behaviorCount > 100)
            {
                Error2("Behavior count limit reached for %s",npc->GetName());
                break;
            }


            forceRunScript = false;

            result = active->Advance(0.0, npc);
            if(result == Behavior::BEHAVIOR_COMPLETED)
            {
                // This behavior is done so set it inactive
                active->SetIsActive(false);
                active = NULL;
            }
            else if(result == Behavior::BEHAVIOR_FAILED)
            {
                // This behavior is done so set it inactive
                active->SetIsActive(false);
                active = NULL;
                break; // Breake the loop
            }
            else if(result == Behavior::BEHAVIOR_WILL_COMPLETE_LATER)
            {
                break; // Break the loop
            }
            else // Behavior::BEHAVIOR_NOT_COMPLETED
            {
                // This behavior isn't done yet
            }
        }
    }

    NPCDebug(npc,15, "Active behavior is '%s'", (active?active->GetName():"(null)"));
    return;
}

void BehaviorSet::Interrupt(NPC* npc)
{
    if(active)
    {
        active->InterruptScript(npc);
    }
}

void BehaviorSet::DeepCopy(BehaviorSet &other)
{
    Behavior* b,*b2;
    for(size_t i=0; i<other.behaviors.GetSize(); i++)
    {
        b  = other.behaviors[i];
        b2 = new Behavior(*b);
        behaviors.Push(b2);
    }
}

Behavior* BehaviorSet::Find(const char* name)
{
    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        if(!strcmp(behaviors[i]->GetName(),name))
            return behaviors[i];
    }
    return NULL;
}

void BehaviorSet::DumpBehaviorList(csString &output, NPC* npc)
{
    output.AppendFmt("Appl. IA %-30s %6s %6s %5s\n","Behavior","Curr","New","Step");

    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        char applicable = 'N';
        if(npc && behaviors[i]->ApplicableToNPCState(npc))
        {
            applicable = 'Y';
        }

        output.AppendFmt("%c     %s%s %-30s %6.1f %6.1f %2zu/%-2zu\n",applicable,
                         (behaviors[i]->IsInterrupted()?"!":" "),
                         (behaviors[i]->IsActive()?"*":" "),
                         behaviors[i]->GetName(),behaviors[i]->CurrentNeed(),
                         behaviors[i]->NewNeed(),
                         behaviors[i]->GetCurrentStep(),
                         behaviors[i]->GetLastStep());
    }
}

csString BehaviorSet::InfoBehaviors(NPC* npc)
{
    csString reply;

    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        reply.AppendFmt("%s(%s%s%.1f) ",
                        behaviors[i]->GetName(),
                        (behaviors[i]->IsInterrupted()?"!":""),
                        (behaviors[i]->IsActive()?"*":""),
                        behaviors[i]->CurrentNeed());
    }
    return reply;
}

float BehaviorSet::GetHighestNeed(NPC* npc)
{
    float highest = 0.0;
    float temp;

    for(size_t i=0; i<behaviors.GetSize(); i++)
    {
        Behavior* b = behaviors[i];
        if(b->ApplicableToNPCState(npc))
        {
            if((temp = b->CurrentNeed()) > highest)
            {
                highest = temp;
            }
            if((temp = b->NewNeed()) > highest)
            {
                highest = temp;
            }
        }
    }

    return highest;
}




//---------------------------------------------------------------------------

Behavior::Behavior()
{
    name                    = "";
    loop                    = false;
    isActive                = false;
    is_applicable_when_dead = false;
    need_decay_rate         = 0;
    need_growth_rate        = 0;
    completion_decay        = 0;
    init_need               = 0;
    resume_after_interrupt  = false;
    current_need            = init_need;
    new_need                = -999;
    interrupted             = false;
    current_step            = 0;
    minLimitValid           = false;
    minLimit                = 0.0;
    maxLimitValid           = false;
    maxLimit                = 0.0;
    stepCount               = 0;
}

Behavior::Behavior(const char* n)
{
    name                    = n;
    loop                    = false;
    isActive                = false;
    is_applicable_when_dead = false;
    need_decay_rate         = 0;
    need_growth_rate        = 0;
    completion_decay        = 0;
    init_need               = 0;
    resume_after_interrupt  = false;
    current_need            = init_need;
    new_need                =-999;
    interrupted             = false;
    current_step            = 0;
    minLimitValid           = false;
    minLimit                = 0.0;
    maxLimitValid           = false;
    maxLimit                = 0.0;
    stepCount               = 0;
}

Behavior::Behavior(Behavior &other)
{
    DeepCopy(other);
}


void Behavior::DeepCopy(Behavior &other)
{
    name                    = other.name;
    loop                    = other.loop;
    isActive                = other.isActive;
    is_applicable_when_dead = other.is_applicable_when_dead;
    need_decay_rate         = other.need_decay_rate;  // need lessens while performing behavior
    need_growth_rate        = other.need_growth_rate; // need grows while not performing behavior
    completion_decay        = other.completion_decay;
    init_need               = other.init_need;
    resume_after_interrupt  = other.resume_after_interrupt;
    interruptPerception     = other.interruptPerception;
    current_need            = other.current_need;
    new_need                = -999;
    interrupted             = false;
    minLimitValid           = other.minLimitValid;
    minLimit                = other.minLimit;
    maxLimitValid           = other.maxLimitValid;
    maxLimit                = other.maxLimit;
    failurePerception       = other.failurePerception;

    for(size_t x=0; x<other.sequence.GetSize(); x++)
    {
        ScriptOperation* copy = other.sequence[x]->MakeCopy();
        copy->SetParent(this);
        sequence.Push(copy);
    }

    // Instance local variables. No need to copy.
    current_step = 0;
}

bool Behavior::Load(iDocumentNode* node)
{
    // This function can be called recursively, so we only get attributes at top level
    name = node->GetAttributeValue("name");
    if(name.Length() == 0)
    {
        Error1("Behavior has no name attribute. Error in XML");
        return false;
    }

    loop                    = node->GetAttributeValueAsBool("loop",false);
    need_decay_rate         = node->GetAttributeValueAsFloat("decay");
    completion_decay        = node->GetAttributeValueAsFloat("completion_decay");
    need_growth_rate        = node->GetAttributeValueAsFloat("growth");
    init_need               = node->GetAttributeValueAsFloat("initial");
    is_applicable_when_dead = node->GetAttributeValueAsBool("when_dead");
    resume_after_interrupt  = node->GetAttributeValueAsBool("resume",false);
    interruptPerception     = node->GetAttributeValue("interrupt");
    failurePerception       = node->GetAttributeValue("failure");

    if(node->GetAttributeValue("min"))
    {
        minLimitValid = true;
        minLimit = node->GetAttributeValueAsFloat("min");
    }
    else
    {
        minLimitValid = false;
    }
    if(node->GetAttributeValue("max"))
    {
        maxLimitValid = true;
        maxLimit = node->GetAttributeValueAsFloat("max");
    }
    else
    {
        maxLimitValid = false;
    }

    // TODO: Remove this check when all servers has been updated.
    // This field of the behavior is now depricated.
    csString tmp            = node->GetAttributeValue("auto_memorize");
    if(tmp.Length())
    {
        Error2("Auto_memorize is not longer suppored. Do not use in behavior %s",name.GetDataSafe());
        return false;
    }

    current_need            = init_need;

    bool result = LoadScript(node,true);
    if(result)
    {
        // Check if we have a sequence
        if(sequence.IsEmpty())
        {
            Error2("No script operations for behavior %s",name.GetDataSafe());
            return false;
        }
    }
    return result;
}

bool Behavior::LoadScript(iDocumentNode* node,bool top_level)
{
    // Now read in script for this behavior
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();
        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        ScriptOperation* op = NULL;

        // Some Responses need post load functions.
        bool postLoadBeginLoop = false;

        // Needed by the post load of begin loop
        int beginLoopWhere = -1;

        // This is a operation so read it's factory to create it.
        if(strcmp(node->GetValue(), "assess") == 0)
        {
            op = new AssessOperation;
        }
        else if(strcmp(node->GetValue(), "auto_memorize") == 0)
        {
            op = new AutoMemorizeOperation;
        }
        else if(strcmp(node->GetValue(), "build") == 0)
        {
            op = new BuildOperation;
        }
        else if(strcmp(node->GetValue(), "busy") == 0)
        {
            op = new BusyOperation(true);
        }
        else if(strcmp(node->GetValue(), "cast") == 0)
        {
            op = new CastOperation;
        }
        else if(strcmp(node->GetValue(), "change_brain") == 0)
        {
            op = new ChangeBrainOperation;
        }
        else if(strcmp(node->GetValue(), "chase") == 0)
        {
            op = new ChaseOperation;
        }
        else if(strcmp(node->GetValue(), "circle") == 0)
        {
            op = new CircleOperation;
        }
        else if(strcmp(node->GetValue(), "control") == 0)
        {
            op = new ControlOperation(true);
        }
        else if(strcmp(node->GetValue(), "copy_locate") == 0)
        {
            op = new CopyLocateOperation;
        }
        else if(strcmp(node->GetValue(), "debug") == 0)
        {
            op = new DebugOperation;
        }
        else if(strcmp(node->GetValue(), "delete_npc") == 0)
        {
            op = new DeleteNPCOperation;
        }
        else if(strcmp(node->GetValue(), "dequip") == 0)
        {
            op = new DequipOperation;
        }
        else if(strcmp(node->GetValue(), "drop") == 0)
        {
            op = new DropOperation;
        }
        else if(strcmp(node->GetValue(), "eat") == 0)
        {
            op = new EatOperation;
        }
        else if(strcmp(node->GetValue(), "emote") == 0)
        {
            op = new EmoteOperation;
        }
        else if(strcmp(node->GetValue(), "equip") == 0)
        {
            op = new EquipOperation;
        }
        else if(strcmp(node->GetValue(), "hate_list") == 0)
        {
            op = new HateListOperation;
        }
        else if(strcmp(node->GetValue(), "idle") == 0)
        {
            op = new BusyOperation(false);
        }
        else if(strcmp(node->GetValue(), "invisible") == 0)
        {
            op = new InvisibleOperation;
        }
        else if(strcmp(node->GetValue(), "locate") == 0)
        {
            op = new LocateOperation;
        }
        else if(strcmp(node->GetValue(), "loop") == 0)
        {
            op = new LoopBeginOperation;
            beginLoopWhere = (int)sequence.GetSize(); // Where will sequence be pushed
            postLoadBeginLoop = true;
        }
        else if(strcmp(node->GetValue(), "loot") == 0)
        {
            op = new LootOperation;
        }
        else if(strcmp(node->GetValue(), "melee") == 0)
        {
            op = new MeleeOperation;
        }
        else if(strcmp(node->GetValue(), "memorize") == 0)
        {
            op = new MemorizeOperation;
        }
        else if(strcmp(node->GetValue(), "move") == 0)
        {
            op = new MoveOperation;
        }
        else if(strcmp(node->GetValue(), "movepath") == 0)
        {
            op = new MovePathOperation;
        }
        else if(strcmp(node->GetValue(), "moveto") == 0)
        {
            op = new MoveToOperation;
        }
        else if(strcmp(node->GetValue(), "navigate") == 0)
        {
            op = new NavigateOperation;
        }
        else if(strcmp(node->GetValue(), "nop") == 0)
        {
            op = new NOPOperation;
        }
        else if(strcmp(node->GetValue(), "percept") == 0)
        {
            op = new PerceptOperation;
        }
        else if(strcmp(node->GetValue(), "pickup") == 0)
        {
            op = new PickupOperation;
        }
        else if(strcmp(node->GetValue(), "release_control") == 0)
        {
            op = new ControlOperation(false);
        }
        else if(strcmp(node->GetValue(), "reproduce") == 0)
        {
            op = new ReproduceOperation;
        }
        else if(strcmp(node->GetValue(), "resurrect") == 0)
        {
            op = new ResurrectOperation;
        }
        else if(strcmp(node->GetValue(), "reward") == 0)
        {
            op = new RewardOperation;
        }
        else if(strcmp(node->GetValue(), "rotate") == 0)
        {
            op = new RotateOperation;
        }
        else if(strcmp(node->GetValue(), "script") == 0)
        {
            op = new ProgressScriptOperation;
        }
        else if(strcmp(node->GetValue(), "sequence") == 0)
        {
            op = new SequenceOperation;
        }
        else if(strcmp(node->GetValue(), "set_buffer") == 0)
        {
            op = new SetBufferOperation;
        }
        else if(strcmp(node->GetValue(), "share_memories") == 0)
        {
            op = new ShareMemoriesOperation;
        }
        else if(strcmp(node->GetValue(), "sit") == 0)
        {
            op = new SitOperation(true);
        }
        else if(strcmp(node->GetValue(), "standup") == 0)
        {
            op = new SitOperation(false);
        }
        else if(strcmp(node->GetValue(), "talk") == 0)
        {
            op = new TalkOperation;
        }
        else if(strcmp(node->GetValue(), "teleport") == 0)
        {
            op = new TeleportOperation;
        }
        else if(strcmp(node->GetValue(), "transfer") == 0)
        {
            op = new TransferOperation;
        }
        else if(strcmp(node->GetValue(), "tribe_home") == 0)
        {
            op = new TribeHomeOperation;
        }
        else if(strcmp(node->GetValue(), "tribe_type") == 0)
        {
            op = new TribeTypeOperation;
        }
        else if(strcmp(node->GetValue(), "unbuild") == 0)
        {
            op = new UnbuildOperation;
        }
        else if(strcmp(node->GetValue(), "vel_source") == 0)
        {
            op = new VelSourceOperation();
        }
        else if(strcmp(node->GetValue(), "visible") == 0)
        {
            op = new VisibleOperation;
        }
        else if(strcmp(node->GetValue(), "wait") == 0)
        {
            op = new WaitOperation;
        }
        else if(strcmp(node->GetValue(), "wander") == 0)
        {
            op = new WanderOperation;
        }
        else if(strcmp(node->GetValue(), "watch") == 0)
        {
            op = new WatchOperation;
        }
        else if(strcmp(node->GetValue(), "work") == 0)
        {
            op = new WorkOperation;
        }
        else
        {
            Error2("Node '%s' under Behavior is not a valid script operation name. Error in XML",
                   node->GetValue());
            return false;
        }

        if(!op->Load(node))
        {
            Error2("Could not load <%s> ScriptOperation. Error in XML",op->GetName());
            delete op;
            return false;
        }
        op->SetParent(this);
        sequence.Push(op);

        // Execute any outstanding post load operations.
        if(postLoadBeginLoop)
        {
            LoopBeginOperation* blop = dynamic_cast<LoopBeginOperation*>(op);
            if(!blop)
            {
                Error1("Loop begin operation not a loop operation!");
                return false;
            }

            if(!LoadScript(node,false))  // recursively load within loop
            {
                Error1("Could not load within Loop Operation. Error in XML");
                return false;
            }

            LoopEndOperation* op2 = new LoopEndOperation(beginLoopWhere,blop->iterations);
            op2->SetParent(this);
            sequence.Push(op2);
        }
    }

    return true; // success
}

void Behavior::UpdateNeed(float delta, NPC* npc)
{
    // Initialize new_need if not updated before.
    if(new_need == -999)
    {
        new_need = current_need;
    }

    if(isActive)
    {
        // Apply delta to need, will check for limits as well
        ApplyNeedDelta(npc, -delta * need_decay_rate);

        NPCDebug(npc, 11, "%s - Advance active delta: %.3f Need: %.2f Decay Rate: %.2f",
                 name.GetData(),delta,new_need,need_decay_rate);
    }
    else
    {
        // Apply delta to need, will check for limits as well
        ApplyNeedDelta(npc, delta * need_growth_rate);

        NPCDebug(npc, 11, "%s - Advance none active delta: %.3f Need: %.2f Growth Rate: %.2f",
                 name.GetData(),delta,new_need,need_growth_rate);
    }

}


Behavior::BehaviorResult Behavior::Advance(float delta, NPC* npc)
{
    if(!isActive)
    {
        Error1("Trying to advance not active behavior!!");
        return BEHAVIOR_FAILED;
    }

    Behavior::BehaviorResult behaviorResult = BEHAVIOR_FAILED;
    ScriptOperation::OperationResult result;

    if(sequence[current_step]->GetState() == ScriptOperation::READY_TO_RUN ||
            sequence[current_step]->GetState() == ScriptOperation::INTERRUPTED)
    {
        NPCDebug(npc, 2, "Running %s step %d - %s operation%s",
                 name.GetData(),current_step,sequence[current_step]->GetName(),
                 (interrupted?" Interrupted":""));

        interrupted = (sequence[current_step]->GetState() == ScriptOperation::INTERRUPTED);

        // Run the script
        sequence[current_step]->SetState(ScriptOperation::RUNNING);
        result = sequence[current_step]->Run(npc, interrupted);
        interrupted = false; // Reset the interrupted flag after operation has run
    }
    else if(sequence[current_step]->GetState() == ScriptOperation::RUNNING)
    {
        NPCDebug(npc, 10, "%s - Advance active delta: %.3f Need: %.2f Decay Rate: %.2f",
                 name.GetData(),delta,new_need,need_decay_rate);
        result = sequence[current_step]->Advance(delta,npc);
    }
    else
    {
        Error5("Don't know how to advance sequence in behavior %s for %s(%s) in state %d",
               GetName(),npc->GetName(),ShowID(npc->GetEID()),sequence[current_step]->GetState());
        return BEHAVIOR_FAILED;
    }


    switch(result)
    {
        case ScriptOperation::OPERATION_NOT_COMPLETED:
        {
            // Operation not completed and should relinquish
            NPCDebug(npc, 2, "Behavior %s step %d - %s will complete later...",
                     name.GetData(),current_step,sequence[current_step]->GetName());

            behaviorResult = Behavior::BEHAVIOR_WILL_COMPLETE_LATER; // This behavior is done for now.
            break;
        }
        case ScriptOperation::OPERATION_COMPLETED:
        {
            OperationCompleted(npc);
            if(stepCount >= sequence.GetSize() || (current_step == 0 && !loop))
            {
                NPCDebug(npc, 3, "Terminating behavior '%s' since it has looped all once.",GetName());
                behaviorResult = Behavior::BEHAVIOR_COMPLETED; // This behavior is done
            }
            else
            {
                behaviorResult = Behavior::BEHAVIOR_NOT_COMPLETED; // More to do for this behavior
            }
            break;
        }
        case ScriptOperation::OPERATION_FAILED:
        {
            OperationFailed(npc);
            behaviorResult = Behavior::BEHAVIOR_FAILED; // This behavior is done
            break;
        }
    }

    return behaviorResult;
}


void Behavior::CommitAdvance()
{
    // Only update the current_need if new_need has been initialized.
    if(new_need!=-999)
    {
        current_need = new_need;
    }
}

void Behavior::ApplyNeedDelta(NPC* npc, float deltaDesire)
{
    // Initialize new_need if not updated before.
    if(new_need==-999)
    {
        new_need = current_need;
    }

    // Apply the delta to new_need
    new_need += deltaDesire;

    // Handle min desire limit
    if(minLimitValid && (new_need < minLimit))
    {
        NPCDebug(npc, 5, "%s - ApplyNeedDelta limited new_need of %.3f to min value %.3f.",
                 name.GetData(),new_need,minLimit);

        new_need = minLimit;
    }

    // Handle max desire limit
    if(maxLimitValid && (new_need > maxLimit))
    {
        NPCDebug(npc, 5, "%s - ApplyNeedDelta limited new_need of %.3f to max value %.3f.",
                 name.GetData(),new_need,maxLimit);

        new_need = maxLimit;
    }

    // Floor behaviours to -998.0. (the -999.0 is an invalid uninit value).
    if(new_need < -998.0)
    {
        new_need = -998.0;
    }
}

void Behavior::ApplyNeedAbsolute(NPC* npc, float absoluteDesire)
{
    new_need = absoluteDesire;

    // Handle min desire limit
    if(minLimitValid && (new_need < minLimit))
    {
        NPCDebug(npc, 5, "%s - ApplyNeedAbsolute limited new_need of %.3f to min value %.3f.",
                 name.GetData(),new_need,minLimit);

        new_need = minLimit;
    }

    // Handle max desire limit
    if(maxLimitValid && (new_need > maxLimit))
    {
        NPCDebug(npc, 5, "%s - ApplyNeedAbsolute limited new_need of %.3f to max value %.3f.",
                 name.GetData(),new_need,maxLimit);

        new_need = maxLimit;
    }
}

void Behavior::SetCurrentStep(int step)
{
    if(step < 0 || step >= (int)sequence.GetSize())
    {
        Error3("Behavior trying to set current step to value %d that is outside sequence 0..%zu",
               step,sequence.GetSize());
        return;
    }

    sequence[current_step]->SetState(ScriptOperation::COMPLETED);

    current_step = step;

    // Now that current_step has ben advanced, mark the next sequence as ready to run.
    sequence[current_step]->SetState(ScriptOperation::READY_TO_RUN);
}


bool Behavior::ApplicableToNPCState(NPC* npc)
{
    return npc->IsAlive() || (!npc->IsAlive() && is_applicable_when_dead);
}

void Behavior::DoCompletionDecay(NPC* npc)
{
    if(completion_decay)
    {
        float delta_decay = completion_decay;

        if(completion_decay == -1)
        {
            delta_decay = current_need;
        }

        NPCDebug(npc, 10, "Subtracting completion decay of %.2f from behavior '%s'.",delta_decay,GetName());
        new_need = current_need - delta_decay;
    }
}

void Behavior::StartScript(NPC* npc)
{
    if(interrupted && resume_after_interrupt)
    {
        NPCDebug(npc, 3, "Resuming behavior %s after interrupt at step %d - %s.",
                 name.GetData(), current_step, sequence[current_step]->GetName());
    }
    else
    {
        // Start at the first step of the script.
        current_step = 0;
        sequence[current_step]->SetState(ScriptOperation::READY_TO_RUN);

        if(interrupted)
        {
            // We don't resume_after_interrupt, but the flag needs to be cleared
            NPCDebug(npc, 3, "Restarting behavior %s after interrupt at step %d - %s.",
                     name.GetData(), current_step, sequence[current_step]->GetName());
            interrupted = false;
        }
    }

}

void Behavior::OperationCompleted(NPC* npc)
{
    NPCDebug(npc, 2, "Completed step %d - %s of behavior %s",
             current_step,sequence[current_step]->GetName(),name.GetData());

    sequence[current_step]->SetState(ScriptOperation::COMPLETED);

    // Increate the couter to tell how many operation has been run this periode.
    stepCount++;

    // Increment the current step pointer
    current_step++;
    if(current_step >= sequence.GetSize())
    {
        current_step = 0; // behaviors automatically loop around to the top

        if(loop)
        {
            NPCDebug(npc, 1, "Loop back to start of behaviour '%s'",GetName());
        }
        else
        {
            DoCompletionDecay(npc);
            NPCDebug(npc, 1, "End of non looping behaviour '%s'",GetName());
        }
    }

    // Now that current_step has ben advanced, mark the next sequence as ready to run.
    sequence[current_step]->SetState(ScriptOperation::READY_TO_RUN);
}

void Behavior::OperationFailed(NPC* npc)
{
    sequence[current_step]->Failure(npc);
    current_step = 0; // Restart operation next time
    DoCompletionDecay(npc);
    NPCDebug(npc, 1, "End of behaviour '%s'",GetName());
}

void Behavior::SetStartStep()
{
    stepCount = 0;
}

void Behavior::InterruptScript(NPC* npc)
{
    NPCDebug(npc, 2, "Interrupting behaviour %s at step %d - %s",
             name.GetData(),current_step,sequence[current_step]->GetName());

    if(interruptPerception.Length())
    {
        Perception perception(interruptPerception);
        npc->TriggerEvent(&perception);
    }

    sequence[current_step]->InterruptOperation(npc);
    interrupted = true;

    if(sequence[current_step]->GetState() == ScriptOperation::RUNNING)
    {
        sequence[current_step]->SetState(ScriptOperation::INTERRUPTED);
    }

}

void Behavior::Failure(NPC* npc, ScriptOperation* op)
{
    if(failurePerception.IsEmpty())
    {
        NPCDebug(npc, 5, "Operation %s failed with no failure perception",op->GetName());
    }
    else
    {
        NPCDebug(npc, 5, "Operation %s failed tigger failure perception: %s",
                 op->GetName(),failurePerception.GetData());

        Perception perception(failurePerception);
        npc->TriggerEvent(&perception);
    }
}


//---------------------------------------------------------------------------

void psGameObject::GetPosition(gemNPCObject* object, csVector3 &pos, float &yrot,iSector* &sector)
{
    npcMesh* pcmesh = object->pcmesh;

    // Position
    if(!pcmesh->GetMesh())
    {
        CPrintf(CON_ERROR,"ERROR! NO MESH FOUND FOR OBJECT %s!\n",object->GetName());
        return;
    }

    iMovable* npcMovable = pcmesh->GetMesh()->GetMovable();

    iSectorList* npcSectors = npcMovable->GetSectors();

    pos = npcMovable->GetPosition();

    // rotation
    csMatrix3 transf = npcMovable->GetTransform().GetT2O();
    yrot = psWorld::Matrix2YRot(transf);
    if(CS::IsNaN(yrot))
    {
        yrot = 0;
    }

    // Sector
    if(npcSectors->GetCount())
    {
        sector = npcSectors->Get(0);
    }
    else
    {
        sector = NULL;
    }

    //    NPC* npc = object->GetNPC();
    //    if (npc)
    //    {
    //        NPCDebug(npc, 1, "============ GetPosition(%s, %.2f) =============", toString(pos,sector).GetDataSafe(), yrot);
    //    }
}

void psGameObject::GetPosition(gemNPCObject* object, csVector3 &pos,iSector* &sector)
{
    npcMesh* pcmesh = object->pcmesh;

    // Position
    if(!pcmesh->GetMesh())
    {
        CPrintf(CON_ERROR,"ERROR! NO MESH FOUND FOR OBJECT %s!\n",object->GetName());
        return;
    }

    iMovable* npcMovable = pcmesh->GetMesh()->GetMovable();

    pos = npcMovable->GetPosition();

    // Sector
    if(npcMovable->GetSectors()->GetCount())
    {
        sector = npcMovable->GetSectors()->Get(0);
    }
    else
    {
        sector = NULL;
    }

    //    NPC* npc = object->GetNPC();
    //    if (npc)
    //    {
    //        NPCDebug(npc, 1, "============ GetPosition(%s) =============", toString(pos,sector).GetDataSafe());
    //    }

}


void psGameObject::SetPosition(gemNPCObject* object, const csVector3 &pos, iSector* sector)
{
    npcMesh* pcmesh = object->pcmesh;
    pcmesh->MoveMesh(sector,pos);
}

void psGameObject::SetRotationAngle(gemNPCObject* object, float angle)
{
    npcMesh* pcmesh = object->pcmesh;

    csMatrix3 matrix = (csMatrix3) csYRotMatrix3(angle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T(matrix);
}

float psGameObject::CalculateIncidentAngle(const csVector3 &pos, const csVector3 &dest)
{
    csVector3 diff = dest-pos;  // Get vector from player to desired position

    if(!diff.x)
        diff.x = .000001F; // div/0 protect

    float angle = atan2(-diff.x,-diff.z);

    return angle;
}

void psGameObject::ClampRadians(float &target_angle)
{
    if(CS::IsNaN(target_angle)) return;

    // Clamp the angle witin 0 to 2*PI
    while(target_angle < 0)
        target_angle += TWO_PI;
    while(target_angle > TWO_PI)
        target_angle -= TWO_PI;
}

void psGameObject::NormalizeRadians(float &target_angle)
{
    if(CS::IsNaN(target_angle)) return;

    // Normalize angle within -PI to PI
    while(target_angle < PI)
        target_angle += TWO_PI;
    while(target_angle > PI)
        target_angle -= TWO_PI;
}

csVector3 psGameObject::DisplaceTargetPos(const iSector* mySector, const csVector3 &myPos, const iSector* targetSector, const csVector3 &targetPos , float offset)
{
    csVector3 displace;

    // This prevents NPCs from wanting to occupy the same physical space as something else
    if(mySector == targetSector)
    {
        csVector3 displacement = targetPos - myPos;
        displacement.y = 0;
        displace = offset*displacement.Unit();
    }
    return displace;
}

csVector3 psGameObject::DisplaceTargetPos(const iSector* mySector, const csVector3 &myPos, const iSector* targetSector, const csVector3 &targetPos , float offset, float angle)
{
    csVector3 displace;

    // This prevents NPCs from wanting to occupy the same physical space as something else
    if(mySector == targetSector)
    {
        csVector3 displacement = targetPos - myPos;
        displacement.y = 0;

        csTransform transform;
        displacement = csMatrix3(0.0,1.0,0.0,angle)*displacement;

        displace = offset*displacement.Unit();
    }
    return displace;
}


float psGameObject::Calc2DDistance(const csVector3 &a, const csVector3 &b)
{
    csVector3 diff = a-b;
    diff.y = 0;
    return diff.Norm();
}

csString psGameObject::ReplaceNPCVariables(NPC* npc, const csString &object)
{
    // Check if there are any $ sign in the string. If not there is no more to do
    // If there are no NPC then we just return as well.
    if((object.FindFirst('$') == ((size_t)-1)) || (!npc)) return object;

    // Now check if any of the $ signs is something to replace
    csString result(object);  // Result will hold the string with all variables replaced

    // First replace buffers, this so that keywords can be put into buffers as well.
    npc->ReplaceBuffers(result);

    // Replace locations
    npc->ReplaceLocations(result);

    // Replace tribe stuff
    if(npc->GetTribe())
    {
        npc->GetTribe()->ReplaceBuffers(result);

        // Now that we are done with buffers, replace keywords

        result.ReplaceAll("$tribe",npc->GetTribe()->GetName());
        result.ReplaceAll("$member_type",npc->GetTribeMemberType());
        result.ReplaceAll("$resource_area",npc->GetTribe()->GetNeededResourceAreaType());
    }

    result.ReplaceAll("$name",npc->GetName());
    if(npc->GetRaceInfo())
    {
        csString size;
        size.Append(csMax(npc->GetRaceInfo()->size.x,npc->GetRaceInfo()->size.y));
        result.ReplaceAll("$race_size",size);
        result.ReplaceAll("$race",npc->GetRaceInfo()->name.GetDataSafe());
    }
    if(npc->GetOwner())
    {
        result.ReplaceAll("$owner",npc->GetOwnerName());
    }
    if(npc->GetTarget())
    {
        result.ReplaceAll("$target",npc->GetTarget()->GetName());
    }

    if(npc->GetLastPerception())
    {
        result.ReplaceAll("$perception_type",npc->GetLastPerception()->GetType());
    }

    NPCDebug(npc, 10, "Replaced variables in '%s' to get '%s'",object.GetDataSafe(),result.GetDataSafe());

    return result;
}


bool psGameObject::ReplaceNPCVariablesBool(NPC* npc, const csString &object)
{
    csString replaced = ReplaceNPCVariables(npc, object);

    return (replaced.CompareNoCase("true") || replaced.CompareNoCase("yes"));
}



//---------------------------------------------------------------------------
