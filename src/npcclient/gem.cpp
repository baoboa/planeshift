/*
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
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/engine.h>
#include <imap/loader.h>
#include <imesh/nullmesh.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>

//=============================================================================
// Project Library Includes
//=============================================================================
#include <ibgloader.h>
#include "util/consoleout.h"
#include "util/psxmlparser.h"

#include "engine/linmove.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "gem.h"
#include "npc.h"
#include "npcmesh.h"
#include "networkmgr.h"
#include "npcbehave.h"

//-----------------------------------------------------------------------------

csRef<iMeshFactoryWrapper> gemNPCObject::nullfact = NULL;

psNpcMeshAttach::psNpcMeshAttach(gemNPCObject* objectToAttach) : scfImplementationType(this)
{
    object = objectToAttach;
}


//-------------------------------------------------------------------------------

gemNPCObject::gemNPCObject(psNPCClient* npcclient, EID id)
    :pcmesh(NULL), eid(id), type(0), visible(true), invincible(false),
     isAlive(true), scale(1.0), baseScale(1.0), instance(DEFAULT_INSTANCE)
{
}

gemNPCObject::~gemNPCObject()
{
    delete pcmesh;
}

void gemNPCObject::Move(const csVector3 &pos, float rotangle,  const char* room, InstanceID instance)
{
    SetInstance(instance);
    Move(pos,rotangle,room);
}


void gemNPCObject::Move(const csVector3 &pos, float rotangle,  const char* room)
{
    // Position and sector
    iSector* sector = psNPCClient::npcclient->GetEngine()->FindSector(room);
    if(sector == NULL)
    {
        CPrintf(CON_DEBUG, "Can't move npc object %s. Sector %s not found!\n", ShowID(eid), room);
        return;
    }
    pcmesh->MoveMesh(sector , pos);

    // Rotation
    csMatrix3 matrix = (csMatrix3) csYRotMatrix3(rotangle);
    pcmesh->GetMesh()->GetMovable()->GetTransform().SetO2T(matrix);

}

bool gemNPCObject::InitMesh(const char* factname,
                            const char* filename,
                            const csVector3 &pos,
                            const float rotangle,
                            const char* room
                           )
{
    pcmesh = new npcMesh(psNPCClient::npcclient->GetObjectReg(), this, psNPCClient::npcclient);

    csRef<iEngine> engine = csQueryRegistry<iEngine> (psNPCClient::npcclient->GetObjectReg());
    csRef<iMeshWrapper> mesh;

    if(csString("").Compare(filename) || csString("nullmesh").Compare(factname))
    {
        if(!nullfact)
        {
            nullfact = engine->CreateMeshFactory("crystalspace.mesh.object.null", "nullmesh", false);
            csRef<iNullFactoryState> nullstate = scfQueryInterface<iNullFactoryState> (nullfact->GetMeshObjectFactory());
            csBox3 bbox;
            bbox.AddBoundingVertex(csVector3(0.0f));
            nullstate->SetBoundingBox(bbox);
        }

        mesh = engine->CreateMeshWrapper(nullfact, name);
    }
    else
    {
        // Replace helm group token with the default race.
        csString fact_name(factname);
        fact_name.ReplaceAll("$H", "stonebm");
        fact_name.ReplaceAll("$B", "stonebm");
        fact_name.ReplaceAll("$E", "stonebm");
        fact_name.ReplaceAll("$C", "stonebm");
        factname = fact_name;

        csRef<iBgLoader> loader = csQueryRegistry<iBgLoader> (psNPCClient::npcclient->GetObjectReg());
        factory = loader->LoadFactory(factname, true);

        if(!(factory.IsValid() && factory->IsFinished() && factory->WasSuccessful()))
        {
            factory = loader->LoadFactory("stonebm", true);
        }

        if(factory.IsValid() && factory->IsFinished() && factory->WasSuccessful())
        {
            csRef<iMeshFactoryWrapper> meshFact = scfQueryInterface<iMeshFactoryWrapper>(factory->GetResultRefPtr());
            mesh = meshFact->CreateMeshWrapper();
        }
        else
        {
            Error2("Could neither load factory nor use dummy CVS mesh with factname=%s", factname);
            return false;
        }
    }

    if(!pcmesh->GetMesh())
    {
        pcmesh->SetMesh(mesh);
    }


    csRef<iSpriteCal3DFactoryState> sprite = scfQueryInterface<iSpriteCal3DFactoryState> (mesh->GetFactory()->GetMeshObjectFactory());
    if(sprite)
    {
        baseScale = sprite->GetScaleFactor();

        // Normalize the mesh scale to the base scale of the mesh.
        Debug5(LOG_CELPERSIST,0,"DEBUG: %s Normalize scale: %f / %f = %f\n",
               factname, scale, baseScale, scale / baseScale);
    }


    Move(pos,rotangle,room);

    return true;
}

void gemNPCObject::FiniMesh()
{
    // Don't wait for static destructor to be called.
    nullfact.Invalidate();
}

void gemNPCObject::SetAlive(bool alive)
{
    isAlive = alive;
}


void gemNPCObject::SetPosition(csVector3 &pos, iSector* sector, InstanceID* instance)
{
    psGameObject::SetPosition(this, pos, sector);

    if(instance)
    {
        SetInstance(*instance);
    }
}

iMeshWrapper* gemNPCObject::GetMeshWrapper()
{
    return pcmesh->GetMesh();
}

//-------------------------------------------------------------------------------


gemNPCActor::gemNPCActor(psNPCClient* npcclient, psPersistActor &mesg)
    : gemNPCObject(npcclient, mesg.entityid), pcmove(NULL), npc(NULL), withinTribe(NULL)
{
    name = mesg.name;
    type = mesg.type;
    playerID = mesg.playerID;
    ownerEID = mesg.ownerEID;
    race = mesg.race;
    scale = mesg.scale;

    SetInvisible((mesg.flags & psPersistActor::INVISIBLE)?  true : false);
    SetInvincible((mesg.flags & psPersistActor::INVINCIBLE) ?  true : false);
    SetAlive((mesg.flags & psPersistActor::IS_ALIVE) ?  true : false);
    SetInstance(mesg.instance);

    Debug3(LOG_CELPERSIST, eid.Unbox(), "Actor %s(%s) Received\n", mesg.name.GetData(), ShowID(mesg.entityid));

    csString filename;
    filename.Format("/planeshift/models/%s/%s.cal3d", mesg.factname.GetData(), mesg.factname.GetData());
    InitMesh(mesg.factname, filename, mesg.pos, mesg.yrot, mesg.sectorName);
    InitLinMove(mesg.pos, mesg.yrot, mesg.sectorName, mesg.top, mesg.bottom, mesg.offset);
    InitCharData(mesg.texParts, mesg.equipment);
}

gemNPCActor::~gemNPCActor()
{
    if(npc)
    {
        npc->ClearState();
        npc->SetActor(NULL);
        npc = NULL;
    }
    if(pcmove)
    {
        delete pcmove;
        pcmove = NULL;
    }
}

void gemNPCActor::AttachNPC(NPC* newNPC)
{
    npc = newNPC;
    npc->SetActor(this);
    npc->SetAlive(isAlive);
}

bool gemNPCActor::SetWithinTribe(Tribe* tribe, Tribe** oldTribe)
{
    // Check if this is a new tribe
    bool result = (tribe != withinTribe);

    if(oldTribe)
    {
        *oldTribe = withinTribe;
    }

    // Update the within tribe
    withinTribe = tribe;

    return result;
}

double gemNPCActor::GetProperty(MathEnvironment* env, const char* ptr)
{
    csString property(ptr);
    if(property == "HP")
    {
        return GetHP();
    }
    if(property == "MaxHP")
    {
        return GetMaxHP();
    }
    if(property == "Mana")
    {
        return GetMana();
    }
    if(property == "MaxMana")
    {
        return GetMaxMana();
    }
    if(property == "PStamina")
    {
        return GetPysStamina();
    }
    if(property == "MaxPStamina")
    {
        return GetMaxPysStamina();
    }
    if(property == "MStamina")
    {
        return GetMenStamina();
    }
    if(property == "MaxMStamina")
    {
        return GetMaxMenStamina();
    }

    Error2("Requested gemNPCActor property not found '%s'", ptr);
    return 0.0;
}

double gemNPCActor::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    csString function(functionName);

    Error2("Requested gemNPCActor function not found '%s'", functionName);
    return 0.0;
}

const char* gemNPCActor::ToString()
{
    return "Actor";
}

bool gemNPCActor::InitCharData(const char* textParts, const char* equipment)
{
    return true;
}

bool gemNPCActor::InitLinMove(const csVector3 &pos, float angle, const char* sector,
                              csVector3 top, csVector3 bottom, csVector3 offset)
{
    pcmove =  new psLinearMovement(psNPCClient::npcclient->GetObjectReg());

    csRef<iEngine> engine =  csQueryRegistry<iEngine> (psNPCClient::npcclient->GetObjectReg());

    pcmove->InitCD(top, bottom, offset, GetMeshWrapper());
    pcmove->SetPosition(pos,angle,engine->FindSector(sector));

    pcmove->SetScale(scale/baseScale);

    return true;  // right now this func never fail, but might later.
}


void gemNPCActor::SetHP(float hp)
{
    this->hp.SetValue(hp, csGetTicks());
}

float gemNPCActor::GetHP()
{
    hp.Update(csGetTicks());
    return hp.GetValue();
}

void gemNPCActor::SetMaxHP(float maxHP)
{
    hp.SetMax(maxHP);
}

float gemNPCActor::GetMaxHP() const
{
    return hp.GetMax();
}

void gemNPCActor::SetHPRate(float hpRate)
{
    hp.SetRate(hpRate, csGetTicks());
}

float gemNPCActor::GetHPRate() const
{
    return hp.GetRate();
}

void gemNPCActor::SetMana(float mana)
{
    this->mana.SetValue(mana, csGetTicks());
}

float gemNPCActor::GetMana()
{
    mana.Update(csGetTicks());
    return mana.GetValue();
}

void gemNPCActor::SetMaxMana(float maxMana)
{
    mana.SetMax(maxMana);
}

float gemNPCActor::GetMaxMana() const
{
    return mana.GetMax();
}

void gemNPCActor::SetManaRate(float manaRate)
{
    mana.SetRate(manaRate, csGetTicks());
}

float gemNPCActor::GetManaRate() const
{
    return mana.GetRate();
}

void gemNPCActor::SetPysStamina(float pysStamina)
{
    this->pysStamina.SetValue(pysStamina, csGetTicks());
}

float gemNPCActor::GetPysStamina()
{
    pysStamina.Update(csGetTicks());
    return pysStamina.GetValue();
}

void gemNPCActor::SetMaxPysStamina(float maxPysStamina)
{
    pysStamina.SetMax(maxPysStamina);
}

float gemNPCActor::GetMaxPysStamina() const
{
    return pysStamina.GetMax();
}

void gemNPCActor::SetPysStaminaRate(float pysStaminaRate)
{
    pysStamina.SetRate(pysStaminaRate, csGetTicks());
}

float gemNPCActor::GetPysStaminaRate() const
{
    return pysStamina.GetRate();
}

void gemNPCActor::SetMenStamina(float menStamina)
{
    this->menStamina.SetValue(menStamina, csGetTicks());
}

float gemNPCActor::GetMenStamina()
{
    menStamina.Update(csGetTicks());
    return menStamina.GetValue();
}

void gemNPCActor::SetMaxMenStamina(float maxMenStamina)
{
    menStamina.SetMax(maxMenStamina);
}

float gemNPCActor::GetMaxMenStamina() const
{
    return menStamina.GetMax();
}

void gemNPCActor::SetMenStaminaRate(float menStaminaRate)
{
    menStamina.SetRate(menStaminaRate, csGetTicks());
}

float gemNPCActor::GetMenStaminaRate() const
{
    return menStamina.GetRate();
}


gemNPCItem::gemNPCItem(psNPCClient* npcclient, psPersistItem &mesg)
    : gemNPCObject(npcclient, mesg.eid), flags(NONE)
{
    name = mesg.name;
    Debug3(LOG_CELPERSIST, 0, "Item %s(%s) Received", mesg.name.GetData(), ShowID(mesg.eid));
    type = mesg.type;
    uid = mesg.uid;
    tribeID = mesg.tribeID;

    if(!mesg.factname.GetData())
    {
        Error2("Item %s has bad data! Check cstr_gfx_mesh for this item!\n", mesg.name.GetData());
    }

    InitMesh(mesg.factname.GetDataSafe(), "", mesg.pos, mesg.yRot, mesg.sector);
    if(mesg.flags & psPersistItem::NOPICKUP) flags |= NOPICKUP;
}

gemNPCItem::~gemNPCItem()
{
}

//Here we check the flag to see if we can pick up this item
bool gemNPCItem::IsPickable()
{
    return !(flags & NOPICKUP);
}

uint32_t gemNPCItem::GetUID() const
{
    return uid;
}

uint32_t gemNPCItem::GetTribeID() const
{
    return tribeID;
}


