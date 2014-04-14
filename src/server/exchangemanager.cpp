/*
 * exchangemanager.cpp
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

#include <psconfig.h>
#include "exchangemanager.h"
//=============================================================================
// Crystal Space Includes
//=============================================================================

//=============================================================================
// Project Includes
//=============================================================================
#include "bulkobjects/pscharacterloader.h"
#include "bulkobjects/dictionary.h"
#include "bulkobjects/psitem.h"
#include "bulkobjects/psglyph.h"
#include "bulkobjects/psnpcdialog.h"

#include "util/log.h"
#include "util/psconst.h"
#include "util/psstring.h"
#include "util/eventmanager.h"
#include "util/psxmlparser.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "psserver.h"
#include "psserverchar.h"
#include "clients.h"
#include "gem.h"
#include "chatmanager.h"
#include "cachemanager.h"
#include "playergroup.h"
#include "invitemanager.h"
#include "globals.h"

/** Question client. Handles YesNo response of client for trade request. */
class PendingTradeInvite : public PendingInvite
{
public:
    PendingTradeInvite(Client* inviter, Client* invitee,
                       const char* question) : PendingInvite(inviter, invitee, true,
                                   question, "Accept", "Reject",
                                   "You ask %s to trade with you.",
                                   "%s asks to trade with you.",
                                   "%s agrees to trade with you.",
                                   "You agree to trade.",
                                   "%s does not want to trade.",
                                   "You refuse to trade with %s.",
                                   psQuestionMessage::generalConfirm)
    {

    }

    void HandleAnswer(const csString &answer)
    {
        Client* invitedClient = psserver->GetConnections()->Find(clientnum);
        if(!invitedClient)
            return;

        PendingInvite::HandleAnswer(answer);

        Client* inviterClient = psserver->GetConnections()->Find(inviterClientNum);
        if(!inviterClient)
            return;

        psCharacter* invitedCharData = invitedClient->GetCharacterData();
        psCharacter* inviterCharData = inviterClient->GetCharacterData();
        const char* invitedCharFirstName = invitedCharData->GetCharName();
        const char* inviterCharFirstName = inviterCharData->GetCharName();

        if(answer == "no")
        {
            // Inform proposer that the trade has been rejected
            csString message;
            message.Format("%s has rejected your trade offer.", invitedCharFirstName);
            psserver->SendSystemError(inviterClient->GetClientNum(),
                                      message.GetData());

            return;
        }

        // Ignore the answer if the invited client is busy with something else
        if(invitedClient->GetActor()->GetMode() != PSCHARACTER_MODE_PEACE && invitedClient->GetActor()->GetMode() != PSCHARACTER_MODE_SIT && invitedClient->GetActor()->GetMode() != PSCHARACTER_MODE_OVERWEIGHT)
        {
            csString err;
            err.Format("You can't trade while %s.", invitedClient->GetActor()->GetModeStr());
            psserver->SendSystemError(invitedClient->GetClientNum(), err);

            // Not telling the inviter what the invited client is doing...
            psserver->SendSystemError(inviterClient->GetClientNum(), "%s is already busy with something else.", invitedCharFirstName);
            return;
        }

        // Ignore the answer if the invited client is already in trade
        if(invitedClient->GetExchangeID())
        {
            psserver->SendSystemError(invitedClient->GetClientNum(), "You are already busy with another trade");
            psserver->SendSystemError(inviterClient->GetClientNum(), "%s is already busy with another trade", invitedCharFirstName);
            return;
        }
        // Ignore the answer if the inviter client is already in trade
        if(inviterClient->GetExchangeID())
        {
            psserver->SendSystemError(inviterClient->GetClientNum(), "You are already busy with another trade");
            psserver->SendSystemError(invitedClient->GetClientNum(), "%s is already busy with another trade", inviterCharFirstName);
            return;
        }

        Exchange* exchange = new PlayerToPlayerExchange(inviterClient, invitedClient,
                psserver->GetExchangeManager());

        // Check range
        if(!exchange->CheckRange(inviterClient->GetClientNum(),
                                 inviterClient->GetActor(), invitedClient->GetActor()))
        {
            psserver->SendSystemError(inviterClient->GetClientNum(),
                                      "%s is too far away to trade.", invitedCharFirstName);
            delete exchange;
            return;
        }

        // Overwrite client trading state
        bool oldTradingStopped = inviterCharData->SetTradingStopped(false);
        if(!inviterCharData->ReadyToExchange())
        {
            psserver->SendSystemInfo(inviterClient->GetClientNum(),
                                     "You are not ready to trade.");
            inviterCharData->SetTradingStopped(oldTradingStopped);
            delete exchange;
            return;
        }

        inviterCharData->SetTradingStopped(oldTradingStopped);

        if(!invitedCharData->ReadyToExchange())
        {
            psserver->SendSystemInfo(inviterClient->GetClientNum(),
                                     "Target is not ready to trade.");
            delete exchange;
            return;
        }

        inviterClient->SetExchangeID(exchange->GetID());
        invitedClient->SetExchangeID(exchange->GetID());

        psserver->GetExchangeManager()->AddExchange(exchange);
    }
};


/** Sends text message describing change of offers to user 'clientNum'.
  * otherName - name of the other character (the person that the user exchanges with)
  * object - item that was moved (e.g. "2x sword")
  * actionOfClient - did the user that we send the message to do the operation ? (or the other character?)
  * movedToOffer - was the object moved to offered stuff ? (or removed from offered stuff and returned to inventory?)
  */
/*************
void SendTextDescription(int clientNum, const csString & otherName, const csString & object, bool actionOfClient, bool movedToOffer)
{
    psString text;

    if (actionOfClient)
    {
        if (movedToOffer)
            text.AppendFmt("You have just offered %s to %s.", object.GetData(), otherName.GetData());
        else
            text.AppendFmt("You have just removed %s from offer.", object.GetData());
    }
    else
    {
        if (movedToOffer)
            text.AppendFmt("%s has just offered %s to you.", otherName.GetData(), object.GetData());
        else
            text.AppendFmt("%s has just removed %s from offer.", otherName.GetData(), object.GetData());
    }
    psserver->SendSystemInfo(clientNum, text);
}

csString MakeMoneyObject(int coin, int count)
{
    csString object;

    object = count;
    object += "x ";

    switch (coin)
    {
        case MONEY_TRIAS: object += "tria"; break;
        case MONEY_OCTAS: object += "octa"; break;
        case MONEY_HEXAS: object += "hexa"; break;
        case MONEY_CIRCLES: object += "circle"; break;
    }
    return object;
}

void SendTextDescriptionOfMoney(int clientNum, const csString & otherName, psMoney oldMoney, psMoney newMoney, bool actionOfClient)
{
    csString object;

    if (oldMoney.GetTrias() != newMoney.GetTrias())
        object = MakeMoneyObject(MONEY_TRIAS, abs(oldMoney.GetTrias() - newMoney.GetTrias()));
    else if (oldMoney.GetOctas() != newMoney.GetOctas())
        object = MakeMoneyObject(MONEY_OCTAS, abs(oldMoney.GetOctas() - newMoney.GetOctas()));
    else if (oldMoney.GetHexas() != newMoney.GetHexas())
        object = MakeMoneyObject(MONEY_HEXAS, abs(oldMoney.GetHexas() - newMoney.GetHexas()));
    else if (oldMoney.GetCircles() != newMoney.GetCircles())
        object = MakeMoneyObject(MONEY_CIRCLES, abs(oldMoney.GetCircles() - newMoney.GetCircles()));

    SendTextDescription(clientNum, otherName, object, actionOfClient, newMoney > oldMoney);
}
***************/




