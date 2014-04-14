/*
 * pawswidget.cpp - Author: Andrew Craig
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
// pawswidget.cpp: implementation of the pawsWidget class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>

#include <ctype.h>

#include <iutil/cfgmgr.h>
#include <iutil/evdefs.h>
#include <ivideo/fontserv.h>
#include <csutil/xmltiny.h>

#include "util/localization.h"

#include "pawswidget.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"
#include "pawsprefmanager.h"
#include "pawsborder.h"
#include "pawsmenu.h"
#include "pawsmainwidget.h"
#include "pawsscript.h"
#include "pawstitle.h"
#include "util/log.h"
#include "util/psstring.h"
#include "widgetconfigwindow.h"

#define RX(x) ((int) ((((float)x)*ratioX)+.5))
#define RY(y) ((int) ((((float)y)*ratioY)+.5))

//#define PAWS_CONSTRUCTION // Enable this to be able to move around widgets at will


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsWidget::pawsWidget() :
    id(-1),
    parent(NULL),
    defaultFrame(csRect(0, 0, 0, 0)),
    screenFrame(csRect(0, 0, 0, 0)),
    clipRect(csRect(0, 0, 0, 0)),
    titleBar(NULL),
    close_widget(NULL),
    onEnter(NULL),
    visible(true),
    saveWidgetPositions(false),
    configurable(false),
    movable(false),
    isResizable(false),
    showResize(true),
    resizeToScreen(false),
    keepaspect(true),
    alwaysOnTop(false),
    min_width(DEFAULT_MIN_WIDTH),
    min_height(DEFAULT_MIN_HEIGHT),
    name("None"),
    bgColour(-1),
    border(NULL),
    attachFlags(0),
    hasFocus(false),
    hasMouseFocus(false),
    fadeVal(100),
    alpha(0),
    alphaMin(0),
    fade(true),
    fadeSpeed(8.0f),
    contextMenu(NULL),
    hasBorderColours(false),
    defaultFontColour(-1),
    defaultFontSize(10),
    fontSize(10),
    scaleFont(true),
    fontStyle(0),
    ignore(false),
    margin(0),
    extraData(NULL),
    needsRender(false),
    parentDraw(true)

{
    graphics2D = PawsManager::GetSingleton().GetGraphics2D();

    max_height = graphics2D->GetHeight();
    max_width = graphics2D->GetWidth();

    for(size_t a = 0; a < PW_SCRIPT_EVENT_COUNT; ++a)
        scriptEvents[a] = 0;
}
pawsWidget::pawsWidget(const pawsWidget &origin) :
    id(origin.id),
    parent(NULL),
    defaultFrame(csRect(0, 0, 0, 0)),
    screenFrame(origin.screenFrame),
    clipRect(origin.clipRect),
    titleBar(origin.titleBar),
    close_widget(origin.close_widget),
    onEnter(NULL),
    visible(origin.visible),
    saveWidgetPositions(origin.saveWidgetPositions),
    configurable(origin.configurable),
    movable(origin.movable),
    isResizable(origin.isResizable),
    showResize(origin.showResize),
    resizeToScreen(origin.resizeToScreen),
    keepaspect(origin.keepaspect),
    alwaysOnTop(origin.alwaysOnTop),
    min_width(origin.min_width),
    min_height(origin.min_height),
    max_width(origin.max_width),
    max_height(origin.max_height),
    name(origin.name),
    bgColour(origin.bgColour),
    border(NULL),
    attachFlags(origin.attachFlags),
    hasFocus(origin.hasFocus),
    hasMouseFocus(origin.hasMouseFocus),
    fadeVal(origin.fadeVal),
    alpha(origin.alpha),
    alphaMin(origin.alphaMin),
    fade(origin.fade),
    fadeSpeed(origin.fadeSpeed),
    contextMenu(NULL),
    hasBorderColours(origin.hasBorderColours),
    defaultFontColour(origin.defaultFontColour),
    defaultFontSize(origin.defaultFontSize),
    fontSize(origin.fontSize),
    scaleFont(origin.scaleFont),
    fontStyle(origin.fontStyle),
    ignore(origin.ignore),
    margin(origin.margin),
    extraData(origin.extraData),
    needsRender(origin.needsRender),
    parentDraw(origin.parentDraw)
{
    graphics2D = PawsManager::GetSingleton().GetGraphics2D();

    for(size_t a = 0; a < PW_SCRIPT_EVENT_COUNT; ++a)
        scriptEvents[a] = origin.scriptEvents[a];

    //
    id = origin.id;//id overlapped!!
    parent = origin.parent;
    defaultFrame = origin.defaultFrame;
    screenFrame = origin.screenFrame;
    clipRect = origin.clipRect;
    visible = origin.visible;
    saveWidgetPositions = origin.saveWidgetPositions;
    configurable = origin.configurable;
    movable = origin.movable;
    isResizable = origin.isResizable;
    showResize = origin.showResize;
    resizeToScreen = origin.resizeToScreen;
    keepaspect = origin.keepaspect;
    alwaysOnTop = origin.alwaysOnTop;
    min_height = origin.min_height;
    min_width = origin.min_width;
    max_height = origin.max_height;
    max_width = origin.max_width;
    name = origin.name + "_c";
    bgColour = origin.bgColour;
    attachFlags = origin.attachFlags;
    hasBorderColours = origin.hasBorderColours;
    hasFocus = origin.hasFocus;
    hasMouseFocus = origin.hasMouseFocus;
    fadeVal = origin.fadeVal;
    alpha = origin.alpha;
    alphaMin = origin.alphaMin;
    fade = origin.fade;
    fadeSpeed = origin.fadeSpeed;
    defaultFontColour = origin.defaultFontColour;
    defaultFontShadowColour = origin.defaultFontShadowColour;
    defaultFontSize = origin.defaultFontSize;
    fontName = origin.fontName;
    fontSize = origin.fontSize;
    scaleFont = origin.scaleFont;
    fontStyle = origin.fontStyle;
    ignore = origin.ignore;
    margin = origin.margin;
    factory = origin.factory;
    needsRender = origin.needsRender;
    parentDraw = origin.parentDraw;
    toolTip = origin.toolTip;
    defaultToolTip = origin.defaultToolTip;

    bgImage = origin.bgImage;
    maskImage = origin.maskImage;
    myFont = origin.myFont;

    //derived widget could copy other attributes here
    if(origin.titleBar != 0)
    {
        titleBar = new pawsTitle(*origin.titleBar);
        titleBar->SetParent(this);
    }

    if(origin.border != 0)
    {
        border = new pawsBorder(*origin.border);
        border->SetParent(this);
    }

    if(origin.contextMenu != 0)
    {
        contextMenu = new pawsMenu(*origin.contextMenu);
        contextMenu->SetNotify(this);
        PawsManager::GetSingleton().GetMainWidget()->AddChild(contextMenu);
    }

    //copy all children
    for(unsigned int i = 0 ; i < origin.children.GetSize(); i++)
    {
        pawsWidget* pw = PawsManager::GetSingleton().CreateWidget(origin.children[i]->factory, origin.children[i]); //copy constructor is called by factory
        if(pw)
        {
            if(origin.close_widget == origin.children[i]) close_widget = dynamic_cast<pawsButton*>(pw);
            if(origin.onEnter == origin.children[i]) onEnter = pw;
            pw->SetParent(this);
            children.Push(pw);
        }
    }
}

pawsWidget::~pawsWidget()
{
    // Check to see if the widget position should be save.
    if(saveWidgetPositions)
        SavePosition();

    if(configurable)
        SaveSettings();

    while(children.GetSize() > 0)
    {
        pawsWidget* wdg = children.Pop();
        if(wdg)
            delete wdg;
    }

    if(contextMenu != NULL)
    {
        PawsManager::GetSingleton().GetMainWidget()->DeleteChild(contextMenu);
        contextMenu = NULL;
    }

    PawsManager::GetSingleton().UnSubscribe(this);

    PawsManager::GetSingleton().OnWidgetDeleted(this);

    if(border)
        delete border;

    if(extraData)
        delete extraData;

    if(parent)
        parent->RemoveChild(this);

    for(size_t i = 0; i < PW_SCRIPT_EVENT_COUNT; i++)
    {
        if(scriptEvents[i])
        {
            delete scriptEvents[i];
            scriptEvents[i] = NULL;
        }
    }
}

void pawsWidget::SetFilename(const char* name)
{
    filename=name;
}

const char* pawsWidget::GetFilename()
{
    return filename;
}

bool pawsWidget::CheckButtonPressed(int button, int modifiers, pawsWidget* pressedWidget)
{
    return OnButtonPressed(button, modifiers, pressedWidget);
}

bool pawsWidget::CheckButtonReleased(int button, int modifiers, pawsWidget* pressedWidget)
{
    if(pressedWidget->GetName()!=NULL  &&  GetCloseName()!=NULL  &&  strcmp(pressedWidget->GetName(),GetCloseName())==0)
    {
        if(modifiers == (CSMASK_CTRL | CSMASK_ALT))
            ReloadWidget();
        else
            Close();

        return true;
    }
    else
    {
        return OnButtonReleased(button, modifiers, pressedWidget);
    }
}

/*
 * Check to see if this widget contains these coordinates at all.
 * Recurse through children to see if they could the one the mouse is over.
 * Return this widget if no children contain the coords.
 */
pawsWidget* pawsWidget::WidgetAt(int x, int y)
{
    if(ignore || !Contains(x,y))
        return NULL;

    //Recurse through children to see if they could the one the mouse is over.
    for(size_t z = 0; z < children.GetSize(); z++)
    {
        if(!children[z]->ignore  &&  children[z]->IsVisible())
        {
            if(children[z]->Contains(x,y))
                return children[z]->WidgetAt(x , y);
        }
    }

    //Return this widget if no children contain the coords.
    return this;
}

bool pawsWidget::Contains(int x, int y)
{
    bool val = false;

    if(titleBar)
    {
        val = titleBar->screenFrame.Contains(x,y) || screenFrame.Contains(x,y);
    }
    else if(border)
    {
        val = border->GetRect().Contains(x,y);
    }
    else
    {
        val = screenFrame.Contains(x,y);
    }

    return val;
}

void pawsWidget::SetParent(pawsWidget* newParent)
{
    parent = newParent;
}

void pawsWidget::AddChild(pawsWidget* childWidget)
{
    if(!childWidget)
        return;

    if(childWidget->IsAlwaysOnTop())
        children.Insert(0, childWidget);
    else
        children.Push(childWidget);

    // Let the child know that he is attached to this widget.
    childWidget->SetParent(this);
}

void pawsWidget::AddChild(size_t Index, pawsWidget* childWidget)
{
    if(!childWidget)
        return;

    //if ( childWidget->IsAlwaysOnTop() )
    children.Insert(Index, childWidget);
    //else
    //children.Push( childWidget );

    // Let the child know that he is attached to this widget.
    childWidget->SetParent(this);
}

