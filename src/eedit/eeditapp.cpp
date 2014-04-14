/*
* Author: Andrew Robberts
*
* Copyright (C) 2004 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "eeditapp.h"
#include "eeditglobals.h"

#include <csutil/xmltiny.h>
#include <iutil/csinput.h>
#include <iutil/cfgmgr.h>
#include <iutil/event.h>
#include <csutil/event.h>
#include <iutil/eventq.h>
#include <iutil/objreg.h>
#include <iutil/plugin.h>
#include <iutil/vfs.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>
#include <cstool/csview.h>
#include <iengine/mesh.h>
#include <iengine/movable.h>
#include <csutil/flags.h>
#include <imesh/spritecal3d.h>
#include <imesh/object.h>
#include <csutil/scf.h>
#include <iutil/object.h>
#include <igraphic/imageio.h>
#include <csutil/stringarray.h>
#include <csutil/floatrand.h>
#include <iutil/stringarray.h>
#include <csutil/databuf.h>
#include <csutil/scfstringarray.h>

#include "paws/pawsmanager.h"
#include "paws/pawsmainwidget.h"

#include "eeditreporter.h"
#include "pawseedit.h"

#include "util/pscssetup.h"

#include "effects/pseffectmanager.h"
#include "effects/pseffect.h"

#include "pscal3dcallback.h"

#include "econtrolmanager.h"

#include "eedittoolboxmanager.h"
#include "eeditinputboxmanager.h"
#include "eeditpositiontoolbox.h"
#include "eedittargettoolbox.h"
#include "eeditloadeffecttoolbox.h"
#include "eeditpartlisttoolbox.h"
#include "eeditediteffecttoolbox.h"
#include "eeditloadmaptoolbox.h"
#include "eediterrortoolbox.h"
#include "eeditfpstoolbox.h"
#include "eeditshortcutstoolbox.h"
#include "eeditrequestcombo.h"
#include "eeditrendertoolbox.h"

#include <ibgloader.h>

const char * EEditApp::CONFIG_FILENAME = "/this/eedit.cfg";
const char * EEditApp::APP_NAME        = "planeshift.eedit.application";
const char * EEditApp::WINDOW_CAPTION  = "PlaneShift Effect Editor";
const char * EEditApp::KEY_DEFS_FILENAME = "/this/data/eedit/keys_def.xml";

CS_IMPLEMENT_APPLICATION

EEditApp *editApp;
/* FIXME NAMESPACE */

EEditApp::EEditApp(iObjectRegistry * obj_reg, EEditReporter * _reporter)
            :camFlags(CAM_COUNT)
{
    object_reg  = obj_reg;
    paws        = 0;
    drawScreen  = true;
    editWindow  = 0;

    reporter = _reporter;

    toolboxManager = 0;
    inputboxManager = 0;
    
    currEffectID = 0;
    effectLoaded = false;
    effectPaused = false;

    camFlags.Clear();
    camYaw = 0.0f;
    camPitch = 0.0f;

    controlManager = 0;
    cal3DCallbackLoader.AttachNew(new psCal3DCallbackLoader(obj_reg, this));
    
    rand = new csRandomFloatGen();
    rand->Initialize();
}

EEditApp::~EEditApp()
{
    delete rand;

    if (event_handler && queue)
        queue->RemoveListener(event_handler);

    delete paws;
    
    delete toolboxManager;
    delete inputboxManager;
    
    delete controlManager;
    
    // delete of effectManager is missing? TODO
}

void EEditApp::SevereError(const char* msg)
{
    csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, APP_NAME, "%s", msg);
}

