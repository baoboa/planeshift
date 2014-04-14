/*
 * psclientchar.cpp
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
 * The client side version of char manager that talks with the server side
 * version. Used for things like inventory query's and such.
 *
 * Client version of character manager that talks to server version about
 * the details involved about a player.
 */
#include <psconfig.h>
#include "psclientchar.h"
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/databuff.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <csutil/sysfunc.h>
#include <iutil/objreg.h>
#include <iutil/vfs.h>
#include <imesh/object.h>
#include <imesh/partsys.h>
#include <imesh/emit.h>
#include <iutil/plugin.h>
#include <csutil/xmltiny.h>
#include <iutil/cfgmgr.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/message.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/charmessages.h"

#include "engine/linmove.h"

#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsobjectview.h"

#include "gui/inventorywindow.h"
#include "gui/pawssummary.h"
#include "gui/pawsinfowindow.h"

#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"

#include "isoundmngr.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscharcontrol.h"
#include "pscelclient.h"
#include "charapp.h"
#include "pscamera.h"
#include "globals.h"

//------------------------------------------------------------------------------

void Trait::Load( iDocumentNode* node )
{
    csString genderStr;
    name                 = node->GetAttributeValue( "name" );
    int cstr_id_mesh     = node->GetAttributeValueAsInt( "mesh" );
    //int cstr_id_texture  = node->GetAttributeValueAsInt( "tex" );
    int cstr_id_material = node->GetAttributeValueAsInt( "mat" );
    genderStr            = node->GetAttributeValue( "gender" );
    location             = ConvertTraitLocationString(node->GetAttributeValue( "loc" ));
    csString shaderColours = node->GetAttributeValue("shader");
    if ( shaderColours.Length() > 0 )
    {
        sscanf( shaderColours.GetData(), "%f,%f,%f", &shader.x, &shader.y, &shader.z );
    }
    else
    {
        shader = csVector3(0.0f);
    }

    material = psengine->FindCommonString(cstr_id_material);
    mesh = psengine->FindCommonString(cstr_id_mesh);

    if (genderStr.IsEmpty())
        genderStr = "N";
    if (genderStr != "F" && genderStr != "M" && genderStr != "N")
    {
       Error2("%s isn't a legal gender. Use (F,M,N)",genderStr.GetData());
       genderStr = "N";
    }

    if (genderStr == "F")
    {
        gender = PSCHARACTER_GENDER_FEMALE;
    }
    else if (genderStr == "M")
    {
        gender = PSCHARACTER_GENDER_MALE;
    }
    else
    {
        gender = PSCHARACTER_GENDER_NONE;
    }

    csRef<iDocumentAttribute> idNode = node->GetAttribute("id");
    if ( idNode.IsValid())
        uid = idNode->GetValueAsInt();
    else
        uid = 0;

    csRef<iDocumentAttribute> nextNode = node->GetAttribute("next");
    if ( nextNode.IsValid())
        next_trait_uid = nextNode->GetValueAsInt();
    else
        next_trait_uid = 0;

    csRef<iDocumentAttribute> raceNode = node->GetAttribute("race");
    if ( raceNode.IsValid())
        raceID = raceNode->GetValueAsInt();
    else
        raceID = 0;

    next_trait = NULL;
    prev_trait = NULL;
}

//------------------------------------------------------------------------------

PSTRAIT_LOCATION Trait::ConvertTraitLocationString(const char *locationstring)
{

    if (locationstring==NULL)
        return PSTRAIT_LOCATION_NONE;

    if (!strcasecmp(locationstring,"HAIR_STYLE"))
        return PSTRAIT_LOCATION_HAIR_STYLE;

    if (!strcasecmp(locationstring,"HAIR_COLOR"))
        return PSTRAIT_LOCATION_HAIR_COLOR;

    if (!strcasecmp(locationstring,"BEARD_STYLE"))
        return PSTRAIT_LOCATION_BEARD_STYLE;

    if (!strcasecmp(locationstring,"FACE"))
        return PSTRAIT_LOCATION_FACE;

    if (!strcasecmp(locationstring,"SKIN_TONE"))
        return PSTRAIT_LOCATION_SKIN_TONE;

    if (!strcasecmp(locationstring,"ITEM"))
        return PSTRAIT_LOCATION_ITEM;

    if (!strcasecmp(locationstring,"EYE_COLOR"))
        return PSTRAIT_LOCATION_EYE_COLOR;

    return PSTRAIT_LOCATION_NONE;
}
//------------------------------------------------------------------------------

psClientCharManager::psClientCharManager(iObjectRegistry *objectreg)
{
    ready = true;

    objectReg = objectreg;
    vfs =  csQueryRegistry<iVFS> (objectReg);

    charCreation = 0;

    target = 0;
    targetEffect = 0;
    lockedTarget = false;
}

