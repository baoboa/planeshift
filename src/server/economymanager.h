/*
 * economymanager.h
 *
 * Copyright (C) 2005 Atomic Blue (info@planeshift.it, http://www.planeshift.it)
 *
 * Credits : Christian Svensson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2
 * of the License).
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Creation Date: 11/Jan 2005
 * Description : This is the header file for the economymanager
 *               This manager handles all the exchange/trade things and calculates
 *               prices and keeps histories of transactions and so on
 *
 */

#ifndef ECONOMYMANAGER_HEADER
#define ECONOMYMANAGER_HEADER

//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <csutil/hash.h>
#include <csutil/sysfunc.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"             // Parent class


struct TransactionEntity : public csRefCount
{
    PID from;
    PID to;

    csString fromName;
    csString toName;
    csString itemName;

    unsigned int item;
    int count;
    int quality;
    unsigned int price;

    bool moneyIn;
    int stamp;

    TransactionEntity() :
        from(0), to(0), fromName("NA"), toName("NA"), itemName("NA"), item(0),
        count(0), quality(0), price(0), moneyIn(false), stamp(0)
    { }
};

struct ItemSupplyDemandInfo : public csRefCount
{
    unsigned int itemId;
    unsigned int bought;
    unsigned int sold;
};

class EconomyManager : public MessageManager<EconomyManager>
{
public:
    EconomyManager();
    ~EconomyManager();

    void HandleBuyMessage(MsgEntry* me,Client* client);
    void HandleSellMessage(MsgEntry* me,Client* client);
    void HandlePickupMessage(MsgEntry* me,Client* client);
    void HandleDropMessage(MsgEntry* me,Client* client);
    void HandleLootMessage(MsgEntry* me,Client* client);

    void AddTransaction(TransactionEntity* trans, bool sell, const char* type);

    TransactionEntity* GetTransaction(int id);
    unsigned int GetTotalTransactions();
    void ClearTransactions();
    void ScheduleDrop(csTicks ticks,bool loop);

    ItemSupplyDemandInfo* GetItemSupplyDemandInfo(unsigned int itemId);

    struct Economy
    {
        // Money flowing in to the economy
        unsigned int lootValue;
        unsigned int sellingValue;
        unsigned int pickupsValue;
        // Money flowing out of the economy
        unsigned int buyingValue;
        unsigned int droppedValue;

        Economy() : lootValue(0), sellingValue(0), pickupsValue(0), buyingValue(0), droppedValue(0) { };
    };

    Economy economy;

protected:
    csArray< csRef<TransactionEntity> > history;
    csHash< csRef<ItemSupplyDemandInfo> > supplyDemandInfo;
};


class psEconomyDrop : public psGameEvent
{
public:
    psEconomyDrop(EconomyManager* manager,csTicks ticks,bool loop);
    void Trigger();
protected:
    EconomyManager* economy;
    csTicks eachTimeTicks;
    bool loop;
};

#endif