bool Exchange::CheckRange(int clientNum, gemObject* ourActor, gemObject* otherActor)
{
    if(ourActor->RangeTo(otherActor) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(clientNum, "You are not in range to trade with %s.", otherActor->GetName());
        return false;
    }
    return true;
}

/***********************************************************************************
*
*                              class ExchangingCharacter
*
***********************************************************************************/


ExchangingCharacter::ExchangingCharacter(int clientNum,psCharacterInventory &inv)
{
    client = clientNum;
    chrinv = &inv;
}

bool ExchangingCharacter::OfferItem(int exchSlotNum, INVENTORY_SLOT_NUMBER invSlot, int stackcount, bool test)
{
    if(exchSlotNum>=EXCHANGE_SLOT_COUNT)
        return false;

    psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(exchSlotNum);
    if(itemInSlot == NULL)  // slot is open
    {
        if(!test)
        {
            chrinv->SetExchangeOfferSlot(NULL,invSlot,exchSlotNum,stackcount);
        }
        return true;
    }
    else
    {
        return false;
    }
}


bool ExchangingCharacter::RemoveItemFromOffer(int exchSlotNum, int count, int &remain)
{
    if(exchSlotNum >= EXCHANGE_SLOT_COUNT)
        return false;

    psCharacterInventory::psCharacterInventoryItem* item = chrinv->FindExchangeSlotOffered(exchSlotNum);
    if(item == NULL)
        return false;

    if(count == -1)
        count = item->exchangeStackCount;  // stack count of offer, not of item

    if(item->exchangeStackCount <= count)
    {
        item->exchangeStackCount = 0; // take out of offering array
        item->exchangeOfferSlot = -1;
    }
    else
    {
        item->exchangeStackCount -= count;
        remain = item->exchangeStackCount;
    }
    return true;
}

bool ExchangingCharacter::MoveOfferedItem(int fromSlot,int stackcount,int toSlot)
{
    psCharacterInventory::psCharacterInventoryItem* item = chrinv->FindExchangeSlotOffered(fromSlot);
    if(item == NULL)
        return false;

    psCharacterInventory::psCharacterInventoryItem* item2 = chrinv->FindExchangeSlotOffered(toSlot);

    item->exchangeOfferSlot = toSlot;
    if(item2)
    {
        item2->exchangeOfferSlot = fromSlot;  // swap if dest slot is occupied
        return true; // true if swapped
    }
    return false;
}


psCharacter* ExchangingCharacter::GetClientCharacter(int clientNum)
{
    Client* client = psserver->GetConnections()->Find(clientNum);
    if(client == NULL)
        return NULL;

    return client->GetCharacterData();
}



void ExchangingCharacter::TransferMoney(int targetClientNum)
{
    psCharacter* target; //,*cclient;

    if(targetClientNum == 0)
        return;

    target =  GetClientCharacter(targetClientNum);

    if(target == NULL)
        return;

    TransferMoney(target);
}

void ExchangingCharacter::TransferMoney(psCharacter* target)
{
    psMoney money;
    money.Set(offeringMoney.Get(MONEY_CIRCLES),offeringMoney.Get(MONEY_OCTAS),offeringMoney.Get(MONEY_HEXAS), offeringMoney.Get(MONEY_TRIAS));

    target->AdjustMoney(money, false);

    // Now negate and take away from offerer
    money.Set(-offeringMoney.Get(MONEY_CIRCLES),-offeringMoney.Get(MONEY_OCTAS),-offeringMoney.Get(MONEY_HEXAS), -offeringMoney.Get(MONEY_TRIAS));
    chrinv->owner->AdjustMoney(money, false);

    offeringMoney.Set(0, 0, 0, 0);
}

bool ExchangingCharacter::IsOfferingSane()
{
    // Make sure offered items have valid stack counts.
    for(int i=0; i < EXCHANGE_SLOT_COUNT; i++)
    {
        psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(i);
        const psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;

        if(item && itemInSlot->exchangeStackCount > item->GetStackCount())
            return false;
    }

    //make sure offered money isn't an invalid amount (eg < 0)
    if(offeringMoney.Get(MONEY_CIRCLES) < 0 ||
            offeringMoney.Get(MONEY_OCTAS)  < 0 ||
            offeringMoney.Get(MONEY_HEXAS)  < 0 ||
            offeringMoney.Get(MONEY_TRIAS)  < 0)
    {
        return false;
    }

    //Make sure offered money is really available
    psMoney characterMoney = chrinv->owner->Money();
    if(characterMoney.Get(MONEY_CIRCLES) < offeringMoney.Get(MONEY_CIRCLES) ||
            characterMoney.Get(MONEY_OCTAS)   < offeringMoney.Get(MONEY_OCTAS)   ||
            characterMoney.Get(MONEY_HEXAS)   < offeringMoney.Get(MONEY_HEXAS)   ||
            characterMoney.Get(MONEY_TRIAS)   < offeringMoney.Get(MONEY_TRIAS))
    {
        return false;
    }

    return true;
}

void ExchangingCharacter::TransferOffer(int targetClientNum)
{
    psCharacter* target = NULL;

    if(targetClientNum != 0)
    {
        target = GetClientCharacter(targetClientNum);
        if(!target)
            return;
    }

    if(target)
    {
        for(int i=0; i < EXCHANGE_SLOT_COUNT; i++)
        {
            psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(i);
            psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;

            if(item)
            {
                CS_ASSERT(itemInSlot->exchangeStackCount <= item->GetStackCount());

                psItem* newItem = item->CreateNew();
                item->Copy(newItem);
                newItem->SetStackCount(itemInSlot->exchangeStackCount);

                if(!target->Inventory().Add(newItem))
                {
                    // TODO: Probably rollback here if anything failed
                    printf("    Could not add item to inv, so dropping on ground.\n");

                    // Drop item on ground!!
                    target->DropItem(newItem);
                }
            }
        }
        // Money is handled in TransferMoney
    }
    chrinv->PurgeOffered(); // Now delete anything we just put in the other guy's inv
}


void ExchangingCharacter::TransferOffer(psCharacter* npc)
{
    chrinv->PurgeOffered();  // npc's just destroy stuff
}

void ExchangingCharacter::AdjustMoney(int type, int delta)
{
    offeringMoney.Adjust(type, delta);
}

