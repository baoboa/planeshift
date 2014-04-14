 /*
 * psattackqueue.cpp              creator hartra344@gmail.com
 *
 * Copyright (C) 2001-2011 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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


#include<psconfig.h>
//====================================================================================
// Crystal Space Includes
//====================================================================================
#include <csutil/csstring.h>
#include <csutil/list.h>

//====================================================================================
// Project Includes
//====================================================================================
#include <idal.h>
//====================================================================================
// Local Includes
//====================================================================================
#include "cachemanager.h"
#include "combatmanager.h"
#include "psattackqueue.h"


psAttackQueue::psAttackQueue(psCharacter* pschar) :
    character(pschar)
{
}

bool psAttackQueue::Push(psAttack* attack)
{
    if(getAttackListCount() < DEFAULT_ATTACKQUEUE_SIZE)
    {
        attackList.PushBack(attack);
        psserver->GetCombatManager()->sendAttackQueue(character);
        return true;
    }

    return false;
}

csList< csRef<psAttack> >& psAttackQueue::getAttackList()
{
    return attackList;
}

size_t psAttackQueue::getAttackListCount()
{
    size_t i = 0;
    csList<csRef<psAttack> >::Iterator it(attackList);
    while(it.HasNext())
    {
        it.Next();
        i++;
    }
    return i;
}

bool psAttackQueue::Pop()
{
    bool val = attackList.PopFront();
    psserver->GetCombatManager()->sendAttackQueue(character);
    return val;
}

psAttack* psAttackQueue::First()
{
    if(!attackList.IsEmpty())
    {
        return attackList.Front();
    }
    return 0;
}

void psAttackQueue::Purge()
{
    attackList.DeleteAll();
    psserver->GetCombatManager()->sendAttackQueue(character);
}
