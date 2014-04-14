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

#ifndef PS_EFFECT_OBJ_TEXT_2D_HEADER
#define PS_EFFECT_OBJ_TEXT_2D_HEADER

#include <csgeom/vector2.h>
#include <csutil/array.h>

#include "pseffectobj.h"
#include "effects/pseffectobjtextable.h"

class psEffect2DRenderer;
class psEffect2DElement;
class psEffect2DTextElement;
class psEffect2DImgElement;

/**
 * \addtogroup common_effects
 * @{ */

enum psEffectBackgroundAlign
{
    EA_NONE = 0,

    EA_TOP_LEFT,
    EA_TOP,
    EA_TOP_RIGHT,

    EA_LEFT,
    EA_CENTER,
    EA_RIGHT,

    EA_BOTTOM_LEFT,
    EA_BOTTOM,
    EA_BOTTOM_RIGHT
};
struct psEffectBackgroundElem
{
    psEffectBackgroundAlign     align;

    int                         umin;
    int                         vmin;
    int                         umax;
    int                         vmax;

    bool                        scale;
    bool                        tile;
    int                         offsetx;
    int                         offsety;
};

class psEffectObjText2D : public psEffectObj, public psEffectObjTextable
{
private:
    csRef<iGraphics3D>      g3d;
    csRef<iGraphics2D>      g2d;
    csRef<iTextureManager>  txtmgr;

    // font
    csString            fontName;
    int                 fontSize;
    csRef<iFont>        font;

    int                 maxWidth;
    int                 maxHeight;

    psEffectBackgroundAlign             backgroundAlign;
    iMaterialWrapper*                   backgroundMat;
    csArray<psEffectBackgroundElem>     backgroundElems;

    csArray<psEffect2DElement*>    elems;

public:
    psEffectObjText2D(iView* parentView, psEffect2DRenderer* renderer2d);
    ~psEffectObjText2D();

    // draws 2d text
    bool SetText(const csArray<psEffectTextElement> &elements);
    bool SetText(const csArray<psEffectTextRow> &rows);
    bool SetText(int rows, ...);

    // inheritted function overloads
    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    bool Render(const csVector3 &up);
    bool AttachToAnchor(psEffectAnchor* newAnchor);
    bool Update(csTicks elapsed);
    psEffectObj* Clone() const;

protected:

    bool PostSetup();
    void DrawTextElement(const psEffectTextElement &element);
};

/** @} */

#endif
