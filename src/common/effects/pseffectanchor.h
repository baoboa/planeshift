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

#ifndef PS_EFFECT_ANCHOR_HEADER
#define PS_EFFECT_ANCHOR_HEADER

#include <csutil/csstring.h>
#include <csutil/array.h>
#include <csutil/bitarray.h>
#include <csutil/leakguard.h>
#include <csutil/parray.h>
#include <csutil/refcount.h>
#include <iutil/virtclk.h>
#include <imesh/object.h>

struct iDocumentNode;
struct iEngine;
struct iView;
struct iMeshFactoryWrapper;
struct iSector;
struct iSectorList;

/**
 * \addtogroup common_effects
 * @{ */

/**
 * Stores data for a specific effect anchor keyframe
 */
class psEffectAnchorKeyFrame
{
public:
    psEffectAnchorKeyFrame();
    psEffectAnchorKeyFrame(iDocumentNode* node, const psEffectAnchorKeyFrame* prevKeyFrame);
    ~psEffectAnchorKeyFrame();

    /**
     * Applies default values to this anchor keyframe.
     */
    void SetDefaults();

    /**
     * Sets up this keyframe for the special case of being the first key frame in the group.
     */
    void SetupFirstFrame();

    /// this is the time of the keyframe animation (in seconds)
    float time;

    enum INTERP_TYPE
    {
        IT_NONE = 0,
        IT_FLOOR,
        IT_CEILING,
        IT_LERP,

        IT_COUNT
    };

    enum KEY_ACTION
    {
        KA_POS_X = 1, KA_POS_Y, KA_POS_Z,
        KA_TOTARGET_X, KA_TOTARGET_Y, KA_TOTARGET_Z,

        KA_COUNT
    };
    float actions[KA_COUNT];

    inline const char* GetActionName(size_t idx)
    {
        switch(idx)
        {
            case KA_POS_X:
                return "Position X";
            case KA_POS_Y:
                return "Position Y";
            case KA_POS_Z:
                return "Position Z";
            case KA_TOTARGET_X:
                return "To Target X";
            case KA_TOTARGET_Y:
                return "To Target Y";
            case KA_TOTARGET_Z:
                return "To Target Z";
        }
        return "";
    }

    inline bool IsActionSet(size_t idx)
    {
        return specAction.IsBitSet(idx);
    }

    /// keep track of which actions were specified for which
    csBitArray specAction;
};


/**
 * Effect anchor KeyFrame group.
 */
class psEffectAnchorKeyFrameGroup : public csRefCount
{
private:
    csPDelArray<psEffectAnchorKeyFrame> keyFrames;

public:
    psEffectAnchorKeyFrameGroup();
    ~psEffectAnchorKeyFrameGroup();

    /**
     * Returns the number of keyframes in the group.
     *
     * @return the keyframe count
     */
    size_t GetSize() const
    {
        return keyFrames.GetSize();
    }

    /**
     * Returns the keyframe at the given index.
     *
     * @param idx the index of the keyframe to grab
     * @return the keyframe
     */
    psEffectAnchorKeyFrame* Get(size_t idx) const
    {
        return keyFrames[idx];
    }

    /**
     * Returns the keyframe at the given index.
     *
     * @param idx the index of the keyframe to grab
     * @return the keyframe
     */
    psEffectAnchorKeyFrame* operator [](size_t idx) const
    {
        return keyFrames[idx];
    }

    /**
     * Pushes a keyframe onto the group.
     *
     * @param keyFrame the keyframe to push
     */
    void Push(psEffectAnchorKeyFrame* keyFrame)
    {
        keyFrames.Push(keyFrame);
    }

    /**
     * Deletes the keyframe at the given index.
     *
     * @param idx the index of the keyframe to delete
     */
    void DeleteIndex(size_t idx)
    {
        keyFrames.DeleteIndex(idx);
    }

    /**
     * Deletes all of the keyframes in this group.
     */
    void DeleteAll()
    {
        keyFrames.DeleteAll();
    }
};

