/*
* psclientdr.cpp by Matze Braun <MatzeBraun@gmx.de>
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
*/
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iutil/databuff.h>
#include <csutil/sysfunc.h>
#include <iengine/mesh.h>
#include <iengine/sector.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/engseq.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/message.h"
#include "net/clientmsghandler.h"
#include "net/connection.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psclientdr.h"
#include "pscelclient.h"
#include "modehandler.h"
#include "clientvitals.h"
#include "psnetmanager.h"
#include "globals.h"
#include "psclientchar.h"
#include "zonehandler.h"

////////////////////////////////////////////////////////////////////////////
//  PAWS INCLUDES
////////////////////////////////////////////////////////////////////////////
#include "paws/pawsmanager.h"
#include "gui/pawsgroupwindow.h"
#include "gui/pawsinfowindow.h"
#include "gui/pawscharpick.h"

psClientDR::psClientDR()
{
    celclient = NULL;
    msgstrings = NULL;
    gotStrings = false;
    groupWindow = 0;
}

psClientDR::~psClientDR()
{
    if (msghandler)
    {
        msghandler->Unsubscribe(this,MSGTYPE_DEAD_RECKONING);
        msghandler->Unsubscribe(this,MSGTYPE_FORCE_POSITION);
        msghandler->Unsubscribe(this,MSGTYPE_STATDRUPDATE);
        msghandler->Unsubscribe(this,MSGTYPE_MSGSTRINGS);
        msghandler->Unsubscribe(this,MSGTYPE_OVERRIDEACTION);
        msghandler->Unsubscribe(this,MSGTYPE_SEQUENCE);
    }

    // allocated by message cracker, but released here
    delete msgstrings;
}

bool psClientDR::Initialize(iObjectRegistry* object_reg, psCelClient* celclient, MsgHandler* msghandler )
{
    psClientDR::object_reg = object_reg;
    psClientDR::celclient = celclient;
    psClientDR::msghandler = msghandler;
    
    msgstrings = NULL; // will get it in a MSGTYPE_MSGSTRINGS message

    msghandler->Subscribe(this,MSGTYPE_DEAD_RECKONING);
    msghandler->Subscribe(this,MSGTYPE_FORCE_POSITION);
    msghandler->Subscribe(this,MSGTYPE_STATDRUPDATE);
    msghandler->Subscribe(this,MSGTYPE_MSGSTRINGS);
    msghandler->Subscribe(this,MSGTYPE_OVERRIDEACTION);
    msghandler->Subscribe(this,MSGTYPE_SEQUENCE);

    return true; 
}

// time till next update packet is send
#define UPDATEWAIT  200

void psClientDR::CheckDeadReckoningUpdate()
{
    static float prevVelY = 0;  // vertical velocity of the character when this method was last called
    csVector3 vel;              // current  velocity of the character
    bool beganFalling;

    if ( !celclient->GetMainPlayer() )
    {
        return;
    }            

    if(celclient->IsUnresSector(celclient->GetMainPlayer()->GetSector()))
        return;                 // Main actor still in unresolved sector

    vel = celclient->GetMainPlayer()->GetVelocity();

    beganFalling = (prevVelY>=0 && vel.y<0);
    prevVelY = vel.y;

    uint8_t priority;

    if (! (celclient->GetMainPlayer()->NeedDRUpdate(priority)  ||  beganFalling) )
    {
        // no need to send new packet, real position is as calculated
        return;
    }

    if (!msgstrings)
    {
        Error1("msgstrings not received, cannot handle DR");
        return;
    }

    celclient->GetMainPlayer()->SendDRUpdate(priority,msgstrings);

    return;
}

void psClientDR::CheckSectorCrossing(GEMClientActor *actor)
{
    csString curr = actor->GetSector()->QueryObject()->GetName();

    if (last_sector.IsEmpty())
    {
        last_sector = curr;
        return;
    }

    if (curr != last_sector)
    {
        // crossed sector boundary since last update
        csVector3 pos;
        float yrot;
        iSector* sector;

        actor->GetLastPosition (pos, yrot, sector);
        psNewSectorMessage cross(last_sector, curr, pos);
        msghandler->Publish(cross.msg);  // subscribers to sector messages can process here

        last_sector = curr;
    }
}

