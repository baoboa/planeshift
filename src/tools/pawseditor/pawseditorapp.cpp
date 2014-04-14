/*
* pawseditorapp.cpp
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*            Michael Cummings <cummings.michael@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation (version 2
* of the License).
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Creation Date: 1/20/2005
* Description : main class for pawseditor
*
*/

#include <psconfig.h>

#include <csutil/xmltiny.h>
#include <iutil/cfgmgr.h>
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
#include <csutil/databuf.h>

#include "util/pscssetup.h"
#include "util/psutil.h"
#include "util/psxmlparser.h"
#include "util/consoleout.h"

#include "paws/pawsmanager.h"
#include "paws/pawstexturemanager.h"
#include "paws/pawsmainwidget.h"
#include "paws/pawsimagedrawable.h"

#include "pewidgettree.h"
#include "pawseditorapp.h"
#include "pawseditorglobals.h"
#include "pemenu.h"
#include "peeditablewidget.h"
#include "pepawsmanager.h"


const char * PawsEditorApp::CONFIG_FILENAME = "/this/pawseditor.cfg";
const char * PawsEditorApp::APP_NAME        = "planeshift.pawseditor.application";
const char * PawsEditorApp::WINDOW_CAPTION  = "Planeshift Paws Editor";
const char * PawsEditorApp::KEY_DEFS_FILENAME = "/this/data/pawseditor/keys_def.xml";

CS_IMPLEMENT_APPLICATION

PawsEditorApp *editApp;

PawsEditorApp::PawsEditorApp(iObjectRegistry *obj_reg)
            :camFlags(CAM_COUNT)
{
    object_reg  = obj_reg;
    paws        = 0;
    drawScreen  = true;

    currentWidget = NULL;
    widgetTree = NULL;
    
    camFlags.Clear();
    camYaw = 0.0f;
    camPitch = 0.0f;

    fileName = "";
}

PawsEditorApp::~PawsEditorApp()
{
    vfs->Unmount( mountedVPath.GetData(), mountedPath.GetData() );
    mountedVPath.Empty();
    mountedPath.Empty();

    if (event_handler && queue)
        queue->RemoveListener(event_handler);

    delete paws;
        
}

void PawsEditorApp::SevereError(const char* msg)
{
    csReport(object_reg, CS_REPORTER_SEVERITY_ERROR, APP_NAME, "%s", msg);
}

bool PawsEditorApp::Init()
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
    // Load the log settings
    LoadLogSettings();

    g3d =  csQueryRegistry<iGraphics3D> (object_reg);
    if (!g3d)
    {
        SevereError("No iGraphics3D plugin!");
        return false;
    }

    g2d = g3d->GetDriver2D();
    if (!g2d)
    {
        SevereError("Could not load iGraphics2D!");
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

    // paws initialization
    paws = new pePawsManager(object_reg, "/this/art/pawseditor.zip");
    if (!paws)
    {
        SevereError("Could not initialize PAWS!");
        return false;
    }

    // Mount base skin to satisfy unskined elements
	csString skinPath = cfgmgr->GetStr("Planeshift.GUI.Skin.Base","/planeshift/art/skins/base/client_base.zip");
	if(!paws->LoadSkinDefinition(skinPath))
        Error2("Couldn't load base skin '%s'!\n",skinPath.GetData());

    mainWidget = new pawsMainWidget();
    paws->SetMainWidget(mainWidget);

    mainWidget->LoadFromFile( "data/pawseditor/pebackground.xml" );
    RegisterFactories();

    // Load and assign a default button click sound for pawsbutton
    //paws->LoadSound("/this/art/music/gui/ccreate/next.wav","sound.standardButtonClick");

    if (!LoadWidgets())
    {
        csReport(object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
            "Warning: Some PAWS widgets failed to load");
    }

    paws->FindWidget( "peMenu" )->SetAlwaysOnTop( true );
    widgetTree = (peWidgetTree *) paws->FindWidget( "peWidgetTree" );
    widgetTree->SetAlwaysOnTop( true );

    skinSelector = (peSkinSelector *) paws->FindWidget( "peSkinSelector" );

    // Register our event handler
    event_handler.AttachNew(new EventHandler (this));
    csEventID esub[] = {
	  csevFrame (object_reg),
	  csevMouseEvent (object_reg),
	  csevKeyboardEvent (object_reg),
	  CS_EVENTLIST_END
	};
    queue->RegisterListener(event_handler, esub);

    // Load Default Skin
    csString skin = cfgmgr->GetStr("PlaneShift.GUI.Skin.Selected","default");
    printf("a  %s\n", skin.GetData());
    if(skin != "")  //should not happen but who knows what an user might do :P
        LoadSkin( skin );

    // Inform debug that everything initialized successfully
    csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, APP_NAME,
        "Application initialized successfully.");

    return true;
}