void pawsWidget::RemoveChild(pawsWidget* widget)
{
    if(!widget)
        return;

    if(children.Delete(widget))
        widget->SetParent(NULL);
}

void pawsWidget::DeleteChild(pawsWidget* widget)
{
    RemoveChild(widget);
    delete widget;
}

bool pawsWidget::IsIndirectChild(pawsWidget* widget)
{
    for(size_t z = 0; z < children.GetSize(); z++)
    {
        if(children[z] == widget)
            return true;
        if(children[z]->IsIndirectChild(widget))
            return true;
    }
    return false;
}

bool pawsWidget::IsChildOf(pawsWidget* someParent)
{
    // Search through all parents to check if someParent is an actual parent
    pawsWidget* widget = this;
    do
    {
        if(widget == someParent)
        {
            return true;
        }
        widget = widget->GetParent();
    }
    while(widget);

    return false;
}


bool pawsWidget::Includes(pawsWidget* widget)
{
    return widget && (widget == this || IsIndirectChild(widget));
}

bool pawsWidget::LoadAttributes(iDocumentNode* node)
{
    // Get the name for this widget.
    csRef<iDocumentAttribute> atr = node->GetAttribute("name");
    if(atr)
        SetName(csString(atr->GetValue()));
    /*    {
            Error1("Widget has not defined a name. Error in XML");
            return false;
        }
        */

    atr = node->GetAttribute("style");

    if(atr)
        PawsManager::GetSingleton().ApplyStyle(atr->GetValue(), node);
    else
        PawsManager::GetSingleton().ApplyStyle(FindDefaultWidgetStyle(factory), node);

    ReadDefaultWidgetStyles(node);

    //if we don't find the attribute we set the previous one
    ignore = node->GetAttributeValueAsBool("ignore", ignore);


    // Check to see if this widget is visible directly after a load.
    if(node->GetAttributeValueAsBool("visible", visible))
    {
        Show();
    }
    else
    {
        Hide();
    }

    // Check to see if this widget should save it's position
    saveWidgetPositions = node->GetAttributeValueAsBool("savepositions", saveWidgetPositions);

    // Check to see if this widget is movable
    movable = node->GetAttributeValueAsBool("movable", movable);

    // Check to see if this widget is configurable
    configurable = node->GetAttributeValueAsBool("configurable", configurable);

    isResizable = node->GetAttributeValueAsBool("resizable", isResizable);

    resizeToScreen = node->GetAttributeValueAsBool("resizetoscreen", parent? parent->resizeToScreen : resizeToScreen);

    keepaspect = node->GetAttributeValueAsBool("keepaspect", keepaspect);

    atr = node->GetAttribute("ContextMenu");
    if(atr)
        contextMenuFile = atr->GetValue();

    id = node->GetAttributeValueAsInt("id", id);

    atr = node->GetAttribute("xmlbinding");
    if(atr)
    {
        xmlbinding = atr->GetValue();
    }

    alwaysOnTop = node->GetAttributeValueAsBool("alwaysontop", alwaysOnTop);

    // Get tool tip, if any
    atr = node->GetAttribute("tooltip");
    if(atr)
    {
        toolTip = PawsManager::GetSingleton().Translate(atr->GetValue());
        defaultToolTip = toolTip;
    }

    bool inheritFont = true;
    atr = node->GetAttribute("inheritfont");
    if(atr)
    {
        csString choice = csString(atr->GetValue());
        if(choice == "no") inheritFont = false;
    }

    // Set font to be the same as the parent widget.
    if(inheritFont && parent)
    {
        fontName = parent->fontName;
        if(!fontName.IsEmpty())
        {
            defaultFontSize = parent->fontSize;
            fontSize = defaultFontSize;
            scaleFont = parent->scaleFont;
            fontSize = parent->fontSize;
            myFont = parent->myFont;
            defaultFontColour = parent->defaultFontColour;
            defaultFontShadowColour = parent->defaultFontShadowColour;
            fontStyle = parent->fontStyle;
        }
    }

    // Get specific font settings for this widget, if specified.
    csRef<iDocumentNode> fontAttribute = node->GetNode("font");
    if(fontAttribute)
    {
        fontName = fontAttribute->GetAttributeValue("name");

        // Check if it's a short definition
        if(!fontName.StartsWith("/") && fontName.Length())
        {
            fontName = PawsManager::GetSingleton().GetLocalization()->FindLocalizedFile("data/ttf/" + fontName);
        }
        defaultFontSize = fontAttribute->GetAttributeValueAsFloat("size");
        fontSize = defaultFontSize;
        bool scaleToScreen = fontAttribute->GetAttributeValueAsBool("resizetoscreen", true);
        scaleFont = fontAttribute->GetAttributeValueAsBool("scalefont", false);

        if(this->resizeToScreen && scaleToScreen)
            fontSize *= PawsManager::GetSingleton().GetFontFactor();

        if(fontName.Length())
        {
            myFont = graphics2D->GetFontServer()->LoadFont(fontName, (fontSize)?fontSize:10);
            if(!myFont)
            {
                Error2("Could not load font: >%s<", (const char*)fontName);
                return false;
            }
        }
        int r = fontAttribute->GetAttributeValueAsInt("r");
        int g = fontAttribute->GetAttributeValueAsInt("g");
        int b = fontAttribute->GetAttributeValueAsInt("b");

        if(r == -1  &&  g == -1  &&  b == -1)
            defaultFontColour = PawsManager::GetSingleton().GetPrefs()->GetDefaultFontColour();
        else
            defaultFontColour = graphics2D->FindRGB(r, g, b);

        r = fontAttribute->GetAttributeValueAsInt("sr");
        g = fontAttribute->GetAttributeValueAsInt("sg");
        b = fontAttribute->GetAttributeValueAsInt("sb");

        defaultFontShadowColour = graphics2D->FindRGB(r, g, b);

        if(fontAttribute->GetAttributeValueAsBool("shadow"))
            fontStyle |= FONT_STYLE_DROPSHADOW;
        if(fontAttribute->GetAttributeValueAsBool("bold"))
            fontStyle |= FONT_STYLE_BOLD;
    }

    // Get the frame for this widget.
    csRef<iDocumentNode> frameNode = node->GetNode("frame");
    if(frameNode)
    {
        defaultFrame.xmin = GetActualWidth(frameNode->GetAttributeValueAsInt("x"));
        defaultFrame.ymin = GetActualHeight(frameNode->GetAttributeValueAsInt("y"));
        int width  = GetActualWidth(frameNode->GetAttributeValueAsInt("width"));
        int height = GetActualHeight(frameNode->GetAttributeValueAsInt("height"));

        if(width < min_width)
            width = min_width;
        if(height < min_height)
            height = min_height;

        defaultFrame.SetSize(width, height);

        // If this widget has a parent then move it to it's correct relative position.
        if(parent)
        {
            MoveTo(parent->GetScreenFrame().xmin + defaultFrame.xmin,
                   parent->GetScreenFrame().ymin + defaultFrame.ymin);
            screenFrame.SetSize(width, height);
        }
        else
        {
            MoveTo(defaultFrame.xmin, defaultFrame.ymin);
            SetSize(defaultFrame.Width(), defaultFrame.Height());
        }

        csRef<iDocumentAttribute> useBorder = frameNode->GetAttribute("border");
        if(useBorder)
        {
            csString borderString = useBorder->GetValue();

            if(borderString != "no")
            {
                if(borderString == "yes")
                    borderString.Replace("line");

                if(border)
                    delete border;

                border = new pawsBorder(borderString);
                border->SetParent(this);
            }
        }

        if(border)
        {
            if(frameNode->GetAttributeValueAsBool("justtitle", false))
                border->JustTitle();
        }

        margin = frameNode->GetAttributeValueAsInt("margin", margin);
    }

    // Process title bar, if any
    csRef<iDocumentNode> titleNode = node->GetNode("title");
    if(titleNode)
    {
        csString title = PawsManager::GetSingleton().Translate(titleNode->GetAttributeValue("text"));
        csString image = titleNode->GetAttributeValue("resource");
        csString align = titleNode->GetAttributeValue("align");
        csString close = titleNode->GetAttributeValue("close_button");
        bool shadowTitle = titleNode->GetAttributeValueAsBool("shadow", true);

        SetTitle(title, image, align, close, shadowTitle);
    }

    // new title bar
    csRef<iDocumentNode> newTitleNode = node->GetNode("newtitle");
    if(newTitleNode)
        titleBar = new pawsTitle(this, newTitleNode);

    csRef<iDocumentNode> borderNode = node->GetNode("childborders");
    if(borderNode)
        LoadBorderColours(borderNode);

    csRef<iDocumentNode> bgColourNode = node->GetNode("bgcolour");
    if(bgColourNode)
    {
        int r = bgColourNode->GetAttributeValueAsInt("r");
        int g = bgColourNode->GetAttributeValueAsInt("g");
        int b = bgColourNode->GetAttributeValueAsInt("b");

        bgColour = graphics2D->FindRGB(r, g, b);
    }
    else
    {
        bgColour = -1;
    }

    // Get the background image attribute
    csRef<iDocumentNode> bgImageNode = node->GetNode("bgimage");
    if(bgImageNode)
    {
        csString image = bgImageNode->GetAttributeValue("resource");
        bgImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(image);
        if(!bgImage)
        {
            Warning2(LOG_PAWS, "GUI image '%s' not found.\n", image.GetData());
        }
        else
        {
            csRef<iDocumentAttribute> alphaattr = bgImageNode->GetAttribute("alpha");
            if(alphaattr)
                alpha = alphaattr->GetValueAsInt();
            else
                alpha = bgImage->GetDefaultAlpha();

            //alphaMax = alpha;
            fade = bgImageNode->GetAttributeValueAsBool("fading", fade);
        }
    }

    if(configurable)
    {
        LoadSettings();
    }

    // Get the masking image attribute
    csRef<iDocumentNode> maskImageNode = node->GetNode("mask");
    if(maskImageNode)
    {
        csString imageStr = maskImageNode->GetAttributeValue("resource");
        if(!imageStr.IsEmpty())
            SetMaskingImage(imageStr);
    }

    // Get the data subscriptions
    csRef<iDocumentNode> subsNode = node->GetNode("subscriptions");
    if(subsNode)
    {
        overwrite_subscription = subsNode->GetAttributeValueAsBool("overwrite",true);
        subscription_format    = subsNode->GetAttributeValue("format");
        csRef<iDocumentNodeIterator> points = subsNode->GetNodes();
        while(points->HasNext())
        {
            csRef<iDocumentNode> point = points->Next();
            if(point->GetAttributeValue("data"))
            {
                PawsManager::GetSingleton().Subscribe(point->GetAttributeValue("data"),this);
            }
        }
    }

    // Get the data we publish
    csRef<iDocumentNodeIterator> publishNodes = node->GetNodes("publish");
    while(publishNodes->HasNext())
    {
        csRef<iDocumentNode> publishNode = publishNodes->Next();
        csString data = publishNode->GetAttributeValue("data");
        if(!data.IsEmpty())
            publishList.Push(data);
    }

    // Get the attachment points
    csRef<iDocumentNode> attachPointsNode = node->GetNode("attachpoints");
    if(attachPointsNode)
    {
        csRef<iDocumentNodeIterator> points = attachPointsNode->GetNodes();
        while(points->HasNext())
        {
            csRef<iDocumentNode> point = points->Next();
            attachFlags |= GetAttachFlag(point->GetAttributeValue("point"));
        }
    }

    // Get the min width and height
    csRef<iDocumentNode> minFrameNode = node->GetNode("minframe");
    if(minFrameNode)
    {
        min_width = GetActualWidth(minFrameNode->GetAttributeValueAsInt("width"));
        min_height = GetActualHeight(minFrameNode->GetAttributeValueAsInt("height"));
    }

    // Get the max width and height
    csRef<iDocumentNode> maxFrameNode = node->GetNode("maxframe");
    if(maxFrameNode)
    {
        max_width = GetActualWidth(maxFrameNode->GetAttributeValueAsInt("width"));
        max_height = GetActualHeight(maxFrameNode->GetAttributeValueAsInt("height"));
    }


    return true;
}

