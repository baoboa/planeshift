/*
* bartender.cpp - Author: Andrew Craig, Stefano Angeleri
*
* Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "globals.h"

//=============================================================================
// Application Includes
//=============================================================================
#include "bartender.h"
#include "pawsslot.h"
#include "paws/pawsbutton.h"


//=============================================================================
// Defines
//=============================================================================
#define BARTENDER_FILE "/planeshift/userdata/options/bartender.xml"
#define DEFAULT_BARTENDER_FILE "/planeshift/data/options/bartender_def.xml"
#define ROOTNODE "bartender"
#define SLOTNODE "slot"
#define BTN_LOCK   100
#define BTN_UNLOCK 101

//=============================================================================
// Classes
//=============================================================================

pawsBartenderWindow::pawsBartenderWindow()
{
}

bool pawsBartenderWindow::PostSetup()
{
    //prepares to load the file
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    iDocumentSystem* xml = psengine->GetXMLParser ();
    csRef<iDocumentNode> root, bartenderNode;
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDataBuffer> buf (vfs->ReadFile (BARTENDER_FILE));
    //if the file is empty or not opeanable we are done. We return true because it's not a failure.
    if (!buf || !buf->GetSize ())
    {
        return true;
    }
    const char* error = doc->Parse( buf );
    if ( error )
    {
        Error2("Error loading bartender entries: %s\n", error);
        return true;
    }

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error2("%s has no XML root", BARTENDER_FILE);
    }
    bartenderNode = root->GetNode(ROOTNODE);
    if (bartenderNode == NULL)
    {
        Error2("%s has no <bartender> tag", BARTENDER_FILE);
    }

    //iterate all the nodes starting with slot in order to find the ones
    //to associate to the slots
    csRef<iDocumentNodeIterator> slotNodes = bartenderNode->GetNodes(SLOTNODE);
    while (slotNodes->HasNext())
    {
        csRef<iDocumentNode> slotNode = slotNodes->Next();

        //get the data needed for the widget to be setup
        csString slotName = slotNode->GetAttributeValue("name");
        csString slotImage = slotNode->GetAttributeValue("image");
        csString slotToolTip = slotNode->GetAttributeValue("tooltip");
        csString slotAction = slotNode->GetAttributeValue("action");

        //checks if this widget is a pawsslot
        pawsSlot * slot = dynamic_cast<pawsSlot*>(FindWidget(slotName));

        //if it's a pawsslot and it's a bartenderslot associate the data taken above to it
        if(slot && slot->IsBartender())
        {
            slot->PlaceItem(slotImage, "", "", 1);
            slot->SetToolTip(slotToolTip);
            slot->SetBartenderAction(slotAction);
        }
    }
    return true;
}

pawsBartenderWindow::~pawsBartenderWindow()
{
    //prepares the file for writing
    csRef<iDocumentSystem> xml;
    xml.AttachNew(new csTinyDocumentSystem);
    csRef<iDocument> doc = xml->CreateDocument();
    csRef<iDocumentNode> root = doc->CreateRoot ();
    csRef<iDocumentNode> bartenderNode = root->CreateNodeBefore(CS_NODE_ELEMENT);
    bartenderNode->SetValue(ROOTNODE);

    //searches all childrens for pawsslot supporting bartender
    for (size_t i = 0; i < GetChildrenCount(); i++)
    {
        pawsSlot * slot = dynamic_cast<pawsSlot*>(GetChild(i));
        if (slot && slot->IsBartender() && !slot->IsEmpty())
        {
            //if we found a not empty bartender we save it's current status in order to restore it on relog.
            csRef<iDocumentNode> slotNode = bartenderNode->CreateNodeBefore(CS_NODE_ELEMENT);
            slotNode->SetValue(SLOTNODE);
            slotNode->SetAttribute("name", slot->GetName());
            slotNode->SetAttribute("image", slot->ImageName());
            slotNode->SetAttribute("tooltip", slot->GetToolTip());
            slotNode->SetAttribute("action", slot->GetAction());
        }
    }

    //finally save the file
    csRef<iVFS> vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    doc->Write(vfs, BARTENDER_FILE);
}


void pawsBartenderWindow::SetLock(bool state)
{
    locked = state;
    for (size_t i = 0; i < GetChildrenCount(); i++)
    {
        pawsSlot * slot = dynamic_cast<pawsSlot*>(GetChild(i));
        if (slot && slot->IsBartender())
        {
            slot->SetLock(locked);
        }
    }
    printf("Hotbar has been %s\n", locked ? "locked." : "unlocked.");
}
 
bool pawsBartenderWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch(widget->GetID())
    {
        case BTN_LOCK:
        {
            //must be a button
            SetLock(dynamic_cast<pawsButton*>(widget)->GetState());
            return true;
        }
    }
    return false;
}




