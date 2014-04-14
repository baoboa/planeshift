/*
 * pawsmenu.cpp - Author: Ondrej Hurt
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


// CS INCLUDES
#include <psconfig.h>

// COMMON INCLUDES
#include "util/log.h"
#include "util/psxmlparser.h"
#include "util/localization.h"

// PAWS INCLUDES
#include "pawsmenu.h"
#include "pawsborder.h"
#include "pawsmanager.h"
#include "pawsmainwidget.h"


#define BORDER_SIZE   6
#define ITEM_SPACING  5
#define BUTTON_SIZE   16

#define SUBMENU_ACTION_NAME  "submenu"

#define SUBMENU_ARROW_NAME   "SubmenuArrow"
#define SUBMENU_ARROW_SIZE   16

int ParseColor(const csString &str, iGraphics2D* g2d);
static csRef<iDocumentNode> FindFirstWidget(iDocument* doc);


void DrawRect(iGraphics2D* graphics2D, int xmin, int ymin, int xmax, int ymax, int color)
{
    graphics2D->DrawLine(xmin, ymin, xmax, ymin, color);
    graphics2D->DrawLine(xmax, ymin, xmax, ymax, color);
    graphics2D->DrawLine(xmin, ymax, xmax, ymax, color);
    graphics2D->DrawLine(xmin, ymin, xmin, ymax, color);
}

static csRef<iDocumentNode> FindFirstWidget(iDocument* doc)
{
    csRef<iDocumentNode> root, topNode, widgetNode;

    root = doc->GetRoot();
    if(root == NULL)
    {
        Error1("No root in XML");
        return NULL;
    }
    topNode = root->GetNode("widget_description");
    if(topNode == NULL)
    {
        Error1("No <widget_description> in XML");
        return NULL;
    }
    widgetNode = topNode->GetNode("widget");
    if(widgetNode == NULL)
    {
        Error1("No <widget> in <widget_description>");
        return NULL;
    }
    return widgetNode;
}

//////////////////////////////////////////////////////////////////////
//                        class pawsMenuItem
//////////////////////////////////////////////////////////////////////

pawsMenuItem::pawsMenuItem()
{
    checkbox  = NULL;
    image     = NULL;
    label = new pawsTextBox();
    AddChild(label);

    imageEnabled    = false;
    checkboxEnabled = false;
    factory = "pawsMenuItem";

    graphics2d    = PawsManager::GetSingleton().GetGraphics2D();
}

pawsMenuItem::pawsMenuItem(const pawsMenuItem &origin):
    pawsIMenuItem(origin),
    imageEnabled(origin.imageEnabled),
    checkboxEnabled(origin.checkboxEnabled),
    spacing(origin.spacing),
    border(origin.border),
    graphics2d(origin.graphics2d)
{
    action.name = origin.action.name;
    for(unsigned int i = 0 ; i < origin.action.params.GetSize() ; i++)
        action.params[i] = origin.action.params[i];

    label = 0;
    checkbox = 0;
    image = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        if(origin.label == origin.children[i]) label = dynamic_cast<pawsTextBox*>(children[i]);
        else if(origin.checkbox == origin.children[i]) checkbox = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.image == origin.children[i]) image = children[i];

        if(label != 0 && checkbox !=0 && image != 0) break;
    }
}
bool pawsMenuItem::Load(iDocumentNode* node)
{
    return Setup(node);
}

bool pawsMenuItem::Setup(iDocumentNode* node)
{
    csRef<iDocumentAttribute> attr, onImg, offImg;
    int xmlLabelWidth, xmlSpacing, xmlBorder;
    csString on, off;


    attr = node->GetAttribute("image");
    if(attr != NULL)
    {
        EnableImage(true);
        SetImage(attr->GetValue());
    }

    attr = node->GetAttribute("label");
    if(attr != NULL)
    {
        label->SetText(PawsManager::GetSingleton().Translate(attr->GetValue()));
        label->SetSizeByText(5,5);
    }

    attr = node->GetAttribute("colour");
    if(attr != NULL)
        label->SetColour(ParseColor(attr->GetValue(), PawsManager::GetSingleton().GetGraphics2D()));
    else
        label->SetColour(0xffffff);

    attr = node->GetAttribute("checked");
    if(attr != NULL)
    {
        EnableCheckbox(true);
        SetCheckboxState((strcmp(attr->GetValue(), "false")? true:false));

        onImg = node->GetAttribute("CheckboxOn");
        if(onImg != NULL)
            on = onImg->GetValue();
        else
            on = "Up Arrow";

        offImg = node->GetAttribute("CheckboxOff");
        if(offImg != NULL)
            off = offImg->GetValue();
        else
            off = "Down Arrow";
        SetCheckboxImages(on, off);
    }

    attr = node->GetAttribute("LabelWidth");
    if(attr != NULL)
        xmlLabelWidth = attr->GetValueAsInt();
    else
        xmlLabelWidth = -1;
    attr = node->GetAttribute("spacing");
    if(attr != NULL)
        xmlSpacing = attr->GetValueAsInt();
    else
        xmlSpacing = 1;
    attr = node->GetAttribute("border");
    if(attr != NULL)
        xmlBorder = attr->GetValueAsInt();
    else
        xmlBorder = 1;
    SetSizes(xmlLabelWidth, xmlSpacing, xmlBorder);

    LoadAction(node);
    return true;
}

void pawsMenuItem::LoadAction(iDocumentNode* node)
{
    csRef<iDocumentAttribute> attr;
    int paramNum;
    csString paramName;
    pawsMenuAction newAction;

    newAction.name = node->GetAttributeValue("action");

    paramNum = 1;
    while(true)
    {
        paramName = "param ";
        paramName.SetAt(5, paramNum + '0');
        attr = node->GetAttribute(paramName);

        // LOOP EXIT:
        if(attr == NULL)
            break;

        newAction.params.SetSize(paramNum);
        newAction.params[paramNum-1] = attr->GetValue();

        paramNum++;
    }
    SetAction(newAction);
}

void pawsMenuItem::EnableCheckbox(bool enable)
{
    checkboxEnabled = enable;

    if(enable && (checkbox == NULL))
    {
        checkbox = new pawsButton();
        checkbox->SetRelativeFrameSize(BUTTON_SIZE, BUTTON_SIZE);
        checkbox->Show();
        AddChild(checkbox);
    }
    else if(!enable && (image != NULL))
    {
        checkbox = NULL;
        pawsWidget::DeleteChild(checkbox);
    }
    SetLayout();
}

void pawsMenuItem::EnableImage(bool enable)
{
    imageEnabled = enable;

    if(enable && (image == NULL))
    {
        image = new pawsWidget();
        image->SetRelativeFrameSize(BUTTON_SIZE, BUTTON_SIZE);
        image->Show();
        AddChild(image);
    }
    else if(!enable && (image != NULL))
    {
        image = NULL;
        pawsWidget::DeleteChild(image);
    }
    SetLayout();
}

void pawsMenuItem::SetCheckboxImages(const csString &on, const csString &off)
{
    if(checkbox != NULL)
    {
        checkbox->SetUpImage(off);
        checkbox->SetDownImage(on);
    }
}

void pawsMenuItem::SetCheckboxState(bool checked)
{
    if(checkbox != NULL)
        checkbox->SetState(checked);
}

void pawsMenuItem::SetLabel(const csString &newLabel)
{
    label->SetText(newLabel);
}

void pawsMenuItem::SetImage(const csString &newImage)
{
    assert(image != NULL);
    image->SetBackground(newImage);
}

void pawsMenuItem::SetSizes(int labelWidth, int _spacing, int _border)
{
    if(labelWidth == -1)
        label->SetSizeByText(5,5);
    else
        label->SetRelativeFrameSize(labelWidth, label->GetDefaultFrame().Height());
    spacing = _spacing;
    border  = _border;

    SetLayout();
}

void pawsMenuItem::SetLayout()
{
    int maxHeight;
    int x, y;

    maxHeight = 0;
    x = border;
    y = border;

    if(imageEnabled)
    {
        maxHeight = csMax(maxHeight, image->GetDefaultFrame().Height());
        image->SetRelativeFramePos(x, y);
        x += image->GetDefaultFrame().Width() + spacing;
    }

    maxHeight = csMax(maxHeight, label->GetDefaultFrame().Height());
    label->SetRelativeFramePos(x, y);
    x += label->GetDefaultFrame().Width() + spacing;

    if(checkboxEnabled)
    {
        maxHeight = csMax(maxHeight, checkbox->GetDefaultFrame().Height());
        checkbox->SetRelativeFramePos(x, y);
        x += checkbox->GetDefaultFrame().Width();
    }

    x += SUBMENU_ARROW_SIZE;
    x += border;

    SetRelativeFrameSize(x, maxHeight + 2*border);
}

void pawsMenuItem::SetAction(const pawsMenuAction &newAction)
{
    action = newAction;
}

void pawsMenuItem::Draw()
{
    psPoint mousePos;
    csRect rect;
    pawsWidget* widgetUnderMouse;

    pawsWidget::Draw();

    mousePos = PawsManager::GetSingleton().GetMouse()->GetPosition();

    rect = screenFrame;
    rect.xmin = parent->GetScreenFrame().xmin + BORDER_SIZE-2;
    rect.xmax = parent->GetScreenFrame().xmax - BORDER_SIZE+2;
    if(rect.Contains(mousePos.x, mousePos.y))
    {
        /*
         * The rectangle is drawn only if our menu item is not covered (hidden) by another widget:
         */
        widgetUnderMouse = PawsManager::GetSingleton().GetMainWidget()->WidgetAt(mousePos.x, mousePos.y);
        if(
            (widgetUnderMouse == this->WidgetAt(mousePos.x, mousePos.y))
            ||
            (widgetUnderMouse == parent)
        )
        {
            graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
            DrawRect(graphics2d, rect.xmin+1, rect.ymin+1, rect.xmax-3, rect.ymax+1, graphics2d->FindRGB(240, 194, 89));
        }
    }
}

