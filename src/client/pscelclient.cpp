/*
 * psCelClient.cpp by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
 *  Implements the various things relating to the CEL for the client.
 */
#include <psconfig.h>

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/scf.h>
#include <csutil/csstring.h>
#include <cstool/collider.h>
#include <iutil/cfgmgr.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/stringarray.h>
#include <iutil/vfs.h>
#include <ivaria/collider.h>
#include <iengine/engine.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/sector.h>
#include <iengine/scenenode.h>
#include <imesh/genmesh.h>
#include <imesh/object.h>
#include <imesh/objmodel.h>
#include <imesh/spritecal3d.h>
#include <imesh/nullmesh.h>
#include <ivaria/engseq.h>
#include <ivideo/material.h>
#include <csgeom/math3d.h>

//============================
// Cal3D includes
//============================
#include "cal3d/cal3d.h"

//=============================================================================
// Library Includes
//=============================================================================
#include "effects/pseffect.h"
#include "effects/pseffectmanager.h"

#include "engine/psworld.h"
#include "engine/solid.h"
#include "engine/colldet.h"

#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "util/psconst.h"
#include "net/connection.h"

#include "gui/pawsinfowindow.h"
#include "gui/pawsconfigkeys.h"
#include "gui/inventorywindow.h"
#include "gui/chatwindow.h"
#include "gui/pawslootwindow.h"

#include "paws/pawsmanager.h"
#include "paws/pawsobjectview.h"


//=============================================================================
// Application Includes
//=============================================================================
#include <ibgloader.h>
#include "pscelclient.h"
#include "charapp.h"
#include "clientvitals.h"
#include "modehandler.h"
#include "zonehandler.h"
#include "psclientdr.h"
#include "entitylabels.h"
#include "shadowmanager.h"
#include "pscharcontrol.h"
#include "pscamera.h"
#include "psclientchar.h"
#include "pscal3dcallback.h"
#include "meshattach.h"
#include "globals.h"

psCelClient* GEMClientObject::cel = NULL;

psCelClient::psCelClient()
{
    instantiateItems = false;

    requeststatus = 0;

    clientdr        = NULL;
    gameWorld       = NULL;
    entityLabels    = NULL;
    shadowManager   = NULL;
    local_player    = NULL;
}

psCelClient::~psCelClient()
{
    delete gameWorld;

    if(msghandler)
    {
        msghandler->Unsubscribe(this, MSGTYPE_CELPERSIST);
        msghandler->Unsubscribe(this, MSGTYPE_PERSIST_WORLD);
        msghandler->Unsubscribe(this, MSGTYPE_PERSIST_ACTOR);
        msghandler->Unsubscribe(this, MSGTYPE_PERSIST_ITEM);
        msghandler->Unsubscribe(this, MSGTYPE_PERSIST_ACTIONLOCATION);
        msghandler->Unsubscribe(this, MSGTYPE_REMOVE_OBJECT);
        msghandler->Unsubscribe(this, MSGTYPE_NAMECHANGE);
        msghandler->Unsubscribe(this, MSGTYPE_GUILDCHANGE);
        msghandler->Unsubscribe(this, MSGTYPE_GROUPCHANGE);
        msghandler->Unsubscribe(this, MSGTYPE_STATS);
        msghandler->Unsubscribe(this, MSGTYPE_MECS_ACTIVATE);
    }

    delete clientdr;
    delete entityLabels;
    delete shadowManager;

    entities.DeleteAll();
    entities_hash.DeleteAll();

    // Delete effectItems
    csHash<ItemEffect*, csString>::GlobalIterator i = effectItems.GetIterator();
    while (i.HasNext())
    {
        delete i.Next();
    }
    effectItems.DeleteAll();
}



bool psCelClient::Initialize(iObjectRegistry* object_reg, MsgHandler* newmsghandler)
{
    this->object_reg = object_reg;
    entityLabels = new psEntityLabels();
    entityLabels->Initialize(object_reg, this);

    shadowManager = new psShadowManager();

    vfs =  csQueryRegistry<iVFS> (object_reg);
    if(!vfs)
    {
        return false;
    }

    msghandler = newmsghandler;
    msghandler->Subscribe(this, MSGTYPE_CELPERSIST);
    msghandler->Subscribe(this, MSGTYPE_PERSIST_WORLD);
    msghandler->Subscribe(this, MSGTYPE_PERSIST_ACTOR);
    msghandler->Subscribe(this, MSGTYPE_PERSIST_ITEM);
    msghandler->Subscribe(this, MSGTYPE_PERSIST_ACTIONLOCATION);
    msghandler->Subscribe(this, MSGTYPE_REMOVE_OBJECT);
    msghandler->Subscribe(this, MSGTYPE_NAMECHANGE);
    msghandler->Subscribe(this, MSGTYPE_GUILDCHANGE);
    msghandler->Subscribe(this, MSGTYPE_GROUPCHANGE);
    msghandler->Subscribe(this, MSGTYPE_STATS);

    // mechanisms
    msghandler->Subscribe(this, MSGTYPE_MECS_ACTIVATE);

    clientdr = new psClientDR;
    if(!clientdr->Initialize(object_reg, this, msghandler))
    {
        delete clientdr;
        clientdr = NULL;
        return false;
    }

    unresSector = psengine->GetEngine()->CreateSector("SectorWhereWeKeepEntitiesResidingInUnloadedMaps");

    instantiateItems = psengine->GetConfig()->GetBool("PlaneShift.Items.Instantiate", false);

    LoadEffectItems();

    return true;
}

GEMClientObject* psCelClient::FindObject(EID id)
{
    return entities_hash.Get(id, NULL);
}

void psCelClient::SetMainActor(GEMClientActor* actor)
{
    CS_ASSERT(actor);

    // If updating the player we currently control, some of our data is likely
    // newer than the server's (notably DRcounter); we should keep ours.
    if(local_player && local_player->GetEID() == actor->GetEID())
        actor->CopyNewestData(*local_player);

    psengine->GetCharControl()->GetMovementManager()->SetActor(actor);
    psengine->GetPSCamera()->SetActor(actor);
    psengine->GetModeHandler()->SetEntity(actor);
    // Used for debugging
    psEngine::playerName = actor->GetName();
    local_player = actor;

}

void psCelClient::RequestServerWorld()
{
    psPersistWorldRequest world;
    msghandler->SendMessage(world.msg);

    requeststatus=1;
}

bool psCelClient::IsReady()
{
    return local_player != NULL;
}

void psCelClient::HandleWorld(MsgEntry* me)
{
    psPersistWorld mesg(me);
    psNewSectorMessage cross("", mesg.sector, mesg.pos);
    msghandler->Publish(cross.msg);

    gameWorld = new psWorld();
    gameWorld->Initialize(object_reg);

    requeststatus = 0;
}

void psCelClient::RequestActor()
{
    psPersistActorRequest mesg;
    msghandler->SendMessage(mesg.msg);
}

void psCelClient::HandleActor(MsgEntry* me)
{
    // Check for loading errors first
    if(psengine->LoadingError() || !GetClientDR()->GetMsgStrings())
    {
        Error1("Ignoring Actor message.  LoadingError or missing strings table!");
        psengine->FatalError("Cannot load main actor. Error during loading.");
        return;
    }
    psPersistActor msg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());

    GEMClientActor* actor = new GEMClientActor(this, msg);

    // Extra steps for a controlled actor/the main player:
    if((!local_player && player_name == msg.name) ||
       (local_player && local_player->GetEID() == msg.entityid))
    {
        SetMainActor(actor);

        // This triggers the server to update our proxlist
        //local_player->SendDRUpdate(PRIORITY_LOW,GetClientDR()->GetMsgStrings());

        //update the window title with the char name
        psengine->UpdateWindowTitleInformations();
    }

    AddEntity(actor);
}

void psCelClient::HandleItem(MsgEntry* me)
{
    psPersistItem msg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());
    GEMClientItem* item = new GEMClientItem(this, msg);
    AddEntity(item);
}

void psCelClient::HandleActionLocation(MsgEntry* me)
{
    psPersistActionLocation msg(me);
    GEMClientActionLocation* action = new GEMClientActionLocation(this, msg);
    AddEntity(action);
    actions.Push(action);
}

void psCelClient::AddEntity(GEMClientObject* obj)
{
    CS_ASSERT(obj);

    // If we already have an entity with this ID, remove it.  The server might
    // have sent an updated version (say, with a different mesh); alternatively
    // we may have simply missed the psRemoveObject message.
    GEMClientObject* existing = (GEMClientObject*) FindObject(obj->GetEID());
    if(existing)
    {
        // If we're removing the targeted entity, update the target to the new one:
        GEMClientObject* target = psengine->GetCharManager()->GetTarget();
        if(target == existing)
        {
            //we need to forcefully unlock the old target as this is going to get deleted.
            psengine->GetCharManager()->LockTarget(false);
            psengine->GetCharManager()->SetTarget(obj, "select", false);
            psengine->GetPSCamera()->npcTargetReplaceIfEqual(existing, obj);
        }

        Debug3(LOG_CELPERSIST, 0, "Found existing entity >%s< with %s - removing.", existing->GetName(), ShowID(existing->GetEID()));
        RemoveObject(existing);

    }
    entities.Push(obj);
    entities_hash.Put(obj->GetEID(), obj);
}

void psCelClient::HandleObjectRemoval(MsgEntry* me)
{
    psRemoveObject mesg(me);

    ForceEntityQueues();

    GEMClientObject* entity = FindObject(mesg.objectEID);

    if(entity)
    {
        RemoveObject(entity);
        Debug2(LOG_CELPERSIST, 0, "Object %s Removed", ShowID(mesg.objectEID));
    }
    else
    {
        Debug2(LOG_CELPERSIST, 0, "Server told us to remove %s, but it does not seem to exist.", ShowID(mesg.objectEID));
    }
}

