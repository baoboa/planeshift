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
#include <ctype.h>
#include <psconfig.h>

#include <csutil/xmltiny.h>
#include <iengine/engine.h>
#include <iengine/material.h>
#include <ivideo/material.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <iengine/camera.h>
#include <cstool/csview.h>
#include <imesh/objmodel.h>
#include <csutil/flags.h>
#include <csgeom/tri.h>

#include "effects/pseffectobjlabel.h"
#include "effects/pseffectanchor.h"
#include "effects/pseffect2drenderer.h"

#include "util/log.h"
#include "util/pscssetup.h"

#define NUM_GLYPHS 256

psEffectObjLabel::psEffectObjLabel(iView* parentView, psEffect2DRenderer* renderer2d)
    : psEffectObj(parentView, renderer2d)
{
    meshFact = 0;
    mesh = 0;
    xpos.SetSize(NUM_GLYPHS,0);
    ypos.SetSize(NUM_GLYPHS,0);
    width.SetSize(NUM_GLYPHS,64);
    height.SetSize(NUM_GLYPHS,64);
    labelwidth = 3.0;
    for(int i=0; i<256; i++)
    {
        xpos[i] = (i%16) * 64;
        ypos[i] = (i/16-2) * 64;
    }
}

psEffectObjLabel::~psEffectObjLabel()
{
    if(meshFact)
    {
        engine->RemoveObject(meshFact);
    }
    if(mesh)
    {
        engine->RemoveObject(mesh);
    }
    //printf("label destroyed\n");
}

bool psEffectObjLabel::Load(iDocumentNode* node, iLoaderContext* ldr_context)
{
    // get the attributes
    name.Clear();
    materialName.Clear();
    sizeFileName.Clear();
    csRef<iDocumentAttributeIterator> attribIter = node->GetAttributes();
    while(attribIter->HasNext())
    {
        csRef<iDocumentAttribute> attr = attribIter->Next();
        csString attrName = attr->GetName();

        attrName.Downcase();
        if(attrName == "name")
            name = attr->GetValue();
        else if(attrName == "material")
            materialName = attr->GetValue();
        else if(attrName == "sizefile")
            sizeFileName = attr->GetValue();
        else if(attrName == "labelwidth")
            labelwidth = attr->GetValueAsFloat();
    }
    if(name.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no name.\n");
        return false;
    }
    if(materialName.IsEmpty())
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Attempting to create an effect obj with no material.\n");
        return false;
    }
    if(!sizeFileName.IsEmpty())
    {
        LoadGlyphs(sizeFileName);
    }

    if(!psEffectObj::Load(node, ldr_context))
    {
        return false;
    }
    return PostSetup();
}


void psEffectObjLabel::LoadGlyphs(csString filename)
{
    csRef<iVFS> vfs = csQueryRegistry<iVFS > (psCSSetup::object_reg);
    csRef<iDocumentSystem> xml = csQueryRegistry<iDocumentSystem> (psCSSetup::object_reg);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDataBuffer> buf(vfs->ReadFile(filename));
    if(!buf || !buf->GetSize())
    {
        return ;
    }
    const char* error = doc->Parse(buf);
    if(error)
    {
        Error2("Error loading glyphs file: %s", error);
        return ;
    }

    csRef<iDocumentNodeIterator> iter = doc->GetRoot()->GetNode("glyphs")->GetNodes();
    int number;

    while(iter->HasNext())
    {
        csRef<iDocumentNode> child = iter->Next();
        if(child->GetType() != CS_NODE_ELEMENT)
            continue;
        if(strcmp(child->GetValue(),"glyph"))
        {
            Error2("Unknown child %s encountered", child->GetValue());
            continue;
        }
        number = child->GetAttributeValueAsInt("id");
        if(number < 0 || number >= NUM_GLYPHS)
            continue;
        width[number] = child->GetAttributeValueAsInt("width");
        height[number] = child->GetAttributeValueAsInt("height");
        xpos[number] = child->GetAttributeValueAsInt("x");
        ypos[number] = child->GetAttributeValueAsInt("y");
        //printf("loaded %d - %d %d %d %d\n", number, width[number], height[number], xpos[number], ypos[number]);
    }
}


bool psEffectObjLabel::PostSetup()
{
//    if (!CreateMeshFact())
//      return false;

    return true;
}

bool psEffectObjLabel::CreateMeshFact()
{
    //printf("label: creating meshfact\n");
    static unsigned int uniqueID = 0;
    csString facName = "effect_label_fac_";
    facName += uniqueID++;

    meshFact = engine->CreateMeshFactory("crystalspace.mesh.object.genmesh", facName.GetData());
    effectsCollection->Add(meshFact->QueryObject());

    iMeshObjectFactory* fact = meshFact->GetMeshObjectFactory();
    facState =  scfQueryInterface<iGeneralFactoryState> (fact);
    if(!facState)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't create genmesh: %s\n", name.GetData());
        return false;
    }

    // setup the material
    csRef<iMaterialWrapper> mat = effectsCollection->FindMaterial(materialName);
    if(mat)
    {
        fact->SetMaterialWrapper(mat);
    }
    else
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "No material for label mesh: %s\n", materialName.GetData());
    }