void PawsEditorApp::LoadSkin( const char* name )
{
    
    csString zip;
    zip += cfgmgr->GetStr("Planeshift.GUI.Skin.Dir","/planeshift/art/skins/");
    zip += name;
    // zip += ".zip";   // Not doing this anymore so that we can use directories unzipped without naming them .zip  :)

    //const char *zip = "/this/art/skins/cvs.zip";
    csString slash(CS_PATH_SEPARATOR);
    if ( vfs->Exists(zip + slash) )
        zip += slash;

    if ( !vfs->Exists( zip ) )
    {
        Error1("Current skin doesn't exist, skipping..\n");
        return;
    }

    //// Make sure the skin is selected
    //if ( skins->GetSelectedRowString() != name )
    //{
    //    skins->Select( name );
    //    return; // To prevent recursion
    //}

    // Get the path
    csRef<iDataBuffer> real = vfs->GetRealPath(zip);
    
    // Unmount the current skin
    if ( mountedVPath.Length() != 0 ) vfs->Unmount( mountedVPath, mountedPath );

    // Temporarily mount zip to read skin.xml
    // Parse XML
    vfs->Mount( "/paws/pawseditor/pawsskin", real->GetData() );
    csRef<iDocument> xml = ParseFile( object_reg, "/paws/pawseditor/pawsskin/skin.xml" );
    if (!xml)
    {
        Error1("Failed to parse file /paws/pawseditor/pawsskin/skin.xml" );
        return;
    }
    vfs->Unmount( "/paws/pawseditor/pawsskin", real->GetData() );

    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error1("No XML root in /paws/pawseditor/pawsskin/skin.xml");
        return;
    }
    root = root->GetNode("xml");
    if(!root)
    {
        Error1("No <xml> tag in /paws/pawseditor/pawsskin/skin.xml");
        return;
    }
    csRef<iDocumentNode> skinInfo = root->GetNode("skin_information");
    if(!skinInfo)
    {
        Error1("No <skin_information> tag in /paws/pawseditor/pawsskin/skin.xml");
        return;
    }

    csRef<iDocumentNode> mountNode = root->GetNode("mount_path");

    if( !mountNode )
    {
        Error1("skin.xml is missing mount_path!\n");
        return;
    }

    // Mount the skin
    mountedVPath = mountNode->GetContentsValue();
    mountedPath = real->GetData();
    if ( !vfs->Mount( mountedVPath.GetData(), mountedPath.GetData() ) )
    {
        printf( "Failed to mount %s in %s to VFS at %s\n", name, mountedPath.GetData(), mountedVPath.GetData() );
        return;
    }
    printf( "Mounted %s in %s to VFS at %s\n", name, mountedPath.GetData(), mountedVPath.GetData() );

    currentSkin = name;

    // Load the skin
    if ( !LoadResources( mountedVPath ) )
    {
        paws->CreateWarningBox("Couldn't load skin! Check the console for detailed output");
        return;
    }
}

