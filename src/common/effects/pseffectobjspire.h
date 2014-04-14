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

#ifndef PS_EFFECT_OBJ_SPIRE_HEADER
#define PS_EFFECT_OBJ_SPIRE_HEADER

#include "pseffectobj.h"
#include <imesh/genmesh.h>

class psEffect2DRenderer;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjSpire : public psEffectObj
{
public:
    struct MeshAnimControl : public scfImplementation1<MeshAnimControl, iGenMeshAnimationControl>
    {
    private:
        psEffectObjSpire* parent;

    public:
        MeshAnimControl(psEffectObjSpire* parent) : scfImplementationType(this)
        {
            this->parent = parent;
        }

        virtual ~MeshAnimControl()
        {
            parent = 0;
        }

        void Update(csTicks /*current*/, int /*num_verts*/, uint32 /*version_id*/)
        {
        }

        // genmesh animation control
        bool AnimatesVertices() const
        {
            return true;
        }
        bool AnimatesTexels() const
        {
            return true;
        }
        bool AnimatesNormals() const
        {
            return false;
        }
        bool AnimatesColors()const
        {
            return true;
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
        const csVector3* UpdateNormals(csTicks /*current*/, const csVector3* normals,
                                       int /*num_normals*/, uint32 /*version_id*/)
        {
            return normals;
        }
        const csColor4* UpdateColors(csTicks current, const csColor4* colors, int num_colors, uint32 version_id);
    };
    csRef<MeshAnimControl> meshControl;
    friend struct MeshAnimControl;



    psEffectObjSpire(iView* parentView, psEffect2DRenderer* renderer2d);
    virtual ~psEffectObjSpire();

    // inheritted function overloads
    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    bool Render(const csVector3 &up);
    bool Update(csTicks elapsed);
    psEffectObj* Clone() const;


    enum SPI_SHAPE
    {
        SPI_CIRCLE = 0,
        SPI_CYLINDER,
        SPI_ASTERIX,
        SPI_STAR,
        SPI_LAYERED,

        SPI_COUNT
    };

private:

    /** Fills the given buffers with appropriate data for the given state.
     *   \param shape       The shape of the new spire.
     *   \param segments    The number of segments of the new spire.
     *   \param verts       The buffer to hold the vertices.
     *   \param texels      The buffer to hold the texels.
     *   \param topScale    The topScale of the new spire.
     *   \param height      The height of the new spire.
     *   \param padding     The padding between segments for certain spires.
     */
    static void CalculateData(int shape, int segments, csVector3* verts, csVector2* texels, float topScale=1, float height=1, float padding=1);

    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    bool PostSetup();

    csRef<iGeneralMeshState> genState;

    int shape;
    int segments;

    csVector3* vert;
    csVector2* texel;
    csColor4*   colour;
    int vertCount;
};

/** @} */

#endif