/**
 * Effect anchors provide a base location / anchor point for all effect objs.
 */
class psEffectAnchor
{
public:
    psEffectAnchor();
    virtual ~psEffectAnchor();

    /**
     * Loads the effect anchors from an xml node.
     *
     * @param node the xml node containing the effect anchor, must be valid.
     * @return true on success, false otherwise.
     */
    virtual bool Load(iDocumentNode* node);

    /**
     * Creates the effect anchor.
     *
     * @param offset the offset of the position of the obj.
     * @param posAttach the mesh to attach this anchor to.
     * @param rotateWithMesh Rotate the anchor with the mesh.
     * @return true on success.
     */
    virtual bool Create(const csVector3 &offset, iMeshWrapper* posAttach, bool rotateWithMesh = false);

    /**
     * Updates the effect anchor -- called every frame.
     *
     * @param elapsed the ticks elapsed since last update.
     * @return false if the anchor is useless and can be removed by its parent effect.
     */
    virtual bool Update(csTicks elapsed);

    /**
     * Convenience function to clone the base member variables.
     *
     * @param newAnchor reference to the new anchor that will contain the cloned variables
     */
    void CloneBase(psEffectAnchor* newAnchor) const;

    /**
     * Clones the effect anchor.  This will almost always be overloaded.
     */
    virtual psEffectAnchor* Clone() const;

    /**
     * Sets a new position for the effect anchor.
     *
     * @param basePos The new position of the anchor.
     * @param sector The new sector of the anchor
     * @param transf The transform of the position.
     */
    virtual void SetPosition(const csVector3 &basePos, iSector* sector, const csMatrix3 &transf);

    /**
     * Sets a new position for the effect anchor.
     *
     * @param basePos The new position of the anchor.
     * @param sectors The new sectors of the anchor.
     * @param transf The transform of the position.
     */
    virtual void SetPosition(const csVector3 &basePos, iSectorList* sectors, const csMatrix3 &transf);

    /**
     * Sets the target of the effect anchor.
     *
     * @param newTarget The new target.
     * @param transf The transform of the target.
     */
    void SetTarget(const csVector3 &newTarget, const csMatrix3 &transf)
    {
        target = newTarget;
        targetTransf = transf;
    }

    /**
     * Returns the name of this effect anchor.
     *
     * @return The name of this effect anchor.
     */
    const csString &GetName() const
    {
        return name;
    }

    /**
     * Sets the name of this effect anchor.
     *
     * @param newName The new name of the anchor.
     */
    void SetName(const csString &newName)
    {
        name = newName;
    }

    /**
     * Returns the mesh that's associated with this effect anchor (almost always a nullmesh).
     *
     * @return Generally returns either a nullmesh or 0.
     */
    iMeshWrapper* GetMesh() const
    {
        return mesh;
    }

    /**
     * Sets the base rotation matrix of the effect anchor.
     *
     * @param newRotBase The base rotation matrix.
     */
    virtual void SetRotBase(const csMatrix3 &newRotBase)
    {
        matBase = newRotBase;
    }

    /**
     * Transforms the offset according to the mesh's settings, like abs_dir, etc.
     *
     * @param offset the vector to be transformed.
     */
    void TransformOffset(csVector3 &offset);

    /**
     * Returns the number of keyframes in this anchor.
     *
     * @return The keyFrame count.
     */
    size_t GetKeyFrameCount() const
    {
        return keyFrames->GetSize();
    }

    /**
     * Returns the keyframe at the given index.
     *
     * @param idx The index of the keyframe to grab.
     * @return The keyframe at the given index.
     */
    psEffectAnchorKeyFrame* GetKeyFrame(size_t idx) const
    {
        return keyFrames->Get(idx);
    }

    /**
     * Gets the direction type of this anchor.
     *
     * @return the direction type (none, origin, target).
     */
    const char* GetDirectionType() const;

