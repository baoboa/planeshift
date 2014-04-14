/*
* bankmanager.cpp - Author: Mike Gist
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

#include <psconfig.h>
//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/psguildinfo.h"

#include "util/mathscript.h"
#include "util/eventmanager.h"
#include "util/psdatabase.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "bankmanager.h"
#include "client.h"
#include "cachemanager.h"
#include "progressionmanager.h"
#include "globals.h"


enum coin
{
    CIRCLE, OCTA, HEXA, TRIA
};

#define DAY (24*60*60*1000)

///// Still need to stop people from being able to run away and continue banking as long as the screen is open.
///// I think this is possible for a lot of activities actually.... maybe a check for movement over a period would work?

BankManager::BankManager()
{
    Subscribe(&BankManager::HandleBanking, MSGTYPE_BANKING, REQUIRE_READY_CLIENT);

    // Load money events.
    Result result(db->Select("select * from money_events"));

    if(!result.IsValid())
    {
        CPrintf(CON_ERROR, "Could not load money_events from DB! *DIE*\n");
        exit(1);
        return;
    }

    for(uint currentRow = 0; currentRow < result.Count(); currentRow++)
    {
        iResultRow &row = result[currentRow];
        MoneyEvent monEvt;
        monEvt.id = row.GetInt("id");
        monEvt.taxid = row.GetInt("taxid");
        monEvt.guild = row.GetInt("guild") == 0 ? false : true;
        monEvt.nextEvent = row.GetInt("nextEvent");
        monEvt.interval = row.GetInt("inter");
        monEvt.totalTrias = row.GetInt("totaltrias");
        monEvt.paid = row["prg_evt_paid"];
        monEvt.npaid = row["prg_evt_npaid"];
        monEvt.latePayment = row.GetInt("latePayment") == 0 ? false : true;
        monEvt.lateBy = row.GetInt("lateBy");
        monEvt.updateDate = true;
        monEvt.instance = row.GetUInt32("instance");
        monEvts.Push(monEvt);
    }

    //load the mathscripts
    MathScriptEngine* eng = psserver->GetMathScriptEngine();
    accountGuildLvlScript = eng->FindScript("Calc Guild Account Level");
    accountCharLvlScript = eng->FindScript("Calc Char Account Level");
    CalcBankFeeScript = eng->FindScript("Calc Bank Fee");

    ProcessTax();
}

BankManager::~BankManager()
{
    //do nothing
}

psMoneyGameEvent::psMoneyGameEvent(int delayTicks, BankManager* bankMan)
    : psGameEvent(0, delayTicks, "psMoneyGameEvent")
{
    bankManager = bankMan;
}

template<class T>
void BankManager::TaxAccount(T guildOrChar, MoneyEvent &monEvt, int index)
{
    // Check if the money is available. Else do Something Bad.
    int moneyAvail;
    int circlesAvail;
    int octasAvail;
    int hexasAvail;
    int triasAvail;

    if(monEvt.guild)
    {
        psGuildInfo* g = (psGuildInfo*)guildOrChar;
        psMoney mon = g->GetBankMoney();
        moneyAvail = mon.GetTotal();
        circlesAvail = mon.GetCircles();
        octasAvail = mon.GetOctas();
        hexasAvail = mon.GetHexas();
        triasAvail = mon.GetTrias();
    }
    else
    {
        psCharacter* c = (psCharacter*)guildOrChar;
        psMoney* mon = GetTotalFunds(c, monEvt.guild);
        moneyAvail = mon->GetTotal();
        circlesAvail = mon->GetCircles();
        octasAvail = mon->GetOctas();
        hexasAvail = mon->GetHexas();
        triasAvail = mon->GetTrias();
    }
    if(moneyAvail < monEvt.totalTrias)
    {
        // Don't update the date of the event outside of this!
        monEvt.updateDate = false;

        if(!monEvt.latePayment)
        {
            // Mark payment as late and alter to run once a day for a week.
            monEvt.latePayment = true;
            monEvt.interval = 1;
            csString sql;
            sql.Format("UPDATE money_events SET latePayment=%u, inter=%u WHERE id=%u", 1, 1, monEvt.id);
            if(db->Command(sql) != 1)
                Error3("Couldn't mark payment as 'late' in database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());

            // Trigger 'not paid' script.
            //psserver->GetProgressionManager()->CreateEvent("Money_Event", monEvt.npaid)->Run(0, 0, false);
        }
        else
        {
            // Inc no. of days that the payment has been late by.
            monEvt.lateBy++;
            if(monEvt.lateBy > 6)
            {
                // Trigger 'final not paid' script.
                //psserver->GetProgressionManager()->CreateEvent("Money_Event", monEvt.fnpaid)->Run(0, 0, false);

                // Remove money event from money_events.
                monEvts.DeleteIndex(index);
                csString sql;
                sql.Format("DELETE FROM money_events WHERE id=%u", monEvt.id);
                if(db->Command(sql) != 1)
                    Error3("Couldn't remove money event from database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
            }
            else
            {
                // Update db with lateBy.
                csString sql;
                sql.Format("UPDATE money_events SET lateBy=%u WHERE id=%u", monEvt.lateBy, monEvt.id);
                if(db->Command(sql) != 1)
                    Error3("Couldn't update 'lateBy' in database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
            }
        }

        return;
    }
    // If the payment was marked as 'late' we need to unmark it, and run the 'paid' script.
    if(monEvt.latePayment)
    {
        // Unmark
        monEvt.latePayment = false;
        csString sql;
        sql.Format("UPDATE money_events SET latePayment=%u, lateBy=%u WHERE id=%u", 0, 0, monEvt.id);
        if(db->Command(sql) != 1)
            Error3("Couldn't unmark payment as 'late' in database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());

        // Paid
        //psserver->GetProgressionManager()->CreateEvent("Money_Event", monEvt.paid)->Run(0, 0, false);
    }

    // Work out how much of each coin to remove.
    int circles = 0;
    int octas = 0;
    int hexas = 0;
    int trias = 0;

    // Circles
    circles = monEvt.totalTrias / 250;
    if(circlesAvail < circles)
        circles = circlesAvail;
    monEvt.totalTrias -= circles * 250;

    // Octas
    octas = monEvt.totalTrias / 50;
    if(octasAvail < octas)
        octas = octasAvail;
    monEvt.totalTrias -= octas * 50;

    // Hexas
    hexas = monEvt.totalTrias / 10;
    if(hexasAvail < hexas)
        hexas = hexasAvail;
    monEvt.totalTrias -= hexas * 10;

    // Trias
    trias = monEvt.totalTrias;

    if(trias > triasAvail)
    {
        // Start breaking down bigger coins.
        while(hexas < hexasAvail && trias > triasAvail)
        {
            trias -= 10;
            hexas++;
        }
        while(octas < octasAvail && trias > triasAvail)
        {
            trias -= 50;
            octas++;
        }
        while(circles < circlesAvail && trias > triasAvail)
        {
            trias -=250;
            circles++;
        }
        // Tidy up any extra tria.
        if(trias < 0)
        {
            while(trias <= -50)
            {
                octas--;
                trias += 50;
            }
            while(trias <= -10)
            {
                hexas--;
                trias += 10;
            }
        }
    }

    // Deduct!
    guildOrChar->AdjustMoney(psMoney(-circles, -octas, -hexas, -trias), true);
}

void BankManager::ProcessTax()
{
    // Get current time.
    time_t curr = time(0);
    tm* gmtm = gmtime(&curr);
    int time = gmtm->tm_year * 1000;
    time += gmtm->tm_yday;

    // Check if any taxing is due and tax if so, also set next event date.
    for(uint i=0; i < monEvts.GetSize(); i++)
    {
        MoneyEvent temp = monEvts.Get(i);
        if(temp.nextEvent < time)
        {
            // Check for guild or character removal to remove auto-tax.
            if(temp.guild)
            {
                psGuildInfo* g = psserver->GetCacheManager()->FindGuild(temp.taxid);
                if(!g)
                {
                    monEvts.DeleteIndexFast(i);
                    csString sql;
                    sql.Format("DELETE FROM money_events WHERE id=%u", temp.id);
                    if(db->Command(sql) != 1)
                        Error3("Couldn't remove money event from database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
                    return;
                }
                // Tax
                TaxAccount(g, temp, i);
            }
            else
            {
                psCharacter* character = psserver->CharacterLoader.LoadCharacterData(temp.taxid, false);
                if(!character)
                {
                    monEvts.DeleteIndexFast(i);
                    csString sql;
                    sql.Format("DELETE FROM money_events WHERE id=%u", temp.id);
                    if(db->Command(sql) != 1)
                        Error3("Couldn't remove money event from database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
                    return;
                }
                // Tax
                TaxAccount(character, temp, i);
            }

            if(temp.updateDate)
            {
                // Set next date.
                int nextEventDate = 0;
                // We loop to catch up and get the right date, as it's possible we miss one or more (long server downtime from a Bad Thing e.g. nuclear war, alien invasion, etc.)
                int newDate = temp.nextEvent;
                while(true)
                {
                    newDate += temp.interval - temp.lateBy;
                    int daysInYear = 364;
                    if(gmtm->tm_isdst == 1)
                        daysInYear = 365;
                    if((newDate - gmtm->tm_year*1000) > daysInYear)
                        newDate += (1000 - daysInYear);
                    if(time < newDate)
                        break;
                    nextEventDate = newDate;
                }

                // Set'n'Save
                monEvts.Get(i).nextEvent = nextEventDate;
                monEvts.Get(i).interval = 7;
                csString sql;
                sql.AppendFmt("UPDATE money_events SET nextEvent=%u, inter=%u WHERE id=%u", nextEventDate, 7, temp.id);
                if(db->Command(sql) != 1)
                    Error3("Couldn't save next event date to database.\nCommand was <%s>.\nError returned was <%s>\n",db->GetLastQuery(),db->GetLastError());
            }
        }
    }

    // Schedule the next check.
    psMoneyGameEvent* event = new psMoneyGameEvent(DAY, this);
    psserver->GetEventManager()->Push(event);
}

void BankManager::DepositFunds(Client* client, bool guild, int circles, int octas, int hexas, int trias)
{
    if(circles < 0 || octas < 0 || hexas < 0 || trias < 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot deposit a negative amount!");
        return;
    }

    psCharacter* character = client->GetCharacterData();
    psMoney mon = character->Money();
    if(mon.GetCircles() < circles || mon.GetOctas() < octas || mon.GetHexas() < hexas || mon.GetTrias() < trias)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot deposit more than you have!");
        return;
    }

    if(guild)
    {
        psGuildInfo* g = character->GetGuild();
        if(!g)
            return;

        character->AdjustMoney(psMoney(-circles, -octas, -hexas, -trias), false);
        g->AdjustMoney(psMoney(circles, octas, hexas, trias));
    }
    else
    {
        character->AdjustMoney(psMoney(-circles, -octas, -hexas, -trias), false);
        character->AdjustMoney(psMoney(circles, octas, hexas, trias), true);
    }
}

void BankManager::WithdrawFunds(Client* client, bool guild, int circles, int octas, int hexas, int trias)
{
    if(circles < 0 || octas < 0 || hexas < 0 || trias < 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot withdraw a negative amount!");
        return;
    }

    psCharacter* character = client->GetCharacterData();
    psMoney* mon = GetTotalFunds(character, guild);

    if(!mon)
        return;

    if(mon->GetCircles() < circles || mon->GetOctas() < octas || mon->GetHexas() < hexas || mon->GetTrias() < trias)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot withdraw more than you have!");
        return;
    }

    if(guild)
    {
        psGuildInfo* g = character->GetGuild();
        if(!g)
            return;

        g->AdjustMoney(psMoney(-circles, -octas, -hexas, -trias));
        character->AdjustMoney(psMoney(circles, octas, hexas, trias), false);
    }
    else
    {
        character->AdjustMoney(psMoney(-circles, -octas, -hexas, -trias), true);
        character->AdjustMoney(psMoney(circles, octas, hexas, trias), false);
    }
}

void BankManager::ExchangeFunds(Client* client, bool guild, int coins, int coin)
{
    if(coins < 0)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot exchange a negative amount!");
        return;
    }

    psCharacter* character = client->GetCharacterData();

    int circlesToDeduct = 0;
    int octasToDeduct = 0;
    int hexasToDeduct = 0;
    int triasToDeduct = 0;

    // Convert the coins to tria, and add the fee.
    // Then convert, biggest possible coin first, into the other coins.
    // The Yliakum bank is nice and rounds down! :-)

    float fee = CalculateFee(character, guild);

    switch(coin)
    {
        case CIRCLE:
        {
            int coin2Tria = (int)((coins * 250) * (1 + fee/100));
            octasToDeduct = (coin2Tria / 50);
            coin2Tria = (coin2Tria % 50);
            hexasToDeduct = (coin2Tria / 10);
            triasToDeduct = (coin2Tria % 10);
            character->AdjustMoney(psMoney(coins, 0, 0, 0), false);
            break;
        }
        case OCTA:
        {
            int coin2Tria = (int)((coins * 50) * (1 + fee/100));
            circlesToDeduct = (coin2Tria / 250);
            coin2Tria = (coin2Tria % 250);
            hexasToDeduct = (coin2Tria / 10);
            triasToDeduct = (coin2Tria % 10);
            character->AdjustMoney(psMoney(0, coins, 0, 0), false);
            break;
        }
        case HEXA:
        {
            int coin2Tria = (int)((coins * 10) * (1 + fee/100));
            circlesToDeduct = (coin2Tria / 250);
            coin2Tria = (coin2Tria % 250);
            octasToDeduct = (coin2Tria / 50);
            triasToDeduct = (coin2Tria % 50);
            character->AdjustMoney(psMoney(0, 0, coins, 0), false);
            break;
        }
        case TRIA:
        {
            int coin2Tria = (int)(coins * (1 + fee/100));
            circlesToDeduct = (coin2Tria / 250);
            coin2Tria = (coin2Tria % 250);
            octasToDeduct = (coin2Tria / 50);
            hexasToDeduct = (coin2Tria % 50);
            character->AdjustMoney(psMoney(0, 0, 0, coins), false);
            break;
        }
        default:
            return;
    }

    if(guild)
    {
        psGuildInfo* guild = character->GetGuild();
        if(!guild)
            return;

        // Convert money into what we actually have and deduct.
        while(circlesToDeduct > guild->GetBankMoney().GetCircles())
        {
            if(coin != OCTA)
                octasToDeduct += 5;
            else
                hexasToDeduct += 25;
            circlesToDeduct--;
        }
        while(octasToDeduct > guild->GetBankMoney().GetOctas())
        {
            if(coin != HEXA)
                hexasToDeduct += 5;
            else
                triasToDeduct += 50;
            octasToDeduct--;
        }
        while(hexasToDeduct > guild->GetBankMoney().GetHexas())
        {
            triasToDeduct += 10;
            hexasToDeduct--;
        }
        // Coin is never TRIA at this point.
        if(triasToDeduct > guild->GetBankMoney().GetTrias())
        {
            // Start breaking down bigger coins.
            while(hexasToDeduct < guild->GetBankMoney().GetHexas() && coin != HEXA && triasToDeduct > guild->GetBankMoney().GetTrias())
            {
                triasToDeduct -= 10;
                hexasToDeduct++;
            }
            while(octasToDeduct < guild->GetBankMoney().GetOctas() && coin != OCTA && triasToDeduct > guild->GetBankMoney().GetTrias())
            {
                triasToDeduct -= 50;
                octasToDeduct++;
            }
            while(circlesToDeduct < guild->GetBankMoney().GetCircles() && coin != CIRCLE && triasToDeduct > guild->GetBankMoney().GetTrias())
            {
                triasToDeduct -=250;
                circlesToDeduct++;
            }
            // Tidy up any extra tria.
            if(triasToDeduct < 0)
            {
                while(triasToDeduct <= -50)
                {
                    octasToDeduct--;
                    triasToDeduct += 50;
                }
                while(triasToDeduct <= -10)
                {
                    hexasToDeduct--;
                    triasToDeduct += 10;
                }
            }
        }
        // Finally deduct and add the amounts needed.
        guild->AdjustMoney(psMoney(-circlesToDeduct, -octasToDeduct, -hexasToDeduct, -triasToDeduct));
    }
    else
    {
        // Convert money into what we actually have and deduct.
        while(circlesToDeduct > character->BankMoney().GetCircles())
        {
            if(coin != OCTA)
                octasToDeduct += 5;
            else
                hexasToDeduct += 25;
            circlesToDeduct--;
        }
        while(octasToDeduct > character->BankMoney().GetOctas())
        {
            if(coin != HEXA)
                hexasToDeduct += 5;
            else
                triasToDeduct += 50;
            octasToDeduct--;
        }
        while(hexasToDeduct > character->BankMoney().GetHexas())
        {
            triasToDeduct += 10;
            hexasToDeduct--;
        }
        // Coin is never TRIA at this point.
        if(triasToDeduct > character->BankMoney().GetTrias())
        {
            // Start breaking down bigger coins.
            while(hexasToDeduct < character->BankMoney().GetHexas() && coin != HEXA && triasToDeduct > character->BankMoney().GetTrias())
            {
                triasToDeduct -= 10;
                hexasToDeduct++;
            }
            while(octasToDeduct < character->BankMoney().GetOctas() && coin != OCTA && triasToDeduct > character->BankMoney().GetTrias())
            {
                triasToDeduct -= 50;
                octasToDeduct++;
            }
            while(circlesToDeduct < character->BankMoney().GetCircles() && coin != CIRCLE && triasToDeduct > character->BankMoney().GetTrias())
            {
                triasToDeduct -=250;
                circlesToDeduct++;
            }
            // Tidy up any extra tria.
            if(triasToDeduct < 0)
            {
                while(triasToDeduct <= -50)
                {
                    octasToDeduct--;
                    triasToDeduct += 50;
                }
                while(triasToDeduct <= -10)
                {
                    hexasToDeduct--;
                    triasToDeduct += 10;
                }
            }
        }
        // Finally deduct and add the amounts needed.
        character->AdjustMoney(psMoney(-circlesToDeduct, -octasToDeduct, -hexasToDeduct, -triasToDeduct), true);
    }
}

psMoney* BankManager::GetTotalFunds(psCharacter* pschar, bool guild)
{
    if(guild)
    {
        psGuildInfo* g = pschar->GetGuild();
        return g ? &g->GetBankMoney() : NULL;
    }
    else
    {
        return &pschar->BankMoney();
    }
}

void BankManager::SendBankWindow(Client* client, bool guild, bool forceOpen)
{
    psCharacter* pschar = client->GetActor()->GetCharacterData();

    // If no pschar then return. It shouldn't happen.
    if(!pschar)
        return;

    psMoney* moneyBanked = GetTotalFunds(pschar, guild);
    if(!moneyBanked)
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot access a guild bank account without being a member of a guild!");
        return;
    }

    // Check if the player has the permission to use the bank account of his guild.
    if(guild && !pschar->GetGuildMembership()->HasRights(RIGHTS_USE_BANK))
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot access your guild bank account without having the permission to use the account of your guild!");
        return;
    }

    float fee = CalculateFee(pschar, guild);

    // Calculate how many of each coin is available for exchange.
    int maxCircles = CoinsForExchange(pschar, guild, CIRCLE, fee);
    int maxOctas = CoinsForExchange(pschar, guild, OCTA, fee);
    int maxHexas = CoinsForExchange(pschar, guild, HEXA, fee);
    int maxTrias = CoinsForExchange(pschar, guild, TRIA, fee);

    psGUIBankingMessage newmsg(client->GetClientNum(),
                               psGUIBankingMessage::VIEWBANK,
                               guild,
                               pschar->Money().GetCircles(),
                               pschar->Money().GetOctas(),
                               pschar->Money().GetHexas(),
                               pschar->Money().GetTrias(),
                               moneyBanked->GetCircles(),
                               moneyBanked->GetOctas(),
                               moneyBanked->GetHexas(),
                               moneyBanked->GetTrias(),
                               maxCircles,
                               maxOctas,
                               maxHexas,
                               maxTrias,
                               fee,
                               forceOpen);

    Debug2(LOG_EXCHANGES, client->GetClientNum(),"Sending psGUIBankingMessage w/ stats to %d, Valid: ",int(client->GetClientNum()));
    if(newmsg.valid)
    {
        Debug1(LOG_EXCHANGES, client->GetClientNum(),"Yes\n");
        psserver->GetEventManager()->SendMessage(newmsg.msg);

    }
    else
    {
        Debug1(LOG_EXCHANGES, client->GetClientNum(),"No\n");
        Bug2("Could not create valid psGUIBankingMessage for client %u.\n",client->GetClientNum());
    }
}


void BankManager::HandleBanking(MsgEntry* me, Client* client)
{
    psGUIBankingMessage msg(me);
    if(!msg.valid)
    {
        Debug2(LOG_NET,me->clientnum,"Received unparsable psGUIBankingMessage from client %u.\n", me->clientnum);
        return;
    }
    // Check we're still near the banker.
    gemObject* banker = NULL;
    banker = client->GetTargetObject();
    if(!banker || !banker->GetCharacterData() || !banker->GetCharacterData()->IsBanker())
    {
        psserver->SendSystemError(client->GetClientNum(), "Your target must be a banker!");
        return;
    }

    // Check range
    if(client->GetActor()->RangeTo(banker) > RANGE_TO_SELECT)
    {
        psserver->SendSystemError(client->GetClientNum(),
                                  "You are not within range to interact with %s.",banker->GetCharacterData()->GetCharName());
        return;
    }

    // Check that the banker is alive!
    if(!banker->IsAlive())
    {
        psserver->SendSystemError(client->GetClientNum(), "You can't interact with a dead banker!");
        return;
    }

    // Check if the player has the permission to use the bank account of his guild.
    if(msg.guild && !client->GetCharacterData()->GetGuildMembership()->HasRights(RIGHTS_USE_BANK))
    {
        psserver->SendSystemError(client->GetClientNum(), "You cannot access your guild bank account without having the permission to use the account of your guild!");
        return;
    }

    // Make sure that we're not busy doing something else.
    if(client->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE)
    {
        csString err;
        err.Format("You can't access your bank account while %s.", client->GetActor()->GetModeStr());
        psserver->SendSystemError(client->GetClientNum(), err);
        return;
    }

    // Handle the message now.
    switch(msg.command)
    {
        case psGUIBankingMessage::WITHDRAWFUNDS:
        {
            WithdrawFunds(client, msg.guild, msg.circles, msg.octas, msg.hexas, msg.trias);
            SendBankWindow(client, msg.guild, false);
            break;
        }
        case psGUIBankingMessage::DEPOSITFUNDS:
        {
            DepositFunds(client, msg.guild, msg.circles, msg.octas, msg.hexas, msg.trias);
            SendBankWindow(client, msg.guild, false);
            break;
        }
        case psGUIBankingMessage::EXCHANGECOINS:
        {
            psCharacter* pschar = client->GetCharacterData();
            if(msg.coins > CoinsForExchange(pschar, msg.guild, msg.coin, CalculateFee(pschar, msg.guild)))
            {
                psserver->SendSystemError(client->GetClientNum(), "You cannot exchange this much!");
                SendBankWindow(client, msg.guild, false);
                return;
            }

            ExchangeFunds(client, msg.guild, msg.coins, msg.coin);
            SendBankWindow(client, msg.guild, false);
            break;
        }
    }
}

int BankManager::CoinsForExchange(psCharacter* pschar, bool guild, int type, float fee)
{
    psMoney* mon = GetTotalFunds(pschar, guild);

    switch(type)
    {
        case CIRCLE:
        {
            int totalAvail = (int)((mon->GetTotal() - mon->GetCircles() * 250) * (100-fee) / 100);
            return (totalAvail / 250);
        }
        case OCTA:
        {
            int totalAvail = (int)((mon->GetTotal() - mon->GetOctas() * 50) * (100-fee) / 100);
            return (totalAvail / 50);
        }
        case HEXA:
        {
            int totalAvail = (int)((mon->GetTotal() - mon->GetHexas() * 10) * (100-fee) / 100);
            return (totalAvail / 10);
        }
        case TRIA:
        {
            int totalAvail = (int)((mon->GetTotal() - mon->GetTrias()) * (100-fee) / 100);
            return totalAvail;
        }
    }

    return 0;
}

int BankManager::CalculateAccountLevel(psCharacter* pschar, bool guild)
{
    MathScript* script;
    MathEnvironment env;

    if(guild)
    {
        psGuildInfo* g = pschar->GetGuild();
        if(!g)
        {
            return 0;
        }

        if(!accountGuildLvlScript)
        {
            return 0;
        }
        script = accountGuildLvlScript;
        env.Define("TotalTrias", g->GetBankMoney().GetTotal());
    }
    else
    {
        if(!accountCharLvlScript)
        {
            return 0;
        }
        script = accountCharLvlScript;
        env.Define("TotalTrias", pschar->BankMoney().GetTotal());
    }

    script->Evaluate(&env);
    MathVar* accountLevel = env.Lookup("AccountLevel");

    return accountLevel->GetRoundValue();
}

float BankManager::CalculateFee(psCharacter* pschar, bool guild)
{
    if(!CalcBankFeeScript)
    {
        return 0;
    }

    MathEnvironment env;
    env.Define("AccountLevel", CalculateAccountLevel(pschar, guild));
    CalcBankFeeScript->Evaluate(&env);
    MathVar* bankFee = env.Lookup("BankFee");
    return bankFee->GetValue();
}

