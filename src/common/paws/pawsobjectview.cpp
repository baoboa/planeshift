/*
 * pawsobjectview.cpp - Author: Andrew Craig
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

#include <csutil/documenthelper.h>
#include <csutil/scfstringarray.h>

#include <iutil/object.h>
#include <iutil/objreg.h>
#include <imap/loader.h>
#include <iutil/vfs.h>
#include <iengine/texture.h>
#include <csutil/cscolor.h>
#include "pawsobjectview.h"
#include "pawsmanager.h"
#include "util/log.h"
#include "util/psconst.h"

pawsObjectView::pawsObjectView()
{
    loader = csQueryRegistry<iBgLoader>(PawsManager::GetSingleton().GetObjectRegistry());
    engine = csQueryRegistry<iEngine>(PawsManager::GetSingleton().GetObjectRegistry());
    ID = 0;

    cameraLocked = false;
    rotateTime = orgTime = 0;
    rotateRadians = orgRadians = 0;
    camRotate = 0.0f;
    objectPos = csVector3(0,0,0); // Center of podium
    lookingAt = csVector3(0,0,0);
    cameraMod = csVector3(0,0,0);
    charApp = NULL;

    spinMouse = false;
    mouseControlled = false;
    doRotate = true;
    mouseDownUnlock = false;
    mouseDownRotate = false;

    needsDraw = true;
}

pawsObjectView::~pawsObjectView()
{
    if(!rmTargets.IsValid())
    {
        // we failed to load, so there's nothing to deallocate
        return;
    }

    if(meshTarget.IsValid())
    {
        // unregister tex from render targets
        rmTargets->UnregisterRenderTarget(meshTarget);
    }

    if(stageTarget.IsValid())
    {
        // unregister tex from render targets
        rmTargets->UnregisterRenderTarget(stageTarget);
    }

    PawsManager::GetSingleton().RemoveObjectView(this);

    // free map ressources
    if(view.IsValid())
    {
        engine->RemoveObject(view);
    }

    if(meshView.IsValid())
    {
        engine->RemoveObject(meshView);
    }

    if(meshSector.IsValid())
    {
        engine->RemoveObject(meshSector);
    }

    Clear();
}

bool pawsObjectView::Setup(iDocumentNode* node)
{
    rmTargets = scfQueryInterface<iRenderManagerTargets>(engine->GetRenderManager());
    if(!rmTargets.IsValid())
    {
        Error1("pawsObjectView: RenderManager doesn't support targets! Object views will be disabled");
        return true;
    }

    PawsManager::GetSingleton().AddObjectView(this);

    csRef<iDocumentNode> distanceNode = node->GetNode("distance");
    if(distanceNode)
        distance = distanceNode->GetAttributeValueAsFloat("value");
    else
        distance = 4;

    origDistance = distance;

    csRef<iDocumentNode> cameraModNode = node->GetNode("cameramod");
    if(cameraModNode)
    {
        cameraMod = csVector3(cameraModNode->GetAttributeValueAsFloat("x"),
                              cameraModNode->GetAttributeValueAsFloat("y"),
                              cameraModNode->GetAttributeValueAsFloat("z"));
    }

    csRef<iDocumentNode> mapNode = node->GetNode("map");
    if(mapNode)
    {
        csString mapFile = mapNode->GetAttributeValue("file");
        csString sector  = mapNode->GetAttributeValue("sector");

        return LoadMap(mapFile, sector);
    }
    else
    {
        Error1("pawsObjectView map failed to load because the mapNode doesn't exist!");
        return false;
    }

}

void pawsObjectView::OnResize()
{
    if(rmTargets.IsValid() && stageTarget.IsValid() && meshTarget.IsValid())
    {
        // unregister old texs from render targets
        rmTargets->UnregisterRenderTarget(stageTarget);
        rmTargets->UnregisterRenderTarget(meshTarget);
    }

    pawsWidget::OnResize();
    csRef<iTextureManager> texman = PawsManager::GetSingleton().GetGraphics3D()->GetTextureManager();
    meshTarget  = texman->CreateTexture(screenFrame.Width(), screenFrame.Height(), csimg2D, "rgba8",
                                        CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_SCALE_UP);
    stageTarget = texman->CreateTexture(screenFrame.Width(), screenFrame.Height(), csimg2D, "rgba8",
                                        CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS | CS_TEXTURE_SCALE_UP);

    if(meshTarget.IsValid() && stageTarget.IsValid())
    {
        meshTarget->SetAlphaType(csAlphaMode::alphaBinary);
        stageTarget->SetAlphaType(csAlphaMode::alphaBinary);
    }
    else
    {
        Error1("pawsObjectView couldn't create render target!");
    }

    // update the views
    int w, h;
    int originW = screenFrame.Width();
    int originH = screenFrame.Height();

    // update the stage view
    if(view)
    {
        stageTarget->GetRendererDimensions(w,h);
        view->SetWidth(w);
        view->SetHeight(h);
        view->SetRectangle(0, 0, w, h, false);
        view->GetCamera()->SetViewportSize(w, h);
        //we need to use the original height and widht of the window because we just resize the texture
        //to fill the whole space (and we don't do a relative rescaling so the fov would screw up if
        //we used the render size)
        view->GetPerspectiveCamera()->SetFOV((float)(originH)/originW, 1);
    }

    // update the doll view
    if(meshView)
    {
        meshTarget->GetRendererDimensions(w,h);
        meshView->SetWidth(w);
        meshView->SetHeight(h);
        meshView->SetRectangle(0, 0, w, h, false);
        meshView->GetCamera()->SetViewportSize(w, h);
        meshView->GetPerspectiveCamera()->SetFOV((float)(originH)/originW, 1);
    }

    needsDraw = true;
}

bool pawsObjectView::LoadMap(const char* map, const char* sector)
{
    csRef<iStringArray> zone;
    zone.AttachNew(new scfStringArray());
    zone->Push(map);
    if(!loader->LoadPriorityZones(zone))
    {
        Error2("Failed to load priority zone '%s'", map);
    }

    stage = engine->FindSector(sector);
    if(!stage)
    {
        Error2("couldn't find stage sector '%s'", sector);
        return false;
    }

    static uint sectorCount = 0;
    meshSector = engine->CreateSector(csString(sector).AppendFmt("%u", sectorCount++));

    meshView.AttachNew(new csView(engine, PawsManager::GetSingleton().GetGraphics3D()));
    meshView->SetAutoResize(false);
    meshView->GetCamera()->SetSector(meshSector);
    meshView->GetCamera()->GetTransform().SetOrigin(csVector3(0, 1, -distance));

    view.AttachNew(new csView(engine, PawsManager::GetSingleton().GetGraphics3D()));
    view->SetAutoResize(false);
    view->GetCamera()->SetSector(stage);
    view->GetCamera()->GetTransform().SetOrigin(csVector3(0, 1, -distance));

    return true;
}

bool pawsObjectView::ContinueLoad(bool onlyMesh)
{
    if(!rmTargets.IsValid())
        return true;

    if(loader->GetLoadingCount() == 0)
    {
        //we requested a loading because of a mesh change but no change
        //in the sector so no need to do other operations aside from
        //requesting the loader to complete loading
        //if, instead we are loading all, prepare also the rest
        if(!onlyMesh)
        {
            // precache stage
            stage->PrecacheDraw();

            // copy ambient light
            meshSector->SetDynamicAmbientLight(stage->GetDynamicAmbientLight());

            // copy static and pseudo-dynamic lights
            iLightList* lightList = meshSector->GetLights();
            iLightList* stageLightList = stage->GetLights();

            for(int i=0; i<stageLightList->GetCount(); i++)
            {
                lightList->Add(stageLightList->Get(i));
            }

            // precache mesh sector
            meshSector->PrecacheDraw();
        }
        return true;
    }
    else
    {
        loader->ContinueLoading(true);
        return false;
    }
}

bool pawsObjectView::View(const char* factName)
{
    csRef<iThreadReturn> factory = loader->LoadFactory(factName, true);

    if(!factory.IsValid())
    {
        Error2("failed to find factory %s", factName);
        return false;
    }
    else if(!factory->IsFinished() || !factory->WasSuccessful())
    {
        Error2("failed to load factory %s", factName);
        return false;
    }

    csRef<iMeshFactoryWrapper> meshfact = scfQueryInterface<iMeshFactoryWrapper>(factory->GetResultRefPtr());
    View(meshfact);
    meshFactory = factory;
    return true;
}

void pawsObjectView::View(iMeshFactoryWrapper* wrapper)
{
    Clear();

    CS_ASSERT(wrapper);
    if(wrapper)
    {
        mesh = engine->CreateMeshWrapper(wrapper, "PaperDoll", meshSector, csVector3(0,0,0));
    }
}

void pawsObjectView::View(iMeshWrapper* wrapper)
{
    CS_ASSERT(wrapper);
    if(wrapper)
    {
        View(wrapper->GetFactory());
    }
}

void pawsObjectView::Rotate(int speed,float radians)
{
    RotateTemp(speed,radians);
    orgRadians= rotateRadians;
    orgTime = rotateTime;
}

void pawsObjectView::Rotate(float radians)
{
    camRotate = radians;
}

void pawsObjectView::RotateDef()
{
    rotateTime = orgTime;
    rotateRadians = orgRadians;
}

void pawsObjectView::RotateTemp(int speed,float radians)
{
    rotateTime = speed;
    rotateRadians = radians;

    if(speed == -1)
    {
        rotateTime = 0; // Don't enter rotate code
        rotateRadians = 0;
        camRotate = 0;
    }
}

void pawsObjectView::Draw3D(iGraphics3D* /*graphics3D*/)
{
    if(!meshTarget.IsValid() || !stageTarget.IsValid() || !view || !meshView)
    {
        return;
    }

    // update the camera
    if(doRotate)
        DrawRotate();
    else
        DrawNoRotate();

    // draw the stage if needed
    if(needsDraw)
    {
        rmTargets->RegisterRenderTarget(stageTarget, view, 0, iRenderManagerTargets::updateOnce | iRenderManagerTargets::clearScreen);
        rmTargets->MarkAsUsed(stageTarget);
        engine->GetRenderManager()->RenderView(view);
    }

    // draw the mesh
    rmTargets->RegisterRenderTarget(meshTarget, meshView, 0, iRenderManagerTargets::updateOnce | iRenderManagerTargets::clearScreen);
    rmTargets->MarkAsUsed(meshTarget);
    engine->GetRenderManager()->RenderView(meshView);

    needsDraw = doRotate && !cameraLocked;
}

