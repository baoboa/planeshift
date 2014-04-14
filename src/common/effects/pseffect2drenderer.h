/*
* Author: Andrew Robberts
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

#ifndef PS_EFFECT_2D_RENDERER
#define PS_EFFECT_2D_RENDERER

#include <csutil/parray.h>
#include <cstool/cspixmap.h>
#include <csgeom/csrect.h>
#include <ivideo/texture.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <ivideo/fontserv.h>

/**
 * \addtogroup common_effects
 * @{ */

#define PS_EFFECT_2D_TEXT_MAX_CHARS 512

/** Base class for all possible 2D Effect Elements
 */
class psEffect2DElement
{
protected:
    int zOrder;
    int alpha;

public:
    int originx;
    int originy;

public:
    psEffect2DElement(int zOrder, int alpha);
    virtual ~psEffect2DElement();
    int GetZOrder() const;
    int GetAlpha() const;
    void SetAlpha(int alpha);
    virtual void Draw(iGraphics3D* g3d, iGraphics2D* g2d);
};

/** A 2D Text Effect Element
 */
class psEffect2DTextElement : public psEffect2DElement
{
public:
    csRef<iFont>    font;
    char            text[PS_EFFECT_2D_TEXT_MAX_CHARS];
    int             x;
    int             y;
    int             fgColor;
    int             bgColor;
    int				shadowColor;
    int				outlineColor;

public:
    psEffect2DTextElement(int zOrder, iFont* font, const char* text, int x, int y, int fgColor, int bgColor, int outlineColor, int shadowColor, int alpha);
    virtual ~psEffect2DTextElement();
    virtual void Draw(iGraphics3D* g3d, iGraphics2D* g2d);
};

/** A 2D Image Effect Element
 */
class psEffect2DImgElement : public psEffect2DElement
{
public:
    csRef<iTextureHandle> texHandle;
    csRect                texRect;
    csRect                destRect;
    bool                  tiled;

public:
    psEffect2DImgElement(int zOrder, iTextureHandle* texHandle, const csRect &texRect, const csRect &destRect, int alpha, bool tiled);
    virtual ~psEffect2DImgElement();
    virtual void Draw(iGraphics3D* g3d, iGraphics2D* g2d);
};

/** The manager of all 2D effect elements.
 */
class psEffect2DRenderer
{
private:
    csPDelArray<psEffect2DElement>  effect2DElements;

public:
    psEffect2DRenderer();
    ~psEffect2DRenderer();

    psEffect2DElement* Add2DElement(psEffect2DElement* elem);
    void Remove2DElement(psEffect2DElement* elem);
    void Remove2DElementByIndex(size_t index);
    void RemoveAll2DElements();
    size_t Get2DElementCount() const;

    void Render(iGraphics3D* g3d, iGraphics2D* g2d);
};

/** @} */

#endif // PS_EFFECT_2D_RENDERER