bool EEditApp::Init()
{
    queue =  csQueryRegistry<iEventQueue> (object_reg);
    if (!queue)
    {
        SevereError("No iEventQueue plugin!");
        return false;
    }

    vfs =  csQueryRegistry<iVFS> (object_reg);
    if (!vfs)
    {
        SevereError("No iVFS plugin!");
        return false;
    }

    engine =  csQueryRegistry<iEngine> (object_reg);
    if (!engine)
    {
        SevereError("No iEngine plugin!");
        return false;
    }

    cfgmgr =  csQueryRegistry<iConfigManager> (object_reg);
    if (!cfgmgr)
    {
        SevereError("No iConfigManager plugin!");
        return false;
    }

    g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
    {
        SevereError("No iGraphics3D plugin!");
        return false;
    }

    txtmgr = g3d->GetTextureManager();
    if (!txtmgr)
    {
        SevereError("Couldn't load iTextureManager!");
        return false;
    }

    g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        SevereError("Could not load iGraphics2D!");
        return false;
    }
  
    vc =  csQueryRegistry<iVirtualClock> (object_reg);
    if (!vc)
    {
        SevereError("Could not load iVirtualClock!");
        return false;
    }

    loader =  csQueryRegistry<iLoader> (object_reg);
    if (!loader)
    {
        SevereError("Could not load iLoader!");
        return false;
    }

    // set the window caption
    iNativeWindow *nw = g3d->GetDriver2D()->GetNativeWindow();
    if (nw)
        nw->SetTitle(WINDOW_CAPTION);

    // loads materials, meshes and maps
    csRef<iBgLoader> loader = csQueryRegistry<iBgLoader>(object_reg);
    if (!loader)
    {
        SevereError("Could not load iBgLoader!");
        return false;
    }

    loader->PrecacheDataWait("/planeshift/materials/materials.cslib");

    csRef<iStringArray> meshes = vfs->FindFiles("/planeshift/meshes/");
    for(size_t j=0; j<meshes->GetSize(); ++j)
    {
        loader->PrecacheDataWait(meshes->Get(j));
    }

    csRef<iStringArray> maps = vfs->FindFiles("/planeshift/world/");

    for(size_t j=0; j<maps->GetSize(); ++j)
    {
        loader->PrecacheDataWait(maps->Get(j));
    }
    //CPrintf(CON_CMDOUTPUT,"Loader cache filled");

    csRef<iStringArray> regions;
    regions.AttachNew(new scfStringArray());
    regions->Push("npcroom1");

    loader->LoadZones(regions, true);
    while (loader->GetLoadingCount() != 0)
    {
        loader->ContinueLoading(true);
    }
    
    // paws initialization
    paws = new PawsManager(object_reg, "/this/art/eedit.zip", NULL);
    if (!paws)
    {
        SevereError("Could not initialize PAWS!");
        return false;
    }
    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    toolboxManager = new EEditToolboxManager();
    inputboxManager = new EEditInputboxManager();

    RegisterFactories();

    // Load and assign a default button click sound for pawsbutton
    //paws->LoadSound("/this/art/music/gui/ccreate/next.wav","sound.standardButtonClick");

    if (!LoadWidgets())
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Some PAWS widgets failed to load");
    }

    paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

    editWindow =     (pawsEEdit *)          paws->FindWidget("eedit");
    if (!editWindow)
    {
        SevereError("Could not load edit window!");
        return false;
    }

    // Register our event handler
    event_handler.AttachNew(new EventHandler (this));
    csEventID esub[] = {
	  csevFrame (object_reg),
	  csevMouseEvent (object_reg),
	  csevKeyboardEvent (object_reg),
	  CS_EVENTLIST_END
    };
    queue->RegisterListener(event_handler, esub);

    // Inform debug that everything initialized successfully
    csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
        "Application initialized successfully.");

    ((EEditLoadMapToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_LOAD_MAP))->SetMapFile(editWindow->GetMapFile());
    
    // effect stuff
    effectManager.AttachNew(new psEffectManager(object_reg));
    //effectManager->LoadFromEffectsList("/this/data/effects/effectslist.xml", editWindow->GetView());
    //effectManager->Prepare();
    
    pos_anchor = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "pos_anchor", editWindow->GetView()->GetCamera()->GetSector(), csVector3(-33,0.1f,-194));
    tar_anchor = engine->CreateMeshWrapper("crystalspace.mesh.object.null", "tar_anchor", editWindow->GetView()->GetCamera()->GetSector(), csVector3(-43,0.1f,-194));
    SetDefaultActorPositions();

    RefreshEffectList();
    ((EEditParticleListToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_PARTICLES))->FillList(engine);

    ((EEditRenderToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_RENDER))->SetEngine(engine);

    controlManager = new eControlManager(object_reg);
    if (!controlManager->LoadKeyMap(KEY_DEFS_FILENAME))
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
                "Failed to load keymap file. File doesn't exist or parse error.");
    }

    LoadPositionMesh("/this/art/eedit/stonebm/stonebm.cal3d");
    LoadTargetMesh("/this/art/eedit/gobble/gobble.cal3d");
   
    CreateEffectPathList("/this/data/effects");
    
    CreateShortcuts();

    HideToolbox(EEditToolbox::T_EDIT_EFFECT);

    // Set the default camera height around the character's eye level
    csVector3 eyeLevel = csVector3(0,1,0); // Default eye level height

    csVector3 actorPos = csVector3(0,0,0);
    if (pos_anchor->GetMovable())
    actorPos = pos_anchor->GetMovable()->GetPosition();

    iCamera *camera = editWindow->GetView()->GetCamera();
    csVector3 cameraPos = camera->GetTransform().This2Other(csVector3(0,0,0));

    cameraPos.x=0;
    cameraPos.y=actorPos.y-cameraPos.y;
    cameraPos.z=0;

    camera->Move(cameraPos + eyeLevel);

    return true;
}