void ExchangingCharacter::AdjustMoney(const psMoney &amount)
{
    offeringMoney += amount;
}

void ExchangingCharacter::UpdateReceivingMoney(psMoney &money)
{
    psExchangeMoneyMsg message(client, CONTAINER_RECEIVING_MONEY,
                               money.Get(MONEY_TRIAS),
                               money.Get(MONEY_HEXAS),
                               money.Get(MONEY_CIRCLES),
                               money.Get(MONEY_OCTAS));
    message.SendMessage();
}

void ExchangingCharacter::UpdateOfferingMoney()
{
    psExchangeMoneyMsg message(client, CONTAINER_OFFERING_MONEY,
                               offeringMoney.Get(MONEY_TRIAS),
                               offeringMoney.Get(MONEY_HEXAS),
                               offeringMoney.Get(MONEY_CIRCLES),
                               offeringMoney.Get(MONEY_OCTAS));
    message.SendMessage();
}

psItem* ExchangingCharacter::GetOfferedItem(int slot)
{
    psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(slot);
    psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
    return item;
}


void ExchangingCharacter::GetOffering(csString &buff)
{
    csString moneystr = offeringMoney.ToString();
    buff.Format("<L MONEY=\"%s\">", moneystr.GetData());
    unsigned int z;

    for(z = 0; z < EXCHANGE_SLOT_COUNT; z++)
    {
        psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(z);
        psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
        if(item)
        {
            csString itemStr;
            itemStr.Format(
                "<ITEM N=\"%s\" "
                "SLT_POS=\"%d\" "
                "C=\"%d\" "
                "WT=\"%.2f\" "
                "IMG=\"%s\"/>",
                item->GetName(),
                z,
                itemInSlot->exchangeStackCount,
                0.0, //////////item->GetSumWeight(),
                item->GetImageName());

            buff.Append(itemStr);
        }
    }

    buff.Append("</L>");
}

void ExchangingCharacter::GetSimpleOffering(csString &buff, psCharacter* chr, bool exact)
{
    csString moneystr = offeringMoney.ToString();
    if(exact)
    {
        buff.Format("<l money=\"%s\">", moneystr.GetData());
    }
    else
    {
        buff.Format("<l money=\"0,0,0,%d\">", offeringMoney.GetTotal());
    }


    psStringArray xmlItems;

    for(unsigned int i = 0; i < EXCHANGE_SLOT_COUNT; i++)
    {
        psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(i);
        psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
        if(item)
        {
            xmlItems.FormatPush("<item n=\"%s\" c=\"%d\"/>",
                                item->GetBaseStats()->GetName(),
                                itemInSlot->exchangeStackCount);
        }
    }

    // We need to sort the items to match the same order as the triggers;
    // QuestManager::ParseItemList also does this.
    xmlItems.Sort();

    for(size_t i = 0; i < xmlItems.GetSize(); i++)
    {
        buff.Append(xmlItems[i]);
    }

    buff.Append("</l>");
}

/*
float ExchangingCharacter::GetSumSize()
{
    float size = 0.0;

    psItem *item = NULL;

    for (int i=0; i < EXCHANGE_SLOT_COUNT; i++)
    {
        psCharacterInventory::psCharacterInventoryItem *itemInSlot = chrinv->FindExchangeSlotOffered(i);
        psItem *item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
        if (item != NULL)
        {
            size += item->GetTotalStackSize();
        }
    }
    return size;
}


float ExchangingCharacter::GetSumWeight()
{
    float weight = 0.0;
    psItem *item = NULL;

    for (int i=0; i < EXCHANGE_SLOT_COUNT; i++)
    {
        psCharacterInventory::psCharacterInventoryItem *itemInSlot = chrinv->FindExchangeSlotOffered(i);
        psItem *item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
        if (item != NULL)
        {
            weight += item->GetWeight();
        }
    }
    return weight;
}
*/

void ExchangingCharacter::GetOfferingCSV(csString &buff)
{
    buff.Empty();
    bool first = true;
    for(int i=0; i<EXCHANGE_SLOT_COUNT; i++)
    {
        psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(i);
        psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
        if(item)
        {
            if(!first)
                buff.AppendFmt(", ");
            buff.AppendFmt("%d %s",item->GetStackCount(), item->GetName());
            if(first)
                first = false;
        }
    }
}

bool ExchangingCharacter::GetExchangedItems(csString &text)
{
    text.Clear();
    int count = 0;
    size_t last_len = 0;

    // RMS: Loop through all items offered
    for(int i=0; i < EXCHANGE_SLOT_COUNT; i++)
    {
        psCharacterInventory::psCharacterInventoryItem* itemInSlot = chrinv->FindExchangeSlotOffered(i);
        psItem* item = (itemInSlot) ? itemInSlot->GetItem() : NULL;
        if(item != NULL)
        {
            last_len = text.Length();
            count++;
            text += item->GetQuantityName(item->GetName(),itemInSlot->exchangeStackCount, item->GetCreative(), true);
            text += ", ";
            printf("text[%d] '%s'\n", i, text.GetData()); // XXX
        }
    }

    // RMS: Add the money count
    if(offeringMoney.GetTotal() > 0)
    {
        last_len = text.Length();
        count++;
        text.AppendFmt("%i Tria, ", offeringMoney.GetTotal());
    }
    if(count)
    {
        text.Truncate(text.Length() - 2);
        if(count > 1)
        {
            csString tempText = text.Slice(0, last_len - (count == 2 ? 2 : 1))
                                + " and" + text.Slice(last_len - 1);
            text = tempText;
        }
        return true;
    }

    return false;
}








/***********************************************************************************
*
*                              class Exchange
*
***********************************************************************************/

/**
 * Common base for different kinds of exchanges
 */

Exchange::Exchange(Client* starter, bool automaticExchange, ExchangeManager* manager)
    : starterChar(starter->GetClientNum(), starter->GetCharacterData()->Inventory())
{
    id = next_id++;
    exchangeEnded = false;
    exchangeSuccess = false;
    exchangeMgr = manager;
    starterClient = starter;
    this->player = starter->GetClientNum();
    this->automaticExchange = automaticExchange;

    starter->GetActor()->RegisterCallback(this);
    starter->GetCharacterData()->Inventory().BeginExchange();
}

Exchange::~Exchange()
{
    // printf("In ~Exchange()\n");

    // Must first get rid of exchange references because vtable is now deleted
    starterClient->SetExchangeID(0);
    if(!exchangeSuccess)
    {
        starterClient->GetCharacterData()->Inventory().RollbackExchange();
        psserver->GetCharManager()->SendInventory(player);
    }

    SendEnd(player);
}

bool Exchange::AddItem(Client* fromClient, INVENTORY_SLOT_NUMBER fromSlot, int stackCount, int toSlot)
{
    psCharacterInventory::psCharacterInventoryItem* invItem = fromClient->GetCharacterData()->Inventory().GetCharInventoryItem(fromSlot);
    if(!invItem)
        return false;

    SendAddItemMessage(fromClient, toSlot, invItem);

    return true;
}

