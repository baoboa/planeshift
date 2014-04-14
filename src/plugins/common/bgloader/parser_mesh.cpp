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

//#ifdef CS_DEBUG
#undef  CS_ASSERT_MSG
#define CS_ASSERT_MSG(msg, x) if(!x) printf("ART ERROR: %s\n", msg)
//#endif

CS_PLUGIN_NAMESPACE_BEGIN(bgLoader)
{
    bool BgLoader::MeshFact::Parse(iDocumentNode* meshfactNode, ParserData& parserData)
    {
         SetName(meshfactNode->GetAttributeValue("name"));
         path = parserData.vfsPath;

         if(!parserData.data.config.cache)
         {
             if(parserData.realRoot && !parserData.parsedMeshFact)
             {
                 // Load this file when needed to save memory.
                 filename = csString(parserData.path).Slice(csString(parserData.path).FindLast('/')+1);
             }

             // Mark that we've already loaded a meshfact in this file.
             parserData.parsedMeshFact = true;
         }

         if(!filename.GetData())
         {
             data = meshfactNode;
         }

         csRef<iDocumentNodeIterator> it(meshfactNode->GetNodes());
         while(it->HasNext())
         {
             csRef<iDocumentNode> node(it->Next()); 
             csStringID id = parserData.data.xmltokens.Request(node->GetValue());
             switch(id)
             {
                 case PARSERTOKEN_PARAMS:
                 {
                     csRef<iDocumentNodeIterator> paramsIt(node->GetNodes());
                     while(paramsIt->HasNext())
                     {
                         csRef<iDocumentNode> paramNode(paramsIt->Next());
                         csStringID param = parserData.data.xmltokens.Request(paramNode->GetValue());
                         switch(param)
                         {
                             case PARSERTOKEN_CELLS:
                             {
                                 if(!ParseCells(paramNode, parserData))
                                 {
                                     return false;
                                 }
                             }
                             break;

                             case PARSERTOKEN_MATERIAL:
                             {
                                 ParseMaterialReference(parserData.data, paramNode->GetContentsValue(),
                                                        "meshfact", GetName());
                             }
                             break;

                             case PARSERTOKEN_SUBMESH:
                             {
                                 const char* submeshName = paramNode->GetAttributeValue("name");
                                 csRef<iDocumentNode> materialNode = paramNode->GetNode("material");
                                 if(materialNode.IsValid())
                                 {
                                     csString description("meshfact '");
                                     description.Append(GetName());
                                     description.Append("' submesh");
                                     ParseMaterialReference(parserData.data, materialNode->GetContentsValue(),
                                                            description.GetData(), submeshName);
                                 }
                                 submeshes.Push(submeshName);
                             }
                             break;

                             case PARSERTOKEN_MESH:
                             {
                                 csString description("cal3d meshfact '");
                                 description.Append(GetName());
                                 description.Append("' mesh");
                                 ParseMaterialReference(parserData.data, paramNode->GetAttributeValue("material"),
                                                        description.GetData(), paramNode->GetAttributeValue("name"));
                             }
                             break;
                         }
                     }
                 }
                 break;

                 case PARSERTOKEN_KEY:
                 {
                     if(csString("bbox").Compare(node->GetAttributeValue("name")))
                     {
                         csRef<iDocumentNodeIterator> vs = node->GetNodes("v");
                         while(vs->HasNext())
                         {
                             csRef<iDocumentNode> v = vs->Next();
                             csVector3 vtex;
                             parserData.data.syntaxService->ParseVector(v, vtex);
                             bboxvs.Push(vtex);
                         }
                         break;
                     }
                 }
                 break;
             }
         }
         return true;
    }

    bool BgLoader::MeshFact::ParseCells(iDocumentNode* paramNode, ParserData& parserData)
    {
         csRef<iDocumentNodeIterator> cellsIt(paramNode->GetNodes());
         while(cellsIt->HasNext())
         {
             csRef<iDocumentNode> cellNode(cellsIt->Next());
             csStringID cell = parserData.data.xmltokens.Request(cellNode->GetValue());
             switch(cell)
             {
                 case PARSERTOKEN_CELLDEFAULT:
                 case PARSERTOKEN_CELL:
                 {
                     csRef<iDocumentNodeIterator> cellIt(cellNode->GetNodes());
                     while(cellIt->HasNext())
                     {
                         csRef<iDocumentNode> cellParamNode(cellIt->Next());
                         csStringID cellParamID = parserData.data.xmltokens.Request(cellParamNode->GetValue());
                         switch(cellParamID)
                         {
                             case PARSERTOKEN_SIZE:
                             {
                                 if(cellParamNode.IsValid())
                                 {
                                     csVector3 bound(cellParamNode->GetAttributeValueAsInt("x")/2,
                                                     cellParamNode->GetAttributeValueAsInt("y"),
                                                     cellParamNode->GetAttributeValueAsInt("z")/2);
                                     bboxvs.Push(bound);
                                     bound *= -1;
                                     bound.y = 0;
                                     bboxvs.Push(bound);
                                 }
                             }
                             break;

                             case PARSERTOKEN_BASEMATERIAL:
                             case PARSERTOKEN_ALPHASPLATMATERIAL:
                             {
                                 ParseMaterialReference(parserData.data, cellParamNode->GetContentsValue(), "terrain mesh", 0);
                             }
                             break;

                             case PARSERTOKEN_FEEDERPROPERTIES:
                             {
                                 csRef<iDocumentNode> alphaMatNode = cellParamNode->GetNode("alphamap");
                                 if(alphaMatNode.IsValid())
                                 {
                                     ParseMaterialReference(parserData.data, alphaMatNode->GetAttributeValue("material"),
                                                            "terrain mesh", 0);
                                 }
                                 /*csRef<iiDocumentNodeIterator> propertyIt(cellParamNode->GetNodes("param"));
                                 while(propertyIt->HasNext())
                                 {
                                     csRef<iDocumentNode> propertyNode(propertyIt->Next());
                                     csString propertyName = propertyNode->GetAttributeValue("name");
                                     if(name == "heightmap source" || name == "normalmap source")
                                     {
                                         
                                     }
                                 }*/
                             }
                         }
                     }
                 }
                 break;
             }
         }

         return true;
    }

    bool BgLoader::MeshObj::Parse(iDocumentNode* meshNode, ParserData& parserData, bool& alwaysLoaded)
    {
        SetName(meshNode->GetAttributeValue("name"));
        data = meshNode;
        path = parserData.vfsPath;

        sector = parserData.currentSector;

        dynamicLighting = parserData.data.config.enabledGfxFeatures & useHighShaders;
        alwaysLoaded = false;

        // Check for a params file and switch to use it to continue parsing.
        csRef<iDocumentNode> params(meshNode);
        csRef<iDocumentNode> paramfileNode(meshNode->GetNode("paramsfile"));
        if(paramfileNode.IsValid())
        {
            csRef<iDocumentSystem> docsys = csQueryRegistry<iDocumentSystem>(parserData.data.object_reg);
            csRef<iDocument> pdoc = docsys->CreateDocument();
            iVFS* vfs = GetParent()->GetVFS();
            {
                csVfsDirectoryChanger dirchange(vfs);
                dirchange.ChangeTo(parserData.vfsPath);
                csRef<iDataBuffer> pdata = vfs->ReadFile(paramfileNode->GetContentsValue());
                CS_ASSERT_MSG("Invalid params file.\n", pdata.IsValid());
                pdoc->Parse(pdata, true);
                params = pdoc->GetRoot();
            }
            GetParent()->ReleaseVFS();
        }
        params = params->GetNode("params");
        if(!params.IsValid())
        {
            return false;
        }

        csRef<iDocumentNodeIterator> paramIt(params->GetNodes());
        while(paramIt->HasNext())
        {
            csRef<iDocumentNode> paramNode(paramIt->Next());
            csStringID id = parserData.data.xmltokens.Request(paramNode->GetValue());
            switch(id)
            {
                case PARSERTOKEN_SUBMESH:
                {
                    const char* submeshName = paramNode->GetAttributeValue("name");

                    csRef<iDocumentNode> materialNode(paramNode->GetNode("material"));
                    if(materialNode.IsValid())
                    {
                        csString description("meshobj '");
                        description.Append(GetName());
                        description.Append("' submesh in sector");
                        ParseMaterialReference(parserData.data, materialNode->GetContentsValue(), description.GetData(),
                                               parserData.currentSector->GetName());
                    }

                    csRef<iDocumentNodeIterator> shaderVarIt(paramNode->GetNodes("shadervar"));
                    while(shaderVarIt->HasNext())
                    {
                        csRef<iDocumentNode> shaderVar(shaderVarIt->Next());
                        csRef<ShaderVar> sv;
                        sv.AttachNew(new ShaderVar(GetParent(), this));
                        if(sv->Parse<false>(shaderVar, parserData.data))
                        {
                            shadervars.Push(sv);
                        }
                    }

                    if(!FindSubmesh(submeshName))
                    {
                        csString msg;
                        msg.Format("Invalid submesh reference '%s' in meshobj '%s' in sector '%s'",
                                    submeshName, GetName(), parserData.currentSector->GetName());
                        CS_ASSERT_MSG(msg.GetData(), false);
                    }
                }
                break;

                case PARSERTOKEN_FACTORY:
                {
                    csString factoryName(paramNode->GetContentsValue());
                    csRef<MeshFact> meshfact = parserData.data.factories.Get(factoryName);

                    if(meshfact.IsValid())
                    {
                        csRef<iDocumentNode> moveNode = meshNode->GetNode("move");
                        if(moveNode.IsValid())
                        {
                            csRef<iDocumentNode> posNode(moveNode->GetNode("v"));
                            if(posNode.IsValid())
                            {
                                csVector3 pos;
                                csMatrix3 rot;
                                parserData.data.syntaxService->ParseVector(posNode, pos);

                                csRef<iDocumentNode> matrixNode(moveNode->GetNode("matrix"));
                                if(matrixNode.IsValid())
                                {
                                    parserData.data.syntaxService->ParseMatrix(matrixNode, rot);
                                    rot.Invert();
                                }

                                csReversibleTransform t(rot, pos);
                                const csArray<csVector3>& vertices = meshfact->GetVertices();
                                for(size_t i = 0; i < vertices.GetSize(); i++)
                                {
                                    bbox.AddBoundingVertex(t.This2Other(vertices[i]));
                                }
                            }
                        }

                        AddDependency(meshfact);
                    }
                    else
                    {
                        // Validation.
                        csString msg;
                        msg.Format("Invalid factory reference '%s' in meshobj '%s' in sector '%s'", factoryName.GetData(),
                                    GetName(), parserData.currentSector->GetName());
                        CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
                        return false;
                    }
                }
                break;

                case PARSERTOKEN_MATERIAL:
                {
                    csString description("terrain '");
                    description.Append(GetName());
                    description.Append("' object in sector");
                    ParseMaterialReference(parserData.data, paramNode->GetContentsValue(), description.GetData(),
                                           parserData.currentSector->GetName());
                }
                break;

                case PARSERTOKEN_MATERIALPALETTE:
                {
                    csRef<iDocumentNodeIterator> materialIt(paramNode->GetNodes("material"));
                    while(materialIt->HasNext())
                    {
                        csRef<iDocumentNode> materialNode(materialIt->Next());
                        ParseMaterialReference(parserData.data, materialNode->GetContentsValue(),
                                               "terrain materialpalette", 0);
                    }
                }
                break;

                case PARSERTOKEN_CELLS:
                {
                    csRef<iDocumentNodeIterator> cellIt(paramNode->GetNodes("cell"));
                    while(cellIt->HasNext())
                    {
                        csRef<iDocumentNode> cellNode(cellIt->Next());
                        csRef<iDocumentNode> renderProps(cellNode->GetNode("renderproperties"));
                        if(renderProps.IsValid())
                        {
                            csRef<iDocumentNodeIterator> shaderVarIt(renderProps->GetNodes("shadervar"));
                            while(shaderVarIt->HasNext())
                            {
                                csRef<iDocumentNode> shaderVar(shaderVarIt->Next());
                                csRef<ShaderVar> sv;
                                sv.AttachNew(new ShaderVar(GetParent(), this));
                                if(sv->Parse<false>(shaderVar, parserData.data))
                                {
                                    shadervars.Push(sv);
                                }
                            }
                        }
                    }
                }
                break;

                case PARSERTOKEN_ALWAYSLOADED:
                {
                    alwaysLoaded = true;
                }
                break;
            }
        }

        alwaysLoaded |= bbox.Empty() || bbox.IsNaN();
        return true;
    }

    bool BgLoader::MeshObj::ParseTriMesh(iDocumentNode* meshNode, ParserData& parserData, bool& alwaysLoaded)
    {
        SetName(meshNode->GetAttributeValue("name"));
        data = meshNode;
        path = parserData.vfsPath;

        sector = parserData.currentSector;

        alwaysLoaded = true;
        csRef<iDocumentNodeIterator> nodeIt = meshNode->GetNodes();
        while(nodeIt->HasNext())
        {
            csRef<iDocumentNode> node(nodeIt->Next());
            csStringID id = parserData.data.xmltokens.Request(node->GetValue());
            switch(id)
            {
                case PARSERTOKEN_MESH:
                {
                    csRef<iDocumentNodeIterator> vertexIt(node->GetNodes("v"));
                    while(vertexIt->HasNext())
                    {
                        csRef<iDocumentNode> vertex(vertexIt->Next());
                        csVector3 v;
                        parserData.data.syntaxService->ParseVector(vertex, v);
                        bbox.AddBoundingVertex(v);
                    }
                    alwaysLoaded = false;
                }
                break;

                case PARSERTOKEN_BOX:
                {
                    parserData.data.syntaxService->ParseBox(node, bbox);
                    alwaysLoaded = false;
                }
                break;
            }
        }

        alwaysLoaded |= bbox.Empty() || bbox.IsNaN();

        return true;
    }

    bool BgLoader::MeshGen::Parse(iDocumentNode* meshNode, ParserData& parserData)
    {
        if(!(parserData.data.config.enabledGfxFeatures & useMeshGen))
        {
            return false;
        }

        SetName(meshNode->GetAttributeValue("name"));
        data = meshNode;
        path = parserData.vfsPath;

        sector = parserData.currentSector;

        csRef<iDocumentNode> objNode(meshNode->GetNode("meshobj"));
        if(objNode.IsValid())
        {
            mesh = parserData.data.meshes.Get(objNode->GetContentsValue());
        }

        if(!mesh.IsValid())
        {
            return false;
        }

        csRef<iDocumentNode> boxNode(meshNode->GetNode("samplebox"));
        if(boxNode.IsValid())
        {
            csVector3 min;
            csVector3 max;

            parserData.data.syntaxService->ParseVector(boxNode->GetNode("min"), min);
            parserData.data.syntaxService->ParseVector(boxNode->GetNode("max"), max);

            bbox = csBox3(min,max);
        }

        csRef<iDocumentNodeIterator> geometryIt(meshNode->GetNodes("geometry"));
        while(geometryIt->HasNext())
        {
            csRef<iDocumentNode> geometry(geometryIt->Next());

            csString factoryName(geometry->GetNode("factory")->GetAttributeValue("name"));

            csRef<MeshFact> meshfact = parserData.data.factories.Get(factoryName);

            if(meshfact.IsValid())
            {
                AddDependency(meshfact);
            }
            else
            {
                // Validation.
                csString msg;
                msg.Format("Invalid meshfact reference '%s' in meshgen '%s'", meshfact->GetName(), GetName());
                CS_ASSERT_MSG(msg.GetData(), meshfact.IsValid());
            }

            csRef<iDocumentNodeIterator> matfactors = geometry->GetNodes("materialfactor");
            while(matfactors->HasNext())
            {
                csRef<iDocumentNode> materialNode(matfactors->Next());
                ParseMaterialReference(parserData.data, materialNode->GetAttributeValue("material"), "meshgen", GetName());
            }
        }

        return true;
    }
}
CS_PLUGIN_NAMESPACE_END(bgLoader)

