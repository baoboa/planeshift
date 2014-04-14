/*
 *  ccheck.cpp - Author: Mike Gist
 *
 * Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include <cstool/initapp.h>
#include <csutil/cmdhelp.h>
#include <csutil/documenthelper.h>
#include <csutil/stringarray.h>
#include <csutil/xmltiny.h>

#include <iutil/cmdline.h>
#include <iutil/document.h>
#include <iutil/stringarray.h>

#include "ccheck.h"
#include "util/fileutil.h"

CS_IMPLEMENT_APPLICATION

CCheck::CCheck(iObjectRegistry* object_reg) : object_reg(object_reg), strip(false) , duplicateNum(0)
{
    docsys = csQueryRegistry<iDocumentSystem>(object_reg);
    vfs = csQueryRegistry<iVFS>(object_reg);
    strings = csQueryRegistryTagInterface<iStringSet>(object_reg, "crystalspace.shared.stringset");
    log = vfs->Open("/this/ccheck.log", VFS_FILE_WRITE);
}

CCheck::~CCheck()
{
}

void CCheck::PrintHelp()
{
    printf("This application checks for duplicate meshfact and texture inclusions in art files.\n\n");

    printf("Options:\n");
    printf("-in The vfs path to directory to search in. Defaults to /this/ccheck/\n\n");
    printf("-check Whether to do a meshfact conflict check.\n\n");
    printf("-process Whether to process art into a optimal format for bgloader.\n\n");
    printf("-strip Whether to strip data not needed for collision.\n\n");
    printf("-out The vfs path to the directory to output processed art files in. Defaults to /this/ccheckout/\n\n");
    printf("Usage: ccheck(.exe) -path=/this/path/to/directory/\n");
}

void CCheck::Run()
{
    PrintOutput("Art Conflict Checker.\n\n");

    csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser>(object_reg);
    if (csCommandLineHelper::CheckHelp (object_reg))
    {
        PrintHelp();
        return;
    }

    bool checking = cmdline->GetBoolOption("check");
    bool processing = cmdline->GetBoolOption("process");

    csString inpath = cmdline->GetOption("in");
    if(inpath.IsEmpty())
    {
        inpath = "/this/ccheckin/";
    }

    outpath = cmdline->GetOption("out");
    if(outpath.IsEmpty())
    {
        outpath = "/this/ccheckout/";
    }

    strip = cmdline->GetBoolOption("strip");

    csRef<iDocument> doc = docsys->CreateDocument();
    texmat = doc->CreateRoot();
    texmat = texmat->CreateNodeBefore(CS_NODE_ELEMENT);
    texmat->SetValue("library");

    csRef<iStringArray> files = vfs->FindFiles(inpath);
    for(size_t i=0; i<files->GetSize(); i++)
    {
        csRef<iDataBuffer> filePath = vfs->GetRealPath(files->Get(i));
        
        if(csString(files->Get(i)).Length() > 4 &&
            csString(files->Get(i)).Slice(csString(files->Get(i)).Length()-4).Compare(".zip"))
        {
            csString fileName(files->Get(i));
            fileName = fileName.Slice(fileName.FindLast('/')+1);

            vfs->Mount("/ccheck/", filePath->GetData());
            csRef<iStringArray> zipfiles = vfs->FindFiles("/ccheck/");
            for(size_t j=0; j<zipfiles->GetSize(); j++)
            {
                csString zipfile = zipfiles->Get(j);
                if(zipfile.GetAt(zipfile.Length()-1) != '/')
                    ParseFile(zipfile, fileName, processing);
                else if(zipfile.Find("lightmaps") == size_t(-1) && zipfile.Find("bindata") == size_t(-1))
                {
                    csRef<iStringArray> files = vfs->FindFiles(zipfile);
                    for(size_t k=0; k<files->GetSize(); ++k)
                    {
                        csString file = files->Get(k);
                        ParseFile(file, file, processing);
                    }
                }

                if(processing)
                {
                    if(zipfile.Find("lightmaps") != size_t(-1) && zipfile.Find("cslib") == size_t(-1))
                    {
                        csRef<iStringArray> lightmaps = vfs->FindFiles(zipfile);
                        for(size_t k=0; k<lightmaps->GetSize(); k++)
                        {
                            FileUtil futil(vfs);
                            csString path = lightmaps->Get(k);
                            futil.CopyFile(lightmaps->Get(k), outpath+"/materials/lightmaps/" + path.Slice(path.FindLast('/')), true, false);
                        }
                    }

                    if(zipfile.Find("bindata") != size_t(-1))
                    {
                        csRef<iStringArray> bindata = vfs->FindFiles(zipfile);
                        for(size_t k=0; k<bindata->GetSize(); k++)
                        {
                            FileUtil futil(vfs);
                            csString path = bindata->Get(k);
                            if((path.GetAt(path.Length()-3) == '_' && path.GetAt(path.Length()-2) == 'l' && path.GetAt(path.Length()-1) == 'm'))
                                futil.CopyFile(bindata->Get(k), outpath+"/world/bindata/" + path.Slice(path.FindLast('/')), true, false);
                            else if(path.Find("_c") != size_t(-1))
                            {
                                //if we have found a _c check if it's really the right one (_c# or _c#_pd#)
                                bool found = true; //we take for granted at the start that we have found all correctly as expected
                                //we start from the c our checks
                                for(size_t i  = path.Find("_c") + 2; i < path.Length(); i++)
                                {
                                    //when we find numbers we are ok
                                    if(path.GetAt(i) >= '0' && path.GetAt(i) <= '9')
                                        continue;
                                    //when we find _ we are ok only if we have enough space to house pd + at least a number
                                    //and the next chars are actually pd
                                    if(path.Length() > i + 3 && path.GetAt(i) == '_' && path.GetAt(i+1) == 'p' && path.GetAt(i+2) == 'd')
                                    {
                                        i += 2; //move to the end of _pd
                                        continue;
                                    }

                                    //if the two tests above failed it means we have something different so abort the parsing
                                    found = false;
                                    break;
                                }
                                if(found)
                                    futil.CopyFile(bindata->Get(k), outpath+"/world/bindata/" + path.Slice(path.FindLast('/')), true, false);
                                else
                                    futil.CopyFile(bindata->Get(k), outpath+"/meshes/bindata/" + path.Slice(path.FindLast('/')), true, false);

                            }
                            else
                                futil.CopyFile(bindata->Get(k), outpath+"/meshes/bindata/" + path.Slice(path.FindLast('/')), true, false);
                        }
                    }
                }
            }
            vfs->Unmount("/ccheck/", filePath->GetData());
        }
        else
        {
            ParseFile(filePath->GetData(), filePath->GetData(), processing);
        }
    }

    if(checking)
    {
        PrintOutput("\nConflicted meshfacts:\n");

        for(size_t i=0; i<meshfacts.GetSize(); i++)
        {
            csArray<csString> conflicted = cmeshfacts.GetAll(meshfacts[i]);
            for(size_t j=0; 1<conflicted.GetSize() && j<conflicted.GetSize(); j++)
            {
                if(j == 0)
                    PrintOutput("\nMeshFact '%s' conflicts in: ", meshfacts[i].GetData());

                PrintOutput("%s ", conflicted[j].GetData());
            }
        }
    }

    if(processing)
        doc->Write(vfs, outpath+"/materials/materials.cslib");
}

void CCheck::ParseFile(const char* filePath, const char* fileName, bool processing)
{
    if(processing)
    {
        if(!strip && (csString(filePath).Find(".dds") != (size_t)-1 || csString(filePath).Find(".mng") != (size_t)-1 || csString(filePath).Find("_alpha.png") != (size_t)-1 || csString(filePath).Find("_icon.png") != (size_t)-1))
        {
            FileUtil futil(vfs);
            futil.CopyFile(filePath, outpath+"/materials/"+csString(filePath).Slice(csString(filePath).FindLast('/')), true, false);
        }

        if(csString(filePath).Find(".CSF") != (size_t)-1 || csString(filePath).Find(".CMF") != (size_t)-1 || csString(filePath).Find(".CAF") != (size_t)-1)
        {
            FileUtil futil(vfs);
            futil.CopyFile(filePath, outpath+"/meshes/"+csString(filePath).Slice(csString(filePath).FindLast('/')), true, false);
        }

        if(csString(filePath).Find(".params") != (size_t)-1 || csString(filePath).Find("_heightmap.png") != (size_t)-1 || csString(filePath).Find("_density.png") != (size_t)-1)
        {
            csString paramsPath = outpath+"/world/"+csString(filePath).Slice(csString(filePath).FindLast('/'));
            if(vfs->Exists(paramsPath))
            {
                printf("Warning: file %s from map %s already exists", filePath, fileName);
            }
            FileUtil futil(vfs);
            futil.CopyFile(filePath, paramsPath, true, false);
        }

        if(csString(filePath).Find("sound.xml") != (size_t)-1)
        {
            FileUtil futil(vfs);
            futil.CopyFile(filePath, outpath+"/world/sound/"+csString(fileName).Slice(0, csString(fileName).FindLast('.'))+".xml", true, false);
        }
    }

    csRef<iDataBuffer> buf = vfs->ReadFile(filePath);
    if(!buf.IsValid())
        return;

    csRef<iDocument> bdoc = docsys->CreateDocument();
    bdoc->Parse(buf, true);
    csRef<iDocumentNode> broot = bdoc->GetRoot();

    if(!broot.IsValid())
        return;

    csRef<iDocument> doc = tinydoc.CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot();
    CS::DocSystem::CloneNode(broot, root);

    bool library = true;
    root = doc->GetRoot()->GetNode("library");
    if(!root.IsValid())
    {
        library = false;
        root = doc->GetRoot()->GetNode("world");
    }

    if(!root.IsValid())
    {
        return;
    }

    if(processing)
    {

        if(root->GetNode("shaders"))
        {
            if(!strip)
            {
                csRef<iDocumentNode> newShaders = texmat->GetNode("shaders");
                if(!newShaders.IsValid())
                {
                    newShaders = texmat->CreateNodeBefore(CS_NODE_ELEMENT, texmat->GetNode("materials"));
                    newShaders->SetValue("shaders");
                }
                csRef<iDocumentNodeIterator> itr = root->GetNode("shaders")->GetNodes("shader");
                while(itr->HasNext())
                {
                    csRef<iDocumentNode> node = itr->Next();
                    csRef<iDocumentNode> fileNode = node->GetNode("file");
                    if(fileNode.IsValid())
                    {
                        size_t prevSize = shaders.GetSize();
                        shaders.PushSmart(fileNode->GetContentsValue());
                        if(prevSize != shaders.GetSize())
                        {
                            csRef<iDocumentNode> newNode = newShaders->CreateNodeBefore(CS_NODE_ELEMENT);
                            CS::DocSystem::CloneNode(node, newNode);
                        }
                    }
                }
            }
            
            root->RemoveNode(root->GetNode("shaders"));
        }

        if(root->GetNode("textures"))
        {
            if(!strip)
            {
                csRef<iDocumentNode> newTextures = texmat->GetNode("textures");
                if(!newTextures.IsValid())
                {
                    newTextures = texmat->CreateNodeBefore(CS_NODE_ELEMENT);
                    newTextures->SetValue("textures");
                }
                csRef<iDocumentNodeIterator> itr = root->GetNode("textures")->GetNodes("texture");
                while(itr->HasNext())
                {
                    csRef<iDocumentNode> node = itr->Next();
                    size_t prevSize = textures.GetSize();
                    textures.PushSmart(node->GetAttributeValue("name"));
                    if(prevSize != textures.GetSize())
                    {
                        csRef<iDocumentNode> newNode = newTextures->CreateNodeBefore(CS_NODE_ELEMENT);
                        CS::DocSystem::CloneNode(node, newNode);
                    }
                }
            }
            
            root->RemoveNode(root->GetNode("textures"));
        }

        if(root->GetNode("materials"))
        {
            csRef<iDocumentNode> newMaterials = texmat->GetNode("materials");
            if(!newMaterials.IsValid())
            {
                newMaterials = texmat->CreateNodeBefore(CS_NODE_ELEMENT);
                newMaterials->SetValue("materials");
            }
            csRef<iDocumentNodeIterator> itr = root->GetNode("materials")->GetNodes("material");
            while(itr->HasNext())
            {
                csRef<iDocumentNode> node = itr->Next();
                size_t prevSize = materials.GetSize();
                materials.PushSmart(node->GetAttributeValue("name"));
                if(prevSize != materials.GetSize())
                {
                    csRef<iDocumentNode> newNode = newMaterials->CreateNodeBefore(CS_NODE_ELEMENT);
                    if(strip)
                    {
                        newNode->SetAttribute("name", node->GetAttributeValue("name"));
                    }
                    else
                    {
                        CS::DocSystem::CloneNode(node, newNode);
                    }
                }
            }

            root->RemoveNode(root->GetNode("materials"));
        }
    }

    csRef<iDocumentNodeIterator> itr = root->GetNodes("meshfact");
    while(itr->HasNext())
    {
        csRef<iDocumentNode> node = itr->Next();
        meshfacts.PushSmart(node->GetAttributeValue("name"));
        cmeshfacts.Put(node->GetAttributeValue("name"), fileName);

        if(processing)
        {
            csRef<iDocument> mdoc = docsys->CreateDocument();
            csRef<iDocumentNode> newNode = mdoc->CreateRoot();
            newNode = newNode->CreateNodeBefore(CS_NODE_ELEMENT);
            CS::DocSystem::CloneNode(node, newNode);

            csString filename = outpath+"/meshes/"+node->GetAttributeValue("name");

            if(vfs->Exists(filename))
            {
                csRef<iDataBuffer> dbuf = vfs->ReadFile(outpath+"/meshes/"+node->GetAttributeValue("name"));
                if(dbuf.IsValid())
                {
                    csRef<iDocument> ddoc = docsys->CreateDocument();
                    ddoc->Parse(dbuf, true);
                    csRef<iDocumentNode> droot = ddoc->GetRoot();
                    if(droot.IsValid() && droot->GetNode("meshfact"))
                    {
                        if(!strcmp(droot->GetNode("meshfact")->GetAttributeValue("name"), node->GetAttributeValue("name")))
                        {
                            continue;
                        }
                    }
                }

                // Not guaranteed to produce correct results in-game - the render buffer data may be the same.
                printf("Warning - case sensitive duplication with mesh %s\n", node->GetAttributeValue("name"));
                filename.AppendFmt("_%u", ++duplicateNum);
            }

            mdoc->Write(vfs, filename);
        }
    }

    if(processing && !library)
    {
        root->RemoveNodes(root->GetNodes("library"));
        root->RemoveNodes(root->GetNodes("meshfact"));

        csRef<iDocument> wdoc = docsys->CreateDocument();
        csRef<iDocumentNode> root = doc->GetRoot();
        csRef<iDocumentNode> newRoot = wdoc->CreateRoot();
        CS::DocSystem::CloneNode(root, newRoot);
        wdoc->Write(vfs, outpath+"/world/"+csString(fileName).Slice(0, csString(fileName).FindLast('.')));
    }
}

void CCheck::PrintOutput(const char* string, ...)
{
    csString outputString;
    va_list args;
    va_start (args, string);
    outputString.FormatV (string, args);
    va_end (args);

    printf("%s", outputString.GetData());

    if(log.IsValid())
    {
        log->Write(outputString.GetData(), outputString.Length());
    }    
}

int main(int argc, char** argv)
{
    iObjectRegistry* object_reg = csInitializer::CreateEnvironment(argc, argv);
    if(!object_reg)
    {
        printf("Object Reg failed to Init!\n");
        return 1;
    }

    csInitializer::RequestPlugins (object_reg, CS_REQUEST_VFS,
	CS_REQUEST_PLUGIN("crystalspace.documentsystem.multiplexer", iDocumentSystem),
	CS_REQUEST_END);

    CCheck* ccheck = new CCheck(object_reg);
    ccheck->Run();
    delete ccheck;
    CS_STATIC_VARIABLE_CLEANUP
    return 0;
}
