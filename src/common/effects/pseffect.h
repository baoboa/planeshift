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

#ifndef PS_EFFECT_HEADER
#define PS_EFFECT_HEADER

#include <iutil/document.h>
#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csgeom/matrix3.h>
#include <iengine/movable.h>
#include <csutil/ref.h>
#include <csutil/scf.h>
#include <iutil/virtclk.h>
#include <iengine/movable.h>
#include <iengine/objwatch.h>
#include <csutil/parray.h>

struct iDocumentNode;
struct iLoaderContext;
struct iView;

class psEffectAnchor;
class psEffectObj;
class psEffect2DRenderer;
class psEffectObjTextable;

/**
 * \addtogroup common_effects
 * @{ */

/**
 * The effect class.
 */
class psEffect
{
public:
    /**
     * Constructor.
     */
    psEffect();

    /**
     * Destructor.
     */
    ~psEffect();

    /**
     * Creates an effect anchor given a type string.
     *
     * @param type a string identifying the type of the anchor.
     *          Possible types are:
     *          <ul>
     *            <li>basic
     *            <li>spline
     *            <li>socket
     *          </ul>
     * @return the effect anchor if successfull, 0 otherwise.
     */
    psEffectAnchor* CreateAnchor(const csString &type);

    /**
     * Adds an effect anchor to this effect.
     *
     * @param anchor a valid effect anchor.
     * @return the index of the effect anchor; Can be accessed with GetAnchor(index)
     */
    size_t AddAnchor(psEffectAnchor* anchor);

    /**
     * Loads the effect from an xml node.
     *
     * @param node an xml node containing the effect, must be valid.
     * @param parentView the CS viewport that views the effect.
     * @param renderer2d the 2D renderer.
     * @param ldr_context the current loader context.
     * @return true on success, false otherwise.
     */
    bool Load(iDocumentNode* node, iView* parentView, psEffect2DRenderer* renderer2d,
              iLoaderContext* ldr_context);

    /**
     * Renders the effect.
     *
     * @param sectors the starting sectors of the effect
     * @param offset the offset of the position of the effect
     * @param attachPos the object to attach the effect to, 0 for 0,0,0
     * @param attachTarget the target of the effect
     * @param up the base up vector of the effect
     * @param uniqueIDOverride overrides the unique ID of the effect (for things like group effects)
     * @param rotateWithMesh Should the effect rotate with the mesh.
     * @return 0 on failure, a unique ID on success
     */
    unsigned int Render(iSectorList* sectors, const csVector3 &offset, iMeshWrapper* attachPos,
                        iMeshWrapper* attachTarget, const csVector3 &up, const unsigned int uniqueIDOverride = 0,
                        bool rotateWithMesh = false);

    /**
     * Renders the effect.
     *
     * @param sector the starting sector of the effect
     * @param offset the offset of the position of the effect
     * @param attachPos the object to attoch the effect to, 0 for 0,0,0
     * @param attachTarget the target of the effect, 0 for
     * @param up the base up vector of the effect
     * @param uniqueIDOverride overrides the unique ID of the effect (for things like group effects)
     * @return 0 on failure, a unique ID on success
     */
    unsigned int Render(iSector* sector, const csVector3 &offset, iMeshWrapper* attachPos,
                        iMeshWrapper* attachTarget, const csVector3 &up, const unsigned int uniqueIDOverride = 0);

    /**
     * Updates the spell effect -- called every frame.
     *
     * @param elapsed the ticks elapsed since last update
     * @return false if the obj is useless and can be removed
     */
    bool Update(csTicks elapsed);

    /**
     * Clones the effect.
     */
    psEffect* Clone() const;

    /**
     * Returns the uniqueID of this effect
     *
     * @return the uniqueID of the effect
     */
    unsigned int GetUniqueID() const;

    /**
     * Sets the time after which this effect will be killed.
     *
     * @param newKillTime new time in ms
     * @note the actual kill time may still vary due to fixed times in effects
     */
    void SetKillTime(const int newKillTime);

    /**
     * Returns the time after which this effect will be killed.
     */
    int GetKillTime() const;

    /**
     * Set the scalings for each frame parameter.
     */
    bool SetFrameParamScalings(const float* scale);

    /**
     * Returns the animation length of the effect.
     *
     * @return the animation length (max anim length of effect objs)
     */
    float GetAnimLength() const;

    /**
     * Returns the number of effect anchors in this effect.
     *
     * @return the effect anchor count
     */
    size_t GetAnchorCount() const;