void pawsMenuItem::Invoke()
{
    pawsIMenu* menu;
    menu = dynamic_cast<pawsIMenu*>(parent);
    assert(menu);
    menu->DoAction(this);
}

//////////////////////////////////////////////////////////////////////
//                        class pawsMenu
//////////////////////////////////////////////////////////////////////

pawsMenu::pawsMenu()
{
    parentMenu    = NULL;
    stickyButton  = NULL;
    closeButton   = NULL;
    notifyTarget  = NULL;
    label         = NULL;

    align         = alignLeft;
    autosize      = true;
    sticky        = false;

    factory       = "pawsMenu";

    graphics2d    = PawsManager::GetSingleton().GetGraphics2D();
}

pawsMenu::pawsMenu(const pawsMenu &origin):
    pawsIMenu(origin),
    parentMenu(NULL),
    notifyTarget(NULL),
    arrowImage(origin.arrowImage),
    align(origin.align),
    autosize(origin.autosize),
    sticky(origin.sticky),
    graphics2d(origin.graphics2d)
{
    stickyButton  = NULL;
    closeButton   = NULL;
    label         = NULL;

    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        for(unsigned int j = 0 ; j < origin.items.GetSize() ; j++)
        {
            if(origin.items[j] == origin.children[i])
            {
                pawsMenuItem* pi = dynamic_cast<pawsMenuItem*>(children[i]);
                if(pi == 0)
                    Error1("failed in copying pawsMenu\n")
                    else items.Push(pi);
                break;
            }
        }
        if(origin.closeButton == origin.children[i])
            closeButton = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.label == origin.children[i])
            label = dynamic_cast<pawsTextBox*>(children[i]);
        else if(origin.stickyButton == origin.children[i])
            stickyButton = dynamic_cast<pawsButton*>(children[i]);

        if(closeButton != 0 && label != 0 && stickyButton != 0 && items.GetSize() == origin.items.GetSize())
            break;
    }
}