void psClientDR::HandleMessage (MsgEntry* me)
{
    if (me->GetType() == MSGTYPE_DEAD_RECKONING)
    {
        HandleDeadReckon( me );
    }
    else if (me->GetType() == MSGTYPE_FORCE_POSITION)
    {
        HandleForcePosition(me);
    }
    else if (me->GetType() == MSGTYPE_STATDRUPDATE)
    {
        HandleStatsUpdate( me );
    }
    else if (me->GetType() == MSGTYPE_MSGSTRINGS)
    {
        HandleStrings( me );
    }
    else if (me->GetType() == MSGTYPE_OVERRIDEACTION)
    {
        HandleOverride( me );
    }
    else if (me->GetType() == MSGTYPE_SEQUENCE)
    {
        HandleSequence( me );
    }
}

void psClientDR::ResetMsgStrings()
{
    if(psengine->IsGameLoaded())
        return; // Too late

    delete msgstrings;
    msgstrings = NULL;
    gotStrings = false;
}

void psClientDR::HandleDeadReckon( MsgEntry* me )
{
    psDRMessage drmsg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers() );
    GEMClientActor* gemActor = (GEMClientActor*)celclient->FindObject( drmsg.entityid );
     
    if (!gemActor)
    {
        Error2("Got DR message for unknown entity %s.", ShowID(drmsg.entityid));
        return;
    }

    if (!msgstrings)
    {
        Error1("msgstrings not received, cannot handle DR");
        return;
    }

    // Ignore any updates to the main player...psForcePositionMessage handles that.
    if (gemActor == celclient->GetMainPlayer())
    {
        psengine->GetModeHandler()->SetModeSounds(drmsg.mode);
        return;
    }

    // Set data for this actor
    gemActor->SetDRData(drmsg);
}

void psClientDR::HandleForcePosition(MsgEntry *me)
{
    psForcePositionMessage msg(me, psengine->GetNetManager()->GetConnection()->GetAccessPointers());

    if(last_sector != msg.sectorName)
    {
        psengine->GetZoneHandler()->HandleDelayAndAnim(msg.loadTime, msg.start, msg.dest, msg.backgroundname, msg.loadWidget);
        psNewSectorMessage cross(last_sector, msg.sectorName, msg.pos);
        msghandler->Publish(cross.msg);
        Error3("Sector crossed from %s to %s after forced position update.\n", last_sector.GetData(), msg.sectorName.GetData());
    }
    else
    {
        psengine->GetZoneHandler()->HandleDelayAndAnim(msg.loadTime, msg.start, msg.dest, msg.backgroundname, msg.loadWidget);
        psengine->GetZoneHandler()->LoadZone(msg.pos, msg.yrot, msg.sectorName, csVector3(0.0f, 0.0f, msg.vel), true);
    }
}

void psClientDR::HandleStatsUpdate( MsgEntry* me )
{
    psStatDRMessage statdrmsg(me);
    GEMClientActor* gemObject  = (GEMClientActor*)celclient->FindObject( statdrmsg.entityid );
    if (!gemObject)
    {
        Error2("Stat request failed because CelClient not ready for %s", ShowID(statdrmsg.entityid));
        return;
    }

    // Dirty short cut to allways display 0 HP when dead.
    if (!gemObject->IsAlive() && statdrmsg.hp)
    {
        Error1("Server report HP but object is not alive");
        statdrmsg.hp = 0;
        statdrmsg.hp_rate = 0;
    }
    
    // Check if this client actor was updated
    GEMClientActor* mainActor = celclient->GetMainPlayer();
    
    if (mainActor == gemObject)
    {
        gemObject->GetVitalMgr()->HandleDRData(statdrmsg,"Self");
    }        
    else 
    {   // Publish Vitals data using EntityID
        csString ID;
        ID.Append(gemObject->GetEID().Unbox());
        gemObject->GetVitalMgr()->HandleDRData(statdrmsg, ID.GetData() );
    }

    //It is not an else if so Target is always published
    if (psengine->GetCharManager()->GetTarget() == gemObject)
        gemObject->GetVitalMgr()->HandleDRData(statdrmsg,"Target"); 
    
    if (mainActor != gemObject && gemObject->IsGroupedWith(celclient->GetMainPlayer()) )
    {
        if (!groupWindow)
        {
            // Get the windowMgr

            pawsWidget* widget = PawsManager::GetSingleton().FindWidget("GroupWindow");
            groupWindow = (pawsGroupWindow*)widget;

            if (!groupWindow) 
            {
                Error1("Group Window Was Not Found. Bad Error");
                return;
            }
        }
        
        groupWindow->SetStats(gemObject);
    }
}

