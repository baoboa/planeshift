/*
 * Author: Andrew Robberts
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

#ifndef PS_EFFECT_MANAGER_HEADER
#define PS_EFFECT_MANAGER_HEADER


// CS includes
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csutil/parray.h>
#include <csutil/ref.h>
#include <iengine/collection.h>
#include <iutil/virtclk.h>
#include <csutil/hash.h>
#include <imap/reader.h>
#include <ivideo/graph3d.h>
#include <csutil/scf_implementation.h>
#include <csutil/threading/rwmutex.h>

#include "effects/pseffect2drenderer.h"

struct iLight;
struct iMeshWrapper;
struct iMovable;
struct iSector;
struct iSectorList;
struct iThreadReturn;
struct iView;
class psEffect;
class psLight;

class psEffectManager;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectGroup
{
public:
    csString name;
    csArray<csString> effects;
};


/**
 * Loader plugin for loading PS effects.
 */
class psEffectLoader : public scfImplementation1<psEffectLoader,iLoaderPlugin>
{
public:
    psEffectLoader();

    /// Sets the effect manager
    void SetManager(psEffectManager* manager);

    // used to handle and parse the <addon> tags.
    csPtr<iBase> Parse(iDocumentNode* node, iStreamSource*, iLoaderContext* ldr_context, iBase* context);

    bool IsThreadSafe()
    {
        return true;
    }

private:
    psEffectManager* manager;
    CS::Threading::ReadWriteMutex parseLock;
};


class psEffectManager : public csRefCount
{
public:
    psEffectManager(iObjectRegistry* objReg);
    virtual ~psEffectManager();

    /**
     * Loads one or more gfx effects from a file.
     *
     * @param fileName the vfs path to the file that holds the spell effect
     * @param parentView the CS viewport that views the effects
     * @return true on success, false otherwise
     */
    csPtr<iThreadReturn> LoadEffects(const csString &fileName, iView* parentView);

    /**
     * Loads the effect files listed in the given effects list.
     *
     * @param fileName the vfs path to the file that holds the effects list
     * @param parentView the CS viewport that views the effects
     * @return true on success, false otherwise
     */
    bool LoadFromEffectsList(const csString &fileName, iView* parentView);

    /**
     * Loads all effect files that can be found in a directory path.
     *
     * @param path            The vfs path to search.
     * @param includeSubDirs  If True, it will extend its search into subdirectories.
     * @param parentView      The CS viewport that views the effects.
     * @return                True on success, false otherwise.
     */
    bool LoadFromDirectory(const csString &path, bool includeSubDirs, iView* parentView);

    /**
     * Deletes an effect.
     *
     * @param effectID the unique ID of the effect (as returned by RenderEffect())
     * @return true if the effect manager no longer contains an effect by that ID, false if there was a problem
     */
    bool DeleteEffect(unsigned int effectID);

    /**
     * Begins rendering of an effect that is attached to an iMeshWrapper.
     *
     * @param effectName the name of the effect to render
     * @param offset the position offset of the effect from the attached object
     * @param attachPos the object to attach the effect to
     * @param attachTarget the target of the effect, 0 assumes that the target is the same as the attachPos
     * @param up the base up vector of the effect
     * @param uniqueIDOverride overrides the unique ID of the effect (for things like group effects)
     * @param rotateWithMesh Rotate with the mesh.
     * @param scale input to the scale params for effects
     * @return 0 on failure, a unique ID of the effect otherwise
     */
    unsigned int RenderEffect(const csString &effectName, const csVector3 &offset, iMeshWrapper* attachPos,
                              iMeshWrapper* attachTarget=0, const csVector3 &up=csVector3(0,1,0),
                              const unsigned int uniqueIDOverride = 0, bool rotateWithMesh = false, const float* scale = NULL);

    /**
     * Begins rendering an effect that isn't attached to anything.
     *
     * @param effectName the name of the effect to render
     * @param sector the starting sector of the new effect
     * @param pos the position of the new effect
     * @param attachTarget the target of the effect, 0 assumes that the target is the same as the offset
     * @param up the base up vector of the effect
     * @param uniqueIDOverride overrides the unique ID of the effect (for things like group effects)
     * @param scale input to the scale params for effects
     * @return 0 on failure, a unique ID of the effect otherwise
     */
    unsigned int RenderEffect(const csString &effectName, iSector* sector, const csVector3 &pos,
                              iMeshWrapper* attachTarget, const csVector3 &up=csVector3(0,1,0),
                              const unsigned int uniqueIDOverride = 0, const float* scale = NULL);

    /**
     * Begins rendering an effect that isn't attached to anything.
     *
     * @param effectName the name of the effect to render
     * @param sectors the sectors the new effect is in
     * @param pos the position of the new effect
     * @param attachTarget the target of the effect, 0 assumes that the target is the same as the offset
     * @param up the base up vector of the effect
     * @param uniqueIDOverride overrides the unique ID of the effect (for things like group effects)
     * @param scale input to the scale params for effects
     * @return 0 on failure, a unique ID of the effect otherwise
     */
    unsigned int RenderEffect(const csString &effectName, iSectorList* sectors, const csVector3 &pos,
                              iMeshWrapper* attachTarget=0, const csVector3 &up=csVector3(0,1,0),
                              const unsigned int uniqueIDOverride = 0, const float* scale = NULL);

    unsigned int AttachLight(const char* name, const csVector3 &pos,
                             float radius, const csColor &colour, iMeshWrapper* mw);
    void DetachLight(unsigned int lightID);

    /**
     * Updates the spell effects (should be called every frame).
     *
     * @param elapsed the time in ms that has elapsed
     */
    void Update(csTicks elapsed = 0);

    /**
     * Clears all effects.
     */
    void Clear();

    /**
     * Gets the effect with the given ID.
     *
     * @param ID the id of the effect (the value returned by RenderEffect)
     * @return a reference to the effect, 0 if the effect could not be found
     */
    psEffect* FindEffect(unsigned int ID) const;

    /**
     * Finds an effect by the given name.
     *
     * @param name the name of the effect to find
     * @return the effect if it found it, null if not
     */
    psEffect* FindEffect(const csString &name) const;

    /**
     * Gets a global iterator that will iterate over all effect factories.
     *
     * @return the iterator.
     */
    csHash<psEffect*, csString>::GlobalIterator GetEffectsIterator();


    /**
     * Hide or Show the effect.
     *
     * @param id effect ID
     * @param value show or hide
     */
    void ShowEffect(unsigned int id,bool value = true);

    void AddEffect(const char* name, psEffect* effect);

    iView* GetView() const
    {
        return view;
    }

    void Render2D(iGraphics3D* g3d, iGraphics2D* g2d);

    psEffect2DRenderer* Get2DRenderer() const
    {
        return effect2DRenderer;
    }

private:

    iObjectRegistry* object_reg;

    /// Virtual clock to keep track of the ticks passed between frames.
    csRef<iVirtualClock> vc;

    /**
     * The collection of effects that don't do anything themselves, they're just ready to be quickly copied when a
     * new effect is needed.
     */
    csHash<psEffect*, csString> effectFactories;

    /// the actual effects that are seen
    csHash<psEffect*, unsigned int> actualEffects;

    /// Effects are stored in a collection to make them easier to manage.
    csRef<iCollection> effectsCollection;

    iView* view;

    /// Loader plugin for loading PS effects. We keep it here for unregistering in the destructor.
    csRef<psEffectLoader> effectLoader;

    psEffect2DRenderer* effect2DRenderer;

    csHash<psLight*, unsigned int> lightList;
};

/** @} */

#endif
