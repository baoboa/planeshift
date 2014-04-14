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
#ifndef PS_GEM_HEADER
#define PS_GEM_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/mesh.h>
#include <csutil/csobject.h>

//=============================================================================
// Project Library Includes
//=============================================================================
#include "net/messages.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "npcclient.h"
#include "stat.h"

/**
 * \addtogroup npcclient
 * @{ */

class gemNPCActor;
class gemNPCObject;
class npcMesh;


//-----------------------------------------------------------------------------

/** Helper class to attach a PlaneShift npc gem object to a particular mesh.
  */
class psNpcMeshAttach : public scfImplementationExt1<psNpcMeshAttach,
    csObject,
    scfFakeInterface<psNpcMeshAttach> >
{
public:
    SCF_INTERFACE(psNpcMeshAttach, 0, 0, 1);

    /** Setup this helper with the object we want to attach with.
     * @param object  The npcObject we want to attach to a mesh.
     */
    psNpcMeshAttach(gemNPCObject* object);

    /** Get the npc Object that the mesh has attached.
     */
    gemNPCObject* GetObject()
    {
        return object;
    }

private:
    gemNPCObject* object;          ///< The object that is attached to a iMeshWrapper object.
};

//-----------------------------------------------------------------------------



class gemNPCObject : public CS::Utility::WeakReferenced
{
public:
    gemNPCObject(psNPCClient* npcclient, EID id);
    virtual ~gemNPCObject();

    bool InitMesh(const char* factname,const char* filename,
                  const csVector3 &pos,const float rotangle, const char* sector);
    static void FiniMesh();

    iMeshWrapper* GetMeshWrapper();
    void Move(const csVector3 &pos, float rotangle, const char* room);
    void Move(const csVector3 &pos, float rotangle, const char* room, InstanceID instance);

    EID GetEID()
    {
        return eid;
    }

    int GetType()
    {
        return type;
    }

    const char* GetName()
    {
        return name.GetDataSafe();
    }
    virtual PID GetPID()
    {
        return PID(0);
    }

    virtual const char* GetObjectType()
    {
        return "Object";
    }
    virtual gemNPCActor* GetActorPtr()
    {
        return NULL;
    }

    virtual bool IsPickable()
    {
        return false;
    }
    virtual bool IsVisible()
    {
        return visible;
    }
    virtual bool IsInvisible()
    {
        return !visible;
    }
    virtual void SetVisible(bool vis)
    {
        visible = vis;
    }
    virtual void SetInvisible(bool invis)
    {
        SetVisible(!invis);
    }

    virtual bool IsInvincible()
    {
        return invincible;
    }
    virtual void SetInvincible(bool inv)
    {
        invincible = inv;
    }

    virtual bool IsAlive()
    {
        return isAlive;
    }
    virtual void SetAlive(bool alive);

    virtual NPC* GetNPC()
    {
        return NULL;
    }

    virtual void SetPosition(csVector3 &pos, iSector* sector = NULL, InstanceID* instance = NULL);
    virtual void SetInstance(InstanceID instance)
    {
        this->instance = instance;
    }
    virtual InstanceID GetInstance()
    {
        return instance;
    };

    npcMesh* pcmesh;

protected:
    static csRef<iMeshFactoryWrapper> nullfact;

    csString name;
    EID  eid;
    int  type;
    bool visible;
    bool invincible;
    bool isAlive;
    float scale;
    float baseScale;

    InstanceID  instance;

    csRef<iThreadReturn> factory;
};


class gemNPCActor : public gemNPCObject, public iScriptableVar
{
public:

    gemNPCActor(psNPCClient* npcclient, psPersistActor &mesg);
    virtual ~gemNPCActor();

    psLinearMovement* pcmove;

    virtual PID GetPID()
    {
        return playerID;
    }
    virtual EID GetOwnerEID()
    {
        return ownerEID;
    }

    csString &GetRace()
    {
        return race;
    };