bool pawsWidget::LoadEventScripts(iDocumentNode* node)
{
    // default of null for all events
    size_t a;
    for(a=0; a<PW_SCRIPT_EVENT_COUNT; ++a)
        scriptEvents[a] = 0;

    // load show event
    csRef<iDocumentNode> showEvent = node->GetNode("eventShow");
    if(showEvent)
        scriptEvents[PW_SCRIPT_EVENT_SHOW] = new pawsScript(this, showEvent->GetContentsValue());

    // load hide event
    csRef<iDocumentNode> hideEvent = node->GetNode("eventHide");
    if(hideEvent)
        scriptEvents[PW_SCRIPT_EVENT_HIDE] = new pawsScript(this, hideEvent->GetContentsValue());

    // load mousedown event
    csRef<iDocumentNode> mousedownEvent = node->GetNode("eventMouseDown");
    if(mousedownEvent)
        scriptEvents[PW_SCRIPT_EVENT_MOUSEDOWN] = new pawsScript(this, mousedownEvent->GetContentsValue());

    // load mouseup event
    csRef<iDocumentNode> mouseupEvent = node->GetNode("eventMouseUp");
    if(mouseupEvent)
        scriptEvents[PW_SCRIPT_EVENT_MOUSEUP] = new pawsScript(this, mouseupEvent->GetContentsValue());

    // load value changed event
    csRef<iDocumentNode> valueChangedEvent = node->GetNode("eventValueChanged");
    if(valueChangedEvent)
        scriptEvents[PW_SCRIPT_EVENT_VALUECHANGED] = new pawsScript(this, valueChangedEvent->GetContentsValue());

    return true;
}

void pawsWidget::ReloadWidget()
{
    if(!GetFilename())
        parent->ReloadWidget();
    else
    {
        csString copyname = filename;
        delete this;
        PawsManager::GetSingleton().LoadWidget(copyname);
    }
}

bool pawsWidget::Load(iDocumentNode* node)
{
    if(! LoadAttributes(node))
        return false;

    // Call the setup for this widget.
    if(!Setup(node))
    {
        Error2("Widget %s Setup Failed", GetName());
        return false;
    }

    //if (name=="ChatWindow")
    //    printf("Chat found.");

    if(! LoadChildren(node))
        return false;


    // load scripts after we've loaded all children
    if(!LoadEventScripts(node))
        return false;

    if(!PostSetup())
    {
        Error2("Failed the PostSetup for %s", GetName());
        return false;
    }

    // Check to see if we have possible saved positions to use.
    if(saveWidgetPositions)
    {
        csRect pos = LoadPositions();
        // Resize to loaded positions.
        MoveTo(pos.xmin, pos.ymin);

        //only set size if widget is resizable
        if(IsResizable())
            SetSize(pos.Width(), pos.Height());
    }

    Resize();
    Resize(0, 0, RESIZE_RIGHT | RESIZE_LEFT | RESIZE_TOP | RESIZE_BOTTOM);
    StopResize();

    if(visible)
        RunScriptEvent(PW_SCRIPT_EVENT_SHOW);
    return true;
}

bool pawsWidget::LoadChildren(iDocumentNode* node)
{
    pawsWidget* widget;
    // Get an iterator over all the child widgets
    csRef<iDocumentNodeIterator> childIter = node->GetNodes();

    while(childIter->HasNext())
    {
        csRef<iDocumentNode> childWidgetNode = childIter->Next();

        if(!strcmp(childWidgetNode->GetValue(), "map"))
            continue;

        widget=NULL;

        // Check if the child should be loaded from
        csString file = childWidgetNode->GetAttributeValue("file");
        if(!file.IsEmpty())
        {
            size_t i;
            csArray<pawsWidget*> childarray;
            PawsManager::GetSingleton().LoadChildWidgets(file,childarray);

            for(i=0; i<childarray.GetSize(); i++)
                AddChild(childarray[i]);

            // If more than one child widget was loaded, then any sub configuration in this file is meaningless
            if(childarray.GetSize() > 1)
                return true;
            if(childarray.GetSize())
                widget=childarray[0];
        }

        // If a widget hasn't been loaded here - either due to a failure to load above or
        //  because no file was specified, parse this node as an embedded widget definition
        if(!widget)
        {
            csString factory = childWidgetNode->GetAttributeValue("factory");

            if(factory.Length() == 0)
            {
                factory=childWidgetNode->GetValue();
                if(strncmp(factory,"paws",4) != 0)
                    continue;
            }

            widget = PawsManager::GetSingleton().CreateWidget(factory);
            CS_ASSERT_MSG("Creating widget from factory name failed.", widget!=NULL);

            AddChild(widget);
        }

        // Let the widget parse the rest of the node information
        if(!widget->Load(childWidgetNode))
        {
            Error1("Failed to load child widget");
            return false;
        }
    }

    // Need to load form properties here, because we need to have the children loaded

    // Get form properties
    csRef<iDocumentNode> formNode = node->GetNode("form");
    if(formNode)
    {
        csRef<iDocumentNodeIterator> fNode = formNode->GetNodes();
        while(fNode->HasNext())
        {
            csRef<iDocumentNode> lNode = fNode->Next();

            if(!strcmp(lNode->GetValue(),"enter"))
            {
                onEnter = FindWidget(lNode->GetAttributeValue("name"));
            }

            if(!strcmp(lNode->GetValue(),"tab"))
            {
                pawsWidget* wdg = FindWidget(lNode->GetAttributeValue("name"));
                if(wdg)
                    taborder.Push(wdg);
            }
        }
    }

    return true;
}

void pawsWidget::ShowBehind()
{
    //printf("Called pawsWidget::ShowBehind on %s \n",this->GetName());
    visible = true;
    if(border) border->Show();

    //this mystic code makes nonsense.
    //if a widget is focused why it should be shown, likewise why it should be brought to top?
    /* pawsWidget * focused = PawsManager::GetSingleton().GetCurrentFocusedWidget();
     if ( focused )
     {
         if(focused->IsVisible())
             BringToTop(focused);
         else
             focused->Show();
     }*/
}

void pawsWidget::Show()
{
    //printf("Called pawsWidget::Show on %s \n",this->GetName());
    visible = true;
    if(border) border->Show();
    BringToTop(this);
    RunScriptEvent(PW_SCRIPT_EVENT_SHOW);
}

void pawsWidget::Hide()
{
    //printf("Called pawsWidget::Hide on %s \n",this->GetName());
    visible = false;
    if(border)
        border->Hide();

    PawsManager::GetSingleton().OnWidgetHidden(this);
    RunScriptEvent(PW_SCRIPT_EVENT_HIDE);
}

void pawsWidget::SetRelativeFrame(int x, int y, int width, int height)
{
    // Must set width and height first so children don't get squeezed
    SetRelativeFrameSize(width, height);
    SetRelativeFramePos(x, y);
}

void MoveRect(csRect &rect, int x, int y)
{
    int width  = rect.Width();
    int height = rect.Height();

    rect.Set(x, y, x+width, y+height);
}

void pawsWidget::SetRelativeFramePos(int x, int y)
{
    MoveRect(defaultFrame, x, y);
    if(parent != NULL)
        MoveRect(screenFrame, parent->GetScreenFrame().xmin + x, parent->GetScreenFrame().ymin + y);
    else
        MoveRect(screenFrame, x, y);

    for(size_t x = 0; x < children.GetSize(); x++)
        children[x]->RecalcScreenPositions();
}

void pawsWidget::SetRelativeFrameSize(int width, int height)
{
    defaultFrame .SetSize(width, height);
    screenFrame  .SetSize(width, height);

    OnResize();

    for(size_t x = 0; x < children.GetSize(); x++)
        children[x]->RecalcScreenPositions();
}

void pawsWidget::RecalcScreenPositions()
{
    if(parent)
    {
        screenFrame.xmin  = parent->GetScreenFrame().xmin + defaultFrame.xmin;
        screenFrame.ymin  = parent->GetScreenFrame().ymin + defaultFrame.ymin;
        screenFrame.SetSize(defaultFrame.Width(), defaultFrame.Height());

        for(size_t x = 0; x < children.GetSize(); x++)
            children[x]->RecalcScreenPositions();
    }
}

void pawsWidget::UseBorder(const char* style)
{
    if(border)
        delete border;
    border = new pawsBorder(style);
    border->SetParent(this);
}

void pawsWidget::SetBackground(const char* image)
{
    if(!image || (strcmp(image,"") == 0))
    {
        bgImage = NULL;
        return;
    }
    bgImage = PawsManager::GetSingleton().GetTextureManager()->GetOrAddPawsImage(image);

    if(!bgImage)
    {
        csString parentName("None");
        if(parent)
            parentName = parent->GetName();
        return;
    }

    alpha = bgImage->GetDefaultAlpha();

}

csString pawsWidget::GetBackground()
{
    if(bgImage)
        return bgImage->GetName();

    return csString("");
}

void pawsWidget::SetBackgroundAlpha(int value)
{
    alpha = value;
    alphaMin = value;
}

pawsWidget* pawsWidget::FindWidget(const char* name, bool complain)
{
    if(!name)
        return NULL;

    if(this->name == name)
        return this;

    for(size_t z = 0; z < children.GetSize(); z++)
    {
        pawsWidget* widget = children[z]->FindWidget(name, false);
        if(widget != NULL)
            return widget;
    }
    /*This check needs to be done here, because it might happen that even if this->name == "None",
    it has children and so it can return a widget! */
    if(this->name == "None")
        return NULL;
    if(complain)
        Error4("Could not locate widget %s in %s (%s)", name, this->name.GetData(), this->GetFilename());
    return NULL;
}


