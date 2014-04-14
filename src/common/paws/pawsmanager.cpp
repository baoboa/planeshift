/*
 * pawsmanger.cpp - Author: Andrew Craig
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

// PawsManager.cpp: implementation of the PawsManager class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>

////////////////////////////////////////////////////////////////////////////
//  CS INCLUDES
////////////////////////////////////////////////////////////////////////////
#include <ivideo/graph2d.h>
#include <ivideo/graph3d.h>

#include <iutil/event.h>
#include <iutil/databuff.h>
#include <iutil/cfgmgr.h>
#include <ivideo/txtmgr.h>
#include <csutil/event.h>
#include <csutil/eventnames.h>
#include <csutil/objreg.h>
#include <csutil/syspath.h>
#include <csutil/xmltiny.h>
#include <iutil/plugin.h>

#include <isoundmngr.h>

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
#include <ivaria/xwindow.h>
#include <X11/Xlib.h>
#endif

////////////////////////////////////////////////////////////////////////////
// COMMON INCLUDES
////////////////////////////////////////////////////////////////////////////
#include "util/log.h"
#include "util/localization.h"
#include "util/psxmlparser.h"

////////////////////////////////////////////////////////////////////////////
// PAWS INCLUDES
////////////////////////////////////////////////////////////////////////////
#include "pawsmanager.h"
#include "pawswidget.h"
#include "pawsbutton.h"
#include "pawscrollbar.h"
#include "pawsmainwidget.h"
#include "pawstextbox.h"
#include "pawskeyselectbox.h"
#include "pawstexturemanager.h"
#include "pawsmouse.h"
//#include "pawsinfowindow.h"
#include "pawsprefmanager.h"
#include "pawsobjectview.h"
#include "pawsgenericview.h"
#include "pawstree.h"
#include "pawsprogressbar.h"
#include "pawsmenu.h"
#include "pawsfilenavigation.h"


//////////////////////////////////////////////////////////////////////
// PAWS WIDGET FACTORIES
//////////////////////////////////////////////////////////////////////
#include "pawslistbox.h"
#include "pawsradio.h"
#include "pawsokbox.h"
#include "pawscombo.h"
#include "pawsselector.h"
#include "pawsyesnobox.h"
#include "pawscheckbox.h"
#include "pawstabwindow.h"
#include "pawsspinbox.h"
#include "pawssimplewindow.h"
#include "pawscombopromptwindow.h"
#include "pawscolorpromptwindow.h"

#define TEMP_SKIN_MOUNT "/paws/temp_skin"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
PawsManager::PawsManager(iObjectRegistry* object, const char* skin, const char* skinBase) :
    ToolTipEnable(true), ToolTipEnableBgColor(false), render2texture(false)
{
    objectReg = object;

    graphics2D = csQueryRegistry<iGraphics2D > (objectReg);
    graphics3D = csQueryRegistry<iGraphics3D > (objectReg);

    nameRegistry = csEventNameRegistry::GetRegistry(objectReg);

    // Initialize event shortcuts
    MouseMove = csevMouseMove(nameRegistry, 0);
    MouseDown = csevMouseDown(nameRegistry, 0);
    MouseDoubleClick = csevMouseDoubleClick(nameRegistry, 0);
    MouseUp = csevMouseUp(nameRegistry, 0);
    KeyboardDown = csevKeyboardDown(objectReg);
    KeyboardUp = csevKeyboardUp(objectReg);

    vfs = csQueryRegistry<iVFS> (objectReg);
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iConfigManager> cfg = csQueryRegistry<iConfigManager> (objectReg);

    // Getting sound manager
    soundManager = csQueryRegistryOrLoad<iSoundManager>(objectReg, "iSoundManager");
    if(!soundManager)
    {
        // if the main sound manager is not found load the dummy plugin
        soundManager = csQueryRegistryOrLoad<iSoundManager>(objectReg, "crystalspace.planeshift.sound.dummy");
        if(!soundManager)
        {
            Error1("Could not find iSoundManager!");
        }
    }

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    csRef<iPluginManager> plugin_mgr = csQueryRegistry<iPluginManager> (objectReg);
    xwin = csQueryPluginClass<iXWindow>(plugin_mgr, "crystalspace.window.x");
    if(!xwin)
    {
        printf("Didn't find iXWindow\n");
    }

    SelectionNotifyEvent = csevSelectionNotify(objectReg);
#endif

    float screenWidth = cfg->GetFloat("Video.ScreenWidth", 800.0f);
    float screenHeight = cfg->GetFloat("Video.ScreenHeight", 600.0f);
    fontFactor = min((screenWidth  / 800.0f), (screenHeight /  600.0f));

    Debug1(LOG_PAWS,0,"Registering Factories.");
    RegisterFactories();

    mainWidget = new pawsMainWidget();

    textureManager = new pawsTextureManager(objectReg);

    Debug1(LOG_PAWS,0,"Loading Pref Manager.");
    prefs = new pawsPrefManager();

    csString prefsFile = cfg->GetStr("PlaneShift.GUI.PrefsFile", "/this/data/prefs.xml");
    csString borderFile = cfg->GetStr("PlaneShift.GUI.BorderFile", "/this/data/gui/borderlist.xml");

    styles = new pawsStyles(objectReg);

    Debug1(LOG_PAWS,0,"Loading Skins Definitions.");
    if(!LoadSkinDefinition(skin))
    {
        Error2("Failed to load skin %s!", skin);
        exit(1);
    }

    // now load standard styles for anything else
    if(!styles->LoadStyles("/this/data/gui/styles.xml"))
    {
        Error1("Failed to load PAWS styles, all style application attempts will be ignored");
        delete styles;
        styles = NULL;
    }

    // Mount base skin to satisfy unskinned elements
    if(skinBase)
    {
        Debug1(LOG_PAWS,0,"Mount base skin.");
        if(!LoadSkinDefinition(skinBase))
        {
            Error2("Couldn't load base skin '%s'!", skinBase);
        }
    }

    if(!prefs->LoadPrefFile(prefsFile))
        Error2("Failed to load prefsFile '%s'", prefsFile.GetData());
    if(!prefs->LoadBorderFile(borderFile))
        Error2("Failed to load borderFile '%s'", borderFile.GetData());

    mouse = new pawsMouse();
    mouse->ChangeImage("Standard Mouse Pointer");

    resizeImg = textureManager->GetPawsImage("ResizeBox");

    graphics2D->SetMouseCursor(csmcNone);

    currentFocusedWidget = mainWidget;
    movingWidget         = 0;
    focusOverridesControls = false;

    resizingWidget       = 0;
    resizingFlags        = 0;

    modalWidget = 0;
    mouseoverWidget = 0;
    lastfadeWidget = 0;

    assert(cfg);
    csString lang = cfg->GetStr("PlaneShift.GUI.Language", "");
    localization = new psLocalization();
    localization->Initialize(objectReg);
    localization->SetLanguage(lang);

    tipDelay = cfg->GetInt("PlaneShift.GUI.ToolTipDelay", 250);

    hadKeyDown = false;
    dragDropWidget = NULL;

    timeOver = 0;

    // Init render texture.
    csRef<iTextureManager> texman = graphics3D->GetTextureManager();
    guiTexture = texman->CreateTexture(graphics3D->GetWidth(), graphics3D->GetHeight(),
                                       csimg2D, "rgba8", 0x9);
    if(guiTexture.IsValid())
        guiTexture->SetAlphaType(csAlphaMode::alphaBinary);
    else
        render2texture = false;

    // Load tooltip settings from file
    // Check if custom tooltips.xml was created. If not load defaults from skin.zip or data/options.
    Debug1(LOG_PAWS,0,"Loading Tooltips.");
    if(vfs->Exists(CONFIG_TOOLTIPS_FILE_NAME))
        LoadTooltips(CONFIG_TOOLTIPS_FILE_NAME);          // custom file
    else if(vfs->Exists(GetLocalization()->FindLocalizedFile("tooltips.xml")))
        LoadTooltips(GetLocalization()->FindLocalizedFile("tooltips.xml"));     // skin.zip
    else
        LoadTooltips(CONFIG_TOOLTIPS_FILE_NAME_DEF);      // data/options
}

PawsManager::~PawsManager()
{
    delete mainWidget;
    delete textureManager;
    delete prefs;
    delete localization;
    delete mouse;
    delete styles;

    if(dragDropWidget != NULL)
        delete dragDropWidget;

    // Free all subscriptions
    PAWSSubscriptionsHash::GlobalIterator iter = subscriptions.GetIterator();
    while(iter.HasNext())
    {
        PAWSSubscription* p = iter.Next();
        delete p;
    }
    subscriptions.DeleteAll();


    factories.DeleteAll();
}

void PawsManager::SetMainWidget(pawsMainWidget* widg)
{
    if(mainWidget)
        delete mainWidget;
    mainWidget = widg;
}

pawsWidget* PawsManager::FindWidget(const char* widgetName, bool complain)
{
    return mainWidget->FindWidget(widgetName, complain);
}

bool PawsManager::RemoveWidget(const char* widgetName, bool complain)
{
    pawsWidget* widget = mainWidget->FindWidget(widgetName, complain);
    if(!widget)
        return false;
    mainWidget->DeleteChild(widget);
    return true;
}

void PawsManager::MovingWidget(pawsWidget* widget)
{
    if(modalWidget == NULL)
    {
        movingWidget = widget;
    }
}

void PawsManager::ResizingWidget(pawsWidget* widget, int flags)
{
    if(modalWidget == NULL)
    {
        resizingWidget = widget;
        resizingFlags  = flags;
    }
}

bool PawsManager::HandleEvent(iEvent &event)
{
    csRef<iEventNameRegistry> namereg = csEventNameRegistry::GetRegistry(
                                            GetObjectRegistry());
    if(CS_IS_KEYBOARD_EVENT(namereg, event))
    {
        if(event.Name == KeyboardDown || event.Name == KeyboardUp)
        {
            return HandleKeyDown(event);
        }
    }
    else if(CS_IS_MOUSE_EVENT(namereg, event))
    {
        csMouseEventType type = csMouseEventHelper::GetEventType(&event);
        csMouseEventData data;
        csMouseEventHelper::GetEventData(&event, data);
        //csKeyModifiers key_modifiers;
        //csKeyEventHelper::GetModifiers (&event, key_modifiers);
        //uint32 modifiers = csMouseEventHelper::GetModifiers (&event);

        if(type == csMouseEventTypeMove)
        {
            return HandleMouseMove(data);
        }
        else if(type == csMouseEventTypeUp)
        {
            return HandleMouseUp(data);
        }
        else if(type == csMouseEventTypeDown)
        {
            return HandleMouseDown(data);
        }
        /*
        else if (type == csMouseEventTypeClick)
        {
        }
        */
        else if(type == csMouseEventTypeDoubleClick)
        {
            return HandleDoubleClick(data);
        }
    }

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
    if(event.Name == SelectionNotifyEvent)
    {
        return HandleSelectionNotify(event);
    }
