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

#include <psconfig.h>

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <ivideo/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/camera.h>
#include <cstool/csview.h>
#include <ivideo/graph2d.h>
#include <iengine/sector.h>

#include "effects/pseffectobjtext2d.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"
#include "util/pscssetup.h"
#include "util/log.h"

static int zorder = 0;

psEffectObjText2D::psEffectObjText2D(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
}

psEffectObjText2D::~psEffectObjText2D()
{
    size_t a = elems.GetSize();
    while(a)
    {
        --a;
        renderer2d->Remove2DElement(elems[a]);
    }
}

bool psEffectObjText2D::SetText(const csArray<psEffectTextElement> &elements)
{
    size_t elementCount = elements.GetSize();
    size_t a;
    size_t len;

    maxWidth = 0;
    maxHeight = 0;

    // calculate dimensions
    for(a=0; a<elementCount; ++a)
    {
        int right = elements[a].x + elements[a].width;
        int bottom = elements[a].y + elements[a].height;

        if(right > maxWidth)
            maxWidth = right;
        if(bottom > maxHeight)
            maxHeight = bottom;
    }

    int w = maxWidth / 2;
    int h = maxHeight / 2;

    // draw the backgrounds
    len = backgroundElems.GetSize();
    for(a=0; a<len; ++a)
    {
        csRect destRect;
        psEffectBackgroundElem* elem = &backgroundElems[a];

        // horizontal align
        switch(elem->align)
        {
            case EA_TOP_LEFT:
            case EA_LEFT:
            case EA_BOTTOM_LEFT:
                destRect.xmax = -w;
                destRect.xmin = -w - elem->umax + elem->umin;
                break;
            case EA_TOP:
            case EA_CENTER:
            case EA_BOTTOM:
                if(elem->scale)
                {
                    destRect.xmin = -w;
                    destRect.xmax = w;
                }
                else
                {
                    destRect.xmin = -(elem->umax - elem->umin) / 2;
                    destRect.xmax = -destRect.xmin;
                }
                break;
            case EA_TOP_RIGHT:
            case EA_RIGHT:
            case EA_BOTTOM_RIGHT:
                destRect.xmin = w;
                destRect.xmax = w + elem->umax - elem->umin;
                break;
            default:
                break;
        }

        // vertical align
        switch(elem->align)
        {
            case EA_TOP_LEFT:
            case EA_TOP:
            case EA_TOP_RIGHT:
                destRect.ymax = -h;
                destRect.ymin = -h - elem->vmax + elem->vmin;
                break;
            case EA_LEFT:
            case EA_CENTER:
            case EA_RIGHT:
                if(elem->scale)
                {
                    destRect.ymin = -h;
                    destRect.ymax = h;
                }
                else
                {
                    destRect.ymin = -(elem->vmax - elem->vmin) / 2;
                    destRect.ymax = -destRect.ymin;
                }
                break;
            case EA_BOTTOM_LEFT:
            case EA_BOTTOM:
            case EA_BOTTOM_RIGHT:
                destRect.ymin = h;
                destRect.ymax = h + elem->vmax - elem->vmin;
                break;
            default:
                break;
        }

        if(elem->scale)
        {
            // if it's scaled and has an offset, then the offset should scale both sides, otherwise it's just a translation
            if(elem->align == EA_TOP || elem->align == EA_BOTTOM || elem->align == EA_CENTER)
                destRect.xmin -= elem->offsetx;
            else
                destRect.xmin += elem->offsetx;
            destRect.xmax += elem->offsetx;

            if(elem->align == EA_LEFT || elem->align == EA_RIGHT || elem->align == EA_CENTER)
                destRect.ymin -= elem->offsety;
            else
                destRect.ymin += elem->offsety;
            destRect.ymax += elem->offsety;
        }
        else
        {
            destRect.xmin += elem->offsetx;
            destRect.xmax += elem->offsetx;
            destRect.ymin += elem->offsety;
            destRect.ymax += elem->offsety;
        }

        csRect tr(elem->umin, elem->vmin, elem->umax, elem->vmax);
        elems.Push(renderer2d->Add2DElement(new psEffect2DImgElement(zorder++, backgroundMat->GetMaterial()->GetTexture(), tr, destRect, 0, elem->tile)));
    }

    // draw all the text elements
    for(a=0; a<elementCount; ++a)
        DrawTextElement(elements[a]);

    return true;
}