void psCelClient::LoadEffectItems()
{
    if(vfs->Exists("/planeshift/art/itemeffects.xml"))
    {
        csRef<iDocument> doc = ParseFile(psengine->GetObjectRegistry(), "/planeshift/art/itemeffects.xml");
        if(!doc)
        {
            Error2("Couldn't parse file %s", "/planeshift/art/itemeffects.xml");
            return;
        }

        csRef<iDocumentNode> root = doc->GetRoot();
        if(!root)
        {
            Error2("The file(%s) doesn't have a root", "/planeshift/art/itemeffects.xml");
            return;
        }

        csRef<iDocumentNodeIterator> itemeffects = root->GetNodes("itemeffect");
        while(itemeffects->HasNext())
        {
            csRef<iDocumentNode> itemeffect = itemeffects->Next();
            csRef<iDocumentNodeIterator> effects = itemeffect->GetNodes("pseffect");
            csRef<iDocumentNodeIterator> lights = itemeffect->GetNodes("light");
            if(effects->HasNext() || lights->HasNext())
            {
                ItemEffect* ie = new ItemEffect();
                while(effects->HasNext())
                {
                    csRef<iDocumentNode> effect = effects->Next();
                    Effect* eff = new Effect();
                    eff->rotateWithMesh = effect->GetAttributeValueAsBool("rotatewithmesh", false);
                    eff->effectname = csString(effect->GetNode("effectname")->GetContentsValue());
                    eff->effectoffset = csVector3(effect->GetNode("offset")->GetAttributeValueAsFloat("x"),
                                                  effect->GetNode("offset")->GetAttributeValueAsFloat("y"),
                                                  effect->GetNode("offset")->GetAttributeValueAsFloat("z"));
                    ie->effects.PushSmart(eff);
                }
                while(lights->HasNext())
                {
                    csRef<iDocumentNode> light = lights->Next();
                    Light* li = new Light();
                    li->colour = csColor(light->GetNode("colour")->GetAttributeValueAsFloat("r"),
                                         light->GetNode("colour")->GetAttributeValueAsFloat("g"),
                                         light->GetNode("colour")->GetAttributeValueAsFloat("b"));
                    li->lightoffset = csVector3(light->GetNode("offset")->GetAttributeValueAsFloat("x"),
                                                light->GetNode("offset")->GetAttributeValueAsFloat("y"),
                                                light->GetNode("offset")->GetAttributeValueAsFloat("z"));
                    li->radius = light->GetNode("radius")->GetContentsValueAsFloat();
                    ie->lights.PushSmart(li);
                }
                ie->activeOnGround = itemeffect->GetAttributeValueAsBool("activeonground");
                csString meshname = itemeffect->GetAttributeValue("meshname");
                if (!effectItems.Get(meshname, 0))
                    effectItems.Put(meshname, ie);
                else
                    delete ie;
            }
        }
    }
    else
    {
        Error1("Could not load itemeffects.xml!");
    }
}

void psCelClient::HandleItemEffect(const char* factName, iMeshWrapper* mw, bool onGround, const char* slot,
                                   csHash<int, csString>* effectids, csHash<int, csString>* lightids)
{
    ItemEffect* ie = effectItems.Get(factName, 0);
    if(ie)
    {
        if(onGround && !ie->activeOnGround)
        {
            return;
        }

        if(!mw)
        {
            Error2("Error loading effect for item %s. iMeshWrapper is null.", factName);
            return;
        }

        csString shaderLevel = psengine->GetConfig()->GetStr("PlaneShift.Graphics.Shaders");
        if(shaderLevel == "Highest" || shaderLevel == "High")
        {
            for(size_t i=0; i<ie->lights.GetSize(); i++)
            {
                Light* l = ie->lights.Get(i);
                unsigned int id = psengine->GetEffectManager()->AttachLight(factName, l->lightoffset,
                                  l->radius, l->colour, mw);

                if(!id)
                {
                    Error2("Failed to create light on item %s!", factName);
                }
                else if(slot && lightids)
                {
                    lightids->PutUnique(slot, id);
                }
            }
        }

        csString particleLevel = psengine->GetConfig()->GetStr("PlaneShift.Graphics.Particles", "High");
        if(particleLevel == "High")
        {
            for(size_t i=0; i<ie->effects.GetSize(); i++)
            {
                Effect* e = ie->effects.Get(i);
                unsigned int id = psengine->GetEffectManager()->RenderEffect(e->effectname, e->effectoffset, mw, 0,
                                  csVector3(0,1,0), 0, e->rotateWithMesh);
                if(!id)
                {
                    Error3("Failed to load effect %s on item %s!", e->effectname.GetData(), factName);
                }
                else if(slot && effectids)
                {
                    effectids->PutUnique(slot, id);
                }
            }
        }
    }
}

csList<UnresolvedPos*>::Iterator psCelClient::FindUnresolvedPos(GEMClientObject* entity)
{
    csList<UnresolvedPos*>::Iterator posIter(unresPos);
    while(posIter.HasNext())
    {
        UnresolvedPos* pos = posIter.Next();
        if(pos->entity == entity)
            return posIter;
    }
    return csList<UnresolvedPos*>::Iterator();
}

void psCelClient::RemoveObject(GEMClientObject* entity)
{
    if(entity == local_player)
    {
        Error1("Nearly deleted player's object");
        return;
    }

    if(psengine->GetCharManager()->GetTarget() == entity)
    {
        pawsWidget* widget = PawsManager::GetSingleton().FindWidget("InteractWindow");
        if(widget)
        {
            widget->Hide();
        }
        //we need to forcefully unlock the target as this is going to get deleted.
        psengine->GetCharManager()->LockTarget(false);
        psengine->GetCharManager()->SetTarget(NULL, "select");
    }

    entityLabels->RemoveObject(entity);
    shadowManager->RemoveShadow(entity);
    psengine->GetPSCamera()->npcTargetReplaceIfEqual(entity, NULL);
    pawsLootWindow* loot = (pawsLootWindow*)PawsManager::GetSingleton().FindWidget("LootWindow");
    if(loot && loot->GetLootingActor() == entity->GetEID())
    {
        loot->Hide();
    }

    // delete record in unresolved position list
    csList<UnresolvedPos*>::Iterator posIter = FindUnresolvedPos(entity);
    if(posIter.HasCurrent())
    {
        //Error2("Removing deleted entity %s from unresolved",entity->GetName());
        delete *posIter;
        unresPos.Delete(posIter);
    }

    // Delete from action list if action
    if(dynamic_cast<GEMClientActionLocation*>(entity))
        actions.Delete(static_cast<GEMClientActionLocation*>(entity));

    entities_hash.Delete(entity->GetEID(), entity);
    entities.Delete(entity);
}

bool psCelClient::IsMeshSubjectToAction(const char* sector,const char* mesh)
{
    if(!mesh)
        return false;

    for(size_t i = 0; i < actions.GetSize(); i++)
    {
        GEMClientActionLocation* action = actions[i];
        const char* sec = action->GetSector()->QueryObject()->GetName();

        if(!strcmp(action->GetMeshName(),mesh) && !strcmp(sector,sec))
            return true;
    }

    return false;
}

GEMClientActor* psCelClient::GetActorByName(const char* name, bool trueName) const
{
    size_t len = entities.GetSize();
    csString testName, firstName;
    csString csName(name);
    csName = csName.Downcase();

    for(size_t a=0; a<len; ++a)
    {
        GEMClientActor* actor = dynamic_cast<GEMClientActor*>(entities[a]);
        if(!actor)
            continue;

        testName = actor->GetName(trueName);
        firstName = testName.Slice(0, testName.FindFirst(' ')).Downcase();
        if(firstName == csName)
            return actor;
    }
    return 0;
}

void psCelClient::HandleNameChange(MsgEntry* me)
{
    psUpdateObjectNameMessage msg(me);
    GEMClientObject* object = FindObject(msg.objectID);

    if(!object)
    {
        Error2("Warning: Got rename message, but couldn't find actor %s!", ShowID(msg.objectID));
        return;
    }

    // Slice the names into parts
    csString oldFirstName = object->GetName();
    oldFirstName = oldFirstName.Slice(0,oldFirstName.FindFirst(' '));

    csString newFirstName = msg.newObjName;
    newFirstName = newFirstName.Slice(0,newFirstName.FindFirst(' '));

    // Remove old name from chat auto complete and add new
    pawsChatWindow* chat = (pawsChatWindow*)(PawsManager::GetSingleton().FindWidget("ChatWindow"));
    chat->RemoveAutoCompleteName(oldFirstName);
    chat->AddAutoCompleteName(newFirstName);

    // Apply the new name
    object->ChangeName(msg.newObjName);

    // We don't have a label over our own char
    if(GetMainPlayer() != object)
        entityLabels->OnObjectArrived(object);
    else //we have to update the window title
        psengine->UpdateWindowTitleInformations();

    // If object is targeted update the target information.
    if(psengine->GetCharManager()->GetTarget() == object)
    {
        if(object->GetType() == GEM_ACTOR)
            PawsManager::GetSingleton().Publish("sTargetName",((GEMClientActor*)object)->GetName(false));
        else
            PawsManager::GetSingleton().Publish("sTargetName",object->GetName());
    }
}

void psCelClient::HandleGuildChange(MsgEntry* me)
{
    psUpdatePlayerGuildMessage msg(me);

    // Change every entity
    for(size_t i = 0; i < msg.objectID.GetSize(); i++)
    {
        int id = (int)msg.objectID[i];
        GEMClientActor* actor = dynamic_cast<GEMClientActor*>(FindObject(id));

        //Apply the new name
        if(!actor)
        {
            Error2("Couldn't find actor %d!",id);
            continue;
        }

        actor->SetGuildName(msg.newGuildName);

        // we don't have a label over our own char
        if(GetMainPlayer() != actor)
            entityLabels->OnObjectArrived(actor);
    }
}

void psCelClient::HandleGroupChange(MsgEntry* me)
{
    psUpdatePlayerGroupMessage msg(me);
    GEMClientActor* actor = (GEMClientActor*)FindObject(msg.objectID);

    //Apply the new name
    if(!actor)
    {
        Error2("Couldn't find %s, ignoring group change.", ShowID(msg.objectID));
        return;
    }
    Debug4(LOG_GROUP,0,"Got group update for actor %s (%s) to group %d\n", actor->GetName(), ShowID(actor->GetEID()), msg.groupID);
    unsigned int oldGroup = actor->GetGroupID();
    actor->SetGroupID(msg.groupID);

    // repaint label
    if(GetMainPlayer() != actor)
        entityLabels->RepaintObjectLabel(actor);
    else // If it's us, we need to repaint every label with the group = ours
    {
        for(size_t i =0; i < entities.GetSize(); i++)
        {
            GEMClientObject* object = entities[i];
            GEMClientActor* act = dynamic_cast<GEMClientActor*>(object);
            if(!act)
                continue;

            if(act != actor && (actor->IsGroupedWith(act) || (oldGroup != 0 && oldGroup == act->GetGroupID())))
                entityLabels->RepaintObjectLabel(act);
        }
    }

}


void psCelClient::HandleStats(MsgEntry* me)
{
    psStatsMessage msg(me);

    PawsManager::GetSingleton().Publish("fmaxhp", msg.hp);
    PawsManager::GetSingleton().Publish("fmaxmana", msg.mana);
    PawsManager::GetSingleton().Publish("fmaxweight", msg.weight);
    PawsManager::GetSingleton().Publish("fmaxcapacity", msg.capacity);

}