pawsMenu::~pawsMenu()
{
}

void pawsMenu::AddItem(pawsIMenuItem* item, pawsIMenuItem* nextItem)
{
    if(nextItem == NULL)
        items.Push(item);
    else
    {
        for(size_t x = 0; x < items.GetSize(); x++)
        {
            if(nextItem == items[x])
            {
                if(x == 0)
                    items.Insert(x, item);
                else
                    items.Insert(x-1, item);
            }
        }
    }

    AddChild(item);
    item->Show();

    if(autosize)
        Autosize();
    SetPositionsOfItems();
}

void pawsMenu::SetPositionsOfItems()
{
    csRect rect;
    int contHeight;
    int itemX, itemY;

    contHeight = GetContentHeight();

    itemY = label->GetScreenFrame().Height() + GetActualHeight(2*BORDER_SIZE + ITEM_SPACING)
            + (screenFrame.Height() - GetActualHeight(label->GetDefaultFrame().Height()+2*BORDER_SIZE+2*ITEM_SPACING) - contHeight) / 2;

    for(size_t x = 0; x < items.GetSize(); x++)
    {
        rect = items[x]->GetDefaultFrame();
        if(align == alignLeft)
            itemX = GetActualWidth(BORDER_SIZE);
        else
            itemX = (defaultFrame.Width() - rect.Width()) / 2;

        items[x]->SetRelativeFramePos(itemX, itemY);

        itemY += rect.Height() + GetActualHeight(ITEM_SPACING);
    }


}