void Exchange::SendAddItemMessage(Client* fromClient, int slot, psCharacterInventory::psCharacterInventoryItem* invItem)
{
    if(automaticExchange) return;

    psItem* item = invItem->GetItem();

    psExchangeAddItemMsg msg(fromClient->GetClientNum(), item->GetName(),
                             item->GetMeshName(), item->GetTextureName(), CONTAINER_EXCHANGE_OFFERING, slot, invItem->exchangeStackCount,
                             item->GetImageName(), psserver->GetCacheManager()->GetMsgStrings());

    psserver->GetEventManager()->SendMessage(msg.msg);
}

void Exchange::SendRemoveItemMessage(Client* fromClient, int slot)
{
    if(automaticExchange) return; //this is improbable to happen

    psExchangeRemoveItemMsg msg(fromClient->GetClientNum(), CONTAINER_EXCHANGE_OFFERING, slot, 0);
    psserver->GetEventManager()->SendMessage(msg.msg);
}

void Exchange::MoveItem(Client* fromClient, int fromSlot, int stackCount, int toSlot)
{
    psCharacterInventory &inv = fromClient->GetCharacterData()->Inventory();
    psCharacterInventory::psCharacterInventoryItem* srcItem  = inv.FindExchangeSlotOffered(fromSlot);
    psCharacterInventory::psCharacterInventoryItem* destItem = inv.FindExchangeSlotOffered(toSlot);

    srcItem->exchangeOfferSlot = toSlot;

    if(destItem)
        destItem->exchangeOfferSlot = fromSlot; // swap if destination slot is occupied

    SendRemoveItemMessage(fromClient, fromSlot);
    if(destItem)
    {
        SendRemoveItemMessage(fromClient, toSlot);
        SendAddItemMessage(fromClient, fromSlot, destItem);
    }
    SendAddItemMessage(fromClient, toSlot, srcItem);
}

psMoney Exchange::GetOfferedMoney(Client* client)
{
    CS_ASSERT(client == starterClient);
    return starterChar.GetOfferedMoney();
}


bool Exchange::AdjustMoney(Client* client, int moneyType, int newMoney)
{
    if(client == starterClient)
    {
        starterChar.AdjustMoney(moneyType, newMoney);
        starterChar.UpdateOfferingMoney();
        return true;
    }
    return false;
}

bool Exchange::AdjustMoney(Client* client, const psMoney &amount)
{
    if(client == starterClient)
    {
        starterChar.AdjustMoney(amount);
        starterChar.UpdateOfferingMoney();
        return true;
    }
    return false;
}

bool Exchange::RemoveItem(Client* fromClient, int fromSlot, int count)
{
    psCharacterInventory::psCharacterInventoryItem* invItem = fromClient->GetCharacterData()->Inventory().FindExchangeSlotOffered(fromSlot);
    if(!invItem)
        return false;

    if(count == -1)
        count = invItem->exchangeStackCount;  // stack count of offer, not of item

    if(invItem->exchangeStackCount <= count)
    {
        invItem->exchangeStackCount = 0; // take out of offering array
        invItem->exchangeOfferSlot = -1;
        SendRemoveItemMessage(fromClient, fromSlot);
    }
    else
    {
        invItem->exchangeStackCount -= count;
        SendRemoveItemMessage(fromClient, fromSlot);
        SendAddItemMessage(fromClient, fromSlot, invItem);
    }
    return true;
}


void Exchange::SendEnd(int clientNum)
{
    // printf("In Exchange::SendEnd(%d)\n",clientNum);
    if(automaticExchange) return;
    psExchangeEndMsg msg(clientNum);
    psserver->GetEventManager()->SendMessage(msg.msg);
}

psItem* Exchange::GetStarterOffer(int slot)
{
    if(slot < 0 || slot >= EXCHANGE_SLOT_COUNT)
        return NULL;

    return starterChar.GetOfferedItem(slot);
}

int Exchange::next_id = 1;

//------------------------------------------------------------------------------

PlayerToPlayerExchange::PlayerToPlayerExchange(Client* player, Client* target, ExchangeManager* manager)
    :Exchange(player, false, manager), targetChar(target->GetClientNum(),target->GetCharacterData()->Inventory())
{
    targetClient = target;
    this->target = target->GetClientNum();

    starterAccepted = targetAccepted = false;

    SendRequestToBoth();

    targetClient->GetActor()->RegisterCallback(this);

    target->GetCharacterData()->Inventory().BeginExchange();
}


PlayerToPlayerExchange::~PlayerToPlayerExchange()
{
    // printf("In ~PlayerToPlayerExchange()\n");
    targetClient->SetExchangeID(0);

    // In case this is set to true JUST before he disappears...
    // Hopefully this could fix the exploit reported by Edicho
    targetAccepted = false;
    starterAccepted = false;

    if(!exchangeSuccess)
    {
        targetClient->GetCharacterData()->Inventory().RollbackExchange();
        psserver->GetCharManager()->SendInventory(target);
    }

    SendEnd(target);

    starterClient->GetActor()->UnregisterCallback(this);
    targetClient->GetActor()->UnregisterCallback(this);
}

void PlayerToPlayerExchange::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    // printf("PlayerToPlayerExchange::DeleteObjectCallback(). Actor in exchange is being deleted, so exchange is ending.\n");

    // Make sure both actors are disconnected.
    starterClient->GetActor()->UnregisterCallback(this);
    targetClient->GetActor()->UnregisterCallback(this);

    HandleEnd(starterClient);
    exchangeMgr->DeleteExchange(this);
}



void PlayerToPlayerExchange::SendRequestToBoth()
{
    csString targetName(targetClient->GetName());
    psExchangeRequestMsg one(starterClient->GetClientNum(), targetName, true);
    psserver->GetEventManager()->SendMessage(one.msg);

    csString clientName(starterClient->GetName());
    psExchangeRequestMsg two(targetClient->GetClientNum(), clientName, true);
    psserver->GetEventManager()->SendMessage(two.msg);
}

Client* PlayerToPlayerExchange::GetOtherClient(Client* client) const
{
    return (client == starterClient) ? targetClient : starterClient;
}

bool PlayerToPlayerExchange::AddItem(Client* fromClient, INVENTORY_SLOT_NUMBER fromSlot, int stackCount, int toSlot)
{
    if(!Exchange::AddItem(fromClient, fromSlot, stackCount, toSlot))
        return false;

    starterAccepted = false;
    targetAccepted = false;
    SendExchangeStatusToBoth();
    return true;
}

void PlayerToPlayerExchange::SendAddItemMessage(Client* fromClient, int slot, psCharacterInventory::psCharacterInventoryItem* invItem)
{
    Exchange::SendAddItemMessage(fromClient, slot, invItem);
    psItem* item = invItem->GetItem();

    Client* toClient = GetOtherClient(fromClient);

    psExchangeAddItemMsg msg(toClient->GetClientNum(), item->GetName(),
                             item->GetMeshName(), item->GetTextureName(), CONTAINER_EXCHANGE_RECEIVING, slot, invItem->exchangeStackCount,
                             item->GetImageName(), psserver->GetCacheManager()->GetMsgStrings());

    psserver->GetEventManager()->SendMessage(msg.msg);
}

