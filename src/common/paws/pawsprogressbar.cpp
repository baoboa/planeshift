/*
 * pawsprogressbar.cpp- Author: Andrew Craig
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
#include <csgeom/vector4.h>
#include "pawsprogressbar.h"
#include "pawsmanager.h"

pawsProgressBar::pawsProgressBar()
{
    totalValue = 0.0f;
    complete  = false;
    currentValue = 0.0f;
    percent      = 0.0f;
    factory      = "pawsProgressBar";
}
pawsProgressBar::pawsProgressBar(const pawsProgressBar &origin)
    :pawsWidget(origin),
     totalValue(origin.totalValue),
     currentValue(origin.currentValue),
     percent(origin.percent),
     complete(origin.complete),
     start_r(origin.start_r),start_g(origin.start_g),start_b(origin.start_b),
     diff_r(origin.diff_r),diff_g(origin.diff_g),diff_b(origin.diff_b)
{

}
pawsProgressBar::~pawsProgressBar()
{
}

bool pawsProgressBar::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> ColourNode = node->GetNode("color");
    if(ColourNode)
    {
        start_r = ColourNode->GetAttributeValueAsInt("r");
        start_g = ColourNode->GetAttributeValueAsInt("g");
        start_b = ColourNode->GetAttributeValueAsInt("b");
    }
    else
    {
        start_r = 0;
        start_g = 0;
        start_b = 25;
    }

    csRef<iDocumentNode> ColourNode2 = node->GetNode("fadecolor");
    if(ColourNode2)
    {
        diff_r = ColourNode2->GetAttributeValueAsInt("r") - start_r;
        diff_g = ColourNode2->GetAttributeValueAsInt("g") - start_g;
        diff_b = ColourNode2->GetAttributeValueAsInt("b") - start_b;
    }
    else
    {
        diff_r = 0;
        diff_g = 0;
        diff_b = 0;
    }
    return true;
}

void pawsProgressBar::SetCurrentValue(float newValue)
{
    currentValue = newValue;

    percent = totalValue ? (currentValue / totalValue) : 0;
}

void pawsProgressBar::Draw()
{
    ClipToParent(false);

    int alpha = 255;
    if(parent
            && !parent->GetBackground().IsEmpty()
            && parent->isFadeEnabled()
            && parent->GetMaxAlpha() != parent->GetMinAlpha())
        alpha = (int)
                (255 - (parent->GetMinAlpha() + (parent->GetMaxAlpha()-parent->GetMinAlpha()) * parent->GetFadeVal() * 0.010));
    DrawBackground();
    DrawProgressBar(screenFrame, PawsManager::GetSingleton().GetGraphics3D(), percent,
                    start_r, start_g, start_b,
                    diff_r,  diff_g,  diff_b, alpha);
    DrawChildren();
    DrawMask();
}

void pawsProgressBar::DrawProgressBar(
    const csRect &rect, iGraphics3D* graphics3D, float percent,
    int start_r, int start_g, int start_b,
    int diff_r,  int diff_g,  int diff_b, int alpha)
{
    csSimpleRenderMesh mesh;
    static uint indices[4] = {0, 1, 2, 3};
    csVector3 verts[4];
    csVector4 colors[4];
    float fr1 = start_r / 255.0f;
    float fg1 = start_g / 255.0f;
    float fb1 = start_b / 255.0f;
    float fr2 = fr1 + percent * (diff_r / 255.0f);
    float fg2 = fg1 + percent * (diff_g / 255.0f);
    float fb2 = fb1 + percent * (diff_b / 255.0f);
    float fa = alpha / 255.0f;

    mesh.meshtype = CS_MESHTYPE_QUADS;
    mesh.indexCount = 4;
    mesh.indices = indices;
    mesh.vertexCount = 4;
    mesh.vertices = verts;
    mesh.colors = colors;
    mesh.mixmode = CS_FX_COPY;
    mesh.alphaType.autoAlphaMode = false;
    mesh.alphaType.alphaType = csAlphaMode::alphaSmooth;

    verts[0].Set(rect.xmin, rect.ymin, 0);
    colors[0].Set(fr1, fg1, fb1, fa);

    verts[1].Set(rect.xmin + (rect.Width() * percent), rect.ymin, 0);
    colors[1].Set(fr2, fg2, fb2, fa);

    verts[2].Set(rect.xmin + (rect.Width() * percent), rect.ymax, 0);
    colors[2].Set(fr2, fg2, fb2, fa);

    verts[3].Set(rect.xmin, rect.ymax, 0);
    colors[3].Set(fr1, fg1, fb1, fa);

    graphics3D->DrawSimpleMesh(mesh, csSimpleMeshScreenspace);
}

void pawsProgressBar::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    SetCurrentValue(value.GetFloat());
}