#endif

    return false;
}


bool PawsManager::HandleDoubleClick(csMouseEventData &data)
{
    pawsWidget* widget = NULL;

    //we need to handle this specially else modality is broken as the
    //mainwidget contains all the other widgets.
    if(modalWidget != mainWidget)
        widget = mainWidget->WidgetAt(data.x, data.y);
    else
        widget = mainWidget;

    if(widget != NULL)
    {
        if(modalWidget != NULL)
        {
            // Check to see if the widget is a child of the modal widget.
            // These are the only components allowed access durning modal
            // mode
            pawsWidget* check = modalWidget->FindWidget(widget->GetName(), false);
            if(check != widget)
            {
                widget = modalWidget->WidgetAt(data.x, data.y);
            }
        }
        if(widget != NULL)
        {
            bool returnResult;

            SetCurrentFocusedWidget(widget);
            if(widget != mainWidget)
            {
                widget->GetParent()->BringToTop(widget);
            }
            returnResult = widget->OnDoubleClick(data.Button, data.Modifiers, data.x, data.y);

            return returnResult;
        }
        return false;
    }
    return false;
}



bool PawsManager::LoadSkinDefinition(const char* zip)
{
    //Mount the skin
    printf("Mounting skin: %s\n", zip);
    csRef<iDataBuffer> realPath = vfs->GetRealPath(zip);

    if(!realPath.IsValid())
    {
        Error2("Could not mount skin %s.  Bad virtual path.",zip);
        return false;

    }

    if(!vfs->Mount(TEMP_SKIN_MOUNT, realPath->GetData()))
    {
        Error2("Could not mount skin %s!",zip);
        return false;
    }

    csRef<iDocument> xml = ParseFile(objectReg,TEMP_SKIN_MOUNT "/skin.xml");
    if(!xml)
    {
        Error2("Could not read skin.xml on %s!",zip);
        return false;
    }

    // Get the mount path
    csRef<iDocumentNode> root = xml->GetRoot();
    if(!root)
    {
        Error2("skin.xml badly formed on %s!",zip);
        return false;
    }
    root = root->GetNode("xml");
    if(!root)
    {
        Error2("Could not find xml node on %s!",zip);
        return false;
    }
    csRef<iDocumentNode> mount = root->GetNode("mount_path");

    if(!mount)
    {
        Error1("There was no mount_path found in the skin.xml file.  Please check the <xml> node to make sure mount_path is defined");
        return false;
    }

    if(vfsPathToSkin.Length() == 0)
    {
        vfsPathToSkin = mount->GetContentsValue();
        vfsPathToSkin.ReplaceAll("\\","/");
        vfsPathToSkin.Append("/");
    }
    csString mountPath = mount->GetContentsValue();

    // Unmount the temp location and remount real
    vfs->Unmount(TEMP_SKIN_MOUNT,realPath->GetData());

    // See if there is another skin with that mount path
    csRef<iStringArray> mounts = vfs->GetMounts();
    for(size_t i = 0; i < mounts->GetSize(); i++)
    {
        csString mount = mounts->Get(i);
        if(mount == mountPath || mount == mountPath + "/")
        {
            Error3("The mount place (%s) for skin '%s' is already taken!",mountPath.GetData(),zip);
            return false;
        }
    }

    // mount
    vfs->Mount(mountPath,realPath->GetData());

    // Load additional styles
    csString stylesFile = mountPath + "/styles.xml";
    if(vfs->Exists(stylesFile) && !styles->LoadStyles(stylesFile))
    {
        Error2("Couldn't load styles file: %s", stylesFile.GetData());
        return false;
    }

    // Load additional resources
    csString imageFile = mountPath + "/imagelist.xml";
    textureManager->LoadImageList(imageFile);

    return true;
}

