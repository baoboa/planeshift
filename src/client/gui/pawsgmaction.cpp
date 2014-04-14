/*
* pawsgmaction.cpp
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
* Description : client window for editing clickable map object actions
*
*/

#include <psconfig.h>
#include <iutil/objreg.h>

#include "net/cmdhandler.h"
#include "net/clientmsghandler.h"
#include "net/messages.h"
#include "util/psxmlparser.h"

#include "../globals.h"
#include "paws/pawslistbox.h"
#include "paws/pawsyesnobox.h"
#include "paws/pawsobjectview.h"
#include "paws/pawstextbox.h"
#include "paws/pawstree.h"
#include "paws/pawscombo.h"
#include "paws/pawscheckbox.h"
#include "pawsgmaction.h"
#include "../actionhandler.h"

pawsGMActionWindow::pawsGMActionWindow()
{
}

pawsGMActionWindow::~pawsGMActionWindow()
{
}


bool pawsGMActionWindow::PostSetup()
{           
    isDirty = false;

    txtID            = ( pawsEditTextBox * )FindWidget( "txtID" );
    txtName            = ( pawsEditTextBox * )FindWidget( "txtName" );
    txtSector        = ( pawsEditTextBox * )FindWidget( "txtSector" );
    txtMesh            = ( pawsEditTextBox * )FindWidget( "txtMesh" );
    chkFullMesh        = ( pawsCheckBox    * )FindWidget( "chkFullMesh" );
    chkActive         = ( pawsCheckBox    * )FindWidget( "chkActive" );
    txtPolygon        = ( pawsEditTextBox * )FindWidget( "txtPoly" );
    txtPosX            = ( pawsEditTextBox * )FindWidget( "txtX" );
    txtPosY            = ( pawsEditTextBox * )FindWidget( "txtY" );
    txtPosZ            = ( pawsEditTextBox * )FindWidget( "txtZ" );
    txtInstance        = ( pawsEditTextBox * )FindWidget( "txtInstance" );
    txtRadius        = ( pawsEditTextBox * )FindWidget( "txtRadius" );
    txtMasterID        = ( pawsEditTextBox * )FindWidget( "txtMaster" );
    cboTriggerType    = ( pawsComboBox    * )FindWidget( "cboTriggerType" );
    cboResponseType    = ( pawsComboBox    * )FindWidget( "cboResponseType" );
    txtResponse        = ( pawsMultilineEditTextBox * )FindWidget( "txtAction" );

    cboTriggerType->NewOption( "SELECT"    );
    cboTriggerType->NewOption( "PROXIMITY" );

    cboResponseType->NewOption( "EXAMINE" );
    cboResponseType->NewOption( "SCRIPT"  );

    confirm = dynamic_cast <pawsYesNoBox*> (PawsManager::GetSingleton().FindWidget("YesNoWindow"));
    if (confirm == NULL)
    {
        Error1("Could not find YesNoWindow !");
        return false;
    }
    return true;
}

bool pawsGMActionWindow::OnButtonPressed(int /*button*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {
    case 100:    // Save
        // TODO: Validate data.
        if ( strlen( txtName->GetText() ) == 0  || strlen( txtSector->GetText() ) == 0  ||
             strlen( txtMesh->GetText() ) == 0  || strlen( txtResponse->GetText() ) == 0 ) return false;

        psengine->GetActionHandler()->Save( txtID->GetText(), 
                                            txtMasterID->GetText(),
                                            txtName->GetText(),
                                            txtSector->GetText(), 
                                            txtMesh->GetText(), 
                                            txtPolygon->GetText(), 
                                            txtPosX->GetText(), 
                                            txtPosY->GetText(), 
                                            txtPosZ->GetText(),
                                            txtInstance->GetText(),
                                            txtRadius->GetText(), 
                                            cboTriggerType->GetSelectedRowString(), 
                                            cboResponseType->GetSelectedRowString(), 
                                            txtResponse->GetText(),
                                            chkActive->GetState() ? "Y" : "N");
        Close();
        break;
    case 101:    // Cancel
        Close();
        break;
    case 102:    // Edit Master
        //if ( !masterid.CompareNoCase("0") )
        //{
        //    csString xml = psengine->GetActionHandler()->Query( masterid );
        //    LoadAction( xml );
        //}
        break;
    case 103: // Use Full Mesh
        if (chkFullMesh->GetState())
        {
            txtPolygon->SetText( "0" );
            txtPosX->SetText( "0" );
            txtPosY->SetText( "0" );
            txtPosZ->SetText( "0" );
            txtRadius->SetText( "0" );
        }
        else
        {
            txtPolygon->SetText( polygon );
            txtPosX->SetText( X );
            txtPosY->SetText( Y );
            txtPosZ->SetText( Z );
            txtRadius->SetText( radius );
        }
        break;
    }
    return true;
}

bool pawsGMActionWindow::OnChange(pawsWidget* /*widget*/)
{
    isDirty = true;
    return true;
}