    /**
     * Returns the effect anchor at the given index.
     *
     * @param idx the index of the anchor to grab
     * @return the effect anchor at the given index
     */
    psEffectAnchor* GetAnchor(size_t idx) const;

    /**
     * Finds an effect anchor of the given name.
     *
     * @param anchorName the name of the anchor to find
     * @return the anchor if it's found, 0 otherwise
     */
    psEffectAnchor* FindAnchor(const csString &anchorName) const;

    /**
     * Returns the number of effect objs in this effect.
     *
     * @return the effect obj count
     */
    size_t GetObjCount() const;

    /**
     * Returns the effect obj at the given index.
     *
     * @param idx the index of the obj to grab
     * @return the effect obj at the given index
     */
    psEffectObj* GetObj(size_t idx) const;

    /**
     * Finds an effect obj of the given name.
     *
     * @param objName the name of the obj to find
     * @return the obj if it's found, 0 otherwise
     */
    psEffectObj* FindObj(const csString &objName) const;

    /**
     * Gets the name of the effect.
     *
     * @return the name of the effect
     */
    const csString &GetName() const;

    /**
     * @todo comment this
     */
    psEffectObjTextable* GetMainTextObj() const;

    /**
     * Returns visibility value.
     */
    bool IsVisible();

    /**
     * Shows the effect.
     */
    void Show();

    /**
     * Hides the effect.
     */
    void Hide();

    /**
     * Some effects use these scaling parameters to scale the object.
     */
    void SetScaling(float scale, float aspect);

private:
    /**
     * This makes the anchoring to an iMovable possible by being informed of every change to the iMovable.
     */
    class psEffectMovableListener : public scfImplementation1<psEffectMovableListener,iMovableListener>
    {
    private:
        bool movableDead;
        bool movableChanged;
        iMovable* movable;

    public:
        psEffectMovableListener()
            : scfImplementationType(this)
        {
            movableDead = false;
            movableChanged = false;
            movable = 0;
        }

        virtual ~psEffectMovableListener() {}

        /**
         * Implementation of the iMovableListener function, allows us to detect the movements of the movable.
         */
        virtual void MovableChanged(iMovable* movable)
        {
            movableChanged = true;
            this->movable = movable;
        }

        /**
         * Implementation of the iMovableListener function, allows us to detect if the movable was destroyed.
         */
        virtual void MovableDestroyed(iMovable* /*movable*/)
        {
            movableDead = true;
            this->movable = 0;
            movableChanged = false;
        }

        /**
         * Checks to see if the movable has disappeared.
         *
         * @return true if the movable is still alive, false otherwise
         */
        bool IsMovableDead() const
        {
            return movableDead;
        }

        /**
         * Gets the new movable data if there is any.
         *
         * @param newSectors a container to store all the sectors that the movable is in
         * @param newPos a container to store the new position of the movable
         * @param newTransf a container to store the new transform of the movable
         * @return true if the movable has changed since this function was called last
         */
        bool GrabNewData(iSectorList* &newSectors, csVector3 &newPos, csMatrix3 &newTransf)
        {
            if(!movableChanged || !movable)
                return false;

            newPos = movable->GetFullPosition();
            newSectors = movable->GetSectors();
            newTransf = movable->GetFullTransform().GetT2O();

            movableChanged = false;
            return true;
        }

        /**
         * Gets the new movable data if there is any.
         *
         * @param newPos a container to store the new position of the movable
         * @param newTransf a container to store the new transform of the movable
         * @return true if the movable has changed since this function was called last
         */
        bool GrabNewData(csVector3 &newPos, csMatrix3 &newTransf)
        {
            if(!movableChanged || !movable)
                return false;

            newPos = movable->GetFullPosition();
            newTransf = movable->GetFullTransform().GetT2O();
            movableChanged = false;
            return true;
        }

        csVector3 GetPosition()
        {
            if(movable)
                return movable->GetFullPosition();
            else
                return csVector3(0, 0, 0);
        }
    };

    csRef<psEffectMovableListener> positionListener;
    csRef<psEffectMovableListener> targetListener;

    bool visible;
    unsigned int uniqueID;
    csString name;

    csPDelArray<psEffectAnchor> effectAnchors;
    csPDelArray<psEffectObj> effectObjs;

    size_t mainTextObj;
    psEffect2DRenderer* renderer2d;
};

/** @} */

#endif
