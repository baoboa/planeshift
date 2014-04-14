/*
* bankmanager.h - Author: Mike Gist
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

#ifndef __BANKMANAGER_H__
#define __BANKMANAGER_H__
//=============================================================================
// Project Includes
//=============================================================================
#include "util/gameevent.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "msgmanager.h"

class psMoneyGameEvent;
class Client;

class BankManager : public MessageManager<BankManager>
{
public:
    BankManager();
    ~BankManager();
    inline void StartBanking(Client* client, bool guild)
    {
        SendBankWindow(client, guild, true);
    }
    void ProcessTax();

protected:
    struct MoneyEvent
    {
        uint id;
        uint taxid;
        bool guild;
        int nextEvent; // YearsFrom1900:DaysSinceJan1st e.g. 107000 for Jan 1st 2007
        int interval; // In days
        int totalTrias;
        csString paid;
        csString npaid;
        csString fnpaid;
        bool latePayment;
        int lateBy;
        bool updateDate;
        InstanceID instance;
    };


    template<class T>
    void TaxAccount(T guildOrChar, MoneyEvent &monEvt, int index);

private:
    void SendBankWindow(Client* client, bool guild, bool forceOpen);
    void WithdrawFunds(Client* client, bool guild, int circles, int octas, int hexas, int trias);
    void DepositFunds(Client* client, bool guild, int circles, int octas, int hexas, int trias);
    void ExchangeFunds(Client* client, bool guild, int coins, int coin);
    psMoney* GetTotalFunds(psCharacter* pschar, bool guild);
    void HandleBanking(MsgEntry* me, Client* client);
    int CoinsForExchange(psCharacter* pschar, bool guild, int type, float fee);
    int CalculateAccountLevel(psCharacter* pschar, bool guild);
    inline float CalculateFee(psCharacter* pschar, bool guild);

    csArray<MoneyEvent> monEvts;

    MathScript* accountCharLvlScript;
    MathScript* accountGuildLvlScript;
    MathScript* CalcBankFeeScript;
};


class psMoneyGameEvent : public psGameEvent
{
public:

    psMoneyGameEvent(int delayTicks, BankManager* bankMan);

    virtual void Trigger()
    {
        bankManager->ProcessTax();    // Abstract event processing function
    }

private:
    BankManager* bankManager;
};

#endif // __BANKMANAGER_H__