bool EEditApp::LoadWidget(const char * file)
{
    if (!paws->LoadWidget(file))
    {
        csString err = "Warning: Loading '" + csString(file) + "' failed!";
        SevereError(err);
        return false;
    }
    return true;
}

bool EEditApp::LoadWidgets()
{
    bool succeeded = true;

    succeeded &= LoadWidget("data/eedit/eedit.xml");

    if (!toolboxManager->LoadWidgets(paws))
    {
        SevereError("Warning: Loading toolboxes failed!");
        succeeded = false;
    }

    if (!inputboxManager->LoadWidgets(paws))
    {
        SevereError("Warning: Loading inputboxes failed!");
        succeeded = false;
    }

    // Load all other custom made paws widgets here
    succeeded &= LoadWidget("data/eedit/requestcombo.xml");

    return succeeded;
}

bool EEditApp::HandleEvent(iEvent &ev)
{
    if (ev.Name == csevKeyboardDown (object_reg) || ev.Name == csevKeyboardUp (object_reg))
    {
        EEditShortcutsToolbox * shortcutsToolbox = (EEditShortcutsToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_SHORTCUTS);
        if (!shortcutsToolbox->IsVisible())
        {
            csKeyEventData event;
            csKeyEventHelper::GetEventData(&ev, event);
            if (event.eventType == csKeyEventTypeDown)
            {
                int modifiers = 0;
                if (event.modifiers.modifiers[csKeyModifierTypeAlt] != 0)
                    modifiers |= CSMASK_ALT;
                if (event.modifiers.modifiers[csKeyModifierTypeShift] != 0)
                    modifiers |= CSMASK_SHIFT;
                if (event.modifiers.modifiers[csKeyModifierTypeCtrl] != 0)
                    modifiers |= CSMASK_CTRL;

                shortcutsToolbox->ExecuteShortcutCommand(event.codeRaw, modifiers);
            }
        }
    }
    else if (ev.Name == csevMouseMove (object_reg, 0))
    {
        csMouseEventData event;
        csMouseEventHelper::GetEventData(&ev,event);
        mousePointer.x = event.x;
        mousePointer.y = event.y;
    }

    if (paws->HandleEvent(ev))
        return true;

    if (controlManager->HandleEvent(ev))
        return true;

    if (ev.Name == csevFrame (object_reg))
    {
        Update();

        if (drawScreen)
        {
            g3d->BeginDraw(CSDRAW_2DGRAPHICS);
            paws->Draw();
        }
        else
        {
            csSleep(150);
        }

        g3d->FinishDraw ();
        g3d->Print (0);

        return true;
    }
    else if (ev.Name == csevCanvasHidden (object_reg, g3d->GetDriver2D ()))
        drawScreen = false;
    else if (ev.Name == csevCanvasExposed (object_reg, g3d->GetDriver2D ()))
        drawScreen = true;
    return false;
}

void EEditApp::RefreshEffectList()
{
    ((EEditErrorToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_ERROR))->SetLoadingEffects(true);
    CreateEffectPathList("/this/data/effects");
    effectManager->Clear();
    effectManager->LoadFromDirectory("/this/data/effects", true, editWindow->GetView());
    ((EEditErrorToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_ERROR))->SetLoadingEffects(false);

    ((EEditLoadEffectToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_LOAD_EFFECT))->FillList(effectManager);
    ((EEditLoadEffectToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_LOAD_EFFECT))->SelectEffect(currEffectName);
}

