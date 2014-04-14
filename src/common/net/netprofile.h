/*
 * netprofile.h by Ondrej Hurt
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

#ifndef __NETPROFILE_H__
#define __NETPROFILE_H__

#include <csutil/parray.h>

#include "message.h"
#include "util/psprofile.h"

/**
 * \addtogroup common_net
 * @{ */

/**
 * Statistics of receiving or sending of network messages.
 */
class psNetMsgProfiles : public psOperProfileSet
{
public:
    void AddSentMsg(MsgEntry * me);
    void AddReceivedMsg(MsgEntry * me);
    csString Dump();
    void Reset();
protected:
    void AddEnoughRecords(csArray<psOperProfile*> & profs, int neededIndex, const char * desc);
    
    /**
     * Statistics for receiving and sending of different message types.
     */
    csArray<psOperProfile*> recvProfs, sentProfs;
};

/** @} */

#endif