pawsWidget* pawsWidget::FindWidget(int ID, bool complain)
{
    if(this->id == ID)
        return this;

    for(size_t z = 0; z < children.GetSize(); z++)
    {
        pawsWidget* widget = children[z]->FindWidget(ID, false);
        if(widget != NULL)
            return widget;
    }

    if(complain)
        Error3("Could not locate widget ID=%i in %s", ID, name.GetData());
    return NULL;
}

pawsWidget* pawsWidget::FindWidgetXMLBinding(const char* xmlbinding)
{
    if((this->xmlbinding == xmlbinding) ||
            (this->xmlbinding.IsEmpty() && name == xmlbinding))
        return this;

    for(size_t z = 0; z < children.GetSize(); z++)
    {
        pawsWidget* widget = children[z]->FindWidgetXMLBinding(xmlbinding);
        if(widget != NULL)
            return widget;
    }
    return NULL;
}

void pawsWidget::DrawBackground()
{
    int drawAlpha;

    if(bgColour != -1)
    {
        graphics2D->DrawBox(screenFrame.xmin,
                            screenFrame.ymin,
                            screenFrame.Width(),
                            screenFrame.Height(),
                            bgColour);
    }

    if(bgImage)
    {
        bool focus = hasFocus;
        // If my children are focused, I should fade like I am focused
        if(!focus)
        {
            for(size_t x = children.GetSize(); x-- > 0;)
                focus = children[x]->HasFocus();
        }

        drawAlpha = -1;
        // if the fading feature for this widget is enabled
        if(alpha && fade)
        {
            // if the widget hasn't got the focus
            if(!focus)
            {
                // and the mouse is inside the widget
                if(hasMouseFocus)
                {
                    // slowly become opaque
                    fadeVal-=fadeSpeed;
                }
                else
                {
                    // else slowly fade out
                    fadeVal+=fadeSpeed;
                }
            }
            else
            {
                // when it has got the focus become/stay opaque
                fadeVal=0;
            }

            if(fadeVal<0) fadeVal=0;
            if(fadeVal>100) fadeVal=100;

            drawAlpha = (int)(alphaMin + (alpha-alphaMin) * fadeVal * 0.010);
        }

        bgImage->Draw(screenFrame, drawAlpha);
    }

    if(border)
        border->Draw();

    if(IsResizable() && showResize)
    {
        csRef<iPawsImage> resize = PawsManager::GetSingleton().GetResizeImage();
        if(resize)
            resize->Draw(GetScreenFrame().xmax-8, GetScreenFrame().ymax-8,8,8);
    }
}

void pawsWidget::DrawMask()
{
    // Draw the masking image
    int drawAlpha = -1;
    if(fade && parent && parent->GetMaxAlpha() >= 0 && bgImage && maskImage)
    {
        fadeVal = parent->GetFadeVal();
        alpha = parent->GetMaxAlpha();
        alphaMin = parent->GetMinAlpha();
        drawAlpha = (int)(alphaMin + (alpha-alphaMin) * fadeVal * 0.010);
    }
    if(maskImage)
    {
        graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
        maskImage->Draw(screenFrame.xmin, screenFrame.ymin, screenFrame.Width(), screenFrame.Height(), drawAlpha);
    }
}

bool pawsWidget::DrawWindow()
{
    // Setup our clipping rect so we know where we can draw
    ClipToParent(true);

    // If we can't draw anywhere, then don't try.  Our children can't be drawn either.
    if(clipRect.IsEmpty())
        return false;

    DrawBackground();
    return true;
}

void pawsWidget::DrawForeground()
{
    ClipToParent(false);
    DrawChildren();
    DrawMask();

    if(titleBar)
        titleBar->Draw();
}

void pawsWidget::Draw()
{
    if(DrawWindow())
    {
        DrawForeground();
    }
}

void pawsWidget::DrawWidgetText(const char* text, int x, int y, int style)
{
    csRef<iFont> font = GetFont();
    if(style==-1)
        style = GetFontStyle();

    if(parent && !parent->GetBackground().IsEmpty() && parent->isFadeEnabled() && parent->GetMaxAlpha() != parent->GetMinAlpha())
    {
        int a = (int)(255 - (parent->GetMinAlpha() + (parent->GetMaxAlpha()-parent->GetMinAlpha()) * parent->GetFadeVal() * 0.010));
        int r, g, b;

        if(style & FONT_STYLE_DROPSHADOW)
        {
            graphics2D->GetRGB(GetFontShadowColour(), r, g, b);
            graphics2D->Write(font, x+2, y+2, graphics2D->FindRGB(r, g, b, a), -1, text);
        }
        graphics2D->GetRGB(GetFontColour(), r, g, b);
        graphics2D->Write(font, x, y, graphics2D->FindRGB(r, g, b, a), -1, text);
        if(style & FONT_STYLE_BOLD)
        {
            graphics2D->Write(font, x+1, y, graphics2D->FindRGB(r, g, b, a), -1, text);
        }
    }
    else
    {
        if(style & FONT_STYLE_DROPSHADOW)
            graphics2D->Write(font, x+2, y+2, GetFontShadowColour(), -1, text);

        graphics2D->Write(font, x, y, GetFontColour(), -1, text);
        if(style & FONT_STYLE_BOLD)
        {
            graphics2D->Write(font, x+1, y, GetFontColour(), -1, text);
        }
    }
}

csRect pawsWidget::GetWidgetTextRect(const char* text, int x, int y, int /*style*/)
{
    csRef<iFont> font = GetFont();

    int width, height;
    font->GetDimensions(text, width, height);
    csRect rect(x,y,x+width,y+height);
    return rect;
}

void pawsWidget::SetFade(bool value)
{
    fade = value;
}

void pawsWidget::SetFontScaling(bool value)
{
    scaleFont = value;

    if(value)
        ChangeFontSize(defaultFontSize * float(screenFrame.Width())/float(defaultFrame.Width()));
    else
        ChangeFontSize(defaultFontSize);

    for(uint i=0; i < children.GetSize(); i++)
    {
        children.Get(i)->SetFontScaling(value);
    }
}

void pawsWidget::FormatToolTip(const char* fmt, ...)
{
    char text[128];
    va_list args;
    va_start(args, fmt);
    cs_vsnprintf(text,sizeof(text),fmt,args);
    va_end(args);
    SetToolTip((const char*)text);
}

void pawsWidget::DrawToolTip(int x, int y)
{
    if(toolTip.Length() == 0)
        return;

    if(!IsVisible())
        return;

    if(PawsManager::GetSingleton().getToolTipEnable())
    {
        int width=0;
        int height=0;

        int realX = x, realY = y;

        //iFont* font = GetFont();
        csString fontName = "/planeshift/data/ttf/cupandtalon.ttf";

        csRef<iFont> font = graphics2D->GetFontServer()->LoadFont(fontName,16);
        iFont* fontPtr = font;

        if(!fontPtr)
        {
            Error2("Couldn't load font '%s', reverting to default",fontName.GetData());
            fontPtr = GetFont();
        }

        fontPtr->GetDimensions(toolTip , width, height);

        // Draw above the cursor for the time being
        realY = realY - height - 2;

        realX += 2;
        realY += 1;

        if(realY < 0)
        {
            realY = 5;
            realX += PawsManager::GetSingleton().GetMouse()->GetImageSize().width + 2;
        }

        if(realX + width > graphics2D->GetWidth())
            realX = graphics2D->GetWidth() - width - 5;

        // Note: negative value on realX is impossible

        // BgColor
        if(PawsManager::GetSingleton().getToolTipEnableBgColor())
            graphics2D->DrawBox(realX, realY, width+4, height+4, PawsManager::GetSingleton().getTooltipsColors(0));

        // Shadow
        graphics2D->Write(fontPtr, realX+3, realY+3, PawsManager::GetSingleton().getTooltipsColors(2), -1, toolTip);

        // Text
        graphics2D->Write(fontPtr, realX+2, realY+2, PawsManager::GetSingleton().getTooltipsColors(1), -1, toolTip);
    }
}

void pawsWidget::DrawChildren()
{
    for(size_t x = children.GetSize(); x-- > 0;)
    {
        if(children[x]->IsVisible() && children[x]->ParentDraw() &&
                children[x] != titleBar)
        {
            children[x]->Draw();
        }
    }
}

int pawsWidget::CalcChildPosition(pawsWidget* child)
{
    //printf("Called pawsWidget::CalcChildPosition on %s \n",(child ? child->GetName(): "NULL"));
    int pos;

    pos = 0;
    if(! child->IsAlwaysOnTop())
    {
        for(size_t x = 0; x < children.GetSize(); x++)
            if(children[x]->IsAlwaysOnTop())
                pos++;
    }
    return pos;
}

void pawsWidget::BringToTop(pawsWidget* widget)
{
    //printf("Called pawsWidget::BringToTop on %s \n",(widget ? widget->GetName(): "NULL"));
    for(size_t x = 0; x < children.GetSize(); x++)
    {
        if(children[x] == widget)
        {
            children.DeleteIndex(x);
            children.Insert(CalcChildPosition(widget), widget);
            break;
        }
    }

    if(parent)
        parent->BringToTop(this);
}

void pawsWidget::SendToBottom(pawsWidget* widget)
{
    //printf("Called pawsWidget::SendToBottom on %s \n",(widget ? widget->GetName(): "NULL"));
    for(size_t x = 0; x < children.GetSize(); x++)
    {
        if(children[x] == widget)
        {
            //printf("Called pawsWidget::SendToBottom removing element %d \n",x);
            children.DeleteIndex(x);
            children.Push(widget);
            break;
        }
    }
}

void pawsWidget::SetTitle(const char* title)
{
    if(border!=NULL)
    {
        border->SetTitle(title, borderTitleShadow);
        border->Draw();
    }
}


void pawsWidget::SaveSettings()
{
    csRef<iConfigManager> cfgMgr;
    cfgMgr =  csQueryRegistry<iConfigManager > (PawsManager::GetSingleton().GetObjectRegistry());

    csString configName;

    configName.Format("PlaneShift.GUI.%s.MinTransparency", GetName());
    cfgMgr->SetInt(configName, alphaMin);

    configName.Format("PlaneShift.GUI.%s.MaxTransparency", GetName());
    cfgMgr->SetInt(configName, alpha);

    configName.Format("PlaneShift.GUI.%s.FadeSpeed", GetName());
    cfgMgr->SetFloat(configName, fadeSpeed);

    configName.Format("PlaneShift.GUI.%s.Fade", GetName());
    cfgMgr->SetBool(configName, fade);

    configName.Format("PlaneShift.GUI.%s.ScaleFont", GetName());
    cfgMgr->SetBool(configName, scaleFont);

    cfgMgr->Save();
}

