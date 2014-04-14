/*
 * pawsmerchantwindow.cpp -  Anders Reggestad <andersr@pvv.org>
 *                        -  PAWS version Andrew Craig <acraig@paqrat.com>
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

////////////////////////////////////////////////////////////////////////////
//  COMMON INCLUDES
/////////////////////////////////////////////////////////////////////////////
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "rpgrules/psmoney.h"
#include "util/psxmlparser.h"

/////////////////////////////////////////////////////////////////////////////
//  CLIENT INCLUDES
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  PAWS INCLUDES
/////////////////////////////////////////////////////////////////////////////
#include "paws/pawsmanager.h"
#include "paws/pawsradio.h"
#include "paws/pawslistbox.h"
#include "pawsmerchantwindow.h"
#include "inventorywindow.h"
#include "pawsslot.h"

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  GUI BUTTON IDENTIFIERS
/////////////////////////////////////////////////////////////////////////////
#define CATEGORY_LIST     10
#define ITEM_LIST         11
#define DO_SALE           100
#define EXCHANGE_ALL      110
#define EXCHANGE_SINGLE   120
#define VIEW_ITEM         150
#define BUY_RADIO_BUTTON  1000
#define SELL_RADIO_BUTTON 2000
/////////////////////////////////////////////////////////////////////////////


/** Returns text of listbox row column of type pawsTextBox */
csString GetColumnText(pawsListBoxRow* row, int colNum)
{
    pawsWidget * colAsWidget;
    pawsTextBox * colAsTextBox;

    colAsWidget = row->GetColumn(colNum);
    if (colAsWidget == NULL) return "";
    colAsTextBox = dynamic_cast <pawsTextBox*> (colAsWidget);
    if (colAsTextBox == NULL) return "";
    
    return colAsTextBox->GetText();
}

/** Sets text of listbox row column of type pawsTextBox */
void SetColumnText(pawsListBoxRow* row, int colNum, const csString & text)
{
    pawsWidget * colAsWidget;
    pawsTextBox * colAsTextBox;

    colAsWidget = row->GetColumn(colNum);
    if (colAsWidget == NULL) return;
    colAsTextBox = dynamic_cast <pawsTextBox*> (colAsWidget);
    if (colAsTextBox == NULL) return;
    
    colAsTextBox->SetText(text);
}

/*****************************************************************************
*
*                        class pawsMerchantWindow
*
******************************************************************************/
pawsMerchantWindow::pawsMerchantWindow():
    merchantID(0),tradeCommand(0),selectedItem(0),
    categoryBox(NULL),itemsBox(NULL),trias(NULL)
{
}

pawsMerchantWindow::~pawsMerchantWindow()
{
}


bool pawsMerchantWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GUIMERCHANT);

    categoryBox = (pawsListBox*)FindWidget("Categories");
    itemsBox    = (pawsListBox*)FindWidget("Items");
    trias       = (pawsTextBox*)FindWidget("TotalTrias");

    return true;
}

void pawsMerchantWindow::HandleMessage( MsgEntry* me )
{
    psGUIMerchantMessage incomming( me );

    //    printf("pawsMerchantWindow::HandleMessage %d %s\n",
    //           incomming.command,(const char*)incomming.commandData);

    switch ( incomming.command )
    {
        case psGUIMerchantMessage::MERCHANT:
        {
            HandleMerchant ( incomming.commandData ); 
            Show();
            return;
        }

        case psGUIMerchantMessage::CATEGORIES:
        {
            HandleCategories( incomming.commandData );
            Show();
            return;
        }

        case psGUIMerchantMessage::ITEMS:
        {
            HandleItems( incomming.commandData );
            Show();
            return;
        }
        
        case psGUIMerchantMessage::MONEY:
        {
            HandleMoney(incomming.commandData);
            return;
        }
    }
}




void pawsMerchantWindow::HandleMoney( const char* moneyData )
{
    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDocument> doc = xml->CreateDocument();

    const char* error = doc->Parse(moneyData );
    if ( error )
    {
        Error3("ERROR: %s in %s", error, moneyData );
        return;
    }
    csRef<iDocumentNode> merchant = doc->GetRoot();
    if(!merchant)
    {
        Error2("No XML Root in %s", moneyData);
        return;
    }

    csRef<iDocumentNode> data = merchant->GetNode("M");
    if(!data)
    {
        Error2("No <M> tag in %s", moneyData);
        return;
    }



    csString moneyString = data->GetAttributeValue("MONEY");

    psMoney money(moneyString);

    UpdateMoney("circles","MoneyCircles",  money.GetCircles());
    UpdateMoney("octas",  "MoneyOctas",    money.GetOctas());
    UpdateMoney("hexas",  "MoneyHexas",    money.GetHexas());
    UpdateMoney("trias",  "MoneyTrias",    money.GetTrias());
    
    PawsManager::GetSingleton().Publish( "sigInvMoney", money.ToString() );         
    PawsManager::GetSingleton().Publish( "sigInvMoneyTotal", money.GetTotal() );

    
    trias->SetText(csString().Format("%d Trias",money.GetTotal()));    
}