bool pawsMenu::OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* widget)
{
    if((widget == stickyButton) && !sticky)
    {
        sticky = true;
        stickyButton->SetToggle(false);
        return true;
    }
    else if(widget == closeButton)
    {
        DestroyMenu(closeCloseClicked);
        return true;
    }
    return false;
}

bool pawsMenu::Setup(iDocumentNode* node)
{
    csRef<iDocumentAttribute> attr;
    csString colourStr;

    arrowImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("Right Arrow");
    if(!arrowImage)
        return false;

    stickyButton = new pawsButton();
    stickyButton->SetRelativeFrameSize(GetActualWidth(BUTTON_SIZE), GetActualHeight(BUTTON_SIZE));
    stickyButton->SetUpImage("stickyoff");
    stickyButton->SetDownImage("stickyon");
    stickyButton->SetToggle(true);
    if(node->GetAttributeValue("label"))
        AddChild(stickyButton);

    closeButton = new pawsButton();
    closeButton->SetToggle(false);
    closeButton->SetRelativeFrameSize(GetActualWidth(BUTTON_SIZE), GetActualHeight(BUTTON_SIZE));
    closeButton->SetUpImage("quit");
    if(node->GetAttributeValue("label"))
        AddChild(closeButton);

    SetButtonPositions();

    label = new pawsTextBox();
    label->SetRelativeFramePos(GetActualWidth(BORDER_SIZE), GetActualHeight(BORDER_SIZE));
    if(node->GetAttributeValue("label"))
    {
        label->SetText(PawsManager::GetSingleton().Translate(node->GetAttributeValue("label")));
        label->SetSizeByText(5,5);
        label->Show();
        AddChild(label);
    }

    attr = node->GetAttribute("align");
    if(attr != NULL)
    {
        if(strcmp(attr->GetValue(), "center") == 0)
            align = alignCenter;
        else
            align = alignLeft;
    }
    else
        align = alignLeft;

    attr = node->GetAttribute("autosize");
    if(attr != NULL)
        autosize = (strcmp(attr->GetValue(), "true") == 0);
    else
        autosize = true;

    attr = node->GetAttribute("colour");
    if(attr != NULL)
        colourStr = attr->GetValue();
    else
        colourStr = "00ffff";
    label->SetColour(ParseColor(colourStr, PawsManager::GetSingleton().GetGraphics2D()));
    return true;
}

bool pawsMenu::PostSetup()
{
    pawsIMenuItem* childAsItem;

    for(size_t i = 0; i < pawsWidget::children.GetSize(); i++)
    {
        childAsItem = dynamic_cast<pawsIMenuItem*>(pawsWidget::children[i]);
        if(childAsItem != NULL)
            items.Push(childAsItem);
    }
    if(autosize)
        Autosize();
    SetPositionsOfItems();
    return true;
}

int pawsMenu::GetContentWidth()
{
    int contWidth = 0;
    for(size_t x = 0; x < items.GetSize(); x++)
    {
        contWidth = csMax(contWidth, items[x]->GetDefaultFrame().Width());
    }
    return contWidth;
}

int pawsMenu::GetContentHeight()
{
    int contHeight = 0;
    for(size_t x = 0; x < items.GetSize(); x++)
    {
        if(x > 0)
            contHeight += GetActualHeight(ITEM_SPACING);
        contHeight += items[x]->GetDefaultFrame().Height();
    }
    return contHeight;
}

void pawsMenu::Autosize()
{
    SetRelativeFrameSize
    (
        csMax(
            GetContentWidth() + GetActualWidth(2*BORDER_SIZE),
            label->GetDefaultFrame().Width() + GetActualWidth(2*BUTTON_SIZE + 5*BORDER_SIZE)
        ),
        GetContentHeight() + GetActualHeight(label->GetDefaultFrame().Height() + 2*BORDER_SIZE + 2*ITEM_SPACING)
    );
    SetButtonPositions();
}

void pawsMenu::SetButtonPositions()
{
    stickyButton -> SetRelativeFramePos(screenFrame.Width() - GetActualWidth(BORDER_SIZE*2 + BUTTON_SIZE*2),
                                        GetActualHeight(BORDER_SIZE));
    closeButton  -> SetRelativeFramePos(screenFrame.Width() - GetActualWidth(BORDER_SIZE + BUTTON_SIZE),
                                        GetActualHeight(BORDER_SIZE));
}

