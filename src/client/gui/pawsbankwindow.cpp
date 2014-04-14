/*
* pawsbankwindow.cpp - Author: Mike Gist
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
#include "globals.h"

#include <csutil/csstring.h>

#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "pawsbankwindow.h"

/* Widget ID's */
#define MONEYBUTTON   1000
#define ADMINBUTTON   1002
#define WITHDRAW      1101
#define DEPOSIT       1102
#define EXCHANGE      1103
#define CIRCLESBUTTON 1104
#define OCTASBUTTON   1105
#define HEXASBUTTON   1106
#define TRIASBUTTON   1107

pawsBankWindow::pawsBankWindow()
{
}

pawsBankWindow::~pawsBankWindow()
{
    psengine->GetMsgHandler()->Unsubscribe(this, MSGTYPE_BANKING);
}

bool pawsBankWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_BANKING);

    Money = (pawsButton*)(FindWidget("MoneyButton"));
    if(!Money)
        return false;
    Admin = (pawsButton*)(FindWidget("AdminButton"));
    if(!Admin)
        return false;

    moneyWindow = FindWidget("MoneyWindow");
    if(!moneyWindow)
        return false;
    adminWindow = FindWidget("AdminWindow");
    if(!adminWindow)
        return false;

    circlesCanWithdraw = dynamic_cast <pawsTextBox*> (FindWidget("CirclesCanWithdraw"));
    if(!circlesCanWithdraw)
        return false;
    octasCanWithdraw = dynamic_cast <pawsTextBox*> (FindWidget("OctasCanWithdraw"));
    if(!octasCanWithdraw)
        return false;
    hexasCanWithdraw = dynamic_cast <pawsTextBox*> (FindWidget("HexasCanWithdraw"));
    if(!hexasCanWithdraw)
        return false;
    triasCanWithdraw = dynamic_cast <pawsTextBox*> (FindWidget("TriasCanWithdraw"));
    if(!triasCanWithdraw)
        return false;
    circlesCanDeposit = dynamic_cast <pawsTextBox*> (FindWidget("CirclesCanDeposit"));
    if(!circlesCanDeposit)
        return false;
    octasCanDeposit = dynamic_cast <pawsTextBox*> (FindWidget("OctasCanDeposit"));
    if(!octasCanDeposit)
        return false;
    hexasCanDeposit = dynamic_cast <pawsTextBox*> (FindWidget("HexasCanDeposit"));
    if(!hexasCanDeposit)
        return false;
    triasCanDeposit = dynamic_cast <pawsTextBox*> (FindWidget("TriasCanDeposit"));
    if(!triasCanDeposit)
        return false;
    circlesToWithdraw = dynamic_cast <pawsEditTextBox*> (FindWidget("CirclesToWithdraw"));
    if(!circlesToWithdraw)
        return false;
    circlesToWithdraw->SetText("0");
    octasToWithdraw = dynamic_cast <pawsEditTextBox*> (FindWidget("OctasToWithdraw"));
    if(!octasToWithdraw)
        return false;
    octasToWithdraw->SetText("0");
    hexasToWithdraw = dynamic_cast <pawsEditTextBox*> (FindWidget("HexasToWithdraw"));
    if(!hexasToWithdraw)
        return false;
    hexasToWithdraw->SetText("0");
    triasToWithdraw = dynamic_cast <pawsEditTextBox*> (FindWidget("TriasToWithdraw"));
    if(!triasToWithdraw)
        return false;
    triasToWithdraw->SetText("0");
    circlesToDeposit = dynamic_cast <pawsEditTextBox*> (FindWidget("CirclesToDeposit"));
    if(!circlesToDeposit)
        return false;
    circlesToDeposit->SetText("0");
    octasToDeposit = dynamic_cast <pawsEditTextBox*> (FindWidget("OctasToDeposit"));
    if(!octasToDeposit)
        return false;
    octasToDeposit->SetText("0");
    hexasToDeposit = dynamic_cast <pawsEditTextBox*> (FindWidget("HexasToDeposit"));
    if(!hexasToDeposit)
        return false;
    hexasToDeposit->SetText("0");
    triasToDeposit = dynamic_cast <pawsEditTextBox*> (FindWidget("TriasToDeposit"));
    if(!triasToDeposit)
        return false;
    triasToDeposit->SetText("0");
    circlesCanExchange = dynamic_cast <pawsTextBox*> (FindWidget("CirclesCanExchange"));
    if(!circlesCanExchange)
        return false;
    octasCanExchange = dynamic_cast <pawsTextBox*> (FindWidget("OctasCanExchange"));
    if(!octasCanExchange)
        return false;
    hexasCanExchange = dynamic_cast <pawsTextBox*> (FindWidget("HexasCanExchange"));
    if(!hexasCanExchange)
        return false;
    triasCanExchange = dynamic_cast <pawsTextBox*> (FindWidget("TriasCanExchange"));
    if(!triasCanExchange)
        return false;
    coinsToExchange = dynamic_cast <pawsEditTextBox*> (FindWidget("CoinsToExchange"));
    if(!coinsToExchange)
        return false;
    coinsToExchange->SetText("0");
  
    circles = (pawsRadioButton*)(FindWidget("Circles"));
    octas = (pawsRadioButton*)(FindWidget("Octas"));
    hexas = (pawsRadioButton*)(FindWidget("Hexas"));
    trias = (pawsRadioButton*)(FindWidget("Trias"));

    feeInfo = dynamic_cast <pawsTextBox*> (FindWidget("MoneyExchangeInfo"));
    if(!feeInfo)
        return false;

    moneyWindow->SetAlwaysOnTop(true);
    adminWindow->SetAlwaysOnTop(true);
    moneyWindow->Show();
    Money->SetState(true);

    return true;
}

