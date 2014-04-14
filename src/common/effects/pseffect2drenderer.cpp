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

#include <psconfig.h>

#include <csgeom/math.h>

#include "pseffect2drenderer.h"


psEffect2DElement::psEffect2DElement(int zOrder, int alpha)
    : zOrder(zOrder), alpha(alpha), originx(0), originy(0)
{
}

psEffect2DElement::~psEffect2DElement()
{
}

int psEffect2DElement::GetZOrder() const
{
    return zOrder;
}

int psEffect2DElement::GetAlpha() const
{
    return alpha;
}

void psEffect2DElement::SetAlpha(int alpha)
{
    this->alpha = alpha;
}

void psEffect2DElement::Draw(iGraphics3D* /*g3d*/, iGraphics2D* /*g2d*/)
{
}

psEffect2DTextElement::psEffect2DTextElement(int zOrder, iFont* font, const char* text, int x, int y, int fgColor, int bgColor, int outlineColor, int shadowColor, int alpha)
    : psEffect2DElement(zOrder, alpha), font(font), x(x), y(y), fgColor(fgColor), bgColor(bgColor), shadowColor(shadowColor), outlineColor(outlineColor)
{
    strncpy(this->text, text, PS_EFFECT_2D_TEXT_MAX_CHARS);
    this->text[PS_EFFECT_2D_TEXT_MAX_CHARS-1] = 0;
}

psEffect2DTextElement::~psEffect2DTextElement()
{
}

void psEffect2DTextElement::Draw(iGraphics3D* /*g3d*/, iGraphics2D* g2d)
{
    int x = this->x + originx;
    int y = this->y + originy;
    if(outlineColor >= 0)
    {
        g2d->Write(font, x-2, y-2, outlineColor, bgColor, text);
        g2d->Write(font, x-2, y,   outlineColor, bgColor, text);
        g2d->Write(font, x-2, y+2, outlineColor, bgColor, text);
        g2d->Write(font, x,   y-2, outlineColor, bgColor, text);
        g2d->Write(font, x,   y+2, outlineColor, bgColor, text);
        g2d->Write(font, x+2, y-2, outlineColor, bgColor, text);
        g2d->Write(font, x+2, y,   outlineColor, bgColor, text);
        g2d->Write(font, x+2, y+2, outlineColor, bgColor, text);
    }
    if(shadowColor >= 0)
        g2d->Write(font, x+1, y+1, shadowColor, bgColor, text);

    g2d->Write(font, x, y, fgColor, bgColor, text);
}

psEffect2DImgElement::psEffect2DImgElement(int zOrder, iTextureHandle* texHandle, const csRect &texRect, const csRect &destRect, int alpha, bool tiled)
    : psEffect2DElement(zOrder, alpha), texHandle(texHandle), texRect(texRect), destRect(destRect), tiled(tiled)
{
}

psEffect2DImgElement::~psEffect2DImgElement()
{
}

void psEffect2DImgElement::Draw(iGraphics3D* g3d, iGraphics2D* /*g2d*/)
{
    int tw = texRect.Width();
    int th = texRect.Height();
    int left = destRect.xmin + originx;
    int top = destRect.ymin + originy;
    if(!tiled)
        g3d->DrawPixmap(texHandle, left, top, destRect.Width(), destRect.Height(),
                        texRect.xmin, texRect.ymin, tw, th, 255 - alpha);
    else
    {
        int right = left + destRect.Width();
        int bottom = top + destRect.Height();
        for(int x=left; x<right;  x+=tw)
        {
            for(int y=top; y<bottom; y+=th)
            {
                int w = csMin<int>(tw, right - x);
                int h = csMin<int>(th, bottom - y);
                g3d->DrawPixmap(texHandle, x, y, w, h, texRect.xmin, texRect.ymin, w, h, 255 - alpha);
            }
        }
    }
}


psEffect2DRenderer::psEffect2DRenderer()
{
}

psEffect2DRenderer::~psEffect2DRenderer()
{
}

int psEffect2DRendererCompareZOrder(psEffect2DElement* const &a, psEffect2DElement* const &b)
{
    if(a->GetZOrder() < b->GetZOrder())
        return -1;
    else if(a->GetZOrder() == b->GetZOrder())
        return 0;
    return 1;
}

psEffect2DElement* psEffect2DRenderer::Add2DElement(psEffect2DElement* elem)
{
    effect2DElements.Push(elem);
    effect2DElements.Sort(&psEffect2DRendererCompareZOrder);

    return elem;
}

void psEffect2DRenderer::Remove2DElement(psEffect2DElement* elem)
{
    size_t a;
    size_t len = effect2DElements.GetSize();
    for(a=0; a<len; ++a)
    {
        if(effect2DElements[a] != elem)
            continue;

        effect2DElements.DeleteIndex(a);
        return;
    }
}

void psEffect2DRenderer::Remove2DElementByIndex(size_t index)
{
    effect2DElements.DeleteIndex(index);
}

void psEffect2DRenderer::RemoveAll2DElements()
{
    effect2DElements.DeleteAll();
}

size_t psEffect2DRenderer::Get2DElementCount() const
{
    return effect2DElements.GetSize();
}

void psEffect2DRenderer::Render(iGraphics3D* g3d, iGraphics2D* g2d)
{
    size_t a;
    size_t len = effect2DElements.GetSize();
    for(a=0; a<len; ++a)
        effect2DElements[a]->Draw(g3d, g2d);
}
