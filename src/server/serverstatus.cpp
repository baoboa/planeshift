/*
 * serverconsole.cpp - author: Matze Braun <matze@braunis.de>
 *
 * Copyright (C) 2001 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
// Project Includes
//=============================================================================
#include "util/eventmanager.h"
#include "util/psxmlparser.h"

#include "bulkobjects/psguildinfo.h"
//=============================================================================
// Local Includes
//=============================================================================
#include "serverstatus.h"
#include "psserver.h"
#include "playergroup.h"
#include "netmanager.h"
#include "gem.h"
#include "clients.h"
#include "entitymanager.h"
#include "globals.h"
#include "clientstatuslogger.h"
#include "bulkobjects/pssectorinfo.h"
#include "economymanager.h"


/*****************************************************************
*                psServerStatusRunEvent
******************************************************************/

class psServerStatusRunEvent : public psGameEvent
{
public:
    psServerStatusRunEvent(csTicks interval);
    void Trigger();
    void ReportClient(Client* curr, ClientStatusLogger &clientLogger, csString &reportString);
    void ReportNPC(psCharacter* chardata, csString &reportString);
};

psServerStatusRunEvent::psServerStatusRunEvent(csTicks interval)
    : psGameEvent(0, interval, "psServerStatusRunEvent")
{

}

void psServerStatusRunEvent::Trigger()
{
    struct tm currentTime;
    time_t now;

    csString reportString;
    csString timeString;

    csRef<iDocumentSystem> docSystem;
    docSystem.AttachNew(new csTinyDocumentSystem());
    csRef<iDocument> doc = docSystem->CreateDocument();
    csRef<iDocumentNode> rootNode = doc->CreateRoot();

    // create ClientStatusLogger object to log info under node
    ClientStatusLogger clientLogger(rootNode);

    time(&now);
    currentTime = *gmtime(&now);
    timeString = asctime(&currentTime);
    timeString.Trim();
    EconomyManager::Economy &economy = psserver->GetEconomyManager()->economy;

    unsigned int moneyIn = economy.lootValue + economy.sellingValue + economy.pickupsValue;
    unsigned int moneyOut = economy.buyingValue + economy.droppedValue;

    ClientConnectionSet* clients = psserver->entitymanager->GetClients();
    reportString.Format("<server_report time=\"%s\" now=\"%ld\" number=\"%u\" client_count=\"%zu\" mob_births=\"%u\" mob_deaths=\"%u\" player_deaths=\"%u\" sold_items=\"%u\" sold_value=\"%u\" totalMoneyIn=\"%u\" totalMoneyOut=\"%u\">\n",
                        timeString.GetData(), now, ServerStatus::count, clients->Count(), ServerStatus::mob_birthcount, ServerStatus::mob_deathcount, ServerStatus::player_deathcount, ServerStatus::sold_items, ServerStatus::sold_value, moneyIn, moneyOut);
    ClientIterator i(*clients);
    while(i.HasNext())
    {
        Client* curr = i.Next();
        ReportClient(curr, clientLogger, reportString);
    }
    // Record npc data
    csHash<gemObject*, EID> &gems = psserver->entitymanager->GetGEM()->GetAllGEMS();
    csHash<gemObject*, EID>::GlobalIterator gemi(gems.GetIterator());
    gemObject* obj;

    while(gemi.HasNext())
    {
        obj = gemi.Next();
        if(!obj->GetClient() && obj->GetCharacterData())
            ReportNPC(obj->GetCharacterData(), reportString);
    }
    reportString.Append("</server_report>");

    csRef<iFile> logFile = psserver->vfs->Open(ServerStatus::reportFile, VFS_FILE_WRITE);
    logFile->Write(reportString, reportString.Length());
    logFile->Flush();

    // write XML log to file
    csRef<iFile> logFileTest = psserver->vfs->Open(csString("/this/testlog.xml"), VFS_FILE_WRITE);
    doc->Write(logFileTest);

    ServerStatus::count++;
    ServerStatus::ScheduleNextRun();
}

void psServerStatusRunEvent::ReportClient(Client* curr, ClientStatusLogger &clientLogger, csString &reportString)
{
    if(curr->IsSuperClient() || !curr->GetActor())
        return;

    const psCharacter* chr = curr->GetCharacterData();
    // log this client's info with the clientLogger
    clientLogger.LogClientInfo(curr);

    psGuildInfo* guild = curr->GetActor()->GetGuild();
    csString guildTitle;
    csString guildName;
    if(guild && guild->GetID() && !guild->IsSecret())
    {
        psGuildLevel* level = curr->GetActor()->GetGuildLevel();
        if(level)
        {
            guildTitle = EscpXML(level->title);
        }
        guildName = EscpXML(guild->GetName());
    }

    reportString.AppendFmt("<player name=\"%s\" characterID=\"%u\" guild=\"%s\" title=\"%s\" security=\"%d\" kills=\"%u\" deaths=\"%u\" suicides=\"%u\" pos_x=\"%.2f\" pos_y=\"%.2f\" pos_z=\"%.2f\" sector=\"%s\" />\n",
                           EscpXML(curr->GetName()).GetData(),
                           chr->GetPID().Unbox(),
                           guildName.GetDataSafe(),
                           guildTitle.GetDataSafe(),
                           curr->GetSecurityLevel(),
                           chr->GetKills(),
                           chr->GetDeaths(),
                           chr->GetSuicides(),
                           chr->GetLocation().loc.x,
                           chr->GetLocation().loc.y,
                           chr->GetLocation().loc.z,
                           EscpXML(chr->GetLocation().loc_sector->name).GetData());
}

void psServerStatusRunEvent::ReportNPC(psCharacter* chardata, csString &reportString)
{
    csString escpxml_name = EscpXML(chardata->GetCharFullName());
    csString player;

    player.Format("<npc name=\"%s\" characterID=\"%u\" kills=\"%u\" deaths=\"%u\" suicides=\"%u\" pos_x=\"%.2f\" pos_y=\"%.2f\" pos_z=\"%.2f\" sector=\"%s\" />\n",
                  escpxml_name.GetData(), chardata->GetPID().Unbox(),
                  chardata->GetKills(), chardata->GetDeaths(), chardata->GetSuicides(), chardata->GetLocation().loc.x, chardata->GetLocation().loc.y, chardata->GetLocation().loc.z, (const char*) EscpXML(chardata->GetLocation().loc_sector->name));

    reportString.Append(player);
}

/*****************************************************************
*                ServerStatus
******************************************************************/

csTicks      ServerStatus::reportRate;
csString     ServerStatus::reportFile;
unsigned int          ServerStatus::count;
unsigned int          ServerStatus::mob_birthcount;
unsigned int          ServerStatus::mob_deathcount;
unsigned int          ServerStatus::player_deathcount;
unsigned int          ServerStatus::sold_items;
unsigned int          ServerStatus::sold_value;

bool ServerStatus::Initialize(iObjectRegistry* objreg)
{
    csRef<iConfigManager> configmanager =  csQueryRegistry<iConfigManager> (objreg);
    if(!configmanager)
        return false;

    bool reportOn = configmanager->GetInt("PlaneShift.Server.Status.Report", 0) ? true : false;
    if(!reportOn)
        return true;

    reportRate = configmanager->GetInt("PlaneShift.Server.Status.Rate", 1000);
    reportFile = configmanager->GetStr("PlaneShift.Server.Status.LogFile", "/this/serverfile");

    ScheduleNextRun();
    return true;
}

void ServerStatus::ScheduleNextRun()
{
    psserver->GetEventManager()->Push(new psServerStatusRunEvent(reportRate));
}