void PlayerToPlayerExchange::SendRemoveItemMessage(Client* fromClient, int slot)
{
    Exchange::SendRemoveItemMessage(fromClient, slot);

    Client* toClient = GetOtherClient(fromClient);

    psExchangeRemoveItemMsg msg(toClient->GetClientNum(), CONTAINER_EXCHANGE_RECEIVING, slot, 0);
    psserver->GetEventManager()->SendMessage(msg.msg);
}


psMoney PlayerToPlayerExchange::GetOfferedMoney(Client* client)
{
    CS_ASSERT(client == starterClient || client == targetClient);
    if(client == starterClient)
        return starterChar.GetOfferedMoney();
    else
        return targetChar.GetOfferedMoney();
}

bool PlayerToPlayerExchange::AdjustMoney(Client* client, int moneyType, int newMoney)
{
    if(Exchange::AdjustMoney(client, moneyType, newMoney))
    {
        targetChar.UpdateReceivingMoney(starterChar.offeringMoney);
    }
    else if(client == targetClient)
    {
        targetChar.AdjustMoney(moneyType, newMoney);
        targetChar.UpdateOfferingMoney();
        starterChar.UpdateReceivingMoney(targetChar.offeringMoney);
    }

    //Remove accepted state
    starterAccepted = false;
    targetAccepted = false;
    SendExchangeStatusToBoth();

    return true;
}

bool PlayerToPlayerExchange::AdjustMoney(Client* client, const psMoney &money)
{
    if(Exchange::AdjustMoney(client, money))
    {
        targetChar.UpdateReceivingMoney(starterChar.offeringMoney);
    }
    else if(client == targetClient)
    {
        targetChar.AdjustMoney(money);
        targetChar.UpdateOfferingMoney();
        starterChar.UpdateReceivingMoney(targetChar.offeringMoney);
    }

    //Remove accepted state
    starterAccepted = false;
    targetAccepted = false;
    SendExchangeStatusToBoth();

    return true;
}


bool PlayerToPlayerExchange::RemoveItem(Client* fromClient, int fromSlot, int count)
{
    if(!Exchange::RemoveItem(fromClient, fromSlot, count))
        return false;

    starterAccepted = false;
    targetAccepted = false;
    SendExchangeStatusToBoth();
    return true;
}


bool PlayerToPlayerExchange::CheckExchange(uint32_t clientNum, bool checkRange)
{
    ClientConnectionSet* clients = psserver->GetConnections();

    Client* playerClient = clients->Find(player);
    if(playerClient == NULL)
    {
        exchangeEnded = true;
        return false;
    }

    Client* targetClient = clients->Find(target);
    if(targetClient == NULL)
    {
        exchangeEnded = true;
        return false;
    }

    if(!starterChar.IsOfferingSane() || !targetChar.IsOfferingSane())
    {
        exchangeEnded = true;
        return false;
    }

    if(checkRange)
    {
        if(clientNum == player)
        {
            if(! CheckRange(player, playerClient->GetActor(), targetClient->GetActor()))
            {
                psserver->GetCharManager()->SendInventory(clientNum);
                return false;
            }
        }
        else
        {
            if(! CheckRange(target, targetClient->GetActor(), playerClient->GetActor()))
            {
                psserver->GetCharManager()->SendInventory(clientNum);
                return false;
            }
        }
    }

    return true;
}


bool PlayerToPlayerExchange::HandleAccept(Client* client)
{
    if(! CheckExchange(client->GetClientNum(), true))
        return exchangeEnded;

    if(client->GetClientNum() == player)
    {
        starterAccepted = true;
    }
    else
    {
        targetAccepted = true;
    }

    if(starterAccepted && targetAccepted)
    {
        Debug1(LOG_EXCHANGES,client->GetClientNum(),"Both Exchanged\n");
        Debug2(LOG_EXCHANGES,client->GetClientNum(),"Exchange %d have been accepted by both clients\n",id);

        ClientConnectionSet* clients = psserver->GetConnections();
        if(!clients)
        {
            return false;
        }

        const char* playerName;
        const char* targetName;
        if(player !=0)
        {
            playerName = clients->Find(player)->GetName();
        }
        else
        {
            playerName = "None";
        }
        if(target !=0)
        {
            targetName = clients->Find(target)->GetName();
        }
        else
        {
            targetName = "None";
        }

        // We must set the exchange ID to 0 so that the items will now save
        targetClient->SetExchangeID(0);
        starterClient->SetExchangeID(0);

        // Exchange logging
        csString items;
        csString buf;
        starterChar.GetOfferingCSV(items);

        if(!items.IsEmpty() || starterChar.GetOfferedMoney().GetTotal() > 0)
        {
            buf.Format("%s, %s, %s, \"%s\", %d, %u", playerName, targetName, "P2P Exchange", items.GetDataSafe(), 0, starterChar.GetOfferedMoney().GetTotal());
            psserver->GetLogCSV()->Write(CSV_EXCHANGES, buf);
        }

        targetChar.GetOfferingCSV(items);

        if(!items.IsEmpty() || targetChar.GetOfferedMoney().GetTotal() > 0)
        {
            buf.Format("%s, %s, %s, \"%s\", %d, %u", targetName, playerName, "P2P Exchange", items.GetDataSafe(), 0, targetChar.GetOfferedMoney().GetTotal());
            psserver->GetLogCSV()->Write(CSV_EXCHANGES, buf);
        }

        csString itemsOffered;

        // RMS: Output items start client gave target client
        if(starterChar.GetExchangedItems(itemsOffered))
        {
            psserver->SendSystemBaseInfo(targetClient->GetClientNum(), "%s gave %s %s.", starterClient->GetName(), targetClient->GetName(), itemsOffered.GetData());                              // RMS: Trade cancelled without items being offered
            psserver->SendSystemBaseInfo(starterClient->GetClientNum(), "%s gave %s %s.", starterClient->GetName(), targetClient->GetName(), itemsOffered.GetData());                             // RMS: Trade cancelled without items being offered
        }

        // RMS: Output items target client gave start client
        if(targetChar.GetExchangedItems(itemsOffered))
        {
            psserver->SendSystemBaseInfo(targetClient->GetClientNum(), "%s gave %s %s.", targetClient->GetName(), starterClient->GetName(), itemsOffered.GetData());                              // RMS: Trade cancelled without items being offered
            psserver->SendSystemBaseInfo(starterClient->GetClientNum(), "%s gave %s %s.", targetClient->GetName(), starterClient->GetName(), itemsOffered.GetData());                             // RMS: Trade cancelled without items being offered
        }

        starterChar.TransferOffer(target);
        starterChar.TransferMoney(target);

        targetChar.TransferOffer(player);
        targetChar.TransferMoney(player);

        psserver->SendSystemOK(targetClient->GetClientNum(),"Trade complete");
        psserver->SendSystemOK(starterClient->GetClientNum(),"Trade complete");

        psserver->GetCharManager()->UpdateItemViews(player);
        psserver->GetCharManager()->UpdateItemViews(target);

        SendEnd(player);
        SendEnd(target);

        exchangeSuccess = true;
        return true;
    }
    else
    {
        SendExchangeStatusToBoth();
    }
    return false;
}