void pawsGMActionWindow::Show()
{
    pawsWidget::Show();
}


void pawsGMActionWindow::LoadAction( const char * xml )
{
    csRef<iDocument> doc         = ParseString( xml );    
    if(!doc)
    {
        Error2("Error parsing %s", xml);
        return;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", xml);
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode( "location" );
    if(!topNode)
    {
        Error2("Missing <location> tag in %s", xml);
        return;
    }

    LoadAction( topNode );
}

void pawsGMActionWindow::LoadAction( csRef<iDocumentNode> topNode )
{
    csRef<iDocumentNode> node;
    csRef<iDocumentNode> posNode;

    id="";
    name=""; masterid="0",
    sectorName=""; meshName=""; polygon="0";
    X="0"; Y="0"; Z="0"; pos_instance="4294967295", radius="0";
    triggertype=""; responsetype=""; response="";
    

    // sector
    node = topNode->GetNode( "id" );
    if ( node )
    {
        id = node->GetContentsValue();
    }

    // This is only for a new node
    chkFullMesh->SetVisibility( strlen( id ) == 0 );
    chkFullMesh->SetState( false );

    node = topNode->GetNode( "masterid" );
    if ( node )
    {
        masterid = node->GetContentsValue();
    }

    node = topNode->GetNode( "name" );
    if ( node )
    {
        name = node->GetContentsValue();
    }

    node = topNode->GetNode( "sector" );
    if ( node )
    {
        sectorName = node->GetContentsValue();
    }

    node = topNode->GetNode( "mesh" );
    if ( node )
    {
        meshName = node->GetContentsValue();
    }

    node = topNode->GetNode( "polygon" );
    if ( node )
    {
        polygon = node->GetContentsValue();
    }

    posNode = topNode->GetNode( "position" );
    if ( posNode )
    {
        node = posNode->GetNode( "x" );
        if ( node )
        {
            X = node->GetContentsValue();
        }

        node = posNode->GetNode( "y" );
        if ( node )
        {
            Y = node->GetContentsValue();
        }

        node = posNode->GetNode( "z" );
        if ( node )
        {
            Z = node->GetContentsValue();
        }
    }

    node = topNode->GetNode( "pos_instance" );
    if ( node )
    {
        pos_instance = node->GetContentsValue();
    }

    node = topNode->GetNode( "radius" );
    if ( node )
    {
        radius = node->GetContentsValue();
    }

    node = topNode->GetNode( "triggertype" );
    if ( node )
    {
        triggertype = node->GetContentsValue();
    }

    node = topNode->GetNode( "responsetype" );
    if ( node )
    {
        responsetype = node->GetContentsValue();
    }

    node = topNode->GetNode( "response" );
    if ( node )
    {
        response = node->GetContentsValue();
    }

    node = topNode->GetNode( "active" );
    if ( node )
    {
        const char *active = node->GetContentsValue();
        chkActive->SetState(active[0] == 'Y');
    }

    // Populate controls
    if ( IsVisible() && isDirty )
    {
        ////prompt to save changes
        //csString text;
        //text.Format("Do you want to save your changes? " );

        //confirm->SetNotify( this );
        //confirm->SetText( text );
        //confirm->CenterTo( this->GetScreenFrame().Width() / 2, this->GetScreenFrame().Height() / 2 );
        //confirm->SetModalState( true );
        //confirm->Show();
    }

    txtID->SetText( id );
    txtName->SetText( name );
    txtSector->SetText( sectorName );
    txtMesh->SetText( meshName );
    txtPolygon->SetText( polygon );
    txtPosX->SetText( X );
    txtPosY->SetText( Y );
    txtPosZ->SetText( Z );
    txtInstance->SetText(pos_instance);
    txtRadius->SetText( radius );
    if ( !IsVisible() )
    {
        txtMasterID->SetText( masterid );
        cboTriggerType->Select( triggertype );
        cboResponseType->Select( responsetype );
        txtResponse->SetText( response );
    }
    isDirty = false;

    Show();
}