void pawsObjectView::Draw()
{
    graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
    iGraphics3D* graphics3D = PawsManager::GetSingleton().GetGraphics3D();
    int w = screenFrame.Width();
    int h = screenFrame.Height();

    if(stageTarget.IsValid())
    {
        graphics3D->DrawPixmap(stageTarget,
                               screenFrame.xmin, screenFrame.ymin, w, h, 0, 0, w, h);
    }

    if(meshTarget.IsValid())
    {
        graphics3D->DrawPixmap(meshTarget,
                               screenFrame.xmin, screenFrame.ymin, w, h, 0, 0, w, h);
    }
    pawsWidget::Draw();
}

void pawsObjectView::LockCamera(csVector3 where, csVector3 at, bool mouseBreak, bool mouseRotate)
{
    cameraLocked = true;
    oldPosition = cameraPosition;
    oldLookAt = lookingAt;
    mouseDownUnlock = mouseBreak;
    mouseDownRotate = mouseRotate;

    doRotate = false;
    cameraPosition = where;
    lookingAt = at;
}

void pawsObjectView::UnlockCamera()
{
    if(cameraLocked)
    {
        cameraPosition = oldPosition;
        lookingAt = oldLookAt;

        doRotate = true;
        cameraLocked = false;
    }
}

void pawsObjectView::DrawNoRotate()
{
    view->GetCamera()->GetTransform().SetOrigin(cameraPosition);
    view->GetCamera()->GetTransform().LookAt(lookingAt-cameraPosition, csVector3(0, 1, 0));

    meshView->GetCamera()->GetTransform().SetOrigin(cameraPosition);
    meshView->GetCamera()->GetTransform().LookAt(lookingAt-cameraPosition, csVector3(0, 1, 0));
}