bool psEffectObjText2D::SetText(const csArray<psEffectTextRow> &rows)
{
    const size_t len = rows.GetSize();
    static csArray<psEffectTextElement> elemBuffer;
    psEffectTextElement newElem;
    const psEffectTextRow* row = 0;
    int y = 0;

    elemBuffer.Empty();

    // Loop through all rows
    for(size_t a=0; a<len; ++a)
    {
        row = &rows[a];

        // Text and Formatting
        newElem.colour = row->colour;
        newElem.hasOutline = row->hasOutline;
        newElem.hasShadow = row->hasShadow;
        newElem.shadowColour = row->shadowColour;
        newElem.outlineColour = row->outlineColour;
        newElem.text = row->text;
        newElem.align = row->align;

        // Positioning
        font->GetDimensions(newElem.text, newElem.width, newElem.height);
        newElem.x = 0;
        newElem.y = y;
        elemBuffer.Push(newElem);
        y += newElem.height;
    }

    // draw the batch of text elements
    if(!SetText(elemBuffer))
        return false;

    return true;
}

bool psEffectObjText2D::SetText(int rows, ...)
{
    static csArray<psEffectTextElement> elemBuffer;
    psEffectTextElement newElem;
    psEffectTextRow* row = 0;

    elemBuffer.Empty();

    va_list arg;
    va_start(arg, rows);

    int y = 0;

    // Loop through all rows
    for(int i = 0; i < rows; i++)
    {
        row = va_arg(arg, psEffectTextRow*);

        // Text and Formatting
        newElem.colour = row->colour;
        newElem.hasOutline = row->hasOutline;
        newElem.hasShadow = row->hasShadow;
        newElem.shadowColour = row->shadowColour;
        newElem.outlineColour = row->outlineColour;
        newElem.text = row->text;
        newElem.align = row->align;

        // Positioning
        font->GetDimensions(newElem.text, newElem.width, newElem.height);
        newElem.x = 0;
        newElem.y = y;
        elemBuffer.Push(newElem);
        y += newElem.height;
    }
    va_end(arg);

    // draw the batch of text elements
    if(!SetText(elemBuffer))
        return false;

    return true;
}

bool psEffectObjText2D::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    if(!psEffectObj::Load(node, ldr_context))
        return false;

    // default text attributes
    name.Clear();
    fontName = "/this/data/ttf/LiberationSans-Regular.ttf";
    fontSize = 20;

    // read text attributes
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();

        if(attrName == "name")
            name = attr->GetValue();
        else if(attrName == "font")
            fontName = attr->GetValue();
        else if(attrName == "fontsize")
            fontSize = attr->GetValueAsInt();
    }
    csString backAlignText = node->GetAttributeValue("align");
    backAlignText.Downcase();
    if(backAlignText == "top")
        backgroundAlign = EA_TOP;
    else if(backAlignText == "bottom")
        backgroundAlign = EA_BOTTOM;
    else
        backgroundAlign = EA_CENTER;

    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }

    csRef<iDocumentNode> dataNode = node->GetNode("background");

    backgroundMat = effectsCollection->FindMaterial(dataNode->GetAttributeValue("material"));
    if(!backgroundMat)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects",
                 "Declared background without a valid material\n");
        return false;
    }

    csRef<iDocumentNodeIterator> elemNodes = dataNode->GetNodes("elem");
    while(elemNodes->HasNext())
    {
        dataNode = elemNodes->Next();
        psEffectBackgroundElem elem = { EA_NONE, 0, 0, 0, 0, true, 0, 0, 0 };

        // align
        csString alignText = dataNode->GetAttributeValue("align");
        alignText.Downcase();
        if(alignText == "top-left")
            elem.align = EA_TOP_LEFT;
        else if(alignText == "top")
            elem.align = EA_TOP;
        else if(alignText == "top-right")
            elem.align = EA_TOP_RIGHT;
        else if(alignText == "left")
            elem.align = EA_LEFT;
        else if(alignText == "center")
            elem.align = EA_CENTER;
        else if(alignText == "right")
            elem.align = EA_RIGHT;
        else if(alignText == "bottom-left")
            elem.align = EA_BOTTOM_LEFT;
        else if(alignText == "bottom")
            elem.align = EA_BOTTOM;
        else if(alignText == "bottom-right")
            elem.align = EA_BOTTOM_RIGHT;

        // uv
        elem.umin = dataNode->GetAttributeValueAsInt("umin");
        elem.umax = dataNode->GetAttributeValueAsInt("umax");
        elem.vmin = dataNode->GetAttributeValueAsInt("vmin");
        elem.vmax = dataNode->GetAttributeValueAsInt("vmax");

        // scale
        elem.scale = dataNode->GetAttributeValueAsBool("scale", true);
        elem.tile = dataNode->GetAttributeValueAsBool("tile", false);

        // offset
        elem.offsetx = dataNode->GetAttributeValueAsInt("offsetx");
        elem.offsety = dataNode->GetAttributeValueAsInt("offsety");

        // add the element
        backgroundElems.Push(elem);
    }

    return PostSetup();
}