psClientCharManager::~psClientCharManager()
{
    if (msghandler)
    {
        msghandler->Unsubscribe(this, MSGTYPE_CHARREJECT);
        msghandler->Unsubscribe(this, MSGTYPE_EQUIPMENT);
        msghandler->Unsubscribe(this, MSGTYPE_EFFECT);
        msghandler->Unsubscribe(this, MSGTYPE_EFFECT_STOP);
        msghandler->Unsubscribe(this, MSGTYPE_PLAYSOUND);
        msghandler->Unsubscribe(this, MSGTYPE_USERACTION);
        msghandler->Unsubscribe(this, MSGTYPE_GUITARGETUPDATE);
    }
    delete charCreation;
}

bool psClientCharManager::Initialize( MsgHandler* msgHandler,
                                      psCelClient* GEMSupervisor)
{
    msghandler = msgHandler;
    cel = GEMSupervisor;

    msghandler->Subscribe(this, MSGTYPE_CHARREJECT);
    msghandler->Subscribe(this, MSGTYPE_EQUIPMENT);
    msghandler->Subscribe(this, MSGTYPE_EFFECT);
    msghandler->Subscribe(this, MSGTYPE_EFFECT_STOP);
    msghandler->Subscribe(this, MSGTYPE_PLAYSOUND);
    msghandler->Subscribe(this, MSGTYPE_USERACTION);
    msghandler->Subscribe(this, MSGTYPE_GUITARGETUPDATE);
    msghandler->Subscribe(this, MSGTYPE_CHANGE_TRAIT);
    charCreation = new psCreationManager( objectReg );

    return true;
}

void psClientCharManager::HandleMessage ( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_CHANGE_TRAIT:
        {
            ChangeTrait(me);
            break;
        }

        case MSGTYPE_USERACTION:
        {
            HandleAction( me );
            break;
        }
        case MSGTYPE_CHARREJECT:
        {
            HandleRejectCharMessage( me );
            break;
        }

        case MSGTYPE_EQUIPMENT:
        {
            HandleEquipment(me);
            return;
        }

        case MSGTYPE_EFFECT:
        {
            HandleEffect(me);
            return;
        }

        case MSGTYPE_EFFECT_STOP:
        {
            HandleEffectStop(me);
            return;
        }

        case MSGTYPE_PLAYSOUND:
        {
            if(psengine->GetSoundStatus())
            {
                HandlePlaySound(me);
            }
            return;
        }

        case MSGTYPE_GUITARGETUPDATE:
        {
            HandleTargetUpdate(me);
            return;
        }

        default:
        {
            ready = true;
            return;
        }
    }
}

void psClientCharManager::ChangeTrait( MsgEntry* me )
{
    psTraitChangeMessage mesg(me);
    EID objectID = mesg.target;

    GEMClientActor *actor = dynamic_cast<GEMClientActor*>(psengine->GetCelClient()->FindObject(objectID));
    if (!actor)
    {
        Error2("Got trait change for %s, but couldn't find it!", ShowID(objectID));
        return;
    }

    // Update main object
    //psengine->BuildAppearance( object->pcmesh->GetMesh(), mesg.string );
    actor->CharAppearance()->ApplyTraits(mesg.string);

    // Update any doll views registered for changes
    csArray<iPAWSSubscriber*> dolls = PawsManager::GetSingleton().ListSubscribers("sigActorUpdate");
    for (size_t i=0; i<dolls.GetSize(); i++)
    {
        if (dolls[i] == NULL)
            continue;
        pawsObjectView* doll = dynamic_cast<pawsObjectView*>(dolls[i]);

        if (doll == NULL)
            continue;
        if (doll->GetID() == objectID.Unbox()) // This is a doll of the updated object
        {
            iMeshWrapper* dollObject = doll->GetObject();
            if (dollObject == NULL)
            {
                Error2("Cannot update registered doll view with ID %d because it has no object", doll->GetID());
                continue;
            }
            psCharAppearance* p = doll->GetCharApp();
            p->Clone(actor->CharAppearance());
            p->SetMesh(dollObject);
            p->ApplyTraits(mesg.string);
        }
    }
}


void psClientCharManager::LockTarget(bool state)
{
    lockedTarget = state;
}