void psCelClient::HandleMecsActivate(MsgEntry* me)
{
    psMechanismActivateMessage msg(me);

    Debug2(LOG_ACTIONLOCATION,0,"Received HandleMecsActivate message mesh: %s!", msg.meshName.GetData());

    Debug2(LOG_ACTIONLOCATION,0,"Received HandleMecsActivate message move: %s!", msg.move.GetData());

    Debug2(LOG_ACTIONLOCATION,0,"Received HandleMecsActivate message rot: %s!", msg.rot.GetData());

    csRef<iMeshWrapper> objectWrapper = psengine->GetEngine()->FindMeshObject(msg.meshName);

    // object found
    if(objectWrapper)
    {
        // name of the sequence needs to be unique
        csString sequenceName = msg.meshName.Append(msg.move).Append(msg.rot);
        // creates a sequence to move/rotate the object
        csRef<iEngineSequenceManager> sequenceMngr = csQueryRegistryOrLoad<iEngineSequenceManager> (object_reg,"crystalspace.utilities.sequence.engine", false);
        csRef<iSequenceWrapper> sequence = sequenceMngr->CreateSequence(sequenceName);
        csRef<iParameterESM> meshParam = sequenceMngr->CreateParameterESM(objectWrapper);
        const int ANIM_DURATION = 5000;
        // move the object
        if (msg.move && msg.move!="")
        {
            Debug2(LOG_ACTIONLOCATION,0,"Found mesh move! %s", objectWrapper->QueryObject()->GetName());
            csStringArray tokens;
            tokens.SplitString(msg.move, ",");
            csVector3 v(atof(tokens.Get(0)), atof(tokens.Get(1)), atof(tokens.Get(2)));
            sequence->AddOperationMoveDuration(0,meshParam,v,ANIM_DURATION);
            sequenceMngr->RunSequenceByName(sequenceName,0);
        }
        // rotates the object
        else if (msg.rot && msg.rot!="")
        {
            Debug2(LOG_ACTIONLOCATION,0,"Found mesh rotate! %s", objectWrapper->QueryObject()->GetName());
            csStringArray tokens;
            tokens.SplitString(msg.rot, ",");

            sequence->AddOperationRotateDuration (0, meshParam, 0, atof(tokens.Get(0)), 1, atof(tokens.Get(1)), 2, atof(tokens.Get(2)), csVector3(0,0,0), ANIM_DURATION, true);
            sequenceMngr->RunSequenceByName(sequenceName,0);
        }
    }
}

void psCelClient::ForceEntityQueues()
{
    while(!newActorQueue.IsEmpty())
    {
        csRef<MsgEntry> me = newActorQueue.Pop();
        HandleActor(me);
    }
    if(!newItemQueue.IsEmpty())
    {
        csRef<MsgEntry> me = newItemQueue.Pop();
        HandleItem(me);
    }
}

void psCelClient::CheckEntityQueues()
{
    if(newActorQueue.GetSize())
    {
        csRef<MsgEntry> me = newActorQueue.Pop();
        HandleActor(me);
        return;
    }
    if(newItemQueue.GetSize())
    {
        csRef<MsgEntry> me = newItemQueue.Pop();
        HandleItem(me);
        return;
    }
}

void psCelClient::Update(bool loaded)
{
    if(local_player)
    {
        if(psengine->BackgroundWorldLoading())
        {
            const char* sectorName = local_player->GetSector()->QueryObject()->GetName();
            if(!sectorName)
                return;

            // Update loader.
            psengine->GetLoader()->UpdatePosition(local_player->Pos(), sectorName, false);
        }
        else
        {
            psengine->GetLoader()->ContinueLoading(false);
        }

        //const char* sectorName = local_player->GetSector()->QueryObject()->GetName();
        // Check if we're inside a water area.
        //csColor4* waterColour = 0;
        //csVector3 pos = psengine->GetPSCamera()->GetPosition();
        /*if(psengine->GetLoader()->InWaterArea(sectorName, &pos, &waterColour))
        {
            psWeatherMessage::NetWeatherInfo fog;
            //fog.fogType = CS_FOG_MODE_EXP;
            fog.r = (*waterColour)[0];
            fog.g = (*waterColour)[1];
            fog.b = (*waterColour)[2];
            fog.has_downfall = false;
            fog.has_fog = true;
            fog.has_lightning = false;
            fog.sector = sectorName;
            fog.fog_density = 100000;
            fog.fog_fade = 0;
            psengine->GetModeHandler()->ProcessFog(fog);
        }
        else
        {
            printf("Removing\n");
            psWeatherMessage::NetWeatherInfo fog;
            fog.has_downfall = false;
            fog.has_fog = false;
            fog.has_lightning = false;
            fog.sector = sectorName;
            fog.fog_density = 0;
            fog.fog_fade = 0;
            psengine->GetModeHandler()->ProcessFog(fog);
        }*/
    }

    if(loaded)
    {
        for(size_t i =0; i < entities.GetSize(); i++)
        {
            entities[i]->Update();
        }

        shadowManager->UpdateShadows();
    }
}

void psCelClient::HandleMessage(MsgEntry* me)
{
    switch(me->GetType())
    {
        case MSGTYPE_STATS:
        {
            HandleStats(me);
            break;
        }

        case MSGTYPE_PERSIST_WORLD:
        {
            HandleWorld(me);
            break;
        }

        case MSGTYPE_PERSIST_ACTOR:
        {
            // Delaying the processing for the main actor causes
            // problems with replaying or skipping animation and
            // velocity changes so handle it now rather than queueing.
            if(!local_player ||
               local_player->GetEID() == psPersistActor::PeekEID(me))
            {
                HandleActor(me);
            }
            else
            {
                newActorQueue.Push(me);
            }
            break;
        }

        case MSGTYPE_PERSIST_ITEM:
        {
            newItemQueue.Push(me);
            break;

        }
        case MSGTYPE_PERSIST_ACTIONLOCATION:
        {
            HandleActionLocation(me);
            break;
        }

        case MSGTYPE_REMOVE_OBJECT:
        {
            HandleObjectRemoval(me);
            break;
        }

        case MSGTYPE_NAMECHANGE:
        {
            HandleNameChange(me);
            break;
        }

        case MSGTYPE_GUILDCHANGE:
        {
            HandleGuildChange(me);
            break;
        }

        case MSGTYPE_GROUPCHANGE:
        {
            HandleGroupChange(me);
            break;
        }

        case MSGTYPE_MECS_ACTIVATE:
        {
            HandleMecsActivate(me);
            break;
        }

        default:
        {
            Error1("CEL UNKNOWN MESSAGE!!!");
            break;
        }
    }
}

void psCelClient::OnRegionsDeleted(csArray<iCollection*> &regions)
{
    size_t entNum;

    for(entNum = 0; entNum < entities.GetSize(); entNum++)
    {
        csRef<iSector> sector = entities[entNum]->GetSector();
        if(sector.IsValid())
        {
            // Shortcut the lengthy region check if possible
            if(IsUnresSector(sector))
                continue;

            bool unresolved = true;

            iSector* sectorToBeDeleted = 0;

            // Are all the sectors going to be unloaded?
            iSectorList* sectors = entities[entNum]->GetSectors();
            for(int i = 0; i<sectors->GetCount(); i++)
            {
                // Get the iRegion this sector belongs to
                csRef<iCollection> region =  scfQueryInterfaceSafe<iCollection> (sectors->Get(i)->QueryObject()->GetObjectParent());
                if(regions.Find(region)==csArrayItemNotFound)
                {
                    // We've found a sector that won't be unloaded so the mesh won't need to be moved
                    unresolved = false;
                    break;
                }
                else
                {
                    sectorToBeDeleted = sectors->Get(i);
                }
            }

            if(unresolved && sectorToBeDeleted)
            {
                // All the sectors the mesh is in are going to be unloaded
                Warning1(LOG_ANY,"Moving entity to temporary sector");
                // put the mesh to the sector that server uses for keeping meshes located in unload maps
                HandleUnresolvedPos(entities[entNum], entities[entNum]->GetPosition(), csVector3(0),
                                    sectorToBeDeleted->QueryObject()->GetName());
            }
        }
    }
}

void psCelClient::HandleUnresolvedPos(GEMClientObject* entity, const csVector3 &pos, const csVector3 &rot, const csString &sector)
{
    //Error3("Handling unresolved %s at %s", entity->GetName(),sector.GetData());
    csList<UnresolvedPos*>::Iterator posIter = FindUnresolvedPos(entity);
    // if we already have a record about this entity, just update it, otherwise create new one
    if(posIter.HasCurrent())
    {
        (*posIter)->pos = pos;
        (*posIter)->rot = rot;
        (*posIter)->sector = sector;
        //Error1("-editing");
    }
    else
    {
        unresPos.PushBack(new UnresolvedPos(entity, pos, rot, sector));
        //Error1("-adding");
    }

    GEMClientActor* actor = dynamic_cast<GEMClientActor*>(entity);
    if(actor)
    {
        // This will disable CD algorithms temporarily
        actor->Movement().SetOnGround(true);

        if(actor == local_player && psengine->GetCharControl())
            psengine->GetCharControl()->GetMovementManager()->StopAllMovement();
        actor->StopMoving(true);
    }

    // move the entity to special sector where we keep entities with unresolved position
    entity->SetPosition(pos, rot, unresSector);
}

void psCelClient::OnMapsLoaded()
{
    // look if some of the unresolved entities can't be resolved now
    csList<UnresolvedPos*>::Iterator posIter(unresPos);

    ++posIter;
    while(posIter.HasCurrent())
    {
        UnresolvedPos* pos = posIter.FetchCurrent();
        iSector* sector = psengine->GetEngine()->GetSectors()->FindByName(pos->sector);
        if(sector)
        {
            pos->entity->SetPosition(pos->pos, pos->rot, sector);

            GEMClientActor* actor = dynamic_cast<GEMClientActor*>(pos->entity);
            if(actor)
                // we are now in a physical sector
                actor->Movement().SetOnGround(false);

            delete *posIter;
            unresPos.Delete(posIter);
            ++posIter;
        }
        else
            ++posIter;
    }

    GEMClientActor* actor = GetMainPlayer();
    if(actor)
        actor->Movement().SetOnGround(false);
}

void psCelClient::PruneEntities()
{
    // Only perform every 3 frames
    static int count = 0;
    count++;

    if(count % 3 != 0)
        return;
    else
        count = 0;

    for(size_t entNum = 0; entNum < entities.GetSize(); entNum++)
    {
        if((GEMClientActor*) entities[entNum] == local_player)
            continue;

        csRef<iMeshWrapper> mesh = entities[entNum]->GetMesh();
        if(mesh != NULL)
        {
            GEMClientActor* actor = dynamic_cast<GEMClientActor*>(entities[entNum]);
            if(actor)
            {
                csVector3 vel;
                vel = actor->Movement().GetVelocity();
                if(vel.y < -50)             // Large speed puts too much stress on CPU
                {
                    Debug3(LOG_ANY, 0, "Disabling CD on actor %s(%s)", actor->GetName(), ShowID(actor->GetEID()));
                    actor->Movement().SetOnGround(false);
                    // Reset velocity
                    actor->StopMoving(true);
                }
            }
        }
    }
}