bool psEffectObjText2D::Render(const csVector3 & /*up*/)
{
    return true;
}

bool psEffectObjText2D::AttachToAnchor(psEffectAnchor* newAnchor)
{
    if(newAnchor && newAnchor->GetMesh())
        anchorMesh = newAnchor->GetMesh();
    anchor = newAnchor;
    return true;
}

bool psEffectObjText2D::Update(csTicks elapsed)
{
    size_t a, len;
    int lerpAlpha = 255;

    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    life += elapsed;
    if(life > animLength && killTime <= 0)
    {
        life %= animLength;
        if(!life)
            life += animLength;
    }

    life |= (life >= birth);

    // calculate 2D position of text
    csVector3 p = anchorMesh->GetMovable()->GetPosition();
    if(keyFrames->GetSize() > 0)
    {
        currKeyFrame = FindKeyFrameByTime(life);
        nextKeyFrame = (currKeyFrame + 1) % keyFrames->GetSize();

        float lerpfactor = LERP_FACTOR;

        // position
        p += LERP_VEC_KEY(KA_POS,lerpfactor);

        // calculate alpha
        lerpAlpha = (int)(LERP_KEY(KA_ALPHA,lerpfactor) * 255);
    }

    // transform 3D to camera
    p = view->GetCamera()->GetTransform().Other2This(p);
    csVector2 sp;
    if(p.z <= 0.0f)
        sp.Set(-5000.0f, -5000.0f);
    else
    {
        // apply perspective
        sp = view->GetCamera()->Perspective(p);
        sp.y = view->GetPerspectiveCamera()->GetShiftY() * g2d->GetHeight() * 2 - sp.y;
    }

    len = elems.GetSize();
    int yAlign = 0;
    if(backgroundAlign == EA_TOP)
        yAlign = -maxHeight / 2;
    else if(backgroundAlign == EA_BOTTOM)
        yAlign = maxHeight / 2;

    for(a=0; a<len; ++a)
    {
        elems[a]->originx = (int)sp.x;
        elems[a]->originy = (int)sp.y + yAlign;
        elems[a]->SetAlpha(lerpAlpha);
    }

    if(killTime <= 0)
        return true;

    killTime -= elapsed;
    if(killTime <= 0)
        return false;

    return true;
}

psEffectObj* psEffectObjText2D::Clone() const
{
    psEffectObjText2D* newObj = new psEffectObjText2D(view, renderer2d);
    CloneBase(newObj);

    newObj->g3d = g3d;
    newObj->g2d = g2d;
    newObj->txtmgr = txtmgr;

    newObj->fontName = fontName;
    newObj->fontSize = fontSize;
    newObj->font = font;

    newObj->maxWidth = maxWidth;
    newObj->maxHeight = maxHeight;

    newObj->backgroundAlign = backgroundAlign;
    newObj->backgroundMat = backgroundMat;
    newObj->backgroundElems = backgroundElems;

    return newObj;
}

bool psEffectObjText2D::PostSetup()
{
    // get reference to iGraphics3D and iGraphics2D
    g3d =  csQueryRegistry<iGraphics3D> (psCSSetup::object_reg);
    if(!g3d)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't get iGraphics3D plugin!");
        return false;
    }
    g2d = g3d->GetDriver2D();

    // get reference to texture manager
    txtmgr = g3d->GetTextureManager();
    if(!txtmgr)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't get iTextureManager!");
        return false;
    }

    // load the font
    font = g2d->GetFontServer()->LoadFont(fontName, fontSize);

    return true;
}

void psEffectObjText2D::DrawTextElement(const psEffectTextElement &element)
{
    int x = 0;
    switch(element.align)
    {
        case ETA_LEFT:
            x = -maxWidth / 2;
            break;
        case ETA_CENTER:
            x = -element.width / 2;
            break;
        case ETA_RIGHT:
            x = maxWidth / 2 - element.width;
            break;
    }

    int y = element.y - maxHeight / 2;
    const char* text = element.text;

    psEffect2DTextElement* textElem = new psEffect2DTextElement(zorder++, font, text, x, y, element.colour, -1, element.hasOutline ? element.outlineColour : -1, element.hasShadow ? element.shadowColour : -1, 0);
    elems.Push(renderer2d->Add2DElement(textElem));
}