void pawsWidget::LoadSettings()
{
    csRef<iConfigManager> cfgMgr;
    cfgMgr =  csQueryRegistry<iConfigManager > (PawsManager::GetSingleton().GetObjectRegistry());

    csString configName;

    configName.Format("PlaneShift.GUI.%s.MaxTransparency", GetName());
    alpha = cfgMgr->GetInt(configName, alpha);

    configName.Format("PlaneShift.GUI.%s.MinTransparency", GetName());
    alphaMin = cfgMgr->GetInt(configName, alphaMin);

    configName.Format("PlaneShift.GUI.%s.Fade", GetName());
    fade = cfgMgr->GetBool(configName, fade);

    configName.Format("PlaneShift.GUI.%s.ScaleFont", GetName());
    scaleFont = cfgMgr->GetBool(configName, scaleFont);

    configName.Format("PlaneShift.GUI.%s.FadeSpeed", GetName());
    fadeSpeed = cfgMgr->GetFloat(configName, fadeSpeed);

}

void pawsWidget::CreateWidgetConfigWindow()
{
    psString title = name + "settings";
    pawsWidget* widget = PawsManager::GetSingleton().GetMainWidget()->FindWidget(title);

    // there can only be one config window active for each widget
    WidgetConfigWindow* configWindow = dynamic_cast<WidgetConfigWindow*>(widget);
    if(widget == NULL)
    {
        configWindow = new WidgetConfigWindow();
        configWindow->LoadFromFile("widgetconfigwindow.xml");
        configWindow->SetConfigurableWidget(this);
        configWindow->SetName(title);
        configWindow->Show();
        PawsManager::GetSingleton().GetMainWidget()->AddChild(configWindow);
    }
    else
    {
        configWindow->SetConfigurableWidget(this);
        configWindow->Show();
    }
}

void pawsWidget::DestroyWidgetConfigWindow()
{
    psString title = name + "settings";
    pawsWidget* widget = PawsManager::GetSingleton().GetMainWidget()->FindWidget(title);

    // there can only be one config window active for each widget
    WidgetConfigWindow* configWindow = dynamic_cast<WidgetConfigWindow*>(widget);
    if(configWindow)
    {
        configWindow->Hide();
        PawsManager::GetSingleton().GetMainWidget()->DeleteChild(configWindow);
    }
}

bool pawsWidget::CreateContextMenu()
{
    psPoint mousePos;

    mousePos = PawsManager::GetSingleton().GetMouse()->GetPosition();

    contextMenu = new pawsMenu();
    if(! contextMenu->LoadFromFile(contextMenuFile))
    {
        delete contextMenu;
        contextMenu = NULL;
        return false;
    }
    contextMenu->SetNotify(this);
    PawsManager::GetSingleton().GetMainWidget()->AddChild(contextMenu);
    contextMenu->CenterTo(mousePos.x, mousePos.y);
    contextMenu->MakeFullyVisible();
    contextMenu->Show();
    contextMenu->SetAlwaysOnTop(true);

    // add an 'settings' option to the menu if 'configurable'
    // is set to 'yes'
    if(configurable)
    {
        // add menu item "ConfigWindow"
        pawsMenuItem* item = new pawsMenuItem();
        item->MakeFullyVisible();
        item->Show();
        item->SetAlwaysOnTop(true);
        item->SetColour(0xffffff);
        item->SetLabel("Settings");
        item->SetSizes(-1, 1, 1);
        item->SetName("Settings");

        // assign "configure"-action to this menu item
        pawsMenuAction action;
        action.name="configure";
        item->SetAction(action);
        //    item->SetConfigurableWidget(this);

        // add item to menu
        contextMenu->AddItem(item);
    }

    return true;
}

int pawsWidget::ResizeFlags(int x, int y)
{
    csRect frame = GetScreenFrame();
    csRect hotButton(0,0,0,0);
    int flag = 0;

    hotButton= csRect(frame.xmax-8,
                      frame.ymax-8,
                      frame.xmax,
                      frame.ymax);

    if(hotButton.Contains(x,y))
    {
        flag |= RESIZE_BOTTOM;
        flag |= RESIZE_RIGHT;
    }
    return flag;
}

bool pawsWidget::OnMouseEnter()
{
    if(parent)
        return parent->OnChildMouseEnter(this);
    else return true;
}

bool pawsWidget::OnChildMouseEnter(pawsWidget* /*widget*/)
{
    if(parent)
        return parent->OnChildMouseEnter(this);
    return true;
}

bool pawsWidget::OnMouseExit()
{
    if(parent)
        return parent->OnChildMouseExit(this);
    else return true;
}

bool pawsWidget::OnChildMouseExit(pawsWidget* /*child*/)
{
    if(parent)
        return parent->OnChildMouseExit(this);
    return true;
}


bool pawsWidget::OnMouseDown(int button, int modifiers, int x, int y)
{
#ifdef PAWS_CONSTRUCTION
    if(modifiers == 1)
    {
        SetMovable(true);
        isResizable = true;
        showResize = true;
    }
    if(modifiers == 2)
    {
        printf("-----------WIDGET %s DUMP---------\n", name.GetData());
        printf("Position: (%d,%d)\n", GetScreenFrame().xmin-parent->GetScreenFrame().xmin, ScreenFrame().ymin-parent->GetScreenFrame().ymin);
        printf("Size    : (%d,%d)\n", GetScreenFrame().Width(), ScreenFrame().Height());
    }
    if(modifiers == 3)
    {
        ReloadWidget();
        return true;
    }
#endif

    if(!PawsManager::GetSingleton().GetDragDropWidget())
    {
#ifdef CS_PLATFORM_MACOSX
        if(button == csmbLeft && !(modifiers & CSMASK_CTRL))
#else
        if(button == csmbLeft)
#endif
        {
            if(isResizable && showResize)
            {
                // Check to see if the mouse was in the resizing flags
                int flags = ResizeFlags(x,y) ;

                if(flags)
                {
                    PawsManager::GetSingleton().ResizingWidget(this, flags);
                    return true;
                }
            }


            if(movable)
            {
                PawsManager::GetSingleton().MovingWidget(this);
                return true;
            }
        }
#ifdef CS_PLATFORM_MACOSX
        else if((button == csmbRight) || (button == csmbLeft && modifiers == CSMASK_CTRL))
#else
        else if((button == csmbRight))
#endif
        {
            if(!contextMenu && !contextMenuFile.IsEmpty())
            {
                CreateContextMenu();
                return true;
            }
            else if(contextMenuFile.IsEmpty() && configurable)
            {
                CreateWidgetConfigWindow();
                return true;
            }
        }
    }

    if(parent && PawsManager::GetSingleton().GetModalWidget() != this)
        return parent->OnMouseDown(button, modifiers, x, y);

    return false;
}

bool pawsWidget::OnSelected(pawsWidget* /*widget*/)
{
    return false;
}

bool pawsWidget::OnMenuAction(pawsWidget* widget, const pawsMenuAction &action)
{
    if(action.name == MENU_DESTROY_ACTION_NAME)
    {
        PawsManager::GetSingleton().GetMainWidget()->DeleteChild(widget);
        if(widget == contextMenu)
            contextMenu = NULL;
        return true;
    }

    if(action.name=="configure")
        CreateWidgetConfigWindow();

    return false;
}

csRect pawsWidget::GetScreenFrame()
{
    return screenFrame;
}


bool pawsWidget::OnMouseUp(int button, int modifiers, int x, int y)
{
    if(parent && PawsManager::GetSingleton().GetModalWidget() != this)
        return parent->OnMouseUp(button, modifiers, x, y);
    else
    {
        return false;
    }
}

bool pawsWidget::OnDoubleClick(int button, int modifiers, int x, int y)
{
    if(parent && PawsManager::GetSingleton().GetModalWidget() != this)
    {
        return parent->OnDoubleClick(button, modifiers, x, y);
    }
    else
    {
        return false;
    }
}

bool pawsWidget::OnJoypadDown(int button, int modifiers)
{
    if(parent && PawsManager::GetSingleton().GetModalWidget() != this)
    {
        return parent->OnJoypadDown(button, modifiers);
    }
    else
    {
        return false;
    }
}

bool pawsWidget::OnKeyDown(utf32_char keyCode, utf32_char key, int modifiers)
{
    if(key == CSKEY_ENTER)
    {
        if(!onEnter)
        {
            if(parent && PawsManager::GetSingleton().GetModalWidget() != this)
                return parent->OnKeyDown(keyCode,key,modifiers);
            //else we ignore the command and proceed
        }
        else
        {
            onEnter->OnMouseDown(1,0,0,0);
        }
        return true;

    }

    bool tab = false;
    bool forward = true;

    if(keyCode==CSKEY_TAB && modifiers == 1)
    {
        tab = true;
        forward = false;
    }
    else if(keyCode==CSKEY_TAB)
    {
        tab = true;
        forward = true;
    }

    if(tab && taborder.GetSize() > 0)
    {
        int z;

        for(z = 0; z < (int)taborder.GetSize(); z++)
        {
            if(taborder[z]->HasFocus())
            {
                break;
            }
        }

        while(true)
        {
            // take proper action
            if(forward)
                if(z + 1 < (int)taborder.GetSize())
                    z++;
                else
                    z = 0;
            else if(z - 1 >= 0)
                z--;
            else
                z = (int)taborder.GetSize() - 1;

            // if this is a vaild widget to focus on, break the loop
            if(taborder[z]->OnGainFocus(false))
                break;
        }

        // Set current focued widget
        PawsManager::GetSingleton().SetCurrentFocusedWidget(taborder[z]);
    }

    if(keyCode < 255 && keyCode > 31)
    {
        for(size_t i=0; i<children.GetSize(); i++)
        {
            bool testKey = children[i]->CheckKeyHandled(toupper(keyCode));
            if(testKey)
                return true;
        }
    }
    if(parent && PawsManager::GetSingleton().GetModalWidget() != this)
        return parent->OnKeyDown(keyCode, key, modifiers);

    return false;
}

bool pawsWidget::OnClipboard(const csString &content)
{
    if(parent)
    {
        return parent->OnClipboard(content);
    }
    else
    {
        return false;
    }
}



void pawsWidget::MoveDelta(int deltaX, int deltaY)
{
    //snap to edge of screen
    if(movable)  //avoid moving internal elements
    {
        const int SNAP_RANGE = 3;
        int screenWidth = graphics2D->GetWidth();
        int screenHeight = graphics2D->GetHeight();

        int titleHeight;
        if(border && border->GetTitle()) titleHeight = 20;
        else titleHeight = 0;

        if(abs(screenFrame.xmin + deltaX) < SNAP_RANGE)
            deltaX = 0 - screenFrame.xmin;
        else if(abs(screenFrame.xmax + deltaX - screenWidth) < SNAP_RANGE)
            deltaX = screenWidth - screenFrame.xmax;
        if(abs(screenFrame.ymin + deltaY - titleHeight) < SNAP_RANGE)
            deltaY = 0 + titleHeight - screenFrame.ymin;
        else if(abs(screenFrame.ymax + deltaY - screenHeight) < SNAP_RANGE)
            deltaY = screenHeight - screenFrame.ymax;
    }

    screenFrame.xmin += deltaX;
    screenFrame.ymin += deltaY;
    screenFrame.xmax += deltaX;
    screenFrame.ymax += deltaY;
    if(titleBar)
        titleBar->SetWindowRect(screenFrame);

    for(size_t x = children.GetSize(); x-- > 0;)
    {
        children[x]->MoveDelta(deltaX, deltaY);
    }
}