void psCelClient::AttachObject(iObject* object, GEMClientObject* clientObject)
{
    csRef<psGemMeshAttach> attacher;
    attacher.AttachNew(new psGemMeshAttach(clientObject));
    attacher->SetName(clientObject->GetName());  // @@@ For debugging mostly.
    csRef<iObject> attacher_obj(scfQueryInterface<iObject> (attacher));
    object->ObjAdd(attacher_obj);
}


void psCelClient::UnattachObject(iObject* object, GEMClientObject* clientObject)
{
    csRef<psGemMeshAttach> attacher(CS::GetChildObject<psGemMeshAttach>(object));
    if(attacher.IsValid())
    {
        if(attacher->GetObject() == clientObject)
        {
            csRef<iObject> attacher_obj(scfQueryInterface<iObject> (attacher));
            object->ObjRemove(attacher_obj);
        }
    }
}

GEMClientObject* psCelClient::FindAttachedObject(iObject* object)
{
    GEMClientObject* found = 0;

    csRef<psGemMeshAttach> attacher(CS::GetChildObject<psGemMeshAttach>(object));
    if(attacher.IsValid())
    {
        found = attacher->GetObject();
    }

    return found;
}


csArray<GEMClientObject*> psCelClient::FindNearbyEntities(iSector* sector, const csVector3 &pos, float radius, bool doInvisible)
{
    csArray<GEMClientObject*> list;

    csRef<iMeshWrapperIterator> obj_it = psengine->GetEngine()->GetNearbyMeshes(sector, pos, radius);

    while(obj_it->HasNext())
    {
        iMeshWrapper* m = obj_it->Next();
        if(!doInvisible)
        {
            bool invisible = m->GetFlags().Check(CS_ENTITY_INVISIBLE);
            if(invisible)
                continue;
        }
        GEMClientObject* object = FindAttachedObject(m->QueryObject());

        if(object)
        {
            list.Push(object);
        }
    }
    return list;
}

csPtr<InstanceObject> psCelClient::FindInstanceObject(const char* name) const
{
    return csPtr<InstanceObject> (instanceObjects.Get(name, csRef<InstanceObject>()));
}

void psCelClient::AddInstanceObject(const char* name, csRef<InstanceObject> object)
{
    instanceObjects.Put(name, object);
}

void psCelClient::RemoveInstanceObject(const char* name)
{
    instanceObjects.Delete(name);
}

void psCelClient::replaceRacialGroup(csString &string)
{
    //avoids useless elaborations
    if(!string.Length()) return;
    //safe defaults
    csString HelmReplacement("stonebm");
    csString BracerReplacement("stonebm");
    csString BeltReplacement("stonebm");
    csString CloakReplacement("stonebm");
    if(GetMainPlayer())
    {
        HelmReplacement = GetMainPlayer()->helmGroup;
        BracerReplacement = GetMainPlayer()->BracerGroup;
        BeltReplacement = GetMainPlayer()->BeltGroup;
        CloakReplacement = GetMainPlayer()->CloakGroup;
    }
    Debug2(LOG_CELPERSIST,0,"DEBUG: replacing string %s",string.GetData());
    string.ReplaceAll("$H", HelmReplacement);
    string.ReplaceAll("$B", BracerReplacement);
    string.ReplaceAll("$E", BeltReplacement);
    string.ReplaceAll("$C", CloakReplacement);
    Debug2(LOG_CELPERSIST,0," replaced:  %s \n",string.GetData());
}

//-------------------------------------------------------------------------------


GEMClientObject::GEMClientObject()
{
    entitylabel = NULL;
    shadow = 0;
    hasLabel = false;
    hasShadow = false;
    type = 0;
    flags = 0;
}

GEMClientObject::GEMClientObject(psCelClient* cel, EID id) : eid(id)
{
    if(!this->cel)
        this->cel = cel;

    eid = id;
    entitylabel = NULL;
    shadow = 0;
    hasLabel = false;
    hasShadow = false;
    type = 0;
    flags = 0;
}

GEMClientObject::~GEMClientObject()
{
    if(pcmesh.IsValid())
    {
        cel->GetShadowManager()->RemoveShadow(this);
        cel->UnattachObject(pcmesh->QueryObject(), this);
        psengine->GetEngine()->RemoveObject(pcmesh);
    }
    psengine->UnregisterDelayedLoader(this);
}

int GEMClientObject::GetMasqueradeType(void)
{
    return type;
}

void GEMClientObject::SetPosition(const csVector3 &pos, const csVector3 &rot, iSector* sector)
{
    if(pcmesh.IsValid())
    {
        if(instance.IsValid())
        {
            // Update the sector and position of real mesh.
            if(pcmesh->GetMovable()->GetSectors()->GetCount() > 0)
            {
                iSector* old = pcmesh->GetMovable()->GetSectors()->Get(0);
                if(old != sector)
                {
                    // Remove the old sector.
                    if(instance->sectors.Delete(old) && instance->sectors.Find(old) == csArrayItemNotFound)
                    {
                        instance->pcmesh->GetMovable()->GetSectors()->Remove(old);
                    }

                    // Add the new sector.
                    if(instance->sectors.Find(sector) == csArrayItemNotFound)
                    {
                        instance->pcmesh->GetMovable()->GetSectors()->Add(sector);
                    }

                    instance->sectors.Push(sector);
                }
            }
            else
            {
                if(instance->pcmesh->GetMovable()->GetSectors()->Find(sector) == (int)csArrayItemNotFound)
                {
                    instance->pcmesh->GetMovable()->GetSectors()->Add(sector);
                }

                instance->sectors.Push(sector);
            }

            instance->pcmesh->GetMovable()->SetPosition(0.0f);
            instance->pcmesh->GetMovable()->UpdateMove();
        }

        if(sector)
            pcmesh->GetMovable()->SetSector(sector);

        pcmesh->GetMovable()->SetPosition(pos);

        // Rotation
        Rotate(rot.x, rot.y, rot.z);

        // Rotate updates the movable and instance transform for us
        // therefore it's left out here
    }
}

void GEMClientObject::Rotate(float xRot, float yRot, float zRot)
{
    // calculate the rotation matrix from the pitch-roll-yaw angles
    csYRotMatrix3 pitch(xRot);
    csYRotMatrix3 roll(yRot);
    csZRotMatrix3 yaw(zRot);

    // obtain current transform
    const csReversibleTransform &movTrans = pcmesh->GetMovable()->GetTransform();

    // obtain the point to use as base for the rotation
    csVector3 rotBase = (GetBBox().GetCenter() / movTrans) - movTrans.GetOrigin();

    // obtain orignal translation
    csVector3 origTrans = movTrans.GetO2TTranslation();

    // create the final transformation
    // move to center - apply roll and yaw
    csReversibleTransform trans(roll*yaw,-rotBase);
    // apply pitch, revert move to center and move to final position
    trans = trans * csReversibleTransform(pitch,rotBase+origTrans);

    // set new transformation
    pcmesh->GetMovable()->SetTransform(trans);
    pcmesh->GetMovable()->UpdateMove();

    // Set instancing transform.
    if(instance.IsValid())
    {
        position->SetValue(trans);
    }
}

csVector3 GEMClientObject::GetPosition()
{
    return pcmesh->GetMovable()->GetFullPosition();
}

float GEMClientObject::GetRotation()
{
    // Rotation
    csMatrix3 transf = pcmesh->GetMovable()->GetTransform().GetT2O();
    return psWorld::Matrix2YRot(transf);
}

iSector* GEMClientObject::GetSector() const
{
    if(pcmesh && pcmesh->GetMovable()->InSector())
    {
        return pcmesh->GetMovable()->GetSectors()->Get(0);
    }
    return NULL;
}

iSectorList* GEMClientObject::GetSectors() const
{
    return pcmesh->GetMovable()->GetSectors();
}

const csBox3 &GEMClientObject::GetBBox() const
{
    if(instance.IsValid())
    {
        return instance->bbox;
    }
    else
    {
        return pcmesh->GetMeshObject()->GetObjectModel()->GetObjectBoundingBox();
    }
}

iMeshWrapper* GEMClientObject::GetMesh() const
{
    return pcmesh;
}

void GEMClientObject::Update()
{
}

void GEMClientObject::Move(const csVector3 &pos, const csVector3 &rotangle, const char* room)
{
    // If we're moving to a new sector, wait until we're finished loading.
    // If we're moving to the same sector, continue.
    if(!psengine->GetZoneHandler()->IsLoading() ||
            (GetSector() != NULL && strcmp(GetSector()->QueryObject()->GetName(), room) == 0))
    {
        iSector* sector = psengine->GetEngine()->FindSector(room);
        if(sector)
        {
            GEMClientObject::SetPosition(pos, rotangle, sector);
            return;
        }
    }

    // Else wait until the sector is loaded.
    cel->HandleUnresolvedPos(this, pos, rotangle, room);
}

void GEMClientObject::SubstituteRacialMeshFact()
{
    // Items like helmets, bracers, etc. have racial specific meshes.
    // We substitute for the main player's race here, both so it'll be
    // the right one when equipped, but also so if the player drops it,
    // it continues to look the same as it did before
    csString HelmReplacement("stonebm");
    csString BracerReplacement("stonebm");
    csString BeltReplacement("stonebm");
    csString CloakReplacement("stonebm");
    if(cel->GetMainPlayer())
    {
        HelmReplacement = cel->GetMainPlayer()->helmGroup;
        BracerReplacement = cel->GetMainPlayer()->BracerGroup;
        BeltReplacement = cel->GetMainPlayer()->BeltGroup;
        CloakReplacement = cel->GetMainPlayer()->CloakGroup;
    }
    factName.ReplaceAll("$H", HelmReplacement);
    factName.ReplaceAll("$B", BracerReplacement);
    factName.ReplaceAll("$E", BeltReplacement);
    factName.ReplaceAll("$C", CloakReplacement);

    if(matName.Length() && matName.Find("$F") != (size_t) -1)
        matName.Empty();
}

void GEMClientObject::LoadMesh()
{
    SubstituteRacialMeshFact();

    // Start loading the real artwork in the background (if necessary).
    // When done, the nullmesh will be swapped out for the real one.
    if(factName != "nullmesh")
    {
        // Set up callback.
        psengine->RegisterDelayedLoader(this);

        // Check if the mesh is already loaded.
        CheckLoadStatus();
    }
}

void GEMClientObject::ChangeName(const char* name)
{
    this->name = name;
}