bool PawsManager::HandleKeyDown(iEvent &event)
{
    if(currentFocusedWidget)
    {
        if((csKeyEventHelper::GetEventType(&event) == csKeyEventTypeUp))
        {
            if(hadKeyDown)
                return true;
        }


        utf32_char raw = csKeyEventHelper::GetRawCode(&event);
        utf32_char cooked = csKeyEventHelper::GetCookedCode(&event);
        uint32 modifiers = csKeyEventHelper::GetModifiersBits(&event);

        //we don't handle capslock/numlock/scorr block (see PS#4806)
        modifiers = modifiers &~ CSMASK_ALLLOCKS;

        bool result = currentFocusedWidget->OnKeyDown(raw,
                      cooked,
                      modifiers);
        hadKeyDown = result;
        return result;
    }

    return false;
}

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
bool PawsManager::HandleSelectionNotify(iEvent &/*ev*/)
{
    int result = false;

    if(currentFocusedWidget)
    {
        Display* dpy = xwin->GetDisplay();
        if(dpy == NULL)
        {
            Error1("Failed to find display");
            return false;
        }

        Window w = xwin->GetWindow();
        if(w == 0)
        {
            Error1("Failed to find window");
            return false;
        }

        XEvent event = xwin->GetStoredEvent();

        if(event.xselection.property == None)
        {
            Error1("Asked format was denied");
            return false;
        }
        else
        {
            uint8* clipData;
            Atom actualType;
            int  actualFormat;
            unsigned long nitems, bytesLeft;
            if((result = XGetWindowProperty(dpy, w, event.xselection.property,
                                            0L /* offset */, 1000000 /* length (max) */, false,
                                            AnyPropertyType /* format */,
                                            &actualType, &actualFormat, &nitems, &bytesLeft,
                                            &clipData)) == Success)
            {
                Debug6(LOG_PAWS, 0, "Clipboard data: type: %i len: %lu format: %i byte_left: %lu %s",
                       (int)actualType, nitems, actualFormat, bytesLeft, clipData);
                /*
                  if (actualType == atom_UTF8_STRING && actualFormat == 8)
                  {
                  returnData = String::fromUTF8 (clipData, nitems);
                  }
                  else if (actualType == XA_STRING && actualFormat == 8)
                  {
                  returnData = String((const char*)clipData, nitems);
                  }
                */

                csString content((const char*)clipData);

                if(clipData != 0)
                    XFree(clipData);

                XDeleteProperty(dpy, w, event.xselection.property);

                return currentFocusedWidget->OnClipboard(content);
            }
            else
            {
                XDeleteProperty(dpy, w, event.xselection.property);

                if(result == BadAtom)
                {
                    printf("Check data: Bad Atom\n");
                }
                if(result == BadWindow)
                {
                    printf("Check data: Bad Window\n");
                }
                return false;
            }
        }
    }

    return false;
}
#endif

bool PawsManager::HandleMouseDown(csMouseEventData &data)
{
    pawsWidget* widget = 0;

    //we need to handle this specially else modality is broken as the
    //mainwidget contains all the other widgets.
    if(modalWidget != mainWidget)
        widget = mainWidget->WidgetAt(data.x, data.y);
    else
        widget = mainWidget;

    if(widget != NULL)
    {
        // Enforce modality by only allowing button clicks within the modal widget if there is one.
        if(modalWidget != NULL)
        {
            // Check to see if the widget is a child of the modal widget.
            // These are the only components allowed access durring modal
            // mode
            pawsWidget* check = modalWidget->FindWidget(widget->GetName(), false);
            if(check != widget)
            {
                widget = modalWidget->WidgetAt(data.x, data.y);
            }
        }

        if(widget != NULL)
        {
            // Handle focus and ordering

            SetCurrentFocusedWidget(widget);

            if(widget != mainWidget)
            {
                if(widget->GetParent())
                {
                    widget->GetParent()->BringToTop(widget);
                }
            }

            // Distribute the event to the widget

            uint32 modifiers = data.Modifiers;

            //we don't handle capslock/numlock/scorr block (see PS#4806)
            modifiers = modifiers &~ CSMASK_ALLLOCKS;

            bool returnResult = widget->OnMouseDown(data.Button,
                                                    modifiers,
                                                    data.x,
                                                    data.y);

            widget->RunScriptEvent(PW_SCRIPT_EVENT_MOUSEDOWN);

            return returnResult;
        }

        return false;
    }

    return false;
}

