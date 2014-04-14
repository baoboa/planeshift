/*
* worldeditor.cpp - Author: Mike Gist
*
* Copyright (C) 2009 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#include <cstool/csview.h>
#include <cstool/enginetools.h>
#include <cstool/genmeshbuilder.h>
#include <cstool/initapp.h>
#include <csutil/event.h>
#include <iengine/sector.h>
#include <iutil/cfgmgr.h>
#include <iutil/cmdline.h>
#include <iutil/csinput.h>
#include <iutil/eventq.h>
#include <iutil/object.h>
#include <iutil/virtclk.h>
#include <ivaria/view.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>

#include <ibgloader.h>
#include <iscenemanipulate.h>
#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"
#include "util/log.h"

#include "worldeditor.h"

#define APPNAME "PlaneShift World Editor"
#define WEDIT_CONFIG_FILENAME "/this/worldeditor.cfg"

WorldEditor::WorldEditor(int argc, char* argv[]) :
    editMode(Select), moveCamera(true), paws(NULL), mainWidget(NULL), rotX(0), rotY(0)
{
    // Init CS.
    objectReg = csInitializer::CreateEnvironment(argc, argv);
    csInitializer::SetupConfigManager(objectReg, WEDIT_CONFIG_FILENAME);
    csInitializer::RequestPlugins(objectReg, CS_REQUEST_FONTSERVER,
        CS_REQUEST_IMAGELOADER, CS_REQUEST_OPENGL3D, CS_REQUEST_END);

    // Initialize event shortcuts.
    csRef<iEventNameRegistry> eventNameReg = csEventNameRegistry::GetRegistry (objectReg);
    FrameEvent = csevFrame (eventNameReg);
    KeyDown = csevKeyboardDown(eventNameReg);
    MouseMove = csevMouseMove (eventNameReg, 0);
    MouseDown = csevMouseDown (eventNameReg, 0);
}

WorldEditor::~WorldEditor()
{
    view.Invalidate();
    sceneManipulator.Invalidate();
    csInitializer::DestroyApplication(objectReg);
}

void WorldEditor::Run()
{
    if(Init())
    {
        // Hand over control to CS.
        csDefaultRunLoop(objectReg);
    }

    if(paws)
    {
        // Delete paws.
        delete paws;
        paws = 0;
    }

    // Close window.
    csInitializer::CloseApplication(objectReg);
}

bool WorldEditor::Init()
{
    pslog::Initialize(objectReg);

    csRef<iVFS> vfs = csQueryRegistry<iVFS> (objectReg);
    if (!vfs)
    {
        printf("vfs failed to Init!\n");
        return false;
    }

    csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (objectReg);
    if (!configManager)
    {
        printf("configManager failed to Init!\n");
        return false;
    }

    csRef<iEventQueue> queue = csQueryRegistry<iEventQueue> (objectReg);
    if (!queue)
    {
        printf("No iEventQueue plugin!\n");
        return false;
    }

    g3d = csQueryRegistry<iGraphics3D> (objectReg);
    if (!g3d)
    {
        printf("iGraphics3D failed to Init!\n");
        return false;
    }

    csRef<iGraphics2D> g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        printf("GetDriver2D failed to Init!\n");
        return false;
    }

    engine = csQueryRegistry<iEngine> (objectReg);
    if (!engine)
    {
        printf("iEngine failed to Init!\n");
        return false;
    }

    kbd = csQueryRegistry<iKeyboardDriver> (objectReg);
    if (!kbd)
    {
        printf("iKeyboardDriver failed to Init!\n");
        return false;
    }

    vc = csQueryRegistry<iVirtualClock> (objectReg);
    if (!vc)
    {
        printf("iVirtualClock failed to Init!\n");
        return false;
    }

    csRef<iThreadManager> threadman = csQueryRegistry<iThreadManager> (objectReg);
    if (!threadman)
    {
        printf("iThreadManager failed to Init!\n");
        return false;
    }

    csRef<iBgLoader> loader = csQueryRegistry<iBgLoader>(objectReg);
    if(!loader.IsValid())
    {
        printf("Failed to load iBgLoader!\n");
        return false;
    }

    loader->SetLoadRange(1000);
    sceneManipulator = scfQueryInterface<iSceneManipulate>(loader);

    if(!csInitializer::OpenApplication(objectReg))
    {
        printf("Error initialising app (CRYSTAL not set?)\n");
        return false;
    }

    iNativeWindow *nw = g2d->GetNativeWindow();
    if (nw)
      nw->SetTitle(APPNAME);

    // Paws initialization
    vfs->Mount("/planeshift/", "$^");
    csString skinPath = configManager->GetStr("PlaneShift.GUI.Skin.Base","/planeshift/art/skins/base/client_base.zip");
    paws = new PawsManager(objectReg, skinPath);
    if (!paws)
    {
        printf("Failed to init PAWS!\n");
        return false;
    }

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    // Load and assign a default button click sound for pawsbutton
    //paws->LoadSound("/planeshift/art/sounds/gui/next.wav","sound.standardButtonClick");

    // Set mouse image.
    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    // Register our event handler
    csRef<EventHandler> event_handler;
    event_handler.AttachNew(new EventHandler (this));
    csEventID esub[] = 
    {
        csevFrame (objectReg),
        csevMouseEvent (objectReg),
        csevKeyboardEvent (objectReg),
        csevQuit (objectReg),
        CS_EVENTLIST_END
    };
    queue->RegisterListener(event_handler, esub);

    // Set up view.
    view.AttachNew(new csView(engine, g3d));
    view->SetRectangle(0, 0, g2d->GetWidth(), g2d->GetHeight());
    view->GetPerspectiveCamera()->SetPerspectiveCenter(0.5, 0.5);

    // Create blackbox world.
    iSector* blackbox = engine->CreateSector("BlackBox");
    blackbox->SetDynamicAmbientLight(csColor(1.0f, 1.0f, 1.0f));
    view->GetCamera()->SetSector(blackbox);
    view->GetCamera()->GetTransform().SetOrigin(csVector3(0.0f, 1.0f, 0.0f));

    // Create the base geometry.
    using namespace CS::Geometry;
    TesselatedBox box (csVector3 (-CS_BOUNDINGBOX_MAXVALUE, 0, -CS_BOUNDINGBOX_MAXVALUE),
                       csVector3 (CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE, CS_BOUNDINGBOX_MAXVALUE));
    box.SetFlags (Primitives::CS_PRIMBOX_INSIDE);

    // Now we make a factory and a mesh at once.
    csRef<iMeshWrapper> bbox = GeneralMeshBuilder::CreateFactoryAndMesh (
        engine, blackbox, "blackbox", "blackbox_factory", &box);

    // Create and set material.
    bbox->GetMeshObject ()->SetMaterialWrapper(engine->CreateMaterial("black", engine->CreateBlackTexture("black", 2, 2, 0, 0)));
    bbox->GetFlags().Set(CS_ENTITY_NOHITBEAM);

    // Check for world to load (will be done via gui in future).
    csRef<iCommandLineParser> cmdLine = csQueryRegistry<iCommandLineParser> (objectReg);
    const char* mapPath = cmdLine->GetOption("map");
    if(mapPath)
    {
        loader->PrecacheDataWait(mapPath);
        csRef<StartPosition> startPos = loader->GetStartPositions().Get(0);
        loader->UpdatePosition(startPos->position, startPos->sector, true);
        while(loader->GetLoadingCount() != 0)
        {
            threadman->Process(10);
            loader->ContinueLoading(true);
        }
        view->GetCamera()->SetSector(engine->GetSectors()->FindByName(startPos->sector));
        view->GetCamera()->GetTransform().SetOrigin(startPos->position);
    }

    // Prepare engine.
    engine->Prepare();
    engine->PrecacheDraw();

    return true;
}

bool WorldEditor::HandleEvent (iEvent &ev)
{
    if(ev.Name == FrameEvent)
    {
        if(moveCamera)
        {
            HandleMovement();
        }

        g3d->BeginDraw(CSDRAW_3DGRAPHICS);
        view->Draw();
        g3d->FinishDraw();
        g3d->Print(0);
        return false;
    }

    if(!paws->HandleEvent(ev))
    {
        HandleMeshManipulation(ev);
    }
    
    return false;
}

void WorldEditor::HandleMovement()
{
    // Handle camera movement.
    bool rotated = false;
    csTicks elapsedTicks = vc->GetElapsedTicks();
    float elapsedSeconds = elapsedTicks / 1000.0f;

    if(kbd->GetKeyState(CSKEY_UP))
    {
        view->GetCamera()->Move(CS_VEC_FORWARD * 5.0f * elapsedSeconds);
    }

    if(kbd->GetKeyState(CSKEY_DOWN))
    {
        view->GetCamera()->Move(CS_VEC_BACKWARD * 5.0f * elapsedSeconds);
    }

    if(kbd->GetKeyState(CSKEY_LEFT))
    {
        rotated = true;
        rotY -= 1.20f * elapsedSeconds;
    }

    if(kbd->GetKeyState(CSKEY_RIGHT))
    {
        rotated = true;
        rotY += 1.20f * elapsedSeconds;
    }

    if(kbd->GetKeyState(CSKEY_PGUP))
    {
        rotated = true;
        rotX += 1.20f * elapsedSeconds;
    }

    if(kbd->GetKeyState(CSKEY_PGDN))
    {
        rotated = true;
        rotX -= 1.20f * elapsedSeconds;
    }

    if(rotated)
    {
        csMatrix3 rot = csXRotMatrix3 (rotX) * csYRotMatrix3 (rotY);
        csOrthoTransform ot (rot, view->GetCamera()->GetTransform ().GetOrigin ());
        view->GetCamera()->SetTransform (ot);
    }
}

void WorldEditor::HandleMeshManipulation(iEvent &ev)
{
    if(ev.Name == MouseDown)
    {
        psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();
        switch(editMode)
        {
        case Select:
            {
                iMeshWrapper* selected = sceneManipulator->SelectMesh(view->GetCamera(), csVector2(p.x, p.y));
                if(selected)
                {
                    printf("Selected mesh: %s\n", selected->QueryObject()->GetName());
                }
                else
                {
                    printf("Selection cleared!\n");
                }
                break;
            }
        case Create:
            {
                break;
            }
        case TranslateXZ:
        case TranslateY:
        case Rotate:
            {
                editMode = Select;
                break;
            }
        case Remove:
            {
                iMeshWrapper* selected = sceneManipulator->SelectMesh(view->GetCamera(), csVector2(p.x, p.y));
                if(selected)
                {
                    printf("Removing mesh: %s\n", selected->QueryObject()->GetName());
                    sceneManipulator->RemoveSelected();
                }
                break;
            }
        }
    }
    else if(ev.Name == MouseMove)
    {
        psPoint p = PawsManager::GetSingleton().GetMouse()->GetPosition();
        switch(editMode)
        {
        case TranslateXZ:
            {
                sceneManipulator->TranslateSelected(false, view->GetCamera(), csVector2(p.x, p.y));
                break;
            }
        case TranslateY:
            {
                sceneManipulator->TranslateSelected(true, view->GetCamera(), csVector2(p.x, p.y));
                break;
            }
        case Rotate:
            {
                sceneManipulator->RotateSelected(csVector2(p.x, p.y));
                break;
            }
        default:
            break;
        }
    }
    else if(ev.Name == KeyDown)
    {
        int cooked = csKeyEventHelper::GetCookedCode(&ev);
        switch(cooked)
        {
        case 'c':
            {
                editMode = Create;
                break;
            }
        case 'z':
            {
                editMode = TranslateXZ;
                break;
            }
        case 'y':
            {
                editMode = TranslateY;
                break;
            }
        case 'r':
            {
                editMode = Rotate;
                break;
            }
        case 'x':
            {
                editMode = Remove;
                break;
            }
        case 's':
            {
                editMode = Select;
                break;
            }
        case 'e':
            {
                moveCamera = false;
                break;
            }
        case 'm':
            {
                moveCamera = true;
                break;
            }
        }
    }
}
