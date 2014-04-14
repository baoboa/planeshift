/*
 * pawsstoragewindow.cpp -  Stefano Angeleri <weltall2@gmail.com>
 *
 * Copyright (C) 2010 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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
#include "pawsstoragewindow.h"
#include "inventorywindow.h"
#include "pawsslot.h"

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  GUI BUTTON IDENTIFIERS
/////////////////////////////////////////////////////////////////////////////
#define CATEGORY_LIST         10
#define ITEM_LIST             11
#define EXCHANGE_AMT          100
#define EXCHANGE_ALL          110
#define EXCHANGE_SINGLE       120
#define VIEW_ITEM             150
#define WITHDRAW_RADIO_BUTTON 1000
#define STORE_RADIO_BUTTON    2000
/////////////////////////////////////////////////////////////////////////////


/** Returns text of listbox row column of type pawsTextBox */
csString pawsStorageWindow::GetColumnText(pawsListBoxRow* row, int colNum)
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
void pawsStorageWindow::SetColumnText(pawsListBoxRow* row, int colNum, const csString & text)
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
*                        class pawsStorageWindow
*
******************************************************************************/
pawsStorageWindow::pawsStorageWindow()
{
}

pawsStorageWindow::~pawsStorageWindow()
{
}


bool pawsStorageWindow::PostSetup()
{
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_GUISTORAGE);

    categoryBox = (pawsListBox*)FindWidget("Categories");
    itemsBox    = (pawsListBox*)FindWidget("Items");
    trias       = (pawsTextBox*)FindWidget("TotalTrias");

    return true;
}

void pawsStorageWindow::HandleMessage( MsgEntry* me )
{
    psGUIStorageMessage incoming( me );

    switch ( incoming.command )
    {
        case psGUIStorageMessage::STORAGE:
        {
            HandleStorage ( incoming.commandData ); 
            Show();
            return;
        }

        case psGUIStorageMessage::CATEGORIES:
        {
            HandleCategories( incoming.commandData );
            Show();
            return;
        }

        case psGUIStorageMessage::ITEMS:
        {
            HandleItems( incoming.commandData );
            Show();
            return;
        }
        
        case psGUIStorageMessage::MONEY:
        {
            HandleMoney(incoming.commandData);
            return;
        }
    }
}




void pawsStorageWindow::HandleMoney( const char* moneyData )
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


void pawsStorageWindow::UpdateMoney( const char* moneyName, const char* imageName, int value )
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


void pawsStorageWindow::HandleStorage( const char* merchantData )
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
    csRef<iDocumentNode> data = merchant->GetNode("STORAGE");
    if(!data)
    {
        Error2("No <STORAGE> tag in %s", merchantData);
        return;
    }

    merchantID   = data->GetAttributeValueAsInt( "ID" );
    tradeCommand = data->GetAttributeValueAsInt( "TRADE_CMD" );
    
    bool withdraw = true; 
    if ( tradeCommand == psGUIStorageMessage::STORE )
    {
        withdraw = false;
    }

    pawsRadioButtonGroup* group = (pawsRadioButtonGroup*)FindWidget("WithdrawStore");
    if (withdraw)
        group->SetActive( "Withdraw" );
    else 
        group->SetActive( "Store" );
        
}


void pawsStorageWindow::HandleCategories( const char* data )
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

void pawsStorageWindow::HandleItems( const char* data )
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

void pawsStorageWindow::TradeSelectedItem(bool all, bool single)
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

void pawsStorageWindow::OnListAction( pawsListBox* widget, int status )
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

            psGUIStorageMessage outgoing(psGUIStorageMessage::CATEGORY, commandData);
            outgoing.SendMessage();
        }
    }
    else if ( widget->GetID()==ITEM_LIST  &&  status==LISTBOX_SELECTED )
    {
        // Emulate VIEW button
        OnButtonPressed(1,0,FindWidget(VIEW_ITEM));
    }
}

void pawsStorageWindow::Close()
{
    csString commandData;
                           
    commandData.Format("<C ID=\"%d\"/>", merchantID);
    psGUIStorageMessage outgoing(psGUIStorageMessage::CANCEL, commandData);
    outgoing.SendMessage();

    // Clean the boxes
    categoryBox->Clear();
    categoryBox->Select(NULL);
    itemsBox->Clear();
    itemsBox->Select(NULL);
    Hide();
}


bool pawsStorageWindow::OnButtonPressed(int /*mouseButton*/, int /*keyModifier*/, pawsWidget* widget)
{
    switch ( widget->GetID() )
    {

        ////////////////////////////////////////////////////////////////
        // These two are for the radio buttons
        ////////////////////////////////////////////////////////////////
        case WITHDRAW_RADIO_BUTTON:
        {
            SetTradeMode( true );
            return true;
        }
        case STORE_RADIO_BUTTON:
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
        case EXCHANGE_AMT:
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

                psGUIStorageMessage outgoing(psGUIStorageMessage::VIEW, commandData);
                outgoing.SendMessage();
            }
            return true;
        }
    }

    return false;
}

void pawsStorageWindow::SetTradeMode( bool withdraw )
{
    if (withdraw)
    {
        psGUIStorageMessage exchange(psGUIStorageMessage::REQUEST,"<R TYPE=\"WITHDRAW\"/>");
        exchange.SendMessage();
    }
    else
    {
        psGUIStorageMessage exchange(psGUIStorageMessage::REQUEST,"<R TYPE=\"STORE\"/>");
        exchange.SendMessage();
    }
    itemsBox->Clear();
    categoryBox->Clear();
}

void pawsStorageWindow::DoTrade(int count,const char* itemName,const char* itemID)
{
    char commandData[200];
    csString escpxml = EscpXML(itemName);
    sprintf(commandData, "<T ID=\"%d\" ITEM=\"%s\" COUNT=\"%d\" ITEM_ID=\"%s\" />", 
            merchantID, escpxml.GetData(), count, itemID);
    psGUIStorageMessage outgoing(tradeCommand, commandData);
    outgoing.SendMessage();
    Error2("%s", commandData);
}

void pawsStorageWindow::OnNumberEntered(const char* /*name*/, int /*param*/, int value)
{
    if (value == -1)
        return;

    DoTrade(value,currentItem,currentID);
}