void psClientCharManager::SetTarget(GEMClientObject *newTarget, const char *action, bool notifyServer)
{

if(!lockedTarget)
{
    if (target != newTarget)
    {
        target = newTarget;

        GEMClientActor* gemActor = dynamic_cast<GEMClientActor*>(newTarget);
        if (gemActor)
            PawsManager::GetSingleton().Publish("sTargetName",gemActor->GetName(false) );
        else
        {
            PawsManager::GetSingleton().Publish("sTargetName",target?target->GetName():"" );
            PawsManager::GetSingleton().Publish("fVitalValue0:Target",0);
        }
    }

    // delete the old target effect
    psengine->GetEffectManager()->DeleteEffect(targetEffect);
    targetEffect = 0;

    EID mappedID;

    // Action locations don't have effects
    if (target && target->GetObjectType() != GEM_ACTION_LOC)
    {
         // render the target effect
         csRef<iMeshWrapper> targetMesh = target->GetMesh();
         if (targetMesh)
             targetEffect = psengine->GetEffectManager()->RenderEffect("target", csVector3(0,0,0), targetMesh);

         // notify the server of selection
         mappedID = target->GetEID();
    }

    // if it's a message sent by server, there is no need to resend back the same information
    if (notifyServer)
    {
        psUserActionMessage userAction(0, mappedID, action);
        userAction.SendMessage();
    }
}
}

void psClientCharManager::HandleAction( MsgEntry* me )
{
    psUserActionMessage action(me);
    GEMClientActor* actor = dynamic_cast<GEMClientActor*>(cel->FindObject(action.target));
    if (actor)
    {
        if ( !actor->SetAnimation(action.action) )
            Error2("Error playing animation %s!", action.action.GetData() );
    }
    else
        Error1("Received psUserActionMessage for invalid object.");
}

void psClientCharManager::HandlePlaySound( MsgEntry* me )
{
    psengine->GetSoundManager()->PlaySound(me->GetStr(), false, iSoundManager::ACTION_SNDCTRL);
}

void psClientCharManager::HandleTargetUpdate( MsgEntry* me )
{
    psGUITargetUpdateMessage targetMsg(me);
    if(targetMsg.targetID.IsValid()) //we have an eid
    {
        GEMClientObject* object = cel->FindObject( targetMsg.targetID );
        if ( object )
            SetTarget( object, "select", false);
        else
            Error1("Received TagetUpdateMessage with invalid target.");
    }
    else //we have an invalid eid (0) so we untarget
    {
         SetTarget(NULL, "select", false);
    }
}

void psClientCharManager::HandleEquipment(MsgEntry* me)
{
    psEquipmentMessage equip(me);
    unsigned int playerID = equip.player;

    GEMClientActor* object = (GEMClientActor*)cel->FindObject(playerID);
    if (!object)
    {
        Error2("Got equipment for actor %d, but couldn't find it!", playerID);
        return;
    }

    Debug2(LOG_CELPERSIST,0,"Got equipment for actor %d", playerID);


    csString slotname(psengine->slotName.GetName(equip.slot));

    //If the mesh has a $ it means it's an helm ($H) / bracher ($B) / belt ($E) / cloak ($C) so search for replacement
    equip.mesh.ReplaceAll("$H",object->helmGroup);
    equip.mesh.ReplaceAll("$B",object->BracerGroup);
    equip.mesh.ReplaceAll("$E",object->BeltGroup);
    equip.mesh.ReplaceAll("$C",object->CloakGroup);

    // Update any doll views registered for changes
    csArray<iPAWSSubscriber*> dolls = PawsManager::GetSingleton().ListSubscribers("sigActorUpdate");
    for (size_t i=0; i<dolls.GetSize(); i++)
    {
        if (dolls[i] == NULL)
            continue;
        pawsObjectView* doll = dynamic_cast<pawsObjectView*>(dolls[i]);

        if (doll == NULL)
            continue;
        if (doll->GetID() == playerID) // This is a doll of the updated object
        {
            iMeshWrapper* dollObject = doll->GetObject();
            if (dollObject == NULL)
            {
                Error2("Cannot update registered doll view with ID %d because it has no object", doll->GetID());
                continue;
            }
            psCharAppearance* p = doll->GetCharApp();
            p->Clone(object->CharAppearance());
            p->SetMesh(dollObject);
            if (equip.type == psEquipmentMessage::EQUIP)
            {
                p->Equip(slotname,equip.mesh,equip.part,equip.partMesh,equip.texture,equip.removedMesh);
            }
            else
            {
                p->Dequip(slotname,equip.mesh,equip.part,equip.partMesh,equip.texture,equip.removedMesh);
            }
        }
    }

    // Update the actor
    if (equip.type == psEquipmentMessage::EQUIP)
    {
        object->CharAppearance()->Equip(slotname,equip.mesh,equip.part,equip.partMesh,equip.texture,equip.removedMesh);
    }
    else
    {
        object->CharAppearance()->Dequip(slotname,equip.mesh,equip.part,equip.partMesh,equip.texture,equip.removedMesh);
    }
}

void psClientCharManager::HandleEffectStop(MsgEntry* me)
{
    psStopEffectMessage effect(me);

    if (psengine->GetEffectManager ())
    {
        // This code work as long as PutUnique is used to enter
        // new entries to the hash.
        unsigned int effectID = effectMapper.Get(effect.uid,0);
        psengine->GetEffectManager()->DeleteEffect(effectID);
        effectMapper.DeleteAll(effect.uid);
    }
}

