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
#include <imesh/particle.h>
#include <imesh/objmodel.h>
#include <csutil/flags.h>

#include "effects/pseffectobjtext.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"
#include "util/pscssetup.h"
#include "util/log.h"

#define TEXT_BUFFER 5

psEffectObjText::psEffectObjText(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObjQuad(parentView, renderer2d)
{
}

psEffectObjText::~psEffectObjText()
{
    if(meshFact)
        engine->RemoveObject(meshFact);

    if(generatedMat)
        engine->RemoveObject(generatedMat);

    if(generatedTex)
        engine->RemoveObject(generatedTex);
}

bool psEffectObjText::SetText(const csArray<psEffectTextElement> &elements)
{
    size_t elementCount = elements.GetSize();
    size_t a;

    static unsigned int uniqueID = 0;
    csString matName = "effect_text_mat_";
    matName += uniqueID++;

    // delete the old material
    if(generatedMat)
        engine->RemoveObject(generatedMat);

    if(generatedTex)
        engine->RemoveObject(generatedTex);

    // calculate dimensions of text area
    int maxWidth = 0;
    int maxHeight = 0;

    for(a=0; a<elementCount; ++a)
    {
        int right = elements[a].x + elements[a].width;
        int bottom = elements[a].y + elements[a].height;

        if(right > maxWidth)
            maxWidth = right;
        if(bottom > maxHeight)
            maxHeight = bottom;
    }

    // calculate size of texture
    texOriginalWidth = maxWidth + TEXT_BUFFER * 2;
    texOriginalHeight = maxHeight + TEXT_BUFFER * 2;
    quadAspect = (float)texOriginalHeight / (float)texOriginalWidth;

    texPO2Width = ToPowerOf2(texOriginalWidth);
    texPO2Height = ToPowerOf2(texOriginalHeight);

    float uMax = (float)texOriginalWidth / (float)texPO2Width;
    float vMax = (float)texOriginalHeight / (float)texPO2Height;
    texel[0].Set(uMax, 0.0f);
    texel[1].Set(0.0f, 0.0f);
    texel[2].Set(0.0f, vMax);
    texel[3].Set(uMax, vMax);

    // create the texture
    csColor transp(51/255.0F,51/255.0F,254/255.0F);
    generatedTex = engine->CreateBlackTexture(matName, texPO2Width, texPO2Height, &transp, CS_TEXTURE_2D);
    generatedTex->Register(txtmgr);
    generatedMat = engine->CreateMaterial(matName, generatedTex);

    g3d->SetRenderTarget(generatedMat->GetMaterial()->GetTexture());
    g3d->BeginDraw(CSDRAW_2DGRAPHICS);

    // initialize background
    g2d->DrawBox(0, 0, texPO2Width, texPO2Height, g2d->FindRGB(51, 51, 254));

    // draw all the text elements
    for(a=0; a<elementCount; ++a)
        DrawTextElement(elements[a]);

    g3d->FinishDraw();
    return true;
}

bool psEffectObjText::SetText(const csArray<psEffectTextRow> &rows)
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
    if(SetText(elemBuffer))
    {
        mesh->GetMeshObject()->SetMaterialWrapper(generatedMat);
        return true;
    }
    return false;
}

bool psEffectObjText::SetText(int rows, ...)
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
    if(SetText(elemBuffer))
    {
        mesh->GetMeshObject()->SetMaterialWrapper(generatedMat);
        return true;
    }
    return false;
}

bool psEffectObjText::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    // default text attributes
    fontName = "/this/data/ttf/LiberationSans-Regular.ttf";
    fontSize = 20;

    // read text attributes
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();
        attrName.Downcase();

        if(attrName == "font")
            fontName = attr->GetValue();
        else if(attrName == "fontquality")
            fontSize = attr->GetValueAsInt();
    }

    if(!psEffectObjQuad::Load(node, ldr_context))
        return false;

    return true;
}

psEffectObj* psEffectObjText::Clone() const
{
    psEffectObjText* newObj = new psEffectObjText(view, renderer2d);
    CloneBase(newObj);

    newObj->g3d = g3d;
    newObj->g2d = g2d;
    newObj->txtmgr = txtmgr;

    newObj->fontName = fontName;
    newObj->fontSize = fontSize;
    newObj->font = font;

    return newObj;
}

bool psEffectObjText::PostSetup()
{
    if(!psEffectObjQuad::PostSetup())
        return false;

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

void psEffectObjText::DrawTextElement(const psEffectTextElement &element)
{
    int x = 0;
    bool antiAlias = false;
    switch(element.align)
    {
        case ETA_LEFT:
            x = TEXT_BUFFER;
            break;
        case ETA_CENTER:
            x = (texOriginalWidth - element.width) / 2;
            break;
        case ETA_RIGHT:
            x = texOriginalWidth - element.width - TEXT_BUFFER;
            break;
    }

    int y = TEXT_BUFFER + element.y;
    const char* text = element.text;
    if(element.hasOutline)
    {
        // so, um, change this
        g2d->Write(font, x,   y-2, element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x+2, y-2, element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x+2, y,   element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x+2, y+2, element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x,   y+2, element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x-2, y+2, element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x-2, y,   element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
        g2d->Write(font, x-2, y-2, element.outlineColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
    }

    if(element.hasShadow)
        g2d->Write(font, x+2, y+2, element.shadowColour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));

    g2d->Write(font, x, y, element.colour, -1, text, (antiAlias ? 0 : CS_WRITE_NOANTIALIAS));
}