void pawsMenu::OnParentMenuDestroyed(pawsMenuClose /*reason*/)
{
    parentMenu = NULL;
    if(!sticky)
        DestroyMenu(closeParentClosed);
}

bool pawsMenu::HasSubmenus()
{
    return (submenus.GetSize() != 0);
}

void pawsMenu::OnChildMenuDestroyed(pawsIMenu* child, pawsMenuClose reason)
{
    for(size_t x = 0; x < submenus.GetSize(); x++)
    {
        if(submenus[x] == child)
        {
            submenus.Delete(submenus[x]);
            break;
        }
    }

    if(!sticky)
        switch(reason)
        {
            case closeAction:
            case closeCloseClicked:
            case closeChildClosed:
                DestroyMenu(closeChildClosed);
                break;
            case closeSiblingOpened:
            case closeParentClosed:
                break;
        }
}

void pawsMenu::DoAction(pawsIMenuItem* item)
{
    pawsMenuAction action;
    csRef<iDocument> doc;
    csRef<iDocumentNode> submenuNode;
    csString fileName;


    action = item->GetAction();

    if(action.name == SUBMENU_ACTION_NAME)
    {
        for(size_t x = 0; x < submenus.GetSize(); x++)
        {
            submenus[x]->OnSiblingOpened();
        }

        if(action.params.GetSize() == 0)
        {
            Error1("Submenu invocation menu item must have one or two parameters");
            return;
        }

        fileName = PawsManager::GetSingleton().GetLocalization()->FindLocalizedFile(action.params[0]);
        doc = ParseFile(PawsManager::GetSingleton().GetObjectRegistry(), fileName);
        if(doc == NULL)
        {
            Error2("Parsing of file %s failed", fileName.GetData());
            return;
        }
        if(action.params.GetSize() == 1)
            submenuNode = FindFirstWidget(doc);
        else if(action.params.GetSize() == 2)
        {
            csRef<iDocumentNode> root = doc->GetRoot();
            submenuNode = FindSubmenuNode(root, action.params[1]);
        }
        if(submenuNode == NULL)
        {
            Error1("Submenu XML node not found");
            return;
        }

        pawsMenu* newMenu = new pawsMenu();
        if(! newMenu->Load(submenuNode))
        {
            Error2("Could not load menu %s", fileName.GetData());
            delete newMenu;
            return;
        }
        PawsManager::GetSingleton().GetMainWidget()->AddChild(newMenu);
        SetSubmenuPos(newMenu, item->GetScreenFrame().ymin);
        newMenu->SetAlwaysOnTop(true);
        newMenu->Show();
        newMenu->SetParentMenu(this);
        newMenu->SetNotify(notifyTarget);
        submenus.Push(newMenu);
    }
    else
    {
        SendOnMenuAction(action);
        if(!sticky)
            DestroyMenu(closeAction);
    }
}

csPtr<iDocumentNode> pawsMenu::FindSubmenuNode(iDocumentNode* node, const csString &name)
{
    csRef<iDocumentNodeIterator> iter;
    csRef<iDocumentNode> child, foundNode;
    csString nodeName;

    if((node->GetType() != CS_NODE_ELEMENT) && (node->GetType() != CS_NODE_DOCUMENT))
        return NULL;

    nodeName = node->GetAttributeValue("name");
    if(nodeName == name)
        return csPtr<iDocumentNode>(node);

    iter = node->GetNodes();
    while(iter->HasNext())
    {
        child = iter->Next();
        foundNode = FindSubmenuNode(child, name);
        if(foundNode != NULL)
            return csPtr<iDocumentNode>(foundNode);
    }
    return NULL;
}

void pawsMenu::SendDestroyAction()
{
    pawsMenuAction action;
    action.name = MENU_DESTROY_ACTION_NAME;
    SendOnMenuAction(action);
}

void pawsMenu::SetNotify(pawsWidget* _notifyTarget)
{
    notifyTarget = _notifyTarget;
}

void pawsMenu::SendOnMenuAction(const pawsMenuAction &action)
{
    if(notifyTarget != NULL)
        notifyTarget->OnMenuAction(this, action);
    else
        Error1("Nowhere to send OnMenuAction - menu has no notification target");
}

void pawsMenu::SetParentMenu(pawsIMenu* _parentMenu)
{
    parentMenu = _parentMenu;
}

