/*
* pawsbankwindow.h - Author: Mike Gist
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

#ifndef __PAWS_BANK_WINDOW_H__
#define __PAWS_BANK_WINDOW_H__

#include "net/cmdbase.h"
#include "paws/pawswidget.h"
#include "paws/pawstextbox.h"
#include "paws/pawsbutton.h"
#include "paws/pawslistbox.h"
#include "paws/pawsradio.h"

/** The bank window in PlaneShift.
 * This is the window that people will use to interact with their bank accounts.
 */
class pawsBankWindow : public pawsWidget, public psClientNetSubscriber
{
public:
    pawsBankWindow();
    virtual ~pawsBankWindow();

    /* From pawsWidget */
    bool PostSetup();
    bool OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget );
    
    /* From iNetSubscriber */
    void HandleMessage( MsgEntry* me );
    
protected:

    /* Text that shows the total amount of money available to withdraw. */
    pawsTextBox *circlesCanWithdraw, *octasCanWithdraw, *hexasCanWithdraw, *triasCanWithdraw;

    /* Text that shows the total amount of money available to deposit. */
    pawsTextBox *circlesCanDeposit, *octasCanDeposit, *hexasCanDeposit, *triasCanDeposit;

    /* Text that shows the maximum amount of each coin available to exchange. */
    pawsTextBox *circlesCanExchange, *octasCanExchange, *hexasCanExchange, *triasCanExchange;

    /* Box for specifying how much to withdraw. */
    pawsEditTextBox *circlesToWithdraw, *octasToWithdraw, *hexasToWithdraw, *triasToWithdraw;

    /* Box for specifying how much to deposit. */
    pawsEditTextBox *circlesToDeposit, *octasToDeposit, *hexasToDeposit, *triasToDeposit;

    /* Box for specifying how much to exchange. */
    pawsEditTextBox *coinsToExchange;

    /* Coin selection radio group */
    pawsRadioButtonGroup *coinSelect;

    /* Radio buttons for specifying which coin type to exchange. */
    pawsRadioButton *circles, *octas, *hexas, *trias;

    /* Fee info for coin conversions. */
    pawsTextBox *feeInfo;

    /* Tab buttons. */
    pawsButton *Money, *Admin;

    /* two sub-windows. */
    pawsWidget *moneyWindow, *adminWindow;

    /* Whether or not this is a guild bank account. */
    bool guild;
}; 

CREATE_PAWS_FACTORY( pawsBankWindow );

#endif // __PAWS_BANK_WINDOW_H__