void pawsWidget::MoveTo(int x, int y)
{
    int deltaX = x - screenFrame.xmin;
    int deltaY = y - screenFrame.ymin;

    int width = screenFrame.Width();
    int height = screenFrame.Height();

    screenFrame.xmin = x;
    screenFrame.ymin = y;
    screenFrame.SetSize(width, height);
    if(titleBar)
        titleBar->SetWindowRect(screenFrame);

    for(size_t z = children.GetSize(); z-- > 0;)
    {
        children[z]->MoveDelta(deltaX, deltaY);
    }

    for(size_t z = 0; z < children.GetSize(); z++)
    {
        children[z]->RecalcScreenPositions();
    }
}

void pawsWidget::CenterTo(int x, int y)
{
    MoveTo(x - screenFrame.Width()/2, y - screenFrame.Height()/2);
}

void pawsWidget::CenterToMouse()
{
    psPoint mouse;
    mouse = PawsManager::GetSingleton().GetMouse()->GetPosition();
    CenterTo(mouse.x, mouse.y);
    MakeFullyVisible();
}

csRect pawsWidget::LoadPositions()
{
    csRef<iConfigManager> cfgMgr;
    cfgMgr =  csQueryRegistry<iConfigManager > (PawsManager::GetSingleton().GetObjectRegistry());

    csString configName;

    configName.Format("PlaneShift.GUI.%s.Visible", GetName());
    SetVisibility(cfgMgr->GetBool(configName, visible));

    configName.Format("PlaneShift.GUI.%s.PosX", GetName());
    int winPosX = cfgMgr->GetInt(configName, defaultFrame.xmin);

    configName.Format("PlaneShift.GUI.%s.PosY", GetName());
    int winPosY = cfgMgr->GetInt(configName, defaultFrame.ymin);

    int screenWidth  = graphics2D->GetWidth();
    int screenHeight = graphics2D->GetHeight();

    int winWidth = defaultFrame.Width();
    int winHeight = defaultFrame.Height();

    //don't do this if widget is not resizable
    if(IsResizable())
    {
        configName.Format("PlaneShift.GUI.%s.Width", GetName());
        winWidth = cfgMgr->GetInt(configName, defaultFrame.Width());

        configName.Format("PlaneShift.GUI.%s.Height", GetName());
        winHeight = cfgMgr->GetInt(configName, defaultFrame.Height());
        if(resizeToScreen)
        {
            float ratio = ((float) winHeight) / ((float) winWidth);
            if(winWidth > 800)
            {
                winWidth = 800;
                if(ratio > 1.0f)
                    ratio = 1.0f;
                winHeight = int (800.0f * ratio);
            }
            else if(winHeight > 600)
            {
                winHeight = 600;
                if(ratio > 1.0f)
                    ratio = 1.0f;
                winWidth = int (600.0f / ratio);
            }
            winWidth = GetActualWidth(winWidth);
            winHeight = GetActualHeight(winHeight);
        }

        if(winWidth < min_width) winWidth = min_width;
        if(winHeight < min_height) winHeight = min_height;
    }

    if(winPosX < 0) winPosX = 0;
    if(winPosY < 0) winPosY = 0;

    if(winPosX + winWidth  > screenWidth) winPosX = screenWidth  - winWidth;
    if(winPosY + winHeight > screenHeight) winPosY = screenHeight - winHeight;

    csRect newPosition = csRect(winPosX,winPosY, winPosX+winWidth, winPosY+winHeight);
    return newPosition;
}

void pawsWidget::PerformAction(const char* action)
{
    csString temp(action);
    if(temp == "togglevisibility")
    {
        if(IsVisible())
        {
            Hide();
        }
        else
        {
            Show();
        }
        return;
    }
}

void pawsWidget::SavePosition()
{
    csRef<iConfigManager> cfgMgr;
    cfgMgr =  csQueryRegistry<iConfigManager > (PawsManager::GetSingleton().GetObjectRegistry());

    int desktopWidth  = graphics2D->GetWidth();
    int desktopHeight = graphics2D->GetHeight();

    float ratioX = (float) 800.0f / (float)desktopWidth;
    float ratioY = (float) 600.0f / (float)desktopHeight;

    if(!resizeToScreen)
        ratioX = ratioY = 1;

    csString configName;

    configName.Format("PlaneShift.GUI.%s.Visible", GetName());
    cfgMgr->SetBool(configName, visible);

    configName.Format("PlaneShift.GUI.%s.PosX", GetName());
    cfgMgr->SetInt(configName, screenFrame.xmin);

    configName.Format("PlaneShift.GUI.%s.PosY", GetName());
    cfgMgr->SetInt(configName, screenFrame.ymin);

    if(IsResizable())
    {
        configName.Format("PlaneShift.GUI.%s.Width", GetName());
        cfgMgr->SetInt(configName, RX(screenFrame.Width()));

        configName.Format("PlaneShift.GUI.%s.Height", GetName());
        cfgMgr->SetInt(configName, RY(screenFrame.Height()));
    }

    cfgMgr->Save();
}

void pawsWidget::SetForceSize(int newWidth, int newHeight)
{
    screenFrame.SetSize(newWidth, newHeight);
    OnResize();
}

void pawsWidget::SetSize(int newWidth, int newHeight)
{
    SetForceSize(newWidth,newHeight);

    for(size_t x = children.GetSize(); x-- > 0;)
    {
        children[x]->Resize();
    }
}

void pawsWidget::StopResize()
{
    for(size_t x = 0; x < children.GetSize(); x++)
    {
        children[x]->StopResize();
    }
}

void pawsWidget::Resize(int flags)
{
    psPoint mousePos = PawsManager::GetSingleton().GetMouse()->GetPosition();
    csRect newFrame = screenFrame;

    float scale = 1.0f;
    if(defaultFrame.Height() && defaultFrame.Width())
    {
        scale = defaultFrame.Width() / defaultFrame.Height();
    }

    if(keepaspect && flags & RESIZE_RIGHT && flags & RESIZE_BOTTOM)
    {
        int deltaX = (mousePos.x - screenFrame.xmin);
        int deltaY = (mousePos.y - screenFrame.ymin);
        if(deltaX < (deltaY * scale))
        {
            newFrame.ymax = mousePos.y;
            newFrame.xmax = screenFrame.xmin + (int)(deltaY * scale);
        }
        else
        {
            newFrame.xmax = mousePos.x;
            newFrame.ymax = screenFrame.ymin + (int)(deltaX / scale);
        }
    }
    else
    {
        if(flags & RESIZE_RIGHT)
        {
            newFrame.xmax = mousePos.x;
        }
        if(flags & RESIZE_BOTTOM)
        {
            newFrame.ymax = mousePos.y;
        }
    }

    if(flags & RESIZE_LEFT)
    {
        newFrame.xmin = mousePos.x;
    }
    if(flags & RESIZE_TOP)
    {
        newFrame.ymin = mousePos.y;
    }

    /// prevent the widget from going below its min height and width
    if(newFrame.xmax - newFrame.xmin < min_width) newFrame.xmax = newFrame.xmin + min_width;
    if(newFrame.ymax - newFrame.ymin < min_height) newFrame.ymax = newFrame.ymin + min_height;

    /// prevent the widget from going beyond its max height and width
    if(newFrame.xmax - newFrame.xmin > max_width) newFrame.xmax = max_width + newFrame.xmin;
    if(newFrame.ymax - newFrame.ymin > max_height) newFrame.ymax = max_height + newFrame.ymin;

    screenFrame = newFrame;

    OnResize();

    for(size_t x = children.GetSize(); x-- > 0;)
    {
        children[x]->Resize();
    }
}

void pawsWidget::Resize(int deltaX, int deltaY, int flags)
{
    csRect newFrame = screenFrame;

    float scale = 0.0f;
    if(defaultFrame.Height() && defaultFrame.Width())
    {
        scale = defaultFrame.Width() / defaultFrame.Height();
    }

    if(keepaspect && flags & RESIZE_RIGHT && flags & RESIZE_BOTTOM)
    {
        int delta = (deltaX<deltaY) ? deltaX:deltaY;
        newFrame.ymax += delta;
        newFrame.xmax += (int)(delta*scale);
    }
    else
    {
        if(flags & RESIZE_RIGHT)
        {
            newFrame.xmax += deltaX;
        }
        if(flags & RESIZE_BOTTOM)
        {
            newFrame.ymax += deltaY;
        }
    }

    if(flags & RESIZE_LEFT)
    {
        newFrame.xmin += deltaX;
    }
    if(flags & RESIZE_TOP)
    {
        newFrame.ymin += deltaY;
    }

    /// prevent the widget from going below its min height and width
    if(newFrame.xmax - newFrame.xmin < min_width) newFrame.xmax = newFrame.xmin + min_width;
    if(newFrame.ymax - newFrame.ymin < min_height) newFrame.ymax = newFrame.ymin + min_height;

    /// prevent the widget from going beyond its max size
    if(newFrame.xmax - newFrame.xmin > max_width) newFrame.xmax = max_width + newFrame.xmin;
    if(newFrame.ymax - newFrame.ymin > max_height) newFrame.ymax = max_height + newFrame.ymin;

    screenFrame = newFrame;

    OnResize();

    for(size_t x = children.GetSize(); x-- > 0;)
    {
        children[x]->Resize();
    }
}


