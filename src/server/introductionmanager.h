/*
 * introductionmanager.h
 *
 * Copyright (C) 2007 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#ifndef __INTRODUCTIONMANAGER_HEADER__
#define __INTRODUCTIONMANAGER_HEADER__

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>
#include <csutil/set.h>

//=============================================================================
// Project Includes
//=============================================================================

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class

class IntroductionManager : public MessageManager<IntroductionManager>
{
public:
    IntroductionManager();
    ~IntroductionManager();

protected:
    void HandleIntroduction(MsgEntry* pMsg,Client* client);
};

#endif
