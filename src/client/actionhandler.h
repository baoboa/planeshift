/*
* actionhandler.h
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
#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/parray.h>
#include <csutil/ref.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================

class  MsgHandler;

/**
 *  This class handles mode messages from the server, specifying
 *  actions for a clicked location.
 */
class ActionHandler : public psClientNetSubscriber
{
public:

    ActionHandler( MsgHandler* mh, iObjectRegistry* object_reg );
    virtual ~ActionHandler();
    void HandleMessage( MsgEntry* me );

    /**
     * Creates and XML Query to send to the server.
     *
     * Create XML query
     *   <PRE>
     *   \<location\>
     *     \<sector\>\</sector\>
     *     \<mesh\>\</mesh\>
     *     \<polygon\>\</polygon\>
     *     \<position\>\<x/\>\<y/\>\<z/\>\</position\>
     *     \<triggertype\>\</triggertype\>
     *   \<location\>
     *   </PRE>
     */
    void Query( const char* trigger, const char* sector, const char* mesh, int32_t poly, csVector3 pos );

    /**
     * Command server to save an action location to the database.
     *
     * Create XML query
     * <PRE>
     * \<location\>
     *     \<id\>\</id\>
     *     \<masterid\>\</masterid\>
     *     \<name\>\</name\>
     *     \<sector\>\</sector\>
     *     \<mesh\>\</mesh\>
     *     \<polygon\>\</polygon\>
     *     \<position\>\<x/\>\<y/\>\<z/\>\</position\>
     *     \<radius\>\</radius\>
     *     \<triggertype\>\</triggertype\>
     *     \<responsetype\>\</responsetype\>
     *     \<response\>\</response\>
     *     \<active\>\</active\>
     * \<location>          
     * </PRE>
     */
    void Save( const char* id, const char* masterid, const char* name, const char* sector, const char* mesh, 
        const char* poly, const char* posx, const char* posy, const char* posz, const char* pos_instance, const char* radius, 
        const char* triggertype, const char* responsetype, const char* response, const char* active );

    /**
     * Command server to remove an action location. 
     * Create XML query
     * <PRE>
     * \<location\>
     *     \<id\>\</id\>
     * \<location\> 
     * </PRE>
     */
    void DeleteAction( const char* id );

    /** Resend the client actions back to the client. */
    void ReloadCache( );
    
protected:
    iObjectRegistry*         objectReg;
    csRef<MsgHandler>        msgHandler;    
};

#endif