#if 0
    facState->SetVertexCount(4);

    // we have to set the vertices so that the quad actually gets rendered
    facState->GetVertices()[0].Set(0.5f, 0,  0.5f);
    facState->GetVertices()[1].Set(-0.5f, 0,  0.5f);
    facState->GetVertices()[2].Set(-0.5f, 0, -0.5f);
    facState->GetVertices()[3].Set(0.5f, 0, -0.5f);
    facState->GetTexels()[0].Set(5.0/8,4.0/16);
    facState->GetTexels()[1].Set(4.0/8,4.0/16);
    facState->GetTexels()[2].Set(4.0/8,5.0/16);
    facState->GetTexels()[3].Set(5.0/8,5.0/16);
    facState->GetNormals()[0].Set(0,1,0);
    facState->GetNormals()[1].Set(0,1,0);
    facState->GetNormals()[2].Set(0,1,0);
    facState->GetNormals()[3].Set(0,1,0);

    facState->SetTriangleCount(4);

    facState->GetTriangles()[0].Set(0, 2, 1);
    facState->GetTriangles()[1].Set(0, 2, 3);
    facState->GetTriangles()[2].Set(2, 0, 1);
    facState->GetTriangles()[3].Set(2, 0, 3);
#endif
    return true;
}


bool psEffectObjLabel::Render(const csVector3 &up)
{
    //printf("label: render\n");
    if(!CreateMeshFact())
        return false;

    objUp = up;

    static unsigned long nextUniqueID = 0;
    csString effectID = "effect_label_";
    effectID += nextUniqueID++;

    // create a mesh wrapper from the factory we just created
    mesh = engine->CreateMeshWrapper(meshFact, effectID.GetData());
    if(!mesh)
    {
        csReport(psCSSetup::object_reg, CS_REPORTER_SEVERITY_ERROR, "planeshift_effects", "Couldn't create meshwrapper in label effect\n");
        return false;
    }

    // do the up vector
//    csReversibleTransform rt;
//    rt.LookAt(csVector3(up.x, up.z, up.y), csVector3(0,2,1));
//    matUp = rt.GetT2O();
//    matBase = matUp;

    // common flags
    mesh->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    mesh->GetFlags().Set(CS_ENTITY_NODECAL);
    // we can have labels overlay geometry, or we can have them be occluded by it like normal objects
    //priority = engine->GetAlphaRenderPriority();
    // zFunc = CS_ZBUF_NONE;
    priority = engine->GetObjectRenderPriority();
    zFunc = CS_ZBUF_USE;
    mixmode = CS_FX_COPY;
    //printf("zbufmode: %d pri: %d mixmode: %d\n", zFunc, priority, mixmode);
    mesh->SetZBufMode(zFunc);
    mesh->SetRenderPriority(priority);

    // disable culling

    csStringID viscull_id = globalStringSet->Request("viscull");
    mesh->GetMeshObject()->GetObjectModel()->SetTriangleData(viscull_id, 0);

    genState =  scfQueryInterface<iGeneralMeshState> (mesh->GetMeshObject());

    // obj specific

    if(mixmode != CS_FX_ALPHA)
        mesh->GetMeshObject()->SetMixMode(mixmode);  // to check
    // genState->SetMixMode(mixmode); //was not working with latest CS

    return true;
}

bool psEffectObjLabel::Update(csTicks elapsed)
{
    if(!anchor || !anchor->IsReady())  // wait for anchor to be ready
        return true;

    if(!psEffectObj::Update(elapsed))
        return false;

    return true;
}

void psEffectObjLabel::CloneBase(psEffectObj* newObj) const
{
    psEffectObj::CloneBase(newObj);

    psEffectObjLabel* newLabelObj = dynamic_cast<psEffectObjLabel*>(newObj);

    newLabelObj->materialName = materialName;
    newLabelObj->labelwidth = labelwidth;

    for(int i=0; i<NUM_GLYPHS; i++)
    {
        newLabelObj->width[i] = width[i];
        newLabelObj->height[i] = height[i];
        newLabelObj->xpos[i] = xpos[i];
        newLabelObj->ypos[i] = ypos[i];
    }
}

psEffectObj* psEffectObjLabel::Clone() const
{
    //printf("label: cloning\n");
    psEffectObjLabel* newObj = new psEffectObjLabel(view, renderer2d);
    CloneBase(newObj);

    return newObj;
}

bool psEffectObjLabel::SetText(const csArray<psEffectTextElement> & /*elements*/)
{
    Error1("settext <array> not supported");
    return false;
}

bool psEffectObjLabel::SetText(const csArray<psEffectTextRow> & /*rows*/)
{
    Error1("settext <textrow> not supported");
    return false;
}