void pawsBankWindow::HandleMessage( MsgEntry* me )
{
    if(me->GetType() == MSGTYPE_BANKING)
    {
        psGUIBankingMessage incoming(me);

        switch(incoming.command)
        {
        case psGUIBankingMessage::VIEWBANK:
            {
                if(!IsVisible() && incoming.openWindow) 
                {
                    Show();
                }
                // Is there a better way to do this? Enlighten me.
                csString circles;
                csString octas;
                csString hexas;
                csString trias;
                csString circlesBanked;
                csString octasBanked;
                csString hexasBanked;
                csString triasBanked;
                csString maxCircles;
                csString maxOctas;
                csString maxHexas;
                csString maxTrias;
                csString fInfo;
                circlesCanDeposit->SetText(circles.AppendFmt("%i", incoming.circles));
                octasCanDeposit->SetText(octas.AppendFmt("%i", incoming.octas));
                hexasCanDeposit->SetText(hexas.AppendFmt("%i", incoming.hexas));
                triasCanDeposit->SetText(trias.AppendFmt("%i", incoming.trias));
                circlesCanWithdraw->SetText(circlesBanked.AppendFmt("%i", incoming.circlesBanked));
                octasCanWithdraw->SetText(octasBanked.AppendFmt("%i", incoming.octasBanked));
                hexasCanWithdraw->SetText(hexasBanked.AppendFmt("%i", incoming.hexasBanked));
                triasCanWithdraw->SetText(triasBanked.AppendFmt("%i", incoming.triasBanked));
                circlesCanExchange->SetText(maxCircles.AppendFmt("%i", incoming.maxCircles));
                octasCanExchange->SetText(maxOctas.AppendFmt("%i", incoming.maxOctas));
                hexasCanExchange->SetText(maxHexas.AppendFmt("%i", incoming.maxHexas));
                triasCanExchange->SetText(maxTrias.AppendFmt("%i", incoming.maxTrias));
                feeInfo->SetText(fInfo.AppendFmt("Fee charged for your account level: %.2f%%", incoming.exchangeFee));

                guild = incoming.guild;
                break;
            }
        }
    }
}

bool pawsBankWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    if(widget->GetID() == MONEYBUTTON)
    {
        Money->SetState(true);
        Admin->SetState(false);

        moneyWindow->Show();
        adminWindow->Hide();
    }

    if(widget->GetID() == ADMINBUTTON)
    {
        Money->SetState(false);
        Admin->SetState(true);

        moneyWindow->Hide();
        adminWindow->Show();
    }

    if(widget->GetID() == CIRCLESBUTTON)
    {
        octas->SetState(false);
        hexas->SetState(false);
        trias->SetState(false);
    }

    if(widget->GetID() == OCTASBUTTON)
    {
        circles->SetState(false);
        hexas->SetState(false);
        trias->SetState(false);
    }

    if(widget->GetID() == HEXASBUTTON)
    {
        circles->SetState(false);
        octas->SetState(false);
        trias->SetState(false);
    }

    if(widget->GetID() == TRIASBUTTON)
    {
        circles->SetState(false);
        octas->SetState(false);
        hexas->SetState(false);
    }

    if(widget->GetID() == WITHDRAW)
    {
        // Send request to server.
        int circles = 0;
        sscanf(circlesToWithdraw->GetText(), "%d", &circles);

        int octas = 0;
        sscanf(octasToWithdraw->GetText(), "%d", &octas);

        int hexas = 0;
        sscanf(hexasToWithdraw->GetText(), "%d", &hexas);

        int trias = 0;
        sscanf(triasToWithdraw->GetText(), "%d", &trias);

        psGUIBankingMessage outgoing(psGUIBankingMessage::WITHDRAWFUNDS,
                                     guild, circles, octas, hexas, trias);
        outgoing.SendMessage();

        // Reset to 0.
        circlesToWithdraw->SetText("0");
        octasToWithdraw->SetText("0");
        hexasToWithdraw->SetText("0");
        triasToWithdraw->SetText("0");

        return true;
    }

    if(widget->GetID() == DEPOSIT)
    {
        // Send request to server.
        int circles = 0;
        sscanf(circlesToDeposit->GetText(), "%d", &circles);

        int octas = 0;
        sscanf(octasToDeposit->GetText(), "%d", &octas);

        int hexas = 0;
        sscanf(hexasToDeposit->GetText(), "%d", &hexas);

        int trias = 0;
        sscanf(triasToDeposit->GetText(), "%d", &trias);


        psGUIBankingMessage outgoing(psGUIBankingMessage::DEPOSITFUNDS,
                                     guild, circles, octas, hexas, trias);
        outgoing.SendMessage();

        // Reset to 0.
        circlesToDeposit->SetText("0");
        octasToDeposit->SetText("0");
        hexasToDeposit->SetText("0");
        triasToDeposit->SetText("0");

        return true;
    }

    if(widget->GetID() == EXCHANGE)
    {
        // Send request to server.
        int coins = 0;
        sscanf(coinsToExchange->GetText(), "%d", &coins);

        int coin = -1;
        if(circles->GetState())
        {
            coin = 0;
            circles->SetState(false);
        }
        else if(octas->GetState())
        {
            coin = 1;
            octas->SetState(false);
        }
        else if(hexas->GetState())
        {
            coin = 2;
            hexas->SetState(false);
        }
        else if(trias->GetState())
        {
            coin = 3;
            trias->SetState(false);
        }

        if(coin != -1)
        {
            psGUIBankingMessage outgoing(psGUIBankingMessage::EXCHANGECOINS,
                                         guild, coins, coin);
            outgoing.SendMessage();
        }

        // Reset to 0.
        coinsToExchange->SetText("0");

        return true;
    }

    return false;
}