float GEMClientObject::RangeTo(GEMClientObject* obj, bool ignoreY)
{
    if(ignoreY)
    {
        csVector3 pos1 = this->GetPosition();
        csVector3 pos2 = obj->GetPosition();
        return (sqrt((pos1.x - pos2.x)*(pos1.x - pos2.x)+
                     (pos1.z - pos2.z)*(pos1.z - pos2.z)));
    }
    else
    {
        return (cel->GetWorld()->Distance(this->GetMesh() , obj->GetMesh()));
    }
}

GEMClientActor::GEMClientActor(psCelClient* cel, psPersistActor &mesg)
    : GEMClientObject(cel, mesg.entityid), linmove(new psLinearMovement(psengine->GetObjectRegistry())), post_load(new PostLoadData)
{
    chatBubbleID = 0;
    name = mesg.name;
    race = mesg.race;
    mountFactname = mesg.mountFactname;
    MounterAnim = mesg.MounterAnim;
    helmGroup = mesg.helmGroup;
    BracerGroup = mesg.bracerGroup;
    BeltGroup = mesg.beltGroup;
    CloakGroup = mesg.cloakGroup;
    type = mesg.type;
    masqueradeType = mesg.masqueradeType;
    guildName = mesg.guild;
    flags   = mesg.flags;
    groupID = mesg.groupID;
    gender = mesg.gender;
    factName = mesg.factname;
    partName = factName;
    matName = mesg.matname;
    scale = mesg.scale;
    if(scale == 0.0)
    {
        scale = 1.0;
    }
    baseScale = 1.0;
    mountScale = mesg.mountScale;
    ownerEID = mesg.ownerEID;
    lastSentVelocity = lastSentRotation = 0.0f;
    stationary = true;
    path_sent = false;
    movementMode = mesg.mode;
    serverMode = mesg.serverMode;
    alive = true;
    vitalManager = new psClientVitals;
    equipment = mesg.equipment;
    post_load->pos = mesg.pos;
    post_load->yrot = mesg.yrot;
    post_load->sector_name = mesg.sectorName;
    post_load->sector = mesg.sector;
    post_load->top = mesg.top;
    post_load->bottom = mesg.bottom;
    post_load->offset = mesg.offset;
    post_load->vel = mesg.vel;
    post_load->texParts = mesg.texParts;

    charApp = new psCharAppearance(psengine->GetObjectRegistry());

    if(helmGroup.Length() == 0)
        helmGroup = factName;

    if(BracerGroup.Length() == 0)
        BracerGroup = factName;

    if(BeltGroup.Length() == 0)
        BeltGroup = factName;

    if(CloakGroup.Length() == 0)
        CloakGroup = factName;

    Debug5(LOG_CELPERSIST, 0, "Actor %s(%s) Received, scale: %f, Bracergroup: %s", mesg.name.GetData(), ShowID(mesg.entityid),scale, BracerGroup.GetData());

    // Set up a temporary nullmesh.  The real mesh may need to be background
    // loaded, but since the mesh stores position, etc., it's important to
    // have something in the meantime so we can work with the object without crashing.
    csRef<iMeshFactoryWrapper> nullmesh = psengine->GetEngine()->FindMeshFactory("nullmesh");
    if(!nullmesh.IsValid())
    {
        nullmesh = psengine->GetEngine()->CreateMeshFactory("crystalspace.mesh.object.null", "nullmesh");
        csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (nullmesh->GetMeshObjectFactory());

        // Give the nullmesh a 1/2m cube for a bounding box, just so it
        // has something sensible while the real art's being loaded.
        csBox3 bbox;
        bbox.AddBoundingVertex(csVector3(0.5f));
        nullstate->SetBoundingBox(bbox);
    }

    pcmesh = nullmesh->CreateMeshWrapper();
    pcmesh->QueryObject()->SetName(name);

    // make sure char app won't crash
    charApp->SetMesh(pcmesh);

    linmove->InitCD(mesg.top, mesg.bottom, mesg.offset, pcmesh);
    linmove->SetDeltaLimit(0.2f);

    if(mesg.sector != NULL && !psengine->GetZoneHandler()->IsLoading())
        linmove->SetDRData(mesg.on_ground, mesg.pos, mesg.yrot, mesg.sector, mesg.vel, mesg.worldVel, mesg.ang_vel);
    else
    {
        cel->HandleUnresolvedPos(this, mesg.pos, csVector3(0, mesg.yrot, 0), mesg.sectorName);
    }

    LoadMesh();

    DRcounter = 0;  // mesg.counter cannot be trusted as it may have changed while the object was gone
    DRcounter_set = false;
    lastDRUpdateTime = 0;
}


GEMClientActor::~GEMClientActor()
{
    psengine->GetSoundManager()->RemoveObjectEntity(pcmesh, race);
    delete vitalManager;
    delete linmove;
    delete post_load;

    cal3dstate.Invalidate();
    animeshObject.Invalidate();
    if(rider.IsValid())
    {
        psengine->GetEngine()->RemoveObject(rider);
        cel->UnattachObject(rider->QueryObject(), this);
    }

    if(pcmesh.IsValid())
    {
        cel->GetShadowManager()->RemoveShadow(this);
        cel->UnattachObject(pcmesh->QueryObject(), this);
        psengine->GetEngine()->RemoveObject(pcmesh);
        pcmesh.Invalidate();
    }

    delete charApp;
}

void GEMClientActor::SwitchToRealMesh(iMeshWrapper* mesh)
{
    // Clean up old mesh.
    psengine->GetEngine()->RemoveObject(pcmesh);

    // Switch to real mesh.
    pcmesh = mesh;

    // Init CD.
    linmove->InitCD(post_load->top, post_load->bottom, post_load->offset, pcmesh);

    // Set position and other data.
    SetPosition(post_load->pos, post_load->yrot, post_load->sector);
    InitCharData(post_load->texParts, equipment);
    RefreshCal3d();
    SetAnimationVelocity(post_load->vel);
    SetMode(serverMode, true);
    if(cel->GetMainPlayer() != this && (flags & psPersistActor::NAMEKNOWN))
    {
        cel->GetEntityLabels()->OnObjectArrived(this);
        hasLabel = true;
    }
    cel->GetShadowManager()->CreateShadow(this);
    hasShadow = true;

    delete post_load;
    post_load = NULL;
}

void GEMClientActor::CopyNewestData(GEMClientActor &oldActor)
{
    DRcounter = oldActor.DRcounter;
    DRcounter_set = true;
    vitalManager->SetVitals(*oldActor.GetVitalMgr());
    SetPosition(oldActor.Pos(), oldActor.GetRotation(), oldActor.GetSector());
}

int GEMClientActor::GetAnimIndex(csStringHashReversible* msgstrings, csStringID animid)
{
    if(!cal3dstate && (!animeshObject || !animeshObject->GetSkeleton() || !animeshObject->GetSkeleton()->GetFactory()))
    {
        return -1;
    }

    int idx = anim_hash.Get(animid, -1);
    if(idx >= 0)
    {
        return idx;
    }

    // Not cached yet.
    csString animName = msgstrings->Request(animid);

    if(animName.IsEmpty()) //check if we have an hit else bug the user for the bad data
    {
        Error2("Missing animName from common strings for animid %lu!", (unsigned long int)animid);
    }
    else //no need to call this with an empty string in case of bad data so let's skip it in that case
    {
        //a cal3d mesh
        if(cal3dstate)
            idx = cal3dstate->FindAnim(animName.GetData());
        else // an animesh mesh
        {
            CS::Animation::iSkeletonAnimPacketFactory* packetFactory = animeshObject->GetSkeleton()->GetFactory()->GetAnimationPacket();
            idx = -1; //we initialize this at -1 in case we are unable to find our result
            if(packetFactory)
            {
                idx = packetFactory->FindAnimationIndex(animName);
            }
        }
    }

    if(idx >= 0)
    {
        // Cache it.
        anim_hash.Put(animid, idx);
    }

    return idx;
}

void GEMClientActor::Update()
{
    linmove->TickEveryFrame();
}

void GEMClientActor::GetLastPosition(csVector3 &pos, float &yrot, iSector* &sector)
{
    linmove->GetLastPosition(pos,yrot,sector);
}

const csVector3 GEMClientActor::GetVelocity() const
{
    return linmove->GetVelocity();
}

float GEMClientActor::GetYRotation() const
{
    return linmove->GetYRotation();
}

csVector3 GEMClientActor::Pos() const
{
    csVector3 pos;
    float yrot;
    iSector* sector;
    linmove->GetLastPosition(pos, yrot, sector);
    return pos;
}

csVector3 GEMClientActor::Rot() const
{
    csVector3 pos;
    float yrot;
    iSector* sector;
    linmove->GetLastPosition(pos, yrot, sector);
    return yrot;
}

iSector* GEMClientActor::GetSector() const
{
    return linmove->GetSector();
}

bool GEMClientActor::NeedDRUpdate(unsigned char &priority)
{
    // Never send DR messages until client is "ready"
    if(!cel->GetMainPlayer())
        return false;

    // We don't need to update our position if we are dead.
    if (serverMode == psModeMessage::DEAD)
        return false;

    csVector3 vel = linmove->GetVelocity();
    csVector3 angularVelocity = linmove->GetAngularVelocity();

    if(linmove->IsPath() && !path_sent)
    {
        priority = PRIORITY_HIGH;
        return true;
    }

    csTicks timenow = csGetTicks();
    float delta =  timenow - lastDRUpdateTime;
    priority = PRIORITY_LOW;

    if(vel.IsZero() && angularVelocity.IsZero() &&
            lastSentVelocity.IsZero() && lastSentRotation.IsZero() &&
            delta>3000 && !stationary)
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        priority = PRIORITY_HIGH;
        stationary = true;
        return true;
    }

    // has the velocity rotation changed since last DR msg sent?
    if((angularVelocity != lastSentRotation || vel.x != lastSentVelocity.x
            || vel.z != lastSentVelocity.z) &&
            ((delta>250) || angularVelocity.IsZero()
             || lastSentRotation.IsZero() || vel.IsZero()
             || lastSentVelocity.IsZero()))
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        stationary = false;
        lastDRUpdateTime = timenow;
        return true;
    }

    if((vel.y  && !lastSentVelocity.y) || (!vel.y && lastSentVelocity.y) || (vel.y > 0 && lastSentVelocity.y < 0) || (vel.y < 0 && lastSentVelocity.y > 0))
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        stationary = false;
        lastDRUpdateTime = timenow;
        return true;
    }

    // if in motion, send every second instead of every 3 secs.
    if((!lastSentVelocity.IsZero() || !lastSentRotation.IsZero()) &&
            (delta > 1000))
    {
        lastSentRotation = angularVelocity;
        lastSentVelocity = vel;
        stationary = false;
        lastDRUpdateTime = timenow;
        return true;
    }

    // Real position/rotation/action is as calculated, no need for update.
    return false;
}