void psClientDR::HandleStrings( MsgEntry* me )
{
    // Receive message strings.
    psMsgStringsMessage msg(me);

    // Check for digest message.
    if(msg.only_carrying_digest)
    {
        bool request_strings = true;

        // Check for cached strings and compare with digest.
        if(psengine->GetVFS()->Exists("/planeshift/userdata/cache/commonstrings"))
        {
            csRef<iDataBuffer> buf = psengine->GetVFS()->ReadFile("/planeshift/userdata/cache/commonstrings");
            csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(psengine->GetObjectRegistry());
            csRef<iDocument> doc = docsys->CreateDocument();
            doc->Parse(buf);

            // Check that the cached strings are up to date.
            csRef<iDocumentNode> root = doc->GetRoot();
            if(msg.digest->HexString() == root->GetNode("md5sum")->GetContentsValue())
            {
                // They are, so let's load them.
                request_strings = false;
                msgstrings = new csStringHashReversible();
                
                csRef<iDocumentNodeIterator> strings = root->GetNodes("string");
                while(strings->HasNext())
                {
                    csRef<iDocumentNode> string = strings->Next();
                    msgstrings->Register(string->GetAttributeValue("string"), csStringID(string->GetAttributeValueAsInt("id")));
                }
            }
        }

        if(request_strings)
        {
            psMsgStringsMessage request;
            request.SendMessage();
            return;
        }
    }
    else
    {
        // Not a digest message, get strings.
        msgstrings = msg.msgstrings;

        // Write cache.
        csRef<iDocumentSystem> docsys = csLoadPluginCheck<iDocumentSystem>(psengine->GetObjectRegistry(),
            "crystalspace.documentsystem.binary");
        csRef<iDocument> doc = docsys->CreateDocument();
        csRef<iDocumentNode> root = doc->CreateRoot();

        // Write md5sum.
        root = root->CreateNodeBefore(CS_NODE_ELEMENT);
        root->SetValue("md5sum");
        root = root->CreateNodeBefore(CS_NODE_TEXT);
        root->SetValue(msg.digest->HexString());

        // Write strings.
        root = doc->GetRoot();
        csStringHashReversible::GlobalIterator it = msgstrings->GetIterator();
        while (it.HasNext())
        {
            const char* string;
            csStringID id = it.Next(string);

            root = root->CreateNodeBefore(CS_NODE_ELEMENT);
            root->SetValue("string");
            root->SetAttributeAsInt("id", id.GetHash());
            root->SetAttribute("string", string);
            root = root->GetParent();
        }

        doc->Write(psengine->GetVFS(), "/planeshift/userdata/cache/commonstrings");
    }

    psengine->GetNetManager()->GetConnection()->SetMsgStrings(0, msgstrings);
    psengine->GetNetManager()->GetConnection()->SetEngine(psengine->GetEngine());
    gotStrings = true;

    // Update status.
    pawsCharacterPickerWindow* charPick = (pawsCharacterPickerWindow*)PawsManager::GetSingleton().FindWidget("CharPickerWindow");
    if(charPick) // If it doesn't exist now, it will check the gotStrings flag on show. In other word no need to panic
    {
        charPick->ReceivedStrings();

        if(psengine->IsLoggedIn())
        {
            PawsManager::GetSingleton().FindWidget("CharPickerWindow")->Show();
            delete PawsManager::GetSingleton().FindWidget("LoginWindow");
            delete PawsManager::GetSingleton().FindWidget("CreditsWindow");
        }
    }    
}

void psClientDR::HandleOverride( MsgEntry* me )
{
    psOverrideActionMessage msg(me);
    if (!msg.valid)
        return;

    GEMClientActor* gemActor = (GEMClientActor*)celclient->FindObject(msg.entity_id);
    if (!gemActor)
        return;

    gemActor->SetAnimation( msg.action.GetData(), msg.duration );
}


void psClientDR::HandleSequence( MsgEntry* me )
{
    psSequenceMessage msg(me);
    if (!msg.valid)
        return;

    csRef<iEngineSequenceManager> seqmgr(
        csQueryRegistry<iEngineSequenceManager> (psengine->GetObjectRegistry()));
    if (seqmgr)
    {
        seqmgr->RunSequenceByName (msg.name,0);
    }
}

void psClientDR::HandleDeath(GEMClientActor * gemObject)
{
    // Check if this client actor was updated    
    if ( gemObject == celclient->GetMainPlayer() )
    {
        gemObject->GetVitalMgr()->HandleDeath("Self");
    }
    else 
    {   // Publish Vitals data using EntityID
        csString ID((size_t) gemObject->GetEID().Unbox());
        gemObject->GetVitalMgr()->HandleDeath( ID.GetData() );
    }

    //It is not an else if so Target is always published
    if (psengine->GetCharManager()->GetTarget() == gemObject)
    {
        gemObject->GetVitalMgr()->HandleDeath("Target"); 
    }
}