    /**
     * Sets the direction type of this anchor.
     *
     * @param newDir the new direction type (none, origin, target).
     */
    void SetDirectionType(const char* newDir);

    /**
     * Sets the animation length of the effect anchor.
     *
     * @param newAnimLength the new animation length.
     */
    void SetAnimLength(float newAnimLength)
    {
        animLength = newAnimLength;
    }

    /**
     * Gets the animation length of the effect anchor.
     *
     * @return the animation length.
     */
    float GetAnimGetSize()
    {
        return animLength;
    }

    /**
     * Creates a new keyframe and attaches it to this movable.
     *
     * @param time The time of this new keyFrame.
     * @return the index where the new keyframe can be reached using GetKeyFrame(index).
     */
    size_t AddKeyFrame(float time);

    /**
     * Check to see if this anchor is ready.
     *
     * @return true if it's ready.
     */
    bool IsReady() const
    {
        return isReady;
    }

    enum DIR_TYPE
    {
        DT_NONE = 0,
        DT_ORIGIN,
        DT_TARGET,

        DT_COUNT
    };

    /**
     * Interpolates keyFrame actions for ones that weren't specified in each keyframe.
     */
    void FillInLerps();

protected:

    /**
     * Finds the index of the keyFrame at the specified time.
     *
     * @param time The time to lookup.
     * @return The index of the keyFrame at the specified time.
     */
    size_t FindKeyFrameByTime(float time) const;

    /**
     * Finds the next key frame where the specific action is specified.
     *
     * @param startFrame The first frame to start looking.
     * @param action The action to look for.
     * @param index A container to store the index of the found key frame.
     * @return true if it found one and the index is stored, false otherwise.
     */
    bool FindNextKeyFrameWithAction(size_t startFrame, size_t action, size_t &index) const;

    csString name; /// A unique name identifying this anchor.

    float life;       /// The amount of time this anchor has been alive (gets reset on loop).
    float animLength; /// The length of each loop of this anchor.

    csMatrix3 matBase; /// The base transform matrix of this anchor.

    /**
     * objEffectPos is pretty much the position of the effect, (that's not a requirement though).
     * It's the only position vector that can be modified externally (by the effect after creation)
     * and this one shouldn't be modified by the effect anchor in any way.
     */
    csVector3 objEffectPos;

    /// Base pos for the effect anchor, this is used for things like the variable offset from the anchoring mesh.
    csVector3 objBasePos;

    /// Stores the delta to the effect target.
    csVector3 objTargetOffset;

    /// The complete offset, this gets modified by the position action, generally.
    csVector3 objOffset;
    csMatrix3 posTransf;

    /// Stores the target of the effect.
    csVector3 target;
    csMatrix3 targetTransf;

    /// The mesh that makes up this anchor.
    csRef<iMeshWrapper> mesh;

    /// What type of direction does this movable have
    int dir;

    /// Whether the anchor should rotate with the mesh. This is so effects can stay aligned, e.g. flame sword.
    bool rotateWithMesh;

    size_t currKeyFrame; /// The current keyframe the anchor is on.
    size_t nextKeyFrame; /// The next keyframe the anchor will be on.

    /// Stores the keyframes of this anchor (this is shared between all anchor of this type).
    csRef<psEffectAnchorKeyFrameGroup> keyFrames;

    /// Reference to CS's iEngine.
    csRef<iEngine> engine;

    bool isReady;

    /// Linear interpolation function for a floating point.
    inline float lerp(float f1, float f2, float t1, float t2, float t)
    {
        if(t2 == t1)
            return f1;

        return f1 + (f2-f1)*(t-t1)/(t2-t1);
    }

    /// Linear interpolation function for a 3D vector.
    inline csVector3 lerpVec(const csVector3 &v1, const csVector3 &v2, float t1, float t2, float t)
    {
        if(t2 == t1)
            return v1;

        return v1 + (v2-v1)*(t-t1)/(t2-t1);
    }
};

/** @} */

#endif