void PlayerToPlayerExchange::HandleEnd(Client* client)
{
    printf("In P2PExch::HandleEnd\n");

    csString itemsOffered;

    // RMS: Target client
    if(starterChar.GetExchangedItems(itemsOffered))
        psserver->SendSystemResult(targetClient->GetClientNum(), "Trade declined");    // RMS: Trade cancelled without items being offered
    else
        psserver->SendSystemResult(targetClient->GetClientNum(), "Trade cancelled");   // RMS: Starter client

    if(targetChar.GetExchangedItems(itemsOffered))
        psserver->SendSystemResult(starterClient->GetClientNum(), "Trade declined");   // RMS: Trade cancelled without items being offered
    else
        psserver->SendSystemResult(starterClient->GetClientNum(), "Trade cancelled");

    targetClient->GetCharacterData()->Inventory().RollbackExchange();
    starterClient->GetCharacterData()->Inventory().RollbackExchange();

    if(client->GetClientNum() == player)
        SendEnd(target);
    else
        SendEnd(player);
}

void PlayerToPlayerExchange::SendExchangeStatusToBoth()
{
    psExchangeStatusMsg starterMsg(starterClient->GetClientNum(), starterAccepted, targetAccepted);
    psserver->GetEventManager()->SendMessage(starterMsg.msg);

    psExchangeStatusMsg targetMsg(targetClient->GetClientNum(), targetAccepted, starterAccepted);
    psserver->GetEventManager()->SendMessage(targetMsg.msg);
}

psItem* PlayerToPlayerExchange::GetTargetOffer(int slot)
{
    if(slot < 0 || slot >= EXCHANGE_SLOT_COUNT)
        return NULL;

    return targetChar.GetOfferedItem(slot);
}

/***********************************************************************************
*
*                              class NPCExchange
*
***********************************************************************************/

PlayerToNPCExchange::PlayerToNPCExchange(Client* player, gemObject* target, bool automaticExchange, int questID, ExchangeManager* manager)
    : Exchange(player, automaticExchange, manager)
{
    this->target = target;

    csString targetName(target->GetName());
    if(!automaticExchange)
    {
        psExchangeRequestMsg one(starterClient->GetClientNum(), targetName, false);
        psserver->GetEventManager()->SendMessage(one.msg);
    }

    target->RegisterCallback(this);

    this->questID = questID;

//    player->GetCharacterData()->Inventory().BeginExchange();
}

PlayerToNPCExchange::~PlayerToNPCExchange()
{
    starterClient->GetCharacterData()->Inventory().RollbackExchange();

    starterClient->GetActor()->UnregisterCallback(this);
    target->UnregisterCallback(this);
}

gemObject* PlayerToNPCExchange::GetTargetGEM()
{
    return target;
}

bool PlayerToNPCExchange::CheckExchange(uint32_t clientNum, bool checkRange)
{
    ClientConnectionSet* clients = psserver->GetConnections();

    Client* playerClient = clients->Find(player);
    if(playerClient == NULL)
    {
        exchangeEnded = true;
        return false;
    }

    gemObject* targetGEM = GetTargetGEM();
    if(targetGEM == NULL)
    {
        exchangeEnded = true;
        return false;
    }

    if(!starterChar.IsOfferingSane())
    {
        exchangeEnded = true;
        return false;
    }

    if(checkRange)
    {
        if(! CheckRange(clientNum, playerClient->GetActor(), targetGEM))
        {
            psserver->GetCharManager()->SendInventory(clientNum);
            return false;
        }
    }
    return true;
}



void PlayerToNPCExchange::HandleEnd(Client* client)
{
    client->GetCharacterData()->Inventory().RollbackExchange();
    return;
}

bool PlayerToNPCExchange::CheckXMLResponse(Client* client, psNPCDialog* dlg, csString trigger)
{
    NpcResponse* resp = dlg->FindXMLResponse(client, trigger, questID);
    if(resp && resp->type != NpcResponse::ERROR_RESPONSE)
    {
        csTicks delay = resp->ExecuteScript(client->GetActor(), (gemNPC*)target);
        if(delay != (csTicks)SIZET_NOT_FOUND)
        {
            // Give offered items to npc
            starterChar.TransferOffer(target->GetCharacterData());
            starterChar.TransferMoney(target->GetCharacterData());
            exchangeSuccess = true;
            client->GetCharacterData()->SetLastResponse(resp->id);
            client->GetCharacterData()->GetQuestMgr().SetAssignedQuestLastResponse(resp->quest,resp->id, target);
            if(resp->menu)
                resp->menu->ShowMenu(client, delay, (gemNPC*)target);
            return true;
        }
    }
    return false;
}


bool PlayerToNPCExchange::HandleAccept(Client* client)
{
    if(! CheckExchange(client->GetClientNum(), true))
        return exchangeEnded;


    // We must set the exchange ID to 0 so that the items will now save
    starterClient->SetExchangeID(0);

    bool exchange_accepted = true;

    // Send to the npc as an event trigger
    psNPCDialog* dlg = target->GetNPCDialogPtr();
    if(dlg)
    {
        csString trigger;

        // Check if NPC require the exact among of money
        starterChar.GetSimpleOffering(trigger,client->GetCharacterData(),true);
        if(!CheckXMLResponse(client,dlg,trigger))
        {
            // Check if NPC is OK with all the money total in circles
            starterChar.GetSimpleOffering(trigger,client->GetCharacterData(),false);
            if(!CheckXMLResponse(client,dlg,trigger))
            {
                psserver->SendSystemError(client->GetClientNum(), "%s does not need that.", target->GetName());
                psserver->SendSystemOK(client->GetClientNum(),"Trade Declined");

                exchange_accepted = false;
            }
        }
    }
    else
    {
        psserver->SendSystemError(client->GetClientNum(), "%s does not respond to gifts.", target->GetName());

        exchange_accepted = false;
    }

    if(exchange_accepted)
    {
        // This is redundant but harmless.  CheckXMLResponse also does this but it's easy to miss so
        // leaving here incase someone removes it.
        client->GetCharacterData()->Inventory().PurgeOffered();

        psserver->SendSystemOK(client->GetClientNum(),"Trade complete");

        psserver->GetCharManager()->SendInventory(player);
        SendEnd(player);

        // Not committing will cause rollback to do bad things instead of being harmless.
        starterClient->GetCharacterData()->Inventory().CommitExchange();

        return true;
    }
    else
    {
        // NPC didn't accept the exchange so rollback and return.
        client->GetCharacterData()->Inventory().RollbackExchange();
        psserver->GetCharManager()->SendInventory(player);
        SendEnd(player);

        return false;
    }
}

