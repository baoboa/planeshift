/*
 *  loader.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008-10 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cssysdef.h>
#include <cstool/collider.h>
#include <cstool/enginetools.h>
#include <cstool/vfsdirchange.h>
#include <csutil/scanstr.h>
#include <csutil/scfstringarray.h>
#include <iengine/camera.h>
#include <iengine/movable.h>
#include <iengine/portal.h>
#include <imap/services.h>
#include <imesh/object.h>
#include <iutil/cfgmgr.h>
#include <iutil/document.h>
#include <iutil/object.h>
#include <iutil/plugin.h>
#include <ivaria/collider.h>
#include <ivaria/engseq.h>
#include <ivideo/graph2d.h>
#include <ivideo/material.h>

#include "util/psconst.h"
#include "loader.h"

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
    void BgLoader::ParseShaders()
    {
        if(!parserData.config.parseShaders || parserData.config.parsedShaders)
            return;

        csRef<iConfigManager> config = csQueryRegistry<iConfigManager> (object_reg);

        // Parse basic set of shaders.
        csString shaderList = config->GetStr("PlaneShift.Loading.ShaderList", "/shader/shaderlist.xml");
        if(vfs->Exists(shaderList))
        {
            csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
            csRef<iDocument> doc = docsys->CreateDocument();
            csString path;
            {
                CS::Threading::RecursiveMutexScopedLock lock(vfsLock);
                csRef<iDataBuffer> data = vfs->ReadFile(shaderList);
                doc->Parse(data, true);
                path = vfs->GetCwd();
            }

            if(doc->GetRoot())
            {
                csRef<iDocumentNode> node = doc->GetRoot()->GetNode("shaders");
                if(node.IsValid())
                {
                    csRefArray<iThreadReturn> rets;

                    csRef<iDocumentNodeIterator> nodeItr = node->GetNodes("shader");
                    while(nodeItr->HasNext())
                    {
                        csRef<iDocumentNode> shaderNode(nodeItr->Next());

                        const char* typeName = shaderNode->GetNode("type")->GetContentsValue();
                        const char* shaderName = shaderNode->GetAttributeValue("name");

                        csRef<iDocumentNode> file = shaderNode->GetNode("file");
                        if(file.IsValid())
                        {
                            const char* fileName = file->GetContentsValue();
                            if(parserData.shaders.Contains(fileName) == csArrayItemNotFound)
                            {
                                CS::Threading::ScopedWriteLock lock(parserData.shaderLock);
                                parserData.shaders.Push(fileName);
                                csRef<iThreadReturn> shaderRet = tloader->LoadShader(path, fileName);
                                if(parserData.config.blockShaderLoad)
                                {
                                    shaderRet->Wait();
                                }
                                else
                                {
                                    // Dispatch shader load to a thread.
                                    rets.Push(shaderRet);
                                }
                            }

                            csStringID shaderID = parserData.strings->Request(typeName);
                            parserData.shadersByUsage.Put(shaderID, shaderName);
                        }
                    }
                    
                    nodeItr = node->GetNodes("alias");
                    while(nodeItr->HasNext())
                    {
                        csRef<iDocumentNode> aliasNode(nodeItr->Next());
                        const char* shaderName = aliasNode->GetAttributeValue("name");
                        csString aliasName(aliasNode->GetAttributeValue("alias"));

                        csRef<iDocumentNode> file = aliasNode->GetNode("file");
                        if(file.IsValid())
                        {
                            const char* fileName = file->GetContentsValue();
                            if(parserData.shaders.Contains(fileName) == csArrayItemNotFound)
                            {
                                CS::Threading::ScopedWriteLock lock(parserData.shaderLock);
                                parserData.shaders.Push(fileName);
                                csRef<iThreadReturn> shaderRet = tloader->LoadShader(path, fileName);
                                if(parserData.config.blockShaderLoad)
                                {
                                    shaderRet->Wait();
                                }
                                else
                                {
                                    // Dispatch shader load to a thread.
                                    rets.Push(shaderRet);
                                }
                            }

                            parserData.shaderAliases.Put(aliasName, shaderName);
                        }
                    }

                    // Wait for shader loads to finish.
                    tman->Wait(rets);
                }
            }
        }

        parserData.config.parsedShaders = true;
    }

    bool BgLoader::Texture::Parse(iDocumentNode* node, ParserData& parserData)
    {
        SetName(node->GetAttributeValue("name"));
        data = node;
        path = parserData.vfsPath;
        return true;
    }

    template<bool check> bool BgLoader::ShaderVar::Parse(iDocumentNode* node, GlobalParserData& data)
    {
        // Parse the different types. Currently texture, vector2 and vector3 are supported.
        csString typeName;
        csString name;
        if(csString("texture").Compare(node->GetValue()))
        {
            typeName = "texture";
            name = "tex diffuse";
        }
        else
        {
            typeName = node->GetAttributeValue("type");
            name = node->GetAttributeValue("name");
        }
        nameID = data.svstrings->Request(name);
        value = node->GetContentsValue();

        if(check && !data.config.parseShaderVars)
        {
            if(typeName != "texture" || name != "tex diffuse")
            {
                return false;
            }
        }

        if(typeName == "texture")
        {
            type = csShaderVariable::TEXTURE;

            // Ignore some shader variables if the functionality they bring is not enabled.
            if(check)
            {
                switch(data.config.enabledGfxFeatures)
                {
                    case useLowestShaders:
                        // Fall through to check that no tex with higher value is defined.
                    case useLowShaders:
                        if(name == "tex normal" || name == "tex normal compressed")
                        {
                            return false;
                        }
                        // Fall through to check that no tex with higher value is defined.
                    case useMediumShaders:
                        if(name == "tex height" || name == "tex ambient occlusion")
                        {
                            return false;
                        }
                        // Fall through to check that no tex with higher value is defined.
                    case useHighShaders:
                        if(name == "tex specular")
                        {
                            return false;
                        }
                        // Fall through to check that no tex with higher value is defined.
                    case useHighestShaders:
                        // This is the highest so name is ok.
                        break;
                }

                // filter out standard textures we don't have to load and a special texture
                // tht is used for undefined materials
                if(value == "materialnotdefined" || !data.config.parseShaderVars)
                {
                    value = "standardtex white";
                }
            }

            if(value.StartsWith("stdtex") || value.StartsWith("standardtex"))
            {
                return true;
            }

            csRef<Texture> texture = data.textures.Get(value);

            if(texture.IsValid())
            {
                object->AddDependency(texture);
            }
            else
            {
                csString msg;
                msg.Format("Invalid texture reference '%s' in ShaderVar '%s'", value.GetData(), name.GetData());
                CS_ASSERT_MSG(msg.GetData(), false);
                return false;
            }
        }
        else if(typeName == "float")
        {
            type = csShaderVariable::FLOAT;
            vec1 = node->GetContentsValueAsFloat();
        }
        else if(typeName == "vector2")
        {
            type = csShaderVariable::VECTOR2;
            csScanStr(node->GetContentsValue(), "%f,%f", &vec2.x, &vec2.y);
        }
        else if(typeName == "vector3")
        {
            type = csShaderVariable::VECTOR3;
            csScanStr(node->GetContentsValue(), "%f,%f,%f", &vec3.x, &vec3.y, &vec3.z);
        }
        else if(typeName == "vector4")
        {
            type = csShaderVariable::VECTOR4;
            csScanStr(node->GetContentsValue(), "%f,%f,%f,%f", &vec4.x, &vec4.y, &vec4.z, &vec4.w);
        }
        else
        {
            // unknown type
            csString msg;
            msg.Format("Unknown variable type in shadervar '%s': '%s'", name.GetData(), typeName.GetData());
            CS_ASSERT_MSG(msg.GetData(), false);
            return false;
        }
        return true;
    }

    // explicitly instantiate template as we defined, but not declared it in the header
    template bool BgLoader::ShaderVar::Parse<true>(iDocumentNode*, GlobalParserData&);
    template bool BgLoader::ShaderVar::Parse<false>(iDocumentNode*, GlobalParserData&);

    bool BgLoader::Material::Parse(iDocumentNode* materialNode, GlobalParserData& parserData)
    {
        SetName(materialNode->GetAttributeValue("name"));

        csRef<iShaderManager> shaderMgr = csQueryRegistry<iShaderManager> (parserData.object_reg);

        // Parse the texture for a material. Construct a shader variable for it.
        csRef<iDocumentNodeIterator> nodeIt = materialNode->GetNodes();
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = parserData.xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_TEXTURE:
                case PARSERTOKEN_SHADERVAR:
                {
                    csRef<ShaderVar> sv;
                    sv.AttachNew(new ShaderVar(GetParent(), this));
                    if(sv->Parse<true>(node, parserData))
                    {
                        shadervars.Push(sv);
                    }
                }
                break;

                case PARSERTOKEN_SHADER:
                {
                    csString shaderName(node->GetContentsValue());
                    {
                        csString oldShaderName(shaderName);

                        CS::Threading::ScopedReadLock lock(parserData.shaderLock);
                        if(parserData.shaderAliases.Contains(shaderName))
                        {
                            shaderName = parserData.shaderAliases.Get(oldShaderName, oldShaderName);
                        }
                    }
                    Shader shader;
                    shader.shader = shaderMgr->GetShader(shaderName);
                    shader.type = parserData.strings->Request(node->GetAttributeValue("type"));
                    shaders.Push(shader);
                }
                break;
            }
        }
        return true;
    }

    void BgLoader::ParseMaterials(iDocumentNode* materialsNode)
    {
        csRef<iDocumentNodeIterator> it = materialsNode->GetNodes("material");
        while(it->HasNext())
        {
            csRef<iDocumentNode> materialNode = it->Next();
            csRef<Material> m;
            m.AttachNew(new Material(this));
            if(m->Parse(materialNode, parserData))
            {
                parserData.materials.Put(m);
            }
        }
    }

    void BgLoader::MaterialLoader::ParseMaterialReference(GlobalParserData& data, const char* name, const char* type, const char* parentName)
    {
        csRef<Material> material= data.materials.Get(name);

        if(material.IsValid())
        {
            AddDependency(material);
        }
        else
        {
            // Validation.
            csString msg;
            msg.Format("Invalid material reference '%s' in %s", name, type);
            if(parentName)
            {
                msg.AppendFmt(" '%s'", parentName);
            }
            CS_ASSERT_MSG(msg.GetData(), false);
        }
    }

    bool BgLoader::Portal::Parse(iDocumentNode* portalNode, ParserData& parserData)
    {
        SetName(portalNode->GetAttributeValue("name"));
        sector = parserData.currentSector;

        csRef<iSyntaxService> syntaxService = parserData.data.syntaxService;

        csRef<iDocumentNodeIterator> nodeIt(portalNode->GetNodes());
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = parserData.data.xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_MATRIX:
                {
                    warp = true;
                    syntaxService->ParseMatrix(node, matrix);
                }
                break;

                case PARSERTOKEN_WV:
                {
                    warp = true;
                    syntaxService->ParseVector(node, wv);
                }
                break;

                case PARSERTOKEN_WW:
                {
                    warp = true;
                    ww_given = true;
                    syntaxService->ParseVector(node, ww);
                }
                break;

                case PARSERTOKEN_FLOAT:
                {
                    flags |= CS_PORTAL_FLOAT;
                }
                break;

                case PARSERTOKEN_CLIP:
                {
                    flags |= CS_PORTAL_CLIPDEST;
                }
                break;

                case PARSERTOKEN_ZFILL:
                {
                    flags |= CS_PORTAL_ZFILL;
                }
                break;

                case PARSERTOKEN_AUTORESOLVE:
                {
                    autoresolve = true;
                }
                break;

                case PARSERTOKEN_SECTOR:
                {
                    const char* sectorName = node->GetContentsValue();
                    targetSector = csRef<Sector>(parserData.data.sectors.Get(sectorName));

                    if(!targetSector)
                    {
                        csRef<Sector> newSector;
                        newSector.AttachNew(new Sector(GetParent()));

                        parserData.data.sectors.Put(newSector, sectorName);
                        targetSector = newSector;
                    }
                }
                break;

                case PARSERTOKEN_V:
                {
                    csVector3 vertex;
                    syntaxService->ParseVector(node, vertex);
                    poly.AddVertex(vertex);
                    bbox.AddBoundingVertex(vertex);
                }
                break;
            }
        }

        if(!ww_given)
        {
            ww = wv;
        }

        transform = csReversibleTransform(matrix.GetInverse(), ww - matrix * wv);

        return true;
    }

    bool BgLoader::Light::Parse(iDocumentNode* lightNode, ParserData& parserData)
    {
        SetName(lightNode->GetAttributeValue("name"));
        sector = parserData.currentSector;

        // set default values
        attenuation = CS_ATTN_LINEAR;
        dynamic = CS_LIGHT_DYNAMICTYPE_STATIC;
        type = CS_LIGHT_POINTLIGHT;

        csRef<iDocumentNodeIterator> nodeIt(lightNode->GetNodes());
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = parserData.data.xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_ATTENUATION:
                {
                    csStringID type = parserData.data.xmltokens.Request(node->GetContentsValue());
                    switch(type)
                    {
                        case PARSERTOKEN_NONE:
                        {
                            attenuation = CS_ATTN_NONE;
                        }
                        break;

                        case PARSERTOKEN_LINEAR:
                        {
                            attenuation = CS_ATTN_LINEAR;
                        }
                        break;

                        case PARSERTOKEN_INVERSE:
                        {
                            attenuation = CS_ATTN_INVERSE;
                        }
                        break;

                        case PARSERTOKEN_REALISTIC:
                        {
                            attenuation = CS_ATTN_REALISTIC;
                        }
                        break;

                        case PARSERTOKEN_CLQ:
                        {
                            attenuation = CS_ATTN_CLQ;
                        }
                        break;
                    }
                }
                break;

                case PARSERTOKEN_DYNAMIC:
                {
                    dynamic = CS_LIGHT_DYNAMICTYPE_PSEUDO;
                }
                break;

                case PARSERTOKEN_CENTER:
                {
                    parserData.data.syntaxService->ParseVector(node, pos);
                    bbox.SetCenter(pos);
                }
                break;
                
                case PARSERTOKEN_RADIUS:
                {
                    radius = node->GetContentsValueAsFloat();
                    bbox.SetSize(csVector3(radius));
                }
                break;

                case PARSERTOKEN_COLOR:
                {
                    parserData.data.syntaxService->ParseColor(node, colour);
                }
                break;
            }
        }

        return true;
    }

    bool BgLoader::Sector::Parse(iDocumentNode* sectorNode, ParserData& parserData)
    {
        SetName(sectorNode->GetAttributeValue("name"));

        parent = parserData.zone;
        parserData.zone->AddDependency(this);

        // let the other parser functions know about us
        parserData.currentSector = this;

        // check whether we want to force a specific culler
        if(parserData.data.config.forceCuller)
        {
            culler = parserData.data.config.culler;
        }

        // set this sector as initialized
        init = true;

        bool portalsOnly = parserData.data.config.portalsOnly;
        bool meshesOnly = parserData.data.config.meshesOnly;

        csRef<iDocumentNodeIterator> it(sectorNode->GetNodes());
        while(it->HasNext())
        {
            csRef<iDocumentNode> node(it->Next());
            csStringID id = parserData.data.xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_CULLERP:
                {
                    if(!parserData.data.config.forceCuller)
                    {
                        culler = node->GetContentsValue();
                    }
                }
                break;

                case PARSERTOKEN_AMBIENT:
                {
                    ambient = csColor(node->GetAttributeValueAsFloat("red"),
                                      node->GetAttributeValueAsFloat("green"),
                                      node->GetAttributeValueAsFloat("blue"));
                }
                break;

                case PARSERTOKEN_KEY:
                {
                    if(meshesOnly)
                    {
                        break;
                    }

                    if(csString("water").Compare(node->GetAttributeValue("name")))
                    {
                        csRef<iDocumentNodeIterator> areasIt(node->GetNodes("area"));
                        while(areasIt->HasNext())
                        {
                            csRef<iDocumentNode> areaNode(areasIt->Next());
                            WaterArea area;
                            csRef<iDocumentNode> colourNode = areaNode->GetNode("colour");
                            if(colourNode.IsValid())
                            {
                                parserData.data.syntaxService->ParseColor(colourNode, area.colour);
                            }
                            else
                            {
                                area.colour = csColor4(0.0f, 0.17f, 0.49f, 0.6f);
                            }

                            csRef<iDocumentNodeIterator> vs = areaNode->GetNodes("v");
                            while(vs->HasNext())
                            {
                                csRef<iDocumentNode> v(vs->Next());
                                csVector3 vector;
                                parserData.data.syntaxService->ParseVector(v, vector);
                                area.bbox.AddBoundingVertex(vector);
                            }

                            waterareas.Push(area);
                        }
                    }
                }
                break;

                case PARSERTOKEN_PORTALS:
                {
                    csRef<iDocumentNode> renderDistNode = node->GetNode("maxrenderdist");
                    float renderDist = 0.f;
                    if(renderDistNode.IsValid())
                    {
                        renderDist = renderDistNode->GetAttributeValueAsFloat("value");
                    }
                    
                    csRef<iDocumentNodeIterator> it(node->GetNodes("portal"));
                    while(it->HasNext())
                    {
                        csRef<iDocumentNode> portalNode(it->Next());
                        csRef<Portal> p;
                        p.AttachNew(new Portal(GetParent(), renderDist));
                        if(p->Parse(portalNode, parserData))
                        {
                            AddDependency(p);
                        }
                        else
                        {
                            csString msg;
                            msg.Format("Portal %s failed to parse!", portalNode->GetAttributeValue("name"));
                            CS_ASSERT_MSG(msg.GetData(), false);
                        }
                    }
                }
                break;

                case PARSERTOKEN_LIGHT:
                {
                    if(meshesOnly)
                    {
                        break;
                    }

                    csRef<Light> l;
                    l.AttachNew(new Light(GetParent()));
                    if(l->Parse(node, parserData))
                    {
                        AddDependency(l);

                        parserData.lights.Put(l);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Light %s failed to parse!", node->GetAttributeValue("name"));
                        CS_ASSERT_MSG(msg.GetData(), false);
                    }
                }
                break;

                case PARSERTOKEN_MESHOBJ:
                case PARSERTOKEN_TRIMESH:
                {
                    if(portalsOnly)
                    {
                        break;
                    }

                    csRef<MeshObj> mesh;
                    mesh.AttachNew(new MeshObj(GetParent()));
                    bool alwaysLoaded;
                    if((id == PARSERTOKEN_MESHOBJ && mesh->Parse(node, parserData, alwaysLoaded))
                        || (id == PARSERTOKEN_TRIMESH && mesh->ParseTriMesh(node, parserData, alwaysLoaded)))
                    {
                        if(alwaysLoaded)
                        {
                            AddAlwaysLoaded(mesh);
                        }
                        else
                        {
                            AddDependency(mesh);
                        }

                        parserData.data.meshes.Put(mesh);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Mesh %s failed to parse!", node->GetAttributeValue("name"));
                        CS_ASSERT_MSG(msg.GetData(), false);
                    }
                }
                break;

                case PARSERTOKEN_MESHGEN:
                {
                    if(portalsOnly || !(parserData.data.config.enabledGfxFeatures & useMeshGen))
                    {
                        break;
                    }

                    csRef<MeshGen> meshGen;
                    meshGen.AttachNew(new MeshGen(GetParent()));
                    if(meshGen->Parse(node, parserData))
                    {
                        AddDependency(meshGen);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("MeshGen %s failed to parse!", node->GetAttributeValue("name"));
                        CS_ASSERT_MSG(msg.GetData(), false);
                    }
                }
                break;
            }
        }

        return true;
    }

    bool BgLoader::Sequence::Parse(iDocumentNode* sequenceNode, ParserData& parserData)
    {
        SetName(sequenceNode->GetAttributeValue("name"));
        data = sequenceNode;
        path = parserData.vfsPath;

        bool loaded = false;
        bool failed = false;

        // used to work around ambigious call issues
        csRef<Sequence> self(this);

        // objects we already added ourself as dependency to
        // used in case we fail to parse
        csArray<ObjectLoader<Sequence>* > parents;

        csRef<iDocumentNodeIterator> nodeIt(sequenceNode->GetNodes());
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = parserData.data.xmltokens.Request(node->GetValue());
            bool parsed = true;
            switch(id)
            {
                // sequence types operating on a sector.
                case PARSERTOKEN_SETAMBIENT:
                case PARSERTOKEN_FADEAMBIENT:
                case PARSERTOKEN_SETFOG:
                case PARSERTOKEN_FADEFOG:
                {
                    const char* name = node->GetAttributeValue("sector");
                    csRef<Sector> s = parserData.data.sectors.Get(name);

                    if(s.IsValid())
                    {
                        s->AddDependency(self);
                        parents.Push(s);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid sector reference '%s' in sequence '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // sequence types operating on a mesh.
                case PARSERTOKEN_SETCOLOR:
                case PARSERTOKEN_FADECOLOR:
                case PARSERTOKEN_MATERIAL:
                case PARSERTOKEN_ROTATE:
                case PARSERTOKEN_MOVE:
                {
                    const char* name = node->GetAttributeValue("mesh");
                    csRef<MeshObj> m = parserData.data.meshes.Get(name);

                    if(m.IsValid())
                    {
                        AddDependency(m);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid mesh reference '%s' in sequence '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // sequence types operating on a light.
                case PARSERTOKEN_ROTATELIGHT:
                case PARSERTOKEN_MOVELIGHT:
                case PARSERTOKEN_FADELIGHT:
                case PARSERTOKEN_SETLIGHT:
                {
                    const char* name = node->GetAttributeValue("light");
                    csRef<Light> l = parserData.lights.Get(name);

                    if(l.IsValid())
                    {
                        AddDependency(l);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid light reference '%s' in sequence '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // sequence types operating on a sequence.
                case PARSERTOKEN_RUN:
                {
                    const char* name = node->GetAttributeValue("sequence");
                    csRef<Sequence> seq = parserData.sequences.Get(name);

                    if(seq.IsValid())
                    {
                        seq->AddDependency(self);
                        parents.Push(seq);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid sequence reference '%s' in sequence '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // sequence types operating on a trigger.
                case PARSERTOKEN_ENABLE:
                case PARSERTOKEN_DISABLE:
                case PARSERTOKEN_CHECK:
                case PARSERTOKEN_TEST:
                {
                    const char* name = node->GetAttributeValue("trigger");
                    csRef<Trigger> t = parserData.triggers.Get(name);

                    if(t.IsValid())
                    {
                        AddDependency(t);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid trigger reference '%s' in sequence '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // miscellanous sequence types.
                case PARSERTOKEN_DELAY:
                case PARSERTOKEN_SETVAR:
                default:
                {
                    parsed = false;
                }
                break;
            }
            loaded |= parsed;
        }

        if(failed)
        {
            // failed to parse, remove us as dependency from parents
            for(size_t i = 0; i < parents.GetSize(); ++i)
            {
                parents[i]->RemoveDependency(self);
            }

            return false;
        }

        CS_ASSERT_MSG("Unknown sequence type!", loaded);
        return loaded;
    }

    bool BgLoader::Trigger::Parse(iDocumentNode* triggerNode, ParserData& parserData)
    {
        SetName(triggerNode->GetAttributeValue("name"));
        data = triggerNode;
        path = parserData.vfsPath;
        bool loaded = false;
        bool failed = false;

        // used to work around issues with ambigious calls
        csRef<Trigger> self(this);

        // objects we already added ourself as dependency to
        // used in case we fail to parse
        csArray<ObjectLoader<Trigger>* > parents;

        csRef<iDocumentNodeIterator> nodeIt(triggerNode->GetNodes());
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = parserData.data.xmltokens.Request(node->GetValue());
            bool parsed = true;
            switch(id)
            {
                // triggers fired by a sector.
                case PARSERTOKEN_SECTORVIS:
                {
                    const char* name = node->GetAttributeValue("sector");
                    csRef<Sector> s = parserData.data.sectors.Get(name);

                    if(s.IsValid())
                    {
                        s->AddDependency(self);
                        parents.Push(s);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid sector reference '%s' in trigger '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // triggers fired by a mesh.
                case PARSERTOKEN_ONCLICK:
                {
                    const char* name = node->GetAttributeValue("mesh");
                    csRef<MeshObj> m = parserData.data.meshes.Get(name);

                    if(m.IsValid())
                    {
                        AddDependency(m);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid mesh reference '%s' in trigger '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // triggers fired by a light.
                case PARSERTOKEN_LIGHTVALUE:
                {
                    const char* name = node->GetAttributeValue("light");
                    csRef<Light> l = parserData.lights.Get(name);

                    if(l.IsValid())
                    {
                        AddDependency(l);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid light reference '%s' in trigger '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // triggers fired by a sequence.
                case PARSERTOKEN_FIRE:
                {
                    const char* name = node->GetAttributeValue("sequence");
                    csRef<Sequence> seq = parserData.sequences.Get(name);

                    if(seq.IsValid())
                    {
                        // Since sequences are parsed first, if it has
                        // a dependency on this trigger, there is a loop.
                        if(seq->ObjectLoader<Trigger>::HasDependency(GetName()))
                        {
                            seq->ObjectLoader<Trigger>::RemoveDependency(this);
                        }
                        AddDependency(seq);
                    }
                    else
                    {
                        csString msg;
                        msg.Format("Invalid sequence reference '%s' in trigger '%s'", name, GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                        failed = true;
                    }
                }
                break;

                // triggers fired by a miscellanous operation.
                case PARSERTOKEN_MANUAL:
                default:
                {
                    parsed = false;
                }
                break;
            }

            loaded |= parsed;
        }

        if(failed)
        {
            // failed to parse, remove us as dependency from parents
            for(size_t i = 0; i < parents.GetSize(); ++i)
            {
                parents[i]->RemoveDependency(self);
            }

            return false;
        }

        CS_ASSERT_MSG("Unknown trigger type!", loaded);
        return loaded;
    }

    THREADED_CALLABLE_IMPL1(BgLoader, PrecacheData, const char* path)
    {
        (void)sync; // prevent unused variable warning

        // Make sure shaders are parsed at this point.
        ParseShaders();

        ParserData data(parserData);
        data.path = path;

        // Don't parse folders.
        data.vfsPath = path;
        if(data.vfsPath.GetAt(data.vfsPath.Length()-1) == '/')
            return false;

        if(vfs->Exists(data.vfsPath))
        {
            csRef<iDocumentNode> root;
            {
                CS::Threading::RecursiveMutexScopedLock lock(vfsLock);

                // Restores any directory changes.
                csVfsDirectoryChanger dirchange(vfs);

                // XML doc structures.
                csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(object_reg);
                csRef<iDocument> doc = docsys->CreateDocument();
                csRef<iDataBuffer> buffer = vfs->ReadFile(data.path);
                if(!buffer.IsValid())
                    return false;

                doc->Parse(buffer, true);

                // Check that it's an xml file.
                if(!doc->GetRoot())
                    return false;

                dirchange.ChangeTo(data.vfsPath.Truncate(data.vfsPath.FindLast('/')+1));

                // Begin document parsing.
                data.realRoot = false;
                root = doc->GetRoot()->GetNode("world");
                if(!root.IsValid())
                {
                    root = doc->GetRoot()->GetNode("library");
                    if(!root)
                    {
                        data.realRoot = true;
                        root = doc->GetRoot();
                    }
                }
                else
                {
                    csString zonen(path);
                    zonen = zonen.Slice(zonen.FindLast('/')+1);
                    csRef<Zone> zone;
                    zone.AttachNew(new Zone(this, zonen));
                    data.zone = zone;

                    parserData.zones.Put(zone);
                }
            }

            if(root.IsValid())
            {
                bool portalsOnly = parserData.config.portalsOnly;
                bool meshesOnly = parserData.config.meshesOnly;
                data.parsedMeshFact = false;
                csRef<iDocumentNode> sequences;
                csRef<iDocumentNode> triggers;

                csRef<iDocumentNodeIterator> nodeIt(root->GetNodes());
                while(nodeIt->HasNext())
                {
                    csRef<iDocumentNode> node(nodeIt->Next());
                    csStringID id = parserData.xmltokens.Request(node->GetValue());
                    switch(id)
                    {
                        // Parse referenced libraries.
                        case PARSERTOKEN_LIBRARY:
                        {
                            csString target(data.vfsPath);
                            target.Append(node->GetContentsValue());
                            PrecacheDataTC(ret, false, target);
                        }
                        break;

                        // Parse needed plugins.
                        case PARSERTOKEN_PLUGINS:
                        {
                            data.rets.Push(tloader->LoadNode(data.vfsPath, node));
                        }
                        break;

                        // Parse referenced shaders.
                        case PARSERTOKEN_SHADERS:
                        {
                            csRef<iDocumentNodeIterator> shaderIt(node->GetNodes("shader"));
                            while(shaderIt->HasNext())
                            {
                                csRef<iDocumentNode> shader(shaderIt->Next());

                                csRef<iDocumentNode> fileNode(shader->GetNode("file"));
                                if(fileNode.IsValid())
                                {
                                    bool loadShader = false;
                                    const char* fileName = fileNode->GetContentsValue();

                                    CS::Threading::ScopedWriteLock lock(parserData.shaderLock);

                                    // Keep track of shaders that have already been parsed.
                                    if(parserData.shaders.Contains(fileName) == csArrayItemNotFound)
                                    {
                                        parserData.shaders.Push(fileName);
                                        loadShader = true;
                                    }

                                    if(loadShader && parserData.config.parseShaders)
                                    {
                                        // Dispatch shader load to a thread.
                                        csRef<iThreadReturn> shaderRet = tloader->LoadShader(data.vfsPath, fileName);
                                        if(parserData.config.blockShaderLoad)
                                        {
                                            shaderRet->Wait();
                                        }
                                        else
                                        {
                                            data.rets.Push(shaderRet);
                                        }
                                    }
                                }
                            }
                        }
                        break;

                        // Parse all referenced textures.
                        case PARSERTOKEN_TEXTURES:
                        {
                            if(portalsOnly)
                            {
                                break;
                            }

                            csRef<iDocumentNodeIterator> textureIt(node->GetNodes("texture"));
                            while(textureIt->HasNext())
                            {
                                csRef<iDocumentNode> textureNode(textureIt->Next());
                                csRef<Texture> t;
                                t.AttachNew(new Texture(this));
                                if(t->Parse(textureNode, data))
                                {
                                    parserData.textures.Put(t);
                                }
                                else
                                {
                                    csString msg;
                                    msg.Format("Texture %s failed to parse!", textureNode->GetAttributeValue("name"));
                                    CS_ASSERT_MSG(msg.GetData(), false);
                                }
                            }
                        }
                        break;

                        // Parse all referenced materials.
                        case PARSERTOKEN_MATERIALS:
                        {
                            if(portalsOnly)
                            {
                                break;
                            }

                            ParseMaterials(node);
                        }
                        break;

                        // Parse mesh factory.
                        case PARSERTOKEN_MESHFACT:
                        {
                            if(portalsOnly)
                            {
                                break;
                            }

                            csRef<MeshFact> factory;
                            factory.AttachNew(new MeshFact(this));
                            if(factory->Parse(node, data))
                            {
                                parserData.factories.Put(factory);
                            }
                            else
                            {
                                csString msg;
                                msg.Format("Mesh factory %s failed to parse!", node->GetAttributeValue("name"));
                                CS_ASSERT_MSG(msg.GetData(), false);
                            }
                        }
                        break;

                        // Parse sector.
                        case PARSERTOKEN_SECTOR:
                        {
                            const char* name = node->GetAttributeValue("name");
                            csRef<Sector> sector = parserData.sectors.Get(name);
                            if(!sector.IsValid())
                            {
                                sector.AttachNew(new Sector(this));
                                parserData.sectors.Put(sector, name);
                            }

                            if(!sector->Parse(node, data))
                            {
                                parserData.sectors.Delete(name);

                                csString msg;
                                msg.Format("Sector %s failed to parse!", name);
                                CS_ASSERT_MSG(msg.GetData(), false);
                            }
                        }
                        break;

                        // Parse start position.
                        case PARSERTOKEN_START:
                        {
                            csRef<StartPosition> startPos;
                            startPos.AttachNew(new StartPosition());
                            startPos->name = node->GetAttributeValue("name");
                            startPos->zone = data.zone->GetName();
                            startPos->sector = node->GetNode("sector")->GetContentsValue();
                            parserData.syntaxService->ParseVector(node->GetNode("position"), startPos->position);

                            parserData.positions.Put(startPos);
                        }
                        break;

                        case PARSERTOKEN_SEQUENCES:
                        {
                            if(meshesOnly)
                            {
                                break;
                            }
                            sequences = node;
                        }
                        break;

                        case PARSERTOKEN_TRIGGERS:
                        {
                            if(meshesOnly)
                            {
                                break;
                            }
                            triggers = node;
                        }
                        break;
                    }
                }
                // Sequences and triggers are parsed at the end because
                // all sectors and other objects need to be present.
                if (!LoadSequencesAndTriggers(sequences, triggers, data))
                    return false;
            }

            // Wait for plugin and shader loads to finish.
            tman->Wait(data.rets);
        }
        
        return true;
    }

  bool BgLoader::LoadSequencesAndTriggers (iDocumentNode* snode,
                                           iDocumentNode* tnode,
                                           ParserData& data)
  {
    // We load sequences and triggers in three passes.
    // In the first pass, we will create all triggers.
    // In the second pass, we will create and parse sequences.
    // In the third pass, we will parse the triggers.
    // This will create the data structures and dependencies but
    // not the actual Crystalspace engine data structures until
    // they are visible or needed.
    // It also makes sure that sequences are created before triggers
    // regardless of the order they may appear in the XML file.

    if (tnode)
    {
      csRef<iDocumentNodeIterator> it = tnode->GetNodes ();
      while (it->HasNext ())
      {
        csRef<iDocumentNode> child = it->Next ();
        if (child->GetType () != CS_NODE_ELEMENT)
          continue;
        const char* value = child->GetValue ();
        csStringID id = parserData.xmltokens.Request (value);
        switch (id)
        {
        case PARSERTOKEN_TRIGGER:
          {
            const char* name = child->GetAttributeValue ("name");
            csRef<Trigger> trig;
            trig.AttachNew(new Trigger(this));
            data.triggers.Put(trig, name);
            trig->SetName(name);
          }
          break;
        default:
          parserData.syntaxService->ReportBadToken (child);
          return false;
        }
      }
    }

    if (snode)
    {
      csRef<iDocumentNodeIterator> it = snode->GetNodes ();
      while (it->HasNext ())
      {
        csRef<iDocumentNode> child = it->Next ();
        if (child->GetType () != CS_NODE_ELEMENT)
          continue;
        const char* value = child->GetValue ();
        csStringID id = parserData.xmltokens.Request (value);
        switch (id)
        {
        case PARSERTOKEN_SEQUENCE:
          {
            const char* name = child->GetAttributeValue ("name");
            csRef<Sequence> seq;
            seq.AttachNew(new Sequence(this));
            data.sequences.Put(seq, name);
            if (!seq->Parse (child, data))
            {
              data.sequences.Delete(name);
              csString msg;
              msg.Format("Sequence %s failed to parse!", name);
              CS_ASSERT_MSG(msg.GetData(), false);
              return false;
            }
            break;
          }
        default:
          parserData.syntaxService->ReportBadToken (child);
          return false;
        }
      }
    }

    if (tnode)
    {
      csRef<iDocumentNodeIterator> it = tnode->GetNodes ();
      while (it->HasNext ())
      {
        csRef<iDocumentNode> child = it->Next ();
        if (child->GetType () != CS_NODE_ELEMENT)
          continue;
        const char* value = child->GetValue ();
        csStringID id = parserData.xmltokens.Request (value);
        switch (id)
        {
        case PARSERTOKEN_TRIGGER:
          {
            const char* name = child->GetAttributeValue ("name");
            csRef<Trigger> trig (data.triggers.Get(name));
            if (!trig->Parse (child, data))
            {
              // failed to parse the trigger, remove it from the lookup table
              data.triggers.Delete(name);
              csString msg;
              msg.Format("Trigger %s failed to parse!", name);
              CS_ASSERT_MSG(msg.GetData(), false);
              return false;
            }
            break;
          }
        default:
          parserData.syntaxService->ReportBadToken (child);
          return false;
        }
      }
    }

    return true;
  }

}
CS_PLUGIN_NAMESPACE_END(bgLoader)