void psClientCharManager::HandleEffect( MsgEntry* me )
{
    psEffectMessage effect(me);

    // get the anchor
    GEMClientObject* gemAnchor = cel->FindObject(effect.anchorID);
    iMeshWrapper * anchor = 0;

    if (!gemAnchor && effect.anchorID != 0)
    {
        // The entity isn't here, probably because it's too far away, so don't render the effect.
        // Not sure if we should do anything here, but meh.
    }
    else
    {
        if (gemAnchor)
            anchor = gemAnchor->GetMesh();

        // get the target
        GEMClientObject* gemTarget = cel->FindObject(effect.targetID);
        iMeshWrapper *meshtarget = anchor;
        if (gemTarget)
            meshtarget = gemTarget->GetMesh();

        // render the actual effect
        if (psengine->GetEffectManager ())
        {
            unsigned int effectID = 0;
            unsigned int uniqueIDOverride = effectMapper.Get(effect.uid, 0); // Will return 0 if no uid in store or uid is 0.
            csVector3 up(0,1,0);

            if (anchor)
            {
                effectID = psengine->GetEffectManager()->RenderEffect(effect.name, effect.offset,
                                                                      anchor, meshtarget, up, uniqueIDOverride,
                                                                      false, effect.scale);
            }
            else
            {
                iSector * sector = psengine->GetPSCamera()->GetICamera()->GetCamera()->GetSector(); // Sector should come in the message
                effectID = psengine->GetEffectManager()->RenderEffect(effect.name, sector,
                                                                      effect.offset, meshtarget,
                                                                      up, uniqueIDOverride, effect.scale);
            }

            if (effectID == 0)
            {
                Error2("Failed to render effect %s",effect.name.GetDataSafe());
            }
            else
            {
                if(effect.duration)
                {
                    psEffect* eff = psengine->GetEffectManager()->FindEffect(effectID);
                    eff->SetKillTime(effect.duration);
                }
            }

            if ( effect.uid != 0 )
            {
                effectMapper.PutUnique( effect.uid, effectID );
            }
        }
    }
}


void psClientCharManager::HandleRejectCharMessage( MsgEntry* me )
{
    psCharRejectedMessage reject( me );

    switch ( reject.errorType )
    {
        case psCharRejectedMessage::NON_LEGAL_NAME:
        case psCharRejectedMessage::NON_UNIQUE_NAME:
        case psCharRejectedMessage::RESERVED_NAME:
        case psCharRejectedMessage::INVALID_CREATION:
        {
            PawsManager::GetSingleton().CreateWarningBox( reject.errorMesg );
            PawsManager::GetSingleton().FindWidget("CharCreateMain")->Show();
            PawsManager::GetSingleton().FindWidget("Summary")->Hide();

            break;
        }
    }

}

//------------------------------------------------------------------------------

psCreationManager::psCreationManager( iObjectRegistry* objReg )
{
    objectReg = objReg;
    LoadRaceInformation();

    msgHandler = psengine->GetMsgHandler();
    msgHandler->Subscribe(this, MSGTYPE_CHAR_CREATE_CP);
    msgHandler->Subscribe(this, MSGTYPE_CHAR_CREATE_PARENTS);
    msgHandler->Subscribe(this, MSGTYPE_CHAR_CREATE_CHILDHOOD);
    msgHandler->Subscribe( this, MSGTYPE_CHAR_CREATE_UPLOAD );
    msgHandler->Subscribe( this, MSGTYPE_CHAR_CREATE_VERIFY );
    msgHandler->Subscribe( this, MSGTYPE_CHAR_CREATE_LIFEEVENTS );
    msgHandler->Subscribe( this, MSGTYPE_CHAR_CREATE_TRAITS );

    hasParentData = false;
    hasChildhoodData = false;
    hasLifeEventData = false;
    hasTraitData = false;

    selectedRace = -1;
    selectedFace = -1;
    selectedHairStyle = -1;
    selectedBeardStyle = -1;
    selectedHairColour = -1;
    selectedSkinColour = -1;
    selectedGender = PSCHARACTER_GENDER_MALE;
    fatherMod = 1;
    motherMod = 1;

    nameGenerator = new NameGenerationSystem();
    nameGenerator->LoadDatabase( objReg );

    ClearChoices();
}

psCreationManager::~psCreationManager()
{
    msgHandler->Unsubscribe( this, MSGTYPE_CHAR_CREATE_UPLOAD );
    msgHandler->Unsubscribe( this, MSGTYPE_CHAR_CREATE_LIFEEVENTS );
    msgHandler->Unsubscribe( this, MSGTYPE_CHAR_CREATE_VERIFY );
    delete nameGenerator;

}

