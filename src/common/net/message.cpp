/*
 * message.cpp by Anders Reggestad <andersr@pvv.org>
 *
 * Copyright (C) 2012 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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


#include "message.h"
#include "net/netbase.h"


void MsgEntry::Add(const iSector* sector)
{
    const char* sectorName = const_cast<iSector*>(sector)->QueryObject()->GetName ();

    csStringID sectorNameStrId = NetBase::GetAccessPointers()->Request(sectorName);
    Add( (uint32_t) sectorNameStrId );
    
    if (sectorNameStrId == csInvalidStringID)
    {
        Add(sectorName);
    }
}



iSector* MsgEntry::GetSector()    
{
    csString sectorName;
    csStringID sectorNameStrId;
    
    sectorNameStrId = GetUInt32();

    if (sectorNameStrId != csStringID(uint32_t(csInvalidStringID)))
    {
        sectorName = NetBase::GetAccessPointers()->Request(sectorNameStrId);
    }
    else
    {
        sectorName = GetStr();
    }
    
    if(!sectorName.IsEmpty())
    {
        return NetBase::GetAccessPointers()->engine->GetSectors ()->FindByName (sectorName);
    }
    
    return NULL;
}