bool PawsManager::HandleMouseUp(csMouseEventData &data)
{
    // Check to see if we are moving a widget.
    if(movingWidget)
    {
        movingWidget = NULL;
        return true;
    }

    if(resizingWidget)
    {
        resizingWidget->StopResize();
        resizingWidget = NULL;
        return true;
    }

    if(currentFocusedWidget)
    {
        return currentFocusedWidget->OnMouseUp(data.Button,
                                               data.Modifiers,
                                               data.x,
                                               data.y);
    }

    pawsWidget* widget = mainWidget->WidgetAt(data.x, data.y);

    SetCurrentFocusedWidget(widget);

    if(widget)
    {
        if(widget != mainWidget)
        {
            widget->GetParent()->BringToTop(widget);
        }

        bool returnResult = widget->OnMouseUp(data.Button,
                                              data.Modifiers,
                                              data.x,
                                              data.y);

        widget->RunScriptEvent(PW_SCRIPT_EVENT_MOUSEUP);

        return returnResult;
    }

    return false;
}

/*
psPoint PawsManager::MouseLocation( iEvent &ev )
{
    csMouseEventData event;

    const void *_ax = 0; size_t _ax_sz = 0;
    csEventError ok = csEventErrNone;
    ok = ev.Retrieve("mAxes", _ax, _ax_sz);
    CS_ASSERT(ok == csEventErrNone);
    ok = ev.Retrieve("mNumAxes", event.numAxes);
    CS_ASSERT(ok == csEventErrNone);
    for (int iter=0 ; iter<CS_MAX_MOUSE_AXES ; iter++)
    {
        if (iter<(int)event.numAxes)
            event.axes[iter] = ((int *)_ax)[iter];
        else
            event.axes[iter] = 0;
    }

    psPoint point;
    point.x = event.axes[0];
    point.y = event.axes[1];
    return point;
}
*/



bool PawsManager::HandleMouseMove(csMouseEventData &data)
{
    mouse->SetPosition(data.x, data.y);

    timeOver = csGetTicks();

    if(movingWidget)
    {
        movingWidget->MoveDelta(mouse->GetDeltas().x, mouse->GetDeltas().y);
        return true;
    }

    if(resizingWidget)
    {
        resizingWidget->Resize(resizingFlags);
        return true;
    }

    // Handle mouseoverWidget and start fading due to loss of mouse focus
    pawsWidget* widget = mainWidget->WidgetAt(data.x, data.y);
    if(widget)
    {
        if((modalWidget == NULL) || widget->IsChildOf(modalWidget))
        {
            // Handle mouse over for
            if(widget && widget != mouseoverWidget)
            {
                if(mouseoverWidget)
                    mouseoverWidget->OnMouseExit();
                widget->OnMouseEnter();
            }

            mouseoverWidget = widget;

            // Now only handle fading if not modal windows are involved
            if(modalWidget == NULL)
            {
                pawsWidget* widgetFade = widget;

                // Only fade in/out topmost parent
                if(widgetFade != mainWidget)
                {
                    for(pawsWidget* mainParent = widgetFade->GetParent(); mainParent != mainWidget; mainParent=widgetFade->GetParent())
                    {
                        widgetFade = mainParent;
                    }
                }


                if(lastfadeWidget != widgetFade && widgetFade && widgetFade->IsVisible())
                {
                    if(lastfadeWidget && lastfadeWidget != mainWidget)
                    {
                        lastfadeWidget->MouseOver(false);
                    }
                    lastfadeWidget = widgetFade;

                    if(widgetFade != mainWidget)
                        widgetFade->MouseOver(true);                // Fade in
                }
            }
        }
        else
        {
            mouseoverWidget = NULL;
        }
    }
    else
    {
        mouseoverWidget = NULL;
    }

    return false;
}

void PawsManager::Draw()
{
    // First draw the main gui.
    //render2texture = true;
    if(!render2texture || true/* TODO: mainWidget->NeedsRender()*/)
    {
        if(render2texture)
        {
            graphics3D->SetRenderTarget(guiTexture);
            graphics3D->BeginDraw(CSDRAW_2DGRAPHICS);
        }

        mainWidget->DrawChildren();

        if(render2texture)
        {
            graphics3D->FinishDraw();
            graphics3D->Print(0);
        }
    }

    if(render2texture)
    {
        graphics3D->SetRenderTarget(0);
        graphics3D->BeginDraw(CSDRAW_2DGRAPHICS);
        graphics3D->DrawPixmap(guiTexture, 0, 0, graphics3D->GetWidth(), graphics3D->GetHeight(),
                               0, 0, graphics3D->GetWidth(), graphics3D->GetHeight());
    }

    // Now everything else.
    if(modalWidget != NULL) modalWidget->Draw();

    graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());

    // Draw the tooltip above all other windows after 250 csTicks
    if((timeOver < (csGetTicks() - tipDelay)) && mouseoverWidget)
    {
        mouseoverWidget->DrawToolTip(mouse->GetPosition().x, mouse->GetPosition().y);
    }

    mouse->Draw();
}

void PawsManager::Draw3D()
{
    if(objectViews.IsEmpty())
        return; // can't render object views

    for(size_t i=0; i<objectViews.GetSize(); ++i)
    {
        if(objectViews[i]->IsVisible())
        {
            objectViews[i]->Draw3D(graphics3D);
        }
    }
}

void PawsManager::RegisterWidgetFactory(pawsWidgetFactory* fact)
{
    Debug2(LOG_PAWS,0,"Registering Factory %s.",fact->GetName());

    factories.Push(fact);
}

csPtr<iDocumentNode> PawsManager::ParseWidgetFile(const char* widgetFile)
{
    csRef<iDocument> doc;
    const char* error;

    Debug2(LOG_PAWS,0,"Parsing Widget file %s.",widgetFile);
    csRef<iDataBuffer> buff = vfs->ReadFile(widgetFile);
    if(buff == NULL)
    {
        Error2("Could not find file: %s", widgetFile);
        return NULL;
    }
    doc=xml->CreateDocument();
    error=doc->Parse(buff);
    if(error)
    {
        Error3("Parse error in %s: %s", widgetFile, error);
        return NULL;
    }

    csRef<iDocumentNode> root;
    if((root=doc->GetRoot()))
    {
        csRef<iDocumentNode> widgetNode = root->GetNode("widget_description");
        if(!widgetNode)
            Error2("File %s has no <widget_description> tag", widgetFile);
        return csPtr<iDocumentNode>(widgetNode);
    }
    else
    {
        Error2("Bad XML document in %s", widgetFile);
        return NULL;
    }

}