void GEMClientActor::SendDRUpdate(unsigned char priority, csStringHashReversible* msgstrings)
{
    // send update out
    EID mappedid = eid;  // no mapping anymore, IDs are identical
    float yrot, ang_vel;
    bool on_ground;
    csVector3 pos, vel, worldVel;
    iSector* sector;

    linmove->GetDRData(on_ground, pos, yrot, sector, vel, worldVel, ang_vel);

    ZoneHandler* zonehandler = psengine->GetZoneHandler();
    if(zonehandler && zonehandler->IsLoading())
    {
        // disable movement to stop stamina drain while map is loading
        on_ground = true;
        vel = 0;
        ang_vel = 0;
    }

    // Hack to guarantee out of order packet detection -- KWF
    //if (DRcounter%20 == 0)
    //{
    //    DRcounter-=2;
    //    hackflag = true;
    //}

    // ++DRcounter is the sequencer of these messages so the server and other
    // clients do not use out of date messages when delivered out of order.
    psDRMessage drmsg(0, mappedid, on_ground, 0, ++DRcounter, pos, yrot, sector,
                      sector->QueryObject()->GetName(), vel, worldVel, ang_vel,
                      psengine->GetNetManager()->GetConnection()->GetAccessPointers());
    drmsg.msg->priority = priority;

    //if (hackflag)
    //    DRcounter+=2;

    psengine->GetMsgHandler()->SendMessage(drmsg.msg);
}