bool EEditApp::UpdateEffect()
{
    CancelEffect();

    currEffectID = effectManager->RenderEffect(currEffectName, csVector3(0,0,0), pos_anchor, tar_anchor);
    
    if (currEffectID == 0)
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Problem when rendering effect!");
        return false;
    }
    effectManager->Update(1);
    return true;
}

bool EEditApp::RenderEffect(const csString &effectName)
{
    CancelEffect();

    currEffectID = effectManager->RenderEffect(effectName, csVector3(0,0,0), pos_anchor, tar_anchor);
    //effectManager->FindEffect(currEffectID)->SetText("BLAH");
    
    if (currEffectID == 0)
    {
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Problem when rendering effect!");
        return false;
    }
    effectManager->Update(1);

    ((EEditRenderToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_RENDER))->LoadEffect(effectManager, effectName);

    return true;
}

bool EEditApp::RenderCurrentEffect()
{
    return RenderEffect(currEffectName);
}

void EEditApp::CreateParticleSystem(const csString & name)
{
    iMeshFactoryWrapper* fact = engine->FindMeshFactory (name);
    if (fact)
    {
        if (particleSystem)
	{
	    engine->RemoveObject(particleSystem);
	    particleSystem = 0;
	}
	csOrthoTransform& trans = editWindow->GetView()->GetCamera()->GetTransform();
	csVector3 pos = trans.This2Other (csVector3 (0, 0, 2));
        particleSystem = engine->CreateMeshWrapper (fact, name, 
                  editWindow->GetView()->GetCamera()->GetSector(), pos);
    }
}

bool EEditApp::ReloadCurrentEffect()
{
    effectLoaded = true;

   ((EEditErrorToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_ERROR))->SetLoadingEffects(true);
    effectManager->Clear();
    effectManager->LoadEffects(currEffectLoc, editWindow->GetView());
    ((EEditErrorToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_ERROR))->SetLoadingEffects(false);

    effectLoaded = true;
    psEffect* eff = effectManager->FindEffect(currEffectName);
    ((EEditEditEffectToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_EDIT_EFFECT))->LoadEffect(eff);

    ((EEditRenderToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_RENDER))->LoadEffect(effectManager, currEffectName);

    return true;
}
    
bool EEditApp::TogglePauseEffect()
{
    effectPaused = !effectPaused;
    return effectPaused;
}

bool EEditApp::CancelEffect()
{
    return effectManager->DeleteEffect(currEffectID);
}

csPtr<iMeshWrapper> EEditApp::LoadCustomMesh(const csString & meshFile, const csString & name, csVector3 pos)
{
    if (meshFile.IsEmpty())
    {
        return engine->CreateMeshWrapper("crystalspace.mesh.object.null",
                  name, editWindow->GetView()->GetCamera()->GetSector(), pos);
    }
    else if (meshFile == "axis")
    {
        return csPtr<iMeshWrapper> (0);
    }
    else
    {
        csRef<iMeshFactoryWrapper> meshFact = 
            loader->LoadMeshObjectFactory(meshFile);
        if (meshFact)
        {
            csRef<iMeshWrapper> mesh = engine->CreateMeshWrapper(meshFact, name, editWindow->GetView()->GetCamera()->GetSector(), pos);
             
            csRef<iSpriteCal3DState> cal3d =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
            if (cal3d)
            {
                cal3d->SetUserData((void *)(iMeshWrapper *)mesh);
            }
            
            mesh->IncRef();
            return (iMeshWrapper *)mesh;
        }
        else
        {
            return engine->CreateMeshWrapper("crystalspace.mesh.object.null",
                  name, editWindow->GetView()->GetCamera()->GetSector(), pos);
        }
    }
}

bool EEditApp::LoadPositionMesh(const csString & meshFile)
{
    csVector3 pos = csVector3(0,0,0);
    if (pos_anchor)
    {
        if (pos_anchor->GetMovable())
            pos = pos_anchor->GetMovable()->GetPosition();
        engine->RemoveObject(pos_anchor);
    }

    pos_anchor = LoadCustomMesh(meshFile, "pos_anchor", pos);
    if (!pos_anchor)
    {
        printf("ERROR: Couldn't load position mesh: %s\n", meshFile.GetData());
        return false;
    }
    
    csRef<iSpriteCal3DState> cal3d =  scfQueryInterface<iSpriteCal3DState> (pos_anchor->GetMeshObject());
    if (cal3d)
    {
        cal3d->SetAnimCycle(0, 1);
        ((EEditPositionToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_POSITION))->FillAnimList(cal3d);
    }
    
    return true;
}

