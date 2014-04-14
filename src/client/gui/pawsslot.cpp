/*
 * pawsslot.cpp
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include <iostream>
using namespace std;

//=============================================================================
// PlaneShift Includes
//=============================================================================
#include "pawsslot.h"
#include "psslotmgr.h"
#include "globals.h"
#include "pscelclient.h"
#include "paws/pawstextbox.h"
#include "net/clientmsghandler.h"
#include "net/cmdhandler.h"


//=============================================================================
// Classes
//=============================================================================

pawsSlot::pawsSlot()
{
    emptyOnZeroCount = true;
    empty = true;
    containerID = -100;
    slotID = -1;

    SetRelativeFrame( 0,0, GetActualWidth(48), GetActualHeight(48) );

    purifySign = new pawsWidget();
    AddChild(purifySign);
    purifySign->SetRelativeFrame(GetActualWidth(5), GetActualHeight(5), GetActualWidth(37), GetActualHeight(37));
    purifySign->Hide();
    purifySign->Ignore(true);
    SetPurifyStatus(0);

    stackCountLabel = new pawsTextBox;
    AddChild( stackCountLabel );
    stackCountLabel->SetAlwaysOnTop(true);
    stackCountLabel->SetRelativeFrame( 8, 8, 40, 20 );
    stackCountLabel->SetFont(NULL,8);
    stackCountLabel->SetColour( graphics2D->FindRGB(255,255,255) );
    stackCountLabel->Hide();
    stackCountLabel->Ignore(true);
    dragDrop = true;
    StackCount(0);

    reserved = false;

    drawStackCount = true;

    locked = false;
}


pawsSlot::~pawsSlot()
{
}


bool pawsSlot::Setup( iDocumentNode* node )
{
    csRef<iDocumentNode> ident = node->GetNode( "ident" );
    if ( ident )
    {
        containerID = ident->GetAttributeValueAsInt("container");
        slotID = ident->GetAttributeValueAsInt("id");
    }

    csRef<iDocumentNode> showStackAmount = node->GetNode("show_stack_amount");
    if ( showStackAmount )
    {
       DrawStackCount(showStackAmount->GetContentsValueAsInt() != 0);
    }

    mgr = psengine->GetSlotManager();

    return true;
}


bool pawsSlot::OnMouseDown( int button, int modifiers, int x, int y )
{
    if ( button == csmbWheelUp || button == csmbWheelDown || button == csmbHWheelLeft || button == csmbHWheelRight)
    {
        return parent->OnMouseDown( button, modifiers, x, y);
    }

    if ( !psengine->GetCelClient()->GetMainPlayer()->IsAlive() )
        return true;

    if ( !empty && psengine->GetMouseBinds()->CheckBind("ContextMenu",button,modifiers) )
    {
        psViewItemDescription out(containerID, slotID);
        out.SendMessage();
        return true;
    }
    else
    {
        bool grab = psengine->GetMouseBinds()->CheckBind("EntityDragDrop", button, modifiers);
        bool grabAll = psengine->GetMouseBinds()->CheckBind("EntityDragDropAll", button, modifiers);
        bool grabOne = psengine->GetMouseBinds()->CheckBind("EntityDragDropOne", button, modifiers);

        if(!grab && !grabAll && !grabOne)
        {
            // TODO: this should be removed sometime
            // fallback for old configuration files (pre 0.4.02)
            grab = psengine->GetMouseBinds()->CheckBind("EntitySelect", button, modifiers);
            grabAll = (modifiers == 2 || button == 2);
            grabOne = (modifiers == 1);
        }

        if ( dragDrop && (grab || grabAll || grabOne) && (!empty || psengine->GetSlotManager()->IsDragging()) )
        {
            // Grab one item if EntityDragDropOne modifiers key are used. Grab everything in the slot
            // if EntityDragDropAll modifiers key are used
            mgr->Handle( this, grabOne, grabAll );
            return true;
        }
        return pawsWidget::OnMouseDown(button, modifiers, x, y);
    }
}


void pawsSlot::SetToolTip( const char* text )
{
    pawsWidget::SetToolTip( text );
    stackCountLabel->SetToolTip( text );
}


void pawsSlot::StackCount( int newCount )
{
    if (emptyOnZeroCount && newCount == 0)
    {
        Clear();
    }
    else
    {
        stackCount = newCount;

        csString countText;
        if (newCount > 0)
            countText.Format("%d", newCount);
        stackCountLabel->SetText(countText);
    }
}


void pawsSlot::PlaceItem( const char* imageName, const char* meshFactName, const char* matName, int count )
{

    meshfactName = meshFactName;
    materialName = matName;

    psengine->GetCelClient()->replaceRacialGroup(meshfactName);

    empty = false;

    image = PawsManager::GetSingleton().GetTextureManager()->GetOrAddPawsImage(imageName);

    if (drawStackCount)
        stackCountLabel->ShowBehind();

    stackCount = count;
    StackCount( count );

    reserved = false;
}


void pawsSlot::Draw()
{
    if (!drawStackCount && stackCountLabel->IsVisible())
        stackCountLabel->Hide();

    pawsWidget::Draw();
    ClipToParent(false);
    csRect frame = screenFrame;

    // Deal with frames that are taller than they are high (left/right hand
    // slots).
    if (frame.Height() > frame.Width())
    {
        int excess = frame.Height() - frame.Width();
        frame.ymin += excess/2;
        frame.ymax -= excess/2;
    }
    //Deal with money slots that are wider than they are high
    if (frame.Height() < frame.Width())
    {
        int excess = frame.Width() - frame.Height();
        frame.xmax -= excess;
    }

    if (!empty)
    {
        if (image)
            image->Draw( frame );
        //Need to redraw the count
        if (drawStackCount)
            stackCountLabel->Draw();
    }

    graphics2D->SetClipRect( 0,0, graphics2D->GetWidth(), graphics2D->GetHeight());
}


void pawsSlot::Clear()
{
    empty = true;
    stackCount = 0;
    stackCountLabel->Hide();
    SetPurifyStatus(0);
    SetToolTip("");

    reserved = false;
    image = NULL;
}


const char *pawsSlot::ImageName()
{
    if (image)
        return image->GetName();
    return NULL;
}


int pawsSlot::GetPurifyStatus()
{
    return purifyStatus;
}


void pawsSlot::SetPurifyStatus(int status)
{
    purifyStatus = status;

    switch (status)
    {
        case 0:
            purifySign->Hide();
            break;
        case 1:
            purifySign->ShowBehind();
            purifySign->SetBackground("GlyphSlotPurifying");
            break;
        case 2:
            purifySign->ShowBehind();
            purifySign->SetBackground("GlyphSlotPurified");
            break;
    }
}


void pawsSlot::DrawStackCount(bool value)
{
    drawStackCount = value;

    if (value)
        stackCountLabel->ShowBehind();
    else
        stackCountLabel->Hide();
}


bool pawsSlot::SelfPopulate( iDocumentNode *node)
{
    if (node->GetAttributeValue("icon"))
    {
        this->DrawStackCount(false);
        this->SetDrag(false);
        PlaceItem(node->GetAttributeValue("icon"), "", "", 1);
    }

    return true;
}


void pawsSlot::OnUpdateData(const char *dataname,PAWSData& value)
{
    csString sig(dataname);

    if (sig.StartsWith("sigClear"))
    {
        // Clear out the pub-sub cache for this specific slot to avoid stale
        // stuff coming back to life when a slot subscribes again.
        if (!slotName.IsEmpty())
            PawsManager::GetSingleton().Publish(slotName, "");

        Clear();
        SetDefaultToolTip();
    }
    else if (value.IsData())
    {
        psString data(value.GetStr());

        if (data.IsEmpty())
        {
            Clear();
            SetDefaultToolTip();
            return;
        }

        psString icon;
        psString count;
        psString name;
        psString mesh;
        psString material;
        psString status;

        csStringArray words;
        words.SplitString(data.GetData(), " ", csStringArray::delimSplitEach);
        icon = words[0];
        count = words[1];
        status = words[2];
        mesh = words[3];
        material = words[4];
        data.GetSubString( name, icon.Length()+count.Length()+status.Length()+mesh.Length()+material.Length()+5, data.Length());

        PlaceItem( icon, mesh, material, atoi(  count.GetData() ) );
        SetToolTip( name );
        SetPurifyStatus( atoi(status.GetData())  );
    }

}


void pawsSlot::ScalePurifyStatus()
{
    csRect rect = this->ClipRect();
    int width = rect.Width()- GetActualWidth(11);
    int height = rect.Height() - GetActualHeight(11);
    purifySign->SetRelativeFrameSize(width, height);
}