pawsWidget* PawsManager::LoadWidgetFromString(const char* widgetDefinition)
{
    csRef<iDocument> doc;
    const char* error;

    doc=xml->CreateDocument();
    error=doc->Parse(widgetDefinition);
    if(error)
    {
        Error3("Parse error in %s: %s", widgetDefinition, error);
        return NULL;
    }
    csRef<iDocumentNode> root,node;
    root = doc->GetRoot();
    node = root->GetNode("widget");
    pawsWidget* widget = LoadWidget(node);
    if(widget)
        widget->PostSetup();
    return widget;
}

bool PawsManager::LoadWidget(const char* widgetFile)
{
    // check for file in skin zip, then localized, then /data/gui, then /this
    csString fullPath;
    fullPath = vfsPathToSkin + widgetFile;
    if(!localization->FileExists(fullPath))
    {
        fullPath = localization->FindLocalizedFile(widgetFile);
    }
    csRef<iDocumentNode> topNode = ParseWidgetFile(fullPath);
    if(!topNode)
    {
        Error2("Error parsing xml in file: %s", fullPath.GetDataSafe());
        return false;
    }

    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a widget so read it's factory to create it.
        pawsWidget* widget = LoadWidget(node);
        if(widget)
        {
            widget->SetFilename(widgetFile);
            mainWidget->AddChild(widget);
        }
        else
        {
            Error2("Could not load child widget in file %s",fullPath.GetDataSafe());
            return false;
        }
    }
    return true;
}


bool PawsManager::LoadChildWidgets(const char* widgetFile, csArray<pawsWidget*> &loadedWidgets)
{
    bool errors=false;
    loadedWidgets.DeleteAll();
    csString fullPath = localization->FindLocalizedFile(widgetFile);

    csRef<iDocumentNode> topNode = ParseWidgetFile(fullPath);
    if(!topNode) return false;
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        // This is a widget so read it's factory to create it.
        pawsWidget* widget = LoadWidget(node);
        if(widget)
            loadedWidgets.Push(widget);
        else
            errors = true;
    }
    return (!errors);
}

pawsWidget* PawsManager::LoadWidget(iDocumentNode* widgetNode)
{
    pawsWidget* widget;
    csString factory;

    if(strcmp(widgetNode->GetValue(), "widget") == 0)        // old syntax is <widget factory="bla"....>
        factory = widgetNode->GetAttributeValue("factory");
    else
        factory = widgetNode->GetValue();   // new syntax is using factory name as tag name directly: <pawsChatWindow ....>

    if(factory.Length() == 0)
    {
        Error1("Could not read factory from XML node. Error in XML");
        return NULL;
    }

    widget = CreateWidget(factory);
    if(!widget)
    {
        Error2("Failed to create widget %s", factory.GetData());
        return NULL;
    }

    if(!widget->Load(widgetNode))
    {
        Error3("Widget %s failed to load %s", widget->GetName(), factory.GetData());
        delete widget;
        return NULL;
    }

    return widget;
}

bool PawsManager::LoadObjectViews()
{
    bool ret = true;
    for(size_t i=0; i<objectViews.GetSize(); ++i)
        ret &= ((pawsObjectView*)objectViews[i])->ContinueLoad();
    return ret;
}

pawsWidget* PawsManager::CreateWidget(const char* factoryName, const pawsWidget* origin)
{
    Debug2(LOG_PAWS,0,"Creating Widget %s from origin",factoryName);
    csString factory(factoryName);
    for(size_t x = 0; x < factories.GetSize(); x++)
    {
        if(factory == factories[x]->GetName())
        {
            pawsWidget* newWidget = factories[x]->Create(origin);
            newWidget->SetFactory(factoryName);
            return newWidget;
        }
    }

    Error2("Could not locate Factory: %s,using default copy constructor to create", factoryName);
    return new pawsWidget(*origin);
}

pawsWidget* PawsManager::CreateWidget(const char* factoryName)
{
    Debug2(LOG_PAWS,0,"Creating Widget %s",factoryName);
    csString factory(factoryName);
    for(size_t x = 0; x < factories.GetSize(); x++)
    {
        if(factory == factories[x]->GetName())
        {
            pawsWidget* newWidget = factories[x]->Create();
            newWidget->SetFactory(factoryName);
            return newWidget;
        }
    }

    Error2("Could not locate Factory: %s", factoryName);
    return NULL;
}

void PawsManager::SetModalWidget(pawsWidget* widget)
{
    if(widget != NULL)
        SetCurrentFocusedWidget(widget);

    modalWidget = widget;
}

void PawsManager::SetCurrentFocusedWidget(pawsWidget* widget)
{
    if(widget == NULL)
    {
        widget = mainWidget;
    }
    if(currentFocusedWidget == widget)
        return;

    // Allow widget to be focused if it is a child of the modal widget, or if there is no modal widget.
    if(!modalWidget || modalWidget->FindWidget(widget->GetName(), false))
    {
        // Check if widget CAN be focused, if it can't then pass it to the parent
        while(currentFocusedWidget != widget && widget != mainWidget && !widget->OnGainFocus())
        {
            widget = widget->GetParent();
        }
    }

    if(currentFocusedWidget)
    {
        currentFocusedWidget->OnLostFocus();
    }
    currentFocusedWidget = widget;
    currentFocusedWidget->OnGainFocus();

    // Check if the focused widget or any of it's parents grab the keyboard input
    for(focusOverridesControls = false; widget != NULL && !focusOverridesControls; widget = widget->GetParent())
    {
        focusOverridesControls = widget->GetFocusOverridesControls();
    }
}


void PawsManager::OnWidgetDeleted(pawsWidget* widget)
{
    if(currentFocusedWidget == widget)
        currentFocusedWidget = NULL;
    if(mouseoverWidget == widget)
        mouseoverWidget = NULL;
    if(movingWidget == widget)
        movingWidget = NULL;
    if(modalWidget == widget)
        modalWidget = NULL;
    if(resizingWidget == widget)
        resizingWidget = NULL;
    if(lastfadeWidget == widget)
        lastfadeWidget = NULL;
}

void PawsManager::OnWidgetHidden(pawsWidget* widget)
{
    // if the focused widget will be hidden, removed focus from it
    if(widget->Includes(currentFocusedWidget))
        currentFocusedWidget = NULL;

    if(widget->Includes(mouseoverWidget))
        mouseoverWidget = NULL;
}