bool EEditApp::LoadTargetMesh(const csString & meshFile)
{
    csVector3 pos = csVector3(0,0,0);
    if (tar_anchor)
    {
        if (tar_anchor->GetMovable())
            pos = tar_anchor->GetMovable()->GetPosition();
        engine->RemoveObject(tar_anchor);
    }

    tar_anchor = LoadCustomMesh(meshFile, "pos_anchor", pos);
    if (!tar_anchor)
    {
        printf("ERROR: Couldn't load position mesh: %s\n", meshFile.GetData());
        return false;
    }
    
    csRef<iSpriteCal3DState> cal3d =  scfQueryInterface<iSpriteCal3DState> (tar_anchor->GetMeshObject());
    if (cal3d)
    {
        cal3d->SetAnimCycle(0, 1);
        ((EEditTargetToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_TARGET))->FillAnimList(cal3d);
    }
    
    return true;
}

void EEditApp::SetPositionAnim(size_t index)
{
    csRef<iSpriteCal3DState> cal3d =  scfQueryInterface<iSpriteCal3DState> (pos_anchor->GetMeshObject());
    if (cal3d)
        cal3d->SetAnimAction((int)index, .25f, .5f);

}

void EEditApp::SetTargetAnim(size_t index)
{
    csRef<iSpriteCal3DState> cal3d =  scfQueryInterface<iSpriteCal3DState> (tar_anchor->GetMeshObject());
    if (cal3d)
        cal3d->SetAnimAction((int)index, .25f, .5f);
}

void EEditApp::SetPositionData(const csVector3 & pos, float rot)
{
    if (!pos_anchor)
    {
        printf("WARNING: Attempting to move the position anchor when it doesn't exist!\n");
        return;
    }
    pos_anchor->GetMovable()->SetPosition(pos);
    pos_anchor->GetMovable()->SetTransform(csYRotMatrix3(rot));
    pos_anchor->GetMovable()->UpdateMove();
}

void EEditApp::SetTargetData(const csVector3 & pos, float rot)
{
    if (!tar_anchor)
    {
        printf("WARNING: Attempting to move the target anchor when it doesn't exist!\n");
        return;
    }
    tar_anchor->GetMovable()->SetPosition(pos);
    tar_anchor->GetMovable()->SetTransform(csYRotMatrix3(rot));
    tar_anchor->GetMovable()->UpdateMove();
}   

bool EEditApp::SetCurrEffect(const csString & name)
{
    // figure out where this effect is located
    currEffectLoc.Clear();
    for (size_t a=0; a<effectPaths.GetSize(); ++a)
    {
        if (effectPaths[a].name == name)
        {
            currEffectLoc = effectPaths[a].path;
            break;
        }
    }
    
    currEffectName = name;
    //((EEditLoadEffectToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_LOAD_EFFECT))->FillList(effectManager);
   ((EEditLoadEffectToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_LOAD_EFFECT))->SelectEffect(name);
    effectLoaded = false;
    ReloadCurrentEffect();
    return true;
}

csString EEditApp::GetCurrEffectName()
{
    return currEffectName;
}

csString EEditApp::GetCurrParticleSystemName()
{
    if (particleSystem)
	return particleSystem->QueryObject ()->GetName ();
    else
	return "";
}

bool EEditApp::SetDefaultActorPositions()
{
    iCamera * cam = editWindow->GetView()->GetCamera();
    pos_anchor->GetMovable()->SetSector(cam->GetSector());
    tar_anchor->GetMovable()->SetSector(cam->GetSector());
    csVector3 origin = cam->GetTransform().This2Other(csVector3(0,0,5));
    origin.y = 0;
    ((EEditPositionToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_POSITION))->SetPosition(origin);
    ((EEditTargetToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_TARGET))->SetPosition(origin + csVector3(10, 0, 5));
    return true;
}

bool EEditApp::LoadMap(const char * mapFile)
{
    if (!editWindow->LoadMap(mapFile))
    {
        Error2("Couldn't load map %s\n", mapFile);
        return false;
    }
    return SetDefaultActorPositions();
}

csVector2 EEditApp::GetMousePointer()
{
    return mousePointer;
}

void EEditApp::Exit()
{
    queue->GetEventOutlet()->Broadcast (csevQuit (object_reg));
}