bool PawsEditorApp::LoadResources( const char* mountPath )
{
    csString imagelist( mountPath );
    imagelist += "/imagelist.xml";

    // Open the image list
    csRef<iDocument> xml = ParseFile( object_reg, imagelist );
    if(!xml)
    {
        Error2("Parse error in %s", imagelist.GetData());
        return false;
    }
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", imagelist.GetData());
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode( "image_list" );
    if(!topNode)
    {
        Error1("No <image_list> tag in /paws/pawseditor/pawsskin/skin.xml");
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    // Find the resource
    csString filename;
    csString resource;

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;
        
        if ( strcmp( node->GetValue(), "image" ) == 0 )
        {
            // Remove the resource if it exists
            resource = node->GetAttributeValue( "resource" );
            filename = node->GetAttributeValue( "file" );

            paws->GetTextureManager()->Remove( resource );

            if( !filename.Length() )
            {
                printf( "Couldn't locate resource %s in the current skin!\n", resource.GetData() );
                return false;
            }

            // Skin uses /paws/skin which we can't use since the app is already using that
            // So we need to replace that with /skin which we can and do use
            filename.DeleteAt(0,strlen(mountPath));
            filename.Insert(0,"/skin/");

            if( !vfs->Exists( filename ) ) 
            {
                printf( "Skin is missing the '%s' resource at file '%s'!\n", resource.GetData(), filename.GetData() );
                //return false;
                continue;
            }


            // Compile the structure
            if (!strcmp(node->GetValue(), "image"))
            {
                csRef<iPawsImage> drawable;
                drawable.AttachNew(new pawsImageDrawable(node));
                paws->GetTextureManager()->AddPawsImage(drawable);
            }

        }
    }

    return true;
}

void PawsEditorApp::LoadLogSettings()
{
    int count=0;
    for (int i=0; i< MAX_FLAGS; i++)
    {
        if (pslog::GetName(i))
        {
            pslog::SetFlag(pslog::GetName(i),cfgmgr->GetBool(pslog::GetSettingName(i)), 0);  
            if(cfgmgr->GetBool(pslog::GetSettingName(i)))
                count++;
        }
    }
    if(count==0)
    {
        CPrintf(CON_CMDOUTPUT,"All LOGS are off.\n");
    }
    
    csString debugFile =  cfgmgr->GetStr("PlaneShift.DebugFile");
    if ( debugFile.Length() > 0 )
    {
        csRef<iStandardReporterListener> reporter =  csQueryRegistry<iStandardReporterListener > ( object_reg);
        
        
        reporter->SetMessageDestination (CS_REPORTER_SEVERITY_DEBUG, true, false ,false, false, true, false);
        reporter->SetMessageDestination (CS_REPORTER_SEVERITY_ERROR, true, false ,false, false, true, false);       
        reporter->SetMessageDestination (CS_REPORTER_SEVERITY_BUG, true, false ,false, false, true, false);               
        
        time_t curr=time(0);
        tm* localtm = localtime(&curr);

        csString timeStr;
        timeStr.Format("-%d-%02d-%02d-%02d:%02d:%02d",
            localtm->tm_year+1900,
            localtm->tm_mon+1,
            localtm->tm_mday,
            localtm->tm_hour,
            localtm->tm_min,
            localtm->tm_sec);

        debugFile.Append( timeStr );                   
        reporter->SetDebugFile( debugFile, true );
       
        Debug2(LOG_STARTUP,0, "PlaneShift Server Log Opened............. %s", timeStr.GetData());
       
    }
}

bool PawsEditorApp::LoadWidget(const char * file)
{
    if (!paws->LoadWidget(file))
    {
        csString err = "Warning: Loading '" + csString(file) + "' failed!";
        SevereError(err);
        return false;
    }
    return true;
}

bool PawsEditorApp::LoadWidgets()
{
    bool succeeded = true;

    succeeded &= LoadWidget("data/pawseditor/pewidgettree.xml");
    succeeded &= LoadWidget("data/pawseditor/pemenu.xml");
    succeeded &= LoadWidget("data/pawseditor/peskinselector.xml");

    return succeeded;
}

bool PawsEditorApp::HandleEvent(iEvent &ev)
{
    if (ev.Name == csevMouseMove (object_reg, 0))
    {
        csMouseEventData mouse;
        csMouseEventHelper::GetEventData(&ev,mouse);

        mousePointer.x = mouse.x;
        mousePointer.y = mouse.y;
    }

    if (paws->HandleEvent(ev))
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
    {
        drawScreen = false;
    }
    else if (ev.Name == csevCanvasExposed (object_reg, g3d->GetDriver2D ()))
    {
        drawScreen = true;
    }
    return false;
}