int psCreationManager::GetRaceCP( int race )
{
    if ( raceDescriptions[race]->startingCP == -1 )
    {
        // This is a simple message so don't need a seperate class for it.
        // We can just create the message our selves.
        csRef<MsgEntry> msg;
        msg.AttachNew(new MsgEntry( 100 ));
        msg->SetType(MSGTYPE_CHAR_CREATE_CP);
        msg->Add( (int32_t) race );
        msg->ClipToCurrentSize();

        msgHandler->SendMessage(msg);
        return REQUESTING_CP;
    }
    else
    {
        return raceDescriptions[race]->startingCP;
    }
}

void psCreationManager::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_CHAR_CREATE_CP:
        {
            int race   = me->GetInt32();
            int points = me->GetInt32();
            raceDescriptions[race]->startingCP = points;
            return;
        }
        case MSGTYPE_CHAR_CREATE_PARENTS:
        {
            HandleParentsData( me );
            hasParentData = true;
            break;
        }
        case MSGTYPE_CHAR_CREATE_CHILDHOOD:
        {
            HandleChildhoodData( me );
            hasChildhoodData = true;
            break;
        }

        case MSGTYPE_CHAR_CREATE_LIFEEVENTS:
        {
            HandleLifeEventData( me );
            hasLifeEventData = true;
            break;
        }

        case MSGTYPE_CHAR_CREATE_TRAITS:
        {
            HandleTraitData( me );
            hasTraitData = true;
            break;
        }

        case MSGTYPE_CHAR_CREATE_VERIFY:
        {
            HandleVerify( me );
            break;
        }

        case MSGTYPE_CHAR_CREATE_UPLOAD:
        {
            pawsWidget* wdg = PawsManager::GetSingleton().FindWidget("Summary");
            if (wdg)
                wdg->Hide();

            psengine->StartLoad();
            break;
        }

    }
}

void psCreationManager::HandleLifeEventData( MsgEntry* me )
{
    psLifeEventMsg incomming( me );

    lifeEventData.SetSize( incomming.choices.GetSize() );

    for ( size_t x = 0; x < incomming.choices.GetSize(); x++ )
    {
        lifeEventData[x].id = incomming.choices[x]->id;
        lifeEventData[x].name = incomming.choices[x]->name;
        lifeEventData[x].description = incomming.choices[x]->description;
        lifeEventData[x].common = incomming.choices[x]->common;
        lifeEventData[x].cpCost = incomming.choices[x]->cpCost;

        incomming.choices[x]->adds.TransferTo( lifeEventData[x].adds );
        incomming.choices[x]->removes.TransferTo( lifeEventData[x].removes );
    }
}