bool psEffectObjLabel::SetText(int rows, ...)
{
    size_t lettercount = 0;

    static csArray<psEffectTextElement> elemBuffer;
    psEffectTextElement newElem;
    psEffectTextRow* row = 0;
    int y = 0;

    // calculate dimensions of text area
    int maxWidth = 0;
    int maxHeight = 0;

    elemBuffer.Empty();

    va_list arg;
    va_start(arg, rows);

    // Loop through all rows
    for(int a=0; a<rows; a++)
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
        newElem.width = 0;
        newElem.height = 0;
        for(uint b=0; b<newElem.text.Length(); b++)
        {

            if(newElem.text[b] >= NUM_GLYPHS)
            {
                newElem.text[b] = 0;
                Error2("UNSUPPORTED CHARACTERS: %s\n", newElem.text.GetData());
            }

            newElem.width += width[newElem.text[b]];
            if(newElem.height < height[newElem.text[b]])
            {
                newElem.height = height[newElem.text[b]];
            }
        }
        newElem.x = 0;
        newElem.y = y;
        elemBuffer.Push(newElem);
        y += newElem.height;
        lettercount += newElem.text.Length();

        int right = newElem.x + newElem.width;
        int bottom = newElem.y + newElem.height;

        if(right > maxWidth)
            maxWidth = right;
        if(bottom > maxHeight)
            maxHeight = bottom;
    }
    va_end(arg);

    CS_ASSERT(facState!=0);
    iMeshObjectFactory* fact = meshFact->GetMeshObjectFactory();
    int mw,mh;
    fact->GetMaterialWrapper()->GetMaterial()->GetTexture()->GetOriginalDimensions(mw, mh);

    facState->SetVertexCount((int)lettercount * 4);
    facState->SetTriangleCount((int)lettercount * 2);

    size_t elementCount = elemBuffer.GetSize();
    int cp = 0;

    for(size_t i=0; i<elementCount; i++)
    {
        int x = elemBuffer[i].width;

        switch(elemBuffer[i].align)
        {
            case ETA_LEFT:
                x = maxWidth;
                break;
            case ETA_CENTER:
                x = (maxWidth-x)/2;
                break;
            case ETA_RIGHT:
                x = maxWidth-x;
                break;
        }
        int y = elemBuffer[i].y;
        csString text = elemBuffer[i].text;
        float scalefactor = labelwidth / (float)maxWidth;
        for(size_t j=0; j<text.Length(); j++)
        {
            uint c = text.GetAt(j);
            if(c >= NUM_GLYPHS)
            {
                c = 0;
                Error2("UNSUPPORTED CHARACTERS: %s\n", text.GetData());
            }

            float fx1, fy1, fx2, fy2;

            fx1 = (float)x * scalefactor - labelwidth/2.0;
            fy1 = (float)(maxHeight/2 - y) * scalefactor;
            fx2 = (float)(x+width[c]) * scalefactor - labelwidth/2.0;
            fy2 = (float)(maxHeight/2 - y + height[c]) * scalefactor;
            //printf("rendering char %d pos %d,%d w/h %d,%d xp/yp %d,%d - %f %f %f %f\n", cp/4, x, y, width[c], height[c], xpos[c], ypos[c], fx1, fy1, fx2, fy2);
            facState->GetVertices()[cp  ].Set(fx2,0,fy1);
            facState->GetVertices()[cp+1].Set(fx1,0,fy1);
            facState->GetVertices()[cp+2].Set(fx1,0,fy2);
            facState->GetVertices()[cp+3].Set(fx2,0,fy2);
            //printf("character %c: x %d y %d w %d\n", c, xpos[c], ypos[c], width[c]);
            float fracx1 = (float)xpos[c] / mw;
            float fracy1 = (float)ypos[c] / mh;
            float fracx2 = (float)(xpos[c]+width[c]) / mw;
            float fracy2 = (float)(ypos[c]+height[c]) / mh;
            //printf("texels %c %d,%d - %f %f %f %f\n", text[j], xpos[c], ypos[c], fracx1, fracy1, fracx2, fracy2);
            facState->GetTexels()[cp  ].Set(fracx2, fracy2);
            facState->GetTexels()[cp+1].Set(fracx1, fracy2);
            facState->GetTexels()[cp+2].Set(fracx1, fracy1);
            facState->GetTexels()[cp+3].Set(fracx2, fracy1);
            facState->GetNormals()[0].Set(0,1,0);
            facState->GetNormals()[1].Set(0,1,0);
            facState->GetNormals()[2].Set(0,1,0);
            facState->GetNormals()[3].Set(0,1,0);
            facState->GetTriangles()[(cp/2)  ].Set(cp  , cp+2, cp+3);
            facState->GetTriangles()[(cp/2)+1].Set(cp+2, cp  , cp+1);
            cp += 4;
            x += width[c];
        }
        int colour = elemBuffer[i].colour;
        csVector3 color((float)((colour>>16) & 255)/255.0F,
                        (float)((colour>> 8) & 255)/255.0F,
                        (float)((colour) & 255)/255.0F);
        CS::ShaderVarStringID varName = stringSet->Request("color modulation");
        csShaderVariable* var = mesh->GetSVContext()->GetVariableAdd(varName);
        if(var)
        {
            var->SetValue(color);
        }
    }
    facState->CalculateNormals();
//    facState->SetLighting(false);
//    fact->SetMixMode(CS_FX_ADD);
    return true;
}