csVector2 PawsEditorApp::GetMousePointer()
{
    return mousePointer;
}

iConfigManager* PawsEditorApp::GetConfigManager()
{
    return cfgmgr;
}

iVFS* PawsEditorApp::GetVFS()
{
    return vfs;
}

void PawsEditorApp::Exit()
{
    queue->GetEventOutlet()->Broadcast (csevQuit (object_reg));
}

    
void PawsEditorApp::RegisterFactories()
{
    new peEditableWidgetFactory();
    new peWidgetTreeFactory();
    new peMenuFactory();
    new peSkinSelectorFactory();
}

void PawsEditorApp::Update()
{

}


void PawsEditorApp::SetCamFlag(int flag, bool val)
{
    if (val)
        camFlags.SetBit(flag);
    else
        camFlags.ClearBit(flag);
}

bool PawsEditorApp::GetCamFlag(int flag)
{
    return camFlags.IsBitSet(flag);
}

void PawsEditorApp::TakeScreenshot(const csString & fileName)
{
    csRef<iImageIO> imageio =  csQueryRegistry<iImageIO> (object_reg);
    csRef<iImage> image = g2d->ScreenShot();
    csRef<iDataBuffer> buffer = imageio->Save(image, "image/jpg");
    vfs->WriteFile(fileName.GetData(), buffer->GetData(), buffer->GetSize());

    printf("Screenshot taken.\n");
}

csPtr<iDocumentNode> PawsEditorApp::ParseWidgetFile( const char* widgetFile )
{
    csRef<iDocument> doc = ParseFile(paws->GetObjectRegistry(), widgetFile);  

    if (!doc)
    {
        Error2("Parse error in %s", widgetFile);
        return NULL;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root %s", widgetFile);
        return NULL;
    }
    csRef<iDocumentNode> widgetNode = root->GetNode("widget_description");
    if (!widgetNode)
        Error2("File %s has no <widget_description> tag", widgetFile);
    return csPtr<iDocumentNode>(widgetNode);
}


csPtr<iDocumentNode> PawsEditorApp::ParseWidgetFile_mod( const char* widgetFile )
{
    vfs =  csQueryRegistry<iVFS > (object_reg);
    assert(vfs);
    csRef<iDataBuffer> databuff = vfs->ReadFile(widgetFile);
    if (databuff == NULL)
    {
        Error2("Could not find file: %s", widgetFile);
        return NULL;
    }

    csString buff = databuff->operator*();

    //TODO: Shall we implement a limited pawsMoney, pawsglyphslot, pawsInventoryDollView and
    //      pawsSlot? ATM just falling back to the peEditableWidget
    buff.ReplaceAll("factory=\"pawsMoney\"", "factory=\"peEditableWidget\"");
    buff.ReplaceAll("factory=\"pawsGlyphSlot\"", "factory=\"peEditableWidget\""); 
    buff.ReplaceAll("factory=\"pawsInventoryDollView\"", "factory=\"peEditableWidget\""); 
    buff.ReplaceAll("factory=\"pawsSlot\"", "factory=\"peEditableWidget\""); 

    //This is for debug purposes
    //printf("string =>\n %s\n---------------------\nCUT HERE\n", buff.GetData());
    csRef<iDocument> doc = ParseString(buff);

    if (!doc)
    {
        Error2("Parse error in %s", widgetFile);
        return NULL;
    }
    csRef<iDocumentNode> root = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root %s", widgetFile);
        return NULL;
    }
    csRef<iDocumentNode> widgetNode = root->GetNode("widget_description");
    if (!widgetNode)
        Error2("File %s has no <widget_description> tag", widgetFile);

    csRef<iDocumentNode> widgetintnode = widgetNode->GetNode("widget");

    if(!widgetintnode)
    {
        Error2("File %s has no <widget> tag", widgetFile);
    }
    else
    {
        widgetintnode->SetAttribute("factory", "peEditableWidget");
        widgetintnode->SetAttribute("configurable", "no");
        widgetintnode->SetAttribute("savepositions", "no");
    }
    
    return csPtr<iDocumentNode>(widgetNode);
}

