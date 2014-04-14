/*
 *  navgen.cpp - Author: Matthieu Kraus
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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

#include "navgen.h"

#include <cstool/initapp.h>
#include <csutil/cmdhelp.h>
#include <iutil/cmdline.h>
#include <iutil/stringarray.h>
#include <iutil/eventq.h>
#include <iutil/virtclk.h>
#include <iengine/sector.h>

CS_IMPLEMENT_APPLICATION

#define CONFIGFILE "/planeshift/navgen.cfg"

NavGen::NavGen(iObjectRegistry* object_reg) : object_reg(object_reg)
{
    engine  = csQueryRegistryOrLoad<iEngine>(object_reg, "crystalspace.engine.3d");
    vfs     = csQueryRegistry<iVFS>(object_reg);
    loader  = csQueryRegistryOrLoad<iBgLoader>(object_reg, "crystalspace.bgloader");
    builder = csQueryRegistryOrLoad<iCelHNavStructBuilder>(object_reg, "cel.hnavstructbuilder");
    config  = csQueryRegistry<iConfigManager>(object_reg);
}

NavGen::~NavGen()
{
}

void NavGen::PrintHelp()
{
    csPrintf("This application creates the NavMesh to be used with the client and server.\n\n");
    csPrintf("Optional parmeters:\n");
    csPrintf("  -materials=dir  set material directory (/planeshift/materials/)\n");
    csPrintf("  -meshes=dir     set mesh directory     (/planeshift/meshes/)\n");
    csPrintf("  -world=dir      set world directory    (/planeshift/world/)\n");
    csPrintf("  -output=dir     set output directory   (/planeshift/navmesh/)\n");
}

void NavGen::Run()
{
    csPrintf("Navmesh Generator.\n\n");

    csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser>(object_reg);
    if (csCommandLineHelper::CheckHelp (object_reg))
    {
        PrintHelp();
        return;
    }

    csString basePath = "/planeshift/";

    csString materials = cmdline->GetOption("materials");
    if(materials.IsEmpty())
        materials = config->GetStr("NavGen.MaterialDir", basePath+"materials/");

    csString meshes = cmdline->GetOption("meshes");
    if(meshes.IsEmpty())
        meshes = config->GetStr("NavGen.MeshDir", basePath+"meshes/");

    csString world = cmdline->GetOption("world");
    if(world.IsEmpty())
        world = config->GetStr("NavGen.WorldDir", basePath+"world/");

    csString output = cmdline->GetOption("output");
    if(output.IsEmpty())
        output = config->GetStr("NavGen.OutputDir", basePath+"navmesh");

    float height = config->GetFloat("NavGen.Agent.Height", 2.f);
    float width  = config->GetFloat("NavGen.Agent.Width", 0.5f);
    float slope  = config->GetFloat("NavGen.Agent.Slope", 45.f);
    float step   = config->GetFloat("NavGen.Agent.MaxStepSize", 0.5f);
    float cellSize = config->GetFloat("NavGen.CellSize", width/2);
    float cellHeight = config->GetFloat("NavGen.CellHeight", cellSize/2);
    int tileSize = config->GetInt("NavGen.TileSize", 64) / cellSize;
    int borderSize = csMin(config->GetInt("NavGen.BorderSize", width+1),1);

    csRef<iEventQueue> queue = csQueryRegistry<iEventQueue>(object_reg);
    csRef<iVirtualClock> vc = csQueryRegistry<iVirtualClock>(object_reg);

    // Disable threaded loading.
    csRef<iThreadManager> tman = csQueryRegistry<iThreadManager>(object_reg);
    tman->SetAlwaysRunNow(true);

    // output execution parameters
    csPrintf("-- INPUT Parameters --\n");
    csPrintf("Materials: %s\n", materials.GetData());
    csPrintf("Meshes: %s\n", meshes.GetData());
    csPrintf("World: %s\n", world.GetData());
    csPrintf("NavGen.Agent.Height: %f\n", height);
    csPrintf("NavGen.Agent.Width: %f\n", width);
    csPrintf("NavGen.Agent.Slope: %f\n", slope);
    csPrintf("NavGen.Agent.MaxStepSize: %f\n", step);
    csPrintf("NavGen.CellSize: %f\n", cellSize);
    csPrintf("NavGen.CellHeight: %f\n", cellHeight);
    csPrintf("NavGen.TileSize: %d\n", tileSize);
    csPrintf("NavGen.BorderSize: %d\n", borderSize);
    csPrintf("---\n");

    vc->Advance();
    queue->Process();

    // precache materials
    {
        csPrintf("Caching materials...\n");
        csRef<iThreadReturn> ret = loader->PrecacheDataWait(materials+"materials.cslib");
        if(!ret->IsFinished() || !ret->WasSuccessful())
        {
            csPrintf("Failed to cache materials\n");
            return;
        }
    }

    vc->Advance();
    queue->Process();

    // precache world
    {
        csPrintf("Finding meshes and world...\n");
        csRef<iStringArray> meshFiles = vfs->FindFiles(meshes);
        csRef<iStringArray> worldFiles = vfs->FindFiles(world);

        size_t progress = 0;

        csPrintf("Caching meshes...\n");
        csRefArray<iThreadReturn> returns;
        for(size_t i = 0; i < meshFiles->GetSize(); i++)
        {
            returns.Push(loader->PrecacheData(meshFiles->Get(i)));
        }

        csPrintf("Finishing mesh cache...\n");
        while(!returns.IsEmpty())
        {
            for(size_t i = 0; i < returns.GetSize(); i++)
            {
                csRef<iThreadReturn> ret = returns.Get(i);
                if(ret->IsFinished())
                {
                    if(!ret->WasSuccessful())
                    {
                    }
                    returns.DeleteIndex(i);
                    i--;
                    progress++;
                }
            }

            // print progress

            vc->Advance();
            queue->Process();
            csSleep(100); // wait a bit before checking again
        }

        csPrintf("Caching world...\n");
        for(size_t i = 0; i < worldFiles->GetSize(); i++)
        {
            returns.Push(loader->PrecacheData(worldFiles->Get(i)));
        }

        csPrintf("Finishing world cache...\n");
        while(!returns.IsEmpty())
        {
            for(size_t i = 0; i < returns.GetSize(); i++)
            {
                csRef<iThreadReturn> ret = returns.Get(i);
                if(ret->IsFinished())
                {
                    if(!ret->WasSuccessful())
                    {
                    }
                    returns.DeleteIndex(i);
                    i--;
                    progress++;
                }
            }

            // print progress

            vc->Advance();
            queue->Process();
            csSleep(100); // wait a bit before checking again
        }
    }

    // load world
    {
        csPrintf("Loading world...\n");
        if(!loader->LoadZones())
        {
            csPrintf("failed to load world\n");
            return;
        }

        csPrintf("Finishing world load...\n");
        while(loader->GetLoadingCount())
        {
            loader->ContinueLoading(true);

            // print progress

            vc->Advance();
            queue->Process();
            csSleep(100);
        }
    }

    // process navmesh
    {
        csPrintf("Loading meshes from sectors...\n");
        // set creation parameters
        csRef<iCelNavMeshParams> parameters;
        parameters.AttachNew(builder->GetNavMeshParams()->Clone());
        parameters->SetSuggestedValues(height, width, slope);
        parameters->SetAgentMaxClimb(step);
        parameters->SetTileSize(tileSize);
        parameters->SetCellSize(cellSize);
        parameters->SetCellHeight(cellHeight);
        parameters->SetBorderSize(borderSize);
        builder->SetNavMeshParams(parameters);

        // get list of loaded sectors
        csRefArray<iSector> sectors;
        csRef<iSectorList> sectorList = engine->GetSectors();
        for(int i = 0; i < sectorList->GetCount(); i++)
        {
            sectors.Push(sectorList->Get(i));
        }

        // set sectors in navigation mesh
        if(!builder->SetSectors(&sectors))
        {
            csPrintf("failed to set sector on navigation mesh builder\n");
            return;
        }

        // build navmesh
        csPrintf("Building navmesh...\n");
        csRef<iCelHNavStruct> navMesh = builder->BuildHNavStruct();
        if(!navMesh.IsValid())
        {
            csPrintf("failed to build navigation mesh\n");
            return;
        }
        csPrintf("Done generating, writing navmesh\n");
        // save navmesh
        if(!navMesh->SaveToFile(vfs, output))
        {
            csPrintf("failed to save navigation data");
            return;
        }
    }
    csPrintf("Done writing, closing up.\n");

    loader.Invalidate();
    builder.Invalidate();
    config.Invalidate();
    vfs.Invalidate();

    csRef<iThreadReturn> ret = engine->DeleteAll();

    while(!ret->IsFinished())
    {
        vc->Advance();
        queue->Process();
        csSleep(100);
    }
}

int main(int argc, char** argv)
{
    iObjectRegistry* object_reg = csInitializer::CreateEnvironment(argc, argv);
    if(!object_reg)
    {
        csPrintf("Object Reg failed to Init!\n");
        return -1;
    }

    if(!csInitializer::SetupConfigManager(object_reg, CONFIGFILE))
    {
        csPrintf("Failed to read config file!\n");
        return -2;
    }

    csInitializer::RequestPlugins (object_reg, CS_REQUEST_VFS,
	CS_REQUEST_PLUGIN("crystalspace.documentsystem.multiplexer", iDocumentSystem),
	CS_REQUEST_END);

    NavGen* navgen = new NavGen(object_reg);
    if(!csInitializer::OpenApplication(object_reg))
    {
        csPrintf("csInitializer::OpenApplication failed!\n"
                 "Is your CRYSTAL environment var set?");
        return -2;
    }
    navgen->Run();

    delete navgen;
    CS_STATIC_VARIABLE_CLEANUP
    csInitializer::DestroyApplication(object_reg);

    return 0;
}