void pawsObjectView::DrawRotate()
{
    if(spinMouse)
    {
        // NOTE: Y isn't used, but I will do it here if we need it later
        psPoint pos = PawsManager::GetSingleton().GetMouse()->GetPosition();
        csVector2 blur;
        blur.Set(downPos);

        // Unite pos for all reses
        pos.x = (pos.x * 800) / graphics2D->GetWidth();
        pos.y = (pos.y * 600) / graphics2D->GetHeight();

        blur.x = (blur.x * 800) / graphics2D->GetWidth();
        blur.y = (blur.y * 600) / graphics2D->GetHeight();

        // Scale down, we want blurry positions
        pos.x = pos.x / 10;
        pos.y = pos.y / 10;
        blur.x = blur.x / 10;
        blur.y = blur.y / 10;

        float newRad;
        newRad = pos.x - blur.x;
        newRad /= 100;
        if(newRad != rotateRadians)
        {
            RotateTemp(10,newRad);
        }
    }

    if(rotateTime != 0)
    {
        static unsigned int ticks = csGetTicks();
        if(csGetTicks() > ticks + rotateTime)
        {
            ticks = csGetTicks();
            camRotate += rotateRadians;

            float currentAngle = (camRotate*180)/TWO_PI;
            if(currentAngle > 180.0f)
            {
                camRotate = camRotate - TWO_PI; // (180*TWO_PI)/180 = TWO_PI
            }
        }
    }

    csVector3 camera;
    if(cameraLocked)
    {
        float lDistance = (cameraPosition - lookingAt).Norm();

        camera.x = cameraPosition.x - sinf(camRotate) * lDistance;
        camera.y = cameraPosition.y;
        camera.z = cameraPosition.z + (1 + cosf(camRotate - PI)) * lDistance;

        view->GetCamera()->GetTransform().SetOrigin(camera);
        view->GetCamera()->GetTransform().LookAt(lookingAt - camera, csVector3(0, 1, 0));

        meshView->GetCamera()->GetTransform().SetOrigin(camera);
        meshView->GetCamera()->GetTransform().LookAt(lookingAt - camera, csVector3(0, 1, 0));
    }
    else
    {
        camera.x = objectPos.x + sinf(camRotate)*((-distance)-1);
        camera.y = 1;
        camera.z = objectPos.z + cosf(camRotate)*((-distance)-1);

        view->GetCamera()->GetTransform().SetOrigin(camera);
        view->GetCamera()->GetTransform().LookAt(objectPos - camera + cameraMod, csVector3(0, 1, 0));

        meshView->GetCamera()->GetTransform().SetOrigin(camera);
        meshView->GetCamera()->GetTransform().LookAt(objectPos - camera + cameraMod, csVector3(0, 1, 0));
    }
}