void pawsWidget::Resize()
{
    if(!parent)
        return;

    if(attachFlags & ATTACH_LEFT)
    {
        int width = screenFrame.Width();

        screenFrame.xmin = parent->GetScreenFrame().xmin + defaultFrame.xmin;
        screenFrame.SetSize(width, screenFrame.Height());

        // Left-Right Attachment
        if(attachFlags & ATTACH_RIGHT)
        {
            int newX = parent->GetDefaultFrame().Width() - (defaultFrame.xmin + defaultFrame.Width());
            screenFrame.xmax = parent->GetScreenFrame().xmax - newX;
        }

        // Left-Right Proportional Attachment
        if(attachFlags & PROPORTIONAL_RIGHT)
        {
            int z = (parent->GetScreenFrame().Width() * (parent->GetDefaultFrame().Width() - defaultFrame.xmax)) /
                    parent->GetDefaultFrame().Width();

            int newMax = parent->GetScreenFrame().xmax - z;
            screenFrame.xmax = newMax;
        }
    }

    if(attachFlags & ATTACH_RIGHT)
    {
        //Attach Right Only
        if(!(attachFlags & ATTACH_LEFT))
        {
            int width = screenFrame.Width();
            screenFrame.xmin = parent->GetScreenFrame().xmax - (parent->GetDefaultFrame().Width() - defaultFrame.xmax + width);
            screenFrame.SetSize(width, screenFrame.Height());
        }

        //Attach Right-Left Proportional
        if(attachFlags & PROPORTIONAL_LEFT)
        {
            int z = (parent->GetScreenFrame().Width() *  defaultFrame.xmin) /
                    parent->GetDefaultFrame().Width();

            int newMin = parent->GetScreenFrame().xmin + z;
            screenFrame.xmin = newMin;
        }
    }

    if(attachFlags & PROPORTIONAL_RIGHT)
    {
        //Attach Proportional RIGHT Only
        if(!(attachFlags & ATTACH_LEFT) && !(attachFlags & PROPORTIONAL_LEFT))
        {

            int z = (parent->GetScreenFrame().Width() * (parent->GetDefaultFrame().Width() - defaultFrame.xmax)) /
                    parent->GetDefaultFrame().Width();

            int newMin = parent->GetScreenFrame().xmax - z - defaultFrame.Width();
            int width = screenFrame.Width();
            screenFrame.xmin = newMin;
            screenFrame.SetSize(width, screenFrame.Height());
        }
        //Attach PropLeft-PropRight
        if(attachFlags & PROPORTIONAL_LEFT)
        {
            int z = (parent->GetScreenFrame().Width() * (parent->GetDefaultFrame().Width() - defaultFrame.xmax)) /
                    parent->GetDefaultFrame().Width();

            int newMax = parent->GetScreenFrame().xmax - z;
            screenFrame.xmax = newMax;

            z = (parent->GetScreenFrame().Width() *  defaultFrame.xmin) /
                parent->GetDefaultFrame().Width();

            int newMin = parent->GetScreenFrame().xmin + z;
            screenFrame.xmin = newMin;
        }
    }

    if(attachFlags & PROPORTIONAL_LEFT)
    {
        //Attach Proportional Left Only
        if(!(attachFlags & ATTACH_RIGHT) && !(attachFlags & PROPORTIONAL_RIGHT))
        {
            int z = (parent->GetScreenFrame().Width() * defaultFrame.xmin) /
                    parent->GetDefaultFrame().Width();

            int newMin = parent->GetScreenFrame().xmin + z;
            int width = screenFrame.Width();
            screenFrame.xmin = newMin;
            screenFrame.SetSize(width, screenFrame.Height());
        }
    }
    //printf("Before %i\n", screenFrame.Height());

    if(attachFlags & ATTACH_TOP)
    {
        int height = defaultFrame.Height();
        //printf("Height: %i\n", height);

        screenFrame.ymin = parent->GetScreenFrame().ymin + defaultFrame.ymin;
        screenFrame.SetSize(screenFrame.Width(), height);

        //Attach TOP-BOTTOM
        if(attachFlags & ATTACH_BOTTOM)
        {
            int newY = parent->GetDefaultFrame().Height() - (defaultFrame.ymin + defaultFrame.Height());
            screenFrame.ymax = parent->GetScreenFrame().ymax - newY;
        }

        //Attach TOP-BOTTOM Proportional
        if(attachFlags & PROPORTIONAL_BOTTOM)
        {
            int z = (parent->GetScreenFrame().Height() * (parent->GetDefaultFrame().Height() - defaultFrame.ymax)) /
                    parent->GetDefaultFrame().Height();

            int newMax = parent->GetScreenFrame().ymax - z;
            screenFrame.ymax = newMax;
        }
    }
    //printf("After %i\n", screenFrame.Height());
    if(attachFlags & ATTACH_BOTTOM)
    {
        // Attach BOTTOM Only
        if(!(attachFlags & ATTACH_TOP))
        {

            int height = screenFrame.Height();
            screenFrame.ymin = parent->GetScreenFrame().ymax - (parent->GetDefaultFrame().Height() - defaultFrame.ymax + height);
            screenFrame.SetSize(screenFrame.Width(), height);
        }

        //Attach BOTTOM-TOP Proportional
        if(attachFlags & PROPORTIONAL_TOP)
        {
            int z = (parent->GetScreenFrame().Height() *  defaultFrame.ymin) /
                    parent->GetDefaultFrame().Height();

            int newMin = parent->GetScreenFrame().ymin + z;
            screenFrame.ymin = newMin;
        }
    }

    // Attach Proportional TOP Only
    if(attachFlags & PROPORTIONAL_TOP)
    {
        if(!(attachFlags & ATTACH_BOTTOM) && !(attachFlags & PROPORTIONAL_BOTTOM))
        {
            int z = (parent->GetScreenFrame().Height() * defaultFrame.ymin) /
                    parent->GetDefaultFrame().Height();

            int newMin = parent->GetScreenFrame().ymin + z;
            int height = screenFrame.Height();
            screenFrame.ymin = newMin;
            screenFrame.SetSize(screenFrame.Width(), height);
        }
    }

    if(attachFlags & PROPORTIONAL_BOTTOM)
    {
        // Attach Proportional BOTTOM Only
        if(!(attachFlags & ATTACH_TOP) && !(attachFlags & PROPORTIONAL_TOP))
        {
            int z = (parent->GetScreenFrame().Height() * (parent->GetDefaultFrame().Height() - defaultFrame.ymax)) /
                    parent->GetDefaultFrame().Height();

            int newMin = parent->GetScreenFrame().ymax - z - defaultFrame.Height();
            int height = screenFrame.Height();
            screenFrame.ymin = newMin;
            screenFrame.SetSize(screenFrame.Width(), height);
        }

        // Attach PropTOP-PropBOTTOM
        if(attachFlags & PROPORTIONAL_TOP)
        {
            int z = (parent->GetScreenFrame().Height() * (parent->GetDefaultFrame().Height() - defaultFrame.ymax)) /
                    parent->GetDefaultFrame().Height();

            int newMax = parent->GetScreenFrame().ymax - z;
            screenFrame.ymax = newMax;

            z = (parent->GetScreenFrame().Height() *  defaultFrame.ymin) /
                parent->GetDefaultFrame().Height();

            int newMin = parent->GetScreenFrame().ymin + z;
            screenFrame.ymin = newMin;
        }
    }

    OnResize();
    for(size_t x = 0; x < children.GetSize(); x++)
    {
        children[x]->Resize();
    }

    /// Scale the font to match the change in size.
    if(scaleFont && !resizeToScreen)
    {
        if(screenFrame.Width() && defaultFrame.Height())
        {
            SetFontScaling(true);
        }
    }
}

void pawsWidget::OnResize()
{
    SetCloseButtonPos();
    if(titleBar)
        titleBar->SetWindowRect(screenFrame);
}

void pawsWidget::SetModalState(bool isModal)
{
    if(isModal)
        PawsManager::GetSingleton().SetModalWidget(this);
    else
        PawsManager::GetSingleton().SetModalWidget(0);
}



int pawsWidget::GetAttachFlag(const char* flag)
{
    csString str(flag);

    if(str == "ATTACH_RIGHT") return ATTACH_RIGHT;
    if(str == "ATTACH_LEFT")  return ATTACH_LEFT;
    if(str == "ATTACH_BOTTOM") return ATTACH_BOTTOM;
    if(str == "ATTACH_TOP") return ATTACH_TOP;

    if(str == "PROPORTIONAL_LEFT") return PROPORTIONAL_LEFT;
    if(str == "PROPORTIONAL_RIGHT") return PROPORTIONAL_RIGHT;
    if(str == "PROPORTIONAL_TOP") return  PROPORTIONAL_TOP;
    if(str == "PROPORTIONAL_BOTTOM") return  PROPORTIONAL_BOTTOM;

    return 0;
}

void pawsWidget::MouseOver(bool value)
{
    hasMouseFocus = value;
}

//csRef<iDocument> ParseFile(iObjectRegistry* object_reg, const csString & name);

bool pawsWidget::LoadFromFile(const csString &fileName)
{
    csRef<iDocument> doc;
    csRef<iDocumentNode> root, topNode, widgetNode;

    doc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(),
                    PawsManager::GetSingleton().GetLocalization()->FindLocalizedFile(fileName));
    if(doc == NULL)
        return false;

    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("No root in XML");
        return false;
    }
    topNode = root->GetNode("widget_description");
    if(topNode == NULL)
    {
        Error1("No <widget_description> in XML");
        return false;
    }
    widgetNode = topNode->GetNode("widget");
    if(widgetNode == NULL)
    {
        Error1("No <widget> in <widget_description>");
        return false;
    }

    if(! Load(widgetNode))
        return false;

    return true;
}

void pawsWidget::MakeFullyVisible()
{
    csRect parentFrame;
    int targetX, targetY;

    if(parent == NULL)
        return;

    parentFrame = parent->GetScreenFrame();

    if(screenFrame.xmin < parentFrame.xmin)
        targetX = parentFrame.xmin;
    else if(screenFrame.xmax > parentFrame.xmax)
        targetX = parentFrame.xmax - screenFrame.Width() + 1;
    else
        targetX = screenFrame.xmin;
    if(screenFrame.ymin < parentFrame.ymin)
        targetY = parentFrame.ymin;
    else if(screenFrame.ymax > parentFrame.ymax)
        targetY = parentFrame.ymax - screenFrame.Height() + 1;
    else
        targetY = screenFrame.ymin;

    if((targetX != screenFrame.xmin) || (targetY != screenFrame.ymin))
        MoveTo(targetX, targetY);
}

void pawsWidget::ClipToParent(bool allowForBackgroundBorder)
{
    if(!parent)
    {
        // If we have no parent widget, then we are top level and clip only to the 2D canvas region
        graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
        clipRect = csRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
    }
    else
    {
        // Otherwise we should clip to the overlap of our parent frame and either our frame or our border frame

        // If we have a border, start with that, otherwise use our widget frame
        if(titleBar)
        {
            clipRect = screenFrame;
            clipRect.ymin = titleBar->screenFrame.ymin;
        }
        else if(border && border->GetTitleImage())
            clipRect = border->GetRect();
        else
            clipRect = screenFrame;

        if(allowForBackgroundBorder && bgImage)
        {
            bgImage->ExpandClipRect(clipRect);
        }

        // Calculate intersection of our frame and parent frame
        clipRect.Intersect(parent->ClipRect());

        // If we wont be drawing anyway, don't bother making a graphics call to adjust clipping
        if(clipRect.IsEmpty())
            return;

        // Adjust the canvas clipping for future draw calls to clip to our allowed area
        graphics2D->SetClipRect(ClipRect().xmin, ClipRect().ymin,
                                ClipRect().xmax, ClipRect().ymax);
    }

}

