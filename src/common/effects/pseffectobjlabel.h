/*
 * Author: Roland Schulz
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PS_EFFECT_OBJ_LABEL_HEADER
#define PS_EFFECT_OBJ_LABEL_HEADER

#include "pseffectobj.h"
#include <imesh/genmesh.h>
#include <csutil/cscolor.h>
#include "pseffectobjtextable.h"

struct iGraphics3D;
struct iGraphics2D;
struct iTextureManager;
struct iTextureWrapper;
struct iFont;
struct iParticle;
class  psEffect2DRenderer;

/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjLabel : public psEffectObj, public psEffectObjTextable
{
private:
    // needed managers
    csRef<iTextureManager>  txtmgr;
    csRef<iGeneralFactoryState> facState;
    csRef<iGeneralMeshState> genState;

    // font
    csString       sizeFileName;
    float          labelwidth;
    csArray<uint16> xpos;
    csArray<uint16> ypos;
    csArray<uint16> width;
    csArray<uint16> height;

public:
    psEffectObjLabel(iView* parentView, psEffect2DRenderer* renderer2d);
    virtual ~psEffectObjLabel();

    /** Creates a material that will fit an array of text elements and draws those elements.
     */
    virtual bool SetText(const csArray<psEffectTextElement> &elements);
    virtual bool SetText(const csArray<psEffectTextRow> &rows);
    virtual bool SetText(int rows, ...);

    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    void LoadGlyphs(csString name);
    psEffectObj* Clone() const;
    // inherited function overloads
    virtual bool Render(const csVector3 &up);
    virtual bool Update(csTicks elapsed);
    virtual void CloneBase(psEffectObj* newObj) const;

protected:
    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    bool PostSetup();

    void DrawTextElement(const psEffectTextElement &element);

    /** Creates a mesh factory for this quad.
     */
    virtual bool CreateMeshFact();

};

/** @} */

#endif