bool pawsObjectView::OnMouseDown(int button, int /*mod*/, int x, int y)
{
    if(!mouseControlled)
        return false;

    if(mouseDownUnlock && button == csmbRight)
    {
        UnlockCamera();
        mouseDownUnlock = false;
        return true;
    }

    if(mouseDownRotate)
        doRotate = true;

    spinMouse = true;
    downPos.Set(x,y);
    downTime = csGetTicks();
    return true;
}

bool pawsObjectView::OnMouseUp(int /*button*/, int /*mod*/, int x, int y)
{
    if(!mouseControlled)
        return false;

    // 1 sec and about the same pos
    if(csGetTicks() - downTime < 1000 && int(downPos.x / 10) == int(x/10) && int(downPos.y / 10) == int(y/10))
    {
        downTime = 0;
        // Click == stop or begin
        if(rotateTime != 0)
            RotateTemp(-1,0);
        else
            RotateDef();

        spinMouse = false;
    }
    else if(spinMouse)
    {
        spinMouse = false;
        RotateTemp(-1,0);
    }
    return true;
}

bool pawsObjectView::OnMouseExit()
{
    if(!mouseControlled)
        return false;

    if(spinMouse)
    {
        spinMouse = false;
        RotateTemp(-1,0);
    }
    return true;
}

void pawsObjectView::Clear()
{
    // remove the mesh
    if(mesh.IsValid())
    {
        engine->RemoveObject(mesh);
        mesh.Invalidate();
    }

    // notify the loader that we don't need the factory, anymore
    meshFactory.Invalidate();
}

