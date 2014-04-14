/*
 * pawsselector.cpp - Author: Andrew Craig
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

#include "pawsmanager.h"
#include "pawslistbox.h"
#include "pawsselector.h"
#include "pawsbutton.h"

pawsSelectorBox::pawsSelectorBox()
{
    moved = NULL;
    factory = "pawsSelectorBox";
}

pawsSelectorBox::pawsSelectorBox(const pawsSelectorBox &origin)
    :pawsWidget(origin)
{
    add = remove = NULL;
    available = selected = 0;
    moved = 0;

    for(unsigned int i = 0 ; i < origin.children.GetSize() ; i++)
    {
        if(origin.add == origin.children[i])
            add = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.available == origin.children[i])
            available = dynamic_cast<pawsListBox*>(children[i]);
        else if(origin.remove == origin.children[i])
            remove = dynamic_cast<pawsButton*>(children[i]);
        else if(origin.selected == origin.children[i])
            selected = dynamic_cast<pawsListBox*>(children[i]);

        if(add != 0 && remove != 0 && available != 0 && selected!= 0)
            break;
    }
}

pawsSelectorBox::~pawsSelectorBox()
{

}



bool pawsSelectorBox::Setup(iDocumentNode* node)
{
    ///////////////////////////////////////////////////////////////////////
    // Create the available list box
    ///////////////////////////////////////////////////////////////////////
    csRef<iDocumentNode> availableNode = node->GetNode("available");
    int width = availableNode->GetAttributeValueAsInt("width");
    int rowHeight = availableNode->GetAttributeValueAsInt("rowheight");

    available = new pawsListBox;
    AddChild(available);

    available->SetRelativeFrame(0 , 0, GetActualWidth(width), defaultFrame.Height());
    available->PostSetup();
    available->Show();
    available->UseTitleRow(false);
    available->SetID(AVAILABLE_BOX);
    //available->UseBorder();

    csString widgetDef("<widget name=\"Text\" factory=\"pawsTextBox\" ></widget>");
    available->SetTotalColumns(1);
    available->SetColumnDef(0,
                            width-20 , rowHeight,
                            widgetDef);

    ///////////////////////////////////////////////////////////////////////
    // Create the selected list box
    ///////////////////////////////////////////////////////////////////////
    csRef<iDocumentNode> selectedNode = node->GetNode("selected");
    width = selectedNode->GetAttributeValueAsInt("width");
    rowHeight = selectedNode->GetAttributeValueAsInt("rowheight");

    selected = new pawsListBox;
    AddChild(selected);

    selected->SetRelativeFrame(defaultFrame.Width()-width , 0, width, defaultFrame.Height());
    selected->PostSetup();
    selected->Show();
    selected->SetName("Selected");
    selected->UseTitleRow(false);
    selected->SetID(SELECTED_BOX);
    //selected->UseBorder();

    csString selectedDef("<widget name=\"Text\" factory=\"pawsTextBox\" ></widget>");

    selected->SetTotalColumns(1);
    selected->SetColumnDef(0,
                           width-20 , rowHeight,
                           selectedDef);

    ///////////////////////////////////////////////////////////////////////
    // Create the addbutton
    ///////////////////////////////////////////////////////////////////////
    csRef<iDocumentNode> addNode = node->GetNode("addbutton");
    if(addNode.IsValid())
    {
        add = new pawsButton;
        AddChild(add);

        // Puts the button at the edge of the text box widget
        add->SetRelativeFrame(GetActualWidth(addNode->GetAttributeValueAsInt("x")),  GetActualHeight(addNode->GetAttributeValueAsInt("y")), 16,16);

        //get some replacement resources for the addbutton up/down
        csString upAddImageName = addNode->GetAttributeValue("buttonup");
        csString downAddImageName = addNode->GetAttributeValue("buttondown");
        if(upAddImageName.IsEmpty())
            upAddImageName = "Right Arrow";
        if(downAddImageName.IsEmpty())
            downAddImageName = "Right Arrow";

        add->SetUpImage(upAddImageName);
        add->SetDownImage(downAddImageName);

        add->SetID(SELECTOR_ADD_BUTTON);

        add->PostSetup();
    }

    ///////////////////////////////////////////////////////////////////////
    // Create the removebutton
    ///////////////////////////////////////////////////////////////////////
    csRef<iDocumentNode> removeNode = node->GetNode("removebutton");
    if(removeNode.IsValid())
    {
        remove = new pawsButton;
        AddChild(remove);

        // Puts the button at the edge of the text box widget
        remove->SetRelativeFrame(GetActualWidth(removeNode->GetAttributeValueAsInt("x")),  GetActualHeight(removeNode->GetAttributeValueAsInt("y")), 16,16);

        //get some replacement resources for the removebutton up/down
        csString upRemoveImageName = removeNode->GetAttributeValue("buttonup");
        csString downRemoveImageName = removeNode->GetAttributeValue("buttondown");
        if(upRemoveImageName.IsEmpty())
            upRemoveImageName = "Left Arrow";
        if(downRemoveImageName.IsEmpty())
            downRemoveImageName = "Left Arrow";

        remove->SetUpImage(upRemoveImageName);
        remove->SetDownImage(downRemoveImageName);

        remove->SetID(SELECTOR_REMOVE_BUTTON);

        remove->PostSetup();
    }

    return true;
}

bool pawsSelectorBox::OnButtonPressed(int mouseButton, int keyModifier, pawsWidget* widget)
{
    switch(widget->GetID())
    {
        case SELECTOR_ADD_BUTTON:
        {
            // Remove from the available list and add to the selected list
            moved = available->RemoveSelected();

            selected->AddRow(moved);
            if(moved != NULL)
                return parent->OnButtonPressed(mouseButton, keyModifier, widget);

            return true;
        }

        case SELECTOR_REMOVE_BUTTON:
        {
            // Remove from the selected list and add to the available list
            moved = selected->RemoveSelected();
            available->AddRow(moved);
            if(moved != NULL)
                return parent->OnButtonPressed(mouseButton, keyModifier, widget);

            return true;
        }
    }

    return false;
}


void pawsSelectorBox::OnListAction(pawsListBox* widget, int status)
{
    if(status==LISTBOX_HIGHLIGHTED)
    {
        switch(widget->GetID())
        {
            case AVAILABLE_BOX:
            {
                // Make sure only the Available box has an item selected.
                selected->Select(NULL);
                break;
            }

            case SELECTED_BOX:
            {
                // Make sure only the selected box has an item selected.
                available->Select(NULL);
                break;
            }
        }
    }

    if(parent)
        parent->OnListAction(widget, status);

    return;
}

void pawsSelectorBox::RemoveFromAvailable(int id)
{
    available->Remove(id);
}

void pawsSelectorBox::RemoveFromSelected(int id)
{
    selected->Remove(id);
}

int pawsSelectorBox::GetAvailableCount(void)
{
    return available->GetRowCount();
}

pawsListBoxRow* pawsSelectorBox::CreateOption()
{
    return available->NewRow();
}

bool pawsSelectorBox::SelectAndMoveRow(int rowNo, bool toSelected)
{
    pawsListBox* sourceList = toSelected ? available : selected;
    pawsListBox* destList = toSelected ? selected : available;

    if(rowNo < 0 || rowNo >= (int)sourceList->GetRowCount())
        return false;

    // get row from row-number
    pawsListBoxRow* row = sourceList->GetRow(rowNo);
    if(!row)
        return false;

    sourceList->Remove(row);
    destList->AddRow(row);

    moved = row;

    return true;
}