    virtual const char* GetObjectType()
    {
        return "Actor";
    }
    virtual gemNPCActor* GetActorPtr()
    {
        return this;
    }

    virtual void AttachNPC(NPC* newNPC);
    virtual NPC* GetNPC()
    {
        return npc;
    }


    /**
     * Set the actor HP
     */
    void SetHP(float hp);

    /**
     * Get the actor HP
     */
    float GetHP();

    /**
     * Set the actor MaxHP
     */
    void SetMaxHP(float maxHP);

    /**
     * Get the actor MaxHP
     */
    float GetMaxHP() const;

    /**
     * Set the actor HP
     */
    void SetHPRate(float hpRate);

    /**
     * Get the actor HPRate
     */
    float GetHPRate() const;

    /**
     * Set the actor Mana
     */
    void SetMana(float mana);

    /**
     * Get the actor Mana
     */
    float GetMana();

    /**
     * Set the actor MaxMana
     */
    void SetMaxMana(float maxMana);

    /**
     * Get the actor MaxMana
     */
    float GetMaxMana() const;

    /**
     * Set the actor ManaRate
     */
    void SetManaRate(float manaRate);

    /**
     * Get the actor ManaRate
     */
    float GetManaRate() const;

    /**
     * Set the actor PysStamina
     */
    void SetPysStamina(float pysStamina);

    /**
     * Get the actor PysStamina
     */
    float GetPysStamina();

    /**
     * Set the actor MaxPysStamina
     */
    void SetMaxPysStamina(float maxPysStamina);

    /**
     * Get the actor MaxPysStamina
     */
    float GetMaxPysStamina() const;

    /**
     * Set the actor PysStaminaRate
     */
    void SetPysStaminaRate(float pysStaminaRate);

    /**
     * Get the actor PysStaminaRate
     */
    float GetPysStaminaRate() const;

    /**
     * Set the actor MenStamina
     */
    void SetMenStamina(float menStamina);

    /**
     * Get the actor MenStamina
     */
    float GetMenStamina();

    /**
     * Set the actor MaxMenStamina
     */
    void SetMaxMenStamina(float maxMenStamina);

    /**
     * Get the actor MaxMenStamina
     */
    float GetMaxMenStamina() const;

    /**
     * Set the actor MenStaminaRate
     */
    void SetMenStaminaRate(float menStaminaRate);

    /**
     * Get the actor MenStaminaRate
     */
    float GetMenStaminaRate() const;

    /** Set the within tribe.
     * @return true if a new within tribe is set
     */
    virtual bool SetWithinTribe(Tribe* tribe,Tribe** oldTribe = NULL);

private:
    /** @name iScriptableVar implementation
     * Functions that implement the iScriptableVar interface.
     */
    ///@{
    virtual double GetProperty(MathEnvironment* env, const char* ptr);
    virtual double CalcFunction(MathEnvironment* env, const char* functionName, const double* params);
    virtual const char* ToString();
    ///@}

protected:

    bool InitLinMove(const csVector3 &pos,float angle, const char* sector,
                     csVector3 top, csVector3 bottom, csVector3 offset);

    bool InitCharData(const char* textures, const char* equipment);

    PID playerID;
    EID ownerEID;
    csString race;

    NPC* npc;

    Tribe* withinTribe; /// Points to the tribe home where this tribe is inside.

    // Stats
    Stat               hp;
    Stat               mana;
    Stat               pysStamina;
    Stat               menStamina;
};


class gemNPCItem : public gemNPCObject
{
public:
    enum Flags
    {
        NONE           = 0,
        NOPICKUP       = 1 << 0
    };

    gemNPCItem(psNPCClient* npcclient, psPersistItem &mesg);
    virtual ~gemNPCItem();

    virtual const char* GetObjectType()
    {
        return "Item";
    }

    virtual bool IsPickable();

    uint32_t GetUID() const;
    uint32_t GetTribeID() const;

protected:
    uint32_t uid;
    uint32_t tribeID;
    int flags;
};

/** @} */

#endif