csString PawsManager::Translate(const csString &orig)
{
    return localization->Translate(orig);
}

void PawsManager::CreateWarningBox(const char* message, pawsWidget* notify, bool modal)
{
    pawsOkBox* ok = (pawsOkBox*)FindWidget("OkWindow");
    if(!ok)
    {
        LoadWidget("ok.xml");
        ok = (pawsOkBox*)FindWidget("OkWindow");
        if(!ok)
        {
            Error1("Cannot load the ok window!");
            return;
        }
    }

    ok->Show();
    ok->SetText(message);
    ok->MoveTo((graphics2D->GetWidth() - ok->GetActualWidth(512)) / 2,
               (graphics2D->GetHeight() - ok->GetActualHeight(256))/2);

    if(notify)
        ok->SetNotify(notify);

    if(modal)
        SetModalWidget(ok);
}


void PawsManager::CreateYesNoBox(const char* message, pawsWidget* notify, bool modal, bool translate)
{
    pawsYesNoBox* yesNoBox = (pawsYesNoBox*)FindWidget("YesNoWindow");

    if(!yesNoBox)
    {
        LoadWidget("yesno.xml");
        yesNoBox = (pawsYesNoBox*)FindWidget("YesNoWindow");
        if(!yesNoBox)
        {
            Error1("Cannot Load YesNo Window");
            return;
        }
    }

    yesNoBox->MoveTo((graphics2D->GetWidth() - yesNoBox->GetActualWidth(512)) / 2,
                     (graphics2D->GetHeight() - yesNoBox->GetActualHeight(256))/2);
    yesNoBox->SetText(translate ? Translate(message).GetData() : message);
    yesNoBox->Show();

    if(modal)
        SetModalWidget(yesNoBox);

    if(notify)
        yesNoBox->SetNotify(notify);
}


pawsWidget* PawsManager::GetDragDropWidget()
{
    return dragDropWidget;
}

void PawsManager::SetDragDropWidget(pawsWidget* dragDropWidget)
{
    if(PawsManager::dragDropWidget != NULL)
        delete PawsManager::dragDropWidget;
    PawsManager::dragDropWidget = dragDropWidget;
    mouse->UpdateDragPosition();
}

#define RegisterFactory(factoryclass)   \
    factory = new factoryclass(); \
    CS_ASSERT(factory);


void PawsManager::RegisterFactories()
{
    pawsWidgetFactory* factory = NULL;

    RegisterFactory(pawsBaseWidgetFactory);
    RegisterFactory(pawsButtonFactory);
    RegisterFactory(pawsScrollBarFactory);
    RegisterFactory(pawsObjectViewFactory);
    RegisterFactory(pawsGenericViewFactory);
    RegisterFactory(pawsTextBoxFactory);
    RegisterFactory(pawsKeySelectBoxFactory);
    RegisterFactory(pawsMultiLineTextBoxFactory);

    RegisterFactory(pawsMessageTextBoxFactory);
    RegisterFactory(pawsEditTextBoxFactory);
    RegisterFactory(pawsMultilineEditTextBoxFactory);
    RegisterFactory(pawsColorInputFactory);
    RegisterFactory(ComboWrapperFactory);
//    RegisterFactory (pawsInfoWindowFactory);
    RegisterFactory(pawsYesNoBoxFactory);
    RegisterFactory(pawsListBoxFactory);
    RegisterFactory(pawsComboBoxFactory);
    RegisterFactory(pawsRadioButtonGroupFactory);
    RegisterFactory(pawsRadioButtonFactory);
    RegisterFactory(pawsProgressBarFactory);
    RegisterFactory(pawsOkBoxFactory);
    RegisterFactory(pawsSelectorBoxFactory);
    RegisterFactory(pawsSpinBoxFactory);
    RegisterFactory(pawsMultiPageTextBoxFactory);
    /*
        RegisterFactory (pawsSplashWindowFactory);
        RegisterFactory (pawsLoadWindowFactory);
        RegisterFactory (pawsChatWindowFactory);
        RegisterFactory (pawsInventoryWindowFactory);
        RegisterFactory (pawsItemDescriptionWindowFactory);
        RegisterFactory (pawsContainerDescWindowFactory);
        RegisterFactory (pawsInteractWindowFactory);
        RegisterFactory (pawsControlWindowFactory);
        RegisterFactory (pawsGroupWindowFactory);
        RegisterFactory (pawsExchangeWindowFactory);
        RegisterFactory (pawsSpellBookWindowFactory);
        RegisterFactory (pawsGlyphWindowFactory);
        RegisterFactory (pawsMerchantWindowFactory);
    */

    RegisterFactory(pawsTreeFactory);
    RegisterFactory(pawsSimpleTreeFactory);
    RegisterFactory(pawsSimpleTreeNodeFactory);
    RegisterFactory(pawsSeqTreeNodeFactory);
    RegisterFactory(pawsWidgetTreeNodeFactory);
    /*
        RegisterFactory (pawsConfigWindowFactory);
        RegisterFactory (pawsConfigKeysFactory);
        RegisterFactory (pawsFingeringWindowFactory);
        RegisterFactory (pawsConfigDetailsFactory);
        RegisterFactory (pawsConfigMouseFactory);
        RegisterFactory (pawsConfigEntityLabelsFactory);

        RegisterFactory (pawsPetitionWindowFactory);
        RegisterFactory (pawsPetitionGMWindowFactory);
        RegisterFactory (pawsPetitionViewWindowFactory);
        RegisterFactory (pawsScrollMenuFactory);
        RegisterFactory (pawsDnDButtonFactory);
        RegisterFactory (pawsShortcutWindowFactory);
        RegisterFactory (pawsLoginWindowFactory);
        RegisterFactory (pawsCharacterPickerWindowFactory);
        RegisterFactory (pawsGuildWindowFactory);
        RegisterFactory (pawsGuildJoinWindowFactory);
        RegisterFactory (pawsLootWindowFactory);

        RegisterFactory (pawsCreationMainFactory);
        RegisterFactory (pawsCharParentsFactory);
        RegisterFactory (pawsChildhoodWindowFactory);
        RegisterFactory (pawsLifeEventWindowFactory);
        RegisterFactory (pawsPathWindowFactory);
        RegisterFactory (pawsSummaryWindowFactory);
    */
    RegisterFactory(pawsMenuFactory);
    RegisterFactory(pawsMenuItemFactory);
    RegisterFactory(pawsMenuSeparatorFactory);
    /*
        RegisterFactory (pawsSkillWindowFactory);
        RegisterFactory (pawsQuestListWindowFactory);
        RegisterFactory (pawsSpellCancelWindowFactory);
    */
    RegisterFactory(pawsCheckBoxFactory);
    RegisterFactory(pawsTabWindowFactory);
    RegisterFactory(pawsFileNavigationFactory);
    RegisterFactory(pawsFadingTextBoxFactory);

    RegisterFactory(pawsSimpleWindowFactory);
    RegisterFactory(pawsDocumentViewFactory);
    RegisterFactory(pawsMultiPageDocumentViewFactory);
    RegisterFactory(pawsThumbFactory);
}

