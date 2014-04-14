/*
* pawstitle.cpp
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
#include <ivideo/fontserv.h>

#include "pawstitle.h"

pawsTitle::pawsTitle(pawsWidget* parent, iDocumentNode* node)
    : titleAlign(PTA_CENTER), scaleWidth(true), width(1.0f), height(16), text(""), textAlign(PTA_CENTER)
{
    parent->AddChild(this);
    Load(node);
}

pawsTitle::~pawsTitle()
{
}

pawsTitle::PAWS_TITLE_ALIGN pawsTitle::GetAlign(const char* alignText)
{
    if(!strcasecmp(alignText, "left"))
        return PTA_LEFT;
    if(!strcasecmp(alignText, "right"))
        return PTA_RIGHT;
    return PTA_CENTER;
}

bool pawsTitle::Setup(iDocumentNode* node)
{
    csRef<iDocumentAttribute> attr;
    csRef<iGraphics2D> graphics2D = PawsManager::GetSingleton().GetGraphics2D();

    attr = node->GetAttribute("textalign");
    if(attr)
        textAlign = GetAlign(attr->GetValue());

    attr = node->GetAttribute("text");
    if(attr)
        text = attr->GetValue();

    attr = node->GetAttribute("textoffsetx");
    if(attr)
        textOffsetx = attr->GetValueAsInt();

    attr = node->GetAttribute("textoffsety");
    if(attr)
        textOffsety = attr->GetValueAsInt();

    attr = node->GetAttribute("scalewidth");
    if(attr)
        scaleWidth = attr->GetValueAsBool();

    attr = node->GetAttribute("width");
    if(attr)
        width = attr->GetValueAsFloat();

    attr = node->GetAttribute("height");
    if(attr)
        height = attr->GetValueAsInt();

    csRef<iDocumentNodeIterator> titleButtonNodes = node->GetNodes("titlebutton");
    while(titleButtonNodes->HasNext())
    {
        csRef<iDocumentNode> titleButtonNode = titleButtonNodes->Next();
        pawsTitleButton titleButton;

        strcpy(titleButton.widgetName, titleButtonNode->GetAttributeValue("name"));

        titleButton.align = PTA_RIGHT;
        attr = titleButtonNode->GetAttribute("align");
        if(attr)
            titleButton.align = GetAlign(attr->GetValue());

        titleButton.offsetx = titleButtonNode->GetAttributeValueAsInt("offsetx");
        titleButton.offsety = titleButtonNode->GetAttributeValueAsInt("offsety");

        titleButtons.Push(titleButton);
    }

    return true;
}

bool pawsTitle::PostSetup()
{
    size_t a;
    for(a=0; a<titleButtons.GetSize(); ++a)
        titleButtons[a].buttonWidget = FindWidget(titleButtons[a].widgetName);
    return true;
}
pawsTitle::pawsTitle(const pawsTitle &origin) : pawsWidget(origin)  //copy common attributes and children
{
    height = origin.height;
    scaleWidth = origin.scaleWidth;
    text = origin.text;
    textAlign = origin.textAlign;
    textOffsetx = origin.textOffsetx;
    textOffsety = origin.textOffsety;
    titleAlign = origin.titleAlign;
    width = origin.width;

    for(unsigned int i = 0 ; i < origin.titleButtons.GetSize() ; i++)
        titleButtons.Push(origin.titleButtons[i]);
    for(unsigned int i = 0 ; i < titleButtons.GetSize() ; i++)
        titleButtons[i].buttonWidget = new pawsWidget(*origin.titleButtons[i].buttonWidget);
}
void pawsTitle::SetWindowRect(const csRect &windowRect)
{
    size_t a;
    int winWidth = windowRect.Width();
    int w = (int)width;
    if(scaleWidth)
        w = (int)(winWidth * width);

    int deltaX = screenFrame.xmin;
    int deltaY = screenFrame.ymin;
    screenFrame.xmin = windowRect.xmin + (winWidth - w) / 2;
    screenFrame.ymin = windowRect.ymin - height / 2;
    screenFrame.SetSize(w, height);
    deltaX = screenFrame.xmin - deltaX;
    deltaY = screenFrame.ymin - deltaY;

    for(a=0; a<children.GetSize(); ++a)
        children[a]->Resize();

    for(a=0; a<titleButtons.GetSize(); ++a)
    {
        switch(titleButtons[a].align)
        {
            case PTA_LEFT:
                titleButtons[a].buttonWidget->MoveTo(screenFrame.xmin + titleButtons[a].offsetx, screenFrame.ymin + titleButtons[a].offsety);
                break;
            case PTA_CENTER:
                titleButtons[a].buttonWidget->MoveTo(screenFrame.xmin + (screenFrame.Width() - titleButtons[a].buttonWidget->GetScreenFrame().Width())/2 + titleButtons[a].offsetx, screenFrame.ymin + titleButtons[a].offsety);
                break;
            case PTA_RIGHT:
                titleButtons[a].buttonWidget->MoveTo(screenFrame.xmax - titleButtons[a].buttonWidget->GetScreenFrame().Width() + titleButtons[a].offsetx, screenFrame.ymin + titleButtons[a].offsety);
                break;
            case PTA_COUNT:
                break;
        }
    }
}

void pawsTitle::Draw()
{
    pawsWidget::Draw();
    graphics2D->SetClipRect(0, 0, graphics2D->GetWidth(), graphics2D->GetHeight());

    int w, h;
    GetFont()->GetDimensions(text, w, h);

    int textx = 0;
    int texty = screenFrame.ymin + (screenFrame.Height() - h)/2;
    switch(textAlign)
    {
        case PTA_LEFT:
            textx = screenFrame.xmin;
            break;
        case PTA_CENTER:
            textx = screenFrame.xmin + (screenFrame.Width() - w)/2;
            break;
        case PTA_RIGHT:
            textx = screenFrame.xmax - w;
            break;
        case PTA_COUNT:
            break;
    }
    textx += textOffsetx;
    texty += textOffsety;
    DrawWidgetText(text, textx, texty);
}
