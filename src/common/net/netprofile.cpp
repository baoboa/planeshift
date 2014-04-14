/*
 * netprofile.cpp by Ondrej Hurt
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
#include "netprofile.h"
#include "messages.h"


void psNetMsgProfiles::AddEnoughRecords(csArray<psOperProfile*> & arr, int neededIndex, const char * desc)
{
    size_t i = arr.GetSize();
    if ((size_t)neededIndex < i)
        return;
    arr.SetSize(neededIndex + 1);
    size_t d = profs.GetSize();
    profs.SetSize(d + arr.GetSize() - i);
    for (; i < arr.GetSize(); i++, d++)
    {
        csStringFast<100> fullDesc = GetMsgTypeName((int)arr.GetSize()) + "-" + desc;
        psOperProfile * newProf = new psOperProfile(fullDesc);
        arr[i] = newProf;
        profs[d] = newProf;
    }
}

void psNetMsgProfiles::AddSentMsg(MsgEntry * me)
{
    AddEnoughRecords(sentProfs, me->bytes->type, "sent");
    sentProfs[me->bytes->type]->AddConsumption(me->bytes->size);
}

void psNetMsgProfiles::AddReceivedMsg(MsgEntry * me)
{
    AddEnoughRecords(recvProfs, me->bytes->type, "recv");
    recvProfs[me->bytes->type]->AddConsumption(me->bytes->size);
}

csString psNetMsgProfiles::Dump()
{
    csStringFast<50> header, list;
    
    psOperProfileSet::Dump("byte", header, list);
    return "=================\nBandwidth profile\n=================\n" + header + list;
}

void psNetMsgProfiles::Reset()
{
    recvProfs.DeleteAll();
    sentProfs.DeleteAll();
    
    psOperProfileSet::Reset();
}
