/*
 * psclientpersist.h by Matze Braun <MatzeBraun@gmx.de>
 *
 * Copyright (C) 2002 Atomic Blue (info@planeshift.it, http://www.atomicblue.org) 
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
#ifndef __PSCLIENTPERSIST_H__
#define __PSCLIENTPERSIST_H__
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/ref.h>
#include <csutil/strhashr.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "net/cmdbase.h"

//=============================================================================
// Local Includes
//=============================================================================


class psCelClient;
class MsgHandler;
class MsgEntry;
class pawsGroupWindow;
class pawsPetStatWindow;
class GEMClientActor;

class psClientDR : public psClientNetSubscriber
{
public:
    psClientDR();
    virtual ~psClientDR();

    bool Initialize(iObjectRegistry* object_reg, psCelClient* celbase, MsgHandler* msghandler );

    void CheckDeadReckoningUpdate();

    virtual void HandleMessage(MsgEntry* me);

    csStringHashReversible * GetMsgStrings() { return msgstrings; }
    bool GotStrings() {return gotStrings;}

    void CheckSectorCrossing(GEMClientActor* actor);
    void ResetMsgStrings();

    /**
     * Make sure DR data stopped on death
     */
    void HandleDeath(GEMClientActor * gemObject);

protected:

    bool gotStrings;

    psCelClient* celclient;
    csRef<MsgHandler> msghandler;
    
    csString last_sector;
    iObjectRegistry* object_reg;

    pawsGroupWindow* groupWindow;

    csStringHashReversible* msgstrings;
    
private:
    void HandleOverride( MsgEntry* me );
    void HandleStrings( MsgEntry* me );
    void HandleStatsUpdate( MsgEntry* me );
    void HandleDeadReckon( MsgEntry* me );
    void HandleForcePosition(MsgEntry *me);
    void HandleSequence( MsgEntry* me );
};

#endif