void pawsMenu::DestroyMenu(pawsMenuClose reason)
{
    if(parentMenu != NULL)
        parentMenu->OnChildMenuDestroyed(this, reason);

    for(size_t x = 0; x < submenus.GetSize(); x++)
    {
        submenus[x]->OnParentMenuDestroyed(reason);
    }

    SendDestroyAction();
}

void pawsMenu::OnSiblingOpened()
{
    if(!sticky)
        DestroyMenu(closeSiblingOpened);
}

void pawsMenu::SetSubmenuPos(pawsMenu* submenu, int recommY)
{
    int screenWidth, screenHeight;
    csRect submenuRect;
    int x, y;

    screenWidth   = graphics2d->GetWidth();
    screenHeight  = graphics2d->GetHeight();
    submenuRect   = submenu->GetScreenFrame();

    if(recommY < 0)
        y = 0;
    else if(recommY + submenuRect.Height() > screenHeight)
        y = screenHeight - submenuRect.Height();
    else
        y = recommY;

    if(screenFrame.xmax + 2 + submenuRect.Width() <= screenWidth)
        x = screenFrame.xmax + 2;
    else
        x = screenFrame.xmin - 2 - submenuRect.Width();

    submenu->MoveTo(x, y);
}

void pawsMenu::Draw()
{
    csRect itemFrame, arrowFrame;
    csRect rect;

    pawsWidget::Draw();

    graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());

    if(arrowImage)
    {
        for(size_t x = 0; x < items.GetSize(); x++)
        {
            if(items[x]->GetAction().name == SUBMENU_ACTION_NAME)
            {
                itemFrame = items[x]->GetScreenFrame();
                arrowFrame.SetPos(screenFrame.xmax - BORDER_SIZE - 4 - SUBMENU_ARROW_SIZE,
                                  itemFrame.ymin + itemFrame.Height()/2 - SUBMENU_ARROW_SIZE/2 + 1);
                arrowFrame.SetSize(SUBMENU_ARROW_SIZE, SUBMENU_ARROW_SIZE);
                arrowImage->Draw(arrowFrame);
            }
        }
    }

    rect.xmin = screenFrame.xmin+2;
    rect.ymin = screenFrame.ymin+2;
    rect.xmax = screenFrame.xmax-2;
    rect.ymax = screenFrame.ymin + GetActualHeight(2*BORDER_SIZE + label->GetDefaultFrame().Height())-2;
    DrawBumpFrame(graphics2d, this, rect, GetBorderStyle());

    rect.ymin = screenFrame.ymin + GetActualHeight(2*BORDER_SIZE + label->GetDefaultFrame().Height());
    rect.ymax = screenFrame.ymax-2;
    DrawBumpFrame(graphics2d, this, rect, GetBorderStyle());
}

bool pawsMenu::OnMouseDown(int button, int modifiers, int x, int y)
{
    csRect rowRect;

    for(size_t i = 0; i < items.GetSize(); i++)
    {
        rowRect = items[i]->GetScreenFrame();
        rowRect.xmin = screenFrame.xmin + BORDER_SIZE;
        rowRect.xmax = screenFrame.xmax - BORDER_SIZE;
        if(rowRect.Contains(x, y))
        {
            items[i]->Invoke();
            return true;
        }
    }

    return pawsWidget::OnMouseDown(button, modifiers, x, y);
}

//////////////////////////////////////////////////////////////////////
//                      class pawsMenuSeparator
//////////////////////////////////////////////////////////////////////

pawsMenuSeparator::pawsMenuSeparator()
{
    SetRelativeFrameSize(10, 10);
    factory = "pawsMenuSeparator";
    graphics2d = PawsManager::GetSingleton().GetGraphics2D();
}
pawsMenuSeparator::pawsMenuSeparator(const pawsMenuSeparator &origin)
    :pawsIMenuItem(origin),
     graphics2d(origin.graphics2d)
{

}
void pawsMenuSeparator::Draw()
{
    csRect parentFrame, sepFrame;

    graphics2D->SetClipRect(0,0, graphics2D->GetWidth(), graphics2D->GetHeight());

    parentFrame = parent->GetScreenFrame();
    sepFrame.xmin = parentFrame.xmin + BORDER_SIZE;
    sepFrame.xmax = parentFrame.xmax - BORDER_SIZE;
    sepFrame.ymin = screenFrame.ymin + 6;
    sepFrame.ymax = screenFrame.ymin + 9;
    DrawBumpFrame(graphics2d, this, sepFrame, GetBorderStyle());
}

