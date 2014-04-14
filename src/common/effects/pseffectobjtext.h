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

#ifndef PS_EFFECT_OBJ_TEXT_HEADER
#define PS_EFFECT_OBJ_TEXT_HEADER

#include "pseffectobjquad.h"
#include <imesh/sprite2d.h>

#include "pseffectobjtextable.h"

struct iGraphics3D;
struct iGraphics2D;
struct iTextureManager;
struct iTextureWrapper;
struct iFont;
struct iParticle;
class psEffect2DRenderer;


/**
 * \addtogroup common_effects
 * @{ */

class psEffectObjText : public psEffectObjQuad, public psEffectObjTextable
{
private:
    // needed managers
    csRef<iGraphics3D>      g3d;
    iGraphics2D*            g2d;
    csRef<iTextureManager>  txtmgr;

    // font
    csString            fontName;
    int                 fontSize;
    csRef<iFont>        font;

    // generated texture
    csRef<iTextureWrapper>      generatedTex;
    csRef<iMaterialWrapper>     generatedMat;

    int             texOriginalWidth;
    int             texOriginalHeight;

    int             texPO2Width;
    int             texPO2Height;

public:
    psEffectObjText(iView* parentView, psEffect2DRenderer* renderer2d);
    ~psEffectObjText();

    /** Creates a material that will fit an array of text elements and draws those elements.
     */
    virtual bool SetText(const csArray<psEffectTextElement> &elements);
    virtual bool SetText(const csArray<psEffectTextRow> &rows);
    virtual bool SetText(int rows, ...);

    // inheritted function overloads
    bool Load(iDocumentNode* node, iLoaderContext* ldr_context);
    psEffectObj* Clone() const;

protected:
    /** performs the post setup (after the effect obj has been loaded).
     *  Things like create mesh factory, etc are initialized here.
     */
    bool PostSetup();

    void DrawTextElement(const psEffectTextElement &element);

    inline int ToPowerOf2(int n)
    {
        for(int a=sizeof(int) * 8-1; a>0; --a)
        {
            if(n & (1 << a))
            {
                if(n == (1 << a))
                    return n;
                else
                    return (1 << (a+1));
            }
        }
        return 1;
    };
};

/** @} */

#endif