iObjectRegistry * EEditApp::GetObjectRegistry() const
{
    return object_reg;
}

csRef<iEngine> EEditApp::GetEngine() const        
{
    return engine;
}

csRef<iVFS> EEditApp::GetVFS() const           
{
    return vfs;
}

csRef<iEventQueue> EEditApp::GetEventQueue() const    
{
    return queue;
}

csRef<iConfigManager> EEditApp::GetConfigManager() const 
{
    return cfgmgr;
}

csRef<iTextureManager> EEditApp::GetTextureManager() const
{
    return txtmgr; 
}

csRef<iGraphics3D> EEditApp::GetGraphics3D() const    
{
    return g3d;
}

csRef<iGraphics2D> EEditApp::GetGraphics2D() const    
{
    return g2d;
}

csRef<iVirtualClock> EEditApp::GetVirtualClock() const  
{
    return vc;
}

csRef<iLoader> EEditApp::GetLoader() const        
{
    return loader; 
}

EEditReporter * EEditApp::GetReporter() const
{
    return reporter;
}

const char * EEditApp::GetConfigString(const char * key, const char * defaultValue) const
{
    return cfgmgr->GetStr(key, defaultValue);
}

void EEditApp::SetConfigString(const char * key, const char * value)
{
    csRef<iConfigFile> config = cfgmgr->LookupDomain(EEditApp::CONFIG_FILENAME);
    config->SetStr(key, value);
}

float EEditApp::GetConfigFloat(const char * key, float defaultValue) const
{
    return cfgmgr->GetFloat(key, defaultValue);
}

void EEditApp:: SetConfigFloat(const char * key, float value)
{
    csRef<iConfigFile> config = cfgmgr->LookupDomain(EEditApp::CONFIG_FILENAME);
    config->SetFloat(key, value);
}

bool EEditApp::GetConfigBool(const char * key, bool defaultValue) const
{
    return cfgmgr->GetBool(key, defaultValue);
}

void EEditApp::SetConfigBool(const char * key, bool value)
{
    csRef<iConfigFile> config = cfgmgr->LookupDomain(EEditApp::CONFIG_FILENAME);
    config->SetBool(key, value);
}