void psCreationManager::HandleTraitData( MsgEntry* me )
{
    psCharCreateTraitsMessage msg(me);
    iDocumentSystem* xml = psengine->GetXMLParser ();
    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse(msg.GetString().GetData());
    if ( error )
    {
        Error2("Error in XML: %s", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in Trait Data");
        return;
    }
    csRef<iDocumentNodeIterator> iter1 = root->GetNode("traits")->GetNodes("trait");

    // Build the traits list
    while ( iter1->HasNext() )
    {
        csRef<iDocumentNode> node = iter1->Next();

        Trait *t = new Trait;
        t->Load( node );

        traits.Push(t);
    }

    TraitIterator iter2 = GetTraitIterator();
    while ( iter2.HasNext() )
    {
        Trait * t = iter2.Next();
        t->next_trait = GetTrait(t->next_trait_uid);
        if (t->next_trait != NULL)
        {
           t->next_trait->prev_trait = t;
        }
    }

    // Insert into the custom location structure all top trait
    TraitIterator iter3 = GetTraitIterator();
    while ( iter3.HasNext() )
    {
        Trait * t = iter3.Next();
        // Check for top trait
        if (t->prev_trait == NULL)
        {
            RaceDefinition * race = GetRace(t->raceID);
            if (race != NULL)
            {
                if (t->gender == PSCHARACTER_GENDER_NONE)
                {
                    race->location[t->location][PSCHARACTER_GENDER_MALE].Push(t); // still needed?
                    race->location[t->location][PSCHARACTER_GENDER_FEMALE].Push(t); // still needed?
                    race->location[t->location][PSCHARACTER_GENDER_NONE].Push(t);
                }
                else
                {
                    race->location[t->location][t->gender].Push(t);
                }
            }
            else
            {
                Error3("Failed to insert trait '%s' into location table for race %d.\n",t->name.GetData(),t->raceID);
            }
        }
    }
}

Trait* psCreationManager::GetTrait(unsigned int uid)
{
    TraitIterator tIter = GetTraitIterator();
    while ( tIter.HasNext() )
    {
        Trait * t = tIter.Next();
        if (t->uid == uid)
            return t;
    }
    return NULL;
}

void psCreationManager::HandleVerify( MsgEntry* me )
{
    psCharVerificationMesg mesg(me);
    pawsSummaryWindow * window = (pawsSummaryWindow*)PawsManager::GetSingleton().FindWidget("Summary");
    if ( window )
        window->SetVerify( mesg.stats, mesg.skills );
}

void psCreationManager::HandleChildhoodData( MsgEntry* me )
{
    psCreationChoiceMsg incomming( me );
    // Simple create a copy of the data in the message.
    incomming.choices.TransferTo( childhoodData );
}


void psCreationManager::HandleParentsData( MsgEntry* me )
{
    psCreationChoiceMsg incomming( me );

    // Simple create a copy of the data in the message.
    incomming.choices.TransferTo( parentData );
}


void psCreationManager::LoadRaceInformation()
{
    iDocumentSystem* xml = psengine->GetXMLParser ();
    iVFS* vfs = psengine->GetVFS ();

    csRef<iDataBuffer> buff = vfs->ReadFile( "/this/data/races/descriptions.xml" );

    if ( !buff || !buff->GetSize() )
    {
        Error2( "Could not load XML: %s", "descriptions.xml" );
        return;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( buff );
    if ( error )
    {
        Error3( "Error parsing XML file %s: %s",  "descriptions.xml", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in descriptions.xml");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("descriptions");
    if(!topNode)
    {
        Error1("No <description> tag in descriptions.xml");
        return;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        csString raceName     = node->GetAttributeValue( "name" );
        csString description  = node->GetAttributeValue( "description" );
        csString maleModel    = node->GetAttributeValue( "male" );
        csString femaleModel  = node->GetAttributeValue( "female" );
        bool mavailable       = node->GetAttributeValueAsBool("male_available");
        bool favailable       = node->GetAttributeValueAsBool("female_available");

        RaceDefinition* race = new RaceDefinition;
        race->name = raceName;
        race->description = description;
        race->startingCP = -1;
        race->femaleModelName = femaleModel;
        race->maleModelName = maleModel;
        race->femaleAvailable = favailable;
        race->maleAvailable = mavailable;

        // defaults
        race->FollowPos.Set(0,3,4);
        race->LookatPos.Set(0,2,0);
        race->FirstPos.Set(0,1.5,-.25);

        csRef<iDocumentNode> fp = node->GetNode("FollowPos");
        if (fp)
            race->FollowPos.Set(fp->GetAttributeValueAsFloat("x"),fp->GetAttributeValueAsFloat("y"),fp->GetAttributeValueAsFloat("z"));


        fp = node->GetNode("LookatPos");
        if (fp)
            race->LookatPos.Set(fp->GetAttributeValueAsFloat("x"),fp->GetAttributeValueAsFloat("y"),fp->GetAttributeValueAsFloat("z"));


        fp = node->GetNode("FirstPos");
        if (fp)
            race->FirstPos.Set(fp->GetAttributeValueAsFloat("x"),fp->GetAttributeValueAsFloat("y"),fp->GetAttributeValueAsFloat("z"));


        // Load the default camera position in the custom carachter generator
        fp = node->GetNode("ViewerPos");
        if (fp)
            race->ViewerPos.Set(fp->GetAttributeValueAsFloat("x"),fp->GetAttributeValueAsFloat("y"),fp->GetAttributeValueAsFloat("z"));

        for ( int z = 0; z < PSTRAIT_LOCATION_COUNT; z++ )
        {
            race->zoomLocations[z].Set( 0,0,0 );
        }

        fp = node->GetNode("FACEPOS");

        if (fp)
            race->zoomLocations[PSTRAIT_LOCATION_FACE].Set(fp->GetAttributeValueAsFloat("x"),fp->GetAttributeValueAsFloat("y"),fp->GetAttributeValueAsFloat("z"));


        raceDescriptions.Push( race );
    }

}

void psCreationManager::LoadPathInfo()
{
    iDocumentSystem* xml = psengine->GetXMLParser ();
    iVFS* vfs = psengine->GetVFS ();

    csRef<iDataBuffer> buff = vfs->ReadFile( "/this/data/races/quickpaths.xml" );

    if ( !buff || !buff->GetSize() )
    {
        Error2( "Could not load XML: %s", "quickpaths.xml" );
        return;
    }

    csRef<iDocument> doc = xml->CreateDocument();
    const char* error = doc->Parse( buff );
    if ( error )
    {
        Error3( "Error parsing XML file %s: %s",  "quickpaths.xml", error );
        return;
    }

    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in quickpaths.xml");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("paths");
    if(!topNode)
    {
        Error1("No <paths> tag in quickpaths.xml");
        return;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        PathDefinition* path = new PathDefinition();
        path->name = node->GetAttributeValue("name");
        if ( path->name.IsEmpty() )
            path->name = "None";
        path->info = node->GetAttributeValue("info");
        if ( path->info.IsEmpty() )
            path->info = "None";
        path->parents = node->GetAttributeValue("parents");
        if ( path->parents.IsEmpty() )
             path->parents = "None";
        path->life = node->GetAttributeValue("life");
        if ( path->life.IsEmpty() )
             path->life = "None";

        //Load in all of the stat bonuses
        csRef<iDocumentNodeIterator> iter = node->GetNodes("StatBonus");
        while (iter->HasNext())
        {
            csRef<iDocumentNode> tmp = iter->Next();

            PathDefinition::Bonus* bonus = new PathDefinition::Bonus();

            csString name  = tmp->GetAttributeValue("name");
            float value = tmp->GetAttributeValueAsFloat("value");
            bonus->name = name;
            bonus->value = value;
            path->statBonuses.Push(bonus);
        }

        //Load in all of the bonus skills
        iter = node->GetNodes("SkillBonus");
        while (iter->HasNext())
        {
            csRef<iDocumentNode> tmp = iter->Next();

            PathDefinition::Bonus* bonus = new PathDefinition::Bonus();

            csString skillName  = tmp->GetAttributeValue("name");
            float rank = tmp->GetAttributeValueAsFloat("rank");
            bonus->name = skillName;
            bonus->value = rank;
            path->skillBonuses.Push(bonus);
        }
        pathDefinitions.Push(path);
    }
}

const char* psCreationManager::GetRaceDescription( int race )
{
    if ( (size_t)race < raceDescriptions.GetSize() )
        return raceDescriptions[race]->description;
    else
        return "None";
}

const char* psCreationManager::GetModelName( int race, int gender )
{
    if ( (size_t)race < raceDescriptions.GetSize() && race >= 0 )
    {
        if ( gender == PSCHARACTER_GENDER_NONE ||
             gender == PSCHARACTER_GENDER_MALE )
        {
            return raceDescriptions[race]->maleModelName;
        }
        else
        {
            return raceDescriptions[race]->femaleModelName;
        }
    }

    return NULL;
}



void psCreationManager::GetParentData()
{
    if ( !hasParentData )
    {
        psCreationChoiceMsg outgoing(MSGTYPE_CHAR_CREATE_PARENTS);
        outgoing.SendMessage();
    }
}

void psCreationManager::GetChildhoodData()
{
    if ( !hasChildhoodData )
    {
        psCreationChoiceMsg outgoing(MSGTYPE_CHAR_CREATE_CHILDHOOD);
        outgoing.SendMessage();
    }
}

void psCreationManager::GetLifeEventData()
{
    if ( !hasLifeEventData )
    {
        psLifeEventMsg outgoing;
        outgoing.SendMessage();
    }
}

void psCreationManager::GetTraitData()
{
    if ( !hasTraitData )
    {
        psCreationChoiceMsg outgoing(MSGTYPE_CHAR_CREATE_TRAITS);
        outgoing.SendMessage();
    }
}


RaceDefinition* psCreationManager::GetRace( int race )
{
    return raceDescriptions[race];
}

const char* psCreationManager::GetDescription( int id )
{
    size_t x;
    for ( x = 0; x < parentData.GetSize(); x++ )
    {
        if (  parentData[x].id == id )
            return parentData[x].description;
    }

    for ( x = 0; x < childhoodData.GetSize(); x++ )
    {
        if (  childhoodData[x].id == id )
            return childhoodData[x].description;
    }


    return "None";
}

CreationChoice* psCreationManager::GetChoice( int id )
{
    size_t x;
    for ( x = 0; x < parentData.GetSize(); x++ )
    {
        if (  parentData[x].id == id )
            return &parentData[x];
    }

    for ( x = 0; x < childhoodData.GetSize(); x++ )
    {
        if (  childhoodData[x].id == id )
            return &childhoodData[x];
    }


    return NULL;
}

void psCreationManager::SetRace( int newRace )
{
    selectedRace = newRace;
}

void psCreationManager::UploadChar( bool verify )
{
    RaceDefinition* race = GetRace( selectedRace );
    if ( !race )
        return;

    csString firstname;
    csString lastname;

    // Separate names
    firstname = selectedName.Slice(0,selectedName.FindFirst(' '));
    if (selectedName.FindFirst(' ') != SIZET_NOT_FOUND)
        lastname = selectedName.Slice(selectedName.FindFirst(' ')+1,selectedName.Length());
    else
        lastname.Clear();

    // Make BIO
    pawsSummaryWindow* summary = (pawsSummaryWindow*)PawsManager::GetSingleton().FindWidget("Summary");
    pawsMultiLineTextBox* btext = (pawsMultiLineTextBox*) summary->FindWidget("text_birth");
    pawsMultiLineTextBox* ltext = (pawsMultiLineTextBox*) summary->FindWidget("text_life");

    csString bio;
    bio += btext->GetText();
    bio += ltext->GetText();

    psCharUploadMessage upload( verify, firstname.GetDataSafe(), lastname.GetDataSafe(),
                                selectedRace, selectedGender, choicesMade, motherMod, fatherMod,
                                lifeEventsMade, selectedFace, selectedHairStyle, selectedBeardStyle,
                                selectedHairColour, selectedSkinColour, bio.GetDataSafe(), path.GetDataSafe() );

    upload.SendMessage();
}

void psCreationManager::SetName( const char* newName )
{
    selectedName.Replace( newName );
}

csString psCreationManager::GetName()
{
    return selectedName;
}



LifeEventChoice* psCreationManager::FindLifeEvent( int idNumber )
{
    for ( size_t x = 0; x < lifeEventData.GetSize(); x++ )
    {
        if ( lifeEventData[x].id == idNumber )
            return &lifeEventData[x];
    }

    return NULL;
}


void psCreationManager::AddChoice( int choice, int modifier )
{
    if(!GetChoice(choice))
    {
        Error2("Invalid creation choice: %i", choice);
        return;
    }
    choicesMade.Push( choice );
    currentCP-=GetCost( choice )*modifier;
}


void psCreationManager::RemoveChoice( uint32_t choice, int modifier )
{
    for ( size_t x = 0; x < choicesMade.GetSize(); x++ )
    {
        if ( choicesMade[x] == choice )
        {
            currentCP += GetCost( choicesMade[x] )*modifier;
            choicesMade.DeleteIndex(x);

            return;
        }
    }
}

void psCreationManager::AddLifeEvent( int event )
{
    lifeEventsMade.Push( event );
    currentCP -= GetLifeCost( event );
}

void psCreationManager::RemoveLifeEvent( uint32_t event )
{
    for ( size_t x = 0; x < lifeEventsMade.GetSize(); x++ )
    {
        if ( lifeEventsMade[x] == event )
        {
            currentCP += GetLifeCost( event );
            lifeEventsMade.DeleteIndex(x);
            return;
        }
    }
}

int psCreationManager::GetCost( int id )
{
    size_t x;
    for ( x = 0; x < parentData.GetSize(); x++ )
    {
        if (  parentData[x].id == id )
            return parentData[x].cpCost;
    }

    for ( x = 0; x < childhoodData.GetSize(); x++ )
    {
        if (  childhoodData[x].id == id )
            return childhoodData[x].cpCost;
    }

    return -1;
}

int psCreationManager::GetLifeCost( int id )
{
    LifeEventChoice* life = FindLifeEvent( id );
    if ( life != NULL )
        return life->cpCost;

    return 0;
}

void psCreationManager::SetCustomization( int face, int hairStyle, int beardStyle, int hairColour, int skinColour )
{
    selectedFace = face;
    selectedHairStyle = hairStyle;
    selectedBeardStyle = beardStyle;
    selectedHairColour = hairColour;
    selectedSkinColour = skinColour;
}

void psCreationManager::GetCustomization( int& face, int& hairStyle, int& beardStyle, int& hairColour, int& skinColour )
{
    face = selectedFace;
    hairStyle = selectedHairStyle;
    beardStyle = selectedBeardStyle;
    hairColour = selectedHairColour;
    skinColour = selectedSkinColour;
}

RaceDefinition* psCreationManager::GetRace( const char* name )
{
    for ( size_t i = 0; i < raceDescriptions.GetSize(); i++ )
    {
        if ( raceDescriptions[i]->name == name )
            return raceDescriptions[i];
    }
    return NULL;
}

csPDelArray<PathDefinition>::Iterator psCreationManager::GetPathIterator()
{
    if (pathDefinitions.GetSize() == 0)
        LoadPathInfo();
    return pathDefinitions.GetIterator();
}

PathDefinition* psCreationManager::GetPath(int i)
{
    return pathDefinitions[i];
}

const char* psCreationManager::GetLifeEventDescription( int id )
{
     for ( size_t n = 0; n < lifeEventData.GetSize(); n++ )
     {
        if ( lifeEventData[n].id == id )
        {
            return lifeEventData[n].description;
        }
     }

     return "None";
}

bool psCreationManager::IsAvailable(int id, int gender)
{
    RaceDefinition* race = GetRace(id);
    if (!race)
        return false;

    if (gender==PSCHARACTER_GENDER_NONE)
        return race->maleAvailable; //If neutral gender, check if male gender is available
    else if (gender==PSCHARACTER_GENDER_FEMALE)
        return race->femaleAvailable;
    else if (gender==PSCHARACTER_GENDER_MALE)
        return race->maleAvailable;

    return false;
}

void psCreationManager::ClearChoices()
{
    if (selectedRace != -1)
    {
        currentCP = GetRaceCP( selectedRace );
        path = "None";
    }
    choicesMade.DeleteAll();
    fatherMod = 1;
    motherMod = 1;
    lifeEventsMade.DeleteAll();
}