void pawsMerchantWindow::UpdateMoney( const char* moneyName, const char* imageName, int value )
{
    pawsSlot* itemSlot =  (pawsSlot*)FindWidget(moneyName);
    
    if ( itemSlot != NULL )
    {
        if (value)
        {            
            itemSlot->PlaceItem(imageName, "");
            itemSlot->StackCount(value);            
            itemSlot->DrawStackCount(true);
        } 
        else
        {
            itemSlot->Clear();
        }
    }
}


void pawsMerchantWindow::HandleMerchant( const char* merchantData )
{
    Show();

    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem > ( PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDocument> doc = xml->CreateDocument();
    if(!doc)
    {
        Error2("Parse error in %s", merchantData );
        return;
    }

    doc->Parse( merchantData );
    csRef<iDocumentNode> merchant = doc->GetRoot();
    if(!merchant)
    {
        Error2("No XML root in %s", merchantData);
        return;
    }
    csRef<iDocumentNode> data = merchant->GetNode("MERCHANT");
    if(!data)
    {
        Error2("No <MERCHANT> tag in %s", merchantData);
        return;
    }

    merchantID   = data->GetAttributeValueAsInt( "ID" );
    tradeCommand = data->GetAttributeValueAsInt( "TRADE_CMD" );
    
    bool buy = true; 
    if ( tradeCommand == psGUIMerchantMessage::SELL )
    {
        buy = false;
    }

    pawsRadioButtonGroup* group = (pawsRadioButtonGroup*)FindWidget("BuySell");
    if ( buy )
        group->SetActive( "Buy" );
    else 
        group->SetActive( "Sell" );
        
}


void pawsMerchantWindow::HandleCategories( const char* data )
{
    categoryBox->Clear();

    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem> (PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDocument> catList  = xml->CreateDocument();

    const char* error = catList->Parse( data );
    if ( error )
    {
        Error3("Error in XML: %sin %s", error, data );
        return;
    }

    csRef<iDocumentNode> root = catList->GetRoot();
    if (!root)
    {
        Error2("No XML root in %s", data);
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("L");
    if(!topNode)
    {
        Error2("No <L> tag in %s", data);
        return;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();
    
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> category = iter->Next();
        
		if ( !category )
		{
			Error2("No proper category in  %s", data);
			return;
		}
        pawsListBoxRow* row = categoryBox->NewRow();
        row->SetName( category->GetAttributeValue("NAME") );
        SetColumnText(row, 0, category->GetAttributeValue("NAME") );
    } 
    categoryBox->SortRows();
}

void pawsMerchantWindow::HandleItems( const char* data )
{
    itemsBox->Clear();
    csRef<iDocumentSystem> xml =  csQueryRegistry<iDocumentSystem> (PawsManager::GetSingleton().GetObjectRegistry());

    csRef<iDocument> itemList  = xml->CreateDocument();

    const char* error = itemList->Parse( data );
    if ( error )
    {
        Error3("Error in XML: %sin %s", error, data );
        return;
    }

    csRef<iDocumentNode> root = itemList->GetRoot();
    if(!root)
    {
        Error2("No XML root in %s", data);
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("L");
    if (!topNode)
    {
        Error2("No <L> tag in %s", data);
        return;
    }
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes();
    
    while ( iter->HasNext() )
    {
        csRef<iDocumentNode> item = iter->Next();

        csString name = item->GetAttributeValue("NAME");

        pawsListBoxRow* row = itemsBox->NewRow();
        SetColumnText(row, 0, item->GetAttributeValue("COUNT") );
        SetColumnText(row, 2, item->GetAttributeValue("PRICE") );
        SetColumnText(row, 4, item->GetAttributeValue("ID") );
        SetColumnText(row, 5, name );

        if (item->GetAttributeValueAsBool("PURIFIED"))
            name += " (Purified)";

        SetColumnText(row, 3, name );
        
        pawsWidget* widget = row->GetColumn(1);
        if (widget != NULL)
            widget->SetBackground( item->GetAttributeValue("IMG") );
    }
    itemsBox->SortRows();
    if (itemsBox->GetRowCount() <= (size_t)selectedItem)
        selectedItem=itemsBox->GetRowCount()-1;
    itemsBox->SelectByIndex(selectedItem);
}

void pawsMerchantWindow::TradeSelectedItem(bool all, bool single)
{
    pawsListBoxRow* row = itemsBox->GetSelectedRow();
    selectedItem = itemsBox->GetSelection();
    if ( row == NULL )
	{
        return;
	}
    // If the merchant only has 1 item or if we want to trade all, no need to request the count from the user
    if (atoi(GetColumnText(row, 0)) == 1 || all || single)
    {
            DoTrade(single?1:atoi(GetColumnText(row, 0)),GetColumnText(row, 5), GetColumnText(row, 4));
            return;
    }

    currentItem = GetColumnText(row,5);
    currentID   = GetColumnText(row,4);

    pawsNumberPromptWindow::Create("Enter count", -1, 1, atoi(GetColumnText(row, 0)),
                                   this, "Count" );
}

void pawsMerchantWindow::OnListAction( pawsListBox* widget, int status )
{
    if ( widget->GetID()==CATEGORY_LIST  &&  status==LISTBOX_HIGHLIGHTED )
    {
        /* This is from the category list.  So we get the name of the 
        * category and send it back to the server so it can send a list of the 
        * items the merchant has.
        */

        {
            pawsListBoxRow* rowWidget = widget->GetSelectedRow();

            pawsTextBox* nameWidget = (pawsTextBox*)(rowWidget->GetColumn(0));

            csString commandData;
            commandData.Format("<C ID=\"%d\" CATEGORY=\"%s\" />", merchantID,
                               EscpXML(nameWidget->GetText()).GetData());

            psGUIMerchantMessage outgoing(psGUIMerchantMessage::CATEGORY, commandData);
            outgoing.SendMessage();
        }
    }
    else if ( widget->GetID()==ITEM_LIST  &&  status==LISTBOX_SELECTED )
    {
        // Emulate VIEW button
        OnButtonPressed(1,0,FindWidget(VIEW_ITEM));
    }
}

void pawsMerchantWindow::Close()
{
    csString commandData;
                           
    commandData.Format("<C ID=\"%d\"/>", merchantID);
    psGUIMerchantMessage outgoing(psGUIMerchantMessage::CANCEL, commandData);
    outgoing.SendMessage();

    // Clean the boxes
    categoryBox->Clear();
    categoryBox->Select(NULL);
    itemsBox->Clear();
    itemsBox->Select(NULL);
    Hide();
}


bool pawsMerchantWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {

        ////////////////////////////////////////////////////////////////
        // These two are for the radio buttons
        ////////////////////////////////////////////////////////////////
        case BUY_RADIO_BUTTON:
        {
            SetTradeMode( true );
            return true;
        }
        case SELL_RADIO_BUTTON:
        {
            SetTradeMode( false );
            return true;
        }

        ////////////////////////////////////////////////////////////////
        // These two are for to do the transaction or cancel it
        ////////////////////////////////////////////////////////////////
		case EXCHANGE_SINGLE:
		{
			TradeSelectedItem(false,true);
			return true;
		}
		case EXCHANGE_ALL:
		{
			TradeSelectedItem(true);
			return true;
		}
        case DO_SALE:
        {
            TradeSelectedItem();
            return true;
        }
        
        case VIEW_ITEM:
        {
            pawsListBoxRow* row = itemsBox->GetSelectedRow();
            if ( row )
            {                
                char commandData[200];
                csString escpxml = EscpXML (GetColumnText(row, 2));
                csString cmdtxt = GetColumnText(row, 4);
                sprintf(commandData, "<V ID=\"%d\" TRADE_CMD=\"%d\" ITEM=\"%s\" ITEM_ID=\"%s\" />", 
                        merchantID, tradeCommand, escpxml.GetData(), cmdtxt.GetData());

                psGUIMerchantMessage outgoing(psGUIMerchantMessage::VIEW, commandData);
                outgoing.SendMessage();
            }
            return true;
        }
    }

    return false;
}

void pawsMerchantWindow::SetTradeMode( bool buy )
{
    if (buy)
    {
        psGUIMerchantMessage exchange(psGUIMerchantMessage::REQUEST,"<R TYPE=\"BUY\"/>");
        exchange.SendMessage();
    }
    else
    {
        psGUIMerchantMessage exchange(psGUIMerchantMessage::REQUEST,"<R TYPE=\"SELL\"/>");
        exchange.SendMessage();
    }
    itemsBox->Clear();
    categoryBox->Clear();
}

void pawsMerchantWindow::DoTrade(int count,const char* itemName,const char* itemID)
{
    char commandData[200];
    csString escpxml = EscpXML(itemName);
    sprintf(commandData, "<T ID=\"%d\" ITEM=\"%s\" COUNT=\"%d\" ITEM_ID=\"%s\" />", 
            merchantID, escpxml.GetData(), count, itemID);
    psGUIMerchantMessage outgoing(tradeCommand, commandData);
    outgoing.SendMessage();
    Error2("%s", commandData);
}

void pawsMerchantWindow::OnNumberEntered(const char* /*name*/, int /*param*/, int value)
{
    if (value == -1)
        return;

    DoTrade(value,currentItem,currentID);
}