void PawsEditorApp::ReloadWidget()
{
    if ( fileName.Length() != 0 ) OpenWidget( fileName.GetData() );
}

void PawsEditorApp::OpenWidget(const csString & text)
{
    if ( text.Length() == 0 ) return;
    csRef<iDocumentNode> widgetNode = ParseWidgetFile( text );
    if(!widgetNode)
        return;
        
    fileName = text;

    if ( currentWidget )
    {
        currentWidget->Close();
        delete currentWidget;
    }

    csRef<iDocumentNodeIterator> iter = widgetNode->GetNodes();

    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> node = iter->Next();

        if ( node->GetType() != CS_NODE_ELEMENT )
            continue;

        // This is a widget so read it's factory to create it.
        if ( strcmp( node->GetValue(), "widget" ) == 0 )
        {
            //node->RemoveAttribute( node->GetAttribute( "factory" ) );
            //node->SetAttribute( "factory" , "peEditableWidget" );
            csString name = "<missing name>";
            csString factory = "<missing factory>";
            if(node->GetAttribute( "name" ))
                name = node->GetAttribute( "name" )->GetValue();
            if(node->GetAttribute( "factory" ))
                factory = node->GetAttribute( "factory" )->GetValue();
            widgetTree->LoadWidget( widgetNode );
            //as we need to change some things in the xml and the parser doesn't allow us to do it let's change it
            //on the xml document (in ram) and then let's reload it in our parser TODO: MAKE THIS BETTER, this is
            //pratically an hack
            csRef<iDocumentNode> widgetNode = ParseWidgetFile_mod(text);
            csRef<iDocumentNodeIterator> iter = widgetNode->GetNodes();

            while ( iter->HasNext() )
            {
                csRef<iDocumentNode> node = iter->Next();

                if ( node->GetType() != CS_NODE_ELEMENT )
                    continue;

                // This is a widget so read it's factory to create it.
                if ( strcmp( node->GetValue(), "widget" ) == 0 )
                {
                    currentWidget = paws->LoadWidget(node);
                    if ( !currentWidget ) 
                    {
                         csString err = "Warning: Loading '" + csString( text ) + "' failed!";
                          editApp->SevereError( err );
                          return;
                    }
                    paws->GetMainWidget()->FindWidget( "preview" )->AddChild( currentWidget );
                    currentWidget->SetFilename( text );
                    currentWidget->SetMinAlpha( 0 );
                    currentWidget->SetMaxAlpha( 1 );
                    currentWidget->Show();
                    return;
                }
            }
        }
    }
    
}

void PawsEditorApp::OpenImageList(const csString & text)
{    
    paws->GetTextureManager()->LoadImageList( text );
}


/** Application entry point
 */
int main (int argc, char *argv[])
{
    psCSSetup *CSSetup = new psCSSetup(argc, argv, PawsEditorApp::CONFIG_FILENAME, PawsEditorApp::CONFIG_FILENAME);
    iObjectRegistry *object_reg = CSSetup->InitCS();
    
    editApp = new PawsEditorApp(object_reg);

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

/* Some notes - These files still have problems loading in the pawseditor:
 * configcamera.xml type D
 * configmouse.xml type D
 *
 * gmaddeditaction.xml type C
 * gmguiwindow.xml type C
 * money.xml
 * newoptions.xml => mathscripts?
 * questnotebook.xml type C
 * shortcutwindow.xml type C
 * 
 * eedit.xml strange effect --ignored
 * 
 * These were fixed but by changing some of their factories in runtime as the main paws library is missing thing needed
 * by them (they are ps client only paws and integrated in the main ps client)
 * 
 * exchange.xml -> fixedA
 * glyph.xml -> fixedA
 * inventory.xml -> fixedA
 * merchant.xml -> fixedA
 * skillwindow.h -> fixedA
 * smallinventory.h -> fixedA
 * 
 * nothing => assert
 * type C=>  alpha?
 * type D=> tree nodes
 * fixed A=> fixed trough a substitution as a paws factory isn't part of the main paws library
 */