void pawsWidget::LoadBorderColours(iDocumentNode* node)
{
    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    int index = 0;
    while(index < 5 && iter->HasNext())
    {
        csRef<iDocumentNode> colour = iter->Next();

        int r = colour->GetAttributeValueAsInt("r");
        int g = colour->GetAttributeValueAsInt("g");
        int b = colour->GetAttributeValueAsInt("b");

        int col = graphics2D->FindRGB(r, g, b);
        borderColours[index++] = col;
    }
    hasBorderColours = true;
}

int pawsWidget::GetBorderColour(int which)
{
    if(hasBorderColours)
        return borderColours[which];
    else if(parent)
        return parent->GetBorderColour(which);
    else
        return PawsManager::GetSingleton().GetPrefs()->GetBorderColour(which);
}

void pawsWidget::SetFont(const char* Name , int Size)
{
    if(Name)
    {
        defaultFontSize = Size;
        fontSize = defaultFontSize;

        if(resizeToScreen)
            fontSize *= PawsManager::GetSingleton().GetFontFactor();

        fontName = Name;
        myFont = graphics2D->GetFontServer()->LoadFont(Name, fontSize);
    }
    else
    {
        myFont = NULL;
    }
}

void pawsWidget::SetColour(int newColour)
{
    //-2 means we get the default color from the settings,
    //else we set the not set color (so that we get the value from the parents, like what
    //happens when nothing is defined or requested)
    if(newColour != -2)
    {
        defaultFontColour = newColour;
    }
    else
    {
        defaultFontColour = PawsManager::GetSingleton().GetPrefs()->GetDefaultFontColour();
    }
}

void pawsWidget::ChangeFontSize(float newSize)
{
    if(newSize == fontSize)
        return;

    fontSize = newSize;

    if(resizeToScreen)
        fontSize *= PawsManager::GetSingleton().GetFontFactor();

    if(fontName.IsEmpty())
        fontName = PawsManager::GetSingleton().GetPrefs()->GetDefaultFontName();

    myFont = graphics2D->GetFontServer()->LoadFont(fontName, fontSize);
}

iFont* pawsWidget::GetFont(bool scaled)
{
    csRef<iFont>  font;

    if(myFont)
        font = myFont;
    else if(parent)
        font = parent->GetFont(scaled);
    else
        font = PawsManager::GetSingleton().GetPrefs()->GetDefaultFont(scaled);

    return font;
}

int pawsWidget::GetFontColour()
{
    if(defaultFontColour != -1)
        return defaultFontColour;
    else if(parent)
        return parent->GetFontColour();
    else
        return PawsManager::GetSingleton().GetPrefs()->GetDefaultFontColour();
}

int pawsWidget::GetFontShadowColour()
{
    if(myFont)
        return defaultFontShadowColour;
    else if(parent)
        return parent->GetFontShadowColour();
    else
        return graphics2D->FindRGB(0,0,0);
}

float pawsWidget::GetFontSize()
{
    if(myFont)
        return fontSize;
    else if(parent)
        return parent->GetFontSize();
    else
        return DEFAULT_FONT_SIZE;
}

int pawsWidget::GetFontStyle()
{
    if(fontStyle)
        return fontStyle;
    else if(parent)
        return parent->GetFontStyle();
    else
        return DEFAULT_FONT_STYLE;
}

void pawsWidget::SetFontStyle(int style)
{
    fontStyle = style;
}

bool pawsWidget::SelfPopulateXML(const char* xmlstr)
{
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);

    csRef<iDocument> doc= xml->CreateDocument();
    const char* error = doc->Parse(xmlstr);
    if(error)
    {
        Error2("%s\n", error);
        return false;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("Missing root in %s", xmlstr);
        return false;
    }
    csRef<iDocumentNode> topNode = root->GetNode(xmlbinding!=""?xmlbinding:name);
    if(topNode)
    {
        return SelfPopulate(topNode);
    }
    else
    {
        Error3("SelfPopulate failed because tag <%s> could not be found in xmlstr '%s'.\n",name.GetData(),xmlstr);
        return false;
    }
}

bool pawsWidget::SelfPopulate(iDocumentNode* node)
{
    // No self population here since generic widgets have no data

    // Now generically check to populate children if any are here
    if(!children.GetSize())
        return true;

    csRef<iDocumentNodeIterator> iter = node->GetNodes();

    while(iter->HasNext())
    {
        csRef<iDocumentNode> node = iter->Next();

        if(node->GetType() != CS_NODE_ELEMENT)
            continue;

        pawsWidget* nodewidget = FindWidgetXMLBinding(node->GetValue());
        if(nodewidget)
        {
            if(!nodewidget->SelfPopulate(node))
                return false;
        }
        else
            Warning2(LOG_PAWS, "Could not locate widget with XML binding %s", node->GetValue());
    }
    return true;
}

bool pawsWidget::OnChange(pawsWidget* widget)
{
    if(parent != NULL)
        return parent->OnChange(widget);
    else
        return false;
}

void pawsWidget::Dump(csString tab)
{
    printf("%s<widget name=\"%s\">\n",tab.GetData(),name.GetData());
    for(size_t x = 0; x < children.GetSize(); x++)
    {
        children[x]->Dump(tab+"\t");
    }
    printf("%s</widget>\n",tab.GetData());
}

//------------------------------------------------------------------------

void pawsWidgetFactory::Register(const char* name)
{
    factoryName = name;

    PawsManager::GetSingleton().RegisterWidgetFactory(this);
}

pawsWidget* pawsWidgetFactory::Create()
{
    return NULL;
}

pawsWidget* pawsWidgetFactory::Create(const pawsWidget* origin)
{
    return new pawsWidget(*origin);
}

void pawsWidget::SetCloseButtonPos()
{
    pawsButton* button = dynamic_cast <pawsButton*>(FindWidget(GetCloseName(), false));

    if(button != NULL)
        button->SetRelativeFrame(defaultFrame.Width() - 20, - 18, 16, 16);
}

void pawsWidget::RemoveTitle()
{
    if(border)
    {
        border->SetTitle(NULL);
        border->SetTitleImage(NULL);
    }

    if(close_widget)
        close_widget->DeleteYourself();
}

bool pawsWidget::SetTitle(const char* text, const char* image, const char* align, const char* close_button, const bool shadowTitle)
{
    if(!border)
    {
        return false;
    }
    else
    {
        borderTitleShadow = shadowTitle;
        border->SetTitle(text, borderTitleShadow);
        csRef<iPawsImage> titleImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(image);
        border->SetTitleImage(titleImage);

        int alignValue = ALIGN_LEFT;
        if(align)
        {
            if(!strcmp(align,"center"))
                alignValue = ALIGN_CENTER;
            else if(!strcmp(align,"right"))
                alignValue = ALIGN_RIGHT;
        }
        border->SetTitleAlign(alignValue);

        if(close_button && (strcmp(close_button,"yes") == 0))
        {
            close_widget = new pawsButton;
            close_widget->SetParent(this);
            close_widget->SetBackground("quit");
            csString name(GetName());
            name.Append("_close");
            close_widget->SetName(name);

            int flags = ATTACH_TOP | ATTACH_RIGHT;
            close_widget->SetAttachFlags(flags);
            AddChild(close_widget);
            SetCloseButtonPos();
            close_widget->SetSound("gui.cancel");
        }
    }

    return true;
}

csString pawsWidget::GetPathInWidgetTree()
{
    csString path, widget;

    widget =  "n=";
    widget += name;
    widget +=" f=";
    widget += factory;

    if(parent != NULL)
    {
        csString path;
        path =  parent->GetPathInWidgetTree();
        path += " ---> ";
        path += widget;
        return path;
    }
    else
        return widget;
}

void pawsWidget::SetMaskingImage(const char* image)
{
    if(image == NULL || *image == '\0')
    {
        maskImage = 0;
        return;
    }
    maskImage = PawsManager::GetSingleton().GetTextureManager()->GetOrAddPawsImage(image);

    if(!maskImage)
    {
        Warning3(LOG_PAWS, "Could not locate masking image %s for widget %s",image,GetName());
    }
}

void pawsWidget::ClearMaskingImage()
{
    maskImage = 0;
}


void pawsWidget::SetBackgroundColor(int r,int g, int b)
{
    bgColour = graphics2D->FindRGB(r,g,b);
}

void pawsWidget::RunScriptEvent(PAWS_WIDGET_SCRIPT_EVENTS event)
{
    pawsScript* script = scriptEvents[(size_t)event];
    if(script)
        script->Execute();
}

double pawsWidget::GetProperty(MathEnvironment* /*env*/, const char* ptr)
{
    if(!strcasecmp(ptr, "visible"))
        return visible ? 1.0 : 0.0;

    Error2("pawsWidget::GetProperty(%s) failed\n", ptr);
    return 0.0;
}

double pawsWidget::CalcFunction(MathEnvironment* env, const char* functionName, const double* params)
{
    if(!strcasecmp(functionName, "closeparent"))
    {
        pawsWidget* w = this;
        for(size_t a=0; a<(size_t)(params[0] + 0.5f) && w->parent; ++a)
            w = w->parent;
        w->Close();
        return 0.0;
    }
    else if(!strcasecmp(functionName, "Show"))
    {
        Show();
        return 0.0;
    }
    else if(!strcasecmp(functionName, "Hide"))
    {
        Hide();
        return 0.0;
    }
    /*else if(parent)
    {
        return parent->CalcFunction(env, functionName, params);
    }*/

    Error2("pawsWidget::CalcFunction(%s) failed\n", functionName);
    return 0.0;
}

void pawsWidget::SetProperty(const char* ptr, double value)
{
    if(!strcasecmp(ptr, "visible"))
    {
        if(fabs(value) < 0.001)
            Hide();
        else
            Show();
    }
    else
        Error2("pawsWidget::SetProperty(%s) failed\n", ptr);
}

bool pawsWidget::ReadDefaultWidgetStyles(iDocumentNode* node)
{
    csRef<iDocumentNodeIterator> defaults = node->GetNodes();
    while(defaults->HasNext())
    {
        csRef<iDocumentNode> child = defaults->Next();
        if(!strcmp(child->GetValue(),"defaultstyle"))
        {
            //printf("Adding style %s for widget %s.\n",child->GetAttributeValue("style"),child->GetAttributeValue("widget"));
            defaultWidgetStyles.Put(child->GetAttributeValue("widget"),child->GetAttributeValue("style"));
        }
    }
    return true;
}

const char* pawsWidget::FindDefaultWidgetStyle(const char* factoryName)
{
    static csString style;

    style = defaultWidgetStyles.Get(factoryName,csString("not found"));
    if(style == "not found" && parent != NULL)
    {
        // walk up the chain of parents
        return parent->FindDefaultWidgetStyle(factoryName);
    }
    if(style == "not found")
        return NULL;
    else
    {
        return style;
    }
}