void EEditApp::ExecuteCommand(const char * cmd)
{
    if (strcmp(cmd, "PositionToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_POSITION);
    else if (strcmp(cmd, "TargetToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_TARGET);
    else if (strcmp(cmd, "CameraToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_CAMERA);
    else if (strcmp(cmd, "RenderToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_RENDER);
    else if (strcmp(cmd, "LoadEffectToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_LOAD_EFFECT);
    else if (strcmp(cmd, "LoadPartListToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_PARTICLES);
    else if (strcmp(cmd, "EditEffectToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_EDIT_EFFECT);
    else if (strcmp(cmd, "LoadMapToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_LOAD_MAP);
    else if (strcmp(cmd, "ErrorPrompt.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_ERROR);
    else if (strcmp(cmd, "FPSToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_FPS);
    else if (strcmp(cmd, "ShortcutsToolbox.ToggleVisibility") == 0)
        ToggleToolbox(EEditToolbox::T_SHORTCUTS);
}

bool EEditApp::CreateEffectPathList(const csString & path, bool clear)
{
    if (clear)
        effectPaths.DeleteAll();

    csRef<iVFS> vfs =  csQueryRegistry<iVFS> (psCSSetup::object_reg);
    assert(vfs);

    if (!vfs->Exists(path))
    {
        Error2("Failed to load effects from directory; %s does not exist", path.GetData());
        return false;
    }

    if (!vfs->ChDir(path))
    {
        Error2("Failed to load effects from directory; %s read failed", path.GetData());
        return false;
    }

    csRef<iStringArray> files = vfs->FindFiles("*");
    for (size_t a=0; a<files->GetSize(); ++a)
    {
        const char * file = files->Get(a);
        if (file[0] == '.')
            continue;

        if (strcmp(file, path) == 0)
            continue;

        size_t len = strlen(file);
        char lastChar = file[len-1];
        if (lastChar == '/' || lastChar == '\\')
        {
            if (!CreateEffectPathList(file, false))
                return false;
        }
        else if (len > 4 && ((lastChar == 'f' || lastChar == 'F') && (file[len-2] == 'f' || file[len-2] == 'F') && (file[len-3] == 'e' || file[len-3] == 'E') && file[len-4] == '.'))
        {
            AppendEffectPathList(files->Get(a), path);
        }
    }
    return true;
}

bool EEditApp::AppendEffectPathList(const csString & effectFile, const csString & sourcePath)
{
    if (!vfs->Exists(effectFile))
        return false;

    csRef<iDocument> doc;
    csRef<iDocumentNodeIterator> xmlbinds;
    csRef<iDocumentSystem>  xml;
    const char* error;

    csRef<iDataBuffer> buff = vfs->ReadFile(effectFile);
    if (buff == 0)
    {
        Error2("Could not find file: %s", effectFile.GetData());
        return false;
    }
    xml = csQueryRegistry<iDocumentSystem>(object_reg);
    assert(xml);
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        printf("Parse error in %s: %s", effectFile.GetData(), error);
        return false;
    }
    if (doc == 0)
        return false;

    csRef<iDocumentNode> root = doc->GetRoot();
    if (root == 0)
    {
        Error1("No root in XML");
        return false;
    }

    csRef<iDocumentNode> listNode = root->GetNode("library");
    if (listNode == 0)
    {
        Error1("No <library> tag");
        return false;
    }

    listNode = listNode->GetNode("addon");
    if (listNode != 0)
    {
        xmlbinds = listNode->GetNodes("effect");
        csRef<iDocumentNode> effectNode;
        while (xmlbinds->HasNext())
        {
            effectNode = xmlbinds->Next();
            effectPaths.Push(EffectPath(effectNode->GetAttributeValue("name"), effectFile));
        }
    }
    return true;
}
    
void EEditApp::CreateShortcuts()
{
    EEditShortcutsToolbox * shortcuts = (EEditShortcutsToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_SHORTCUTS);
    if (!shortcuts)
        return;

    shortcuts->AddShortcut("PositionToolbox.ToggleVisibility");
    shortcuts->AddShortcut("TargetToolbox.ToggleVisibility");
    shortcuts->AddShortcut("CameraToolbox.ToggleVisibility");
    shortcuts->AddShortcut("RenderToolbox.ToggleVisibility");
    shortcuts->AddShortcut("LoadEffectToolbox.ToggleVisibility");
    shortcuts->AddShortcut("EditEffectToolbox.ToggleVisibility");
    shortcuts->AddShortcut("LoadPartListToolbox.ToggleVisibility");
    shortcuts->AddShortcut("LoadMapToolbox.ToggleVisibility");
    shortcuts->AddShortcut("ErrorPrompt.ToggleVisibility");
    shortcuts->AddShortcut("FPSToolbox.ToggleVisibility");
    shortcuts->AddShortcut("ShortcutsToolbox.ToggleVisibility");
}

void EEditApp::RegisterFactories()
{
    pawsWidgetFactory* factory;
    
    factory = new pawsEEditFactory();
    CS_ASSERT(factory);

    // Use the toolbox manager to register all toolbox
    // window factories
    toolboxManager->RegisterFactories();

    // Use the inputbox manager to register all inputbox
    // factories
    inputboxManager->RegisterFactories();

    // Register the factories of all other custom made
    // paws widgets here
    factory = new EEditRequestComboFactory();
    CS_ASSERT(factory);
}

void EEditApp::Update()
{
    csTicks elapsedTicks = vc->GetElapsedTicks();
    float elapsedSeconds = elapsedTicks / 1000.0f;
   
    // fps limiter
    float fps =       ((EEditFPSToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_FPS))->GetFPS();

    if (fps > 0)
    {
        float fpsTarget = ((EEditFPSToolbox *)toolboxManager->GetToolbox(EEditToolbox::T_FPS))->GetTargetFPS();

        int desiredWait = (int)(1000.0f / (fpsTarget*1.2f));
        int sleepTime = desiredWait - (int)elapsedTicks;
        if (sleepTime > 0)
            csSleep(sleepTime);
    }

    static float ts = 5.0f;
    if (ts > 0.0f)
    {
        ts -= elapsedSeconds;
        if (ts <= 0.0f)
        {
//            TakeScreenshot(csString("/this/eedit.jpg"));
//            Exit();
        }
    }
    
    
    iCamera *c = editWindow->GetView()->GetCamera();
    
    if (GetCamFlag(CAM_FORWARD))
        c->Move(CS_VEC_FORWARD * 6.0f * elapsedSeconds);
    if (GetCamFlag(CAM_BACK))
        c->Move(CS_VEC_BACKWARD * 6.0f * elapsedSeconds);

    if (GetCamFlag(CAM_UP))
        camPitch += 1.0f * elapsedSeconds;
    if (GetCamFlag(CAM_DOWN))
        camPitch -= 1.0f * elapsedSeconds;
    
    if (GetCamFlag(CAM_RIGHT))
        camYaw += 1.0f * elapsedSeconds;
    if (GetCamFlag(CAM_LEFT))
        camYaw -= 1.0f * elapsedSeconds;
    
    /*    
    static csVector3 turbulenceDir(0,0,0);
    static float turbulence = 0.1f;
    
    turbulenceDir += csVector3(rand->Get(-turbulence - turbulenceDir.x*1.5f, turbulence - turbulenceDir.x*1.5f),
                               rand->Get(-turbulence - turbulenceDir.y*1.5f, turbulence - turbulenceDir.y*1.5f),
                               rand->Get(-turbulence - turbulenceDir.z*1.5f, turbulence - turbulenceDir.z*1.5f));
    
    c->Move(turbulenceDir);                                                      
    */
        
    csMatrix3 rot = csXRotMatrix3(camPitch) * csYRotMatrix3(camYaw);
    csOrthoTransform ot(rot, c->GetTransform().GetOrigin());
    c->SetTransform(ot);

    if (!effectPaused)
        effectManager->Update(elapsedTicks);
    
    //if (editWindow)
    //    editWindow->Update(elapsedSeconds);

    if (toolboxManager)
        toolboxManager->UpdateAll(elapsedTicks);
}

bool EEditApp::ToggleToolbox(size_t toolbox)
{
    if (IsToolboxVisible(toolbox))
    {
        HideToolbox(toolbox);
        return false;
    }
    else
    {
        ShowToolbox(toolbox);
        return true;
    }
}

void EEditApp::HideToolbox(size_t toolbox)
{
    pawsWidget * w = toolboxManager->GetToolboxWidget(toolbox);
    w->SetVisibility(false);
}

void EEditApp::ShowToolbox(size_t toolbox)
{
    pawsWidget * w = toolboxManager->GetToolboxWidget(toolbox);
    w->SetVisibility(true);
}

bool EEditApp::IsToolboxVisible(size_t toolbox)
{
    pawsWidget * w = toolboxManager->GetToolboxWidget(toolbox);
    return w->IsVisible();
}

EEditToolboxManager * EEditApp::GetToolboxManager()
{
    return toolboxManager;
}

psEffectManager * EEditApp::GetEffectManager()
{
    return effectManager;
}

EEditInputboxManager * EEditApp::GetInputboxManager()
{
    return inputboxManager;
}

void EEditApp::SetCamFlag(int flag, bool val)
{
    if (val)
        camFlags.SetBit(flag);
    else
        camFlags.ClearBit(flag);
}

bool EEditApp::GetCamFlag(int flag)
{
    return camFlags.IsBitSet(flag);
}

void EEditApp::TakeScreenshot(const csString & fileName)
{
    csRef<iImageIO> imageio =  csQueryRegistry<iImageIO> (object_reg);
    csRef<iImage> image = g2d->ScreenShot();
    csRef<iDataBuffer> buffer = imageio->Save(image, "image/jpg");
    vfs->WriteFile(fileName.GetData(), buffer->GetData(), buffer->GetSize());

    printf("Screenshot taken\n");
}

/** Application entry point
 */
int main (int argc, char *argv[])
{
    psCSSetup *CSSetup = new psCSSetup(argc, argv, EEditApp::CONFIG_FILENAME, EEditApp::CONFIG_FILENAME);

    csRef<EEditReporter> reporter;
    reporter.AttachNew(new EEditReporter(CSSetup->GetObjectRegistry()));

    iObjectRegistry *object_reg = CSSetup->InitCS(reporter);
    
    editApp = new EEditApp(object_reg, reporter);

    // Initialize application
    if (!editApp->Init())
    {
        editApp->SevereError("Failed to init app!");
        PS_PAUSEEXIT(1);
    }

    // start the main event loop
    csDefaultRunLoop(object_reg);

    // clean up
    delete editApp;
    delete CSSetup;

    csInitializer::DestroyApplication(object_reg);
    return 0;
}