void PlayerToNPCExchange::DeleteObjectCallback(iDeleteNotificationObject* object)
{
    starterClient->GetActor()->UnregisterCallback(this);
    target->UnregisterCallback(this);

    HandleEnd(starterClient);
    exchangeMgr->DeleteExchange(this);
}

/***********************************************************************************
*
*                              class ExchangeManager
*
***********************************************************************************/

ExchangeManager::ExchangeManager(ClientConnectionSet* pClnts)
{
    clients      = pClnts;

    Subscribe(&ExchangeManager::HandleExchangeRequest, MSGTYPE_EXCHANGE_REQUEST, REQUIRE_READY_CLIENT | REQUIRE_ALIVE | REQUIRE_TARGETACTOR);
    Subscribe(&ExchangeManager::HandleExchangeAccept, MSGTYPE_EXCHANGE_ACCEPT, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&ExchangeManager::HandleExchangeEnd, MSGTYPE_EXCHANGE_END, REQUIRE_READY_CLIENT | REQUIRE_ALIVE);
    Subscribe(&ExchangeManager::HandleAutoGive, MSGTYPE_EXCHANGE_AUTOGIVE, REQUIRE_READY_CLIENT | REQUIRE_ALIVE | REQUIRE_TARGETACTOR);
}

ExchangeManager::~ExchangeManager()
{
    //do nothing
}

bool ExchangeManager::ExchangeCheck(Client* client, gemObject* target, csString* errorMessage)
{
    PSCHARACTER_MODE mode = client->GetActor()->GetMode();

    if(mode != PSCHARACTER_MODE_PEACE && mode != PSCHARACTER_MODE_SIT && mode != PSCHARACTER_MODE_OVERWEIGHT)
    {
        if(errorMessage)
        {
            errorMessage->Format("You can't trade while %s.", client->GetActor()->GetModeStr());
        }
        return false;
    }

    //don't allow frozen clients to drain all their possessions to an alt before punishment
    if(client->GetActor()->IsFrozen())
    {
        if(errorMessage)
        {
            *errorMessage = "You can't trade while being frozen.";
        }
        return false;
    }

    if(client->GetExchangeID())
    {
        if(errorMessage)
        {
            *errorMessage = "You are already busy with another trade." ;
        }
        return false;
    }

    if(!target->IsAlive())
    {
        if(errorMessage)
        {
            *errorMessage = "Cannot give items to dead things!";
        }
        return false;
    }

    // Check to make sure that the client or target are not busy with a merchant first.
    if(target->GetCharacterData()->GetTradingStatus() != psCharacter::NOT_TRADING)
    {
        if(errorMessage)
        {
            errorMessage->Format("%s is busy at the moment", target->GetName());
        }
        return false;
    }
    if(client->GetCharacterData()->GetTradingStatus() != psCharacter::NOT_TRADING)
    {
        if(errorMessage)
        {
            errorMessage->Format("You are busy with a merchant");
        }
        return false;
    }


    return true;
}


void ExchangeManager::StartExchange(Client* client, bool withPlayer, bool automaticExchange, int questID)
{
    gemObject* target = client->GetTargetObject();

    ///////////
    // Check preconditions for traiding.
    // Range check is outside TradingCheck to prevent dublicate range checks in gemNPC::SendBehaviorMessage

    // Check within select range
    if(client->GetActor()->RangeTo(target) > RANGE_TO_SELECT)
    {
        psserver->SendSystemInfo(client->GetClientNum(), "You are not in range to %s with %s.", withPlayer?"exchange":"give", target->GetName());
        return;
    }

    // Check all other preconditions
    csString errorMessage;
    if(!ExchangeCheck(client, client->GetTargetObject(),&errorMessage))
    {
        psserver->SendSystemInfo(client->GetClientNum(), errorMessage);
        return;
    }
    //TODO: Move more of the checks bellow into the ExchangeCheck!!!!

    // End of preconditons
    ///////////


    // if the command was "/give":
    if(!withPlayer)
    {
        if(!target->GetNPCPtr())
        {
            psserver->SendSystemError(client->GetClientNum(), "You can give items to NPCs only");
            return;
        }


        Exchange* exchange = new PlayerToNPCExchange(client, target, automaticExchange, questID, this);
        client->SetExchangeID(exchange->GetID());
        exchanges.Push(exchange);
    }

    // if the command was "/trade":
    else
    {
        if(target->GetNPCPtr())
        {
            psserver->SendSystemError(client->GetClientNum(), "You can trade with other players only.");
            return;
        }

        Client* targetClient = target->GetClient();
        if(targetClient == NULL)
        {
            psserver->SendSystemError(client->GetClientNum(), "Cannot find your target!");
            return;
        }

        if(client == targetClient)
        {
            psserver->SendSystemError(client->GetClientNum(),"You can not trade with yourself.");
            return;
        }

        if(targetClient->GetActor()->IsFrozen())
        {
            psserver->SendSystemInfo(client->GetClientNum(), "%s was frozen and cannot trade.", target->GetName());
            return;
        }

        if(targetClient->GetExchangeID())
        {
            psserver->SendSystemError(client->GetClientNum(), "%s is busy with another trade.", targetClient->GetName());
            return;
        }

        // Must ask the other player before starting the trade
        csString question;
        question.Format("Do you want to trade with %s ?", client->GetCharacterData()->GetCharName());
        PendingQuestion* invite = new PendingTradeInvite(
            client, targetClient, question.GetData());
        psserver->questionmanager->SendQuestion(invite);
    }
}

void ExchangeManager::HandleExchangeRequest(MsgEntry* me,Client* client)
{
    Notify2(LOG_TRADE, "Trade Requested from %u", client->GetClientNum());
    psExchangeRequestMsg msg(me);
    StartExchange(client, msg.withPlayer);
}

void ExchangeManager::HandleExchangeAccept(MsgEntry* me,Client* client)
{
    Notify3(LOG_TRADE, "Exchange %d Accept from %d", client->GetExchangeID(), client->GetClientNum());

    Exchange* exchange = GetExchange(client->GetExchangeID());
    if(exchange)
    {
        if(exchange->HandleAccept(client))
        {
            DeleteExchange(exchange);
        }
    }
}

void ExchangeManager::HandleExchangeEnd(MsgEntry* me,Client* client)
{
    Notify2(LOG_TRADE, "Trade %d Rejected", client->GetExchangeID());

    Exchange* exchange = GetExchange(client->GetExchangeID());
    if(exchange)
    {
        exchange->HandleEnd(client);
        DeleteExchange(exchange);
    }
    else
    {
        Warning2(LOG_TRADE, "Trade %d Not located", client->GetExchangeID());
    }
}