bool PawsManager::ApplyStyle(const char* name, iDocumentNode* target)
{
    if(styles != NULL)
        styles->ApplyStyle(name, target);
    return true;
}

csString PAWSData::temp_buffer;  // declare static string for returning const char* across stack

const char* PAWSData::GetStr()
{
    if(type == PAWS_DATA_STR || type == PAWS_DATA_INT_STR)
        return str;

    switch(type)
    {
        case PAWS_DATA_BOOL:
            temp_buffer = boolval?"True":"False";
            break;
        case PAWS_DATA_INT:
            temp_buffer.Format("%d",intval);
            break;
        case PAWS_DATA_FLOAT:
            temp_buffer.Format("%.2f",floatval);
            break;
        case PAWS_DATA_UINT:
            temp_buffer.Format("%u",uintval);
            break;
        case PAWS_DATA_STR:
        case PAWS_DATA_INT_STR:
        case PAWS_DATA_UNKNOWN:
            break;
    }
    return temp_buffer;
}

float PAWSData::GetFloat()
{
    if(type == PAWS_DATA_FLOAT)
        return floatval;

    switch(type)
    {
        case PAWS_DATA_BOOL:
            return boolval?1:0;
        case PAWS_DATA_INT:
            return intval;
        case PAWS_DATA_INT_STR:
        case PAWS_DATA_STR:
            return atof(str);
        case PAWS_DATA_UINT:
            return uintval;
        case PAWS_DATA_FLOAT:
        case PAWS_DATA_UNKNOWN:
            break;
    }
    return 0;
}

int   PAWSData::GetInt()
{
    if(type == PAWS_DATA_INT || type == PAWS_DATA_INT_STR)
        return intval;

    switch(type)
    {
        case PAWS_DATA_BOOL:
            return boolval?1:0;
        case PAWS_DATA_FLOAT:
            return (int)floatval;
        case PAWS_DATA_STR:
            return atoi(str);
        case PAWS_DATA_UINT:
            return uintval; //this should never be used (wrap warning!)
        case PAWS_DATA_INT:
        case PAWS_DATA_INT_STR:
        case PAWS_DATA_UNKNOWN:
            break;
    }
    return 0;
}

unsigned int   PAWSData::GetUInt()
{
    if(type == PAWS_DATA_UINT)
        return uintval;

    switch(type)
    {
        case PAWS_DATA_BOOL:
            return boolval?1:0;
        case PAWS_DATA_FLOAT:
            return (unsigned int)floatval;
        case PAWS_DATA_INT_STR:
        case PAWS_DATA_STR:
            return atoi(str);
        case PAWS_DATA_INT:
        case PAWS_DATA_UINT:
        case PAWS_DATA_UNKNOWN:
            break;
    }
    return 0;
}

bool  PAWSData::GetBool()
{
    if(type == PAWS_DATA_BOOL)
        return boolval;

    switch(type)
    {
        case PAWS_DATA_INT:
            return intval?true:false;
        case PAWS_DATA_FLOAT:
            return floatval?true:false;
        case PAWS_DATA_INT_STR:
        case PAWS_DATA_STR:
            if(!str ||
                    !strlen(str) ||
                    !strcasecmp(str,"False") ||
                    !strcasecmp(str,"No"))
                return false;
            else
                return true;
        case PAWS_DATA_UINT:
            return uintval?true:false;
        case PAWS_DATA_BOOL:
        case PAWS_DATA_UNKNOWN:
            break;
    }
    return false;
}


void PawsManager::UnSubscribe(iPAWSSubscriber* listener)
{
    PAWSSubscriptionsHash::GlobalIterator iter = subscriptions.GetIterator();
    while(iter.HasNext())
    {
        PAWSSubscription* p = iter.Next();
        if(p->subscriber == listener)
            p->subscriber = 0;
    }
}

void PawsManager::Subscribe(const char* dataname,iPAWSSubscriber* listener)
{
    Debug3(LOG_PAWS,0,"Subscription to %s, from %p.", dataname, listener);

    // Check for duplicates.
    PAWSSubscriptionsHash::Iterator iter = subscriptions.GetIterator(dataname);
    while(iter.HasNext())
    {
        PAWSSubscription* p = iter.Next();
        if(p->subscriber == listener)
            return;
    }

    listener->NewSubscription(dataname);

    iter.Reset();
    if(!iter.HasNext())   // no one has subscribed yet, so just save subscription
    {
        // and wait for publish
        PAWSSubscription* p = new PAWSSubscription;
        p->subscriber = listener;
        subscriptions.Put(dataname,p);
    }
    else
    {
        PAWSData lastKnownValue;

        // Check for entries without subscriber and store lastKnownValue
        while(iter.HasNext())
        {
            PAWSSubscription* p = iter.Next();
            if(!p->subscriber)   //  data is present already but no subscriber
            {
                p->subscriber = listener;
                if(p->lastKnownValue.IsData())
                    p->subscriber->OnUpdateData(dataname,p->lastKnownValue);
                return;
            }
            lastKnownValue = p->lastKnownValue;
        }

        // Otherwise no blank slots so just subscribe and publish last Known Value
        PAWSSubscription* p = new PAWSSubscription;
        p->subscriber = listener;
        p->lastKnownValue = lastKnownValue; // Make sure new entries have the same value
        // as other entries of same dataname.
        subscriptions.Put(dataname,p);
        if(p->lastKnownValue.IsData())
            p->subscriber->OnUpdateData(dataname,p->lastKnownValue);
    }
}