void GEMClientActor::SetDRData(psDRMessage &drmsg)
{
    if(drmsg.sector != NULL)
    {
        if(!DRcounter_set || drmsg.IsNewerThan(DRcounter))
        {
            float cur_yrot;
            csVector3 cur_pos;
            iSector* cur_sector;
            linmove->GetLastPosition(cur_pos,cur_yrot,cur_sector);

            if(DoLogDebug(LOG_DRDATA))
            {
                Debug(LOG_DRDATA, GetEID().Unbox(), "DRDATA: %s, %s, %s, %s, pos(%f, %f, %f), rot(%f), %s, %f, %f, %f, %f, %f, %f, %f",
                      "PSCLIENT", "OLD", ShowID(GetEID()),GetName(), cur_pos.x, cur_pos.y, cur_pos.z, cur_yrot,
                      cur_sector->QueryObject()->GetName(), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

                Debug(LOG_DRDATA, GetEID().Unbox(), "DRDATA: %s, %s, %s, %s, pos(%f, %f, %f), rot(%f), %s, %f, %f, %f, %f, %f, %f, %f",
                      "PSCLIENT", "SET", ShowID(GetEID()),drmsg.on_ground?"TRUE":"FALSE", drmsg.pos.x, drmsg.pos.y, drmsg.pos.z, drmsg.yrot,
                      drmsg.sector->QueryObject()->GetName(), drmsg.vel.x, drmsg.vel.y, drmsg.vel.z, drmsg.worldVel.x, drmsg.worldVel.y, drmsg.worldVel.z, drmsg.ang_vel);
            }

            // If we're loading and this is a sector change, set unresolved.
            // We don't want to move to a new sector which may be loading.
            if(drmsg.sector != cur_sector && psengine->GetZoneHandler()->IsLoading())
            {
                cel->HandleUnresolvedPos(this, drmsg.pos, csVector3(0,drmsg.yrot,0), drmsg.sectorName);
            }
            else
            {
                // Force hard DR update on sector change, low speed, or large delta pos
                if(drmsg.sector != cur_sector || (drmsg.vel < 0.1f) || (csSquaredDist::PointPoint(cur_pos,drmsg.pos) > 25.0f))
                {
                    // Do hard DR when it would make you slide
                    linmove->SetDRData(drmsg.on_ground,drmsg.pos,drmsg.yrot,drmsg.sector,drmsg.vel,drmsg.worldVel,drmsg.ang_vel);
                }
                else
                {
                    // Do soft DR when moving
                    linmove->SetSoftDRData(drmsg.on_ground,drmsg.pos,drmsg.yrot,drmsg.sector,drmsg.vel,drmsg.worldVel,drmsg.ang_vel);
                }

                DRcounter = drmsg.counter;
                DRcounter_set = true;
            }
        }
        else
        {
            Error4("Ignoring DR pkt version %d for entity %s with version %d.", drmsg.counter, GetName(), DRcounter);
            return;
        }
    }
    else
    {
        cel->HandleUnresolvedPos(this, drmsg.pos, csVector3(0,drmsg.yrot,0), drmsg.sectorName);
    }

    // Update character mode and idle animation
    SetCharacterMode(drmsg.mode);

    // Update the animations to match velocity
    SetAnimationVelocity(drmsg.vel);

    //update sound manager to the new position
    psengine->GetSoundManager()->UpdateObjectEntity(pcmesh, race);
}

void GEMClientActor::StopMoving(bool worldVel)
{
    // stop this actor from moving
    csVector3 zeros(0.0f, 0.0f, 0.0f);
    linmove->SetVelocity(zeros);
    linmove->SetAngularVelocity(zeros);
    if(worldVel)
        linmove->ClearWorldVelocity();
}

void GEMClientActor::SetPosition(const csVector3 &pos, float rot, iSector* sector)
{
    linmove->SetPosition(pos, rot, sector);
}

void GEMClientActor::SetVelocity(const csVector3 &vel)
{
    linmove->SetVelocity(vel);
}

void GEMClientActor::SetYRotation(const float yrot)
{
    linmove->SetYRotation(yrot);
}



void GEMClientActor::InitCharData(const char* traits, const char* equipment)
{
    this->traits = traits;
    this->equipment = equipment;

    charApp->ApplyTraits(this->traits);
    charApp->ApplyEquipment(this->equipment);

    // Update any doll views registered for changes
    csArray<iPAWSSubscriber*> dolls = PawsManager::GetSingleton().ListSubscribers("sigActorUpdate");
    for(size_t i=0; i<dolls.GetSize(); i++)
    {
        if(dolls[i] == NULL)
            continue;

        pawsObjectView* doll = dynamic_cast<pawsObjectView*>(dolls[i]);

        if(doll == NULL)
            continue;

        if(doll->GetID() == GetEID().Unbox())  // This is a doll of the updated object
        {
            iMeshWrapper* dollObject = doll->GetObject();
            if(dollObject == NULL)
            {
                Error2("Cannot update registered doll view with ID %d because it has no object", doll->GetID());
                continue;
            }
            psCharAppearance* pp = doll->GetCharApp();
            pp->ApplyTraits(this->traits);
            pp->ApplyEquipment(this->equipment);
        }
    }
}

psLinearMovement &GEMClientActor::Movement()
{
    return *linmove;
}

bool GEMClientActor::IsGroupedWith(GEMClientActor* actor)
{
    if(actor && actor->GetGroupID() == groupID && groupID != 0)
        return true;
    else
        return false;
}

bool GEMClientActor::IsOwnedBy(GEMClientActor* actor)
{
    if(actor && actor->GetOwnerEID() == GetOwnerEID() && GetOwnerEID() != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool GEMClientActor::SetAnimation(const char* anim, int duration)
{
    if(!cal3dstate)
        return true;

    int animation = cal3dstate->FindAnim(anim);
    if(animation < 0)
    {
        Error3("Didn't find animation '%s' for '%s'.",anim , GetName());
        return false;
    }

    if(!cal3dstate->GetCal3DModel()->getCoreModel()->getCoreAnimation(animation))
    {
        Error3("Could not get core animation '%s' for '%s'.",anim , GetName());
        return false;
    }

    float ani_duration = cal3dstate->GetCal3DModel()->getCoreModel()->getCoreAnimation(animation)->getDuration();

    // Check if the duration demands more than 1 playback?
    if(duration > ani_duration)
    {
        // Yes. Set up callback to handle repetition
        int repeat = (int)(duration / ani_duration);

        csRef<iSpriteCal3DFactoryState> sprite =
            scfQueryInterface<iSpriteCal3DFactoryState> (pcmesh->GetFactory()->GetMeshObjectFactory());


        vmAnimCallback* callback = new vmAnimCallback;

        // Stuff callback's face with what he needs.
        callback->callbackcount = repeat;
        callback->callbacksprite = sprite;
        callback->callbackspstate = cal3dstate;
        callback->callbackanimation = anim;
        if(!sprite->RegisterAnimCallback(anim,callback,99999999999.9F))  // Make time huge as we do want only the end here
        {
            Error2("Failed to register callback for animation %s", anim);
            delete callback;
        }

    }

    float fadein = 0.25;
    float fadeout = 0.25;

    // TODO: Get these numbers from a file somewhere
    if(strcmp(anim,"sit")==0 || strcmp(anim,"stand up")==0)
    {
        fadein = 0.0;
        fadeout = 1.5;
    }

    //removes the idle_var animation if it's in execution to avoid interferences.
    cal3dstate->GetCal3DModel()->getMixer()->removeAction(cal3dstate->FindAnim("idle_var"));

    return cal3dstate->SetAnimAction(anim,fadein,fadeout);
}

// TO CHECK : why this function is called also if the character is standing still
void GEMClientActor::SetAnimationVelocity(const csVector3 &velocity)
{
    if(!alive)   // No zombies please
        return;

    if(!cal3dstate && !speedNode.IsValid())
        return;

    // Taking larger of the 2 axis; cal3d axis are the opposite of CEL's
    bool useZ = ABS(velocity.z) > ABS(velocity.x);
    float cal3dvel = useZ ? velocity.z : velocity.x;
    if(cal3dvel == 0.0)
    {
        cal3dvel = velocity.y;
    }
    if(cal3dstate)
    {
        cal3dstate->SetVelocity(-cal3dvel, &psengine->GetRandomGen());
    }
    else
    {
        speedNode->SetSpeed(-cal3dvel);
    }
    if((velocity.x != 0 || velocity.z != 0) && velocity.Norm() < 1.3)
    {

        charApp->SetSneak(true);
    }
    else
    {
        charApp->SetSneak(false);
    }
}

void GEMClientActor::SetMode(uint8_t mode, bool newactor)
{
    // TODO: Hacky fix for now when model has not finished loaded.
    if(!cal3dstate) return;

    if((serverMode == psModeMessage::OVERWEIGHT ||
        serverMode == psModeMessage::DEFEATED) &&
       mode != psModeMessage::OVERWEIGHT &&
       mode != psModeMessage::DEFEATED)
    {
        cal3dstate->SetAnimAction("stand up", 0.0f, 1.0f);
    }

    SetAlive(mode != psModeMessage::DEAD, newactor);

    switch(mode)
    {
        case psModeMessage::PEACE:
        case psModeMessage::WORK:
        case psModeMessage::EXHAUSTED:
            SetIdleAnimation(psengine->GetCharControl()->GetMovementManager()->GetModeIdleAnim(movementMode));
            break;
        case psModeMessage::SPELL_CASTING:
        case psModeMessage::PLAY:
        case psModeMessage::WALK:
        case psModeMessage::RUN:
            break;

        case psModeMessage::COMBAT:
            // TODO: Get stance and set anim for that stance
            SetIdleAnimation("combat stand");
            break;

        case psModeMessage::DEAD:
            cal3dstate->ClearAllAnims();
            cal3dstate->SetAnimAction("death",0.0f,1.0f);
            linmove->SetVelocity(0);
            linmove->SetAngularVelocity(0);
            if(newactor)  // If we're creating a new actor that's already dead, we shouldn't show the animation...
                cal3dstate->SetAnimationTime(9999);  // the very end of the death animation ;)
            break;

        case psModeMessage::SIT:
        case psModeMessage::OVERWEIGHT:
        case psModeMessage::DEFEATED:
            if(!newactor && serverMode != mode)  //we do this only if this isn't a new actor
                cal3dstate->SetAnimAction("sit", 0.0f, 1.0f);
            SetIdleAnimation("sit idle");
            break;

        case psModeMessage::STATUE: //used to make statue like character which don't move
            cal3dstate->ClearAllAnims();
            cal3dstate->SetAnimAction("statue",0.0f,1.0f);
            linmove->SetVelocity(0);
            linmove->SetAngularVelocity(0);
            cal3dstate->SetAnimationTime(9999);  // the very end of the animation
            break;

        default:
            Error2("Unhandled mode: %d", mode);
            return;
    }
    serverMode = mode;
}

void GEMClientActor::SetCharacterMode(size_t newmode)
{
    //Debug5(LOG_CELPERSIST, 0, "MovementMode: %d New mode: %d for %s eid: %d", movementMode, newmode, GetName(), this->GetEID());

    if(newmode == movementMode)
        return;

    Debug4(LOG_CELPERSIST, 0, "Setting New mode: %zu for %s eid: %d", newmode, GetName(), this->GetEID().Unbox());

    movementMode = newmode;

    if(serverMode == psModeMessage::PEACE)
        SetIdleAnimation(psengine->GetCharControl()->GetMovementManager()->GetModeIdleAnim(movementMode));

    psengine->GetSoundManager()->SetEntityState(newmode, pcmesh, race, false);
}

void GEMClientActor::SetAlive(bool newvalue, bool newactor)
{
    if(alive == newvalue)
        return;

    alive = newvalue;

    if(!newactor)
        psengine->GetCelClient()->GetEntityLabels()->RepaintObjectLabel(this);

    if(!alive)
        psengine->GetCelClient()->GetClientDR()->HandleDeath(this);
}

void GEMClientActor::SetIdleAnimation(const char* anim)
{
    if(!cal3dstate && !speedNode.IsValid()) return;
    if(cal3dstate)
        cal3dstate->SetDefaultIdleAnim(anim);
    if(lastSentVelocity.IsZero())
    {
        if(cal3dstate)
            cal3dstate->SetVelocity(0);
        else
            speedNode->SetSpeed(0);
    }
}

void GEMClientActor::RefreshCal3d()
{
    cal3dstate =  scfQueryInterface<iSpriteCal3DState > (pcmesh->GetMeshObject());
    animeshObject = scfQueryInterface<CS::Mesh::iAnimatedMesh>(pcmesh->GetMeshObject());
    if(animeshObject)
    {
        CS::Animation::iSkeletonAnimNode* rootNode = animeshObject->GetSkeleton()->GetAnimationPacket()->GetAnimationRoot();
        speedNode = scfQueryInterface<CS::Animation::iSkeletonSpeedNode> (rootNode->FindNode("speed"));
    }
    if(cal3dstate)
    {
        cal3dstate->SetCyclicBlendFactor(0.1f);
    }
    CS_ASSERT(cal3dstate || animeshObject);
}

void GEMClientActor::SetChatBubbleID(unsigned int chatBubbleID)
{
    this->chatBubbleID = chatBubbleID;
}

unsigned int GEMClientActor::GetChatBubbleID() const
{
    return chatBubbleID;
}

const char* GEMClientActor::GetName(bool trueName)
{
    static const char* strUnknown = "[Unknown]";
    if(trueName || (Flags() & psPersistActor::NAMEKNOWN) || (eid == psengine->GetCelClient()->GetMainPlayer()->GetEID()))
        return name;
    return strUnknown;
}

bool GEMClientActor::CheckLoadStatus()
{
    if(psengine->GetZoneHandler()->IsLoading() || !post_load->sector)
    {
        post_load->sector = psengine->GetEngine()->GetSectors()->FindByName(post_load->sector_name);
        return false;
    }

    if(!factory.IsValid())
    {
        factory = psengine->GetLoader()->LoadFactory(factName);

        if(!factory.IsValid())
        {
            Error2("Failed to find factory: '%s'", factName.GetData());
            psengine->UnregisterDelayedLoader(this);
            return false;
        }
    }

    if(!factory->IsFinished())
    {
        return false;
    }
    else if(!factory->WasSuccessful())
    {
        Error2("Failed to load factory: '%s'", factName.GetData());
        psengine->UnregisterDelayedLoader(this);
        return false;
    }

    if(!mountFactname.IsEmpty() && !mountFactname.Compare("null"))
    {
        if(!mountFactory.IsValid())
        {
            mountFactory = psengine->GetLoader()->LoadFactory(mountFactname);

            if(!mountFactory.IsValid())
            {
                Error2("Couldn't find the mesh factory: '%s' for mount.", mountFactname.GetData());
                psengine->UnregisterDelayedLoader(this);
                return false;
            }
        }

        if(!mountFactory->IsFinished())
        {
            return false;
        }
        else if(!mountFactory->WasSuccessful())
        {
            Error2("Couldn't load the mesh factory: '%s' for mount.", mountFactname.GetData());
            psengine->UnregisterDelayedLoader(this);
            return false;
        }
    }

    csRef<iMeshFactoryWrapper> meshFact = scfQueryInterface<iMeshFactoryWrapper>(factory->GetResultRefPtr());
    csRef<iMeshWrapper> mesh = meshFact->CreateMeshWrapper();
    charApp->SetMesh(mesh);

    if(!matName.IsEmpty())
    {
        charApp->ChangeMaterial(partName, matName);
    }

    if(!mountFactname.IsEmpty() && !mountFactname.Compare("null"))
    {
        rider = mesh;
        csRef<iMeshFactoryWrapper> mountMeshFact = scfQueryInterface<iMeshFactoryWrapper>(mountFactory->GetResultRefPtr());
        csRef<iMeshWrapper> mountMesh = mountMeshFact->CreateMeshWrapper();
        SwitchToRealMesh(mountMesh);
        charApp->ApplyRider(pcmesh);
        csRef<iSpriteCal3DState> riderstate = scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
        riderstate->SetAnimCycle(MounterAnim,100);
    }
    else
    {
        SwitchToRealMesh(mesh);

        // If the mesh has a scale apply it.
        if(scale >= 0)
        {
            csRef<iMovable> movable = mesh->GetMovable();
            csReversibleTransform &trans = movable->GetTransform();
            csRef<iSpriteCal3DFactoryState> sprite = scfQueryInterface<iSpriteCal3DFactoryState> (mesh->GetFactory()->GetMeshObjectFactory());

            // Normalize the mesh scale to the base scale of the mesh.
            Debug4(LOG_CELPERSIST,0,"DEBUG: Normalize scale: %f / %f = %f\n",scale, sprite->GetScaleFactor(), scale / sprite->GetScaleFactor());
            baseScale = sprite->GetScaleFactor();  
            scale = scale / baseScale;
            linmove->SetScale(scale);

            // Apply the resize transformation.
            trans = csReversibleTransform(1.0f / scale * csMatrix3(),csVector3(0)) * trans;
            movable->UpdateMove();
        }
    }

    pcmesh->GetFlags().Set(CS_ENTITY_NODECAL);

    csRef<iSpriteCal3DState> calstate = scfQueryInterface<iSpriteCal3DState> (pcmesh->GetMeshObject());
    if(calstate)
        calstate->SetUserData((void*)this);

    cel->AttachObject(pcmesh->QueryObject(), this);

    // If this is a mounted object add also what is mounting it so it can be clicked.
    if(rider.IsValid())
    {
        cel->AttachObject(rider->QueryObject(), this);
    }

    psengine->UnregisterDelayedLoader(this);

    psengine->GetSoundManager()->AddObjectEntity(pcmesh, race);

    return true;
}

GEMClientItem::GEMClientItem(psCelClient* cel, psPersistItem &mesg)
    : GEMClientObject(cel, mesg.eid), post_load(new PostLoadData())
{
    name = mesg.name;
    Debug3(LOG_CELPERSIST, 0, "Item %s(%s) Received", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;
    factName = mesg.factname;
    matName = mesg.matname;
    solid = 0;
    post_load->pos = mesg.pos;
    post_load->xRot = mesg.xRot;
    post_load->yRot = mesg.yRot;
    post_load->zRot = mesg.zRot;
    post_load->sector = mesg.sector;
    flags = mesg.flags;

    shman = csQueryRegistry<iShaderManager>(psengine->GetObjectRegistry());
    strings = csQueryRegistryTagInterface<iStringSet>(
                  psengine->GetObjectRegistry(), "crystalspace.shared.stringset");

    LoadMesh();
}

GEMClientItem::~GEMClientItem()
{
    delete solid;
    delete post_load;

    if(pcmesh.IsValid())
    {
        psengine->GetEngine()->RemoveObject(pcmesh);
        pcmesh.Invalidate();
    }

    if(instance.IsValid())
    {
        // Remove instance.
        instance->pcmesh->RemoveInstance(position);
        if(instance->GetRefCount() == 2)
        {
            instance.Invalidate();
            cel->RemoveInstanceObject(factName+matName);
        }
    }
}

csPtr<iMaterialWrapper> GEMClientItem::CloneMaterial(iMaterialWrapper* mw)
{
    if(!mw)
    {
        return csPtr<iMaterialWrapper>(0);
    }

    iMaterial* material = mw->GetMaterial();

    if(!material)
    {
        return csPtr<iMaterialWrapper>(0);
    }

    // Get the base shader.
    csStringID baseType = strings->Request("base");
    iShader* shader = material->GetShader(baseType);

    // Get the depthtest shader.
    csStringID depthTestType = strings->Request("oc_depthtest");
    iShader* shadert = material->GetShader(depthTestType);

    // Get the depth shader. TODO: change to oc_depthwrite
    csStringID depthType = strings->Request("depthwrite");
    iShader* shaderz = material->GetShader(depthType);

    // Get the diffuse shader type.
    csStringID diffuseType = strings->Request("diffuse");

    // Find an instance shader matching this type.
    iBgLoader* loader = psengine->GetLoader();
    csRef<iStringArray> shaders = loader->GetShaderName("default");
    csRef<iStringArray> shadersa = loader->GetShaderName("default_alpha");
    if(!shader || shaders->Contains(shader->QueryObject()->GetName()) != csArrayItemNotFound)
    {
        csRef<iStringArray> shaderName = loader->GetShaderName("instance");
        shader = shman->GetShader(shaderName->Get(0));

        // set instanced depthwrite shader for non-alpha objects.
        shaderName = loader->GetShaderName("instance_early_z");
        shaderz = shman->GetShader(shaderName->Get(0));
    }
    else if(shadersa->Contains(shader->QueryObject()->GetName()) != csArrayItemNotFound)
    {
        csRef<iStringArray> shaderName = loader->GetShaderName("instance_alpha");
        shader = shman->GetShader(shaderName->Get(0));

        // alpha objects don't use the depthwrite pass for now.
        shaderz = shman->GetShader("*null");
        // instanced alpha objects need to use z_only_instanced for depthtest
        shadert = shman->GetShader("z_only_instanced");
    }
    else
    {
        Error3("Unhandled shader %s for mesh %s!", shader->QueryObject()->GetName(), factName.GetData());
    }

    // Construct a new material using the selected shaders.
    csRef<iTextureWrapper> tex = psengine->GetEngine()->GetTextureList()->CreateTexture(material->GetTexture());
    csRef<iMaterial> mat = psengine->GetEngine()->CreateBaseMaterial(tex);

    // Set the base shader on this material.
    mat->SetShader(baseType, shader);

    // Set the diffuse shader on this material.
    mat->SetShader(diffuseType, shader);

    // Set the depthtest shader on this material
    mat->SetShader(depthTestType, shadert);

    // Set the early_z shader on this material.
    mat->SetShader(depthType, shaderz);

    // Copy all shadervars over.
    for(size_t j=0; j<material->GetShaderVariables().GetSize(); ++j)
    {
        mat->AddVariable(material->GetShaderVariables().Get(j));
    }

    // Create a new wrapper for this material.
    csRef<iMaterialWrapper> matwrap = psengine->GetEngine()->GetMaterialList()->CreateMaterial(mat, factName + "_instancemat");
    return csPtr<iMaterialWrapper>(matwrap);
}

bool GEMClientItem::CheckLoadStatus()
{
    // Check if an instance of the mesh already exists.
    instance = cel->FindInstanceObject(factName+matName);
    if(!instance.IsValid())
    {
        if(!factory.IsValid())
        {
            factory = psengine->GetLoader()->LoadFactory(factName);

            if(!factory.IsValid())
            {
                Error3("Unable to find factory %s for item %s!", factName.GetData(), name.GetData());
                psengine->UnregisterDelayedLoader(this);
                return false;
            }
        }

        if(!factory->IsFinished())
        {
            return false;
        }
        else if(!factory->WasSuccessful())
        {
            Error3("Unable to load item %s with factory %s!", name.GetData(), factName.GetData());
            psengine->UnregisterDelayedLoader(this);
            return false;
        }

        if(!matName.IsEmpty())
        {
            if(!material.IsValid())
            {
                material = psengine->GetLoader()->LoadMaterial(matName);

                if(!material.IsValid())
                {
                    Error4("Unable to find material %s for item %s (factory %s)!",
                           matName.GetData(), name.GetData(), factName.GetData());
                    psengine->UnregisterDelayedLoader(this);
                    return false;
                }
            }

            if(!material->IsFinished())
            {
                return false;
            }
            else if(!factory->WasSuccessful())
            {
                Error4("Unable to load item %s with material %s (factory %s)!",
                       name.GetData(), matName.GetData(), factName.GetData());
                psengine->UnregisterDelayedLoader(this);
                return false;
            }
        }

        // obtain the load results
        csRef<iMeshFactoryWrapper> meshFact = scfQueryInterface<iMeshFactoryWrapper>(factory->GetResultRefPtr());
        csRef<iMaterialWrapper> matWrap;
        if(material.IsValid())
        {
            matWrap = scfQueryInterface<iMaterialWrapper>(material->GetResultRefPtr());
        }

        // create the mesh
        if(cel->InstanceItems())
        {
            csRef<iMeshFactoryWrapper> meshFact = scfQueryInterface<iMeshFactoryWrapper>(factory->GetResultRefPtr());

            instance.AttachNew(new InstanceObject());
            instance->pcmesh = meshFact->CreateMeshWrapper();
            instance->pcmesh->GetFlags().Set(CS_ENTITY_NODECAL | CS_ENTITY_NOHITBEAM);
            instance->meshFact = factory;
            instance->material = material;
            psengine->GetEngine()->PrecacheMesh(instance->pcmesh);
            cel->AddInstanceObject(factName+matName, instance);

            if(matWrap.IsValid())
            {
                matWrap = CloneMaterial(matWrap);
                CS_ASSERT(matWrap.IsValid());
                instance->pcmesh->GetMeshObject()->SetMaterialWrapper(matWrap);
            }
        }
        else
        {
            pcmesh = meshFact->CreateMeshWrapper();
            pcmesh->GetFlags().Set(CS_ENTITY_NODECAL);
            psengine->GetEngine()->PrecacheMesh(pcmesh);

            if(matWrap.IsValid())
            {
                pcmesh->GetMeshObject()->SetMaterialWrapper(matWrap);
            }
        }

        // set appropriate shaders for instanced objects
        if(cel->InstanceItems())
        {
            // Set appropriate shader.
            csRef<iGeneralFactoryState> gFact = scfQueryInterface<iGeneralFactoryState>(meshFact->GetMeshObjectFactory());
            csRef<iGeneralMeshState> gState = scfQueryInterface<iGeneralMeshState>(instance->pcmesh->GetMeshObject());

            for(size_t i=0; (!gFact.IsValid() && i == 0) || (gFact.IsValid() && i<gFact->GetSubMeshCount()); ++i)
            {
                // Check if this is a genmesh
                iGeneralMeshSubMesh* gSubmesh = NULL;
                if(gFact.IsValid())
                {
                    gSubmesh = gFact->GetSubMesh(i);
                    gSubmesh = gState->FindSubMesh(gSubmesh->GetName());
                }

                // Get the current material for this submesh.
                iMaterialWrapper* material;
                if(gSubmesh != NULL)
                {
                    material = gSubmesh->GetMaterial();
                }
                else if(instance->pcmesh->GetMeshObject()->GetMaterialWrapper() != NULL)
                {
                    material = instance->pcmesh->GetMeshObject()->GetMaterialWrapper();
                }
                else
                {
                    material = meshFact->GetMeshObjectFactory()->GetMaterialWrapper();
                }

                csRef<iMaterialWrapper> clonedMatWrap = CloneMaterial(material);
                if(gSubmesh)
                {
                    gSubmesh->SetMaterial(clonedMatWrap);
                }
                else
                {
                    instance->pcmesh->GetMeshObject()->SetMaterialWrapper(clonedMatWrap);
                }
            }

            // create nullmesh factory
            instance->nullFactory = psengine->GetEngine()->CreateMeshFactory("crystalspace.mesh.object.null", factName + "_nullmesh", false);
            csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (instance->nullFactory->GetMeshObjectFactory());
            nullstate->SetBoundingBox(instance->bbox);
            nullstate->SetCollisionMeshData(meshFact->GetMeshObjectFactory()->GetObjectModel());

            // Set biggest bbox so that instances aren't wrongly culled.
            instance->bbox = meshFact->GetMeshObjectFactory()->GetObjectModel()->GetObjectBoundingBox();
            meshFact->GetMeshObjectFactory()->GetObjectModel()->SetObjectBoundingBox(csBox3(-CS_BOUNDINGBOX_MAXVALUE,
                    -CS_BOUNDINGBOX_MAXVALUE, -CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE,
                    CS_BOUNDINGBOX_MAXVALUE));
        }
    }

    if(cel->InstanceItems())
    {
        csVector3 Pos = csVector3(0.0f);
        csMatrix3 Rot = csMatrix3();
        position = instance->pcmesh->AddInstance(Pos, Rot);

        // Init nullmesh factory.
        pcmesh = instance->nullFactory->CreateMeshWrapper();
        pcmesh->GetFlags().Set(CS_ENTITY_NODECAL);
        csRef<iNullMeshState> nullmeshstate = scfQueryInterface<iNullMeshState> (pcmesh->GetMeshObject());
        nullmeshstate->SetHitBeamMeshObject(instance->pcmesh->GetMeshObject());
    }

    cel->AttachObject(pcmesh->QueryObject(), this);

    psengine->UnregisterDelayedLoader(this);

    PostLoad();

    // Handle item effect if there is one.
    cel->HandleItemEffect(factName, pcmesh);

    return true;
}

void GEMClientItem::PostLoad()
{
    csVector3 rot(post_load->xRot, post_load->yRot, post_load->zRot);
    Move(post_load->pos, rot, post_load->sector);

    if(flags & psPersistItem::COLLIDE)
    {
        solid = new psSolid(psengine->GetObjectRegistry());
        solid->SetMesh(pcmesh);
        solid->Setup();
    }

    cel->GetEntityLabels()->OnObjectArrived(this);
    hasLabel = true;
    cel->GetShadowManager()->CreateShadow(this);
    hasShadow = true;

    delete post_load;
    post_load = NULL;
}

GEMClientActionLocation::GEMClientActionLocation(psCelClient* cel, psPersistActionLocation &mesg)
    : GEMClientObject(cel, mesg.eid)
{
    name = mesg.name;

    Debug3(LOG_CELPERSIST, 0, "Action location %s(%s) Received", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;
    meshname = mesg.mesh;

    csRef< iEngine > engine = psengine->GetEngine();
    pcmesh = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "ActionLocation");
    if(!pcmesh)
    {
        Error1("Could not create GEMClientActionLocation because crystalspace.mesh.onbject.null could not be created.");
        return ;
    }

    csRef<iNullMeshState> state =  scfQueryInterface<iNullMeshState> (pcmesh->GetMeshObject());
    if(!state)
    {
        Error1("No NullMeshState.");
        return ;
    }
    state->SetRadius(1.0);

    Move(csVector3(0,0,0), csVector3(0,0,0), mesg.sector);
}

InstanceObject::~InstanceObject()
{
    if(pcmesh.IsValid())
    {
        csRef<iMeshFactoryWrapper> fact = pcmesh->GetFactory();

        if(fact)
        {
            // reset factory bbox so it won't be wrong on next instance creation
            fact->GetMeshObjectFactory()->GetObjectModel()->SetObjectBoundingBox(bbox);
        }

        psengine->GetEngine()->RemoveObject(pcmesh);
    }
    psengine->GetEngine()->RemoveObject(nullFactory);

    // explicitly free ressources so they're freed before the
    // automatic frees of the loader-allocated object in order
    // to let leak detection work properly
    pcmesh.Invalidate();
    nullFactory.Invalidate();
}

