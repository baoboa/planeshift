/*
* actionhandler.cpp
*
* Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
*
* Credits : 
*           Michael Cummings <cummings.michael@gmail.com>
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
* Description : client handler for clickable map object actions
*
*/
#include <psconfig.h>
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/objreg.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/messages.h"
#include "net/clientmsghandler.h"

#include "util/log.h"

#include "paws/pawsmanager.h"
#include "paws/pawscombo.h"

#include "gui/pawsinfowindow.h"
#include "gui/chatwindow.h"
#include "gui/pawsgmgui.h"
#include "gui/pawsgmaction.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "actionhandler.h"
#include "pscelclient.h"
#include "globals.h"



ActionHandler::ActionHandler( MsgHandler* mh, iObjectRegistry* obj_reg )
{
    msgHandler   = mh;
    objectReg   = obj_reg;

    if ( msgHandler )
    {
        msgHandler->Subscribe( this, MSGTYPE_MAPACTION );
    }
}


ActionHandler::~ActionHandler()
{
    if ( msgHandler )
    {
        msgHandler->Unsubscribe( this, MSGTYPE_MAPACTION );
    }
}


void ActionHandler::HandleMessage(MsgEntry* me)
{
    psMapActionMessage msg( me );

    if ( !msg.valid ) 
        return;

    switch ( msg.command )
    {
        case psMapActionMessage::NOT_HANDLED:
        {
            // Must Be GM9
            if ( psengine->GetCelClient()->GetMainPlayer()->GetType() < 29 )
                return;

            // Must have GM Window Open
            pawsGmGUIWindow * gm = (pawsGmGUIWindow *) PawsManager::GetSingleton().FindWidget( "GmGUI" );

            if (gm != NULL)
            {
                if ( !gm->IsVisible() ) 
                    return;
                if ( gm->GetCurrentTab() != 2 ) 
                    return;
            }

            // Get Handle to Add/Edit window
            pawsGMActionWindow *edit = (pawsGMActionWindow *)PawsManager::GetSingleton().FindWidget( "AddEditActionWindow" );
            if ( edit ) 
            {
                edit->LoadAction( msg.actionXML );
                (( pawsComboBox *)edit->FindWidget( "cboTriggerType" ))->Select( "" );
            }
            break;
        }
    }
}


void ActionHandler::Query( const char* trigger, const char* sector, const char* mesh, int32_t poly, csVector3 pos )
{
    csString poly_str; 
    poly_str.Format( "%d", poly );
    
    csString pos_str;  
    pos_str.Format( "<x>%f</x><y>%f</y><z>%f</z>", pos.x, pos.y, pos.z );
    
    csString xml;
    xml.Append( "<location>" );
        xml.Append( "<sector>" ); 
            xml.Append( sector ); 
        xml.Append( "</sector>" ); 
        xml.Append( "<mesh>" ); 
            xml.Append( mesh ); 
        xml.Append( "</mesh>" ); 
        xml.Append( "<polygon>" ); 
            xml.Append( poly_str ); 
        xml.Append( "</polygon>" ); 
        xml.Append( "<position>" ); 
            xml.Append( pos_str );
        xml.Append( "</position>" ); 
        xml.Append( "<triggertype>" );
            xml.Append( trigger );
        xml.Append( "</triggertype>" );
    xml.Append( "</location>" );
    // Create Message

    psMapActionMessage query_msg( 0, psMapActionMessage::QUERY, xml.GetData() );

    // Send Message
    query_msg.SendMessage();
}


void ActionHandler::Save( const char* id, const char* masterid, const char* name, const char* sector, 
                         const char* mesh, const char* poly, const char* posx, const char* posy, 
                         const char* posz, const char* pos_instance, const char* radius, const char* triggertype, 
                         const char* responsetype, const char* response, const char* active )
{
    csString xml;
    csString escpxml_response = EscpXML(response);

    xml.Append( "<location>" );
        xml.Append( "<id>" ); 
            xml.Append( id ); 
        xml.Append( "</id>" ); 
        xml.Append( "<masterid>" ); 
            xml.Append( masterid ); 
        xml.Append( "</masterid>" ); 
        xml.Append( "<name>" ); 
            xml.Append( name ); 
        xml.Append( "</name>" ); 
        xml.Append( "<sector>" ); 
            xml.Append( sector ); 
        xml.Append( "</sector>" ); 
        xml.Append( "<mesh>" ); 
            xml.Append( mesh ); 
        xml.Append( "</mesh>" ); 
        xml.Append( "<polygon>" ); 
            xml.Append( poly ); 
        xml.Append( "</polygon>" ); 
        xml.Append( "<position>" ); 
            xml.Append( "<x>" ); 
                xml.Append( posx );
            xml.Append( "</x>" ); 
            xml.Append( "<y>" ); 
                xml.Append( posy );
            xml.Append( "</y>" ); 
            xml.Append( "<z>" ); 
                xml.Append( posz );
            xml.Append( "</z>" ); 
        xml.Append( "</position>" ); 
        xml.Append( "<pos_instance>" ); 
            xml.Append( pos_instance ); 
        xml.Append( "</pos_instance>" ); 
        xml.Append( "<radius>" ); 
            xml.Append( radius ); 
        xml.Append( "</radius>" ); 
        xml.Append( "<triggertype>" ); 
            xml.Append( triggertype ); 
        xml.Append( "</triggertype>" ); 
        xml.Append( "<responsetype>" ); 
            xml.Append( responsetype ); 
        xml.Append( "</responsetype>" ); 
        xml.Append( "<response>" ); 
            xml.Append( escpxml_response ); 
        xml.Append( "</response>" ); 
        xml.Append( "<active>" ); 
            xml.Append( active ); 
        xml.Append( "</active>" ); 
    xml.Append( "</location>" );
    // Create Message

    psMapActionMessage query_msg( 0, psMapActionMessage::SAVE, xml.GetData() );

    // Send Message
    query_msg.SendMessage();
}


void ActionHandler::DeleteAction( const char* id )
{
    csString xml;
    xml.Append( "<location>" );
        xml.Append( "<id>" ); 
            xml.Append( id ); 
        xml.Append( "</id>" ); 
    xml.Append( "</location>" );

    // Create Message
    psMapActionMessage query_msg( 0, psMapActionMessage::DELETE_ACTION, xml.GetData() );

    query_msg.SendMessage();
}


void ActionHandler::ReloadCache( )
{
    psMapActionMessage query_msg( 0, psMapActionMessage::RELOAD_CACHE, "" );
    query_msg.SendMessage();
}