void ExchangeManager::HandleAutoGive(MsgEntry* me,Client* client)
{
    psSimpleStringMessage give(me);

    Notify2(LOG_TRADE, "Got autogive of '%s'\n", give.str.GetDataSafe());

    // Expecting xml string like: <l money="0,0,0,0"><item n="Steel Falchion" c="1"/><questid id="1234"/></l>

    csRef<iDocument> doc = ParseString(give.str);
    if(doc == NULL)
    {
        Error2("Failed to parse script %s", give.str.GetData());
        return;
    }
    csRef<iDocumentNode> root    = doc->GetRoot();
    if(!root)
    {
        Error1("No XML root in progression script");
        return;
    }
    csRef<iDocumentNode> topNode = root->GetNode("l");
    if(!topNode)
    {
        Error1("Could not find <evt> tag in progression script!");
        return;
    }

    // First check if the search has to be only for a quest or not
    csRef<iDocumentNode> questNode = topNode->GetNode("questid");
    int questIDHint = -1;
    if(questNode.IsValid())
    {
        questIDHint = questNode->GetAttributeValueAsInt("id");
        if(questIDHint < -1)
        {
            questIDHint = -1;
        }
    }

    // Create a temporary exchange to actually do all the gift giving
    StartExchange(client, false, true, questIDHint);

    // Check to make sure it worked
    Exchange* exchange = GetExchange(client->GetExchangeID());
    if(!exchange)
    {
        psserver->SendSystemError(client->GetClientNum(),"Could not give requested items.");
        return;
    }

    // Now validate that the person has all the required items here
    int exchangeSlot = 0, itemCount = 0;
    csRef<iDocumentNodeIterator> iter = topNode->GetNodes("item");
    while(iter->HasNext())
    {
        itemCount++;

        csRef<iDocumentNode> node = iter->Next();
        csString itemName  = node->GetAttributeValue("n");
        int      itemCount = node->GetAttributeValueAsInt("c");

        // Get the definition of the item from the name
        psItemStats* itemstat = psserver->GetCacheManager()->GetBasicItemStatsByName(itemName);
        if(itemstat)
        {
            // Now find that item in the player's inv from the definition
            size_t foundIndex = client->GetCharacterData()->Inventory().FindItemStatIndex(itemstat);
            if(foundIndex != SIZET_NOT_FOUND)
            {
                // Now verify that the player has exactly the right count of these items, and not more of them in another slot
                psItem* invItem = client->GetCharacterData()->Inventory().GetInventoryIndexItem(foundIndex);
                if(invItem->GetStackCount() < itemCount || invItem->GetStackCount() > itemCount || client->GetCharacterData()->Inventory().FindItemStatIndex(itemstat,foundIndex+1) != SIZET_NOT_FOUND || strcmp(invItem->GetName(), itemstat->GetName()) != 0)
                {
                    // Distinguish the cases from each other in order to send out the correct error message
                    if(invItem->GetStackCount() < itemCount)
                    {
                        psserver->SendSystemError(client->GetClientNum(), "You have too few %s.  Come back when you have the correct amount.", itemName.GetData());
                    }
                    else if(strcmp(invItem->GetName(), itemstat->GetName()) != 0)
                    {
                        psserver->SendSystemError(client->GetClientNum(), "You must give the items manually because you have different types of %s.", itemName.GetData());
                    }
                    else
                    {
                        psserver->SendSystemError(client->GetClientNum(), "You must give the items manually because you have too many %s.", itemName.GetData());
                    }
                    HandleExchangeEnd(NULL,client);
                    break;
                }
                // Finally add the item to the exchange
                client->GetCharacterData()->Inventory().SetExchangeOfferSlot(NULL, invItem->GetLocInParent(true), exchangeSlot, itemCount);
                exchange->AddItem(client,invItem->GetLocInParent(true),itemCount,exchangeSlot);
                exchangeSlot++;
            }
            else
            {
                psserver->SendSystemError(client->GetClientNum(), "You do not have required item: %s",itemName.GetData());
                HandleExchangeEnd(NULL,client);
                break;
            }
        }
        else
        {
            Error2("Could not find item stat for autogive, item name '%s'.",itemName.GetData());
            HandleExchangeEnd(NULL,client);
            break;
        }
    }
    // Now execute the exchange if all items were found
    if(exchangeSlot == itemCount)  // successfully added everything to the exchange
    {
        // Now parse the money attribute here and add any money found to the exchange
        psMoney money(topNode->GetAttributeValue("money"));
        if(money.GetTotal() > 0 && client->GetCharacterData()->Money().GetTotal() > money.GetTotal())
        {
            if(money.GetTrias())
            {
                psMoney newmoney = client->GetCharacterData()->Money();
                newmoney.EnsureTrias(money.GetTrias());
                client->GetCharacterData()->SetMoney(newmoney);
                psSlotMovementMsg trias(CONTAINER_INVENTORY_MONEY,MONEY_TRIAS,CONTAINER_EXCHANGE_OFFERING,MONEY_TRIAS,money.GetTrias());
                trias.msg->clientnum = client->GetClientNum();  // must set this before publishing
                trias.FireEvent();
            }
            if(money.GetHexas())
            {
                psMoney newmoney = client->GetCharacterData()->Money();
                newmoney.EnsureHexas(money.GetHexas());
                client->GetCharacterData()->SetMoney(newmoney);
                psSlotMovementMsg hexas(CONTAINER_INVENTORY_MONEY,MONEY_HEXAS,CONTAINER_EXCHANGE_OFFERING,MONEY_HEXAS,money.GetHexas());
                hexas.msg->clientnum = client->GetClientNum();  // must set this before publishing
                hexas.FireEvent();
            }
            if(money.GetOctas())
            {
                psMoney newmoney = client->GetCharacterData()->Money();
                newmoney.EnsureOctas(money.GetOctas());
                client->GetCharacterData()->SetMoney(newmoney);
                psSlotMovementMsg octas(CONTAINER_INVENTORY_MONEY,MONEY_OCTAS,CONTAINER_EXCHANGE_OFFERING,MONEY_OCTAS,money.GetOctas());
                octas.msg->clientnum = client->GetClientNum();  // must set this before publishing
                octas.FireEvent();
            }
            if(money.GetCircles())
            {
                psMoney newmoney = client->GetCharacterData()->Money();
                newmoney.EnsureCircles(money.GetCircles());
                client->GetCharacterData()->SetMoney(newmoney);
                psSlotMovementMsg circles(CONTAINER_INVENTORY_MONEY,MONEY_CIRCLES,CONTAINER_EXCHANGE_OFFERING,MONEY_CIRCLES,money.GetCircles());
                circles.msg->clientnum = client->GetClientNum();  // must set this before publishing
                circles.FireEvent();
            }
        }
        HandleExchangeAccept(NULL, client);
    }
}

Exchange* ExchangeManager::GetExchange(int id)
{
    // TODO:  Make this a hashmap
    for(size_t n = 0; n < exchanges.GetSize(); n++)
    {
        if(exchanges[n]->GetID() == id) return exchanges[n];
    }
    Error2("Can't find exchange: %d !!!",id);
    return NULL;
}

void ExchangeManager::DeleteExchange(Exchange* exchange)
{
    exchanges.Delete(exchange); // deletes the exchange object
}
