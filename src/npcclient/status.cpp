/*
* status.cpp
*
* Copyright (C) 2008 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/csstring.h>
#include <csutil/xmltiny.h>
#include <iutil/objreg.h>
#include <iutil/cfgmgr.h>

//=============================================================================
// Library Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/psxmlparser.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "globals.h"
#include "status.h"
#include "npcclient.h"



class psNPCStatusRunEvent : public psGameEvent
{
public:
    psNPCStatusRunEvent(csTicks interval);
    void Trigger();
    virtual csString ToString() const
    {
        return "Not implemented";
    }
};

psNPCStatusRunEvent::psNPCStatusRunEvent(csTicks interval)
    : psGameEvent(0, interval, "psNPCStatusRunEvent")
{

}

//-----------------------------------------------------------------------------

void psNPCStatusRunEvent::Trigger()
{
    struct tm currentTime;
    time_t now;

    csString reportString;
    csString timeString;

    time(&now);
    currentTime = *gmtime(&now);
    timeString = asctime(&currentTime);


    reportString.Format("<npc_report time=\"%s\" now=\"%ld\" number=\"%u\">\n",
                        timeString.GetData(), now, NPCStatus::count);
    reportString.Append("</npc_report>");

    csRef<iFile> logFile = npcclient->GetVFS()->Open(NPCStatus::reportFile, VFS_FILE_WRITE);
    if(logFile.IsValid())
    {
        logFile->Write(reportString, reportString.Length());
        logFile->Flush();
    }


    NPCStatus::count++;
    NPCStatus::ScheduleNextRun();
}

csTicks         NPCStatus::reportRate;
csString        NPCStatus::reportFile;
unsigned int    NPCStatus::count;


bool NPCStatus::Initialize(iObjectRegistry* objreg)
{
    csRef<iConfigManager> configmanager =  csQueryRegistry<iConfigManager> (objreg);
    if(!configmanager)
    {
        return false;
    }

    bool reportOn = configmanager->GetInt("PlaneShift.NPCClient.Status.Report", 0) ? true : false;
    if(!reportOn)
    {
        return true;
    }

    reportRate = configmanager->GetInt("PlaneShift.NPCClient.Status.Rate", 1000);
    reportFile = configmanager->GetStr("PlaneShift.NPCClient.Status.LogFile", "/this/npcfile");

    ScheduleNextRun();
    return true;
}


void NPCStatus::ScheduleNextRun()
{
    npcclient->GetEventManager()->Push(new psNPCStatusRunEvent(reportRate));
}
