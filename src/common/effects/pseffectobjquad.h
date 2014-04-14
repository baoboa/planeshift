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

#ifndef PS_EFFECT_OBJ_QUAD_HEADER
#define PS_EFFECT_OBJ_QUAD_HEADER

#include "pseffectobj.h"
#include <imesh/genmesh.h>
#include <csutil/cscolor.h>

class psEffect2DRenderer;

class psEffectObjQuad : public psEffectObj
{
public:
    struct MeshAnimControl : public scfImplementation1<MeshAnimControl, iGenMeshAnimationControl>
    {
    private:
        psEffectObjQuad* parent;

    public:
        MeshAnimControl(psEffectObjQuad* parent) : scfImplementationType(this)
        {
            this->parent = parent;
        }

        virtual ~MeshAnimControl()
        {
            parent = 0;
        }

        void Update(csTicks /*elapsed*/, int /*num_verts*/, uint32 /*version_id*/)
        {
        }

        // genmesh animation control
        bool AnimatesVertices() const
        {
            return false;
        }
        bool AnimatesTexels() const
        {
            return true;
        }
        bool AnimatesNormals() const
        {
            return false;
        }
        bool AnimatesColors() const
        {
            return false;
        }
        bool AnimatesBBoxRadius() const
        {
            return false;
        }

        const csBox3 &UpdateBoundingBox(csTicks /*current*/, uint32 /*version_id*/, const csBox3 &bbox)
        {
            return bbox;
        }
        const float UpdateRadius(csTicks /*current*/, uint32 /*version_id*/, const float radius)
        {
            return radius;
        }
        const csBox3* UpdateBoundingBoxes(csTicks /*current*/, uint32 /*version_id*/)
        {
            return nullptr;
        }
        const csVector3* UpdateVertices(csTicks current, const csVector3* verts, int num_verts, uint32 version_id);
        const csVector2* UpdateTexels(csTicks current, const csVector2* texels, int num_texels, uint32 version_id);
        const csVector3* UpdateNormals(csTicks /*current*/, const csVector3* normals, int /*num_normals*/,
                                       uint32 /*version_id*/)
        {
            return normals;
        }
        const csColor4* UpdateColors(csTicks /*current*/, const csColor4* colors, int /*num_colors*/,
                                     uint32 /*version_id*/)
        {
            return colors;
        }
    };
    csRef<MeshAnimControl> meshControl;
    friend struct MeshAnimControl;

    psEffectObjQuad(iView* parentView, psEffect2DRenderer* renderer2d);
    virtual ~psEffectObjQuad();

    // inheritted function overloads
    virtual bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    virtual bool Render(const csVector3 &up);
    virtual bool Update(csTicks elapsed);
    virtual void CloneBase(psEffectObj* newObj) const;
    virtual psEffectObj* Clone() const;


protected:

    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    virtual bool PostSetup();

    /** Creates a mesh factory for this quad.
     */
    virtual bool CreateMeshFact();

    /** Decides whether or not this quad will have a unique mesh factory per effect instance.
     *  One example where a unique mesh fact is necessary is with text objs where a unique
     *  material is needed per mesh (a unique material requires a unique mesh fact).
     */
    virtual bool UseUniqueMeshFact() const;

    csRef<iGeneralMeshState> genState;

    csVector3 			vert[4];
    csVector2 			texel[4];

    csRef<iMaterialWrapper>	mat;

    float quadAspect;
};

#endif