void PawsManager::Publish(const csString &dataname,PAWSData &data)
{
    Debug2(LOG_PAWS,0,"Publishing to %s.", dataname.GetData());

    PAWSSubscriptionsHash::Iterator iter = subscriptions.GetIterator(dataname);
    if(!iter.HasNext())   // no one has subscribed yet, so just save value
    {
        PAWSSubscription* p = new PAWSSubscription;
        p->subscriber       = NULL;
        p->lastKnownValue   = data;
        subscriptions.Put(dataname,p);
    }

    // Now publish to any subscribers already active
    while(iter.HasNext())
    {
        PAWSSubscription* p = iter.Next();
        p->lastKnownValue = data;
        if(p->subscriber)
            p->subscriber->OnUpdateData(dataname,data);
    }
}

void PawsManager::Publish(const csString &dataname,const char* datavalue)
{
    PAWSData data;
    data.type = PAWS_DATA_STR;
    data.str = datavalue;
    Publish(dataname,data);
}

void PawsManager::Publish(const csString &dataname,bool  datavalue)
{
    PAWSData data;
    data.type = PAWS_DATA_BOOL;
    data.boolval = datavalue;
    Publish(dataname,data);
}

void PawsManager::Publish(const csString &dataname,int   datavalue)
{
    PAWSData data;
    data.type = PAWS_DATA_INT;
    data.intval = datavalue;
    Publish(dataname,data);
}

void PawsManager::Publish(const csString &dataname,unsigned int   datavalue)
{
    PAWSData data;
    data.type = PAWS_DATA_UINT;
    data.uintval = datavalue;
    Publish(dataname,data);
}

void PawsManager::Publish(const csString &dataname,float datavalue)
{
    PAWSData data;
    data.type = PAWS_DATA_FLOAT;
    data.floatval = datavalue;
    Publish(dataname,data);
}

void PawsManager::Publish(const csString &dataname,const char* datavalue, int color)
{
    PAWSData data;
    data.type = PAWS_DATA_INT_STR;
    data.str = datavalue;
    data.intval = color;
    Publish(dataname,data);
}

void PawsManager::Publish(const csString &dataname)
{
    PAWSData data;
    Publish(dataname,data);
}

csArray<iPAWSSubscriber*> PawsManager::ListSubscribers(const char* dataname)
{
    csArray<iPAWSSubscriber*> list;

    PAWSSubscriptionsHash::Iterator iter = subscriptions.GetIterator(dataname);
    while(iter.HasNext())
    {
        PAWSSubscription* p = iter.Next();
        if(p->subscriber)
            list.Push(p->subscriber);
    }

    return list;
}

#if defined(CS_PLATFORM_UNIX) && defined(INCLUDE_CLIPBOARD)
void PawsManager::RequestClipboardContent()
{
    printf("Requesting Clipboard\n");

    if(!xwin)
    {
        printf("Need xwin in order to request selection.\n");
        return;
    }

    Display* dpy = xwin->GetDisplay();
    if(dpy == NULL)
    {
        printf("Failed to find display\n");
        return;
    }

    Window w = xwin->GetWindow();
    if(!w)
    {
        printf("Failed to find window.\n");
        return;
    }

    //for some reason unknown only to mortals, XA_CLIPBOARD is not an internal atom defined in Xatom.h
    Atom XA_CLIPBOARD = XInternAtom(dpy, "CLIPBOARD", 0);

    // Check if there is an owner of primary selection
    Atom selection = XA_PRIMARY;
    Window selection_owner = None;
    if((selection_owner = XGetSelectionOwner(dpy, selection)) == None)
    {
        printf("No Primary selection, trying clipboard\n");
        selection = XA_CLIPBOARD;
        selection_owner = XGetSelectionOwner(dpy, selection);
    }


    if(selection_owner != None)
    {
        if(selection_owner == w)
        {
            // Copy from internal clipboard
            // content = localClipboardContent;
            // call OnClipboard( content ) for focus widget.
        }
        else
        {
            printf("Found selection owner other than self\n");

            Atom XA_UTF8_STRING = XInternAtom(dpy, "UTF8_STRING", False);

            //bool ok = juce_x11_requestSelectionContent(content, selection, atom_UTF8_STRING);
            //static bool juce_x11_requestSelectionContent(String &selection_content, Atom selection, Atom requested_format)

            Atom property_name = XInternAtom(dpy, "PLANESHIFT_SEL", false);

            /* the selection owner will be asked to set the PLANESHIFT_SEL property on the PS window(w) with the selection content */
            XConvertSelection(dpy, selection, XA_UTF8_STRING, property_name, w, CurrentTime);

        }
    }
}
#endif

bool PawsManager::LoadTooltips(const char* fileName)
{
    // if the file structure changes don't forget to change it in pawsConfigTooltips too!
    csRef<iDocument> ToolTipdoc;
    csRef<iDocumentNode> ToolTiproot,TooltipsNode;
    csString TooltipOption;

    ToolTipdoc = ParseFile(GetObjectRegistry(), fileName);
    if(ToolTipdoc == NULL)
    {
        Error1("Failed to parse file");
        return false;
    }

    ToolTiproot = ToolTipdoc->GetRoot();
    if(ToolTiproot == NULL)
    {
        Error1("File has no XML root");
        return false;
    }

    TooltipsNode = ToolTiproot->GetNode("tooltips");
    if(TooltipsNode == NULL)
    {
        Error1("File has no <tooltips> tag");
        return false;
    }
    else
    {
        csRef<iDocumentNodeIterator> ToolTipoNodes = TooltipsNode->GetNodes();
        while(ToolTipoNodes->HasNext())
        {
            csRef<iDocumentNode> TooltipOption = ToolTipoNodes->Next();
            csString ToolTipnodeName(TooltipOption->GetValue());

            if(ToolTipnodeName == "enable_tooltips")
                ToolTipEnable = TooltipOption->GetAttributeValueAsBool("value");

            if(ToolTipnodeName == "enable_bgcolor")
                ToolTipEnableBgColor = TooltipOption->GetAttributeValueAsBool("value");

            if(ToolTipnodeName == "bgcolor")
                TooltipsColors[0] = TooltipOption->GetAttributeValueAsInt("value");

            if(ToolTipnodeName == "fontcolor")
                TooltipsColors[1] = TooltipOption->GetAttributeValueAsInt("value");

            if(ToolTipnodeName == "shadowcolor")
                TooltipsColors[2] = TooltipOption->GetAttributeValueAsInt("value");
        }
    }
    return true;
}

csString PawsManager::getToolTipSkinPath()
{
    csString t;
    t = GetLocalization()->FindLocalizedFile("tooltips.xml");
    return t;
}
